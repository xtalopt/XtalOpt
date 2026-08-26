/**********************************************************************
  io_results - Write the derived output files: results, hull, and movie frames.

  Copyright (C) 2009-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/xtalopt.h>

#include <xtalopt/structures/xtal.h>

#include <common/compatibility/qt_compat.h>
#include <common/fileutils.h>
#include <common/output.h>
#include <common/stringutils.h>
#include <common/timing.h>
#include <search/queuemanager.h>
#include <search/structure.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QTextStream>

#include <mutex>

using namespace Search;

namespace XtalOpt {

namespace {

// Refresh the ".old" backup, then rewrite the output file in place. The
//   file is never removed or renamed, so a "tail -f" on it keeps working.
bool writeOutputFileWithBackup(const QString& filename, const QString& contents,
                               const char* caller)
{
  if (QFile::exists(filename)) {
    const QString oldFilename = filename + ".old";
    if ((QFile::exists(oldFilename) && !QFile::remove(oldFilename)) ||
        !QFile::copy(filename, oldFilename)) {
      Common::error(QString("%1: could not write backup file %2.")
                    .arg(caller).arg(oldFilename));
      return false;
    }
  }

  QFile file(filename);
  if (!file.open(QIODevice::WriteOnly)) {
    Common::error(QString("%1: could not open file %2 for writing...")
                  .arg(caller).arg(file.fileName()));
    return false;
  }

  QTextStream out(&file);
  out << contents;

  // A failed write must report it for a retry
  out.flush();
  file.close();
  if (file.error() != QFile::NoError) {
    Common::error(QString("%1: could not write file %2.").arg(caller).arg(filename));
    return false;
  }
  return true;
}

} // namespace

void XtalOpt::queueHullSnapshot()
{
  if (isReadOnly() || getLocWorkDir().isEmpty())
    return;

  // Collect the frame data now after a fresh hull
  const QString path = Common::localPath(getLocWorkDir(), "movie");
  const unsigned long long sequence = x_hullSnapshotSequence.fetch_add(1) + 1;
  const QString filename = Common::localPath(path, Common::uniqueTimestampString(QString::number(sequence)));
  const QString contents = hullFileContents(queue()->getAllStructures());

  std::lock_guard<std::mutex> guard(x_filesNeedingSaveMutex);
  x_pendingHullSnapshots.append(QPair<QString, QString>(filename, contents));
}

bool XtalOpt::writeResultsFile(const QList<Structure*>& structures, bool notify)
{
  Common::ScopedTimer _timer("XtalOpt::writeResultsFile");
  if (getLocWorkDir().isEmpty())
    return true;

  if (!QDir().mkpath(getLocWorkDir())) {
    Common::error(QString("%1: could not create local work directory %2...")
                    .arg(__func__).arg(getLocWorkDir()));
    return false;
  }

  const QString resultsFilename = Common::localPath(getLocWorkDir(), "results.txt");
  if (notify)
    updateProgressValue(-1, tr("Saving: Writing %1...").arg(resultsFilename));

  QString contents;
  QTextStream out(&contents);
  QList<Structure*> sortedStructures(structures);
  const int objectiveOffset = (getObjectivesNum() > 0) ? getFirstUserObjectiveIndex() : 0;
  const int userObjectivesNum = getUserObjectivesNum();
  const int constraintsNum = getConstraintsNum();
  if (!sortedStructures.isEmpty()) {
    Structure::sortAndRankStructures(&sortedStructures);
    out << sortedStructures.first()->getResultsHeader(userObjectivesNum, objectiveOffset,
                                                      constraintsNum)
        << QtCompat::endl;
  }


  for (auto* structure : sortedStructures) {
    if (!structure)
      continue;
    QReadLocker structureLocker(&structure->lock());
    out << structure->getResultsEntry(userObjectivesNum, structure->getCurrentOptStep(),
                                      objectiveOffset, constraintsNum)
        << QtCompat::endl;
  }

  return writeOutputFileWithBackup(resultsFilename, contents, __func__);
}

QString XtalOpt::hullFileContents(const QList<Structure*>& structures)
{
  QString contents;
  QTextStream out(&contents);

  const QList<QString> chemSystem = getChemicalSystem();
  out << Xtal::getHullHeader(chemSystem) << "\n";

  for (auto* structure : structures) {
    Xtal* xtal = qobject_cast<Xtal*>(structure);
    if (!xtal) {
      Common::error(QString("%1: non-Xtal structure in hull file: %2")
                    .arg(__func__).arg(structure->getTag()));
      continue;
    }

    QReadLocker structureLocker(&xtal->lock());
    if (GS_ISNAN(xtal->getDistAboveHull()))
      continue;
    out << xtal->getHullEntry(chemSystem) << "\n";
  }

  const std::vector<double> refData = getReferenceEnergiesVector();
  const int refCount = static_cast<int>(refData.size()) / (chemSystem.size() + 1);
  for (int i = 0; i < refCount; ++i) {
    for (int j = 0; j < chemSystem.size(); ++j)
      out << QString(" %1")
             .arg(refData[i * (chemSystem.size() + 1) + j], 7);
    out << QString(" %1")
           .arg(refData[i * (chemSystem.size() + 1) + chemSystem.size()], 14, 'f', 6);
    out << QString("  # %1 %2 %3  %4")
           .arg("ref", 14).arg("ref", 7).arg("ref", 7).arg("ref")
        << "\n";
  }

  return contents;
}

bool XtalOpt::writeHullFile(const QList<Structure*>& structures, const QString& filename)
{
  // Make sure the destination directory exists.
  const QString hullDir = QFileInfo(filename).absolutePath();
  if (!QDir().mkpath(hullDir)) {
    Common::error(QString("%1: could not create directory %2...").arg(__func__).arg(hullDir));
    return false;
  }

  return writeOutputFileWithBackup(filename, hullFileContents(structures), __func__);
}

bool XtalOpt::saveRequestedOutputFiles(bool saveAll, bool showProgress)
{
  Common::ScopedTimer _timer("XtalOpt::saveRequestedOutputFiles");

  // Take the collected save requests and clear the markers.
  bool saveResults = saveAll;
  bool saveHull = saveAll;
  QList<QPair<QString, QString>> snapshots;
  {
    std::lock_guard<std::mutex> guard(x_filesNeedingSaveMutex);
    saveResults = saveResults || x_resultsFileNeedsSave;
    x_resultsFileNeedsSave = false;
    saveHull = saveHull || x_hullFileNeedsSave;
    x_hullFileNeedsSave = false;
    snapshots = x_pendingHullSnapshots;
    x_pendingHullSnapshots.clear();
  }

  if (!saveResults && !saveHull && snapshots.isEmpty())
    return true;

  QList<Structure*> structures = trackedStructuresSnapshot();
  if (structures.isEmpty()) {
    // Nothing for the results/hull files; queued movie frames still might be there!
    saveResults = false;
    saveHull = false;
    if (snapshots.isEmpty())
      return true;
  }

  bool resultsFailed = false;
  bool hullFailed = false;
  bool frontsChanged = false;
  QList<QPair<QString, QString>> failedSnapshots;
  {
    // One "write" each time! We pass on failed writings.
    std::lock_guard<std::mutex> saveGuard(x_outputSaveMutex);

    // Track the whole pass's duration (fronts and both files); so we can set the writing pace.
    const qint64 passStart = x_saveClock.elapsed();
    x_lastOutputWriteEndMs.store(passStart);

    // Fill in the display fronts from the latest parent selection data
    frontsChanged = applyParentSelectionFronts();

    if (saveResults && !writeResultsFile(structures, showProgress))
      resultsFailed = true;

    // Check if we have local work dir set for hull file
    if (saveHull && !getLocWorkDir().isEmpty() &&
        !writeHullFile(structures, Common::localPath(getLocWorkDir(), "hull.txt")))
      hullFailed = true;

    // Write the queued hull movie frames to disk
    for (int i = 0; i < snapshots.size(); ++i) {
      const QString& snapshotFilename = snapshots.at(i).first;
      QDir().mkpath(QFileInfo(snapshotFilename).absolutePath());
      QFile file(snapshotFilename);
      bool written = file.open(QIODevice::WriteOnly);
      if (written) {
        QTextStream out(&file);
        out << snapshots.at(i).second;
        out.flush();
        file.close();
        written = file.error() == QFile::NoError;
      }
      if (!written)
        failedSnapshots.append(snapshots.at(i));
    }

    x_lastOutputWriteMs.store(x_saveClock.elapsed() - passStart);
    x_lastOutputWriteEndMs.store(x_saveClock.elapsed());
  }

  // To update the front properly in GUI progress tab (results and hull files are fine)
  if (frontsChanged)
    emit structureViewDataChanged();

  if (!resultsFailed && !hullFailed && failedSnapshots.isEmpty())
    return true;

  // Keep only the failed writes in the list for the next retry.
  {
    std::lock_guard<std::mutex> guard(x_filesNeedingSaveMutex);
    x_resultsFileNeedsSave = x_resultsFileNeedsSave || resultsFailed;
    x_hullFileNeedsSave = x_hullFileNeedsSave || hullFailed;
    x_pendingHullSnapshots = failedSnapshots + x_pendingHullSnapshots;
  }

  (void)QMetaObject::invokeMethod(x_saveRetryTimer, "start", Qt::QueuedConnection);
  return false;
}

} // namespace XtalOpt

/**********************************************************************
  evaluate - Derived results: hull, ranks, similarity, and output files.

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

#include <common/chull.h>
#include <common/compatibility/qt_compat.h>
#include <common/constants.h>
#include <common/fileutils.h>
#include <common/output.h>
#include <common/stringutils.h>
#include <common/timing.h>
#include <search/queuemanager.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QTextStream>

#include <cmath>
#include <limits>
#include <vector>

using namespace Search;

namespace XtalOpt {

// Cast to Xtal* or flag an invalid one (to exclude from Pareto front) and
//   return null. Caller must already hold the structure's write lock.
static Xtal* xtalOrFlagged(Structure* structure, const char* context)
{
  Xtal* xtal = qobject_cast<Xtal*>(structure);
  if (!xtal) {
    Common::error(QString("%1: non-Xtal structure in XtalOpt: %2")
                    .arg(context)
                    .arg(structure->getTag()));
    structure->setParetoFront(-1);
  }
  return xtal;
}

// Refresh the ".old" backup, then rewrite the output file in place. The
//   file is never removed or renamed, so a "tail -f" on it keeps working.
static bool writeOutputFileWithBackup(const QString& filename, const QString& contents,
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

void XtalOpt::updateStructureEvaluationInfo()
{
  // Take the collected evaluation requests and clear the markers.
  QSet<Structure*> structuresToEvaluate;
  bool fullPass = false;
  {
    std::lock_guard<std::mutex> guard(x_filesNeedingSaveMutex);
    structuresToEvaluate = x_structuresNeedingEvaluation;
    x_structuresNeedingEvaluation.clear();
    fullPass = x_fullEvaluationNeeded;
    x_fullEvaluationNeeded = false;
  }
  if (structuresToEvaluate.isEmpty() && !fullPass)
    return;

  // For a new structures, above hull is first calculated using
  //   "existing on hull points". A full calculation is done only
  //   if this preliminary distance comes back zero-ish.
  bool evaluated = false;
  if (!fullPass) {
    evaluated = evaluateStructuresIncrementally(structuresToEvaluate);
    fullPass = !evaluated;
  }

  if (fullPass) {
    evaluated = refreshStructureEvaluationData();
    // Add a single hull movie frame to writing queue (if requested).
    if (evaluated && !isReadOnly() && getSaveHullSnapshots())
      queueHullSnapshot();
  } else if (evaluated) {
    // The full pass emits this itself; do the same for incremental updates.
    emit queue()->hullCalculationFinished();
  }

  // The similarity check needs the fresh distances!
  if (evaluated)
    checkForSimilarities();

  // Write the output files: this writes hull file only if data really changed.
  requestResultsFileSave(evaluated);
}

bool XtalOpt::evaluateStructuresIncrementally(const QSet<Structure*>& structures)
{
  Common::ScopedTimer _timer("XtalOpt::evaluateStructuresIncrementally");

  // No existing hull points (fresh session): a full hull calc should run.
  if (x_hullPointsCache.empty())
    return false;

  const QList<QString> chemSystem = getChemicalSystem();
  const int elementCount = chemSystem.size();
  if (elementCount == 0)
    return false;
  const int hullDimension = elementCount + 1;
  if (static_cast<int>(x_hullPointsCache.size()) % hullDimension != 0)
    return false;

  const std::vector<double> refData = getReferenceEnergiesVector();
  const int totalObjectives = getObjectivesNum();

  bool anyAssigned = false;
  for (Structure* structure : structures) {
    Xtal* xtal = qobject_cast<Xtal*>(structure);
    if (!xtal)
      continue;

    // Read this structure's hull row and objective values.
    std::vector<double> row;
    QList<double> currentValues;
    {
      QReadLocker structureLocker(&xtal->lock());
      if (!xtal->isOptimizedState())
        continue;
      // A structure that was on the hull before needs a full pass
      const double previous = xtal->getDistAboveHull();
      if (!GS_ISNAN(previous) && previous <= ZERO06)
        return false;
      row.reserve(hullDimension);
      for (int i = 0; i < elementCount; ++i)
        row.push_back(static_cast<double>(xtal->getNumberOfAtomsOfSymbol(chemSystem[i])));
      row.push_back(xtal->getEnthalpy());
      currentValues = xtal->getStrucObjValuesVec();
    }

    // The hull of "kept hull points plus this point": a positive value is true
    //   distance for the point; zero one means we need a full hull calc.
    std::vector<double> hullData(x_hullPointsCache);
    hullData.insert(hullData.end(), refData.begin(), refData.end());
    hullData.insert(hullData.end(), row.begin(), row.end());
    const int pointCount = static_cast<int>(hullData.size()) / hullDimension;
    std::vector<double> distances(pointCount);
    if (!Common::distAboveHull(hullData, pointCount, hullDimension, distances))
      return false;

    const double distance = distances.back();
    if (distance <= ZERO06)
      return false;

    // Now set the distance and the built-in objective
    QList<double> objectiveValues;
    objectiveValues.reserve(totalObjectives);
    for (int j = 0; j < totalObjectives; ++j)
      objectiveValues.append(std::numeric_limits<double>::quiet_NaN());

    bool isValid = true;
    if (currentValues.size() == totalObjectives) {
      objectiveValues = currentValues;
    } else if (currentValues.isEmpty() && !hasUserObjectives()) {
      // Built-in above-hull objective is set below.
    } else {
      Common::error(QString("%1: structure %2 has unexpected objective value count %3 "
                    "(expected %4)")
                    .arg(__func__).arg(structure->getTag())
                    .arg(currentValues.size()).arg(totalObjectives));
      isValid = false;
    }

    if (isValid && totalObjectives > 0)
      objectiveValues[getBuiltinObjectiveIndex()] = distance;

    if (isValid) {
      for (int j = 0; j < totalObjectives; ++j) {
        if (GS_ISNAN(objectiveValues[j]) || GS_ISINF(objectiveValues[j])) {
          Common::error(QString("%1: structure %2 is missing objective value %3")
                        .arg(__func__).arg(xtal->getTag()).arg(j));
          isValid = false;
          break;
        }
      }
    }

    QWriteLocker structureLocker(&xtal->lock());
    xtal->setDistAboveHull(distance);
    if (isValid)
      xtal->setStrucObjValuesVec(objectiveValues);
    else
      xtal->setParetoFront(-1);
    anyAssigned = true;
    structureLocker.unlock();

    // The structure can be added to the parent pool now with values completed.
    refreshParentPoolMembership(xtal);
  }

  // The parent pool changed; the selection table is rebuilt at the next selection round.
  if (anyAssigned)
    markParentSelectionForUpdate();

  return true;
}

bool XtalOpt::refreshStructureEvaluationData()
{
  Common::ScopedTimer _timer("XtalOpt::refreshStructureEvaluationData");
  QList<Structure*> structures = queue()->getAllStructures();
  if (structures.isEmpty())
    return false;

  const QList<QString> chemSystem = getChemicalSystem();
  const int elementCount = chemSystem.size();
  if (elementCount == 0)
    return false;

  const int hullDimension = elementCount + 1;
  QList<Structure*> optimizedStructures;

  std::vector<double> hullData;
  hullData.reserve(structures.size() * hullDimension);

  for (auto* structure : structures) {
    QReadLocker lock(&structure->lock());
    if (!structure->isOptimizedState())
      continue;

    optimizedStructures.append(structure);
    for (int i = 0; i < elementCount; ++i)
      hullData.push_back(static_cast<double>(structure->getNumberOfAtomsOfSymbol(chemSystem[i])));
    hullData.push_back(structure->getEnthalpy());
  }

  if (optimizedStructures.isEmpty())
    return false;

  const std::vector<double> refData = getReferenceEnergiesVector();
  const int refCount = static_cast<int>(refData.size()) / hullDimension;
  for (size_t i = 0; i < refData.size(); ++i)
    hullData.push_back(refData[i]);

  const int totalHullPoints = static_cast<int>(optimizedStructures.size()) + refCount;
  std::vector<double> hullDistances(totalHullPoints);
  if (!Common::distAboveHull(hullData, totalHullPoints, hullDimension, hullDistances))
    return false;

  // Store hull members' row indices for later short-hull calculations.
  x_hullPointsCache.clear();
  for (int i = 0; i < optimizedStructures.size(); ++i) {
    if (hullDistances[i] > ZERO06)
      continue;
    for (int j = 0; j < hullDimension; ++j)
      x_hullPointsCache.push_back(hullData[i * hullDimension + j]);
  }

  const int totalObjectives = getObjectivesNum();
  std::vector<int> validIndices;
  QList<QList<double>> objectiveValuesList;
  objectiveValuesList.reserve(optimizedStructures.size());

  for (int i = 0; i < optimizedStructures.size(); ++i) {
    QReadLocker lock(&optimizedStructures[i]->lock());
    QList<double> currentValues = optimizedStructures[i]->getStrucObjValuesVec();
    QList<double> objectiveValues;
    objectiveValues.reserve(totalObjectives);
    for (int j = 0; j < totalObjectives; ++j)
      objectiveValues.append(std::numeric_limits<double>::quiet_NaN());

    bool isValid = true;

    if (currentValues.size() == totalObjectives) {
      objectiveValues = currentValues;
    } else if (currentValues.isEmpty() && !hasUserObjectives()) {
      // Built-in above-hull objective is set below.
    } else {
      Common::error(QString("%1: structure %2 has unexpected objective value count %3 "
                    "(expected %4)")
              .arg(__func__)
              .arg(optimizedStructures[i]->getTag())
              .arg(currentValues.size())
              .arg(totalObjectives));
      isValid = false;
    }

    if (isValid && totalObjectives > 0)
      objectiveValues[getBuiltinObjectiveIndex()] = hullDistances[i];

    if (isValid) {
      for (int j = 0; j < totalObjectives; ++j) {
        if (GS_ISNAN(objectiveValues[j]) || GS_ISINF(objectiveValues[j])) {
          Common::error(QString("%1: structure %2 is missing objective value %3")
                  .arg(__func__)
                  .arg(optimizedStructures[i]->getTag())
                  .arg(j));
          isValid = false;
          break;
        }
      }
    }

    if (isValid) {
      validIndices.push_back(i);
      objectiveValuesList.append(objectiveValues);
    } else {
      lock.unlock();
      QWriteLocker clearFrontLock(&optimizedStructures[i]->lock());
      optimizedStructures[i]->setParetoFront(-1);
    }
  }

  for (int i = 0; i < optimizedStructures.size(); ++i) {
    QWriteLocker lock(&optimizedStructures[i]->lock());
    Xtal* xtal = xtalOrFlagged(optimizedStructures[i], __func__);
    if (!xtal)
      continue;
    xtal->setDistAboveHull(hullDistances[i]);
  }

  for (int i = 0; i < static_cast<int>(validIndices.size()); ++i) {
    const int structureIndex = validIndices[static_cast<size_t>(i)];
    QWriteLocker lock(&optimizedStructures[structureIndex]->lock());
    Xtal* xtal = xtalOrFlagged(optimizedStructures[structureIndex], __func__);
    if (!xtal)
      continue;
    optimizedStructures[structureIndex]->setStrucObjValuesVec(objectiveValuesList[i]);
  }

  // The new values may change the structures' pool eligibility.
  for (Structure* structure : optimizedStructures)
    refreshParentPoolMembership(structure);

  // The parent pool and its fitness changed; the selection table is rebuilt
  //   at the next parent selection.
  markParentSelectionForUpdate();

  emit queue()->hullCalculationFinished();
  return true;
}

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

void XtalOpt::resetSimilarities()
{
  if (!isSessionActive() || isSessionStarting() || isReadOnly()) {
    return;
  }
  x_similaritiesNeedReset.store(true);
  x_similarityCheckJob.request();
}

void XtalOpt::resetSimilarities_()
{
  x_similaritiesNeedReset.store(true);
  checkForSimilarities_();
}

void XtalOpt::checkForSimilarities()
{
  if (!isSessionActive() || isSessionStarting() || isReadOnly())
    return;

  x_similarityCheckJob.request();
}

void XtalOpt::checkForSimilarities_()
{
  Common::ScopedTimer _timer("XtalOpt::checkForSimilarities");
  QReadLocker runtimeLocker(runtimeSettingsLock());

  // A tolerance or relevant setting change requires a full re-check.
  const bool didReset = x_similaritiesNeedReset.exchange(false);
  if (didReset) {
    QList<Structure*> structures = trackedStructuresSnapshot();
    for (auto* structure : structures) {
      Xtal* xtal = qobject_cast<Xtal*>(structure);
      if (!xtal)
        continue;
      QWriteLocker xtalLocker(&xtal->lock());
      xtal->structureChanged(); // This also clears the similarity value
    }
  }

  struct SimilarityCandidate
  {
    Xtal* xtal;
    double distAboveHull;
    bool changed;
    bool loaded;
    CellComp composition;
    QList<QString> symbols;
    bool rdfReady;
  };

  // Copy the candidate data under a short read lock, then let the lock go before
  //   the long comparison below (holding it there would stop the queue thread
  //   from adding structures for the whole check). candidates is a std::vector,
  //   so it doesn't share data with the live list.
  std::vector<SimilarityCandidate> candidates;
  {
    QReadLocker trackerLocker(tracker()->rwLock());
    const QList<Structure*>* structures = tracker()->list();
    candidates.reserve(static_cast<size_t>(structures->size()));
    for (auto* structure : *structures) {
      Xtal* xtal = qobject_cast<Xtal*>(structure);
      if (!xtal)
        continue;

      QReadLocker xtalLocker(&xtal->lock());
      const double abvhull = xtal->getDistAboveHull();
      if (xtal->getStatus() != Xtal::Optimized || xtal->isSimilar() || GS_ISNAN(abvhull))
        continue;

      // Composition and symbols are checked later only for the pairs that really get compared.
      SimilarityCandidate candidate = { xtal, abvhull, xtal->hasChangedSinceSimChecked(), false, CellComp(), QList<QString>(), false };
      candidates.push_back(candidate);
    }
  }

  const bool rdfMode = getTolRdf() > 0.0 && getTolRdf() <= 1.0;

  // Prepare the saved comparison data before taking pair read locks.
  for (auto& candidate : candidates) {
    if (!rdfMode && !candidate.changed)
      continue;
    QWriteLocker xtalLocker(&candidate.xtal->lock());
    candidate.changed = candidate.changed || candidate.xtal->hasChangedSinceSimChecked();
    if (rdfMode) {
      candidate.rdfReady = candidate.xtal->calculateNormalizedRDF(getTolRdfNbins(), getTolRdfCutoff(), getTolRdfSigma());
    }
    candidate.xtal->setChangedSinceSimChecked(false);
  }

  // Only check pairs involving a changed Xtal; the rest were checked before.
  QList<Xtal*> markedXtals;
  for (size_t i = 0; i < candidates.size(); ++i) {
    if (!candidates[i].changed)
      continue;

    for (size_t j = 0; j < candidates.size(); ++j) {
      if (i == j)
        continue;
      if (candidates[j].changed && j < i)
        continue;

      // Perform a coarse energy screening to cut down on number of comparisons.
      if (fabs(candidates[i].distAboveHull - candidates[j].distAboveHull) >= 0.1)
        continue;

      if (rdfMode && (!candidates[i].rdfReady || !candidates[j].rdfReady))
        continue;

      // Now, build composition and symbols cache for as soon as we need a comparison.
      const size_t pairIndex[2] = { i, j };
      for (int p = 0; p < 2; ++p) {
        SimilarityCandidate& candidate = candidates[pairIndex[p]];
        if (candidate.loaded)
          continue;
        QReadLocker xtalLocker(&candidate.xtal->lock());
        candidate.composition = getXtalComposition(candidate.xtal);
        candidate.symbols = candidate.xtal->getSymbols();
        candidate.loaded = true;
      }

      // Xtals of different compositions are automatically excluded from similarity check
      if (compareCompositions(candidates[i].composition, candidates[j].composition) == 0)
        continue;

      Xtal* marked = checkIfSimilar(candidates[i].xtal, candidates[j].xtal,
                                    candidates[i].symbols, candidates[j].symbols);
      if (marked)
        markedXtals.append(marked);
    }
  }

  // A structure marked as similar leaves the parent pool. The refresh bumps
  //   the selection stamp, so the table is rebuilt at the next selection.
  for (Xtal* marked : markedXtals)
    refreshParentPoolMembership(marked);

  // A full re-check may also have released previously similar structures
  //   back into the pool; re-derive the whole pool for that case.
  if (didReset)
    rebuildParentPoolMembership();

  emit refreshAllStructureInfo();
}

Xtal* XtalOpt::checkIfSimilar(Xtal* a, Xtal* b, const QList<QString>& aSymbols, const QList<QString>& bSymbols)
{
  // Timed per screen-passing pair: the call count tracks how many pairs
  //   reach the real comparison.
  Common::ScopedTimer _timer("XtalOpt::checkIfSimilar");
  if (a == b)
    return nullptr;
  Xtal *kickXtal, *keepXtal;
  // Lock the lower-address structure first, so two threads locking the same
  //   pair always agree on the order (avoids a deadlock).
  QReadLocker iLocker(&(a < b ? a : b)->lock());
  QReadLocker jLocker(&(a < b ? b : a)->lock());
  if (a->hasChangedSinceSimChecked() || b->hasChangedSinceSimChecked())
    return nullptr;
  // If they are already both marked as similar, just return.
  if (a->isSimilar() && b->isSimilar()) {
    return nullptr;
  }

  // With the variable-composition search, we have the possibilities of having:
  //   (1) xtals of different composition (even those which are sub-system seeds)
  //   (2) one xtal being a supercell of the other one without explicitly marked as such.
  //
  // As of XtalOpt v14, we have two options for similarity check:
  //   (1) XtalComp, (2) RDF dot product.
  // RDF is used when its tolerance is non-zero. The default is 0.98.
  //   For RDF check, we just pass xtals as is for similarity check.
  //   For XtalComp, we first convert them to primitive cells, and compare them.

  bool theyAreSimilar = false;

  if (getTolRdf() > 0.0 && getTolRdf() <= 1.0) {
    if (aSymbols != bSymbols) {
      theyAreSimilar = false;
    } else {
      double dotprod = 0.0;
      theyAreSimilar = a->compareRDF(*b, getTolRdfNbins(), getTolRdfCutoff(),
                                     getTolRdfSigma(), getTolRdf(), dotprod);

      if (isVerbose()) {
        const QString outs = QString("   RDF dot product of %1 and %2 is %3 with tolerance %4")
                             .arg(a->getTag(), 10)
                             .arg(b->getTag(), 10)
                             .arg(dotprod, 12, 'f', 6)
                             .arg(getTolRdf());
        Common::message(outs);
      }
    }
  } else {
    Common::ScopedTimer _xcTimer("XtalOpt::xtalCompCompare");
    Atoms::Geometry xtali(*a);
    Atoms::Geometry xtalj(*b);
    xtali.reduceToPrimitive(getTolSpg());
    xtalj.reduceToPrimitive(getTolSpg());
    theyAreSimilar = xtali.compareXtalComp(xtalj, getTolXcLength(), getTolXcAngle());
  }

  if (!theyAreSimilar)
    return nullptr;

  // Mark the newest xtal as a similarity to the oldest. This keeps the
  //   lowest-energy plot trace accurate.
  // For some reason, primitive structures do not always update their
  //   indices immediately, and they remain the default "-1". So, if one
  //   of the indices is -1, set that to be the kickXtal
  if (a->getIndex() == -1) {
    kickXtal = a;
    keepXtal = b;
  } else if (b->getIndex() == -1) {
    kickXtal = b;
    keepXtal = a;
  } else if (a->getIndex() > b->getIndex()) {
    kickXtal = a;
    keepXtal = b;
  } else {
    kickXtal = b;
    keepXtal = a;
  }
  // If the kickXtal is already a similar, just return
  if (kickXtal->isSimilar()) {
    return nullptr;
  }
  // Capture what we need from keepXtal while its read lock is still held.
  const uint keepGen = keepXtal->getGeneration();
  const uint keepId = keepXtal->getIDNumber();
  // Drop both read locks before taking the write lock.
  iLocker.unlock();
  jLocker.unlock();

  // Take both locks again in the same address order used above. Either
  //   structure may have changed while the locks were released.
  const bool kickFirst = kickXtal < keepXtal;
  if (kickFirst) {
    kickXtal->lock().lockForWrite();
    keepXtal->lock().lockForRead();
  } else {
    keepXtal->lock().lockForRead();
    kickXtal->lock().lockForWrite();
  }

  Xtal* marked = nullptr;
  if (kickXtal->getStatus() == Xtal::Optimized && !kickXtal->isSimilar() &&
      keepXtal->getStatus() == Xtal::Optimized && !keepXtal->isSimilar()) {
    kickXtal->setSimilarityString(QString("%1x%2").arg(keepGen).arg(keepId));
    marked = kickXtal;
  }

  if (kickFirst) {
    keepXtal->lock().unlock();
    kickXtal->lock().unlock();
  } else {
    kickXtal->lock().unlock();
    keepXtal->lock().unlock();
  }

  return marked;
}

} // namespace XtalOpt

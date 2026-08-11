/**********************************************************************
  io_state - XtalOpt session settings save and load (state file format)

  Copyright (C) 2009-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

// As of XtalOpt v15, state files use format version 5, with two main settings groups:
//   [xtalopt/input]      all input settings, keyed by their input keyword (the
//                        scalar values and the repeatable entry arrays)
//   [xtalopt/optscheme]  per optimization step setting (templates, and assets)
//
// NOTE: the main code only understands the "current" (v5) state file type!
//   Reading an older supported version requires conversion, kept out of the
//   main handler here, implemented in xtalopt/legacy/, and called before the
//   main code's reader.

#include <xtalopt/xtalopt.h>

#include <xtalopt/legacy/input_compat.h>
#include <xtalopt/legacy/state_compat.h>
#include <xtalopt/legacy/structure_state_compat.h>
#include <xtalopt/settings.h>
#include <xtalopt/structures/xtal.h>

#include <atoms/eleminfo.h>
#include <common/constants.h>
#include <common/timing.h>
#include <common/fileutils.h>
#include <search/structure.h>
#include <search/optimizer.h>
#include <common/output.h>
#include <search/queuemanager.h>
#include <search/queueinterface.h>
#include <search/queueinterfaces/batch.h>
#include <search/tracker.h>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QReadWriteLock>
#include <QSettings>
#include <QTemporaryFile>
#include <QTextStream>
#include <QVariant>
#include <QHash>
#include <QList>
#include <QPair>
#include <QThread>

#include <algorithm>

using namespace Search;

namespace XtalOpt {

namespace {

typedef bool (*SaveSuccessCheck)(const QString& filename);

QString sharedTemporaryStateFilePath(const QString& filename)
{
  return filename + ".tmp";
}

bool sharedRemoveExistingStateFile(const QString& filename, const char* caller)
{
  if (!QFile::exists(filename))
    return true;

  if (QFile::remove(filename))
    return true;

  Common::error(QString("%1: could not remove file %2.").arg(caller).arg(filename));

  return false;
}

bool sharedReplaceStateFileWithTemp(const QString& tempFilename, const QString& filename,
                                    const char* caller)
{
  if (!QFile::exists(tempFilename)) {
    Common::error(QString("%1: temporary state file %2 does not exist.")
                    .arg(caller).arg(tempFilename));
    return false;
  }

  const QString replacedFilename = filename + ".replaced";
  const bool replacingExistingFile = QFile::exists(filename);
  if (replacingExistingFile) {
    if (!sharedRemoveExistingStateFile(replacedFilename, caller))
      return false;
    if (!QFile::rename(filename, replacedFilename)) {
      Common::error(QString("%1: could not preserve file %2 before replacing it.")
                      .arg(caller).arg(filename));
      return false;
    }
  }

  if (QFile::rename(tempFilename, filename)) {
    if (replacingExistingFile && !QFile::remove(replacedFilename)) {
      Common::warning(QString("%1: could not remove replaced state file %2.")
                        .arg(caller).arg(replacedFilename));
    }
    return true;
  }

  Common::error(QString("%1: could not replace %2 with temporary state file %3.")
                  .arg(caller).arg(filename).arg(tempFilename));
  if (replacingExistingFile && !QFile::rename(replacedFilename, filename)) {
    Common::error(QString("%1: could not restore preserved state file %2.")
                    .arg(caller).arg(filename));
  }
  return false;
}

// Check if a state file was saved successfully.
bool stateFileSaveSucceeded(const QString& filename)
{
  QSETTINGS_FILE(filename);
  return settings->value("xtalopt/saveSuccessful", false).toBool();
}

// Check if a structure state was saved successfully.
bool structureStateFileSaveSucceeded(const QString& filename)
{
  QSETTINGS_FILE(filename);
  return settings->value("structure/saveSuccessful", false).toBool();
}

// Make an empty temp file in the local temp dir and return its name. We write
//   structure.state here first (while the structure is locked), so the lock is
//   not held during the slow write to the final (often networked) location.
QString sharedScratchStateFilePath()
{
  QTemporaryFile tempFile(QDir::tempPath() + "/xtalopt-state-XXXXXX.tmp");
  tempFile.setAutoRemove(false);
  if (!tempFile.open()) {
    Common::error("Could not create a temporary state file in " + QDir::tempPath());
    return QString();
  }
  const QString name = tempFile.fileName();
  tempFile.close();
  return name;
}

// Write a state file: this checks if written file can be read back, produces
//   an ".old" backup. We never overwrite exisitng file, until we make sure
//   the new one is fine. Also, QFile::rename just copies when the new file
//   is on a different filesystem.
bool sharedFinalizeStateFileWrite(const QString& freshFilename, const QString& filename,
                                  SaveSuccessCheck saveSuccessful, const char* caller)
{
  if (!saveSuccessful(freshFilename)) {
    Common::error(QString("%1: failed to write state file for %2.").arg(caller).arg(filename));
    QFile::remove(freshFilename);
    return false;
  }

  // Only a complete existing file is backed up (won't overwrite the .old
  //    with a not correctly written file). 
  if (QFile::exists(filename) && saveSuccessful(filename)) {
    const QString backupFilename = filename + ".old";
    const QString tempBackupFilename = sharedTemporaryStateFilePath(backupFilename);
    if (!sharedRemoveExistingStateFile(tempBackupFilename, caller) ||
        !QFile::copy(filename, tempBackupFilename) ||
        !saveSuccessful(tempBackupFilename) ||
        !sharedReplaceStateFileWithTemp(tempBackupFilename, backupFilename, caller)) {
      Common::error(QString("%1: could not write backup file %2.").arg(caller).arg(backupFilename));
      QFile::remove(tempBackupFilename);
      QFile::remove(freshFilename);
      return false;
    }
  }

  if (sharedReplaceStateFileWithTemp(freshFilename, filename, caller))
    return true;

  QFile::remove(freshFilename);
  return false;
}

// Write one structured multi-entry input as an array of processed text lines,
//   one "entry" per element. The array is always written, even when empty (it
//   then records size 0), so every structured category is present in the state
//   file and an empty list is distinguishable from an absent one.
void writeStateEntryArray(QSettings& settings, const QString& arrayName,
                          const QStringList& entries)
{
  settings.remove(arrayName);
  settings.beginWriteArray(arrayName);
  for (int i = 0; i < entries.size(); ++i) {
    settings.setArrayIndex(i);
    settings.setValue("entry", entries.at(i));
  }
  settings.endArray();
}

// Write every input setting under one group, keyed by its input keyword: a
//   scalar as a single value, a repeated keyword as an "entry" array (always
//   written, size 0 when empty, so each category is present in the state file).
bool writeStateInputGroup(QSettings& settings, XtalOpt& xtalopt)
{
  settings.beginGroup("xtalopt/input");
  for (const QString& keyword : Settings::allSettingKeywords()) {
    if (Settings::hasScalarSettingBinding(keyword))
      settings.setValue(keyword, Settings::scalarSettingValue(xtalopt, keyword));
    else if (Settings::hasRepeatedSettingBinding(keyword))
      writeStateEntryArray(settings, keyword, Settings::repeatedSettingEntries(xtalopt, keyword));
  }
  settings.endGroup();
  return true;
}

QString schemeQueuePrefix(size_t optStep)
{
  return "xtalopt/optscheme/queue/" + QString::number(optStep);
}

QString schemeOptimizerPrefix(size_t optStep)
{
  return "xtalopt/optscheme/optimizer/" + QString::number(optStep);
}

void writeStateSchemeQueue(XtalOpt& xtalopt, size_t optStep,
                           const QString& settingsFilename)
{
  QueueInterface* queue = xtalopt.queueInterface(optStep);
  if (!queue) {
    Common::error(QString("%1: queue interface at opt step %2 does not exist.")
            .arg(__func__).arg(optStep + 1));
    return;
  }

  QSETTINGS_FILE(settingsFilename);
  settings->beginGroup(schemeQueuePrefix(optStep) + "/templates/" + queue->getIDString().toLower());

  QStringList filenames = queue->getQueueInterfaceTemplateFileNames();

  for (const auto& filename : filenames) {
    settings->setValue(filename, xtalopt.getQueueInterfaceTemplate(optStep,
                       filename.toStdString()).c_str());
  }

  settings->endGroup();
}

void writeStateSchemeOptimizer(XtalOpt& xtalopt, size_t optStep,
                               const QString& settingsFilename)
{
  Optimizer* optim = xtalopt.optimizer(optStep);
  if (!optim) {
    Common::error(QString("%1: optimizer at opt step %2 does not exist.")
            .arg(__func__).arg(optStep + 1));
    return;
  }

  QSETTINGS_FILE(settingsFilename);
  const QString optimizerPrefix = schemeOptimizerPrefix(optStep);
  settings->setValue(optimizerPrefix + "/directRunCommand", optim->getDirectRunCommand());
  settings->beginGroup(optimizerPrefix + "/templates/" + optim->getIDString().toLower());

  QStringList filenames = optim->getOptimizerTemplateFileNames();

  for (const auto& filename : filenames) {
    settings->setValue(filename,
      xtalopt.getOptimizerTemplate(optStep, filename.toStdString()).c_str());
  }

  settings->endGroup();

  settings->beginGroup(optimizerPrefix + "/assets/" + optim->getIDString().toLower());
  QStringList assetNames = optim->getOptimizerInputAssetNames();
  for (const auto& assetName : assetNames) {
    settings->setValue(assetName,
      xtalopt.getOptimizerInputAsset(optStep, assetName.toStdString()).c_str());
  }
  settings->endGroup();
}

bool writeStateScheme(XtalOpt& xtalopt, const QString& filename)
{
  for (size_t i = 0; i < xtalopt.getNumOptSteps(); ++i) {
    if (!xtalopt.optimizer(i) || !xtalopt.queueInterface(i)) {
      Common::error(QString("%1: optimization step %2 is incomplete.").arg(__func__).arg(i + 1));
      return false;
    }
  }

  QSETTINGS_FILE(filename);
  settings->beginGroup("xtalopt/optscheme");

  settings->setValue("numOptSteps", QString::number(xtalopt.getNumOptSteps()));
  for (size_t i = 0; i < xtalopt.getNumOptSteps(); ++i) {
    writeStateSchemeQueue(xtalopt, i, filename);
    writeStateSchemeOptimizer(xtalopt, i, filename);
    settings->setValue("optimizer/" + QString::number(i) + "/interface",
                       xtalopt.optimizer(i)->getIDString().toLower());
    settings->setValue("queue/" + QString::number(i) + "/interface",
                       xtalopt.queueInterface(i)->getIDString().toLower());
  }
  settings->endGroup();

  // Batch-queue commands: the layout here is XtalOpt's own; the key names are
  //   the queue's commands defined in engine (see BatchQueueInterface).
  for (size_t i = 0; i < xtalopt.getNumOptSteps(); ++i) {
    const auto* batch = qobject_cast<const BatchQueueInterface*>(xtalopt.queueInterface(i));
    if (!batch)
      continue;
    settings->beginGroup("xtalopt/optscheme/queue/" + QString::number(i) +
                         "/commands/" + batch->getIDString().toLower());
    settings->setValue("submit", batch->submitCommand());
    settings->setValue("status", batch->statusCommand());
    settings->setValue("cancel", batch->cancelCommand());
    settings->endGroup();
  }

  return true;
}

// Read all "entry" lines of a structured multi-entry array into a list.
QStringList readStateEntryArray(QSettings& settings, const QString& arrayName)
{
  QStringList entries;
  const int size = settings.beginReadArray(arrayName);
  for (int i = 0; i < size; ++i) {
    settings.setArrayIndex(i);
    entries.append(settings.value("entry").toString());
  }
  settings.endArray();
  return entries;
}

// Read every input setting back from the one group. Order matters: scalars
//   first, then recompute the derived data from the raw single-line inputs
//   (composition, volumes, forcedSpgs, ...), then the repeated keywords -
//   which depend on that derived data (e.g. composition) and on flags like
//   usingCustomIAD.
bool readStateInputGroup(QSettings& settings, XtalOpt& xtalopt, bool fullState)
{
  settings.beginGroup("xtalopt/input");

  // 1. Scalars (a bad value just gets a warning and is skipped).
  for (const QString& keyword : Settings::allSettingKeywords()) {
    if (!Settings::hasScalarSettingBinding(keyword) || !settings.contains(keyword))
      continue;
    const QString value = settings.value(keyword).toString();
    if (!Settings::applyScalarSetting(xtalopt, keyword, value)) {
      Common::warning(QString("Ignored invalid state value for '%1': %2")
                        .arg(keyword).arg(value));
    }
  }

  // 2. Recompute all derived data from the raw single-line scalar inputs.
  //    A state load is forgiving: validateSettings resets bad values later.
  xtalopt.rebuildDerivedSettings();

  // Exit requests are special: never reuse them when loading the state.
  xtalopt.setSoftExit(false);
  xtalopt.setHardExit(false);

  // 3. Repeated keywords (full state only): clear, then apply each "entry" line
  //    through the same settings-table handlers the input reader uses, so
  //    validation and the objective total-weight limit work exactly the same.
  if (fullState) {
    for (const QString& keyword : Settings::allSettingKeywords()) {
      if (!Settings::hasRepeatedSettingBinding(keyword))
        continue;
      const QStringList entries = readStateEntryArray(settings, keyword);
      Settings::clearRepeatedSetting(xtalopt, keyword);
      for (const QString& entry : entries) {
        if (entry.trimmed().isEmpty())
          continue;
        if (!Settings::addRepeatedSettingEntry(xtalopt, keyword, entry)) {
          settings.endGroup();
          Common::error(QString("Invalid %1 entry in state file: %2")
                          .arg(keyword, entry));
          return false;
        }
      }
    }
    // Objectives need their built-in weight refreshed after (re)loading.
    xtalopt.refreshBuiltinObjectiveWeight();
  }

  settings.endGroup();
  return true;
}

void readStateSchemeQueue(XtalOpt& xtalopt, size_t optStep, const QString& settingsFile)
{
  QueueInterface* queue = xtalopt.queueInterface(optStep);
  if (!queue) {
    Common::error(QString("%1: queue interface at opt step %2 does not exist.")
                  .arg(__func__).arg(optStep + 1));
    return;
  }

  QSETTINGS_FILE(settingsFile);

  settings->beginGroup(schemeQueuePrefix(optStep) + "/templates/" + queue->getIDString().toLower());
  QStringList filenames = queue->getQueueInterfaceTemplateFileNames();
  for (const auto& filename : filenames) {
    QString temp = settings->value(filename).toString();
    xtalopt.setQueueInterfaceTemplate(optStep, filename.toStdString(), temp.toStdString());
  }
  settings->endGroup();
}

void readStateSchemeOptimizer(XtalOpt& xtalopt, size_t optStep,
                              const QString& settingsFile)
{
  Optimizer* optim = xtalopt.optimizer(optStep);
  if (!optim) {
    Common::error(QString("%1: optimizer at opt step %2 does not exist.")
                  .arg(__func__).arg(optStep + 1));
    return;
  }

  QSETTINGS_FILE(settingsFile);

  const QString optimizerPrefix = schemeOptimizerPrefix(optStep);
  const QString directRunCommand = settings->value(optimizerPrefix + "/directRunCommand",
                    optim->getDirectRunCommand()).toString();
  optim->setDirectRunCommand(directRunCommand);
  settings->beginGroup(optimizerPrefix + "/templates/" + optim->getIDString().toLower());
  QStringList filenames = optim->getOptimizerTemplateFileNames();
  for (const auto& filename : filenames) {
    QString temp = settings->value(filename).toString();

    xtalopt.setOptimizerTemplate(optStep, filename.toStdString(), temp.toStdString());
  }
  settings->endGroup();

  settings->beginGroup(optimizerPrefix + "/assets/" + optim->getIDString().toLower());
  QStringList assetNames = optim->getOptimizerInputAssetNames();
  for (const auto& assetName : assetNames) {
    const QString value = settings->value(assetName).toString();
    xtalopt.setOptimizerInputAsset(optStep, assetName.toStdString(), value.toStdString());
  }
  settings->endGroup();
}

bool readStateScheme(XtalOpt& xtalopt, const QString& filename)
{
  QSETTINGS_FILE(filename);
  settings->beginGroup("xtalopt/optscheme");

  size_t numOptSteps = settings->value("numOptSteps",Settings::defaultValue("numOptimizationSteps")).toUInt();

  if (numOptSteps == 0)
    numOptSteps = 1;

  const QString defaultQueueInterface = Settings::defaultValue("queueInterface");
  const QString defaultOptimizer = Settings::defaultValue("optimizer");

  // Check every interface before replacing the current scheme. A bad scheme
  //   must not leave part of itself behind.
  const QStringList queueInterfaces = QueueInterface::registeredQueueInterfaces();
  const QStringList optimizers = Optimizer::registeredOptimizers();
  for (size_t i = 0; i < numOptSteps; ++i) {
    const QString queueInterface =
      settings->value("queue/" + QString::number(i) + "/interface", defaultQueueInterface).toString().trimmed();
    if (!queueInterfaces.contains(queueInterface, Qt::CaseInsensitive)) {
      Common::error(QString("Unknown queue interface at optimization step %1: %2").arg(i + 1).arg(queueInterface));
      settings->endGroup();
      return false;
    }

    const QString optimizer =
      settings->value("optimizer/" + QString::number(i) + "/interface", defaultOptimizer).toString().trimmed();
    if (!optimizers.contains(optimizer, Qt::CaseInsensitive)) {
      Common::error(QString("Unknown optimizer at optimization step %1: %2").arg(i + 1).arg(optimizer));
      settings->endGroup();
      return false;
    }
  }

  xtalopt.clearOptSteps();
  for (size_t i = 0; i < numOptSteps; ++i) {
    xtalopt.appendOptStep();

    QString queueInterface =
      settings->value("queue/" + QString::number(i) + "/interface", defaultQueueInterface).toString().trimmed().toLower();

    if (!xtalopt.setQueueInterface(i, queueInterface.toStdString())) {
      settings->endGroup();
      return false;
    }

    readStateSchemeQueue(xtalopt, i, filename);
    if (auto* batch = qobject_cast<BatchQueueInterface*>(xtalopt.queueInterface(i))) {
      QSETTINGS_FILE(filename);
      settings->beginGroup("xtalopt/optscheme/queue/" + QString::number(i) +
                           "/commands/" + batch->getIDString().toLower());
      batch->setSubmitCommand(settings->value("submit", batch->submitCommand()).toString());
      batch->setStatusCommand(settings->value("status", batch->statusCommand()).toString());
      batch->setCancelCommand(settings->value("cancel", batch->cancelCommand()).toString());
      settings->endGroup();
    }

    QString optimizerStr =
      settings->value("optimizer/" + QString::number(i) + "/interface", defaultOptimizer).toString().trimmed().toLower();

    if (!xtalopt.setOptimizer(i, optimizerStr.toStdString())) {
      settings->endGroup();
      return false;
    }

    readStateSchemeOptimizer(xtalopt, i, filename);
  }

  if (!xtalopt.anyBatchQueueInterfaces())
    xtalopt.setRemoteQueue(false);

  settings->endGroup();
  return true;
}

bool writeStructureStateGeometry(const Xtal& xtal, const QString& filename)
{
  QSETTINGS_FILE(filename);
  settings->beginGroup("structure/current");
  settings->setValue("hasEnthalpy", xtal.hasEnthalpy());
  settings->setValue("enthalpy", xtal.getEnthalpy());
  settings->setValue("energy", xtal.getEnergy());
  settings->setValue("PV", xtal.getPV());

  // Atomic numbers
  settings->remove("atomicNums");
  settings->beginWriteArray("atomicNums");
  for (size_t i = 0; i < xtal.numAtoms(); i++) {
    settings->setArrayIndex(i);
    settings->setValue("value", QString::number(xtal.atom(i).atomicNumber()));
  }
  settings->endArray();

  // Cartesian coords
  Common::Vector3 cartCoords;
  settings->remove("coords");
  settings->beginWriteArray("coords");
  for (size_t i = 0; i < xtal.numAtoms(); i++) {
    cartCoords = xtal.atom(i).pos();
    settings->setArrayIndex(i);
    settings->setValue("x", QString::number(cartCoords[0], 'g',
                       std::numeric_limits<double>::max_digits10));
    settings->setValue("y", QString::number(cartCoords[1], 'g',
                       std::numeric_limits<double>::max_digits10));
    settings->setValue("z", QString::number(cartCoords[2], 'g',
                       std::numeric_limits<double>::max_digits10));
  }
  settings->endArray();

  // Check for valid cell info before saving it (to handle future non-periodic structures!)
  settings->remove("cell");
  settings->setValue("hasCellInfo", false);
  if (xtal.is3D()) {
    settings->setValue("hasCellInfo", true);
    // Cell info
    Common::Matrix3 uc = xtal.unitCell().cellMatrix();
    settings->beginGroup("cell");
    settings->setValue("00", (uc(0, 0)));
    settings->setValue("01", (uc(0, 1)));
    settings->setValue("02", (uc(0, 2)));
    settings->setValue("10", (uc(1, 0)));
    settings->setValue("11", (uc(1, 1)));
    settings->setValue("12", (uc(1, 2)));
    settings->setValue("20", (uc(2, 0)));
    settings->setValue("21", (uc(2, 1)));
    settings->setValue("22", (uc(2, 2)));
    settings->endGroup(); // cell
  }
  settings->endGroup(); // structure/current

  settings->sync();
  return settings->status() == QSettings::NoError;
}

bool readStructureStateWorkflow(Xtal& xtal, const QString& filename)
{
  if (!structureStateFileSaveSucceeded(filename))
    return false;

  QSETTINGS_FILE(filename);
  settings->beginGroup("structure");
  {
    int loadedStatusValue = -1;
    xtal.setGeneration(settings->value("generation", 0).toInt());
    xtal.setIDNumber(settings->value("id", 0).toInt());
    xtal.setIndex(settings->value("index", 0).toInt());
    xtal.setJobID(settings->value("jobID", 0).toUInt());

    unsigned int currentOptStep = settings->value("currentOptStep", 0).toUInt();
    const int restartOptStep = settings->value("restartOptStep", -1).toInt();

    xtal.setFailCount(settings->value("failCount", 0).toInt());
    xtal.setFixCount(settings->value("fixCount", 0).toInt());
    xtal.setParents(settings->value("parents", "").toString());
    xtal.setRempath(settings->value("rempath", "").toString());
    xtal.setLocpath(settings->value("locpath", xtal.getLocpath()).toString());
    loadedStatusValue = settings->value("status", -1).toInt();

    xtal.setOptTimerStart(QDateTime::fromString(settings->value("startTime", "").toString()));
    xtal.setOptTimerEnd(QDateTime::fromString(settings->value("endTime", "").toString()));

    int size = settings->beginReadArray("copyFiles");
    xtal.clearCopyFiles();
    for (int i = 0; i < size; ++i) {
      settings->setArrayIndex(i);
      xtal.appendCopyFile(settings->value("value").toString().toStdString());
    }
    settings->endArray();

    // Objectives (multi-objective run): read current info from structure.state file
    xtal.resetStrucObj();
    xtal.setStrucObjValues(std::numeric_limits<double>::quiet_NaN());

    size = settings->beginReadArray("userObjectives");

    for (int i = 0; i < size; i++) {
      settings->setArrayIndex(i);
      const QString savedValue = settings->value("value").toString();
      bool ok = false;
      double value = savedValue.toDouble(&ok);
      if (!ok && savedValue.compare("nan", Qt::CaseInsensitive) == 0) {
        value = std::numeric_limits<double>::quiet_NaN();
        ok = true;
      }
      xtal.setStrucObjValues(ok ? value : std::numeric_limits<double>::quiet_NaN());
    }
    settings->endArray();
    xtal.setStrucObjState(Structure::ObjectivesState(
      settings->value("objectivesState", 0).toInt()));

    // Constraints: now stored separately from objectives.
    xtal.resetStrucConstraint();
    size = settings->beginReadArray("constraints");
    for (int i = 0; i < size; i++) {
      settings->setArrayIndex(i);
      xtal.setStrucConstraintValues(settings->value("value").toDouble());
    }
    settings->endArray();
    xtal.setStrucConstraintState(Structure::ConstraintState(
      settings->value("constraintsState", 0).toInt()));
    xtal.setStrucConstraintRedoCount(settings->value("constraintRedoCount", 0).toInt());
    xtal.setCurrentOptStep(currentOptStep);
    xtal.setStatus(static_cast<Structure::State>(loadedStatusValue));
    if (xtal.getStatus() == Structure::Restart)
      xtal.setRestartOptStep(restartOptStep);

    xtal.setReusePreoptBonding(settings->value("reusePreoptBonding", false).toBool());
    size = settings->beginReadArray("preoptBonds");
    std::vector<Atoms::Bond> preoptBonds;
    for (int i = 0; i < size; ++i) {
      settings->setArrayIndex(i);
      QString entry = settings->value("value").toString();
      const QStringList bondParts = entry.split(',');
      if (bondParts.size() != 2)
        continue;

      const QStringList atomAndOrder = bondParts.at(1).split(':');
      if (atomAndOrder.size() != 2)
        continue;

      size_t ind1 = bondParts.at(0).toUInt();
      size_t ind2 = atomAndOrder.at(0).toUInt();
      size_t bondOrder = atomAndOrder.at(1).toUInt();
      preoptBonds.push_back(Atoms::Bond(ind1, ind2, bondOrder));
    }
    xtal.setPreoptBonding(preoptBonds);
    settings->endArray();

    // History: read them first; then re-built using engine's appendHistoryEntry().
    settings->beginGroup("history");
    int size2;
    QList<QList<unsigned int>> histNums;
    size = settings->beginReadArray("atomicNums");
    for (int i = 0; i < size; i++) {
      settings->setArrayIndex(i);
      size2 = settings->beginReadArray(QString("atomicNums-%1").arg(i));
      QList<unsigned int> cur;
      for (int j = 0; j < size2; j++) {
        settings->setArrayIndex(j);
        cur.append(settings->value("value").toUInt());
      }
      settings->endArray();
      histNums.append(cur);
    }
    settings->endArray();

    QList<QList<Common::Vector3>> histCoords;
    size = settings->beginReadArray("coords");
    for (int i = 0; i < size; i++) {
      settings->setArrayIndex(i);
      size2 = settings->beginReadArray(QString("coords-%1").arg(i));
      QList<Common::Vector3> cur;
      for (int j = 0; j < size2; j++) {
        settings->setArrayIndex(j);
        double x = settings->value("x").toDouble();
        double y = settings->value("y").toDouble();
        double z = settings->value("z").toDouble();
        cur.append(Common::Vector3(x, y, z));
      }
      settings->endArray();
      histCoords.append(cur);
    }
    settings->endArray();

    QList<double> histEnergies;
    size = settings->beginReadArray("energies");
    for (int i = 0; i < size; i++) {
      settings->setArrayIndex(i);
      histEnergies.append(settings->value("value").toDouble());
    }
    settings->endArray();

    QList<double> histEnthalpies;
    size = settings->beginReadArray("enthalpies");
    for (int i = 0; i < size; i++) {
      settings->setArrayIndex(i);
      histEnthalpies.append(settings->value("value").toDouble());
    }
    settings->endArray();

    QList<Common::Matrix3> histCells;
    size = settings->beginReadArray("cells");
    for (int i = 0; i < size; i++) {
      settings->setArrayIndex(i);
      Common::Matrix3 cur;
      cur(0, 0) = settings->value("00").toDouble();
      cur(0, 1) = settings->value("01").toDouble();
      cur(0, 2) = settings->value("02").toDouble();
      cur(1, 0) = settings->value("10").toDouble();
      cur(1, 1) = settings->value("11").toDouble();
      cur(1, 2) = settings->value("12").toDouble();
      cur(2, 0) = settings->value("20").toDouble();
      cur(2, 1) = settings->value("21").toDouble();
      cur(2, 2) = settings->value("22").toDouble();
      histCells.append(cur);
    }
    settings->endArray();

    xtal.clearHistory();
    const int entries = histEnergies.size();
    for (int i = 0; i < entries; ++i) {
      xtal.appendHistoryEntry(i < histNums.size() ? histNums.at(i) : QList<unsigned int>(),
                              i < histCoords.size() ? histCoords.at(i) : QList<Common::Vector3>(),
                              histEnergies.at(i), i < histEnthalpies.size() ? histEnthalpies.at(i) : 0.0,
                              i < histCells.size() ? histCells.at(i) : Common::Matrix3());
    }

    settings->endGroup(); // history
  }
  settings->endGroup();

  // Crystal-specific extras in the same group.
  {
    QSETTINGS_FILE(filename);
    settings->beginGroup("structure");
    xtal.setCompositionValidity(settings->value("hasValidComposition", true).toBool());
    settings->endGroup();
  }

  return true;
}

bool readStructureStateGeometry(Xtal& xtal, const QString& filename)
{
  QSETTINGS_FILE(filename);
  settings->beginGroup("structure/current");

  // Atomic nums
  int size = settings->beginReadArray("atomicNums");
  QList<unsigned int> atomicNums;
  bool atomsAreValid = size > 0;
  for (int i = 0; i < size; i++) {
    settings->setArrayIndex(i);
    bool ok = false;
    const unsigned int atomicNum = settings->value("value").toUInt(&ok);
    if (!ok || atomicNum == 0 || atomicNum > 117)
      atomsAreValid = false;
    atomicNums.append(atomicNum);
  }
  settings->endArray();

  size = settings->beginReadArray("coords");
  QList<Common::Vector3> cartCoords;
  for (int i = 0; i < size; i++) {
    settings->setArrayIndex(i);
    bool xOk = false;
    bool yOk = false;
    bool zOk = false;
    const double x = settings->value("x").toDouble(&xOk);
    const double y = settings->value("y").toDouble(&yOk);
    const double z = settings->value("z").toDouble(&zOk);
    if (!xOk || !yOk || !zOk || GS_IS_NAN_OR_INF(x) ||
        GS_IS_NAN_OR_INF(y) || GS_IS_NAN_OR_INF(z))
      atomsAreValid = false;
    cartCoords.append(Common::Vector3(x, y, z));
  }
  settings->endArray();

  if (atomicNums.size() != cartCoords.size()) {
    Common::error(QString("Current structure info in %1 is invalid: "
                          "%2 atomic numbers but %3 coordinate entries.")
                          .arg(filename).arg(atomicNums.size()).arg(cartCoords.size()));
    settings->endGroup();
    return false;
  }

  if (!atomsAreValid) {
    Common::error(QString("Current structure atom information in %1 is invalid.").arg(filename));
    settings->endGroup();
    return false;
  }

  const bool hasEnthalpy = settings->value("hasEnthalpy", false).toBool();
  bool enthalpyOk = !hasEnthalpy;
  bool energyOk = false;
  bool pvOk = false;
  const double enthalpy = settings->value("enthalpy", 0).toDouble(&enthalpyOk);
  const double energy = settings->value("energy", 0).toDouble(&energyOk);
  const double pv = settings->value("PV", 0).toDouble(&pvOk);
  if (!enthalpyOk || !energyOk || !pvOk || GS_IS_NAN_OR_INF(enthalpy) ||
      GS_IS_NAN_OR_INF(energy) || GS_IS_NAN_OR_INF(pv)) {
    Common::error(QString("Current structure energy information in %1 is invalid.").arg(filename));
    settings->endGroup();
    return false;
  }

  const bool hasCellInfo = settings->value("hasCellInfo", false).toBool();
  Common::Matrix3 cellMatrix;
  if (hasCellInfo) {
    settings->beginGroup("cell");
    bool cellIsValid = true;
    for (size_t row = 0; row < 3; ++row) {
      for (size_t column = 0; column < 3; ++column) {
        bool ok = false;
        cellMatrix(row, column) = settings->value(QString("%1%2").arg(row).arg(column)).toDouble(&ok);
        if (!ok || GS_IS_NAN_OR_INF(cellMatrix(row, column)))
          cellIsValid = false;
      }
    }
    settings->endGroup();

    if (!cellIsValid || !Atoms::Geometry::isCellMatrixUsable(cellMatrix)) {
      Common::error(QString("Current structure cell info in %1 is invalid.")
                    .arg(filename));
      settings->endGroup();
      return false;
    }
  }

  // Set these only after the file is found ok; so a corrupt file doesn't leave
  //   the structure half-updated (new energy but old atoms).
  if (hasEnthalpy)
    xtal.setEnthalpy(enthalpy);
  xtal.setEnergy(energy);
  xtal.setPV(pv);
  if (hasCellInfo)
    xtal.setCellInfo(cellMatrix);

  // Just in case: clear atoms if they were set elsewhere for some reason ...
  xtal.clearAtoms();

  // Add the atoms
  for (int i = 0; i < atomicNums.size(); i++) {
    Atoms::Atom& newAtom = xtal.addAtom();
    newAtom.setAtomicNumber(atomicNums.at(i));
    newAtom.setPos(cartCoords.at(i));
  }
  settings->endGroup();
  return true;
}

QString readStructureStateParentTag(const QString& filename)
{
  QSETTINGS_FILE(filename);
  return settings->value("structure/parentStructure", "").toString();
}

bool findReadableStructureStateFile(const QString& primaryStateFile, QString& loadableStateFile)
{
  loadableStateFile = primaryStateFile;
  if (structureStateFileSaveSucceeded(loadableStateFile))
    return true;

  loadableStateFile = primaryStateFile + ".old";
  return structureStateFileSaveSucceeded(loadableStateFile);
}

// Read a structure state file: it is converted if an old version.
Xtal* loadStructureFromStateFile(const QString& stateFile, const QString& locPath,
                                 const QString& mainStateFile)
{
  Xtal* xtal = new Xtal();
  QWriteLocker locker(&xtal->lock());

  // An old file may already provide the current structure information while it
  //   is converted; if it doesn't, read that information here.
  bool currentInfoRead = false;
  if (!readStructureStateWorkflow(*xtal, stateFile) ||
      !Legacy::convertAndReadStructureState(*xtal, stateFile, mainStateFile, currentInfoRead) ||
      (!currentInfoRead && !readStructureStateGeometry(*xtal, stateFile))) {
    locker.unlock();
    delete xtal;
    return nullptr;
  }

  // Re-set the local work dir to where the file was actually found; instead of
  //   using the saved path. This allows a copied session to be resumed.
  xtal->setLocpath(locPath);
  return xtal;
}

void sortStructuresByStateIndex(QList<Structure*>& structures)
{
  QHash<int, int> indexCounts;
  for (int i = 0; i < structures.size(); i++) {
    const int savedIndex = structures.at(i)->getIndex();
    if (savedIndex < 0) {
      Common::warning(QString("Structure %1 has invalid saved index %2.")
                              .arg(structures.at(i)->getTag()).arg(savedIndex));
    }
    indexCounts[savedIndex]++;
  }

  for (auto it = indexCounts.constBegin(); it != indexCounts.constEnd(); ++it) {
    if (it.value() > 1) {
      Common::warning(QString("Saved structure index %1 is used %2 times.").arg(it.key()).arg(it.value()));
    }
  }

  std::stable_sort(structures.begin(), structures.end(),
                   [](const Structure* left, const Structure* right) {
                     return left->getIndex() < right->getIndex();
                   });

  for (int i = 0; i < structures.size(); i++)
    structures.at(i)->setIndex(i);
}

void restoreStructureParentLinks(const QList<QPair<Structure*, QString>>& parentRequests,
                                 const QList<Structure*>& loadedStructures)
{
  QHash<QString, Structure*> rankedParents;
  for (auto* structure : loadedStructures) {
    if (structure)
      rankedParents.insert(structure->getTag(), structure);
  }

  for (const auto& request : parentRequests) {
    Structure* structure = request.first;
    const QString& parentTag = request.second;
    if (!structure || parentTag.isEmpty())
      continue;

    const auto parent = rankedParents.constFind(parentTag);
    if (parent != rankedParents.constEnd())
      structure->setParentStructure(parent.value());
  }
}

} // namespace

QString XtalOpt::stateFilePath() const
{
  return Common::localPath(getLocWorkDir(), "xtalopt.state");
}

QStringList XtalOpt::readStructureStateDirectories(const QString& stateFile) const
{
  const QDir dataDir = QFileInfo(stateFile).absoluteDir();
  QStringList xtalDirs =
    dataDir.entryList(QStringList(), QDir::AllDirs, QDir::Size);
  xtalDirs.removeAll(".");
  xtalDirs.removeAll("..");

  for (int i = 0; i < xtalDirs.size(); ++i) {
    const QString structureStateFile =
      Common::localPath(dataDir.filePath(xtalDirs.at(i)), "structure.state");
    if (!QFile::exists(structureStateFile) &&
        !QFile::exists(structureStateFile + ".old")) {
      xtalDirs.removeAt(i);
      --i;
    }
  }

  return xtalDirs;
}

// The "needs save" markers are caught and files are written only here and in saveRequestedOutputFiles
//   saveAll=false: writes just the requested files (background calls)
//   saveAll=true:  writes all files (saveSessionState() calls)
// Failed save attempts are retried after a period of time.
bool XtalOpt::savePendingStateFiles(const QString& filename, bool saveAll, bool showProgress)
{
  Common::ScopedTimer _timer("XtalOpt::savePendingStateFiles");

  // Take the collected save requests and clear the markers.
  QSet<Structure*> structuresToSave;
  bool saveSettings = saveAll;
  {
    std::lock_guard<std::mutex> guard(x_filesNeedingSaveMutex);
    structuresToSave = x_structuresNeedingSave;
    x_structuresNeedingSave.clear();
    saveSettings = saveSettings || x_settingsStateNeedsSave;
    x_settingsStateNeedsSave = false;
  }

  QList<Structure*> structures = trackedStructuresSnapshot();

  if (saveAll) {
    structuresToSave.clear();
    for (Structure* structure : structures)
      structuresToSave.insert(structure);
  }

  if (structuresToSave.isEmpty() && !saveSettings)
    return true;

  QSet<Structure*> failedStructures;
  bool settingsFailed = false;
  {
    // One "write" each time! We pass on failed writings.
    std::lock_guard<std::mutex> saveGuard(x_stateSaveMutex);

    failedStructures = saveStructureStateFiles(structures, structuresToSave, showProgress);

    // Finally, the state file.
    if (saveSettings) {
      if (failedStructures.isEmpty())
        settingsFailed = !saveStateFile(filename);
      else
        settingsFailed = true;
    }
  }

  if (failedStructures.isEmpty() && !settingsFailed)
    return true;

  // Re-try the failed writes after a wait.
  {
    std::lock_guard<std::mutex> guard(x_filesNeedingSaveMutex);
    x_structuresNeedingSave.unite(failedStructures);
    x_settingsStateNeedsSave = x_settingsStateNeedsSave || settingsFailed;
  }
  (void)QMetaObject::invokeMethod(x_saveRetryTimer, "start", Qt::QueuedConnection);
  return false;
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

  // Re-try the failed writes after a wait.
  {
    std::lock_guard<std::mutex> guard(x_filesNeedingSaveMutex);
    x_resultsFileNeedsSave = x_resultsFileNeedsSave || resultsFailed;
    x_hullFileNeedsSave = x_hullFileNeedsSave || hullFailed;
    x_pendingHullSnapshots = failedSnapshots + x_pendingHullSnapshots;
  }
  (void)QMetaObject::invokeMethod(x_saveRetryTimer, "start", Qt::QueuedConnection);
  return false;
}

bool XtalOpt::saveSessionState(QString filename, bool notify)
{
  if (filename.isEmpty()) {
    Common::error("Cannot save XtalOpt state without an explicit filename.");
    return false;
  }

  if (isSessionStarting() || isReadOnly()) {
    Common::error(QString("%1: cannot save while search is %2.").arg(__func__).arg(isSessionStarting() ? "starting" : "read-only"));
    return false;
  }

  if (notify)
    beginProgressUpdate(tr("Saving: Writing %1...").arg(filename), 0, 0);

  const bool savedStates = savePendingStateFiles(filename, true, notify);
  const bool savedOutputs = saveRequestedOutputFiles(true, notify);

  if (notify)
    endProgressUpdate();

  return savedStates && savedOutputs;
}

QSet<Structure*> XtalOpt::saveStructureStateFiles(const QList<Search::Structure*>& structures,
                                                   const QSet<Search::Structure*>& structuresToSave,
                                                   bool showProgress)
{
  Common::ScopedTimer _timer("XtalOpt::saveStructureStateFiles");

  // Keep track of failed writes and return their list to caller.
  QSet<Structure*> failed = structuresToSave;

  for (int i = 0; i < structures.size(); ++i) {
    Structure* structure = structures.at(i);
    if (!structuresToSave.contains(structure))
      continue;

    const QString structureStateFileName =
      Common::localPath(structure->getLocpath(), "structure.state");

    // 1) write to a local scratch file.
    QString scratchFileName;
    {
      QReadLocker structureLocker(&structure->lock());
      // A structure being replaced (Empty) or having its optimizer results
      //   read in (Updating) is between two stable points and has no
      //   resumable snapshot. Keep its previous state file and leave it in
      //   the failed set; the save retry writes it once it settles.
      const Structure::State transientCheck = structure->getStatus();
      if (transientCheck == Structure::Empty ||
          transientCheck == Structure::Updating)
        continue;
      // The index is atomic. The read lock keeps the rest of the saved structure stable.
      structure->setIndex(i);
      Xtal* xtal = qobject_cast<Xtal*>(structure);
      if (!xtal) {
        failed.remove(structure);
        continue;
      }
      scratchFileName = sharedScratchStateFilePath();
      if (scratchFileName.isEmpty()) {
        return failed;
      }
      writeStructureStateFile(*xtal, scratchFileName);
    }

    // 2) save file in final location.
    if (showProgress) {
      updateProgressValue(-1, tr("Saving: Writing %1...").arg(structureStateFileName));
    }
    if (sharedFinalizeStateFileWrite(scratchFileName, structureStateFileName,
                                     structureStateFileSaveSucceeded, __func__))
      failed.remove(structure);
  }

  return failed;
}

// Write all current settings groups to filename.
// Callers write into a fresh file, so no old-format entry cleanup is needed.
// NOTE: this is not for writing structure.state, hull output, or the
//   saveSuccessful flag (the caller does that).
bool XtalOpt::writeStateFileContents(const QString& filename)
{
  if (filename.isEmpty()) {
    Common::error("Cannot write the XtalOpt state file without an explicit filename.");
    return false;
  }

  // Take one copy of settings; a runtime/GUI edit must not overlap the write.
  QReadLocker runtimeLocker(runtimeSettingsLock());

  {
    QSETTINGS_FILE(filename);
    settings->setValue("xtalopt/version", CurrentStateSchemaVersion);
    if (!writeStateInputGroup(*settings, *this))
      return false;
  }
  return writeStateScheme(*this, filename);
}

bool XtalOpt::writeFreshStateFile(const QString& filename)
{
  if (filename.isEmpty()) {
    Common::error("Cannot write the XtalOpt state file without an explicit filename.");
    return false;
  }

  // Actual .state saves write using an empty temporary file first.
  // This keeps the writer away from any obsolete keys that may exist in
  //   the previous on-disk file (so, compatibility is purely a reading step).
  if (!sharedRemoveExistingStateFile(filename, __func__))
    return false;

  // Start by saveSuccessful=false; so we won't keep a corrupted written file.
  {
    QSETTINGS_FILE(filename);
    settings->setValue("xtalopt/version", CurrentStateSchemaVersion);
    settings->setValue("xtalopt/saveSuccessful", false);
  }

  if (!writeStateFileContents(filename))
    return false;

  {
    QSETTINGS_FILE(filename);
    settings->setValue("xtalopt/saveSuccessful", true);
    settings->sync();
    if (settings->status() != QSettings::NoError) {
      Common::error(QString("%1: failed to sync state file %2.").arg(__func__).arg(filename));
      return false;
    }
  }

  return true;
}

bool XtalOpt::saveStateFile(const QString& filename)
{
  Common::ScopedTimer _timer("XtalOpt::saveStateFile");
  if (filename.isEmpty()) {
    Common::error("Cannot save the XtalOpt state file without an explicit filename.");
    return false;
  }

  const QString tempFilename = sharedTemporaryStateFilePath(filename);
  if (!writeFreshStateFile(tempFilename)) {
    QFile::remove(tempFilename);
    return false;
  }

  return sharedFinalizeStateFileWrite(tempFilename, filename, stateFileSaveSucceeded, __func__);
}

bool XtalOpt::saveSchemeFile(const QString& filename)
{
  if (filename.isEmpty()) {
    Common::error("Cannot write scheme without an explicit filename.");
    return false;
  }

  // Take one copy of settings; a runtime/GUI edit must not overlap the write.
  QReadLocker runtimeLocker(runtimeSettingsLock());

  const QString tempFilename = sharedTemporaryStateFilePath(filename);
  if (!sharedRemoveExistingStateFile(tempFilename, __func__))
    return false;
  // Write the state file version, exactly as writeStateFileContents() does for
  //   the full state file: a "scheme" file is just the optscheme subset of
  //   full state, so the legacy layer can properly handle these files and
  //   convert them if needed.
  {
    QSETTINGS_FILE(tempFilename);
    settings->setValue("xtalopt/version", CurrentStateSchemaVersion);
  }
  if (!writeStateScheme(*this, tempFilename)) {
    QFile::remove(tempFilename);
    return false;
  }
  return sharedReplaceStateFileWithTemp(tempFilename, filename, __func__);
}

// Write the current structure information.
void writeStructureStateFile(Xtal& xtal, const QString& filename)
{
  QSETTINGS_FILE(filename);
  settings->beginGroup("structure");
  // Queue/resume-related entries; QueueManager handles the normal runtime
  //   parameters.
  settings->setValue("saveSuccessful", false);
  settings->setValue("version", CurrentStateSchemaVersion);
  settings->setValue("generation", xtal.getGeneration());
  settings->setValue("id", xtal.getIDNumber());
  settings->setValue("index", xtal.getIndex());
  settings->setValue("jobID", xtal.getJobID());
  settings->setValue("currentOptStep", xtal.getCurrentOptStep());
  settings->setValue("restartOptStep", xtal.getRestartOptStep());
  settings->setValue("parents", xtal.getParents());
  settings->setValue("rempath", xtal.getRempath());
  settings->setValue("locpath", xtal.getLocpath());
  settings->setValue("status", int(xtal.getStatus()));
  settings->setValue("failCount", xtal.getFailCount());
  settings->setValue("fixCount", xtal.getFixCount());
  settings->setValue("startTime", xtal.getOptTimerStart().toString());
  settings->setValue("endTime", xtal.getOptTimerEnd().toString());
  settings->remove("copyFiles");
  settings->beginWriteArray("copyFiles");
  const std::vector<std::string> copyFiles = xtal.copyFiles();
  for (size_t i = 0; i < copyFiles.size(); ++i) {
    settings->setArrayIndex(i);
    settings->setValue("value", copyFiles[i].c_str());
  }
  settings->endArray();

  // Objectives (multi-objective run): write current info to structure.state file
  settings->remove("objectives");
  settings->remove("userObjectives");
  settings->beginWriteArray("userObjectives");
  for (int i = XtalOpt::getFirstUserObjectiveIndex(); i < xtal.getStrucObjNumber(); i++) {
    settings->setArrayIndex(i - XtalOpt::getFirstUserObjectiveIndex());
    settings->setValue("value", QString::number(xtal.getStrucObjValues(i), 'g',
                       std::numeric_limits<double>::max_digits10));
  }
  settings->endArray();
  settings->setValue("objectivesState", xtal.getStrucObjState());

  // Constraints: now handled separately from objectives.
  settings->remove("constraints");
  settings->beginWriteArray("constraints");
  for (int i = 0; i < xtal.getStrucConstraintNumber(); i++) {
    settings->setArrayIndex(i);
    settings->setValue("value", QString::number(xtal.getStrucConstraintValues(i), 'g',
                       std::numeric_limits<double>::max_digits10));
  }
  settings->endArray();
  settings->setValue("constraintsState", xtal.getStrucConstraintState());
  settings->setValue("constraintRedoCount", xtal.getStrucConstraintRedoCount());

  settings->setValue("reusePreoptBonding", xtal.reusePreoptBonding());
  settings->remove("preoptBonds");
  settings->beginWriteArray("preoptBonds");
  const std::vector<Atoms::Bond>& preoptBonds = xtal.getPreoptBonding();
  for (size_t i = 0; i < preoptBonds.size(); ++i) {
    settings->setArrayIndex(i);
    QString entry = QString::number(preoptBonds[i].first()) + "," +
                    QString::number(preoptBonds[i].second()) + ":" +
                    QString::number(preoptBonds[i].bondOrder());
    settings->setValue("value", entry);
  }
  settings->endArray();

  // Parent pointer saved by tag; start by removing existing one first so
  //   if a structure -for any reason- doesn't have a parent now it won't
  //   keep a wrong link.
  settings->remove("parentStructure");
  if (xtal.hasParentStructure()) {
    QString parentStructure = xtal.getParentStructure()->getTag();
    settings->setValue("parentStructure", parentStructure);
  }

  // History: entries are obtained from the engine's retrieveHistoryEntry().
  const unsigned int historySize = xtal.sizeOfHistory();
  QList<QList<unsigned int>> histNums;
  QList<QList<Common::Vector3>> histCoords;
  QList<double> histEnergies;
  QList<double> histEnthalpies;
  QList<Common::Matrix3> histCells;
  for (unsigned int i = 0; i < historySize; ++i) {
    QList<unsigned int> nums;
    QList<Common::Vector3> coords;
    double energy = 0.0;
    double enthalpy = 0.0;
    Common::Matrix3 cell;
    xtal.retrieveHistoryEntry(i, &nums, &coords, &energy, &enthalpy, &cell);
    histNums.append(nums);
    histCoords.append(coords);
    histEnergies.append(energy);
    histEnthalpies.append(enthalpy);
    histCells.append(cell);
  }

  settings->beginGroup("history");
  // Atomic nums
  settings->remove("atomicNums");
  settings->beginWriteArray("atomicNums");
  for (int i = 0; i < histNums.size(); i++) {
    settings->setArrayIndex(i);
    const QList<unsigned int>* ptr = &(histNums.at(i));
    settings->beginWriteArray(QString("atomicNums-%1").arg(i));
    for (int j = 0; j < ptr->size(); j++) {
      settings->setArrayIndex(j);
      settings->setValue("value", ptr->at(j));
    }
    settings->endArray();
  }
  settings->endArray();

  // Coords
  settings->remove("coords");
  settings->beginWriteArray("coords");
  for (int i = 0; i < histCoords.size(); i++) {
    settings->setArrayIndex(i);
    const QList<Common::Vector3>* ptr = &(histCoords.at(i));
    settings->beginWriteArray(QString("coords-%1").arg(i));
    for (int j = 0; j < ptr->size(); j++) {
      settings->setArrayIndex(j);
      settings->setValue("x", ptr->at(j).x());
      settings->setValue("y", ptr->at(j).y());
      settings->setValue("z", ptr->at(j).z());
    }
    settings->endArray();
  }
  settings->endArray();

  // Energies
  settings->remove("energies");
  settings->beginWriteArray("energies");
  for (int i = 0; i < histEnergies.size(); i++) {
    settings->setArrayIndex(i);
    settings->setValue("value", histEnergies.at(i));
  }
  settings->endArray();

  // Enthalpies
  settings->remove("enthalpies");
  settings->beginWriteArray("enthalpies");
  for (int i = 0; i < histEnthalpies.size(); i++) {
    settings->setArrayIndex(i);
    settings->setValue("value", histEnthalpies.at(i));
  }
  settings->endArray();

  // Cells
  settings->remove("cells");
  settings->beginWriteArray("cells");
  for (int i = 0; i < histCells.size(); i++) {
    settings->setArrayIndex(i);
    const Common::Matrix3* ptr = &histCells.at(i);
    settings->setValue("00", (*ptr)(0, 0));
    settings->setValue("01", (*ptr)(0, 1));
    settings->setValue("02", (*ptr)(0, 2));
    settings->setValue("10", (*ptr)(1, 0));
    settings->setValue("11", (*ptr)(1, 1));
    settings->setValue("12", (*ptr)(1, 2));
    settings->setValue("20", (*ptr)(2, 0));
    settings->setValue("21", (*ptr)(2, 1));
    settings->setValue("22", (*ptr)(2, 2));
  }
  settings->endArray();

  settings->endGroup(); // history
  settings->endGroup(); // structure

  // Crystal-specific extras in the same group.
  settings->beginGroup("structure");
  settings->setValue("hasValidComposition", xtal.hasValidComposition());
  settings->endGroup();

  // The success status comes last, and that is only if everything before it were written correctly. 
  settings->sync();
  if (settings->status() != QSettings::NoError)
    return;
  if (!writeStructureStateGeometry(xtal, filename))
    return;
  settings->setValue("structure/saveSuccessful", true);
  settings->sync();
}

bool XtalOpt::restorePopulation(const QString& stateFile,
                                const QStringList& xtalDirs)
{
  Common::ScopedTimer _timer("XtalOpt::restorePopulation");
  const bool readOnlyLoad = isReadOnly();
  QFileInfo stateInfo(stateFile);
  QDir dataDir = stateInfo.absoluteDir();

  Common::message("\n");

  if (readOnlyLoad)
    Common::message(QString("%1 xtals were found!").arg(xtalDirs.size()));

  // Xtals
  if (!readOnlyLoad)
    updateProgressValue(-1, QString(), -1, xtalDirs.size());

  // Load xtals
  QList<Structure*> loadedStructures;
  QList<QPair<Structure*, QString>> parentRequests;
  bool errorMsgAlreadyGiven = false;
  QThread* restoredThread = readOnlyLoad ? nullptr : tracker()->thread();

  for (int i = 0; i < xtalDirs.size(); i++) {
    const QString xtalPath = Common::localPath(dataDir.absolutePath(), xtalDirs.at(i));
    if (readOnlyLoad) {
      Common::message(QString("Loading xtal %1...").arg(i + 1));
    } else {
      updateProgressValue(i, tr("Loading structures(%1 of %2)...").arg(i + 1).arg(xtalDirs.size()));
    }

    QString xtalStateFileName = Common::localPath(xtalPath, "structure.state");
    Common::message(tr("Loading structure %1").arg(xtalStateFileName));

    // The error message is given by a pop-up
    QString errorMsg = readOnlyLoad
        ? tr("Failed to load some structures: ignoring them (check output for details)")
        : tr("Failed to load some structures: will overwrite them if search resumes (check output for details)");

    // The warning message is given in the log
    QString warningMsg = tr("structure.state file was not saved successfully for %1."
                            " This structure will be excluded.").arg(xtalPath);

    if (!findReadableStructureStateFile(xtalStateFileName, xtalStateFileName)) {
      if (!errorMsgAlreadyGiven) {
        Common::error(errorMsg);
        errorMsgAlreadyGiven = true;
      }
      Common::warning(warningMsg);
      continue;
    }

    Xtal* xtal = loadStructureFromStateFile(xtalStateFileName, xtalPath, stateFile);
    if (!xtal && !xtalStateFileName.endsWith(".old") &&
        structureStateFileSaveSucceeded(xtalStateFileName + ".old")) {
      xtalStateFileName += ".old";
      xtal = loadStructureFromStateFile(xtalStateFileName, xtalPath, stateFile);
    }
    if (!xtal) {
      if (!errorMsgAlreadyGiven) {
        Common::error(errorMsg);
        errorMsgAlreadyGiven = true;
      }
      Common::warning(warningMsg);
      continue;
    }

    // Setup that only happens once, after the structure is read.
    {
      QWriteLocker locker(&xtal->lock());
      if (restoredThread) {
        xtal->moveToThread(restoredThread);
        xtal->setupConnections();
      }
      xtal->findSpaceGroup(getTolSpg());

      // Re-use saved status/timer to keep the geometry refresh consistent.
      Xtal::State state = xtal->getStatus();
      QDateTime endtime = xtal->getOptTimerEnd();
      // This is just to make sure that timers for in-process structures
      //   does not tick in a read-only mode.
      if (readOnlyLoad && !Structure::isQueueTerminalState(state) &&
          !xtal->getOptTimerStart().isNull() && endtime.isNull()) {
        const QDateTime savedTime = QFileInfo(xtalStateFileName).lastModified();
        if (savedTime.isValid() && xtal->getOptTimerStart().secsTo(savedTime) >= 0)
          endtime = savedTime;
      }
      xtal->setStatus(state);
      xtal->setOptTimerEnd(endtime);
    }

    if (xtal->getCurrentOptStep() >= getNumOptSteps()) {
      if (!errorMsgAlreadyGiven) {
        Common::error(errorMsg);
        errorMsgAlreadyGiven = true;
      }
      Common::warning(
        tr("Structure %1 uses saved optimization step %2, but only %3 steps "
           "are available. This structure will be excluded.")
           .arg(xtalPath).arg(xtal->getCurrentOptStep() + 1).arg(static_cast<qulonglong>(getNumOptSteps())));
      Tracker::deleteStructure(xtal);
      continue;
    }

    QString parentStructureString = readStructureStateParentTag(xtalStateFileName);
    if (!parentStructureString.isEmpty())
      parentRequests.append(qMakePair(qobject_cast<Structure*>(xtal),
                                      parentStructureString));

    if (!readOnlyLoad) {
      QWriteLocker locker(&xtal->lock());
      Xtal::State state = xtal->getStatus();
      // An interrupted optimizer-results import: the job itself had finished,
      //   so treat the structure as in process and let the usual queue status
      //   check decide (direct/local runs collapse to a restart below).
      if (state == Structure::Updating)
        state = Structure::InProcess;
      // An interrupted replacement: the file still holds a complete structure
      //   (the old one or its replacement), so just restart it.
      if (state == Structure::Empty)
        state = Structure::Restart;
      // Direct/local jobs finish with the process, so restart them and clear
      //   the process job IDs. Remote batch jobs are fine, and will be
      //   checked later (reconnected through queue status check).
      QueueInterface* structureQueue = queueInterface(xtal->getCurrentOptStep());
      if (!(structureQueue && structureQueue->isBatchQueue())) {
        if (state == Structure::InProcess || state == Structure::Submitted) {
          state = Structure::Restart;
        }
        xtal->setJobID(0);
      }
      // Objective/constraint script launches are basically "direct" runs; too.
      if (state == Structure::ConstraintCalculation) {
        state = Structure::Postprocessing;
        xtal->resetStrucConstraint();
      }
      if (state == Structure::ObjectiveCalculation) {
        state = Structure::Postprocessing;
        xtal->resetStrucObj();
      }
      QDateTime endtime = xtal->getOptTimerEnd();

      xtal->setStatus(state);
      xtal->setOptTimerEnd(endtime);
    }

    loadedStructures.append(qobject_cast<Structure*>(xtal));
  }

  // If no structures were loaded successfully, report restore failure.
  if (loadedStructures.size() == 0) {
    if (readOnlyLoad) {
      Common::error(QString("No structures were loaded successfully in %1. "
                            "Please check your data directory and try again.")
                            .arg(dataDir.absolutePath()));
    } else {
      Common::error(tr("No structures were loaded successfully."));
    }
    return false;
  }

  if (!readOnlyLoad) {
    updateProgressValue(0, "Updating structure indices...", 0, loadedStructures.size());
  }

  sortStructuresByStateIndex(loadedStructures);
  // Parent links are saved by tag; rebuild them once all structures are loaded.
  restoreStructureParentLinks(parentRequests, loadedStructures);

  if (!readOnlyLoad) {
    updateProgressValue(0, "Preparing tracker state...", 0, loadedStructures.size());
  } else {
    Common::message("Preparing GUI...");
  }

  Structure* s = 0;
  emit structureViewUpdateBlocked(true);

  for (int i = 0; i < loadedStructures.size(); i++) {
    s = loadedStructures.at(i);
    if (readOnlyLoad) {
      Common::message(QString("Loading xtal %1 into the GUI...").arg(i + 1));
    } else {
      updateProgressValue(i);
    }

    addRestoredStructure(s, !readOnlyLoad);
  }

  // Refresh hull-derived values after rebuilding memory with the current loaded data.
  QList<Structure*> structures = queue()->getAllStructures();
  const bool refreshed = refreshStructureEvaluationData();
  resetSimilarities_();
  // Generate the parent pool (including fronts) from the loaded structures
  rebuildParentPoolMembership();
  refreshParentSelectionFronts(getAllParentPoolStructures());
  if (!refreshed)
    Structure::sortAndRankStructures(&structures);

  emit structureViewUpdateBlocked(false);

  emit structureViewDataChanged();

  if (!readOnlyLoad)
    updateProgressValue(-1, "Done!");

  // Set this up to prevent a bug if "replace with random" is the failure
  //   action and "Initialize with RandSpg" is checked.
  if (!readOnlyLoad && minXtalsOfSpg().empty()) {
    for (size_t spg = 1; spg <= 230; spg++)
      minXtalsOfSpg().append(0);
  }

  return true;
}

bool XtalOpt::readStateFile(const QString& filename, bool fullState,
                            bool* stateWasConverted)
{
  if (stateWasConverted)
    *stateWasConverted = false;

  if (filename.isEmpty()) {
    Common::error("Cannot read the XtalOpt state file without an explicit filename.");
    return false;
  }

  QString readFilename;
  const bool keepCompatibilityCopy = !isReadOnly();
  if (!Legacy::convertStateFile(filename, fullState, keepCompatibilityCopy,
                                readFilename))
    return false;

  if (stateWasConverted)
    *stateWasConverted = readFilename != filename;

  bool readOk = true;
  {
    QSETTINGS_FILE(readFilename);
    if (!readStateInputGroup(*settings, *this, fullState))
      readOk = false;
  }
  if (readOk && !readStateScheme(*this, readFilename))
    readOk = false;

  if (!keepCompatibilityCopy && readFilename != filename &&
      !QFile::remove(readFilename)) {
    Common::warning(QString("Could not remove compatibility state copy %1.")
                      .arg(readFilename));
  }

  if (!readOk)
    return false;

  // Check all settings. For a loaded session, reset wrong values and warn.
  Settings::validateSettings(*this, Settings::InvalidSettingAction::ResetToDefault);
  rebuildDerivedSettings();

  if (fullState && getUsingCustomIAD() && !checkCustomIADs(false)) {
    setUsingCustomIAD(false);
    setUsingScaledIAD(true);
    clearCustomIADs();
    Common::warning("Saved custom IAD settings are incomplete. "
                    "Resetting to scaled interatomic distances.");
  }

  // In resume, we always assume that the work directory is the state file's
  //   directory, regardless of what was stored in the state file. We do this
  //   so the resumed session can -hopefully- read the rest of data properly.
  if (fullState)
    setLocWorkDir(QFileInfo(filename).absoluteDir().absolutePath());

  return true;
}

bool XtalOpt::importStateFile(const QString& filename)
{
  if (filename.isEmpty()) {
    Common::error("Cannot import the XtalOpt state file without an explicit filename.");
    return false;
  }

  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly)) {
    Common::error(QString("%1: could not open file %2 for reading...")
                  .arg(__func__).arg(file.fileName()));
    return false;
  }

  {
    QSETTINGS_FILE(filename);
    if (settings->contains("xtalopt/saveSuccessful") &&
        !settings->value("xtalopt/saveSuccessful", false).toBool()) {
      Common::error(QString("%1: file %2 is incomplete, corrupt, or invalid.")
                    .arg(__func__).arg(file.fileName()));
      return false;
    }
  }

  // Settings-only ".state" files still contain the saved state-style settings
  //   groups. So, readStateFile(filename, true) reads those groups; while
  //   importStateFile() makes sure no structure or runtime state is loaded.
  if (!readStateFile(filename, true))
    return false;

  setRunMode(RunModeGui);
  // Return the engine to idle through the session functions.
  abortSession();

  return true;
}

bool XtalOpt::loadSchemeFile(const QString& filename, bool fullState)
{
  if (filename.isEmpty()) {
    Common::error("Cannot read scheme without an explicit filename.");
    return false;
  }

  // A scheme file contains the optscheme subset of a full state file, so it
  //   uses the same compatibility conversion.
  QString readFilename;
  const bool keepCompatibilityCopy = !isReadOnly();
  if (!Legacy::convertStateFile(filename, fullState, keepCompatibilityCopy,
                                readFilename))
    return false;

  const bool readOk = readStateScheme(*this, readFilename);
  if (!keepCompatibilityCopy && readFilename != filename &&
      !QFile::remove(readFilename)) {
    Common::warning(QString("Could not remove compatibility state copy %1.")
                      .arg(readFilename));
  }
  return readOk;
}

bool XtalOpt::convertLegacyFileToCurrent(const QString& filename)
{
  if (filename.isEmpty()) {
    Common::error("Cannot convert without an explicit filename.");
    return false;
  }
  if (!Common::isReadableFile(filename)) {
    Common::error(QString("Cannot read file to convert: %1").arg(filename));
    return false;
  }

  QStringList groups;
  {
    QSettings probe(filename, QSettings::IniFormat);
    groups = probe.childGroups();
  }

  if (groups.contains("xtalopt")) {
    QString readFilename;
    if (!Legacy::convertStateFile(filename, true, true, readFilename))
      return false;
    if (readFilename == filename) {
      Common::message(QString("%1 is already current; nothing written.").arg(filename));
      return true;
    }
    Common::message(QString("Converted session file to current format: %1.").arg(readFilename));
    return true;
  }

  if (groups.contains("structure")) {
    Common::error("structure.state files are converted while loading their "
                  "XtalOpt session and cannot be converted separately.");
    return false;
  }

  QString inputText;
  QString outputText;
  QString compatibilityFilename;
  QString error;
  if (!Common::readFileToQString(filename, &inputText)) {
    Common::error(QString("Cannot read file to convert: %1").arg(filename));
    return false;
  }
  if (!Legacy::convertInputText(filename, inputText, outputText, true,
                                &compatibilityFilename, &error)) {
    Common::error(error);
    return false;
  }
  if (compatibilityFilename.isEmpty()) {
    Common::message(QString("%1 is already current; nothing written.").arg(filename));
    return true;
  }
  Common::message(QString("Converted input file to current format: %1.")
                  .arg(compatibilityFilename));
  return true;
}

} // namespace XtalOpt

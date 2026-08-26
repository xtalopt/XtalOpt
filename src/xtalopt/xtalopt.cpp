/**********************************************************************
  XtalOpt - XtalOpt application search workflow implementation

  Copyright (C) 2009-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

// Functions for the XtalOpt search. main.cpp selects the run mode; this file
// starts or reads the session, then the search engine runs it.

#include <xtalopt/xtalopt.h>

#include <xtalopt/constants.h>
#include <xtalopt/structures/xtal.h>

#include <common/fileutils.h>
#include <common/output.h>
#include <common/timing.h>
#include <common/vector.h>
#include <common/compatibility/platform_compat.h>
#include <atoms/eleminfo.h>
#include <search/optimizer.h>
#include <search/queuemanager.h>
#include <search/queueinterface.h>
#include <search/slottedwaitcondition.h>
#include <search/tracker.h>

#include <search/queueinterfaces/batch.h>
#include <search/ssh/sshmanager.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QSettings>
#include <QThread>
#include <QThreadPool>
#include <QTimer>

#include <functional>
#include <limits>
#include <vector>

using namespace Search;

namespace XtalOpt {

namespace {

// Set the default values from the settings table before reading user settings.
void applyBuiltInDefaults(XtalOpt& xtalopt)
{
  Settings::applyDefaultSettings(xtalopt);
  xtalopt.clearCustomIADs();

  // Create one default optimization step with the default queue and optimizer.
  xtalopt.clearOptSteps();
  size_t numOptSteps = Settings::defaultValue("numOptimizationSteps").toUInt();
  if (numOptSteps == 0)
    numOptSteps = 1;
  for (size_t i = 0; i < numOptSteps; ++i) {
    xtalopt.appendOptStep();
    if (!xtalopt.setQueueInterface(i, Settings::defaultValue("queueInterface").toStdString()))
      Common::error("Failed to initialize default queue interface.");
    if (!xtalopt.setOptimizer(i, Settings::defaultValue("optimizer").toStdString()))
      Common::error("Failed to initialize default optimizer.");
  }
}

bool checkReadableFile(const QString& filename, const QString& description, QString* errorMessage)
{
  QString error;
  if (!Common::isReadableFile(filename)) {
    error = QString("%1 was not found or is not readable: %2").arg(description).arg(filename);
  }

  if (!error.isEmpty()) {
    if (errorMessage)
      *errorMessage = error;
    return false;
  }

  const QFileInfo info(filename);
  const QString readablePath = info.canonicalFilePath().isEmpty()
                                 ? info.absoluteFilePath()
                                 : info.canonicalFilePath();

  Common::message(QString("Checked --- %1: %2").arg(description).arg(readablePath));
  return true;
}

bool checkPercentFileKeyword(const QString& entry, const QString& keyword,
                             const QString& ownerDescription, QString* errorMessage)
{
  if (!entry.startsWith(keyword + ":", Qt::CaseInsensitive))
    return true;

  const int colon = entry.indexOf(':');
  const QString filename = entry.mid(colon + 1).trimmed();

  if (filename.isEmpty()) {
    const QString error = QString("%1 has an empty %2 filename.").arg(ownerDescription).arg(keyword);
    if (errorMessage)
      *errorMessage = error;
    return false;
  }

  return checkReadableFile(filename, QString("%1 %2 file").arg(ownerDescription).arg(keyword),
                           errorMessage);
}

bool checkTemplateFiles(const QString& text, const QString& templateName, QString* errorMessage)
{
  // Note: here, intentionally, we don't check if template files are empty! This is to
  //   allow user easily use an implemented optimizer as a format to run their own
  //   external code.
  const QStringList parts = text.split('%');
  const QString ownerDescription = QString("Template %1").arg(templateName);
  for (const auto& part : parts) {
    if (!checkPercentFileKeyword(part, "fileContents", ownerDescription, errorMessage))
      return false;
    if (!checkPercentFileKeyword(part, "copyFile", ownerDescription, errorMessage))
      return false;
  }
  return true;
}

} // namespace

XtalOpt::XtalOpt(QObject* parent)
  : SearchBase(parent),
    x_settingsStateNeedsSave(false),
    x_resultsFileNeedsSave(false),
    x_hullFileNeedsSave(false),
    x_fullEvaluationNeeded(false),
    x_resultsFileSaveScheduled(false),
    x_hullSnapshotSequence(0),
    x_lastOutputWriteMs(0),
    x_lastOutputWriteEndMs(0),
    x_fileSaveJob([this]() { savePendingStateFiles(stateFilePath(), false, false); }),
    x_outputSaveJob([this]() { saveRequestedOutputFiles(false, false); }),
    x_similarityCheckJob([this]() { checkForSimilarities_(); requestResultsFileSave(); }),
    x_spacegroupResetJob([this]() { resetSpacegroups_(); }),
    x_similaritiesNeedReset(false),
    x_runMode(RunModeUnknown),
    x_resultsSaveTimer(new QTimer(this)),
    x_saveRetryTimer(new QTimer(this)),
    x_runtimeTimer(new QTimer(this)),
    x_initWC(new SlottedWaitCondition(this))
{
  x_xtalInitMutex.reset(new QMutex);
  setSearchIDString("XtalOpt");

  registerXtalOptOptimizerAndQueue();
  registerXtalOptKeywords();

  applyBuiltInDefaults(*this);

  // Connect the engine signals handled by XtalOpt (similarity check is handled different!)
  connect(queue(), &QueueManager::structureFinished,
          this, &XtalOpt::requestStructureEvaluation);
  connect(queue(), &QueueManager::structureFailed,
          this, &XtalOpt::requestEvaluationAfterFail);
  connect(this, &SearchBase::structureStateChanged,
          this,
          static_cast<void (XtalOpt::*)(Search::Structure*)>(
              &XtalOpt::requestStructureStateFileSave),
          Qt::DirectConnection);
  connect(this, &SearchBase::startingSession,
          this, &XtalOpt::clearPendingRequests, Qt::DirectConnection);
  connect(this, &SearchBase::structureEvaluationUpdateRequested,
          this, &XtalOpt::updateStructureEvaluationInfo, Qt::DirectConnection);
  connect(this, &SearchBase::activeSessionFinalizing,
          this, &XtalOpt::finishSearch, Qt::DirectConnection);
  // This is engine's signal for the end of the run; XtalOpt just quits!
  connect(this, &SearchBase::sessionEnded, this, []() { QCoreApplication::quit(); });

  x_saveClock.start();
  x_resultsSaveTimer->setSingleShot(true);
  connect(x_resultsSaveTimer, &QTimer::timeout, this, &XtalOpt::markResultsFileNeedsSave);

  x_saveRetryTimer->setSingleShot(true);
  x_saveRetryTimer->setInterval(SAVE_RETRY_DELAY);
  connect(x_saveRetryTimer, &QTimer::timeout, this, &XtalOpt::retryPendingFileSaves);

  // Set the runtime file timer for CLI runs.
  connect(x_runtimeTimer, &QTimer::timeout, this, &XtalOpt::checkRuntimeFile);

  // Update values from the input.
  rebuildDerivedSettings();
}

void XtalOpt::registerXtalOptOptimizerAndQueue()
{
  static bool registered = false;
  if (registered)
    return;

  // Register the optimizers and queue interfaces.
  for (const auto& name : Optimizer::availableBuiltInOptimizers())
    Optimizer::registerBuiltInOptimizer(name);

  for (const auto& name : QueueInterface::availableBuiltInQueueInterfaces())
    QueueInterface::registerBuiltInQueueInterface(name);

  registered = true;
}

void XtalOpt::registerXtalOptKeywords()
{
  // XtalOpt-specific template keywords (generic keywords are registered by SearchBase).

  // clang-format off
  registerKeyword("mtpAtomsInfo", [this](Structure* s) -> QString {
    const std::vector<Atoms::Atom>& atoms = s->atoms();
    QString rep;
    int tag = 1;
    for (auto it = atoms.begin(); it != atoms.end(); ++it) {
      const Common::Vector3 coords = s->cartToFrac((*it).pos());
      QString sym = QString(Atoms::ElementInfo::getAtomicSymbol((*it).atomicNumber()).c_str());
      int typ = getChemicalSystem().indexOf(sym);
      rep += QString("    %1  %2  %3  %4  %5\n")
               .arg(tag++, 5, 10, QChar(' '))
               .arg(typ, 5, 10, QChar(' '))
               .arg(coords.x(), 14, 'f', 8, QChar(' '))
               .arg(coords.y(), 14, 'f', 8, QChar(' '))
               .arg(coords.z(), 14, 'f', 8, QChar(' '));
    }
    rep += "Feature chemical_system ";
    for (const auto& sym : getChemicalSystem())
      rep += sym + " ";
    rep += "\n";
    return rep;
  }, "%mtpAtomsInfo% -- MTP-style atomic coordinates and full chemical system info");
  registerKeyword("chemicalSystem", [this](Structure*) -> QString {
    QString rep;
    for (const auto& sym : getChemicalSystem())
      rep += sym + " ";
    rep += "\n";
    return rep;
  }, "%chemicalSystem% -- List of element symbols in alphabetical order");
  // clang-format on
}

XtalOpt::~XtalOpt()
{
  x_fileSaveJob.shutdown();
  x_outputSaveJob.shutdown();
  x_similarityCheckJob.shutdown();
  x_spacegroupResetJob.shutdown();

  // Stop the queue drivers before XtalOpt saves.
  stopQueueThread();

  // Script launches use QueueManager's separate worker pool.
  queue()->waitForScriptCalculations(-1);
  QThreadPool::globalInstance()->waitForDone(-1);

  // Save the state file.
  if (!isReadOnly() && !queue()->getAllStructures().isEmpty()) {
    Common::message("Saving XtalOpt state...");
    finishSearch();
  }

  // Destroy queuemanager
  queue()->reset();
  deleteTrackedStructures();


  x_initWC->deleteLater();
  x_initWC = nullptr;
}

bool XtalOpt::canRequestStateFileSave() const
{
  return isSessionActive() && !isSessionStarting() && !isReadOnly();
}

QList<Search::Structure*> XtalOpt::trackedStructuresSnapshot()
{
  // Copy the pointers while the tracker is locked. The returned list is separate
  // from the live tracker, so callers can release the lock before doing their work.
  QList<Search::Structure*> structures;
  QReadLocker trackerLocker(tracker()->rwLock());
  structures.reserve(tracker()->list()->size());
  for (Search::Structure* structure : *tracker()->list())
    structures.append(structure);
  return structures;
}

void XtalOpt::requestStateFileSave()
{
  if (!canRequestStateFileSave())
    return;

  {
    std::lock_guard<std::mutex> guard(x_filesNeedingSaveMutex);
    x_settingsStateNeedsSave = true;
  }
  x_fileSaveJob.request();
}

void XtalOpt::requestStructureStateFileSave(Search::Structure* structure)
{
  // Objc/cons calculation statuses don't need a save operation
  if (structure) {
    QReadLocker locker(&structure->lock());
    if (structure->isScriptCalculationState())
      return;
  }

  QList<Search::Structure*> structures;
  structures.append(structure);
  requestStructureStateFileSave(structures);
}

void XtalOpt::requestStructureStateFileSave(const QList<Search::Structure*>& structures)
{
  if (structures.isEmpty() || !canRequestStateFileSave())
    return;

  {
    std::lock_guard<std::mutex> guard(x_filesNeedingSaveMutex);
    for (Search::Structure* structure : structures) {
      if (structure)
        x_structuresNeedingSave.insert(structure);
    }
  }
  x_fileSaveJob.request();
  // We must update results since it contains structures' data
  requestResultsFileSave();
}

void XtalOpt::requestResultsFileSave(bool alsoHullFile)
{
  if (!canRequestStateFileSave())
    return;

  if (alsoHullFile) {
    std::lock_guard<std::mutex> guard(x_filesNeedingSaveMutex);
    x_hullFileNeedsSave = true;
  }

  // Start the collection timer once to merge all requests meanwhile
  bool expected = false;
  if (!x_resultsFileSaveScheduled.compare_exchange_strong(expected, true))
    return;

  // Keep results writing apart by at least "results writing interval factor * last write's duration"
  int delay = RESULTS_SAVE_DELAY;
  const qint64 wait = RESULTS_SAVE_SPACING_FACTOR * x_lastOutputWriteMs.load() -
                      (x_saveClock.elapsed() - x_lastOutputWriteEndMs.load());
  if (wait > delay)
    delay = static_cast<int>(wait);

  if (!QMetaObject::invokeMethod(x_resultsSaveTimer, "start", Qt::QueuedConnection, Q_ARG(int, delay)))
    x_resultsFileSaveScheduled.store(false);
}

void XtalOpt::markResultsFileNeedsSave()
{
  x_resultsFileSaveScheduled.store(false);
  if (!canRequestStateFileSave())
    return;

  {
    std::lock_guard<std::mutex> guard(x_filesNeedingSaveMutex);
    x_resultsFileNeedsSave = true;
  }
  x_outputSaveJob.request();
}

void XtalOpt::retryPendingFileSaves()
{
  if (!canRequestStateFileSave())
    return;

  x_fileSaveJob.request();
  x_outputSaveJob.request();
}

void XtalOpt::requestStructureEvaluation(Search::Structure* structure)
{
  if (!structure)
    return;

  {
    std::lock_guard<std::mutex> guard(x_filesNeedingSaveMutex);
    x_structuresNeedingEvaluation.insert(structure);
  }
  requestStructureEvaluationUpdate();
}

void XtalOpt::requestFullEvaluation()
{
  {
    std::lock_guard<std::mutex> guard(x_filesNeedingSaveMutex);
    x_fullEvaluationNeeded = true;
  }
  requestStructureEvaluationUpdate();
}

void XtalOpt::requestEvaluationAfterFail(Search::Structure* structure)
{
  if (!structure)
    return;

  // Only failing of an optimized structure (Removed) can move change hull.
  bool wasOptimized = false;
  {
    QReadLocker structureLocker(&structure->lock());
    wasOptimized = structure->getStatus() == Search::Structure::Removed;
  }
  if (wasOptimized)
    handleOptimizedDeparture(structure);
}

void XtalOpt::handleOptimizedDeparture(Search::Structure* structure)
{
  if (!structure)
    return;

  QString tag;
  {
    QWriteLocker structureLocker(&structure->lock());
    tag = structure->getTag();
    Xtal* xtal = qobject_cast<Xtal*>(structure);
    if (xtal)
      xtal->setDistAboveHull(std::numeric_limits<double>::quiet_NaN());
    structure->setParetoFront(-1);
    QList<double> objectiveValues = structure->getStrucObjValuesVec();
    const int builtinObjective = getBuiltinObjectiveIndex();
    if (builtinObjective >= 0 && builtinObjective < objectiveValues.size()) {
      objectiveValues[builtinObjective] = std::numeric_limits<double>::quiet_NaN();
      structure->setStrucObjValuesVec(objectiveValues);
    }
  }

  // The invalidated objective values make this structure ineligible as a
  //   parent immediately; do not wait for a later queue update to remove it.
  refreshParentPoolMembership(structure);

  QList<Search::Structure*> structures = trackedStructuresSnapshot();

  // Re-set the structures that were similar to the recovered one!
  QList<Search::Structure*> released;
  for (Search::Structure* other : structures) {
    if (other == structure)
      continue;
    QWriteLocker otherLocker(&other->lock());
    if (other->getSimilarityString() != tag)
      continue;
    other->structureChanged(); // Clears the status and sets the re-check flag
    released.append(other);
  }
  // A released structure rejoins the parent pool (refresh takes the
  //   structure lock itself, so it must run after the loop's write locks).
  for (Search::Structure* other : released)
    refreshParentPoolMembership(other);
  if (!released.isEmpty())
    checkForSimilarities();

  requestFullEvaluation();
}

void XtalOpt::clearPendingRequests()
{
  std::lock_guard<std::mutex> guard(x_filesNeedingSaveMutex);
  x_structuresNeedingSave.clear();
  x_settingsStateNeedsSave = false;
  x_resultsFileNeedsSave = false;
  x_hullFileNeedsSave = false;
  x_pendingHullSnapshots.clear();
  x_structuresNeedingEvaluation.clear();
  x_fullEvaluationNeeded = false;
  x_hullPointsCache.clear();
}

void XtalOpt::finishSearch()
{
  if (x_runtimeTimer->thread() == QThread::currentThread()) {
    x_runtimeTimer->stop();
  } else {
    (void)QMetaObject::invokeMethod(x_runtimeTimer, "stop",
                                    Qt::QueuedConnection);
  }

  // Save the final results.
  if (isReadOnly() || queue()->getAllStructures().isEmpty())
    return;

  refreshStructureEvaluationData();
  refreshParentSelectionFronts(getAllParentPoolStructures());

  // Structure files are kept current throughout the run; write the ones
  //   still waiting, and always refresh the state file.
  {
    std::lock_guard<std::mutex> guard(x_filesNeedingSaveMutex);
    x_settingsStateNeedsSave = true;
  }
  savePendingStateFiles(stateFilePath(), false, false);
  saveRequestedOutputFiles(true, false);
}

bool XtalOpt::startSearch()
{
  return runSearch(QString(), nullptr);
}

bool XtalOpt::resumeSearch(const QString& filename, bool* onlyMainStateWasLoaded)
{
  if (filename.isEmpty()) {
    Common::error("Cannot resume an XtalOpt search without a state file.");
    return false;
  }

  return runSearch(filename, onlyMainStateWasLoaded);
}

bool XtalOpt::runSearch(const QString& stateFile, bool* onlyMainStateWasLoaded)
{
  const bool restoring = !stateFile.isEmpty();
  const bool readOnly = isReadOnly();
  if (!readOnly)
    Common::timingActivated() = true;
  bool stateWasConverted = false;
  QString loadedStateFile = stateFile;
  QStringList xtalDirs;

  if (onlyMainStateWasLoaded)
    *onlyMainStateWasLoaded = false;

  if (!beginSession())
    return false;

  // A saved search first supplies the settings needed by the checks below.
  if (restoring) {
    const QString formattedTime =
      QDateTime::currentDateTime().toString("MMMM dd, yyyy   hh:mm:ss");
    Common::message("\n=== XtalOpt session started to load ... " +
                    formattedTime.toLocal8Bit() + "\n");
    Common::message(QString("\nSession source: '%1'\n\n")
                      .arg(QFileInfo(stateFile).absoluteDir().absolutePath()));

    for (;;) {
      QFile file(loadedStateFile);
      if (!file.open(QIODevice::ReadOnly)) {
        Common::error(QString("%1: could not open file %2 for reading...")
                        .arg(__func__).arg(file.fileName()));
        abortSession();
        return false;
      }

      QSettings settings(loadedStateFile, QSettings::IniFormat);
      const bool fileIsValid =
        settings.value("xtalopt/saveSuccessful", false).toBool();
      bool loadedStateWasConverted = false;
      if (fileIsValid && readStateFile(loadedStateFile, true, &loadedStateWasConverted)) {
        stateWasConverted = loadedStateWasConverted;
        break;
      }

      if (loadedStateFile.endsWith(".old")) {
        Common::error(QString("%1: file %2 is incomplete, corrupt, or invalid. "
                              "Cannot begin run. Please check your state file.")
                        .arg(__func__).arg(loadedStateFile));
        abortSession();
        return false;
      }

      if (!requestBooleanDecision(
            tr("XtalOpt::resumeSearch(): File:\n\n'%1'\n\nis incomplete, corrupt, "
               "or invalid. Would you like to try loading:\n\n'%1.old'\n\ninstead?")
              .arg(loadedStateFile))) {
        abortSession();
        return false;
      }
      loadedStateFile += ".old";
    }

    xtalDirs = readStructureStateDirectories(loadedStateFile);
    if (xtalDirs.isEmpty()) {
      Common::error(QString("No structures were found in %1.")
                      .arg(QFileInfo(loadedStateFile).absoluteDir().absolutePath()));
      if (onlyMainStateWasLoaded)
        *onlyMainStateWasLoaded = true;
      abortSession();
      return false;
    }
  }

  QString startupError;
  if (!readOnly) {
    Common::message("\n=== Pre-checks\n");

    const QString sessionStateFile = stateFilePath();
    bool hasExistingStateFile = QFile::exists(sessionStateFile) || QFile::exists(sessionStateFile + ".old");

    if (!restoring) {
      const QString locWorkDir = getLocWorkDir();
      if (locWorkDir.isEmpty()) {
        startupError = tr("Set an absolute local working directory before starting the search.");
      } else if (!QDir::isAbsolutePath(locWorkDir)) {
        startupError = tr("The local working directory must be an absolute "
                          "path before starting the search.\n\nCurrent value:\n%1")
                         .arg(locWorkDir);
      } else if (hasExistingStateFile || !readStructureStateDirectories(sessionStateFile).isEmpty()) {
        // If any xtalopt or structure state files are present.
        startupError = tr("XtalOpt data is already saved at:\n%1"
                          "\n\nEmpty the directory to proceed or "
                          "select a new 'Local working directory'!")
                         .arg(locWorkDir);
      }
    }

    if (startupError.isEmpty() && compList().isEmpty())
      startupError = tr("Cannot create structures. Composition is not set.");

    if (startupError.isEmpty() &&
        !Settings::validateSettings(*this, Settings::InvalidSettingAction::Reject)) {
      startupError = tr("Settings have invalid values; see the log for details.");
    }

    if (startupError.isEmpty() && getUsingCustomIAD() && !verifyCustomIADValues())
      startupError = tr("Custom IAD mode requires a complete custom IAD table.");

    if (startupError.isEmpty() &&
        !checkLocalInputFiles(!restoring, &startupError) && startupError.isEmpty())
      startupError = tr("Some local input files could not be read.");

    if (startupError.isEmpty() &&
        !checkOptimizerAndQueue(restoring ? "resume" : "start", &startupError) &&
        startupError.isEmpty())
      startupError = tr("The optimizer or queue is not ready.");

    if (startupError.isEmpty() && !restoring) {
      InitialGenerationPlan plan;
      buildInitialGenerationPlan(plan, false);
      const int maxStructures = getMaxNumStructures();
      if (plan.totalTarget > static_cast<uint>(maxStructures)) {
        Common::warning(
          tr("Requested initial generation exceed maxNumStructures; max will apply to later generation\n"
             "  Total initial %1 (%2 seed, %3 forced RandSpg, %4 random numInitial) - maxNumStructures (%5)")
            .arg(plan.totalTarget)
            .arg(plan.seedCount)
            .arg(plan.forcedRandSpgCount)
            .arg(plan.randomCount)
            .arg(maxStructures));
      }
    }

    if (!startupError.isEmpty()) {
      Common::error(startupError);
      emit errorDialogRequested(startupError);
      abortSession();
      return false;
    }
  }

  if (restoring) {
    if (!restorePopulation(loadedStateFile, xtalDirs)) {
      abortSession();
      return false;
    }
  } else {
    const QString formattedTime =
      QDateTime::currentDateTime().toString("MMMM dd, yyyy   hh:mm:ss");
    Common::message("\n=== Optimization started ... " +
                    formattedTime.toLocal8Bit() + "\n");
    if (!generateInitialStructures()) {
      abortSession();
      return false;
    }
  }

  // Save all structure files before replacing a converted state file.
  // The old state is still needed while its old structure files are read.
  if (restoring && !readOnly && stateWasConverted) {
    if (!savePendingStateFiles(stateFilePath(), true, false)) {
      const QString saveError = tr("Could not finish updating the old structure state files. "
                                   "The search was not resumed.");
      Common::error(saveError);
      emit errorDialogRequested(saveError);
      abortSession();
      return false;
    }
  }

  launchSession();

  if (!restoring) {
    saveSessionState(stateFilePath(), false);
  } else {
    const QString formattedTime =
      QDateTime::currentDateTime().toString("MMMM dd, yyyy   hh:mm:ss");
    if (readOnly) {
      Common::message("\n=== XtalOpt session loaded read-only ... " +
                      formattedTime.toLocal8Bit() + "\n\n");
    } else {
      Common::message("\n=== Optimization resumed ... " +
                      formattedTime.toLocal8Bit() + "\n\n");
      saveRequestedOutputFiles(true, false);
    }
  }

  // Write and watch the runtime file for CLI runs.
  if (x_runMode == RunModeCliStart || x_runMode == RunModeCliResume) {
    saveRuntimeFile();
    x_runtimeTimer->start(RUNTIME_FILE_CHECK_INTERVAL);
  }

  return true;
}

bool XtalOpt::checkLocalInputFiles(bool includeSeeds, QString* errorMessage) const
{
  if (includeSeeds) {
    for (const auto& seed : seedList()) {
      if (!checkReadableFile(seed, "Seed structure", errorMessage))
        return false;
    }
  }

  for (size_t optStep = 0; optStep < getNumOptSteps(); ++optStep) {
    const QueueInterface* queue = queueInterface(optStep);
    if (queue) {
      const QStringList filenames = queue->getQueueInterfaceTemplateFileNames();
      for (const auto& filename : filenames) {
        const QString text = getQueueInterfaceTemplate(optStep, filename.toStdString()).c_str();
        const QString tmp  = QString("opt step %1 queue template %2").arg(optStep + 1).arg(filename);
        if (!checkTemplateFiles(text, tmp, errorMessage)) {
          return false;
        }
      }
    }

    const Optimizer* optim = optimizer(optStep);
    if (!optim)
      continue;

    const QStringList filenames = optim->getOptimizerTemplateFileNames();
    for (const auto& filename : filenames) {
      const QString text = getOptimizerTemplate(optStep, filename.toStdString()).c_str();
      const QString tmp  = QString("opt step %1 optimizer template %2").arg(optStep + 1).arg(filename);
      if (!checkTemplateFiles(text, tmp, errorMessage)) {
        return false;
      }
    }

    const QStringList assetNames = optim->getOptimizerInputAssetNames();
    for (const auto& assetName : assetNames) {
      const QString owner = QString("opt step %1 optimizer input asset %2")
                                   .arg(optStep + 1).arg(assetName);
      const OptimizerInputAssetMap assetFiles =
        getOptimizerInputAssets(optStep, assetName.toStdString());

      // Currently only VASP optimizer supports "system" asset id
      const bool systemFile = assetName.compare("POTCAR", Qt::CaseInsensitive) == 0 &&
                              assetFiles.count("system") != 0;

      // Check exactly the entries the optimizer will read: the "system" file
      //   when it is set, and one file per element otherwise.
      QStringList usedIds;
      if (systemFile) {
        usedIds.append("system");
      } else {
        for (const auto& symbol : getChemicalSystem()) {
          if (assetFiles.count(symbol.toStdString()) == 0) {
            if (errorMessage)
              *errorMessage = owner + " has no entry for element " + symbol + ".";
            return false;
          }
          usedIds.append(symbol);
        }
      }

      for (const auto& id : usedIds) {
        const auto assetIt = assetFiles.find(id.toStdString());
        if (!checkReadableFile(QString::fromStdString(assetIt->second),
                               owner + " file for " + id, errorMessage))
          return false;
      }
    }
  }

  return true;
}

bool XtalOpt::checkOptimizerAndQueue(const QString& readinessAction, QString* errorMessage)
{
  if (isRemoteQueue() && anyBatchQueueInterfaces()) {
    if (!createSSHConnections()) {
      if (errorMessage)
        *errorMessage = tr("Could not create ssh connections.");
      return false;
    }
  }

  QString err;
  if (!isReadyToSearch(&err)) {
    if (errorMessage) {
      *errorMessage = tr("Search is not ready to %1: %2")
                        .arg(readinessAction)
                        .arg(err);
    }
    return false;
  }

  // Warn about a search that starts in "paused" state (not able to generate/submit structures).
  if (isLimitRunningJobs() && getRunningJobLimit() == 0) {
    Common::warning(tr("The number of running jobs is currently set to 0.\n"
                       "You will need to increase this value before the search " "can begin.\n"
                       "(You can change this in the runtime file or in "
                       "the interactive search settings.)"));
  }

  if (getContStructs() == 0) {
    Common::warning(tr("The number of continuous structures is currently set to 0.\n"
                       "You will need to increase this value before the search "
                       "can move past the first generation.\n"
                       "(You can change this in the runtime file or in "
                       "the interactive search settings.)"));
  }

  return true;
}

int XtalOpt::getUserObjectivesNum() const
{
  const int userObjectives = getObjectivesNum() - getFirstUserObjectiveIndex();
  return std::max(0, userObjectives);
}

int XtalOpt::getUserObjectiveIndex(int userObjectiveNumber) const
{
  return getFirstUserObjectiveIndex() + userObjectiveNumber;
}

bool XtalOpt::hasUserObjectives() const
{
  return getUserObjectivesNum() > 0;
}

bool XtalOpt::needsObjectiveOrConstraintCalculations() const
{
  return hasUserObjectives() || getConstraintsNum() > 0;
}

bool XtalOpt::removeUserObjective(int index)
{
  if (index < getFirstUserObjectiveIndex() || index >= getObjectivesNum())
    return false;

  SearchBase::removeObjective(index);
  // The objectives (weights) changed; adjust the built-in weight. 
  refreshBuiltinObjectiveWeight();
  return true;
}

bool XtalOpt::removeConstraint(int index)
{
  if (index < 0 || index >= getConstraintsNum())
    return false;

  SearchBase::removeConstraint(index);
  return true;
}

void XtalOpt::ensureBuiltinObjective()
{
  if (getObjectivesNum() != 0)
    return;

  addObjective(ObjType::Ot_Min, QString(), QString(), 1.0);
}

void XtalOpt::refreshBuiltinObjectiveWeight()
{
  ensureBuiltinObjective();

  double totalUserWeight = 0.0;
  for (int userObjective = 0; userObjective < getUserObjectivesNum(); ++userObjective)
    totalUserWeight += getObjectivesWgt(getUserObjectiveIndex(userObjective));

  setObjectivesWgt(getBuiltinObjectiveIndex(), std::max(0.0, 1.0 - totalUserWeight));
}

} // end namespace XtalOpt

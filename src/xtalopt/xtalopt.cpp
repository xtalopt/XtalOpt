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
#include <vector>

using namespace Search;

namespace XtalOpt {

// Set the default values from the settings table before reading user settings.
void applyBuiltInDefaults(XtalOpt& xtalopt)
{
  Settings::applyAllDefaults(xtalopt);
  xtalopt.interComp().clear();

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

// Check an objective or constraint output file name. It must be a simple
// relative name so the output stays in the structure directory.
static bool isSafeOutputFilename(const QString& filename)
{
  const QString trimmed = filename.trimmed();
  if (trimmed.isEmpty() || trimmed != filename)
    return false;
  if (trimmed == "." || trimmed == "..")
    return false;
  if (trimmed.contains('/') || trimmed.contains('\\'))
    return false;
  if (trimmed.contains(QChar::fromLatin1('\0')))
    return false;
  return QFileInfo(trimmed).fileName() == trimmed;
}

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
    x_loadedVersion4State(false),
    x_fileSaveJob([this]() { saveRequestedStateFiles(searchStateFilePath(), false, false); }),
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
  connect(queue(), &QueueManager::structureKilled,
          this, &XtalOpt::requestEvaluationAfterKill);
  connect(this, &SearchBase::structureStateChanged,
          this,
          static_cast<void (XtalOpt::*)(Search::Structure*)>(
              &XtalOpt::requestStructureStateSave),
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
  connect(x_saveRetryTimer, &QTimer::timeout, this, &XtalOpt::retryFileSave);

  // Set the run-time file timer for CLI runs.
  connect(x_runtimeTimer, &QTimer::timeout, this, &XtalOpt::updateRuntimeState);

  // Update values from the input.
  processInputData();
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
}

XtalOpt::~XtalOpt()
{
  x_fileSaveJob.shutdown();
  x_outputSaveJob.shutdown();
  x_similarityCheckJob.shutdown();
  x_spacegroupResetJob.shutdown();

  // Stop the queue drivers before XtalOpt saves.
  stopQueueThread();

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

bool XtalOpt::canRequestFileSave() const
{
  return isSessionActive() && !isSessionStarting() && !isReadOnly();
}

void XtalOpt::requestSettingsStateSave()
{
  if (!canRequestFileSave())
    return;

  {
    std::lock_guard<std::mutex> guard(x_filesNeedingSaveMutex);
    x_settingsStateNeedsSave = true;
  }
  x_fileSaveJob.request();
}

void XtalOpt::requestStructureStateSave(Search::Structure* structure)
{
  // Objc/cons calculation statuses don't need a save operation
  if (structure) {
    QReadLocker locker(&structure->lock());
    if (structure->isPostOptimizationCalculationState())
      return;
  }

  QList<Search::Structure*> structures;
  structures.append(structure);
  requestStructureStateSave(structures);
}

void XtalOpt::requestStructureStateSave(const QList<Search::Structure*>& structures)
{
  if (structures.isEmpty() || !canRequestFileSave())
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
  if (!canRequestFileSave())
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
  if (!canRequestFileSave())
    return;

  {
    std::lock_guard<std::mutex> guard(x_filesNeedingSaveMutex);
    x_resultsFileNeedsSave = true;
  }
  x_outputSaveJob.request();
}

void XtalOpt::retryFileSave()
{
  if (!canRequestFileSave())
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

void XtalOpt::requestEvaluationAfterKill(Search::Structure* structure)
{
  if (!structure)
    return;

  // Only the kill of an optimized structure (Removed) can move change hull.
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
    QReadLocker structureLocker(&structure->lock());
    tag = structure->getTag();
  }

  QList<Search::Structure*> structures;
  {
    QReadLocker trackerLocker(tracker()->rwLock());
    structures.reserve(tracker()->list()->size());
    for (Search::Structure* trackerStructure : *tracker()->list())
      structures.append(trackerStructure);
  }

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
  //   still waiting, and always refresh the main state file.
  {
    std::lock_guard<std::mutex> guard(x_filesNeedingSaveMutex);
    x_settingsStateNeedsSave = true;
  }
  saveRequestedStateFiles(searchStateFilePath(), false, false);
  saveRequestedOutputFiles(true, false);
}

bool XtalOpt::startSearch()
{
  return runSearch(QString(), nullptr);
}

bool XtalOpt::resumeSearch(const QString& filename, bool* settingsOnlyLoaded)
{
  if (filename.isEmpty()) {
    Common::error("Cannot resume an XtalOpt search without a state file.");
    return false;
  }

  return runSearch(filename, settingsOnlyLoaded);
}

bool XtalOpt::runSearch(const QString& stateFile, bool* settingsOnlyLoaded)
{
  const bool restoring = !stateFile.isEmpty();
  const bool readOnly = isReadOnly();
  QString loadedStateFile = stateFile;
  QStringList xtalDirs;

  if (settingsOnlyLoaded)
    *settingsOnlyLoaded = false;

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
      if (fileIsValid && readSettings(loadedStateFile, true))
        break;

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

    xtalDirs = structureStateDirs(loadedStateFile);
    if (xtalDirs.isEmpty()) {
      Common::error(QString("No structures were found in %1.")
                      .arg(QFileInfo(loadedStateFile).absoluteDir().absolutePath()));
      if (settingsOnlyLoaded)
        *settingsOnlyLoaded = true;
      abortSession();
      return false;
    }
  }

  QString startupError;
  if (!readOnly) {
    Common::message("\n=== Pre-checks\n");

    if (!restoring) {
      const QString locWorkDir = getLocWorkDir();
      if (locWorkDir.isEmpty()) {
        startupError = tr("Set an absolute local working directory before starting the search.");
      } else if (!QDir::isAbsolutePath(locWorkDir)) {
        startupError = tr("The local working directory must be an absolute "
                          "path before starting the search.\n\nCurrent value:\n%1")
                         .arg(locWorkDir);
      } else if (hasExistingSearchStateFile()) {
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

    if (startupError.isEmpty() && !checkLimits())
      startupError = tr("Cannot create structures. Check log for details.");

    if (startupError.isEmpty() &&
        !checkLocalInputFiles(!restoring, &startupError) && startupError.isEmpty())
      startupError = tr("Some local input files could not be read.");

    if (startupError.isEmpty() &&
        !checkOptimizerAndQueue(restoring ? "resume" : "start", &startupError) &&
        startupError.isEmpty())
      startupError = tr("The optimizer or queue is not ready.");

    if (startupError.isEmpty() && !restoring) {
      InitialGenerationPlan plan;
      if (!buildInitialGenerationPlan(plan, &startupError, false)) {
        if (startupError.isEmpty())
          startupError = tr("Could not prepare the initial structures.");
      } else {
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

  // Save all converted structure files before replacing the old main state file.
  // The old main file is needed to read any structures left in the old format.
  if (restoring && !readOnly && x_loadedVersion4State) {
    if (!saveRequestedStateFiles(searchStateFilePath(), true, false)) {
      const QString saveError = tr("Could not finish updating the old structure state files. "
                                   "The search was not resumed.");
      Common::error(saveError);
      emit errorDialogRequested(saveError);
      abortSession();
      return false;
    }
    x_loadedStateConstraintObjectiveIndices.clear();
    x_loadedVersion4State = false;
  }

  launchSession();

  if (!restoring) {
    save(searchStateFilePath(), false);
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

  // Write and watch the run-time file for CLI runs.
  if (x_runMode == RunModeCliStart || x_runMode == RunModeCliResume) {
    writeInitialRuntimeFile();
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
      const QString text = getOptimizerInputAsset(optStep, assetName.toStdString()).c_str();
      const QString owner = QString("opt step %1 optimizer input asset %2")
                                   .arg(optStep + 1).arg(assetName);
      QHash<QString, QString> assetFiles;
      const QStringList assetLines = Optimizer::inputAssetTextToFiles(text).split('\n');
      for (const auto& line : assetLines) {
        if (line.trimmed().isEmpty())
          continue;

        QString id, fileEntry;
        if (!Optimizer::parseAssetIdFileLine(line, id, fileEntry)) {
          if (errorMessage)
            *errorMessage = owner + " contains an invalid entry: " + line;
          return false;
        }

        assetFiles.insert(id.toLower(), fileEntry);
      }

      const bool systemFile = assetName.compare("POTCAR", Qt::CaseInsensitive) == 0 && assetFiles.contains("system");

      if (!systemFile) {
        for (const auto& symbol : getChemicalSystem()) {
          if (!assetFiles.contains(symbol.toLower())) {
            if (errorMessage)
              *errorMessage = owner + " has no entry for element " + symbol + ".";
            return false;
          }
        }
      }

      const QStringList parts = text.split('%');
      for (const auto& part : parts) {
        if (!checkPercentFileKeyword(part, "fileContents", owner, errorMessage)) {
          return false;
        }
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
                       "(You can change this in the runtime options file or in "
                       "the interactive search settings.)"));
  }

  if (getContStructs() == 0) {
    Common::warning(tr("The number of continuous structures is currently set to 0.\n"
                       "You will need to increase this value before the search "
                       "can move past the first generation.\n"
                       "(You can change this in the runtime options file or in "
                       "the interactive search settings.)"));
  }

  return true;
}

bool XtalOpt::buildInitialGenerationPlan(InitialGenerationPlan& plan, QString*, bool reportWarnings)
{
  plan = InitialGenerationPlan();
  plan.seedCount = static_cast<uint>(seedList().size());

  // The list of forced space-group counts (index = spg-1, 0 = not forced).
  plan.randSpgCounts.clear();
  for (uint spg = 1; spg <= 230; ++spg) {
    const int requested = (static_cast<int>(spg) <= minXtalsOfSpg().size())
        ? minXtalsOfSpg().at(spg - 1)
        : 0;
    plan.randSpgCounts.append(requested > 0 ? requested : 0);
  }
  for (int i = 0; i < plan.randSpgCounts.size(); ++i) {
    const int requested = plan.randSpgCounts.at(i);
    if (requested <= 0)
      continue;

    const uint spg = static_cast<uint>(i + 1);
    const int compatibleFormulas = randSpgCompatibleFormulaStrings(spg).size();
    if (compatibleFormulas == 0) {
      if (reportWarnings && isVerbose()) {
        Common::message(tr("   forced RandSpg space group %1 (requested %2 times) can't work "
                           "for any input formula.").arg(spg).arg(requested));
      }
      plan.randSpgCounts[i] = -1;
      continue;
    }

    plan.forcedRandSpgCount += static_cast<uint>(requested * compatibleFormulas);
  }

  plan.randomCount = getNumInitial();
  plan.totalTarget = plan.seedCount + plan.forcedRandSpgCount + plan.randomCount;
  return true;
}

bool XtalOpt::generateInitialStructures()
{
  ////////////////////////////////////////////////////////////////////////
  /// This function generates "initial population". Generally, it generates:
  ///
  /// a) "forced structures" when applicable:
  ///   1) seed structures (if user provides any)
  ///   2) relevant spg#s to all input formulas (if forced randSpg is set)
  ///
  /// b) "random structures" up to "total random initial structures", from:
  ///   3) random structures using randSpg (if user sets usingRandSpg)
  ///   4) random structures using molUnit (if user sets molUnits)
  ///   5) random structures from compositions
  ////////////////////////////////////////////////////////////////////////

  InitialGenerationPlan plan;
  QString planningError;
  if (!buildInitialGenerationPlan(plan, &planningError)) {
    if (!planningError.isEmpty())
      Common::error(planningError);
    return false;
  }

  // Set up progress reporting
  beginProgressUpdate(tr("Generating structures..."), 0, 0);

  // Initialize loop variables
  int failed = 0;
  QString filename;

  // Use new xtal count in case "addXtal" falls behind so that we
  //   don't duplicate structures when switching from seeds -> random.
  uint newXtalCount = 0;

  // Load seeds...
  for (int i = 0; i < seedList().size(); i++) {
    filename = seedList().at(i);
    if (this->addSeed(filename)) {
      updateProgressBar(plan.totalTarget, newXtalCount + failed, newXtalCount);
      newXtalCount++;
    } else {
      failed++;
    }
  }

  // Generate requested RandSpg structures. The plan keeps the remaining count.
  if (plan.forcedRandSpgCount > 0) {
    QList<int> spgStillNeeded = plan.randSpgCounts;
    for (int i = 0; i < spgStillNeeded.size(); i++) {
      while (spgStillNeeded.at(i) > 0) {
        for (int compi = 0; compi < compList().size(); compi++) {
          uint spg = i + 1;

          // If the spacegroup isn't possible, just continue
          if (!isRandSpgPossibleForComposition(spg, compList()[compi]))
            continue;

          updateProgressBar(plan.totalTarget, newXtalCount + failed, newXtalCount);
          if (acceptInitialXtal(randSpgXtal(1, newXtalCount + 1, compList()[compi], spg)))
            newXtalCount++;
          else
            failed++;
        }
        spgStillNeeded[i]--;
      }
    }
  }

  // Generate the requested random initial structures (retry each until one is
  //   accepted, up to a fixed total count).
  const int maxRandomAttempts = 10000;
  uint randomGenerated = 0;
  while (randomGenerated < plan.randomCount) {
    bool accepted = false;
    for (int attempt = 0; attempt < maxRandomAttempts; ++attempt) {
      updateProgressBar(plan.totalTarget, newXtalCount + failed, newXtalCount);
      if (acceptInitialXtal(generateRandomXtal(1, newXtalCount + 1))) {
        newXtalCount++;
        accepted = true;
        break;
      }
      failed++;
    }
    if (!accepted) {
      Common::warning(QString("%1: failed too many times while generating %2. "
                              "Giving up.")
                        .arg(__func__)
                        .arg(tr("random initial structure")));
      endProgressUpdate();
      return false;
    }
    randomGenerated++;
  }

  // Wait for all structures to appear in tracker
  updateProgressValue(-1, tr("Waiting for structures to initialize..."), 0, newXtalCount);

  connect(tracker(), &Tracker::newStructureAdded, x_initWC, &SlottedWaitCondition::wakeAllSlot);

  x_initWC->prewaitLock();
  do {
    updateProgressValue(tracker()->size(),
                        tr("Waiting for structures to initialize (%1 of %2)...")
                          .arg(tracker()->size())
                          .arg(newXtalCount));
    // Do not wait here forever. A final signal can arrive before the wait.
    x_initWC->wait(INIT_WAIT_TIMEOUT);
  } while (tracker()->size() < static_cast<int>(newXtalCount));
  x_initWC->postwaitUnlock();

  // We're done with x_initWC.
  disconnect(tracker(), &Tracker::newStructureAdded, x_initWC, &SlottedWaitCondition::wakeAllSlot);

  endProgressUpdate();

  return true;
}

// Check and add an initial structure. Delete it when it is not accepted.
bool XtalOpt::acceptInitialXtal(Xtal* generated)
{
  if (!checkXtal(generated)) {
    delete generated;
    return false;
  }

  generated->findSpaceGroup(getTolSpg());
  if (isVerbose()) {
    Common::message(QString("   generated initial structure: %1")
                    .arg(generated->getParents()));
  }
  initializeAndAddXtal(generated, 1, generated->getParents());
  return true;
}

void XtalOpt::updateRuntimeState()
{
  // Only refresh the runtime file in an active CLI session.
  if (!isSessionActive() ||
      (x_runMode != RunModeCliStart && x_runMode != RunModeCliResume))
    return;

  const QString filename = CLIRuntimeFile();
  if (filename.isEmpty() || !QFileInfo(filename).exists())
    return;

  QString runtimeText;
  if (!Common::readFileToQString(filename, &runtimeText))
    return;
  if (runtimeText == x_lastRuntimeText)
    return;

  // Do not read an unchanged run-time file. Warnings belong to one file update.
  x_lastRuntimeText = runtimeText;
  readRuntimeOptions(runtimeText);
}

bool XtalOpt::validateUserObjectiveDefinition(ObjType objtyp, const QString& objexe,
                                              const QString& objout, double objwgt,
                                              QString* errorMessage) const
{
  if (objtyp != ObjType::Ot_Min && objtyp != ObjType::Ot_Max) {
    if (errorMessage)
      *errorMessage = "objective type is invalid";
    return false;
  }

  if (objexe.trimmed().isEmpty()) {
    if (errorMessage)
      *errorMessage = "objective executable path cannot be empty";
    return false;
  }

  if (objout.trimmed().isEmpty()) {
    if (errorMessage)
      *errorMessage = "objective output filename cannot be empty";
    return false;
  }

  if (!isSafeOutputFilename(objout)) {
    if (errorMessage)
      *errorMessage = "objective output filename must be a simple relative filename";
    return false;
  }

  if (!GS_ISFINITE(objwgt) || objwgt < 0.0 || objwgt > 1.0) {
    if (errorMessage)
      *errorMessage = "objective weight should be a finite number in [0,1]";
    return false;
  }

  return true;
}

bool XtalOpt::validateConstraintDefinition(const QString& exe, const QString& out,
                                           QString* errorMessage) const
{
  if (exe.trimmed().isEmpty()) {
    if (errorMessage)
      *errorMessage = "constraint executable path cannot be empty";
    return false;
  }

  if (out.trimmed().isEmpty()) {
    if (errorMessage)
      *errorMessage = "constraint output filename cannot be empty";
    return false;
  }

  if (!isSafeOutputFilename(out)) {
    if (errorMessage)
      *errorMessage = "constraint output filename must be a simple relative filename";
    return false;
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
  processInputData();
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

QString XtalOpt::failActionText() const
{
  switch (getFailAction()) {
    case Search::SearchBase::FA_DoNothing:
      return "keepTrying";
    case Search::SearchBase::FA_KillIt:
      return "kill";
    case Search::SearchBase::FA_Randomize:
      return "replaceWithRandom";
    case Search::SearchBase::FA_NewOffspring:
      return "replaceWithOffspring";
  }
  return "unknown";
}

bool XtalOpt::setFailActionText(const QString& v)
{
  const QString n = v.trimmed().toLower();
  if (n == "keeptrying")
    setFailAction(Search::SearchBase::FA_DoNothing);
  else if (n == "kill")
    setFailAction(Search::SearchBase::FA_KillIt);
  else if (n == "replacewithrandom")
    setFailAction(Search::SearchBase::FA_Randomize);
  else if (n == "replacewithoffspring")
    setFailAction(Search::SearchBase::FA_NewOffspring);
  else
    return false;
  return true;
}

QString XtalOpt::optimizationTypeText() const
{
  switch (getOptimizationType()) {
    case Search::SearchBase::OT_Basic:
      return "basic";
    case Search::SearchBase::OT_Pareto:
      return "pareto";
  }
  return "unknown";
}

bool XtalOpt::setOptimizationTypeText(const QString& v)
{
  const QString n = v.trimmed().toLower();
  if (n == "basic")
    setOptimizationType(Search::SearchBase::OT_Basic);
  else if (n == "pareto")
    setOptimizationType(Search::SearchBase::OT_Pareto);
  else
    return false;
  return true;
}

QString XtalOpt::seedStructuresText() const
{
  return seedList().join(",");
}

void XtalOpt::setSeedStructuresText(const QString& v)
{
  seedList().clear();
  const QStringList parts = v.split(',');
  for (const QString& part : parts) {
    const QString trimmed = part.simplified();
    if (!trimmed.isEmpty())
      seedList().append(trimmed);
  }
}

// Return the repeated keywords' values.
QStringList XtalOpt::objectiveLines() const
{
  QStringList out;
  for (int i = 0; i < getUserObjectivesNum(); ++i)
    out << objectiveEntryToText(getUserObjectiveIndex(i));
  return out;
}

QStringList XtalOpt::constraintLines() const
{
  QStringList out;
  for (int i = 0; i < getConstraintsNum(); ++i)
    out << constraintEntryToText(i);
  return out;
}

// Return the custom IAD values. Both (a,b) and (b,a) for each pair are stored.
QStringList XtalOpt::customIADLines() const
{
  QStringList out;
  for (auto it = interComp().constBegin(); it != interComp().constEnd(); ++it)
    out << customIADEntryToText(it.key().first, it.key().second, it.value().minIAD);
  return out;
}

QStringList XtalOpt::molUnitLines() const
{
  return moleculeUnitInputs();
}

} // end namespace XtalOpt

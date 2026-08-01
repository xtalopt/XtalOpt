/**********************************************************************
  QueueManager - Generic queue manager to track running structures

  Copyright (C) 2010-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/queuemanager.h>

#include <common/compatibility/qt_compat.h>
#include <common/fileutils.h>
#include <common/output.h>
#include <common/timing.h>
#include <search/constants.h>
#include <search/search.h>
#include <search/optimizer.h>
#include <search/queueinterface.h>
#include <search/queueinterfaces/batch.h>
#include <search/structure.h>

#include <QDateTime>
#include <QDir>
#include <QSet>
#include <memory>
#include <QReadWriteLock>
#include <QThread>
#include <QThreadPool>
#include <QTimer>
#include <QtConcurrent>

// A couple helper functions/classes; disable doxygen parsing:
/// \cond
namespace {

void appendTrackerSnapshot(QList<Search::Structure*>& out, Search::Tracker* tracker)
{
  const QList<Search::Structure*>* source = tracker->list();
  out.reserve(out.size() + source->size());
  for (auto* structure : *source)
    out.append(structure);
}

QString objectiveOrConstraintFailureState(Search::Structure* s)
{
  if (s->getStrucConstraintState() == Search::Structure::Cs_Fail)
    return QString("constraint state %1").arg(s->getStrucConstraintState());
  return QString("objective state %1").arg(s->getStrucObjState());
}

QString objectiveOrConstraintDismissState(Search::Structure* s)
{
  if (s->getStrucConstraintState() == Search::Structure::Cs_Dismiss)
    return QString("constraint state %1").arg(s->getStrucConstraintState());
  return QString("objective state %1").arg(s->getStrucObjState());
}

QString objectiveOrConstraintDismissRedoReason(Search::Structure* s)
{
  if (s->getStrucConstraintState() == Search::Structure::Cs_Dismiss)
    return QObject::tr("dismissed by constraint calculation");
  return QObject::tr("failed objective calculation");
}

}
/// \endcond

namespace Search {

QueueManager::QueueManager(QThread* thread, SearchBase* srch)
  : QObject(), m_search(srch), m_thread(thread), m_tracker(srch->tracker()),
    m_requestedStructures(0), m_isDestroying(false)
{
  m_objectiveThreadPool.setMaxThreadCount(qMax(2, QThread::idealThreadCount()));
  moveToQMThread();
}

QueueManager::~QueueManager()
{
  m_isDestroying.store(true);
  this->disconnect();

  // Wait for running work to finish before its structures are deleted.
  while (!m_runningHandlers.waitForAll(STRUCTURE_HANDLER_WAIT_TIMEOUT))
    Common::warning("QueueManager: still waiting for running handlers...");

  // Wait for m_requestedStructures to == 0.
  {
    unsigned int timeout = 150;
    QtCompat::MutexLocker lock(&m_destroyMutex);
    while (timeout > 0 && m_requestedStructures > 0) {
      Common::debug("Waiting for structure generation threads to finish...");
      m_destroyWait.wait(&m_destroyMutex, 100);
      --timeout;
    }
  }
}

void QueueManager::prepareForThreadStop()
{
  m_isDestroying.store(true);
  QtCompat::MutexLocker lock(&m_destroyMutex);
  m_requestedStructures = 0;
  m_destroyWait.wakeAll();
}

void QueueManager::moveToQMThread()
{
  this->moveToThread(m_thread);

  // Set the queue connections before a structure can emit a signal.
  setupConnections();
  connect(this, &QueueManager::movedToQMThread,
          this, &QueueManager::startCheckLoop, Qt::QueuedConnection);

  emit movedToQMThread();
}

void QueueManager::setupConnections()
{
  // opt connections
  connect(this, &QueueManager::needNewStructure,
          m_search, &SearchBase::generateNewStructure, Qt::QueuedConnection);

  // re-emit connections
  connect(this, &QueueManager::structureStarted, this, &QueueManager::structureUpdated);
  connect(this, &QueueManager::structureSubmitted, this, &QueueManager::structureUpdated);
  connect(this, &QueueManager::structureKilled, this, &QueueManager::structureUpdated);
  connect(this, &QueueManager::structureFinished, this, &QueueManager::structureUpdated);
  // internal connections
  connect(this, &QueueManager::structureStarted, this,
          static_cast<void(QueueManager::*)(Search::Structure*)>(
              &QueueManager::addStructureToSubmissionQueue), Qt::QueuedConnection);
}

void QueueManager::startCheckLoop()
{
  QTimer::singleShot(0, this, &QueueManager::checkLoop);
  QTimer::singleShot(0, this, &QueueManager::submitQueuedJobs);
}


bool QueueManager::RunningHandlers::tryStart(Structure* s, int h)
{
  QtCompat::MutexLocker locker(&m_mutex);
  const QPair<Structure*, int> key(s, h);
  if (m_pairs.contains(key))
    return false;
  m_pairs.insert(key);
  ++m_perStructure[s];
  return true;
}

void QueueManager::RunningHandlers::finish(Structure* s, int h)
{
  QtCompat::MutexLocker locker(&m_mutex);
  const QPair<Structure*, int> key(s, h);
  if (!m_pairs.remove(key))
    return;
  if (--m_perStructure[s] <= 0)
    m_perStructure.remove(s);
  if (m_pairs.isEmpty())
    m_emptyWait.wakeAll();
}

bool QueueManager::RunningHandlers::hasHandlerFor(Structure* s) const
{
  QtCompat::MutexLocker locker(&m_mutex);
  return m_perStructure.contains(s);
}

bool QueueManager::RunningHandlers::hasHandler(Structure* s, int h) const
{
  QtCompat::MutexLocker locker(&m_mutex);
  return m_pairs.contains(QPair<Structure*, int>(s, h));
}

bool QueueManager::RunningHandlers::waitForAll(int timeoutMs)
{
  QtCompat::MutexLocker locker(&m_mutex);
  int remaining = timeoutMs;
  while (!m_pairs.isEmpty() && remaining > 0) {
    m_emptyWait.wait(&m_mutex, 100);
    remaining -= 100;
  }
  return m_pairs.isEmpty();
}

void QueueManager::RunningHandlers::clear()
{
  QtCompat::MutexLocker locker(&m_mutex);
  m_pairs.clear();
  m_perStructure.clear();
  m_emptyWait.wakeAll();
}

void QueueManager::runHandler(Structure* s, StructureHandlerType handler)
{
  // One worker per handler type, indexed by the enum value.
  typedef void (QueueManager::*HandlerWorker)(Structure*);
  static const HandlerWorker kWorkers[] = {
    &QueueManager::handleOptimizedStructure,
    &QueueManager::handleStepOptimizedStructure,
    &QueueManager::handleInProcessStructure,
    &QueueManager::handleErrorStructure,
    &QueueManager::handleSubmittedStructure,
    &QueueManager::handleKilledStructure,
    &QueueManager::handleRestartStructure,
    &QueueManager::handlePostprocessingStructure,
    &QueueManager::handleFailObjectiveStructure,
    &QueueManager::handleDismissObjectiveStructure,
  };
  static_assert(sizeof(kWorkers) / sizeof(kWorkers[0]) == SubmissionHandler,
                "kWorkers needs one entry per state function.");

  if (handler == SubmissionHandler) {
    addStructureToSubmissionQueue(s);
    return;
  }
  // These handlers do not have worker functions.
  if (handler >= SubmissionHandler)
    return;
  if (!m_runningHandlers.tryStart(s, handler))
    return;
  const HandlerWorker worker = kWorkers[handler];
  (void)QtConcurrent::run([this, s, handler, worker]() {
    (this->*worker)(s);
    m_runningHandlers.finish(s, handler);
  });
}

void QueueManager::reset()
{
  // Wait for running work to finish before resetting the structures.
  while (!m_runningHandlers.waitForAll(STRUCTURE_HANDLER_WAIT_TIMEOUT))
    Common::warning(QString("%1: still waiting for running handlers...").arg(__func__));

  QtCompat::MutexLocker namingLocker(&m_namingMutex);
  QList<Tracker*> trackers = allTrackers();

  for (auto it = trackers.begin(), it_end = trackers.end(); it != it_end; it++) {
    QWriteLocker locker((*it)->rwLock());
    (*it)->reset();
  }
  m_nextIdByGeneration.clear();
  m_runningHandlers.clear();
  {
    QtCompat::MutexLocker waitsLocker(&m_scriptWaitsMutex);
    m_scriptWaits.clear();
  }
}

void QueueManager::checkLoop()
{
  // Ensure that this is only called from the QM thread:
  Q_ASSERT_X(QThread::currentThread() == m_thread, Q_FUNC_INFO,
             "Attempting to run QueueManager::checkLoop "
             "from a thread other than the QM thread. ");

  if (m_isDestroying.load())
    return;

  if (!m_search->isReadOnly() && !m_search->isSessionStarting() && m_search->isSessionActive()) {
    {
      QReadLocker runtimeLocker(m_search->runtimeSettingsLock());
      if (m_search->isHardExit()) {
        checkExit();
        return;
      }
      checkPopulation();
      checkRunning();
    }
    // Exit without holding the settings lock, or waiting work can stop the exit.
    checkExit();
  }

  QTimer::singleShot(m_checkInterval, this, &QueueManager::checkLoop);
}

void QueueManager::checkPopulation()
{
  Common::ScopedTimer _timer("QueueManager::checkPopulation");
  // Count jobs
  uint running = 0;
  uint optimized = 0;

  QReadLocker trackerReadLocker(m_tracker->rwLock());
  QList<Structure*> structures;
  appendTrackerSnapshot(structures, m_tracker);

  uint tot = structures.size();

  QList<Structure*> runningTrackerAdds;
  QList<Structure*> runningTrackerRemoves;
  runningTrackerAdds.reserve(structures.size());
  runningTrackerRemoves.reserve(structures.size());

  // Check to see that the number of running jobs is >= that specified:
  int fail = 0;
  for (auto* structure : structures) {
    QReadLocker structureLocker(&structure->lock());
    Structure::State state = structure->getStatus();
    if (Structure::isStoppedFinalState(state))
      ++fail;
    structureLocker.unlock();

    // Count running jobs and update trackers
    if (!Structure::isQueueTerminalState(state)) {
      runningTrackerAdds.append(structure);
      ++running;
    } else {
      if (Structure::isOptimizedState(state))
        ++optimized;
      runningTrackerRemoves.append(structure);
    }
  }
  trackerReadLocker.unlock();

  // Update the running structures in one pass.
  {
    QWriteLocker runningTrackerLocker(m_runningTracker.rwLock());
    for (auto* structure : runningTrackerAdds)
      m_runningTracker.append(structure);
    for (auto* structure : runningTrackerRemoves)
      m_runningTracker.remove(structure);
  }

  emit newStatusOverview(optimized, running, fail, tot);

  // Count the running jobs before taking the tracker lock.
  int runningCount = 0;
  {
    QReadLocker runningLocker(m_runningTracker.rwLock());
    runningCount = m_runningTracker.size();
  }

  // Generate requests
  // Write lock for m_requestedStructures var
  Common::ScopedTimer _lockWait("QueueManager::checkPopulation:trackerWriteLockWait");
  QWriteLocker trackerWriteLocker(m_tracker->rwLock());
  _lockWait.stop();
  QReadLocker newStructureTrackerLocker(m_newStructureTracker.rwLock());

  // Avoid convenience function calls here; we already hold the relevant
  // tracker locks.
  int requestedStructures = 0;
  {
    QtCompat::MutexLocker requestedLocker(&m_destroyMutex);
    requestedStructures = m_requestedStructures;
  }

  int total = m_tracker->size() + m_newStructureTracker.size() + requestedStructures;
  // incomplete is getAllRunningStructures.size() + m_requestedStructures.
  int incomplete = runningCount + m_newStructureTracker.size() + requestedStructures;
  int needed = m_search->getContStructs() - incomplete;
  needed = qMin(needed, m_search->getMaxNumStructures() - total);

  if (
    // Are we at the continuous structure limit?
    (needed > 0) &&
    // Are we below the total structure limit?
    (total < m_search->getMaxNumStructures())) {
    // emit requests
    Common::debug(QString("Need %1 structures. %2 already incomplete.")
                      .arg(needed)
                      .arg(incomplete));
    for (int i = 0; i < needed; ++i) {
      int totalRequested = 0;
      {
        QtCompat::MutexLocker requestedLocker(&m_destroyMutex);
        if (m_isDestroying.load())
          break;
        totalRequested = ++m_requestedStructures;
      }
      emit needNewStructure();
      Common::debug(QString("Requested new structure. Total requested: %1")
                        .arg(totalRequested));
    }
  }
}

void QueueManager::submitQueuedJobs()
{
  Q_ASSERT_X(QThread::currentThread() == m_thread, Q_FUNC_INFO,
             "Attempting to submit jobs from outside the QueueManager thread.");

  if (m_isDestroying.load())
    return;

  bool hasQueuedJobs = false;
  {
    QReadLocker jobStartTrackerLocker(m_jobStartTracker.rwLock());
    hasQueuedJobs = m_jobStartTracker.size() != 0;
  }

  if (m_search->isReadOnly() || m_search->isSessionStarting() ||
      !m_search->isSessionActive() || !hasQueuedJobs) {
    QTimer::singleShot(SUBMISSION_MIN_GAP_MS,
                       this, &QueueManager::submitQueuedJobs);
    return;
  }

  // Hold the settings lock only while deciding what to submit
  QList<Structure*> toSubmit;
  {
    QReadLocker runtimeLocker(m_search->runtimeSettingsLock());

    uint submitted = 0;
    if (m_search->isLimitRunningJobs()) {
      QReadLocker runningTrackerLocker(m_runningTracker.rwLock());
      const QList<Structure*>* structures = m_runningTracker.list();
      for (Structure* structure : *structures) {
        QReadLocker structureLocker(&structure->lock());
        if (Structure::isQueueInProgressState(structure->getStatus()))
          ++submitted;
      }
    }

    bool batchSubmitted = false;
    QWriteLocker jobStartTrackerLocker(m_jobStartTracker.rwLock());
    while (m_jobStartTracker.size() != 0 &&
           (!m_search->isLimitRunningJobs() ||
            submitted < m_search->getRunningJobLimit())) {
      Structure* structure = m_jobStartTracker.at(0);
      int optStep = 0;
      Structure::State status = Structure::Empty;
      {
        QReadLocker structureLocker(&structure->lock());
        optStep = structure->getCurrentOptStep();
        status = structure->getStatus();
      }

      if (status != Structure::WaitingForOptimization) {
        Structure* discarded = nullptr;
        if (!m_jobStartTracker.popFirst(discarded))
          break;
        continue;
      }

      const bool isBatchJob = qobject_cast<BatchQueueInterface*>(
        m_search->queueInterface(optStep)) != nullptr;
      if (isBatchJob && batchSubmitted)
        break;

      Structure* popped = nullptr;
      if (!m_jobStartTracker.popFirst(popped))
        break;
      toSubmit.append(popped);
      ++submitted;
      if (isBatchJob)
        batchSubmitted = true;
    }
  }

  for (Structure* structure : toSubmit)
    startJob(structure);

  QTimer::singleShot(SUBMISSION_MIN_GAP_MS, this, &QueueManager::submitQueuedJobs);
}

void QueueManager::checkRunning()
{
  Common::ScopedTimer _timer("QueueManager::checkRunning");
  // Ensure that this is only called from the QM thread:
  Q_ASSERT_X(QThread::currentThread() == m_thread, Q_FUNC_INFO,
             "Attempting to run QueueManager::checkRunning "
             "from a thread other than the QM thread. ");

  // Get list of running structures
  QList<Structure*> runningStructures = getAllRunningStructures();

  // iterate over all structures and handle each based on its status
  for (auto s_it = runningStructures.begin(), s_it_end = runningStructures.end();
       s_it != s_it_end; ++s_it) {

    // Assign pointer for convenience
    Structure* structure = *s_it;

    // Skip structures that already have a function running.
    if (m_runningHandlers.hasHandlerFor(structure)) {
      continue;
    }

    // Lookup status
    Structure::State status;
    {
      QReadLocker structLocker(&structure->lock());
      status = structure->getStatus();
    }
    // Check the structure state before the exact state table.
    if (Structure::isFailedFinalState(status)) {
      runHandler(structure, ObjectiveFailHandler);
      continue;
    }
    if (Structure::isDismissedFinalState(status)) {
      runHandler(structure, ObjectiveDismissHandler);
      continue;
    }
    if (Structure::isPostOptimizationCalculationState(status)) {
      // Check for objc/const script outputs
      watchScriptCalculation(structure, status == Structure::ConstraintCalculation);
      continue;
    }

    // Select the state function. States not listed here need no work.
    //  - WaitingForOptimization: waiting on the submission queue,
    //  - Updating: an optimizer update is already running,
    //  - Empty: shouldn't be in the running list,
    //  - Optimized: handled by onPostprocessing, and may already be off the
    //    running list by the time checkRunning sees it.
    struct StateActionRow
    {
      Structure::State state;
      StructureHandlerType state_handler;
    };
    static const StateActionRow kStateActions[] = {
      { Structure::InProcess,      InProcessHandler },
      { Structure::StepOptimized,  StepOptimizedHandler },
      { Structure::Error,          ErrorHandler },
      { Structure::Submitted,      SubmittedHandler },
      { Structure::Killed,         KilledHandler },
      { Structure::Removed,        KilledHandler },
      { Structure::Restart,        RestartHandler },
      { Structure::Postprocessing, PostprocessingHandler },
    };

    for (const auto& row : kStateActions) {
      if (row.state == status) {
        runHandler(structure, row.state_handler);
        break;
      }
    }
  }

  // If structure is not running; clear the time bookkeeping.
  {
    QtCompat::MutexLocker waitsLocker(&m_scriptWaitsMutex);
    if (!m_scriptWaits.isEmpty()) {
      // Build the set with a plain loop: the QSet iterator-range
      //   constructor needs Qt 5.14.
      QSet<Structure*> running;
      running.reserve(runningStructures.size());
      for (Structure* structure : runningStructures)
        running.insert(structure);
      for (auto it = m_scriptWaits.begin(); it != m_scriptWaits.end();) {
        if (!running.contains(it.key()))
          it = m_scriptWaits.erase(it);
        else
          ++it;
      }
    }
  }

  return;
}

void QueueManager::startScriptCalculations(Structure* s, bool constraints)
{
  {
    QtCompat::MutexLocker waitsLocker(&m_scriptWaitsMutex);
    ScriptWaitInfo info;
    info.started = QDateTime::currentDateTime();
    info.nextCheck = info.started;
    m_scriptWaits.insert(s, info);
  }

  // The handler slot is held during the run and is released when it returns.
  (void)QtConcurrent::run(&m_objectiveThreadPool,
                          [this, s, constraints]() {
    bool ok = true;
    if (!m_search->isShuttingDown()) {
      if (constraints)
        ok = m_search->startConstraintCalculations(s);
      else
        ok = m_search->startObjectiveCalculations(s);
    }

    if (!ok) {
      const Structure::State state = constraints ? Structure::ConstraintCalculation : Structure::ObjectiveCalculation;
      QWriteLocker locker(&s->lock());
      if (s->getStatus() == state) {
        if (constraints)
          s->setStrucConstraintState(Structure::Cs_Fail);
        else
          s->setStrucObjState(Structure::Os_Fail);
        s->setStatus(Structure::Postprocessing);
        locker.unlock();
        emit structureUpdated(s);
      }
    }

    m_runningHandlers.finish(s, ScriptLaunchHandler);
  });
}

bool QueueManager::waitForScriptCalculations(int timeoutMs)
{
  return m_objectiveThreadPool.waitForDone(timeoutMs);
}

void QueueManager::watchScriptCalculation(Structure* s, bool constraints)
{
  const Structure::State status = constraints ? Structure::ConstraintCalculation : Structure::ObjectiveCalculation;
  const QDateTime now = QDateTime::currentDateTime();
  bool timedOut = false;
  bool checkDue = false;
  {
    QtCompat::MutexLocker waitsLocker(&m_scriptWaitsMutex);
    auto it = m_scriptWaits.find(s);
    if (it == m_scriptWaits.end()) {
      // Start the calculation's timer.
      ScriptWaitInfo info;
      info.started = now;
      info.nextCheck = now;
      it = m_scriptWaits.insert(s, info);
    }
    if (m_search->cancelScriptAfterTime() &&
        it->started.secsTo(now) >= static_cast<qint64>(m_search->hoursForCancelScriptAfterTime() * 3600.0)) {
      timedOut = true;
      m_scriptWaits.erase(it);
    } else if (now >= it->nextCheck) {
      checkDue = true;
      // Check the output files: use delay only for remote runs
      const int delaySec = m_search->isRemoteQueue() ? qMax(1, m_search->queueRefreshInterval()) : 0;
      it->nextCheck = now.addSecs(delaySec);
    }
  }

  if (timedOut) {
    Common::error(tr("%1 calculations for %2 timed out waiting for output file(s).")
                    .arg(status == Structure::ConstraintCalculation ? tr("Constraint") : tr("Objective"))
                    .arg(s->getTag()));
    {
      QWriteLocker locker(&s->lock());
      if (s->getStatus() != status)
        return;
      if (status == Structure::ConstraintCalculation)
        s->setStrucConstraintState(Structure::Cs_Fail);
      else
        s->setStrucObjState(Structure::Os_Fail);
      s->setStatus(Structure::Postprocessing);
    }
    runHandler(s, PostprocessingHandler);
    return;
  }

  if (checkDue)
    runHandler(s, PostprocessingHandler);
}

void QueueManager::clearScriptWait(Structure* s)
{
  QtCompat::MutexLocker waitsLocker(&m_scriptWaitsMutex);
  m_scriptWaits.remove(s);
}

void QueueManager::checkExit()
{
  // If either hardExit or softExit flags are set, this function calls
  //   the performTheExit function. For a hard exit, the code will quit
  //   immediately. For a soft exit, it will check if there is no any
  //   running/pending jobs, then waits a few second to make sure all
  //   files are transferred and status' are saved before quitting.

  if (! (m_search->isHardExit() || m_search->isSoftExit())) {
    // If no hard or soft exit, return.
    return;
  } else if (m_search->isHardExit()) {
    // If hard exit.
    Common::message(tr("\nPerforming a hard exit ..."));
    m_search->performTheExit();
  } else {
    // If not a hard exit; then we're here for a soft exit.
    int total = 0;
    int pending = 0;
    int maxStructures = 0;
    {
      QReadLocker runtimeLocker(m_search->runtimeSettingsLock());
      maxStructures = m_search->getMaxNumStructures();
    }

    QReadLocker trackerReadLocker(m_tracker->rwLock());
    QList<Structure*> struct2;
    appendTrackerSnapshot(struct2, m_tracker);
    for (auto* str2 : struct2)
    {
      QReadLocker structureLocker(&str2->lock());
      Structure::State st = str2->getStatus();
      structureLocker.unlock();
      if (!Structure::isQueueTerminalState(st))
        ++pending;
      else
        ++total;
    }
    trackerReadLocker.unlock();

    if (pending == 0 && total >= maxStructures) {
      // Do not announce the exit while state workers are still running.
      if (!m_runningHandlers.waitForAll(0))
        return;

      // Wait for all running async tasks (similarity checks, hull updates,
      // saves) to finish before announcing exit, so their output doesn't
      // appear after the warning.
      QThreadPool::globalInstance()->waitForDone(-1);
      Common::message(tr("\nPerforming a soft exit (total, finished, and "
                           "pending runs: %1 , %2 , %3)")
                          .arg(maxStructures)
                          .arg(total)
                          .arg(pending));
      // Call the perform_the_exit with a delay in quitting to make
      //   sure all output files are transferred/written.
      m_search->performTheExit(SOFT_EXIT_GRACE_S);
    }
  }
}


QList<Tracker*> QueueManager::allTrackers()
{
  QList<Tracker*> trackers;
  trackers.append(&m_jobStartTracker);
  trackers.append(&m_runningTracker);
  trackers.append(&m_newStructureTracker);
  return trackers;
}



// Doxygen skip:
/// @cond
void QueueManager::handleInProcessStructure(Structure* s)
{
  int optStep = 0;
  {
    QReadLocker locker(&s->lock());
    if (s->getStatus() != Structure::InProcess)
      return;
    optStep = s->getCurrentOptStep();
  }

  bool cancelJob = false;
  double cancelJobHours = 0.0;
  {
    QReadLocker runtimeLocker(m_search->runtimeSettingsLock());
    cancelJob = m_search->cancelJobAfterTime();
    cancelJobHours = m_search->hoursForCancelJobAfterTime();
  }

  QueueInterface* qi = m_search->queueInterface(optStep);
  switch (qi->getStatus(s)) {
    case QueueInterface::Running:
    case QueueInterface::Queued:
    case QueueInterface::CommunicationError:
    case QueueInterface::Unknown:
    case QueueInterface::Pending:
    case QueueInterface::Started:
    {
      // Kill the structure if it has exceeded the allowable time.
      // This is applied to all local and batch local optimizations.
      double elapsedHours = 0.0;
      {
        QReadLocker structLocker(&s->lock());
        elapsedHours = s->getOptElapsedHours();
      }
      if (cancelJob && elapsedHours > cancelJobHours) {
        // This will emit structureKilled, which is then re-emitted as structureUpdated.
        killStructure(s);
        return;
      }
      // Nothing to do but wait
      break;
    }
    case QueueInterface::Success:
      updateStructure(s);
      break;
    case QueueInterface::Error:
      {
        QWriteLocker structLocker(&s->lock());
        if (s->getStatus() != Structure::InProcess)
          return;
        s->setStatus(Structure::Error);
      }
      emit structureUpdated(s);
      break;
  }

  return;
}
/// @endcond


// Doxygen skip:
/// @cond
void QueueManager::handleOptimizedStructure(Structure* s)
{
  {
    QReadLocker locker(&s->lock());
    if (s->getStatus() != Structure::Optimized)
      return;

    QString output;
    output += QString(" %1 ").arg(s->getEnthalpyPerAtom(), 20, 'f', 12);
    for (int i = 0; i < m_search->getObjectivesNum(); ++i) {
      if (i < s->getStrucObjNumber())
        output += QString(" %1 ").arg(s->getStrucObjValues(i), 20, 'f', 12);
    }
    // Constraint values are pass/fail (1/0), so a narrow field is enough.
    for (int i = 0; i < m_search->getConstraintsNum(); ++i) {
      if (i < s->getStrucConstraintNumber())
        output += QString(" %1 ").arg(s->getStrucConstraintValues(i), 4, 'f', 0);
    }
    output += QString(" %1   FINAL_OBJECTIVES").arg(s->getTag(), 8);
    Common::message(output + "\n");
  }

  stopJobAndRemoveFromRunning(s);

  emit structureFinished(s);
}
/// @endcond


// Doxygen skip:
/// @cond
void QueueManager::handleStepOptimizedStructure(Structure* s)
{
  QWriteLocker locker(&s->lock());

  // Validate assumptions
  if (s->getStatus() != Structure::StepOptimized) {
    return;
  }

  s->stopOptTimer();

  QString err;
  if (!m_search->checkStepOptimizedStructure(s, &err)) {
    // Structure failed a post optimization step:
    Common::warning(QString("Structure %1 failed a post-optimization step: %2")
                     .arg(s->getTag())
                     .arg(err));
    s->setStatus(Structure::Killed);
    locker.unlock();
    emit structureUpdated(s);
    return;
  }

  // update optstep and relaunch if necessary
  if (s->getCurrentOptStep() + 1 <
      static_cast<unsigned int>(m_search->getNumOptSteps())) {

    Common::message(tr("Structure %1 completed opt step %2")
                      .arg(s->getTag())
                      .arg(s->getCurrentOptStep() + 1));

    s->setCurrentOptStep(s->getCurrentOptStep() + 1);

    // Update status
    s->setStatus(Structure::WaitingForOptimization);
    locker.unlock();
    {
      QWriteLocker runningLocker(m_runningTracker.rwLock());
      m_runningTracker.append(s);
    }
    emit structureUpdated(s);
    addStructureToSubmissionQueue(s);
    return;
  }
  // Otherwise, it's done
  else {
    s->setStatus(Structure::Postprocessing);
    locker.unlock();
    runHandler(s, PostprocessingHandler);
    return;
  }
}
/// @endcond

// Doxygen skip:
/// @cond
void QueueManager::handlePostprocessingStructure(Structure* s)
{
  QWriteLocker locker(&s->lock());

  // Objc/const calculation states
  const Structure::State entryStatus = s->getStatus();
  if (Structure::isPostOptimizationCalculationState(entryStatus)) {
    locker.unlock();
    const bool finished =
      (entryStatus == Structure::ConstraintCalculation)
        ? m_search->finishConstraintCalculations(s)
        : m_search->finishObjectiveCalculations(s);
    if (!finished)
      return;
    locker.relock();
    if (s->getStatus() != entryStatus)
      return;
    s->setStatus(Structure::Postprocessing);
  } else if (entryStatus != Structure::Postprocessing) {
    return;
  }

  if (m_search->getConstraintsNum() > 0) {
    if (s->getStrucConstraintState() == Structure::Cs_NotCalculated) {
      locker.unlock();
      // If a previous script launch has not returned yet; stay in post-processing and retry.
      if (!m_runningHandlers.tryStart(s, ScriptLaunchHandler))
        return;
      // Just remove any existing file from a previous run.
      if (!m_search->removeOldScriptOutputs(s, true)) {
        m_runningHandlers.finish(s, ScriptLaunchHandler);
        return;
      }
      locker.relock();
      if (s->getStatus() != Structure::Postprocessing ||
          s->getStrucConstraintState() != Structure::Cs_NotCalculated) {
        m_runningHandlers.finish(s, ScriptLaunchHandler);
        return;
      }
      s->resetStrucConstraint();
      clearScriptWait(s);
      s->setStatus(Structure::ConstraintCalculation);
      locker.unlock();
      emit structureUpdated(s);
      startScriptCalculations(s, true);
      return;
    }

    if (s->getStrucConstraintState() == Structure::Cs_Dismiss) {
      s->setStatus(Structure::Dismissed);
      locker.unlock();
      runHandler(s, ObjectiveDismissHandler);
      return;
    }

    if (s->getStrucConstraintState() == Structure::Cs_Fail) {
      s->setStatus(Structure::ConsFailed);
      locker.unlock();
      runHandler(s, ObjectiveFailHandler);
      return;
    }
  }

  if (m_search->hasExternalObjectiveCalculations()) {
    if (s->getStrucObjState() == Structure::Os_NotCalculated) {
      locker.unlock();
      // If a previous script launch has not returned yet; stay in post-processing and retry.
      if (!m_runningHandlers.tryStart(s, ScriptLaunchHandler))
        return;
      // Delete remaining files from any earlier calcs before making
      //   structure ready for adding to queue etc.
      if (!m_search->removeOldScriptOutputs(s, false)) {
        m_runningHandlers.finish(s, ScriptLaunchHandler);
        return;
      }
      locker.relock();
      if (s->getStatus() != Structure::Postprocessing ||
          s->getStrucObjState() != Structure::Os_NotCalculated) {
        m_runningHandlers.finish(s, ScriptLaunchHandler);
        return;
      }
      s->resetStrucObj();
      clearScriptWait(s);
      s->setStatus(Structure::ObjectiveCalculation);
      locker.unlock();
      emit structureUpdated(s);
      startScriptCalculations(s, false);
      return;
    }

    if (s->getStrucObjState() == Structure::Os_Fail) {
      s->setStatus(Structure::ObjcFailed);
      locker.unlock();
      runHandler(s, ObjectiveFailHandler);
      return;
    }

    if (s->getStrucObjState() == Structure::Os_Dismiss) {
      s->setStatus(Structure::Dismissed);
      locker.unlock();
      runHandler(s, ObjectiveDismissHandler);
      return;
    }
  }

  Common::message(tr("Structure %1 is optimized!").arg(s->getTag()));

  s->setStatus(Structure::Optimized);
  locker.unlock();
  runHandler(s, OptimizedHandler);
}
/// @endcond


// Doxygen skip:
/// @cond
void QueueManager::handleFailObjectiveStructure(Structure* s)
{
  QWriteLocker locker(&s->lock());

  // Revalidate assumptions
  if (!Structure::isFailedFinalState(s->getStatus()))
    return;

  Common::error(tr("Postprocessing Fail (%1): removing the structure %2 ")
      .arg(objectiveOrConstraintFailureState(s)).arg(s->getTag()));

  // Release the structure lock before stopping the job: stopJob() may make
  // blocking invokes into the QM thread, which takes per-structure locks.
  locker.unlock();
  stopJobAndRemoveFromRunning(s);
  emit structureKilled(s);
}
/// @endcond


// Doxygen skip:
/// @cond
void QueueManager::handleDismissObjectiveStructure(Structure* s)
{
  bool constraintsReDo = false;
  bool verbose = false;
  SearchBase::FailActions failAction = SearchBase::FA_DoNothing;
  {
    QReadLocker runtimeLocker(m_search->runtimeSettingsLock());
    constraintsReDo = m_search->isConstraintsReDo();
    verbose = m_search->isVerbose();
    failAction = m_search->getFailAction();
  }

  QWriteLocker locker(&s->lock());

  // Revalidate assumptions
  if (s->getStatus() != Structure::Dismissed)
    return;

  if (constraintsReDo && s->getStrucConstraintRedoCount() == 0 &&
      (failAction == SearchBase::FA_Randomize ||
       failAction == SearchBase::FA_NewOffspring))
  {
    if (verbose) {
      QString outstr;
      outstr = QString("   Redo struc %1 with %2 ( action = %3 ) !")
          .arg(s->getTag(), 8)
          .arg(objectiveOrConstraintDismissState(s))
          .arg(static_cast<int>(failAction), 3);
      Common::message(outstr);
    }

    // Record the redo; each structure is given at most one redo.
    s->setStrucConstraintRedoCount(s->getStrucConstraintRedoCount() + 1);
    if (failAction == SearchBase::FA_Randomize) {
      locker.unlock();
      replaceStructureForRestart(
        s, SearchBase::FA_Randomize, objectiveOrConstraintDismissRedoReason(s),
        Structure::Dismissed, tr("Structure %1 could not be replaced after objective dismissal."));
      return;
    } else if (failAction == SearchBase::FA_NewOffspring) {
      locker.unlock();
      replaceStructureForRestart(
        s, SearchBase::FA_NewOffspring, objectiveOrConstraintDismissRedoReason(s),
        Structure::Dismissed,
        tr("Structure %1 could not be replaced with offspring after objective dismissal."));
      return;
    }
  }

  // Except than the above cases; we just dismiss the structure
  /*
  Common::warning(tr("Postprocessing Dismiss (%1): removing the structure %2 ")
      .arg(objectiveOrConstraintDismissState(s)).arg(s->getTag()));
  */

  s->setStatus(Structure::Dismissed);

  // Release the structure lock before stopping the job: stopJob() may make
  // blocking invokes into the QM thread, which takes per-structure locks.
  locker.unlock();
  stopJobAndRemoveFromRunning(s);
  emit structureKilled(s);
}
/// @endcond





// Doxygen skip:
/// @cond
void QueueManager::handleErrorStructure(Structure* s)
{
  if (s->getStatus() != Structure::Error) {
    return;
  }

  Common::warning(tr("Structure %1 failed").arg(s->getTag()));

  uint failLimit = 0;
  SearchBase::FailActions failAction = SearchBase::FA_DoNothing;
  {
    QReadLocker runtimeLocker(m_search->runtimeSettingsLock());
    failLimit = m_search->getFailLimit();
    failAction = m_search->getFailAction();
  }

  if (!stopJob(s)) {
    Common::warning(tr("Structure %1 remains in the error state because its "
                       "scheduler job could not be cancelled.").arg(s->getTag()));
    return;
  }

  // Lock for writing
  QWriteLocker locker(&s->lock());
  if (s->getStatus() != Structure::Error)
    return;

  s->addFailure();

  // If the number of failures has exceed the limit, take
  // appropriate action
  if (s->getFailCount() >= failLimit) {
    switch (failAction) {
      case SearchBase::FA_DoNothing:
      default:
        break;
      case SearchBase::FA_KillIt:
        locker.unlock();
        // This will emit structureKilled, which is then re-emitted as structureUpdated.
        killStructure(s);
        return;
      case SearchBase::FA_Randomize:
        locker.unlock();
        replaceStructureForRestart(s, SearchBase::FA_Randomize, tr("failures: random"),
          Structure::Killed, tr("Structure %1 could not be replaced after failure."));
        return;
      case SearchBase::FA_NewOffspring:
        locker.unlock();
        replaceStructureForRestart(s, SearchBase::FA_NewOffspring, tr("failures: offspring"),
          Structure::Killed,
          tr("Structure %1 could not be replaced with offspring after failure."));
        return;
    }
  }

  s->setStatus(Structure::Restart);
  locker.unlock();
  emit structureUpdated(s);
}
/// @endcond

void QueueManager::replaceStructureForRestart(Structure* s, int action, const QString& reason,
  int failureState, const QString& failureMessage)
{
  {
    QWriteLocker replacementLocker(&s->lock());
    s->setStatus(Structure::Empty);
  }

  bool replacementSucceeded = false;
  if (action == SearchBase::FA_Randomize) {
    replacementSucceeded = (m_search->replaceWithRandom(s, reason) != nullptr);
  } else if (action == SearchBase::FA_NewOffspring) {
    replacementSucceeded = (m_search->replaceWithOffspring(s, reason) != nullptr);
  }

  if (!replacementSucceeded) {
    Common::warning(failureMessage.arg(s->getTag()));
    QWriteLocker replacementLocker(&s->lock());
    s->setStatus(static_cast<Structure::State>(failureState));
    replacementLocker.unlock();
    // structureKilled is re-emitted as structureUpdated.
    emit structureKilled(s);
    return;
  }

  {
    QWriteLocker replacementLocker(&s->lock());
    s->setStatus(Structure::Restart);
  }
  emit structureUpdated(s);
}


// Doxygen skip:
/// @cond
void QueueManager::handleSubmittedStructure(Structure* s)
{
  int optStep = 0;
  {
    QReadLocker locker(&s->lock());
    if (s->getStatus() != Structure::Submitted)
      return;
    optStep = s->getCurrentOptStep();
  }

  QueueInterface* queue = m_search->queueInterface(optStep);
  if (!queue) {
    QWriteLocker locker(&s->lock());
    if (s->getStatus() == Structure::Submitted)
      s->setStatus(Structure::Error);
    locker.unlock();
    emit structureUpdated(s);
    return;
  }

  switch (queue->getStatus(s)) {
    case QueueInterface::Running:
    case QueueInterface::Queued:
    case QueueInterface::Success:
    case QueueInterface::Started:
      // Update the structure as "InProcess"
      {
        QWriteLocker structLocker(&s->lock());
        if (s->getStatus() != Structure::Submitted)
          return;
        s->setStatus(Structure::InProcess);
      }
      emit structureUpdated(s);
      break;
    case QueueInterface::Error:
      {
        QWriteLocker structLocker(&s->lock());
        if (s->getStatus() != Structure::Submitted)
          return;
        s->setStatus(Structure::Restart);
      }
      emit structureUpdated(s);
      break;
    case QueueInterface::CommunicationError:
    case QueueInterface::Unknown:
    case QueueInterface::Pending:
    default:
      // nothing to do but wait
      break;
  }
}
/// @endcond


// Doxygen skip:
/// @cond
void QueueManager::handleKilledStructure(Structure* s)
{
  // Removed structures end up here, too; see handleRemovedStructure below.
  {
    QReadLocker locker(&s->lock());
    if (!s->isKilledOrRemovedState())
      return;
    if (m_runningHandlers.hasHandler(s, SubmissionHandler) ||
        m_runningHandlers.hasHandler(s, JobStartHandler) ||
        m_runningHandlers.hasHandler(s, KillRequestHandler))
      return;
  }

  stopJobAndRemoveFromRunning(s);
}
/// @endcond



// Doxygen skip:
/// @cond
void QueueManager::handleRestartStructure(Structure* s)
{
  if (s->getStatus() != Structure::Restart) {
    return;
  }

  if (!stopJob(s)) {
    Common::warning(tr("Structure %1 cannot restart until its scheduler job "
                       "has been cancelled.").arg(s->getTag()));
    return;
  }

  addStructureToSubmissionQueue(s);
}

void QueueManager::updateStructure(Structure* s)
{
  Common::ScopedTimer _timer("QueueManager::updateStructure");
  int optStep = 0;
  {
    QWriteLocker structLocker(&s->lock());
    if (s->getStatus() != Structure::InProcess)
      return;
    optStep = s->getCurrentOptStep();
    s->stopOptTimer();
    s->setStatus(Structure::Updating);
  }
  Optimizer* optimizer = m_search->optimizer(optStep);
  if (!optimizer || !optimizer->update(s)) {
    QWriteLocker structLocker(&s->lock());
    if (s->getStatus() != Structure::Updating)
      return;
    s->setStatus(Structure::Error);
    structLocker.unlock();
    emit structureUpdated(s);
    return;
  }
  {
    QWriteLocker structLocker(&s->lock());
    if (s->getStatus() != Structure::Updating)
      return;
    s->resetFailCount();
    s->setStatus(Structure::StepOptimized);
  }
  emit structureUpdated(s);
  return;
}
/// @endcond

void QueueManager::killStructure(Structure* s)
{
  // Don't kill the same structure twice at the same time.
  if (!m_runningHandlers.tryStart(s, KillRequestHandler))
    return;

  bool submitInProgress = false;

  // A running job becomes Killed; an optimized one becomes Removed.
  {
    QWriteLocker structLocker(&s->lock());
    if (s->isStoppedFinalState()) {
      structLocker.unlock();
      m_runningHandlers.finish(s, KillRequestHandler);
      return;
    }
    s->stopOptTimer();
    s->setStatus(s->getStatus() != Structure::Optimized ? Structure::Killed : Structure::Removed);
    submitInProgress =
      m_runningHandlers.hasHandler(s, SubmissionHandler) ||
      m_runningHandlers.hasHandler(s, JobStartHandler);
  }
  {
    QWriteLocker jobStartLocker(m_jobStartTracker.rwLock());
    m_jobStartTracker.remove(s);
  }

  bool stopped = true;
  if (!submitInProgress)
    stopped = stopJob(s);

  bool restored = false;
  if (!stopped) {
    QWriteLocker structLocker(&s->lock());
    if (s->getStatus() == Structure::Killed && s->getJobID() != 0) {
      s->setStatus(Structure::Submitted);
      s->setOptTimerEnd(QDateTime());
      restored = true;
    }
  }

  if (restored) {
    Common::warning(tr("Structure %1 was not killed because the scheduler "
                       "cancellation failed. The job will remain tracked.").arg(s->getTag()));
    emit structureSubmitted(s);
  } else {
    emit structureKilled(s);
  }

  m_runningHandlers.finish(s, KillRequestHandler);
}

void QueueManager::addStructureToSubmissionQueue(Structure* s, int optStep)
{
  if (!m_runningHandlers.tryStart(s, SubmissionHandler))
    return;

  (void)QtConcurrent::run([this, s, optStep]() {
    this->handleSubmissionStructure(s, optStep);
  });
}

// Doxygen skip:
/// @cond
void QueueManager::handleSubmissionStructure(Structure* s, int optStep)
{
  bool emitError = false;
  bool cleanStoppedStructure = false;

  // Update structure.
  {
    QWriteLocker structLocker(&s->lock());
    const Structure::State status = s->getStatus();
    if (status != Structure::Empty &&
        status != Structure::WaitingForOptimization &&
        status != Structure::Restart) {
      cleanStoppedStructure = s->isKilledOrRemovedState();
      m_runningHandlers.finish(s, SubmissionHandler);
      structLocker.unlock();
      if (cleanStoppedStructure)
        stopJob(s);
      return;
    }
    s->setStatus(Structure::WaitingForOptimization);
    if (optStep != -1)
      s->setCurrentOptStep(optStep);
  }

  // Perform writing.
  QueueInterface* queue = m_search->queueInterface(s->getCurrentOptStep());
  if (!queue || !queue->writeInputFiles(s)) {
    QWriteLocker structLocker(&s->lock());
    if (s->getStatus() == Structure::WaitingForOptimization) {
      s->setStatus(Structure::Error);
      emitError = true;
    } else {
      cleanStoppedStructure = s->isKilledOrRemovedState();
    }
    m_runningHandlers.finish(s, SubmissionHandler);
    structLocker.unlock();
    if (cleanStoppedStructure)
      stopJob(s);
    if (emitError)
      emit structureUpdated(s);
    return;
  }

  // Add to the running tracker first.
  {
    QWriteLocker runningLocker(m_runningTracker.rwLock());
    m_runningTracker.append(s);
  }
  {
    QWriteLocker jobStartLocker(m_jobStartTracker.rwLock());
    m_jobStartTracker.append(s);
  }

  bool queued = false;
  {
    QWriteLocker structLocker(&s->lock());
    queued = s->getStatus() == Structure::WaitingForOptimization;
    cleanStoppedStructure = s->isKilledOrRemovedState();
    m_runningHandlers.finish(s, SubmissionHandler);
  }

  if (!queued) {
    QWriteLocker jobStartLocker(m_jobStartTracker.rwLock());
    m_jobStartTracker.remove(s);
    jobStartLocker.unlock();
    if (cleanStoppedStructure)
      stopJob(s);
    return;
  }

  emit structureUpdated(s);
}
/// @endcond

void QueueManager::startJob(Structure* s)
{
  Common::ScopedTimer _timer("QueueManager::startJob");

  QueueInterface* queue = nullptr;
  {
    QWriteLocker structLocker(&s->lock());
    if (s->getStatus() != Structure::WaitingForOptimization)
      return;
    if (!m_runningHandlers.tryStart(s, JobStartHandler))
      return;
    queue = m_search->queueInterface(s->getCurrentOptStep());
  }

  const bool started = queue->startJob(s);
  bool reliable = true;
  {
    QReadLocker structLocker(&s->lock());
    reliable = !qobject_cast<BatchQueueInterface*>(queue) || s->getJobID() != 0;
  }

  if (started && reliable) {
    bool submitted = false;
    bool killed = false;
    {
      QWriteLocker structLocker(&s->lock());
      if (s->getStatus() == Structure::WaitingForOptimization) {
        s->setStatus(Structure::Submitted);
        submitted = true;
        m_runningHandlers.finish(s, JobStartHandler);
      } else {
        killed = s->isKilledOrRemovedState();
        if (!killed)
          m_runningHandlers.finish(s, JobStartHandler);
      }
    }

    if (submitted) {
      QReadLocker locker(&s->lock());
      Common::message(tr("Structure %1 has been submitted opt step %2")
                        .arg(s->getTag()).arg(s->getCurrentOptStep() + 1));
      locker.unlock();
      emit structureSubmitted(s);
      return;
    }

    if (!killed)
      return;

    const bool cancelled = queue->stopJob(s);
    bool restored = false;
    {
      QWriteLocker structLocker(&s->lock());
      if (s->getStatus() == Structure::Killed && !cancelled && s->getJobID() != 0) {
        s->setStatus(Structure::Submitted);
        s->setOptTimerEnd(QDateTime());
        restored = true;
      }
      m_runningHandlers.finish(s, JobStartHandler);
    }
    if (restored) {
      Common::warning(tr("Structure %1 was not killed because the scheduler "
                         "cancellation failed. The job will remain tracked.").arg(s->getTag()));
      emit structureSubmitted(s);
    }
    return;
  }

  if (started && !reliable) {
    bool submitted = false;
    {
      QWriteLocker structLocker(&s->lock());
      if (s->getStatus() == Structure::WaitingForOptimization ||
          s->isKilledOrRemovedState()) {
        s->setStatus(Structure::Submitted);
        submitted = true;
      }
      m_runningHandlers.finish(s, JobStartHandler);
    }
    if (submitted)
      emit structureSubmitted(s);
    return;
  }

  bool emitError = false;
  bool cleanStoppedStructure = false;
  {
    QWriteLocker structLocker(&s->lock());
    if (s->getStatus() == Structure::WaitingForOptimization) {
      Common::error(tr("%1: job did not start successfully for structure %2 opt step %3.")
                       .arg(__func__).arg(s->getTag()).arg(s->getCurrentOptStep() + 1));
      s->setStatus(Structure::Error);
      emitError = true;
    } else {
      cleanStoppedStructure = s->isKilledOrRemovedState();
    }
    m_runningHandlers.finish(s, JobStartHandler);
  }

  if (cleanStoppedStructure)
    stopJob(s);
  if (emitError)
    emit structureUpdated(s);
}

bool QueueManager::stopJob(Structure* s)
{
  int optStep = 0;
  {
    QReadLocker locker(&s->lock());
    optStep = s->getCurrentOptStep();
  }
  QueueInterface* queue = m_search->queueInterface(optStep);
  if (!queue) {
    Common::error(tr("%1: structure %2 has invalid optimization step %3.")
                    .arg(__func__).arg(s->getTag()).arg(optStep + 1));
    return false;
  }
  return queue->stopJob(s);
}

void QueueManager::stopJobAndRemoveFromRunning(Structure* s)
{
  stopJob(s);
  QWriteLocker runningLocker(m_runningTracker.rwLock());
  m_runningTracker.remove(s);
}

QList<Structure*> QueueManager::getAllRunningStructures()
{
  QReadLocker runningLocker(m_runningTracker.rwLock());
  QReadLocker newStructLocker(m_newStructureTracker.rwLock());
  QList<Structure*> list;
  appendTrackerSnapshot(list, &m_runningTracker);
  appendTrackerSnapshot(list, &m_newStructureTracker);
  return list;
}

QList<Structure*> QueueManager::getAllOptimizedStructures()
{
  QList<Structure*> list;
  QReadLocker trackerLocker(m_tracker->rwLock());
  for (auto* s : *m_tracker->list()) {
    if (!s)
      continue;

    QReadLocker structLocker(&s->lock());
    if (s->getStatus() == Structure::Optimized)
      list.append(s);
  }
  return list;
}

QList<Structure*> QueueManager::getAllSimilarStructures()
{
  QList<Structure*> list;
  QReadLocker trackerLocker(m_tracker->rwLock());
  for (auto* s : *m_tracker->list()) {
    if (!s)
      continue;

    QReadLocker structLocker(&s->lock());
    if (s->isSimilar())
      list.append(s);
  }
  return list;
}

QList<Structure*> QueueManager::getAllStructures()
{
  QReadLocker trackerLocker(m_tracker->rwLock());
  QReadLocker newStructLocker(m_newStructureTracker.rwLock());
  QList<Structure*> list;
  appendTrackerSnapshot(list, m_tracker);
  appendTrackerSnapshot(list, &m_newStructureTracker);
  return list;
}

QList<Structure*> QueueManager::lockForNaming()
{
  m_namingMutex.lock();
  m_tracker->lockForRead();
  m_newStructureTracker.lockForRead();
  QList<Structure*> list;
  appendTrackerSnapshot(list, m_tracker);
  appendTrackerSnapshot(list, &m_newStructureTracker);

  return list;
}

void QueueManager::addNewStructure(Structure* s, uint generation, const QString& parents)
{
  if (!s) {
    structureGenerationFailed();
    return;
  }

  QList<Structure*> allStructures = lockForNaming();
  const uint id = nextIdForGeneration(generation, allStructures);

  QWriteLocker locker(&s->lock());
  s->setIDNumber(id);
  s->setGeneration(generation);
  s->setParents(parents);

  const QString dirTag = s->getDirectoryTag();
  const QString locpath = Common::localPath(m_search->getLocWorkDir(), dirTag);
  const QString rempath = Common::remotePath(m_search->getRemWorkDir(), dirTag);

  QDir dir(locpath);
  if (!dir.exists() && !dir.mkpath(locpath)) {
    Common::error(QString("%1: cannot write to path: %2")
                    .arg(__func__)
                    .arg(locpath));
    s->setStatus(Structure::Error);
    locker.unlock();
    if (!m_search->isSessionStarting())
      structureGenerationFailed();
    m_newStructureTracker.unlock();
    m_tracker->unlock();
    m_namingMutex.unlock();
    delete s;
    return;
  }

  s->moveToThread(m_thread);
  s->setupConnections();
  s->setLocpath(locpath);
  s->setRempath(rempath);
  s->setCurrentOptStep(0);
  locker.unlock();

  unlockForNaming(s);
}

uint QueueManager::nextIdForGeneration(uint generation, const QList<Structure*>& allStructures)
{
  auto it = m_nextIdByGeneration.find(generation);
  if (it == m_nextIdByGeneration.end()) {
    uint nextId = 1;
    for (auto* structure : allStructures) {
      if (!structure)
        continue;

      QReadLocker locker(&structure->lock());
      if (structure->getGeneration() == generation && structure->getIDNumber() >= nextId) {
        nextId = structure->getIDNumber() + 1;
      }
    }
    it = m_nextIdByGeneration.insert(generation, nextId);
  }

  const uint id = it.value();
  it.value() = id + 1;
  return id;
}

void QueueManager::unlockForNaming(Structure* s)
{
  m_newStructureTracker.unlock();
  if (!s) {
    m_tracker->unlock();
    m_namingMutex.unlock();
    return;
  }

  // Discard structure if we're shutting down
  if (m_isDestroying.load()) {
    decrementRequestedStructures();
    m_tracker->unlock();
    m_namingMutex.unlock();
    Tracker::deleteStructure(s);
    return;
  }

  if (!m_search->isSessionStarting()) {
    decrementRequestedStructures();
  }

  // Append to tracker after decrementing
  // m_requestedStructures. This keeps behavior predictable during
  // session initialization.
  {
    QWriteLocker wl(m_newStructureTracker.rwLock());
    // the Tracker keeps this structure from here on
    m_newStructureTracker.append(s);

    {
      QtCompat::MutexLocker requestedLocker(&m_destroyMutex);
      Q_ASSERT_X(m_requestedStructures >= 0, Q_FUNC_INFO,
                 "The requested structures counter has become negative.");
    }

    Common::message(QString("New structure accepted (%1)").arg(s->getTag()));
  }
  m_tracker->unlock();
  m_namingMutex.unlock();

  // Make sure shutdown waits for this naming task.
  if (m_runningHandlers.tryStart(s, NamingHandler)) {
    (void)QtConcurrent::run([this, s]() {
      this->unlockForNaming_();
      m_runningHandlers.finish(s, NamingHandler);
    });
  }
}

void QueueManager::structureGenerationFailed()
{
  decrementRequestedStructures();
}

void QueueManager::decrementRequestedStructures()
{
  QtCompat::MutexLocker requestedLocker(&m_destroyMutex);
  if (m_requestedStructures > 0) {
    --m_requestedStructures;
  } else if (!m_isDestroying.load()) {
    Common::warning(QString("%1: structure request counter is already zero.")
                     .arg(__func__));
  }
  m_destroyWait.wakeAll();
}

// Doxygen skip:
/// @cond
void QueueManager::unlockForNaming_()
{
  Structure* s;
  bool optimized;
  {
    QWriteLocker trackerLocker(m_tracker->rwLock());
    QWriteLocker newStructLocker(m_newStructureTracker.rwLock());
    if (!m_newStructureTracker.popFirst(s)) {
      return;
    }

    // Update structure
    {
      QWriteLocker structLocker(&s->lock());
      optimized = s->getStatus() == Structure::Optimized;
      if (!optimized)
        s->setStatus(Structure::WaitingForOptimization);
    }

    m_tracker->append(s);
  }

  if (optimized)
    emit structureFinished(s);
  else
    emit structureStarted(s);
}
/// @endcond

void QueueManager::trackRestoredStructure(Structure* s, bool queueWaitingForOptimization)
{
  if (!s)
    return;

  Structure::State status;
  {
    QReadLocker structureLocker(&s->lock());
    status = s->getStatus();
  }

  if (queueWaitingForOptimization && status == Structure::WaitingForOptimization) {
    QWriteLocker jobStartLocker(m_jobStartTracker.rwLock());
    m_jobStartTracker.append(s);
  }

  if (!Structure::isQueueTerminalState(status)) {
    QWriteLocker runningLocker(m_runningTracker.rwLock());
    m_runningTracker.append(s);
  }
}

} // end namespace Search

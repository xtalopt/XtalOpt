/**********************************************************************
  QueueManager - Generic queue manager to track running structures

  Copyright (C) 2010-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <globalsearch/queuemanager.h>

#include <globalsearch/macros.h>
#include <globalsearch/searchbase.h>
#include <globalsearch/optimizer.h>
#include <globalsearch/queueinterface.h>
#include <globalsearch/queueinterfaces/remote.h>
#include <globalsearch/random.h>
#include <globalsearch/structure.h>

#include <QDateTime>
#include <QDebug>
#include <QTimer>
#include <QtConcurrent>

// A couple helper functions/classes -- disable doxygen parsing:
/// \cond
namespace {
class removeFromTrackerWhenScopeEnds
{
  GlobalSearch::Tracker* m_tracker;
  GlobalSearch::Structure* m_structure;

public:
  removeFromTrackerWhenScopeEnds(GlobalSearch::Structure* s,
                                 GlobalSearch::Tracker* t)
    : m_tracker(t), m_structure(s)
  {
  }
  ~removeFromTrackerWhenScopeEnds()
  {
    m_tracker->lockForWrite();
    m_tracker->remove(m_structure);
    m_tracker->unlock();
  }
};

// Locks tracker for reading and calls t->contains(s)
bool trackerContainsStructure(GlobalSearch::Structure* s,
                              GlobalSearch::Tracker* t)
{
  t->lockForRead();
  bool b = t->contains(s);
  t->unlock();
  return b;
}
}
/// \endcond

namespace GlobalSearch {

QueueManager::QueueManager(QThread* thread, SearchBase* srch)
  : QObject(), m_search(srch), m_thread(thread), m_tracker(srch->tracker()),
    m_requestedStructures(0), m_isDestroying(false),
    m_lastSubmissionTimeStamp(new QDateTime(QDateTime::currentDateTime()))
{
  moveToQMThread();
}

QueueManager::~QueueManager()
{
  m_isDestroying = true;
  this->disconnect();

  // Wait for handler trackers to empty.
  QList<Tracker*> trackers;
  trackers.append(&m_newlyOptimizedTracker);
  trackers.append(&m_stepOptimizedTracker);
  trackers.append(&m_inProcessTracker);
  trackers.append(&m_errorTracker);
  trackers.append(&m_submittedTracker);
  trackers.append(&m_newlyKilledTracker);
  trackers.append(&m_newSimilarityTracker);
  trackers.append(&m_restartTracker);
  trackers.append(&m_newSubmissionTracker);
  trackers.append(&m_objectiveRetainTracker);
  trackers.append(&m_objectiveFailTracker);
  trackers.append(&m_objectiveDismissTracker);

  // Used to break wait loops if they take too long
  unsigned int timeout;

  for (QList<Tracker *>::iterator it = trackers.begin(),
                                  it_end = trackers.end();
       it != it_end; it++) {
    timeout = 10;
    while (timeout > 0 && (*it)->size()) {
      qDebug() << "Spinning on QueueManager handler trackers to empty...";
      GS_SLEEP(1);
      --timeout;
    }
  }

  // Wait for m_requestedStructures to == 0
  timeout = 15;
  while (timeout > 0 && m_requestedStructures > 0) {
    qDebug() << "Waiting for structure generation threads to finish...";
    GS_SLEEP(1);
    --timeout;
  }

  delete m_lastSubmissionTimeStamp;
}

void QueueManager::moveToQMThread()
{
  this->moveToThread(m_thread);

  connect(this, SIGNAL(movedToQMThread()), this, SLOT(setupConnections()),
          Qt::QueuedConnection);

  emit movedToQMThread();
}

void QueueManager::setupConnections()
{
  // opt connections
  connect(this, SIGNAL(needNewStructure()), m_search, SLOT(generateNewStructure()),
          Qt::QueuedConnection);
  connect(m_search, SIGNAL(doneWithObjectives(Structure*)), this,
      SLOT(updateStructureObjectiveState(Structure*)));

  // re-emit connections
  connect(this, SIGNAL(structureStarted(GlobalSearch::Structure*)), this,
          SIGNAL(structureUpdated(GlobalSearch::Structure*)));
  connect(this, SIGNAL(structureSubmitted(GlobalSearch::Structure*)), this,
          SIGNAL(structureUpdated(GlobalSearch::Structure*)));
  connect(this, SIGNAL(structureKilled(GlobalSearch::Structure*)), this,
          SIGNAL(structureUpdated(GlobalSearch::Structure*)));
  connect(this, SIGNAL(structureFinished(GlobalSearch::Structure*)), this,
          SIGNAL(structureUpdated(GlobalSearch::Structure*)));
  // This helps to update cli results.txt properly.
  connect(this, SIGNAL(readyForObjectiveCalculations(GlobalSearch::Structure*)), this,
          SIGNAL(structureUpdated(GlobalSearch::Structure*)));

  // internal connections
  connect(this, SIGNAL(structureStarted(GlobalSearch::Structure*)), this,
          SLOT(addStructureToSubmissionQueue(GlobalSearch::Structure*)),
          Qt::QueuedConnection);

// Work around bug in Qt 4.6.3:
#if QT_VERSION == 0x040603
  connect(this, SIGNAL(newStructureQueued()), this, SLOT(unlockForNaming_()),
          Qt::QueuedConnection);
#endif // QT_VERSION == 4.6.3

  QTimer::singleShot(0, this, SLOT(checkLoop()));
}

void QueueManager::reset()
{
  QList<Tracker*> trackers;
  trackers.append(m_tracker);
  trackers.append(&m_jobStartTracker);
  trackers.append(&m_runningTracker);
  trackers.append(&m_newStructureTracker);
  trackers.append(&m_newlyOptimizedTracker);
  trackers.append(&m_stepOptimizedTracker);
  trackers.append(&m_inProcessTracker);
  trackers.append(&m_errorTracker);
  trackers.append(&m_submittedTracker);
  trackers.append(&m_newlyKilledTracker);
  trackers.append(&m_newSimilarityTracker);
  trackers.append(&m_restartTracker);
  trackers.append(&m_newSubmissionTracker);
  trackers.append(&m_objectiveRetainTracker);
  trackers.append(&m_objectiveFailTracker);
  trackers.append(&m_objectiveDismissTracker);

  for (QList<Tracker *>::iterator it = trackers.begin(),
                                  it_end = trackers.end();
       it != it_end; it++) {
    (*it)->lockForWrite();
    (*it)->reset();
    (*it)->unlock();
  }
}

void QueueManager::checkLoop()
{
  // Ensure that this is only called from the QM thread:
  Q_ASSERT_X(QThread::currentThread() == m_thread, Q_FUNC_INFO,
             "Attempting to run QueueManager::checkLoop "
             "from a thread other than the QM thread. ");

  // Update runtime options by reading a file if we are not using the GUI
  // This needs to be here first because sometimes, our CLI settings are
  // over-written from somewhere else when starting.
  if (!m_search->usingGUI())
    m_search->readRuntimeOptions();

  if (!m_search->readOnly && !m_search->isStarting) {
    checkPopulation();
    checkRunning();
    checkExit();
  }

  QTimer::singleShot(1000, this, SLOT(checkLoop()));
}

void QueueManager::checkPopulation()
{
  // Count jobs
  uint running = 0;
  uint optimized = 0;
  uint submitted = 0;

  QReadLocker trackerReadLocker(m_tracker->rwLock());
  QList<Structure*> structures = *m_tracker->list();

  uint tot = structures.size();

  // Check to see that the number of running jobs is >= that specified:
  int fail = 0;
  for (int i = 0; i < structures.size(); ++i) {
    Structure* structure = structures.at(i);
    QReadLocker structureLocker(&structure->lock());
    Structure::State state = structure->getStatus();
    if (structure->getFailCount() != 0)
      ++fail;
    structureLocker.unlock();

    QWriteLocker runningTrackerLocker(m_runningTracker.rwLock());
    // Count submitted structures
    if (state == Structure::Submitted || state == Structure::InProcess ||
        state == Structure::ObjectiveCalculation) {
      m_runningTracker.append(structure);
      ++submitted;
    }
    // Count running jobs and update trackers
    if (state != Structure::Optimized && state != Structure::Similar &&
        state != Structure::Killed &&
        state != Structure::Removed && state != Structure::ObjectiveFail &&
        state != Structure::ObjectiveDismiss) {
      m_runningTracker.append(structure);
      ++running;
    } else {
      if (state == Structure::Optimized)
        ++optimized;
      m_runningTracker.remove(structure);
    }
  }
  trackerReadLocker.unlock();
  emit newStatusOverview(optimized, running, fail, tot);

  // Submit any jobs if needed
  QWriteLocker jobStartTrackerLocker(m_jobStartTracker.rwLock());
  int pending = m_jobStartTracker.size();
  if (pending != 0 &&
      (!m_search->limitRunningJobs || submitted < m_search->runningJobLimit)) {
// Submit a single throttled job (1 submission per 3-8 seconds) if using
// a remote queue interface. Interval is randomly chosen each iteration.
// This prevents hammering the pbs server from multiple XtalOpt instances
// if there is a problem with the queue.
#ifdef ENABLE_SSH
    Structure* s = m_jobStartTracker.at(0);
    if (qobject_cast<RemoteQueueInterface*>(
          m_search->queueInterface(s->getCurrentOptStep())) != nullptr) {
      if (m_lastSubmissionTimeStamp->secsTo(QDateTime::currentDateTime()) >=
          3 + (6 * getRandDouble())) {
        startJob();
        ++submitted;
        --pending;
        *m_lastSubmissionTimeStamp = QDateTime::currentDateTime();
      }
    } else {
// Local job submission doesn't need to be throttled
#endif
      while (pending != 0 &&
             (!m_search->limitRunningJobs || submitted < m_search->runningJobLimit)) {
        startJob();
        ++submitted;
        --pending;
      }
#ifdef ENABLE_SSH
    }
#endif
  }
  jobStartTrackerLocker.unlock();

  // Generate requests
  // Write lock for m_requestedStructures var
  QWriteLocker trackerWriteLocker(m_tracker->rwLock());
  QReadLocker newStructureTrackerLocker(m_newStructureTracker.rwLock());

  // Avoid convience function calls here, as occaisional deadlocks
  // can occur.
  //
  // total is getAllStructures().size() + m_requestedStructures;
  int total =
    m_tracker->size() + m_newStructureTracker.size() + m_requestedStructures;
  // incomplete is getAllRunningStructures.size() + m_requestedStructures:
  int incomplete = m_runningTracker.size() + m_newStructureTracker.size() +
                   m_requestedStructures;
  int needed = m_search->contStructs - incomplete;

  if (
    // Are we at the continuous structure limit?
    (needed > 0) &&
    // Is the cutoff either disabled or reached/exceeded?
    (m_search->maxNumStructures <= 0 || total < m_search->maxNumStructures) &&
    // Check if we are testing. If so, have we reached the testing limit?
    (!m_search->testingMode || total < m_search->test_nStructs)) {
    // emit requests
    qDebug() << "Need " << needed << " structures. " << incomplete
             << " already incomplete.";
    for (int i = 0; i < needed; ++i) {
      ++m_requestedStructures;
      emit needNewStructure();
      qDebug() << "Requested new structure. Total requested: "
               << m_requestedStructures;
    }
  }
}

void QueueManager::checkRunning()
{
  // Ensure that this is only called from the QM thread:
  Q_ASSERT_X(QThread::currentThread() == m_thread, Q_FUNC_INFO,
             "Attempting to run QueueManager::checkRunning "
             "from a thread other than the QM thread. ");

  // Get list of running structures
  QList<Structure*> runningStructures = getAllRunningStructures();

  // iterate over all structures and handle each based on its status
  for (QList<Structure *>::iterator s_it = runningStructures.begin(),
                                    s_it_end = runningStructures.end();
       s_it != s_it_end; ++s_it) {

    // Assign pointer for convenience
    Structure* structure = *s_it;

    // Check if this structure has any handlers pending. Skip if so.
    if (m_newlyOptimizedTracker.contains(structure) ||
        m_stepOptimizedTracker.contains(structure) ||
        m_inProcessTracker.contains(structure) ||
        m_errorTracker.contains(structure) ||
        m_submittedTracker.contains(structure) ||
        m_newlyKilledTracker.contains(structure) ||
        m_newSimilarityTracker.contains(structure) ||
        m_restartTracker.contains(structure) ||
        m_newSubmissionTracker.contains(structure) ||
        m_objectiveRetainTracker.contains(structure) ||
        m_objectiveFailTracker.contains(structure) ||
        m_objectiveDismissTracker.contains(structure)) {
      continue;
    }

    // Lookup status
    structure->lock().lockForRead();
    Structure::State status = structure->getStatus();
    structure->lock().unlock();

    // Check status
    switch (status) {
      case Structure::InProcess:
        handleInProcessStructure(structure);
        break;
      case Structure::WaitingForOptimization:
        handleWaitingForOptimizationStructure(structure);
        break;
      case Structure::StepOptimized:
        handleStepOptimizedStructure(structure);
        break;
      case Structure::Optimized:
        // Shouldn't happen -- this is called by handleStepOptimizedStructure
        // when needed. There is a race condition between the check* functions
        // -- The structure may be removed from the list of running structures
        // by checkPopulation before checkRunning is called.
        // handleOptimizedStructure(structure);
        break;
      case Structure::Error:
        handleErrorStructure(structure);
        break;
      case Structure::Submitted:
        handleSubmittedStructure(structure);
        break;
      case Structure::Killed:
        handleKilledStructure(structure);
        break;
      case Structure::Removed:
        handleRemovedStructure(structure);
        break;
      case Structure::Restart:
        handleRestartStructure(structure);
        break;
      case Structure::Updating:
        handleUpdatingStructure(structure);
        break;
      case Structure::Similar:
        handleSimilarStructure(structure);
        break;
      case Structure::Empty:
        handleEmptyStructure(structure);
        break;
      case Structure::ObjectiveFail:
        handleFailObjective(structure);
        break;
      case Structure::ObjectiveDismiss:
        handleDismissObjective(structure);
        break;
      case Structure::ObjectiveRetain:
        handleRetainObjective(structure);
        break;
      case Structure::ObjectiveCalculation:
        // Nothing to be done! Wait for signal!
        break;
    }
  }

  return;
}

void QueueManager::checkExit()
{
  // If either hardExit or softExit flags are set, this function calls
  //   the performTheExit function. For a hard exit, the code will quit
  //   immediately. For a soft exit, it will check if there is no any
  //   running/pending jobs, then waits a few second to make sure all
  //   files are transferred and status' are saved before quitting.

  if (! (m_search->m_hardExit || m_search->m_softExit)) {
    // If no hard or soft exit, return.
    return;
  } else if (m_search->m_hardExit) {
    // If hard exit.
    m_search->warning(tr("Performing a hard exit ..."));
    m_search->performTheExit();
  } else {
    // If not a hard exit; then we're here for a soft exit.
    int total = 0;
    int pending = 0;

    QReadLocker trackerReadLocker(m_tracker->rwLock());
    QList<Structure*> struct2 = *m_tracker->list();
    for (int i = 0; i < struct2.size(); ++i)
    {
      Structure* str2 = struct2.at(i);
      QReadLocker structureLocker(&str2->lock());
      Structure::State st = str2->getStatus();
      structureLocker.unlock();
      if (st != Structure::Optimized   && st != Structure::Killed         &&
          st != Structure::ObjectiveFail && st != Structure::ObjectiveDismiss &&
          st != Structure::Removed     && st != Structure::Similar)
        ++pending;
      else
        ++total;
    }
    trackerReadLocker.unlock();

    if (pending == 0 && total >= m_search->maxNumStructures) {
      m_search->warning(
          tr("Performing a soft exit (total, finished, and pending runs: %1 , %2 , %3)")
          .arg(m_search->maxNumStructures).arg(total).arg(pending));
      // Call the perform_the_exit with a delay in quitting to make
      //   sure all output files are transferred/written.
      m_search->performTheExit(3);
    }
  }
}

void QueueManager::handleInProcessStructure(Structure* s)
{
  QWriteLocker locker(m_inProcessTracker.rwLock());
  if (!m_inProcessTracker.append(s)) {
    return;
  }
  QtConcurrent::run(this, &QueueManager::handleInProcessStructure_, s);
}

// Doxygen skip:
/// @cond
void QueueManager::handleInProcessStructure_(Structure* s)
{
  Q_ASSERT(trackerContainsStructure(s, &m_inProcessTracker));
  removeFromTrackerWhenScopeEnds popper(s, &m_inProcessTracker);

  // Revalidate assumptions
  if (s->getStatus() != Structure::InProcess) {
    return;
  }

  QueueInterface* qi = m_search->queueInterface(s->getCurrentOptStep());
  switch (qi->getStatus(s)) {
    case QueueInterface::Running:
    case QueueInterface::Queued:
    case QueueInterface::CommunicationError:
    case QueueInterface::Unknown:
    case QueueInterface::Pending:
    case QueueInterface::Started:
    {
      // Kill the structure if it has exceeded the allowable time.
      // Only perform this for remote queues.
      if (m_search->cancelJobAfterTime() &&
          s->getOptElapsedHours() > m_search->hoursForCancelJobAfterTime() &&
          qi->getIDString().toLower() != "local") {
        killStructure(s);
        emit structureUpdated(s);
        return;
      }
      // Nothing to do but wait
      break;
    }
    case QueueInterface::Success:
      updateStructure(s);
      break;
    case QueueInterface::Error:
      s->lock().lockForWrite();
      s->setStatus(Structure::Error);
      s->lock().unlock();
      emit structureUpdated(s);
      break;
  }

  return;
}
/// @endcond

void QueueManager::handleOptimizedStructure(Structure* s)
{
  QWriteLocker locker(m_newlyOptimizedTracker.rwLock());
  if (!m_newlyOptimizedTracker.append(s)) {
    return;
  }
  QtConcurrent::run(this, &QueueManager::handleOptimizedStructure_, s);
}

// Doxygen skip:
/// @cond
void QueueManager::handleOptimizedStructure_(Structure* s)
{
  Q_ASSERT(trackerContainsStructure(s, &m_newlyOptimizedTracker));
  removeFromTrackerWhenScopeEnds popper(s, &m_newlyOptimizedTracker);

  // Revalidate assumptions
  if (s->getStatus() != Structure::Optimized) {
    return;
  }

  // Ensure that the job is not tying up the queue
  stopJob(s);

  // Remove from running tracker
  m_runningTracker.lockForWrite();
  m_runningTracker.remove(s);
  m_runningTracker.unlock();

  emit structureFinished(s);
}
/// @endcond

void QueueManager::handleStepOptimizedStructure(Structure* s)
{
  QWriteLocker locker(m_stepOptimizedTracker.rwLock());
  m_stepOptimizedTracker.append(s);
  QtConcurrent::run(this, &QueueManager::handleStepOptimizedStructure_, s);
}

// Doxygen skip:
/// @cond
void QueueManager::handleStepOptimizedStructure_(Structure* s)
{
  Q_ASSERT(trackerContainsStructure(s, &m_stepOptimizedTracker));
  removeFromTrackerWhenScopeEnds popper(s, &m_stepOptimizedTracker);

  QWriteLocker locker(&s->lock());

  // Validate assumptions
  if (s->getStatus() != Structure::StepOptimized) {
    return;
  }

  s->stopOptTimer();

  QString err;
  if (!m_search->checkStepOptimizedStructure(s, &err)) {
    // Structure failed a post optimization step:
    m_search->warning(QString("Structure %1 failed a post-optimization step: %2")
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

    // Print an update to the terminal if we are not using the GUI
    if (!m_search->usingGUI()) {
      qDebug() << "Structure" << s->getTag()
               << "completed step" << s->getCurrentOptStep();
    }

    s->setCurrentOptStep(s->getCurrentOptStep() + 1);

    // Update status
    s->setStatus(Structure::WaitingForOptimization);
    m_runningTracker.lockForWrite();
    m_runningTracker.append(s);
    m_runningTracker.unlock();
    locker.unlock();
    emit structureUpdated(s);
    addStructureToSubmissionQueue(s);
    return;
  }
  // Otherwise, it's done
  else {
    if (m_search->m_calculateObjectives) {
      // Initiate objective calculations if needed
      s->resetStrucObj();
      s->setStatus(Structure::ObjectiveCalculation);
      locker.unlock();
      emit readyForObjectiveCalculations(s);
      return;
    } else {
      // No objective calculations; proceed to optimized!
      if (!m_search->usingGUI()) {
        qDebug() << "Structure" << s->getTag()
                 << "is optimized!";
      }
      s->setStatus(Structure::Optimized);
      locker.unlock();
      handleOptimizedStructure(s);
      return;
    }
  }
}
/// @endcond

void QueueManager::updateStructureObjectiveState(Structure* s)
{
  // This is the main entry into processing finished objective calculations.
  // This function:
  //   (1) Starts by doneWithObjectives signal,
  //   (2) Checks if objective calculations are
  //       finished, and if so, sets the structure status according to the
  //       outcomes which hands the structure over to the appropriate
  //       handler function.
  // Note:
  //   (1) If one of the above "doneWith..." signals is emitted while the
  //       other calculation is not finished, we just return from here
  //       doing nothing. By the time the second one is emitted, we get
  //       here again, and this time both calculations are finished.
  //   (2) This function is called only with a signal; so basically a tracker
  //       is not needed.

  QWriteLocker locker(&s->lock());

  // If any of the objectives is not finished, return.
  if ((m_search->m_calculateObjectives &&
       s->getStrucObjState() == Structure::Os_NotCalculated))
    return;

  // Getting here means that we have objective calculation for sure.
  if (s->getStrucObjState() == Structure::Os_Retain)
    s->setStatus(Structure::ObjectiveRetain);
  else if (s->getStrucObjState() == Structure::Os_Dismiss)
    s->setStatus(Structure::ObjectiveDismiss);
  else // objective calculation is failed
    s->setStatus(Structure::ObjectiveFail);

  return;
}

void QueueManager::handleRetainObjective(Structure *s)
{
  QWriteLocker locker(m_objectiveRetainTracker.rwLock());
  if (!m_objectiveRetainTracker.append(s)) {
    return;
  }
  QtConcurrent::run(this, &QueueManager::handleRetainObjective_, s);
}

// Doxygen skip:
/// @cond
void QueueManager::handleRetainObjective_(Structure* s)
{
  Q_ASSERT(trackerContainsStructure(s, &m_objectiveRetainTracker));
  removeFromTrackerWhenScopeEnds popper(s, &m_objectiveRetainTracker);

  QWriteLocker locker(&s->lock());

  // Validate assumptions
  if (s->getStatus() != Structure::ObjectiveRetain)
    return;

  if (!m_search->usingGUI()) {
    qDebug() << "Structure" << s->getTag()
             << "is optimized!";
  }

  s->setStatus(Structure::Optimized);
  handleOptimizedStructure(s);

  return;
}
/// @endcond

void QueueManager::handleFailObjective(Structure *s)
{
  QWriteLocker locker(m_objectiveFailTracker.rwLock());
  if (!m_objectiveFailTracker.append(s)) {
    return;
  }
  QtConcurrent::run(this, &QueueManager::handleFailObjective_, s);
}

// Doxygen skip:
/// @cond
void QueueManager::handleFailObjective_(Structure* s)
{
  Q_ASSERT(trackerContainsStructure(s, &m_objectiveFailTracker));
  removeFromTrackerWhenScopeEnds popper(s, &m_objectiveFailTracker);

  QWriteLocker locker(&s->lock());

  // This might happen if objectives fail for any reason
  //   (failed communication with remote server, no script present, ...)
  m_search->error(tr("Objectives Fail (%1): removing the structure %2 ")
      .arg(s->getStrucObjState()).arg(s->getTag()));

  s->setStatus(Structure::ObjectiveFail);

  // Ensure that the job is not tying up the queue
  stopJob(s);
  // Remove from running tracker
  m_runningTracker.lockForWrite();
  m_runningTracker.remove(s);
  m_runningTracker.unlock();
  emit structureKilled(s);
}
/// @endcond

void QueueManager::handleDismissObjective(Structure *s)
{
  QWriteLocker locker(m_objectiveDismissTracker.rwLock());
  if (!m_objectiveDismissTracker.append(s)) {
    return;
  }
  QtConcurrent::run(this, &QueueManager::handleDismissObjective_, s);
}

// Doxygen skip:
/// @cond
void QueueManager::handleDismissObjective_(Structure* s)
{
  Q_ASSERT(trackerContainsStructure(s, &m_objectiveDismissTracker));
  removeFromTrackerWhenScopeEnds popper(s, &m_objectiveDismissTracker);

  QWriteLocker locker(&s->lock());

  if (m_search->m_objectivesReDo && s->getStrucObjFailCt() == 0)
  {

    if (m_search->m_verbose) {
      QString outstr;
      outstr.sprintf(
          "   Redo struc %8s with objective outcome % 2d ( action = % 3d ) !",
          s->getTag().toStdString().c_str(),
          s->getStrucObjState(), SearchBase::FailActions(m_search->failAction));
      qDebug().noquote() << outstr;
    }

    // Update structure objective info; for the record, and to
    //   not repeat this step (we do this once)
    s->setStrucObjFailCt(s->getStrucObjFailCt()+1);
    // save info to history; as it might be recalculated!
    s->updateAndAddObjectivesToHistory(s);
    if (SearchBase::FailActions(m_search->failAction) == SearchBase::FA_Randomize) {
      s->setStatus(Structure::Empty);
      locker.unlock();
      m_search->replaceWithRandom(s, tr("failed objective calculation"));
      s->setStatus(Structure::Restart);
      emit structureUpdated(s);
      return;
    } else if (SearchBase::FailActions(m_search->failAction) == SearchBase::FA_NewOffspring) {
      s->setStatus(Structure::Empty);
      locker.unlock();
      m_search->replaceWithOffspring(s, tr("failed objective calculation"));
      s->setStatus(Structure::Restart);
      emit structureUpdated(s);
      return;
    }
  }

  // Except than the above cases; we just dismiss the structure
  m_search->warning(tr("Objectives Dismiss (%1): removing the structure %2 ")
      .arg(s->getStrucObjState()).arg(s->getTag()));

  s->setStatus(Structure::ObjectiveDismiss);

  // Ensure that the job is not tying up the queue
  stopJob(s);
  // Remove from running tracker
  m_runningTracker.lockForWrite();
  m_runningTracker.remove(s);
  m_runningTracker.unlock();
  emit structureKilled(s);
}
/// @endcond

void QueueManager::handleWaitingForOptimizationStructure(Structure* s)
{
  // Nothing to do but wait for the structure to be submitted
}

void QueueManager::handleEmptyStructure(Structure* s)
{
  // Nothing to do but wait (this should never actually happen...)
}

void QueueManager::handleUpdatingStructure(Structure* s)
{
  // Nothing to do but wait
}

void QueueManager::handleErrorStructure(Structure* s)
{
  QWriteLocker locker(m_errorTracker.rwLock());
  if (!m_errorTracker.append(s)) {
    return;
  }
  QtConcurrent::run(this, &QueueManager::handleErrorStructure_, s);
}

// Doxygen skip:
/// @cond
void QueueManager::handleErrorStructure_(Structure* s)
{
  Q_ASSERT(trackerContainsStructure(s, &m_errorTracker));
  removeFromTrackerWhenScopeEnds popper(s, &m_errorTracker);

  if (s->getStatus() != Structure::Error) {
    return;
  }

  if (!m_search->usingGUI()) {
    qDebug() << "Structure" << s->getTag()
             << "failed";
  }

  stopJob(s);

  // Lock for writing
  QWriteLocker locker(&s->lock());

  s->addFailure();

  // If the number of failures has exceed the limit, take
  // appropriate action
  if (s->getFailCount() >= m_search->failLimit) {
    switch (SearchBase::FailActions(m_search->failAction)) {
      case SearchBase::FA_DoNothing:
      default:
        // resubmit job
        s->setStatus(Structure::Restart);
        emit structureUpdated(s);
        return;
      case SearchBase::FA_KillIt:
        locker.unlock();
        killStructure(s);
        emit structureUpdated(s);
        return;
      case SearchBase::FA_Randomize:
        s->setStatus(Structure::Empty);
        locker.unlock();
        m_search->replaceWithRandom(s, tr("failures: random"));
        s->setStatus(Structure::Restart);
        emit structureUpdated(s);
        return;
      case SearchBase::FA_NewOffspring:
        s->setStatus(Structure::Empty);
        locker.unlock();
        m_search->replaceWithOffspring(s, tr("failures: offspring"));
        s->setStatus(Structure::Restart);
        emit structureUpdated(s);
        return;
    }
  }
  // Resubmit job if failure limit hasn't been reached
  else {
    s->setStatus(Structure::Restart);
    emit structureUpdated(s);
    return;
  }
}
/// @endcond

void QueueManager::handleSubmittedStructure(Structure* s)
{
  QWriteLocker locker(m_submittedTracker.rwLock());
  if (!m_submittedTracker.append(s)) {
    return;
  }
  QtConcurrent::run(this, &QueueManager::handleSubmittedStructure_, s);
}

// Doxygen skip:
/// @cond
void QueueManager::handleSubmittedStructure_(Structure* s)
{
  Q_ASSERT(trackerContainsStructure(s, &m_submittedTracker));
  removeFromTrackerWhenScopeEnds popper(s, &m_submittedTracker);

  if (s->getStatus() != Structure::Submitted) {
    return;
  }

  switch (m_search->queueInterface(s->getCurrentOptStep())->getStatus(s)) {
    case QueueInterface::Running:
    case QueueInterface::Queued:
    case QueueInterface::Success:
    case QueueInterface::Started:
      // Update the structure as "InProcess"
      s->lock().lockForWrite();
      s->setStatus(Structure::InProcess);
      s->lock().unlock();
      emit structureUpdated(s);
      break;
    case QueueInterface::Error:
      s->lock().lockForWrite();
      s->setStatus(Structure::Restart);
      s->lock().unlock();
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

void QueueManager::handleKilledStructure(Structure* s)
{
  QWriteLocker locker(m_newlyKilledTracker.rwLock());
  if (!m_newlyKilledTracker.append(s)) {
    return;
  }
  QtConcurrent::run(this, &QueueManager::handleKilledStructure_, s);
}

// Doxygen skip:
/// @cond
void QueueManager::handleKilledStructure_(Structure* s)
{
  Q_ASSERT(trackerContainsStructure(s, &m_newlyKilledTracker));
  removeFromTrackerWhenScopeEnds popper(s, &m_newlyKilledTracker);

  if (s->getStatus() != Structure::Killed &&
      // Remove structures end up here, too, so check this (see
      // handleRemovedStructure below)
      s->getStatus() != Structure::Removed) {
    return;
  }

  // Ensure that the job is not tying up the queue
  stopJob(s);

  // Remove from running tracker
  m_runningTracker.lockForWrite();
  m_runningTracker.remove(s);
  m_runningTracker.unlock();
}
/// @endcond

void QueueManager::handleRemovedStructure(Structure* s)
{
  handleKilledStructure(s);
}

void QueueManager::handleSimilarStructure(Structure* s)
{
  QWriteLocker locker(m_newSimilarityTracker.rwLock());
  if (!m_newSimilarityTracker.append(s)) {
    return;
  }
  QtConcurrent::run(this, &QueueManager::handleSimilarStructure_, s);
}

// Doxygen skip:
/// @cond
void QueueManager::handleSimilarStructure_(Structure* s)
{
  Q_ASSERT(trackerContainsStructure(s, &m_newSimilarityTracker));
  removeFromTrackerWhenScopeEnds popper(s, &m_newSimilarityTracker);

  if (s->getStatus() != Structure::Similar) {
    return;
  }

  // Ensure that the job is not tying up the queue
  stopJob(s);

  // Remove from running tracker
  m_runningTracker.lockForWrite();
  m_runningTracker.remove(s);
  m_runningTracker.unlock();
}
/// @endcond

void QueueManager::handleRestartStructure(Structure* s)
{
  QWriteLocker locker(m_restartTracker.rwLock());
  if (!m_restartTracker.append(s)) {
    return;
  }
  QtConcurrent::run(this, &QueueManager::handleRestartStructure_, s);
}

// Doxygen skip:
/// @cond
void QueueManager::handleRestartStructure_(Structure* s)
{
  Q_ASSERT(trackerContainsStructure(s, &m_restartTracker));
  removeFromTrackerWhenScopeEnds popper(s, &m_restartTracker);

  if (s->getStatus() != Structure::Restart) {
    return;
  }

  stopJob(s);

  addStructureToSubmissionQueue(s);
}

void QueueManager::updateStructure(Structure* s)
{
  s->lock().lockForWrite();
  s->stopOptTimer();
  s->resetFailCount();
  s->setStatus(Structure::Updating);
  s->lock().unlock();
  if (!m_search->optimizer(s->getCurrentOptStep())->update(s)) {
    s->lock().lockForWrite();
    s->setStatus(Structure::Error);
    s->lock().unlock();
    emit structureUpdated(s);
    return;
  }
  s->lock().lockForWrite();
  s->setStatus(Structure::StepOptimized);
  s->lock().unlock();
  emit structureUpdated(s);
  return;
}
/// @endcond

void QueueManager::killStructure(Structure* s)
{
  // End job if currently running
  if (s->getStatus() != Structure::Optimized) {
    s->lock().lockForWrite();
    s->stopOptTimer();
    s->setStatus(Structure::Killed);
    s->lock().unlock();
  } else {
    s->lock().lockForWrite();
    s->stopOptTimer();
    s->setStatus(Structure::Removed);
    s->lock().unlock();
  }
  stopJob(s);
  emit structureKilled(s);
}

void QueueManager::addStructureToSubmissionQueue(Structure* s, int optStep)
{
  QWriteLocker locker(m_newSubmissionTracker.rwLock());
  if (!m_newSubmissionTracker.append(s)) {
    return;
  }

  QtConcurrent::run(this, &QueueManager::addStructureToSubmissionQueue_, s,
                    optStep);
}

// Doxygen skip:
/// @cond
void QueueManager::addStructureToSubmissionQueue_(Structure* s, int optStep)
{
  Q_ASSERT(trackerContainsStructure(s, &m_newSubmissionTracker));
  removeFromTrackerWhenScopeEnds popper(s, &m_newSubmissionTracker);

  // Update structure
  s->lock().lockForWrite();
  if (s->getStatus() != Structure::Optimized) {
    s->setStatus(Structure::WaitingForOptimization);
    if (optStep != -1) {
      s->setCurrentOptStep(optStep);
    }
  }
  s->lock().unlock();

  // Perform writing
  m_search->queueInterface(s->getCurrentOptStep())->writeInputFiles(s);

  m_jobStartTracker.lockForWrite();
  m_jobStartTracker.append(s);
  m_jobStartTracker.unlock();

  m_runningTracker.lockForWrite();
  m_runningTracker.append(s);
  m_runningTracker.unlock();

  emit structureUpdated(s);
}
/// @endcond

void QueueManager::startJob()
{
  Structure* s;
  if (!m_jobStartTracker.popFirst(s)) {
    return;
  }

  if (!m_search->queueInterface(s->getCurrentOptStep())->startJob(s)) {
    s->lock().lockForWrite();
    m_search->warning(tr("QueueManager::startJob_: Job did not start "
                      "successfully for structure %1-%2.")
                     .arg(s->getTag())
                     .arg(s->getCurrentOptStep()));
    s->setStatus(Structure::Error);
    s->lock().unlock();
    return;
  }

  s->lock().lockForWrite();
  s->setStatus(Structure::Submitted);
  s->lock().unlock();

  if (!m_search->usingGUI()) {
    QReadLocker locker(&s->lock());
    qDebug() << "Structure"
             << s->getTag()
             << "has been submitted"
             << "step" << s->getCurrentOptStep();
  }

  emit structureSubmitted(s);
}

void QueueManager::stopJob(Structure* s)
{
  m_search->queueInterface(s->getCurrentOptStep())->stopJob(s);
}

QList<Structure*> QueueManager::getAllRunningStructures()
{
  m_runningTracker.lockForRead();
  m_newStructureTracker.lockForRead();
  QList<Structure*> list(*m_runningTracker.list());
  list.append(*m_newStructureTracker.list());
  m_newStructureTracker.unlock();
  m_runningTracker.unlock();
  return list;
}

QList<Structure*> QueueManager::getAllOptimizedStructures()
{
  QList<Structure*> list;
  m_tracker->lockForRead();
  Structure* s;
  for (int i = 0; i < m_tracker->list()->size(); i++) {
    s = m_tracker->list()->at(i);
    s->lock().lockForRead();
    if (s->getStatus() == Structure::Optimized)
      list.append(s);
    s->lock().unlock();
  }
  m_tracker->unlock();
  return list;
}

QList<Structure*> QueueManager::getAllParentPoolStructures()
{
  QList<Structure*> list;

  m_tracker->lockForRead();
  Structure* s;
  for (int i = 0; i < m_tracker->list()->size(); i++) {
    s = m_tracker->list()->at(i);
    s->lock().lockForRead();

    // If structure is not optimized (includes calculation of objectives)
    //   or above hull not calculated, ignore structure!
    bool strIsOk = true;
    if (s->getStatus() != Structure::Optimized || std::isnan(s->getDistAboveHull()))
      strIsOk = false;

    if (strIsOk)
      list.append(s);

    s->lock().unlock();
  }
  m_tracker->unlock();
  return list;

}

QList<Structure*> QueueManager::getAllSimilarStructures()
{
  QList<Structure*> list;
  m_tracker->lockForRead();
  Structure* s;
  for (int i = 0; i < m_tracker->list()->size(); i++) {
    s = m_tracker->list()->at(i);
    s->lock().lockForRead();
    if (s->getStatus() == Structure::Similar)
      list.append(s);
    s->lock().unlock();
  }
  m_tracker->unlock();
  return list;
}

QList<Structure*> QueueManager::getAllStructures()
{
  m_tracker->lockForRead();
  m_newStructureTracker.lockForRead();
  QList<Structure*> list(*m_tracker->list());
  list.append(*m_newStructureTracker.list());
  m_newStructureTracker.unlock();
  m_tracker->unlock();
  return list;
}

QList<Structure*> QueueManager::lockForNaming()
{
  m_tracker->lockForRead();
  m_newStructureTracker.lockForRead();
  QList<Structure*> list(*m_tracker->list());
  list.append(*m_newStructureTracker.list());

  return list;
}

void QueueManager::unlockForNaming(Structure* s)
{
  m_newStructureTracker.unlock();
  if (!s) {
    m_tracker->unlock();
    return;
  }

  // Discard structure if we're shutting down
  if (m_isDestroying) {
    --m_requestedStructures;
    m_tracker->unlock();
    return;
  }

  if (!m_search->isStarting) {
    --m_requestedStructures;
  }

  // Append to tracker after decrementing
  // m_requestedStructures. This keeps behavior predictable during
  // session initialization.
  m_newStructureTracker.lockForWrite();
  m_newStructureTracker.append(s);

  Q_ASSERT_X(m_requestedStructures >= 0, Q_FUNC_INFO,
             "The requested structures counter has become negative.");

  qDebug() << "New structure accepted (" << s->getTag() << ")";

  m_newStructureTracker.unlock();
  m_tracker->unlock();
#if QT_VERSION == 0x040603
  emit newStructureQueued();
#else  // QT_VERSION == 4.6.3
  QtConcurrent::run(this, &QueueManager::unlockForNaming_);
#endif // QT_VERSION == 4.6.3
}

// Doxygen skip:
/// @cond
void QueueManager::unlockForNaming_()
{
  Structure* s;
  m_tracker->lockForWrite();
  m_newStructureTracker.lockForWrite();
  if (!m_newStructureTracker.popFirst(s)) {
    m_newStructureTracker.unlock();
    m_tracker->unlock();
    return;
  }

  // Update structure
  s->lock().lockForWrite();
  if (s->getStatus() != Structure::Optimized)
    s->setStatus(Structure::WaitingForOptimization);
  s->lock().unlock();

  m_tracker->append(s);

  m_newStructureTracker.unlock();
  m_tracker->unlock();

  if (s->getStatus() != Structure::Optimized)
    emit structureStarted(s);
  else if (s->getStatus() == Structure::Optimized)
    emit structureFinished(s);
}
/// @endcond

void QueueManager::appendToJobStartTracker(Structure* s)
{
  m_jobStartTracker.lockForWrite();
  m_jobStartTracker.append(s);
  m_jobStartTracker.unlock();
}

} // end namespace GlobalSearch

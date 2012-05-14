/**********************************************************************
  QueueManager - Generic queue manager to track running structures

  Copyright (C) 2010-2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <globalsearch/queuemanager.h>

#include <globalsearch/macros.h>
#include <globalsearch/optbase.h>
#include <globalsearch/optimizer.h>
#include <globalsearch/queueinterface.h>
#include <globalsearch/queueinterfaces/remote.h>
#include <globalsearch/structure.h>

#include <QtGui/QApplication>

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QtConcurrentRun>
#include <QtCore/QTimer>

// A couple helper functions/classes -- disable doxygen parsing:
/// \cond
namespace {
  class removeFromTrackerWhenScopeEnds
  {
    GlobalSearch::Tracker *m_tracker;
    GlobalSearch::Structure *m_structure;
  public:
    removeFromTrackerWhenScopeEnds(GlobalSearch::Structure *s,
                                   GlobalSearch::Tracker *t)
      : m_tracker(t), m_structure(s) {}
    ~removeFromTrackerWhenScopeEnds() {
      m_tracker->lockForWrite();
      m_tracker->remove(m_structure);
      m_tracker->unlock(); }
  };

  // Locks tracker for reading and calls t->contains(s)
  bool trackerContainsStructure(GlobalSearch::Structure *s,
                                GlobalSearch::Tracker *t)
  { t->lockForRead();
    bool b = t->contains(s);
    t->unlock();
    return b; }
}
/// \endcond

namespace GlobalSearch {

  QueueManager::QueueManager(QThread *thread, OptBase *opt) :
    QObject(),
    m_opt(opt),
    m_thread(thread),
    m_tracker(opt->tracker()),
    m_requestedStructures(0),
    m_isDestroying(false),
    m_lastSubmissionTimeStamp(new QDateTime (QDateTime::currentDateTime()))
  {
    // Thread connections
    connect(m_thread, SIGNAL(started()),
            this, SLOT(moveToQMThread()));
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
    trackers.append(&m_newDuplicateTracker);
    trackers.append(&m_restartTracker);
    trackers.append(&m_newSubmissionTracker);

    // abort any preoptimizations
    foreach (Structure *s, *m_preOptTracker.list()) {
      s->lock()->lockForWrite();
      s->abortPreoptimization();
      s->lock()->unlock();
    }

    // Used to break wait loops if they take too long
    unsigned int timeout;

    for (QList<Tracker*>::iterator
           it = trackers.begin(),
           it_end = trackers.end();
         it != it_end;
         it++) {
      timeout = 10;
      while (timeout > 0 && (*it)->size()) {
        qDebug() << "Spinning on QueueManager handler trackers to empty...";
        GS_SLEEP(1);
        QApplication::processEvents(QEventLoop::AllEvents, 500);
        --timeout;
      }
    }

    qDebug() << "Clearing QueueManager event loop...";
    QApplication::processEvents(QEventLoop::AllEvents);
    qDebug() << "QueueManager event loop cleared.";

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

    connect(this, SIGNAL(movedToQMThread()),
            this, SLOT(setupConnections()),
            Qt::QueuedConnection);

    emit movedToQMThread();
  }

  void QueueManager::setupConnections()
  {
    // opt connections
    connect(this, SIGNAL(needNewStructure()),
            m_opt, SLOT(generateNewStructure()),
            Qt::QueuedConnection);

    // re-emit connections
    connect(this, SIGNAL(structureStarted(GlobalSearch::Structure *)),
            this, SIGNAL(structureUpdated(GlobalSearch::Structure *)));
    connect(this, SIGNAL(structureSubmitted(GlobalSearch::Structure *)),
            this, SIGNAL(structureUpdated(GlobalSearch::Structure *)));
    connect(this, SIGNAL(structureKilled(GlobalSearch::Structure *)),
            this, SIGNAL(structureUpdated(GlobalSearch::Structure *)));
    connect(this, SIGNAL(structureFinished(GlobalSearch::Structure *)),
            this, SIGNAL(structureUpdated(GlobalSearch::Structure *)));

    // Work around bug in Qt 4.6.3:
#if QT_VERSION == 0x040603
    connect(this, SIGNAL(newStructureQueued()),
            this, SLOT(unlockForNaming_()),
            Qt::QueuedConnection);
#endif // QT_VERSION == 4.6.3

    QTimer::singleShot(0, this, SLOT(checkLoop()));
  }

  void QueueManager::reset()
  {
    QList<Tracker*> trackers;
    trackers.append(m_tracker);
    trackers.append(&m_needPreOptTracker);
    trackers.append(&m_preOptTracker);
    trackers.append(&m_jobStartTracker);
    trackers.append(&m_runningTracker);
    trackers.append(&m_newStructureTracker);
    trackers.append(&m_newlyOptimizedTracker);
    trackers.append(&m_stepOptimizedTracker);
    trackers.append(&m_inProcessTracker);
    trackers.append(&m_errorTracker);
    trackers.append(&m_submittedTracker);
    trackers.append(&m_newlyKilledTracker);
    trackers.append(&m_newDuplicateTracker);
    trackers.append(&m_restartTracker);
    trackers.append(&m_newSubmissionTracker);

    for (QList<Tracker*>::iterator
           it = trackers.begin(),
           it_end = trackers.end();
         it != it_end;
         it++) {
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
               "from a thread other than the QM thread. "
               );

    if (!m_opt->readOnly &&
        !m_opt->isStarting ) {
      checkPopulation();
      checkRunning();
    }

    QTimer::singleShot(1000, this, SLOT(checkLoop()));
  }

  void QueueManager::checkPopulation()
  {
    // Count jobs
    uint running = 0;
    uint optimized = 0;
    uint submitted = 0;
    m_tracker->lockForRead();
    QList<Structure*> structures = *m_tracker->list();
    m_tracker->unlock();

    // Check to see that the number of running jobs is >= that specified:
    Structure *structure = 0;
    Structure::State state;
    int fail=0;
    for (int i = 0; i < structures.size(); ++i) {
      structure = structures.at(i);
      structure->lock()->lockForRead();
      state = structure->getStatus();
      if (structure->getFailCount() != 0) fail++;
      bool needsPreOpt = structure->needsPreoptimization();
      structure->lock()->unlock();
      // Check for structures that need preoptimization
      if ( needsPreOpt ) {
        m_jobStartTracker.lockForWrite();
        m_jobStartTracker.remove(structure);
        m_jobStartTracker.unlock();
        m_needPreOptTracker.lockForWrite();
        m_needPreOptTracker.append(structure);
        m_needPreOptTracker.unlock();
      }
      // Check for structures that are preoptimizating
      if ( state == Structure::Preoptimizing ) {
        m_preOptTracker.lockForWrite();
        m_preOptTracker.append(structure);
        m_preOptTracker.unlock();
        ++submitted;
      }
      // Count submitted structures
      if ( state == Structure::Submitted ||
           state == Structure::InProcess ){
        m_runningTracker.lockForWrite();
        m_runningTracker.append(structure);
        m_runningTracker.unlock();
        submitted++;
      }
      // Count running jobs and update trackers
      if ( state != Structure::Optimized &&
           state != Structure::Duplicate &&
           state != Structure::Killed &&
           state != Structure::Removed  &&
           state != Structure::Preoptimizing ){
        running++;
        m_runningTracker.lockForWrite();
        m_runningTracker.append(structure);
        m_runningTracker.unlock();
      }
      else {
        if ( state == Structure::Optimized ) {
          optimized++;
        }
        m_runningTracker.lockForWrite();
        m_runningTracker.remove(structure);
        m_runningTracker.unlock();
      }
    }
    emit newStatusOverview(optimized, running, fail);

    // Start a preoptimizations if needed
    m_needPreOptTracker.lockForWrite();
    m_preOptTracker.lockForWrite();
    int needPreOpt = m_needPreOptTracker.size();
    int preOpting  = m_preOptTracker.size();
    m_preOptTracker.unlock();
    m_needPreOptTracker.unlock();
    if (needPreOpt != 0 && // Structures need preopt and
        preOpting == 0 ){     // no preopts running
      startPreoptimization();
    }

    // Submit any jobs if needed
    m_jobStartTracker.lockForWrite();
    int pending = m_jobStartTracker.list()->size();
    if (pending != 0 &&
        (
          !m_opt->limitRunningJobs ||
          submitted < m_opt->runningJobLimit
          )) {
#ifdef ENABLE_SSH
      // Submit a single throttled job (1 submission per 3-8 seconds) if using
      // a remote queue interface. Interval is randomly chosen each iteration.
      // This prevents hammering the pbs server from multiple XtalOpt instances
      // if there is a problem with the queue.
      if (qobject_cast<RemoteQueueInterface*>
          (m_opt->queueInterface()) != NULL) {
        if (m_lastSubmissionTimeStamp->secsTo(QDateTime::currentDateTime())
            >= 3 + (6 * RANDDOUBLE())) {
          startJob();
          ++submitted;
          --pending;
          *m_lastSubmissionTimeStamp = QDateTime::currentDateTime();
        }
      }
	  else
#endif // ENABLE_SSH
      {
        // Local job submission doesn't need to be throttled
        while (pending != 0 &&
               (
                 !m_opt->limitRunningJobs ||
                 submitted < m_opt->runningJobLimit
                 )
               ) {
          startJob();
          ++submitted;
          --pending;
        }
      }
    }
    m_jobStartTracker.unlock();

    // Generate requests
    m_tracker->lockForWrite(); // Write lock for m_requestedStructures var
    m_newStructureTracker.lockForRead();

    // Avoid convience function calls here, as occaisional deadlocks
    // can occur.
    //
    // total is getAllStructures().size() + m_requestedStructures;
    int total = m_tracker->size() + m_newStructureTracker.size()
      + m_requestedStructures;
    // incomplete is getAllRunningStructures.size() + m_requestedStructures:
    int incomplete = m_runningTracker.size() + m_newStructureTracker.size()
      + m_requestedStructures;
    int needed = m_opt->contStructs - incomplete;

    if (
        // Are we at the continuous structure limit?
        ( needed > 0) &&
        // Is the cutoff either disabled or reached/exceeded?
        ( m_opt->cutoff <= 0 || total < m_opt->cutoff) &&
        // Check if we are testing. If so, have we reached the testing limit?
        ( !m_opt->testingMode || total < m_opt->test_nStructs)
        ) {
      // emit requests
      qDebug() << "Need " << needed << " structures. " << incomplete << " already incomplete.";
      for (int i = 0; i < needed; ++i) {
        ++m_requestedStructures;
        emit needNewStructure();
        qDebug() << "Requested new structure. Total requested: " << m_requestedStructures;
      }
    }

    m_newStructureTracker.unlock();
    m_tracker->unlock();
    return;
  }

  void QueueManager::checkRunning()
  {
    // Ensure that this is only called from the QM thread:
    Q_ASSERT_X(QThread::currentThread() == m_thread, Q_FUNC_INFO,
               "Attempting to run QueueManager::checkRunning "
               "from a thread other than the QM thread. "
               );

    // Get list of running and preoptimizing structures
    QList<Structure*> runningStructures = getAllRunningStructures();
    runningStructures.append(*m_preOptTracker.list());


    // iterate over all structures and handle each based on its status
    for (QList<Structure*>::iterator
           s_it = runningStructures.begin(),
           s_it_end = runningStructures.end();
         s_it != s_it_end;
         ++s_it) {

      // Assign pointer for convenience
      Structure *structure = *s_it;

      // Check if this structure has any handlers pending. Skip if so.
      if (m_newlyOptimizedTracker.contains(structure) ||
          m_stepOptimizedTracker.contains(structure)  ||
          m_inProcessTracker.contains(structure)      ||
          m_errorTracker.contains(structure)          ||
          m_submittedTracker.contains(structure)      ||
          m_newlyKilledTracker.contains(structure)    ||
          m_newDuplicateTracker.contains(structure)   ||
          m_restartTracker.contains(structure)        ||
          m_newSubmissionTracker.contains(structure)) {
        continue;
      }

      // Lookup status
      structure->lock()->lockForRead();
      Structure::State status = structure->getStatus();
      structure->lock()->unlock();

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
        //handleOptimizedStructure(structure);
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
      case Structure::Duplicate:
        handleDuplicateStructure(structure);
        break;
      case Structure::Empty:
        handleEmptyStructure(structure);
        break;
      case Structure::Preoptimizing:
        handlePreoptimizingStructure(structure);
        break;
      }
    }

    return;
  }

  void QueueManager::handleInProcessStructure(Structure *s)
  {
    QWriteLocker locker (m_inProcessTracker.rwLock());
    if (!m_inProcessTracker.append(s)) {
      return;
    }
    QtConcurrent::run(this,
                      &QueueManager::handleInProcessStructure_, s);
  }

  // Doxygen skip:
  /// @cond
  void QueueManager::handleInProcessStructure_(Structure *s)
  {
    Q_ASSERT(trackerContainsStructure(s, &m_inProcessTracker));
    removeFromTrackerWhenScopeEnds popper (s, &m_inProcessTracker);

    // Revalidate assumptions
    if (s->getStatus() != Structure::InProcess) {
      return;
    }

    switch (m_opt->queueInterface()->getStatus(s)) {
    case QueueInterface::Running:
    case QueueInterface::Queued:
    case QueueInterface::CommunicationError:
    case QueueInterface::Unknown:
    case QueueInterface::Pending:
    case QueueInterface::Started:
      // Nothing to do but wait
      break;
    case QueueInterface::Success:
      updateStructure(s);
      break;
    case QueueInterface::Error:
      s->lock()->lockForWrite();
      s->setStatus(Structure::Error);
      s->lock()->unlock();
      emit structureUpdated(s);
      break;
    }

    return;
  }
  /// @endcond

  void QueueManager::handleOptimizedStructure(Structure *s)
  {
    QWriteLocker locker (m_newlyOptimizedTracker.rwLock());
    if (!m_newlyOptimizedTracker.append(s)) {
      return;
    }
    QtConcurrent::run(this,
                      &QueueManager::handleOptimizedStructure_, s);
  }

  // Doxygen skip:
  /// @cond
  void QueueManager::handleOptimizedStructure_(Structure *s)
  {
    Q_ASSERT(trackerContainsStructure(s, &m_newlyOptimizedTracker));
    removeFromTrackerWhenScopeEnds popper (s, &m_newlyOptimizedTracker);

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

  void QueueManager::handleStepOptimizedStructure(Structure *s)
  {
    QWriteLocker locker (m_stepOptimizedTracker.rwLock());
    m_stepOptimizedTracker.append(s);
    QtConcurrent::run(this,
                      &QueueManager::handleStepOptimizedStructure_, s);
  }

  // Doxygen skip:
  /// @cond
  void QueueManager::handleStepOptimizedStructure_(Structure *s)
  {
    Q_ASSERT(trackerContainsStructure(s, &m_stepOptimizedTracker));
    removeFromTrackerWhenScopeEnds popper (s, &m_stepOptimizedTracker);

    QWriteLocker locker (s->lock());

    // Validate assumptions
    if (s->getStatus() != Structure::StepOptimized) {
      return;
    }

    s->stopOptTimer();

    QString err;
    if (!m_opt->checkStepOptimizedStructure(s, &err)) {
      // Structure failed a post optimization step:
      m_opt->warning(QString("Structure %1 failed a post-optimization step: %2")
                     .arg(s->getIDString())
                     .arg(err));
      s->setStatus(Structure::Error);
      emit structureUpdated(s);
      return;
    }

    // update optstep and relaunch if necessary
    if (s->getCurrentOptStep()
        < static_cast<unsigned int>(m_opt->optimizer()->getNumberOfOptSteps())) {
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
      s->setStatus(Structure::Optimized);
      locker.unlock();
      handleOptimizedStructure(s);
    }
  }
  /// @endcond

  void QueueManager::handleWaitingForOptimizationStructure(Structure *s)
  {
    // Nothing to do but wait for the structure to be submitted
  }

  void QueueManager::handleEmptyStructure(Structure *s)
  {
    // Nothing to do but wait (this should never actually happen...)
  }

  void QueueManager::handlePreoptimizingStructure(Structure *s)
  {
    // Nothing to do but wait
  }

  void QueueManager::handleUpdatingStructure(Structure *s)
  {
    // Nothing to do but wait
  }

  void QueueManager::handleErrorStructure(Structure *s)
  {
    QWriteLocker locker (m_errorTracker.rwLock());
    if (!m_errorTracker.append(s)) {
      return;
    }
    QtConcurrent::run(this,
                      &QueueManager::handleErrorStructure_, s);
  }

  // Doxygen skip:
  /// @cond
  void QueueManager::handleErrorStructure_(Structure *s)
  {
    Q_ASSERT(trackerContainsStructure(s, &m_errorTracker));
    removeFromTrackerWhenScopeEnds popper (s, &m_errorTracker);

    if (s->getStatus() != Structure::Error) {
      return;
    }

    stopJob(s);

    // Lock for writing
    QWriteLocker locker (s->lock());

    s->addFailure();

    // If the number of failures has exceed the limit, take
    // appropriate action
    if (s->getFailCount() >= m_opt->failLimit) {
      switch (OptBase::FailActions(m_opt->failAction)) {
      case OptBase::FA_DoNothing:
      default:
        // resubmit job
        s->setStatus(Structure::Restart);
        emit structureUpdated(s);
        return;
      case OptBase::FA_KillIt:
        locker.unlock();
        killStructure(s);
        emit structureUpdated(s);
        return;
      case OptBase::FA_Randomize:
        s->setStatus(Structure::Empty);
        locker.unlock();
        m_opt->replaceWithRandom(s, tr("excessive failures"));
        s->setStatus(Structure::Restart);
        emit structureUpdated(s);
      case OptBase::FA_NewOffspring:
        s->setStatus(Structure::Empty);
        locker.unlock();
        m_opt->replaceWithOffspring(s, tr("excessive failures"));
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

  void QueueManager::handleSubmittedStructure(Structure *s)
  {
    QWriteLocker locker (m_submittedTracker.rwLock());
    if (!m_submittedTracker.append(s)) {
      return;
    }
    QtConcurrent::run(this,
                      &QueueManager::handleSubmittedStructure_, s);
  }

  // Doxygen skip:
  /// @cond
  void QueueManager::handleSubmittedStructure_(Structure *s)
  {
    Q_ASSERT(trackerContainsStructure(s, &m_submittedTracker));
    removeFromTrackerWhenScopeEnds popper (s, &m_submittedTracker);

    if (s->getStatus() != Structure::Submitted) {
      return;
    }

    switch (m_opt->queueInterface()->getStatus(s)) {
    case QueueInterface::Running:
    case QueueInterface::Queued:
    case QueueInterface::Success:
    case QueueInterface::Started:
      // Update the structure as "InProcess"
      s->lock()->lockForWrite();
      s->setStatus(Structure::InProcess);
      s->lock()->unlock();
      emit structureUpdated(s);
      break;
    case QueueInterface::Error:
      s->lock()->lockForWrite();
      s->setStatus(Structure::Restart);
      s->lock()->unlock();
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

  void QueueManager::handleKilledStructure(Structure *s)
  {
    QWriteLocker locker (m_newlyKilledTracker.rwLock());
    if (!m_newlyKilledTracker.append(s)) {
      return;
    }
    QtConcurrent::run(this,
                      &QueueManager::handleKilledStructure_, s);
  }

  // Doxygen skip:
  /// @cond
  void QueueManager::handleKilledStructure_(Structure *s)
  {
    Q_ASSERT(trackerContainsStructure(s, &m_newlyKilledTracker));
    removeFromTrackerWhenScopeEnds popper (s, &m_newlyKilledTracker);

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

  void QueueManager::handleRemovedStructure(Structure *s)
  {
    handleKilledStructure(s);
  }

  void QueueManager::handleDuplicateStructure(Structure *s)
  {
    QWriteLocker locker (m_newDuplicateTracker.rwLock());
    if (!m_newDuplicateTracker.append(s)) {
      return;
    }
    QtConcurrent::run(this,
                      &QueueManager::handleDuplicateStructure_, s);
  }

  // Doxygen skip:
  /// @cond
  void QueueManager::handleDuplicateStructure_(Structure *s)
  {
    Q_ASSERT(trackerContainsStructure(s, &m_newDuplicateTracker));
    removeFromTrackerWhenScopeEnds popper (s, &m_newDuplicateTracker);

    if (s->getStatus() != Structure::Duplicate) {
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

  void QueueManager::handleRestartStructure(Structure *s)
  {
    QWriteLocker locker (m_restartTracker.rwLock());
    if (!m_restartTracker.append(s)) {
      return;
    }
    QtConcurrent::run(this,
                      &QueueManager::handleRestartStructure_, s);
  }

  // Doxygen skip:
  /// @cond
  void QueueManager::handleRestartStructure_(Structure *s)
  {
    Q_ASSERT(trackerContainsStructure(s, &m_restartTracker));
    removeFromTrackerWhenScopeEnds popper (s, &m_restartTracker);

    if (s->getStatus() != Structure::Restart) {
      return;
    }

    stopJob(s);

    addStructureToSubmissionQueue(s);
  }

  void QueueManager::updateStructure(Structure *s)
  {
    this->stopJob(s);
    s->lock()->lockForWrite();
    s->stopOptTimer();
    s->resetFailCount();
    s->setStatus(Structure::Updating);
    s->lock()->unlock();
    if (!m_opt->optimizer()->update(s)) {
      s->lock()->lockForWrite();
      s->setStatus(Structure::Error);
      s->lock()->unlock();
      emit structureUpdated(s);
      return;
    }
    s->lock()->lockForWrite();
    s->setStatus(Structure::StepOptimized);
    s->lock()->unlock();
    emit structureUpdated(s);
    return;
  }
  /// @endcond

  void QueueManager::killStructure(Structure *s) {
    // End job if currently running
    s->lock()->lockForWrite();
    s->stopOptTimer();
    if ( s->getStatus() != Structure::Optimized ) {
      s->setStatus(Structure::Killed);
    }
    else {
      s->setStatus(Structure::Removed);
    }
    s->lock()->unlock();
    stopJob(s);
    emit structureKilled(s);
  }

  void QueueManager::senderHasFinishedPreoptimization()
  {
    Structure *s = qobject_cast<Structure*>(this->sender());
    if (s == NULL) {
      qWarning() << Q_FUNC_INFO << "called with non-structure sender.";
      return;
    }

    m_preOptTracker.remove(s);
    this->addStructureToSubmissionQueue(s, 0);
  }

  void QueueManager::addStructureToSubmissionQueue(Structure *s,
                                                   int optStep)
  {
    QWriteLocker locker (m_newSubmissionTracker.rwLock());
    if (!m_newSubmissionTracker.append(s)) {
      return;
    }

    QtConcurrent::run(this,
                      &QueueManager::addStructureToSubmissionQueue_,
                      s, optStep);
  }

  // Doxygen skip:
  /// @cond
  void QueueManager::addStructureToSubmissionQueue_(Structure *s, int optStep)
  {
    Q_ASSERT(trackerContainsStructure(s, &m_newSubmissionTracker));
    removeFromTrackerWhenScopeEnds popper (s, &m_newSubmissionTracker);

    // Update structure
    s->lock()->lockForWrite();
    s->setStatus(Structure::WaitingForOptimization);
    if (optStep != 0) {
      s->setCurrentOptStep(optStep);
    }
    s->lock()->unlock();

    // Perform writing
    m_opt->queueInterface()->writeInputFiles(s);
    emit structureUpdated(s); // for optimizer lookup table, created during
                              // input file creation.

    m_jobStartTracker.lockForWrite();
    m_jobStartTracker.append(s);
    m_jobStartTracker.unlock();

    m_runningTracker.lockForWrite();
    m_runningTracker.append(s);
    m_runningTracker.unlock();
  }
  /// @endcond

  void QueueManager::startPreoptimization()
  {
    Structure *s;
    m_needPreOptTracker.lockForWrite();
    m_preOptTracker.lockForWrite();
    if (!m_needPreOptTracker.popFirst(s)) {
      m_preOptTracker.unlock();
      m_needPreOptTracker.unlock();
      return;
    }
    s->lock()->lockForWrite();

    // Revalidate assumptions
    if (!s->needsPreoptimization()) {
      s->lock()->unlock();
      m_preOptTracker.unlock();
      m_needPreOptTracker.unlock();
      return;
    }

    s->setStatus(Structure::Preoptimizing);
    m_preOptTracker.append(s);

    s->lock()->unlock();
    m_preOptTracker.unlock();
    m_needPreOptTracker.unlock();


    this->connect(s, SIGNAL(preoptimizationFinished()),
                  SLOT(senderHasFinishedPreoptimization()));

    m_opt->preoptimizeStructure(s);
  }

  void QueueManager::startJob()
  {
    Structure *s;
    if (!m_jobStartTracker.popFirst(s)) {
      return;
    }

    s->lock()->lockForRead();
    bool needsPreOpt = s->needsPreoptimization();
    s->lock()->unlock();

    if (needsPreOpt) {
      // This shouldn't happen. If it does, just queue the job for
      // preoptimization instead.
      qDebug() << "Trying to skip preoptimization??";
      m_opt->printBackTrace();
      this->m_preOptTracker.lockForWrite();
      this->m_preOptTracker.append(s);
      this->m_preOptTracker.unlock();
      return;
    }

    if (!m_opt->queueInterface()->startJob(s)) {
      s->lock()->lockForWrite();
      m_opt->warning(tr("QueueManager::startJob_: Job did not start "
                        "successfully for structure %1-%2.")
                     .arg(s->getIDString())
                     .arg(s->getCurrentOptStep()));
      s->setStatus(Structure::Error);
      s->lock()->unlock();
      return;
    }

    s->lock()->lockForWrite();
    s->setStatus(Structure::Submitted);
    s->lock()->unlock();

    emit structureSubmitted(s);
  }

  void QueueManager::stopJob(Structure *s)
  {
    m_jobStartTracker.lockForWrite();
    m_jobStartTracker.remove(s);
    m_jobStartTracker.unlock();

    s->lock()->lockForRead();
    if (s->isPreoptimizing()) {
      s->abortPreoptimization();
    }
    s->lock()->unlock();

    m_opt->queueInterface()->stopJob(s);
  }

  QList<Structure*> QueueManager::getAllPreoptimizingStructures()
  {
    QReadLocker locker (m_preOptTracker.rwLock()); Q_UNUSED(locker);
    return *m_preOptTracker.list();
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
    Structure *s;
    for (int i = 0; i < m_tracker->list()->size(); i++) {
      s = m_tracker->list()->at(i);
      s->lock()->lockForRead();
      if (s->getStatus() == Structure::Optimized)
        list.append(s);
      s->lock()->unlock();
    }
    m_tracker->unlock();
    return list;
  }

  QList<Structure*> QueueManager::getAllDuplicateStructures()
  {
    QList<Structure*> list;
    m_tracker->lockForRead();
    Structure *s;
    for (int i = 0; i < m_tracker->list()->size(); i++) {
      s = m_tracker->list()->at(i);
      s->lock()->lockForRead();
      if (s->getStatus() == Structure::Duplicate)
        list.append(s);
      s->lock()->unlock();
    }
    m_tracker->unlock();
    return list;
  }

  QList<Structure*> QueueManager::getAllStructures()
  {
    m_tracker->lockForRead();
    m_newStructureTracker.lockForRead();
    QList<Structure*> list (*m_tracker->list());
    list.append(*m_newStructureTracker.list());
    m_newStructureTracker.unlock();
    m_tracker->unlock();
    return list;
  }

  QList<Structure*> QueueManager::lockForNaming()
  {
    QList<Structure*> structures = getAllStructures();
    // prevent compiler from optimizing "structures" out:
    structures.size();

    m_tracker->lockForRead();
    return structures;
  }

  void QueueManager::unlockForNaming(Structure *s)
  {
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

    if (!m_opt->isStarting) {
      --m_requestedStructures;
    }

    // Append to tracker after decrementing
    // m_requestedStructures. This keeps behavior predictable during
    // session initialization.
    m_newStructureTracker.lockForWrite();
    m_newStructureTracker.append(s);

    Q_ASSERT_X(m_requestedStructures >= 0, Q_FUNC_INFO,
               "The requested structures counter has become negative.");

    qDebug() << "New structure received (" << s->getIDString() << ")";

    m_newStructureTracker.unlock();
    m_tracker->unlock();
#if QT_VERSION == 0x040603
    emit newStructureQueued();
#else // QT_VERSION == 4.6.3
    QtConcurrent::run(this, &QueueManager::unlockForNaming_);
#endif // QT_VERSION == 4.6.3
  }

  // Doxygen skip:
  /// @cond
  void QueueManager::unlockForNaming_()
  {
    Structure *s;
    m_tracker->lockForWrite();
    m_newStructureTracker.lockForWrite();
    if (!m_newStructureTracker.popFirst(s)) {
      m_newStructureTracker.unlock();
      m_tracker->unlock();
      return;
    }

    // Update structure
    s->lock()->lockForWrite();
    s->setStatus(Structure::WaitingForOptimization);
    bool needsPreOpt = s->needsPreoptimization();
    s->lock()->unlock();

    if (needsPreOpt) {
      m_needPreOptTracker.lockForWrite();
      m_needPreOptTracker.append(s);
    }

    m_tracker->append(s);
    qDebug() << "New structure accepted (" << s->getIDString() << ")";

    if (needsPreOpt) {
      m_needPreOptTracker.unlock();
    }

    m_newStructureTracker.unlock();
    m_tracker->unlock();

    if (!needsPreOpt) {
      this->addStructureToSubmissionQueue(s);
    }
    emit structureStarted(s);
  }
  /// @endcond

  void QueueManager::addManualStructureRequest(int requests)
  {
    m_tracker->lockForWrite();
    m_requestedStructures += requests;
    m_tracker->unlock();
  }

  void QueueManager::appendToJobStartTracker(Structure *s)
  {
    m_jobStartTracker.lockForWrite();
    m_jobStartTracker.append(s);
    m_jobStartTracker.unlock();
  }

} // end namespace GlobalSearch

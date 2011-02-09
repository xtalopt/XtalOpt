/**********************************************************************
  QueueManager - Generic queue manager to track running structures

  Copyright (C) 2011 by David C. Lonie

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
#include <globalsearch/structure.h>

#include <QtCore/QDebug>
#include <QtCore/QtConcurrentRun>

namespace GlobalSearch {

  QueueManager::QueueManager(QThread *thread, OptBase *opt) :
    QObject(),
    m_opt(opt),
    m_thread(thread),
    m_tracker(opt->tracker()),
    m_requestedStructures(0)
  {
    // Thread connections
    connect(m_thread, SIGNAL(started()),
            this, SLOT(moveToQMThread()));
  }

  QueueManager::~QueueManager()
  {
    this->disconnect();

    // Prevent the check functions from running again
    m_checkRunningMutex.lock();
    m_checkPopulationMutex.lock();

    // Wait for handler trackers to empty.
    QList<Tracker*> trackers;
    trackers.append(&m_newlyOptimizedTracker);
    trackers.append(&m_stepOptimizedTracker);
    trackers.append(&m_inProcessTracker);
    trackers.append(&m_errorTracker);
    trackers.append(&m_submittedTracker);
    trackers.append(&m_newlyKilledTracker);

    for (QList<Tracker*>::iterator
           it = trackers.begin(),
           it_end = trackers.end();
         it != it_end;
         it++) {
      while ((*it)->size()) {
        qDebug() << "Spinning on QueueManager handler trackers to empty...";
        GS_SLEEP(1);
      }
    }
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

    // internal connections
    connect(this, SIGNAL(structureStarted(GlobalSearch::Structure *)),
            this, SIGNAL(structureUpdated(GlobalSearch::Structure *)));
    connect(this, SIGNAL(structureSubmitted(GlobalSearch::Structure *)),
            this, SIGNAL(structureUpdated(GlobalSearch::Structure *)));
    connect(this, SIGNAL(structureKilled(GlobalSearch::Structure *)),
            this, SIGNAL(structureUpdated(GlobalSearch::Structure *)));
    connect(this, SIGNAL(structureFinished(GlobalSearch::Structure *)),
            this, SIGNAL(structureUpdated(GlobalSearch::Structure *)));

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

    if (!m_opt->readOnly ||
        !m_opt->isStarting ) {
      qDebug() << "checkLoop calling checkPopulation...";
      checkPopulation();
      qDebug() << "checkLoop calling checkRunning...";
      checkRunning();
    }

    QTimer::singleShot(1000, this, SLOT(checkLoop()));
  }

  void QueueManager::checkPopulation()
  {
    qDebug() << "checkPopulation called...";
    // Return if already checking
    if (!m_checkPopulationMutex.tryLock()) {
      return;
    }

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
      structure->lock()->unlock();
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
           state != Structure::Removed ) {
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

    // Submit any jobs if needed
    m_jobStartTracker.lockForWrite();
    int pending = m_jobStartTracker.list()->size();
    while (pending != 0 &&
           (
            !m_opt->limitRunningJobs ||
            submitted < m_opt->runningJobLimit
            )
           ) {
      Structure *s;
      if (!m_jobStartTracker.popFirst(s)) {
        break;
      }
      startJob(s);
      submitted++;
      pending--;
    }
    m_jobStartTracker.unlock();

    // Generate requests
    m_tracker->lockForRead();
    m_newStructureTracker.lockForRead();

    int total = getAllStructures().size() + m_requestedStructures;
    int incomplete = getAllRunningStructures().size() + m_requestedStructures;
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
    m_checkPopulationMutex.unlock();
    return;
  }

  void QueueManager::checkRunning()
  {
    qDebug() << "checkRunning called...";

    // Ensure that this is only called from the QM thread:
    Q_ASSERT_X(QThread::currentThread() == m_thread, Q_FUNC_INFO,
               "Attempting to run QueueManager::checkRunning "
               "from a thread other than the QM thread. "
               );

    // Return if mutex is already locked
    if (!m_checkRunningMutex.tryLock()) {
      return;
    }

    // Get list of running structures
    QList<Structure*> runningStructures = getAllRunningStructures();

    // Get queue data
    // TODO is this necessary here, or should this be on its own timer?
    updateQueue();

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
          m_newlyKilledTracker.contains(structure)) {
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
        handleOptimizedStructure(structure);
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
      }
    }

    m_checkRunningMutex.unlock();
    return;
  }

  void QueueManager::handleInProcessStructure(Structure *s)
  {
    m_inProcessTracker.lockForWrite();
    m_inProcessTracker.append(s);
    QtConcurrent::run(this,
                      &QueueManager::handleInProcessStructure_);
    m_inProcessTracker.unlock();
  }

  void QueueManager::handleInProcessStructure_() {
    Structure *s;
    m_inProcessTracker.lockForWrite();
    if (!m_inProcessTracker.popFirst(s)) {
      m_inProcessTracker.unlock();
      return;
    }
    m_inProcessTracker.unlock();

    if (s->getStatus() != Structure::InProcess) {
      return;
    }

    switch (m_opt->optimizer()->getStatus(s)) {
    case Optimizer::Running:
    case Optimizer::Queued:
    case Optimizer::CommunicationError:
    case Optimizer::Unknown:
    case Optimizer::Pending:
    case Optimizer::Started:
      // Nothing to do but wait
      break;
    case Optimizer::Success:
      updateStructure(s);
      break;
    case Optimizer::Error:
      s->lock()->lockForWrite();
      s->setStatus(Structure::Error);
      s->lock()->unlock();
      emit structureUpdated(s);
      break;
    }
    return;
  }

  void QueueManager::handleOptimizedStructure(Structure *s)
  {
    m_newlyOptimizedTracker.lockForWrite();
    m_newlyOptimizedTracker.append(s);
    QtConcurrent::run(this,
                      &QueueManager::handleOptimizedStructure_);
    m_newlyOptimizedTracker.unlock();
  }

  void QueueManager::handleOptimizedStructure_() {
    Structure *s;
    m_newlyOptimizedTracker.lockForWrite();
    if (!m_newlyOptimizedTracker.popFirst(s)) {
      m_newlyOptimizedTracker.unlock();
      return;
    }
    m_newlyOptimizedTracker.unlock();

    if (s->getStatus() != Structure::Optimized) {
      return;
    }

    // Ensure that the job is not tying up the queue
    stopJob(s);

    // Remove from running tracker
    m_runningTracker.lockForWrite();
    m_runningTracker.remove(s);
    m_runningTracker.unlock();
  }

  void QueueManager::handleStepOptimizedStructure(Structure *s)
  {
    m_stepOptimizedTracker.lockForWrite();
    m_stepOptimizedTracker.append(s);
    QtConcurrent::run(this,
                      &QueueManager::handleStepOptimizedStructure_);
    m_stepOptimizedTracker.unlock();
  }

  void QueueManager::handleStepOptimizedStructure_() {
    Structure *s;
    m_stepOptimizedTracker.lockForWrite();
    if (!m_stepOptimizedTracker.popFirst(s)) {
      m_stepOptimizedTracker.unlock();
      return;
    }
    m_stepOptimizedTracker.unlock();

    QWriteLocker locker (s->lock());

    if (s->getStatus() != Structure::StepOptimized) {
      return;
    }

    s->stopOptTimer();

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
      submitStructure(s);
      return;
    }
    // Otherwise, it's done
    else {
      s->setStatus(Structure::Optimized);
      m_runningTracker.lockForWrite();
      m_runningTracker.remove(s);
      m_runningTracker.unlock();
      locker.unlock();
      emit structureFinished(s);
    }
  }

  void QueueManager::handleWaitingForOptimizationStructure(Structure *s)
  {
    // Nothing to do but wait for the structure to be submitted
  }

  void QueueManager::handleEmptyStructure(Structure *s)
  {
    // Nothing to do but wait (this should never actually happen...)
  }

  void QueueManager::handleUpdatingStructure(Structure *s)
  {
    // Nothing to do but wait
  }

  void QueueManager::handleErrorStructure(Structure *s)
  {
    m_errorTracker.lockForWrite();
    m_errorTracker.append(s);
    QtConcurrent::run(this,
                      &QueueManager::handleErrorStructure_);
    m_errorTracker.unlock();
  }

  void QueueManager::handleErrorStructure_() {
    Structure *s;
    m_errorTracker.lockForWrite();
    if (!m_errorTracker.popFirst(s)) {
      m_errorTracker.unlock();
      return;
    }
    m_errorTracker.unlock();

    if (s->getStatus() != Structure::Error) {
      return;
    }

    // Lock for writing
    QWriteLocker locker (s->lock());

    // Update the structure
    s->addFailure();
    s->stopOptTimer();

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
        killStructure(s);
        emit structureUpdated(s);
        return;
      case OptBase::FA_Randomize:
        s->setStatus(Structure::Empty);
        locker.unlock();
        m_opt->replaceWithRandom(s, tr("excessive failures"));
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

  void QueueManager::handleSubmittedStructure(Structure *s)
  {
    m_submittedTracker.lockForWrite();
    m_submittedTracker.append(s);
    QtConcurrent::run(this,
                      &QueueManager::handleSubmittedStructure_);
    m_submittedTracker.unlock();
  }

  void QueueManager::handleSubmittedStructure_() {
    Structure *s;
    m_submittedTracker.lockForWrite();
    if (!m_submittedTracker.popFirst(s)) {
      m_submittedTracker.unlock();
      return;
    }
    m_submittedTracker.unlock();

    if (s->getStatus() != Structure::Submitted) {
      return;
    }

    switch (m_opt->optimizer()->getStatus(s)) {
    case Optimizer::Running:
    case Optimizer::Queued:
    case Optimizer::Error:
    case Optimizer::Success:
    case Optimizer::Started:
      // Update the structure as "InProcess"
      s->lock()->lockForWrite();
      s->setStatus(Structure::InProcess);
      s->lock()->unlock();
      emit structureUpdated(s);
      break;
    case Optimizer::CommunicationError:
    case Optimizer::Unknown:
    case Optimizer::Pending:
    default:
      // nothing to do but wait
      break;
    }
  }

  void QueueManager::handleKilledStructure(Structure *s)
  {
    m_newlyKilledTracker.lockForWrite();
    m_newlyKilledTracker.append(s);
    QtConcurrent::run(this,
                      &QueueManager::handleKilledStructure_);
    m_newlyKilledTracker.unlock();
  }

  void QueueManager::handleKilledStructure_() {
    Structure *s;
    m_newlyKilledTracker.lockForWrite();
    if (!m_newlyKilledTracker.popFirst(s)) {
      m_newlyKilledTracker.unlock();
      return;
    }
    m_newlyKilledTracker.unlock();

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

  void QueueManager::handleRemovedStructure(Structure *s)
  {
    handleKilledStructure(s);
  }

  void QueueManager::handleDuplicateStructure(Structure *s)
  {
    // Ensure that the job is not tying up the queue
    stopJob(s);

    // Remove from running tracker
    m_runningTracker.lockForWrite();
    m_runningTracker.remove(s);
    m_runningTracker.unlock();
  }

  void QueueManager::handleRestartStructure(Structure *s)
  {
    submitStructure(s);
  }

  void QueueManager::updateQueue()
  {
    if (m_opt->isStarting ||
        m_opt->readOnly) {
      return;
    }

    // TODO Throttle?

    m_opt->optimizer()->getQueueList(m_queueData, &m_queueDataMutex);
    return;
  }

  void QueueManager::updateStructure(Structure *s) {
    s->lock()->lockForWrite();
    s->stopOptTimer();
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

  void QueueManager::killStructure(Structure *s) {
    stopJob(s);
    QWriteLocker locker (s->lock());
    s->stopOptTimer();
    s->setStatus(Structure::Killed);
    locker.unlock();
    emit structureKilled(s);
  }

  void QueueManager::submitStructure(Structure *s, int optStep) {
    if (!s) return;

    QWriteLocker locker (s->lock());

    s->setStatus(Structure::WaitingForOptimization);
    if (optStep != 0) {
      s->setCurrentOptStep(optStep);
    }
    locker.unlock();
    emit structureUpdated(s);

    // write/copy in background thread
    QtConcurrent::run(this, &QueueManager::submitStructure_, s);
  }

  void QueueManager::submitStructure_(Structure *s) {
    // Perform writing
    m_opt->optimizer()->writeInputFiles(s);

    s->lock()->lockForWrite();
    s->setStatus(Structure::WaitingForOptimization);
    s->lock()->unlock();

    m_jobStartTracker.lockForWrite();
    m_jobStartTracker.append(s);
    m_jobStartTracker.unlock();

    m_runningTracker.lockForWrite();
    m_runningTracker.append(s);
    m_runningTracker.unlock();

    emit structureUpdated(s);
  }

  void QueueManager::startJob(Structure *structure) {
    if (!structure) return;

    structure->lock()->lockForWrite();
    structure->setStatus(Structure::Submitted);
    structure->lock()->unlock();

    QtConcurrent::run(this, &QueueManager::startJob_, structure);
  }

  void QueueManager::startJob_(Structure *structure) {
    emit structureSubmitted(structure);

    // Make sure no mutexes are locked here -- this can take a while...
    if (!m_opt->optimizer()->startOptimization(structure)) {
      structure->lock()->lockForWrite();
      m_opt->warning(tr("QueueManager::submitStructure_: Job did not run successfully for structure %1-%2.")
                     .arg(structure->getIDString())
                     .arg(structure->getCurrentOptStep()));
      structure->setStatus(Structure::Error);
      structure->lock()->unlock();
      return;
    }
      structure->lock()->lockForWrite();
      structure->setStatus(Structure::Submitted);
      structure->lock()->unlock();
    return;
  }

  void QueueManager::stopJob(Structure *s) {
    m_opt->optimizer()->deleteJob(s);
  }

  QList<Structure*> QueueManager::getAllRunningStructures() {
    m_runningTracker.lockForRead();
    m_newStructureTracker.lockForRead();
    QList<Structure*> list(*m_runningTracker.list());
    list.append(*m_newStructureTracker.list());
    m_newStructureTracker.unlock();
    m_runningTracker.unlock();
    return list;
  }

  QList<Structure*> QueueManager::getAllOptimizedStructures() {
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

  QList<Structure*> QueueManager::getAllDuplicateStructures() {
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

  QList<Structure*> QueueManager::getAllStructures() {
    m_tracker->lockForRead();
    m_newStructureTracker.lockForRead();
    QList<Structure*> list (*m_tracker->list());
    list.append(*m_newStructureTracker.list());
    m_newStructureTracker.unlock();
    m_tracker->unlock();
    return list;
  }

  QList<Structure*> QueueManager::lockForNaming() {
    m_tracker->lockForRead();
    return getAllStructures();
  }

  void QueueManager::unlockForNaming(Structure *s) {
    if (!s) {
      m_tracker->unlock();
      return;
    }

    m_newStructureTracker.lockForWrite();
    m_newStructureTracker.append(s);

    if (!m_opt->isStarting) {
      --m_requestedStructures;
    }

    Q_ASSERT_X(m_requestedStructures >= 0, Q_FUNC_INFO,
               "The requested structures counter has become negative.");

    qDebug() << "New structure accepted (" << s->getIDString() << ")";

    m_newStructureTracker.unlock();
    m_tracker->unlock();
    QtConcurrent::run(this, &QueueManager::unlockForNaming_);
  }

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
    m_newStructureTracker.unlock();

    m_tracker->append(s);
    m_tracker->unlock();

    submitStructure(s);
    emit structureStarted(s);
  }

  void QueueManager::appendToJobStartTracker(Structure *s)
  {
    m_jobStartTracker.lockForWrite();
    m_jobStartTracker.append(s);
    m_jobStartTracker.unlock();
  }

} // end namespace GlobalSearch

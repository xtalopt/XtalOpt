/**********************************************************************
  QueueManager - Generic queue manager to track running structures

  Copyright (C) 2010 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <globalsearch/queuemanager.h>

#include <globalsearch/optbase.h>
#include <globalsearch/structure.h>
#include <globalsearch/optimizer.h>

#include <QtCore/QDebug>
#include <QtCore/QtConcurrentRun>

namespace GlobalSearch {

  QueueManager::QueueManager(OptBase *opt, Tracker *tracker) :
    QObject(opt),
    m_checkPopulationPending(false),
    m_checkRunningPending(false),
    m_queueUpdatePending(false),
    m_requestedStructures(0),
    m_opt(opt),
    m_tracker(tracker)
  {

    trackerList.append(&m_runningTracker);
    trackerList.append(&m_submissionPendingTracker);
    trackerList.append(&m_startPendingTracker);
    trackerList.append(&m_jobStartTracker);
    trackerList.append(&m_nextOptStepTracker);
    trackerList.append(&m_errorPendingTracker);
    trackerList.append(&m_updatePendingTracker);
    trackerList.append(&m_killPendingTracker);

    reset();

    // tracker connections
    connect(m_tracker, SIGNAL(structureCountChanged(int)),
            this, SLOT(checkPopulation()));

    // opt connections
    connect(this, SIGNAL(needNewStructure()),
            m_opt, SLOT(generateNewStructure()));
    connect(m_opt, SIGNAL(sessionStarted()),
            this, SLOT(resetRequestCount()));

    // internal connections
    connect(this, SIGNAL(structureStarted(Structure *)),
            this, SIGNAL(structureUpdated(Structure *)));
    connect(this, SIGNAL(structureSubmitted(Structure *)),
            this, SIGNAL(structureUpdated(Structure *)));
    connect(this, SIGNAL(structureKilled(Structure *)),
            this, SIGNAL(structureUpdated(Structure *)));
    connect(this, SIGNAL(structureFinished(Structure *)),
            this, SIGNAL(structureUpdated(Structure *)));
  }

  QueueManager::~QueueManager()
  {
    while (m_checkPopulationPending) {};
    m_checkPopulationPending = true;
    while (m_checkRunningPending) {};
    m_checkRunningPending = true;

    reset();
  }

  void QueueManager::reset() {
    for (int i = 0; i < trackerList.size(); i++) {
      trackerList.at(i)->reset();
    }
  }

  void QueueManager::checkPopulation() {
    if (m_opt->isStarting ||
        m_opt->readOnly ||
        m_checkPopulationPending) {
      return;
    }

    m_checkPopulationPending = true;
    QtConcurrent::run(this, &QueueManager::checkPopulation_);
  }

  void QueueManager::checkPopulation_() {
    // Count jobs
    uint running = 0;
    uint optimized = 0;
    uint submitted = 0;
    m_tracker->lockForRead();
    QList<Structure*> *structures = m_tracker->list();
    m_startPendingTracker.lockForRead();
    running += m_startPendingTracker.list()->size();
    // Check to see that the number of running jobs is >= that specified:
    Structure *structure = 0;
    Structure::State state;
    int fail=0;
    for (int i = 0; i < structures->size(); i++) {
      structure = structures->at(i);
      structure->lock()->lockForRead();
      state = structure->getStatus();
      if (structure->getFailCount() != 0) fail++;
      structure->lock()->unlock();
      // Count submitted structures
      if ( state == Structure::Submitted ||
           state == Structure::InProcess ){
        m_runningTracker.append(structure);
        submitted++;
      }
      // Count running jobs and update trackers
      if ( state != Structure::Optimized &&
           state != Structure::Duplicate &&
           state != Structure::WaitingForOptimization &&
           state != Structure::Killed &&
           state != Structure::Removed ) {
        running++;
        m_runningTracker.append(structure);
      }
      else if ( state == Structure::WaitingForOptimization ) {
        running++;
        m_runningTracker.append(structure);
      }
      else if ( state == Structure::Optimized ) {
        optimized++;
        m_runningTracker.remove(structure);
      }
      else {
        m_runningTracker.remove(structure);
      }
    }
    m_startPendingTracker.unlock();
    m_tracker->unlock();
    emit newStatusOverview(optimized, running, fail);

    // Submit any jobs if needed
    m_jobStartTracker.lockForRead();
    int pending = m_jobStartTracker.list()->size();
    while (pending != 0 &&
           (
            !m_opt->limitRunningJobs ||
            submitted < m_opt->runningJobLimit
            )
           ) {
      startJob();
      submitted++;
      pending--;
    }
    m_jobStartTracker.unlock();

    // Generate requests
    while ( running + m_requestedStructures < m_opt->contStructs
            &&
            ( !m_opt->testingMode ||
              m_tracker->size() + m_requestedStructures < m_opt->test_nStructs
              )
            &&
            ( m_opt->cutoff < 0 ||
              m_opt->cutoff >= optimized + running + m_requestedStructures
              )
            ) {
      emit needNewStructure();
      m_requestedStructures++;
    }

    m_checkPopulationPending = false;
  }

  void QueueManager::checkRunning() {
    if (m_opt->isStarting ||
        m_opt->readOnly ||
        m_checkRunningPending) {
      return;
    }

    m_checkRunningPending = true;

    QtConcurrent::run(this, &QueueManager::checkRunning_);
  }

  void QueueManager::checkRunning_() {
    if (m_runningTracker.size() == 0) {
      m_checkRunningPending = false;
      return;
    }

    // Get list of running structures
    QList<Structure*> structures = getAllRunningStructures();
    Structure *s;
    // Remove duplicates
    for (int i = 0; i < structures.size(); i++) {
      s = structures.at(i);
      for (int j = i+1; j < structures.size(); j++) {
        if (s == structures.at(j)) structures.removeAt(j);
      }
    }

    // prep variables
    Structure *structure = 0;

    // Get queue data
    updateQueue();

    Structure::State status;

    for (int i = 0; i < structures.size(); i++) {
      structure = structures.at(i);

      structure->lock()->lockForRead();
      status = structure->getStatus();
      structure->lock()->unlock();

      // Check status
      switch (status) {
      case Structure::InProcess: {
        Optimizer::JobState substate = m_opt->optimizer()->getStatus(structure);
        switch (substate) {
        case Optimizer::Running:
        case Optimizer::Queued:
        case Optimizer::CommunicationError:
        case Optimizer::Unknown:
        case Optimizer::Pending:
        case Optimizer::Started:
          break;
        case Optimizer::Success:
          updateStructure(structure);
          break;
        case Optimizer::Error:
          handleStructureError(structure);
          break;
        }
        break;
      }
      case Structure::Submitted: {
        Optimizer::JobState substate = m_opt->optimizer()->getStatus(structure);
        switch (substate) {
        case Optimizer::Running:
        case Optimizer::Queued:
        case Optimizer::Error:
        case Optimizer::Success:
        case Optimizer::Started:
          structure->setStatus(Structure::InProcess);
          break;
        case Optimizer::CommunicationError:
        case Optimizer::Unknown:
        case Optimizer::Pending:
        default:
          break;
        }
        break;
      }
      case Structure::Killed:
      case Structure::Removed:
      case Structure::Duplicate:
      case Structure::Optimized:
        stopJob(structure);
        m_runningTracker.remove(structure);
        break;
      case Structure::StepOptimized:
        prepareStructureForNextOptStep(structure);
        break;
      case Structure::Error:
        handleStructureError(structure);
        break;
      case Structure::Restart:
        prepareStructureForSubmission(structure);
        break;
      case Structure::WaitingForOptimization:
      case Structure::Updating:
      case Structure::Empty:
        break;
      }

      emit structureUpdated(structure);
    }

    m_checkRunningPending = false;
  }

  void QueueManager::updateQueue()
  {
    if (m_opt->isStarting ||
        m_opt->readOnly ||
        m_queueUpdatePending) {
      return;
    }
    m_queueUpdatePending = true;
    QtConcurrent::run(this, &QueueManager::updateQueue_);
  }

  void QueueManager::updateQueue_()
  {
    m_opt->optimizer()->getQueueList(m_queueData, &m_queueDataMutex);
    m_queueUpdatePending = false;
  }

  void QueueManager::prepareStructureForNextOptStep(Structure *s) {
    m_nextOptStepTracker.append(s);
    QtConcurrent::run(this, &QueueManager::prepareStructureForNextOptStep_);
  }

  void QueueManager::prepareStructureForNextOptStep_() {
    m_nextOptStepTracker.lockForWrite();
    QList<Structure*> *nextOptStepList = m_nextOptStepTracker.list();
    if (nextOptStepList->size() == 0) {
      m_nextOptStepTracker.unlock();
      return;
    }
    Structure *structure = nextOptStepList->takeFirst();
    QWriteLocker locker (structure->lock());
    structure->stopOptTimer();

    // update optstep and relaunch if necessary
    if (structure->getCurrentOptStep() < (uint)m_opt->optimizer()->getNumberOfOptSteps()) {
      structure->setCurrentOptStep(structure->getCurrentOptStep() + 1);

      // Update input files
      locker.unlock();
      m_runningTracker.append(structure);
      structure->setStatus(Structure::WaitingForOptimization);
      prepareStructureForSubmission(structure);
    }
    // Otherwise, it's done
    else {
      structure->setStatus(Structure::Optimized);
      emit structureFinished(structure);
      m_runningTracker.remove(structure);
    }
    m_nextOptStepTracker.unlock();
    emit structureUpdated(structure);
  }

  void QueueManager::handleStructureError(Structure *s) {
    if (m_errorPendingTracker.append(s))
      QtConcurrent::run(this, &QueueManager::handleStructureError_);
  }

  void QueueManager::handleStructureError_() {
    m_errorPendingTracker.lockForWrite();
    if (m_errorPendingTracker.list()->size() == 0) {
      m_errorPendingTracker.unlock();
      return;
    }
    bool exists = false;
    int jobID;
    Structure *structure = m_errorPendingTracker.list()->takeFirst();

    // Check that structure still shows error (in case of user intervention)
    if (structure->getStatus() != Structure::Error) {
      m_errorPendingTracker.unlock();
      return;
    }

    // Check if the  job is running under a  different JobID (Stranger
    // things have happened!)
    jobID = m_opt->optimizer()->checkIfJobNameExists(structure, m_queueData, exists);
    QWriteLocker locker (structure->lock());
    structure->addFailure();
    structure->stopOptTimer();
    if (exists) { // Mark the job as running and update the jobID
      m_opt->warning(tr("QueueManager::handleStructureError_: Reclaiming jobID %1 for structure %2")
              .arg(QString::number(jobID))
              .arg(structure->getIDString()));
      structure->setStatus(Structure::InProcess);
      structure->setJobID(jobID);
    }
    else {
      // Check failure count
      if (structure->getFailCount() >= m_opt->failLimit) {
        switch (OptBase::FailActions(m_opt->failAction)) {
        case OptBase::FA_DoNothing:
        default:
          // resubmit job
          prepareStructureForSubmission(structure);
          break;
        case OptBase::FA_KillIt:
          killStructure(structure);
          break;
        case OptBase::FA_Randomize:
          structure->setStatus(Structure::Updating);
          locker.unlock();
          m_errorPendingTracker.unlock();
          m_opt->replaceWithRandom(structure, tr("excessive failures"));
          emit structureUpdated(structure);
          prepareStructureForSubmission(structure);
          return; // Return so as not to unlock the error tracker again
        }
      }
      else {
        // resubmit job
        locker.unlock();
        prepareStructureForSubmission(structure);
      }
    }
    m_errorPendingTracker.unlock();
  }

  void QueueManager::updateStructure(Structure *s) {
    m_updatePendingTracker.append(s);
    QtConcurrent::run(this, &QueueManager::updateStructure_);
  }

  void QueueManager::updateStructure_() {
    Structure *structure = 0;
    if (!m_updatePendingTracker.popFirst(structure))
      return;
    structure->stopOptTimer();
    if (!m_opt->optimizer()->update(structure)) {
      structure->setStatus(Structure::Error);
      handleStructureError(structure);
      return;
    }
    structure->setStatus(Structure::StepOptimized);
    prepareStructureForNextOptStep(structure);
  }

  void QueueManager::killStructure(Structure *s) {
    m_killPendingTracker.append(s);
    stopJob(s);
    QtConcurrent::run(this, &QueueManager::killStructure_);
  }

  void QueueManager::killStructure_() {
    Structure *structure = 0;
    if (!m_killPendingTracker.popFirst(structure)) {
      return;
    }
    QWriteLocker locker (structure->lock());
    structure->stopOptTimer();
    structure->setStatus(Structure::Killed);
    emit structureKilled(structure);
  }

  void QueueManager::prepareStructureForSubmission(Structure *s, int optStep) {
    m_submissionPendingTracker.append(s);
    s->setStatus(Structure::WaitingForOptimization);
    if (optStep != 0)
      s->setCurrentOptStep(optStep);
    QtConcurrent::run(this, &QueueManager::prepareStructureForSubmission_);
  }

  void QueueManager::prepareStructureForSubmission_() {
    m_submissionPendingTracker.lockForWrite();
    if (m_submissionPendingTracker.size() == 0) {
      m_submissionPendingTracker.unlock();
      return;
    }

    Structure *structure = m_submissionPendingTracker.list()->takeFirst();
    m_opt->optimizer()->writeInputFiles(structure);

    m_jobStartTracker.append(structure);
    m_runningTracker.append(structure);
    m_submissionPendingTracker.unlock();
  }

  void QueueManager::startJob() {
    QtConcurrent::run(this, &QueueManager::startJob_);
  }

  void QueueManager::startJob_() {
    Structure *structure;
    if (!m_jobStartTracker.popFirst(structure))
      return;

    structure->setStatus(Structure::Submitted);
    emit structureSubmitted(structure);

    // Make sure no mutexes are locked here -- this can take a while...
    if (!m_opt->optimizer()->startOptimization(structure)) {
      m_opt->warning(tr("QueueManager::submitStructure_: Job did not run successfully for structure %1-%2.")
              .arg(structure->getIDString())
              .arg(structure->getCurrentOptStep()));
      structure->lock()->lockForWrite();
      structure->setStatus(Structure::Error);
      structure->lock()->unlock();
      return;
    }
  }

  void QueueManager::stopJob(Structure *s) {
    QtConcurrent::run(m_opt->optimizer(),
                      &Optimizer::deleteJob,
                      s);
  }

  QList<Structure*> QueueManager::getAllRunningStructures() {
    m_runningTracker.lockForRead();
    QList<Structure*> list(*m_runningTracker.list());
    m_runningTracker.unlock();
    return list;
  }

  QList<Structure*> QueueManager::getAllSubmittedStructures() {
    m_tracker->lockForRead();
    m_submissionPendingTracker.lockForRead();
    m_jobStartTracker.lockForRead();
    QList<Structure*> list (*m_tracker->list());
    list.append(*m_submissionPendingTracker.list());
    list.append(*m_jobStartTracker.list());
    m_submissionPendingTracker.unlock();
    m_jobStartTracker.unlock();
    m_tracker->unlock();
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

  QList<Structure*> QueueManager::getAllPendingStructures() {
    m_startPendingTracker.lockForRead();
    QList<Structure*> list(*m_startPendingTracker.list());
    m_startPendingTracker.unlock();
    return list;
  }

  QList<Structure*> QueueManager::getAllStructures() {
    m_tracker->lockForRead();
    m_startPendingTracker.lockForRead();
    QList<Structure*> list (*m_tracker->list());
    list.append(*m_startPendingTracker.list());
    m_startPendingTracker.unlock();
    m_tracker->unlock();
    return list;
  }

  QList<Structure*> QueueManager::lockForNaming() {
    m_tracker->lockForRead();
    m_startPendingTracker.lockForRead();
    return getAllStructures();
  }

  void QueueManager::unlockForNaming(Structure *s) {
    if (!s) {
      m_startPendingTracker.unlock();
      m_tracker->unlock();
    }
    if (m_startPendingTracker.list()->contains(s)) {
      m_opt->error(tr("QueueManager::unlockForNaming: Attempt to add structure %1 twice?")
                   .arg(s->getIDNumber()));
      m_startPendingTracker.unlock();
      m_tracker->unlock();
      return;
    }
    m_requestedStructures--;
    m_startPendingTracker.list()->append(s);
    addNewStructure();
    m_startPendingTracker.unlock();
    m_tracker->unlock();
  }

  void QueueManager::addNewStructure()
  {
    QtConcurrent::run(this, &QueueManager::addNewStructure_);
  }

  void QueueManager::addNewStructure_()
  {
    m_tracker->lockForWrite();
    Structure *structure = 0;
    if (!m_startPendingTracker.popFirst(structure)) {
      m_tracker->unlock();
      return;
    }
    m_tracker->appendAndUnlock(structure);
    emit structureStarted(structure);
    prepareStructureForSubmission(structure);
  }

  void QueueManager::appendToJobStartTracker(Structure *s) {
    m_jobStartTracker.append(s);
  }

} // end namespace Avogadro

//#include "queuemanager.moc"

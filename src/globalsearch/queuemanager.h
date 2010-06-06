/**********************************************************************
  QueueManager - Generic queue manager to track running structures

  Copyright (C) 2010 by David C. Lonie

  This file is part of the Avogadro molecular editor project.
  For more information, see <http://avogadro.openmolecules.net/>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef QUEUEMANAGER_H
#define QUEUEMANAGER_H

#include <globalsearch/optbase.h>
#include <globalsearch/structure.h>
#include <globalsearch/tracker.h>
#include <globalsearch/optimizer.h>

#include <QDebug>
#include <QReadWriteLock>

namespace Avogadro {

  class QueueManager : public QObject
  {
    Q_OBJECT

  public:
    explicit QueueManager(OptBase *opt, Tracker *tracker);
    virtual ~QueueManager();


  signals:
    void structureStarted(Structure *);
    void structureSubmitted(Structure *);
    void structureKilled(Structure *);
    void structureUpdated(Structure *);
    void needNewStructure();
    void newStatusOverview(int optimized, int running, int failing);

   public slots:
    void reset();
    void checkPopulation();
    void checkRunning();
    void updateQueue(int time = 10);
    void prepareStructureForNextOptStep(Structure *s);
    void handleStructureError(Structure *s);
    void updateStructure(Structure *s);
    void killStructure(Structure *s);
    void prepareStructureForSubmission(Structure *s, int optStep=0);
    void startJob();
    void stopJob(Structure *s);
    // This should only be used when say, resuming
    // sessions. Otherwise, use prepareStructureForSubmission:
    void appendToJobStartTracker(Structure *s);

    QStringList getRemoteQueueData() {return m_queueData;};

    void resetRequestCount(int i = 0) {m_requestedStructures = i;};

    QList<Structure*> getAllRunningStructures();
    QList<Structure*> getAllSubmittedStructures();
    QList<Structure*> getAllOptimizedStructures();
    QList<Structure*> getAllDuplicateStructures();
    QList<Structure*> getAllPendingStructures(); // only those in the startPendingTracker
    QList<Structure*> getAllStructures(); // including those in the startPendingTracker

    // Locks both the main and startPending trackers for reading.
    // Returns a list of all structures that have been submitted thus
    // far.
    QList<Structure*> lockForNaming();
    // Unlocks both the main and startPending trackers. Argument is
    // a the new stucture to be added to the queuemanager and tracker.
    void unlockForNaming(Structure *s = 0);

   private slots:

   private:
    bool m_checkPopulationPending;
    bool m_checkRunningPending;
 
    int m_requestedStructures;


    OptBase *m_opt;

    Tracker *m_tracker;

    QList<Tracker*> trackerList;
    Tracker m_runningTracker;
    Tracker m_submissionPendingTracker;
    Tracker m_startPendingTracker;
    Tracker m_jobStartTracker;
    Tracker m_nextOptStepTracker;
    Tracker m_errorPendingTracker;
    Tracker m_updatePendingTracker;
    Tracker m_killPendingTracker;

    QDateTime m_queueTimeStamp;
    QStringList m_queueData;

    void checkPopulation_();
    void checkRunning_();
    void addNewStructure();
    void addNewStructure_();
    void prepareStructureForNextOptStep_();
    void handleStructureError_();
    void updateStructure_();
    void startStructure_();
    void killStructure_();
    void prepareStructureForSubmission_();
    void startJob_();
    void stopJob_();
  };

} // end namespace Avogadro

#endif

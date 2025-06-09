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

#ifndef QUEUEMANAGER_H
#define QUEUEMANAGER_H

#include <globalsearch/tracker.h>

class QDateTime;

namespace GlobalSearch {
class SearchBase;
class Structure;

/**
 * @class QueueManager queuemanager.h <globalsearch/queuemanager.h>
 *
 * @brief The QueueManager monitors the running jobs and updates
 * Structure status.
 *
 * @author David C. Lonie
 *
 * The QueueManager creates a local queue and monitoring system
 * control the submission of Structures to an optimization
 * engine or queue.
 *
 * For basic usage, connect the needNewStructure() signal to slot
 * that will generate a new Structure and submit it the following
 * way:
@verbatim
// lockForNaming() returns a list of all structures that the main
// tracker is aware of. It also locks a naming mutex to prevent
// simultaneous naming of Structures, avoiding similar
// Structure indices, ID numbers, etc.
QList<Structure*> allStructures = m_queue->lockForNaming();
// Check the Structures in allStructures to determine the next
// available name. The follow code uses a generation and ID number
// for an evolutionary/genetic algorithm. Other methods may only set
// the ID number. (note that the generation number is set already
// in the following example).
Structure *structure;
uint id = 1;
for (int j = 0; j < allStructures.size(); j++) {
structure = allStructures.at(j);
structure->lock().lockForRead();
if (structure->getGeneration() == generation &&
    structure->getIDNumber() >= id) {
  id = structure->getIDNumber() + 1;
}
structure->lock().unlock();
}

// Assign data to structure (created elsewhere)
QWriteLocker locker (newStructure->lock());
newStructure->setIDNumber(id);
newStructure->setGeneration(generation);
newStructure->setParents(parents);
// Determine, create, and assign paths
QString id_s, gen_s, locpath_s, rempath_s;
id_s.sprintf("%05d",structure->getIDNumber());
gen_s.sprintf("%05d",structure->getGeneration());
locpath_s = locWorkDir + "/" + gen_s + "x" + id_s + "/";
rempath_s = remWorkDir + "/" + gen_s + "x" + id_s + "/";
QDir dir (locpath_s);
if (!dir.exists()) {
if (!dir.mkpath(locpath_s)) {
  // Output error
}
}
newStructure->setLocpath(locpath_s);
newStructure->setRempath(rempath_s);
newStructure->setCurrentOptStep(0);
newStructure->findSpaceGroup();
// unlockForNaming(Structure*) unlocks the naming mutex and
// begins processing the structure that is passed.
m_queue->unlockForNaming(newStructure);
@endverbatim
 *
 * Most functions in this class do not need to be called as they are
 * automatically called when needed. Check the source if in doubt.
 */
class QueueManager : public QObject
{
  Q_OBJECT

public:
  /**
   * Constructor.
   *
   * @param thread A QThread instance to run in
   * @param parent The SearchBase class the QueueManager uses
   */
  explicit QueueManager(QThread* thread, SearchBase* parent);

  /**
   * Destructor.
   */
  virtual ~QueueManager();

signals:
  /**
  * Emitted when a structure has successfully finished
  * all optimization steps and is ready for objective calculation
  * @param s Structure
  */
  void readyForObjectiveCalculations(GlobalSearch::Structure* s);

  /**
   * Emitted when the QueueManager has been moved to it's final
   * thread and is ready to accept connections.
   */
  void movedToQMThread();

  /**
   * Emitted when a Structure is accepted into the queuemanager
   * (e.g. after submission through unlockForNaming(Structure*)
   * @param s The Structure that has been accepted
   * @sa lockForNaming
   * @sa unlockForNaming
   */
  void structureStarted(GlobalSearch::Structure* s);

  /**
   *  Emitted when a Structure is submitted for optimization
   * @param s The Structure that has been submitted
   */
  void structureSubmitted(GlobalSearch::Structure* s);

  /**
   * Emitted when a Structure has been killed through
   * killStructure(Structure*)
   * @param s The Structure that has been killed
   * @sa killStructure
   */
  void structureKilled(GlobalSearch::Structure* s);

  /**
   * Emitted when a Structure has changed status. Useful for
   * updating progress tables, plots, etc
   * @param s The Structure that has been updated
   */
  void structureUpdated(GlobalSearch::Structure* s);

  /**
   * Emitted when a Structure has completed all optimization steps.
   *
   * @param s The Structure that has been updated
   */
  void structureFinished(GlobalSearch::Structure* s);

  /**
   * Emitted when the hull calculation is finished (to properly update GUI)
   *
   * @param s The hull info has been updated
   */
  void hullCalculationFinished();

  /**
   * Emitted when the number of unoptimized Structures drops below
   * SearchBase::contStructs. This is connected to
   * SearchBase::generateNewStructure() by default.
   */
  void needNewStructure();

  /**
   * Emitted when checkPopulation() is called to provide a short
   * summary of the queuemanager's status.
   *
   * @param optimized Number of optimized structures
   * @param running Number of running structures (e.g. submitted
   * for optimization)
   * @param failing Number of structures with a getFailCount() > 0
   */
  void newStatusOverview(int optimized, int running, int failing, int total);

// Work around for Qt 4.6.3
#if QT_VERSION == 0x040603
  void newStructureQueued();
#endif // QT_VERSION == 4.6.3

public slots:
  /**
   * Reset all trackers in trackerList
   */
  void reset();

  /**
   * Stops any running optimization processes associated with a
   * structure and sets its status to Structure::Killed.
   *
   * The structureKilled signal is emitted as well.
   *
   * @param s The Structure to kill.
   * @sa structureKilled
   */
  void killStructure(Structure* s);

  /**
   * Appends a Structure to m_jobStartTracker. This should not be
   * used unless resuming a session, and then only for structures
   * that are marked Structure::WaitingForOptimization.
   *
   * All other cases should use
   * prepareStructureForSubmission(Structure*)
   *
   * @param s The Structure to be appended
   * @sa prepareStructureForSubmission
   */
  void appendToJobStartTracker(Structure* s);

  /**
   * @return All Structures in m_runningTracker
   */
  QList<Structure*> getAllRunningStructures();

  /**
   * @return All Structures in m_tracker with status
   * Structure::Optimized.
   */
  QList<Structure*> getAllOptimizedStructures();

  /**
   * @return All Structures in m_tracker with status
   * Structure::Optimized, and their hull and objectives calculated
   */
  QList<Structure*> getAllParentPoolStructures();

  /**
   * @return All Structures in m_tracker with status
   * Structure::Similar.
   */
  QList<Structure*> getAllSimilarStructures();

  /**
   * @return All Structures in m_tracker and m_startPendingTracker
   */
  QList<Structure*> getAllStructures();

  /**
   * Locks the m_tracker and m_newStructure for
   * reading.
   *
   * @return All Structures in getAllStructures()
   */
  QList<Structure*> lockForNaming();

  /**
   * Unlocks both the main and startPending trackers.
   *
   * @param s Optional new stucture to be added to the queuemanager
   * and tracker.
   */
  void unlockForNaming(Structure* s = 0);

protected slots:
  /**
   * Check for the status of finished objective
   * calculations, and label the structure accordingly for
   * further processing with appropriate handler.
   * @param s Structure whose objectives are calculated
   */
  void updateStructureObjectiveState(Structure* s);

  /**
   * This is called automatically when the QueueManager is
   * started. This function sets up a simple event loop that will
   * run checkPopulation and checkRunning regularly.
   */
  void checkLoop();

  /**
   * Writes the input files for the optimization process and queues
   * the Structure to be submitted for optimization.
   *
   * @param s The Structure to be submitted
   * @param optStep The optStep to perform. s->currentOptStep is
   * used if optStep==0.
   */
  void addStructureToSubmissionQueue(GlobalSearch::Structure* s, int optStep);

  /**
   * @overload
   *
   * Writes the input files for the optimization process and queues
   * the Structure to be submitted for optimization at its current
   * optStep.
   *
   * @param s The Structure to be submitted
   */
  void addStructureToSubmissionQueue(GlobalSearch::Structure* s)
  {
    addStructureToSubmissionQueue(s, -1);
  }

  /**
   * Move \b this to the QThread specified in the constructor and
   * setup connections in that thread's event loop.
   */
  void moveToQMThread();

  /**
   * Called by moveToQMThread(), this function installs connections
   * in the owning thread's event loop.
   */
  void setupConnections();

protected:
  /// Cached pointer to main searchbase class
  SearchBase* m_search;

  /// Pointer to the thread where the queuemanager lives
  QThread* m_thread;

  /// Convenience pointer to m_search->tracker()
  Tracker* m_tracker;

  /**
   * Update the data from the remote PBS queue.
   */
  void updateQueue();

  /**
   * Called on Structures that are Structure::StepOptimized, this
   * function will update the Structure with the results of the
   * optimization and pass it along to
   * prepareStructureForNextOptStep.
   *
   * @param s The step optimized structure
   * @sa prepareStructureForNextOptStep
   */
  void updateStructure(Structure* s);

  /**
   * Submits the first Structure in m_jobStartTracker for
   * submission. This should not be called directly, instead call
   * prepareStructureForSubmission(Structure*).
   *
   * @sa prepareStructureForSubmission
   */
  void startJob();

  /**
   * Kills the optimization process for the indicated Structure.
   *
   * @param s The Structure to stop optimizing.
   */
  void stopJob(Structure* s);

  /**
   * Check all Structures in the main Tracker and assign them to
   * other trackers as needed (runningTracker, etc.).
   *
   * If more structures are needed, they are requested in this
   * function by emitting needNewStructure().
   *
   * This function also submits new structures to the optimization
   * engine if needed.
   *
   * Also emits newStatusOverview for a summary of the queue's
   * status.
   *
   * @sa newStatusOverview
   * @sa needNewStructure
   */
  void checkPopulation();

  /**
   * Check the conditions for soft/hard exit.
   * For a soft exit, checks for no running/pending jobs and quit with a delay.
   * For a hard exit, the acting function will be called for an immediate quit.
   */
  void checkExit();

  /**
   * Monitors the Structures in getAllRunningStructures() and
   * updates their statuses if they've changed.
   *
   * @note Do note call this function directly; it is called
   * automatically by the checkLoop function
   */
  void checkRunning();

  /**
   * Perform actions on the Optimized Structure \a s.
   *
   * @param s Structure of interest
   */
  void handleOptimizedStructure(Structure* s);

  /**
   * Perform actions on the StepOptimized Structure \a s.
   *
   * @param s Structure of interest
   */
  void handleStepOptimizedStructure(Structure* s);

  /**
   * Perform actions on the WaitingForOptimization Structure \a s.
   *
   * @param s Structure of interest
   */
  void handleWaitingForOptimizationStructure(Structure* s);

  /**
   * Perform actions on the InProcess Structure \a s.
   *
   * @param s Structure of interest
   */
  void handleInProcessStructure(Structure* s);

  /**
   * Perform actions on the Empty Structure \a s.
   *
   * @param s Structure of interest
   */
  void handleEmptyStructure(Structure* s);

  /**
   * Perform actions on the Updating Structure \a s.
   *
   * @param s Structure of interest
   */
  void handleUpdatingStructure(Structure* s);

  /**
   * Perform actions on the Error'd Structure \a s.
   *
   * @param s Structure of interest
   */
  void handleErrorStructure(Structure* s);

  /**
   * Perform actions on the Submitted Structure \a s.
   *
   * @param s Structure of interest
   */
  void handleSubmittedStructure(Structure* s);

  /**
   * Perform actions on the Killed Structure \a s.
   *
   * @param s Structure of interest
   */
  void handleKilledStructure(Structure* s);

  /**
   * Perform actions on the Removed Structure \a s.
   *
   * @param s Structure of interest
   */
  void handleRemovedStructure(Structure* s);

  /**
   * Perform actions on the similar Structure \a s.
   *
   * @param s Structure of interest
   */
  void handleSimilarStructure(Structure* s);

  /**
   * Perform actions on the Restart'ing Structure \a s.
   *
   * @param s Structure of interest
   */
  void handleRestartStructure(Structure* s);

  /**
   * Perform actions on a structure that has successfully
   * finished objective calculations
   *
   * @param s Structure whose objectives are calculated
   */
  void handleRetainObjective(Structure* s);

  /**
   * Perform actions on a structure dismissed by a filtering objective
   *
   * @param s Structure whose objectives are calculated
   */
  void handleDismissObjective(Structure* s);

  /**
   * Perform actions on a structure failed in objective calculation
   *
   * @param s Structure whose objectives are calculated
   */
  void handleFailObjective(Structure* s);

  // These run in the background and are called by the above
  // functions via QtConcurrent::run.
  /// @cond
  void handleOptimizedStructure_(Structure* s);
  void handleStepOptimizedStructure_(Structure* s);
  void handleInProcessStructure_(Structure* s);
  void handleErrorStructure_(Structure* s);
  void handleSubmittedStructure_(Structure* s);
  void handleKilledStructure_(Structure* s);
  void handleSimilarStructure_(Structure* s);
  void handleRestartStructure_(Structure* s);
  void handleRetainObjective_(Structure* s);
  void handleFailObjective_(Structure* s);
  void handleDismissObjective_(Structure* s);
  /// @endcond

  // Other background handlers
  /// @cond
  void addStructureToSubmissionQueue_(Structure* s, int optStep);
  void startJob_(Structure* s);

#if QT_VERSION == 0x040603
protected slots:
#endif // QT_VERSION == 4.6.3
  void unlockForNaming_();
#if QT_VERSION == 0x040603
protected:
#endif // QT_VERSION == 4.6.3
  /// @endcond

  // Trackers for above handlers
  /// @cond
  Tracker m_newlyOptimizedTracker;
  Tracker m_stepOptimizedTracker;
  Tracker m_inProcessTracker;
  Tracker m_errorTracker;
  Tracker m_submittedTracker;
  Tracker m_newlyKilledTracker;
  Tracker m_newSimilarityTracker;
  Tracker m_restartTracker;
  Tracker m_newSubmissionTracker;
  Tracker m_objectiveRetainTracker;
  Tracker m_objectiveFailTracker;
  Tracker m_objectiveDismissTracker;
  /// @endcond

  /// Tracks which structures are currently running
  Tracker m_runningTracker;
  /// Tracks which structures are queued to be submitted
  Tracker m_jobStartTracker;
  /// Tracks structures that have been returned from m_search but have
  /// not yet been accepted into m_tracker.
  Tracker m_newStructureTracker;

  /// Number of structure requests pending.
  int m_requestedStructures;

  /// Boolean set to true while the destructor is running.
  bool m_isDestroying;

  /// Used to throttle job submissions
  QDateTime* m_lastSubmissionTimeStamp;
};

} // end namespace GlobalSearch

#endif

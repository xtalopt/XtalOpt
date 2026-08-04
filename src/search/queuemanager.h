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

#ifndef QUEUEMANAGER_H
#define QUEUEMANAGER_H

#include <search/constants.h>
#include <search/tracker.h>

#include <QDateTime>
#include <QHash>
#include <QPair>
#include <QSet>
#include <QMutex>
#include <QString>
#include <QThreadPool>
#include <QWaitCondition>
#include <atomic>
#include <memory>

namespace Search {
class SearchBase;
class Structure;

/**
 * @class QueueManager queuemanager.h <search/queuemanager.h>
 *
 * @brief Monitor running jobs and move Structures through their states.
 *
 * @author David C. Lonie
 *
 * After a search starts, the queue thread checks jobs about once a second.
 * It asks for new structures when needed, submits waiting jobs, and starts
 * the work for each running Structure state. Only one function runs for a
 * Structure and state at a time.
 *
 * The application adds structures with addNewStructure(),
 * addStructureToSubmissionQueue(), and killStructure(). SearchBase keeps
 * the search settings and structure functions. QueueManager moves structures
 * through their states. QueueInterface handles jobs and files. Optimizer reads
 * the results.
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

  /** Stop accepting new work before the queue thread stops. */
  void prepareForThreadStop();

  /**
   * Set the delay (in ms) between check loop passes. The default is 1000.
   */
  void setCheckInterval(int ms) { m_checkInterval = ms; }

signals:
  /**
   * Emitted when the QueueManager has been moved to it's final
   * thread and is ready to accept connections.
   */
  void movedToQMThread();

  /**
   * Emitted when a Structure is accepted into the queuemanager.
   * @param s The Structure that has been accepted
   * @sa addNewStructure
   */
  void structureStarted(Search::Structure* s);

  /**
   *  Emitted when a Structure is submitted for optimization
   * @param s The Structure that has been submitted
   */
  void structureSubmitted(Search::Structure* s);

  /**
   * Emitted when a Structure has been killed through
   * killStructure(Structure*)
   * @param s The Structure that has been killed
   * @sa killStructure
   */
  void structureKilled(Search::Structure* s);

  /**
   * Emitted when a Structure has changed status. Useful for
   * updating progress tables, plots, etc
   * @param s The Structure that has been updated
   */
  void structureUpdated(Search::Structure* s);

  /**
   * Emitted when a Structure has completed all optimization and
   * post-optimization processing steps.
   *
   * @param s The Structure that has been updated
   */
  void structureFinished(Search::Structure* s);

  /**
   * Emitted when the hull calculation is finished (to properly update GUI)
   *
   * @param s The hull info has been updated
   */
  void hullCalculationFinished();

  /**
   * Emitted when the number of unoptimized Structures drops below
   * SearchBase::contStructs. Concrete searches connect this to their
   * SearchBase::generateNewStructure() implementation.
   */
  void needNewStructure();

  /**
   * Emitted when checkPopulation() is called to provide a short
   * summary of the queuemanager's status.
   *
   * @param optimized Number of optimized structures
   * @param running Number of running structures (e.g. submitted
   * for optimization)
   * @param failing Number of structures stopped by failure, dismissal, or kill
   */
  void newStatusOverview(int optimized, int running, int failing, int total);


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
   * Restores QueueManager tracking for a loaded structure.
   * @param s The restored Structure.
   * @param queueWaitingForOptimization Whether a waiting structure should be
   * queued for submission.
   * @sa addStructureToSubmissionQueue
   */
  void trackRestoredStructure(Structure* s, bool queueWaitingForOptimization);

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
   * @return All Structures in m_tracker that are similar to another structure.
   */
  QList<Structure*> getAllSimilarStructures();

  /**
   * @return All Structures in m_tracker and m_startPendingTracker
   */
  QList<Structure*> getAllStructures();

  /**
   * Assign the identity and paths to a generated Structure, then add it to
   * the queue. On success the Tracker keeps @p s; on failure it is deleted.
   */
  void addNewStructure(Structure* s, uint generation, const QString& parents);

  /**
   * Report that a requested structure could not be generated.
   */
  void structureGenerationFailed();

  /**
   * Wait until all objective/constraint script launches have returned.
   * @param timeoutMs Wait limit in milliseconds; -1 waits without limit.
   * @return True when no script launch is left running.
   */
  bool waitForScriptCalculations(int timeoutMs);

protected slots:
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
  void addStructureToSubmissionQueue(Search::Structure* s, int optStep);

  /**
   * @overload
   *
   * Writes the input files for the optimization process and queues
   * the Structure to be submitted for optimization at its current
   * optStep.
   *
   * @param s The Structure to be submitted
   */
  void addStructureToSubmissionQueue(Search::Structure* s)
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

  /** Start the periodic check loop; must run on the queue thread. */
  void startCheckLoop();

  /** Submit queued jobs without waiting for the periodic engine tick. */
  void submitQueuedJobs();

protected:
  /// Cached pointer to main searchbase class
  SearchBase* m_search;

  /// Pointer to the thread where the queuemanager lives
  QThread* m_thread;

  /// Delay between checkLoop() passes in ms; see setCheckInterval().
  int m_checkInterval = QUEUE_CHECK_INTERVAL;

  /// Convenience pointer to m_search->tracker()
  Tracker* m_tracker;

  /**
   * Update queue-side bookkeeping before status polling.
   */
  void updateQueue();

  /**
   * Called on Structures that are Structure::StepOptimized, this
   * function will update the Structure with the results of the
   * optimization and move it toward the next queue state.
   *
   * @param s The step optimized structure
   * @sa prepareStructureForNextOptStep
   */
  void updateStructure(Structure* s);

  /**
   * Submits @p s for optimization. @p s must already have been popped from
   * m_jobStartTracker by the caller (see checkPopulation()); this can block
   * for a while, so it must not be called while holding
   * m_jobStartTracker's lock.
   */
  void startJob(Structure* s);

  /**
   * Kills the optimization process for the indicated Structure.
   *
   * @param s The Structure to stop optimizing.
   */
  bool stopJob(Structure* s);
  void stopJobAndRemoveFromRunning(Structure* s);

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
   * Replace a failed/dismissed structure and put it back into the restart
   * flow. If replacement fails, mark it with @p failureState and emit the
   * terminal update signals.
   */
  void replaceStructureForRestart(Structure* s, int action, const QString& reason, int failureState,
                                  const QString& failureMessage);

  // Structure state functions. Only one function can run for the same
  //   structure and state at a time.

  /** One entry for each structure state function. */
  enum StructureHandlerType
  {
    OptimizedHandler = 0,
    StepOptimizedHandler,
    InProcessHandler,
    ErrorHandler,
    SubmittedHandler,
    KilledHandler,
    RestartHandler,
    PostprocessingHandler,
    ObjectiveFailHandler,
    ObjectiveDismissHandler,
    SubmissionHandler,
    JobStartHandler,
    NamingHandler,
    KillRequestHandler,
    ScriptLaunchHandler
  };

  /**
   * Run @p handler for @p s, unless it is already running.
   */
  void runHandler(Structure* s, StructureHandlerType handler);

  // Functions called for each structure state.
  /// @cond
  void handleOptimizedStructure(Structure* s);
  void handleStepOptimizedStructure(Structure* s);
  void handleInProcessStructure(Structure* s);
  void handleErrorStructure(Structure* s);
  void handleSubmittedStructure(Structure* s);
  void handleKilledStructure(Structure* s);
  void handleRestartStructure(Structure* s);
  void handlePostprocessingStructure(Structure* s);
  void handleFailObjectiveStructure(Structure* s);
  void handleDismissObjectiveStructure(Structure* s);
  void handleSubmissionStructure(Structure* s, int optStep);
  /// @endcond

  /**
   * Keep track of structure functions and wait for them at shutdown.
   */
  class RunningHandlers
  {
  public:
    bool tryStart(Structure* s, int h);

    void finish(Structure* s, int h);

    bool hasHandlerFor(Structure* s) const;

    bool hasHandler(Structure* s, int h) const;

    bool waitForAll(int timeoutMs);

    void clear();

  private:
    mutable QMutex m_mutex;
    QWaitCondition m_emptyWait;
    QSet<QPair<Structure*, int>> m_pairs;
    QHash<Structure*, int> m_perStructure;
  };

  RunningHandlers m_runningHandlers;

  // Bookkeeping for structures during objective/constraint run.
  struct ScriptWaitInfo
  {
    QDateTime started;
    QDateTime nextCheck;
  };
  mutable QMutex m_scriptWaitsMutex;
  QHash<Structure*, ScriptWaitInfo> m_scriptWaits;

  // Thread pool reserved for objective/constraint scripts
  QThreadPool m_objectiveThreadPool;

  /**
   * Launch the objective or constraint scripts for @p s on the objective
   * thread pool. A handler slot stays live until the launch returns.
   */
  void startScriptCalculations(Structure* s, bool constraints);

  /**
   * The engine's checker for structure's objective/constraint calculations.
   */
  void watchScriptCalculation(Structure* s, bool constraints);

  /**
   * Remove objc/cons timing data for a single calculation
   */
  void clearScriptWait(Structure* s);

  // Other background handlers
  /// @cond
  void unlockForNaming_();

  /**
   * Locks the m_tracker and m_newStructureTracker for
   * reading.
   *
   * @return All Structures in getAllStructures()
   */
  QList<Structure*> lockForNaming();

  /**
   * Unlocks both the main and new structure trackers.
   *
   * @param s Optional new stucture to be added to the queuemanager
   * and tracker.
   */
  void unlockForNaming(Structure* s = 0);
  uint nextIdForGeneration(uint generation, const QList<Structure*>& allStructures);
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

  void decrementRequestedStructures();

  /// Boolean set to true while the queue is being stopped.
  std::atomic<bool> m_isDestroying;

  /// Lets the destructor wait for pending work instead of busy-waiting.
  QMutex m_destroyMutex;
  QWaitCondition m_destroyWait;

  /// Prevents simultaneous naming of structures; also caches the next id
  /// per generation.
  QMutex m_namingMutex;
  QHash<uint, uint> m_nextIdByGeneration;
};

} // end namespace Search

#endif

/**********************************************************************
  SearchBase - Base class for global search extensions

  Copyright (C) 2010-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef SEARCH_H
#define SEARCH_H

#include <common/compatibility/platform_compat.h>
#include <common/output.h>

#include <search/optsteps.h>

#include <QHash>
#include <QObject>
#include <QReadWriteLock>
#include <QSet>

class QThread;

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace Search {
class Structure;
class Tracker;
class Optimizer;
class QueueManager;
class QueueInterface;
class SSHManager;

/**
 * @class SearchBase search.h <search/search.h>
 *
 * @brief The SearchBase class stores variables and helper functions
 * for global searches.
 *
 * @author David C. Lonie
 *
 * SearchBase is the main class in the search engine. It contains the
 * variables and functions shared by the search programs. This class ties
 * the other engine classes together.
 *
 * The application makes, replaces, checks, saves, and prints structures.
 * QueueManager handles them after they enter the queue.
 *
 * The main thread runs the application and user interface. The queue thread
 * checks jobs. Other threads make structures, save them, and run objective
 * and constraint scripts.
 *
 * When more than one lock is needed, take the settings lock, the QueueManager
 * naming lock, tracker locks, and then structure locks. Do not hold a lock
 * while waiting for another thread or calling the queue. Do not hold a
 * Structure lock while calling the queue.
 *
 * Values that change during a search must use runtimeSettingsLock().
 */
class SearchBase : public QObject
{
  Q_OBJECT

public:
  /**
   * Constructor
   *
   * @param parent QObject parent.
   */
  explicit SearchBase(QObject* parent = nullptr);

  /**
   * Destructor
   */
  virtual ~SearchBase() override;

  /**
   * Types of the optimization for objectives in a multi-objective run
   */
  enum ObjType
  {
    // Minimization objective
    Ot_Min = 0,
    // Maximization objective
    Ot_Max
  };

  /**
   * Definition of one external/user objective.
   */
  struct ObjectiveInfo
  {
    ObjectiveInfo()
      : type(Ot_Min), weight(0.0)
    {
    }

    ObjectiveInfo(ObjType objectiveType, const QString& executable, const QString& outputFile,
                  double objectiveWeight)
      : type(objectiveType), exe(executable), out(outputFile),
        weight(objectiveWeight)
    {
    }

    ObjType type;
    QString exe;
    QString out;
    double weight;
  };

  /**
   * Definition of one constrained-search script.
   */
  struct ConstraintInfo
  {
    ConstraintInfo()
    {
    }

    ConstraintInfo(const QString& executable, const QString& outputFile)
      : exe(executable), out(outputFile)
    {
    }

    QString exe;
    QString out;
  };

  /**
   * Actions to take when a structure has failed optimization too
   * many times.
   *
   * @sa SearchBase::failAction
   * @sa SearchBase::failLimit
   */
  enum FailActions
  {
    // Do nothing; keep submitting for optimization
    FA_DoNothing = 0,
    // Kill the structure
    FA_KillIt,
    // Replace the failing structure with a new random one
    FA_Randomize,
    // Replace with a new offspring structure
    FA_NewOffspring
  };

  /**
   * The global optimization scheme (used for parent selection).
   */
  enum OptimizationType
  {
    // Scalar generalized fitness function
    OT_Basic = 0,
    // Pareto optimization
    OT_Pareto
  };

  //
  // Engine identity
  //

  /**
   * @return An ID string that uniquely identifies this SearchBase.
   */
  QString getIDString() const { return m_idString; }
  void setSearchIDString(const QString& s) { m_idString = s; }

  //
  // Structure handling functions.
  //

  /**
   * Replace the Structure with an appropriate random Structure.
   *
   * @param s The Structure to be replaced. This pointer remains
   * valid -- the structure it points to will be modified.
   * @param reason Reason for replacing. This will appear in the
   * Structure::getParents() string. (Optional)
   *
   * @return The pointer to the structure (same as s).
   */
  virtual Structure* replaceWithRandom(Structure* /*s*/,
                                       const QString& /*reason*/ = "")
  {
    return nullptr;
  }

  /**
   * Replace the Structure with a new offspring. This only makes sense if
   * the search method uses offspring (e.g. a GA). The default
   * implementation of this method calls replaceWithRandom().
   *
   * @param s The Structure to be replaced. This pointer remains
   * valid; the structure it points to will be modified.
   * @param reason Reason for replacing. This will appear in the
   * Structure::getParents() string. (Optional)
   *
   * @return The pointer to the structure (same as s).
   */
  virtual Structure* replaceWithOffspring(Structure* s,
                                          const QString& reason = "")
  {
    return replaceWithRandom(s, reason);
  }

  /**
  * Perform any post-optimization checks that need to be performed when a
  * structure enters the Structure::StepOptimized state.
  * @param s Structure to check
  * @param err If non-NULL, will be overwritten with an explaination of
  * why the check failed.
  * @return True if structure passes, false otherwise.
  */
  virtual bool checkStepOptimizedStructure(Structure* s, QString* err = nullptr)
  {
    Q_UNUSED(s);
    Q_UNUSED(err);
    return true;
  }

  //
  // Parent selection functions.
  //

  /**
   * Select a parent structure from the in-memory parent pool table.
   *
   * The probability of a structure being selected may be obtained from either
   *   multi-objective scalar function or Pareto optimization.
   *
   * @param poolSize The size of the parent pool. After probabilities are
   *                generated, the low probability structures will be trimmed
   *                off until poolSize is reached.
   *
   * @return The selected structure, or nullptr on failure.
   */
  Structure* selectParentStructure(size_t poolSize);

  // Select one tournament winner in parent selection.
  int selectTournamentParent(const QList<Structure*>& structures, const std::vector<int>& strFrnt,
                             const std::vector<double>& strDist,
                             int str_a, int str_b, int total) const;

  //
  // Template interpretation functions.
  // SearchBase handles its own generic keywords; application can use an over-ride for its keywords.
  //

  /**
   * Takes a template and inserts structure specific information by
   * replacing recognized %keyword% entries. Use %% for a literal percent
   * character.
   *
   * @param templateString Template
   * @param structure Structure of interest
   *
   * @return Interpreted template with structure information
   * included
   * @sa getTemplateKeywordHelp
   * @sa getTemplateKeywordHelp_base
   * @sa interpretTemplate_base
   */
  virtual QString interpretTemplate(const QString& templateString,
                                    Structure* structure);

  /**
   * @return A QString defining all known keywords.
   */
  virtual QString getTemplateKeywordHelp()
  {
    return getTemplateKeywordHelp_base();
  };

  //
  // Search objects.
  //

  /**
   * @return A pointer to the main Structure Tracker.
   */
  Tracker* tracker() { return m_tracker.get(); };

  /**
   * @return A pointer to the associated QueueManager.
   */
  QueueManager* queue() { return m_queue.get(); }

  /**
     Queue thread. Direct-run processes use this thread.
   */
  QThread* queueThread() const { return m_queueThread.get(); };

  /**
     Lock for settings that may change during a search.
   */
  QReadWriteLock* runtimeSettingsLock() const
  {
    return &m_runtimeSettingsLock;
  }

  /**
   * @return Thread that restored structures should move to for an active resume.
   */
  QThread* restoredStructureThread() const;

  /**
     Add a saved structure. Queue waiting structures when requested.
   */
  void addRestoredStructure(Structure* structure, bool queueWaitingForOptimization);

  //
  // User interface functions.
  //

  /**
     Set the user question function.
   */
  void setDecisionPromptHandler(const std::function<bool(const QString&, bool)>& handler)
  {
    m_decisionPromptHandler = handler;
  }

  /**
   * Set the password question function.
   */
  void setPasswordPromptHandler(const std::function<bool(const QString&, QString&)>& handler)
  {
    m_passwordPromptHandler = handler;
  }

  /**
     Clear the question functions. Calls without one fail in debug builds.
   */
  void clearPromptHandlers()
  {
    m_decisionPromptHandler = [](const QString&, bool defaultValue) {
      Q_ASSERT_X(false, Q_FUNC_INFO,
                 "Decision prompt handler was used before an interface was installed.");
      return defaultValue;
    };
    m_passwordPromptHandler = [](const QString&, QString&) {
      Q_ASSERT_X(false, Q_FUNC_INFO,
                 "Password prompt handler was used before an interface was installed.");
      return false;
    };
  }

  //
  // Queue and optimizer settings.
  //

  /**
   * Create a queue interface with a given name.
   *
   * By default, the name is looked up in the registered queue interfaces.
   * Override this function in derived classes to change the allowed
   * queue interfaces.
   *
   * @param queueName The name of the queue interface.
   *
   * @return A unique_ptr rvalue with the queue interface pointer stored in
   *         it. Contains a null pointer if @p queueName is invalid.
   */
  virtual std::unique_ptr<QueueInterface> createQueueInterface(
    const std::string& queueName);

  /**
   * Create an optimizer with a given name.
   *
   * By default, the name is looked up in the registered optimizers.
   * Override this function in derived classes to change the allowed
   * optimizers.
   *
   * @param optName The name of the optimizer.
   *
   * @return A unique_ptr rvalue with the optimizer pointer stored in
   *         it. Contains a null pointer if @p optName is invalid.
   */
  virtual std::unique_ptr<Optimizer> createOptimizer(
    const std::string& optName);

  /**
   * @return A pointer to the associated QueueManager.
   * @sa setQueueInterface
   * @sa queueInterfaceChanged
   */
  QueueInterface* queueInterface(int optStep) const
  {
    return m_optSteps.queueInterface(optStep);
  }

  /**
   * @return Get the index of the queue interface pointer or -1 if it does
   *         not exist.
   */
  int queueInterfaceIndex(const QueueInterface* qi) const
  {
    return m_optSteps.indexOf(qi);
  }

  /**
   * @return A pointer to the current Optimizer.
   * @sa setOptimizer
   * @sa optimizerChanged
   */
  Optimizer* optimizer(int optStep) const
  {
    return m_optSteps.optimizer(optStep);
  }

  /**
   * @return A pointer to the SSHManager instance.
   */
  SSHManager* ssh() { return m_ssh.get(); }

  // Remote queue settings used by QueueManager and SSH setup.

  /**
   * Set the refresh interval for checking remote jobs.
   *
   * @param i The interval in seconds
   */
  void setQueueRefreshInterval(int i) { m_queueRefreshInterval = i; }

  /**
   * Get the queue refresh interval for remote jobs in seconds
   */
  int queueRefreshInterval() const { return m_queueRefreshInterval; }

  /**
   * Set whether or not to clean remote directories when a job finishes.
   *
   * @b Whether or not to clean remote directories when a job finishes
   */
  void setCleanRemoteOnStop(bool b) { m_cleanRemoteOnStop = b; }

  /**
   * Get whether or not to clean remote directories when a job finishes.
   */
  bool cleanRemoteOnStop() const { return m_cleanRemoteOnStop; }

  // Runtime exit handling. QueueManager decides when this is needed; SearchBase
  // performs the common shutdown work.

  /**
   * Performs a soft/hard exit: emits the final session signals, closes
   * ssh, etc.
   * @param delay Amount of time (seconds) to wait before quitting; in case
   *   we need to make sure all files are written/copied properly, e.g., a soft_exit
   */
  void performTheExit(int delay = 0);

  //
  // Objective/constraint execution and ranking
  // QueueManager asks SearchBase to run external objective/constraint scripts.
  // SearchBase keeps one value slot per optimization objective. External
  // scripts fill some slots; the derived search may fill others later from
  // its own application-specific data. Ranking only uses structures whose
  // objective values are all filled in; no slot has any special meaning
  // here.
  //

  /**
   * Starts user-defined objective runs for structure @p s.
   * @param s The structure whose objectives is to be calculated.
   */
  bool startObjectiveCalculations(Structure* s);

  /**
   * Start constrained-search script execution for @p s.
   */
  bool startConstraintCalculations(Structure* s);

  /**
   * Finalize the objective script execution for @p s.
   */
  bool finishObjectiveCalculations(Structure* s);

  /**
   * Finalize constrained-search script execution for @p s.
   */
  bool finishConstraintCalculations(Structure* s);

  /**
   * Delete objc/const script outputs from any earlier run.
   */
  bool removeOldScriptOutputs(Structure* s, bool constraints);

  // Multi-objective: add an objective
  void addObjective(ObjType type, const QString& exe = QString(), const QString& out = QString(),
                    double weight = 0.0)
  {
    m_objectives.push_back(ObjectiveInfo(type, exe, out, weight));
    markParentSelectionForUpdate();
  }

  // Multi-objective: retrieve an objective
  int     getObjectivesNum() const { return m_objectives.size(); }
  ObjType getObjectivesTyp(int i) const { return m_objectives.at(i).type; }
  QString getObjectivesExe(int i) const { return m_objectives.at(i).exe; }
  QString getObjectivesOut(int i) const { return m_objectives.at(i).out; }
  double  getObjectivesWgt(int i) const { return m_objectives.at(i).weight; }
  void    setObjectivesWgt(int i, double weight)
  {
    if (m_objectives[i].weight != weight) {
      m_objectives[i].weight = weight;
      markParentSelectionForUpdate();
    }
  }
  bool hasExternalObjectiveCalculations() const
  {
    for (int i = 0; i < getObjectivesNum(); ++i)
      if (objectiveNeedsExternalCalculation(i))
        return true;
    return false;
  }
  // Reset all the objectives
  void resetObjectives()
  {
    if (!m_objectives.isEmpty()) {
      m_objectives.clear();
      markParentSelectionForUpdate();
    }
  }
  // Remove the "i"th objective
  void removeObjective(int i)
  {
    m_objectives.removeAt(i);
    markParentSelectionForUpdate();
  }

  // Constrained-search filters: retrieve and modify constraints
  void addConstraint(const QString& exe = QString(), const QString& out = QString())
  {
    m_constraints.push_back(ConstraintInfo(exe, out));
  }
  int getConstraintsNum() const { return m_constraints.size(); }
  QString getConstraintExe(int i) const { return m_constraints.at(i).exe; }
  QString getConstraintOut(int i) const { return m_constraints.at(i).out; }
  void resetConstraints() { m_constraints.clear(); }
  void removeConstraint(int i) { m_constraints.removeAt(i); }

  //
  // QueueManager settings
  //

  bool isVerbose() const            { return m_verbose; }
  void setVerbose(bool v)           { m_verbose = v; }
  bool isDebugOutput() const        { return m_debugOutput; }
  void setDebugOutput(bool v)       { m_debugOutput = v; Common::setDebugOutputEnabled(v); }

  bool isLimitRunningJobs() const   { return m_limitRunningJobs; }
  void setLimitRunningJobs(bool v)  { m_limitRunningJobs = v; }

  uint getRunningJobLimit() const   { return m_runningJobLimit; }
  void setRunningJobLimit(uint v)   { m_runningJobLimit = v; }

  uint getContStructs() const       { return m_contStructs; }
  void setContStructs(uint v)       { m_contStructs = v; }

  int  getMaxNumStructures() const  { return m_maxNumStructures; }
  void setMaxNumStructures(int v)   { m_maxNumStructures = v; }

  uint getFailLimit() const         { return m_failLimit; }
  void setFailLimit(uint v)         { m_failLimit = v; }

  FailActions getFailAction() const        { return m_failAction; }
  void        setFailAction(FailActions v) { m_failAction = v; }

  // Session paths and remote connection settings.

  const QString& getDescription() const { return m_description; }
  void setDescription(const QString& v) { m_description = v; }

  const QString& getLocWorkDir() const { return m_locWorkDir; }
  // Stored trimmed, so every later check sees the same value.
  void setLocWorkDir(const QString& v) { m_locWorkDir = v.trimmed(); }
  const QString& getHost() const { return m_host; }
  void setHost(const QString& v) { m_host = v; }

  int getPort() const { return m_port; }
  void setPort(int v) { m_port = v; }

  const QString& getUsername() const { return m_username; }
  void setUsername(const QString& v) { m_username = v; }

  const QString& getRemWorkDir() const { return m_remWorkDir; }
  void setRemWorkDir(const QString& v) { m_remWorkDir = v; }

  const QString& sshMethod() const { return m_sshMethod; }
  bool setSshMethod(const QString& m);
  static QString defaultSshMethod();
  static bool isValidSshMethod(const QString& m);
  static bool isSshMethodAvailable(const QString& m);

  bool logErrorDirs() const { return m_logErrorDirs; }
  void setLogErrorDirs(bool v) { m_logErrorDirs = v; }

  //
  // Runtime flags and ranking options
  // These values can affect an active session. Some are also loaded from input
  //   or state files, so startup/resume code may reset live-only flags.
  //

  bool isSoftExit() const           { return m_softExit.load(); }
  void setSoftExit(bool v)          { m_softExit.store(v); }
  bool isHardExit() const           { return m_hardExit.load(); }
  void setHardExit(bool v)          { m_hardExit.store(v); }
  bool isConstraintsReDo() const    { return m_constraintsReDo; }
  void setConstraintsReDo(bool v)   { m_constraintsReDo = v; }
  bool isSessionStarting() const    { return m_isStarting.load(); }

  /**
     True while a session is running.
   */
  bool isSessionActive() const      { return m_sessionActive.load(); }

  /**
     True while a session is in progress.
   */
  bool isSessionInProgress() const  { return isSessionStarting() || isSessionActive(); }

  bool isReadOnly() const           { return m_readOnly.load(); }

  /**
     True while the search is stopping.
   */
  bool isShuttingDown() const       { return m_shuttingDown.load(); }
  const std::atomic<bool>* shutdownFlag() const { return &m_shuttingDown; }

protected:
  /** @sa isSessionStarting(). Set by the session functions. */
  void setSessionStarting(bool v)   { m_isStarting.store(v); }

  /** @sa isSessionActive(). Set by the session functions. */
  void setSessionActive(bool v)     { m_sessionActive.store(v); }

  // Read-only is set only with the application run mode.
  void setReadOnly(bool v)          { m_readOnly.store(v); }

public:
  bool isRemoteQueue() const        { return m_remoteQueue; }
  void setRemoteQueue(bool v)       { m_remoteQueue = v; }
  OptimizationType getOptimizationType() const { return m_optimizationType; }
  void setOptimizationType(OptimizationType v)
  {
    if (m_optimizationType != v) {
      m_optimizationType = v;
      markParentSelectionForUpdate();
    }
  }
  bool isTournamentSelection() const { return m_tournamentSelection; }
  void setTournamentSelection(bool v){ m_tournamentSelection = v; }
  bool isRestrictedPool() const     { return m_restrictedPool; }
  void setRestrictedPool(bool v)    { m_restrictedPool = v; }
  bool isCrowdingDistance() const   { return m_crowdingDistance; }
  void setCrowdingDistance(bool v)
  {
    if (m_crowdingDistance != v) {
      m_crowdingDistance = v;
      markParentSelectionForUpdate();
    }
  }
  bool isParetoFilterZeroWeights() const { return m_paretoFilterZeroWeights; }
  void setParetoFilterZeroWeights(bool v)
  {
    if (m_paretoFilterZeroWeights != v) {
      m_paretoFilterZeroWeights = v;
      markParentSelectionForUpdate();
    }
  }
  int  getObjectivePrecision() const { return m_objectivePrecision; }
  void setObjectivePrecision(int v)
  {
    if (m_objectivePrecision != v) {
      m_objectivePrecision = v;
      markParentSelectionForUpdate();
    }
  }

  // Update the parent selection data after changing the parent list or relevant settings.
  void markParentSelectionForUpdate() { ++m_selectionDataStamp; }

  // Re-check an structure's parent-pool eligibility (after change of status, similarity, objc/cons)
  void refreshParentPoolMembership(Structure* s);

  // Re-construct the parent pool table from all tracked structures (session load/reset).
  void rebuildParentPoolMembership();

  // Number of structures currently in the parent pool.
  int getParentPoolSize() const;

  // A copy of all structure in the current parent pool.
  QList<Structure*> getAllParentPoolStructures() const;

  // Update structures' Pareto front using pool values
  bool applyParentSelectionFronts();

  // Rebuild the selection table for the pool (for sesseion load and final save)
  void refreshParentSelectionFronts(const QList<Structure*>& pool);

  // Report a structure change so the application can save its new data.
  void reportStructureStateChanged(Structure* structure);

signals:
  // Session signals.

  /**
   * Emitted when a session is starting or being loaded.
   * @sa sessionStarted
   */
  void startingSession();

  /**
   * Emitted before tracked structures are deleted.
   */
  void structuresAboutToBeDeleted();

  /**
   * Emitted when a session finishes starting or loading.
   * @sa startingSession
   */
  void sessionStarted();

  /**
   * Emitted when a structure needs to be saved.
   */
  void structureStateChanged(Search::Structure* structure);

  /**
   * Emitted when the application should update structure evaluation data.
   */
  void structureEvaluationUpdateRequested();

  /**
   * Emitted before QueueManager is reset at the end of a search.
   */
  void activeSessionFinalizing();

  /**
   * Emitted when the engine has finished all end-of-run work.
   */
  void sessionEnded();

  // Optimizer and Queue selection signals.

  /**
   * Emitted when the current QueueInterface changes
   * @sa setQueueInterface
   * @sa queueInterface
   */
  void queueInterfaceChanged(const std::string& qiName);

  /**
   * Emitted when the current Optimizer changed
   * @sa setOptimizer
   * @sa optimizer
   */
  void optimizerChanged(const std::string& optName);

  // Progress and error signals.

  /**
   * Emitted when a long operation starts reporting progress.
   */
  void progressRangeChanged(const QString& label, int min, int max);

  /**
   * Emitted when progress text or values should be updated.
   */
  void progressValueChanged(int value, const QString& label, int min, int max);

  /**
   * Emitted when the current progress report is finished.
   */
  void progressEnded();

  /**
   * Emitted when a startup error should also be shown in a dialog.
   *
   * The GUI handler blocks the emitting thread until the dialog is
   * dismissed. Never emit this while holding a tracker, structure, or
   * runtime-settings lock: the GUI thread may need that lock while the
   * dialog runs, deadlocking both threads.
   */
  void errorDialogRequested(const QString& message);

  // Structure-view refresh signals.

  /**
   * Emitted when a major change has occurred affecting many
   * structures, e.g. when similarities are set/reset. It is
   * recommended that any user-visible structure data is rebuilt
   * from scratch when this is called.
   */
  void refreshAllStructureInfo();

  /**
   * Emitted before and after updating the data shown for many structures.
   */
  void structureViewUpdateBlocked(bool blocked);

  /**
   * Emitted when the displayed structure data should be read again.
   */
  void structureViewDataChanged();

public slots:

  //
  // Public slots used by the user interface and QueueManager
  // Qt keeps these callable through the signal/slot system. Some are generic
  //   engine operations; startSearch() and generateNewStructure() are implemented
  //   by the derived application.
  //

  /**
   * Deletes all structures from m_tracker and calls
   * m_tracker->reset() and m_queue->reset().
   */
  virtual void reset();

  /**
   * Creates ssh connections to the remote cluster.
   */
  virtual bool createSSHConnections();

  /**
   * Begin the search.
   */
  virtual bool startSearch() = 0;

  /**
   * Called when the QueueManager requests more Structures.
   * @sa QueueManager
   */
  virtual void generateNewStructure() = 0;

  //
  // Optimizer/Queue and opt-step settings
  //

  /**
   * Get the number of optimization steps for our search.
   *
   * @return The number of optimization steps for our search.
   */
  size_t getNumOptSteps() const { return m_optSteps.numSteps(); }

  /**
   * Clear all optimization steps.
   */
  void clearOptSteps() { m_optSteps.clear(); }

  /**
   * Append an opt step to the current list of opt steps. If the list of
   * opt steps is not empty, it will set the new opt step to be the same
   * as the previous opt step. If the list of opt steps is empty, then
   * the optimizer and queue interface will be set to nullptr, and the
   * template maps will be empty.
   */
  void appendOptStep() { m_optSteps.append(); }

  /**
   * Remove the opt step at @p optStep.
   *
   * @p optStep The optimization step to be removed.
   */
  void removeOptStep(size_t optStep) { m_optSteps.remove(optStep); }

  /**
   * Update the QueueInterface for one optimization step.
   *
   * @param optStep The optimization step for which to set the QI.
   * @param qiName The name of the queue interface to use.
   * @return True if the opt-step selection changed; false on invalid input.
   *
   * @sa queueInterface
   */
  bool setQueueInterface(size_t optStep, const std::string& qiName)
  {
    return m_optSteps.setQueueInterface(optStep, qiName);
  }

  /**
   * Get a queue interface template for a particular opt step and a
   * particular file name.
   *
   * @param optStep The optimization step for which to get the template.
   * @param name The name of the file for which to get the template.
   *
   * @return The queue interface template. Returns an empty string if
   *         @p optStep or @p name are invalid.
   */
  std::string getQueueInterfaceTemplate(size_t optStep,
                                        const std::string& name) const
  {
    return m_optSteps.queueInterfaceTemplate(optStep, name);
  }
  /**
   * Set the queue interface template for a particular opt step and
   * file name.
   *
   * @param optStep The optimization step for which to set the template.
   * @param name The file name for which to set the template.
   * @param temp The template to be set.
   */
  void setQueueInterfaceTemplate(size_t optStep, const std::string& name,
                                 const std::string& temp)
  {
    m_optSteps.setQueueInterfaceTemplate(optStep, name, temp);
  }

  /**
   * Update the Optimizer for one optimization step.
   *
   * @param optStep The opt step for which to set the optimizer
   * @param optName New Optimizer to use.
   * @return True if the opt-step selection changed; false on invalid input.
   *
   * @sa optimizer
   */
  bool setOptimizer(size_t optStep, const std::string& optName)
  {
    return m_optSteps.setOptimizer(optStep, optName);
  }

  /**
    * Get an optimizer template for a particular opt step and a
    * particular file name.
    *
    * @param optStep The optimization step for which to get the template.
    * @param name The name of the file for which to get the template.
    *
    * @return The optimization template. Returns an empty string if
    *         @p optStep or @p name are invalid.
    */
  std::string getOptimizerTemplate(size_t optStep,
                                   const std::string& name) const
  {
    return m_optSteps.optimizerTemplate(optStep, name);
  }

  /**
   * Get an optimizer input asset value for a particular opt step.
   *
   * Input assets are extra optimizer input files, such as VASP POTCAR or
   * SIESTA PSF files. They are not interpreted as user templates.
   */
  std::string getOptimizerInputAsset(size_t optStep, const std::string& name) const
  {
    return m_optSteps.optimizerInputAsset(optStep, name);
  }
  /**
   * Set the optimizer template for a particular opt step and
   * file name.
   *
   * @param optStep The optimization step for which to set the template.
   * @param name The file name for which to set the template.
   * @param temp The template to be set.
   */
  void setOptimizerTemplate(size_t optStep, const std::string& name,
                            const std::string& temp)
  {
    m_optSteps.setOptimizerTemplate(optStep, name, temp);
  }

  /**
   * Set an optimizer input asset value for a particular opt step.
   */
  void setOptimizerInputAsset(size_t optStep, const std::string& name, const std::string& value)
  {
    m_optSteps.setOptimizerInputAsset(optStep, name, value);
  }

  //
  // User template values
  // User values are plain strings that can be inserted into input templates.
  //

  /**
   * @return A user customizable string that is used in template
   * interpretation.
   */
  QString getUser1() const { return m_user1; }

  /**
   * @return A user customizable string that is used in template
   * interpretation.
   */
  QString getUser2() const { return m_user2; }

  /**
   * @return A user customizable string that is used in template
   * interpretation.
   */
  QString getUser3() const { return m_user3; }

  /**
   * @return A user customizable string that is used in template
   * interpretation.
   */
  QString getUser4() const { return m_user4; }

  /**
   * @param s A user customizable string that is used in template
   * interpretation.
   */
  void setUser1(const QString& s) { m_user1 = s; }

  /**
   * @param s A user customizable string that is used in template
   * interpretation.
   */
  void setUser2(const QString& s) { m_user2 = s; }

  /**
   * @param s A user customizable string that is used in template
   * interpretation.
   */
  void setUser3(const QString& s) { m_user3 = s; }

  /**
   * @param s A user customizable string that is used in template
   * interpretation.
   */
  void setUser4(const QString& s) { m_user4 = s; }

  //
  // Checks and user questions
  //

  /**
   * Are we ready to search (checks that essential variables are set and
   * that the queue interfaces and optimizers are ready for the search).
   *
   * @param err Optional; receives the error message if we are not ready.
   *
   * @return True if ready to search. False otherwise.
   */
  bool isReadyToSearch(QString* err) const;

  /**
   * Do we have any batch queue interfaces?
   *
   * @return True if we do. False if we do not.
   */
  bool anyBatchQueueInterfaces() const;

  /**
   * Ask the user a yes/no question and return the answer.
   *
   * The GUI handler blocks the calling thread until the user responds.
   * Never call this while holding a tracker, structure, or
   * runtime-settings lock: the GUI thread may need that lock while the
   * dialog runs, deadlocking both threads.
   */
  bool requestBooleanDecision(const QString& message, bool defaultValue = false);

  /**
   * Request a password from the user, used for libssh
   * authentication.
   *
   * The GUI handler blocks the calling thread until the user responds;
   * the same no-shared-locks rule as requestBooleanDecision() applies.
   *
   * @param message Message to the user.
   * @param newPassword pointer to the QString that will hold the new password.
   * @param ok True if accepted, false if rejected or unavailable.
   */
  bool requestPassword(const QString& message, QString& newPassword);

  /**
   * Start a progress range.
   */
  void beginProgressUpdate(const QString& label, int min, int max);

  /**
   * End the active progress range.
   */
  void endProgressUpdate();

  /**
   * Update the progress value. Default arguments keep the current text/range.
   */
  void updateProgressValue(int value = -1, const QString& label = QString(), int min = -1,
                           int max = -1);

  // Queue polling safeguard.

  /**
   * @return Whether or not we are to cancel a job after a given amount
   *         of time.
   */
  bool cancelJobAfterTime() const { return m_cancelJobAfterTime; }
  void setCancelJobAfterTime(bool v) { m_cancelJobAfterTime = v; }

  /**
   * @return The amount of time in hours that, if exceeded, we are to
   *         cancel a job.
   */
  double hoursForCancelJobAfterTime() const
  {
    return m_hoursForCancelJobAfterTime;
  }
  void setHoursForCancelJobAfterTime(double v)
  {
    m_hoursForCancelJobAfterTime = v;
  }

  /**
   * @return Whether we stop waiting for an objective/constraint script's
   *         output after a set time and mark it failed.
   */
  bool cancelScriptAfterTime() const { return m_cancelScriptAfterTime; }
  void setCancelScriptAfterTime(bool v) { m_cancelScriptAfterTime = v; }

  /**
   * @return The amount of time in hours that, if exceeded, we stop waiting
   *         for an objective/constraint script's output and mark it failed
   */
  double hoursForCancelScriptAfterTime() const
  {
    return m_hoursForCancelScriptAfterTime;
  }
  void setHoursForCancelScriptAfterTime(double v)
  {
    m_hoursForCancelScriptAfterTime = v;
  }

  // Generic ranking helpers.
  // Derived applications prepare raw objective values, then can use these
  // helpers for scalar/Pareto ranking.

  /**
   * @return The total number of optimizable objectives used in generic
   * ranking, including the primary search-specific objective.
   */
  int getOptimizableObjectivesNum() const;

  /**
   * Build the objective count and weight vector. Basic selection uses the
   * relative weights after normalizing a local copy. Pareto selection uses
   * weights only to filter objectives when that option is enabled.
   * @param objNumb  Set to the total number of optimizable objectives.
   * @param objWght  Populated with one weight per objective.
   */
  void buildObjWeights(int& objNumb, std::vector<double>& objWght) const;

  /**
   * Populate objData and strTags from a list of structures that already have
   * their generic ranking-objective table prepared by the derived search.
   * @param structures Input list with ranking values ready.
   * @param objNumb    Number of objectives (from buildObjWeights).
   * @param objData    Output 2-D matrix: one row per structure, one column per objective.
   * @param strTags    Output tag list, parallel to objData rows.
   */
  bool buildObjDataFromPool(const QList<Structure*>& structures, int objNumb,
                            std::vector<std::vector<double>>& objData,
                            QList<QString>& strTags) const;

  /**
   * @return true when @p structure has finite values for every objective used
   * by generic ranking.
   */
  bool hasCompleteObjectiveValues(const Structure& structure) const;

  /**
   * Normalize each objective column to [0,1] and apply configured precision.
   * Objectives with zero spread are flattened to zero.
   */
  void normalizeObjData(std::vector<std::vector<double>>& objData) const;

protected:

  //
  // Functions for derived applications
  //

  // Note: repeated requests from this are being merged.
  void requestStructureEvaluationUpdate();

  // Release queue objects and stop their thread.
  void stopQueueThread();

  // Notify views, then delete every tracked structure.
  void deleteTrackedStructures();

  // The application does its setup between these calls:
  //   beginSession() -> build or restore the population -> launchSession()
  // or abortSession() if setup fails.

  /**
   * Start setting up a new session: refuse a second active or concurrent
   * start, clear the hard-exit flag, emit startingSession(), and reset the
   * queue and tracker.
   *
   * @return False if a session is already active or starting.
   */
  bool beginSession();

  /**
   * Finish session setup: start the queue thread, leave the session-starting
   * state, mark the session active, and emit sessionStarted().
   */
  void launchSession();

  /**
   * Cancel a session that failed during setup, clear its structures, and
   * return the engine to idle.
   */
  void abortSession();

  // Template keyword registry helpers.

  /**
   * Register a template keyword and its handler.
   *
   * @param key     Keyword name (without % delimiters), case-sensitive.
   * @param handler Callable that receives the current Structure and returns the
   *                substitution string.  May include a trailing newline; the
   *                caller strips it.
   * @param help    One-line description shown in getTemplateKeywordHelp_base().
   *                Pass an empty string to omit this keyword from the help text.
   */
  void registerKeyword(const QString& key, std::function<QString(Structure*)> handler,
                       const QString& help = "");

  /// Hidden call to interpretKeyword
  void interpretKeyword_base(QString& keyword, Structure* structure);

  // Hidden call to getTemplateKeywordHelp
  QString getTemplateKeywordHelp_base();

  /**
   * Runs one background job at a time.
   *
   * request() asks for one run. Requests made while the job is running
   * are merged into a single extra run. Meant for work where only the
   * latest state matters, like saving a state file.
   *
   * Call shutdown() and wait for the thread pool before destroying
   * anything used by the job.
   */
  class BackgroundJob
  {
  public:
    explicit BackgroundJob(std::function<void()> job);

    /** Ask for one run; merged if a run is already pending. */
    void request();

    /** Stop new runs. */
    void shutdown();

  private:
    void runLoop();

    std::function<void()> m_job;
    std::mutex m_mutex;
    bool m_pending;
    bool m_running;
    std::atomic<bool> m_shutDown;
  };

private:
  bool objectiveNeedsExternalCalculation(int i) const
  {
    return !m_objectives.at(i).exe.isEmpty() || !m_objectives.at(i).out.isEmpty();
  }
  bool objectiveParticipatesInOptimization(int i) const;

  // Entry in the template-keyword registry.
  struct KeywordEntry {
    std::function<QString(Structure*)> handler;
    QString help;
  };

  // Keyword handler map: keyword -> {handler, help string}
  QHash<QString, KeywordEntry> m_keywordMap;
  // Keyword insertion order (drives help-text output)
  QStringList m_keywordOrder;

  // Multi-objective parameters for the run
  QList<ObjectiveInfo> m_objectives;
  QList<ConstraintInfo> m_constraints;

  // String that uniquely identifies the derived SearchBase
  QString m_idString;

  // Cached pointer to the SSHManager
  std::unique_ptr<SSHManager> m_ssh;

  std::function<bool(const QString&, bool)> m_decisionPromptHandler;
  std::function<bool(const QString&, QString&)> m_passwordPromptHandler;

  // Owning pointer to the main Tracker
  std::unique_ptr<Tracker> m_tracker;

  // Thread to run the QueueManager
  std::unique_ptr<QThread> m_queueThread;

  // Owning pointer to the QueueManager
  std::unique_ptr<QueueManager> m_queue;

  // Protects settings that can change during a search.
  mutable QReadWriteLock m_runtimeSettingsLock;


  // Number of optimization steps. Note: not to be changed directly;
  // only should be modified by append/remove OptStep() setters.
  OptSteps m_optSteps;

  QString m_user1, m_user2, m_user3, m_user4;

  bool m_verbose;
  bool m_debugOutput;
  bool m_limitRunningJobs;
  uint m_runningJobLimit;
  uint m_contStructs;
  int  m_maxNumStructures;
  uint m_failLimit;
  FailActions m_failAction;
  int     m_objectivePrecision;
  // Atomic: written under runtimeSettingsLock, but also read by the queue
  // thread in checkExit() without that lock (see queuemanager.cpp), so the
  // type must make those reads/writes well-defined.
  std::atomic<bool> m_softExit;
  std::atomic<bool> m_hardExit;
  bool    m_remoteQueue;
  bool    m_constraintsReDo;
  OptimizationType m_optimizationType;
  bool    m_tournamentSelection;
  bool    m_restrictedPool;
  bool    m_crowdingDistance;
  bool    m_paretoFilterZeroWeights;

  // Keep the parent-selection data so it is not recalculated for every parent.
  struct ParentSelectionData
  {
    QList<Structure*> pool;
    std::vector<std::vector<double>> objData;
    std::vector<int> fronts;
    std::vector<double> distances;
    std::vector<double> probs;
    bool usePareto = false;
    bool noObjectives = false;
    bool valid = true;
    long long stamp = -1;
    bool built = false;
  };
  void rebuildParentSelectionData(const QList<Structure*>& structures);
  bool parentPoolEligible(Structure* s) const;
  ParentSelectionData m_parentSelectionData;
  // Structures eligible as parents: kept up to date by refreshParentPoolMembership.
  QSet<Structure*> m_parentPool;
  mutable QReadWriteLock m_parentSelectionDataLock;
  std::atomic<long long> m_selectionDataStamp{0};

  int m_queueRefreshInterval; // the unit is seconds.
  bool m_cleanRemoteOnStop;

  bool checkObjectiveAndConstraintScripts(QString* err) const;
  bool checkScriptPath(const QString& path, const QString& label, int index, QueueInterface* queue,
                       QString* err) const;

  QString m_locWorkDir;
  QString m_description;
  QString m_host;
  int m_port;
  QString m_username;
  QString m_remWorkDir;
  QString m_sshMethod;

  std::atomic<bool> m_isStarting;
  std::atomic<bool> m_sessionActive;
  std::atomic<bool> m_readOnly;
  std::atomic<bool> m_shuttingDown;

  bool m_logErrorDirs;

  bool m_cancelJobAfterTime = false;
  double m_hoursForCancelJobAfterTime = 100.0;
  bool m_cancelScriptAfterTime = true;
  double m_hoursForCancelScriptAfterTime = 2.0;

  BackgroundJob m_evaluationUpdateJob;
};

} // end namespace Search

#endif

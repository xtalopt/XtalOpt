/**********************************************************************
  OptBase - Base class for global search extensions

  Copyright (C) 2010-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef OPTBASE_H
#define OPTBASE_H

// Prevent redefinition of symbols on windows
#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#endif // WIN32

#include <QDebug>
#include <QMutex>
#include <QObject>
#include <QFile>
#include <QDir>

#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>

#include <globalsearch/bt.h>

#ifdef XTALOPT_DEBUG
//*******************************************************************
// This part, as a whole, is to save a copy of all messages of the  *
//   run to a log file, by setting up a message handler.            *
// These are here so other parts of the code (than xtalopt module)  *
//   can use them.                                                  *
//                                                                  *
// This output file will be written if XTALOPT_DEBUG flag is        *
//   defined at the cmake input.                                    *
//                                                                  *
// The main variables/functions are:                                *
//   i)  messageHandlerIsSet : (logical) make sure this is set once *
//  ii)  gui_log_filename    : name of the log file                 *
// iii)  customMessageOutput : the message handler function         *
//  iv)  saveLogFileOfRun    : main function to be called           *
//                                                                  *
// The saveLogFileOfRun, whenever called, saves a copy of all the   *
//   output messages to the file. This function, can and must       *
//   be called only once! Either on:                                *
//   (1) Starting a run (in xtalopt.cpp file, startSearch())        *
//   (2) Resuming a run (in xtalopt.cpp file, load())               *
//*******************************************************************
static bool messageHandlerIsSet = false;
static QString run_log_filename = "xtaloptDebug.log";
static void customMessageOutput(QtMsgType type,
                  const QMessageLogContext &, const QString & msg)
{
  if (type == QtFatalMsg)
  abort();

  // Write the message to file
  QFile outFile(run_log_filename);
  outFile.open(QIODevice::WriteOnly | QIODevice::Append);
  QTextStream ts(&outFile);
  ts << msg << endl;
  // Write the message to stdout
  qDebug().noquote() << msg;
}
static void saveLogFileOfRun(QString work_dir)
{
  // Basically, this function is called after the locWorkDir
  //   variable is set. Just in case, if this is not true
  //   then we will just ignore setting up the handler.
  // Also, the logical variable messageHandler... is checked
  //   to make sure that handler is set only once in the run.
  if (work_dir.isEmpty() || messageHandlerIsSet)
    return;
  // Set the log file's full path.
  run_log_filename = work_dir + QDir::separator() + run_log_filename;
  // Setup the message handler.
  qInstallMessageHandler(customMessageOutput);
  messageHandlerIsSet = true;
}
//*******************************************************************
//*******************************************************************
#endif

class AflowML;
class QMutex;
class QNetworkAccessManager;

namespace GlobalSearch {
class Structure;
class Tracker;
class Optimizer;
class QueueManager;
class QueueInterface;
class SSHManager;
class AbstractDialog;

/**
 * @class OptBase optbase.h <globalsearch/optbase.h>
 *
 * @brief The OptBase class stores variables and helper functions
 * for global searches.
 *
 * @author David C. Lonie
 *
 * OptBase is the main class in libglobalsearch. It contains the
 * variables that define a search, as well as handling structure
 * generation. This class ties all others together.
 */
class OptBase : public QObject
{
  Q_OBJECT

public:
  /**
   * Constructor
   *
   * @param parent Dialog window of GUI.
   */
  explicit OptBase(AbstractDialog* parent = nullptr);

  /**
   * Destructor
   */
  virtual ~OptBase() override;

  /**
   * Types of the optimization for objectives in a multi-objective run
   */
  enum ObjectivesType
  {
    // Minimization objective
    Ot_Min = 0,
    // Maximization objective
    Ot_Max,
    // Filtration objective
    Ot_Fil,
    // aflow-hardness
    Ot_Har
  };

  /**
   * Actions to take when a structure has failed optimization too
   * many times.
   *
   * @sa OptBase::failAction
   * @sa OptBase::failLimit
   */
  enum FailActions
  {
    /// Do nothing; keep submitting for optimization
    FA_DoNothing = 0,
    /// Kill the structure
    FA_KillIt,
    /// Replace the failing structure with a new random one
    FA_Randomize,
    /// Replace with a new offspring structure
    FA_NewOffspring
  };

  /**
   * An enumeration for the various template types.
   */
  enum TemplateType
  {
    /// A queue interface template
    TT_QueueInterface = 0,
    /// An optimizer template
    TT_Optimizer,
    /// Unknown template type
    TT_Unknown
  };

  /**
   * @return An ID string that uniquely identifies this OptBase.
   */
  QString getIDString() { return m_idString; }

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
  virtual Structure* replaceWithRandom(Structure* s, const QString& reason = "")
  {
    return 0;
  }

  /**
   * Replace the Structure with a new offspring. This only makes sense if
   * the search method uses offspring (e.g. a GA). The default
   * implementation of this method calls replaceWithRandom().
   *
   * @param s The Structure to be replaced. This pointer remains
   * valid -- the structure it points to will be modified.
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
   * Before starting an optimization, this function will check the
   * parameters of the search to ensure that they are within a
   * reasonable range.
   *
   * @return True if the search parameters are valid, false otherwise.
   */
  virtual bool checkLimits() = 0;

  /**
  * Perform any post-optimization checks that need to be performed when a
  * structure enters the Structure::StepOptimized state.
  * @param s Structure to check
  * @param err If non-NULL, will be overwritten with an explaination of
  * why the check failed.
  * @return True if structure passes, false otherwise.
  */
  virtual bool checkStepOptimizedStructure(Structure* s, QString* err = NULL)
  {
    Q_UNUSED(s);
    Q_UNUSED(err);
    return true;
  }

  /**
   * In CLI mode, read the runtime file to update options.
   * If the runtime file is not found, this should do nothing.
   */
  virtual void readRuntimeOptions() = 0;

  /**
   * Generate a sorted, trimmed, cumulative probability list for selecting a
   * structure.
   *
   * The probability of a crystal being selected is as follows:
   *
   *
   * pi = N [ 1 - w (Hmax - Hi) / (Hmax - Hmin) -
   *              (1 - w) (Ei - Emin) / (Emax - Emin) ]
   *
   *
   * where H is the hardness, E is the enthalpy per formula unit, w is the
   * fractional weight of the hardness (between 0.0 and 1.0), and N is a
   * normalization constant.
   *
   * If w is 0, then the probability selection will be based entirely upon
   * enthalpy (lower enthalpy is more likely to be selected).
   *
   * If w is 1, then the pobability selection will be based entirely upon
   * hardness (higher hardness is more likely to be selected).
   *
   * If w is anywhere between 0 and 1, the probability is based upon a
   * combination of enthalpy and hardness, where low enthalpy is favored and
   * high hardness is favored. A value closer to 0 puts more importance on low
   * enthalpy, and a value closer to 1 puts more importance on high hardness.
   *
   * This function's arguments are adjusted to allow for multi-objective search.
   *
   * @param structures The list of structures to consider (if the hardness
   *                   weight is greater than 0, do not include structures
   *                   whose hardness isn't set (i. e., less than 0)).
   * @param popSize The size of the population. After probabilities are
   *                generated, the low probability structures will be trimmed
   *                off until popSize is reached.
   * @param hardnessWeight w in the probability equation. This value should
   *                be between 0 and 1. 0 means hardness calculated but not used
   *                in optimization, 1 means only hardness consideration;
   *                and -1.0 is the case that no hardness calculation is performed.
   * @param objectives_num (Optional) The number of objectives introduced by
   *                the user for multi-objective optimization.
   * @param objectives_wgt (Optional) The weight of objectives introduced by
   *                the user for multi-objective optimization.
   * @param objectives_typ (Optional) The type of optimization (min/max etc)
   *                for the objectives introduced by the user for multi-objective run.
   *
   * @return A list of pairs with a structure pointer and a double (the
   *         probability). This list will be trimmed so that it is not greater
   *         than popSize, and it will be sorted based upon probability.
   *         The probabilities will be cumulative so that the last probability
   *         is 1. To use this list, generate a random double 'r' between 0
   *         and 1, and select the structure that is immediately larger than r.
   *         Default arguments are added for the multi-objective search which
   *         is not added to gapc part of the code.
   */
  static QList<QPair<Structure*, double>>
  getProbabilityList(const QList<Structure*>& structures,
                     size_t popSize,
                     double hardnessWeight,
                     int    objectives_num = 0,
                     QList<double>  objectives_wgt = {},
                     QList<ObjectivesType> objectives_typ = {});

  /**
   * Use Aflow machine learning to calculate the hardness of structure
   * @param s. Note that @param s will not be updated immediately, but
   * it will be updated with the bulk modulus, shear modulus, and hardness
   * when it receives the data back from the aflow servers.
   *
   * @param s The structure whose hardness is to be calculated.
   */
  void calculateHardness(Structure* s);

  /**
   * In a separate thread, resubmit incomplete hardness calculations every
   * 10 minutes if m_calculateHardness is true.
   */
  void startHardnessResubmissionThread();

  /**
   * Run calculateHardness() on any structure that does not yet have
   * vickersHardness info (i. e., vickersHardness() < 0.0).
   */
  void resubmitUnfinishedHardnessCalcs();

  /**
   * Save the current search. If filename is omitted, default to
   * locWorkDir + "/[search name].state". Will only save once at a time.
   *
   * @param filename Filename to write to. Optional.
   * @param notify Whether to display a user-visible notification
   *
   * @return True if successful, false otherwise.
   */
  virtual bool save(QString filename = "", bool notify = false);

  /**
   * Load a search session from the specified filename.
   *
   * @param filename State file to resume.
   * @param forceReadOnly Set to true to skip any prompts and load the
   * session readonly
   *
   * @return True is successful, false otherwise.
   */
  virtual bool load(const QString& filename, const bool forceReadOnly = false)
  {
    Q_UNUSED(forceReadOnly);
    return false;
  };

  /**
   * Takes a template and inserts structure specific information by
   * replacing keywords.
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

  /**
   * Set the main dialog..
   */
  void setDialog(AbstractDialog* d)
  {
    m_dialog = d;
    emit dialogSet();
  }

  /**
   * @return A pointer to the main dialog..
   */
  AbstractDialog* dialog() { return m_dialog; }

  /**
   * @return A pointer to the main Structure Tracker.
   */
  Tracker* tracker() { return m_tracker; };

  /**
   * @return A pointer to the associated QueueManager.
   */
  QueueManager* queue() { return m_queue; };

  /**
   * Create a queue interface with a given name.
   *
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
  QueueInterface* queueInterface(int optStep) const;

  /**
   * @return Get the index of the queue interface pointer or -1 if it does
   *         not exist.
   */
  int queueInterfaceIndex(const QueueInterface* qi) const;

  /**
   * @return A pointer to the current Optimizer.
   * @sa setOptimizer
   * @sa optimizerChanged
   */
  Optimizer* optimizer(int optStep) const;

  /**
   * @return Get the index of the optimizer pointer or -1 if it does
   *         not exist.
   */
  int optimizerIndex(const Optimizer* optimizer) const;

  /**
   * @return A pointer to the SSHManager instance.
   */
  SSHManager* ssh() { return m_ssh; }

  /**
   * Are we using the GUI?
   *
   * @return Whether or not we are using the GUI.
   */
  bool usingGUI() { return m_usingGUI; }

  /**
   * Are we using the GUI?
   *
   * @param b True if we are using the GUI. False otherwise.
   */
  void setUsingGUI(bool b) { m_usingGUI = b; }

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

  /// Whether to impose the running job limit
  bool limitRunningJobs;

  /// Number of concurrent jobs allowed.
  uint runningJobLimit;

  /// Number of continuous structures generated
  uint contStructs;

  /// How many structures to produce before halting search. -1 for no limit
  int cutoff;

  /// Whether to run benchmarking tests
  bool testingMode;

  /// Starting run number for benchmark
  uint test_nRunsStart;

  /// Ending run number for benchmark
  uint test_nRunsEnd;

  /// Number of Structures per run when benchmarking
  uint test_nStructs;

  /// Number of times a Structure may fail
  /// @sa OptBase::failAction
  /// @sa OptBase::FailActions
  uint failLimit;

  /// What to do when a Structure exceeds failLimit
  /// @sa OptBase::FailActions
  /// @sa OptBase::failLimit
  FailActions failAction;

  /// Local directory to work in
  QString locWorkDir;

  /// Terse description of current search
  QString description;

  /// Host name or IP address of remote PBS server
  QString host;

  /// Port on remote PBS server used for SSH communication
  int port;

  /// Username for ssh login on remote PBS server
  QString username;

  /// Path on remote server to store files during and after
  /// optimization
  QString remWorkDir;

  /// This should be locked whenever the state file (resume file) is
  /// being written
  QMutex* stateFileMutex;

  /// A mutex for saving
  std::mutex saveMutex;

  /// True if a session is starting or being loaded
  bool isStarting;

  /// Whether readOnly mode is enabled (e.g. no connection to server)
  bool readOnly;

  /**
   * Wrapper for calculating the objectives for multi-objective run
   * @param s The structure whose objectives is to be calculated.
   */
  void calculateObjectives(Structure* s);

  /**
   * Starts multi-objective run for structure by generating/copying relevant files, and running objective scripts
   * @param s The structure whose objectives is to be calculated.
   */
  void startObjectiveCalculations(Structure* s);

  /**
   * Finalizes the multi-objective run for a structure by waiting for output files to appear, transferring output
   * files (if they're on a remote server) and processing them, updating structure info, and signal the finished job.
   * @param s The structure whose objectives is to be calculated.
   */
  void finishObjectiveCalculations(Structure* s);

  /**
   * Performs a soft/hard exit after writing state files, closing ssh, etc.
   * @param delay Amount of time (seconds) to wait before quitting; in case
   *   we need to make sure all files are written/copied properly, e.g., a soft_exit
   */
  void performTheExit(int dealy = 0);

  /// Quit once maximum number of structures are generated (cli mode only)
  bool m_softExit;

  /// Quit immediately! This is only a runtime option in the cli mode,
  ///   but won't be written to runtime file; it can be added by the user.
  bool m_hardExit;

  /// To allow for slurm job submission in a local run.
  bool m_localQueue;

  /// Should a redo for structure dismissed by filtration objective be considered?
  bool m_objectivesReDo;

  /// Perform objective calculations in a multi-objective search
  bool m_calculateObjectives;

  /// Multi-objective functions for the run
  void setObjectivesNum(int i)         {m_objectives_num = i;};
  void setObjectivesTyp(ObjectivesType f) {m_objectives_typ.push_back(f);};
  void setObjectivesExe(QString f)     {m_objectives_exe.push_back(f);};
  void setObjectivesOut(QString f)     {m_objectives_out.push_back(f);};
  void setObjectivesWgt(double f)      {m_objectives_wgt.push_back(f);};
  int  getObjectivesNum()              const {return m_objectives_num;};
  ObjectivesType getObjectivesTyp(int i)  const {return m_objectives_typ.at(i);};
  QString     getObjectivesExe(int i)  const {return m_objectives_exe.at(i);};
  QString     getObjectivesOut(int i)  const {return m_objectives_out.at(i);};
  double      getObjectivesWgt(int i)  const {return m_objectives_wgt.at(i);};
  //
  void resetObjectives() { m_objectives_num = 0; m_objectives_typ.clear(); m_objectives_exe.clear();
    m_objectives_out.clear(); m_objectives_wgt.clear();};
  /// Objective list-related: this holds a list of all objective-related entries
  ///   with the cli input format, i.e., "opt_type script_name filename weight"
  void    objectiveListAdd(QString f) {m_objectives_lst.push_back(f);};
  QString objectiveListGet(int i)     {return m_objectives_lst.at(i);};
  void    objectiveListClear()        {m_objectives_lst.clear();};
  void    objectiveListRemove(int i)  {m_objectives_lst.removeAt(i);};
  int     objectiveListSize()         {return m_objectives_lst.size();};

signals:
  /**
   * Emitted when objective calculations for the structure are finished.
   * @param s Structure
   */
  void doneWithObjectives(Structure* s);

  /**
   * Emitted when aflow-hardness calculation for the structure is finished.
   * @param s Structure
   */
  void doneWithHardness(Structure* s);

  /**
   * Emitted when a session is starting or being loaded.
   * @sa sessionStarted
   * @sa emitSessionStarted
   * @sa readOnlySessionStarted
   * @sa emitReadOnlySessionStarted
   * @sa startingSession
   * @sa emitStartingSession
   */
  void startingSession();

  /**
   * Emitted when a session finishes starting or loading.
   * @sa sessionStarted
   * @sa emitSessionStarted
   * @sa readOnlySessionStarted
   * @sa emitReadOnlySessionStarted
   * @sa startingSession
   * @sa emitStartingSession
   */
  void sessionStarted();

  /**
   * Emitted when a read-only session finishes loading.
   * @sa sessionStarted
   * @sa emitSessionStarted
   * @sa readOnlySessionStarted
   * @sa emitReadOnlySessionStarted
   * @sa startingSession
   * @sa emitStartingSession
   */
  void readOnlySessionStarted();

  /**
   * Emitted when the dialog is set.
   */
  void dialogSet();

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

  /**
   * Emitted when debug(const QString&) is called.
   * @param s The debugging statement.
   * @sa debug
   * @sa debugStatement
   * @sa warning
   * @sa warningStatement
   * @sa error
   * @sa errorStatement
   */
  void debugStatement(const QString& s);

  /**
   * Emitted when warning(const QString&) is called.
   * @param s The warning statement.
   * @sa debug
   * @sa debugStatement
   * @sa warning
   * @sa warningStatement
   * @sa error
   * @sa errorStatement
   */
  void warningStatement(const QString& s);

  /**
   * Emitted when error(const QString&) is called.
   * @param s The error statement.
   * @sa debug
   * @sa debugStatement
   * @sa warning
   * @sa warningStatement
   * @sa error
   * @sa errorStatement
   */
  void errorStatement(const QString& s);

  /**
   * Emitted when message(const QString&) is called.
   */
  void messageStatement(const QString& s);

  /**
   * Prompts user with an "Yes/No" dialog
   *
   * @param message Message to the user.
   * @param ok True if user accepts dialog, false if they cancel.
   * @sa promptForBoolean
   */
  void needBoolean(const QString& message, bool* ok);

  /**
   * Request a password from the user, used for libssh
   * authentication.
   *
   * @param message Message to the user.
   * @param newPassword pointer to the QString that will hold the new password.
   * @param ok True if user accepts dialog, false if they cancel.
   * @sa promptForPassword
   */
  void needPassword(const QString& message, QString* newPassword, bool* ok);

  /**
   * Emitted when a major change has occurred affecting many
   * structures, e.g. when duplicates are set/reset. It is
   * recommended that any user-visible structure data is rebuilt
   * from scratch when this is called.
   */
  void refreshAllStructureInfo();

  // Omit the following from doxygen:
  /// \cond
  void sig_setClipboard(const QString& text) const;
  /// \endcond

public slots:

  /**
   * Deletes all structures from m_tracker and calls
   * m_tracker->reset() and m_queue->reset().
   */
  virtual void reset();

#ifdef ENABLE_SSH
  /**
   * Creates ssh connections to the remote cluster.
   */
  virtual bool createSSHConnections();
#endif // ENABLE_SSH

  /**
   * Begin the search.
   */
  virtual bool startSearch() = 0;

  /**
   * Called when the QueueManager requests more Structures.
   * @sa QueueManager
   */
  virtual void generateNewStructure(){};

  /**
   * Prints a debug message to the terminal and emits
   * debugStatement
   * @param s The debug statement.
   * @sa debug
   * @sa debugStatement
   * @sa warning
   * @sa warningStatement
   * @sa error
   * @sa errorStatement
   */
  void debug(const QString& s);

  /**
   * Prints a warning message to the terminal and emits
   * warningStatement
   * @param s The warning message.
   * @sa debug
   * @sa debugStatement
   * @sa warning
   * @sa warningStatement
   * @sa error
   * @sa errorStatement
   */
  void warning(const QString& s);

  /**
   * Prints a error message to the terminal and emits
   * errorStatement
   * @param s The error statement.
   * @sa debug
   * @sa debugStatement
   * @sa warning
   * @sa warningStatement
   * @sa error
   * @sa errorStatement
   */
  void error(const QString& s);

  /**
   * Prints a message to the terminal and emits messageStatement
   */
  void message(const QString& s);

  /**
   * Emits the sessionStarted signal.
   * @sa sessionStarted
   * @sa readOnlySessionStarted
   * @sa emitReadOnlySessionStarted
   * @sa startingSession
   * @sa emitStartingSession
   */
  void emitSessionStarted() { emit sessionStarted(); };

  /**
   * Emits the readOnlySessionStarted signal.
   * @sa sessionStarted
   * @sa emitSessionStarted
   * @sa readOnlySessionStarted
   * @sa startingSession
   * @sa emitStartingSession
   */
  void emitReadOnlySessionStarted() { emit readOnlySessionStarted(); };

  /**
   * Emits the startingSession signal.
   * @sa sessionStarted
   * @sa emitSessionStarted
   * @sa startingSession
   * @sa emitStartingSession
   */
  void emitStartingSession() { emit startingSession(); };

  /**
   * Sets this->isStarting to true;
   * @sa setIsStartingFalse
   */
  void setIsStartingTrue() { isStarting = true; };

  /**
   * Sets this->isStarting to false;
   * @sa setIsStartingTrue
   */
  void setIsStartingFalse() { isStarting = false; };

  /**
   * Sets this->readOnly to true;
   * @sa setReadOnlyFalse
   */
  void setReadOnlyTrue() { readOnly = true; };

  /**
   * Sets this->readOnly to false;
   * @sa setReadOnlyTrue
   */
  void setReadOnlyFalse() { readOnly = false; };

  /**
   * Get the number of optimization steps for our search.
   *
   * @return The number of optimization steps for our search.
   */
  size_t getNumOptSteps() const { return m_numOptSteps; }

  /**
   * Clear all optimization steps.
   */
  void clearOptSteps();

  /**
   * Append an opt step to the current list of opt steps. If the list of
   * opt steps is not empty, it will set the new opt step to be the same
   * as the previous opt step. If the list of opt steps is empty, then
   * the optimizer and queue interface will be set to nullptr, and the
   * template maps will be empty.
   */
  void appendOptStep();

  /**
   * Insert an optimization step at @p optStep. If this insertion is on the
   * end of the list or the list is empty, it will call appendOptStep()
   * instead.
   * If the insertion is at the 0 index, the elements at the current 0 index
   * will be copied to make the inserted element. If the insertion is at
   * any other index, the element at the @p optStep - 1 index will be used
   * to make the inserted element.
   *
   * @p optStep The index that the new optStep will have after insertion.
   */
  void insertOptStep(size_t optStep);

  /**
   * Remove the opt step at @p optStep.
   *
   * @p optStep The optimization step to be removed.
   */
  void removeOptStep(size_t optStep);

  /**
   * Update the QueueInterface to \a q.
   *
   * @param optStep The optimization step for which to set the QI.
   * @param qiName The name of the queue interface to use. Does nothing if
   *               @p qiName isn't in the m_queueInterfaces map.
   *
   * @sa queueInterface
   */
  void setQueueInterface(size_t optStep, const std::string& qiName);

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
                                        const std::string& name) const;
  /**
   * Set the queue interface template for a particular opt step and
   * file name.
   *
   * @param optStep The optimization step for which to set the template.
   * @param name The file name for which to set the template.
   * @param temp The template to be set.
   */
  void setQueueInterfaceTemplate(size_t optStep, const std::string& name,
                                 const std::string& temp);

  /**
   * Update the Optimizer to the one indicated
   *
   * @param optStep The opt step for which to set the optimizer
   * @param optName New Optimizer to use. Does nothing if @p optName
   *                isn't recognized.
   *
   * @sa optimizer
   */
  void setOptimizer(size_t optStep, const std::string& optName);

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
                                   const std::string& name) const;
  /**
   * Set the optimizer template for a particular opt step and
   * file name.
   *
   * @param optStep The optimization step for which to set the template.
   * @param name The file name for which to set the template.
   * @param temp The template to be set.
   */
  void setOptimizerTemplate(size_t optStep, const std::string& name,
                            const std::string& temp);

  /**
   * Get the template type, either a queue interface or an optimizer.
   * This will stop working properly if we ever have a template name that
   * both a queue interface and an optimizer have. So far that is not the
   * case. But this will require some re-working if that ever happens.
   *
   * @param optStep The optimization step to check.
   * @param name The name of the template.
   *
   * @return An enum. Either TT_QueueInterface if it is a queue interface, or
   *         TT_Optimizer if it is an optimizer. Returns TT_Unknown if it
   *         could not be found.
   */
  TemplateType getTemplateType(size_t optStep, const std::string& name) const;

  /**
   * A generic getTemplate() function. It uses getTemplateType() to
   * determine the template type (either queue interface or optimizer), and
   * then it calls getQueueInterfaceTemplate() or getOptimizerTemplate()
   * accordingly.
   *
   * @param optStep The optimization step from which to obtain the template
   * @param name The name of the template to obtain
   *
   * @return The template
   */
  std::string getTemplate(size_t optStep, const std::string& name) const;

  /**
   * A generic setTemplate() function. It uses getTemplateType() to
   * determine the template type (either queue interface or optimizer), and
   * then it calls setQueueInterfaceTemplate() or setOptimizerTemplate()
   * accordingly.
   *
   * @param optStep The optimization step from which to set the template
   * @param name The name of the template to set
   * @param temp The template
   */
  void setTemplate(size_t optStep, const std::string& name,
                   const std::string& temp);

  /**
   * Read the queue interface templates from the settings.
   *
   * @param optStep The optimization step to be read.
   * @param filename The filename to be read. If empty, the config file
   *                 will be used.
   */
  void readQueueInterfaceTemplatesFromSettings(size_t optStep,
                                               const std::string& filename);
  /**
   * Read the optimizer templates from the settings.
   *
   * @param optStep The optimization step to be read.
   * @param filename The filename to be read. If empty, the config file
   *                 will be used.
   */
  void readOptimizerTemplatesFromSettings(size_t optStep,
                                          const std::string& filename);
  /**
   * Read the queue interface and optimizer templates from the settings.
   *
   * @param optStep The optimization step to be read.
   * @param filename The filename to be read. If empty, the config file
   *                 will be used.
   */
  void readTemplatesFromSettings(size_t optStep, const std::string& filename);

  /**
   * Read all templates for every opt step from the settings. Also reads
   * the number of opt steps.
   *
   * @param filename The name of the file to be read. If empty, the
   *                 config file will be used.
   */
  void readAllTemplatesFromSettings(const std::string& filename);

  /**
   * Write the queue interface templates to the settings.
   *
   * @param optStep The optimization step to be written.
   * @param filename The filename to be written to. If empty, the config file
   *                 will be used.
   */
  void writeQueueInterfaceTemplatesToSettings(size_t optStep,
                                              const std::string& filename);

  /**
   * Write the optimizer templates to the settings.
   *
   * @param optStep The optimization step to be written.
   * @param filename The filename to be written to. If empty, the config file
   *                 will be used.
   */
  void writeOptimizerTemplatesToSettings(size_t optStep,
                                         const std::string& filename);

  /**
   * Write the queue interface and optimizer templates to the settings.
   *
   * @param optStep The optimization step to be written.
   * @param filename The filename to be written to. If empty, the config file
   *                 will be used.
   */
  void writeTemplatesToSettings(size_t optStep, const std::string& filename);

  /**
   * Write all templates for every opt step to the settings. Also writes
   * the number of opt steps.
   *
   * @param filename The name of the file to be written to. If empty, the
   *                 config file will be used.
   */
  void writeAllTemplatesToSettings(const std::string& filename);

  /**
   * @param filename Scheme or state file from which to load all
   * user values (m_user[1-4])
   */
  void readUserValuesFromSettings(const std::string& filename = "");

  /**
   * @param filename Scheme or state file in which to write all
   * user values (m_user[1-4])
   */
  void writeUserValuesToSettings(const std::string& filename = "");

  /**
   * @return A user customizable string that is used in template
   * interpretation.
   */
  std::string getUser1() { return m_user1; }

  /**
   * @return A user customizable string that is used in template
   * interpretation.
   */
  std::string getUser2() { return m_user2; }

  /**
   * @return A user customizable string that is used in template
   * interpretation.
   */
  std::string getUser3() { return m_user3; }

  /**
   * @return A user customizable string that is used in template
   * interpretation.
   */
  std::string getUser4() { return m_user4; }

  /**
   * @param s A user customizable string that is used in template
   * interpretation.
   */
  void setUser1(const std::string& s) { m_user1 = s; }

  /**
   * @param s A user customizable string that is used in template
   * interpretation.
   */
  void setUser2(const std::string& s) { m_user2 = s; }

  /**
   * @param s A user customizable string that is used in template
   * interpretation.
   */
  void setUser3(const std::string& s) { m_user3 = s; }

  /**
   * @param s A user customizable string that is used in template
   * interpretation.
   */
  void setUser4(const std::string& s) { m_user4 = s; }

  /**
   * Are we ready to search (checks that essential variables are set and
   * that the queue interfaces and optimizers are ready for the search).
   *
   * @param err The error message if we are not ready to search.
   *
   * @return True if ready to search. False otherwise.
   */
  bool isReadyToSearch(QString& err) const;

  /**
   * Do we have any remote queue interfaces?
   *
   * @return True if we do. False if we do not.
   */
  bool anyRemoteQueueInterfaces() const;

  /**
   * Prompt user with a "Yes/No" dialog.
   *
   * @param message Message to the user.
   * @param ok True if user accepts dialog, false if they cancel.
   * @sa needBoolean
   */
  void promptForBoolean(const QString& message, bool* ok = 0);

  /**
   * Request a password from the user, used for libssh
   * authentication.
   *
   * @param message Message to the user.
   * @param newPassword pointer to the QString that will hold the new password.
   * @param ok True if user accepts dialog, false if they cancel.
   * @sa needPassword
   */
  void promptForPassword(const QString& message, QString* newPassword,
                         bool* ok = 0);

  /**
   * Set the clipboard contents to \a text. Also sets the global
   * mouse selection on supported systems.
   *
   * @param text Text to place on the clipboard
   */
  void setClipboard(const QString& text) const;

  /**
   * @return Whether or not we are to cancel a job after a given amount
   *         of time.
   */
  bool cancelJobAfterTime() const { return m_cancelJobAfterTime; }

  /**
   * @return The amount of time in hours that, if exceeded, we are to
   *         cancel a job.
   */
  double hoursForCancelJobAfterTime() const
  {
    return m_hoursForCancelJobAfterTime;
  }

protected slots:
  // Disable doxygen parser here:
  /// \cond
  void setClipboard_(const QString& text) const;
/// \endcond

  /**
   * Resubmit incomplete hardness calculations every 10 minutes if
   * m_calculateHardness is true (runs in the current thread).
   */
  void _startHardnessResubmissionThread();

  /**
   * This should be called when the aflow calculation is completed for
   * @param ind. It will obtain the data from Aflow and set the data to
   * the structure. It will run in a separate thread.
   *
   * @param ind The AflowML index to be updated.
   */
  void finishHardnessCalculation(size_t ind);

  /**
   * Same as above except runs in the current thread.
   */
  void _finishHardnessCalculation(size_t ind);

#ifdef ENABLE_SSH
#ifndef USE_CLI_SSH
  /**
   * Create ssh connections using the libssh backend.
   */
  bool createSSHConnections_libssh();

#else // not USE_CLI_SSH

  /**
   * Create ssh connections using a commandline ssh backend.
   */
  bool createSSHConnections_cli();

#endif // not USE_CLI_SSH
#endif // ENABLE_SSH
protected:

  /// Multi-objective parameters for the run
  int                m_objectives_num; // total number of objectives (internal variable), default = 0
  QStringList        m_objectives_exe; // location of external scripts for each objective
  QList<ObjectivesType> m_objectives_typ; // type of optimization: minimization/maximization/filtration/hardness
  QList<double>      m_objectives_wgt; // weight for each objective
  QStringList        m_objectives_out; // output file name produced by the external script for each objective
  QStringList        m_objectives_lst; // list of objective inputs in xtalopt.in file (internal variable)

  /// String that uniquely identifies the derived OptBase
  /// @sa getIDString
  QString m_idString;

  /// Cached pointer to the SSHManager
  /// @sa ssh
  SSHManager* m_ssh;

  /// Cached pointer to the Dialog window
  /// @sa tracker
  AbstractDialog* m_dialog;

  /// Cached pointer to the main Tracker
  /// @sa tracker
  Tracker* m_tracker;

  /// Thread to run the QueueManager
  QThread* m_queueThread;

  /// Cached pointer to the QueueManager
  /// @sa queue
  QueueManager* m_queue;

  /// The number of optimization steps
  /// Do not change this number directly. Instead, call appendOptStep(),
  /// insertOptStep(), and removeOptStep().
  int m_numOptSteps;

  /// Queue interfaces for particular opt steps
  std::vector<std::unique_ptr<QueueInterface>> m_queueInterfaceAtOptStep;

  /// Optimizer for particular opt steps
  std::vector<std::unique_ptr<Optimizer>> m_optimizerAtOptStep;

  /// A vector of templates for each opt step. Each vector element is a map
  /// of the template name to its value.
  typedef std::vector<std::map<std::string, std::string>> fileTemplateType;

  fileTemplateType m_queueInterfaceTemplates;

  fileTemplateType m_optimizerTemplates;

  std::string m_user1, m_user2, m_user3, m_user4;

  /// Hidden call to interpretKeyword
  void interpretKeyword_base(QString& keyword, Structure* structure);

  /// Hidden call to getTemplateKeywordHelp
  QString getTemplateKeywordHelp_base();

  /// Current version of save/resume schema
  unsigned int m_schemaVersion;

  bool m_usingGUI;

  /// The queue refresh interval for remote queue interfaces
  int m_queueRefreshInterval;

  /// Whether or not to clean remote directories after completion
  bool m_cleanRemoteOnStop;

public:
  /// Log error directories?
  bool m_logErrorDirs;

  /// Calculate hardness using Aflow machine learning? (Requires internet)
  std::atomic<bool> m_calculateHardness;

  /// What is the weight of the hardness fitness?
  std::atomic<double> m_hardnessFitnessWeight;

  /// Only one QNetworkAccessManager is needed for a whole program
  std::shared_ptr<QNetworkAccessManager> m_networkAccessManager;

  /// For performing Aflow ML calculations
  std::unique_ptr<AflowML> m_aflowML;

  /// A map of the AflowML indicies to their pending hardness calculations
  std::unordered_map<size_t, Structure*> m_pendingHardnessCalculations;

  /// Should we cancel the job if a number of hours are exceeded?
  /// This is primarily implemented because some optimizers have
  /// bugs that cause them to run forever. But also because XtalOpt
  /// will sometimes think a job is queued even though it is not.
  bool m_cancelJobAfterTime = false;
  double m_hoursForCancelJobAfterTime = 100.0;
};

} // end namespace GlobalSearch

#endif

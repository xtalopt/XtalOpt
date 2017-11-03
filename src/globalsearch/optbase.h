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
#include <QObject>
#include <QMutex>

#include <memory>
#include <mutex>

#include <globalsearch/bt.h> // TEMPORARY

class QMutex;

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
    explicit OptBase(AbstractDialog *parent = nullptr);

    /**
     * Destructor
     */
    virtual ~OptBase() override;

    /**
     * Actions to take when a structure has failed optimization too
     * many times.
     *
     * @sa OptBase::failAction
     * @sa OptBase::failLimit
     */
    enum FailActions {
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
    enum TemplateType {
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
    QString getIDString() {return m_idString;}

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
    virtual Structure* replaceWithRandom(Structure *s,
                                         const QString & reason = "")
    {return 0;}

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
    virtual Structure* replaceWithOffspring(Structure *s,
                                            const QString & reason = "")
    {return replaceWithRandom(s, reason);}

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
   	virtual bool checkStepOptimizedStructure(Structure *s, QString *err = NULL)
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
     * Generate a probability list using the enthalpies of a
     * collection of structures.
     *
     * The probability is calculated by:
     *
     * p_i = N * (1 - (H_i - H_min) / (H_max - H_min) )
     *
     * where p_i is the probability of selecting structure i, H_i is
     * the enthalpy of structure i, H_min and H_max are the lowest and
     * highest enthalpies in the collection, and N is a normalization
     * factor.
     *
     * To use the probability list generated by this function, run
@code
double r = getRandDouble();
int ind;
for (ind = 0; ind < probs.size(); ind++)
  if (r < probs.at(ind)) break;
@endcode
     * ind will hold the chosen index.
     *
     * @param structures Collection of Structure objects to use. Must
     * be sorted by enthalpy
     *
     * @sa Structure::sortByEnthalpy()
     *
     * @note IMPORTANT: \a structures must contain one more structure
     * than needed -- the last structure in the list will be removed
     * from the probability list! (e.g. return list has size
     * (structure.size()-1)).
     *
     * @return
     */
    static QList<double> getProbabilityList(
                             const QList<Structure*> &structures);

    /**
     * Save the current search. If filename is omitted, default to
     * m_filePath + "/[search name].state". Will only save once at a time.
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
    virtual bool load(const QString & filename,
                      const bool forceReadOnly = false) {
      Q_UNUSED(forceReadOnly);return false;};

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
    virtual QString interpretTemplate(const QString & templateString,
                                      Structure* structure);

    /**
     * @return A QString defining all known keywords.
     */
    virtual QString getTemplateKeywordHelp() {
      return getTemplateKeywordHelp_base();};

    /**
     * Set the main dialog..
     */
    void setDialog(AbstractDialog* d) { m_dialog = d; emit dialogSet(); }

    /**
     * @return A pointer to the main dialog..
     */
    AbstractDialog* dialog() { return m_dialog; }

    /**
     * @return A pointer to the main Structure Tracker.
     */
    Tracker* tracker() {return m_tracker;};

    /**
     * @return A pointer to the associated QueueManager.
     */
    QueueManager* queue() {return m_queue;};

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
    virtual std::unique_ptr<QueueInterface>
    createQueueInterface(const std::string& queueName);

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
    virtual std::unique_ptr<Optimizer>
    createOptimizer(const std::string& optName);

    /**
     * @return A pointer to the associated QueueManager.
     * @sa setQueueInterface
     * @sa queueInterfaceChanged
     */
    QueueInterface* queueInterface(int optStep) const;

    /**
     * @return A pointer to the current Optimizer.
     * @sa setOptimizer
     * @sa optimizerChanged
     */
    Optimizer* optimizer(int optStep) const;

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

#ifdef ENABLE_MOLECULAR
    /**
     * Are we in molecular mode? If not, then we are in inorganic mode.
     *
     * @return Whether or not we are in molecular mode.
     */
    bool molecularMode() { return m_molecularMode; }

    /**
     * Are we in molecular mode? If not, then we are in inorganic mode.
     *
     * @param b True if we are in molecular mode. False otherwise.
     */
    void setMolecularMode(bool b) { m_molecularMode = b; }

#else

    /**
     * If this is not a molecular build, always return false for this function
     */
    bool molecularMode() { return false; }

#endif // ENABLE_MOLECULAR

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

#ifdef ENABLE_MOLECULAR

    /// Generate conformers using the settings set below
    long long generateConformers();

    /// Conformer generation settings

    /// The initial molecule file (usually SDF) with which to generate
    /// conformers
    std::string m_initialMolFile;

    /// The output directory for the conformers (SDF format) and their energies
    std::string m_conformerOutDir;

    /// The number of conformers to generate
    size_t m_numConformersToGenerate;

    /// The RMSD threshold to use when pruning conformers (also used for
    /// pruning conformers after optimization if that is set to true)
    double m_rmsdThreshold;

    /// The maximum number of optimization iterations (only valid if
    /// m_mmffOptConfs is true)
    size_t m_maxOptIters;

    /// Whether or not to use MMFF94 to optimize conformers after generation
    bool m_mmffOptConfs;

    /// Whether or not to prune conformers again after using MMFF94 to optimize
    /// them.
    bool m_pruneConfsAfterOpt;

#endif // ENABLE_MOLECULAR

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
    QString filePath;

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
    QString rempath;

    /// This should be locked whenever the state file (resume file) is
    /// being written
    QMutex *stateFileMutex;

    /// A mutex for saving
    std::mutex saveMutex;

    /// True if a session is starting or being loaded
    bool isStarting;

    /// Whether readOnly mode is enabled (e.g. no connection to server)
    bool readOnly;

   signals:
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
    void debugStatement(const QString &s);

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
    void warningStatement(const QString &s);

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
    void errorStatement(const QString &s);

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
    void needBoolean(const QString &message, bool *ok);

    /**
     * Request a password from the user, used for libssh
     * authentication.
     *
     * @param message Message to the user.
     * @param newPassword pointer to the QString that will hold the new password.
     * @param ok True if user accepts dialog, false if they cancel.
     * @sa promptForPassword
     */
    void needPassword(const QString &message, QString *newPassword, bool *ok);

    /**
     * Emitted when a major change has occurred affecting many
     * structures, e.g. when duplicates are set/reset. It is
     * recommended that any user-visible structure data is rebuilt
     * from scratch when this is called.
     */
    void refreshAllStructureInfo();

    // Omit the following from doxygen:
    /// \cond
    void sig_setClipboard(const QString &text) const;
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
    virtual void generateNewStructure() {};

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
    void debug(const QString & s);

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
    void warning(const QString & s);

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
    void error(const QString & s);

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
    void emitSessionStarted() {emit sessionStarted();};

    /**
     * Emits the readOnlySessionStarted signal.
     * @sa sessionStarted
     * @sa emitSessionStarted
     * @sa readOnlySessionStarted
     * @sa startingSession
     * @sa emitStartingSession
     */
    void emitReadOnlySessionStarted() {emit readOnlySessionStarted();};

    /**
     * Emits the startingSession signal.
     * @sa sessionStarted
     * @sa emitSessionStarted
     * @sa startingSession
     * @sa emitStartingSession
     */
    void emitStartingSession() {emit startingSession();};

    /**
     * Sets this->isStarting to true;
     * @sa setIsStartingFalse
     */
    void setIsStartingTrue() {isStarting = true;};

    /**
     * Sets this->isStarting to false;
     * @sa setIsStartingTrue
     */
    void setIsStartingFalse() {isStarting = false;};

    /**
     * Sets this->readOnly to true;
     * @sa setReadOnlyFalse
     */
    void setReadOnlyTrue() {readOnly = true;};

    /**
     * Sets this->readOnly to false;
     * @sa setReadOnlyTrue
     */
    void setReadOnlyFalse() {readOnly = false;};

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
    void setQueueInterfaceTemplate(size_t optStep,
                                   const std::string& name,
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
    void setOptimizerTemplate(size_t optStep,
                              const std::string& name,
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
    TemplateType getTemplateType(size_t optStep,
                                 const std::string& name) const;

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
    std::string getTemplate(size_t optStep,
                            const std::string& name) const;

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
    void setTemplate(size_t optStep,
                     const std::string& name,
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
    void readTemplatesFromSettings(size_t optStep,
                                   const std::string& filename);

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
    void writeTemplatesToSettings(size_t optStep,
                                  const std::string& filename);

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
    void promptForBoolean(const QString &message, bool *ok = 0);

    /**
     * Request a password from the user, used for libssh
     * authentication.
     *
     * @param message Message to the user.
     * @param newPassword pointer to the QString that will hold the new password.
     * @param ok True if user accepts dialog, false if they cancel.
     * @sa needPassword
     */
    void promptForPassword(const QString &message, QString *newPassword, bool *ok = 0);

    /**
     * Set the clipboard contents to \a text. Also sets the global
     * mouse selection on supported systems.
     *
     * @param text Text to place on the clipboard
     */
    void setClipboard(const QString &text) const;

  protected slots:
    // Disable doxygen parser here:
    /// \cond
    void setClipboard_(const QString &text) const;
    /// \endcond

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
    /// String that uniquely identifies the derived OptBase
    /// @sa getIDString
    QString m_idString;

    /// Cached pointer to the SSHManager
    /// @sa ssh
    SSHManager *m_ssh;

    /// Cached pointer to the Dialog window
    /// @sa tracker
    AbstractDialog *m_dialog;

    /// Cached pointer to the main Tracker
    /// @sa tracker
    Tracker *m_tracker;

    /// Thread to run the QueueManager
    QThread *m_queueThread;

    /// Cached pointer to the QueueManager
    /// @sa queue
    QueueManager *m_queue;

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
    void interpretKeyword_base(QString &keyword, Structure* structure);

    /// Hidden call to getTemplateKeywordHelp
    QString getTemplateKeywordHelp_base();

    /// Current version of save/resume schema
    unsigned int m_schemaVersion;

    bool m_usingGUI;

    /// The queue refresh interval for remote queue interfaces
    int m_queueRefreshInterval;

    /// Whether or not to clean remote directories after completion
    bool m_cleanRemoteOnStop;

#ifdef ENABLE_MOLECULAR
    /// Whether or not we are in molecular mode
    bool m_molecularMode;
#endif // ENABLE_MOLECULAR

   public:
    /// Log error directories?
    bool m_logErrorDirs;

  };

} // end namespace GlobalSearch

#endif

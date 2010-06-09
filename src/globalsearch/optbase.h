/**********************************************************************
  OptBase - Base class for global search extensions

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

#ifndef OPTBASE_H
#define OPTBASE_H

#include <QObject>
#include <QMutex>

namespace GlobalSearch {
  class Structure;
  class Tracker;
  class Optimizer;
  class QueueManager;

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
    explicit OptBase(QObject *parent);

    /**
     * Destructor
     */
    virtual ~OptBase();

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
      FA_Randomize
    };

    /**
     * Replace the Structure with an appropriate random Structure.
     *
     * @param s The Structure to be replaced. This pointer remains
     * valid -- the structure it points will be modified.
     * @param reason Reason for replacing. This will appear in the
     * Structure::getParents() string.
     *
     * @return The pointer to the structure (same as s).
     */
    virtual Structure* replaceWithRandom(Structure *s, const QString & reason) {};

    /**
     * Before starting an optimization, this function will check the
     * parameters of the search to ensure that they are within a
     * reasonable range.
     *
     * @return True if the search parameters are valid, false otherwise.
     */
    virtual bool checkLimits() = 0;

    /**
     * Save the current search. If filename is omitted, default to
     * m_filePath + "/[search name].state".
     *
     * @param filename Filename to write to. Optional.
     *
     * @return True if successful, false otherwise.
     */
    virtual bool save(const QString & filename = "") = 0;

    /**
     * Load a search session from the specified filename.
     *
     * @param filename State file to resume.
     *
     * @return True is successful, false otherwise.
     */
    virtual bool load(const QString & filename) {};

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
     * @return A pointer to the main Structure Tracker.
     */
    Tracker* tracker() {return m_tracker;};

    /**
     * @return A pointer to the associated QueueManager.
     */
    QueueManager* queue() {return m_queue;};

    /**
     * @return A pointer to the current Optimizer.
     * @sa setOptimizer
     * @sa optimizerChanged
     */
    Optimizer* optimizer() {return m_optimizer;};

    /// Whether to impose the running job limit
    bool limitRunningJobs;

    /// Number of concurrent jobs allowed.
    uint runningJobLimit;

    /// Number of continuous structures generated
    uint contStructs;

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

    /// Command to submit jobs to the queuing system (default: qsub)
    QString qsub;

    /// Command to check the queuing system (default: qstat)
    QString qstat;

    /// Command to check the queuing system (default: qstat)
    QString qdel;

    /// Host name or IP address of remote PBS server
    /// @note Must have passwordless-login enabled (man ssh-copy-id)
    QString host;

    /// Username for ssh login on remote PBS server
    /// @note Must have passwordless-login enabled (man ssh-copy-id)
    QString username;

    /// Path on remote server to store files during and after
    /// optimization
    QString rempath;

    /// Much of OpenBabel is not written with thread safety in
    /// mind. This mutex should be locked whenever non-static
    /// OpenBabel functions are called.
    QMutex *sOBMutex;

    /// This should be locked whenever the state file (resume file) is
    /// being written
    QMutex *stateFileMutex;

    /// This is locked when generating a backtrace.
    QMutex *backTraceMutex;

    /// True if there is a save requested or in progress
    bool savePending;

    /// True if a session is starting or being loaded
    bool isStarting;

    /// Set to false when running unit tests
    bool saveOnExit;

   signals:
    /**
     * Emitted when a session is starting or being loaded.
     * @sa sessionStarted
     * @sa emitSessionStarted
     * @sa startingSession
     * @sa emitStartingSession
     */
    void startingSession();

    /**
     * Emitted when a session finishes starting or loading.
     * @sa sessionStarted
     * @sa emitSessionStarted
     * @sa startingSession
     * @sa emitStartingSession
     */
    void sessionStarted();

    /**
     * Emitted when the current Optimizer changed
     * @sa setOptimizer
     * @sa optimizer
     */
    void optimizerChanged(Optimizer*);

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

   public slots:

    /**
     * Deletes all structures from m_tracker and calls
     * m_tracker->reset() and m_queue->reset().
     */
    virtual void reset();

    /**
     * Begin the search.
     */
    virtual void startOptimization() = 0;

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
     * Emits the sessionStarted signal.
     * @sa sessionStarted
     * @sa emitSessionStarted
     * @sa startingSession
     * @sa emitStartingSession
     */
    void emitSessionStarted() {emit sessionStarted();};

    /**
     * Emits the startingSession signal.
     * @sa sessionStarted
     * @sa emitSessionStarted
     * @sa startingSession
     * @sa emitStartingSession
     */
    void emitStartingSession() {emit startingSession();};

    /**
     * Sets OptBase::isStarting to true;
     * @sa setIsStartingFalse
     */
    void setIsStartingTrue() {isStarting = true;};

    /**
     * Sets OptBase::isStarting to false;
     * @sa setIsStartingTrue
     */
    void setIsStartingFalse() {isStarting = false;};

    /**
     * Prints a backtrace to the terminal
     */
    void printBackTrace();

    /**
     * Update the Optimizer to the one indicated
     *
     * @param o New Optimizer to use.
     * @sa optimizerChanged
     * @sa optimizer
     */
    void setOptimizer(Optimizer *o) {
      setOptimizer_opt(o);};

    /**
     * Update the Optimizer to the one identified by IDString
     *
     * @param IDString Name of new Optimizer to use
     * @param filename Scheme or state file to initialize Optimizer with
     * @sa optimizerChanged
     * @sa optimizer
     */
    void setOptimizer(const QString &IDString, const QString &filename = "") {
      setOptimizer_string(IDString, filename);};

   protected:
    /// Cached pointer to the main Tracker
    /// @sa tracker
    Tracker *m_tracker;

    /// Cached pointer to the QueueManager
    /// @sa queue
    QueueManager *m_queue;

    /// Cache pointer to the current optimizer
    /// @sa optimizerChanged
    /// @sa optimizer
    /// @sa setOptimizer
    Optimizer *m_optimizer;

    /// Hidden call to setOptimizer
    virtual void setOptimizer_opt(Optimizer *o);

    /// Hidden call to setOptimizer
    virtual void setOptimizer_string(const QString &s, const QString &filename = "") {};

    /// Hidden call to interpretKeyword
    void interpretKeyword_base(QString &keyword, Structure* structure);

    /// Hidden call to getTemplateKeywordHelp
    QString getTemplateKeywordHelp_base();

  };

} // end namespace Avogadro

#endif

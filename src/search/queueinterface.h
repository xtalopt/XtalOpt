/**********************************************************************
  QueueInterface - Job submission interface

  Copyright (C) 2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef QUEUEINTERFACE_H
#define QUEUEINTERFACE_H

#include <QHash>
#include <QObject>
#include <QStringList>

#include <functional>
#include <memory>

namespace Search {

class Structure;
class Optimizer;
class SearchBase;

/**
 * @class QueueInterface queueinterface.h <search/queueinterface.h>
 *
 * @brief Interface for job submission.
 *
 * @author David C. Lonie
 *
 * Use DirectRunInterface or BatchQueueInterface for a queue class. A
 * QueueInterface writes input files, starts and stops jobs, reports their
 * status, and prepares files before Optimizer reads the results. QueueManager
 * uses that status to update Structure states.
 */

/// Default values for a queue interface.
struct QueueDefaults
{
  const char* idString;     // e.g. "SLURM"
  const char* templateFile; // e.g. "job.slurm" ("" if none)
  const char* submit;       // batch submit command ("" for non-batch queues)
  const char* status;       // batch status command
  const char* cancel;       // batch cancel command
};

class QueueInterface : public QObject
{
  Q_OBJECT

public:
  /**
   * Register a queue interface under the given name. The registration order
   *   determines the order returned by registeredQueueInterfaces(), which, for
   *   example, determines the order in a GUI menu.
   */
  static bool registerQueueInterface(const QString& name,
    std::function<std::unique_ptr<QueueInterface>(SearchBase*)> creator);

  /** @return Names of all registered queue interfaces, in registration order. */
  static QStringList registeredQueueInterfaces();

  /** @return Names of all engine-provided queue interfaces, in built-in order. */
  static QStringList availableBuiltInQueueInterfaces();

  /** Register one queue interface provided by the engine. */
  static bool registerBuiltInQueueInterface(const QString& name);

  /**
   * Constructor
   *
   * @param parent The search this queue interface belongs to
   * @param settingFile Filename from which to initialize settings.
   */
  explicit QueueInterface(SearchBase* parent, const QString& settingFile = "");

  /**
   * Destructor
   */
  virtual ~QueueInterface() override;

  /**
   * Possible status for running jobs
   * @sa getStatus
   */
  enum QueueStatus
  {
    /// Something very bizarre has happened
    Unknown = -1,
    /// Job has completed successfully
    Success,
    /// Job finished, but the optimization was unsuccessful
    Error,
    /// Job is queued
    Queued,
    /// Job is current running
    Running,
    /// Communication with a remote server has failed
    CommunicationError,
    /// Job has appeared in queue, but the Structure still returns
    /// Structure::Submitted instead of Structure::InProcess. This
    /// will be corrected in the next iteration of
    /// QueueManager::checkRunning().
    Started,
    /// Job has been submitted, but has not appeared in queue
    Pending
  };

  /**
   * Result of a command run through a queue interface.
   */
  struct CommandResult
  {
    CommandResult() : launched(false), exitCode(-1) {}

    /// Whether the command process was launched.
    bool launched;
    /// Process exit code, or -1 when no process exit code is available.
    int exitCode;
    /// Captured standard output.
    QString stdoutText;
    /// Captured standard error.
    QString stderrText;

    /**
     * @return true only when the command launched and exited with code zero.
     */
    bool succeeded() const { return launched && exitCode == 0; }
  };

  /**
   * Check that all mandatory internal variables are set. Check this
   * before starting a search.
   *
   * @param err String to be overwritten with an error message
   *
   * @return true if all variables are initialized, false
   * otherwise. If false, \a err will be overwritten with a
   * user-friendly error message.
   */
  virtual bool isReadyToSearch(QString* err)
  {
    if (err)
      err->clear();
    return true;
  }

public slots:

  /**
   * Write the input files for Structure \a s to the appropriate
   * location.
   *
   * This function will also construct and write any queue-specific
   * files (e.g. job.pbs for PBS queues) and copy them to the execution
   * host if using a remote batch queue.
   *
   * @return True on success, false otherwise.
   */
  virtual bool writeInputFiles(Structure* s) const;

  /**
   * Write the provided files in the hash \a files to the local
   * working directory for Structure \a s and (if appropriate) copy
   * them to a remote server.
   *
   * @param s Structure of interest
   * @param files Key: filename, Value: text.
   *
   * @note The filenames in \a files must not be absolute, but
   * relative to the structure's working directory.
   *
   * @return True on success, false otherwise.
   */
  virtual bool writeFiles(Structure* s,
                          const QHash<QString, QString>& files) const = 0;

  /**
   * Start a job for Structure \a s.
   *
   * @note Ensure that writeFiles is called before attempting to
   * start the job.
   *
   * @return True on success, false otherwise.
   */
  virtual bool startJob(Structure* s) = 0;

  /**
   * Stop any currently running jobs for Structure \a s.
   *
   * @return True on success, false otherwise.
   */
  virtual bool stopJob(Structure* s) = 0;

  /**
   * @return The queue status of Structure \a s.
   */
  virtual QueueInterface::QueueStatus getStatus(Structure* s) const = 0;

  /**
   * Perform any work needed before calling Optimizer::update. This
   * function mainly exists for batch queue classes to copy files
   * back from the execution host, but may be used for other purposes. It is
   * guaranteed to be called by Optimizer before updating.
   *
   * @param s The structure that is to be updated.
   *
   * @return True on success, false otherwise.
   */
  virtual bool prepareForStructureUpdate(Structure* s) const = 0;

  /** Release objects that belong to the queue thread before it stops. */
  virtual void prepareForThreadStop() {}

  /**
   *  Runs a command (e.g., bash command or script) for either a direct
   *  run, a local batch queue, or a remote batch queue.
   *
   *  @note For a remote run, this function creates and discards its own
   *  SSH connection.
   *
   * @param workdir The working directory in which command is running
   * @param command The command to be run
   * @param timeoutMs Maximum run time, or -1 for no time limit
   * @return Command launch, exit code, stdout, and stderr. Callers decide
   * whether a non-zero exit is acceptable for their command.
   */
  virtual CommandResult runACommand(const QString& workdir, const QString& command, int timeoutMs = -1) const = 0;

  /**
   * Copy a file from the execution host to a local destination.
   *
   * @note For direct runs and local batch queues, the execution host is
   *  already the local machine and this does nothing.
   * @note For a remote run, this creates and discards its own SSH
   *  connection.
   *
   * @param rem_file Full path to the remote source file
   * @param loc_file Full path to the local destination file
   */
  virtual bool copyFileFromExecutionHost(const QString& rem_file, const QString& loc_file);

  /**
   * Copy a file from a local source to the execution host.
   *
   * @note For direct runs and local batch queues, the execution host is
   *  already the local machine and this does nothing.
   * @note For a remote run, this creates and discards its own SSH
   *  connection.
   *
   * @param loc_file Full path to the local source file
   * @param rem_file Full path to the remote destination file
   */
  virtual bool copyFileToExecutionHost(const QString& loc_file, const QString& rem_file);

  /**
   * Remove a file from a local or remote working directory of structure.
   *
   * @note This function does not check if file exists. If that was
   * not checked before calling this, then a false return value might
   * be because of both file not existing or an issue in removing it.
   *
   * @note It is a wrapper for both remote and local-batch runs.
   * @note For a remote run, this creates and discards its own SSH
   *  connection.
   *
   * @param filename The full path and name of the file to remove
   */
  virtual bool removeAFile(Structure* s, const QString& filename);

  /**
   * Check if the file \a filename exists in the working directory
   * of Structure \a s and store the result in \a exists.
   *
   * @note This function uses the argument \a exists to report
   * whether or not the file exists. The return value indicates
   * whether the file check was performed without errors
   * (e.g. network errors).
   *
   * @return True if the test encountered no errors, false otherwise.
   */
  virtual bool checkIfFileExists(Structure* s, const QString& filename, bool* exists);

  /**
   * Check several files in one call. Queue interfacesmay override this to
   * avoid repeated remote directory reads; the default just calls
   * checkIfFileExists() for each file.
   */
  virtual bool checkIfFilesExist(Structure* s, const QStringList& filenames,
                                 QHash<QString, bool>* exists);

  /**
   * Retrieve the contents of the file \a filename for Structure \a
   * s as a QString \a contents.
   *
   * @return True on success, false otherwise.
   */
  virtual bool fetchFile(Structure* s, const QString& filename, QString* contents) const;

  /**
   * Grep through the file \a filename in Structure \a s's working
   * directory, looking for \a matchText. The list of matches is
   * returned in the QStringList \a matches and the exit status is
   * returned as \a exitcode.
   *
   * Possible exitcodes:
   *   - 0: Matches were found, execution successful
   *   - 1: No matches found, execution successful
   *   - 2: Execution unsuccessful
   *
   * @param s Structure of interest
   * @param matchText Text to match
   * @param filename Name of file to grep
   * @param matches List of matches (return)
   * @param exitcode Exit code of grep (see details) (return)
   * @param caseSensitive If true, match case. Otherwise, perform
   * case-insensitive search (e.g. grep -i) Default is true.
   *
   * @return True on success, false otherwise.
   *
   * @note There are two types of failure possible here: Either the
   * \a exitcode can be 2 or the function can return false. If the
   * \a exitcode is 2, then grep failed to execute. If false, then
   * there was a failure in the interface code, likely a
   * communication error with a remote server.
   *
   * @note On direct-run interface, grep is not actually used and
   * the exit code behavior is emulated.
   */
  virtual bool grepFile(Structure* s, const QString& matchText,
                        const QString& filename, QStringList* matches = 0, int* exitcode = 0,
                        const bool caseSensitive = true) const;

  /**
   * @return The name of the queue interface (e.g. "Local", "PBS",
   * etc)
   */
  // Built-in interfaces report the id from their default; a custom interface (e.g.
  //   a test mock) with no default may override this.
  virtual QString getIDString() const
  {
    return m_queueDefaults ? QString::fromLatin1(m_queueDefaults->idString) : QString();
  };

  /** @return True if this interface submits through a batch scheduler. */
  virtual bool isBatchQueue() const { return false; }

  /** Batch scheduler submit command, if supported by this interface. */
  virtual QString submitCommand() const { return QString(); }
  /** Set the batch scheduler submit command, if supported by this interface. */
  virtual void setSubmitCommand(const QString&) {}

  /** Batch scheduler cancel command, if supported by this interface. */
  virtual QString cancelCommand() const { return QString(); }
  /** Set the batch scheduler cancel command, if supported by this interface. */
  virtual void setCancelCommand(const QString&) {}

  /** Batch scheduler status command, if supported by this interface. */
  virtual QString statusCommand() const { return QString(); }
  /** Set the batch scheduler status command, if supported by this interface. */
  virtual void setStatusCommand(const QString&) {}

  /**
   * @return The names of all template files associated with this
   * interface.
   */
  QStringList getQueueInterfaceTemplateFileNames() const
  {
    QStringList list;
    if (m_queueDefaults && m_queueDefaults->templateFile[0] != '\0')
      list.append(QString::fromLatin1(m_queueDefaults->templateFile));
    return list;
  };

  /**
   * Get the current optimizer being used for a particular structure.
   */
  Optimizer* getCurrentOptimizer(Structure* s) const;

protected:
  /**
   * Write the contents of @p fileHash as files in the structure's local
   * working directory. Opens each file, streams the text, closes and cleans up.
   * @return true on success; false if any file cannot be opened.
   */
  bool writeHashToLocalDir(Structure* s, const QHash<QString, QString>& fileHash) const;

  /**
   * Copy any "extra" seed files (s->copyFiles()) into the structure's local
   * working directory and clear the list. Existing names in @p extraFilenames
   * are reserved for generated inputs; each copied filename is appended so
   * callers can include it in subsequent SSH transfers.
   * @return true on success; false if any file cannot be copied.
   */
  bool writeCopyFilesToLocalDir(Structure* s, QStringList& extraFilenames) const;

  /**
   * Check that the local working directory exists or can be created.
   */
  bool localWorkingDirectoryReady(QString* err) const;

  /**
   * Check that @p filename is a non-empty relative path without parent
   * traversal.
   */
  static bool safeRelativeFilename(const QString& filename);

  /// Cached pointer to the parent SearchBase class
  SearchBase* m_search;

  /// Per-queue defaults for a built-in interface (id, template, commands);
  ///   each built-in subclass sets this from its own defaults() in its ctor. Null
  ///   until set (e.g. a custom interface that overrides the getters instead).
  const QueueDefaults* m_queueDefaults = nullptr;

private:
  friend class SearchBase;

  /**
   * Create a queue interface by its registered name.
   * @return The new interface, or nullptr if name is unknown.
   */
  static std::unique_ptr<QueueInterface> createRegisteredQueueInterface(
    const QString& name, SearchBase* parent);
};
}

#endif

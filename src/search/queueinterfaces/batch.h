/**********************************************************************
  BatchQueueInterface - Interface for running jobs through a batch scheduler,
     either locally or over SSH.

  Copyright (C) 2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef BATCHQUEUEINTERFACE_H
#define BATCHQUEUEINTERFACE_H

#include <search/queueinterface.h>

#include <QDateTime>
#include <QReadLocker>
#include <QReadWriteLock>
#include <QWriteLocker>


namespace Search {
class SSHConnection;

/**
 * @class BatchQueueInterface batch.h <search/queueinterfaces/batch.h>
 *
 * @brief Interface for running jobs through a batch scheduler.
 *
 * @author David C. Lonie
 */
class BatchQueueInterface : public QueueInterface
{
  Q_OBJECT

public:
  /**
   * Constructor
   *
   * @param parent SearchBase parent
   * @param settingFile Filename from which to initialize settings.
   */
  explicit BatchQueueInterface(SearchBase* parent, const QString& settingFile = "");

  /**
   * Destructor
   */
  virtual ~BatchQueueInterface() override;

  bool isBatchQueue() const override { return true; }

  /**
   * Get the submit command for the queue interface. For example, in slurm,
   * this might be 'sbatch'.
   *
   * @return The submit command.
   */
  QString submitCommand() const override
  {
    QReadLocker locker(&m_commandLock);
    return !m_submitOverride.isEmpty() ? m_submitOverride
           : (m_queueDefaults ? QString::fromLatin1(m_queueDefaults->submit) : QString());
  }

  /**
   * Set the submit command for the queue interface. For example, in slurm,
   * this might be 'sbatch'.
   *
   * @s The submit command.
   */
  void setSubmitCommand(const QString& s) override
  {
    QWriteLocker locker(&m_commandLock);
    m_submitOverride = s;
  }

  /**
   * Get the cancel command for the queue interface. For example, in slurm,
   * this might be 'scancel'.
   *
   * @return The cancel command.
   */
  QString cancelCommand() const override
  {
    QReadLocker locker(&m_commandLock);
    return !m_cancelOverride.isEmpty() ? m_cancelOverride
           : (m_queueDefaults ? QString::fromLatin1(m_queueDefaults->cancel) : QString());
  }

  /**
   * Set the cancel command for the queue interface. For example, in slurm,
   * this might be 'scancel'.
   *
   * @s The cancel command.
   */
  void setCancelCommand(const QString& s) override
  {
    QWriteLocker locker(&m_commandLock);
    m_cancelOverride = s;
  }

  /**
   * Get the status command for the queue interface. For example, in slurm,
   * this might be 'squeue'.
   *
   * @return The status command.
   */
  QString statusCommand() const override
  {
    QReadLocker locker(&m_commandLock);
    return !m_statusOverride.isEmpty() ? m_statusOverride
           : (m_queueDefaults ? QString::fromLatin1(m_queueDefaults->status) : QString());
  }

  /**
   * Set the status command for the queue interface. For example, in slurm,
   * this might be 'squeue'.
   *
   * @s The status command.
   */
  void setStatusCommand(const QString& s) override
  {
    QWriteLocker locker(&m_commandLock);
    m_statusOverride = s;
  }

public slots:

  bool isReadyToSearch(QString* str) override;

  /**
   * Write the input files in the hash \a files to the appropriate
   * location for Structure \a s.
   *
   * This function will also construct and write any queue-specific template
   * files (e.g. job.pbs for PBS queues; see getQueueInterfaceTemplateFileNames()).
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
                          const QHash<QString, QString>& files) const override;

  /**
   * Start a job for Structure \a s.
   *
   * @note Ensure that writeFiles is called before attempting to
   * start the job.
   *
   * @return True on success, false otherwise.
   */
  virtual bool startJob(Structure* s) override;

  /**
   * Stop any currently running jobs for Structure \a s.
   *
   * @return True on success, false otherwise.
   */
  virtual bool stopJob(Structure* s) override;

  /**
   * @return The queue status of Structure \a s.
   */
  virtual QueueInterface::QueueStatus getStatus(Structure* s) const override;

  /**
   * Perform any work needed before calling Optimizer::update. Mainly copies
   * files back from the execution host. Called by Optimizer before updating.
   *
   * @param s The structure that is to be updated.
   *
   * @return True on success, false otherwise.
   */
  virtual bool prepareForStructureUpdate(Structure* s) const override;

  /**
   *  Runs a command (e.g., bash command or script) for a batch queue job.
   *
   * @note This is a wrapper function for both local-batch and remote-batch
   *  jobs. For remote jobs it creates and discards its own SSH connection.
   *
   * @param workdir Working directory for the command
   * @param command The command
   * @return Command launch, exit code, stdout, and stderr.
   */
  virtual CommandResult runACommand(const QString& workdir, const QString& command, int timeoutMs = -1) const override;

  /**
   * Copy a file from the execution host to a local destination.
   *
   * @note This is a wrapper function for both local-batch and remote-batch
   *  jobs. For remote jobs it creates and discards its own SSH connection.
   *
   * @param rem_file Full path to the file to be copied on the execution host
   * @param loc_file Full path to the destination file (a local path)
   */
  virtual bool copyFileFromExecutionHost(const QString& rem_file, const QString& loc_file) override;

  /**
   * Copy a file from a local source to the execution host.
   *
   * @note This is a wrapper function for both local-batch and remote-batch
   *  jobs. For remote jobs it creates and discards its own SSH connection.
   *
   * @param loc_file Full path of the file to be copied (a local path)
   * @param rem_file Full path to the destination file on the execution host
   */
  virtual bool copyFileToExecutionHost(const QString& loc_file, const QString& rem_file) override;

  /**
   * Remove a file by name from the working directory for a structure.
   *
   * @note This is a wrapper function for both local-batch and remote-batch
   *  jobs. For remote jobs, if an existing SSH connection is passed to the
   *  function it will be used; otherwise the function creates and discards one.
   *
   * @param filename The file's name
   * @param ssh An initialized SSHConnection
   *
   * @return True on success, false otherwise
   */
  virtual bool removeAFile(Structure *s, const QString& filename) override;

  /**
   * Check if the file \a filename exists in the working directory
   * of Structure \a s and store the result in \a exists.
   *
   * @note This function uses the argument \a exists to report
   * whether or not the file exists. The return value indicates
   * whether the file check was performed without errors
   * (e.g. network errors).
   *
   * @note This is a wrapper function for both local-batch and remote-batch
   *  jobs. For remote jobs it creates and discards its own SSH connection.
   *
   * @return True if the test encountered no errors, false otherwise.
   */
  virtual bool checkIfFileExists(Structure* s, const QString& filename,
                                 bool* exists) override;
  virtual bool checkIfFilesExist(Structure* s, const QStringList& filenames,
                                 QHash<QString, bool>* exists) override;
  /**
   * Retrieve the contents of the file \a filename for Structure \a
   * s as a QString \a contents.
   * @note This is a wrapper function for both local-batch and remote-batch
   *  jobs. For remote jobs it creates and discards its own SSH connection.
   *
   * @return True on success, false otherwise.
   */
  virtual bool fetchFile(Structure* s, const QString& filename,
                         QString* contents) const override;

  /**
   * Grep through the file \a filename for Structure \a s's working
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
   * @note This is a wrapper function for both local-batch and remote-batch
   *  jobs. For remote jobs it creates and discards its own SSH connection.
   */
  virtual bool grepFile(Structure* s, const QString& matchText,
                        const QString& filename, QStringList* matches = 0,
                        int* exitcode = 0,
                        const bool caseSensitive = true) const override;

protected:
  bool remoteQueueConfigurationReady(QString* err) const;

  virtual QString submitScriptName() const;
  virtual bool submitScriptOnStdin() const { return false; }
  virtual unsigned int parseJobId(const QString& submissionOutput, bool* ok) const = 0;
  // Each subclass turns its scheduler's queue-list lines into a QueueStatus;
  // rawStatus is only used for the warning when the status is not recognized.
  virtual QueueInterface::QueueStatus parseQueueStatus(const QStringList& queueData,
    unsigned int jobId, QString* rawStatus = nullptr) const = 0;
  virtual QString queueListCommand() const;
  virtual bool queueListCommandSucceeded(bool ok, int exitCode, const QString& stdoutText,
                                         const QString& stderrText) const;
  virtual QueueInterface::QueueStatus missingFromQueueWithoutOutputStatus(Structure* s) const;

  QStringList queueList(bool forced = false) const;

  /**
   * Create a working directory for \a structure on the execution host.
   * @note This is a wrapper function for both local-batch and remote-batch
   *  jobs. For remote jobs, if an existing SSH connection is passed to the
   *  function it will be used; otherwise the function creates and discards one.
   *
   * @param structure Structure of interest
   * @param ssh An initialized SSHConnection to use.
   *
   * @sa Structure::getRempath
   *
   * @return True on success, false otherwise
   */
  bool createExecutionDirectory(Structure* structure, SSHConnection* ssh = nullptr) const;

  /**
   * Remove all files from \a structure's execution-host working directory.
   * @note This is a wrapper function for both local-batch and remote-batch
   *  jobs. For remote jobs, if an existing SSH connection is passed to the
   *  function it will be used; otherwise the function creates and discards one.
   *
   * @param structure Structure of interest
   * @param ssh An initialized SSHConnection
   *
   * @return True on success, false otherwise
   */
  bool cleanExecutionDirectory(Structure* structure, SSHConnection* ssh = nullptr) const;

  /**
   * Copy all files from \a structure's execution-host working directory to the
   * local working directory.
   * @note This is a wrapper function for both local-batch and remote-batch
   *  jobs. For remote jobs it creates and discards its own SSH connection.
   *
   * @param structure Structure of interest
   * @param ssh An initialized SSHConnection to use.
   *
   * @return True on success, false otherwise
   */
  bool copyExecutionFilesToLocalDirectory(Structure* structure) const;

  /**
   * Saves a copy of the error directory that caused this structure to fail.
   * Saves it in <remote_path>/errorDirs
   * @note This is a wrapper function for both local-batch and remote-batch
   *  jobs. For remote jobs it creates and discards its own SSH connection.
   *
   * @param structure Structure of interest
   * @param ssh An initialized SSHConnection to use.
   *
   * @return True on success, false otherwise
   */
  bool logErrorDirectory(Structure* structure) const;

private:
  QueueInterface::QueueStatus statusFromMissingQueueEntry(Structure* s) const;
  CommandResult runBatchCommand(const QString& workdir, const QString& command,
                                const QString& stdinFile, int timeoutMs = -1) const;

  // User-defined settings for the batch queue commands.
  QString m_submitOverride;
  QString m_cancelOverride;
  QString m_statusOverride;
  mutable QReadWriteLock m_commandLock;

  mutable QStringList m_queueData;
  mutable QDateTime m_queueTimeStamp;
  mutable QReadWriteLock m_queueMutex;
};
}

#endif // BATCHQUEUEINTERFACE_H

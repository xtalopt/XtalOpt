/**********************************************************************
  RemoteQueueInterface - Interface for running jobs on a remote cluster.

  Copyright (C) 2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef REMOTEQUEUEINTERFACE_H
#define REMOTEQUEUEINTERFACE_H

#ifdef ENABLE_SSH

#include <globalsearch/queueinterface.h>

namespace GlobalSearch {
class SSHConnection;

/**
 * @class RemoteQueueInterface remote.h <globalsearch/remote.h>
 *
 * @brief Interface for running jobs on a remote cluster.
 *
 * @author David C. Lonie
 */
class RemoteQueueInterface : public QueueInterface
{
  Q_OBJECT

public:
  /**
   * Constructor
   *
   * @param parent OptBase parent
   * @param settingFile Filename from which to initialize settings.
   */
  explicit RemoteQueueInterface(OptBase* parent,
                                const QString& settingFile = "");

  /**
   * Destructor
   */
  virtual ~RemoteQueueInterface() override;

  /**
   * Get the submit command for the queue interface. For example, in slurm,
   * this might be 'sbatch'.
   *
   * @return The submit command.
   */
  virtual QString submitCommand() const { return m_submitCommand; }

  /**
   * Set the submit command for the queue interface. For example, in slurm,
   * this might be 'sbatch'.
   *
   * @s The submit command.
   */
  virtual void setSubmitCommand(const QString& s) { m_submitCommand = s; }

  /**
   * Get the cancel command for the queue interface. For example, in slurm,
   * this might be 'scancel'.
   *
   * @return The cancel command.
   */
  virtual QString cancelCommand() const { return m_cancelCommand; }

  /**
   * Set the cancel command for the queue interface. For example, in slurm,
   * this might be 'scancel'.
   *
   * @s The cancel command.
   */
  virtual void setCancelCommand(const QString& s) { m_cancelCommand = s; }

  /**
   * Get the status command for the queue interface. For example, in slurm,
   * this might be 'squeue'.
   *
   * @return The status command.
   */
  virtual QString statusCommand() const { return m_statusCommand; }

  /**
   * Set the status command for the queue interface. For example, in slurm,
   * this might be 'squeue'.
   *
   * @s The status command.
   */
  virtual void setStatusCommand(const QString& s) { m_statusCommand = s; }

public slots:

  /**
   * Write the input files in the hash \a files to the appropriate
   * location for Structure \a s.
   *
   * This function will also construct and write any queue-specific
   * files in m_templates (e.g. job.pbs for PBS queues).
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
  virtual bool startJob(Structure* s) override = 0;

  /**
   * Stop any currently running jobs for Structure \a s.
   *
   * @return True on success, false otherwise.
   */
  virtual bool stopJob(Structure* s) override = 0;

  /**
   * @return The queue status of Structure \a s.
   */
  virtual QueueInterface::QueueStatus getStatus(
    Structure* s) const override = 0;

  /**
   * Perform any work needed before calling Optimizer::update. This
   * function mainly exists for RemoteQueue classes to copy files
   * back from the server, but may be used for other purposes. It is
   * guaranteed to be called by Optimizer before updating.
   *
   * @param s The structure that is to be updated.
   *
   * @return True on success, false otherwise.
   */
  virtual bool prepareForStructureUpdate(Structure* s) const override;

  /**
   *  Runs a command (e.g., bash command or script) on a remote server.
   *
   * @note This is a wrapper function for both remote and local-remote jobs,
   *  and for remote jobs creates/discards ssh connection of its own.
   *
   * @param workdir Workind directory for the command
   * @param command The command
   * @param sout The stdout string
   * @param serr The stderr string
   * @param ercd The exit error code
   *
   * @return For a remote run, True if the command ran successfully and exit code is 0,
   *  False otherwise. For a local-remote run, always returns True.
   */
  virtual bool runACommand(const QString& workdir, const QString& command,
                           QString* sout, QString* serr, int* ercd) const override;

  /**
   * Copy a file from a remote source to a local destination.
   *
   * @note This is a wrapper function for both remote and local-remote jobs,
   *  and for remote jobs creates/discards ssh connection of its own.
   *
   * @param rem_file Full path to the file to be copied (a remote path)
   * @param loc_file Full path to the destination file (a local path)
   */
  virtual bool copyAFileRemoteToLocal(const QString& rem_file,
                                      const QString& loc_file) override;

  /**
   * Copy a file from a local source to a remote destination.
   *
   * @note This is a wrapper function for both remote and local-remote jobs,
   *  and for remote jobs creates/discards ssh connection of its own.
   *
   * @param loc_file Full path of the file to be copied (a local path)
   * @param rem_file Full path to the destination file (a remote path)
   */
  virtual bool copyAFileLocalToRemote(const QString& loc_file,
                                      const QString& rem_file) override;

  /**
   * Check if the file \a filename exists in the working directory
   * of Structure \a s and store the result in \a exists.
   *
   * @note This function uses the argument \a exists to report
   * whether or not the file exists. The return value indicates
   * whether the file check was performed without errors
   * (e.g. network errors).
   *
   * @note This is a wrapper function for both remote and local-remote jobs,
   *  and for remote jobs creates/discards ssh connection of its own.
   *
   * @return True if the test encountered no errors, false otherwise.
   */
  virtual bool checkIfFileExists(Structure* s, const QString& filename,
                                 bool* exists) override;
  /**
   * Retrieve the contents of the file \a filename for Structure \a
   * s as a QString \a contents.
   * @note This is a wrapper function for both remote and local-remote jobs,
   *  and for remote jobs creates/discards ssh connection of its own.
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
   * @note On local queue interface, grep is not actually used and
   * the exit code behavior is emulated.
   *
   * @note This is a wrapper function for both remote and local-remote jobs,
   *  and for remote jobs creates/discards ssh connection of its own.
   */
  virtual bool grepFile(Structure* s, const QString& matchText,
                        const QString& filename, QStringList* matches = 0,
                        int* exitcode = 0,
                        const bool caseSensitive = true) const override;

protected:
  /**
   * Create a working directory for \a structure on the remote
   * cluster.
   * @note This is a wrapper function for both remote and local-remote jobs;
   *  for remote jobs, if an existing ssh connection is passed to the function
   *  it will be used; otherwise the function will create/discard one.
   *
   * @param structure Structure of interest
   * @param ssh An initialized SSHConnection to use.
   *
   * @sa Structure::getRempath, Structure::setRempath
   *
   * @return True on success, false otherwise
   */
  bool createRemoteDirectory(Structure* structure, SSHConnection* ssh = nullptr) const;

  /**
   * Remove all files from \a structure's remote working directory.
   * @note This is a wrapper function for both remote and local-remote jobs;
   *  for remote jobs, if an existing ssh connection is passed to the function
   *  it will be used; otherwise the function will create/discard one.
   *
   * @param structure Structure of interest
   * @param ssh An initialized SSHConnection
   *
   * @return True on success, false otherwise
   */
  bool cleanRemoteDirectory(Structure* structure, SSHConnection* ssh = nullptr) const;

  /**
   * Copy all files from \a structure's remote working directory to the local
   * working directory.
   * @note This is a wrapper function for both remote and local-remote jobs,
   *  and for remote jobs creates/discards ssh connection of its own.
   *
   * @param structure Structure of interest
   * @param ssh An initialized SSHConnection to use.
   *
   * @return True on success, false otherwise
   */
  bool copyRemoteFilesToLocalCache(Structure* structure) const;

  /**
   * Saves a copy of the error directory that caused this structure to fail.
   * Saves it in <remote_path>/errorDirs
   * @note This is a wrapper function for both remote and local-remote jobs,
   *  and for remote jobs creates/discards ssh connection of its own.
   *
   * @param structure Structure of interest
   * @param ssh An initialized SSHConnection to use.
   *
   * @return True on success, false otherwise
   */
  bool logErrorDirectory(Structure* structure) const;

  // Submit command. For example, on slurm, this may be 'sbatch'.
  QString m_submitCommand;

  // Cancel command. For example, on slurm, this may be 'scancel'.
  QString m_cancelCommand;

  // Status command. For example, on slurm, this may be 'squeue'.
  QString m_statusCommand;
};
}

#endif // ENABLE_SSH
#endif // REMOTEQUEUEINTERFACE_H

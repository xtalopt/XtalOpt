/**********************************************************************
  LocalQueueInterface - Interface for running jobs locally.

  Copyright (C) 2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef LOCALQUEUEINTERFACE_H
#define LOCALQUEUEINTERFACE_H

#include <globalsearch/macros.h>
#include <globalsearch/queueinterface.h>

#include <QProcess>

namespace GlobalSearch {
class LocalQueueInterfaceConfigDialog;

/// @cond
// Since QProcess doesn't support polling at all (e.g. the only way
// to track its progress is to monitor signals), this class provides
// a basic polling wrapper.
class LocalQueueProcess : public QProcess
{
  Q_OBJECT
public:
  enum Status
  {
    NotStarted = 0,
    Running,
    Finished,
    Error
  };
  LocalQueueProcess(QObject* parent) : QProcess(parent), m_status(NotStarted)
  {
    connect(this, SIGNAL(started()), this, SLOT(setRunning()));
    connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), this,
            SLOT(setFinished()));
    connect(this, SIGNAL(error(QProcess::ProcessError)), this,
            SLOT(setFinished()));
  }
public slots:
  void setRunning() { m_status = Running; };
  void setFinished() { m_status = Finished; };
  Status status() { return m_status; };
private:
  Status m_status;
};
/// @endcond

/**
 * @class LocalQueueInterface local.h <globalsearch/local.h>
 *
 * @brief Interface for running jobs locally.
 *
 * @author David C. Lonie
 */
class LocalQueueInterface : public QueueInterface
{
  Q_OBJECT

public:
  friend class LocalQueueInterfaceConfigDialog;

  /**
   * Constructor
   *
   * @param parent OptBase parent
   * @param settingFile Filename from which to initialize settings.
   */
  explicit LocalQueueInterface(OptBase* parent,
                               const QString& settingFile = "");

  /**
   * Destructor
   */
  virtual ~LocalQueueInterface() override;

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
  virtual bool isReadyToSearch(QString* err) override;

public slots:

  /**
   * Write the input files in the hash \a files to the appropriate
   * location for Structure \a s.
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
   * Saves a copy of the error directory that caused this structure to fail.
   * Saves it in <local_path>/errorDirs
   *
   * @param structure Structure of interest
   *
   * @return True on success, false otherwise
   */
  bool logErrorDirectory(Structure* s) const;

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
   *  Runs a command (e.g., bash command or script) in a local path.
   *
   * @param workdir Workind directory for the command
   * @param command The command
   * @param sout The stdout string
   * @param serr The stderr string
   * @param ercd The exit error code
   *
   * @return For a local run, always returns True.
   */
  virtual bool runACommand(const QString& workdir, const QString& command,
                           QString* sout, QString* serr, int* ercd) const override;

  /**
   * This is a dummy function! If called while the run is 'local', it does nothing!
   *
   * @param rem_file Full path to the file to be copied (a remote path)
   * @param loc_file Full path to the destination file (a local path)
   */
  virtual bool copyAFileRemoteToLocal(const QString& rem_file,
                                      const QString& loc_file) override;

  /**
   * This is a dummy function! If called while the run is 'local', it does nothing!
   *
   * @param loc_file Full path to the file to be copied (a local path)
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
   * @return True if the test encountered no errors, false otherwise.
   */
  virtual bool checkIfFileExists(Structure* s, const QString& filename,
                                 bool* exists) override;

  /**
   * Retrieve the contents of the file \a filename for Structure \a
   * s as a QString \a contents.
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
   */
  virtual bool grepFile(Structure* s, const QString& matchText,
                        const QString& filename, QStringList* matches = 0,
                        int* exitcode = 0,
                        const bool caseSensitive = true) const override;

  /**
   * @return The configuration dialog for this QueueInterface, if it
   * exists, otherwise 0.
   * @sa hasDialog()
   * @ingroup dialog
   */
  virtual QDialog* dialog() override;

protected:
  /// Look up hash for mapping jobID's to processes.
  /// Key: PID, Value: QProcess handle
  QHash<unsigned long, LocalQueueProcess*> m_processes;
};
}

#endif // LOCALQUEUEINTERFACE_H

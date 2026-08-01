/**********************************************************************
  DirectRunInterface - Interface for running jobs directly without a scheduler.

  Copyright (C) 2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef DIRECTRUNINTERFACE_H
#define DIRECTRUNINTERFACE_H

#include <common/compatibility/qt_compat.h>
#include <search/constants.h>
#include <search/queueinterface.h>

#include <QProcess>
#include <QSharedPointer>

namespace Search {

/// @cond
// QProcess class used to check a process without waiting.
class DirectRunProcess;

// Host for direct-run processes. Processes are started on the queue thread.
class DirectRunProcessHost : public QObject
{
  Q_OBJECT
public slots:
  // Start a process. Return zero if it cannot be started.
  Search::DirectRunProcess* startProcess(const QString& command, const QString& workDir,
                                         const QString& stdinFile, const QString& stdoutFile,
                                         const QString& stderrFile, int startTimeoutMs);
  void flushDeferredDeletes();
};

class DirectRunProcess : public QProcess
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
  DirectRunProcess(QObject* parent) : QProcess(parent), m_status(NotStarted)
  {
    connect(this, &QProcess::started, this, &DirectRunProcess::setRunning);
    connect(this, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &DirectRunProcess::setFinished);
    QtCompat::connectProcessError(this, this, &DirectRunProcess::setError);
  }
public slots:
  void setRunning() { m_status = Running; };
  // A crashed process is an error; an exit code is useful only after a normal exit.
  void setFinished(int /*exitCode*/, QProcess::ExitStatus exitStatus)
  {
    m_status = (exitStatus == QProcess::NormalExit) ? Finished : Error;
  };
  void setError()
  {
    // Keep a normal finish. Some errors arrive late.
    if (m_status != Finished)
      m_status = Error;
  };
  Status status() { return m_status; };
  int processStateValue() const { return static_cast<int>(state()); };
  int statusValue() const { return static_cast<int>(m_status); };
  int exitCodeValue() const { return exitCode(); };
  int processErrorValue() const { return static_cast<int>(error()); };
  QString processErrorStringValue() const { return errorString(); };
  QString readAllStandardOutputString() const
  {
    return QString(const_cast<DirectRunProcess*>(this)->readAllStandardOutput());
  };
  QString readAllStandardErrorString() const
  {
    return QString(const_cast<DirectRunProcess*>(this)->readAllStandardError());
  };
  void killProcess() { kill(); };
  int killAndWait()
  {
    kill();
    waitForFinished(PROCESS_KILL_TIMEOUT);
    return state() == QProcess::NotRunning;
  };
  void terminateAndKill()
  {
    terminate();
    if (!waitForFinished(PROCESS_TERMINATE_TIMEOUT)) {
      kill();
      waitForFinished(PROCESS_KILL_TIMEOUT);
    }
  };
private:
  Status m_status;
};
/// @endcond

/**
 * @class DirectRunInterface directrun.h <search/queueinterfaces/directrun.h>
 *
 * @brief Interface for running jobs directly without a scheduler.
 *
 * @author David C. Lonie
 */
class DirectRunInterface : public QueueInterface
{
  Q_OBJECT

public:
  // Default values.
  static const QueueDefaults& defaults();

  /**
   * Constructor
   *
   * @param parent SearchBase parent
   * @param settingFile Filename from which to initialize settings.
   */
  explicit DirectRunInterface(SearchBase* parent, const QString& settingFile = "");

  /**
   * Destructor
   */
  virtual ~DirectRunInterface() override;

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
   * function mainly exists for batch queue classes to copy files
   * back from the execution host, but may be used for other purposes. It is
   * guaranteed to be called by Optimizer before updating.
   *
   * @param s The structure that is to be updated.
   *
   * @return True on success, false otherwise.
   */
  virtual bool prepareForStructureUpdate(Structure* s) const override;

  void prepareForThreadStop() override;

  /**
   *  Runs a command (e.g., bash command or script) in a local path.
   *
   * @param workdir Working directory for the command
   * @param command The command
   * @return Command launch, exit code, stdout, and stderr.
   */
  virtual CommandResult runACommand(const QString& workdir, const QString& command, int timeoutMs = -1) const override;

private:
  // Running processes. Do not hold this lock while calling a process.
  mutable QHash<uint, QSharedPointer<DirectRunProcess>> m_processes;
  mutable QMutex m_processesMutex;

  // Starts the processes on the queue thread.
  DirectRunProcessHost* m_processHost;
};
}

Q_DECLARE_METATYPE(Search::DirectRunProcess*)

#endif // DIRECTRUNINTERFACE_H

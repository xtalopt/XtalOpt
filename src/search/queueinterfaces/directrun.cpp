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

#include <search/queueinterfaces/directrun.h>
#include <common/fileutils.h>
#include <common/output.h>
#include <common/timing.h>

#include <common/compatibility/platform_defs.h>
#include <common/compatibility/qt_compat.h>
#include <search/optimizer.h>
#include <search/search.h>
#include <search/structure.h>

#include <QDir>
#include <QHash>
#include <QMetaObject>
#include <QProcess>
#include <QString>
#include <QThread>

#include <limits>

namespace Search {

namespace {

Qt::ConnectionType directRunProcessConnectionType(const DirectRunProcess* proc)
{
  if (!proc)
    return Qt::DirectConnection;

  QThread* ownerThread = proc->thread();
  if (!ownerThread || ownerThread == QThread::currentThread() || !ownerThread->isRunning()) {
    return Qt::DirectConnection;
  }

  // Start the process on its thread.
  return Qt::BlockingQueuedConnection;
}

int directRunProcessInt(DirectRunProcess* proc, const char* method, int fallback)
{
  if (!proc)
    return fallback;

  int value = fallback;
  if (!QMetaObject::invokeMethod(proc, method, directRunProcessConnectionType(proc),
                                 Q_RETURN_ARG(int, value))) {
    return fallback;
  }
  return value;
}

QString directRunProcessString(DirectRunProcess* proc, const char* method)
{
  if (!proc)
    return QString();

  QString value;
  if (!QMetaObject::invokeMethod(proc, method, directRunProcessConnectionType(proc),
                                 Q_RETURN_ARG(QString, value))) {
    return QString();
  }
  return value;
}

void directRunProcessCall(DirectRunProcess* proc, const char* method)
{
  if (!proc)
    return;

  QMetaObject::invokeMethod(proc, method, directRunProcessConnectionType(proc));
}

void directRunProcessCallInt(DirectRunProcess* proc, const char* method, int arg)
{
  if (!proc)
    return;

  QMetaObject::invokeMethod(proc, method, directRunProcessConnectionType(proc), Q_ARG(int, arg));
}

void deleteDirectRunProcess(DirectRunProcess* proc)
{
  if (!proc)
    return;

  QThread* ownerThread = proc->thread();
  if (!ownerThread || ownerThread == QThread::currentThread() || !ownerThread->isRunning()) {
    delete proc;
    return;
  }

  QMetaObject::invokeMethod(proc, "deleteLater", Qt::QueuedConnection);
}

}

const QueueDefaults& DirectRunInterface::defaults()
{
  static const QueueDefaults s{ "none", "", "", "", "" };
  return s;
}

DirectRunInterface::DirectRunInterface(SearchBase* parent,
                                       const QString& /*settingFile*/)
  : QueueInterface(parent)
{
  // Needed for DirectRunProcess:
  qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
  qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");
  qRegisterMetaType<Search::DirectRunProcess*>("Search::DirectRunProcess*");

  // Create the process host on the queue thread.
  m_processHost = new DirectRunProcessHost;
  m_processHost->moveToThread(parent->queueThread());

  m_queueDefaults = &defaults();
}

DirectRunInterface::~DirectRunInterface()
{
  prepareForThreadStop();
}

void DirectRunInterface::prepareForThreadStop()
{
  QList<QSharedPointer<DirectRunProcess>> processes;
  {
    QtCompat::MutexLocker locker(&m_processesMutex);
    processes = m_processes.values();
    m_processes.clear();
  }

  for (const auto& proc : processes) {
    if (directRunProcessInt(proc.data(), "processStateValue",
                            static_cast<int>(QProcess::NotRunning)) ==
        static_cast<int>(QProcess::Running)) {
      // terminate, wait, then kill (see terminateAndKill).
      directRunProcessCall(proc.data(), "terminateAndKill");
    }
  }

  // Check the process thread.
  if (!m_processHost)
    return;

  QThread* hostThread = m_processHost->thread();
  if (!hostThread || hostThread == QThread::currentThread() || !hostThread->isRunning()) {
    delete m_processHost;
  } else {
    QMetaObject::invokeMethod(m_processHost, "deleteLater", Qt::QueuedConnection);
  }
  m_processHost = nullptr;
}

DirectRunProcess* DirectRunProcessHost::startProcess(
  const QString& command, const QString& workDir, const QString& stdinFile,
  const QString& stdoutFile, const QString& stderrFile, int startTimeoutMs)
{
  DirectRunProcess* proc = new DirectRunProcess(nullptr);
  proc->setWorkingDirectory(workDir);
  if (!stdinFile.isEmpty())
    proc->setStandardInputFile(stdinFile);
  if (!stdoutFile.isEmpty())
    proc->setStandardOutputFile(stdoutFile);
  if (!stderrFile.isEmpty())
    proc->setStandardErrorFile(stderrFile);

  QtCompat::processStartCommand(*proc, command);

  if (!proc->waitForStarted(startTimeoutMs)) {
    Common::error(QObject::tr("%1: failed to start direct run in %2: %3")
                    .arg(__func__)
                    .arg(workDir)
                    .arg(proc->errorString()));
    delete proc;
    return nullptr;
  }

  const qint64 processId = proc->processId();
  if (processId <= 0 ||
      static_cast<quint64>(processId) > std::numeric_limits<uint>::max()) {
    Common::error(QObject::tr("%1: failed to obtain process ID for the "
                              "direct run in %2.")
                    .arg(__func__)
                    .arg(workDir));
    proc->kill();
    proc->waitForFinished(PROCESS_KILL_TIMEOUT);
    delete proc;
    return nullptr;
  }

  return proc;
}

bool DirectRunInterface::isReadyToSearch(QString* str)
{
  if (!localWorkingDirectoryReady(str))
    return false;

  if (str)
    str->clear();
  return true;
}

bool DirectRunInterface::writeFiles(
  Structure* s, const QHash<QString, QString>& fileHash) const
{
  if (!writeHashToLocalDir(s, fileHash))
    return false;

  return writeCopyFilesToLocalDir(s);
}

// Resolve a direct-run stdio filename to a full local path (empty stays empty).
static QString directRunFilePath(Structure* s, const QString& name)
{
  return name.isEmpty() ? QString() : Common::localPath(s->getLocpath(), name);
}

bool DirectRunInterface::startJob(Structure* s)
{
  Common::ScopedTimer _timer("DirectRunInterface::startJob");
  if (s->getJobID() != 0) {
    Common::error(tr("%1: attempting to start job for structure %2, "
                    "but a JobID is already set (%3).")
                     .arg(__func__)
                     .arg(s->getTag())
                     .arg(s->getJobID()));
    return false;
  }
  // Use the optimizer command as-is; it may be a PATH name, an absolute
  // path, or a user-provided command line.
  QString command = getCurrentOptimizer(s)->getDirectRunCommand();

#if GS_WINDOWS
  command = "cmd.exe /S /C " + command;
#endif // GS_WINDOWS

  const Optimizer* optimizer = getCurrentOptimizer(s);

  // Start the process.
  DirectRunProcess* proc = nullptr;
  const Qt::ConnectionType invokeType = (m_processHost->thread() == QThread::currentThread())
      ? Qt::DirectConnection
      : Qt::BlockingQueuedConnection;
  QMetaObject::invokeMethod(m_processHost, "startProcess", invokeType,
    Q_RETURN_ARG(Search::DirectRunProcess*, proc), Q_ARG(QString, command),
    Q_ARG(QString, s->getLocpath()),
    Q_ARG(QString, directRunFilePath(s, optimizer->stdinFilename())),
    Q_ARG(QString, directRunFilePath(s, optimizer->stdoutFilename())),
    Q_ARG(QString, directRunFilePath(s, optimizer->stderrFilename())),
    Q_ARG(int, PROCESS_START_TIMEOUT));

  if (!proc) {
    Common::error(tr("%1: direct run for structure %2 did not start.")
                    .arg(__func__)
                    .arg(s->getTag()));
    return false;
  }

  const uint pid = static_cast<uint>(proc->processId());

  {
    QWriteLocker locker(&s->lock());
    s->startOptTimer();
    s->setJobID(pid);
  }

  {
    QtCompat::MutexLocker locker(&m_processesMutex);
    m_processes.insert(pid, QSharedPointer<DirectRunProcess>(proc, deleteDirectRunProcess));
  }

  return true;
}

bool DirectRunInterface::logErrorDirectory(Structure* s) const
{
  QString strdir_s = s->getDirectoryTag();
  const QString errorDir = Common::localPath(m_search->getLocWorkDir(), "errorDirs");
  const QString structureErrorDir = Common::localPath(errorDir, strdir_s);

  if (!QDir().mkpath(structureErrorDir)) {
    Common::warning(tr("Could not create error directory %1")
                      .arg(structureErrorDir));
    return false;
  }

  if (!Common::copyDir(s->getLocpath(), structureErrorDir)) {
    Common::warning(tr("Could not copy error directory for structure %1 to %2")
                      .arg(s->getTag())
                      .arg(structureErrorDir));
    return false;
  }

  return true;
}

bool DirectRunInterface::stopJob(Structure* s)
{
  // Copy the structure values.
  uint pid = 0;
  bool logErrors = false;
  {
    QWriteLocker wLocker(&s->lock());
    pid = s->getJobID();
    logErrors = this->m_search->logErrorDirs() && s->isQueueErrorRecoveryState();
  }

  if (logErrors) {
    logErrorDirectory(s);
  }

  if (pid == 0) {
    // Job is not running.
    return true;
  }

  QSharedPointer<DirectRunProcess> proc;
  {
    QtCompat::MutexLocker locker(&m_processesMutex);
    proc = m_processes.take(pid);
  }
  if (!proc) {
    // No process associated with this JobID.
    return true;
  }

  // Stop the process.
  if (directRunProcessInt(proc.data(), "processStateValue",
                          static_cast<int>(QProcess::NotRunning)) ==
      static_cast<int>(QProcess::Running)) {
    directRunProcessCallInt(proc.data(), "killAndWait", PROCESS_KILL_TIMEOUT);
  }

  {
    QWriteLocker wLocker(&s->lock());
    s->setJobID(0);
    s->stopOptTimer();
  }
  return true;
}

QueueInterface::QueueStatus DirectRunInterface::getStatus(Structure* s) const
{
  // Copy the structure values.
  uint pid = 0;
  Structure::State state = Structure::Empty;
  {
    QReadLocker rlocker(&s->lock());
    pid = s->getJobID();
    state = s->getStatus();
  }

  if (!pid && state != Structure::Submitted) {
    return QueueInterface::Error;
  }

  // Copy the shared pointer out under the mutex to keep the process alive; the
  // mutex must not be held across the calls below (see the note in
  // directrun.h).
  QSharedPointer<DirectRunProcess> proc;
  {
    QtCompat::MutexLocker locker(&m_processesMutex);
    proc = m_processes.value(pid);
  }

  // For a submitted structure with no process in the table, no output file
  // means still pending; otherwise the job finished before we polled, so fall
  // through to Started.
  if (state == Structure::Submitted) {
    if (pid == 0 || !proc) {
      bool exists = false;
      if (!getCurrentOptimizer(s)->checkIfOutputFileExists(s, &exists))
        return QueueInterface::CommunicationError;
      if (!exists) {
        return QueueInterface::Pending;
      }
    }
    return QueueInterface::Started;
  }

  if (!proc) {
    return QueueInterface::Error;
  }

  int statusValue = directRunProcessInt(proc.data(), "statusValue",
                        static_cast<int>(DirectRunProcess::Error));
  int exitCode = 0;
  int processError = 0;
  QString processErrorString;
  QString stdoutText;
  QString stderrText;
  if (statusValue == static_cast<int>(DirectRunProcess::Finished)) {
    exitCode = directRunProcessInt(proc.data(), "exitCodeValue", -1);
  }
  // Collect details for both kinds of failure: a crash, or a non-zero exit.
  if (statusValue == static_cast<int>(DirectRunProcess::Error) ||
      (statusValue == static_cast<int>(DirectRunProcess::Finished) && exitCode != 0)) {
    processError = directRunProcessInt(proc.data(), "processErrorValue", 0);
    processErrorString = directRunProcessString(proc.data(), "processErrorStringValue");
    stdoutText = directRunProcessString(proc.data(), "readAllStandardOutputString");
    stderrText = directRunProcessString(proc.data(), "readAllStandardErrorString");
  }

  switch (static_cast<DirectRunProcess::Status>(statusValue)) {
    case DirectRunProcess::NotStarted:
      return QueueInterface::Pending;
    case DirectRunProcess::Running:
      return QueueInterface::Running;
    case DirectRunProcess::Finished:
      if (exitCode != 0) {
        Common::warning(tr("%1: structure %2, PID=%3 failed. QProcess error "
                          "code: %4. Process exit code: %5 errStr: %6\n"
                          "stdout:\n%7\nstderr:\n%8")
                         .arg(__func__)
                         .arg(s->getTag())
                         .arg(pid)
                         .arg(processError)
                         .arg(exitCode)
                         .arg(processErrorString)
                         .arg(stdoutText)
                         .arg(stderrText));
        return QueueInterface::Error;
      }
      {
        bool success = false;
        if (!getCurrentOptimizer(s)->checkForSuccessfulOutput(s, &success))
          return QueueInterface::CommunicationError;
        return success ? QueueInterface::Success : QueueInterface::Error;
      }
    case DirectRunProcess::Error:
      // The process crashed or failed to run; exitCode() means nothing here.
      Common::warning(tr("%1: structure %2, PID=%3 crashed or failed to "
                        "run. QProcess error code: %4. errStr: %5\n"
                        "stdout:\n%6\nstderr:\n%7")
                       .arg(__func__)
                       .arg(s->getTag())
                       .arg(pid)
                       .arg(processError)
                       .arg(processErrorString)
                       .arg(stdoutText)
                       .arg(stderrText));
      return QueueInterface::Error;
    default:
      // Shouldn't reach this point...
      return QueueInterface::Unknown;
  }
}

bool DirectRunInterface::prepareForStructureUpdate(Structure* /*s*/) const
{
  // Nothing to do!
  return true;
}

QueueInterface::CommandResult DirectRunInterface::runACommand(
  const QString& workdir, const QString& command) const
{
  // QProcess stderr is unreliable for direct runs (e.g. srun writes to stderr
  // on success), so don't fail on stderr or exit code; just warn on either.
  CommandResult result;

  QProcess proc;
  if (!workdir.isEmpty()) {
    proc.setWorkingDirectory(workdir);
  }
  QtCompat::processStartCommand(proc, command);
  if (!proc.waitForStarted(PROCESS_START_TIMEOUT)) {
    result.stderrText = proc.errorString();
    Common::warning(tr("Direct command %1 at %2 failed to start: %3")
        .arg(command).arg(workdir).arg(result.stderrText));
    return result;
  }
  proc.waitForFinished(-1);

  result.launched = true;
  result.stdoutText = QString(proc.readAllStandardOutput());
  result.stderrText = QString(proc.readAllStandardError());
  result.exitCode = proc.exitCode();

  if (!result.stderrText.isEmpty() || (result.exitCode != 0)) {
    Common::warning(tr("Direct command %1 at %2 exited with code %3 and error: %4")
        .arg(command).arg(workdir).arg(result.exitCode).arg(result.stderrText));
  }

  return result;
}

}

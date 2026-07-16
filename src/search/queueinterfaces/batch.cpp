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

#include <search/queueinterfaces/batch.h>
#include <common/fileutils.h>
#include <common/output.h>

#include <search/structure.h>
#include <search/optimizer.h>
#include <search/search.h>

#include <search/ssh/sshconnection.h>
#include <search/ssh/sshmanager.h>

#include <common/compatibility/qt_compat.h>
#include <QDir>
#include <QFile>
#include <QReadLocker>
#include <QSet>
#include <QTextStream>
#include <QProcess>
#include <QWriteLocker>
#include <QString>

namespace Search {
namespace {

QString shellSingleQuote(const QString& text)
{
  QString quoted = text;
  quoted.replace("'", "'\\''");
  return "'" + quoted + "'";
}

SSHConnection* remoteConnection(SearchBase* search)
{
  if (!search->ssh()) {
    Common::error(QObject::tr("Remote queue transport was requested before "
                              "SSH was initialized."));
    return nullptr;
  }

  SSHConnection* ssh = search->ssh()->getFreeConnection();
  if (!ssh)
    Common::error(QObject::tr("Cannot connect to ssh server."));
  return ssh;
}

class SSHConnectionLocker
{
public:
  explicit SSHConnectionLocker(SearchBase* search, SSHConnection* existing = nullptr)
    : m_search(search), m_connection(existing), m_owns(existing == nullptr)
  {
    if (m_owns)
      m_connection = remoteConnection(search);
  }

  ~SSHConnectionLocker()
  {
    if (m_owns && m_connection)
      m_search->ssh()->unlockConnection(m_connection);
  }

  SSHConnection* connection() const { return m_connection; }

private:
  SearchBase* m_search;
  SSHConnection* m_connection;
  bool m_owns;
};

} // namespace

BatchQueueInterface::BatchQueueInterface(SearchBase* parent,
                                           const QString& /*settingFile*/)
  : QueueInterface(parent),
    m_queueMutex(QReadWriteLock::Recursive)
{
}

BatchQueueInterface::~BatchQueueInterface()
{
}

bool BatchQueueInterface::isReadyToSearch(QString* str)
{
  if (!localWorkingDirectoryReady(str))
    return false;

  if (cancelCommand().isEmpty()) {
    if (str) {
      *str = tr("%1 command is not set. Check your Queue configuration.")
               .arg("cancel");
    }
    return false;
  }

  if (statusCommand().isEmpty()) {
    if (str) {
      *str = tr("%1 command is not set. Check your Queue configuration.")
               .arg("status");
    }
    return false;
  }

  if (submitCommand().isEmpty()) {
    if (str) {
      *str = tr("%1 command is not set. Check your Queue configuration.")
               .arg("submit");
    }
    return false;
  }

  if (!remoteQueueConfigurationReady(str))
    return false;

  if (str)
    str->clear();
  return true;
}





QString BatchQueueInterface::submitScriptName() const
{
  const QStringList templates = getQueueInterfaceTemplateFileNames();
  return templates.isEmpty() ? QString() : templates.first();
}

bool BatchQueueInterface::startJob(Structure* s)
{
  // Copy the structure values.
  QString workdir;
  {
    QReadLocker locker(&s->lock());
    workdir = m_search->isRemoteQueue() ? s->getRempath() : s->getLocpath();
  }

  const QString command = submitCommand() + " " + submitScriptName();
  const CommandResult result = runACommand(workdir, command);
  if (!result.succeeded())
    return false;

  bool ok = false;
  const unsigned int jobId = parseJobId(result.stdoutText, &ok);
  if (!ok || jobId == 0) {
    Common::error(tr("Could not retrieve jobID for structure %1.")
                    .arg(s->getTag()));
    return false;
  }

  {
    QWriteLocker locker(&s->lock());
    s->setJobID(jobId);
    s->startOptTimer();
  }
  queueList(true);
  return true;
}

bool BatchQueueInterface::stopJob(Structure* s)
{
  // Copy the structure values.
  unsigned long jobId = 0;
  bool logErrors = false;
  {
    QWriteLocker locker(&s->lock());
    jobId = s->getJobID();
    logErrors = m_search->logErrorDirs() && s->isQueueErrorRecoveryState();
  }

  if (logErrors) {
    logErrorDirectory(s);
  }

  if (jobId == 0) {
    if (m_search->cleanRemoteOnStop())
      cleanExecutionDirectory(s);
    return true;
  }

  const QString command = cancelCommand() + " " + QString::number(jobId);
  const CommandResult result = runACommand("", command);

  {
    QWriteLocker locker(&s->lock());
    s->setJobID(0);
    s->stopOptTimer();
  }
  return result.succeeded();
}

QueueInterface::QueueStatus BatchQueueInterface::getStatus(Structure* s) const
{
  // Copy the structure values.
  unsigned int jobId = 0;
  Structure::State state = Structure::Empty;
  {
    QReadLocker locker(&s->lock());
    jobId = static_cast<unsigned int>(s->getJobID());
    state = s->getStatus();
  }

  const QStringList queueData = queueList();

  if (queueData.size() == 1 && queueData[0].compare("CommError") == 0)
    return QueueInterface::CommunicationError;

  if (!jobId && state != Structure::Submitted)
    return QueueInterface::Error;

  QString rawStatus;
  const QueueInterface::QueueStatus queueStatus = parseQueueStatus(queueData, jobId, &rawStatus);

  if (state == Structure::Submitted) {
    if (queueStatus == QueueInterface::Unknown && rawStatus.isEmpty()) {
      bool exists;
      if (!getCurrentOptimizer(s)->checkIfOutputFileExists(s, &exists))
        return QueueInterface::CommunicationError;
      return exists ? QueueInterface::Started : QueueInterface::Pending;
    }
    return QueueInterface::Started;
  }

  if (queueStatus == QueueInterface::Unknown && rawStatus.isEmpty())
    return statusFromMissingQueueEntry(s);

  if (queueStatus != QueueInterface::Unknown)
    return queueStatus;

  Common::warning(tr("Structure %1 with jobID %2 has unrecognized status: %3")
                 .arg(s->getTag())
                 .arg(jobId)
                 .arg(rawStatus));
  return QueueInterface::Unknown;
}

QueueInterface::QueueStatus BatchQueueInterface::statusFromMissingQueueEntry(Structure* s) const
{
  bool outputFileExists;
  if (!getCurrentOptimizer(s)->checkIfOutputFileExists(s, &outputFileExists))
    return QueueInterface::CommunicationError;

  if (outputFileExists) {
    bool success;
    if (!getCurrentOptimizer(s)->checkForSuccessfulOutput(s, &success))
      return QueueInterface::CommunicationError;
    return success ? QueueInterface::Success : QueueInterface::Error;
  }

  return missingFromQueueWithoutOutputStatus(s);
}

QueueInterface::QueueStatus
BatchQueueInterface::missingFromQueueWithoutOutputStatus(Structure* s) const
{
  Common::warning(tr("Structure %1 with jobID %2 is missing from the queue "
                     "and has not written any output.")
                 .arg(s->getTag())
                 .arg(s->getJobID()));
  return QueueInterface::Error;
}

QString BatchQueueInterface::queueListCommand() const
{
  const QString username = m_search->getUsername().trimmed();
  if (username.isEmpty())
    return statusCommand();

  return statusCommand() + " -u " + username;
}

bool BatchQueueInterface::queueListCommandSucceeded(
  bool ok, int exitCode, const QString& stdoutText) const
{
  Q_UNUSED(stdoutText);
  return ok && (exitCode == 0 || exitCode == 1);
}

QStringList BatchQueueInterface::queueList(bool forced) const
{
  QDateTime oldTimeStamp;
  {
    QReadLocker readLocker(&m_queueMutex);
    if (!forced && m_queueTimeStamp.isValid() &&
        m_queueTimeStamp.msecsTo(QDateTime::currentDateTime()) <=
          1000 * m_search->queueRefreshInterval()) {
      return QStringList(m_queueData);
    }
    oldTimeStamp = m_queueTimeStamp;
  }

  QWriteLocker queueLocker(&m_queueMutex);
  if (m_queueTimeStamp != oldTimeStamp) {
    queueLocker.unlock();
    return queueList(false);
  }

  const QString command = queueListCommand();
  const CommandResult result = runACommand("", command);

  if (!queueListCommandSucceeded(result.launched, result.exitCode, result.stdoutText)) {
    Common::warning(tr("Could not execute %1: (%2) %3\n\t"
                       "Treating as a communication error.")
                    .arg(command)
                    .arg(QString::number(result.exitCode))
                    .arg(result.stderrText));
    // Report a queue communication error instead of keeping a saved queue data.
    m_queueData = QStringList(QStringLiteral("CommError"));
    m_queueTimeStamp = QDateTime::currentDateTime();
    return QStringList(m_queueData);
  }

  m_queueData = result.stdoutText.split("\n", QtCompat::SkipEmptyParts);
  m_queueTimeStamp = QDateTime::currentDateTime();
  return QStringList(m_queueData);
}

bool BatchQueueInterface::remoteQueueConfigurationReady(QString* err) const
{
  if (err)
    err->clear();

  if (!m_search->isRemoteQueue())
    return true;

  if (m_search->getHost().isEmpty()) {
    if (err) {
      *err = tr("Hostname of %1 server is not set. Check your Queue " "configuration.")
               .arg(getIDString());
    }
    return false;
  }

  if (m_search->getRemWorkDir().isEmpty()) {
    if (err) {
      *err = tr("Remote working directory is not set. Check your Queue " "configuration.");
    }
    return false;
  }

  if (m_search->getUsername().isEmpty()) {
    if (err) {
      *err = tr("SSH username for %1 server is not set. Check your Queue " "configuration.")
               .arg(getIDString());
    }
    return false;
  }

  if (m_search->getPort() < 1 || m_search->getPort() > 65535) {
    if (err) {
      *err = tr("SSH port is invalid (Port %1). Check your Queue " "configuration.")
               .arg(m_search->getPort());
    }
    return false;
  }

  return true;
}

bool BatchQueueInterface::writeFiles(
  Structure* s, const QHash<QString, QString>& fileHash) const
{
  if (!writeHashToLocalDir(s, fileHash))
    return false;

  // Add the files to copy.
  QStringList filenames = fileHash.keys();
  if (!writeCopyFilesToLocalDir(s, &filenames))
    return false;

  // Local batch jobs already use the local working directory.
  if (!m_search->isRemoteQueue())
    return true;

  SSHConnectionLocker connectionLocker(m_search);
  SSHConnection* ssh = connectionLocker.connection();
  if (ssh == nullptr)
    return false;

  if (!createExecutionDirectory(s, ssh) || !cleanExecutionDirectory(s, ssh)) {
    return false;
  }

  for (auto it = filenames.constBegin(), it_end = filenames.constEnd(); it != it_end; ++it) {
    if (!ssh->copyFileToServer(Common::localPath(s->getLocpath(), *it),
          Common::remotePath(s->getRempath(), *it))) {
      Common::error(tr("Could not copy \"%1\" to remote server (structure %2)")
          .arg(*it)
          .arg(s->getTag()));
      return false;
    }
  }
  return true;
}

bool BatchQueueInterface::prepareForStructureUpdate(Structure* s) const
{
  return copyExecutionFilesToLocalDirectory(s);
}

QueueInterface::CommandResult BatchQueueInterface::runACommand(
  const QString& workdir, const QString& command) const
{
  // Run a batch command.
  CommandResult result;

  // Local batch commands run directly in the local working directory.
  if (!m_search->isRemoteQueue()) {
    QProcess proc;
    if (!workdir.isEmpty()) {
      proc.setWorkingDirectory(workdir);
    }
    QtCompat::processStartCommand(proc, command);
    if (!proc.waitForStarted(-1)) {
      result.stderrText = proc.errorString();
      Common::warning(tr("Local batch command %1 at %2 failed to start: %3")
          .arg(command).arg(workdir).arg(result.stderrText));
      return result;
    }
    proc.waitForFinished(-1);

    result.launched = true;
    result.stdoutText = QString(proc.readAllStandardOutput());
    result.stderrText = QString(proc.readAllStandardError());
    result.exitCode = proc.exitCode();

    // Report the command output.
    if (!result.stderrText.isEmpty() || (result.exitCode != 0)) {
      Common::debug(tr("Local batch command %1 at %2 exited with code %3 and error: %4")
          .arg(command).arg(workdir).arg(result.exitCode).arg(result.stderrText));
    }

    return result;
  }
  SSHConnectionLocker connectionLocker(m_search);
  SSHConnection* ssh = connectionLocker.connection();
  if (ssh == nullptr)
    return result;

  QString runcom;
  if (!workdir.isEmpty()) {
    // Run the command in the working directory.
    runcom = "cd " + shellSingleQuote(workdir) + " && " + command;
  } else {
    runcom = command;
  }

  result.launched = ssh->execute(runcom, result.stdoutText, result.stderrText, result.exitCode);

  // Report the command output.
  if (!result.succeeded()) {
    Common::debug(tr("Remote command %1 at %2 exited with code %3.")
        .arg(command).arg(workdir).arg(result.exitCode));
  }

  return result;
}

bool BatchQueueInterface::copyFileFromExecutionHost(const QString& rem_file,
                                                    const QString& loc_file)
{
  if (!m_search->isRemoteQueue())
    return true;

  SSHConnectionLocker connectionLocker(m_search);
  SSHConnection* ssh = connectionLocker.connection();
  if (ssh == nullptr)
    return false;

  if (!ssh->copyFileFromServer(rem_file, loc_file)) {
    Common::error(tr("Failed copying '%1' from remote server to local '%2'")
        .arg(rem_file).arg(loc_file));
    return false;
  }

  return true;
}

bool BatchQueueInterface::copyFileToExecutionHost(const QString& loc_file, const QString& rem_file)
{
  if (!m_search->isRemoteQueue())
    return true;

  SSHConnectionLocker connectionLocker(m_search);
  SSHConnection* ssh = connectionLocker.connection();
  if (ssh == nullptr)
    return false;

  if (!ssh->copyFileToServer(loc_file, rem_file)) {
    Common::error(tr("Failed copying '%1' to remote server '%2'")
        .arg(loc_file).arg(rem_file));
    return false;
  }
  return true;
}

bool BatchQueueInterface::removeAFile(Structure *s, const QString& filename)
{
  if (!m_search->isRemoteQueue())
    return QueueInterface::removeAFile(s, filename);

  if (!safeRelativeFilename(filename))
    return false;

  SSHConnectionLocker connectionLocker(m_search);
  SSHConnection* ssh2 = connectionLocker.connection();
  if (ssh2 == nullptr)
    return false;

  if (!ssh2->removeRemoteFile(Common::remotePath(s->getRempath(), filename))) {
    Common::error(tr("Could not remove the remote file %1").arg(filename));
    return false;
  }

  return true;
}

bool BatchQueueInterface::checkIfFileExists(Structure* s,
                                             const QString& filename,
                                             bool* exists)
{
  if (!exists)
    return false;

  if (!m_search->isRemoteQueue())
    return QueueInterface::checkIfFileExists(s, filename, exists);

  if (!safeRelativeFilename(filename))
    return false;

  const QString searchPath = s->getRempath();
  QString needle;
  QStringList haystack;

  SSHConnectionLocker connectionLocker(m_search);
  SSHConnection* ssh = connectionLocker.connection();

  if (ssh == nullptr)
    return false;

  needle = Common::remotePath(s->getRempath(), filename);
  if (!ssh->readRemoteDirectoryContents(searchPath, haystack)) {
    Common::error(tr("Could not read directory %1 on %2@%3:%4")
                     .arg(searchPath)
                     .arg(ssh->getUser())
                     .arg(ssh->getHost())
                     .arg(ssh->getPort()));
    return false;
  }

  *exists = false;
  for (auto it = haystack.constBegin(), it_end = haystack.constEnd(); it != it_end; ++it) {
    if (it->compare(needle) == 0) {
      *exists = true;
      break;
    }
  }

  return true;
}

bool BatchQueueInterface::checkIfFilesExist(Structure* s, const QStringList& filenames,
                                             QHash<QString, bool>* exists)
{
  if (!exists)
    return false;

  if (!m_search->isRemoteQueue())
    return QueueInterface::checkIfFilesExist(s, filenames, exists);

  exists->clear();
  for (const auto& filename : filenames) {
    if (!safeRelativeFilename(filename))
      return false;
  }

  QStringList haystack;
  const QString searchPath = s->getRempath();
  SSHConnectionLocker connectionLocker(m_search);
  SSHConnection* ssh = connectionLocker.connection();
  if (ssh == nullptr)
    return false;

  if (!ssh->readRemoteDirectoryContents(searchPath, haystack)) {
    Common::error(tr("Could not read directory %1 on %2@%3:%4")
                     .arg(searchPath)
                     .arg(ssh->getUser())
                     .arg(ssh->getHost())
                     .arg(ssh->getPort()));
    return false;
  }

  QSet<QString> remoteEntries;
  for (const auto& entry : haystack)
    remoteEntries.insert(entry);

  for (const auto& filename : filenames) {
    exists->insert(filename, remoteEntries.contains(Common::remotePath(searchPath, filename)));
  }
  return true;
}

bool BatchQueueInterface::fetchFile(Structure* s, const QString& rel_filename,
                                     QString* contents) const
{
  if (!contents)
    return false;

  if (!m_search->isRemoteQueue())
    return QueueInterface::fetchFile(s, rel_filename, contents);

  if (!safeRelativeFilename(rel_filename))
    return false;

  SSHConnectionLocker connectionLocker(m_search);
  SSHConnection* ssh = connectionLocker.connection();

  if (ssh == nullptr)
    return false;

  if (!ssh->readRemoteFile(Common::remotePath(s->getRempath(), rel_filename), *contents)) {
    return false;
  }

  return true;
}

bool BatchQueueInterface::grepFile(Structure* s, const QString& matchText,
                                    const QString& filename,
                                    QStringList* matches, int* exitcode,
                                    const bool caseSensitive) const
{
  if (!m_search->isRemoteQueue()) {
    return QueueInterface::grepFile(s, matchText, filename, matches, exitcode, caseSensitive);
  }

  if (!safeRelativeFilename(filename))
    return false;

  // Find the text on the remote host.
  QString flags = "";

  if (!caseSensitive) {
    flags = "-i";
  }

  QString stdout_str;
  QString stderr_str;
  int ec;

  SSHConnectionLocker connectionLocker(m_search);
  SSHConnection* ssh = connectionLocker.connection();
  if (ssh == nullptr)
    return false;

  const QString command = QString("grep %1 -F -- %2 %3")
      .arg(flags)
      .arg(shellSingleQuote(matchText))
      .arg(shellSingleQuote(Common::remotePath(s->getRempath(), filename)));
  if (!ssh->execute(command, stdout_str, stderr_str, ec)) {
    return false;
  }

  if (exitcode) {
    *exitcode = ec;
  }

  if (matches) {
    *matches = stdout_str.split('\n', QtCompat::SkipEmptyParts);
  }

  return true;
}

bool BatchQueueInterface::createExecutionDirectory(Structure* structure, SSHConnection* ssh) const
{
  if (!m_search->isRemoteQueue())
    return true;

  // Get an SSH connection.
  SSHConnectionLocker connectionLocker(m_search, ssh);
  SSHConnection* ssh2 = connectionLocker.connection();
  if (ssh2 == nullptr)
    return false;

  const QString command = "mkdir -p " + shellSingleQuote(structure->getRempath());
  QString stdout_str, stderr_str;
  int ec;
  if (!ssh2->execute(command, stdout_str, stderr_str, ec) || ec != 0) {
    Common::error(tr("Could not execute %1: %2").arg(command).arg(stderr_str));
    return false;
  }

  return true;
}

bool BatchQueueInterface::cleanExecutionDirectory(Structure* structure, SSHConnection* ssh) const
{
  if (!m_search->isRemoteQueue())
    return true;

  // Get an SSH connection.
  SSHConnectionLocker connectionLocker(m_search, ssh);
  SSHConnection* ssh2 = connectionLocker.connection();
  if (ssh2 == nullptr)
    return false;

  // 2nd arg keeps the directory, only removes its contents.
  if (!ssh2->removeRemoteDirectory(structure->getRempath(), true)) {
    Common::error(tr("Could not clear remote directory %1").arg(structure->getRempath()));
    return false;
  }

  return true;
}

bool BatchQueueInterface::logErrorDirectory(Structure* structure) const
{
  QString strdir_s = structure->getDirectoryTag();
  const QString errorDir = Common::localPath(m_search->getLocWorkDir(), "errorDirs");

  // Local batch error directories are already local.
  if (!m_search->isRemoteQueue()) {
    QDir dir;
    if (!dir.mkpath(errorDir)) {
      Common::warning("Could not create error directory " + errorDir);
    }
    const QString path = Common::localPath(errorDir, strdir_s);
    return Common::copyDir(structure->getLocpath(), path);
  }
  SSHConnectionLocker connectionLocker(m_search);
  SSHConnection* ssh = connectionLocker.connection();
  if (ssh == nullptr)
    return false;

  const QString path = Common::localPath(errorDir, strdir_s);
  QDir dir;
  if (!dir.mkpath(path)) {
    Common::warning("Could not create error directory " + path);
  }

  if (!ssh->copyDirectoryFromServer(structure->getRempath(), path)) {
    Common::error("Cannot copy from remote directory for Structure " + structure->getTag());
    return false;
  }

  return true;
}

bool BatchQueueInterface::copyExecutionFilesToLocalDirectory(Structure* structure) const
{
  if (!m_search->isRemoteQueue())
    return true;

  SSHConnectionLocker connectionLocker(m_search);
  SSHConnection* ssh = connectionLocker.connection();
  if (ssh == nullptr)
    return false;

  if (!ssh->copyDirectoryFromServer(structure->getRempath(),
        structure->getLocpath())) {
    Common::error("Cannot copy from remote directory for Structure " + structure->getTag());
    return false;
  }

  return true;
}
}

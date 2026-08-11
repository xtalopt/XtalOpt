/**********************************************************************
  SSHConnection - Connection to an ssh server for execution, sftp, etc.

  Copyright (C) 2010-2012 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/constants.h>
#include <search/ssh/sshconnection_cli.h>

#include <common/fileutils.h>
#include <common/constants.h>
#include <common/output.h>
#include <search/ssh/sshmanager_cli.h>

#include <common/compatibility/qt_compat.h>
#include <QElapsedTimer>
#include <QProcess>
#include <QStandardPaths>

namespace Search {
namespace {

QString commandString(const QString& program, const QStringList& args)
{
  return program + " " + args.join(" ");
}

// Quote remote paths so they cannot become shell syntax, while keeping a
// leading "~/" outside the quotes so the remote shell still expands it.
QStringList quoteRemotePathArgs(const QStringList& args)
{
  QStringList quoted;
  for (const auto& arg : args)
    quoted << Common::quoteRemotePath(arg);
  return quoted;
}

struct ProcessResult
{
  bool started;
  bool finished;
  QProcess::ExitStatus exitStatus;
  int exitCode;
  QString stdoutText;
  QString stderrText;
  QString errorString;

  ProcessResult()
    : started(false), finished(false), exitStatus(QProcess::NormalExit),
      exitCode(-1)
  {
  }
};

ProcessResult runCliSshCommand(const QString& program, const QStringList& args,
                              int timeoutMs, const std::atomic<bool>* cancel = nullptr)
{
  ProcessResult result;
  QProcess proc;
  QElapsedTimer timer;
  timer.start();
  proc.start(program, args);

  if (!proc.waitForStarted(timeoutMs)) {
    result.errorString = proc.errorString();
    return result;
  }

  result.started = true;
  proc.closeWriteChannel();
  while (!result.finished) {
    if (cancel && cancel->load())
      break;
    const qint64 remaining = timeoutMs < 0 ? 100 : static_cast<qint64>(timeoutMs) - timer.elapsed();
    if (remaining <= 0)
      break;
    result.finished = proc.waitForFinished(static_cast<int>(qMin<qint64>(remaining, 100)));
  }
  if (!result.finished) {
    proc.kill();
    proc.waitForFinished(PROCESS_KILL_TIMEOUT);
  }

  result.exitStatus = proc.exitStatus();
  result.exitCode = proc.exitCode();
  result.stdoutText = QString(proc.readAllStandardOutput());
  result.stderrText = QString(proc.readAllStandardError());
  return result;
}

void assignProcessOutputs(const ProcessResult& result, QString* stdout_str, QString* stderr_str,
                          int* ec)
{
  if (stdout_str != nullptr)
    *stdout_str = result.stdoutText;
  if (stderr_str != nullptr)
    *stderr_str = result.stderrText;
  if (ec != nullptr)
    *ec = result.exitCode;
}

bool runPrecheckCommand(const QString& program, const QStringList& args, QString* error)
{
  const ProcessResult result = runCliSshCommand(program, args, CLISSH_PRECHECK_TIMEOUT);

  if (!result.started) {
    if (error) {
      *error = QString("Failed to start '%1': %2")
                 .arg(program)
                 .arg(result.errorString);
    }
    return false;
  }

  if (!result.finished) {
    if (error) {
      *error = QString("Command timed out after %1 seconds: %2")
                 .arg(CLISSH_PRECHECK_TIMEOUT / 1000)
                 .arg(commandString(program, args));
    }
    return false;
  }

  if (result.exitStatus != QProcess::NormalExit || result.exitCode != 0) {
    if (error) {
      *error = QString("Command failed with exit code %1: %2\nstdout:\n%3\nstderr:\n%4")
                 .arg(result.exitCode)
                 .arg(commandString(program, args))
                 .arg(result.stdoutText)
                 .arg(result.stderrText);
    }
    return false;
  }

  return true;
}

}

SSHConnectionCLI::SSHConnectionCLI(SSHManagerCLI* parent)
  : SSHConnection(parent)
{
}

SSHConnectionCLI::~SSHConnectionCLI()
{
}

bool SSHConnectionCLI::precheck(QString* error)
{
  const QString sshExe = QStandardPaths::findExecutable("ssh");
  if (sshExe.isEmpty()) {
    if (error)
      *error = "Could not find 'ssh' in PATH.";
    return false;
  }

  const QString scpExe = QStandardPaths::findExecutable("scp");
  if (scpExe.isEmpty()) {
    if (error)
      *error = "Could not find 'scp' in PATH.";
    return false;
  }

  QStringList args;
  args << "-o" << "BatchMode=yes"
       << "-o" << "NumberOfPasswordPrompts=0"
       << "-o" << "ConnectTimeout=10";

  if (!m_user.isEmpty())
    args << "-l" << m_user;

  args << "-p" << QString::number(m_port)
       << m_host
       << "true";

  QString commandError;
  if (!runPrecheckCommand(sshExe, args, &commandError)) {
    if (error) {
      *error = QString("System SSH precheck failed for %1%2%3:%4.\n%5")
                 .arg(m_user)
                 .arg(m_user.isEmpty() ? "" : "@")
                 .arg(m_host)
                 .arg(m_port)
                 .arg(commandError);
    }
    return false;
  }

  return true;
}

bool SSHConnectionCLI::execute(const QString& command, QString& stdout_str,
                               QString& stderr_str, int& exitcode,
                               bool /*printWarning*/, int timeoutMs,
                               const std::atomic<bool>* cancel)
{
  return this->executeSSH(command, QStringList(), &stdout_str, &stderr_str,
                          &exitcode, timeoutMs, cancel);
}

bool SSHConnectionCLI::copyFileToServer(const QString& localpath,
                                        const QString& remotepath)
{
  return this->executeSCPTo(localpath, remotepath);
}

bool SSHConnectionCLI::copyFileFromServer(const QString& remotepath,
                                          const QString& localpath)
{
  return this->executeSCPFrom(remotepath, localpath);
}

bool SSHConnectionCLI::readRemoteFile(const QString& filename,
                                      QString& contents)
{
  int exitCode = -1;
  return this->executeSSH("cat", QStringList(filename), &contents, nullptr, &exitCode) && exitCode == 0;
}

bool SSHConnectionCLI::removeRemoteFile(const QString& filename)
{
  int exitCode = -1;
  return this->executeSSH("rm", QStringList(filename), nullptr, nullptr, &exitCode) && exitCode == 0;
}

bool SSHConnectionCLI::copyDirectoryToServer(const QString& localpath,
                                             const QString& remotepath)
{
  return this->executeSCPTo(localpath, remotepath, QStringList("-r"));
}

bool SSHConnectionCLI::copyDirectoryFromServer(const QString& remotepath,
                                               const QString& localpath)
{
  // This extra step is performed so that the SCP overwrites the local
  // directory with the remote directory instead of erroneously placing a
  // copy of the remote directory inside local directory. PSA
  QString new_localpath = Common::localPath(localpath, "..");
  return this->executeSCPFrom(remotepath, new_localpath, QStringList("-r"));
  //    return this->executeSCPFrom(remotepath, localpath, QStringList("-r"));
}

bool SSHConnectionCLI::readRemoteDirectoryContents(const QString& remotepath,
                                                   QStringList& contents)
{
  QString contents_str;
  const bool fromHome = remotepath == "~" || remotepath.startsWith("~/");
  QString findPath = remotepath;
  if (fromHome) {
    findPath = remotepath == "~" ? "." : remotepath.mid(2);
    if (findPath.isEmpty())
      findPath = ".";
  }
  while (findPath.size() > 1 && findPath.endsWith('/'))
    findPath.chop(1);

  const QString command = (fromHome ? "cd ~ && " : QString()) +
                          "find " + Common::quoteRemotePath(findPath) +
                          " -mindepth 1 -print";
  int exitCode = -1;
  if (!this->executeSSH(command, QStringList(), &contents_str, nullptr, &exitCode) || exitCode != 0) {
    return false;
  }
  contents = contents_str.split("\n", QtCompat::SkipEmptyParts);
  if (fromHome) {
    for (auto& entry : contents) {
      while (entry.startsWith("./"))
        entry.remove(0, 2);
      entry = Common::remotePath("~", entry);
    }
  }
  return true;
}

bool SSHConnectionCLI::removeRemoteDirectory(const QString& remotepath,
                                             bool onlyDeleteContents)
{
  if (remotepath.isEmpty() || remotepath == "/") {
    Common::warning(QString("Refusing to remove directory \"%1\".")
                   .arg(remotepath));
    return false;
  }

  const QString target = Common::quoteRemotePath(remotepath);
  const QString command =
    onlyDeleteContents ? "find " + target + " -mindepth 1 -maxdepth 1 -exec rm -rf -- {} +"
                       : "rm -rf -- " + target;
  int exitCode = -1;
  return this->executeSSH(command, QStringList(), nullptr, nullptr, &exitCode) && exitCode == 0;
}

bool SSHConnectionCLI::executeSSH(const QString& command,
                                  const QStringList& args, QString* stdout_str,
                                  QString* stderr_str, int* ec, int timeoutMs,
                                  const std::atomic<bool>* cancel)
{
  QStringList fullArgs;

  // Add username
  if (!m_user.isEmpty())
    fullArgs << "-l" << m_user;

  // Add port number
  fullArgs << "-p" << QString::number(m_port);

  // Add hostname
  fullArgs << m_host;

  // Add command and original arguments
  fullArgs << command;
  // ssh sends the remote command through the user's shell. All callers pass
  // remote paths as arguments; quote them with the shared remote-path rule.
  fullArgs << quoteRemotePathArgs(args);

  const int timeout_ms = timeoutMs < 0 ? CLISSH_RUN_TIMEOUT : timeoutMs;
  const ProcessResult result = runCliSshCommand("ssh", fullArgs, timeout_ms, cancel);

  if (!result.started) {
    Common::warning(QString("Failed to start ssh command with args \"%1\" "
                           "after %2 seconds.")
                   .arg(fullArgs.join(","))
                   .arg(timeout_ms / 1000));
    return false;
  }

  if (!result.finished) {
    Common::warning(QString("ssh command with args \"%1\" failed to finish "
                           "within %2 seconds.")
                   .arg(fullArgs.join(","))
                   .arg(timeout_ms / 1000));
    return false;
  }

  if (result.exitCode == 255) {
    Common::warning(QString("ssh command with args \"%1\" returned an exit "
                           "code of 255. This usually means that ssh failed "
                           "to connect, but it may also be a valid exit code "
                           "for the command which was run. Assuming that ssh "
                           "has errored. Contact the development team if you "
                           "believe this is an error.\nstdout:\n%2\nstderr:\n%3")
                   .arg(fullArgs.join(","))
                   .arg(result.stdoutText)
                   .arg(result.stderrText));
    assignProcessOutputs(result, stdout_str, stderr_str, ec);
    return false;
  }

  assignProcessOutputs(result, stdout_str, stderr_str, ec);
  return true;
}

bool SSHConnectionCLI::executeSCPTo(const QString& source, const QString& dest,
                                    const QStringList& args,
                                    QString* stdout_str, QString* stderr_str,
                                    int* ec)
{
  // Start with input args
  QStringList fullArgs(args);

  // Add port number
  fullArgs << "-P" << QString::number(m_port);

  // Add source
  fullArgs << source;

  // Add destination. The remote path is passed as-is, on purpose: legacy
  // scp strips quotes through the remote shell, but SFTP-mode scp (the
  // default since OpenSSH 9.0) would keep them as literal characters. A
  // plain path, including a leading "~/", works in both modes.
  fullArgs << QString("%1%2%3:%4")
                .arg(m_user)
                .arg(m_user.isEmpty() ? "" : "@")
                .arg(m_host)
                .arg(dest);

  const int timeout_ms = CLISSH_SCP_TIMEOUT;
  const ProcessResult result = runCliSshCommand("scp", fullArgs, timeout_ms);

  if (!result.started) {
    Common::warning(QString("Failed to start scp command with args \"%1\" "
                           "after %2 seconds.")
                   .arg(fullArgs.join(","))
                   .arg(timeout_ms / 1000));
    return false;
  }

  if (!result.finished) {
    Common::warning(QString("scp command with args \"%1\" failed to finish "
                           "within %2 seconds.")
                   .arg(fullArgs.join(","))
                   .arg(timeout_ms / 1000));
    return false;
  }

  if (result.exitCode != 0) {
    Common::warning(QString("scp command with args \"%1\" failed with an exit "
                           "code of %2.\nstdout:\n%3\nstderr:\n%4")
                   .arg(fullArgs.join(","))
                   .arg(result.exitCode)
                   .arg(result.stdoutText)
                   .arg(result.stderrText));
    return false;
  }

  assignProcessOutputs(result, stdout_str, stderr_str, ec);
  return true;
}

bool SSHConnectionCLI::executeSCPFrom(const QString& source,
                                      const QString& dest,
                                      const QStringList& args,
                                      QString* stdout_str, QString* stderr_str,
                                      int* ec)
{
  // Start with input args
  QStringList fullArgs(args);

  // Add port number
  fullArgs << "-P" << QString::number(m_port);

  // Add source. The remote path is passed as-is; see the note in
  // executeSCPTo() about scp quoting.
  fullArgs << QString("%1%2%3:%4")
                .arg(m_user)
                .arg(m_user.isEmpty() ? "" : "@")
                .arg(m_host)
                .arg(source);

  // Add destination
  fullArgs << dest;

  const int timeout_ms = CLISSH_SCP_TIMEOUT;
  const ProcessResult result = runCliSshCommand("scp", fullArgs, timeout_ms);

  if (!result.started) {
    Common::warning(QString("Failed to start scp command with args \"%1\" "
                           "after %2 seconds.")
                   .arg(fullArgs.join(","))
                   .arg(timeout_ms / 1000));
    return false;
  }

  if (!result.finished) {
    Common::warning(QString("scp command with args \"%1\" failed to finish "
                           "within %2 seconds.")
                   .arg(fullArgs.join(","))
                   .arg(timeout_ms / 1000));
    return false;
  }

  if (result.exitCode != 0) {
    Common::warning(QString("scp command with args \"%1\" failed with an exit "
                           "code of %2.\nstdout:\n%3\nstderr:\n%4")
                   .arg(fullArgs.join(","))
                   .arg(result.exitCode)
                   .arg(result.stdoutText)
                   .arg(result.stderrText));
    return false;
  }

  assignProcessOutputs(result, stdout_str, stderr_str, ec);
  return true;
}

} // end namespace Search

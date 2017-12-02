/**********************************************************************
  SSHConnection - Connection to an ssh server for execution, sftp, etc.

  Copyright (C) 2010-2012 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifdef ENABLE_SSH

#include <globalsearch/sshconnection_cli.h>

#include <globalsearch/sshmanager_cli.h>

#include <QDebug>
#include <QProcess>

namespace GlobalSearch {

SSHConnectionCLI::SSHConnectionCLI(SSHManagerCLI* parent)
  : SSHConnection(parent)
{
}

SSHConnectionCLI::~SSHConnectionCLI()
{
}

bool SSHConnectionCLI::execute(const QString& command, QString& stdout_str,
                               QString& stderr_str, int& exitcode,
                               bool printWarning)
{
  return this->executeSSH(command, QStringList(), &stdout_str, &stderr_str,
                          &exitcode);
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
  return this->executeSSH("cat", QStringList(filename), &contents);
}

bool SSHConnectionCLI::removeRemoteFile(const QString& filename)
{
  return this->executeSSH("rm", QStringList(filename));
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
  // qDebug() << "localpath is" << localpath;
  QString new_localpath = localpath + "..";
  // qDebug() << "new_localpath is" << new_localpath;
  return this->executeSCPFrom(remotepath, new_localpath, QStringList("-r"));
  //    return this->executeSCPFrom(remotepath, localpath, QStringList("-r"));
}

bool SSHConnectionCLI::readRemoteDirectoryContents(const QString& remotepath,
                                                   QStringList& contents)
{
  QString contents_str;
  // This is to produce an output similar to the libssh implementation.
  QStringList args;
  args << remotepath + "/*"
       << "|"
       << "xargs"
       << "ls"
       << "-Fd";
  if (!this->executeSSH("find", args, &contents_str)) {
    return false;
  }
  contents = contents_str.split("\n", QString::SkipEmptyParts);
  return true;
}

bool SSHConnectionCLI::removeRemoteDirectory(const QString& remotepath,
                                             bool onlyDeleteContents)
{
  if (remotepath.isEmpty() || remotepath == "/") {
    qWarning()
      << QString("Refusing to remove directory \"%1\".").arg(remotepath);
    return false;
  }

  QStringList args("-rf");

  if (onlyDeleteContents)
    args << remotepath + "/*";
  else
    args << remotepath;

  return this->executeSSH("rm", args);
}

bool SSHConnectionCLI::executeSSH(const QString& command,
                                  const QStringList& args, QString* stdout_str,
                                  QString* stderr_str, int* ec)
{
  QProcess proc;

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
  fullArgs << args;

  proc.start("ssh", fullArgs);
  int timeout_ms = 60000; // one minute

  if (!proc.waitForStarted(timeout_ms)) {
    qWarning() << QString("Failed to start ssh command with args \"%1\" "
                          "after %2 seconds.")
                    .arg(fullArgs.join(","))
                    .arg(timeout_ms / 1000);
    return false;
  }

  proc.closeWriteChannel();
  if (!proc.waitForFinished(timeout_ms)) {
    qWarning() << QString("ssh command with args \"%1\" failed to finish "
                          "within %2 seconds.")
                    .arg(fullArgs.join(","))
                    .arg(timeout_ms / 1000);
    return false;
  }

  if (proc.exitCode() == 255) {
    qWarning() << QString("ssh command with args \"%1\" returned an exit "
                          "code of 255. This usually means that ssh failed "
                          "to connect, but it may also be a valid exit code"
                          " for the command which was run. Assuming that "
                          "ssh has errored. Contact the development team "
                          "if you believe this is an error.")
                    .arg(fullArgs.join(","))
               << "\nstdout:\n"
               << QString(proc.readAllStandardOutput()) << "\nstderr:\n"
               << QString(proc.readAllStandardError());
    return false;
  }

  if (stdout_str != nullptr)
    *stdout_str = QString(proc.readAllStandardOutput());
  if (stderr_str != nullptr)
    *stderr_str = QString(proc.readAllStandardError());
  if (ec != nullptr)
    *ec = proc.exitCode();

  proc.close();

  return true;
}

bool SSHConnectionCLI::executeSCPTo(const QString& source, const QString& dest,
                                    const QStringList& args,
                                    QString* stdout_str, QString* stderr_str,
                                    int* ec)
{
  QProcess proc;

  // Start with input args
  QStringList fullArgs(args);

  // Add port number
  fullArgs << "-P" << QString::number(m_port);

  // Add source
  fullArgs << source;

  // Add destination
  fullArgs << QString("%1%2%3:%4")
                .arg(m_user)
                .arg(m_user.isEmpty() ? "" : "@")
                .arg(m_host)
                .arg(dest);

  proc.start("scp", fullArgs);
  int timeout_ms = 60000; // one minute

  if (!proc.waitForStarted(timeout_ms)) {
    qWarning() << QString("Failed to start scp command with args \"%1\" "
                          "after %2 seconds.")
                    .arg(fullArgs.join(","))
                    .arg(timeout_ms / 1000);
    return false;
  }

  proc.closeWriteChannel();
  if (!proc.waitForFinished(timeout_ms)) {
    qWarning() << QString("scp command with args \"%1\" failed to finish "
                          "within %2 seconds.")
                    .arg(fullArgs.join(","))
                    .arg(timeout_ms / 1000);
    return false;
  }

  if (proc.exitCode() != 0) {
    qWarning() << QString("scp command with args \"%1\" failed with an exit "
                          "code of %2.")
                    .arg(fullArgs.join(","))
                    .arg(proc.exitCode())
               << "\nstdout:\n"
               << QString(proc.readAllStandardOutput()) << "\nstderr:\n"
               << QString(proc.readAllStandardError());
    return false;
  }

  if (stdout_str != nullptr)
    *stdout_str = QString(proc.readAllStandardOutput());
  if (stderr_str != nullptr)
    *stderr_str = QString(proc.readAllStandardError());
  if (ec != nullptr)
    *ec = proc.exitCode();

  proc.close();

  return true;
}

bool SSHConnectionCLI::executeSCPFrom(const QString& source,
                                      const QString& dest,
                                      const QStringList& args,
                                      QString* stdout_str, QString* stderr_str,
                                      int* ec)
{
  QProcess proc;

  // Start with input args
  QStringList fullArgs(args);

  // Add port number
  fullArgs << "-P" << QString::number(m_port);

  // Add source
  fullArgs << QString("%1%2%3:%4")
                .arg(m_user)
                .arg(m_user.isEmpty() ? "" : "@")
                .arg(m_host)
                .arg(source);

  // Add destination
  fullArgs << dest;

  proc.start("scp", fullArgs);
  int timeout_ms = 60000; // one minute

  if (!proc.waitForStarted(timeout_ms)) {
    qWarning() << QString("Failed to start scp command with args \"%1\" "
                          "after %2 seconds.")
                    .arg(fullArgs.join(","))
                    .arg(timeout_ms / 1000);
    return false;
  }

  proc.closeWriteChannel();
  if (!proc.waitForFinished(timeout_ms)) {
    qWarning() << QString("scp command with args \"%1\" failed to finish "
                          "within %2 seconds.")
                    .arg(fullArgs.join(","))
                    .arg(timeout_ms / 1000);
    return false;
  }

  if (proc.exitCode() != 0) {
    qWarning() << QString("scp command with args \"%1\" failed with an exit "
                          "code of %2.")
                    .arg(fullArgs.join(","))
                    .arg(proc.exitCode())
               << "\nstdout:\n"
               << QString(proc.readAllStandardOutput()) << "\nstderr:\n"
               << QString(proc.readAllStandardError());
    return false;
  }

  if (stdout_str != nullptr)
    *stdout_str = QString(proc.readAllStandardOutput());
  if (stderr_str != nullptr)
    *stderr_str = QString(proc.readAllStandardError());
  if (ec != nullptr)
    *ec = proc.exitCode();

  proc.close();

  return true;
}

} // end namespace GlobalSearch

#endif // ENABLE_SSH

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

#ifndef SSHCONNECTIONCLI_H
#define SSHCONNECTIONCLI_H

#ifdef ENABLE_SSH

#include <globalsearch/sshconnection.h>

#include <QString>
#include <QStringList>

namespace GlobalSearch {
class OptBase;
class SSHManager;
class SSHManagerCLI;

/**
 * @class SSHConnectionCLI sshconnection_cli.h
 * <globalsearch/sshconnection_cli.h>
 *
 * @brief A class to handle command execution and sftp transactions
 * on an ssh server.
 *
 * This class should not be created directly. Instead, see SSHManager.
 *
 * @author David C. Lonie
 */
class SSHConnectionCLI : public SSHConnection
{
  Q_OBJECT

public:
  /**
   * Constructor.
   *
   * @param parent The OptBase parent
   */
  explicit SSHConnectionCLI(SSHManagerCLI* parent = 0);

  /**
   * Destructor.
   */
  virtual ~SSHConnectionCLI() override;

public slots:
  /**
   * Execute an arbitrary command on the connected host.
   *
   * @param command Command to execute
   * @param stdout_str (return) standard output
   * @param stderr_str (return) standard output
   * @param exitcode (return) exit code.
   *
   * @return True on success.
   */
  bool execute(const QString& command, QString& stdout_str, QString& stderr_str,
               int& exitcode, bool printWarning = false);

  /**
   * Copy a file to the remote host
   *
   * @param localpath Local file to copy
   * @param remotepath Destination path on connected host
   *
   * @return True on success.
   */
  bool copyFileToServer(const QString& localpath, const QString& remotepath);

  /**
   * Copy a file from the remote host
   *
   * @param remotepath Remote file to copy
   * @param localpath Destination path on local host
   *
   * @return True on success.
   */
  bool copyFileFromServer(const QString& remotepath, const QString& localpath);

  /**
   * Obtain a QString object contain the contents of a remote text
   * file.
   *
   * @param filename Name of text file on remote host to read
   * @param contents (return) contents of file.
   *
   * @return True on success.
   */
  bool readRemoteFile(const QString& filename, QString& contents);

  /**
   * Delete a file from the remote host
   *
   * @param filename Remote file to remove.
   *
   * @return True on success.
   */
  bool removeRemoteFile(const QString& filename);

  /**
   * Copy a directory to the remote host
   *
   * @param localpath Local directory to copy
   * @param remotepath Destination path on connected host
   *
   * @return True on success.
   */
  bool copyDirectoryToServer(const QString& localpath,
                             const QString& remotepath);

  /**
   * Copy a directory from the remote host
   *
   * @param remotepath Remote directory to copy
   * @param localpath Destination path on local host
   *
   * @return True on success.
   */
  bool copyDirectoryFromServer(const QString& remotepath,
                               const QString& localpath);

  /**
   * List the contents of a remote directory
   *
   * @param remotepath Path to remote directory
   * @param contents (return) List of file/directory names
   *
   * @return True on success.
   */
  bool readRemoteDirectoryContents(const QString& remotepath,
                                   QStringList& contents);

  /**
   * Recursively delete a directory and its contents.
   *
   * @param remotepath Directory on remote host to delete
   * @param onlyDeleteContents If true, only clean the directory
   * (default: false)
   *
   * @return True on success.
   */
  bool removeRemoteDirectory(const QString& remotepath,
                             bool onlyDeleteContents = false);

protected:
  bool executeSSH(const QString& command,
                  const QStringList& args = QStringList(),
                  QString* stdout_str = nullptr, QString* stderr_str = nullptr,
                  int* ec = nullptr);
  bool executeSCPTo(const QString& source, const QString& dest,
                    const QStringList& args = QStringList(),
                    QString* stdout_str = nullptr,
                    QString* stderr_str = nullptr, int* ec = nullptr);
  bool executeSCPFrom(const QString& source, const QString& dest,
                      const QStringList& args = QStringList(),
                      QString* stdout_str = nullptr,
                      QString* stderr_str = nullptr, int* ec = nullptr);
};

} // end namespace GlobalSearch

#endif // ENABLE_SSH
#endif // SSHCONNECTION_H

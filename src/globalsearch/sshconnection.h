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

#ifndef SSHCONNECTION_H
#define SSHCONNECTION_H

#ifdef ENABLE_SSH

#include <QObject>
#include <QString>

namespace GlobalSearch {
class OptBase;
class SSHManager;

/**
 * @class SSHConnection sshconnection.h <globalsearch/sshconnection.h>
 *
 * @brief A class to handle command execution and sftp transactions
 * on an ssh server.
 *
 * This class should not be created directly. Instead, see SSHManager.
 *
 * @author David C. Lonie
 */
class SSHConnection : public QObject
{
  Q_OBJECT

public:
  /// Exceptions
  enum SSHConnectionException
  {
    /// An error connecting to the host has occurred
    SSH_CONNECTION_ERROR = 1,
    /// The host is unknown or has changed its key.
    /// The key can be retrieved through SSHManager::getServerKey()
    /// and accepted via SSHManager::validateServerKey()
    SSH_UNKNOWN_HOST_ERROR,
    /// A bad password was given and public key auth is not set up
    SSH_BAD_PASSWORD_ERROR,
    /// An unknown error has occurred
    SSH_UNKNOWN_ERROR
  };

  /**
   * Constructor.
   *
   * @param parent The OptBase parent
   */
  explicit SSHConnection(SSHManager* parent = 0);

  /**
   * Destructor.
   */
  virtual ~SSHConnection() override;

  /// @return The currently set username
  QString getUser() { return m_user; };
  /// @return The currently set hostname
  QString getHost() { return m_host; };
  /// @return The currently set port
  int getPort() { return m_port; };

public slots:
  /**
   * Set the login details for this connections
   *
   * @param host Hostname
   * @param user Username
   * @param pass Password
   * @param port Port
   */
  virtual void setLoginDetails(const QString& host, const QString& user = "",
                               const QString& pass = "", int port = 22);

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
  virtual bool execute(const QString& command, QString& stdout_str,
                       QString& stderr_str, int& exitcode,
                       bool printWarning = true) = 0;

  /**
   * Copy a file to the remote host
   *
   * @param localpath Local file to copy
   * @param remotepath Destination path on connected host
   *
   * @return True on success.
   */
  virtual bool copyFileToServer(const QString& localpath,
                                const QString& remotepath) = 0;

  /**
   * Copy a file from the remote host
   *
   * @param remotepath Remote file to copy
   * @param localpath Destination path on local host
   *
   * @return True on success.
   */
  virtual bool copyFileFromServer(const QString& remotepath,
                                  const QString& localpath) = 0;

  /**
   * Obtain a QString object contain the contents of a remote text
   * file.
   *
   * @param filename Name of text file on remote host to read
   * @param contents (return) contents of file.
   *
   * @return True on success.
   */
  virtual bool readRemoteFile(const QString& filename, QString& contents) = 0;

  /**
   * Delete a file from the remote host
   *
   * @param filename Remote file to remove.
   *
   * @return True on success.
   */
  virtual bool removeRemoteFile(const QString& filename) = 0;

  /**
   * Copy a directory to the remote host
   *
   * @param localpath Local directory to copy
   * @param remotepath Destination path on connected host
   *
   * @return True on success.
   */
  virtual bool copyDirectoryToServer(const QString& localpath,
                                     const QString& remotepath) = 0;

  /**
   * Copy a directory from the remote host
   *
   * @param remotepath Remote directory to copy
   * @param localpath Destination path on local host
   *
   * @return True on success.
   */
  virtual bool copyDirectoryFromServer(const QString& remotepath,
                                       const QString& localpath) = 0;

  /**
   * List the contents of a remote directory
   *
   * @param remotepath Path to remote directory
   * @param contents (return) List of file/directory names
   *
   * @return True on success.
   */
  virtual bool readRemoteDirectoryContents(const QString& remotepath,
                                           QStringList& contents) = 0;

  /**
   * Recursively delete a directory and its contents.
   *
   * @param remotepath Directory on remote host to delete
   * @param onlyDeleteContents If true, only clean the directory
   * (default: false)
   *
   * @return True on success.
   */
  virtual bool removeRemoteDirectory(const QString& remotepath,
                                     bool onlyDeleteContents = false) = 0;

protected:
  QString m_host;
  QString m_user;
  QString m_pass;
  int m_port;
};

} // end namespace GlobalSearch

#endif // ENABLE_SSH
#endif // SSHCONNECTION_H

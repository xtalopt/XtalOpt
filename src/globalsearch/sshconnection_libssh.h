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

#ifndef SSHCONNECTIONLIBSSH_H
#define SSHCONNECTIONLIBSSH_H

#ifdef ENABLE_SSH

extern "C" {
#include <libssh/libssh.h>
#include <libssh/sftp.h>
}

#include <globalsearch/sshconnection.h>

#include <QDateTime>
#include <QMutex>

#define LIBSSH_BUFFER_SIZE 20480

namespace GlobalSearch {
class SSHManagerLibSSH;

/**
 * @class SSHConnectionLibSSH sshconnection_libssh.h
 * <globalsearch/sshconnection_libssh.h>
 *
 * @brief A class to handle command execution and sftp transactions
 * on an ssh server using the libssh backend.
 *
 * This class should not be created directly. Instead, see SSHManager.
 *
 * @author David C. Lonie
 */
class SSHConnectionLibSSH : public SSHConnection
{
  Q_OBJECT

public:
  /**
   * Constructor.
   *
   * @param parent The OptBase parent
   */
  explicit SSHConnectionLibSSH(SSHManagerLibSSH* parent = 0);

  /**
   * Destructor.
   */
  virtual ~SSHConnectionLibSSH() override;

public slots:
  /// Flag whether this connection is in use or not.
  void setUsed(bool b) { m_inUse = b; };

  /// @return True if this connection is in use.
  bool inUse() { return m_inUse; };

  /**
   * Execute an arbitrary command on the connected host.
   *
   * @param command Command to execute
   * @param stdout_str (return) standard output
   * @param stderr_str (return) standard output
   * @param exitcode (return) exit code.
   * @param printWarning Prints warnings with qWarning() if an error occurs
   *
   * @return True on success.
   */
  virtual bool execute(const QString& command, QString& stdout_str,
                       QString& stderr_str, int& exitcode,
                       bool printWarning = true) override;

  /**
   * Copy a file to the remote host
   *
   * @param localpath Local file to copy
   * @param remotepath Destination path on connected host
   *
   * @return True on success.
   */
  bool copyFileToServer(const QString& localpath,
                        const QString& remotepath) override;

  /**
   * Copy a file from the remote host
   *
   * @param remotepath Remote file to copy
   * @param localpath Destination path on local host
   *
   * @return True on success.
   */
  bool copyFileFromServer(const QString& remotepath,
                          const QString& localpath) override;

  /**
   * Obtain a QString object contain the contents of a remote text
   * file.
   *
   * @param filename Name of text file on remote host to read
   * @param contents (return) contents of file.
   *
   * @return True on success.
   */
  bool readRemoteFile(const QString& filename, QString& contents) override;

  /**
   * Delete a file from the remote host
   *
   * @param filename Remote file to remove.
   *
   * @return True on success.
   */
  bool removeRemoteFile(const QString& filename) override;

  /**
   * Copy a directory to the remote host
   *
   * @param localpath Local directory to copy
   * @param remotepath Destination path on connected host
   *
   * @return True on success.
   */
  bool copyDirectoryToServer(const QString& localpath,
                             const QString& remotepath) override;

  /**
   * Copy a directory from the remote host
   *
   * @param remotepath Remote directory to copy
   * @param localpath Destination path on local host
   *
   * @return True on success.
   */
  bool copyDirectoryFromServer(const QString& remotepath,
                               const QString& localpath) override;

  /**
   * List the contents of a remote directory
   *
   * @param remotepath Path to remote directory
   * @param contents (return) List of file/directory names
   *
   * @return True on success.
   */
  bool readRemoteDirectoryContents(const QString& remotepath,
                                   QStringList& contents) override;

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
                             bool onlyDeleteContents = false) override;

  /// @return True if the session is valid
  bool isValid() { return m_isValid; };

  /// @return True if the sftp is connected
  bool sftpIsConnected();

  /// @return True if the sftp is successfully reconnected
  bool reconnectSftp();

  /// @return True if the sftp either successfully reconnected or did not
  // need to reconnect
  bool reconnectSftpIfNeeded();

  /// @return True if the sftp is successfully disconnected
  bool disconnectSftp();

  /// @return True if the sftp is successfully connected
  bool connectSftp();

  /// @return True if the session is connected
  bool isConnected();

  /**
   * Attempts to create a connection to the remote server. If \a
   * throwExceptions is true, one of the
   * GlobalSearch::SSHConnection::SSHConnectionException exceptions
   * will be thrown on errors. Otherwise, the function will return
   * false on errors.
   *
   * @return True on success.
   */
  bool connectSession(bool throwExceptions = false);

  /**
   * Disconnect and connect this session.
   *
   * @sa disconnectSession
   * @sa connectSession
   */
  bool reconnectSession(bool throwExceptions = false);

  /**
   * Terminate the current connection to the remote host.
   *
   * @return True on success.
   */
  bool disconnectSession();

  /**
   * Similar to reconnectSession, but calls isConnected first and
   * only reconects if needed.
   *
   * @return True if no errors.
   */
  bool reconnectIfNeeded()
  {
    if (!isConnected())
      return reconnectSession(false);
    return true;
  };

  /**
   * Add the passed host's key to the local host's knownhosts cache.
   *
   * @param host Hostname of server
   * @param port Port of SSH server
   *
   * @return True on success.
   */
  static bool addKeyToKnownHosts(const QString& host, unsigned int port = 22);

signals:
  /**
   * Emitted when a host is not recognized
   *
   * @param hexa Hex data representing the server.
   */
  void unknownHostKey(const QString& hexa);

protected:
  // Disable doxygen parsing
  /// \cond
  sftp_session _openSFTP();
  bool _execute(const QString& command, QString& stdout_err,
                QString& stderr_err, int& exitcode, bool printWarning = true);
  bool _copyFileToServer(const QString& localpath, const QString& remotepath);
  bool _copyFileFromServer(const QString& remotepath, const QString& localpath);
  bool _readRemoteFile(const QString& filename, QString& contents);
  bool _removeRemoteFile(const QString& filename);
  bool _copyDirectoryToServer(const QString& localpath,
                              const QString& remotepath);
  bool _copyDirectoryFromServer(const QString& remotepath,
                                const QString& localpath);
  bool _readRemoteDirectoryContents(const QString& remotepath,
                                    QStringList& contents);
  bool _removeRemoteDirectory(const QString& remotepath,
                              bool onlyDeleteContents = false);

  ssh_session m_session;
  ssh_channel m_shell;
  sftp_session m_sftp;

  // For determining when to reconnect the sftp session
  QDateTime m_sftpTimeStamp;

  bool m_isValid;
  bool m_inUse;
  QMutex m_lock;

  // Resume doxygen parsing
  /// \endcond
};

} // end namespace GlobalSearch

#endif // ENABLE_SSH
#endif // SSHCONNECTIONLIBSSH_H

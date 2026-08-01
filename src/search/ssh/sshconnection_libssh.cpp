/**********************************************************************
  SSHConnection - Connection to an ssh server for execution, sftp, etc.

  Copyright (C) 2010-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/


#include <search/ssh/sshconnection_libssh.h>

#include <common/compatibility/qt_compat.h>
#include <common/compatibility/platform_compat.h>
#include <common/fileutils.h>
#include <common/output.h>
#include <search/ssh/sshmanager_libssh.h>

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QThread>

// File access flags for libssh.
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <vector>

using namespace std;

namespace Search {
namespace {

QString sftpPath(const QString& path)
{
  if (path == "~" || path == "~/")
    return ".";
  if (path.startsWith("~/"))
    return path.mid(2);
  return path;
}

}

#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 4, 90)
#define GS_CHANNEL_IS_CLOSED(channel) ssh_channel_is_closed(channel)
#define GS_CHANNEL_IS_EOF(channel) ssh_channel_is_eof(channel)
#define GS_CHANNEL_FREE(channel) ssh_channel_free(channel)
#define GS_CHANNEL_NEW(session) ssh_channel_new(session)
#define GS_CHANNEL_OPEN_SESSION(channel) ssh_channel_open_session(channel)
#define GS_CHANNEL_REQUEST_SHELL(channel) ssh_channel_request_shell(channel)
#define GS_CHANNEL_REQUEST_EXEC(channel, command) \
  ssh_channel_request_exec(channel, command)
#define GS_CHANNEL_CLOSE(channel) ssh_channel_close(channel)
#define GS_CHANNEL_READ(channel, buffer, count, is_stderr) \
  ssh_channel_read(channel, buffer, count, is_stderr)
#define GS_CHANNEL_READ_NONBLOCKING(channel, buffer, count, is_stderr) \
  ssh_channel_read_nonblocking(channel, buffer, count, is_stderr)
#define GS_CHANNEL_SEND_EOF(channel) ssh_channel_send_eof(channel)
#else
#define GS_CHANNEL_IS_CLOSED(channel) channel_is_closed(channel)
#define GS_CHANNEL_IS_EOF(channel) channel_is_eof(channel)
#define GS_CHANNEL_FREE(channel) channel_free(channel)
#define GS_CHANNEL_NEW(session) channel_new(session)
#define GS_CHANNEL_OPEN_SESSION(channel) channel_open_session(channel)
#define GS_CHANNEL_REQUEST_SHELL(channel) channel_request_shell(channel)
#define GS_CHANNEL_REQUEST_EXEC(channel, command) channel_request_exec(channel, command)
#define GS_CHANNEL_CLOSE(channel) channel_close(channel)
#define GS_CHANNEL_READ(channel, buffer, count, is_stderr) \
  channel_read(channel, buffer, count, is_stderr)
#define GS_CHANNEL_READ_NONBLOCKING(channel, buffer, count, is_stderr) \
  channel_read_nonblocking(channel, buffer, count, is_stderr)
#define GS_CHANNEL_SEND_EOF(channel) channel_send_eof(channel)
#endif

#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 11, 0)
static int getChannelExitStatus(ssh_channel channel, int& exitcode)
{
  uint32_t status = static_cast<uint32_t>(-1);
  const int rc = ssh_channel_get_exit_state(channel, &status, nullptr, nullptr);
  if (rc == SSH_OK) {
    exitcode = static_cast<int>(status);
  }
  return rc;
}
#else
static int getChannelExitStatus(ssh_channel channel, int& exitcode)
{
  exitcode = ssh_channel_get_exit_status(channel);
  return (exitcode == -1) ? SSH_AGAIN : SSH_OK;
}
#endif

#define START
#define END

//#define SSH_CONNECTION_LIBSSH_DEBUG

SSHConnectionLibSSH::SSHConnectionLibSSH(SSHManagerLibSSH* parent)
  : SSHConnection(parent), m_session(0), m_shell(0), m_sftp(0),
    m_sftpTimeStamp(QDateTime::currentDateTime()), m_isValid(false),
    m_inUse(false)
{
  if (parent) {
    // block this connection so that a thrown exception won't cause problems
    connect(this, &SSHConnectionLibSSH::unknownHostKey, parent, &SSHManagerLibSSH::setServerKey);
  }
}

SSHConnectionLibSSH::~SSHConnectionLibSSH()
{
  START;
  disconnectSession();
  END;
}

bool SSHConnectionLibSSH::isConnected()
{
  if (!m_session || !m_shell || !m_sftp || GS_CHANNEL_IS_CLOSED(m_shell) ||
      GS_CHANNEL_IS_EOF(m_shell)) {
    Common::warning("SSHConnectionLibSSH is not connected: one or more required "
                   "channels are not initialized.");
    return false;
  };

  if (!ssh_is_connected(m_session)) {
    Common::warning("SSHConnectionLibSSH is not connected.");
    return false;
  }

  START;

  // Check to see if sftp is still connected. If it is not,
  // then reconnect it. It will sometimes disconnect...
  if (!reconnectSftpIfNeeded()) {
    Common::warning(QString("%1: reconnectSftpIfNeeded() failed. "
                           "Reconnecting session.")
                     .arg(__func__));
    return false;
  }

  END;
  return true;
}

bool SSHConnectionLibSSH::disconnectSession()
{
  QtCompat::MutexLocker locker(&m_lock);
  START;

  if (m_sftp)
    sftp_free(m_sftp);
  m_sftp = 0;

  if (m_shell)
    GS_CHANNEL_FREE(m_shell);
  m_shell = 0;

  if (m_session)
    ssh_free(m_session);
  m_session = 0;

  m_isValid = false;
  END;
  return true;
}

bool SSHConnectionLibSSH::reconnectSession(bool throwExceptions)
{
  START;
  if (!disconnectSession())
    return false;
  if (!connectSession(throwExceptions))
    return false;
  END;
  return true;
}

bool SSHConnectionLibSSH::sftpIsConnected()
{
  START;
  int err = sftp_get_error(m_sftp);
  if (err == SSH_FX_FAILURE || err == SSH_FX_NO_CONNECTION || err == SSH_FX_CONNECTION_LOST)
    return false;
  END;
  return true;
}

bool SSHConnectionLibSSH::disconnectSftp()
{
  START;
  if (m_sftp)
    sftp_free(m_sftp);
  m_sftp = 0;
  END;
  return true;
}

bool SSHConnectionLibSSH::connectSftp()
{
  START;
  m_sftp = _openSFTP();
  if (!m_sftp) {
    Common::warning("Could not create sftp channel.");
    return false;
  }
  END;
  return true;
}

bool SSHConnectionLibSSH::reconnectSftp()
{
  START;
  if (!disconnectSftp())
    return false;
  if (!connectSftp())
    return false;
  END;
  return true;
}

bool SSHConnectionLibSSH::reconnectSftpIfNeeded()
{
  START;
  // Check to see if the sftp is even connected
  if (!sftpIsConnected()) {
    bool success = reconnectSftp();
    if (!success) {
      Common::warning(QString("%1: reconnectSftp() failed.")
                       .arg(__func__));
      return false;
    }
    // reset time stamp
    m_sftpTimeStamp = QDateTime::currentDateTime();
  }
  // Check to see if 240 seconds have passed since the last reconnection
  // if it has, reconnect the sftp
  const qint64 timeDifference = m_sftpTimeStamp.msecsTo(QDateTime::currentDateTime());

  // reconnect the sftp every 240 seconds
  const int interval = 240;
  if (timeDifference >= interval * 1000) {
#ifdef SSH_CONNECTION_LIBSSH_DEBUG
    Common::debug(QString("timeDifference is %1").arg(timeDifference));
#endif
    bool success = reconnectSftp();
    if (!success) {
      Common::warning(QString("%1: reconnectSftp() failed.")
                       .arg(__func__));
      return false;
    }
    // reset time stamp
    m_sftpTimeStamp = QDateTime::currentDateTime();
  }

  END;
  return true;
}

bool SSHConnectionLibSSH::connectSession(bool throwExceptions)
{
  QtCompat::MutexLocker locker(&m_lock);
  // Create the SSH session.
  if (m_session) {
    ssh_free(m_session);
    m_session = 0;
  }
  m_session = ssh_new();
  if (!m_session) {
    if (throwExceptions) {
      throw SSH_UNKNOWN_ERROR;
    } else {
      return false;
    }
  }

  // Set options
  int verbosity = SSH_LOG_NOLOG;
  // int verbosity = SSH_LOG_PROTOCOL;
  // int verbosity = SSH_LOG_PACKET;
  int timeout = 15; // timeout in sec

  ssh_options_set(m_session, SSH_OPTIONS_HOST, m_host.toStdString().c_str());
  ssh_options_set(m_session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
  ssh_options_set(m_session, SSH_OPTIONS_TIMEOUT, &timeout);

  if (!m_user.isEmpty()) {
    ssh_options_set(m_session, SSH_OPTIONS_USER, m_user.toStdString().c_str());
  }
  ssh_options_set(m_session, SSH_OPTIONS_PORT, &m_port);

  // Connect
  if (ssh_connect(m_session) != SSH_OK) {
    Common::warning(QString("SSH failure: %1").arg(ssh_get_error(m_session)));
    if (throwExceptions) {
      throw SSH_CONNECTION_ERROR;
    } else {
      return false;
    }
  }

  // Verify that host is known
  int state;
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 8, 0)
  state = ssh_session_is_known_server(m_session);
#else
  state = ssh_is_server_known(m_session);
#endif
  switch (state) {
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 8, 0)
    case SSH_KNOWN_HOSTS_OK:
      break;
    case SSH_KNOWN_HOSTS_CHANGED:
    case SSH_KNOWN_HOSTS_OTHER:
    case SSH_KNOWN_HOSTS_NOT_FOUND:
    case SSH_KNOWN_HOSTS_UNKNOWN: {
#else
    case SSH_SERVER_KNOWN_OK:
      break;
    case SSH_SERVER_KNOWN_CHANGED:
    case SSH_SERVER_FOUND_OTHER:
    case SSH_SERVER_FILE_NOT_FOUND:
    case SSH_SERVER_NOT_KNOWN: {
#endif
      size_t hlen;
      unsigned char* hash = 0;
      ssh_key key = 0;
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 8, 0)
      const int keyRc = ssh_get_server_publickey(m_session, &key);
#else
      const int keyRc = ssh_get_publickey(m_session, &key);
#endif
      bool hostKeyReady = false;
      if (keyRc == SSH_OK && key) {
        const int hashRc = ssh_get_publickey_hash(key, SSH_PUBLICKEY_HASH_SHA1, &hash, &hlen);
        if (hashRc == SSH_OK && hash) {
          char* hexa = ssh_get_hexa(hash, hlen);
          if (hexa) {
            emit unknownHostKey(QString(hexa));
            ssh_string_free_char(hexa);
            hostKeyReady = true;
          }
        }
      }

      if (!hostKeyReady)
        Common::warning(tr("Unable to read SSH host key fingerprint."));

      // Cleanup time
      if (hash)
        ssh_clean_pubkey_hash(&hash);
      if (key)
        ssh_key_free(key);

      if (throwExceptions) {
        throw SSH_UNKNOWN_HOST_ERROR;
      } else {
        return false;
      }
    }
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 8, 0)
    case SSH_KNOWN_HOSTS_ERROR:
#else
    case SSH_SERVER_ERROR:
#endif
      Common::warning(QString("SSH failure: %1").arg(ssh_get_error(m_session)));
      if (throwExceptions) {
        throw SSH_UNKNOWN_ERROR;
      } else {
        return false;
      }
  }

  // Authenticate
  int rc;
  int method;

  // Try to authenticate
  rc = ssh_userauth_none(m_session, nullptr);
  if (rc == SSH_AUTH_ERROR) {
    Common::warning(QString("SSH failure: %1").arg(ssh_get_error(m_session)));
    if (throwExceptions) {
      throw SSH_UNKNOWN_ERROR;
    } else {
      return false;
    }
  }

  method = ssh_auth_list(m_session);
  // while loop here is only so break will work. If execution gets
  // to the end of the loop, an exception is thrown.
  while (rc != SSH_AUTH_SUCCESS) {

    // Try to authenticate with public key first
    if (method & SSH_AUTH_METHOD_PUBLICKEY) {
      rc = ssh_userauth_autopubkey(m_session, m_user.toStdString().c_str());
      if (rc == SSH_AUTH_ERROR) {
        Common::warning("Authentication failed (pubkey)");
        Common::warning(QString("%1").arg(ssh_get_error(m_session)));
        if (throwExceptions) {
          throw SSH_UNKNOWN_ERROR;
        } else {
          return false;
        }
      } else if (rc == SSH_AUTH_SUCCESS) {
        break;
      }
    }

    // Try to authenticate with password
    if (method & SSH_AUTH_METHOD_PASSWORD) {
      rc = ssh_userauth_password(m_session, m_user.toStdString().c_str(),
                                 m_pass.toStdString().c_str());
      if (rc == SSH_AUTH_ERROR) {
        Common::warning("Authentication failed (passwd)");
        Common::warning(QString("%1").arg(ssh_get_error(m_session)));
        if (throwExceptions) {
          throw SSH_UNKNOWN_ERROR;
        } else {
          return false;
        }
      } else if (rc == SSH_AUTH_SUCCESS) {
        break;
      }
    }

    // One of the above should work, else throw an exception
    if (throwExceptions) {
      throw SSH_BAD_PASSWORD_ERROR;
    } else {
      return false;
    }
  }

  // Open shell channel
  if (m_shell) {
    GS_CHANNEL_FREE(m_shell);
    m_shell = 0;
  }

  m_shell = GS_CHANNEL_NEW(m_session);
  if (!m_shell) {
    Common::warning(QString("SSH shell initialization failed: %1")
                   .arg(ssh_get_error(m_session)));
    if (throwExceptions) {
      throw SSH_UNKNOWN_ERROR;
    } else {
      return false;
    }
  }
  if (GS_CHANNEL_OPEN_SESSION(m_shell) != SSH_OK) {
    Common::warning(QString("SSH shell open failed: %1")
                   .arg(ssh_get_error(m_session)));
    if (throwExceptions) {
      throw SSH_UNKNOWN_ERROR;
    } else {
      return false;
    }
  }
  if (GS_CHANNEL_REQUEST_SHELL(m_shell) != SSH_OK) {
    Common::warning(QString("SSH shell request failed: %1")
                   .arg(ssh_get_error(m_session)));
    if (throwExceptions) {
      throw SSH_UNKNOWN_ERROR;
    } else {
      return false;
    }
  }

  // Create the SFTP channel.
  if (m_sftp) {
    sftp_free(m_sftp);
    m_sftp = 0;
  }
  m_sftp = _openSFTP();
  if (!m_sftp) {
    Common::warning("Could not create sftp channel.");
    if (throwExceptions)
      throw SSH_UNKNOWN_ERROR;
    return false;
  }

  m_isValid = true;
  END;
  return true;
}

bool SSHConnectionLibSSH::execute(const QString& command, QString& stdout_str,
                                  QString& stderr_str, int& exitcode,
                                  bool printWarning, int timeoutMs,
                                  const std::atomic<bool>* cancel)
{
  QtCompat::MutexLocker locker(&m_lock);
  return _execute(command, stdout_str, stderr_str, exitcode, printWarning,
                  timeoutMs, cancel);
}

// No need to document this:
/// @cond
bool SSHConnectionLibSSH::_execute(const QString& command, QString& stdout_str,
                                   QString& stderr_str, int& exitcode,
                                   bool printWarning, int timeoutMs,
                                   const std::atomic<bool>* cancel)
{
  START;
#ifdef SSH_CONNECTION_LIBSSH_DEBUG
  Common::debug(QString("The following command is being executed: %1")
               .arg(command));
#endif
  // Open new channel for exec
  ssh_channel channel = GS_CHANNEL_NEW(m_session);
  if (!channel) {
    if (printWarning)
      Common::warning(QString("SSH failure: %1").arg(ssh_get_error(m_session)));
    return false;
  }
  if (GS_CHANNEL_OPEN_SESSION(channel) != SSH_OK) {
    if (printWarning)
      Common::warning(QString("SSH failure: %1").arg(ssh_get_error(m_session)));
    GS_CHANNEL_FREE(channel);
    return false;
  }

  // Execute command
  int ssh_exit = GS_CHANNEL_REQUEST_EXEC(channel, command.toStdString().c_str());

  if (ssh_exit != SSH_OK) {
    GS_CHANNEL_CLOSE(channel);
    GS_CHANNEL_FREE(channel);
    return false;
  }

  // Create string streams
  ostringstream ossout, osserr;

  // Read both streams while the command is running.
  char buffer[LIBSSH_BUFFER_SIZE];
  QElapsedTimer timer;
  timer.start();
  bool readFailed = false;
  bool cancelled = false;
  while (!GS_CHANNEL_IS_EOF(channel) && !GS_CHANNEL_IS_CLOSED(channel)) {
    bool readData = false;
    int len = GS_CHANNEL_READ_NONBLOCKING(channel, buffer, sizeof(buffer), 0);
    if (len == SSH_ERROR) {
      readFailed = true;
      break;
    }
    if (len > 0) {
      ossout.write(buffer, len);
      readData = true;
    }

    len = GS_CHANNEL_READ_NONBLOCKING(channel, buffer, sizeof(buffer), 1);
    if (len == SSH_ERROR) {
      readFailed = true;
      break;
    }
    if (len > 0) {
      osserr.write(buffer, len);
      readData = true;
    }

    if ((cancel && cancel->load()) ||
        (timeoutMs >= 0 && timer.elapsed() >= timeoutMs)) {
      cancelled = true;
      break;
    }
    if (!readData)
      QThread::msleep(10);
  }
  stdout_str = QString::fromStdString(ossout.str());
  stderr_str = QString::fromStdString(osserr.str());

  if (readFailed || cancelled) {
    if (printWarning) {
      Common::warning(cancelled
                        ? "SSH command was cancelled or timed out."
                        : QString("SSH failure while reading command output: %1").arg(ssh_get_error(m_session)));
    }
    GS_CHANNEL_CLOSE(channel);
    GS_CHANNEL_FREE(channel);
    return false;
  }

  GS_CHANNEL_SEND_EOF(channel);

  // 15 iterations, one second sleep each: ~15 second timeout
  exitcode = -1;
  int timeout = 15;
  int exitStatusResult = getChannelExitStatus(channel, exitcode);
  while (exitStatusResult == SSH_AGAIN && timeout > 0) {
#ifdef SSH_CONNECTION_LIBSSH_DEBUG
    Common::debug("Waiting for server to close channel...");
#endif
    if ((cancel && cancel->load()) || (timeoutMs >= 0 && timer.elapsed() >= timeoutMs))
      break;
    GS_SLEEP(1);
    timeout--;
    exitStatusResult = getChannelExitStatus(channel, exitcode);
  }
  if (exitStatusResult != SSH_OK) {
    if (printWarning) {
      Common::warning(exitStatusResult == SSH_AGAIN
                        ? "SSH failure: timed out waiting for command exit status."
                        : QString("SSH failure while reading command exit status: %1")
                            .arg(ssh_get_error(m_session)));
    }
    GS_CHANNEL_CLOSE(channel);
    GS_CHANNEL_FREE(channel);
    return false;
  }

  GS_CHANNEL_CLOSE(channel);
  GS_CHANNEL_FREE(channel);
  END;
  return true;
}
/// @endcond

// No need to document this:
/// @cond
sftp_session SSHConnectionLibSSH::_openSFTP()
{
  sftp_session sftp = sftp_new(m_session);
  if (!sftp) {
    Common::warning(QString("SFTP channel initialization failed\n%1")
                   .arg(ssh_get_error(m_session)));
    return 0;
  }
  if (sftp_init(sftp) != SSH_OK) {
    Common::warning(QString("SFTP initialization failed\n%1\n%2")
                   .arg(ssh_get_error(m_session))
                   .arg(sftp_get_error(sftp)));
    sftp_free(sftp);
    return 0;
  }
  return sftp;
}
/// @endcond

bool SSHConnectionLibSSH::copyFileToServer(const QString& localpath,
                                           const QString& remotepath)
{
  QtCompat::MutexLocker locker(&m_lock);
  if (localpath.trimmed().isEmpty() || remotepath.trimmed().isEmpty()) {
    Common::warning(QString("Refusing to copy to/from empty path: '%1' to '%2'")
                   .arg(localpath, remotepath));
    return false;
  }
  return _copyFileToServer(localpath, remotepath);
}

// No need to document this:
/// @cond
bool SSHConnectionLibSSH::_copyFileToServer(const QString& localpath,
                                            const QString& remotepath)
{
  START;
#ifdef SSH_CONNECTION_LIBSSH_DEBUG
  Common::debug(QString("copying %1 to %2").arg(localpath, remotepath));
#endif

  sftp_session sftp = m_sftp;
  if (!sftp) {
    Common::warning("Could not create sftp channel.");
    return false;
  }

  QFile from(localpath);
  if (!from.open(QIODevice::ReadOnly)) {
    Common::warning(QString("Could not open file %1 for reading.").arg(localpath));
    return false;
  }

  // Create output file handle
  sftp_file to = sftp_open(sftp, sftpPath(remotepath).toStdString().c_str(),
                           O_WRONLY | O_CREAT | O_TRUNC, 0750);
  if (!to) {
    Common::warning(QString("Could not open file %1 for writing.").arg(remotepath));
    return false;
  }

  std::vector<char> buffer(LIBSSH_BUFFER_SIZE);

  qint64 readBytes;
  while ((readBytes = from.read(buffer.data(), static_cast<qint64>(buffer.size()))) > 0) {
    if (sftp_write(to, buffer.data(), readBytes) != readBytes) {
      Common::warning(QString("Could not write to %1").arg(remotepath));
      from.close();
      sftp_close(to);
      return false;
    }
  }
  if (readBytes < 0) {
    Common::warning(QString("Could not read from %1: %2")
                    .arg(localpath)
                    .arg(from.errorString()));
    from.close();
    sftp_close(to);
    return false;
  }
  from.close();
  sftp_close(to);
  END;
  return true;
}
/// @endcond

bool SSHConnectionLibSSH::copyFileFromServer(const QString& remotepath,
                                             const QString& localpath)
{
  QtCompat::MutexLocker locker(&m_lock);
  if (localpath.trimmed().isEmpty() || remotepath.trimmed().isEmpty()) {
    Common::warning(QString("Refusing to copy to/from empty path: '%2' to '%1'")
                   .arg(localpath, remotepath));
    return false;
  }
  return _copyFileFromServer(remotepath, localpath);
}

// No need to document this:
/// @cond
bool SSHConnectionLibSSH::_copyFileFromServer(const QString& remotepath,
                                              const QString& localpath)
{
  START;
#ifdef SSH_CONNECTION_LIBSSH_DEBUG
  Common::debug(QString("copying %1 to %2").arg(remotepath, localpath));
#endif
  sftp_session sftp = m_sftp;
  if (!sftp) {
    Common::warning("Could not create sftp channel.");
    return false;
  }

  // Open remote file
  sftp_file from =
    sftp_open(sftp, sftpPath(remotepath).toStdString().c_str(), O_RDONLY, 0);
  if (!from) {
    Common::warning(QString("Could not open file %1 for reading.").arg(remotepath));
    return false;
  }

  QFile to(localpath);
  if (!to.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    Common::warning(QString("Could not open file %1 for writing.").arg(localpath));
    sftp_close(from);
    return false;
  }

  std::vector<char> buffer(LIBSSH_BUFFER_SIZE);

  int readBytes;
  while ((readBytes = sftp_read(from, buffer.data(), static_cast<uint32_t>(buffer.size()))) > 0) {
    if (to.write(buffer.data(), readBytes) != readBytes) {
      Common::warning(QString("Could not write to %1").arg(localpath));
      to.close();
      sftp_close(from);
      return false;
    }
  }
  if (readBytes < 0) {
    Common::warning(QString("Could not read from remote file %1").arg(remotepath));
    to.close();
    sftp_close(from);
    return false;
  }
  to.close();
  sftp_close(from);
  END;
  return true;
}
/// @endcond

bool SSHConnectionLibSSH::readRemoteFile(const QString& filename,
                                         QString& contents)
{
  QtCompat::MutexLocker locker(&m_lock);
  if (filename.trimmed().isEmpty()) {
    Common::warning(QString("Refusing to read empty filename: '%1'")
                   .arg(filename));
    return false;
  }
  return _readRemoteFile(filename, contents);
}

// No need to document this:
/// @cond
bool SSHConnectionLibSSH::_readRemoteFile(const QString& filename,
                                          QString& contents)
{
  START;
#ifdef SSH_CONNECTION_LIBSSH_DEBUG
  Common::debug(QString("reading %1").arg(filename));
#endif

  sftp_session sftp = m_sftp;
  if (!sftp) {
    Common::warning("Could not create sftp channel.");
    return false;
  }

  // Open remote file
  sftp_file from = sftp_open(sftp, sftpPath(filename).toStdString().c_str(), O_RDONLY, 0);
  if (!from) {
    Common::warning(QString("Could not open file %1 for reading.").arg(filename));
    return false;
  }

  std::vector<char> buffer(LIBSSH_BUFFER_SIZE);

  // Setup output stringstream
  ostringstream oss;

  int readBytes;
  while ((readBytes = sftp_read(from, buffer.data(), static_cast<uint32_t>(buffer.size()))) > 0) {
    oss.write(buffer.data(), readBytes);
  }
  if (readBytes < 0) {
    Common::warning(QString("Could not read remote file %1").arg(filename));
    sftp_close(from);
    return false;
  }
  sftp_close(from);
  const std::string data = oss.str();
  contents = QString::fromLocal8Bit(data.data(), static_cast<int>(data.size()));
  END;
  return true;
}
/// @endcond

bool SSHConnectionLibSSH::removeRemoteFile(const QString& filename)
{
  QtCompat::MutexLocker locker(&m_lock);
  if (filename.trimmed().isEmpty()) {
    Common::warning(QString("Refusing to remove empty filename: '%1'")
                   .arg(filename));
    return false;
  }
  return _removeRemoteFile(filename);
}

// No need to document this:
/// @cond
bool SSHConnectionLibSSH::_removeRemoteFile(const QString& filename)
{
  START;
#ifdef SSH_CONNECTION_LIBSSH_DEBUG
  Common::debug(QString("Removing remote file: %1").arg(filename));
#endif
  sftp_session sftp = m_sftp;
  if (!sftp) {
    Common::warning("Could not create sftp channel.");
    return false;
  }

  if (sftp_unlink(sftp, sftpPath(filename).toStdString().c_str()) != 0) {
    Common::warning(QString("Could not remove remote file %1").arg(filename));
    return false;
  }
  END;
  return true;
}
/// @endcond

bool SSHConnectionLibSSH::copyDirectoryToServer(const QString& local,
                                                const QString& remote)
{
  QtCompat::MutexLocker locker(&m_lock);
  if (local.trimmed().isEmpty() || remote.trimmed().isEmpty()) {
    Common::warning(QString("Refusing to copy to/from empty path: '%1' to '%2'")
                   .arg(local, remote));
    return false;
  }
  return _copyDirectoryToServer(local, remote);
}

// No need to document this:
/// @cond
bool SSHConnectionLibSSH::_copyDirectoryToServer(const QString& local,
                                                 const QString& remote)
{
  START;
#ifdef SSH_CONNECTION_LIBSSH_DEBUG
  Common::debug(QString("copying %1 to %2").arg(local, remote));
#endif

  QString localpath = local;
  QString remotepath = remote;

  // Open local dir
  QDir locdir(localpath);
  if (!locdir.exists()) {
    Common::warning(QString("Could not open local directory %1").arg(localpath));
    return false;
  }

  // Get listing of all items to copy
  QStringList directories =
    locdir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot, QDir::Name);
  QStringList files = locdir.entryList(QDir::Files, QDir::Name);

  sftp_session sftp = m_sftp;
  if (!sftp) {
    Common::warning("Could not create sftp channel.");
    return false;
  }

  // Create remote directory:
  sftp_mkdir(sftp, sftpPath(remotepath).toStdString().c_str(), 0750);

  // Recurse over directories and files (depth-first)
  for (auto dir = directories.begin(); dir != directories.end(); dir++) {
    const QString localDir = Common::localPath(localpath, *dir);
    const QString remoteDir = Common::remotePath(remotepath, *dir);
    if (!_copyDirectoryToServer(localDir, remoteDir)) {
      Common::warning(QString("Could not copy %1 to %2")
                     .arg(localDir, remoteDir));
      return false;
    }
  }
  for (auto file = files.begin(); file != files.end(); file++) {
    const QString localFile = Common::localPath(localpath, *file);
    const QString remoteFile = Common::remotePath(remotepath, *file);
    if (!_copyFileToServer(localFile, remoteFile)) {
      Common::warning(QString("Could not copy %1 to %2")
                     .arg(localFile, remoteFile));
      return false;
    }
  }
  END;
  return true;
}
/// @endcond

bool SSHConnectionLibSSH::copyDirectoryFromServer(const QString& remote,
                                                  const QString& local)
{
  QtCompat::MutexLocker locker(&m_lock);
  if (local.trimmed().isEmpty() || remote.trimmed().isEmpty()) {
    Common::warning(QString("Refusing to copy to/from empty path: '%2' to '%1'")
                   .arg(local, remote));
    return false;
  }
  return _copyDirectoryFromServer(remote, local);
}

// No need to document this:
/// @cond
bool SSHConnectionLibSSH::_copyDirectoryFromServer(const QString& remote,
                                                   const QString& local)
{
  START;
#ifdef SSH_CONNECTION_LIBSSH_DEBUG
  Common::debug(QString("copying %1 to %2").arg(remote, local));
#endif
  QString localpath = local;
  QString remotepath = remote;

  sftp_dir dir;
  sftp_attributes file;
  sftp_session sftp = m_sftp;
  if (!sftp) {
    Common::warning("Could not create sftp channel.");
    return false;
  }

  // Open remote directory
  dir = sftp_opendir(sftp, sftpPath(remotepath).toStdString().c_str());
  if (!dir) {
    Common::warning(QString("Could not open remote directory %1:\n\t%2")
                   .arg(remotepath)
                   .arg(ssh_get_error(m_session)));
    return false;
  }

  // Create local directory
  QDir locdir;
  if (!locdir.mkpath(localpath)) {
    Common::warning(QString("Could not create local directory %1").arg(localpath));
    sftp_closedir(dir);
    return false;
  }

  // Handle each object in the directory:
  while ((file = sftp_readdir(sftp, dir))) {
    if (strcmp(file->name, ".") == 0 || strcmp(file->name, "..") == 0) {
      sftp_attributes_free(file);
      continue;
    }

    switch (file->type) {
      case SSH_FILEXFER_TYPE_DIRECTORY:
      {
        const QString remoteDir = Common::remotePath(remotepath, file->name);
        const QString localDir = Common::localPath(localpath, file->name);
        if (!_copyDirectoryFromServer(remoteDir, localDir)) {
          sftp_attributes_free(file);
          sftp_closedir(dir);
          return false;
        }
        break;
      }
      default:
      {
        const QString remoteFile = Common::remotePath(remotepath, file->name);
        const QString localFile = Common::localPath(localpath, file->name);
        if (!_copyFileFromServer(remoteFile, localFile)) {
          sftp_attributes_free(file);
          sftp_closedir(dir);
          return false;
        }
        break;
      }
    }
    sftp_attributes_free(file);
  }

  // Check for errors
  const bool dirEof = sftp_dir_eof(dir);
  const int closeStatus = sftp_closedir(dir);
  if (!dirEof || closeStatus == SSH_ERROR) {
    Common::warning(QString("Could not copy '%1' to '%2': %3")
                   .arg(remotepath)
                   .arg(localpath)
                   .arg(ssh_get_error(m_session)));
    return false;
  }
  END;
  return true;
}
/// @endcond

bool SSHConnectionLibSSH::readRemoteDirectoryContents(const QString& path,
                                                      QStringList& contents)
{
  QtCompat::MutexLocker locker(&m_lock);
  if (path.trimmed().isEmpty()) {
    Common::warning(QString("Refusing to read empty path contents: '%1'")
                   .arg(path));
    return false;
  }
  return _readRemoteDirectoryContents(path, contents);
}

// No need to document this:
/// @cond
bool SSHConnectionLibSSH::_readRemoteDirectoryContents(const QString& path,
                                                       QStringList& contents)
{
  START;
#ifdef SSH_CONNECTION_LIBSSH_DEBUG
  Common::debug(QString("Reading remote directory contents of: %1").arg(path));
#endif

  QString remotepath = path;
  sftp_dir dir;
  sftp_attributes file;
  contents.clear();

  sftp_session sftp = m_sftp;
  if (!sftp) {
    Common::warning("Could not create sftp channel.");
    return false;
  }

  // Open remote directory
  dir = sftp_opendir(sftp, sftpPath(remotepath).toStdString().c_str());
  if (!dir) {
    Common::warning(QString("Could not open remote directory %1:\n\t%2")
                   .arg(remotepath)
                   .arg(ssh_get_error(m_session)));
    return false;
  }

  // Handle each object in the directory:
  QStringList tmp;
  while ((file = sftp_readdir(sftp, dir))) {
    if (strcmp(file->name, ".") == 0 || strcmp(file->name, "..") == 0) {
      sftp_attributes_free(file);
      continue;
    }
    switch (file->type) {
      case SSH_FILEXFER_TYPE_DIRECTORY:
      {
        const QString entryPath = Common::remotePath(remotepath, file->name);
        contents << entryPath;
        if (!_readRemoteDirectoryContents(entryPath, tmp)) {
          sftp_attributes_free(file);
          sftp_closedir(dir);
          return false;
        }
        contents << tmp;
        break;
      }
      default:
        contents << Common::remotePath(remotepath, file->name);
        break;
    }
    sftp_attributes_free(file);
  }

  // Check for errors
  const bool dirEof = sftp_dir_eof(dir);
  const int closeStatus = sftp_closedir(dir);
  if (!dirEof || closeStatus == SSH_ERROR) {
    Common::warning(QString("Could not read contents of '%1': %2")
                   .arg(remotepath)
                   .arg(ssh_get_error(m_session)));
    return false;
  }
  END;
  return true;
}
/// @endcond

bool SSHConnectionLibSSH::removeRemoteDirectory(const QString& path,
                                                bool onlyDeleteContents)
{
  QtCompat::MutexLocker locker(&m_lock);
  if (path.trimmed().isEmpty() || path.trimmed() == "/") {
    Common::warning(QString("Refusing to remove path: '%1'").arg(path));
    return false;
  }
  return _removeRemoteDirectory(path, onlyDeleteContents);
}

// No need to document this:
/// @cond
bool SSHConnectionLibSSH::_removeRemoteDirectory(const QString& path,
                                                 bool onlyDeleteContents)
{
  START;
#ifdef SSH_CONNECTION_LIBSSH_DEBUG
  Common::debug(QString("Removing remote directory: %1").arg(path));
#endif

  QString remotepath = path;
  sftp_dir dir;
  sftp_attributes file;
  bool ok = true;

  sftp_session sftp = m_sftp;
  if (!sftp) {
    Common::warning("Could not create sftp channel.");
    return false;
  }

  // Open remote directory
  dir = sftp_opendir(sftp, sftpPath(remotepath).toStdString().c_str());
  if (!dir) {
    Common::warning(QString("Could not open remote directory %1:\n\t%2")
                   .arg(remotepath)
                   .arg(ssh_get_error(m_session)));
    return false;
  }

  // Handle each object in the directory:
  while ((file = sftp_readdir(sftp, dir))) {
    if (strcmp(file->name, ".") == 0 || strcmp(file->name, "..") == 0) {
      sftp_attributes_free(file);
      continue;
    }
    switch (file->type) {
      case SSH_FILEXFER_TYPE_DIRECTORY:
      {
        const QString entryPath = Common::remotePath(remotepath, file->name);
        if (!_removeRemoteDirectory(entryPath, false)) {
          Common::warning(QString("Could not remove remote directory %1")
                         .arg(entryPath));
          ok = false;
        }
        break;
      }
      default:
      {
        const QString entryPath = Common::remotePath(remotepath, file->name);
        if (!_removeRemoteFile(entryPath)) {
          Common::warning(QString("Could not remove remote file: %1")
                         .arg(entryPath));
          ok = false;
        }
        break;
      }
    }
    sftp_attributes_free(file);
  }

  // Check for errors
  const bool dirEof = sftp_dir_eof(dir);
  const int closeStatus = sftp_closedir(dir);
  if (!dirEof || closeStatus == SSH_ERROR) {
    Common::warning(QString("Could not read contents of '%1': %2")
                   .arg(remotepath)
                   .arg(ssh_get_error(m_session)));
    return false;
  }
  if (!ok) {
    Common::warning(QString("Some files could not be removed from %1")
                   .arg(remotepath));
    return false;
  }

  // Finally remove directory if asked
  if (!onlyDeleteContents) {
    if (sftp_rmdir(sftp, sftpPath(remotepath).toStdString().c_str()) == SSH_ERROR) {
      Common::warning(QString("Could not remove remote directory %1: %2")
                     .arg(remotepath)
                     .arg(ssh_get_error(m_session)));
      return false;
    }
  }
  END;
  return true;
}
/// @endcond

bool SSHConnectionLibSSH::addKeyToKnownHosts(const QString& host,
                                             unsigned int port)
{
  // Create session
  ssh_session session = ssh_new();
  if (!session) {
    return false;
  }

  // Set options
  int verbosity = SSH_LOG_NOLOG;
  int timeout = 15; // timeout in sec

  ssh_options_set(session, SSH_OPTIONS_HOST, host.toStdString().c_str());
  ssh_options_set(session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
  ssh_options_set(session, SSH_OPTIONS_TIMEOUT, &timeout);
  ssh_options_set(session, SSH_OPTIONS_PORT, &port);

  // Connect
  if (ssh_connect(session) != SSH_OK) {
    Common::warning(QString("SSH failure: %1").arg(ssh_get_error(session)));
    ssh_free(session);
    return false;
  }

#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 8, 0)
  if (ssh_session_update_known_hosts(session) < 0) {
#else
  if (ssh_write_knownhost(session) < 0) {
#endif
    ssh_free(session);
    return false;
  }

  ssh_free(session);
  return true;
}

} // end namespace Search

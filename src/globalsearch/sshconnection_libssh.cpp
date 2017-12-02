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

#ifdef ENABLE_SSH

#include <globalsearch/sshconnection_libssh.h>

#include <globalsearch/macros.h>
#include <globalsearch/sshmanager_libssh.h>

#include <QDebug>
#include <QDir>

#include <fcntl.h>
#include <fstream>
#include <sstream>

using namespace std;

namespace GlobalSearch {

#define START // qDebug() << __FUNCTION__ << " called...";
#define END   // qDebug() << __FUNCTION__ << " finished...";

//#define SSH_CONNECTION_LIBSSH_DEBUG

SSHConnectionLibSSH::SSHConnectionLibSSH(SSHManagerLibSSH* parent)
  : SSHConnection(parent), m_session(0), m_shell(0), m_sftp(0),
    m_sftpTimeStamp(QDateTime::currentDateTime()), m_isValid(false),
    m_inUse(false)
{
  if (parent) {
    // block this connection so that a thrown exception won't cause problems
    connect(this, SIGNAL(unknownHostKey(const QString&)), parent,
            SLOT(setServerKey(const QString&)));
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
  if (!m_session || !m_shell || !m_sftp || channel_is_closed(m_shell) ||
      channel_is_eof(m_shell)) {
    qWarning() << "SSHConnectionLibSSH is not connected: one or more required "
                  "channels are not initialized.";
    return false;
  };

  if (!ssh_is_connected(m_session)) {
    qWarning() << "SSHConnectionLibSSH is not connected.";
    return false;
  }

  START;

  // Check to see if sftp is still connected. If it is not,
  // then reconnect it. It will sometimes disconnect...
  if (!reconnectSftpIfNeeded()) {
    qWarning() << "In SSHConnectionLibSSH::isConnected(),"
               << "reconnectSftpIfNeeded() failed. Reconnecting session.";
    return false;
  }

  END;
  return true;
}

bool SSHConnectionLibSSH::disconnectSession()
{
  QMutexLocker locker(&m_lock);
  START;

  if (m_sftp)
    sftp_free(m_sftp);
  m_sftp = 0;

  if (m_shell)
    channel_free(m_shell);
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
  if (err == 4 || err == 6 || err == 7)
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
    qWarning() << "Could not create sftp channel.";
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
      qWarning() << "SSHConnectionLibSSH::reconnectSftpIfNeeded():"
                 << "reconnectSftp() failed";
      return false;
    }
    // reset time stamp
    m_sftpTimeStamp = QDateTime::currentDateTime();
  }
  // Check to see if 240 seconds have passed since the last reconnection
  // if it has, reconnect the sftp
  auto timeDifference = m_sftpTimeStamp.msecsTo(QDateTime::currentDateTime());

  // reconnect the sftp every 240 seconds
  auto interval = 240;
  if (timeDifference >= interval * 1000) {
#ifdef SSH_CONNECTION_LIBSSH_DEBUG
    qDebug() << "timeDifference is " << QString::number(timeDifference);
#endif
    bool success = reconnectSftp();
    if (!success) {
      qWarning() << "SSHConnectionLibSSH::reconnectSftpIfNeeded():"
                 << "reconnectSftp() failed.";
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
  QMutexLocker locker(&m_lock);
  // Create session
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
    qWarning() << "SSH error: " << ssh_get_error(m_session);
    if (throwExceptions) {
      throw SSH_CONNECTION_ERROR;
    } else {
      return false;
    }
  }

  // Verify that host is known
  int state = ssh_is_server_known(m_session);
  switch (state) {
    case SSH_SERVER_KNOWN_OK:
      break;
    case SSH_SERVER_KNOWN_CHANGED:
    case SSH_SERVER_FOUND_OTHER:
    case SSH_SERVER_FILE_NOT_FOUND:
    case SSH_SERVER_NOT_KNOWN: {
      size_t hlen;
      unsigned char* hash = 0;
      ssh_key key;
      ssh_get_publickey(m_session, &key);
      ssh_get_publickey_hash(key, SSH_PUBLICKEY_HASH_SHA1, &hash, &hlen);

      char* hexa;
      hexa = ssh_get_hexa(hash, hlen);
      emit unknownHostKey(QString(hexa));

      // Cleanup time
      ssh_clean_pubkey_hash(&hash);
      ssh_key_free(key);

      if (throwExceptions) {
        throw SSH_UNKNOWN_HOST_ERROR;
      } else {
        return false;
      }
    }
    case SSH_SERVER_ERROR:
      qWarning() << "SSH error: " << ssh_get_error(m_session);
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
    qWarning() << "SSH error: " << ssh_get_error(m_session);
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
        qWarning() << "Error during auth (pubkey)";
        qWarning() << "Error: " << ssh_get_error(m_session);
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
        qWarning() << "Error during auth (passwd)";
        qWarning() << "Error: " << ssh_get_error(m_session);
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
    channel_free(m_shell);
    m_shell = 0;
  }

  m_shell = channel_new(m_session);
  if (!m_shell) {
    qWarning() << "SSH error initializing shell: " << ssh_get_error(m_session);
    if (throwExceptions) {
      throw SSH_UNKNOWN_ERROR;
    } else {
      return false;
    }
  }
  if (channel_open_session(m_shell) != SSH_OK) {
    qWarning() << "SSH error opening shell: " << ssh_get_error(m_session);
    if (throwExceptions) {
      throw SSH_UNKNOWN_ERROR;
    } else {
      return false;
    }
  }
  if (channel_request_shell(m_shell) != SSH_OK) {
    qWarning() << "SSH error requesting shell: " << ssh_get_error(m_session);
    if (throwExceptions) {
      throw SSH_UNKNOWN_ERROR;
    } else {
      return false;
    }
  }

  m_sftp = _openSFTP();
  if (!m_sftp) {
    qWarning() << "Could not create sftp channel.";
    return false;
  }

  m_isValid = true;
  END;
  return true;
}

bool SSHConnectionLibSSH::execute(const QString& command, QString& stdout_str,
                                  QString& stderr_str, int& exitcode,
                                  bool printWarning)
{
  QMutexLocker locker(&m_lock);
  return _execute(command, stdout_str, stderr_str, exitcode, printWarning);
}

// No need to document this:
/// @cond
bool SSHConnectionLibSSH::_execute(const QString& command, QString& stdout_str,
                                   QString& stderr_str, int& exitcode,
                                   bool printWarning)
{
  START;
#ifdef SSH_CONNECTION_LIBSSH_DEBUG
  qDebug() << "The following command is being executed:" << command;
#endif
  // Open new channel for exec
  ssh_channel channel = channel_new(m_session);
  if (!channel) {
    if (printWarning)
      qWarning() << "SSH error: " << ssh_get_error(m_session);
    return false;
  }
  if (channel_open_session(channel) != SSH_OK) {
    if (printWarning)
      qWarning() << "SSH error: " << ssh_get_error(m_session);
    channel_free(channel);
    return false;
  }

  // Execute command
  int ssh_exit = channel_request_exec(channel, command.toStdString().c_str());

  if (ssh_exit != SSH_OK) {
    channel_close(channel);
    channel_free(channel);
    return false;
  }

  // Create string streams
  ostringstream ossout, osserr;

  // Read output
  char buffer[LIBSSH_BUFFER_SIZE];
  int len;
  while ((len = channel_read(channel, buffer, sizeof(buffer), 0)) > 0) {
    ossout.write(buffer, len);
  }
  stdout_str = QString(ossout.str().c_str());
  while ((len = channel_read(channel, buffer, sizeof(buffer), 1)) > 0) {
    osserr.write(buffer, len);
  }
  stderr_str = QString(osserr.str().c_str());

  channel_send_eof(channel);
  channel_close(channel);

  // 5 second timeout
  int timeout = 15;
  while (channel_get_exit_status(channel) == -1 && timeout >= 0) {
#ifdef SSH_CONNECTION_LIBSSH_DEBUG
    qDebug() << "Waiting for server to close channel...";
#endif
    GS_SLEEP(1);
    timeout--;
  }

  exitcode = channel_get_exit_status(channel);

  channel_free(channel);
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
    qWarning() << "sftp error initialising channel" << endl
               << ssh_get_error(m_session);
    return 0;
  }
  if (sftp_init(sftp) != SSH_OK) {
    qWarning() << "error initialising sftp" << endl
               << ssh_get_error(m_session) << endl
               << sftp_get_error(sftp);
    sftp_free(sftp);
    return 0;
  }
  return sftp;
}
/// @endcond

bool SSHConnectionLibSSH::copyFileToServer(const QString& localpath,
                                           const QString& remotepath)
{
  QMutexLocker locker(&m_lock);
  if (localpath.trimmed().isEmpty() || remotepath.trimmed().isEmpty()) {
    qWarning() << QString("Refusing to copy to/from empty path: '%1' to '%2'")
                    .arg(localpath, remotepath);
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
  qDebug() << "copying" << localpath << "to" << remotepath;
#endif

  sftp_session sftp = m_sftp;
  if (!sftp) {
    qWarning() << "Could not create sftp channel.";
    return false;
  }

  // Create input file handle
  ifstream from(localpath.toStdString().c_str());
  if (!from.is_open()) {
    qWarning() << "Error opening file " << localpath << " for reading.";
    return false;
  }

  // Create output file handle
  sftp_file to = sftp_open(sftp, remotepath.toStdString().c_str(),
                           O_WRONLY | O_CREAT | O_TRUNC, 0750);
  if (!to) {
    qWarning() << "Error opening file " << remotepath << " for writing.";
    return false;
  }

  // Create buffer
  int size = LIBSSH_BUFFER_SIZE;
  char* buffer = new char[size];

  int readBytes;
  while (!from.eof()) {
    from.read(buffer, size);
    readBytes = from.gcount();
    if (sftp_write(to, buffer, readBytes) != readBytes) {
      qWarning() << "Error writing to " << remotepath;
      from.close();
      sftp_close(to);
      delete[] buffer;
      return false;
    }
  }
  from.close();
  sftp_close(to);
  delete[] buffer;
  END;
  return true;
}
/// @endcond

bool SSHConnectionLibSSH::copyFileFromServer(const QString& remotepath,
                                             const QString& localpath)
{
  QMutexLocker locker(&m_lock);
  if (localpath.trimmed().isEmpty() || remotepath.trimmed().isEmpty()) {
    qWarning() << QString("Refusing to copy to/from empty path: '%2' to '%1'")
                    .arg(localpath, remotepath);
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
  qDebug() << "copying" << remotepath << "to" << localpath;
#endif
  sftp_session sftp = m_sftp;
  if (!sftp) {
    qWarning() << "Could not create sftp channel.";
    return false;
  }

  // Open remote file
  sftp_file from =
    sftp_open(sftp, remotepath.toStdString().c_str(), O_RDONLY, 0);
  if (!from) {
    qWarning() << "Error opening file " << remotepath << " for reading.";
    return false;
  }

  // Open local file
  ofstream to(localpath.toStdString().c_str());
  if (!to.is_open()) {
    qWarning() << "Error opening file " << localpath << " for writing.";
    sftp_close(from);
    return false;
  }

  // Create buffer
  char* buffer = new char[LIBSSH_BUFFER_SIZE];

  int readBytes;
  while ((readBytes = sftp_read(from, buffer, LIBSSH_BUFFER_SIZE)) > 0) {
    to.write(buffer, readBytes);
    if (to.bad()) {
      qWarning() << "Error writing to " << localpath;
      to.close();
      sftp_close(from);
      delete[] buffer;
      return false;
    }
  }
  to.close();
  sftp_close(from);
  delete[] buffer;
  END;
  return true;
}
/// @endcond

bool SSHConnectionLibSSH::readRemoteFile(const QString& filename,
                                         QString& contents)
{
  QMutexLocker locker(&m_lock);
  if (filename.trimmed().isEmpty()) {
    qWarning()
      << QString("Refusing to read empty filename: '%1'").arg(filename);
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
  qDebug() << "reading" << filename;
#endif

  sftp_session sftp = m_sftp;
  if (!sftp) {
    qWarning() << "Could not create sftp channel.";
    return false;
  }

  // Open remote file
  sftp_file from = sftp_open(sftp, filename.toStdString().c_str(), O_RDONLY, 0);
  if (!from) {
    qWarning() << "Error opening file " << filename << " for reading.";
    return false;
  }

  // Create buffer
  char* buffer = new char[LIBSSH_BUFFER_SIZE];

  // Setup output stringstream
  ostringstream oss;

  int readBytes;
  while ((readBytes = sftp_read(from, buffer, LIBSSH_BUFFER_SIZE)) > 0) {
    oss.write(buffer, readBytes);
  }
  sftp_close(from);
  contents = QString(oss.str().c_str());
  delete[] buffer;
  END;
  return true;
}
/// @endcond

bool SSHConnectionLibSSH::removeRemoteFile(const QString& filename)
{
  QMutexLocker locker(&m_lock);
  if (filename.trimmed().isEmpty()) {
    qWarning()
      << QString("Refusing to remove empty filename: '%1'").arg(filename);
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
  qDebug() << "Removing remote file: " << filename;
#endif
  sftp_session sftp = m_sftp;
  if (!sftp) {
    qWarning() << "Could not create sftp channel.";
    return false;
  }

  if (sftp_unlink(sftp, filename.toStdString().c_str()) != 0) {
    qWarning() << "Error removing remote file " << filename;
    return false;
  }
  END;
  return true;
}
/// @endcond

bool SSHConnectionLibSSH::copyDirectoryToServer(const QString& local,
                                                const QString& remote)
{
  QMutexLocker locker(&m_lock);
  if (local.trimmed().isEmpty() || remote.trimmed().isEmpty()) {
    qWarning() << QString("Refusing to copy to/from empty path: '%1' to '%2'")
                    .arg(local, remote);
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
  qDebug() << "copying" << local << "to" << remote;
#endif

  // Add trailing slashes:
  QString localpath = local + "/";
  QString remotepath = remote + "/";

  // Open local dir
  QDir locdir(localpath);
  if (!locdir.exists()) {
    qWarning() << "Could not open local directory " << localpath;
    return false;
  }

  // Get listing of all items to copy
  QStringList directories =
    locdir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot, QDir::Name);
  QStringList files = locdir.entryList(QDir::Files, QDir::Name);

  sftp_session sftp = m_sftp;
  if (!sftp) {
    qWarning() << "Could not create sftp channel.";
    return false;
  }

  // Create remote directory:
  sftp_mkdir(sftp, remotepath.toStdString().c_str(), 0750);

  // Recurse over directories and files (depth-first)
  QStringList::const_iterator dir, file;
  for (dir = directories.begin(); dir != directories.end(); dir++) {
    // qDebug() << "Copying " << localpath + (*dir) << " to "
    //          << remotepath + (*dir);
    if (!_copyDirectoryToServer(localpath + (*dir), remotepath + (*dir))) {
      qWarning() << "Error copying " << localpath + (*dir) << " to "
                 << remotepath + (*dir);
      return false;
    }
  }
  for (file = files.begin(); file != files.end(); file++) {
    // qDebug() << "Copying " << localpath + (*file) << " to "
    //            << remotepath + (*file);
    if (!_copyFileToServer(localpath + (*file), remotepath + (*file))) {
      qWarning() << "Error copying " << localpath + (*file) << " to "
                 << remotepath + (*file);
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
  QMutexLocker locker(&m_lock);
  if (local.trimmed().isEmpty() || remote.trimmed().isEmpty()) {
    qWarning() << QString("Refusing to copy to/from empty path: '%2' to '%1'")
                    .arg(local, remote);
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
  qDebug() << "copying" << remote << "to" << local;
#endif
  // Add trailing slashes:
  QString localpath = local + "/";
  QString remotepath = remote + "/";

  sftp_dir dir;
  sftp_attributes file;
  sftp_session sftp = m_sftp;
  if (!sftp) {
    qWarning() << "Could not create sftp channel.";
    return false;
  }

  // Open remote directory
  dir = sftp_opendir(sftp, remotepath.toStdString().c_str());
  if (!dir) {
    qWarning() << "Could not open remote directory " << remotepath << ":\n\t"
               << ssh_get_error(m_session);
    return false;
  }

  // Create local directory
  QDir locdir;
  if (!locdir.mkpath(localpath)) {
    qWarning() << "Could not create local directory " << localpath;
    return false;
  }

  // Handle each object in the directory:
  while ((file = sftp_readdir(sftp, dir))) {
    if (strcmp(file->name, ".") == 0 || strcmp(file->name, "..") == 0) {
      continue;
    }

    switch (file->type) {
      case SSH_FILEXFER_TYPE_DIRECTORY:
        if (!_copyDirectoryFromServer(remotepath + file->name,
                                      localpath + file->name)) {
          sftp_attributes_free(file);
          return false;
        }
        break;
      default:
        if (!_copyFileFromServer(remotepath + file->name,
                                 localpath + file->name)) {
          sftp_attributes_free(file);
          return false;
        }
        break;
    }
    sftp_attributes_free(file);
  }

  // Check for errors
  if (!sftp_dir_eof(dir) && sftp_closedir(dir) == SSH_ERROR) {
    qWarning() << "Error copying \'" << remotepath << "\' to \'" << localpath
               << "\': " << ssh_get_error(m_session);
    return false;
  }
  END;
  return true;
}
/// @endcond

bool SSHConnectionLibSSH::readRemoteDirectoryContents(const QString& path,
                                                      QStringList& contents)
{
  QMutexLocker locker(&m_lock);
  if (path.trimmed().isEmpty()) {
    qWarning()
      << QString("Refusing to read empty path contents: '%1'").arg(path);
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
  qDebug() << "Reading remote directory contents of:" << path;
#endif

  QString remotepath = path + "/";
  sftp_dir dir;
  sftp_attributes file;
  contents.clear();

  sftp_session sftp = m_sftp;
  if (!sftp) {
    qWarning() << "Could not create sftp channel.";
    return false;
  }

  // Open remote directory
  dir = sftp_opendir(sftp, remotepath.toStdString().c_str());
  if (!dir) {
    qWarning() << "Could not open remote directory " << remotepath << ":\n\t"
               << ssh_get_error(m_session);
    return false;
  }

  // Handle each object in the directory:
  QStringList tmp;
  while ((file = sftp_readdir(sftp, dir))) {
    if (strcmp(file->name, ".") == 0 || strcmp(file->name, "..") == 0) {
      continue;
    }
    switch (file->type) {
      case SSH_FILEXFER_TYPE_DIRECTORY:
        contents << remotepath + file->name + "/";
        if (!_readRemoteDirectoryContents(remotepath + file->name, tmp)) {
          sftp_attributes_free(file);
          return false;
        }
        contents << tmp;
        break;
      default:
        contents << remotepath + file->name;
        break;
    }
    sftp_attributes_free(file);
  }

  // Check for errors
  if (!sftp_dir_eof(dir) && sftp_closedir(dir) == SSH_ERROR) {
    qWarning() << "Error reading contents of \'" << remotepath
               << "\': " << ssh_get_error(m_session);
    return false;
  }
  END;
  return true;
}
/// @endcond

bool SSHConnectionLibSSH::removeRemoteDirectory(const QString& path,
                                                bool onlyDeleteContents)
{
  QMutexLocker locker(&m_lock);
  if (path.trimmed().isEmpty()) {
    qWarning() << QString("Refusing to remove empty path: '%1'").arg(path);
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
  qDebug() << "Removing remote directory:" << path;
#endif

  QString remotepath = path + "/";
  sftp_dir dir;
  sftp_attributes file;
  bool ok = true;

  sftp_session sftp = m_sftp;
  if (!sftp) {
    qWarning() << "Could not create sftp channel.";
    return false;
  }

  // Open remote directory
  dir = sftp_opendir(sftp, remotepath.toStdString().c_str());
  if (!dir) {
    qWarning() << "Could not open remote directory " << remotepath << ":\n\t"
               << ssh_get_error(m_session);
    return false;
  }

  // Handle each object in the directory:
  while ((file = sftp_readdir(sftp, dir))) {
    if (strcmp(file->name, ".") == 0 || strcmp(file->name, "..") == 0) {
      continue;
    }
    switch (file->type) {
      case SSH_FILEXFER_TYPE_DIRECTORY:
        if (!_removeRemoteDirectory(remotepath + file->name, false)) {
          qWarning() << "Could not remove remote directory"
                     << remotepath + file->name;
          ok = false;
        }
        break;
      default:
        if (!_removeRemoteFile(remotepath + file->name)) {
          qWarning() << "Could not remove remote file: "
                     << remotepath + file->name;
          ok = false;
        }
        break;
    }
    sftp_attributes_free(file);
  }

  // Check for errors
  if (!sftp_dir_eof(dir) && sftp_closedir(dir) == SSH_ERROR) {
    qWarning() << "Error reading contents of \'" << remotepath
               << "\': " << ssh_get_error(m_session);
    return false;
  }
  if (!ok) {
    qWarning() << "Some files could not be removed from " << remotepath;
    return false;
  }

  // Finally remove directory if asked
  if (!onlyDeleteContents) {
    if (sftp_rmdir(sftp, remotepath.toStdString().c_str()) == SSH_ERROR) {
      qWarning() << "Error removing remote directory " << remotepath << ": "
                 << ssh_get_error(m_session);
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
    qWarning() << "SSH error: " << ssh_get_error(session);
    ssh_free(session);
    return false;
  }

  if (ssh_write_knownhost(session) < 0) {
    ssh_free(session);
    return false;
  }

  ssh_free(session);
  return true;
}

} // end namespace GlobalSearch

#endif // ENABLE_SSH

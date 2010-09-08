/**********************************************************************
  SSHConnection - Connection to an ssh server for execution, sftp, etc.

  Copyright (C) 2010 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <globalsearch/sshconnection.h>

#include <QDebug>
#include <QDir>

#include <fstream>
#include <sstream>
#include <fcntl.h>

using namespace std;

namespace GlobalSearch {

#define START //qDebug() << __PRETTY_FUNCTION__ << " called...";
#define END //qDebug() << __PRETTY_FUNCTION__ << " finished...";

  SSHConnection::SSHConnection(OptBase *parent)
    : QObject(parent),
      m_host(""),
      m_user(""),
      m_pass(""),
      m_port(22),
      m_isValid(false),
      m_inUse(false)
  {
  }

  SSHConnection::~SSHConnection()
  {
    START;
    disconnect();
    END;
  }

  bool SSHConnection::isConnected(ssh_channel channel)
  {
    if (!m_session)
      return false;

    QMutexLocker locker (&m_lock);
    START;
    bool deleteChannel = false;
    if (!channel) {
      deleteChannel = true;
      channel = channel_new(m_session);
      if (!channel) {
        m_isValid = false;
        qWarning() << "SSH error: " << ssh_get_error(m_session);
        return false;
      }
    }

    bool connected = false;
    if (channel_poll(channel, 0) != SSH_ERROR) {
      connected = true;
    }

    if (deleteChannel) {
      channel_free(channel);
    }
    END;
    return connected;
  }

  void SSHConnection::setLoginDetails(const QString &host,
                                      const QString &user,
                                      const QString &pass,
                                      int port)
  {
    m_host = host;
    m_user = user;
    m_pass = pass;
    m_port = port;
  }


  bool SSHConnection::disconnect()
  {
    QMutexLocker locker (&m_lock);
    START;
    if (m_session)
      ssh_free(m_session);
    m_session = 0;
    m_isValid = false;
    END;
  }

  bool SSHConnection::reconnect(bool throwExceptions)
  {
    START;
    disconnect();
    connect(throwExceptions);
    END;
  }

  bool SSHConnection::connect(bool throwExceptions)
  {
    QMutexLocker locker (&m_lock);
    // Create session
    m_session = ssh_new();
    if (!m_session) {
      if (throwExceptions) {
        throw SSH_UNKNOWN_ERROR;
      }
      else {
        return false;
      }
    }

    // Set options
    int verbosity = SSH_LOG_NOLOG;
    //int verbosity = SSH_LOG_PROTOCOL;
    //int verbosity = SSH_LOG_PACKET;
    int timeout = 5; // timeout in sec

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
      }
      else {
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
    case SSH_SERVER_NOT_KNOWN:
      if (throwExceptions) {
        throw SSH_UNKNOWN_HOST_ERROR;
      }
      else {
        return false;
      }
    case SSH_SERVER_ERROR:
      qWarning() << "SSH error: " << ssh_get_error(m_session);
      if (throwExceptions) {
        throw SSH_UNKNOWN_ERROR;
      }
      else {
        return false;
      }
    }

    // Authenticate
    int rc;
    int method;
    char *banner;

    // Try to authenticate
    rc = ssh_userauth_none(m_session, NULL);
    if (rc == SSH_AUTH_ERROR) {
      qWarning() << "SSH error: " << ssh_get_error(m_session);
      if (throwExceptions) {
        throw SSH_UNKNOWN_ERROR;
      }
      else {
        return false;
      }
    }

    method = ssh_auth_list(m_session);
    // while loop here is only so break will work. If execution gets
    // to the end of the loop, an exception is thrown.
    while (rc != SSH_AUTH_SUCCESS) {

      // Try to authenticate with public key first
      if (method & SSH_AUTH_METHOD_PUBLICKEY) {
        rc = ssh_userauth_autopubkey(m_session,
                                     m_user.toStdString().c_str());
        if (rc == SSH_AUTH_ERROR) {
          qWarning() << "Error during auth (pubkey)";
          qWarning() << "Error: " << ssh_get_error(m_session);
          if (throwExceptions) {
            throw SSH_UNKNOWN_ERROR;
          }
          else {
            return false;
          }
        } else if (rc == SSH_AUTH_SUCCESS) {
          break;
        }
      }

      // Try to authenticate with password
      if (method & SSH_AUTH_METHOD_PASSWORD) {
        rc = ssh_userauth_password(m_session,
                                   m_user.toStdString().c_str(),
                                   m_pass.toStdString().c_str());
        if (rc == SSH_AUTH_ERROR) {
          qWarning() << "Error during auth (passwd)";
          qWarning() << "Error: " << ssh_get_error(m_session);
          if (throwExceptions) {
            throw SSH_UNKNOWN_ERROR;
          }
          else {
            return false;
          }
        } else if (rc == SSH_AUTH_SUCCESS) {
          break;
        }
      }

      // One of the above should work, else throw an exception
      if (throwExceptions) {
        throw SSH_BAD_PASSWORD_ERROR;
      }
      else {
        return false;
      }
    }

    m_isValid = true;
    return true;
    END;
  }

  sftp_session SSHConnection::_openSFTP()
  {
    sftp_session sftp = sftp_new(m_session);
    if(!sftp){
      qWarning() << "sftp error initialising channel" << endl
                 << ssh_get_error(m_session);
      return 0;
    }
    if(sftp_init(sftp) != SSH_OK){
      qWarning() << "error initialising sftp" << endl
                 << ssh_get_error(m_session);
      return 0;
    }
    return sftp;
  }

  bool SSHConnection::execute(const QString &command,
                              QString &stdout,
                              QString &stderr,
                              int &exitcode) {
    QMutexLocker locker (&m_lock);
    return _execute(command, stdout, stderr, exitcode);
  }

  bool SSHConnection::_execute(const QString &command,
                               QString &stdout,
                               QString &stderr,
                               int &exitcode)
  {
    START;
    // Open new channel for exec
    ssh_channel channel = channel_new(m_session);
    if (!channel) {
      qWarning() << "SSH error: " << ssh_get_error(m_session);
      return false;
    }
    if (channel_open_session(channel) != SSH_OK) {
      qWarning() << "SSH error: " << ssh_get_error(m_session);
      return false;
    }

    // Execute command
    int ssh_exit = channel_request_exec(channel, command.toStdString().c_str());
    channel_send_eof(channel);

    if (ssh_exit == SSH_ERROR) {
      channel_close(channel);
      return false;
    }

    // Create string streams
    ostringstream ossout, osserr;

    // Read output
    char buffer[SSH_BUFFER_SIZE];
    int len;
    while ((len = channel_read(channel, buffer, sizeof(buffer), 0)) > 0) {
      ossout.write(buffer,len);
    }
    stdout = QString(ossout.str().c_str());
    while ((len = channel_read(channel, buffer, sizeof(buffer), 1)) > 0) {
      osserr.write(buffer,len);
    }
    stderr = QString(osserr.str().c_str());

    exitcode = channel_get_exit_status(channel);

    channel_close(channel);
    channel_free(channel);
    END;
    return true;
  }

  bool SSHConnection::copyFileToServer(const QString & localpath,
                                       const QString & remotepath)
  {
    QMutexLocker locker (&m_lock);
    return _copyFileToServer(localpath, remotepath);
  }

  bool SSHConnection::_copyFileToServer(const QString & localpath,
                                        const QString & remotepath)
  {
    START;
    sftp_session sftp = _openSFTP();
    if (!sftp) {
      qWarning() << "Could not create sftp channel.";
      return false;
    }

    // Create input file handle
    ifstream from (localpath.toStdString().c_str());
    if (!from.is_open()) {
      qWarning() << "Error opening file " << localpath << " for reading.";
      return false;
    }

    // Create output file handle
    sftp_file to = sftp_open(sftp,
                             remotepath.toStdString().c_str(),
                             O_WRONLY | O_CREAT | O_TRUNC,
                             0750);
    if (!to) {
      qWarning() << "Error opening file " << remotepath << " for writing.";
      sftp_free(sftp);
      return false;
    }

    // Create buffer
    int size = SSH_BUFFER_SIZE;
    //int size = 4096;
    char *buffer = new char [size];
    //int size = 4096;
    //char buffer[size];

    int readBytes;
    while (!from.eof()) {
      from.read(buffer, size);
      readBytes = from.gcount();
      if (sftp_write(to, buffer, readBytes) != readBytes) {
        qWarning() << "Error writing to " << remotepath;
        from.close();
        sftp_close(to);
        // TODO Don't forget to uncomment this!
        //delete[] buffer;
        sftp_free(sftp);
        return false;
      }
    }
    from.close();
    sftp_close(to);
    // TODO Don't forget to uncomment this!
    //delete[] buffer;
    sftp_free(sftp);
    END;
    return true;
  }

  bool SSHConnection::copyFileFromServer(const QString & remotepath,
                                         const QString & localpath)
  {
    QMutexLocker locker (&m_lock);
    return _copyFileFromServer(remotepath, localpath);
  }

  bool SSHConnection::_copyFileFromServer(const QString & remotepath,
                                         const QString & localpath)
  {
    START;
    sftp_session sftp = _openSFTP();
    if (!sftp) {
      qWarning() << "Could not create sftp channel.";
      return false;
    }

    // Open remote file
    sftp_file from = sftp_open(sftp,
                               remotepath.toStdString().c_str(),
                               O_RDONLY,
                               0);
    if(!from){
      qWarning() << "Error opening file " << remotepath << " for reading.";
      sftp_free(sftp);
      return false;
    }

    // Open local file
    ofstream to (localpath.toStdString().c_str());
    if (!to.is_open()) {
      qWarning() << "Error opening file " << localpath << " for writing.";
      sftp_close(from);
      sftp_free(sftp);
      return false;
    }

    // Create buffer
    char *buffer = new char [SSH_BUFFER_SIZE];

    int readBytes;
    while ((readBytes = sftp_read(from, buffer, SSH_BUFFER_SIZE)) > 0) {
      to.write(buffer,readBytes);
      if (to.bad()) {
        qWarning() << "Error writing to " << localpath;
        to.close();
        sftp_close(from);
        delete[] buffer;
        sftp_free(sftp);
        return false;
      }
    }
    to.close();
    sftp_close(from);
    delete[] buffer;
    sftp_free(sftp);
    END;
    return true;
  }

  bool SSHConnection::readRemoteFile(const QString &filename,
                                     QString &contents)
  {
    QMutexLocker locker (&m_lock);
    return _readRemoteFile(filename, contents);
  }

  bool SSHConnection::_readRemoteFile(const QString &filename,
                                      QString &contents)
  {
    START;
    sftp_session sftp = _openSFTP();
    if (!sftp) {
      qWarning() << "Could not create sftp channel.";
      return false;
    }

    // Open remote file
    sftp_file from = sftp_open(sftp,
                               filename.toStdString().c_str(),
                               O_RDONLY,
                               0);
    if(!from){
      qWarning() << "Error opening file " << filename << " for reading.";
      sftp_free(sftp);
      return false;
    }

    // Create buffer
    char *buffer = new char [SSH_BUFFER_SIZE];

    // Setup output stringstream
    ostringstream oss;

    int readBytes;
    while ((readBytes = sftp_read(from, buffer, SSH_BUFFER_SIZE)) > 0) {
      oss.write(buffer,readBytes);
    }
    sftp_close(from);
    contents = QString(oss.str().c_str());
    delete[] buffer;
    sftp_free(sftp);
    END;
    return true;
  }

  bool SSHConnection::removeRemoteFile(const QString &filename)
  {
    QMutexLocker locker (&m_lock);
    return _removeRemoteFile(filename);
  }

  bool SSHConnection::_removeRemoteFile(const QString &filename)
  {
    START;
    sftp_session sftp = _openSFTP();
    if (!sftp) {
      qWarning() << "Could not create sftp channel.";
      return false;
    }

    if (sftp_unlink(sftp,
                    filename.toStdString().c_str())
        == 0) {
      sftp_free(sftp);
      END;
      return true;
    }
    qWarning() << "Error removing remote file " << filename;
    sftp_free(sftp);
    return false;
  }

  bool SSHConnection::copyDirectoryToServer(const QString & local,
                                            const QString & remote)
  {
    QMutexLocker locker (&m_lock);
    return _copyDirectoryToServer(local, remote);
  }

  bool SSHConnection::_copyDirectoryToServer(const QString & local,
                                             const QString & remote)
  {
    START;
    sftp_session sftp = _openSFTP();
    if (!sftp) {
      qWarning() << "Could not create sftp channel.";
      return false;
    }

    // Add trailing slashes:
    QString localpath = local + "/";
    QString remotepath = remote + "/";

    // Open local dir
    QDir locdir (localpath);
    if (!locdir.exists()) {
      qWarning() << "Could not open local directory " << localpath;
      sftp_free(sftp);
      return false;
    }

    // Get listing of all items to copy
    QStringList directories = locdir.entryList(QDir::AllDirs |
                                               QDir::NoDotAndDotDot,
                                               QDir::Name);
    QStringList files = locdir.entryList(QDir::Files, QDir::Name);

    // Create remote directory:
    sftp_mkdir(sftp,
               remotepath.toStdString().c_str(),
               0750);

    // Recurse over directories and files (depth-first)
    QStringList::const_iterator dir, file;
    for (dir = directories.begin(); dir != directories.end(); dir++) {
      // qDebug() << "Copying " << localpath + (*dir) << " to "
      //          << remotepath + (*dir);
      if (!_copyDirectoryToServer(localpath + (*dir),
                                  remotepath + (*dir))) {
        qWarning() << "Error copying " << localpath + (*dir) << " to "
                   << remotepath + (*dir);
        sftp_free(sftp);
        return false;
      }
    }
    for (file = files.begin(); file != files.end(); file++) {
      // qDebug() << "Copying " << localpath + (*file) << " to "
      //            << remotepath + (*file);
      if (!_copyFileToServer(localpath + (*file),
                             remotepath + (*file))) {
        qWarning() << "Error copying " << localpath + (*file) << " to "
                   << remotepath + (*file);
        sftp_free(sftp);
        return false;
      }
    }
    sftp_free(sftp);
    END;
    return true;
  }

  bool SSHConnection::copyDirectoryFromServer(const QString & remote,
                                              const QString & local)
  {
    QMutexLocker locker (&m_lock);
    return _copyDirectoryFromServer(remote, local);
  }

  bool SSHConnection::_copyDirectoryFromServer(const QString & remote,
                                               const QString & local)
  {
    START;
    sftp_session sftp = _openSFTP();
    if (!sftp) {
      qWarning() << "Could not create sftp channel.";
      return false;
    }

    // Add trailing slashes:
    QString localpath = local + "/";
    QString remotepath = remote + "/";

    sftp_dir dir;
    sftp_attributes file;

    // Open remote directory
    dir = sftp_opendir(sftp, remotepath.toStdString().c_str());
    if (!dir) {
      qWarning() << "Could not open remote directory " << remotepath
                 << ":\n\t" << ssh_get_error(m_session);
      sftp_free(sftp);
      return false;
    }

    // Create local directory
    QDir locdir;
    if (!locdir.mkpath(localpath)) {
      qWarning() << "Could not create local directory " << localpath;
      sftp_free(sftp);
      return false;
    }

    // Handle each object in the directory:
    while ((file = sftp_readdir(sftp,dir))) {
      if (strcmp(file->name, ".") == 0 ||
          strcmp(file->name, "..") == 0 ) {
        continue;
      }

      switch (file->type) {
      case SSH_FILEXFER_TYPE_DIRECTORY:
        if (!_copyDirectoryFromServer(remotepath + file->name,
                                      localpath + file->name)) {
          sftp_attributes_free(file);
          sftp_free(sftp);
          return false;
        }
        break;
      default:
        if (!_copyFileFromServer(remotepath + file->name,
                                 localpath + file->name)) {
          sftp_attributes_free(file);
          sftp_free(sftp);
          return false;
        }
        break;
      }
      sftp_attributes_free(file);
    }

    // Check for errors
    if ( !sftp_dir_eof(dir) && sftp_closedir(dir) == SSH_ERROR ) {
      qWarning() << "Error copying \'" << remotepath << "\' to \'" << localpath
                 << "\': " << ssh_get_error(m_session);
      sftp_free(sftp);
      return false;
    }
    sftp_free(sftp);
    END;
    return true;
  }

  bool SSHConnection::readRemoteDirectoryContents(const QString & path,
                                                  QStringList & contents)
  {
    QMutexLocker locker (&m_lock);
    return _readRemoteDirectoryContents(path, contents);
  }

  bool SSHConnection::_readRemoteDirectoryContents(const QString & path,
                                                  QStringList & contents)
  {
    START;
    sftp_session sftp = _openSFTP();
    if (!sftp) {
      qWarning() << "Could not create sftp channel.";
      return false;
    }

    QString remotepath = path + "/";
    sftp_dir dir;
    sftp_attributes file;
    contents.clear();

    // Open remote directory
    dir = sftp_opendir(sftp, remotepath.toStdString().c_str());
    if (!dir) {
      qWarning() << "Could not open remote directory " << remotepath
                 << ":\n\t" << ssh_get_error(m_session);
      sftp_free(sftp);
      return false;
    }

    // Handle each object in the directory:
    QStringList tmp;
    while ((file = sftp_readdir(sftp,dir))) {
      if (strcmp(file->name, ".") == 0 ||
          strcmp(file->name, "..") == 0 ) {
        continue;
      }
      switch (file->type) {
      case SSH_FILEXFER_TYPE_DIRECTORY:
        contents << remotepath + file->name + "/";
        if (!_readRemoteDirectoryContents(remotepath + file->name,
                                          tmp)) {
          sftp_attributes_free(file);
          sftp_free(sftp);
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
    if ( !sftp_dir_eof(dir) && sftp_closedir(dir) == SSH_ERROR ) {
      qWarning() << "Error reading contents of \'" << remotepath
                 << "\': " << ssh_get_error(m_session);
      sftp_free(sftp);
      return false;
    }
    sftp_free(sftp);
    END;
    return true;
  }

  bool SSHConnection::removeRemoteDirectory(const QString & path,
                                            bool onlyDeleteContents)
  {
    QMutexLocker locker (&m_lock);
    return _removeRemoteDirectory(path, onlyDeleteContents);
  }

  bool SSHConnection::_removeRemoteDirectory(const QString & path,
                                            bool onlyDeleteContents)
  {
    START;
    sftp_session sftp = _openSFTP();
    if (!sftp) {
      qWarning() << "Could not create sftp channel.";
      return false;
    }

    QString remotepath = path + "/";
    sftp_dir dir;
    sftp_attributes file;
    bool ok = true;

    // Open remote directory
    dir = sftp_opendir(sftp, remotepath.toStdString().c_str());
    if (!dir) {
      qWarning() << "Could not open remote directory " << remotepath
                 << ":\n\t" << ssh_get_error(m_session);
      sftp_free(sftp);
      return false;
    }

    // Handle each object in the directory:
    while ((file = sftp_readdir(sftp,dir))) {
      if (strcmp(file->name, ".") == 0 ||
          strcmp(file->name, "..") == 0 ) {
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
    if ( !sftp_dir_eof(dir) && sftp_closedir(dir) == SSH_ERROR ) {
      qWarning() << "Error reading contents of \'" << remotepath
                 << "\': " << ssh_get_error(m_session);
      sftp_free(sftp);
      return false;
    }
    if ( !ok ) {
      qWarning() << "Some files could not be removed from "
                 << remotepath;
      sftp_free(sftp);
      return false;
    }

    // Finally remove directory if asked
    if (!onlyDeleteContents) {
      if (sftp_rmdir(sftp,
                     remotepath.toStdString().c_str())
          == 0) {
        sftp_free(sftp);
        END;
        return true;
      }
      qWarning() << "Error removing remote directory " << remotepath
                 << ": " << ssh_get_error(m_session);
      sftp_free(sftp);
      return false;
    }
    sftp_free(sftp);
    END;
    return true;
  }

} // end namespace GlobalSearch

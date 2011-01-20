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

#include <globalsearch/sshmanager.h>
#include <globalsearch/macros.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>

#include <fstream>
#include <sstream>
#include <fcntl.h>

using namespace std;

namespace GlobalSearch {

#define START //qDebug() << __PRETTY_FUNCTION__ << " called...";
#define END //qDebug() << __PRETTY_FUNCTION__ << " finished...";

  SSHConnection::SSHConnection(SSHManager *parent)
    : QObject(parent),
      m_session(0),
      m_shell(0),
      m_sftp(0),
      m_host(""),
      m_user(""),
      m_pass(""),
      m_port(22),
      m_isValid(false),
      m_inUse(false)
  {
    if (parent) {
      // block this connection so that a thrown exception won't cause problems
      connect(this, SIGNAL(unknownHostKey(const QString &)),
              parent, SLOT(setServerKey(const QString &)),
              Qt::BlockingQueuedConnection);
    }
  }

  SSHConnection::~SSHConnection()
  {
    START;
    disconnectSession();
    END;
  }

  bool SSHConnection::isConnected()
  {
    if (!m_session || !m_shell || !m_sftp) {
      qWarning() << "SSHConnection is not connected: one or more required channels are not initialized.";
      return false;
    };

    QMutexLocker locker (&m_lock);
    START;

    // Attempt to execute "echo ok" on the host to test if everything
    // is working properly
    const char *command = "echo ok\n";
    if (channel_write(m_shell, command, sizeof(command)) == SSH_ERROR) {
      qWarning() << "SSHConnection is not connected: cannot write to shell; "
                 << ssh_get_error(m_session);
      return false;
    }

    // Set a three second timeout, check every 50 ms for new data.
    int bytesAvail;
    int timeout = 3000;
    do {
      // Poll for number of bytes available
      bytesAvail = channel_poll(m_shell, 0);
      if (bytesAvail == SSH_ERROR) {
        qWarning() << "SSHConnection::isConnected(): server returns an error; "
                   << ssh_get_error(m_session);
        return false;
      }
      // Sleep for 50 ms if no data yet.
      if (bytesAvail <= 0) {
        GS_MSLEEP(50);
        timeout -= 50;
      }
    }
    while (timeout >= 0 && bytesAvail <= 0);
    // Timeout case
    if (timeout < 0 && bytesAvail == 0) {
      qWarning() << "SSHConnection::isConnected(): server timeout.";
      return false;
    }
    // Anything else but "3" is an error
    else if (bytesAvail != 3) {
      qWarning() << "SSHConnection::isConnected(): server returns a bizarre poll value: "
                 << bytesAvail << "; " << ssh_get_error(m_session);
      return false;
    }

    // Read output
    ostringstream ossout;
    char buffer[SSH_BUFFER_SIZE];
    int len;
    while ((len = channel_read(m_shell,
                               buffer,
                               (bytesAvail < SSH_BUFFER_SIZE) ? bytesAvail : SSH_BUFFER_SIZE,
                               0)) > 0) {
      ossout.write(buffer,len);
      // Poll for number of bytes available using a 1 second timeout in case the stack is full.
      timeout = 1000;
      do {
        bytesAvail = channel_poll(m_shell, 0);
        if (bytesAvail == SSH_ERROR) {
          qWarning() << "SSHConnection::_execute: server returns an error; "
                     << ssh_get_error(m_session);
          return false;
        }
        if (bytesAvail <= 0) {
          GS_MSLEEP(50);
          timeout -= 50;
        }
      }
      while (timeout >= 0 && bytesAvail <= 0);
    }

    // Test output
    if (!strcmp(ossout.str().c_str(), "ok")) {
      qWarning() << "SSH error: 'echo ok' on the host returned: "
                 << ossout.str().c_str();
      return false;
    }

    END;
    return true;
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


  bool SSHConnection::disconnectSession()
  {
    QMutexLocker locker (&m_lock);
    START;

    if (m_shell)
      channel_free(m_shell);
    m_shell = 0;

    if (m_sftp)
      sftp_free(m_sftp);
    m_sftp = 0;

    if (m_session)
      ssh_free(m_session);
    m_session = 0;

    m_isValid = false;
    END;
    return true;
  }

  bool SSHConnection::reconnectSession(bool throwExceptions)
  {
    START;
    if (!disconnectSession()) return false;
    if (!connectSession(throwExceptions)) return false;
    END;
    return true;
  }

  bool SSHConnection::connectSession(bool throwExceptions)
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
    case SSH_SERVER_NOT_KNOWN: {
      int hlen;
      unsigned char *hash = 0;
      char *hexa;
      hlen = ssh_get_pubkey_hash(m_session, &hash);
      hexa = ssh_get_hexa(hash, hlen);
      emit unknownHostKey(QString(hexa));
      if (throwExceptions) {
        throw SSH_UNKNOWN_HOST_ERROR;
      }
      else {
        return false;
      }
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

    // Open shell channel
    if (m_shell) {
      channel_free(m_shell);
      m_shell = 0;
    }

    m_shell = channel_new(m_session);
    if (!m_shell) {
      qWarning() << "SSH error initializing shell: "
                 << ssh_get_error(m_session);
      if (throwExceptions) {
        throw SSH_UNKNOWN_ERROR;
      }
      else {
        return false;
      }
    }
    if (channel_open_session(m_shell) != SSH_OK) {
      qWarning() << "SSH error opening shell: "
                 << ssh_get_error(m_session);
      if (throwExceptions) {
        throw SSH_UNKNOWN_ERROR;
      }
      else {
        return false;
      }

    }
    if (channel_request_shell(m_shell) != SSH_OK) {
      qWarning() << "SSH error requesting shell: "
                 << ssh_get_error(m_session);
      if (throwExceptions) {
        throw SSH_UNKNOWN_ERROR;
      }
      else {
        return false;
      }
    }

    // Open sftp session
    m_sftp = sftp_new(m_session);
    if(!m_sftp){
      qWarning() << "sftp error initialising channel" << endl
                 << ssh_get_error(m_session);
      if (throwExceptions) {
        throw SSH_UNKNOWN_ERROR;
      }
      else {
        return false;
      }
    }
    if(sftp_init(m_sftp) != SSH_OK){
      qWarning() << "error initialising sftp" << endl
                 << ssh_get_error(m_session);
      if (throwExceptions) {
        throw SSH_UNKNOWN_ERROR;
      }
      else {
        return false;
      }
    }

    m_isValid = true;
    END;
    return true;
  }

  bool SSHConnection::execute(const QString &command,
                              QString &stdout_str,
                              QString &stderr_str,
                              int &exitcode) {
    QMutexLocker locker (&m_lock);
    return _execute(command, stdout_str, stderr_str, exitcode);
  }

  bool SSHConnection::_execute(const QString &qcommand,
                               QString &stdout_str,
                               QString &stderr_str,
                               int &exitcode)
  {
    START;

    // Append newline to command if needed
    char command[qcommand.size() + 1];
    strcpy(command, qcommand.toStdString().c_str());
    if (!qcommand.endsWith('\n')) {
      strcat(command, "\n");
    }

    // Execute command
    if (channel_write(m_shell, command, strlen(command)) == SSH_ERROR) {
      qWarning() << "SSHConnection::_execute: Error writing\n\t'"
                 << command
                 << "'\n\t to host. " << ssh_get_error(m_session);
      return false;
    }

    // Set a 500 millisecond timeout, check every 50 ms for new data A
    // timeout here is not an error. It is more likely that the
    // command has no output.
    int timeout = 500;
    int bytesAvail;
    do {
      // Poll for number of bytes available
      bytesAvail = channel_poll(m_shell, 0);
      if (bytesAvail == SSH_ERROR) {
        qWarning() << "SSHConnection::_execute: server returns an error; "
                   << ssh_get_error(m_session);
        return false;
      }
      // Sleep for 50 ms if no data yet.
      if (bytesAvail <= 0) {
        GS_MSLEEP(50);
        timeout -= 50;
      }
    }
    while (timeout >= 0 && bytesAvail <= 0);
    // Negative value is an error (SSH_ERROR is explicitly detected earlier)
    if (bytesAvail < 0) {
      qWarning() << "SSHConnection::_execute: server returns a a bizarre poll value: "
                 << bytesAvail << "; " << ssh_get_error(m_session);
      return false;
    }

    // Create string streams
    ostringstream ossout, osserr;

    // Read output
    char buffer[SSH_BUFFER_SIZE];
    int len;
    // stdout (bytesAvail is set earlier)
    while ((len = channel_read(m_shell,
                               buffer,
                               (bytesAvail < SSH_BUFFER_SIZE) ? bytesAvail : SSH_BUFFER_SIZE,
                               0)) > 0) {
      ossout.write(buffer,len);
      // Poll for number of bytes available using a 1 second timeout in case the stack is full.
      timeout = 1000;
      do {
        bytesAvail = channel_poll(m_shell, 0);
        if (bytesAvail == SSH_ERROR) {
          qWarning() << "SSHConnection::_execute: server returns an error; "
                     << ssh_get_error(m_session);
          return false;
        }
        if (bytesAvail <= 0) {
          GS_MSLEEP(50);
          timeout -= 50;
        }
      }
      while (timeout >= 0 && bytesAvail <= 0);
    }
    stdout_str = QString(ossout.str().c_str());

    // stderr
    // Poll for number of bytes available
    bytesAvail = channel_poll(m_shell, 1);
    if (bytesAvail == SSH_ERROR) {
      qWarning() << "SSHConnection::_execute: server returns an error; "
                 << ssh_get_error(m_session);
      return false;
    }

    while ((len = channel_read(m_shell,
                               buffer,
                               (bytesAvail < SSH_BUFFER_SIZE) ? bytesAvail : SSH_BUFFER_SIZE,
                               1)) > 0) {
      osserr.write(buffer,len);
      // Poll for number of bytes available using a 1 second timeout in case the stack is full.
      timeout = 1000;
      do {
        bytesAvail = channel_poll(m_shell, 1);
        if (bytesAvail == SSH_ERROR) {
          qWarning() << "SSHConnection::_execute: server returns an error; "
                     << ssh_get_error(m_session);
          return false;
        }
        if (bytesAvail <= 0) {
          GS_MSLEEP(50);
          timeout -= 50;
        }
      }
      while (timeout >= 0 && bytesAvail <= 0);
    }

    stderr_str = QString(osserr.str().c_str());

    // Get exit code (via "echo $?")
    // Execute command
    char ecCommand[] = "echo $?\n";
    if (channel_write(m_shell, ecCommand, strlen(ecCommand)) == SSH_ERROR) {
      qWarning() << "SSHConnection::_execute: Error writing\n\t'"
                 << ecCommand
                 << "'\n\t to host. " << ssh_get_error(m_session);
      return false;
    }

    // Set a three second timeout, check every 50 ms for new data.
    timeout = 3000;
    do {
      // Poll for number of bytes available
      bytesAvail = channel_poll(m_shell, 0);
      if (bytesAvail == SSH_ERROR) {
        qWarning() << "SSHConnection::_execute: server returns an error; "
                   << ssh_get_error(m_session);
        return false;
      }
      // Sleep for 50 ms if no data yet.
      if (bytesAvail <= 0) {
        GS_MSLEEP(50);
        timeout -= 50;
      }
    }
    while (timeout >= 0 && bytesAvail <= 0);
    // Negative value is an error (SSH_ERROR is explicitly detected earlier)
    if (bytesAvail < 0) {
      qWarning() << "SSHConnection::_execute: server returns a bizarre poll value: "
                 << bytesAvail << "; " << ssh_get_error(m_session);
      return false;
    }
    // Timeout case
    else if (timeout < 0 && bytesAvail == 0) {
      qWarning() << "SSHConnection::_execute: server timeout while waiting for exit code: "
                 << command;
      return false;
    }

    // Assume the exit code is less than 256 char.
    char ecChar[256];
    unsigned int ecCharIndex = 0;

    // Read output
    // stdout (bytesAvail is set earlier)
    while ((len = channel_read(m_shell,
                               buffer,
                               (bytesAvail < SSH_BUFFER_SIZE) ? bytesAvail : SSH_BUFFER_SIZE,
                               0)) > 0) {
      unsigned int bufferInd = 0;
      while (len > 0) {
        ecChar[ecCharIndex++] = buffer[bufferInd++];
        len--;
      }
      // Poll for number of bytes available using a 1 second timeout in case the stack is full.
      timeout = 1000;
      do {
        bytesAvail = channel_poll(m_shell, 0);
        if (bytesAvail == SSH_ERROR) {
          qWarning() << "SSHConnection::_execute: server returns an error; "
                     << ssh_get_error(m_session);
          return false;
        }
        if (bytesAvail <= 0) {
          GS_MSLEEP(50);
          timeout -= 50;
        }
      }
      while (timeout >= 0 && bytesAvail <= 0);
    }
    ecChar[ecCharIndex] = '\0';

    exitcode = atoi(ecChar);

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

    // Create input file handle
    ifstream from (localpath.toStdString().c_str());
    if (!from.is_open()) {
      qWarning() << "Error opening file " << localpath << " for reading.";
      return false;
    }

    // Create output file handle
    sftp_file to = sftp_open(m_sftp,
                             remotepath.toStdString().c_str(),
                             O_WRONLY | O_CREAT | O_TRUNC,
                             0750);
    if (!to) {
      qWarning() << "Error opening file " << remotepath << " for writing.";
      return false;
    }

    // Create buffer
    int size = SSH_BUFFER_SIZE;
    char *buffer = new char [size];

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
    // Open remote file
    sftp_file from = sftp_open(m_sftp,
                               remotepath.toStdString().c_str(),
                               O_RDONLY,
                               0);
    if(!from){
      qWarning() << "Error opening file " << remotepath << " for reading.";
      return false;
    }

    // Open local file
    ofstream to (localpath.toStdString().c_str());
    if (!to.is_open()) {
      qWarning() << "Error opening file " << localpath << " for writing.";
      sftp_close(from);
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
        return false;
      }
    }
    to.close();
    sftp_close(from);
    delete[] buffer;
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
    // Open remote file
    sftp_file from = sftp_open(m_sftp,
                               filename.toStdString().c_str(),
                               O_RDONLY,
                               0);
    if(!from){
      qWarning() << "Error opening file " << filename << " for reading.";
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
    if (sftp_unlink(m_sftp,
                    filename.toStdString().c_str())
        != 0) {
      qWarning() << "Error removing remote file " << filename;
      return false;
    }
    END;
    return true;
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

    // Add trailing slashes:
    QString localpath = local + "/";
    QString remotepath = remote + "/";

    // Open local dir
    QDir locdir (localpath);
    if (!locdir.exists()) {
      qWarning() << "Could not open local directory " << localpath;
      return false;
    }

    // Get listing of all items to copy
    QStringList directories = locdir.entryList(QDir::AllDirs |
                                               QDir::NoDotAndDotDot,
                                               QDir::Name);
    QStringList files = locdir.entryList(QDir::Files, QDir::Name);

    // Create remote directory:
    sftp_mkdir(m_sftp,
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
        return false;
      }
    }
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
    // Add trailing slashes:
    QString localpath = local + "/";
    QString remotepath = remote + "/";

    sftp_dir dir;
    sftp_attributes file;

    // Open remote directory
    dir = sftp_opendir(m_sftp, remotepath.toStdString().c_str());
    if (!dir) {
      qWarning() << "Could not open remote directory " << remotepath
                 << ":\n\t" << ssh_get_error(m_session);
      return false;
    }

    // Create local directory
    QDir locdir;
    if (!locdir.mkpath(localpath)) {
      qWarning() << "Could not create local directory " << localpath;
      return false;
    }

    // Handle each object in the directory:
    while ((file = sftp_readdir(m_sftp,dir))) {
      if (strcmp(file->name, ".") == 0 ||
          strcmp(file->name, "..") == 0 ) {
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
    if ( !sftp_dir_eof(dir) && sftp_closedir(dir) == SSH_ERROR ) {
      qWarning() << "Error copying \'" << remotepath << "\' to \'" << localpath
                 << "\': " << ssh_get_error(m_session);
      return false;
    }
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

    QString remotepath = path + "/";
    sftp_dir dir;
    sftp_attributes file;
    contents.clear();

    // Open remote directory
    dir = sftp_opendir(m_sftp, remotepath.toStdString().c_str());
    if (!dir) {
      qWarning() << "Could not open remote directory " << remotepath
                 << ":\n\t" << ssh_get_error(m_session);
      return false;
    }

    // Handle each object in the directory:
    QStringList tmp;
    while ((file = sftp_readdir(m_sftp,dir))) {
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
      return false;
    }
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

    QString remotepath = path + "/";
    sftp_dir dir;
    sftp_attributes file;
    bool ok = true;

    // Open remote directory
    dir = sftp_opendir(m_sftp, remotepath.toStdString().c_str());
    if (!dir) {
      qWarning() << "Could not open remote directory " << remotepath
                 << ":\n\t" << ssh_get_error(m_session);
      return false;
    }

    // Handle each object in the directory:
    while ((file = sftp_readdir(m_sftp,dir))) {
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
      return false;
    }
    if ( !ok ) {
      qWarning() << "Some files could not be removed from "
                 << remotepath;
      return false;
    }

    // Finally remove directory if asked
    if (!onlyDeleteContents) {
      if (sftp_rmdir(m_sftp,
                     remotepath.toStdString().c_str())
          == SSH_ERROR) {
        qWarning() << "Error removing remote directory " << remotepath
                   << ": " << ssh_get_error(m_session);
        return false;
      }
    }
    END;
    return true;
  }

  bool SSHConnection::addKeyToKnownHosts(const QString &host, unsigned int port)
  {
    // Create session
    ssh_session session = ssh_new();
    if (!session) {
      return false;
    }

    // Set options
    int verbosity = SSH_LOG_NOLOG;
    int timeout = 5; // timeout in sec

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

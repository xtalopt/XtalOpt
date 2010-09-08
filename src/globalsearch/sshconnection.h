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

#ifndef SSHCONNECTION_H
#define SSHCONNECTION_H

extern "C" {
#include <libssh/libssh/libssh.h>
#include <libssh/libssh/sftp.h>
}

#include <globalsearch/optbase.h>

#include <QObject>
#include <QString>
#include <QMutex>

#define SSH_BUFFER_SIZE 20480

namespace GlobalSearch {

  /**
   * @class SSHConnection sshconnection.h <globalsearch/sshconnection.h>
   *
   * @brief A class to handle command execution and sftp transactions
   * on an ssh server.
   *
   * @author David C. Lonie
   */
  class SSHConnection : public QObject
  {
    Q_OBJECT

  public:
    /// Exceptions
    enum SSHConnectionException {
      /// An error connecting to the host has occurred
      SSH_CONNECTION_ERROR = 1,
      /// The host is unknown or has changed its key. Login via
      /// terminal first and verify, then retry.
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
    explicit SSHConnection(OptBase *parent = 0);

    /**
     * Destructor.
     */
    virtual ~SSHConnection();

    QString getUser() {return m_user;};
    QString getHost() {return m_host;};
    int getPort() {return m_port;};

  public slots:
    void setLoginDetails(const QString &host,
                         const QString &user = "",
                         const QString &pass = "",
                         int port = 22);
    void setUsed(bool b) {m_inUse = b;};
    bool inUse() {return m_inUse;};

    bool execute(const QString &command,
                 QString &stdout,
                 QString &stderr,
                 int &exitcode);
    bool copyFileToServer(const QString & localpath,
                          const QString & remotepath);
    bool copyFileFromServer(const QString & remotepath,
                            const QString & localpath);
    bool readRemoteFile(const QString &filename,
                        QString &contents);
    bool removeRemoteFile(const QString &filename);

    bool copyDirectoryToServer(const QString & localpath,
                               const QString & remotepath);
    bool copyDirectoryFromServer(const QString & remotepath,
                                 const QString & localpath);
    bool readRemoteDirectoryContents(const QString & remotepath,
                                     QStringList & contents);
    bool removeRemoteDirectory(const QString & remotepath,
                               bool onlyDeleteContents = false);


    bool isValid() {return m_isValid;};
    bool isConnected(ssh_channel channel = 0);
    bool connect(bool throwExceptions = false);
    bool reconnect(bool throwExceptions = false);
    bool disconnect();

    bool reconnectIfNeeded() {if (!isConnected()) return reconnect(false);
      return true;};

  private:
    bool _execute(const QString &command,
                  QString &stdout,
                  QString &stderr,
                  int &exitcode);
    bool _copyFileToServer(const QString & localpath,
                           const QString & remotepath);
    bool _copyFileFromServer(const QString & remotepath,
                             const QString & localpath);
    bool _readRemoteFile(const QString &filename,
                         QString &contents);
    bool _removeRemoteFile(const QString &filename);
    bool _copyDirectoryToServer(const QString & localpath,
                                const QString & remotepath);
    bool _copyDirectoryFromServer(const QString & remotepath,
                                  const QString & localpath);
    bool _readRemoteDirectoryContents(const QString & remotepath,
                                      QStringList & contents);
    bool _removeRemoteDirectory(const QString & remotepath,
                                bool onlyDeleteContents = false);

    sftp_session _openSFTP();

    ssh_session m_session;
    QString m_host;
    QString m_user;
    QString m_pass;
    int m_port;
    bool m_isValid;
    bool m_inUse;
    QMutex m_lock;
  };

} // end namespace GlobalSearch

#endif

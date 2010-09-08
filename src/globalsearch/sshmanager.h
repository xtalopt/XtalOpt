/**********************************************************************
  SSHManager - Manages a collection of SSHConnections

  Copyright (C) 2010 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef SSHMANAGER_H
#define SSHMANAGER_H

#include <globalsearch/sshconnection.h>

#include <QObject>
#include <QMutex>
#include <QHash>

namespace GlobalSearch {
  class OptBase;

  /**
   * @class SSHManager sshmanager.h <globalsearch/sshmanager.h>
   *
   * @brief A class to manage multiple SSHConnection objects.
   *
   * @author David C. Lonie
   */
  class SSHManager : public QObject
  {
    Q_OBJECT

  public:
    /**
     * Constructor.
     *
     * @param parent The OptBase parent
     */
    explicit SSHManager(unsigned int connections = 5, OptBase *parent = 0);

    /**
     * Destructor.
     */
    virtual ~SSHManager();

    /**
     * Create connections to the specifed host. If the connections
     * cannot be made, an SSHConnection::SSHConnectionException will
     * be thrown.
     */
    void makeConnections(const QString &host,
                         const QString &user = "",
                         const QString &pass = "",
                         unsigned int port = 22);

    bool isValid() {return m_isValid;};
    QString getUser() {return m_user;};
    QString getHost() {return m_host;};
    int getPort() {return m_port;};

  public slots:
    /**
     * Returns a free connection from the pool and locks it.
     * @sa unlockConnection
     */
    SSHConnection *getFreeConnection();

    /**
     * Call this when finished with a connection so other threads can
     * use it.
     */
    void unlockConnection(SSHConnection* ssh);

  private:
    QList<SSHConnection*> m_conns;

    QMutex m_lock;

    QString m_host;
    QString m_user;
    QString m_pass;
    unsigned int m_port;
    unsigned int m_connections;
    bool m_isValid;
  };

} // end namespace GlobalSearch

#endif

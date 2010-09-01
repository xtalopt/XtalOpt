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

#include <globalsearch/sshmanager.h>

#include <QDebug>

#define START //qDebug() << __PRETTY_FUNCTION__ << " called...";
#define END //qDebug() << __PRETTY_FUNCTION__ << " finished...";

using namespace std;

namespace GlobalSearch {

  SSHManager::SSHManager(unsigned int connections,
                         const QString &host,
                         const QString &user,
                         const QString &pass,
                         unsigned int port,
                         OptBase *parent)
    : QObject(parent),
      m_host(host),
      m_user(user),
      m_pass(pass),
      m_port(port)
  {
    QMutexLocker locker (&m_lock);
    START;

    for (unsigned int i = 0; i < connections; i++) {
      m_conns.append(new SSHConnection(host, user, pass, port, parent));
      qDebug() << "Created connection #" << i+1;
    }

    END;
  }

  SSHManager::~SSHManager()
  {
    QMutexLocker locker (&m_lock);
    START;

    QList<SSHConnection*>::iterator it;
    for (it =  m_conns.begin(); it != m_conns.end(); it++) {
      delete (*it);
      (*it) = 0;
    }

    END;
  }

  SSHConnection* SSHManager::getFreeConnection()
  {
    QMutexLocker locker (&m_lock);
    START;

    QList<SSHConnection*>::iterator it;
    for (;;) {
      for (it =  m_conns.begin(); it != m_conns.end(); it++) {
        if (!(*it)->inUse()) {
          (*it)->setUsed(true);
          qDebug() << "Returning SSHConnection instance " << (*it);
          END;
          return (*it);
        }
      }
    }
    END;
  }

  void SSHManager::unlockConnection(SSHConnection* ssh)
  {
    // Don't lock m_lock here
    qDebug() << "Connection " << ssh << " unlocked";
    ssh->setUsed(false);
  }

} // end namespace GlobalSearch

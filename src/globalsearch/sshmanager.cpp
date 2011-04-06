/**********************************************************************
  SSHManager - Manages a collection of SSHConnections

  Copyright (C) 2010-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <globalsearch/sshmanager.h>

#include <globalsearch/macros.h>

#include <QtCore/QDebug>

#define START //qDebug() << __PRETTY_FUNCTION__ << " called...";
#define END //qDebug() << __PRETTY_FUNCTION__ << " finished...";

using namespace std;

namespace GlobalSearch {

  SSHManager::SSHManager(unsigned int connections, OptBase *parent)
    : QObject(parent),
      m_connSemaphore(connections),
      m_connections(connections),
      m_isValid(false)
  {
    for (unsigned int i = 0; i < connections; i++) {
      m_conns.append(new SSHConnection(this));
      //qDebug() << "Created connection #" << i+1;
    }
  }

  void SSHManager::makeConnections(const QString &host,
                                   const QString &user,
                                   const QString &pass,
                                   unsigned int port)
  {
    m_isValid = false;
    QMutexLocker locker (&m_lock);
    START;

    m_host = host;
    m_user = user;
    m_pass = pass;
    m_port = port;

    QList<SSHConnection*>::iterator it;
    for (it = m_conns.begin(); it != m_conns.end(); it++) {
      (*it)->setLoginDetails(m_host, m_user, m_pass, m_port);
      // No need to check return value, the "true" arguement will
      // throw exceptions.
      (*it)->connectSession(true);
    }

    m_isValid = true;
    END;
  }

  SSHManager::~SSHManager()
  {
    QMutexLocker locker (&m_lock);
    START;

    QList<SSHConnection*>::iterator it;
    for (it =  m_conns.begin(); it != m_conns.end(); it++) {
      while ((*it)->inUse()) {
        // Wait for connection to become free
        qDebug() << "Spinning while waiting for SSHConnection to free."
                 << *it;
        GS_SLEEP(1);
      }
      (*it)->setUsed(true);
      delete (*it);
      (*it) = 0;
    }

    END;
  }

  SSHConnection* SSHManager::getFreeConnection()
  {
    // Wait until a connection is available:
    m_connSemaphore.acquire();

    // When a connection is available, allow one thread at a time to
    // obtain the next available SSHConnection
    QMutexLocker locker (&m_lock);

    START;

    QList<SSHConnection*>::iterator it;
    for (it =  m_conns.begin(); it != m_conns.end(); it++) {
      if ((*it) && !(*it)->inUse()) {
        (*it)->setUsed(true);
        //qDebug() << "Returning SSHConnection instance " << (*it);
        END;
        return (*it);
      }
    }
    // If this point is reached, no connections are available. If in
    // debug mode, fail here
    Q_ASSERT_X(false, Q_FUNC_INFO,
               "No SSHConnections available. This should not "
               "happen with the protection provided by "
               "m_connSemaphore. Is SSHManager::unlockConnection "
               "being called correctly?");

    // If this is a release build, release semaphore and tail-recurse.
    m_connSemaphore.release();

    return this->getFreeConnection();
  }

  void SSHManager::unlockConnection(SSHConnection* ssh)
  {
    Q_ASSERT_X(m_conns.contains(ssh), Q_FUNC_INFO,
               "Attempt to unlock an SSHConnection not owned by this "
               "SSHManager.");
    Q_ASSERT(ssh->inUse());

    m_lock.lock();
    ssh->setUsed(false);
    m_connSemaphore.release();
    m_lock.unlock();
  }

  QString SSHManager::getServerKeyHash()
  {
    return m_hexa;
  }

  bool SSHManager::validateServerKey()
  {
    return SSHConnection::addKeyToKnownHosts(m_host, m_port);
  }

  void SSHManager::setServerKey(const QString &hexa)
  {
    m_hexa = hexa;
  }

} // end namespace GlobalSearch

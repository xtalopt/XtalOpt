/**********************************************************************
  SSHManager - Manages a collection of SSHConnections

  Copyright (C) 2010-2012 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifdef ENABLE_SSH

#include <globalsearch/sshmanager_libssh.h>

#include <globalsearch/macros.h>
#include <globalsearch/sshconnection_libssh.h>

#include <QDebug>

#define START // qDebug() << __FUNCTION__ << " called...";
#define END   // qDebug() << __FUNCTION__ << " finished...";

using namespace std;

namespace GlobalSearch {

SSHManagerLibSSH::SSHManagerLibSSH(unsigned int connections, OptBase* parent)
  : SSHManager(parent), m_connSemaphore(connections),
    m_connections(connections), m_isValid(false)
{
  for (unsigned int i = 0; i < connections; i++) {
    m_conns.append(new SSHConnectionLibSSH(this));
    // qDebug() << "Created connection #" << i+1;
  }
}

void SSHManagerLibSSH::makeConnections(const QString& host, const QString& user,
                                       const QString& pass, unsigned int port)
{
  m_isValid = false;
  QMutexLocker locker(&m_lock);
  START;

  m_host = host;
  m_user = user;
  m_pass = pass;
  m_port = port;

  QList<SSHConnectionLibSSH*>::iterator it;
  for (it = m_conns.begin(); it != m_conns.end(); it++) {
    (*it)->setLoginDetails(m_host, m_user, m_pass, m_port);
    // No need to check return value, the "true" arguement will
    // throw exceptions.
    (*it)->connectSession(true);
  }

  m_isValid = true;
  END;
}

SSHManagerLibSSH::~SSHManagerLibSSH()
{
  QMutexLocker locker(&m_lock);
  START;

  int timeout = 30;
  QList<SSHConnectionLibSSH*>::iterator it;
  for (it = m_conns.begin(); it != m_conns.end(); it++) {
    while ((*it)->inUse() && timeout >= 0) {
      // Wait for connection to become free
      qDebug() << "Spinning while waiting for SSHConnection to free." << *it
               << "\nTimeout in" << QString::number(timeout) << "seconds.";
      GS_SLEEP(1);
      timeout--;
    }
    (*it)->setUsed(true);
    delete (*it);
    (*it) = 0;
  }

  END;
}

SSHConnection* SSHManagerLibSSH::getFreeConnection()
{
  // Wait until a connection is available:
  m_connSemaphore.acquire();

  // When a connection is available, allow one thread at a time to
  // obtain the next available SSHConnection
  QMutexLocker locker(&m_lock);

  START;

  QList<SSHConnectionLibSSH*>::iterator it;
  for (it = m_conns.begin(); it != m_conns.end(); it++) {
    if ((*it) && !(*it)->inUse()) {
      (*it)->setUsed(true);
      // qDebug() << "Returning SSHConnectionLibSSH instance " << (*it);
      if (!(*it)->reconnectIfNeeded()) {
        qWarning() << tr("Cannot connect to ssh server %1@%2:%3")
                        .arg((*it)->getUser())
                        .arg((*it)->getHost())
                        .arg((*it)->getPort());
        return nullptr;
      }
      END;
      return (*it);
    }
  }
  // If this point is reached, no connections are available. If in
  // debug mode, fail here
  Q_ASSERT_X(false, Q_FUNC_INFO,
             "No SSHConnections available. This should not "
             "happen with the protection provided by "
             "m_connSemaphore. Is SSHManagerLibSSH::unlockConnection "
             "being called correctly?");

  // If this is a release build, release semaphore and tail-recurse.
  m_connSemaphore.release();

  return this->getFreeConnection();
}

void SSHManagerLibSSH::unlockConnection(SSHConnection* ssh)
{
  SSHConnectionLibSSH* libsshConn = qobject_cast<SSHConnectionLibSSH*>(ssh);
  Q_ASSERT_X(m_conns.contains(libsshConn), Q_FUNC_INFO,
             "Attempt to unlock an SSHConnectionLibSSH not owned by this "
             "SSHManagerLibSSH.");
  Q_ASSERT(libsshConn->inUse());

  m_lock.lock();
  libsshConn->setUsed(false);
  m_connSemaphore.release();
  m_lock.unlock();
}

QString SSHManagerLibSSH::getServerKeyHash()
{
  return m_hexa;
}

bool SSHManagerLibSSH::validateServerKey()
{
  return SSHConnectionLibSSH::addKeyToKnownHosts(m_host, m_port);
}

void SSHManagerLibSSH::setServerKey(const QString& hexa)
{
  m_hexa = hexa;
}

} // end namespace GlobalSearch

#endif // ENABLE_SSH

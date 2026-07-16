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


#include <search/ssh/sshmanager_libssh.h>

#include <common/compatibility/qt_compat.h>
#include <common/compatibility/platform_compat.h>
#include <common/output.h>

#include <memory>
#include <search/ssh/sshconnection_libssh.h>

#define START
#define END

using namespace std;

namespace Search {

SSHManagerLibSSH::SSHManagerLibSSH(unsigned int connections, SearchBase* parent)
  : SSHManager(parent), m_connSemaphore(connections),
    m_connections(connections), m_isValid(false)
{
  for (unsigned int i = 0; i < connections; i++) {
    m_conns.append(new SSHConnectionLibSSH(this));
  }
}

void SSHManagerLibSSH::makeConnections(const QString& host, const QString& user,
                                       const QString& pass, unsigned int port)
{
  m_isValid = false;
  QtCompat::MutexLocker locker(&m_lock);
  START;

  m_host = host;
  m_user = user;
  m_pass = pass;
  m_port = port;

  for (auto it = m_conns.begin(); it != m_conns.end(); it++) {
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
  START;

  // Wait for connections to be released.
  for (SSHConnectionLibSSH* conn : m_conns) {
    int timeout = 30;
    while (conn && conn->inUse() && timeout-- > 0) {
      // Wait for a free connection.
      GS_SLEEP(1);
    }
  }

  // Delete free connections.
  QtCompat::MutexLocker locker(&m_lock);
  for (auto it = m_conns.begin(); it != m_conns.end(); ++it) {
    if (!*it)
      continue;
    if ((*it)->inUse()) {
      Common::warning(tr("Leaking an in-use SSH connection at shutdown."));
      continue;
    }
    delete *it;
    *it = nullptr;
  }

  END;
}

SSHConnection* SSHManagerLibSSH::getFreeConnection()
{
  // Wait until a connection is available:
  m_connSemaphore.acquire();

  // When a connection is available, allow one thread at a time to
  // obtain the next available SSHConnection
  QtCompat::MutexLocker locker(&m_lock);

  START;

  for (auto it = m_conns.begin(); it != m_conns.end(); it++) {
    if ((*it) && !(*it)->inUse()) {
      (*it)->setUsed(true);
      if (!(*it)->reconnectIfNeeded()) {
        Common::warning(tr("Cannot connect to ssh server %1@%2:%3")
                       .arg((*it)->getUser())
                       .arg((*it)->getHost())
                       .arg((*it)->getPort()));
        (*it)->setUsed(false);
        m_connSemaphore.release();
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

  QtCompat::MutexLocker locker(&m_lock);

  if (!libsshConn || !m_conns.contains(libsshConn) || !libsshConn->inUse()) {
    Common::warning(tr("Attempted to unlock an invalid or unused SSH connection."));
    return;
  }

  libsshConn->setUsed(false);
  m_connSemaphore.release();
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

SSHManagerLibSSH* SSHManagerLibSSH::createConnections(
  SearchBase* parent, const QString& host, const QString& user, int port,
  const std::function<bool(const QString&)>& confirmHostKey,
  const std::function<bool(const QString&, QString&)>& promptPassword, QString* error)
{
  std::unique_ptr<SSHManagerLibSSH> manager(new SSHManagerLibSSH(5, parent));
  QString pw = "";
  for (;;) {
    try {
      manager->makeConnections(host, user, pw, port);
    } catch (SSHConnection::SSHConnectionException e) {
      QString err;
      switch (e) {
        case SSHConnection::SSH_CONNECTION_ERROR:
        case SSHConnection::SSH_UNKNOWN_ERROR:
        default:
          err = "There was a problem connecting to the ssh server at " +
                user + "@" + host + ":" + QString::number(port) + ". "
                "Please check that all provided information is correct, and "
                "attempt to log in from a terminal before trying again.";
          if (error)
            *error = err;
          else
            Common::error(err);
          return nullptr;
        case SSHConnection::SSH_UNKNOWN_HOST_ERROR: {
          // The host is not known, or has changed its key.
          // Ask user if this is ok.
          err = "The host " + host + ":" + QString::number(port) +
                " either has an unknown key, or has changed its key:\n" +
                manager->getServerKeyHash() + "\n" + "Would you like to trust the specified host?";
          if (!confirmHostKey(err)) {
            if (error)
              *error = "libssh host-key prompt was cancelled.";
            return nullptr;
          }
          manager->validateServerKey();
          continue;
        } // end case
        case SSHConnection::SSH_BAD_PASSWORD_ERROR: {
          // Chances are that the pubkey auth was attempted but failed,
          // so just prompt user for password.
          err = "Please enter a password for " + user + "@" + host + ":" +
                QString::number(port) + ": ";
          QString newPassword;
          if (!promptPassword(err, newPassword)) {
            if (error)
              *error = "libssh password prompt was cancelled.";
            return nullptr;
          }
          pw = newPassword;
          continue;
        } // end case
      }   // end switch
    }     // end catch
    break;
  } // end forever
  return manager.release();
}

} // end namespace Search

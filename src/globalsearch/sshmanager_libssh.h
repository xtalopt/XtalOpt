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

#ifndef SSHMANAGERLIBSSH_H
#define SSHMANAGERLIBSSH_H

#ifdef ENABLE_SSH

#include <globalsearch/sshmanager.h>

#include <QMutex>
#include <QSemaphore>

namespace GlobalSearch {
class OptBase;
class SSHConnection;
class SSHConnectionLibSSH;

/**
 * @class SSHManagerLibSSH sshmanager_libssh.h
 * <globalsearch/sshmanager_libssh.h>
 *
 * @brief A class to manage multiple SSHConnectionLibSSH objects.
 *
 * @author David C. Lonie
 */
class SSHManagerLibSSH : public SSHManager
{
  Q_OBJECT

public:
  /**
   * Constructor.
   *
   * @param connections The maximum number of simultaneous connections.
   * @param parent The OptBase parent
   */
  explicit SSHManagerLibSSH(unsigned int connections = 5, OptBase* parent = 0);

  /**
   * Destructor.
   */
  virtual ~SSHManagerLibSSH() override;

  /**
   * Create connections to the specifed host. If the connections
   * cannot be made, an SSHConnection::SSHConnectionException will
   * be thrown.
   */
  void makeConnections(const QString& host, const QString& user = "",
                       const QString& pass = "",
                       unsigned int port = 22) override;

  /**
   * @return Whether the connection has been made successfully.
   */
  bool isValid() { return m_isValid; };

public slots:
  /**
   * Returns a free connection from the pool and locks it.
   * @sa unlockConnection
   */
  SSHConnection* getFreeConnection() override;

  /**
   * Call this when finished with a connection so other threads can
   * use it.
   */
  void unlockConnection(SSHConnection* ssh) override;

  /**
   * Retreive the public key from the server. This is set when a
   * connection fails with SSH_UNKNOWN_HOST_ERROR.
   *
   * @sa SSH_UNKNOWN_HOST_ERROR
   * @sa validateServerKey
   */
  QString getServerKeyHash();

  /**
   * Add currently set key to the known host cache.
   *
   * @sa SSH_UNKNOWN_HOST_ERROR
   * @sa getServerKey;
   */
  bool validateServerKey();

  /**
   * Set the server key. This is used internally.
   */
  void setServerKey(const QString& hexa);

protected:
  /// List of all SSHConnection objects managed by this instance
  QList<SSHConnectionLibSSH*> m_conns;

  /// Internally used mutex
  QMutex m_lock;

  /// Internally used semaphore
  QSemaphore m_connSemaphore;

  /// Password
  QString m_pass;
  /// Key
  QString m_hexa;
  /// Number of connections
  unsigned int m_connections;
  /// Monitor whether the connections are valid
  bool m_isValid;
};

} // end namespace GlobalSearch

#endif // ENABLE_SSH
#endif // SSHMANAGERLIBSSH_H

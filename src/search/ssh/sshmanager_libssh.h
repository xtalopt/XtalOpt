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


#include <search/ssh/sshmanager.h>

#include <functional>

#include <QMutex>
#include <QSemaphore>

namespace Search {
class SearchBase;
class SSHConnection;
class SSHConnectionLibSSH;

/**
 * @class SSHManagerLibSSH sshmanager_libssh.h
 * <search/ssh/sshmanager_libssh.h>
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
   * @param parent The SearchBase parent
   */
  explicit SSHManagerLibSSH(unsigned int connections = 5, SearchBase* parent = 0);

  /**
   * Destructor.
   */
  virtual ~SSHManagerLibSSH() override;

  /**
   * Create connections to the specifed host. If the connections
   * cannot be made, an SSHConnection::SSHConnectionException will
   * be thrown.
   */
  /**
   * Create a manager with live connections to @p host, handling host-key
   * confirmation and password retries through the given prompts.
   *
   * @param confirmHostKey Asked to accept an unknown/changed host key.
   * @param promptPassword Asked for a password; fills the second argument.
   * @return The ready manager, or nullptr (with @p error set) on failure.
   */
  static SSHManagerLibSSH* createConnections(
    SearchBase* parent, const QString& host, const QString& user, int port,
    const std::function<bool(const QString&)>& confirmHostKey,
    const std::function<bool(const QString&, QString&)>& promptPassword, QString* error);

  void makeConnections(const QString& host, const QString& user = "",
                       const QString& pass = "",
                       unsigned int port = 22) override;

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
};

} // end namespace Search

#endif // SSHMANAGERLIBSSH_H

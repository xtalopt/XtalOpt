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

#ifndef SSHMANAGER_H
#define SSHMANAGER_H

#ifdef ENABLE_SSH

#include <QObject>

namespace GlobalSearch {
class OptBase;
class SSHConnection;

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
   * @param connections The maximum number of simultaneous connections.
   * @param parent The OptBase parent
   */
  explicit SSHManager(OptBase* parent = 0);

  /**
   * Destructor.
   */
  virtual ~SSHManager() override;

  /**
   * Create connections to the specifed host. If the connections
   * cannot be made, an SSHConnection::SSHConnectionException will
   * be thrown.
   */
  virtual void makeConnections(const QString& host, const QString& user = "",
                               const QString& pass = "",
                               unsigned int port = 22);

  /// Get the currently set user name
  QString getUser() { return m_user; };

  /// Get the currently set hostname
  QString getHost() { return m_host; };

  /// Get the currently set port
  int getPort() { return m_port; };

public slots:
  /**
   * Returns a free connection from the pool and locks it.
   * @sa unlockConnection
   */
  virtual SSHConnection* getFreeConnection() = 0;

  /**
   * Call this when finished with a connection so other threads can
   * use it.
   */
  virtual void unlockConnection(SSHConnection* ssh) = 0;

protected:
  /// Hostname
  QString m_host;
  /// Username
  QString m_user;
  /// Port
  unsigned int m_port;
};

} // end namespace GlobalSearch

#endif // ENABLE_SSH
#endif // SSHMANAGER_H

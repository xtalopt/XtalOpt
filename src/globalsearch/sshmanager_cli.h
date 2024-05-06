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

#ifndef SSHMANAGERCLI_H
#define SSHMANAGERCLI_H

#ifdef ENABLE_SSH

#include <globalsearch/sshmanager.h>

#include <QSemaphore>

namespace GlobalSearch {
class OptBase;
class SSHConnectionCLI;

/**
 * @class SSHManagerCLI sshmanager_cli.h <globalsearch/sshmanager_cli.h>
 *
 * @brief A class to manage SSHConnectionCLI objects.
 *
 * @author David C. Lonie
 */
class SSHManagerCLI : public SSHManager
{
  Q_OBJECT

public:
  /**
   * Constructor.
   *
   * @param parent The OptBase parent
   */
  explicit SSHManagerCLI(unsigned int connections = 5, OptBase* parent = 0);

  /**
   * Destructor.
   */
  virtual ~SSHManagerCLI() override;

  /**
   * Create connections to the specifed host. If the connections
   * cannot be made, an SSHConnection::SSHConnectionException will
   * be thrown.
   */
  void makeConnections(const QString& host, const QString& user,
                       const QString& pass, unsigned int port) override;

public slots:
  /**
   * Returns a free connection from the pool and locks it.
   * @sa unlockConnection
   */
  SSHConnection* getFreeConnection();

  /**
   * Call this when finished with a connection so other threads can
   * use it.
   */
  void unlockConnection(SSHConnection* ssh);

protected:
  SSHConnectionCLI* m_conn;
  QSemaphore* m_semaphore;
};

} // end namespace GlobalSearch

#endif // ENABLE_SSH
#endif // SSHMANAGER_H

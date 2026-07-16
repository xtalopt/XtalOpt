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


#include <search/ssh/sshmanager.h>

#include <QSemaphore>

namespace Search {
class SearchBase;
class SSHConnectionCLI;

/**
 * @class SSHManagerCLI sshmanager_cli.h <search/ssh/sshmanager_cli.h>
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
   * @param parent The SearchBase parent
   */
  explicit SSHManagerCLI(unsigned int connections = 5, SearchBase* parent = 0);

  /**
   * Destructor.
   */
  virtual ~SSHManagerCLI() override;

  /**
   * Create connections to the specifed host. If the connections
   * cannot be made, an SSHConnection::SSHConnectionException will
   * be thrown.
   */
  /**
   * Create a manager with live connections to @p host, prechecked.
   *
   * @return The ready manager, or nullptr (with @p error set) on failure.
   */
  static SSHManagerCLI* createConnections(SearchBase* parent, const QString& host,
                                          const QString& user, int port, QString* error);

  void makeConnections(const QString& host, const QString& user,
                       const QString& pass, unsigned int port) override;

  bool precheck(QString* error = nullptr);

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

protected:
  SSHConnectionCLI* m_conn;
  QSemaphore* m_semaphore;
};

} // end namespace Search

#endif // SSHMANAGERCLI_H

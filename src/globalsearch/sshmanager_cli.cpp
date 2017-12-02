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

#include <globalsearch/sshconnection_cli.h>
#include <globalsearch/sshmanager_cli.h>

namespace GlobalSearch {

SSHManagerCLI::SSHManagerCLI(unsigned int connections, OptBase* parent)
  : SSHManager(parent), m_conn(new SSHConnectionCLI()),
    m_semaphore(new QSemaphore(connections))
{
}

SSHManagerCLI::~SSHManagerCLI()
{
  delete m_conn;
  delete m_semaphore;
}

void SSHManagerCLI::makeConnections(const QString& host, const QString& user,
                                    const QString& pass, unsigned int port)
{
  m_host = host;
  m_user = user;
  m_port = port;
  m_conn->setLoginDetails(host, user, pass, port);
}

SSHConnection* SSHManagerCLI::getFreeConnection()
{
  m_semaphore->acquire();
  return m_conn;
}

void SSHManagerCLI::unlockConnection(SSHConnection* ssh)
{
  m_semaphore->release();
}

} // end namespace GlobalSearch

#endif // ENABLE_SSH

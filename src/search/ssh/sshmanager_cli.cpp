/**********************************************************************
  SSHManager - Manages a collection of SSHConnections

  Copyright (C) 2010-2012 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/


#include <search/ssh/sshconnection_cli.h>
#include <common/output.h>
#include <search/search.h>
#include <search/ssh/sshmanager_cli.h>

#include <memory>

namespace Search {

SSHManagerCLI::SSHManagerCLI(unsigned int connections, SearchBase* parent)
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

bool SSHManagerCLI::precheck(QString* error)
{
  return m_conn->precheck(error);
}

SSHConnection* SSHManagerCLI::getFreeConnection()
{
  SearchBase* search = qobject_cast<SearchBase*>(parent());
  while (!m_semaphore->tryAcquire(1, 100)) {
    if (search && search->isShuttingDown())
      return nullptr;
  }
  return m_conn;
}

void SSHManagerCLI::unlockConnection(SSHConnection* /*ssh*/)
{
  m_semaphore->release();
}



SSHManagerCLI* SSHManagerCLI::createConnections(SearchBase* parent, const QString& host,
                                                const QString& user, int port, QString* error)
{
  std::unique_ptr<SSHManagerCLI> manager(new SSHManagerCLI(5, parent));
  manager->makeConnections(host, user, "", port);

  QString precheckError;
  if (!manager->precheck(&precheckError)) {
    if (error)
      *error = precheckError;
    else
      Common::error(precheckError);
    return nullptr;
  }

  return manager.release();
}

} // end namespace Search

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

#include <globalsearch/sshmanager.h>

#include <globalsearch/macros.h>
#include <globalsearch/optbase.h>

namespace GlobalSearch {

SSHManager::SSHManager(OptBase* parent) : QObject(parent)
{
}

SSHManager::~SSHManager()
{
}

void SSHManager::makeConnections(const QString& host, const QString& user,
                                 const QString& pass, unsigned int port)
{
  m_host = host;
  m_user = user;
  Q_UNUSED(pass);
  m_port = port;
}

} // end namespace GlobalSearch

#endif // ENABLE_SSH

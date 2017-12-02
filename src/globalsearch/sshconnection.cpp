/**********************************************************************
  SSHConnection - Connection to an ssh server for execution, sftp, etc.

  Copyright (C) 2010-2012 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifdef ENABLE_SSH

#include <globalsearch/sshconnection.h>

#include <globalsearch/sshmanager.h>

namespace GlobalSearch {

SSHConnection::SSHConnection(SSHManager* parent)
  : QObject(parent), m_host(""), m_user(""), m_pass(""), m_port(22)
{
}

SSHConnection::~SSHConnection()
{
}

void SSHConnection::setLoginDetails(const QString& host, const QString& user,
                                    const QString& pass, int port)
{
  m_host = host;
  m_user = user;
  m_pass = pass;
  m_port = port;
}

} // end namespace GlobalSearch

#endif // ENABLE_SSH

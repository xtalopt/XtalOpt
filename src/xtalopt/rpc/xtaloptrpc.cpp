/**********************************************************************
  XtalOptRpc - Class for sending GUI updates to Avogadro2 through RPC

  Copyright (C) 2017 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#include <xtalopt/rpc/xtaloptrpc.h>

#include <xtalopt/structures/xtal.h>

#include <QDataStream>
#include <QJsonDocument>
#include <QJsonObject>

namespace XtalOpt {

XtalOptRpc::XtalOptRpc(QObject* parent,
                       const QString& serverName)
  : QObject(parent),
    m_isConnected(false),
    m_idCounter(0),
    m_socket(),
    m_serverName(serverName)
{
  // Cache whether we are connected or not when we receive the signal
  connect(&m_socket, &QLocalSocket::connected,
          [this](){ this->setIsConnected(true); });
  connect(&m_socket, &QLocalSocket::disconnected,
          [this](){ this->setIsConnected(false); });

  // If the local socket produces any errors, let's print them.
  connect(&m_socket, static_cast<void(QLocalSocket::*)(QLocalSocket::LocalSocketError)>(&QLocalSocket::error),
          [this](QLocalSocket::LocalSocketError socketError)
          {
            // If the server wasn't found, it's likely the the user just doesn't have open Avogadro2
            if (socketError != QLocalSocket::ServerNotFoundError)
              qDebug() << "XtalOptRpc received a socket error: " << this->m_socket.errorString();
          });
}

bool XtalOptRpc::updateDisplayedXtal(const Xtal& xtal)
{
  if (!reconnectIfNeeded())
    return false;

  QString poscar = xtal.toPOSCAR();

  QJsonObject params;
  params["format"] = QString("POSCAR");
  params["content"] = poscar;

  QJsonObject message;
  message["jsonrpc"] = QString("2.0");
  message["id"] = QString::number(++m_idCounter);
  message["method"] = QString("loadMolecule");
  message["params"] = params;

  return sendMessage(message);
}

bool XtalOptRpc::isConnected() const
{
  return m_socket.isOpen() && m_isConnected;
}

bool XtalOptRpc::reconnectIfNeeded()
{
  if (isConnected())
    return true;
  else
    return connectToServer();
}

bool XtalOptRpc::connectToServer()
{
  if (m_serverName.isEmpty())
    return false;

  if (m_socket.isOpen())
    m_socket.close();

  m_socket.connectToServer(m_serverName);
  return isConnected();
}

bool XtalOptRpc::sendMessage(const QJsonObject& message)
{
  if (!isConnected())
    return false;

  QJsonDocument document(message);
  QDataStream stream(&m_socket);
  stream << document.toJson();
  return true;
}

} // end namespace XtalOpt

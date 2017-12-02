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

#ifndef XTALOPT_RPC_H
#define XTALOPT_RPC_H

#include <QLocalSocket>
#include <QObject>
#include <QString>

#include <atomic>
#include <memory>

class QDataStream;
class QJsonObject;

namespace XtalOpt {

class Xtal;

/**
 * @class XtalOptRpc
 * @brief This class is used to send molecule updates to Avogadro2 via RPC
 */

class XtalOptRpc : public QObject
{
  Q_OBJECT

public:
  /**
   * Constructor. Pass the server name in as "serverName"
   */
  explicit XtalOptRpc(QObject* parent = nullptr,
                      const QString& serverName = "avogadro");

  /**
   * Send a POSCAR via RPC to Avogadro2 to update the xtal in the window.
   * @return True if we succeeded and false if we failed.
   */
  bool updateDisplayedXtal(const Xtal& xtal);

private:
  /**
   * Are we connected to the Avogadro2 server?
   * @return True if we are and false if we are not.
   */
  bool isConnected() const;

  /**
   * If we are not connected to the Avogadro2 socket, try to reconnect.
   * @return True if we were already connected or are now connected and
   * false if we failed to connect.
   */
  bool reconnectIfNeeded();

  /**
   * Connect to the server specified in the constructor.
   * @return True if we succeed and false if we fail.
   */
  bool connectToServer();

  /**
   * Send a Json object to the Avogadro2 server.
   * @param message The JSON-RPC 2.0 object.
   * @return True on success, false on failure.
   */
  bool sendMessage(const QJsonObject& message);

  /**
   * Set whether or not we are connected. This will be automatically
   * set to true when QLocalSocket::connected() is emitted, and it will
   * be automatically set to false when QLocalSocket::disconnected() is
   * emitted.
   * @param b True if we are connected and false if we are not.
   */
  void setIsConnected(bool b) { m_isConnected = b; }

  /**
   * Reads data from the server. This will be called when m_socket emits
   * readyRead.
   */
  void readData();

  std::atomic<bool> m_isConnected;
  size_t m_idCounter;
  std::unique_ptr<QLocalSocket> m_socket;
  std::unique_ptr<QDataStream> m_dataStream;
  QString m_serverName;
};

} // End namespace XtalOpt

#endif

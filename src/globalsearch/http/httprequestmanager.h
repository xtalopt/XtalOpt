/**********************************************************************
  HttpRequestManager - Submit http 'get' and 'post' requests with a
                       QNetworkAccessManager and receive the results

  Copyright (C) 2017-2018 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <memory>
#include <mutex>
#include <unordered_map>

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>

/**
 * This class can be used to perform an http get or an http post. It takes
 * a shared pointer to a QNetworkAccessManager, and it uses signals and slots
 * to make sure that the QNetworkAccessManager functions are only called in
 * the main thread (the main event loop). This way, HttpRequestManager
 * functions can be used in threads other than the main thread.
 *
 * HttpRequestManager stores a map of pending replies and received replies,
 * and it emits a signal when a new reply is received. When a new get or
 * post is sent, the function returns a requestId to keep track of the request.
 */
class HttpRequestManager : public QObject
{
  Q_OBJECT
public:
  explicit HttpRequestManager(
    const std::shared_ptr<QNetworkAccessManager>& networkManager,
    QObject* parent = nullptr);

  // Sends a URL get and returns an index that is used to access the
  // obtained data.
  size_t sendGet(QUrl url);

  // Sends a POST to a URL and returns an index that is used to access the
  // reply data.
  size_t sendPost(QUrl url, const QByteArray& data);

  // Check to see if data has been received for reply index @p i.
  bool containsData(size_t i) const;

  // Get the received data for a particular reply index @p i. Call
  // "containsData()" to make sure the data was actually received first.
  const QByteArray& data(size_t i) const;

  // If data for a particular reply index @p i exists, erase it from the map.
  void eraseData(size_t i) { m_receivedReplies.erase(i); }

signals:
  // When a reply is received for a particular index, this signal will be
  // emitted with the index.
  void received(size_t);

  // Signal a get request in the main thread
  void signalGet(QNetworkRequest request, size_t requestId);

  // Signal a post request in the main thread
  void signalPost(QNetworkRequest request, QByteArray data, size_t requestId);

private slots:
  // Handle a get request in the main thread
  void handleGet(QNetworkRequest request, size_t requestId);

  // Handle a post request in the main thread
  void handlePost(QNetworkRequest request, QByteArray data, size_t requestId);

  // Handles an error that a QNetworkReply sent.
  void handleError(QNetworkReply::NetworkError ec);

  // Handles a finished QNetworkReply object.
  void handleFinished();

private:
  // A shared pointer to the network access manager.
  std::shared_ptr<QNetworkAccessManager> m_networkManager;

  // The map of pending replies accessed via their unique index
  std::unordered_map<size_t, QNetworkReply*> m_pendingReplies;

  // The map of received replies accessed via their unique index
  std::unordered_map<size_t, QByteArray> m_receivedReplies;

  // This mutex gets locked for all reading and writing operations
  mutable std::mutex m_mutex;

  // A counter for the request indices
  size_t m_requestCounter;
};

#endif // HTTPREQUEST_H

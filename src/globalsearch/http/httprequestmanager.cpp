#include "httprequestmanager.h"

#include <QDebug>
#include <QNetworkRequest>

HttpRequestManager::HttpRequestManager(
    const std::shared_ptr<QNetworkAccessManager>& networkManager,
    QObject* parent) :
 QObject(parent),
 m_networkManager(networkManager),
 m_requestCounter(0)
{
  // This is done so that handleGet and handlePost are always ran in the
  // main thread
  connect(this, &HttpRequestManager::signalGet,
          this, &HttpRequestManager::handleGet);
  connect(this, &HttpRequestManager::signalPost,
          this, &HttpRequestManager::handlePost);
}

size_t HttpRequestManager::sendGet(QUrl url)
{
  std::unique_lock<std::mutex> lock(m_mutex);

  QNetworkRequest request(url);

  emit signalGet(request, m_requestCounter);

  return m_requestCounter++;
}

size_t HttpRequestManager::sendPost(QUrl url, const QByteArray& data)
{
  std::unique_lock<std::mutex> lock(m_mutex);

  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader,
                    "application/x-www-form-urlencoded");

  emit signalPost(request, data, m_requestCounter);

  return m_requestCounter++;
}

bool HttpRequestManager::containsData(size_t i) const
{
  std::unique_lock<std::mutex> lock(m_mutex);
  return m_receivedReplies.find(i) != m_receivedReplies.end();
}

const QByteArray& HttpRequestManager::data(size_t i) const
{
  std::unique_lock<std::mutex> lock(m_mutex);

  static const QByteArray empty = "";

  if (m_receivedReplies.find(i) == m_receivedReplies.end())
    return empty;

  return m_receivedReplies.at(i);
}

void HttpRequestManager::handleGet(QNetworkRequest request, size_t requestId)
{
  std::unique_lock<std::mutex> lock(m_mutex);

  QNetworkReply* reply = m_networkManager->get(request);

  connect(reply, (void (QNetworkReply::*)(QNetworkReply::NetworkError))
                 (&QNetworkReply::error),
          this, &HttpRequestManager::handleError);
  connect(reply, &QNetworkReply::finished,
          this, &HttpRequestManager::handleFinished);

  m_pendingReplies[requestId] = reply;
}

void HttpRequestManager::handlePost(QNetworkRequest request, QByteArray data,
                                    size_t requestId)
{
  std::unique_lock<std::mutex> lock(m_mutex);

  QNetworkReply* reply = m_networkManager->post(request, data);

  connect(reply, (void (QNetworkReply::*)(QNetworkReply::NetworkError))
                 (&QNetworkReply::error),
          this, &HttpRequestManager::handleError);
  connect(reply, &QNetworkReply::finished,
          this, &HttpRequestManager::handleFinished);

  m_pendingReplies[requestId] = reply;
}

void HttpRequestManager::handleError(QNetworkReply::NetworkError ec)
{
  std::unique_lock<std::mutex> lock(m_mutex);

  // Make sure the sender is a QNetworkReply
  QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
  if (!reply) {
    qDebug() << "Error in" << __FUNCTION__ << ": sender() is not a"
             << "QNetworkReply!";
    return;
  }

  // Make sure this HttpRequestManager owns this reply
  auto it = std::find_if(m_pendingReplies.begin(),
                         m_pendingReplies.end(),
                         [reply](const std::pair<size_t, QNetworkReply*>& item)
                         {
                           return reply == item.second;
                         });

  // If not, print an error and return
  if (it == m_pendingReplies.end()) {
    qDebug() << "Error in" << __FUNCTION__ << ": sender() is not owned by"
             << "this HttpRequestManager instance!";
    return;
  }

  size_t receivedInd = it->first;

  // Print a message for some of the more common errors
  if (ec == QNetworkReply::ConnectionRefusedError)
    qDebug() << "QNetworkReply received an error: connection refused";
  else if (ec == QNetworkReply::RemoteHostClosedError)
    qDebug() << "QNetworkReply received an error: remote host closed";
  else if (ec == QNetworkReply::HostNotFoundError)
    qDebug() << "QNetworkReply received an error: host not found";
  else if (ec == QNetworkReply::TimeoutError)
    qDebug() << "QNetworkReply received an error: timeout";
  else
    qDebug() << "QNetworkReply received error code:" << ec;

  m_receivedReplies[receivedInd] = reply->readAll();

  m_pendingReplies.erase(receivedInd);

  reply->deleteLater();

  // Emit a signal
  emit received(receivedInd);
}

void HttpRequestManager::handleFinished()
{
  std::unique_lock<std::mutex> lock(m_mutex);

  // Make sure the sender is a QNetworkReply
  QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
  if (!reply) {
    qDebug() << "Error in" << __FUNCTION__ << ": sender() is not a"
             << "QNetworkReply!";
    return;
  }

  // Make sure this HttpRequestManager owns this reply
  auto it = std::find_if(m_pendingReplies.begin(),
                         m_pendingReplies.end(),
                         [reply](const std::pair<size_t, QNetworkReply*>& item)
                         {
                           return reply == item.second;
                         });

  // If not, just return
  if (it == m_pendingReplies.end())
    return;

  size_t receivedInd = it->first;

  m_receivedReplies[receivedInd] = reply->readAll();

  m_pendingReplies.erase(receivedInd);

  reply->deleteLater();

  // Emit a signal
  emit received(receivedInd);
}

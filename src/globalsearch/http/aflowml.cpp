/**********************************************************************
  AflowML - Submit a machine learning calculation to the AFLOW server and
            get the results

  Copyright (C) 2017-2018 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <chrono>
#include <thread> // for sleep

#include <QDebug>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QtConcurrent>

#include "aflowml.h"
#include "httprequestmanager.h"

AflowML::AflowML(const std::shared_ptr<QNetworkAccessManager>& networkManager,
                 QObject* parent)
  : QObject(parent), m_httpRequestManager(networkManager, parent),
    m_requestCounter(0)
{
  // Qt is not aware of size_t for some reason...
  qRegisterMetaType<size_t>("size_t");
}

// Checks the url every 5 seconds for an update
void AflowML::checkLoop(QUrl url, size_t requestId)
{
  // This will run in a while loop until a result is obtained.
  while (true) {
    std::unique_lock<std::mutex> lock(m_mutex);

    QEventLoop loop;

    // Make a timer for timeout
    QTimer timer;
    // 10 seconds
    int timeOutTime = 10000;
    bool timedOut = false;

    // Quit the event loop on timeout and indicate that timeout occurred
    connect(&timer, &QTimer::timeout, [&timedOut](){ timedOut = true; });
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    // Quit the event loop when we get a response.
    connect(&m_httpRequestManager, &HttpRequestManager::received, &loop,
            &QEventLoop::quit);

    size_t replyInd = m_httpRequestManager.sendGet(url);

    // Wait for a response
    timer.start(timeOutTime);
    loop.exec();

    // If we timed out, just try again
    if (timedOut)
      continue;

    // Make sure it has the data we are looking for
    if (!m_httpRequestManager.containsData(replyInd)) {
      qDebug() << "Error in AflowML:" << __FUNCTION__
               << ": the HttpRequestManager does not contain the response"
               << "data!";
      return;
    }

    QByteArray response = m_httpRequestManager.data(replyInd);
    m_httpRequestManager.eraseData(replyInd);

    // Now read it with json. It should contain "status".
    QJsonDocument doc = QJsonDocument::fromJson(response);
    QJsonObject rootObject = doc.object();

    // It should contain a "status" entry
    if (!rootObject.contains("status")) {
      qDebug() << "Error in AflowML:" << __FUNCTION__
               << ": invalid aflow response:\n"
               << response;
      return;
    }

    QString status = rootObject.value("status").toString();

    // If it says "started" or "pending", wait 5 seconds and run the loop again
    if (status == "STARTED" || status == "PENDING") {
      // Free the mutex while waiting
      lock.unlock();

      // Wait 5 seconds and then run the check loop again
      std::this_thread::sleep_for(std::chrono::seconds(5));
      continue;
    }

    if (status != "SUCCESS") {
      qDebug() << "Error in AflowML:" << __FUNCTION__
               << ": job was not successful:\n"
               << response;
      return;
    }

    AflowMLData data;
    for (const auto& key : rootObject.keys()) {
      data[key.toStdString()] =
        rootObject.value(key).toVariant().toString().toStdString();
    }

    m_receivedData[requestId] = std::move(data);
    emit received(requestId);
    return;
  }
}

size_t AflowML::submitPoscar(const QString& poscar)
{
  size_t newInd = m_requestCounter++;

  // We need to run this in a separate thread
  QtConcurrent::run(this, &AflowML::_submitPoscar, poscar, newInd);
  return newInd;
}

void AflowML::_submitPoscar(QString poscar, size_t requestId)
{
  std::unique_lock<std::mutex> lock(m_mutex);

  // This function should always be called in a thread other than the
  // main thread.
  // We will use an event loop to block until the reply is received.
  QEventLoop loop;

  // Make a timer for timeout
  QTimer timer;
  // 10 seconds
  int timeOutTime = 10000;
  bool timedOut = false;

  // Quit the event loop on timeout and indicate that timeout occurred
  connect(&timer, &QTimer::timeout, [&timedOut](){ timedOut = true; });
  connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

  // Quit the event loop when we get a response.
  connect(&m_httpRequestManager, &HttpRequestManager::received, &loop,
          &QEventLoop::quit);

  QByteArray poscarData = "file=" + QUrl::toPercentEncoding(poscar);

  QUrl aflowServer("http://aflow.org/API/aflow-ml/v1.0/plmf/prediction");

  size_t replyInd = m_httpRequestManager.sendPost(aflowServer, poscarData);

  // Wait for a response
  timer.start(timeOutTime);
  loop.exec();

  // If we timed out, unlock the mutex and tail recurse the function
  if (timedOut) {
    lock.unlock();
    return _submitPoscar(poscar, requestId);
  }

  // Make sure it has the data we are looking for
  if (!m_httpRequestManager.containsData(replyInd)) {
    qDebug() << "Error in" << __FUNCTION__ << ": the HttpRequestManager"
             << "does not contain the response data!";
    return;
  }

  QByteArray response = m_httpRequestManager.data(replyInd);
  m_httpRequestManager.eraseData(replyInd);

  // Now read it with json. It should contain an id.
  QJsonDocument doc = QJsonDocument::fromJson(response);
  QJsonObject rootObject = doc.object();

  // It should contain an "id" entry
  if (!rootObject.contains("id")) {
    qDebug() << "Error in" << __FUNCTION__ << ": invalid aflow response:\n"
             << response;
    return;
  }

  QString id = rootObject.value("id").toString();

  // Create the reply url
  QUrl statusUrl("http://aflow.org/API/aflow-ml/v1.0/prediction/result/" + id);

  // Unlock the mutex so it can be locked in the next function
  lock.unlock();

  // Run the loop for checking the status of the job
  return checkLoop(statusUrl, requestId);
}

bool AflowML::containsData(size_t i) const
{
  std::unique_lock<std::mutex> lock(m_mutex);
  return m_receivedData.find(i) != m_receivedData.end();
}

const AflowMLData& AflowML::data(size_t i) const
{
  std::unique_lock<std::mutex> lock(m_mutex);

  static const AflowMLData empty;
  if (m_receivedData.find(i) == m_receivedData.end())
    return empty;

  return m_receivedData.at(i);
}

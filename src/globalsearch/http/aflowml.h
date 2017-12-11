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

#ifndef AFLOWML_H
#define AFLOWML_H

#include <atomic>
#include <memory>
#include <mutex>

#include <QObject>

#include "httprequestmanager.h"

typedef std::map<std::string, std::string> AflowMLData;

/**
 * This class submits a POSCAR to the Aflow ML server in order to get back
 * machine learning results from it. It generates a separate thread and runs
 * everything in the separate thread. submitPoscar() returns a request Id that
 * can be used to keep track of the request that was sent. And a received()
 * signal is emitted when the result from the machine learning calculation is
 * obtained.
 */
class AflowML : public QObject
{
  Q_OBJECT
public:
  explicit AflowML(const std::shared_ptr<QNetworkAccessManager>& networkManager,
                   QObject* parent = nullptr);

  // Submits a POSCAR and returns an index for the request ID.
  size_t submitPoscar(const QString& poscar);

  // Check to see if data has been received for reply index @p i.
  bool containsData(size_t i) const;

  // Get the received data for a particular reply index @p i. Call
  // "containsData()" to make sure the data was actually received first.
  const AflowMLData& data(size_t i) const;

  // If data for a particular reply index @p i exists, erase it from the map.
  void eraseData(size_t i) { m_receivedData.erase(i); }

signals:
  // When data is received for a particular index, this signal will be
  // emitted with the index.
  void received(size_t);

private:
  // The submitPoscar function to be ran in another thread
  void _submitPoscar(QString poscar, size_t requestId);

  // The check loop that will check the job status every 5 seconds. It runs
  // in a separate thread.
  void checkLoop(QUrl url, size_t requestId);

  // A copy of our HttpRequestManger
  HttpRequestManager m_httpRequestManager;

  // Store our received data in a map
  std::map<size_t, AflowMLData> m_receivedData;

  // For thread safety
  mutable std::mutex m_mutex;

  // A counter for requests
  std::atomic_size_t m_requestCounter;
};

#endif // AFLOWML_H

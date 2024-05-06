/*****************************************************************************
  LoadLevelerInterface - Base class for running jobs remotely on a cluster
  managed by LoadLeveler

  Copyright (C) 2012 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ****************************************************************************/

#ifndef LOADLEVELERINTERFACE_H
#define LOADLEVELERINTERFACE_H

#ifdef ENABLE_SSH

// Tell doxygen to skip this file
/// \cond

#include <globalsearch/queueinterfaces/loadlevelerdialog.h>
#include <globalsearch/queueinterfaces/remote.h>

#include <QDateTime>
#include <QReadWriteLock>
#include <QString>
#include <QStringList>

namespace GlobalSearch {

class LoadLevelerQueueInterface : public RemoteQueueInterface
{
  Q_OBJECT

public:
  explicit LoadLevelerQueueInterface(OptBase* parent,
                                     const QString& settingsFile = "");

  virtual ~LoadLevelerQueueInterface() override;

  virtual bool isReadyToSearch(QString* str) override;

  QDialog* dialog() override;

  friend class LoadLevelerConfigDialog;
  friend class LoadLevelerTest;

public slots:
  void readSettings(const QString& filename = "") override;
  void writeSettings(const QString& filename = "") override;
  bool startJob(Structure* s) override;
  bool stopJob(Structure* s) override;
  QueueInterface::QueueStatus getStatus(Structure* s) const override;

protected:
  // Unit test these:
  QString parseStatus(const QStringList& statusList, unsigned int jobId) const;
  unsigned int parseJobId(const QString& submissionOutput, bool* ok) const;
  // Fetches the queue from the server
  // With "true" argument, refresh is done regardless of the queue refresh interval
  QStringList getQueueList(bool forced = false) const;
  // Cached queue data
  QStringList m_queueData;
  // Limits queue checks to once per second
  QDateTime m_queueTimeStamp;
  // Locks for m_queueData;
  QReadWriteLock m_queueMutex;
};
}

#endif // ENABLE_SSH
#endif // LOADLEVELERINTERFACE_H

// End doxygen skip:
/// \endcond

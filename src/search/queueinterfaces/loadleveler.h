/**********************************************************************
  LoadLevelerInterface - Base class for running jobs on a LOADLEVELER cluster

  Copyright (C) 2012 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef LOADLEVELERINTERFACE_H
#define LOADLEVELERINTERFACE_H

// Tell doxygen to skip this file
/// \cond

#include <search/queueinterfaces/batch.h>

#include <QString>
#include <QStringList>

namespace Search {

// LoadLeveler queue interface.
class LoadLevelerQueueInterface : public BatchQueueInterface
{
  Q_OBJECT

public:
  // Default values.
  static const QueueDefaults& defaults();

  explicit LoadLevelerQueueInterface(SearchBase* parent,
                                     const QString& settingsFile = "");

  virtual ~LoadLevelerQueueInterface() override;

  friend class LoadLevelerTest;

protected:
  QString parseStatus(const QStringList& statusList, unsigned int jobId) const
  {
    QString rawStatus;
    parseQueueStatus(statusList, jobId, &rawStatus);
    return rawStatus;
  }

  unsigned int parseJobId(const QString& submissionOutput, bool* ok) const override;

  QueueInterface::QueueStatus parseQueueStatus(const QStringList& statusList, unsigned int jobId,
                                               QString* rawStatus = nullptr) const override;

  QString queueListCommand() const override;
};
}

#endif // LOADLEVELERINTERFACE_H

// End doxygen skip:
/// \endcond

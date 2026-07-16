/**********************************************************************
  LsfQueueInterface - Base class for running jobs on a LSF cluster.

  Copyright (C) 2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef LSFQUEUEINTERFACE_H
#define LSFQUEUEINTERFACE_H

// Tell doxygen to skip this file
/// \cond

#include <search/queueinterfaces/batch.h>

#include <QString>
#include <QStringList>

namespace Search {

// LSF queue interface.
class LsfQueueInterface : public BatchQueueInterface
{
  Q_OBJECT

public:
  // Default values.
  static const QueueDefaults& defaults();

  explicit LsfQueueInterface(SearchBase* parent, const QString& settingsFile = "");

  virtual ~LsfQueueInterface() override;

protected:
  unsigned int parseJobId(const QString& submissionOutput, bool* ok) const override;
  QueueInterface::QueueStatus parseQueueStatus(const QStringList& queueData, unsigned int jobId,
    QString* rawStatus = nullptr) const override;
};
}

#endif // LSFQUEUEINTERFACE_H

// End doxygen skip:
/// \endcond

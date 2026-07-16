/**********************************************************************
  SgeQueueInterface - Base class for running jobs on a SGE cluster.

  Copyright (C) 2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef SGEQUEUEINTERFACE_H
#define SGEQUEUEINTERFACE_H

// Tell doxygen to skip this file
/// \cond

#include <search/queueinterfaces/batch.h>

#include <QString>
#include <QStringList>

namespace Search {

// SGE queue interface.
class SgeQueueInterface : public BatchQueueInterface
{
  Q_OBJECT

public:
  // Default values.
  static const QueueDefaults& defaults();

  explicit SgeQueueInterface(SearchBase* parent, const QString& settingsFile = "");

  virtual ~SgeQueueInterface() override;

protected:
  unsigned int parseJobId(const QString& submissionOutput, bool* ok) const override;
  QueueInterface::QueueStatus parseQueueStatus(const QStringList& queueData, unsigned int jobId,
    QString* rawStatus = nullptr) const override;
};
}

// End doxygen skip:
/// \endcond

#endif // SGEQUEUEINTERFACE

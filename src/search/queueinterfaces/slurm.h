/**********************************************************************
  SlurmQueueInterface - Base class for running jobs on a SLURM cluster.

  Copyright (C) 2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef SLURMQUEUEINTERFACE_H
#define SLURMQUEUEINTERFACE_H

// Tell doxygen to skip this file
/// \cond

#include <search/queueinterfaces/batch.h>

#include <QString>
#include <QStringList>

namespace Search {

// SLURM queue interface.
class SlurmQueueInterface : public BatchQueueInterface
{
  Q_OBJECT

public:
  // Default values.
  static const QueueDefaults& defaults();

  explicit SlurmQueueInterface(SearchBase* parent,
                               const QString& settingsFile = "");

  virtual ~SlurmQueueInterface() override;

protected:
  unsigned int parseJobId(const QString& submissionOutput, bool* ok) const override;
  QueueInterface::QueueStatus parseQueueStatus(const QStringList& queueData, unsigned int jobId,
    QString* rawStatus = nullptr) const override;
  bool queueListCommandSucceeded(bool ok, int exitCode, const QString& stdoutText) const override;
};
}

#endif // SLURMQUEUEINTERFACE_H

// End doxygen skip:
/// \endcond

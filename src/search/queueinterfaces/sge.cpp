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

// Doxygen skip
/// @cond

#include <search/queueinterfaces/sge.h>
#include <common/fileutils.h>
#include <common/output.h>

#include <search/optimizer.h>
#include <search/structure.h>

#include <common/compatibility/qt_compat.h>
#include <QRegularExpression>

namespace Search {
namespace {

bool hasQueuedSgeState(const QString& status)
{
  return status.contains('q') || status.contains('w') || status.contains('s');
}

} // namespace

const QueueDefaults& SgeQueueInterface::defaults()
{
  static const QueueDefaults s{ "SGE", "job.sh", "qsub", "qstat", "qdel" };
  return s;
}

SgeQueueInterface::SgeQueueInterface(SearchBase* parent,
                                     const QString& /*settingsFile*/)
  : BatchQueueInterface(parent, QString())
{
  m_queueDefaults = &defaults();


}

SgeQueueInterface::~SgeQueueInterface()
{
}

unsigned int SgeQueueInterface::parseJobId(const QString& submissionOutput, bool* ok) const
{
  const QStringList list =
    submissionOutput.split(QRegularExpression("\\s+"), QtCompat::SkipEmptyParts);
  *ok = false;
  return list.size() >= 3 ? list.at(2).toUInt(ok) : 0;
}

QueueInterface::QueueStatus SgeQueueInterface::parseQueueStatus(
  const QStringList& queueData, unsigned int jobId, QString* rawStatus) const
{
  if (rawStatus)
    rawStatus->clear();

  for (const auto& line : queueData) {
    const QStringList list = line.split(' ', QtCompat::SkipEmptyParts);
    bool ok = false;
    const unsigned int currentJobId = list.isEmpty() ? 0 : list.at(0).toUInt(&ok);
    if (ok && currentJobId == jobId) {
      if (list.size() <= 4) {
        // The job is present but we can't parse the output: report the raw line so an empty
        //   so an empty rawStatus is distinguished from a missing job.
        if (rawStatus)
          *rawStatus = line;
        return QueueInterface::Unknown;
      }
      const QString status = list.at(4);
      if (rawStatus)
        *rawStatus = status;
      if (status.contains('r'))
        return QueueInterface::Running;
      if (status.contains('E'))
        return QueueInterface::Error;
      if (hasQueuedSgeState(status))
        return QueueInterface::Queued;
      return QueueInterface::Unknown;
    }
  }
  return QueueInterface::Unknown;
}
}

/// @endcond

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

// Doxygen skip:
/// @cond

#include <search/queueinterfaces/slurm.h>
#include <common/output.h>

#include <common/compatibility/qt_compat.h>
#include <QRegularExpression>

namespace Search {
namespace {

bool isRunningStatus(const QString& status)
{
  return status == "CA" || status == "CD" || status == "CG" ||
         status == "F" || status == "NF" || status == "R" || status == "S" || status == "TO";
}

bool isQueuedStatus(const QString& status)
{
  return status == "CF" || status == "PD";
}

} // namespace

const QueueDefaults& SlurmQueueInterface::defaults()
{
  static const QueueDefaults s{ "SLURM", "job.slurm", "sbatch", "squeue", "scancel" };
  return s;
}

SlurmQueueInterface::SlurmQueueInterface(SearchBase* parent,
                                         const QString& /*settingsFile*/)
  : BatchQueueInterface(parent, QString())
{
  m_queueDefaults = &defaults();


}

SlurmQueueInterface::~SlurmQueueInterface()
{
}

unsigned int SlurmQueueInterface::parseJobId(const QString& submissionOutput, bool* ok) const
{
  const QStringList list =
    submissionOutput.split(QRegularExpression("\\s+"), QtCompat::SkipEmptyParts);
  *ok = false;
  return list.size() >= 4 ? list.at(3).toUInt(ok) : 0;
}

QueueInterface::QueueStatus SlurmQueueInterface::parseQueueStatus(
  const QStringList& queueData, unsigned int jobId, QString* rawStatus) const
{
  if (rawStatus)
    rawStatus->clear();

  for (const auto& line : queueData) {
    const QStringList entryList = line.split(' ', QtCompat::SkipEmptyParts);
    bool ok = false;
    const unsigned int curJobId = entryList.isEmpty() ? 0 : entryList.first().toUInt(&ok);
    if (ok && curJobId == jobId) {
      if (entryList.size() <= 5) {
        // The job is present but we can't parse the output: report the raw line so an empty
        //   so an empty rawStatus is distinguished from a missing job.
        if (rawStatus)
          *rawStatus = line;
        return QueueInterface::Unknown;
      }
      const QString status = entryList.at(4);
      if (rawStatus)
        *rawStatus = status;
      if (isRunningStatus(status))
        return QueueInterface::Running;
      if (isQueuedStatus(status))
        return QueueInterface::Queued;
      return QueueInterface::Unknown;
    }
  }
  return QueueInterface::Unknown;
}

bool SlurmQueueInterface::queueListCommandSucceeded(
  bool ok, int exitCode, const QString& stdoutText) const
{
  return ok && exitCode == 0 && !stdoutText.isEmpty();
}

} // namespace Search

/// @endcond

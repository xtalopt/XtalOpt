/**********************************************************************
  PbsQueueInterface - Base class for running jobs on a PBS cluster.

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

#include <search/queueinterfaces/pbs.h>
#include <common/fileutils.h>
#include <common/output.h>

#include <search/optimizer.h>
#include <search/structure.h>

#include <common/compatibility/qt_compat.h>
#include <QString>
#include <QStringList>

namespace Search {

const QueueDefaults& PbsQueueInterface::defaults()
{
  static const QueueDefaults s{ "PBS", "job.pbs", "qsub", "qstat", "qdel" };
  return s;
}

PbsQueueInterface::PbsQueueInterface(SearchBase* parent,
                                     const QString& /*settingsFile*/)
  : BatchQueueInterface(parent, QString())
{
  m_queueDefaults = &defaults();


}

PbsQueueInterface::~PbsQueueInterface()
{
}

unsigned int PbsQueueInterface::parseJobId(const QString& submissionOutput, bool* ok) const
{
  const QStringList list = submissionOutput.split(".");
  *ok = false;
  return list.isEmpty() ? 0 : list.first().toUInt(ok);
}

QueueInterface::QueueStatus PbsQueueInterface::parseQueueStatus(
  const QStringList& queueData, unsigned int jobId, QString* rawStatus) const
{
  if (rawStatus)
    rawStatus->clear();

  for (const auto& line : queueData) {
    const QStringList entryList = line.split(' ', QtCompat::SkipEmptyParts);
    if (entryList.isEmpty())
      continue;

    bool ok = false;
    const unsigned int curJobId = entryList.first().split(".").first().toUInt(&ok);
    if (!ok || curJobId != jobId)
      continue;

    if (entryList.size() < 10) {
      Common::debug(QString("Skipping short qstat entry; need at least 10 fields: %1").arg(line));
      // The job is present but we can't parse the output: report the raw line so an empty
      //   so an empty rawStatus is distinguished from a missing job.
      if (rawStatus)
        *rawStatus = line;
      return QueueInterface::Unknown;
    }
    const QString status = entryList.at(9);
    if (rawStatus)
      *rawStatus = status;
    if (status.contains('R') || status.contains('E'))
      return QueueInterface::Running;
    if (status.contains('Q') || status.contains('H') ||
        status.contains('T') || status.contains('W') || status.contains('S'))
      return QueueInterface::Queued;
    return QueueInterface::Unknown;
  }

  return QueueInterface::Unknown;
}
}

/// @endcond

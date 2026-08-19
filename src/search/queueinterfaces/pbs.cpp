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
#include <QRegularExpression>

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

  int statusColumn = -1;
  const QRegularExpression statusHeader("(^|\\s)(S|State)(?=\\s|$)");
  for (const QString& line : queueData) {
    const QRegularExpressionMatch match = statusHeader.match(line);
    if (match.hasMatch() && line.contains("Job", Qt::CaseInsensitive)) {
      statusColumn = match.capturedStart(2);
      break;
    }
  }
  if (statusColumn < 0)
    return QueueInterface::Unknown;

  for (const auto& line : queueData) {
    const QStringList entryList = line.split(' ', QtCompat::SkipEmptyParts);
    if (entryList.isEmpty())
      continue;

    bool ok = false;
    const unsigned int curJobId = entryList.first().split(".").first().toUInt(&ok);
    if (!ok || curJobId != jobId)
      continue;

    const QString statusText = line.mid(statusColumn).section(QRegularExpression("\\s+"), 0, 0, QString::SectionSkipEmpty);
    if (statusText.isEmpty()) {
      if (rawStatus)
        *rawStatus = line;
      return QueueInterface::Unknown;
    }
    const QString status = statusText.left(1);
    if (rawStatus)
      *rawStatus = status;
    if (status == "B" || status == "E" || status == "R" || status == "S" ||
        status == "U" || status == "M" ||
        status == "C" || status == "F" || status == "X")
      return QueueInterface::Running;
    if (status == "Q" || status == "H" || status == "T" || status == "W")
      return QueueInterface::Queued;
    return QueueInterface::Unknown;
  }

  return QueueInterface::Unknown;
}
}

/// @endcond

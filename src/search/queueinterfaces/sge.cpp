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
  *ok = false;
  const QRegularExpression expression("^\\s*Your job\\s+(\\d+)(?:\\s+\\([^\\r\\n]*\\))?\\s+has been submitted\\s*$",
                                      QRegularExpression::MultilineOption);
  QRegularExpressionMatchIterator matches = expression.globalMatch(submissionOutput);
  if (!matches.hasNext())
    return 0;
  const QRegularExpressionMatch match = matches.next();
  if (matches.hasNext())
    return 0;
  return match.captured(1).toUInt(ok);
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
        // Keep the raw line so this is not mistaken for a missing job.
        if (rawStatus)
          *rawStatus = line;
        return QueueInterface::Unknown;
      }
      const QString status = list.at(4);
      if (rawStatus)
        *rawStatus = status;
      if (status.contains('E'))
        return QueueInterface::Error;
      if (status.contains('q') || status.contains('w') || status.contains('h'))
        return QueueInterface::Queued;
      if (status.contains('r') || status.contains('R') || status.contains('t') ||
          status.contains('s') || status.contains('S') || status.contains('T') || status.contains('d'))
        return QueueInterface::Running;
      return QueueInterface::Unknown;
    }
  }
  return QueueInterface::Unknown;
}
}

/// @endcond

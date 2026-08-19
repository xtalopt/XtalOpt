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
#include <search/search.h>

#include <common/compatibility/qt_compat.h>
#include <QRegularExpression>

namespace Search {

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
  *ok = false;
  const QRegularExpression expression("^\\s*Submitted batch job\\s+(\\d+)(?:\\s+on\\s+cluster\\s+[^\\r\\n]+)?\\s*$",
                                      QRegularExpression::MultilineOption);
  QRegularExpressionMatchIterator matches = expression.globalMatch(submissionOutput);
  if (!matches.hasNext())
    return 0;
  const QRegularExpressionMatch match = matches.next();
  if (matches.hasNext())
    return 0;
  return match.captured(1).toUInt(ok);
}

QueueInterface::QueueStatus SlurmQueueInterface::parseQueueStatus(
  const QStringList& queueData, unsigned int jobId, QString* rawStatus) const
{
  if (rawStatus)
    rawStatus->clear();

  for (const auto& line : queueData) {
    const QStringList entryList = line.split('|', QtCompat::KeepEmptyParts);
    bool ok = false;
    const unsigned int curJobId = entryList.size() == 2
      ? entryList.first().trimmed().toUInt(&ok) : 0;
    if (ok && curJobId == jobId) {
      const QString status = entryList.at(1).trimmed();
      if (rawStatus)
        *rawStatus = status.isEmpty() ? line : status;
      if (status == "R"  || status == "S"  || status == "ST" ||
          status == "CG" || status == "RS" || status == "SI" ||
          status == "SO" ||
          status == "BF" || status == "CA" || status == "CD" ||
          status == "DL" || status == "F"  || status == "NF" ||
          status == "OOM"|| status == "PR" || status == "RV" ||
          status == "TO")
        return QueueInterface::Running;
      if (status == "CF" || status == "PD" || status == "RD" ||
          status == "RF" || status == "RH" || status == "RQ" ||
          status == "SE")
        return QueueInterface::Queued;
      return QueueInterface::Unknown;
    }
  }
  return QueueInterface::Unknown;
}

QString SlurmQueueInterface::queueListCommand() const
{
  QString command = statusCommand() + " -h -o \"%i|%t\"";
  const QString username = m_search->getUsername().trimmed();
  if (!username.isEmpty())
    command += " -u " + username;
  return command;
}

} // namespace Search

/// @endcond

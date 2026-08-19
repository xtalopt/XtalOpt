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

// Doxygen skip:
/// @cond

#include <search/queueinterfaces/loadleveler.h>
#include <common/output.h>
#include <search/search.h>

#include <QRegularExpression>

namespace Search {

const QueueDefaults& LoadLevelerQueueInterface::defaults()
{
  static const QueueDefaults s{ "LoadLeveler", "job.ll", "llsubmit", "llq", "llcancel" };
  return s;
}

LoadLevelerQueueInterface::LoadLevelerQueueInterface(
  SearchBase* parent, const QString& /*settingsFile*/)
  : BatchQueueInterface(parent, QString())
{
  m_queueDefaults = &defaults();


}

LoadLevelerQueueInterface::~LoadLevelerQueueInterface()
{
}

QueueInterface::QueueStatus LoadLevelerQueueInterface::parseQueueStatus(
  const QStringList& statusList, unsigned int jobId, QString* rawStatus) const
{
  if (rawStatus)
    rawStatus->clear();

  const QRegularExpression statusCapture("^(.+)\\.(\\d+)\\.(\\d+)\\s+(\\w+)\\s*$");

  QString status;
  bool found = false;
  for (const QString& line : statusList) {
    const QRegularExpressionMatch match = statusCapture.match(line.trimmed());
    if (!match.hasMatch())
      continue;
    bool ok = false;
    const unsigned int currentJobId = match.captured(2).toUInt(&ok);
    if (!ok || currentJobId != jobId)
      continue;
    if (found) {
      if (rawStatus)
        *rawStatus = line;
      return QueueInterface::Unknown;
    }
    found = true;
    status = match.captured(4);
  }

  if (rawStatus)
    *rawStatus = status;

  static const QRegularExpression runningStatusMatcher("^(?:CK|CP|E|EP|MP|R|RP|ST|V|VP|C|CA|RM|TX)$");
  static const QRegularExpression queuedStatusMatcher("^(?:D|H|HS|I|NQ|P|S)$");
  static const QRegularExpression errorStatusMatcher("^(?:NR|SX|X|XP)$");

  if (runningStatusMatcher.match(status).hasMatch())
    return QueueInterface::Running;
  if (queuedStatusMatcher.match(status).hasMatch())
    return QueueInterface::Queued;
  if (errorStatusMatcher.match(status).hasMatch()) {
    Common::warning(tr("LoadLeveler returned an error status in the queue: %1").arg(status));
    return QueueInterface::Error;
  }
  return QueueInterface::Unknown;
}

QString LoadLevelerQueueInterface::queueListCommand() const
{
  QString command = statusCommand() + " -f %id %st";
  const QString username = m_search->getUsername().trimmed();
  if (!username.isEmpty())
    command += " -u " + username;
  return command;
}

unsigned int LoadLevelerQueueInterface::parseJobId(
  const QString& submissionOutput, bool* ok) const
{
  QRegularExpression idCapture(".*\".*\\.([0-9]+)\"");
  QRegularExpressionMatch idMatch = idCapture.match(submissionOutput);
  *ok = false;
  if (!idMatch.hasMatch()) {
    Common::error(tr("Cannot parse jobID from output: \"%1\"")
                    .arg(submissionOutput));
    return 0;
  }

  bool idIsInt;
  unsigned int jobId = idMatch.captured(1).toUInt(&idIsInt);
  if (!idIsInt || jobId == 0) {
    Common::error(tr("Invalid jobID. %1 output:\n%2\n"
                    "Parsed jobid: '%3' (must be a positive integer).")
                    .arg(submitCommand())
                    .arg(submissionOutput)
                    .arg(idMatch.captured(1)));
    return 0;
  }

  *ok = true;
  return jobId;
}

} // namespace Search

/// @endcond

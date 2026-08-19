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

// Doxygen skip:
/// @cond

#include <search/queueinterfaces/lsf.h>
#include <common/fileutils.h>
#include <common/output.h>

#include <search/optimizer.h>
#include <search/search.h>
#include <search/structure.h>

#include <common/compatibility/qt_compat.h>
#include <QRegularExpression>

namespace Search {

const QueueDefaults& LsfQueueInterface::defaults()
{
  static const QueueDefaults s{ "LSF", "job.lsf", "bsub", "bjobs", "bkill" };
  return s;
}

LsfQueueInterface::LsfQueueInterface(SearchBase* parent,
                                     const QString& /*settingsFile*/)
  : BatchQueueInterface(parent, QString())
{
  m_queueDefaults = &defaults();


}

LsfQueueInterface::~LsfQueueInterface()
{
}

unsigned int LsfQueueInterface::parseJobId(const QString& submissionOutput, bool* ok) const
{
  const QStringList list = submissionOutput.split(QRegularExpression("<|>"));
  *ok = false;
  return list.size() >= 2 ? list.at(1).toUInt(ok) : 0;
}

QueueInterface::QueueStatus LsfQueueInterface::parseQueueStatus(
  const QStringList& queueData, unsigned int jobId, QString* rawStatus) const
{
  if (rawStatus)
    rawStatus->clear();

  for (const auto& line : queueData) {
    const QStringList entryList = line.split(' ', QtCompat::SkipEmptyParts);
    bool ok = false;
    const unsigned int curJobId = entryList.isEmpty() ? 0 : entryList.first().toUInt(&ok);
    if (ok && curJobId == jobId) {
      if (entryList.size() < 2) {
        // Keep the raw line so this is not mistaken for a missing job.
        if (rawStatus)
          *rawStatus = line;
        return QueueInterface::Unknown;
      }
      const QString status = entryList.at(1);
      if (rawStatus)
        *rawStatus = status;
      if (status == "RUN" || status == "USUSP" || status == "SSUSP" || status == "UNKWN" ||
          status == "DONE" || status == "EXIT" || status == "ZOMBI" ||
          status == "POST_DONE" || status == "POST_ERR")
        return QueueInterface::Running;
      if (status == "PEND" || status == "PROV" || status == "PSUSP" ||
          status == "WAIT" || status == "FWD_PEND")
        return QueueInterface::Queued;
      return QueueInterface::Unknown;
    }
  }

  return QueueInterface::Unknown;
}

QString LsfQueueInterface::queueListCommand() const
{
  QString command = statusCommand() + " -noheader -o \"jobid stat\"";
  const QString username = m_search->getUsername().trimmed();
  if (!username.isEmpty())
    command += " -u " + username;
  return command;
}

bool LsfQueueInterface::queueListCommandSucceeded(
  bool ok, int exitCode, const QString& stdoutText, const QString& stderrText) const
{
  if (ok && exitCode == 0)
    return true;

  const QString output = stdoutText + "\n" + stderrText;

  // executeSSH in System ssh returns false for every exit-255 by construction; including
  //   an empty queue that can accompanied by a message.
  // Correctness of our workflow in this case relies on the exact messaging! Currently, IBM's
  //   documentation states it as "No unfinished job found".
  return exitCode == 255 && output.contains("No unfinished job found", Qt::CaseInsensitive);
}
}

/// @endcond

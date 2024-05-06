/**********************************************************************
  SgeQueueInterface - Base class for running jobs remotely on a SGE
  cluster.

  Copyright (C) 2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifdef ENABLE_SSH

// Doxygen skip
/// @cond

#include <globalsearch/queueinterfaces/sge.h>

#include <globalsearch/macros.h>
#include <globalsearch/optimizer.h>
#include <globalsearch/random.h>
#include <globalsearch/structure.h>

#include <QDir>
#include <QFile>

#include <QProcess>
namespace GlobalSearch {

SgeQueueInterface::SgeQueueInterface(OptBase* parent,
                                     const QString& settingsFile)
  : RemoteQueueInterface(parent, settingsFile),
    m_queueMutex(QReadWriteLock::Recursive)
{
  m_idString = "SGE";
  m_templates.append("job.sh");
  m_hasDialog = true;

  m_statusCommand = "qstat";
  m_submitCommand = "qsub";
  m_cancelCommand = "qdel";

  readSettings(settingsFile);
}

SgeQueueInterface::~SgeQueueInterface()
{
}

bool SgeQueueInterface::isReadyToSearch(QString* str)
{
  // Is a working directory specified?
  if (m_opt->locWorkDir.isEmpty()) {
    *str = tr("Local working directory is not set. Check your Queue "
              "configuration.");
    return false;
  }

  // Can we write to the working directory?
  QDir workingdir(m_opt->locWorkDir);
  bool writable = true;
  if (!workingdir.exists()) {
    if (!workingdir.mkpath(m_opt->locWorkDir)) {
      writable = false;
    }
  } else {
    // If the path exists, attempt to open a small test file for writing
    QString filename =
      m_opt->locWorkDir + QString("queuetest-") + QString::number(getRandUInt());
    QFile file(filename);
    if (!file.open(QFile::ReadWrite)) {
      writable = false;
    }
    file.remove();
  }
  if (!writable) {
    *str = tr("Cannot write to working directory '%1'.\n\nPlease "
              "change the permissions on this directory or specify "
              "a different one in the Queue configuration.")
             .arg(m_opt->locWorkDir);
    return false;
  }

  // Check all other parameters:
  if (m_opt->host.isEmpty()) {
    *str = tr("Hostname of SGE server is not set. Check your Queue "
              "configuration.");
    return false;
  }

  if (m_cancelCommand.isEmpty()) {
    *str = tr("qdel command is not set. Check your Queue "
              "configuration.");
    return false;
  }

  if (m_cancelCommand.isEmpty()) {
    *str = tr("qdel command is not set. Check your Queue "
              "configuration.");
    return false;
  }

  if (m_statusCommand.isEmpty()) {
    *str = tr("qstat command is not set. Check your Queue "
              "configuration.");
    return false;
  }

  if (m_submitCommand.isEmpty()) {
    *str = tr("qsub command is not set. Check your Queue "
              "configuration.");
    return false;
  }

  if (m_opt->remWorkDir.isEmpty()) {
    *str = tr("Remote working directory is not set. Check your Queue "
              "configuration.");
    return false;
  }

  if (m_opt->username.isEmpty()) {
    *str = tr("SSH username for SGE server is not set. Check your Queue "
              "configuration.");
    return false;
  }

  if (m_opt->port < 0) {
    *str = tr("SSH port is invalid (Port %1). Check your Queue "
              "configuration.")
             .arg(m_opt->port);
    return false;
  }

  *str = "";
  return true;
}

QDialog* SgeQueueInterface::dialog()
{
  if (!m_dialog) {
    if (!m_opt->dialog())
      return nullptr;
    m_dialog = new SgeConfigDialog(m_opt->dialog(), m_opt, this);
  }
  SgeConfigDialog* d = qobject_cast<SgeConfigDialog*>(m_dialog);
  d->updateGUI();

  return d;
}

void SgeQueueInterface::readSettings(const QString& filename)
{
  SETTINGS(filename);

  // Figure out what opt index this is.
  int optInd = m_opt->queueInterfaceIndex(this);
  if (optInd < 0)
    return;

  settings->beginGroup(m_opt->getIDString().toLower());
  settings->beginGroup("queueinterface/sgequeueinterface");
  settings->beginGroup(QString::number(optInd));
  int loadedVersion = settings->value("version", 0).toInt();
  settings->beginGroup("paths");

  m_submitCommand = settings->value("qsub", "qsub").toString();
  m_statusCommand = settings->value("qstat", "qstat").toString();
  m_cancelCommand = settings->value("qdel", "qdel").toString();

  settings->endGroup();
  settings->endGroup();
  settings->endGroup();
  settings->endGroup();

  // Update config data
  switch (loadedVersion) {
    case 0: // Load old stuff from /sys/ block
      settings->beginGroup(m_opt->getIDString().toLower());
      settings->beginGroup("sys");
      m_submitCommand = settings->value("queue/qsub", "qsub").toString();
      m_statusCommand = settings->value("queue/qstat", "qstat").toString();
      m_cancelCommand = settings->value("queue/qdel", "qdel").toString();
      settings->endGroup();
      settings->endGroup();
    case 1:
    default:
      break;
  }
}

void SgeQueueInterface::writeSettings(const QString& filename)
{
  SETTINGS(filename);

  const int version = 1;

  // Figure out what opt index this is.
  int optInd = m_opt->queueInterfaceIndex(this);
  if (optInd < 0)
    return;

  settings->beginGroup(m_opt->getIDString().toLower());
  settings->beginGroup("queueinterface/sgequeueinterface");
  settings->beginGroup(QString::number(optInd));
  settings->setValue("version", version);
  settings->beginGroup("paths");

  settings->setValue("qsub", m_submitCommand);
  settings->setValue("qstat", m_statusCommand);
  settings->setValue("qdel", m_cancelCommand);

  settings->endGroup();
  settings->endGroup();
  settings->endGroup();
  settings->endGroup();
}

bool SgeQueueInterface::startJob(Structure* s)
{
  QWriteLocker wlocker(&s->lock());
  QString command = m_submitCommand + " job.sh";
  QString stdout_str, stderr_str;
  int ec;

  if (!this->runACommand(s->getRempath(), command, &stdout_str, &stderr_str, &ec))
    return false;

  // Assuming stdout_str value is
  // your job <jobID> ('job.sh') has been submitted
  unsigned int jobID = stdout_str.split(QRegExp("\\s+"))[2].toUInt();

  s->setJobID(jobID);
  s->startOptTimer();
  // This is done to make sure at least one queue list refresh is done after
  //   the job is submitted and before the first status check. Otherwise, sometimes
  //   using the old queue list might cause the code believe that the job is
  //   missing and with an existing output file, the run will be marked as failed.
  getQueueList(true);
  return true;
}

bool SgeQueueInterface::stopJob(Structure* s)
{
  // lock structure
  QWriteLocker locker(&s->lock());

  // Log error dir if needed
  if (this->m_opt->m_logErrorDirs && (s->getStatus() == Structure::Error ||
                                      s->getStatus() == Structure::Restart)) {
    this->logErrorDirectory(s);
  }

  // jobid has not been set, cannot delete!
  if (s->getJobID() == 0) {
    if (m_opt->cleanRemoteOnStop()) {
      this->cleanRemoteDirectory(s);
    }
    return true;
  }

  // Execute
  const QString command =
    m_cancelCommand + " " + QString::number(s->getJobID());
  QString stdout_str;
  QString stderr_str;
  int ec;
  bool ret = true;

  if (!this->runACommand("", command, &stdout_str, &stderr_str, &ec)) {
    // Most likely job is already gone from queue.
    ret = false;
  }

  s->setJobID(0);
  s->stopOptTimer();
  return ret;
}

QueueInterface::QueueStatus SgeQueueInterface::getStatus(Structure* s) const
{
  // lock structure
  QWriteLocker locker(&s->lock());
  QStringList queueData = getQueueList();
  unsigned int jobID = static_cast<unsigned int>(s->getJobID());

  // If the queueData cannot be fetched, queueData contains a single
  // string, "CommError"
  if (queueData.size() == 1 && queueData[0].compare("CommError") == 0) {
    return QueueInterface::CommunicationError;
  }

  // If jobID = 0 and structure is not in "Submitted" state, return an error.
  if (!jobID && s->getStatus() != Structure::Submitted) {
    return QueueInterface::Error;
  }

  // Determine status if structure is in the queue. queueData
  // entries are of the following format:
  //
  // job-ID prior name  user  state submit/start at queue   function
  // 231    0     hydra craig r     07/13/96        durin.q MASTER
  //                                20:27:15
  //
  // (Note that the whitespace has been condensed in the above)
  QString status;
  QStringList list;
  for (int i = 0; i < queueData.size(); i++) {
    list = queueData.at(i).split(QRegExp("\\s+"), QString::SkipEmptyParts);
    if (list[0].toUInt() == jobID) {
      status = list[4];
      continue;
    }
  }

  // If structure is submitted, check if it is in the queue. If not,
  // check if the completion file has been written.
  //
  // If the completion file exists, then the job finished before the
  // queue checks could see it, and the function will continue on to
  // the status checks below.
  //
  // If the structure in Submitted state
  if (s->getStatus() == Structure::Submitted) {
    // and the jobID isn't in the queue
    if (status.isEmpty()) {
      // check if the output file is absent
      bool exists;
      if (!getCurrentOptimizer(s)->checkIfOutputFileExists(s, &exists)) {
        return QueueInterface::CommunicationError;
      }
      if (!exists) {
        // The job is still pending
        return QueueInterface::Pending;
      } else {
        // The job has completed.
        return QueueInterface::Started;
      }
    } else {
      // The job is in the queue
      return QueueInterface::Started;
    }
  }

  if (status.contains('r')) {
    return QueueInterface::Running;
  } else if (status.contains(QRegExp("q|w|s"))) {
    return QueueInterface::Queued;
  } else { // Entry is missing from queue. Were the output files written?
    locker.unlock();
    bool outputFileExists;
    if (!getCurrentOptimizer(s)->checkIfOutputFileExists(s,
                                                         &outputFileExists)) {
      return QueueInterface::CommunicationError;
    }
    locker.relock();

    if (outputFileExists) {
      // Did the job finish successfully?
      bool success;
      if (!getCurrentOptimizer(s)->checkForSuccessfulOutput(s, &success)) {
        return QueueInterface::CommunicationError;
      }
      if (success) {
        return QueueInterface::Success;
      } else {
        return QueueInterface::Error;
      }
    }
  }
  // Not in queue and no output?
  return QueueInterface::Unknown;
}

// The forced input parameter has a default False value
QStringList SgeQueueInterface::getQueueList(bool forced) const
{
  // recast queue mutex as mutable for safe access:
  QReadWriteLock& queueMutex = const_cast<QReadWriteLock&>(m_queueMutex);

  queueMutex.lockForRead();

  // Limit queries to once per queueRefreshInterval
  if (!forced && m_queueTimeStamp.isValid() &&
// QDateTime::msecsTo is not implemented until Qt 4.7
#if QT_VERSION >= 0x040700
      m_queueTimeStamp.msecsTo(QDateTime::currentDateTime()) <=
        1000 * m_opt->queueRefreshInterval()
#else
      // Check if day is the same. If not, refresh. Otherwise check
      // msecsTo current time
      (m_queueTimeStamp.date() == QDate::currentDate() &&
       m_queueTimeStamp.time().msecsTo(QTime::currentTime()) <=
         1000 * m_opt->queueRefreshInterval())
#endif // QT_VERSION >= 4.7
        ) {
    // If the cache is valid, return it
    QStringList ret(m_queueData);
    queueMutex.unlock();
    return ret;
  }

  // Otherwise, store a copy of the current timestamp and switch
  // queuemutex to writelock
  QDateTime oldTimeStamp(m_queueTimeStamp);
  queueMutex.unlock();

  // Relock mutex
  QWriteLocker queueLocker(&queueMutex);

  // Non-fatal assert: Check current timestamp against the
  // oldTimeStamp from earlier. If they don't match, another thread
  // has already updated the queueData, so tail-recurse this
  // function and try again.
  if (m_queueTimeStamp.time().msecsTo(oldTimeStamp.time()) != 0) {
    queueLocker.unlock();
    return this->getQueueList();
  }

  // Queue will be updated -- cast queue cache and timestamp as
  // mutable
  QStringList& queueData = const_cast<QStringList&>(m_queueData);
  QDateTime& queueTimeStamp = const_cast<QDateTime&>(m_queueTimeStamp);

  QString command = m_statusCommand + " | grep " + m_opt->username;

  // Execute
  QString stdout_str;
  QString stderr_str;
  int ec;
  bool ok;
  // Valid exit codes for grep: (0) matches found, execution successful
  //                            (1) no matches found, execution successful
  //                            (2) execution unsuccessful
  ok = this->runACommand("", command, &stdout_str, &stderr_str, &ec);

  if (!ok || (ec != 0 && ec != 1)) {
    m_opt->warning(tr("Error executing %1: (%2) %3\n\t"
          "Using cached queue data.")
        .arg(command)
        .arg(QString::number(ec))
        .arg(stderr_str));
    queueTimeStamp = QDateTime::currentDateTime();
    QStringList ret(m_queueData);
    return ret;
  }

  queueData = stdout_str.split("\n", QString::SkipEmptyParts);

  QStringList ret(m_queueData);
  queueTimeStamp = QDateTime::currentDateTime();
  return ret;

}
}

/// @endcond

#endif // ENABLE_SSH

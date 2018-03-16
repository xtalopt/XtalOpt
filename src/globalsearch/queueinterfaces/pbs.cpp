/**********************************************************************
  PbsQueueInterface - Base class for running jobs remotely on a PBS
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

// Doxygen skip:
/// @cond

#include <globalsearch/queueinterfaces/pbs.h>

#include <globalsearch/macros.h>
#include <globalsearch/optimizer.h>
#include <globalsearch/random.h>
#include <globalsearch/sshconnection.h>
#include <globalsearch/sshmanager.h>
#include <globalsearch/structure.h>

#include <QDir>
#include <QFile>
#include <QString>
#include <QStringList>

namespace GlobalSearch {

PbsQueueInterface::PbsQueueInterface(OptBase* parent,
                                     const QString& settingsFile)
  : RemoteQueueInterface(parent, settingsFile),
    m_queueMutex(QReadWriteLock::Recursive)
{
  m_idString = "PBS";
  m_templates.append("job.pbs");
  m_hasDialog = true;

  m_statusCommand = "qstat";
  m_submitCommand = "qsub";
  m_cancelCommand = "qdel";

  readSettings(settingsFile);
}

PbsQueueInterface::~PbsQueueInterface()
{
}

bool PbsQueueInterface::isReadyToSearch(QString* str)
{
  // Is a working directory specified?
  if (m_opt->filePath.isEmpty()) {
    *str = tr("Local working directory is not set. Check your Queue "
              "configuration.");
    return false;
  }

  // Can we write to the working directory?
  QDir workingdir(m_opt->filePath);
  bool writable = true;
  if (!workingdir.exists()) {
    if (!workingdir.mkpath(m_opt->filePath)) {
      writable = false;
    }
  } else {
    // If the path exists, attempt to open a small test file for writing
    QString filename =
      m_opt->filePath + QString("queuetest-") + QString::number(getRandUInt());
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
             .arg(m_opt->filePath);
    return false;
  }

  // Check all other parameters:
  if (m_opt->host.isEmpty()) {
    *str = tr("Hostname of PBS server is not set. Check your Queue "
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

  if (m_opt->rempath.isEmpty()) {
    *str = tr("Remote working directory is not set. Check your Queue "
              "configuration.");
    return false;
  }

  if (m_opt->username.isEmpty()) {
    *str = tr("SSH username for PBS server is not set. Check your Queue "
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

QDialog* PbsQueueInterface::dialog()
{
  if (!m_dialog) {
    if (!m_opt->dialog())
      return nullptr;
    m_dialog = new PbsConfigDialog(m_opt->dialog(), m_opt, this);
  }
  PbsConfigDialog* d = qobject_cast<PbsConfigDialog*>(m_dialog);
  d->updateGUI();

  return d;
}

void PbsQueueInterface::readSettings(const QString& filename)
{
  SETTINGS(filename);

  // Figure out what opt index this is.
  int optInd = m_opt->queueInterfaceIndex(this);
  if (optInd < 0)
    return;

  settings->beginGroup(m_opt->getIDString().toLower());
  settings->beginGroup("queueinterface/pbsqueueinterface");
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

void PbsQueueInterface::writeSettings(const QString& filename)
{
  SETTINGS(filename);

  const int version = 1;

  // Figure out what opt index this is.
  int optInd = m_opt->queueInterfaceIndex(this);
  if (optInd < 0)
    return;

  settings->beginGroup(m_opt->getIDString().toLower());
  settings->beginGroup("queueinterface/pbsqueueinterface");
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

bool PbsQueueInterface::startJob(Structure* s)
{
  SSHConnection* ssh = m_opt->ssh()->getFreeConnection();

  if (ssh == nullptr) {
    m_opt->warning(tr("Cannot connect to ssh server"));
    return false;
  }

  QWriteLocker wlocker(&s->lock());

  QString command =
    "cd \"" + s->getRempath() + "\" && " + m_submitCommand + " job.pbs";

  QString stdout_str;
  QString stderr_str;
  int ec;
  if (!ssh->execute(command, stdout_str, stderr_str, ec) || ec != 0) {
    m_opt->warning(tr("Error executing %1: %2").arg(command).arg(stderr_str));
    m_opt->ssh()->unlockConnection(ssh);
    return false;
  }
  m_opt->ssh()->unlockConnection(ssh);

  // Assuming stdout_str value is <jobID>.trailing.garbage.hostname.edu or
  // similar
  QStringList list = stdout_str.split(".");
  bool ok;
  unsigned int jobID;
  if (list.size()) {
    jobID = list.first().toUInt(&ok);
  } else {
    ok = false;
  }

  if (!ok) {
    m_opt->warning(
      tr("Error retrieving jobID for structure %1.").arg(s->getIDString()));
    return false;
  }

  s->setJobID(jobID);
  s->startOptTimer();
  return true;
}

bool PbsQueueInterface::stopJob(Structure* s)
{
  SSHConnection* ssh = m_opt->ssh()->getFreeConnection();

  if (ssh == nullptr) {
    m_opt->warning(tr("Cannot connect to ssh server"));
    return false;
  }

  // lock structure
  QWriteLocker locker(&s->lock());

  // Log errors if needed
  if (this->m_opt->m_logErrorDirs && (s->getStatus() == Structure::Error ||
                                      s->getStatus() == Structure::Restart)) {
    this->logErrorDirectory(s, ssh);
  }

  // jobid has not been set, cannot delete!
  if (s->getJobID() == 0) {
    if (m_opt->cleanRemoteOnStop()) {
      this->cleanRemoteDirectory(s, ssh);
    }
    m_opt->ssh()->unlockConnection(ssh);
    return true;
  }

  const QString command =
    m_cancelCommand + " " + QString::number(s->getJobID());

  // Execute
  QString stdout_str;
  QString stderr_str;
  int ec;
  bool ret = true;
  if (!ssh->execute(command, stdout_str, stderr_str, ec) || ec != 0) {
    // Most likely job is already gone from queue.
    ret = false;
  }

  s->setJobID(0);
  s->stopOptTimer();
  m_opt->ssh()->unlockConnection(ssh);
  return ret;
}

QueueInterface::QueueStatus PbsQueueInterface::getStatus(Structure* s) const
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

  // Determine status if structure is in the queue
  QString status;
  int i = queueData.indexOf(QRegExp("^" + QString::number(jobID) + ".*"));
  if (i != -1) {
    QStringList entryList = queueData.at(i).split(QRegExp("\\s+"));
    if (entryList.size() < 10) {
      m_opt->debug(QString("Skipping shot qstat entry; need at least 10"
                           "fields: %1")
                     .arg(queueData.at(i)));
    } else {
      status = entryList.at(9);
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

  if (status.contains(QRegExp("R|E"))) {
    return QueueInterface::Running;
  } else if (status.contains(QRegExp("Q|H|T|W|S"))) {
    return QueueInterface::Queued;
  } else if (status.isEmpty()) { // Entry is missing from queue. Were the output
                                 // files written?
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
    // Not in queue and no output?
    //
    // I've seen this a few times when mpd dies unexpectedly and the
    // output files are never copied back. Just restart.
    m_opt->debug(tr("Structure %1 with jobID %2 is missing "
                    "from the queue and has not written any output.")
                   .arg(s->getIDString())
                   .arg(s->getJobID()));
    return QueueInterface::Error;
  }
  // Unrecognized status:
  else {
    m_opt->debug(tr("Structure %1 with jobID %2 has "
                    "unrecognized status: %3")
                   .arg(s->getIDString())
                   .arg(s->getJobID())
                   .arg(status));
    return QueueInterface::Unknown;
  }
}

QStringList PbsQueueInterface::getQueueList() const
{
  // recast queue mutex as mutable for safe access:
  QReadWriteLock& queueMutex = const_cast<QReadWriteLock&>(m_queueMutex);

  queueMutex.lockForRead();

  // Limit queries to once per second
  if (m_queueTimeStamp.isValid() &&
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

  // Get SSH connection
  SSHConnection* ssh = m_opt->ssh()->getFreeConnection();

  if (ssh == nullptr) {
    m_opt->warning(tr("Cannot connect to ssh server"));
    queueTimeStamp = QDateTime::currentDateTime();
    queueData.clear();
    queueData << "CommError";
    QStringList ret(m_queueData);
    return ret;
  }

  QString command = m_statusCommand + " -u " + m_opt->username;

  // Execute
  QString stdout_str;
  QString stderr_str;
  int ec;
  // Valid exit codes for grep: (0) matches found, execution successful
  //                            (1) no matches found, execution successful
  //                            (2) execution unsuccessful
  if (!ssh->execute(command, stdout_str, stderr_str, ec) ||
      (ec != 0 && ec != 1)) {
    m_opt->ssh()->unlockConnection(ssh);
    m_opt->warning(tr("Error executing %1: (%2) %3\n\t"
                      "Using cached queue data.")
                     .arg(command)
                     .arg(QString::number(ec))
                     .arg(stderr_str));
    queueTimeStamp = QDateTime::currentDateTime();
    QStringList ret(m_queueData);
    return ret;
  }
  m_opt->ssh()->unlockConnection(ssh);

  queueData = stdout_str.split("\n", QString::SkipEmptyParts);

  QStringList ret(m_queueData);
  queueTimeStamp = QDateTime::currentDateTime();
  return ret;
}
}

/// @endcond

#endif // ENABLE_SSH

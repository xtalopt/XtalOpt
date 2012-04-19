/*****************************************************************************
  LoadLevelerInterface - Base class for running jobs remotely on a cluster
  managed by LoadLeveler

  Copyright (C) 2012 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ****************************************************************************/

#ifdef ENABLE_SSH

// Doxygen skip:
/// @cond

#include <globalsearch/queueinterfaces/loadleveler.h>

#include <globalsearch/macros.h>
#include <globalsearch/optimizer.h>
#include <globalsearch/sshconnection.h>
#include <globalsearch/sshmanager.h>
#include <globalsearch/structure.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace GlobalSearch {

LoadLevelerQueueInterface::LoadLevelerQueueInterface(
    OptBase *parent, const QString &settingsFile) :
  RemoteQueueInterface(parent, settingsFile),
  m_llq("llq"),
  m_llsubmit("llsubmit"),
  m_llcancel("llcancel"),
  m_interval(20),
  m_cleanRemoteOnStop(false)
{
  m_idString = "LoadLeveler";
  m_templates.clear();
  m_templates.append("job.ll");
  m_hasDialog = true;

  readSettings(settingsFile);
}

LoadLevelerQueueInterface::~LoadLevelerQueueInterface()
{
}

bool LoadLevelerQueueInterface::isReadyToSearch(QString *str)
{
  // Is a working directory specified?
  if (m_opt->filePath.isEmpty()) {
    *str = tr("Local working directory is not set. Check your Queue "
              "configuration.");
    return false;
  }

  // Can we write to the working directory?
  QDir workingdir (m_opt->filePath);
  bool writable = true;
  if (!workingdir.exists()) {
    if (!workingdir.mkpath(m_opt->filePath)) {
      writable = false;
    }
  }
  else {
    // If the path exists, attempt to open a small test file for writing
    QString filename = m_opt->filePath + QString("queuetest-")
        + QString::number(RANDUINT());
    QFile file (filename);
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
    *str = tr("Hostname of LoadLeveler server is not set. Check your Queue "
              "configuration.");
    return false;
  }

  if (m_llcancel.isEmpty()) {
    *str = tr("llcancel command is not set. Check your Queue "
              "configuration.");
    return false;
  }

  if (m_llq.isEmpty()) {
    *str = tr("llq command is not set. Check your Queue "
              "configuration.");
    return false;
  }

  if (m_llsubmit.isEmpty()) {
    *str = tr("llsubmit command is not set. Check your Queue "
              "configuration.");
    return false;
  }

  if (m_opt->rempath.isEmpty()) {
    *str = tr("Remote working directory is not set. Check your Queue "
              "configuration.");
    return false;
  }

  if (m_opt->username.isEmpty()) {
    *str = tr("SSH username for LoadLeveler server is not set. Check your Queue "
              "configuration.");
    return false;
  }

  if (m_opt->port < 0) {
    *str = tr("SSH port is invalid (Port %1). Check your Queue "
              "configuration.").arg(m_opt->port);
    return false;
  }

  *str = "";
  return true;
}

QDialog* LoadLevelerQueueInterface::dialog()
{
  if (!m_dialog) {
    m_dialog = new LoadLevelerConfigDialog (m_opt->dialog(),
                                            m_opt,
                                            this);
  }
  LoadLevelerConfigDialog *d =
      qobject_cast<LoadLevelerConfigDialog*>(m_dialog);
  d->updateGUI();

  return d;
}

void LoadLevelerQueueInterface::readSettings(const QString &filename)
{
  SETTINGS(filename);

  settings->beginGroup(m_opt->getIDString().toLower());
  settings->beginGroup("queueinterface/loadlevelerqueueinterface");
  int loadedVersion = settings->value("version", 0).toInt();
  settings->beginGroup("paths");

  m_llsubmit  = settings->value("llsubmit", "llsubmit").toString();
  m_llq       = settings->value("llq",      "llq").toString();
  m_llcancel  = settings->value("llcancel", "llcancel").toString();
  this->setInterval(settings->value("interval", 20).toInt());
  m_cleanRemoteOnStop = settings->value("cleanRemoteOnStop", false).toBool();

  settings->endGroup();
  settings->endGroup();
  settings->endGroup();

  DESTROY_SETTINGS(filename);

  // Update config data
  switch (loadedVersion) {
  case 0:
  case 1:
  default:
    break;
  }

}

void LoadLevelerQueueInterface::writeSettings(const QString &filename)
{
  SETTINGS(filename);

  const int VERSION = 1;

  settings->beginGroup(m_opt->getIDString().toLower());
  settings->beginGroup("queueinterface/loadlevelerqueueinterface");
  settings->setValue("version", VERSION);
  settings->beginGroup("paths");

  settings->setValue("llsubmit",  m_llsubmit);
  settings->setValue("llq", m_llq);
  settings->setValue("llcancel",  m_llcancel);
  settings->setValue("interval",  m_interval);
  settings->setValue("cleanRemoteOnStop", m_cleanRemoteOnStop);

  settings->endGroup();
  settings->endGroup();
  settings->endGroup();

  DESTROY_SETTINGS(filename);
}

bool LoadLevelerQueueInterface::startJob(Structure *s)
{
  SSHConnection *ssh = m_opt->ssh()->getFreeConnection();

  if (ssh == NULL) {
    m_opt->warning(tr("Cannot connect to ssh server"));
    return false;
  }

  QWriteLocker wlocker (s->lock());

  QString command = "cd \"" + s->getRempath() + "\" && " +
      m_llsubmit + " job.ll";

  QString stdout_str;
  QString stderr_str;
  int ec;
  if (!ssh->execute(command, stdout_str, stderr_str, ec) || ec != 0) {
    m_opt->warning(tr("Error executing %1: %2")
                   .arg(command).arg(stderr_str));
    m_opt->ssh()->unlockConnection(ssh);
    return false;
  }
  m_opt->ssh()->unlockConnection(ssh);

  bool ok;
  unsigned int jobId = this->parseJobId(stdout_str, &ok);
  if (!ok) {
    // Ill-formed output
    m_opt->warning(tr("Cannot parse jobID for Structure %1. Command: \"%2\" "
                      "Output: \"%3\"")
                   .arg(s->getIDString())
                   .arg(command)
                   .arg(stdout_str));
    return false;
  }

  s->setJobID(jobId);
  s->startOptTimer();
  return true;
}

bool LoadLevelerQueueInterface::stopJob(Structure *s)
{
  SSHConnection *ssh = m_opt->ssh()->getFreeConnection();

  if (ssh == NULL) {
    m_opt->warning(tr("Cannot connect to ssh server"));
    return false;
  }

  // lock structure
  QWriteLocker locker (s->lock());

  // jobid has not been set, cannot delete!
  if (s->getJobID() == 0) {
    if (m_cleanRemoteOnStop) {
      this->cleanRemoteDirectory(s, ssh);
    }
    m_opt->ssh()->unlockConnection(ssh);
    return true;
  }

  const QString command = m_llcancel + " " + QString::number(s->getJobID());

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

QueueInterface::QueueStatus LoadLevelerQueueInterface::getStatus(Structure *s) const
{
  // lock structure
  QWriteLocker locker (s->lock());
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

  QString status = this->parseStatus(queueData, jobID);

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
      if (!m_opt->optimizer()->checkIfOutputFileExists(s, &exists)) {
        return QueueInterface::CommunicationError;
      }
      if (!exists) {
        // The job is still pending
        return QueueInterface::Pending;
      }
      else {
        // The job has completed.
        return QueueInterface::Started;
      }
    }
    else {
      // The job is in the queue
      return QueueInterface::Started;
    }
  }

  // Parse specific statuses here:
  QRegExp runningStatusMatcher ("C|CP|D|E|EP|MP|NR|NQ|R|RM|RP|ST|TX|V|VP");
  QRegExp queuedStatusMatcher ("H|HS|I|S");
  QRegExp errorStatusMatcher ("SX|X|XP");
  if (runningStatusMatcher.exactMatch(status))
      return QueueInterface::Running;
  else if (queuedStatusMatcher.exactMatch(status)) {
    return QueueInterface::Queued;
  }
  else if (errorStatusMatcher.exactMatch(status)) {
    m_opt->warning(tr("Structure %1 returned an error status in the queue: %2")
                   .arg(s->getIDString()).arg(status));
    return QueueInterface::Error;
  }
  else if (status.isEmpty()) { // Entry is missing from queue. Were the output files written?
    locker.unlock();
    bool outputFileExists;
    if (!m_opt->optimizer()->checkIfOutputFileExists(s, &outputFileExists) ) {
      return QueueInterface::CommunicationError;
    }
    locker.relock();

    if (outputFileExists) {
      // Did the job finish successfully?
      bool success;
      if (!m_opt->optimizer()->checkForSuccessfulOutput(s, &success)) {
        return QueueInterface::CommunicationError;
      }
      if (success) {
        return QueueInterface::Success;
      }
      else {
        return QueueInterface::Error;
      }
    }
    // Not in queue and no output?
    //
    // I've seen this a few times when mpd dies unexpectedly and the
    // output files are never copied back. Just restart.
    m_opt->debug(tr("Structure %1 with jobID %2 is missing "
                    "from the queue and has not written any output.")
                 .arg(s->getIDString()).arg(s->getJobID()));
    return QueueInterface::Error;
  }
  // Unrecognized status:
  else {
    m_opt->debug(tr("Structure %1 with jobID %2 has "
                    "unrecognized status: %3")
                 .arg(s->getIDString()).arg(s->getJobID())
                 .arg(status));
    return QueueInterface::Unknown;
  }
}

void LoadLevelerQueueInterface::setInterval(const int sec)
{
  m_queueMutex.lockForWrite();
  m_interval = sec;
  m_queueMutex.unlock();
}

QString LoadLevelerQueueInterface::parseStatus(const QStringList &statusList,
                                               unsigned int jobId) const
{
  /* Format is:
$ llq -u brownap
Id                       Owner      Submitted   ST PRI Class        Running On
------------------------ ---------- ----------- -- --- ------------ -----------
mars.498.0               brownap    5/20 11:31  R  100 silver       mars
mars.499.0               brownap    5/20 11:31  R  50  No_Class     mars
mars.501.0               brownap    5/20 11:31  I  50  silver

3 job step(s) in query, 1 waiting, 0 pending, 2 running, 0 held, 0 preempted
$
  QRegExp statusCapture (QString(".*\\.%1\\.\\d+" // job name
                                 "\\s*\\w+" // user
                                 "[\\s0-9/:]+" // date/time
                                 "(\\w+).*") // status
                         .arg(jobId));
    */
  QString matchString = QString(
        "^\\w*.%1\\.\\d+\\s*\\w+[\\s0-9/:]+(\\w+)").arg(jobId);
  QRegExp statusCapture (matchString);
  foreach (const QString &str, statusList) {
    if (str.indexOf(statusCapture) == -1) {
      continue;
    }
    break;
  }

  return statusCapture.cap(1); // will be empty if no match
}

unsigned int LoadLevelerQueueInterface::parseJobId(
    const QString &submissionOutput, bool *ok) const
{
  // Assuming stdout_str value is
  //
  // llsubmit: The job "host.102" has been submitted.
  //
  // or similar
  QRegExp idCapture (".*\".*\\.([0-9]+)\"");
  *ok = false;
  if (idCapture.indexIn(submissionOutput) == -1) {
    // Ill-formed output
    m_opt->warning(tr("Cannot parse jobID from output: \"%1\" Match len %2")
                   .arg(submissionOutput)
                   .arg(idCapture.matchedLength()));
    return 0;
  }

  bool idIsInt;
  unsigned int jobId = idCapture.cap(1).toUInt(&idIsInt);

  if (!idIsInt) {
    m_opt->warning(tr("Invalid jobID. %1 output:\n%2\n"
                      "Parsed jobid: '%3'' (must be a positive integer).")
                   .arg(m_llsubmit).arg(submissionOutput)
                   .arg(idCapture.cap(1)));
    return 0;
  }

  *ok = true;
  return jobId;
}

QStringList LoadLevelerQueueInterface::getQueueList() const
{
  // recast queue mutex as mutable for safe access:
  QReadWriteLock &queueMutex = const_cast<QReadWriteLock&> (m_queueMutex);

  queueMutex.lockForRead();

  // Limit queries to once per second
  if (m_queueTimeStamp.isValid() &&
      // QDateTime::msecsTo is not implemented until Qt 4.7
    #if QT_VERSION >= 0x040700
      m_queueTimeStamp.msecsTo(QDateTime::currentDateTime())
      <= 1000*m_interval
    #else
      // Check if day is the same. If not, refresh. Otherwise check
      // msecsTo current time
      (m_queueTimeStamp.date() == QDate::currentDate() &&
       m_queueTimeStamp.time().msecsTo(QTime::currentTime())
       <= 1000*m_interval)
    #endif // QT_VERSION >= 4.7
      ) {
    // If the cache is valid, return it
    QStringList ret (m_queueData);
    queueMutex.unlock();
    return ret;
  }

  // Otherwise, store a copy of the current timestamp and switch
  // queuemutex to writelock
  QDateTime oldTimeStamp (m_queueTimeStamp);
  queueMutex.unlock();

  // Relock mutex
  QWriteLocker queueLocker (&queueMutex);

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
  QStringList &queueData     = const_cast<QStringList&> (m_queueData);
  QDateTime &queueTimeStamp  = const_cast<QDateTime&> (m_queueTimeStamp);

  // Get SSH connection
  SSHConnection *ssh = m_opt->ssh()->getFreeConnection();

  if (ssh == NULL) {
    m_opt->warning(tr("Cannot connect to ssh server"));
    queueTimeStamp = QDateTime::currentDateTime();
    queueData.clear();
    queueData << "CommError";
    QStringList ret (m_queueData);
    return ret;
  }

  QString command = m_llq + " -u " + m_opt->username;

  // Execute
  QString stdout_str;
  QString stderr_str;
  int ec;
  // Valid exit codes for grep: (0) matches found, execution successful
  //                            (1) no matches found, execution successful
  //                            (2) execution unsuccessful
  if (!ssh->execute(command, stdout_str, stderr_str, ec)
      || (ec != 0 && ec != 1 )
      ) {
    m_opt->ssh()->unlockConnection(ssh);
    m_opt->warning(tr("Error executing %1: (%2) %3\n\t"
                      "Using cached queue data.")
                   .arg(command)
                   .arg(QString::number(ec))
                   .arg(stderr_str));
    queueTimeStamp = QDateTime::currentDateTime();
    QStringList ret (m_queueData);
    return ret;
  }
  m_opt->ssh()->unlockConnection(ssh);

  queueData = stdout_str.split("\n", QString::SkipEmptyParts);

  QStringList ret (m_queueData);
  queueTimeStamp = QDateTime::currentDateTime();
  return ret;
}

}

/// @endcond

#endif // ENABLE_SSH

/**********************************************************************
  LsfQueueInterface - Base class for running jobs remotely on a LSF
  cluster.

  Copyright (C) 2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifdef ENABLE_SSH

// Doxygen skip:
/// @cond

#include <globalsearch/queueinterfaces/lsf.h>

#include <globalsearch/macros.h>
#include <globalsearch/optimizer.h>
#include <globalsearch/sshconnection.h>
#include <globalsearch/sshmanager.h>
#include <globalsearch/structure.h>

#include <QtCore/QDir>
#include <QtCore/QFile>

namespace GlobalSearch {

  LsfQueueInterface::LsfQueueInterface(OptBase *parent,
                                       const QString &settingsFile) :
    RemoteQueueInterface(parent, settingsFile),
    m_bjobs("bjobs"),
    m_bsub("bsub"),
    m_bkill("bkill"),
    m_cleanRemoteOnStop(false)
  {
    m_idString = "LSF";
    m_templates.append("job.lsf");
    m_hasDialog = true;

    readSettings(settingsFile);
  }

  LsfQueueInterface::~LsfQueueInterface()
  {
  }

  bool LsfQueueInterface::isReadyToSearch(QString *str)
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
      *str = tr("Hostname of LSF server is not set. Check your Queue "
                "configuration.");
      return false;
    }

    if (m_bkill.isEmpty()) {
      *str = tr("bkill command is not set. Check your Queue "
                "configuration.");
      return false;
    }

    if (m_bjobs.isEmpty()) {
      *str = tr("bjobs command is not set. Check your Queue "
                "configuration.");
      return false;
    }

    if (m_bsub.isEmpty()) {
      *str = tr("bsub command is not set. Check your Queue "
                "configuration.");
      return false;
    }

    if (m_opt->rempath.isEmpty()) {
      *str = tr("Remote working directory is not set. Check your Queue "
                "configuration.");
      return false;
    }

    if (m_opt->username.isEmpty()) {
      *str = tr("SSH username for LSF server is not set. Check your Queue "
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

  QDialog* LsfQueueInterface::dialog()
  {
    if (!m_dialog) {
      m_dialog = new LsfConfigDialog (m_opt->dialog(),
                                      m_opt,
                                      this);
    }
    LsfConfigDialog *d = qobject_cast<LsfConfigDialog*>(m_dialog);
    d->updateGUI();

    return d;
  }

  void LsfQueueInterface::readSettings(const QString &filename)
  {
    SETTINGS(filename);

    settings->beginGroup(m_opt->getIDString().toLower());
    settings->beginGroup("queueinterface/lsfqueueinterface");
    int loadedVersion = settings->value("version", 0).toInt();
    settings->beginGroup("paths");

    m_bsub  = settings->value("bsub",  "bsub").toString();
    m_bjobs = settings->value("bjobs", "bjobs").toString();
    m_bkill = settings->value("bkill", "bkill").toString();
    m_cleanRemoteOnStop = settings->value("cleanRemoteOnStop", false).toBool();

    settings->endGroup();
    settings->endGroup();
    settings->endGroup();

    DESTROY_SETTINGS(filename);

    // Update config data
    switch (loadedVersion) {
    case 0:
    default:
      break;
    }

  }

  void LsfQueueInterface::writeSettings(const QString &filename)
  {
    SETTINGS(filename);

    const int VERSION = 1;

    settings->beginGroup(m_opt->getIDString().toLower());
    settings->beginGroup("queueinterface/lsfqueueinterface");
    settings->setValue("version", VERSION);
    settings->beginGroup("paths");

    settings->setValue("bsub",  m_bsub);
    settings->setValue("bjobs", m_bjobs);
    settings->setValue("bkill", m_bkill);
    settings->setValue("cleanRemoteOnStop", m_cleanRemoteOnStop);

    settings->endGroup();
    settings->endGroup();
    settings->endGroup();

    DESTROY_SETTINGS(filename);
  }

  bool LsfQueueInterface::startJob(Structure *s)
  {
    SSHConnection *ssh = m_opt->ssh()->getFreeConnection();

    if (ssh == NULL) {
      m_opt->warning(tr("Cannot connect to ssh server"));
      return false;
    }

    QWriteLocker wlocker (s->lock());

    QString command = "cd \"" + s->getRempath() + "\" && " +
      m_bsub + "< job.lsf";

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

    // Assuming stdout_str value is
    // Job <[jobID]> is submitted to default queue <[queuename]>
    // (the <> are part of the output, [] are not
    QStringList list = stdout_str.split(QRegExp("<|>"));
    bool ok;
    unsigned int jobID;
    if (list.size() >= 2) {
      jobID = list.at(1).toUInt(&ok);
    }
    else {
      ok = false;
    }

    if (!ok) {
      m_opt->warning(tr("Error retrieving jobID for structure %1.")
                     .arg(s->getIDString()));
      return false;
    }

    s->setJobID(jobID);
    s->startOptTimer();
    return true;
  }

  bool LsfQueueInterface::stopJob(Structure *s)
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

    const QString command = m_bkill + " " + QString::number(s->getJobID());

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

  QueueInterface::QueueStatus LsfQueueInterface::getStatus(Structure *s) const
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

    // Determine status if structure is in the queue. Example:
    //
    // JOBID   USER    STAT  QUEUE      FROM_HOST   EXEC_HOST   JOB_NAME   SUBMIT_TIME
    // 1659    jdhondt RUN   SMP1       hydra3      hydra12     mcdis23    May  8 11:53
    // 1677    jdhondt RUN   SMP1       hydra3      hydra4      mcdis32    May 10 11:12
    // ...
    //
    // (http://www.vub.ac.be/BFUCC/LSF/bjobs-uall.html)
    QString status;
    QStringList entryList;
    unsigned int curJobID = 0;
    bool ok;
    for (int i = 0; i < queueData.size(); i++) {
      entryList = queueData.at(i).split(QRegExp("\\s+"),
                                        QString::SkipEmptyParts);
      if (entryList.size()) {
        curJobID = entryList.first().toUInt(&ok);
        if (!ok) {
          continue;
        }
      }
      else {
        continue;
      }
      if (curJobID == jobID) {
        if (entryList.size() < 3) {
          continue;
        }
        status = entryList.at(2);
        break;
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

    if (status.contains(QRegExp("RUN|DONE|EXIT"))) {
      return QueueInterface::Running;
    }
    else if (status.contains(QRegExp("PEND|PSUSP|USUSP|SSUSP"))) {
      return QueueInterface::Queued;
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
    // Unrecognized status: (ZOMBI and UNKWN not handled)
    else {
      m_opt->debug(tr("Structure %1 with jobID %2 has "
                      "unrecognized status: %3")
                   .arg(s->getIDString()).arg(s->getJobID())
                   .arg(status));
      return QueueInterface::Unknown;
    }
  }

  QStringList LsfQueueInterface::getQueueList() const
  {
    // recast queue mutex as mutable for safe access:
    QReadWriteLock &queueMutex = const_cast<QReadWriteLock&> (m_queueMutex);

    queueMutex.lockForRead();

    // Limit queries to once per second
    if (m_queueTimeStamp.isValid() &&
        // QDateTime::msecsTo is not implemented until Qt 4.7
#if QT_VERSION >= 0x040700
        m_queueTimeStamp.msecsTo(QDateTime::currentDateTime())
        <= 1000
#else
        // Check if day is the same. If not, refresh. Otherwise check
        // msecsTo current time
        (m_queueTimeStamp.date() == QDate::currentDate() &&
         m_queueTimeStamp.time().msecsTo(QTime::currentTime())
         <= 1000)
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

    QString command = m_bjobs + " -u " + m_opt->username;

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

/**********************************************************************
  SlurmQueueInterface - Base class for running jobs remotely on a SLURM
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

#include <globalsearch/queueinterfaces/slurm.h>

#include <globalsearch/macros.h>
#include <globalsearch/optimizer.h>
#include <globalsearch/sshconnection.h>
#include <globalsearch/sshmanager.h>
#include <globalsearch/structure.h>

#include <QtCore/QDir>
#include <QtCore/QFile>

namespace GlobalSearch {

  SlurmQueueInterface::SlurmQueueInterface(OptBase *parent,
                                           const QString &settingsFile) :
    RemoteQueueInterface(parent, settingsFile),
    m_squeue("squeue"),
    m_sbatch("sbatch"),
    m_scancel("scancel"),
    m_interval(1),
    m_cleanRemoteOnStop(false)
  {
    m_idString = "SLURM";
    m_templates.append("job.slurm");
    m_hasDialog = true;

    readSettings(settingsFile);
  }

  SlurmQueueInterface::~SlurmQueueInterface()
  {
  }

  bool SlurmQueueInterface::isReadyToSearch(QString *str)
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
      *str = tr("Hostname of SLURM server is not set. Check your Queue "
                "configuration.");
      return false;
    }

    if (m_scancel.isEmpty()) {
      *str = tr("scancel command is not set. Check your Queue "
                "configuration.");
      return false;
    }

    if (m_squeue.isEmpty()) {
      *str = tr("squeue command is not set. Check your Queue "
                "configuration.");
      return false;
    }

    if (m_sbatch.isEmpty()) {
      *str = tr("sbatch command is not set. Check your Queue "
                "configuration.");
      return false;
    }

    if (m_opt->rempath.isEmpty()) {
      *str = tr("Remote working directory is not set. Check your Queue "
                "configuration.");
      return false;
    }

    if (m_opt->username.isEmpty()) {
      *str = tr("SSH username for SLURM server is not set. Check your Queue "
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

  QDialog* SlurmQueueInterface::dialog()
  {
    if (!m_dialog) {
      m_dialog = new SlurmConfigDialog (m_opt->dialog(),
                                        m_opt,
                                        this);
    }
    SlurmConfigDialog *d = qobject_cast<SlurmConfigDialog*>(m_dialog);
    d->updateGUI();

    return d;
  }

  void SlurmQueueInterface::readSettings(const QString &filename)
  {
    SETTINGS(filename);

    settings->beginGroup(m_opt->getIDString().toLower());
    settings->beginGroup("queueinterface/slurmqueueinterface");
    int loadedVersion = settings->value("version", 0).toInt();
    settings->beginGroup("paths");

    m_sbatch  = settings->value("sbatch",  "sbatch").toString();
    m_squeue  = settings->value("squeue",  "squeue").toString();
    m_scancel = settings->value("scancel", "scancel").toString();
    this->setInterval(settings->value("interval", 1).toInt());
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

  void SlurmQueueInterface::writeSettings(const QString &filename)
  {
    SETTINGS(filename);

    const int VERSION = 0;

    settings->beginGroup(m_opt->getIDString().toLower());
    settings->beginGroup("queueinterface/slurmqueueinterface");
    settings->setValue("version", VERSION);
    settings->beginGroup("paths");

    settings->setValue("sbatch",  m_sbatch);
    settings->setValue("squeue",  m_squeue);
    settings->setValue("scancel", m_scancel);
    settings->setValue("interval",  m_interval);
    settings->setValue("cleanRemoteOnStop",  m_cleanRemoteOnStop);

    settings->endGroup();
    settings->endGroup();
    settings->endGroup();

    DESTROY_SETTINGS(filename);
  }

  bool SlurmQueueInterface::startJob(Structure *s)
  {
    SSHConnection *ssh = m_opt->ssh()->getFreeConnection();

    if (ssh == NULL) {
      m_opt->warning(tr("Cannot connect to ssh server"));
      return false;
    }

    QWriteLocker wlocker (s->lock());

    QString command = "cd \"" + s->getRempath() + "\" && " +
      m_sbatch + " job.slurm";

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

    // Assuming stdout_str value is "Submitted batch job <jobid>";
    QStringList list = stdout_str.split(QRegExp("\\s+"),
                                        QString::SkipEmptyParts);
    bool ok = false;
    unsigned int jobID;
    if (list.size() >= 4) {
      jobID = list.at(3).toUInt(&ok);
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

  bool SlurmQueueInterface::stopJob(Structure *s)
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

    const QString command = m_scancel + " " + QString::number(s->getJobID());

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

  QueueInterface::QueueStatus SlurmQueueInterface::getStatus(Structure *s) const
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

    // Determine status if structure is in the queue
    QString status;
    QStringList entryList;
    unsigned int curJobID = 0;
    bool ok;
    for (int i = 0; i < queueData.size(); ++i) {
      entryList = queueData[i].split(QRegExp("\\s+"),
                                     QString::SkipEmptyParts);
      // Should be 8 entries or so, but we really only need the first five
      if (entryList.size() > 5) {
        curJobID = entryList.first().toUInt(&ok);
        if (!ok) {
          continue;
        }
      }
      else {
        continue;
      }
      if (curJobID == jobID) {
        status = entryList.at(4);
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

    // some of these codes actually indicate failure\completion. Just
    // mark them as running until they actually disappear from the
    // queue and check the output files later. Status codes are taken
    // from the squeue man page:

    // JOB STATE CODES
    //
    // Jobs typically pass through several states in the course of
    // their execution.  The typical states are PENDING, RUNNING,
    // SUSPENDED, COMPLETING, and COMPLETED.  An explanation of each
    // state follows.
    //
    // CA  CANCELLED       Job  was explicitly cancelled by the user or system
    //                     administrator.  The job may or may  not  have  been
    //                     initiated.
    // CD  COMPLETED       Job has terminated all processes on all nodes.
    // CF  CONFIGURING     Job  has  been allocated resources, but are waiting
    //                     for them to become ready for use (e.g. booting).
    // CG  COMPLETING      Job is in the process of completing. Some processes
    //                     on some nodes may still be active.
    // F   FAILED          Job  terminated  with  non-zero  exit code or other
    //                     failure condition.
    // NF  NODE_FAIL       Job terminated due to failure of one or more  allo-
    //                     cated nodes.
    // PD  PENDING         Job is awaiting resource allocation.
    // R   RUNNING         Job currently has an allocation.
    // S   SUSPENDED       Job  has an allocation, but execution has been sus-
    //                     pended.
    // TO  TIMEOUT         Job terminated upon reaching its time limit.

    if (status.contains(QRegExp("CA|CD|CG|F|NF|R|S|TO"))) {
      return QueueInterface::Running;
    }
    else if (status.contains(QRegExp("CF|PD"))) {
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
      // I've seen this a few times on PBS when mpd dies unexpectedly
      // and the (incomplete) output files are never copied back. This
      // is an error, just restart.
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

  void SlurmQueueInterface::setInterval(const int sec)
  {
    m_queueMutex.lockForWrite();
    m_interval = sec;
    m_queueMutex.unlock();
  }

  QStringList SlurmQueueInterface::getQueueList() const
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

    QString command = m_squeue + " | grep " + m_opt->username;

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

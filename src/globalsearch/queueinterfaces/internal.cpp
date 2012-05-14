/**********************************************************************
  LocalQueueInterface - Interface for running jobs locally.

  Copyright (C) 2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <globalsearch/queueinterfaces/internal.h>

#include <globalsearch/macros.h>
#include <globalsearch/optimizer.h>
#include <globalsearch/queueinterfaces/internaldialog.h>
#include <globalsearch/queuemanager.h>
#include <globalsearch/structure.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QProcess>
#include <QtCore/QString>
#include <QtCore/QTextStream>

#ifdef WIN32
// Don't pull in winsock api (needed for libssh)
#define _WINSOCKAPI_
// For extracting PIDs
#include <windows.h>
#endif

#include <globalsearch/queueinterfaces/internal.h>

namespace GlobalSearch {

InternalQueueInterface::InternalQueueInterface(OptBase *parent,
                                               const QString &settingsFile)
  : LocalQueueInterface(parent)
{
  // Needed for InternalQueueProcess
  qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
  qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");

  m_idString = "Internal";
  m_hasDialog = true;
}

InternalQueueInterface::~InternalQueueInterface()
{
  for (QHash<unsigned long, InternalQueueProcess*>::iterator
         it = m_processes.begin(),
         it_end = m_processes.end();
       it != it_end; ++it) {
    if ((*it) && ((*it)->state() == QProcess::Running)) {
      // Give each process 5 seconds to do any cleanup needed, then
      // kill it.
      (*it)->terminate();
      (*it)->waitForFinished(5000);
      (*it)->kill();
    }
  }
}

bool InternalQueueInterface::isReadyToSearch(QString *str)
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
              "change the permissions on this directory or use "
              "a different one.").arg(m_opt->filePath);
    return false;
  }

  *str = "";
  return true;
}

bool InternalQueueInterface::startJob(Structure *s)
{
  if (s->getJobID() != 0) {
    m_opt->warning(tr("InternalQueueInterface::startJob: Attempting to start "
                      "job for structure %1, but a JobID is already set (%2)."
                      ).arg(s->getIDString()).arg(s->getJobID()));
    return false;
  }

  // Just merge all of the command line arguments into the command. For some
  // reason the QProcess::start(command, args) overload isn't working...
  QString command = m_opt->optimizer()->localRunCommand()
      + " " + m_opt->optimizer()->localRunArgs().join(" ");

#ifdef WIN32
  command = "cmd.exe /C \"" + command + "\"";
#endif // WIN32

  InternalQueueProcess *proc = new InternalQueueProcess(this);
  proc->setWorkingDirectory(s->fileName());
  if (!m_opt->optimizer()->stdinFilename().isEmpty()) {
    proc->setStandardInputFile(s->fileName()
                               + "/" + m_opt->optimizer()->stdinFilename());
  }
  if (!m_opt->optimizer()->stdoutFilename().isEmpty()) {
    proc->setStandardOutputFile(s->fileName()
                                + "/" + m_opt->optimizer()->stdoutFilename());
  }
  if (!m_opt->optimizer()->stderrFilename().isEmpty()) {
    proc->setStandardErrorFile(s->fileName()
                               + "/" + m_opt->optimizer()->stderrFilename());
  }

  proc->start(command);

#ifdef WIN32
  unsigned long pid = proc->pid()->dwProcessId;
#else // WIN32
  unsigned long pid = proc->pid();
#endif // WIN32

  s->startOptTimer();
  s->setJobID(pid);

  m_processes.insert(pid, proc);

  return true;
}

bool InternalQueueInterface::stopJob(Structure *s)
{
  QWriteLocker wLocker (s->lock());

  unsigned long pid = static_cast<unsigned long>(s->getJobID());

  if (pid == 0) {
    // The job is not running, so just return
    return true;
  }

  // Look-up process instance
  InternalQueueProcess *proc = m_processes.value(pid, 0);
  if (!proc) {
    // No process is associated with this JobID.
    return true;
  }

  // Kill job
  proc->kill();
  s->setJobID(0);
  s->stopOptTimer();
  m_processes.remove(pid);
  return true;
}

QueueInterface::QueueStatus
InternalQueueInterface::getStatus(Structure *s) const
{
  // lock Structure
  QReadLocker wlocker (s->lock());

  // Look-up process instance
  unsigned long pid = static_cast<unsigned long>(s->getJobID());

  // If jobID = 0 and structure is not in "Submitted" state, return an error.
  if (!pid && s->getStatus() != Structure::Submitted) {
    return QueueInterface::Error;
  }

  // Look-up process instance
  InternalQueueProcess *proc = m_processes.value(pid, 0);

  // If structure is submitted, check if it's process is in the
  // lookup table. If not, check if the completion file has been
  // written.
  //
  // If the completion file exists, then the job finished before the
  // queue checks could see it, and the function will continue on to
  // the status checks below.
  //
  // Is the structure in Submitted state?
  if (s->getStatus() == Structure::Submitted) {
    // If the process isn't in the table
    if (pid == 0 || proc == 0) {
      // Is the output file exist absent?
      bool exists;
      m_opt->optimizer()->checkIfOutputFileExists(s, &exists);
      if (!exists) {
        // The output file does not exist -- the job is still
        // pending.
        return QueueInterface::Pending;
      }
    }
    // The job is either running or finished
    return QueueInterface::Started;
  }

  // If the process is not in the table, return an error
  if (!proc) {
    return QueueInterface::Error;
  }

  // Note that this is not part of QProcess - status() is defined in
  // LocalQueueProcess
  switch (proc->status()) {
  case InternalQueueProcess::NotStarted:
    return QueueInterface::Pending;
  case InternalQueueProcess::Running:
    return QueueInterface::Running;
  case InternalQueueProcess::Error:
    return QueueInterface::Error;
  case InternalQueueProcess::Finished:
    // Was the run successful?
    if (proc->exitCode() != 0) {
      m_opt->warning(tr("%1: Structure %2, PID=%3 failed. QProcess error "
                        "code: %4. Process exit code: %5 errStr: %6\n"
                        "stdout:\n%7\nstderr:\n%8")
                     .arg(Q_FUNC_INFO).arg(s->getIDString()).arg(pid)
                     .arg(proc->error()).arg(proc->exitCode())
                     .arg(proc->errorString())
                     .arg(QString(proc->readAllStandardOutput()))
                     .arg(QString(proc->readAllStandardError())));
      return QueueInterface::Error;
    }
    bool success;
    m_opt->optimizer()->checkForSuccessfulOutput(s, &success);
    if (success) {
      return QueueInterface::Success;
    }
    else {
      return QueueInterface::Error;
    }
  default:
  // Shouldn't reach this point...
  return QueueInterface::Unknown;
  }
}

bool InternalQueueInterface::prepareForStructureUpdate(Structure *s) const
{
  // Nothing to do!
  return true;
}

QDialog* InternalQueueInterface::dialog()
{
  if (!m_dialog) {
    m_dialog = new InternalQueueInterfaceConfigDialog (m_opt->dialog(),
                                                       m_opt,
                                                       this);
  }
  InternalQueueInterfaceConfigDialog *d =
    qobject_cast<InternalQueueInterfaceConfigDialog*>(m_dialog);
  d->updateGUI();

  return d;
}


} // end namespace GlobalSearch

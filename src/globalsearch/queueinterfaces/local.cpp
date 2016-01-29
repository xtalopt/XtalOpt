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

#include <globalsearch/queueinterfaces/local.h>

#include <globalsearch/macros.h>
#include <globalsearch/optimizer.h>
#include <globalsearch/queueinterfaces/localdialog.h>
#include <globalsearch/queuemanager.h>
#include <globalsearch/structure.h>
#include <globalsearch/exceptionhandler.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QProcess>
#include <QtCore/QString>
#include <QtCore/QTextStream>

#ifdef WIN32
// For extracting PIDs
#include <windows.h>
#endif

namespace GlobalSearch {

  LocalQueueInterface::LocalQueueInterface(OptBase *parent,
                                           const QString &settingFile) :
    QueueInterface(parent)
  {
    // Needed for LocalQueueProcess:
    qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
    qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");

    m_idString = "Local";
    m_hasDialog = true;
  }

  LocalQueueInterface::~LocalQueueInterface()
  {
    // Destructors should never throw...
    try {
      for (QHash<unsigned long, LocalQueueProcess*>::iterator
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
    } // end of try{}
    catch(...) {
      ExceptionHandler::handleAllExceptions(__FUNCTION__);
    } // end of catch{}
  }

  bool LocalQueueInterface::isReadyToSearch(QString *str)
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

  bool LocalQueueInterface::writeFiles
  (Structure *s, const QHash<QString, QString> &fileHash) const
  {
    // Create file objects
    QList<QFile*> files;
    QStringList filenames = fileHash.keys();
    for (int i = 0; i < filenames.size(); i++) {
      files.append(new QFile (s->fileName() + "/" + filenames.at(i)));
    }

    // Check that the files can be written to
    for (int i = 0; i < files.size(); i++) {
      if (!files.at(i)->open( QIODevice::WriteOnly | QIODevice::Text ) ) {
        m_opt->error(tr("Cannot write input file %1 (file writing failure)", "1 is a file path").arg(files.at(i)->fileName()));
        qDeleteAll(files);
        return false;
      }
    }

    // Set up text streams
    QList<QTextStream*> streams;
    for (int i = 0; i < files.size(); i++) {
      streams.append(new QTextStream (files.at(i)));
    }

    // Write files
    for (int i = 0; i < streams.size(); i++) {
      *(streams.at(i)) << fileHash[filenames.at(i)];
    }

    // Close files
    for (int i = 0; i < files.size(); i++) {
      files.at(i)->close();
    }

    // Clean up
    qDeleteAll(streams);
    qDeleteAll(files);
    return true;
  }

  bool LocalQueueInterface::startJob(Structure *s)
  {
    if (s->getJobID() != 0) {
      m_opt->warning(tr("LocalQueueInterface::startJob: Attempting to start "
                        "job for structure %1, but a JobID is already set (%2)."
                        ).arg(s->getIDString()).arg(s->getJobID()));
      return false;
    }
    // TODO the corresponding function in Optimizer should prepend a
    // path for e.g. windows
    QString command = m_opt->optimizer()->localRunCommand();

#ifdef WIN32
    command = "cmd.exe /C \"" + command + "\"";
#endif // WIN32

    LocalQueueProcess *proc = new LocalQueueProcess(this);
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

  bool LocalQueueInterface::logErrorDirectory(Structure *s) const
  {
    QProcess proc, proc2;
#ifdef WIN32
    QString command = "mkdir " + this->m_opt->filePath + "\\errorDirs\\";
    proc.start(command);
    // This will wait for, at most, 30 seconds
    proc.waitForFinished();
    // Does robocopy come with all windows computers?
    QString command2 = "robocopy " + s->fileName() + " " +
                       this->m_opt->filePath + "\\errorDirs\\" +
                       QString::number(s->getGeneration) + "x" +
                       QString::number(s->getIDNumber());
    proc2.start(command2);
    proc2.waitForFinished();
#else
    QString command = "mkdir -p " + this->m_opt->filePath + "/errorDirs/";
    proc.start(command);
    // This will wait for, at most, 30 seconds
    proc.waitForFinished();
    QString command2 = "cp -r " + s->fileName() + " " +
                       this->m_opt->filePath + "/errorDirs/";
    proc2.start(command2);
    proc2.waitForFinished();
#endif
    return true;
  }

  bool LocalQueueInterface::stopJob(Structure *s)
  {
    QWriteLocker wLocker (s->lock());

    unsigned long pid = static_cast<unsigned long>(s->getJobID());

    if (this->m_opt->m_logErrorDirs && (s->getStatus() == Structure::Error ||
                                        s->getStatus() == Structure::Restart)) {
      logErrorDirectory(s);
    }

    if (pid == 0) {
      // The job is not running, so just return
      return true;
    }

    // Look-up process instance
    LocalQueueProcess *proc = m_processes.value(pid, 0);
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
  LocalQueueInterface::getStatus(Structure *s) const
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
    LocalQueueProcess *proc = m_processes.value(pid, 0);

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
    case LocalQueueProcess::NotStarted:
      return QueueInterface::Pending;
    case LocalQueueProcess::Running:
      return QueueInterface::Running;
    case LocalQueueProcess::Finished:
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

  bool LocalQueueInterface::prepareForStructureUpdate(Structure *s) const
  {
    // Nothing to do!
    return true;
  }

  bool LocalQueueInterface::checkIfFileExists(Structure *s,
                                              const QString &filename,
                                              bool *exists)
  {
    *exists = QFile::exists(QString("%1/%2")
                            .arg(s->fileName())
                            .arg(filename));
    return true;
  }

  bool LocalQueueInterface::fetchFile(Structure *s,
                                      const QString &rel_filename,
                                      QString *contents) const
  {
    QString filename = s->fileName() + "/" + rel_filename;
    QFile output (filename);
    if (!output.open(QFile::ReadOnly | QFile::Text)) {
      return false;
    }
    *contents = QString(output.readAll());
    output.close();
    return true;
  }

  bool LocalQueueInterface::grepFile(Structure *s,
                                     const QString &matchText,
                                     const QString &filename,
                                     QStringList *matches,
                                     int *exitcode,
                                     const bool caseSensitive) const
  {
    if (exitcode) {
      *exitcode = 1;
    }
    QStringList list;
    QString contents;
    if (!fetchFile(s, filename, &contents)) {
      if (exitcode) {
        *exitcode = 2;
      }
      return false;
    }

    list = contents.split("\n", QString::SkipEmptyParts);

    Qt::CaseSensitivity cs;
    if (caseSensitive) {
      cs = Qt::CaseSensitive;
    }
    else {
      cs = Qt::CaseInsensitive;
    }

    for (QStringList::const_iterator
           it = list.begin(),
           it_end = list.end();
         it != it_end;
         ++it) {
      if ((*it).contains(matchText, cs)) {
        if (exitcode) {
          *exitcode = 0;
        }
        if (matches) {
          *matches << *it;
        }
      }
    }

    return true;
  }

  QDialog* LocalQueueInterface::dialog()
  {
    if (!m_dialog) {
      m_dialog = new LocalQueueInterfaceConfigDialog (m_opt->dialog(),
                                                      m_opt,
                                                      this);
    }
    LocalQueueInterfaceConfigDialog *d =
      qobject_cast<LocalQueueInterfaceConfigDialog*>(m_dialog);
    d->updateGUI();

    return d;
  }

}

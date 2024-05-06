/**********************************************************************
  LocalQueueInterface - Interface for running jobs locally.

  Copyright (C) 2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <globalsearch/queueinterfaces/local.h>

#include <globalsearch/optimizer.h>
#include <globalsearch/queueinterfaces/localdialog.h>
#include <globalsearch/queuemanager.h>
#include <globalsearch/random.h>
#include <globalsearch/structure.h>

#include <QDir>
#include <QFile>
#include <QHash>
#include <QProcess>
#include <QString>
#include <QTextStream>

#ifdef WIN32
// For extracting PIDs
#include <windows.h>
#endif

namespace GlobalSearch {

LocalQueueInterface::LocalQueueInterface(OptBase* parent,
                                         const QString& settingFile)
  : QueueInterface(parent)
{
  // Needed for LocalQueueProcess:
  qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
  qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");

  m_idString = "Local";
  m_hasDialog = true;
}

LocalQueueInterface::~LocalQueueInterface()
{
  for (QHash<unsigned long, LocalQueueProcess *>::iterator
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

bool LocalQueueInterface::isReadyToSearch(QString* str)
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
              "change the permissions on this directory or use "
              "a different one.")
             .arg(m_opt->locWorkDir);
    return false;
  }

  *str = "";
  return true;
}

bool LocalQueueInterface::writeFiles(
  Structure* s, const QHash<QString, QString>& fileHash) const
{
  // Create file objects
  QList<QFile*> files;
  QStringList filenames = fileHash.keys();
  for (int i = 0; i < filenames.size(); i++) {
    files.append(new QFile(s->getLocpath() + "/" + filenames.at(i)));
  }

  // Check that the files can be written to
  for (int i = 0; i < files.size(); i++) {
    if (!files.at(i)->open(QIODevice::WriteOnly | QIODevice::Text)) {
      m_opt->error(tr("Cannot write input file %1 (file writing failure)",
                      "1 is a file path")
                     .arg(files.at(i)->fileName()));
      qDeleteAll(files);
      return false;
    }
  }

  // Set up text streams
  QList<QTextStream*> streams;
  for (int i = 0; i < files.size(); i++) {
    streams.append(new QTextStream(files.at(i)));
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

  // If there are copy files, copy those to the dir as well.
  if (!s->copyFiles().empty()) {
    for (const auto& copyFile : s->copyFiles()) {
      QFile infile(copyFile.c_str());
      QString filename = QFileInfo(infile).fileName();
      QFile outfile(s->getLocpath() + "/" + filename);
      if (!infile.copy(outfile.fileName())) {
        m_opt->error(tr("Failed to copy file %1 to %2")
                       .arg(infile.fileName())
                       .arg(outfile.fileName()));
        return false;
      }
    }
    s->clearCopyFiles();
  }

  return true;
}

bool LocalQueueInterface::startJob(Structure* s)
{
  if (s->getJobID() != 0) {
    m_opt->warning(tr("LocalQueueInterface::startJob: Attempting to start "
                      "job for structure %1, but a JobID is already set (%2).")
                     .arg(s->getTag())
                     .arg(s->getJobID()));
    return false;
  }
  // TODO the corresponding function in Optimizer should prepend a
  // path for e.g. windows
  QString command = getCurrentOptimizer(s)->localRunCommand();

#ifdef WIN32
  command = "cmd.exe /C \"" + command + "\"";
#endif // WIN32

  LocalQueueProcess* proc = new LocalQueueProcess(nullptr);
  proc->setWorkingDirectory(s->getLocpath());
  if (!getCurrentOptimizer(s)->stdinFilename().isEmpty()) {
    proc->setStandardInputFile(s->getLocpath() + "/" +
                               getCurrentOptimizer(s)->stdinFilename());
  }
  if (!getCurrentOptimizer(s)->stdoutFilename().isEmpty()) {
    proc->setStandardOutputFile(s->getLocpath() + "/" +
                                getCurrentOptimizer(s)->stdoutFilename());
  }
  if (!getCurrentOptimizer(s)->stderrFilename().isEmpty()) {
    proc->setStandardErrorFile(s->getLocpath() + "/" +
                               getCurrentOptimizer(s)->stderrFilename());
  }

  proc->start(command);

#ifdef WIN32
  unsigned long pid = proc->pid()->dwProcessId;
#else  // WIN32
  unsigned long pid = proc->pid();
#endif // WIN32

  s->startOptTimer();
  s->setJobID(pid);

  m_processes.insert(pid, proc);

  return true;
}

bool LocalQueueInterface::logErrorDirectory(Structure* s) const
{
  QString id_s, gen_s, strdir_s;
  id_s.sprintf("%05d", s->getIDNumber());
  gen_s.sprintf("%05d", s->getGeneration());
  strdir_s = gen_s + "x" + id_s;

  QString command, command2;
  QProcess proc, proc2;
#ifdef WIN32
  command = "mkdir " + this->m_opt->locWorkDir + "\\errorDirs\\";
  // Does robocopy come with all windows computers?
  command2 = "robocopy " + s->getLocpath() + " " +
                     this->m_opt->locWorkDir + "\\errorDirs\\" + strdir_s;
#else
  command = "mkdir -p " + this->m_opt->locWorkDir + "/errorDirs/" + strdir_s + "/";
  command2 =
    "cp -r " + s->getLocpath() + " " + this->m_opt->locWorkDir + "/errorDirs/" + strdir_s + "/";
#endif

  proc.start(command);
  proc.waitForFinished();
  proc2.start(command2);
  proc2.waitForFinished();

  return true;
}

bool LocalQueueInterface::stopJob(Structure* s)
{
  QWriteLocker wLocker(&s->lock());

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
  LocalQueueProcess* proc = m_processes.value(pid, 0);
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

QueueInterface::QueueStatus LocalQueueInterface::getStatus(Structure* s) const
{
  // lock Structure
  QReadLocker wlocker(&s->lock());

  // Look-up process instance
  unsigned long pid = static_cast<unsigned long>(s->getJobID());

  // If jobID = 0 and structure is not in "Submitted" state, return an error.
  if (!pid && s->getStatus() != Structure::Submitted) {
    return QueueInterface::Error;
  }

  // Look-up process instance
  LocalQueueProcess* proc = m_processes.value(pid, 0);

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
      getCurrentOptimizer(s)->checkIfOutputFileExists(s, &exists);
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
                         .arg(Q_FUNC_INFO)
                         .arg(s->getTag())
                         .arg(pid)
                         .arg(proc->error())
                         .arg(proc->exitCode())
                         .arg(proc->errorString())
                         .arg(QString(proc->readAllStandardOutput()))
                         .arg(QString(proc->readAllStandardError())));
        return QueueInterface::Error;
      }
      bool success;
      getCurrentOptimizer(s)->checkForSuccessfulOutput(s, &success);
      if (success) {
        return QueueInterface::Success;
      } else {
        return QueueInterface::Error;
      }
    default:
      // Shouldn't reach this point...
      return QueueInterface::Unknown;
  }
}

bool LocalQueueInterface::prepareForStructureUpdate(Structure* s) const
{
  // Nothing to do!
  return true;
}

bool LocalQueueInterface::runACommand(const QString& workdir, const QString& command,
                                            QString* sout, QString* serr, int* ercd) const
{
  // General note: catching the errors for local runs which rely on QProcess
  //   is tricky! E.g., the error channel might be polluted by messages from srun
  //   command which end up successful. So, we won't force a return value of False
  //   based on stderr or exit code, and only print a message if stderr is not empty
  //   or non-zero error code to help identifying possible irregularities.

  QString stdout_str;
  QString stderr_str;
  int     exitcode;

  QProcess proc;
  if (!workdir.isEmpty()) {
    proc.setWorkingDirectory(workdir);
  }
  proc.start(command);
  proc.waitForFinished(-1);

  stdout_str = QString(proc.readAllStandardOutput());
  stderr_str = QString(proc.readAllStandardError());
  exitcode   = proc.exitCode();

  if (!stderr_str.isEmpty() || (exitcode != 0)) {
    m_opt->warning(tr("Local command %1 at %2 exited with code %3 and error: %4")
        .arg(command).arg(workdir).arg(exitcode).arg(stderr_str));
  }

  *sout = stdout_str;
  *serr = stderr_str;
  *ercd = exitcode;

  return true;
}

bool LocalQueueInterface::copyAFileRemoteToLocal(const QString& rem_file,
                                                 const QString& loc_file)
{
  // Nothing to do!
  return true;
}

bool LocalQueueInterface::copyAFileLocalToRemote(const QString& loc_file,
                                                 const QString& rem_file)
{
  // Nothing to do!
  return true;
}

bool LocalQueueInterface::checkIfFileExists(Structure* s,
                                            const QString& filename,
                                            bool* exists)
{
  *exists = QFile::exists(s->getLocpath() + QDir::separator() + filename);
  return true;
}

bool LocalQueueInterface::fetchFile(Structure* s, const QString& rel_filename,
                                    QString* contents) const
{
  QString filename = s->getLocpath() + "/" + rel_filename;
  QFile output(filename);
  if (!output.open(QFile::ReadOnly | QFile::Text)) {
    return false;
  }
  *contents = QString(output.readAll());
  output.close();
  return true;
}

bool LocalQueueInterface::grepFile(Structure* s, const QString& matchText,
                                   const QString& filename,
                                   QStringList* matches, int* exitcode,
                                   const bool caseSensitive) const
{
  if (exitcode) {
    *exitcode = 1;
  }
  Qt::CaseSensitivity ces = Qt::CaseSensitive;
  if (!caseSensitive) {
    ces = Qt::CaseInsensitive;
  }

  // Read the file
  QFile infile(s->getLocpath() + "/" + filename);
  if (!infile.open(QFile::ReadOnly | QFile::Text)) {
    return false;
  }

  QTextStream in (&infile);
  QString line;
  do {
    line = in.readLine();
    if (line.contains(matchText, ces)) {
      if (exitcode) {
        *exitcode = 0;
      }
      if (matches) {
        *matches << line;
      }
    }
  } while (!line.isNull());

  return true;
}

QDialog* LocalQueueInterface::dialog()
{
  if (!m_dialog) {
    if (!m_opt->dialog())
      return nullptr;
    m_dialog =
      new LocalQueueInterfaceConfigDialog(m_opt->dialog(), m_opt, this);
  }
  LocalQueueInterfaceConfigDialog* d =
    qobject_cast<LocalQueueInterfaceConfigDialog*>(m_dialog);
  d->updateGUI();

  return d;
}
}

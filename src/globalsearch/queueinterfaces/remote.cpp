/**********************************************************************
  RemoteQueueInterface - Base class for running jobs remotely.

  Copyright (C) 2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifdef ENABLE_SSH

#include <globalsearch/queueinterfaces/remote.h>

#include <globalsearch/sshconnection.h>
#include <globalsearch/sshmanager.h>
#include <globalsearch/structure.h>

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QProcess>
#include <QString>

namespace GlobalSearch {

RemoteQueueInterface::RemoteQueueInterface(OptBase* parent,
                                           const QString& settingFile)
  : QueueInterface(parent)
{
  m_idString = "AbstractRemote";
}

RemoteQueueInterface::~RemoteQueueInterface()
{
}

bool RemoteQueueInterface::writeFiles(
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

      // Also append them to filenames so that they will be copied to
      // the remote dir
      filenames.append(filename);
    }
    s->clearCopyFiles();
  }

  // Copy to remote

  // ===================================================
  // If a local-remote run, perform this part and return
  if (m_opt->m_localQueue) {
    if (!createRemoteDirectory(s) || !cleanRemoteDirectory(s))
      return false;

    QString stdout_str, stderr_str;
    int exitcode;

    for (QStringList::const_iterator it = filenames.constBegin(),
        it_end = filenames.constEnd(); it != it_end; ++it) {
      QString command = "scp " + s->getLocpath() + "/" +
                        (*it) + " " + s->getRempath() + "/" + (*it);
      if (!runACommand("", command, &stdout_str, &stderr_str, &exitcode))
        return false;
    }
    return true;
  }
  // ===================================================

  SSHConnection* ssh = m_opt->ssh()->getFreeConnection();
  if (ssh == nullptr) {
    m_opt->warning(tr("Cannot connect to ssh server."));
    return false;
  }

  if (!createRemoteDirectory(s, ssh) || !cleanRemoteDirectory(s, ssh))
    return false;

  for (QStringList::const_iterator it = filenames.constBegin(),
      it_end = filenames.constEnd(); it != it_end; ++it) {
    if (!ssh->copyFileToServer(s->getLocpath() + "/" + (*it),
          s->getRempath() + "/" + (*it))) {
      m_opt->warning(tr("Error copying \"%1\" to remote server (structure %2)")
          .arg(*it)
          .arg(s->getTag()));
      m_opt->ssh()->unlockConnection(ssh);
      return false;
    }
  }
  m_opt->ssh()->unlockConnection(ssh);
  return true;
}

bool RemoteQueueInterface::prepareForStructureUpdate(Structure* s) const
{
  return copyRemoteFilesToLocalCache(s);
}

bool RemoteQueueInterface::runACommand(const QString& workdir, const QString& command,
                                       QString* sout, QString* serr, int* ercd) const
{
  // General note: catching the errors for local-remote runs which rely on QProcess
  //   is tricky! E.g., the error channel might be polluted by messages from srun
  //   command which end up successful. So, we won't force a return value of False
  //   based on stderr or exit code, and only print a message if stderr is not empty
  //   or non-zero error code to help identifying possible irregularities.

  QString stdout_str;
  QString stderr_str;
  int     exitcode;

  // ===================================================
  // If a local-remote run, perform this part and return
  if (m_opt->m_localQueue) {
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
      m_opt->warning(tr("LocalQueue command %1 at %2 exited with code %3 and error: %4")
          .arg(command).arg(workdir).arg(exitcode).arg(stderr_str));
    }

    *sout = stdout_str;
    *serr = stderr_str;
    *ercd = exitcode;

    return true;
  }
  // ===================================================

  SSHConnection* ssh = m_opt->ssh()->getFreeConnection();
  if (ssh == nullptr) {
    m_opt->warning("Cannot connect to ssh server");
    return false;
  }

  QString runcom;
  if (!workdir.isEmpty()) {
    runcom = "cd \"" + workdir + "\" && " + command;
  } else {
    runcom = command;
  }

  bool ok;
  ok = ssh->execute(runcom, stdout_str, stderr_str, exitcode);

  *sout = stdout_str;
  *serr = stderr_str;
  *ercd = exitcode;

  if (!ok || exitcode !=0) {
    m_opt->warning(tr("Remote command %1 at %2 failed!")
        .arg(command).arg(workdir));
    m_opt->ssh()->unlockConnection(ssh);
    return false;
  }
  m_opt->ssh()->unlockConnection(ssh);

  return true;
}

bool RemoteQueueInterface::copyAFileRemoteToLocal(const QString& rem_file,
                                                  const QString& loc_file)
{
  // ===================================================
  // If a local-remote run, perform this part and return
  if (m_opt->m_localQueue) {
    // QFile copy does not overwrite existing file,
    //   so we first delete it first if it exists.
    if (QFile::exists(loc_file)) {
      QFile::remove(loc_file);
    }
    return QFile::copy(rem_file, loc_file);
  }
  // ===================================================

  SSHConnection* ssh = m_opt->ssh()->getFreeConnection();
  if (ssh == nullptr) {
    m_opt->warning("Cannot connect to ssh server");
    return false;
  }

  if (!ssh->copyFileFromServer(rem_file, loc_file)) {
    m_opt->warning(tr("Failed copying '%1' from remote server to local '%2'")
        .arg(rem_file).arg(loc_file));
    m_opt->ssh()->unlockConnection(ssh);
    return false;
  }
  m_opt->ssh()->unlockConnection(ssh);

  return true;
}

bool RemoteQueueInterface::copyAFileLocalToRemote(const QString& loc_file,
                                                  const QString& rem_file)
{
  // ===================================================
  // If a local-remote run, perform this part and return
  if (m_opt->m_localQueue) {
    // QFile copy does not overwrite existing file,
    //   so we first delete it, if it exists.
    if (QFile::exists(rem_file)) {
      QFile::remove(rem_file);
    }
    return QFile::copy(loc_file, rem_file);
  }
  // ===================================================

  SSHConnection* ssh = m_opt->ssh()->getFreeConnection();
  if (ssh == nullptr) {
    m_opt->warning("Cannot connect to ssh server");
    return false;
  }

  if (!ssh->copyFileToServer(loc_file, rem_file)) {
    m_opt->warning(tr("Failed copying '%1' to remote server '%2'")
        .arg(loc_file).arg(rem_file));
    m_opt->ssh()->unlockConnection(ssh);
    return false;
  }
  m_opt->ssh()->unlockConnection(ssh);
  return true;
}

bool RemoteQueueInterface::checkIfFileExists(Structure* s,
                                             const QString& filename,
                                             bool* exists)
{
  const QString searchPath = s->getRempath();
  QString needle;
  QStringList haystack;

  // ===================================================
  // If a local-remote run, perform this part and return
  if (m_opt->m_localQueue) {
    *exists = QFile::exists(s->getRempath() + QDir::separator() + filename);
    return true;
  }
  // ===================================================

  SSHConnection* ssh = m_opt->ssh()->getFreeConnection();

  if (ssh == nullptr) {
    m_opt->warning(tr("Cannot connect to ssh server"));
    return false;
  }

  needle = s->getRempath() + "/" + filename;
  if (!ssh->readRemoteDirectoryContents(searchPath, haystack)) {
    m_opt->warning(tr("Error reading directory %1 on %2@%3:%4")
                     .arg(searchPath)
                     .arg(ssh->getUser())
                     .arg(ssh->getHost())
                     .arg(ssh->getPort()));
    m_opt->ssh()->unlockConnection(ssh);
    return false;
  }
  m_opt->ssh()->unlockConnection(ssh);

  *exists = false;
  for (QStringList::const_iterator it = haystack.constBegin(),
                                   it_end = haystack.constEnd();
       it != it_end; ++it) {
    if (it->compare(needle) == 0) {
      // Ouch!
      *exists = true;
      break;
    }
  }

  return true;
}

bool RemoteQueueInterface::fetchFile(Structure* s, const QString& rel_filename,
                                     QString* contents) const
{
  // ===================================================
  // If a local-remote run, perform this part and return
  if (m_opt->m_localQueue) {
    QString filename = s->getRempath() + "/" + rel_filename;
    QFile output(filename);

    if (!output.open(QFile::ReadOnly | QFile::Text)) {
      return false;
    }

    *contents = QString(output.readAll());

    output.close();

    return true;
  }
  // ===================================================

  SSHConnection* ssh = m_opt->ssh()->getFreeConnection();

  if (ssh == nullptr) {
    m_opt->warning(tr("Cannot connect to ssh server"));
    return false;
  }

  if (!ssh->readRemoteFile(s->getRempath() + "/" + rel_filename, *contents)) {
    m_opt->ssh()->unlockConnection(ssh);
    return false;
  }
  m_opt->ssh()->unlockConnection(ssh);

  return true;
}

bool RemoteQueueInterface::grepFile(Structure* s, const QString& matchText,
                                    const QString& filename,
                                    QStringList* matches, int* exitcode,
                                    const bool caseSensitive) const
{
  // Since network latency / transfer rates are much slower than
  // reading the file, call grep on the remote server and only
  // transfer back the matches.
  Qt::CaseSensitivity ces = Qt::CaseSensitive;
  QString flags = "";

  if (!caseSensitive) {
    flags = "-i";
    ces = Qt::CaseInsensitive;
  }

  QString stdout_str;
  QString stderr_str;
  int ec;

  // ===================================================
  // If a local-remote run, perform this part and return
  if (m_opt->m_localQueue) {
    if (exitcode) {
      *exitcode = 1;
    }

    // Read the file
    QFile infile(s->getRempath() + "/" + filename);
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
  // ===================================================

  SSHConnection* ssh = m_opt->ssh()->getFreeConnection();
  if (ssh == nullptr) {
    m_opt->warning(tr("Cannot connect to ssh server"));
    return false;
  }

  if (!ssh->execute(QString("grep %1 '%2' %3/%4")
                      .arg(flags)
                      .arg(matchText)
                      .arg(s->getRempath())
                      .arg(filename),
                    stdout_str, stderr_str, ec)) {
    m_opt->ssh()->unlockConnection(ssh);
    return false;
  }
  m_opt->ssh()->unlockConnection(ssh);

  if (exitcode) {
    *exitcode = ec;
  }

  if (matches) {
    *matches = stdout_str.split('\n', QString::SkipEmptyParts);
  }

  return true;
}

// This function, has a default 'nullptr' for ssh connection, so can
//   be called with/without it!
bool RemoteQueueInterface::createRemoteDirectory(Structure* structure,
                                                 SSHConnection* ssh) const
{
  // ===================================================
  // If a local-remote run, perform this part and return
  if (m_opt->m_localQueue) {
    QDir dir;
    return dir.mkpath(structure->getRempath());
  }
  // ===================================================

  // We'll create ssh connection only if one hasn't been passed as argument.
  // Also, we won't destroy a passed ssh as it belongs to the calling function.
  SSHConnection* ssh2 = (!ssh) ? m_opt->ssh()->getFreeConnection() : ssh;
  if (ssh2 == nullptr) {
    m_opt->warning(tr("Cannot connect to ssh server"));
    return false;
  }

  QProcess proc;
  QString command = "mkdir -p \"" + structure->getRempath() + "\"";
  QString stdout_str, stderr_str;
  int ec;
  if (!ssh2->execute(command, stdout_str, stderr_str, ec) || ec != 0) {
    m_opt->warning(tr("Error executing %1: %2").arg(command).arg(stderr_str));
    if (!ssh) m_opt->ssh()->unlockConnection(ssh2);
    return false;
  }
  if (!ssh) m_opt->ssh()->unlockConnection(ssh2);

  return true;
}

// This function, has a default 'nullptr' for ssh connection, so can
//   be called with/without it!
bool RemoteQueueInterface::cleanRemoteDirectory(Structure* structure,
                                                SSHConnection* ssh) const
{
  // ===================================================
  // If a local-remote run, perform this part and return
  if (m_opt->m_localQueue) {
    QDir dir(structure->getRempath());
    dir.setNameFilters(QStringList() << "*");
    dir.setFilter(QDir::Files);

    bool ok = true;
    foreach(QString dirFile, dir.entryList()) {
      if (!dir.remove(dirFile)) {
        ok = false;
      }
    }

    if (!ok) {
      m_opt->warning(
          tr("Error clearing remote directory %1").arg(structure->getRempath()));
      return false;
    }

    return true;
  }
  // ===================================================

  // We'll create ssh connection only if one hasn't been passed as argument.
  // Also, we won't destroy a passed ssh: it belongs to the calling function.
  SSHConnection* ssh2 = (!ssh) ? m_opt->ssh()->getFreeConnection() : ssh;
  if (ssh2 == nullptr) {
    m_opt->warning(tr("Cannot connect to ssh server"));
    return false;
  }

  // 2nd arg keeps the directory, only removes directory contents.
  if (!ssh2->removeRemoteDirectory(structure->getRempath(), true)) {
    m_opt->warning(
      tr("Error clearing remote directory %1").arg(structure->getRempath()));
    if (!ssh) m_opt->ssh()->unlockConnection(ssh2);
    return false;
  }
  if (!ssh) m_opt->ssh()->unlockConnection(ssh2);

  return true;
}

bool RemoteQueueInterface::logErrorDirectory(Structure* structure) const
{
  QString id_s, gen_s, strdir_s;
  id_s.sprintf("%05d", structure->getIDNumber());
  gen_s.sprintf("%05d", structure->getGeneration());
  strdir_s = gen_s + "x" + id_s;
  QString seps;

// Make the directory and copy the files into it
#ifdef WIN32
  seps = "\\";
#else
  seps = "/";
#endif

  QString path = this->m_opt->locWorkDir + seps + "errorDirs" + seps;

  // ===================================================
  // If a local-remote run, perform this part and return
  if (m_opt->m_localQueue) {
    QDir dir;
    if (!dir.mkpath(path)) {
      m_opt->warning("Error: could not create error directory " + path);
    }

    QString stdout_str, stderr_str;
    int exitcode;
    QString command = "scp -r " + structure->getRempath() + " " + path;

    return runACommand("", command, &stdout_str, &stderr_str, &exitcode);
  }
  // ===================================================

  SSHConnection* ssh = m_opt->ssh()->getFreeConnection();
  if (ssh == nullptr) {
    m_opt->warning(tr("Cannot connect to ssh server"));
    return false;
  }

  path += strdir_s + seps;
  QDir dir;
  if (!dir.mkpath(path)) {
    m_opt->warning("Error: could not create error directory " + path);
  }

  if (!ssh->copyDirectoryFromServer(structure->getRempath(), path)) {
    m_opt->error("Cannot copy from remote directory for Structure " +
                 structure->getTag());
    m_opt->ssh()->unlockConnection(ssh);
    return false;
  }
  m_opt->ssh()->unlockConnection(ssh);

  return true;
}

bool RemoteQueueInterface::copyRemoteFilesToLocalCache(Structure* structure) const
{
  // ===================================================
  // If a local-remote run, perform this part and return
  if (m_opt->m_localQueue) {
    QString stdout_str, stderr_str;
    int exitcode;
    QString command = "scp -r " + structure->getRempath() +
      " " + structure->getLocpath() + "..";

    return runACommand("", command, &stdout_str, &stderr_str, &exitcode);
  }
  // ===================================================

  SSHConnection* ssh = m_opt->ssh()->getFreeConnection();
  if (ssh == nullptr) {
    m_opt->warning(tr("Cannot connect to ssh server"));
    return false;
  }

  if (!ssh->copyDirectoryFromServer(structure->getRempath(),
        structure->getLocpath())) {
    m_opt->error("Cannot copy from remote directory for Structure " +
        structure->getTag());
    m_opt->ssh()->unlockConnection(ssh);
    return false;
  }
  m_opt->ssh()->unlockConnection(ssh);

  return true;
}
}

#endif // ENABLE_SSH

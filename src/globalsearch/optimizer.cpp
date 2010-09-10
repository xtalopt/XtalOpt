/**********************************************************************
  Optimizer - Generic optimizer interface

  Copyright (C) 2010 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <globalsearch/optimizer.h>
#include <globalsearch/macros.h>
#include <globalsearch/optbase.h>
#include <globalsearch/queuemanager.h>
#include <globalsearch/sshmanager.h>

#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QString>
#include <QtCore/QSettings>

#include <openbabel/obconversion.h>
#include <openbabel/mol.h>

using namespace OpenBabel;
using namespace Eigen;

namespace GlobalSearch {

  Optimizer::Optimizer(OptBase *parent, const QString &filename) :
    QObject(parent),
    m_opt(parent)
  {
    // Set allowed data structure keys, if any, e.g.
    // m_data.insert("Identifier name",QVariant())

    // Set allowed filenames, e.g.
    // m_templates.insert("filename.extension",QStringList)

    // Setup for completion values
    // m_completionFilename = name of file to check when opt stops
    // m_completionString   = string in m_completionFilename to search for

    // Set output filenames to try to read data from, e.g.
    // m_outputFilenames.append("output filename");
    // m_outputFilenames.append("input  filename");

    // Set the name of the optimizer to be returned by getIDString()
    m_idString = "Generic";

    readSettings(filename);
  }

  Optimizer::~Optimizer()
  {
  }

  void Optimizer::readSettings(const QString &filename)
  {
    // Don't consider default setting,, only schemes and states.
    if (filename.isEmpty()) {
      return;
    }

    SETTINGS(filename);

    readTemplatesFromSettings(filename);
    readUserValuesFromSettings(filename);
    readDataFromSettings(filename);
  }

  void Optimizer::readTemplatesFromSettings(const QString &filename)
  {
    SETTINGS(filename);

    QStringList filenames = getTemplateNames();
    for (int i = 0; i < filenames.size(); i++) {
      m_templates.insert(filenames.at(i),
                         settings->value(m_opt->getIDString().toLower() +
                                         "/optimizer/" +
                                         getIDString() + "/" +
                                         filenames.at(i) + "_list",
                                         "").toStringList());
    }
  }

  void Optimizer::readUserValuesFromSettings(const QString &filename)
  {
    SETTINGS(filename);

    settings->beginGroup(m_opt->getIDString().toLower() +
                         "/optimizer/" +
                         getIDString());
    m_user1 = settings->value("/user1", "").toString();
    m_user2 = settings->value("/user2", "").toString();
    m_user3 = settings->value("/user3", "").toString();
    m_user4 = settings->value("/user4", "").toString();
    settings->endGroup();
  }

  void Optimizer::readDataFromSettings(const QString &filename)
  {
    SETTINGS(filename);

    QStringList ids = getDataIdentifiers();
    for (int i = 0; i < ids.size(); i++) {
      m_data.insert(ids.at(i),
                         settings->value(m_opt->getIDString().toLower() +
                                         "/optimizer/" +
                                         getIDString() + "/data/" +
                                         ids.at(i),
                                         ""));
    }
  }


  void Optimizer::writeSettings(const QString &filename)
  {
    // Don't consider default setting,, only schemes and states.
    if (filename.isEmpty()) {
      return;
    }

    SETTINGS(filename);

    writeTemplatesToSettings(filename);
    writeUserValuesToSettings(filename);
    writeDataToSettings(filename);

    DESTROY_SETTINGS(filename);
  }

  void Optimizer::writeTemplatesToSettings(const QString &filename)
  {
    SETTINGS(filename);
    QStringList filenames = getTemplateNames();
    for (int i = 0; i < filenames.size(); i++) {
      settings->setValue(m_opt->getIDString().toLower() +
                         "/optimizer/" +
                         getIDString() + "/" +
                         filenames.at(i) + "_list",
                         m_templates.value(filenames.at(i)));
    }
  }

  void Optimizer::writeUserValuesToSettings(const QString &filename)
  {
    SETTINGS(filename);

    settings->setValue(m_opt->getIDString().toLower() +
                       "/optimizer/" +
                       getIDString() +
                       "/user1",
                       m_user1);
    settings->setValue(m_opt->getIDString().toLower() +
                       "/optimizer/" +
                       getIDString() +
                       "/user2",
                       m_user2);
    settings->setValue(m_opt->getIDString().toLower() +
                       "/optimizer/" +
                       getIDString() +
                       "/user3",
                       m_user3);
    settings->setValue(m_opt->getIDString().toLower() +
                       "/optimizer/" +
                       getIDString() +
                       "/user4",
                       m_user4);
  }

  void Optimizer::writeDataToSettings(const QString &filename)
  {
    SETTINGS(filename);
    QStringList ids = getDataIdentifiers();
    for (int i = 0; i < ids.size(); i++) {
      settings->setValue(m_opt->getIDString().toLower() +
                         "/optimizer/" +
                         getIDString() +
                         "/data/" +
                         ids.at(i),
                         m_data.value(ids.at(i)));
    }
  }

  bool Optimizer::writeInputFiles(Structure *structure) {
    // Stop any running jobs associated with this structure
    deleteJob(structure);

    // Lock
    QReadLocker locker (structure->lock());

    // Check optstep info
    int optStep = structure->getCurrentOptStep();
    if (optStep < 1 || optStep > getNumberOfOptSteps()) {
      m_opt->error(tr("Error: Requested OptStep (%1) out of range (1-%2)")
                   .arg(optStep)
                   .arg(getNumberOfOptSteps()));
      return false;
    }

    // Create local files
    QDir dir (structure->fileName());
    if (!dir.exists()) {
      if (!dir.mkpath(structure->fileName())) {
        m_opt->warning(tr("Cannot write input files to specified path: %1 (path creation failure)", "1 is a file path.").arg(structure->fileName()));
        return false;
      }
    }
    if (!createRemoteDirectory(structure)) return false;
    if (!cleanRemoteDirectory(structure)) return false;
    if (!writeTemplates(structure)) return false;
    if (!copyLocalTemplateFilesToRemote(structure)) return false;

    locker.unlock();
    structure->lock()->lockForWrite();
    structure->setStatus(Structure::WaitingForOptimization);
    structure->lock()->unlock();
    return true;
  }

  bool Optimizer::createRemoteDirectory(Structure *structure)
  {
    SSHConnection *ssh = m_opt->ssh()->getFreeConnection();

    if (!ssh->reconnectIfNeeded()) {
      m_opt->warning(tr("Cannot connect to ssh server %1@%2:%3")
                     .arg(ssh->getUser())
                     .arg(ssh->getHost())
                     .arg(ssh->getPort())
                     );
      m_opt->ssh()->unlockConnection(ssh);
      return false;
    }
    QString command = "mkdir -p " + structure->getRempath();
    qDebug() << "Optimizer::createRemoteDirectory: Calling " << command;
    QString stdout, stderr; int ec;
    if (!ssh->execute(command, stdout, stderr, ec) || ec != 0) {
      m_opt->warning(tr("Error executing %1: %2").arg(command).arg(stderr));
      m_opt->ssh()->unlockConnection(ssh);
      return false;
    }
    m_opt->ssh()->unlockConnection(ssh);
    return true;
  }

  bool Optimizer::cleanRemoteDirectory(Structure *structure)
  {
    SSHConnection *ssh = m_opt->ssh()->getFreeConnection();

    if (!ssh->reconnectIfNeeded()) {
      m_opt->warning(tr("Cannot connect to ssh server %1@%2:%3")
                     .arg(ssh->getUser())
                     .arg(ssh->getHost())
                     .arg(ssh->getPort())
                     );
      m_opt->ssh()->unlockConnection(ssh);
      return false;
    }
    // 2nd arg keeps the directory, only removes directory contents.
    if (!ssh->removeRemoteDirectory(structure->getRempath(), true)) {
      m_opt->warning(tr("Error clearing remote directory %1")
                     .arg(structure->getRempath()));
      m_opt->ssh()->unlockConnection(ssh);
      return false;
    }
    m_opt->ssh()->unlockConnection(ssh);
    return true;
  }

  bool Optimizer::writeTemplates(Structure *structure) {
    // Create file objects
    QList<QFile*> files;
    QStringList filenames = getTemplateNames();
    for (int i = 0; i < filenames.size(); i++) {
      files.append(new QFile (structure->fileName() + "/" + filenames.at(i)));
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
    int optStepInd = structure->getCurrentOptStep() - 1;
    for (int i = 0; i < streams.size(); i++) {
      *(streams.at(i)) << m_opt->interpretTemplate( m_templates.value(filenames.at(i)).at(optStepInd),
                                                    structure);
    }

    // Close files
    for (int i = 0; i < files.size(); i++) {
      files.at(i)->close();
    }

    // Clean up
    qDeleteAll(files);
    qDeleteAll(streams);
    return true;
  }

  bool Optimizer::copyLocalTemplateFilesToRemote(Structure *structure)
  {
    SSHConnection *ssh = m_opt->ssh()->getFreeConnection();

    if (!ssh->reconnectIfNeeded()) {
      m_opt->warning(tr("Cannot connect to ssh server %1@%2:%3")
                     .arg(ssh->getUser())
                     .arg(ssh->getHost())
                     .arg(ssh->getPort())
                     );
      m_opt->ssh()->unlockConnection(ssh);
      return false;
    }
    QStringList templates = getTemplateNames();
    QStringList::const_iterator it;
    for (it = templates.begin(); it != templates.end(); it++) {
      if (!ssh->copyFileToServer(structure->fileName() + "/" + (*it),
                                          structure->getRempath() + "/" + (*it))) {
        m_opt->warning(tr("Error copying \"%1\" to remote server (structure %2)")
                       .arg(*it)
                       .arg(structure->getIDString()));
        m_opt->ssh()->unlockConnection(ssh);
        return false;
      }
    }
    m_opt->ssh()->unlockConnection(ssh);
    return true;
  }

  bool Optimizer::startOptimization(Structure *structure) {
    SSHConnection *ssh = m_opt->ssh()->getFreeConnection();

    if (!ssh->reconnectIfNeeded()) {
      m_opt->warning(tr("Cannot connect to ssh server %1@%2:%3")
                     .arg(ssh->getUser())
                     .arg(ssh->getHost())
                     .arg(ssh->getPort())
                     );
      m_opt->ssh()->unlockConnection(ssh);
      return false;
    }
    QString command = "cd " + structure->getRempath() + " && " +
      m_opt->qsub + " job.pbs";
    qDebug() << "Optimizer::startOptimization: Calling " << command;
    QString stdout, stderr; int ec;
    if (!ssh->execute(command, stdout, stderr, ec) || ec != 0) {
      m_opt->warning(tr("Error executing %1: %2").arg(command).arg(stderr));
      m_opt->ssh()->unlockConnection(ssh);
      return false;
    }

    // Assuming stdout value is <jobID>.trailing.garbage.hostname.edu or similar
    uint jobID = stdout.split(".")[0].toUInt();

    // lock for writing and update structure
    QWriteLocker wlocker (structure->lock());
    structure->setJobID(jobID);
    structure->startOptTimer();
    structure->setStatus(Structure::Submitted);
    m_opt->ssh()->unlockConnection(ssh);
    return true;
  }

  bool Optimizer::checkIfOutputFileExists(const QString & filename)
  {
    SSHConnection *ssh = m_opt->ssh()->getFreeConnection();

    if (!ssh->reconnectIfNeeded()) {
      m_opt->warning(tr("Cannot connect to ssh server %1@%2:%3")
                     .arg(ssh->getUser())
                     .arg(ssh->getHost())
                     .arg(ssh->getPort())
                     );
      m_opt->ssh()->unlockConnection(ssh);
      return false;
    }
    QString command;

    command = "[ -e " + filename + " ]";
    qDebug() << "Optimizer::checkIfOutputFileExists: Calling " << command;
    QString stdout, stderr; int ec;
    if (!ssh->execute(command, stdout, stderr, ec)) {
      m_opt->warning(tr("Error executing %1: %2").arg(command).arg(stderr));
      m_opt->ssh()->unlockConnection(ssh);
      return false;
    }

    m_opt->ssh()->unlockConnection(ssh);
    return (!ec);
  }

  bool Optimizer::getOutputFile(const QString & filename,
                                QStringList & data)
  {
    SSHConnection *ssh = m_opt->ssh()->getFreeConnection();

    if (!ssh->reconnectIfNeeded()) {
      m_opt->warning(tr("Cannot connect to ssh server %1@%2:%3")
                     .arg(ssh->getUser())
                     .arg(ssh->getHost())
                     .arg(ssh->getPort())
                     );
      m_opt->ssh()->unlockConnection(ssh);
      return false;
    }
    QString tmp;
    if (!ssh->readRemoteFile(filename, tmp)) {
      m_opt->warning(tr("Error retrieving remote file %1.")
                     .arg(filename));
      m_opt->ssh()->unlockConnection(ssh);
      return false;
    }

    // Fill data list
    data = tmp.split("\n", QString::KeepEmptyParts);
    m_opt->ssh()->unlockConnection(ssh);
    return true;
  }

  bool Optimizer::copyRemoteToLocalCache(Structure *structure)
  {
    SSHConnection *ssh = m_opt->ssh()->getFreeConnection();

    if (!ssh->reconnectIfNeeded()) {
      m_opt->warning(tr("Cannot connect to ssh server %1@%2:%3")
                     .arg(ssh->getUser())
                     .arg(ssh->getHost())
                     .arg(ssh->getPort())
                     );
      m_opt->ssh()->unlockConnection(ssh);
      return false;
    }
    // lock structure
    QReadLocker locker (structure->lock());
    QString stdout, stderr; int ec;
    if (!ssh->copyDirectoryFromServer(structure->getRempath(),
                                               structure->fileName())) {
      m_opt->error("Cannot copy from remote directory for Structure "
                   + structure->getIDString());
      m_opt->ssh()->unlockConnection(ssh);
      return false;
    }
    m_opt->ssh()->unlockConnection(ssh);
    return true;
  }

  Optimizer::JobState Optimizer::getStatus(Structure *structure)
  {
    SSHConnection *ssh = m_opt->ssh()->getFreeConnection();

    if (!ssh->reconnectIfNeeded()) {
      m_opt->warning(tr("Cannot connect to ssh server %1@%2:%3")
                     .arg(ssh->getUser())
                     .arg(ssh->getHost())
                     .arg(ssh->getPort())
                     );
      m_opt->ssh()->unlockConnection(ssh);
      return Optimizer::CommunicationError;
    }
    // lock structure
    QWriteLocker locker (structure->lock());
    QStringList queueData (m_opt->queue()->getRemoteQueueData());
    uint jobID = structure->getJobID();

    // If jobID = 0, return an error.
    if (!jobID) {
      structure->setStatus(Structure::Error);
      m_opt->ssh()->unlockConnection(ssh);
      return Optimizer::Error;
    }

    // Determine status if structure is in the queue
    QString status;
    for (int i = 0; i < queueData.size(); i++) {
      if (queueData.at(i).split(".")[0] == QString::number(jobID)) {
        status = (queueData.at(i).split(QRegExp("\\s+")))[4];
        continue;
      }
    }

    // If structure is submitted, check if it is in the queue. If not,
    // check if the completion file has been written.
    //
    // If the completion file exists, then the job finished before the
    // queue checks could see it, and the function will continue on to
    // the status checks below.
    if (structure->getStatus() == Structure::Submitted) {
      // Structure is submitted
      if (status.isEmpty()) {
        // Job is not in queue
        if (checkIfOutputFileExists(structure->getRempath() +
                                    "/" + m_completionFilename) ) {
          // The output file exists -- the job completed. No further
          // action needed.
          qDebug() << "Structure " << structure->getIDString()
                   << " ran before it was noticed in the queue.";
        }
        else {
          // The output file does not exist -- the job is still
          // pending.
          m_opt->ssh()->unlockConnection(ssh);
          return Optimizer::Pending;
        }
      }
      else {
        // The job is in the queue.
        m_opt->ssh()->unlockConnection(ssh);
        return Optimizer::Started;
      }
    }

    if (status == "R") {
      if (structure->getOptElapsed() == "0:00:00")
        structure->startOptTimer();
      m_opt->ssh()->unlockConnection(ssh);
      return Optimizer::Running;
    }
    else if (status == "Q") {
      m_opt->ssh()->unlockConnection(ssh);
      return Optimizer::Queued;
    }
    // Even if the job has errored in the queue, leave it as "running"
    // and wait for it to leave the queue then check the m_completion
    // file. The optimization may have finished OK.
    else if (status == "E") {
      qWarning() << "Optimizer::getStatus: Structure " << structure->getIDString()
                 << " has errored in the queue, but may have optimized successfully.\n"
                 << "Marking job as 'Running' until it's gone from the queue...";
      m_opt->ssh()->unlockConnection(ssh);
      return Optimizer::Running;
    }

    else { // Entry is missing from queue! Were the output files written?
      bool outputFileExists;
      QString rempath = structure->getRempath();
      QString fileName = structure->fileName();

      // Check for m_completionString in m_completionFilename
      locker.unlock();
      outputFileExists = checkIfOutputFileExists(rempath
                                                 + "/"
                                                 + m_completionFilename);
      locker.relock();

      // Check for m_completionString in outputFileData, which indicates success.
      qDebug() << "Optimizer::getStatus: Job  " << jobID << " not in queue. Does output exist? " << outputFileExists;
      if (outputFileExists) {
        QString stdout, stderr; int ec;
        // Valid exit codes for grep: (0) matches found, execution successful
        //                            (1) no matches found, execution successful
        //                            (2) execution unsuccessful
        QString command = "grep \'" + m_completionString + "\' "
          + rempath + "/" + m_completionFilename;
        qDebug() << "Optimizer::getStatus: Calling " << command;
        if (!ssh->execute(command, stdout, stderr, ec)) {
          m_opt->warning(tr("Error executing %1: %2").arg(command).arg(stderr));
          m_opt->ssh()->unlockConnection(ssh);
          return Optimizer::CommunicationError;
        }
        if (ec == 0) {
          structure->resetFailCount();
          m_opt->ssh()->unlockConnection(ssh);
          return Optimizer::Success;
        }
        // Otherwise, it's an error!
        structure->setStatus(Structure::Error);
        m_opt->ssh()->unlockConnection(ssh);
        return Optimizer::Error;
      }
    }
    // Not in queue and no output? Interesting...
    structure->setStatus(Structure::Error);
    m_opt->ssh()->unlockConnection(ssh);
    return Optimizer::Unknown;
  }

  bool Optimizer::deleteJob(Structure *structure) {
    SSHConnection *ssh = m_opt->ssh()->getFreeConnection();

    if (!ssh->reconnectIfNeeded()) {
      m_opt->warning(tr("Cannot connect to ssh server %1@%2:%3")
                     .arg(ssh->getUser())
                     .arg(ssh->getHost())
                     .arg(ssh->getPort())
                     );
      m_opt->ssh()->unlockConnection(ssh);
      return false;
    }
    // lock structure
    QWriteLocker locker (structure->lock());
    QString command;

    // jobid has not been set, cannot delete!
    if (structure->getJobID() == 0) {
      m_opt->ssh()->unlockConnection(ssh);
      return true;
    }

    command = m_opt->qdel + " " + QString::number(structure->getJobID());

    // Execute
    qDebug() << "Optimizer::deleteJob: Calling " << command;
    QString stdout, stderr; int ec;
    if (!ssh->execute(command, stdout, stderr, ec) || ec != 0) {
      m_opt->warning(tr("Error executing %1: %2").arg(command).arg(stderr));
      // Most likely job is already gone from queue. Set jobID to 0.
      structure->setJobID(0);
      m_opt->ssh()->unlockConnection(ssh);
      return false;
    }

    structure->setJobID(0);
    structure->stopOptTimer();
    m_opt->ssh()->unlockConnection(ssh);
    return true;
  }

  bool Optimizer::getQueueList(QStringList & queueData, QMutex *mutex) {
    SSHConnection *ssh = m_opt->ssh()->getFreeConnection();

    if (!ssh->reconnectIfNeeded()) {
      m_opt->warning(tr("Cannot connect to ssh server %1@%2:%3")
                     .arg(ssh->getUser())
                     .arg(ssh->getHost())
                     .arg(ssh->getPort())
                     );
      m_opt->ssh()->unlockConnection(ssh);
      return false;
    }
    QString command;
    command = m_opt->qstat + " | grep " + m_opt->username;

    // Execute
    qDebug() << "Optimizer::getQueueList: Calling " << command;
    QString stdout, stderr; int ec;
    // Valid exit codes for grep: (0) matches found, execution successful
    //                            (1) no matches found, execution successful
    //                            (2) execution unsuccessful
    if (!ssh->execute(command, stdout, stderr, ec)
        || (ec != 0 && ec != 1 )
        ) {
      m_opt->warning(tr("Error executing %1: (%2) %3")
                     .arg(command)
                     .arg(QString::number(ec))
                     .arg(stderr));
      m_opt->ssh()->unlockConnection(ssh);
      return false;
    }

    QMutexLocker queueDataMutexLocker (mutex);
    queueData = stdout.split("\n", QString::SkipEmptyParts);
    m_opt->ssh()->unlockConnection(ssh);
    return true;
  }

  int Optimizer::checkIfJobNameExists(Structure *structure,
                                      const QStringList & queueData,
                                      bool & exists) {
    structure->lock()->lockForRead();
    QFile jobScript (structure->fileName() + "/job.pbs");
    structure->lock()->unlock();
    QString line, jobName;
    QStringList strl;
    exists = false;

    if (!jobScript.open(QIODevice::ReadOnly | QIODevice::Text)) {
      m_opt->warning(tr("Optimizer::checkIfJobNameExists: Error opening file: %1")
                 .arg(jobScript.fileName()));
      return 0;
    }
    while (!jobScript.atEnd()) {
      line = jobScript.readLine();
      if (line.contains("#PBS -N")) {
        jobName = line.split(QRegExp("\\s+"))[2];
        break;
      }
    }

    qDebug() << "Optimizer::checkIfJobNameExists: Job name is "
             << jobName << " according to "
             << jobScript.fileName() << "...";

    for (int i = 0; i < queueData.size(); i++) {
      if (queueData.at(i).contains(jobName)) {
        // Set the bool to true, and return job ID.
        exists = true;
        int jobID = queueData.at(i).split(".").at(0).toInt();
        qDebug() << "Optimizer::checkIfJobNameExists: Found it! ID = " << jobID;
        return jobID;
      }
    }

    return 0;
  }

  // Convenience functions
  bool Optimizer::update(Structure *structure)
  {
    // lock structure
    QWriteLocker locker (structure->lock());

    // Update structure status
    structure->setStatus(Structure::Updating);
    structure->stopOptTimer();

    // Copy remote files over
    locker.unlock();
    bool ok = copyRemoteToLocalCache(structure);
    locker.relock();
    if (!ok) {
      m_opt->warning(tr("Optimizer::update: Error copying remote files to local dir\n\tremote: %1 on %2@%3\n\tlocal: %4")
                     .arg(structure->getRempath())
                     .arg(m_opt->username)
                     .arg(m_opt->host)
                     .arg(structure->fileName()));
      structure->setStatus(Structure::Error);
      return false;
    }

    // Try to read all files in outputFileNames
    ok = false;
    for (int i = 0; i < m_outputFilenames.size(); i++) {
      if (read(structure,
               structure->fileName() + "/" + m_outputFilenames.at(i))) {
        ok = true;
        break;
      }
    }
    if (!ok) {
      m_opt->warning(tr("Optimizer::Update: Error loading structure at %1")
                 .arg(structure->fileName()));
      return false;
    }

    structure->setJobID(0);
    structure->setStatus(Structure::StepOptimized);
    locker.unlock();
    return true;
  }

  bool Optimizer::load(Structure *structure)
  {
    QWriteLocker locker (structure->lock());

    // Try to read all files in outputFileNames
    bool ok = false;
    for (int i = 0; i < m_outputFilenames.size(); i++) {
      if (read(structure,
               structure->fileName() + "/" + m_outputFilenames.at(i))) {
        ok = true;
        break;
      }
    }
    if (!ok) {
      m_opt->warning(tr("Optimizer::load: Error loading structure at %1")
                 .arg(structure->fileName()));
      structure->setStatus(Structure::Error);
      return false;
    }
    return true;
  }

  bool Optimizer::read(Structure *structure,
                       const QString & filename) {
    // Test filename
    QFile file (filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      return false;
    }
    file.close();

    // Read in OBMol
    //
    // OpenBabel::OBConversion;:ReadFile calls a singleton error class
    // that is not thread safe. Hence sOBMutex is necessary.
    m_opt->sOBMutex->lock();
    OBConversion conv;
    OBFormat* inFormat = conv.FormatFromExt(QString(QFile::encodeName(filename.trimmed())).toAscii());

    if ( !inFormat || !conv.SetInFormat( inFormat ) ) {
      m_opt->warning(tr("Optimizer::read: Error setting format for file %1")
                 .arg(filename));
      structure->setStatus(Structure::Error);
      m_opt->sOBMutex->unlock();
      return false;
    }

    OBMol obmol;
    conv.ReadFile( &obmol, QString(QFile::encodeName(filename)).toStdString());
    m_opt->sOBMutex->unlock();

    // Copy settings from obmol -> structure.
    // atoms
    while (structure->numAtoms() < obmol.NumAtoms())
      structure->addAtom();
    QList<Atom*> atoms = structure->atoms();
    uint i = 0;

    FOR_ATOMS_OF_MOL(atm, obmol) {
      atoms.at(i)->setPos(Vector3d(atm->x(), atm->y(), atm->z()));
      atoms.at(i)->setAtomicNumber(atm->GetAtomicNum());
      i++;
    }

    // energy/enthalpy
    const double KCAL_PER_MOL_TO_EV = 0.0433651224;
    if (obmol.HasData("Enthalpy (kcal/mol)"))
      structure->setEnthalpy(QString(obmol.GetData("Enthalpy (kcal/mol)")->GetValue().c_str()).toFloat()
                        * KCAL_PER_MOL_TO_EV);
    if (obmol.HasData("Enthalpy PV term (kcal/mol)"))
      structure->setPV(QString(obmol.GetData("Enthalpy PV term (kcal/mol)")->GetValue().c_str()).toFloat()
                  * KCAL_PER_MOL_TO_EV);
    structure->setEnergy(obmol.GetEnergy());
    // Modify as needed!

    return true;
  }

  int Optimizer::getNumberOfOptSteps() {
    if (m_templates.isEmpty())
      return 0;
    else
      return m_templates.values().first().size();
  }

  bool Optimizer::setTemplate(const QString &filename, const QString &templateData, int optStep)
  {
    // check if template name is valid
    if (!m_templates.contains(filename)) {
      m_opt->warning(tr("Optimizer::setTemplate: unknown filename '%1'")
                     .arg(filename));
      return false;
    }

    // Check if optStep is reasonable
    if (optStep < 0 || optStep > getNumberOfOptSteps() - 1) {
      m_opt->warning(tr("Optimizer::setTemplate: bad optStep '%1'")
                     .arg(optStep));
      return "Error in Optimizer::setTemplate\n";
    }

    m_templates[filename][optStep] = templateData;
    return true;
  }

  bool Optimizer::setTemplate(const QString &filename, const QStringList &templateData)
  {
    // check if template name is valid
    if (!m_templates.contains(filename)) {
      m_opt->warning(tr("Optimizer::setTemplate: unknown filename '%1'")
                     .arg(filename));
      return false;
    }

    m_templates.insert(filename, templateData);
    return true;
  }


  QString Optimizer::getTemplate(const QString &filename, int optStep)
  {
    // check if template name is valid
    if (!m_templates.contains(filename)) {
      m_opt->warning(tr("Optimizer::getTemplate: unknown filename '%1'")
                     .arg(filename));
      return "Error in Optimizer::getTemplate\n";
    }

    // Check if optStep is reasonable
    if (optStep < 0 || optStep > getNumberOfOptSteps() - 1) {
      m_opt->warning(tr("Optimizer::getTemplate: bad optStep '%1'")
                     .arg(optStep));
      return "Error in Optimizer::getTemplate\n";
    }

    // Return value
    return m_templates.value(filename).at(optStep);
  }

  QStringList Optimizer::getTemplate(const QString &filename)
  {
    // check if template name is valid
    if (!m_templates.contains(filename)) {
      m_opt->warning(tr("Optimizer::getTemplate: unknown filename '%1'")
                     .arg(filename));
      return QStringList("Error in Optimizer::getTemplate\n");
    }

    // Return value
    return m_templates.value(filename);
  }

  bool Optimizer::appendTemplate(const QString &filename, const QString &templateData)
  {
    // check if template name is valid
    if (!m_templates.contains(filename)) {
      m_opt->warning(tr("Optimizer::appendTemplate: unknown filename '%1'")
                     .arg(filename));
      return "Error in Optimizer::appendTemplate\n";
    }

    m_templates[filename].append(templateData);
  }

  bool Optimizer::removeTemplate(const QString &filename, int optStep)
  {
    // check if template name is valid
    if (!m_templates.contains(filename)) {
      m_opt->warning(tr("Optimizer::removeTemplate: unknown filename '%1'")
                     .arg(filename));
      return "Error in Optimizer::removeTemplate\n";
    }

    // Check if optStep is reasonable
    if (optStep < 0 || optStep > getNumberOfOptSteps() - 1) {
      m_opt->warning(tr("Optimizer::removeTemplate: bad optStep '%1'")
                     .arg(optStep));
      return "Error in Optimizer::removeTemplate\n";
    }

    m_templates[filename].removeAt(optStep);
  }



  bool Optimizer::setData(const QString &identifier, const QVariant &data)
  {
    // check if template name is valid
    if (!m_data.contains(identifier)) {
      m_opt->warning(tr("Optimizer::setTemplate: unknown data identifier '%1'")
                     .arg(identifier));
      return false;
    }

    m_data.insert(identifier, data);
    return true;
  }

  QVariant Optimizer::getData(const QString &identifier)
  {
    // check if template name is valid
    if (!m_data.contains(identifier)) {
      m_opt->warning(tr("Optimizer::setTemplate: unknown data identifier '%1'")
                     .arg(identifier));
      return false;
    }

    // Return value
    return m_data.value(identifier);
  }

} // end namespace Avogadro

//#include "optimizer.moc"

/**********************************************************************
  Optimizer - Generic optimizer interface

  Copyright (C) 2010 by David C. Lonie

  This file is part of the Avogadro molecular editor project.
  For more information, see <http://avogadro.openmolecules.net/>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include "optimizer.h"
#include "templates.h"
#include "xtal.h"
#include "macros.h"

#include <QDir>
#include <QDebug>
#include <QString>
#include <QProcess>
#include <QSettings>

#include <openbabel/obconversion.h>
#include <openbabel/mol.h>

using namespace std;
using namespace OpenBabel;
using namespace Eigen;

namespace Avogadro {

  Optimizer::Optimizer(XtalOpt *parent) :
    QObject(parent),
    m_opt(parent)    
  {
    // Set allowed data structure keys, if any, e.g.
    // m_data.insert("Identifier name",QVariant())

    // Set allowed filenames, e.g.
    // m_templates.insert("filename.extension",QStringList)

    // Setup for completion values
    // m_completionFilename = name of file to check when opt stops
    // m_completionString   = string in last 100 lines of m_completionFilename to search for

    // Set output filenames to try to read data from, e.g.
    // m_outputFilenames.append("output filename");
    // m_outputFilenames.append("input  filename");

    // Set the name of the optimizer to be returned by getIDString()
    m_idString = "Generic";

    readSettings();
  }

  Optimizer::~Optimizer()
  {
    writeSettings();
  }

  void Optimizer::readSettings(const QString &filename)
  {
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
                         settings->value("xtalopt/optimizer/" + 
                                         getIDString() + "/" +
                                         filenames.at(i) + "_list",
                                         "").toStringList());
    }
  }

  void Optimizer::readUserValuesFromSettings(const QString &filename)
  {
    SETTINGS(filename);

    settings->beginGroup("xtalopt/optimizer/" + getIDString());
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
                         settings->value("xtalopt/optimizer/" + 
                                         getIDString() + "/data/" +
                                         ids.at(i),
                                         ""));
    }
  }


  void Optimizer::writeSettings(const QString &filename)
  {
    SETTINGS(filename);
                         
    writeTemplatesToSettings(filename);
    writeUserValuesToSettings(filename);
    writeDataToSettings(filename);
  }

  void Optimizer::writeTemplatesToSettings(const QString &filename)
  {
    SETTINGS(filename);
    QStringList filenames = getTemplateNames();
    for (int i = 0; i < filenames.size(); i++) {
      settings->setValue("xtalopt/optimizer/" + 
                    getIDString() + "/" +
                    filenames.at(i) + "_list",
                    m_templates.value(filenames.at(i)));
    }
  }

  void Optimizer::writeUserValuesToSettings(const QString &filename)
  {
    SETTINGS(filename);

    settings->setValue("xtalopt/optimizer/" + getIDString() + "/user1",
                       m_user1);
    settings->setValue("xtalopt/optimizer/" + getIDString() + "/user2",
                       m_user2);
    settings->setValue("xtalopt/optimizer/" + getIDString() + "/user3",
                       m_user3);
    settings->setValue("xtalopt/optimizer/" + getIDString() + "/user4",
                       m_user4);
  }

  void Optimizer::writeDataToSettings(const QString &filename)
  {
    SETTINGS(filename);
    QStringList ids = getDataIdentifiers();
    for (int i = 0; i < ids.size(); i++) {
      settings->setValue("xtalopt/optimizer/" + 
                         getIDString() + "/data/" +
                         ids.at(i),
                         m_data.value(ids.at(i)));
    }
  }

  bool Optimizer::writeInputFiles(Structure *structure) {
    // Stop any running jobs associated with this xtal
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
    if (!writeTemplates(structure)) return false;
    if (!copyLocalTemplateFilesToRemote(structure)) return false;

    locker.unlock();
    structure->lock()->lockForWrite();
    structure->setStatus(Structure::WaitingForOptimization);
    structure->lock()->unlock();
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
      *(streams.at(i)) << XtalOptTemplate::interpretTemplate( m_templates.value(filenames.at(i)).at(optStepInd),
                                                           structure, m_opt);
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

  bool Optimizer::copyLocalTemplateFilesToRemote(Structure *structure) {
    // Remote writing
    // Use a QProcess to scp the directory over, but first delete existing rempath
    // e.g.,
    // ssh -q <user>@<host> sh -c 'mkdir -p <rempath>|cat'
    // ssh -q <user>@<host> sh -c 'rm -rf <rempath>/*|cat'
    // scp -q <structure->fileName()>/<FILE> <user>@<host>:<rempath>/..
    // ssh -q <user>@<host> <queueInsertProgram> <scriptname>
    QString command;

    // mkdir
    command = "ssh -q " + m_opt->username + "@" + m_opt->host + 
      " sh -c \'mkdir -p " + structure->getRempath() + "|cat\'";
    qDebug() << "Optimizer::copyLocalTemplateFilesToRemote: Calling " << command;
    if (QProcess::execute(command) != 0) {
      m_opt->warning(tr("Error executing %1").arg(command));
      return false;
    }

    // rm -rf
    // the cat is neccessary on teragrid to get around the 'rm: No match' error when calling
    // * on an empty directory. Even though -f is supposed to ignore non-existant files...
    command = "ssh -q " + m_opt->username + "@" + m_opt->host + 
      " sh -c \'rm -rf " + structure->getRempath() + "/*|cat\'";
    qDebug() << "Optimizer::copyLocalTemplateFilesToRemote: Calling " << command;
    if (QProcess::execute(command) != 0) {
      m_opt->warning(tr("Error executing %1").arg(command));
      return false;
    }

    // scp
    command = "scp -qr " + 
      structure->fileName() + " " + 
      m_opt->username + "@" + m_opt->host + ":" + structure->getRempath() + "/..";
    qDebug() << "Optimizer::copyLocalTemplateFilesToRemote: Calling " << command;
    if (QProcess::execute(command) != 0) {
      m_opt->warning(tr("Optimizer::copyLocalTemplateFilesToRemote: Error executing %1").arg(command));
      return false;
    }

    return true;
  }

  bool Optimizer::startOptimization(Structure *structure) {
    // ssh
    QProcess proc;
    QString command = "ssh -q " + m_opt->username + "@" + m_opt->host + " " + 
      m_opt->qsub + " " + structure->getRempath() + "/job.pbs";
    qDebug() << "Optimizer::startOptimization: Calling " << command;
    proc.start(command);
    proc.waitForFinished(-1);
    if (proc.exitCode() != 0) {
      m_opt->warning(tr("Optimizer::startOptimization: Error executing %1:\n\t%2")
                     .arg(command)
                     .arg(QString(proc.readAllStandardError())));
      return false;
    }

    // read data in
    proc.setReadChannel(QProcess::StandardOutput);

    // Assuming the return value is <jobID>.trailing.garbage.hostname.edu or similar
    uint jobID = (QString(proc.readLine()).split(".")[0]).toUInt();

    // lock for writing and update structure
    QWriteLocker wlocker (structure->lock());
    structure->setJobID(jobID);
    structure->startOptTimer();
    structure->setStatus(Xtal::Submitted);
    return true;
  }

  bool Optimizer::getOutputFile(const QString & filename,
                                QStringList & data)
  {
    QProcess proc;
    QString command;

    // ssh -q <user>@<host> cat <path>/<filename>
    command = "ssh -q " + m_opt->username + "@" + m_opt->host + " " + 
      "cat " + filename;
    qDebug() << "Optimizer::getOutputFile: Calling " << command;
    proc.start(command);
    proc.waitForFinished(-1);
    if (proc.exitCode() != 0) {
      m_opt->warning(tr("Optimizer::getOutputFile: Error executing %1:\n\t%2")
                     .arg(command)
                     .arg(QString(proc.readAllStandardError())));
      return false;
    }

    // Fill data list
    proc.setReadChannel(QProcess::StandardOutput);
    while (!proc.atEnd())
      data << proc.readLine();
    return true;
  }

  bool Optimizer::copyRemoteToLocalCache(Structure *structure)
  {
    // lock structure
    QReadLocker locker (structure->lock());
    QString command;

    command = "scp -r " + 
      m_opt->username + "@" + m_opt->host + ":" + structure->getRempath() + " " + 
      structure->fileName() + "/..";
    qDebug() << "Optimizer::copyRemoteToLocalCache: Calling " << command;
    if (QProcess::execute(command) != 0) {
      m_opt->warning(tr("Optimizer::copyRemoteToLocalCache: Error executing %1")
                     .arg(command));
      return false;
    }
    return true;
  }

  Optimizer::JobState Optimizer::getStatus(Structure *structure)
  {
    // lock structure
    QWriteLocker locker (structure->lock());
    QStringList queueData (m_opt->queue()->getRemoteQueueData());
    QProcess proc;
    QString command;
    uint jobID = structure->getJobID();

    // If jobID = 0, return an error.
    if (!jobID) {
      structure->setStatus(Structure::Error);
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

    // Check if xtal is submitted:
    if (structure->getStatus() == Structure::Submitted) {
      if (status.isEmpty()) {
        return Optimizer::Pending;
      }
      else {
        return Optimizer::Started;
      }
    }

    if (status == "R") {
      if (structure->getOptElapsed() == "0:00:00")
        structure->startOptTimer();
      return Optimizer::Running;
    }
    else if (status == "Q")
      return Optimizer::Queued;
    // Even if the job has errored in the queue, leave it as "running" and wait for it to leave the queue
    // then check the m_completion file. The optimization may have finished OK.
    else if (status == "E") {
      qWarning() << "Optimizer::getStatus: Structure " << structure->getIDString()
                 << " has errored in the queue, but may have optimized successfully.\n"
                 << "Marking job as 'Running' until it's gone from the queue...";
      return Optimizer::Running;
    }

    else { // Entry is missing from queue! Were the output files written?
      QStringList outputFileData;
      bool outputFileExists;
      QString rempath = structure->getRempath();
      QString fileName = structure->fileName();

      // Get the last 100 lines of the m_completion file -- the line we need should be in there...
      locker.unlock();
      outputFileExists = getOutputFile(rempath + "/" + m_completionFilename +"|tail -n 100", outputFileData);
      locker.relock();

      // Check for m_completionString in outputFileData, which indicates success.
      qDebug() << "Optimizer::getStatus: Job  " << jobID << " not in queue. Does output exist? " << outputFileExists;
      if (outputFileExists) {
        for (int i = 0; i < outputFileData.size(); i++) {
          if (outputFileData.at(i).contains(m_completionString)) {
            structure->resetFailCount();
            return Optimizer::Success;
          }
        }
        // Otherwise, it's an error!
        structure->setStatus(Xtal::Error);
        return Optimizer::Error;
      }
    }
    // Not in queue and no output? Interesting...
    structure->setStatus(Xtal::Error);
    return Optimizer::Unknown;
  }

  bool Optimizer::deleteJob(Structure *structure) {
    // lock structure
    QWriteLocker locker (structure->lock());
    QProcess proc;
    QString command;

    if (structure->getJobID() == 0) // jobid has not been set, cannot delete!
      return true;

    // ssh -q <user>@<host> <qdel> <jobID>
    command = "ssh -q " + m_opt->username + "@" + m_opt->host + " " + 
      m_opt->qdel + " " + QString::number(structure->getJobID());

    // Execute
    qDebug() << "Optimizer::deleteJob: Calling " << command;
    proc.start(command);
    proc.waitForFinished(); // times out in 30 seconds
    if (proc.exitCode() != 0) {
      m_opt->warning(tr("Optimizer::deleteJob: Error executing %1:\n\t%2")
                     .arg(command)
                     .arg(QString(proc.readAllStandardError())));
      // Most likely job is already gone from queue. Set jobID to 0.
      structure->setJobID(0);
      return false;
    }

    structure->setJobID(0);
    structure->stopOptTimer();
    return true;
  }

  bool Optimizer::getQueueList(QStringList & queueData) {
    QProcess proc;
    QString command;

    // ssh -q <user>@<host> sh -c '<qstat>|grep <username>|cat'
    // Somewhat ridiculous construct ensures that a non-zero status indicates a problem with ssh, i.e. communication error.
    command = "ssh -q " + m_opt->username + "@" + m_opt->host + " sh -c \'"
      + m_opt->qstat + "|grep " + m_opt->username + "|cat\'";

    // Execute
    qDebug() << "Optimizer::getQueueList: Calling " << command;
    proc.start(command);
    proc.waitForFinished(-1);
    if (proc.exitCode() != 0) {
      m_opt->warning(tr("Optimizer::getQueueList: Error executing %1:\n\t%2")
                     .arg(command)
                     .arg(QString(proc.readAllStandardError())));
      return false;
    }

    // read data in
    proc.setReadChannel(QProcess::StandardOutput);

    queueData.clear();
    while (proc.canReadLine())
      queueData << proc.readLine();

    return true;
  }

  int Optimizer::checkIfJobNameExists(Structure *structure, const QStringList & queueData, bool & exists) {
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

    // Update Xtal status
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
    structure->setStatus(Xtal::StepOptimized);
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
      return false;
    }
    return true;
  }

  bool Optimizer::read(Structure *structure,
                         const QString & filename) {
    // Recast structure as xtal -- we'll need to access cell data later.
    Xtal *xtal = qobject_cast<Xtal*>(structure);
    // Test filename
    QFile file (filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      xtal->setStatus(Xtal::Error);
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
      xtal->setStatus(Xtal::Error);
      m_opt->sOBMutex->unlock();
      return false;
    }

    OBMol obmol;
    conv.ReadFile( &obmol, QString(QFile::encodeName(filename)).toStdString());
    m_opt->sOBMutex->unlock();

    // Copy settings from obmol -> xtal.
    // cell
    OBUnitCell *cell = static_cast<OBUnitCell*>(obmol.GetData(OBGenericDataType::UnitCell));

    if (cell == NULL) {
      m_opt->warning(tr("Optimizer::read: No unit cell in %1? Weird...").arg(filename));
      xtal->setStatus(Xtal::Error);
      return false;
    }

    xtal->setCellInfo(cell->GetCellMatrix());

    // atoms
    while (xtal->numAtoms() < obmol.NumAtoms())
      xtal->addAtom();
    QList<Atom*> atoms = xtal->atoms();
    uint i = 0;

    FOR_ATOMS_OF_MOL(atm, obmol) {
      atoms.at(i)->setPos(Vector3d(atm->x(), atm->y(), atm->z()));
      atoms.at(i)->setAtomicNumber(atm->GetAtomicNum());
      i++;
    }

    // energy/enthalpy
    const double KCAL_PER_MOL_TO_EV = 0.0433651224;
    if (obmol.HasData("Enthalpy (kcal/mol)"))
      xtal->setEnthalpy(QString(obmol.GetData("Enthalpy (kcal/mol)")->GetValue().c_str()).toFloat()
                        * KCAL_PER_MOL_TO_EV);
    if (obmol.HasData("Enthalpy PV term (kcal/mol)"))
      xtal->setPV(QString(obmol.GetData("Enthalpy PV term (kcal/mol)")->GetValue().c_str()).toFloat()
                  * KCAL_PER_MOL_TO_EV);
    xtal->setEnergy(obmol.GetEnergy());
    // Modify as needed!

    xtal->wrapAtomsToCell();
    xtal->findSpaceGroup();
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

#include "optimizer.moc"

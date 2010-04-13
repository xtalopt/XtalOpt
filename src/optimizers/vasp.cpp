/**********************************************************************
  XtalOptVASP - Tools to interface with VASP remotely

  Copyright (C) 2009 by David C. Lonie

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

#include "vasp.h"
#include "../templates.h"

#include <QProcess>
#include <QDir>
#include <QString>
#include <QDebug>

#include <avogadro/molecule.h>
#include <avogadro/atom.h>

#include <openbabel/obconversion.h>
#include <openbabel/mol.h>

using namespace std;
using namespace OpenBabel;
using namespace Eigen;

namespace Avogadro {

  bool XtalOptVASP::writeInputFiles(Xtal* xtal, XtalOpt *p) {
    // lock xtal
    QWriteLocker wlocker (xtal->lock());
    int optStep = xtal->getCurrentOptStep();
    if (optStep < 1 || optStep > p->VASP_INCAR_list.size()) {
      p->warning(tr("Error: Requested OptStep (%1) out of range (1-%2)")
                 .arg(optStep)
                 .arg(p->VASP_INCAR_list.size()));
      return false;
    }

    // Stop any running jobs associated with this xtal
    if (xtal->getJobID() != 0) {
      wlocker.unlock();
      deleteJob(xtal,p);
      wlocker.relock();
    }

    // Create local files
    QDir dir (xtal->fileName());
    if (!dir.exists()) {
      if (!dir.mkpath(xtal->fileName())) {
        p->warning(tr("Cannot write input files to specified path: %1 (path creation failure)", "1 is a file path.").arg(xtal->fileName()));
        return false;
      }
    }

    QFile in (xtal->fileName() + "/INCAR");
    QFile pos (xtal->fileName() + "/POSCAR");
    QFile pot (xtal->fileName() + "/POTCAR");
    QFile kp (xtal->fileName() + "/KPOINTS");
    QFile ls (xtal->fileName() + "/job.pbs");

    if (!in.open( QIODevice::WriteOnly | QIODevice::Text) ||
        !pos.open(QIODevice::WriteOnly | QIODevice::Text) ||
        !pot.open(QIODevice::WriteOnly | QIODevice::Text) ||
        !kp.open( QIODevice::WriteOnly | QIODevice::Text) ||
        !ls.open( QIODevice::WriteOnly | QIODevice::Text) ) {
      p->warning(tr("Cannot write input files to specified path: %1 (file writing failure)", "1 is a file path").arg(xtal->fileName()));
      return false;
    }

    QTextStream incar (&in);
    QTextStream poscar (&pos);
    QTextStream potcar (&pot);
    QTextStream kpoints (&kp);
    QTextStream launcher (&ls);

    int optStepInd = optStep - 1;

    incar  << XtalOptTemplate::interpretTemplate( p->VASP_INCAR_list.at(optStepInd), xtal, p );
    poscar << XtalOptTemplate::interpretTemplate( XtalOptTemplate::input_VASP_POSCAR(), xtal, p );
    potcar << p->VASP_POTCAR_list.at(optStepInd);
    kpoints<< XtalOptTemplate::interpretTemplate( p->VASP_KPOINTS_list.at(optStepInd), xtal, p );
    launcher<< XtalOptTemplate::interpretTemplate( p->VASP_qScript_list.at(optStepInd), xtal, p );

    in.close();
    pos.close();
    pot.close();
    kp.close();
    ls.close();

    // Switch to read lock
    wlocker.unlock();
    QReadLocker rlocker (xtal->lock());

    // Remote writing
    // Use a QProcess to scp the directory over, but first delete existing rempath
    // e.g.,
    // ssh -q <user>@<host> sh -c 'mkdir -p <rempath>|cat'
    // ssh -q <user>@<host> sh -c 'rm -rf <rempath>/*|cat'
    // scp -q <xtal->fileName()>/<FILE> <user>@<host>:<rempath>/..
    // ssh -q <user>@<host> <queueInsertProgram> <scriptname>
    QString command;

    // mkdir
    command = "ssh -q " + p->username + "@" + p->host + " sh -c \'mkdir -p " + xtal->getRempath() + "|cat\'";
    p->debug(command);
    if (QProcess::execute(command) != 0) {
      p->warning(tr("Error executing %1").arg(command));
      return false;
    }

    // rm -rf
    // the cat is neccessary on teragrid to get around the 'rm: No match' error when calling
    // * on an empty directory. Even though -f is supposed to ignore non-existant files...
    command = "ssh -q " + p->username + "@" + p->host + " sh -c \'rm -rf " + xtal->getRempath() + "/*|cat\'";
    p->debug(command);
    if (QProcess::execute(command) != 0) {
      p->warning(tr("Error executing %1").arg(command));
      return false;
    }

    // scp
    command = "scp -qr " + xtal->fileName() + " " + p->username + "@" + p->host + ":" + xtal->getRempath() + "/..";
    p->debug(command);
    if (QProcess::execute(command) != 0) {
      p->warning(tr("Error executing %1").arg(command));
      return false;
    }

    // scp
    command = "scp -q " + xtal->fileName() + "/INCAR " + p->username + "@" + p->host + ":" + xtal->getRempath() + "/INCAR";
    qDebug() << command;
    if (QProcess::execute(command) != 0) {
      qWarning() << tr("Error executing %1").arg(command);
      return false;
    }
    command = "scp -q " + xtal->fileName() + "/job.pbs " + p->username + "@" + p->host + ":" + xtal->getRempath() + "/job.pbs";
    qDebug() << command;
    if (QProcess::execute(command) != 0) {
      qWarning() << tr("Error executing %1").arg(command);
      return false;
    }
    command = "scp -q " + xtal->fileName() + "/POTCAR " + p->username + "@" + p->host + ":" + xtal->getRempath() + "/POTCAR";
    qDebug() << command;
    if (QProcess::execute(command) != 0) {
      qWarning() << tr("Error executing %1").arg(command);
      return false;
    }
    command = "scp -q " + xtal->fileName() + "/KPOINTS " + p->username + "@" + p->host + ":" + xtal->getRempath() + "/KPOINTS";
    qDebug() << command;
    if (QProcess::execute(command) != 0) {
      qWarning() << tr("Error executing %1").arg(command);
      return false;
    }
    command = "scp -q " + xtal->fileName() + "/POSCAR " + p->username + "@" + p->host + ":" + xtal->getRempath() + "/POSCAR";
    qDebug() << command;
    if (QProcess::execute(command) != 0) {
      qWarning() << tr("Error executing %1").arg(command);
      return false;
    }

    rlocker.unlock();
    wlocker.relock();
    xtal->setStatus(Xtal::WaitingForOptimization);
    return true;
  }

  bool XtalOptVASP::startOptimization(Xtal* xtal, XtalOpt *p) {
    // ssh
    QProcess proc;
    QString command = "ssh -q " + p->username + "@" + p->host + " " + p->launchCommand + " " + xtal->getRempath() + "/job.pbs";
    qDebug() << command;
    proc.start(command);
    proc.waitForFinished(-1);
    if (proc.exitCode() != 0) {
      qWarning() << QString("Error executing %1").arg(command)
                 << ":\n\t" << proc.readAllStandardError();
      return false;
    }

    // read data in
    proc.setReadChannel(QProcess::StandardOutput);

    // Assuming the return value is <jobID>.trailing.garbage.hostname.edu or similar
    uint jobID = (QString(proc.readLine()).split(".")[0]).toUInt();

    // lock for writing and update xtal
    QWriteLocker wlocker (xtal->lock());
    xtal->setJobID(jobID);
    xtal->startOptTimer();
    xtal->setStatus(Xtal::Submitted);
    p->updateInfo(xtal);
    return true;
  }

  bool XtalOptVASP::getOutputFile(const QString & path,
                                  const QString & filename,
                                  QStringList & data,
                                  const QString & host,
                                  const QString & username) {

    // First check to see if a remote file is being requested
    if (!username.isEmpty() && !host.isEmpty()) {
      QProcess proc;
      QString command;

      // ssh -q <user>@<host> cat <path>/<filename>
      command = "ssh -q " + username + "@" + host + " cat " + path + "/" + filename;
      qDebug() << command;
      proc.start(command);
      proc.waitForFinished(-1);
      if (proc.exitCode() != 0) {
        qWarning() << tr("Error executing %1").arg(command)
                   << ":\n\t" << proc.readAllStandardError();
        return false;
      }

      // Fill data list
      proc.setReadChannel(QProcess::StandardOutput);
      while (!proc.atEnd())
        data << proc.readLine();
      return true;
    }

    // Local file case:
    QFile file (path + "/" + filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      qWarning() << tr("Error opening file: %1").arg(file.fileName());
      return false;
    }
    while (!file.atEnd())
      data << file.readLine();
    return true;
  }

  bool XtalOptVASP::copyRemoteToLocalCache(Xtal *xtal,
                                           XtalOpt *p) {

    // lock xtal
    QReadLocker locker (xtal->lock());

    if (!p->using_remote) return false;
    QString command;

    command = "scp -r " + p->username + "@" + p->host + ":" + xtal->getRempath() + " " + xtal->fileName() + "/..";
    qDebug() << command;
    if (QProcess::execute(command) != 0) {
      qWarning() << tr("Error executing %1").arg(command);
      return false;
    }

    return true;
  }

  Optimizer::JobState XtalOptVASP::getStatus(Xtal* xtal,
                                              XtalOpt *p) {
    // lock xtal
    QWriteLocker locker (xtal->lock());
    QStringList queueData (p->queueData);
    QProcess proc;
    QString command;
    uint jobID = xtal->getJobID();

    // If jobID = 0, return an error.
    if (!jobID) {
      xtal->setStatus(Xtal::Error);
      return Optimizer::Error;
    }

    QString status;
    for (int i = 0; i < queueData.size(); i++) {
      if (queueData.at(i).split(".")[0] == QString::number(jobID)) {
        status = (queueData.at(i).split(QRegExp("\\s+")))[4];
        continue;
      }
    }

    // Checks if xtal is submitted:
    if (xtal->getStatus() == Xtal::Submitted) {
      if (status.isEmpty()) {
        return Optimizer::Pending;
      }
      else {
        return Optimizer::Started;
      }
    }


    if (status == "R") {
      if (xtal->getOptElapsed() == "0:00:00")
        xtal->startOptTimer();
      return Optimizer::Running;
    }
    else if (status == "Q")
      return Optimizer::Queued;
    // Even if the job has errored in the queue, leave it as "running" and wait for it to leave the queue
    // then check the OUTCAR. The optimization may have finished OK.
    else if (status == "E") {
      qWarning() << "Structure " << xtal->getIDString()
                 << " has errored in the queue, but may have optimized successfully.\n"
                 << "Marking job as 'Running' until it's gone from the queue...";
      return Optimizer::Running;
    }

    else { // Entry is missing from queue! Is there an OUTCAR?
      QStringList OUTCAR;
      bool OUTCARExists;
      QString rempath = xtal->getRempath();
      QString fileName = xtal->fileName();
      // unlock xtal while transferring data...
      locker.unlock();
      if (!rempath.isEmpty()) // Get the last 100 lines of a remote OUTCAR -- the line we need should be in there...
        OUTCARExists = getOutputFile(rempath, "OUTCAR|tail -n 100", OUTCAR, p->host, p->username);
      else
        OUTCARExists = getOUTCAR(fileName, OUTCAR);
      locker.relock();
      // Check for string:
      // "General timing and accounting informations for this job:"
      // in OUTCAR, which indicates success.
      qDebug() << "Job  " << jobID << " not in queue. Is there an outcar? " << OUTCARExists;
      if (OUTCARExists) {
        for (int i = 0; i < OUTCAR.size(); i++) {
          if (OUTCAR.at(i).contains("General timing and accounting informations for this job:")) {
            xtal->resetFailCount();
            return Optimizer::Success;
          }
        }
        // Otherwise, it's an error!
        qDebug() << "OUTCAR exists, but not finished!";
        xtal->setStatus(Xtal::Error);
        return Optimizer::Error;
      }
    }
    // Not in queue and no OUTCAR? Interesting...
    xtal->setStatus(Xtal::Error);
    return Optimizer::Unknown;
  }

  bool XtalOptVASP::deleteJob(Xtal *xtal, XtalOpt *p) {
    // lock xtal
    QWriteLocker locker (xtal->lock());

    QProcess proc;
    QString command;

    // If remote...
    if (p->using_remote) {
      // ssh -q <user>@<host> <queueDelete> <jobID>
      command = "ssh -q " + p->username + "@" + p->host + " " + p->queueDelete + " " + QString::number(xtal->getJobID());
    } else {
      command = p->queueCheck + " " + QString::number(xtal->getJobID());
    }

    // Execute
    qDebug() << command;
    proc.start(command);
    proc.waitForFinished(-1);
    if (proc.exitCode() != 0) {
      qWarning() << tr("Error executing %1").arg(command)
                 << ":\n\t" << proc.readAllStandardError();
      // Most likely job is already gone from queue. Set jobID to 0.
      xtal->setJobID(0);
      return false;
    }

    xtal->setJobID(0);
    xtal->stopOptTimer();
    p->updateInfo(xtal);
    return true;
  }

  bool XtalOptVASP::getQueueList(XtalOpt *p,
                                 QStringList & queueData) {

    QProcess proc;
    QString command;

    // If remote...
    if (p->using_remote) {
      // ssh -q <user>@<host> sh -c '<queueCheckProgram>|grep <username>|cat'
      // Somewhat ridiculous construct ensures that a non-zero status indicates a problem with ssh, i.e. communication error.
      command = "ssh -q " + p->username + "@" + p->host + " sh -c \'"
        + p->queueCheck + "|grep " + p->username + "|cat\'";
    } else {
      // Local...
      // <queueCheckProgram>
      // Trailing cat will override a non zero exit by grep due to no results
      command = p->queueCheck;
    }

    // Execute
    qDebug() << command;
    proc.start(command);
    proc.waitForFinished(-1);
    if (proc.exitCode() != 0) {
      qWarning() << tr("Error executing %1").arg(command)
                 << ":\n\t" << proc.readAllStandardError();
      return false;
    }

    // read data in
    proc.setReadChannel(QProcess::StandardOutput);

    queueData.clear();
    while (proc.canReadLine())
      queueData << proc.readLine();

    return true;
  }

  int XtalOptVASP::checkIfJobNameExists(Xtal* xtal, const QStringList & queueData, bool & exists) {
    xtal->lock()->lockForRead();
    QFile jobScript (xtal->fileName() + "/job.pbs");
    xtal->lock()->unlock();
    QString line, jobName;
    QStringList strl;
    exists = false;

    if (!jobScript.open(QIODevice::ReadOnly | QIODevice::Text)) {
      qWarning() << tr("Error opening file: %1").arg(jobScript.fileName());
      return 0;
    }
    while (!jobScript.atEnd()) {
      line = jobScript.readLine();
      if (line.contains("#PBS -N")) {
        jobName = line.split(QRegExp("\\s+"))[2];
        break;
      }
    }

    qDebug() << "Job name is " << jobName << " according to " << jobScript.fileName() << "...";

    for (int i = 0; i < queueData.size(); i++) {
      if (queueData.at(i).contains(jobName)) {
        // Set the bool to true, and return job ID.
        exists = true;
        int jobID = queueData.at(i).split(".").at(0).toInt();
        qDebug() << "Found it! ID = " << jobID;
        return jobID;
      }
    }

    return 0;
  }

  // Convenience functions
  bool XtalOptVASP::updateXtal(Xtal* xtal,
                               XtalOpt *p) {
    // lock xtal
    QWriteLocker locker (xtal->lock());

    // Update Xtal status
    xtal->setStatus(Xtal::Updating);
    xtal->stopOptTimer();

    // Copy remote files over

    if (p->using_remote) {
      locker.unlock();
      bool ok = copyRemoteToLocalCache(xtal, p);
      locker.relock();
      if (!ok) {
        qWarning() << QString("Error copying remote files to local dir\n\tremote: %1 on %2@%3\n\tlocal: %4")
          .arg(xtal->getRempath())
          .arg(p->username)
          .arg(p->host)
          .arg(xtal->fileName());
        xtal->setStatus(Xtal::Error);
        return false;
      }
    }


    if (!readXtal(xtal, p, xtal->fileName() + "/CONTCAR")) {
      if (!readXtal(xtal, p, xtal->fileName() + "/POSCAR")) {
        p->warning(tr("Error loading xtal at %1").arg(xtal->fileName()));
        return false;
      }
    }

    xtal->setJobID(0);
    xtal->setStatus(Xtal::StepOptimized);
    locker.unlock();
    p->updateInfo(xtal);
    return true;
  }

  bool XtalOptVASP::loadXtal(Xtal* xtal,
                               XtalOpt *p) {

    QWriteLocker locker (xtal->lock());

    if (!readXtal(xtal, p, xtal->fileName() + "/CONTCAR")) {
      if (!readXtal(xtal, p, xtal->fileName() + "/POSCAR")) {
        p->warning(tr("Error loading xtal at %1").arg(xtal->fileName()));
        return false;
      }
    }
    p->updateInfo(xtal);
    return true;
  }

  bool XtalOptVASP::readXtal(Xtal* xtal,
                               XtalOpt *p,
                               const QString & filename) {
    // Test filename
    QFile file (filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      qWarning() << "Cannot open file: " << file.fileName();
      xtal->setStatus(Xtal::Error);
      return false;
    }
    file.close();

    // Read in OBMol
    //
    // OpenBabel::OBConversion;:ReadFile calls a singleton error class
    // that is not thread safe. Stupid. Hence stupidOpenBabelMutex is
    // necessary.
    p->stupidOpenBabelMutex->lock();
    OBConversion conv;
    OBFormat* inFormat = conv.FormatFromExt(QString(QFile::encodeName(filename.trimmed())).toAscii());

    if ( !inFormat || !conv.SetInFormat( inFormat ) ) {
      qWarning() << "Error setting format for file " << filename;
      xtal->setStatus(Xtal::Error);
      p->stupidOpenBabelMutex->unlock();
      return false;
    }

    OBMol obmol;

    qDebug() << "Converting file " << QString(QFile::encodeName(filename));
    conv.ReadFile( &obmol, QString(QFile::encodeName(filename)).toStdString());
    p->stupidOpenBabelMutex->unlock();

    // Copy settings from obmol -> xtal.
    // cell
    OBUnitCell *cell = static_cast<OBUnitCell*>(obmol.GetData(OBGenericDataType::UnitCell));

    if (cell == NULL) {
      p->warning(tr("Error: No unit cell in %1? Weird...").arg(filename));
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
    if (obmol.HasData("Enthalpy (eV)"))
      xtal->setEnthalpy(QString(obmol.GetData("Enthalpy (eV)")->GetValue().c_str()).toFloat());
    if (obmol.HasData("Enthalpy PV term (eV)"))
      xtal->setPV(QString(obmol.GetData("Enthalpy PV term (eV)")->GetValue().c_str()).toFloat());
    xtal->setEnergy(obmol.GetEnergy());
    // Modify as needed!

    xtal->wrapAtomsToCell();
    xtal->findSpaceGroup();
    return true;
  }

  int XtalOptVASP::totalOptSteps(XtalOpt *p) {
    return p->VASP_INCAR_list.size();
  }

} // end namespace Avogadro

#include "vasp.moc"

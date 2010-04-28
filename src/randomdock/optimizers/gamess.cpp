/**********************************************************************
  RandomDockGAMESS - Tools to interface with GAMESS remotely

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

#include "randomdockGAMESS.h"
#include "randomdock.h"
#include "templates.h"

#include <QProcess>
#include <QDir>
#include <QString>
#include <QDebug>

#include <avogadro/moleculefile.h>

#include <openbabel/obconversion.h>
#include <openbabel/mol.h>

using namespace std;
using namespace OpenBabel;
using namespace Eigen;

namespace Avogadro {

  bool RandomDockGAMESS::writeInputFiles(Scene *scene, RandomDockParams *p) {
        // Create local files
    QDir qdir;
    if (!qdir.mkpath(scene->fileName())) {
      qWarning() << tr("Cannot write input files to specified path: %1 (path creation failure)", "1 is a file path.").arg(scene->fileName());
      return false;
    }

    QFile in (scene->fileName() + "/job.inp");
    QFile ls (scene->fileName() + "/job.pbs");

    if (!in.open( QIODevice::WriteOnly | QIODevice::Text) ||
        !ls.open( QIODevice::WriteOnly | QIODevice::Text) ) {
      qWarning() << tr("Cannot write input files to specified path: %1 (file writing failure)", "1 is a file path").arg(scene->fileName());
      return false;
    }

    QTextStream input (&in);
    QTextStream launcher (&ls);

    input  	<< Templates::interpretTemplate( p->GAMESSdockingOpt, scene );
    launcher	<< Templates::interpretTemplate( p->GAMESSqueueScript, scene );

    in.close();
    ls.close();
    return true;
  }

  bool RandomDockGAMESS::readOutputFiles(Scene *scene, RandomDockParams *p) {
    qDebug() << "RandomDockGAMESS::readOutputFiles( " << scene << ", " << p << " ) called";
    QWriteLocker locker (p->rwLock);

    // Update Scene status
    scene->setStatus(Scene::Updating);

    // Test filename
    QFile file (scene->fileName() + "/job.gamout");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      qWarning() << "Cannot open file: " << file.fileName();
      return false;
    }

    file.close();

    // Read in scene
    qDebug() << "Updating scene " << scene->getSceneNumber() << " from " << scene->fileName() + "/job.gamout";
    Molecule *mol = MoleculeFile::readMolecule(scene->fileName() + "/job.gamout", "", "b");
    scene->updateFromMolecule(mol);

    scene->setStatus(Scene::Optimized);
    return true;
  }

  bool RandomDockGAMESS::stripOutputFile(Scene *scene, RandomDockParams *p) {
    qDebug() << "RandomDockGAMESS::stripOutputFiles( " << scene << ", " << p << " ) called";
    QWriteLocker locker (p->rwLock);

    // Open file
    QFile oldfile (scene->fileName() + "/job.gamout");
    if (!oldfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
      qWarning() << "Cannot open file for reading: " << oldfile.fileName();
      return false;
    }

    QStringList newOut;
    QString line;
    QTextStream s (&oldfile);
    while (!s.atEnd()) {
      line = s.readLine();

      // Coordinate data
      if (line.contains("ATOMIC                      COORDINATES (BOHR)")) {
          newOut << "\n" + line;
          newOut << s.readLine(); // Headings
          while (!s.atEnd()) {
            line = s.readLine();
            if (line.split(QRegExp("\\s+"), QString::SkipEmptyParts).size() != 5) {
              break;
            }
            else
              newOut << line;
          }
      }
        
      // Energy data
      if (line.contains("TOTAL ENERGY      ="))
        newOut << "\n" + line + "\n";
    }

    // Overwrite old contents of file with new
    oldfile.remove();
    if (!oldfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
      qWarning() << "Cannot open file for writing: " << oldfile.fileName();
      return false;
    }

    oldfile.write(newOut.join("\n").toStdString().c_str());

    oldfile.close();

    return true;
  }
  
  bool RandomDockGAMESS::copyToRemote(Scene *scene, RandomDockParams *p) {
    // Remote writing
    // Use a QProcess to scp the directory over, but first delete existing rempath
    // e.g.,
    // ssh -q <user>@<host> mkdir -p <rempath>
    // ssh -q <user>@<host> sh -c 'rm -rf <rempath>/*|cat'
    // scp -qr <scene->fileName()> <user>@<host>:<rempath>/..
    QString command;

    // mkdir
    command = "ssh -q " + p->username + "@" + p->host + " mkdir -p " + scene->getRempath();
    qDebug() << command;
    if (QProcess::execute(command) != 0) {
      qWarning() << tr("Error executing %1").arg(command);
      return false;
    }

    // rm -rf
    // the cat is neccessary on teragrid to get around the 'rm: No match' error when calling
    // * on an empty directory. Even though -f is supposed to ignore non-existant files...
    command = "ssh -q " + p->username + "@" + p->host + " sh -c \'rm -rf " + scene->getRempath() + "/*|cat\'";
    qDebug() << command;
    if (QProcess::execute(command) != 0) {
      qWarning() << tr("Error executing %1").arg(command);
      return false;
    }

    // scp
    command = "scp -qr " + scene->fileName() + " " + p->username + "@" + p->host + ":" + scene->getRempath() + "/..";
    qDebug() << command;
    if (QProcess::execute(command) != 0) {
      qWarning() << tr("Error executing %1").arg(command);
      return false;

    }
    return true;
  }

  bool RandomDockGAMESS::copyFromRemote(Scene *scene, RandomDockParams *p) {
    QString command;

    // scp -r <username>@<host>:/<rempath>/job.gamout <filename>
    command = "scp -r " + p->username + "@" + p->host + ":" + scene->getRempath() + "/job.gamout " + scene->fileName();
    qDebug() << command;
    if (QProcess::execute(command) != 0) {
      qWarning() << tr("Error executing %1").arg(command);
      return false;
    }

    // rm -rf the remote path -- don't need it anymore!
    // the cat is neccessary on teragrid to get around the 'rm: No match' error when calling
    // * on an empty directory. Even though -f is supposed to ignore non-existant files...
    command = "ssh -q " + p->username + "@" + p->host + " sh -c \'rm -rf " + scene->getRempath() + "/*|cat\'";
    qDebug() << command;
    if (QProcess::execute(command) != 0) {
      qWarning() << tr("Error executing %1").arg(command);
      return false;
    }

    return true;
  }

  bool RandomDockGAMESS::getQueue(RandomDockParams *p, QStringList & queueData) {
    QProcess proc;
    QString command;

    // ssh -q <user>@<host> sh -c '<queueCheckProgram>|grep <username>|cat'
    // Somewhat ridiculous construct ensures that a non-zero status indicates a problem with ssh, i.e. communication error.
    command = "ssh -q " + p->username + "@" + p->host + " sh -c \'" 
      + p->queueCheck + "|grep " + p->username + "|cat\'";

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

    while (proc.canReadLine()) 
      queueData << proc.readLine();

    return true;
  }


  int RandomDockGAMESS::getStatus(Scene *scene, RandomDockParams *p, const QStringList & queueData){
    QProcess proc;
    QString command;
    uint jobID = scene->getJobID();

    QString status;
    for (int i = 0; i < queueData.size(); i++) {
      if (queueData.at(i).split(".")[0] == QString::number(jobID)) {
        status = (queueData.at(i).split(QRegExp("\\s+")))[4];
        continue;
      }
    }

    if (status == "R") {
      return Running;
    }
    else if (status == "Q")
      return Queued;
    // Even if the job has errored in the queue, leave it as "running" and wait for it to leave the queue
    // then check the OUTCAR. The optimization may have finished OK.
    else if (status == "E") {
      qWarning() << "Job " << jobID
                 << " has errored in the queue, but may have optimized successfully.\n"
                 << "Marking job as 'Running' until it's gone from the queue...";
      return Running;
    }

    else { // Entry is missing from queue! Check the job.gamout.
      QStringList output;
      copyFromRemote(scene, p);
      QFile file (scene->fileName() + "/job.gamout");

      if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << tr("Error opening file: %1").arg(file.fileName());
        return Unknown;
      }
      while (!file.atEnd())
        output << file.readLine();

      // Check for string:
      // "***** EQUILIBRIUM GEOMETRY LOCATED *****"
      // in output, which indicates success.
      qDebug() << "Job  " << jobID << " not in queue..";
      for (int i = output.size() - 1; i >= 0; i--) {
        if (output.at(i).contains("***** EQUILIBRIUM GEOMETRY LOCATED *****"))
          return Success;
      }
      // Otherwise, it's an error!
      qDebug() << "Calculation ended without converging!";
      return Error;
    }
  }

  bool RandomDockGAMESS::startJob(Scene *scene, RandomDockParams *p) {
    QProcess proc;
    QString command;
    // rm old job.gamout and job.dat (prevent erroneous optimization reading)
    // the cat is neccessary on teragrid to get around the 'rm: No match' error when calling
    // * on an empty directory. Even though -f is supposed to ignore non-existant files...
    command = "ssh -q " + p->username + "@" + p->host + " sh -c \'rm -f " + scene->getRempath() + "/job.gamout|cat\'";
    qDebug() << command;
    if (QProcess::execute(command) != 0) {
      qWarning() << tr("Error executing %1").arg(command);
      return false;
    }

    command = "ssh -q " + p->username + "@" + p->host + " sh -c \'rm -f " + scene->getRempath() + "/job.dat|cat\'";
    qDebug() << command;
    if (QProcess::execute(command) != 0) {
      qWarning() << tr("Error executing %1").arg(command);
      return false;
    }

    // ssh -q <user>@<host> <queueInsertProgram> <args> <scriptname>
    command = "ssh -q " + p->username + "@" + p->host + " " + p->launchCommand + " " + scene->getRempath() + "/job.pbs";
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
    qDebug() << "JOBID = " << jobID;
    scene->setJobID(jobID);
    scene->setStatus(Scene::InProcess);
    return true;
  }

  bool RandomDockGAMESS::deleteJob(Scene *scene, RandomDockParams *p) {
    QWriteLocker locker (p->rwLock);
    QProcess proc;
    QString command;

    // ssh -q <user>@<host> <queueDelete> <jobID>
    command = "ssh -q " + p->username + "@" + p->host + " " + p->queueDelete + " " + QString::number(scene->getJobID());

    // Execute
    qDebug() << command;
    proc.start(command);
    proc.waitForFinished(-1);
    if (proc.exitCode() != 0) {
      qWarning() << tr("Error executing %1").arg(command)
                 << ":\n\t" << proc.readAllStandardError();
      // Most likely job is already gone from queue. Set jobID to 0.
      scene->setJobID(0);
      return false;
    }
    
    scene->setJobID(0);
    return true;
}

} // end namespace Avogadro

#include "randomdockGAMESS.moc"

/**********************************************************************
  XtalOptGULP - Tools to interface with GULP

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

#include "gulp.h"
#include "../templates.h"
#include "../xtal.h"

#include <QProcess>
#include <QDir>
#include <QString>
#include <QSemaphore>

#include <openbabel/obconversion.h>
#include <openbabel/mol.h>

using namespace std;
using namespace OpenBabel;
using namespace Eigen;

namespace Avogadro {

  bool XtalOptGULP::writeInputFiles(Xtal *xtal, XtalOpt *p) {
    //qDebug() << "XtalOptGULP::writeInputFiles()";

    // lock xtal
    QWriteLocker wlocker (xtal->lock());
    int optStep = xtal->getCurrentOptStep();
    if (optStep < 1 || optStep > totalOptSteps(p)) {
      p->warning(tr("Error: Requested OptStep (%1) out of range (1-%2)")
                 .arg(optStep)
                 .arg(totalOptSteps(p)));
      return false;
    }

    // Create local files
    QDir dir (xtal->fileName());
    if (!dir.exists()) {
      if (!dir.mkpath(xtal->fileName())) {
        p->warning(tr("Cannot write input files to specified path: %1 (path creation failure)", "1 is a file path.").arg(xtal->fileName()));
        return false;
      }
    }

    QFile gin (xtal->fileName() + "/xtal.gin");
    QFile ls (xtal->fileName() + "/xtal.sh");

    if (!gin.open( QIODevice::WriteOnly | QIODevice::Text) ||
        !ls.open( QIODevice::WriteOnly | QIODevice::Text)) {
      p->warning(tr("Cannot write input files to specified path: %1 (file writing failure)", "1 is a file path").arg(xtal->fileName()));
      return false;
    }

    QTextStream gin_s (&gin);
    QTextStream ls_s (&ls);

    int optStepInd = optStep - 1;

    gin_s	<< XtalOptTemplate::interpretTemplate( p->GULP_gin_list.at(optStepInd), xtal, p );
    ls_s	<< "#!/bin/bash\n"
                << "cd " << xtal->fileName() << endl
                << "gulp < xtal.gin > xtal.got" << endl
                << "exit 0" << endl;
    gin.close();
    ls.close();

    xtal->setJobID(0);
    xtal->setStatus(Xtal::WaitingForOptimization);
    return true;
  }

  bool XtalOptGULP::startOptimization(Xtal *xtal, XtalOpt *p) {
    //qDebug() << "XtalOptGULP::startOptimization()";

    QString command = "bash " + xtal->fileName() + "/xtal.sh";
    //p->debug(command);

    xtal->setStatus(Xtal::InProcess);
    p->updateInfo(xtal);
    xtal->startOptTimer();

    int exitStatus = QProcess::execute(command);

    // lock xtal
    QWriteLocker wlocker (xtal->lock());

    xtal->stopOptTimer();

    if (exitStatus != 0) {
      p->warning(tr("XtalOptGULP::startOptimization: Error running command:\n\t%1").arg(command));
      xtal->setStatus(Xtal::Error);
      p->updateInfo(xtal);
      return false;
    }

    // Was the run sucessful?
    QFile file (xtal->fileName() + "/xtal.got");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      p->warning(tr("XtalOptGULP::getStatus: Error opening file: %1").arg(file.fileName()));
      xtal->setStatus(Xtal::Error);
      p->updateInfo(xtal);
      return false;
    }
    QString line;
    while (!file.atEnd()) {
      line = file.readLine();
      if (line.contains("**** Optimisation achieved ****")) {
        xtal->resetFailCount();
        wlocker.unlock();
        p->updateInfo(xtal);
        return updateXtal(xtal,p);
      }
      if (line.contains("**** unless gradient norm is small (less than 0.1)             ****")) {
        for (int i = 0; i < 4; i++) line = file.readLine();
        double gnorm = (line.split(QRegExp("\\s+"))[4]).toFloat();
        qDebug() << "Checking gnorm: " << gnorm;
        if (gnorm <= 0.1) {
          xtal->resetFailCount();
          wlocker.unlock();
          p->updateInfo(xtal);
          return updateXtal(xtal,p);
        }
        else break;
      }
    }

    return false;
  }

  bool XtalOptGULP::deleteJob(Xtal* xtal, XtalOpt *p) {
    //qDebug() << "XtalOptGULP::deleteJob()";
    Q_UNUSED(p);
    xtal->lock()->lockForWrite();
    xtal->setJobID(0);
    xtal->lock()->unlock();
    return true;
  }

  Optimizer::JobState XtalOptGULP::getStatus(Xtal* xtal,
                                              XtalOpt *p) {
    //qDebug() << "XtalOptGULP::getStatus()";
    Q_UNUSED(p);
    QReadLocker rlocker (xtal->lock());
    if (xtal->getStatus() == Xtal::InProcess) {
      return Optimizer::Running;
    }
    else
      return Optimizer::Unknown;
  }

  bool XtalOptGULP::getQueueList(XtalOpt *p,
                                 QStringList & queueData) {
    //qDebug() << "XtalOptGULP::getQueueList()";
    Q_UNUSED(p);
    Q_UNUSED(queueData);
    // Not really used here!
    return true;
  }

  int XtalOptGULP::checkIfJobNameExists(Xtal* xtal, const QStringList & queueData, bool & exists) {
    //qDebug() << "XtalOptGULP::checkIfJobNameExists()";
    Q_UNUSED(xtal);
    Q_UNUSED(queueData);
    // Again -- does nothing!
    exists = false;
    return 0;
  }

  // Convenience functions
  bool XtalOptGULP::updateXtal(Xtal* xtal,
                               XtalOpt *p) {
    //qDebug() << "XtalOptGULP::updateXtal()";

    // lock xtal
    QWriteLocker locker (xtal->lock());

    // Update Xtal status
    xtal->setStatus(Xtal::Updating);
    xtal->stopOptTimer();

    QString fullFilename = xtal->fileName() + "/xtal.got";

    if (!readXtal(xtal, p, fullFilename)) {
      p->warning(tr("XtalOptGULP::updateXtal: Error updating xtal from %1").arg(fullFilename));
      return false;
    }

    xtal->setStatus(Xtal::StepOptimized);
    p->updateInfo(xtal);
    return true;
  }

  bool XtalOptGULP::loadXtal(Xtal* xtal,
                             XtalOpt *p) {
    //qDebug() << "XtalOptGULP::loadXtal()";
    QWriteLocker locker (xtal->lock());

    if (!readXtal(xtal, p, xtal->fileName() + "/xtal.got")) {
      p->warning(tr("XtalOptGULP::loadXtal: Error loading xtal from %1").arg(xtal->fileName()));
      return false;
    }

    return true;
  }

  bool XtalOptGULP::readXtal(Xtal* xtal,
                               XtalOpt *p,
                               const QString & filename) {
    //qDebug() << "XtalOptGULP::readXtal()";
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

  int XtalOptGULP::totalOptSteps(XtalOpt *p) {
    return p->GULP_gin_list.size();
  }

} // end namespace Avogadro

#include "gulp.moc"

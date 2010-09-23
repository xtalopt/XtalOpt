/**********************************************************************
  GULPOptimizer - Tools to interface with GULP

  Copyright (C) 2009-2010 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <xtalopt/optimizers/gulp.h>
#include <xtalopt/structures/xtal.h>

#include <QtCore/QProcess>
#include <QtCore/QDir>
#include <QtCore/QString>
#include <QtCore/QSemaphore>

#include <openbabel/obconversion.h>
#include <openbabel/mol.h>

namespace XtalOpt {

  GULPOptimizer::GULPOptimizer(OptBase *parent, const QString &filename) :
    XtalOptOptimizer(parent)
  {
    // Set allowed data structure keys, if any, e.g.
    // None here!

    // Set allowed filenames, e.g.
    m_templates.insert("xtal.gin",QStringList(""));

    // Setup for completion values
    m_completionFilename = "xtal.got";
    m_completionString   = "Not used!";

    // Set output filenames to try to read data from, e.g.
    m_outputFilenames.append(m_completionFilename);

    // Set the name of the optimizer to be returned by getIDString()
    m_idString = "GULP";

    readSettings(filename);
  }

  bool GULPOptimizer::writeInputFiles(Structure *structure) {
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
    // Write all explicit templates
    if (!writeTemplates(structure)) return false;

    // No copying to server -- GULP is local only for now.

    // Update info
    locker.unlock();
    structure->lock()->lockForWrite();
    structure->setStatus(Structure::WaitingForOptimization);
    structure->lock()->unlock();
    return true;
  }

  bool GULPOptimizer::startOptimization(Structure *structure) {
    QString command = "cd " + structure->fileName() + " && "
      + " gulp < xtal.gin > xtal.got";


    structure->setStatus(Xtal::InProcess);
    structure->startOptTimer();

    int exitStatus = QProcess::execute(command);

    // lock xtal
    QWriteLocker wlocker (structure->lock());

    structure->stopOptTimer();

    if (exitStatus != 0) {
      m_opt->warning(tr("XtalOptGULP::startOptimization: Error running command:\n\t%1").arg(command));
      structure->setStatus(Xtal::Error);
      return false;
    }

    // Was the run sucessful?
    QFile file (structure->fileName() + "/xtal.got");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      m_opt->warning(tr("XtalOptGULP::getStatus: Error opening file: %1").arg(file.fileName()));
      structure->setStatus(Xtal::Error);
      return false;
    }
    QString line;
    while (!file.atEnd()) {
      line = file.readLine();
      if (line.contains("**** Optimisation achieved ****")) {
        structure->resetFailCount();
        wlocker.unlock();
        return update(structure);
      }
      if (line.contains("**** unless gradient norm is small (less than 0.1)             ****")) {
        for (int i = 0; i < 4; i++) line = file.readLine();
        double gnorm = (line.split(QRegExp("\\s+"))[4]).toFloat();
        qDebug() << "Checking gnorm: " << gnorm;
        if (gnorm <= 0.1) {
          structure->resetFailCount();
          wlocker.unlock();
          return update(structure);
        }
        else break;
      }
    }

    return false;
  }

  Optimizer::JobState GULPOptimizer::getStatus(Structure *structure)
  {
    QReadLocker rlocker (structure->lock());
    if (structure->getStatus() == Xtal::InProcess) {
      return Optimizer::Running;
    }
    else {
      return Optimizer::Unknown;
    }
  }

  bool GULPOptimizer::getQueueList(QStringList & queueData, QMutex *mutex) {
    Q_UNUSED(queueData);
    return true;
  }

  bool GULPOptimizer::copyRemoteToLocalCache(Structure *structure)
  {
    return true;
  }
} // end namespace Avogadro

//#include "gulp.moc"

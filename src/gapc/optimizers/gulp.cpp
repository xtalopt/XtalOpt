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

#include <gapc/optimizers/gulp.h>
#include <gapc/gapc.h>

#include <globalsearch/structure.h>

#include <openbabel/obconversion.h>
#include <openbabel/mol.h>

#include <QtCore/QReadLocker>
#include <QtCore/QProcess>
#include <QtCore/QString>
#include <QtCore/QDir>

namespace GAPC {

  GULPOptimizer::GULPOptimizer(OptBase *parent, const QString &filename) :
    Optimizer(parent)
  {
    // Set allowed data structure keys, if any, e.g.
    // None here!

    // Set allowed filenames, e.g.
    m_templates.insert("cluster.gin",QStringList(""));

    // Setup for completion values
    m_completionFilename = "cluster.got";
    m_completionString   = "Not used!";

    // Set output filenames to try to read data from, e.g.
    m_outputFilenames.append(m_completionFilename);

    // Set the name of the optimizer to be returned by getIDString()
    m_idString = "GULP";

    readSettings(filename);
  }

  bool GULPOptimizer::writeInputFiles(Structure *structure) {
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
    QString command = "\"" + qobject_cast<OptGAPC*>(m_opt)->gulpPath
      + "\"";

#ifdef WIN32
    command = "cmd.exe /C " + command;
#endif // WIN32

    QProcess proc;
    proc.setWorkingDirectory(structure->fileName());
    proc.setStandardInputFile(structure->fileName() + "/cluster.gin");
    proc.setStandardOutputFile(structure->fileName() + "/cluster.got");
    proc.setStandardErrorFile(structure->fileName() + "/cluster.err");

    structure->setStatus(Structure::InProcess);
    structure->startOptTimer();

    proc.start(command);
    proc.waitForFinished(-1);

    int exitStatus = proc.exitCode();

    // lock structure
    QWriteLocker wlocker (structure->lock());

    structure->stopOptTimer();

    if (exitStatus != 0) {
      m_opt->warning(tr("GULPOptimizer::startOptimization: Error running command:\n\t%1").arg(command));
      structure->setStatus(Structure::Error);
      return false;
    }

    // Was the run sucessful?
    QFile file (structure->fileName() + "/cluster.got");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      m_opt->warning(tr("GULPOptimizer::getStatus: Error opening file: %1").arg(file.fileName()));
      structure->setStatus(Structure::Error);
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
    if (structure->getStatus() == Structure::InProcess) {
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
} // end namespace GAPC


/**********************************************************************
  VASPOptimizer - Tools to interface with VASP

  Copyright (C) 2009-2010 by David C. Lonie

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
#include "../../generic/macros.h"
#include "../../generic/xtal.h"

#include <QDir>
#include <QDebug>
#include <QString>
#include <QProcess>
#include <QSettings>

#include <avogadro/molecule.h>
#include <avogadro/atom.h>

#include <openbabel/obconversion.h>
#include <openbabel/mol.h>

using namespace std;
using namespace OpenBabel;
using namespace Eigen;

namespace Avogadro {

  VASPOptimizer::VASPOptimizer(OptBase *parent) :
    XtalOptOptimizer(parent)
  {
    // Set allowed data structure keys, if any
    // "POTCAR info" is of type
    // QList<QHash<QString, QString> >
    // e.g. a list of hashes containing 
    // [atomic symbol : pseudopotential file] pairs
    m_data.insert("POTCAR info",QVariant());
    m_data.insert("Composition",QVariant());

    // Set allowed filenames, e.g.
    m_templates.insert("INCAR",QStringList(""));
    m_templates.insert("POTCAR",QStringList(""));
    m_templates.insert("KPOINTS",QStringList(""));
    m_templates.insert("job.pbs",QStringList(""));

    // Setup for completion values
    m_completionFilename = "OUTCAR";
    m_completionString   = "General timing and accounting informations for this job:";

    // Set output filenames to try to read data from, e.g.
    m_outputFilenames.append("CONTCAR");
    m_outputFilenames.append("POSCAR");

    // Set the name of the optimizer to be returned by getIDString()
    m_idString = "VASP";

    readSettings();

    buildPOTCARs();

  }

  void VASPOptimizer::readSettings(const QString &filename)
  {
    SETTINGS(filename);

    readTemplatesFromSettings(filename);
    readUserValuesFromSettings(filename);
    readDataFromSettings(filename);

    buildPOTCARs();
  }

  void VASPOptimizer::writeTemplatesToSettings(const QString &filename)
  {
    SETTINGS(filename);
    QStringList filenames = getTemplateNames();
    for (int i = 0; i < filenames.size(); i++) {
      // Don't bother saving the actual POTCAR files
      if (filenames.at(i) == "POTCAR") continue;
      settings->setValue("xtalopt/optimizer/" + 
                    getIDString() + "/" +
                    filenames.at(i) + "_list",
                    m_templates.value(filenames.at(i)));
    }
  }

  void VASPOptimizer::writeDataToSettings(const QString &filename)
  {
    // We only want to save POTCAR info and Composition to the resume
    // file, not the main config file, so only dump the data here if
    // we are given a filename and it contains the string
    // "xtalopt.state"
    if (filename.isEmpty() || !filename.contains("xtalopt.state")) {
      return;
    }
    SETTINGS(filename);
    QStringList ids = getDataIdentifiers();
    for (int i = 0; i < ids.size(); i++) {
      settings->setValue("xtalopt/optimizer/" + 
                         getIDString() + "/data/" +
                         ids.at(i),
                         m_data.value(ids.at(i)));
    }
  }

  bool VASPOptimizer::writeInputFiles(Structure *structure) {
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
    // POSCAR is slightly different, must be done here.
    QFile pos (structure->fileName() + "/POSCAR");
    if (!pos.open(QIODevice::WriteOnly | QIODevice::Text)) {
      m_opt->warning(tr("Cannot write input files to specified path: %1 (file writing failure)", "1 is a file path").arg(structure->fileName()));
      return false;
    }
    int optStepInd = optStep - 1;
    QTextStream poscar (&pos);
    poscar << m_opt->interpretTemplate( "%POSCAR%", structure);
    pos.close();

    // Copy to server
    if (!copyLocalTemplateFilesToRemote(structure)) return false;
    // Again, POS is done separatel
    QString command = "scp -q " + structure->fileName() + "/POSCAR " + m_opt->username + "@" + m_opt->host + ":" + structure->getRempath() + "/POSCAR";
    qDebug() << command;
    if (QProcess::execute(command) != 0) {
      qWarning() << tr("Error executing %1").arg(command);
      return false;
    }

    // Update info
    locker.unlock();
    structure->lock()->lockForWrite();
    structure->setStatus(Structure::WaitingForOptimization);
    structure->lock()->unlock();
    return true;
  }

  bool VASPOptimizer::POTCARInfoIsUpToDate(QList<uint> atomicNums)
  {
    // Get optimizer's composition
    QList<uint> oldcomp;
    QList<QVariant> oldcomp_ = getData("Composition").toList();
    for (int i = 0; i < oldcomp_.size(); i++)
      oldcomp.append(oldcomp_.at(i).toUInt());
    // Sort the composition
    qSort(atomicNums);
    qSort(oldcomp);
    if (getData("POTCAR info").toList().size() != getNumberOfOptSteps() ||
        oldcomp != atomicNums
        ) {
      return false;
    }
    return true;
  }

  void VASPOptimizer::buildPOTCARs() {
    double enmax = 0;
    m_templates["POTCAR"].clear();
    // "POTCAR info" is of type
    // QList<QHash<QString, QString> >
    // e.g. a list of hashes containing 
    // [atomic symbol : pseudopotential file] pairs
    QVariantList potcarInfo = m_data["POTCAR info"].toList();
    for (int optIndex = 0;
         optIndex < potcarInfo.size(); optIndex++) {
      QFile file;
      double tmp_enmax;
      QString line, str, POTCAR;
      QStringList tmp_sl;
      QTextStream out (&POTCAR), in;
      QList<QString> symbols = potcarInfo.at(optIndex).toHash().keys();
      qSort(symbols);
      // Make a loop over the alphabetically sorted symbols:
      for (int i = 0; i < symbols.size(); i++) {
        file.setFileName(potcarInfo.at(optIndex).toHash().value(symbols.at(i)).toString());
        file.open(QIODevice::ReadOnly);
        in.setDevice(&file);
        while (!in.atEnd()) {
          line = in.readLine();
          out << line + "\n";
          if (line.contains("ENMAX")) {
            tmp_sl = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);
            str = tmp_sl.at(2);
            str.remove(";");
            tmp_enmax = str.toFloat();
            if (tmp_enmax > enmax) enmax = tmp_enmax;
          }
        }
        file.close();
      }
      appendTemplate("POTCAR", POTCAR);
    }
  }

} // end namespace Avogadro

//#include "vasp.moc"

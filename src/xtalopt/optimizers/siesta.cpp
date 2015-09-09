/**********************************************************************
  SIESTAOptimizer - Tools to interface with SIESTA

  Copyright (C) 2009-2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <xtalopt/optimizers/siesta.h>
#include <xtalopt/structures/xtal.h>
#include <xtalopt/xtalopt.h>


#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QString>
#include <QtCore/QSettings>

#include <openbabel/obconversion.h>
#include <openbabel/mol.h>

using namespace GlobalSearch;

namespace XtalOpt {

  SIESTAOptimizer::SIESTAOptimizer(OptBase *parent, const QString &filename) :
    XtalOptOptimizer(parent)
  {
    // Set allowed data structure keys, if any
    m_data.insert("PSF info",QVariant());
    m_data.insert("Composition",QVariant());

    // Set allowed filenames, e.g.
    m_templates.insert("xtal.fdf",QStringList(""));

    // Setup for completion values
    m_completionFilename = "xtal.out";
    m_completionStrings.append("siesta: Final energy (eV):");

    // Set output filenames to try to read data from, e.g.
    m_outputFilenames.append("BASIS_ENTHALPY");
    m_outputFilenames.append(m_completionFilename);

    // Set the name of the optimizer to be returned by getIDString()
    m_idString = "SIESTA";

    // Local execution setup:
    m_localRunCommand = "siesta";
    m_stdinFilename = "xtal.fdf";
    m_stdoutFilename = "xtal.out";
    m_stderrFilename = "";

    readSettings(filename);
  }

  QHash<QString, QString>
  SIESTAOptimizer::getInterpretedTemplates(Structure *structure)
  {
    QHash<QString, QString> hash = Optimizer::getInterpretedTemplates(structure);
    QVariantList psfInfo = m_data["PSF info"].toList();
    QList<QString> symbols = psfInfo.at(0).toHash().keys();
    qSort(symbols);
    // Make a loop over the alphabetically sorted symbols:
    for (int i = 0; i < symbols.size(); i++) {
      QString PSF = ".psf";
      PSF.prepend(symbols.at(i));
      QFile file;
      QString line, str;
      QString psf = PSF;
      QTextStream in;
      file.setFileName(psfInfo.at(0).toHash().value(symbols.at(i)).toString());
      file.open(QIODevice::ReadOnly);
      QTextStream out(&PSF);
      //out.setDevice(&PSF);
      in.setDevice(&file);
      while (!in.atEnd()) {
        line = in.readLine();
        out << line + "\n";
      }
      file.close();
      hash.insert(psf,PSF);
    }
    hash.remove("xtal.psf");
    //buildPSFs();
    return hash;
  }

  bool SIESTAOptimizer::PSFInfoIsUpToDate(QList<uint> atomicNums)
  {
    // Get optimizer's composition
    QList<uint> oldcomp;
    QList<QVariant> oldcomp_ = getData("Composition").toList();
    for (int i = 0; i < oldcomp_.size(); i++)
      oldcomp.append(oldcomp_.at(i).toUInt());
    // Sort the composition
    qSort(atomicNums);
    qSort(oldcomp);
    if (getData("PSF info").toList().size() != getNumberOfOptSteps() ||
        oldcomp != atomicNums
        ) {
      return false;
    }
    return true;
  }

  void SIESTAOptimizer::buildPSFs() {
    double enmax = 0;
    m_templates["xtal.psf"].clear();
    // "PSF info" is of type
    // QList<QHash<QString, QString> >
    // e.g. a list of hashes containing
    // [atomic symbol : pseudopotential file] pairs
    QVariantList psfInfo = m_data["PSF info"].toList();
    for (int optIndex = 0;
         optIndex < psfInfo.size(); optIndex++) {
      QFile file;
      QString line, str, PSF;
      QStringList tmp_sl;
      QTextStream out (&PSF), in;
      QList<QString> symbols = psfInfo.at(optIndex).toHash().keys();
      qSort(symbols);
      // Make a loop over the alphabetically sorted symbols:
      for (int i = 0; i < symbols.size(); i++) {
//        QString PSF = ".psf"
//        PSF.prepend(symbols.at(i).toString());
        file.setFileName(psfInfo.at(optIndex).toHash().value(symbols.at(i)).toString());
        file.open(QIODevice::ReadOnly);
//        out.setDevice(&PSF)
        in.setDevice(&file);
        while (!in.atEnd()) {
          line = in.readLine();
          out << line + "\n";
        }
        file.close();
      }
      appendTemplate("xtal.psf" , PSF);
    }
  }
/*
  void SIESTAOptimizer::buildPSFs() {
    double enmax = 0;
    // "PSF info" is of type
    // QList<QHash<QString, QString> >
    // e.g. a list of hashes containing
    // [atomic symbol : pseudopotential file] pairs
    QVariantList psfInfo = m_data["PSF info"].toList();
    for (int optIndex = 0;
         optIndex < psfInfo.size(); optIndex++) {
      QFile file;
      QString line, str, PSF;
      QStringList tmp_sl;
      QTextStream in;
      QList<QString> symbols = psfInfo.at(optIndex).toHash().keys();
      qSort(symbols);
      // Make a loop over the alphabetically sorted symbols:
      for (int i = 0; i < symbols.size(); i++) {
        PSF = ".psf";
        PSF.prepend(symbols.at(i));
        m_templates[PSF].clear();
//        m_templates.insert(PSF,QStringList(""));
        file.setFileName(psfInfo.at(optIndex).toHash().value(symbols.at(i)).toString());
        file.open(QIODevice::ReadOnly);
        QTextStream out(&PSF);
        //out.setDevice(&PSF);
        in.setDevice(&file);
        while (!in.atEnd()) {
          line = in.readLine();
          out << line + "\n";
        }
        file.close();
        appendTemplate(PSF , PSF);
      }
    }
  }
*/
} // end namespace XtalOpt

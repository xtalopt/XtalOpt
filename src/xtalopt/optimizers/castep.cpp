/**********************************************************************
  CASTEPOptimizer - Tools to interface with CASTEP

  Copyright (C) 2009-2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <xtalopt/optimizers/castep.h>

#include <openbabel/obconversion.h>
#include <openbabel/mol.h>

#include <QtCore/QDir>
#include <QtCore/QString>
#include <QtCore/QDebug>

using namespace std;
using namespace OpenBabel;

namespace XtalOpt {

  CASTEPOptimizer::CASTEPOptimizer(GlobalSearch::OptBase *parent,
                                   const QString &filename) :
    XtalOptOptimizer(parent)
  {
    // Set allowed data structure keys, if any, e.g.
    // None here!

    // Set allowed filenames, e.g.
    m_templates.insert("xtal.param",QStringList(""));
    m_templates.insert("xtal.cell",QStringList(""));

    // Setup for completion values
    m_completionFilename = "xtal.castep";
    m_completionStrings.clear();
    m_completionStrings.append("Geometry optimization completed successfully.");

    // Set output filenames to try to read data from, e.g.
    m_outputFilenames.append(m_completionFilename);

    // Set the name of the optimizer to be returned by getIDString()
    m_idString = "CASTEP";

    // Local execution setup:
    m_localRunCommand = "castep xtal";
    m_stdinFilename = "";
    m_stdoutFilename = "";
    m_stderrFilename = "";

    readSettings(filename);
  }

} // end namespace XtalOpt

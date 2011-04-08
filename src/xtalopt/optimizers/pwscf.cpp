/**********************************************************************
  PWscfOptimizer - Tools to interface with PWscf

  Copyright (C) 2009-2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <xtalopt/optimizers/pwscf.h>

#include <openbabel/obconversion.h>
#include <openbabel/mol.h>

#include <QtCore/QDir>
#include <QtCore/QString>
#include <QtCore/QDebug>

using namespace GlobalSearch;

namespace XtalOpt {

  PWscfOptimizer::PWscfOptimizer(OptBase *parent, const QString &filename) :
    XtalOptOptimizer(parent)
  {
    // Set allowed data structure keys, if any, e.g.
    // None here!

    // Set allowed filenames, e.g.
    m_templates.insert("xtal.in",QStringList(""));

    // Setup for completion values
    m_completionFilename = "xtal.out";
    m_completionStrings.clear();
    m_completionStrings.append("Final");

    // Set output filenames to try to read data from, e.g.
    m_outputFilenames.append(m_completionFilename);

    // Set the name of the optimizer to be returned by getIDString()
    m_idString = "PWscf";

    // Local execution setup:
    m_localRunCommand = "pw.x";
    m_stdinFilename = "xtal.in";
    m_stdoutFilename = "xtal.out";
    m_stderrFilename = "xtal.err";

    readSettings(filename);
  }

} // end namespace XtalOpt

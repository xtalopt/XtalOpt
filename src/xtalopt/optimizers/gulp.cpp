/**********************************************************************
  GULPOptimizer - Tools to interface with GULP

  Copyright (C) 2009-2011 by David C. Lonie

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
#include <xtalopt/xtalopt.h>

#include <QtCore/QProcess>
#include <QtCore/QDir>
#include <QtCore/QString>
#include <QtCore/QSemaphore>

#include <openbabel/obconversion.h>
#include <openbabel/mol.h>

using namespace GlobalSearch;

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
    m_completionStrings.append("**** Optimisation achieved ****");

    // Set output filenames to try to read data from, e.g.
    m_outputFilenames.append(m_completionFilename);

    // Set the name of the optimizer to be returned by getIDString()
    m_idString = "GULP";

    // Local execution setup:
    m_localRunCommand = "gulp";
    m_stdinFilename = "xtal.gin";
    m_stdoutFilename = "xtal.got";
    m_stderrFilename = "xtal.ger";

    readSettings(filename);
  }

} // end namespace XtalOpt

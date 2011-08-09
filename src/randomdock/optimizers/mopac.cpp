/**********************************************************************
  MopacOptimizer - Tools to interface with MOPAC

  Copyright (C) 2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <randomdock/optimizers/mopac.h>

#include <globalsearch/structure.h>

#include <openbabel/obconversion.h>
#include <openbabel/mol.h>

#include <QtCore/QDir>
#include <QtCore/QString>
#include <QtCore/QDebug>

using namespace GlobalSearch;

namespace RandomDock {

  MopacOptimizer::MopacOptimizer(OptBase *parent, const QString &filename) :
    Optimizer(parent)
  {
    // Set allowed data structure keys, if any, e.g.
    // None here!

    // Set allowed filenames, e.g.
    m_templates.insert("job.mop",QStringList(""));

    // Setup for completion values
    m_completionFilename = "job.out";
    m_completionStrings.clear();
    m_completionStrings.append("          FINAL HEAT OF FORMATION =");

    // Set output filenames to try to read data from
    m_outputFilenames.append(m_completionFilename);
    m_outputFilenames.append("job.arc");

    // Set the name of the optimizer to be returned by getIDString()
    m_idString = "MOPAC";

    // Local execution setup:
    m_localRunCommand = "mopac job";
    m_stdinFilename = "";
    m_stdoutFilename = "";
    m_stderrFilename = "";

    readSettings(filename);
  }

} // end namespace RandomDock


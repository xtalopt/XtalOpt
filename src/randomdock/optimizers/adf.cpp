/**********************************************************************
  ADFOptimizer - Tools to interface with ADF remotely

  Copyright (C) 2009 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <randomdock/optimizers/adf.h>

#include <globalsearch/structure.h>

#include <openbabel/obconversion.h>
#include <openbabel/mol.h>

#include <QtCore/QDir>
#include <QtCore/QString>
#include <QtCore/QDebug>

namespace RandomDock {

  ADFOptimizer::ADFOptimizer(GlobalSearch::OptBase *parent, const QString &filename) :
    GlobalSearch::Optimizer(parent)
  {
    // Set allowed data structure keys, if any, e.g.
    // None here!

    // Set allowed filenames, e.g.
    m_templates.insert("job.pbs",QStringList(""));

    // Setup for completion values
    m_completionFilename = "job.adfout";
    m_completionStrings.clear();
    m_completionStrings.append("GEOMETRY CONVERGED");

    // Set output filenames to try to read data from
    m_outputFilenames.append(m_completionFilename);

    // Set the name of the optimizer to be returned by getIDString()
    m_idString = "ADF";

    // Local execution setup:
    m_localRunCommand = "adf";
    m_stdinFilename = "job.adfin";
    m_stdoutFilename = "job.adfout";
    m_stderrFilename = "job.adferr";

    readSettings(filename);
  }

} // end namespace RandomDock

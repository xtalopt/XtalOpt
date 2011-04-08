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

#include <gapc/optimizers/gulp.h>

using namespace GlobalSearch;

namespace GAPC {

  GULPOptimizer::GULPOptimizer(OptBase *parent, const QString &filename) :
    Optimizer(parent)
  {
    // Set allowed data structure keys, if any, e.g.
    // None here!

    // Set allowed filenames, e.g.
    m_templates.insert("job.gin",QStringList(""));

    // Setup for completion values
    m_completionFilename = "job.got";
    m_completionStrings.append("**** Optimisation achieved ****");

    // Set output filenames to try to read data from, e.g.
    m_outputFilenames.append(m_completionFilename);

    // Set the name of the optimizer to be returned by getIDString()
    m_idString = "GULP";

    // Local execution setup:
    m_localRunCommand = "gulp";
    m_stdinFilename = "job.gin";
    m_stdoutFilename = "job.got";
    m_stderrFilename = "job.ger";

    readSettings(filename);
  }

} // end namespace GAPC

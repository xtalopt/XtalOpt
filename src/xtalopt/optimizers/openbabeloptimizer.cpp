/************************************************************************
  OpenBabelOptimizer - Dummy optimizer interface for use with OpenBabelQI

  Copyright (C) 2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 *************************************************************************/

#include "openbabeloptimizer.h"

namespace XtalOpt
{

OpenBabelOptimizer::OpenBabelOptimizer(GlobalSearch::OptBase *parent,
                                       const QString &filename) :
    XtalOptOptimizer(parent, filename)
{
  // Set allowed data structure keys, if any, e.g.
  // m_data.insert("Identifier name", QVariant())

  // Set allowed filenames, e.g.
  // m_templates.insert("filename.extension",QStringList)

  // Setup for completion values
  m_completionFilename.clear();
  m_completionStrings.clear();
  // m_completionStrings.append("string in m_completionFilename to search for");
  // m_completionStrings.append("Another string");

  // Set output filenames to try to read data from
  m_outputFilenames.append("optimized.cml");
  m_outputFilenames.append("input.cml");

  // Set the name of the optimizer to be returned by getIDString()
  m_idString = "OpenBabel";
}

}

/**********************************************************************
  PWscfOptimizer - Tools to interface with PWscf

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

#include "pwscf.h"
#include "../../generic/templates.h"

#include <QProcess>
#include <QDir>
#include <QString>
#include <QDebug>

#include "../../generic/structure.h"

#include <openbabel/obconversion.h>
#include <openbabel/mol.h>

using namespace std;
using namespace OpenBabel;
using namespace Eigen;

namespace Avogadro {

  PWscfOptimizer::PWscfOptimizer(OptBase *parent) :
    Optimizer(parent)
  {
    // Set allowed data structure keys, if any, e.g.
    // None here!

    // Set allowed filenames, e.g.
    m_templates.insert("xtal.in",QStringList(""));
    m_templates.insert("job.pbs",QStringList(""));

    // Setup for completion values
    m_completionFilename = "xtal.out";
    m_completionString   = "Final";

    // Set output filenames to try to read data from, e.g.
    m_outputFilenames.append(m_completionFilename);

    // Set the name of the optimizer to be returned by getIDString()
    m_idString = "PWscf";

    readSettings();
  }

} // end namespace Avogadro

#include "pwscf.moc"

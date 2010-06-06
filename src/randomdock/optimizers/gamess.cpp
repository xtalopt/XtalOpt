/**********************************************************************
  GAMESSOptimizer - Tools to interface with GAMESS remotely

  Copyright (C) 2009 by David C. Lonie

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

#include <randomdock/optimizers/gamess.h>

#include <globalsearch/structure.h>

#include <openbabel/obconversion.h>
#include <openbabel/mol.h>

#include <QProcess>
#include <QDir>
#include <QString>
#include <QDebug>

using namespace std;
using namespace OpenBabel;
using namespace Eigen;
using namespace Avogadro;

namespace RandomDock {

  GAMESSOptimizer::GAMESSOptimizer(OptBase *parent, const QString &filename) :
    Optimizer(parent)
  {
    // Set allowed data structure keys, if any, e.g.
    // None here!

    // Set allowed filenames, e.g.
    m_templates.insert("job.inp",QStringList(""));
    m_templates.insert("job.pbs",QStringList(""));

    // Setup for completion values
    m_completionFilename = "job.gamout";
    m_completionString   = "***** EQUILIBRIUM GEOMETRY LOCATED *****";

    // Set output filenames to try to read data from
    m_outputFilenames.append(m_completionFilename);

    // Set the name of the optimizer to be returned by getIDString()
    m_idString = "GAMESS";

    readSettings(filename);
  }

} // end namespace Avogadro

//#include "gamess.moc"

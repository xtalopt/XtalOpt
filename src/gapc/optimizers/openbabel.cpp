/**********************************************************************
  OpenBabelOptimizer - Tools to interface with OpenBabel's force fields

  Copyright (C) 2009-2010 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <gapc/optimizers/openbabel.h>
#include <gapc/structures/protectedcluster.h>

#include <avogadro/moleculefile.h>

#include <openbabel/obconversion.h>
#include <openbabel/forcefield.h>
#include <openbabel/mol.h>

#include <QDir>
#include <QString>
#include <QSemaphore>

using namespace GlobalSearch;
using namespace Avogadro;
using namespace OpenBabel;

namespace GAPC {

  OpenBabelOptimizer::OpenBabelOptimizer(OptBase *parent, const QString &filename) :
    Optimizer(parent)
  {
    // Since OpenBabel optimization occurs programmatically, this file
    // is quite different from the other optimizers.

    m_idString = "OpenBabel";

    m_templates.clear();
    m_completionFilename = "NA";
    m_completionStrings.clear();
    m_outputFilenames.append("structure.cml");

    readSettings(filename);
  }
}

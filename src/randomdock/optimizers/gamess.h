/**********************************************************************
  RandomDockGAMESS - Tools to interface with GAMESS remotely

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

#ifndef RDGAMESS_H
#define RDGAMESS_H

#include "../../generic/optimizer.h"

using namespace Avogadro;

namespace RandomDock {

  class GAMESSOptimizer : public Optimizer
  {
    Q_OBJECT

   public:
    explicit GAMESSOptimizer(OptBase *parent, const QString &filename = "");

  };

} // end namespace RandomDock

#endif

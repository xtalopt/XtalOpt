/**********************************************************************
  Scene - Contains a docked substrate in a matrix

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

#ifndef SCENEMOL_H
#define SCENEMOL_H

#include "../../generic/structure.h"

#include <QDebug>

using namespace Avogadro;

namespace RandomDock {
  class Scene : public Structure
  {
    Q_OBJECT

   public:
    Scene(QObject *parent = 0);
    virtual ~Scene();

   signals:

   public slots:
    void updateFromMolecule(Molecule *mol);

   private slots:

   private:

  };

} // end namespace Avogadro

#endif

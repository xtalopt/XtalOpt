/**********************************************************************
  RandomDock - Scene: Wrapper for Avogadro::Molecule to hold the
  central molecule and matrix elements in a docking problem

  Copyright (C) 2009-2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <randomdock/structures/scene.h>

#include <QtCore/QDebug>

using namespace std;
using namespace Avogadro;

namespace RandomDock {

  Scene::Scene(QObject *parent) :
    Structure(parent)
  {
  }

  Scene::~Scene()
  {
  }

} // end namespace RandomDock

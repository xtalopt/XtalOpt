/**********************************************************************
  RandomDock - Scene: Wrapper for Avogadro::Molecule to hold the 
  central molecule and matrix elements in a docking problem

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

#include "scene.h"

#include <QDebug>

using namespace std;

namespace Avogadro {

  Scene::Scene(QObject *parent) : 
    Structure(parent)
  {
  }

  Scene::~Scene()
  {
  }

  void Scene::updateFromMolecule(Molecule *mol) {
    // TODO check if this is all we need to do.
    if (mol->numAtoms() != numAtoms()) {
      qWarning() << "Number of atoms changed during optimization. Killing structure!";
      setStatus(Killed);
      return;
    }

    // Update atom positions, assume indexes are the same.
    for (uint i = 0; i < numAtoms(); i++)
      atom(i)->setPos(mol->atom(i)->pos());

    // Update energy
    setEnergy(mol->energy(0));

    emit moleculeChanged();
  }

} // end namespace Avogadro

#include "scene.moc"

/**********************************************************************
  xtaloptWrapper.h - Contains functions that convert between class 'Crystal'
                     and class 'Xtal'

  Copyright (C) 2016 by Patrick S. Avery

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef XTALOPT_WRAPPER_H
#define XTALOPT_WRAPPER_H

#include <xtalopt/spgInit/spgInit.h>
#include <xtalopt/structures/xtal.h>
#include "crystal.h"

// Returns a dynamically allocated xtal
XtalOpt::Xtal* crystal2Xtal(const Crystal& c)
{
  std::vector<atomStruct> atoms = c.getAtoms();
  latticeStruct lat = c.getLattice();
  XtalOpt::Xtal* xtal = new XtalOpt::Xtal(lat.a, lat.b, lat.c,
                                          lat.alpha, lat.beta, lat.gamma);
  for (size_t i = 0; i < atoms.size(); i++) {
    const atomStruct& as = atoms.at(i);
    Avogadro::Atom* atom = xtal->addAtom();
    // Need to convert these coordinates to cartesian...
    Eigen::Vector3d pos(as.x,as.y,as.z);
    pos = xtal->fracToCart(pos);
    atom->setPos(pos);
    atom->setAtomicNumber(static_cast<int>(as.atomicNum));
  }
  return xtal;
}

Crystal xtal2Crystal(XtalOpt::Xtal* xtal)
{
  latticeStruct lat(xtal->getA(), xtal->getB(), xtal->getC(),
                    xtal->getAlpha(), xtal->getBeta(), xtal->getGamma());
  std::vector<atomStruct> atoms;
  QList<Avogadro::Atom*> xAtoms = xtal->atoms();

  for (size_t i = 0; i < xAtoms.size(); i++) {
    unsigned int atomicNum = xAtoms.at(i)->atomicNumber();
    Eigen::Vector3d fracCoords = *(xtal->cartToFrac(xAtoms.at(i)->pos()));
    atomStruct as(atomicNum, fracCoords[0], fracCoords[1], fracCoords[2]);
    atoms.push_back(as);
  }

  // Last parameter is false because xtalopt uses covalent radii, not vdw
  // radii
  Crystal c(atoms, lat, false);
  return c;
}

#endif

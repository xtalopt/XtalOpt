/**********************************************************************
  crystal.cpp - Custom crystal class for generating crystals with
                specific space groups.

  Copyright (C) 2016 by Patrick S. Avery

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <iostream>
// for sqrt(), sin(), cos(), etc.
#include <cmath>

#include "crystal.h"
#include "spgInit.h"
#include "utilityFunctions.h"

using namespace std;

//#define CRYSTAL_DEBUG

Crystal::Crystal(vector<atomStruct> a, latticeStruct l) :
  m_atoms(a),
  m_lattice(l)
{

}

void Crystal::removeAtomAt(size_t i)
{
  if (i - 1 > m_atoms.size()) std::cout << "Error: tried to remove an atom "
                                        << "at index " << i << " and the "
                                        << "size is only " << m_atoms.size()
                                        << "!\n";
  else m_atoms.erase(m_atoms.begin() + i);
}

static inline bool atomsHaveSamePosition(const atomStruct& a1,
                                         const atomStruct& a2)
{
  // min distance
  double minD = 0.00001;
  double dx = fabs(a1.x - a2.x);
  double dy = fabs(a1.y - a2.y);
  double dz = fabs(a1.z - a2.z);
  if (dx < 0.00001 && dy < 0.00001 && dz < 0.00001) return true;
  return false;
}

// We're assuming we're using fractional coordinates...
static inline void wrapUnitToCell(double& u)
{
  double minD = 0.00001;
  while (u < 0.0) u += 1.0;
  while (u >= 1.0 || fabs(u - 1.0) < minD) u -= 1.0;
}

static inline void wrapAtomToCell(atomStruct& as)
{
  wrapUnitToCell(as.x);
  wrapUnitToCell(as.y);
  wrapUnitToCell(as.z);
}

void Crystal::wrapAtomsToCell()
{
  for (size_t i = 0; i < m_atoms.size(); i++) wrapAtomToCell(m_atoms[i]);
}


void Crystal::removeAtomsWithSameCoordinates()
{
  for (size_t i = 0; i < m_atoms.size(); i++) {
    for (size_t j = i + 1; j < m_atoms.size(); j++) {
      if (atomsHaveSamePosition(m_atoms.at(i), m_atoms.at(j))) {
        removeAtomAt(j);
        j--;
      }
    }
  }
}

void Crystal::addAtomIfPositionIsEmpty(atomStruct& as)
{
  wrapAtomToCell(as);
  bool positionIsEmpty = true;
  for (size_t i = 0; i < m_atoms.size(); i++) {
    if (atomsHaveSamePosition(as, m_atoms.at(i)) &&
        as.atomicNum == m_atoms.at(i).atomicNum) {
      positionIsEmpty = false;
      break;
    }
  }
  if (positionIsEmpty) addAtom(as);
}

atomStruct Crystal::getAtomInCartCoords(const atomStruct& as) const
{
  atomStruct ret = as;
  const double& a = m_lattice.a;
  const double& b = m_lattice.b;
  const double& c = m_lattice.c;
  const double alpha = deg2rad(m_lattice.alpha);
  const double beta = deg2rad(m_lattice.beta);
  const double gamma = deg2rad(m_lattice.gamma);
  const double v = getUnitVolume();
  // We're just gonna do the matrix multiplication by hand...
  ret.x = a * as.x + b * cos(gamma) * as.y + c * cos(beta) * as.z;
  ret.y = b * sin(gamma) * as.y +
          c * (cos(alpha) - cos(beta) * cos(gamma)) / sin(gamma) * as.z;
  ret.z = c * v / sin(gamma) * as.z;
  // I want to make sure these are all positive
  // When the value is zero, unfortunately, it can be very slightly negative
  // sometimes... So I commented out this assertion
  //assert(ret.x > 0 && ret.y > 0 && ret.z > 0);
  return ret;
}

double Crystal::getUnitVolume() const
{
  const latticeStruct& l = m_lattice;
  const double alpha = deg2rad(m_lattice.alpha);
  const double beta = deg2rad(m_lattice.beta);
  const double gamma = deg2rad(m_lattice.gamma);
  return sqrt(1.0 -
              pow(cos(alpha), 2.0) -
              pow(cos(beta), 2.0) -
              pow(cos(gamma), 2.0) +
              2.0 * cos(alpha) * cos(beta) * cos(gamma));
}

double Crystal::getVolume() const
{
  return m_lattice.a * m_lattice.b * m_lattice.c * getUnitVolume();
}

void Crystal::fillCellWithAtom(uint spg, const atomStruct& as)
{
  vector<string> dupVec = SpgInit::getVectorOfDuplications(spg);
  vector<string> fpVec = SpgInit::getVectorOfFillPositions(spg);

  double x = as.x;
  double y = as.y;
  double z = as.z;
  uint atomicNum = as.atomicNum;
  for (size_t j = 0; j < dupVec.size(); j++) {
    // First, we are going to set up the duplicate vector components

    vector<string> dupComponents = split(dupVec.at(j), ',');

    // These are all just numbers, so we can just convert them
    double dupX = stof(dupComponents.at(0));
    double dupY = stof(dupComponents.at(1));
    double dupZ = stof(dupComponents.at(2));

    // Next, we are going to loop through all fill positions
    for (size_t k = 0; k < fpVec.size(); k++) {
      // Skip the first one if we are at j = 0. It is always just (x,y,z)
      if (j == 0 && k == 0) continue;
      vector<string> fpComponents = split(fpVec.at(k), ',');

      double newX = SpgInit::interpretComponent(fpComponents.at(0), x, y, z) + dupX;
      double newY = SpgInit::interpretComponent(fpComponents.at(1), x, y, z) + dupY;
      double newZ = SpgInit::interpretComponent(fpComponents.at(2), x, y, z) + dupZ;

      atomStruct newAtom(atomicNum, newX, newY, newZ);
      addAtomIfPositionIsEmpty(newAtom);
    }
  }

}

double Crystal::getDistance(const atomStruct& as1,
                            const atomStruct& as2) const
{
  atomStruct cAs1 = getAtomInCartCoords(as1);
  atomStruct cAs2 = getAtomInCartCoords(as2);

  return sqrt(pow(cAs1.x - cAs2.x, 2.0) +
              pow(cAs1.y - cAs2.y, 2.0) +
              pow(cAs1.z - cAs2.z, 2.0));
}

double Crystal::findNearestNeighborAtomAndDistance(const atomStruct& as,
                                                   atomStruct& neighbor) const
{
  // We are assuming this atom has already been placed inside this unit cell
  // Once we find it, if we find another one, that's an extra atom on top.
  // TODO: need to center this atom inside the unit cell before calculating
  // distances. This will correct for distances that extend into other cells
  bool selfFound = false;
  double smallestDistance = 1000000.00;
  for (size_t i = 0; i < m_atoms.size(); i++) {
    if (m_atoms.at(i) == as && !selfFound) {
      selfFound = true;
      continue;
    }
    // We found an atom on top of this one
    else if (m_atoms.at(i) == as && selfFound) {
      neighbor = m_atoms.at(i);
      return 0.0;
    }

    double newDistance = getDistance(as, m_atoms.at(i));
    if (newDistance < smallestDistance) {
      smallestDistance = newDistance;
      neighbor = m_atoms.at(i);
    }
  }

  return smallestDistance;
}

void Crystal::fillUnitCell(uint spg)
{
#ifdef CRYSTAL_DEBUG
  cout << "Crystall::fillUnitCell() was called for spg = " << spg
       << " for the following unit cell!\n";
  printCrystalInfo();
#endif
  // In case the atoms aren't already wrapped, go ahead and wrap them...
  wrapAtomsToCell();

  // Keep a copy of what the atoms were initially
  vector<atomStruct> atoms = getAtoms();
  // Loop through all these atoms!
  for (size_t i = 0; i < atoms.size(); i++) fillCellWithAtom(spg, atoms.at(i));
#ifdef CRYSTAL_DEBUG
  cout << "Filling is complete! Info is now:\n";
  printCrystalInfo();
#endif
}

void Crystal::printAtomInfo(const atomStruct& as) const
{
  cout << "  For " << as.atomicNum << ", coords are: ("
       << as.x << "," << as.y << "," << as.z << ")\n";
}

void Crystal::printAtomInfo() const
{
  for (size_t i = 0; i < m_atoms.size(); i++) printAtomInfo(m_atoms.at(i));
}

void Crystal::printLatticeInfo() const
{
  cout << "a: " << m_lattice.a << "\n";
  cout << "b: " << m_lattice.b << "\n";
  cout << "c: " << m_lattice.c << "\n";
  cout << "alpha: " << m_lattice.alpha << "\n";
  cout << "beta: " << m_lattice.beta << "\n";
  cout << "gamma: " << m_lattice.gamma << "\n";
}

void Crystal::printCrystalInfo() const
{
  cout << "\n**** Printing Crystal Info ****\n";
  printLatticeInfo();
  printAtomInfo();
  cout << "**** End Crystal Info ****\n\n";
}


/**********************************************************************
  MolTransformations - some convenience functions for molecular transformations

  Copyright (C) 2017 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#include <iostream>

#include <globalsearch/structures/molecule.h>

#include "moltransformations.h"

namespace GlobalSearch {

Vector3 MolTransformations::getMeanPosition(const Molecule& mol)
{
  Vector3 mean(0.0, 0.0, 0.0);
  for (const auto& atom : mol.atoms()) {
    mean[0] += atom.pos()[0];
    mean[1] += atom.pos()[1];
    mean[2] += atom.pos()[2];
  }
  mean /= static_cast<double>(mol.numAtoms());
  return mean;
}

void MolTransformations::setMeanPosition(Molecule& mol, const Vector3& pos)
{
  // Find the offset
  Vector3 offset = getMeanPosition(mol) - pos;

  // Now shift all the coordinates by that amount
  translateMolecule(mol, -offset);
}

void MolTransformations::rotateMolecule(Molecule& mol, short axis, double theta)
{
  // First, find the two axes that are NOT the axes of rotation. These we
  // will label 'a' and 'b'
  short a, b;
  switch (axis) {
    case 0:
      a = 1;
      b = 2;
      break;
    case 1:
      a = 0;
      b = 2;
      break;
    case 2:
      a = 0;
      b = 1;
      break;
    default:
      std::cerr << "Error in " << __FUNCTION__
                << ": invalid axis number: " << axis
                << "!\nValid axis numbers are 0, 1, and 2\n";
      return;
  }

  // Cache these for improved speed
  double sinTheta = sin(theta);
  double cosTheta = cos(theta);

  for (auto& atom : mol.atoms()) {
    // 'x' corresponds to the positional value in the 'a' direction
    // 'y' corresponds to the positional value in the 'b' direction
    double x = atom.pos()[a];
    double y = atom.pos()[b];
    atom.pos()[a] = x * cosTheta - y * sinTheta;
    atom.pos()[b] = y * cosTheta + x * sinTheta;
  }
}

void MolTransformations::rotateMolecule(Molecule& mol, double thetaX,
                                        double thetaY, double thetaZ)
{
  rotateMolecule(mol, 0, thetaX);
  rotateMolecule(mol, 1, thetaY);
  rotateMolecule(mol, 2, thetaZ);
}

void MolTransformations::translateMolecule(Molecule& mol, const Vector3& vec)
{
  for (auto& atom : mol.atoms())
    atom.pos() += vec;
}

void MolTransformations::translateMolecule(Molecule& mol, double x, double y,
                                           double z)
{
  translateMolecule(mol, Vector3(x, y, z));
}

} // end namespace GlobalSearch

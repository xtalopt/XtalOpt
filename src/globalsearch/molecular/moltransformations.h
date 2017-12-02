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

#ifndef GLOBALSEARCH_MOLTRANSFORMATIONS_H
#define GLOBALSEARCH_MOLTRANSFORMATIONS_H

#include <globalsearch/vector.h>

namespace GlobalSearch {

class Molecule;

class MolTransformations
{
public:
  /**
   * Get the mean position of the atoms in the molecule.
   *
   * @param mol The molecule for whom the mean position will be obtained.
   *
   * @return The mean position of the molecule.
   */
  static Vector3 getMeanPosition(const Molecule& mol);

  /**
   * Set the mean position of the molecule to be at a specific position.
   *
   * @param mol The molecule to be centered.
   * @param pos The position at which the center of the molecule is to be.
   */
  static void setMeanPosition(Molecule& mol, const Vector3& pos);

  /**
   * Translate all atoms in the molecule so that the mean atom position will be
   * at the origin.
   *
   * @param mol The molecule to be centered.
   */
  static void centerMolecule(Molecule& mol)
  {
    setMeanPosition(mol, Vector3(0.0, 0.0, 0.0));
  }

  /**
   * Rotate the molecule about one of the principal Cartesian axes.
   *
   * @param mol The molecule to be rotated.
   * @param axis The axis to use. '0' is for x, '1' is for y,
   *             and '2' is for z. These are the only acceptable numbers.
   * @param theta The angle to rotate (counterclockwise) in radians.
   */
  static void rotateMolecule(Molecule& mol, short axis, double theta);

  /**
   * Rotate the molecule about each of the principal Cartesian axes.
   * The rotation order is as follows: x, y, z.
   *
   * @param mol The molecule to be rotated.
   * @param thetaX The angle to rotate (counterclockwise) in radians for the
   *               x axis.
   * @param thetaY The angle to rotate (counterclockwise) in radians for the
   *               y axis.
   * @param thetaZ The angle to rotate (counterclockwise) in radians for the
   *               z axis.
   */
  static void rotateMolecule(Molecule& mol, double thetaX, double thetaY,
                             double thetaZ);

  /**
   * Translate the molecule by a specific Cartesian vector.
   *
   * @mol The molecule to be translated.
   * @param vec The vector by which the molecule will be translated.
   */
  static void translateMolecule(Molecule& mol, const Vector3& vec);

  /**
   * Translate the molecule by x, y, z coordinates.
   *
   * @mol The molecule to be translated.
   * @param x The distance to translate in the x direction.
   * @param y The distance to translate in the y direction.
   * @param z The distance to translate in the z direction.
   */
  static void translateMolecule(Molecule& mol, double x, double y, double z);
};

} // end namespace GlobalSearch

#endif // GLOBALSEARCH_MOLTRANSFORMATIONS_H

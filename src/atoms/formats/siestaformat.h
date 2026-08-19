/**********************************************************************
  SiestaFormat - A simple reader for SIESTA output structure.

  Copyright (C) 2016 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef ATOMS_SIESTA_FORMAT_H
#define ATOMS_SIESTA_FORMAT_H

#include <iosfwd>

#include <QString>

namespace Atoms {

class Geometry;

/**
 * @class The Spanish Initiative for Electronic Simulations with Thousands
 *        of Atoms (SIESTA) format.
 *        https://departments.icmab.es/leem/siesta/
 */
class SiestaFormat
{
public:
  /**
   * Read a SIESTA input/structure file.
   */
  static bool read(Atoms::Geometry& s, const QString& filename);

  /**
   * Read SIESTA optimizer output.
   */
  static bool readOutput(Atoms::Geometry& s, const QString& filename);

  /**
   * Write a z-matrix using the specification in SIESTA (Spanish Initiative
   * for Electronic Simulations with Thousands of Atoms) to @p out. One
   * molecule block will be used for every set of bonded atoms. If
   * @p reorderAtomsToMatch is true, the atoms in @p s will be re-ordered
   * according to the z-matrix ordering.
   *
   * A molecule wrap to the smallest bonds will always be performed on
   * @p s.
   *
   * @param s The geometry for which to write the z-matrix.
   * @param out The output for the SIESTA z-matrix.
   * @param fixR Whether or not to fix all of the bond distances in the
   *             z-matrix
   * @param fixA Whether or not to fix all of the bond angles in the z-matrix
   * @param fixT Whether or not to fix all the torsions (dihedrals) in the
   *             z-matrix
   * @param reorderAtomsToMatch Whether or not to re-order the atoms according
   *                            to the z-matrix ordering
   *
   * @return True on success. False on failure.
   */
  static bool writeSiestaZMatrix(Atoms::Geometry& s, std::ostream& out, bool fixR, bool fixA,
                                 bool fixT, bool reorderAtomsToMatch = false);

  /**
   * Write a SIESTA Z-matrix for @p s and return the text as a QString.
   *
   * @return Z-matrix text on success, or an empty string on failure.
   */
  static QString writeSiestaZMatrixToString(Atoms::Geometry& s, bool fixR, bool fixA, bool fixT,
                                            bool reorderAtomsToMatch = false);
};
} // namespace Atoms

#endif // ATOMS_SIESTA_FORMAT_H

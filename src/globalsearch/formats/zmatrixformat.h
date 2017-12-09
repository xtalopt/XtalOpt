/**********************************************************************
  ZMatrixFormat -- A simple reader for z-matrix files.

  Copyright (C) 2017 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef GLOBALSEARCH_ZMATRIX_FORMAT_H
#define GLOBALSEARCH_ZMATRIX_FORMAT_H

#include <iostream>
#include <vector>

// Forward declaration
class QString;

namespace GlobalSearch {

// Forward declaration
class Structure;

// For writing z-matrices. Each entry contains indices of the atoms it
// should be connected to.
// NOTE: these indices are zero-based. If you write the output to a z-matrix
// file, you should add 1 to all (except ind) because the actual z-matrix
// format is 1-based.
struct ZMatrixEntry
{
  // The index of the atom for this entry.
  long long ind;

  // The index of the atom that it is bonded to for the r index.
  long long rInd;

  // The index of the atom which is to be used with the r atom to make the
  // angle.
  long long angleInd;

  // The index of the final atom to be used to make the dihedral.
  long long dihedralInd;

  ZMatrixEntry(long long ind = -1, long long rInd = -1, long long angleInd = -1,
               long long dihedralInd = -1)
    : ind(ind), rInd(rInd), angleInd(angleInd), dihedralInd(dihedralInd)
  {
  }
};

/**
 * @class The ZMATRIX format.
 *
 * Note: if the z-matrix does not contain any dummy atoms, this function
 * will generate bonds in the structure using the connections in the
 * z-matrix.
 * If the z-matrix DOES contain dummy atoms, this function will use
 * bond perception to generate the bonds.
 */
class ZMatrixFormat
{
public:
  static bool read(Structure* s, std::istream& in);

  static bool write(const Structure& s, std::ostream& out);

  /**
   * Chooses numberings for atoms to be part of the z-matrix, and sets
   * rInd, angleInd, and dihedralInd for each entry. See the ZMatrixEntry
   * struct definition above for more details about the return type.
   * Carbons will preferentially be first, and hydrogens will preferentially
   * be last.
   * If the structure contains multiple molecules, the first atom
   * of each molecule can be identified because it will have -1 for rInd,
   * angleInd, and dihedralInd.
   * NOTE: these indices are zero-based. If you write the output to a
   * z-matrix file, you should add 1 to all (except ind) because the actual
   * z-matrix format is 1-based.
   */
  static std::vector<ZMatrixEntry> generateZMatrixEntries(const Structure* s);

  /**
   * Write a z-matrix using the specification in SIESTA (Spanish Initiative
   * for Electronic Simulations with Thousands of Atoms) to @p out. One
   * molecule block will be used for every set of bonded atoms. If
   * Structure::reusePreoptBonding() is true, the atoms in @p s will be
   * re-ordered according to the z-matrix ordering, and the pre-optimization
   * bonding information will be stored.
   *
   * A molecule wrap to the smallest bonds will always be performed on
   * @param s.
   *
   * @param s The structure for which to write the z-matrix.
   * @param out The output for the SIESTA z-matrix.
   * @param fixR Whether or not to fix all of the bond distances in the
   *             z-matrix
   * @param fixA Whether or not to fix all of the bond angles in the z-matrix
   * @param fixT Whether or not to fix all the torsions (dihedrals) in the
   *             z-matrix
   *
   * @return True on success. False on failure.
   */
  static bool writeSiestaZMatrix(Structure& s, std::ostream& out, bool fixR,
                                 bool fixA, bool fixT);

  /**
   * Reorder the atoms so that they match the ordering for the z-matrix
   * ordering.
   *
   * @param s The Structure whose atoms are to be reordered.
   */
  static void reorderAtomsToMatchZMatrix(Structure& s);
};
}

#endif // GLOBALSEARCH_ZMATRIX_FORMAT_H

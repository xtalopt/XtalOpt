/**********************************************************************
  XtalComp - Determine if two crystal description represent the same
  structure

  Copyright (C) 2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef XTALCOMP_H
#define XTALCOMP_H

#include "xcmatrix.h"
#include "xctransform.h"
#include "xcvector.h"

#include <vector>

class XtalComp
{
 public:
  /**
   * Compare two crystal descriptions, return true if the underlying
   * structures are same, false otherwise.
   *
   * cellMatrix[1|2] are 3x3 matrices that contain the cell vectors as
   * the matrix rows, e.g.
   * cellMatrix[0][0] = v1[0];
   * cellMatrix[0][1] = v1[1];
   * cellMatrix[0][2] = v1[2];
   * cellMatrix[1][0] = v2[0];
   * ...
   *
   * types[1|2] are vectors of atomic numbers that correspond to entries
   * in postions[1|2].
   *
   * positions[1|2] are vectors of XcVectors (see XcVector.h) that
   * correspond to types[1|2]. The XcVectors are the fractional
   * coordinates of the atoms.
   *
   * @param cellMatrix1 Cell vectors of first description
   * @param types1 Atomic numbers of first description
   * @param positions1 Fractional coordinates of first description
   * @param cellMatrix2 Cell vectors of second description
   * @param types2 Atomic numbers of second description
   * @param positions2 Fractional coordinates of second description
   * @param transform Array to be overwritten with a row-major matrix
   * representing the transformation used if the structures are determined
   * to match.
   * @param cartTol Tolerance for atomic position comparisons in cartesian units
   * @params angleTol Used for matching candidate sublattices -- default of
   * 0.25 degrees should be sufficient.
   *
   * @return True if structures match, false otherwise
   */
  static bool compare(const XcMatrix &cellMatrix1,
                      const std::vector<unsigned int> &types1,
                      const std::vector<XcVector> &positions1,
                      const XcMatrix &cellMatrix2,
                      const std::vector<unsigned int> &types2,
                      const std::vector<XcVector> &positions2,
                      float transform[16] = 0,
                      const double cartTol = 0.05,
                      const double angleTol = 0.25);

  virtual ~XtalComp();

 protected:
  class ReducedXtal;

  // Initialization
  XtalComp(ReducedXtal *rx1, ReducedXtal *rx2,
           const double cartTol, const double angleTol);

  void setLeastFrequentAtomInfo();
  void setReferenceBasis();
  void prepareRx1();
  void getCurrentTransform(float[16]);

  // Are there more comparisons to make?
  bool hasMoreTransforms() const;
  bool hasMoreTranslations() const;

  // Update working coordinates
  void applyNextTransform();
  void applyNextTranslation();

  // Make next comparison
  bool compareCurrent();

  // Tolerance
  double m_lengthtol;
  double m_angletol;

  // Reduced xtals
  ReducedXtal *m_rx1;
  ReducedXtal *m_rx2;

  // Least frequent atom info
  unsigned int m_lfAtomType;
  unsigned int m_lfAtomCount;

  // Supercell of lfAtoms in xtal2
  void buildSuperLfCCoordList2();
  std::vector<XcVector> m_superLfCCoordList2;
  void findCandidateTransforms();

  // Add atoms around cell boundaries for stability during comparisons
  static void expandFractionalCoordinates(std::vector<unsigned int> *types,
                                          std::vector<XcVector> *fcoords,
                                          const XcMatrix &cmat, // needed for tol calc
                                          const double tol);

  // Reference vectors:
  XcVector m_refVec1;
  XcVector m_refVec2;
  XcVector m_refVec3;

  std::vector<XcTransform> m_transforms;
  XcTransform m_transform;
  unsigned int m_transformsIndex;

  void buildTransformedXtal2();
  XcMatrix m_transformedCMat;
  XcMatrix m_transformedFMat;
  std::vector<unsigned int> m_transformedTypes;
  std::vector<XcVector> m_transformedFCoords;
  std::vector<XcVector> m_transformedCCoords;
};

#endif

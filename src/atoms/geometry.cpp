/**********************************************************************
  Geometry - Reusable structure/geometry API.

  Copyright (C) 2009-2011 by David C. Lonie
  Copyright (C) 2017 by Patrick S. Avery
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <atoms/geometry.h>

#include <common/chull.h>
#include <common/compatibility/platform_compat.h>
#include <common/constants.h>
#include <atoms/eleminfo.h>
#include <common/output.h>
#include <common/random.h>
#include <common/timing.h>
#include <common/numericutils.h>
#include <atoms/data/spghmnames.h>

#include <xtalcomp/xtalcomp.h>

extern "C" {
#include <libmsym/src/msym.h>
#include <spglib/spglib.h>
}

#include <QTextStream>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <limits>
#include <numeric>
#include <string>
#include <unordered_map>

namespace Atoms {

namespace {

struct NNHistMap
{
  int i;
  double halfstep;
  std::vector<Common::Vector3>* atomPositions;
  std::vector<Common::Vector3>* translations;
  std::vector<double>* dist;
};

std::vector<int> calcNNHistChunk(const NNHistMap& m)
{
  const Common::Vector3* v1 = &(m.atomPositions->at(m.i));
  const Common::Vector3* v2;
  std::vector<int> freq(m.dist->size(), 0);
  double diff;

  for (int j = m.i + 1; j < static_cast<int>(m.atomPositions->size()); j++) {
    v2 = &(m.atomPositions->at(j));
    diff = std::fabs(((*v1) - (*v2)).norm());
    for (int k = 0; k < static_cast<int>(m.dist->size()); k++) {
      if (std::fabs(diff - m.dist->at(k)) < m.halfstep)
        freq[k]++;
    }
    for (int t = 0; t < static_cast<int>(m.translations->size()); t++) {
      if (m.translations->at(t).norm() < ZERO08)
        continue;
      diff = std::fabs(((*v1) - ((*v2) + m.translations->at(t))).norm());
      for (int k = 0; k < static_cast<int>(m.dist->size()); k++) {
        if (std::fabs(diff - m.dist->at(k)) < m.halfstep)
          freq[k]++;
      }
    }
  }

  return freq;
}

void reduceNNHistChunks(std::vector<double>& final, const std::vector<int>& tmp)
{
  if (final.size() != tmp.size()) {
    final.assign(tmp.begin(), tmp.end());
  } else {
    for (size_t i = 0; i < final.size(); i++)
      final[i] += tmp[i];
  }
}

unsigned int gcd(unsigned int a, unsigned int b)
{
  return b == 0 ? a : gcd(b, a % b);
}

} // namespace

// Convert between the row-based cell matrix and a column-based lattice array.
void cellToColumnLatticeArray(const Common::Matrix3& cell, double lattice[3][3])
{
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      lattice[i][j] = cell(j, i);
}

Common::Matrix3 columnLatticeArrayToCell(const double lattice[3][3])
{
  Common::Matrix3 cell;
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      cell(j, i) = lattice[i][j];
  return cell;
}

void Geometry::setCellInfo(double a, double b, double c, double alpha, double beta, double gamma)
{
  // Assigning a cell makes this 3D.
  g_unitCell.setCellParameters(a, b, c, alpha, beta, gamma);
  clearGeometryCaches();
}

void Geometry::setCellInfo(const Common::Vector3& a, const Common::Vector3& b,
                            const Common::Vector3& c)
{
  // Assigning a cell makes this 3D.
  g_unitCell.setCellVectors(a, b, c);
  clearGeometryCaches();
}

bool Geometry::isCellMatrixUsable(const Common::Matrix3& matrix)
{
  const double determinant = matrix.determinant();
  return GS_ISFINITE(determinant) && std::fabs(determinant) > ZERO08;
}

void Geometry::setVolume(double volume)
{
  // 3D only.
  if (!is3D()) return;
  // Get scaling factor
  double factor = std::pow(volume / getVolume(), 1.0 / 3.0); // Cube root

  // Store position of atoms in fractional units
  std::vector<Atom>& atomList = atoms();
  QList<Common::Vector3> fracCoordsList;
  for (const auto& atm : atomList)
    fracCoordsList.append(cartToFrac(atm.pos()));

  // Scale cell
  setCellInfo(factor * getA(), factor * getB(), factor * getC(), getAlpha(),
              getBeta(), getGamma());

  // Recalculate coordinates:
  for (int i = 0; i < static_cast<int>(atomList.size()); i++)
    atomList.at(i).setPos(fracToCart(fracCoordsList.at(i)));
}

void Geometry::rescaleCell(double a, double b, double c, double alpha,
                            double beta, double gamma)
{
  // 3D only.
  if (!is3D()) return;
  if (a == 0.0 && b == 0.0 && c == 0.0 &&
      alpha == 0.0 && beta == 0.0 && gamma == 0.0)
    return;

  const double targetVolume = getVolume();
  rotateCellAndCoordsToStandardOrientation();

  // Store position of atoms in fractional units
  std::vector<Atom>& atomList = atoms();
  QList<Common::Vector3> fracCoordsList;
  for (const auto& atm : atomList)
    fracCoordsList.append(cartToFrac(atm.pos()));

  double nA = getA();
  double nB = getB();
  double nC = getC();
  double nAlpha = getAlpha();
  double nBeta = getBeta();
  double nGamma = getGamma();

  if (a != 0.0)
    nA = a;
  if (b != 0.0)
    nB = b;
  if (c != 0.0)
    nC = c;
  if (alpha != 0.0)
    nAlpha = alpha;
  if (beta != 0.0)
    nBeta = beta;
  if (gamma != 0.0)
    nGamma = gamma;

  int freeLengths = 0;
  if (a == 0.0)
    ++freeLengths;
  if (b == 0.0)
    ++freeLengths;
  if (c == 0.0)
    ++freeLengths;

  if (freeLengths > 0) {
    UnitCell adjusted;
    adjusted.setCellParameters(nA, nB, nC, nAlpha, nBeta, nGamma);
    const double scale =
      std::pow(targetVolume / adjusted.volume(), 1.0 / freeLengths);
    if (a == 0.0)
      nA *= scale;
    if (b == 0.0)
      nB *= scale;
    if (c == 0.0)
      nC *= scale;
  }

  setCellInfo(nA, nB, nC, nAlpha, nBeta, nGamma);

  // Recalculate coordinates:
  for (int i = 0; i < static_cast<int>(atomList.size()); i++)
    atomList.at(i).setPos(fracToCart(fracCoordsList.at(i)));
}

double Geometry::getVolume() const
{
  // Return the cell volume (3D) or the molecule hull volume (0D).
  if (is3D())
    return unitCell().volume();
  if (!is0D())
    return 0.0;

  std::vector<double> points;
  points.reserve(numAtoms() * 3);
  for (const auto& atm : atoms()) {
    points.push_back(atm.pos().x());
    points.push_back(atm.pos().y());
    points.push_back(atm.pos().z());
  }

  double volume = 0.0;
  if (!Common::convexHullVolume(points, static_cast<int>(numAtoms()), 3, volume))
    return 0.0;
  return volume;
}

double Geometry::getVolumePerAtom() const
{
  const double volume = getVolume();
  if (volume <= 0.0 || numAtoms() == 0)
    return 0.0;
  return volume / numAtoms();
}

void Geometry::wrapAtomsToCell()
{
  // 3D only.
  if (!is3D()) return;
  clearGeometryCaches();
  // Store position of atoms in fractional units
  std::vector<Atom>& atomList = atoms();
  QList<Common::Vector3> fracCoordsList;
  for (const auto& atm : atomList)
    fracCoordsList.append(cartToFrac(atm.pos()));

  // wrap fractional coordinates to [0,1)
  for (auto& fc : fracCoordsList)
    fc = unitCell().wrapFractional(fc);

  // Recalculate cartesian coordinates:
  for (int i = 0; i < static_cast<int>(atomList.size()); i++)
    atomList.at(i).setPos(fracToCart(fracCoordsList.at(i)));
}

void Geometry::wrapBondedComponentsToSmallestBonds()
{
  // 3D only.
  if (!hasBonds() || !is3D())
    return;
  clearGeometryCaches();

  std::vector<bool> atomAlreadyMoved(numAtoms(), false);

  std::vector<size_t> atomsToCheck(1, 0);
  atomAlreadyMoved[0] = true;

  while (!atomsToCheck.empty()) {
    size_t checkInd = atomsToCheck[0];
    for (size_t i = 0; i < numAtoms(); ++i) {
      if (atomAlreadyMoved[i] || checkInd == i)
        continue;

      if (areBonded(checkInd, i)) {
        const auto& pos1 = atom(checkInd).pos();
        const auto& pos2 = atom(i).pos();
        atom(i).setPos(unitCell().minimumImage(pos2 - pos1) + pos1);
        atomAlreadyMoved[i] = true;
        atomsToCheck.push_back(i);
      }
    }
    atomsToCheck.erase(atomsToCheck.begin());

    // Start the next bonded group once this one is done.
    if (atomsToCheck.empty()) {
      auto it =
        std::find(atomAlreadyMoved.begin(), atomAlreadyMoved.end(), false);

      if (it == atomAlreadyMoved.end())
        break;

      size_t newInd = it - atomAlreadyMoved.begin();
      atomAlreadyMoved[newInd] = true;
      atomsToCheck.push_back(newInd);
    }
  }
}

bool Geometry::rotateCellAndCoordsToStandardOrientation()
{
  // 3D only.
  if (!is3D()) {
    Common::error("Standard orientation is applicable only to a crystal");
    return false;
  }

  // Cache fractional coordinates
  QList<Common::Vector3> fcoords;
  for (auto it = atoms().begin(), it_end = atoms().end(); it != it_end; ++it)
    fcoords.append(cartToFrac(it->pos()));

  Common::Matrix3 newMat = unitCell().cellMatrix();
  if (!rotateCellAndCoordsToStandardOrientation(newMat, fcoords, false))
    return false;

  // Set the rotated basis
  setCellInfo(newMat);

  // Reset coords
  Q_ASSERT(static_cast<int>(atoms().size()) == fcoords.size());
  for (int i = 0; i < static_cast<int>(atoms().size()); ++i)
    atom(i).setPos(fracToCart(fcoords[i]));

  return true;
}

bool Geometry::rotateCellAndCoordsToStandardOrientation(Common::Matrix3& cell,
                                                        QList<Common::Vector3>& fractionalCoordinates,
                                                        bool positiveHandedness)
{
  // This is a 3D only function. We reject the rotation if cell is:
  //  - an empty zero matrix,
  //  - any singular or nearly singular matrix,
  //  - a non-finite determinant.
  if (!isCellMatrixUsable(cell))
    return false;

  if (positiveHandedness && cell.determinant() < 0.0) {
    cell.setRow(2, -cell.row(2));
    for (auto& coordinates : fractionalCoordinates)
      coordinates[2] = -coordinates[2];
  }

  Geometry geometry;
  geometry.setCellInfo(cell);
  Common::Matrix3 newMat = geometry.getCellMatrixInStandardOrientation();
  if (newMat.isZero())
    return false;

  cell = newMat;
  return true;
}

Common::Matrix3 Geometry::getCellMatrixInStandardOrientation() const
{
  // 3D only.
  if (!is3D()) return Common::Matrix3::Zero();

  Common::Matrix3 origRowMat = unitCell().cellMatrix();

  // Extract vector components:
  const double& x1 = origRowMat(0, 0);
  const double& y1 = origRowMat(0, 1);
  const double& z1 = origRowMat(0, 2);

  const double& x2 = origRowMat(1, 0);
  const double& y2 = origRowMat(1, 1);
  const double& z2 = origRowMat(1, 2);

  const double& x3 = origRowMat(2, 0);
  const double& y3 = origRowMat(2, 1);
  const double& z3 = origRowMat(2, 2);

  // Cache some frequently used values:
  // Length of v1
  const double L1 = std::sqrt(x1 * x1 + y1 * y1 + z1 * z1);
  // Squared norm of v1's yz projection
  const double sqrdnorm1yz = y1 * y1 + z1 * z1;
  // Squared norm of v2's yz projection
  const double sqrdnorm2yz = y2 * y2 + z2 * z2;
  // Determinant of v1 and v2's projections in yz plane
  const double detv1v2yz = y2 * z1 - y1 * z2;
  // Scalar product of v1 and v2's projections in yz plane
  const double dotv1v2yz = y1 * y2 + z1 * z2;

  // Used for denominators, since we want to check that they are
  // sufficiently far from 0 to keep things reasonable:
  double denom;
  const double DENOM_TOL = ZERO05;

  // Create target matrix, fill with zeros
  Common::Matrix3 newMat(Common::Matrix3::Zero());

  // Set components of new v1:
  newMat(0, 0) = L1;

  // Set components of new v2:
  denom = L1;
  if (std::fabs(denom) < DENOM_TOL)
    return Common::Matrix3::Zero();
  newMat(1, 0) = (x1 * x2 + y1 * y2 + z1 * z2) / denom;

  newMat(1, 1) =
    std::sqrt(x2 * x2 * sqrdnorm1yz + detv1v2yz * detv1v2yz -
              2 * x1 * x2 * dotv1v2yz + x1 * x1 * sqrdnorm2yz) /
    denom;

  // Set components of new v3
  // denom is still L1
  // Q_ASSERT(denom == L1);
  newMat(2, 0) = (x1 * x3 + y1 * y3 + z1 * z3) / denom;

  denom = L1 * L1 * newMat(1, 1);
  if (std::fabs(denom) < DENOM_TOL)
    return Common::Matrix3::Zero();
  newMat(2, 1) = (x1 * x1 * (y2 * y3 + z2 * z3) +
                  x2 * (x3 * sqrdnorm1yz - x1 * (y1 * y3 + z1 * z3)) +
                  detv1v2yz * (y3 * z1 - y1 * z3) - x1 * x3 * dotv1v2yz) /
                 denom;

  denom = L1 * newMat(1, 1);
  if (std::fabs(denom) < DENOM_TOL)
    return Common::Matrix3::Zero();
  // Numerator is determinant of original cell:
  newMat(2, 2) = (x1 * y2 * z3 - x1 * y3 * z2 + x2 * y3 * z1 -
                  x2 * y1 * z3 + x3 * y1 * z2 - x3 * y2 * z1) /
                 denom;

  return newMat;
}

bool Geometry::getShortestInteratomicDistance(double& shortest) const
{
  // 3D or 0D.
  if (!is3D() && !is0D()) return false;
  const std::vector<Atom>& atomList = atoms();
  if (atomList.size() <= 1)
    return false;

  QList<Common::Vector3> atomPositions;
  for (const auto& atm : atomList)
    atomPositions.push_back(atm.pos());

  Common::Vector3 v1 = atomPositions.at(0);
  Common::Vector3 v2 = atomPositions.at(1);
  // Add periodic images for a 3D structure (for a molecule this remains empty)
  QList<Common::Vector3> uVecs;
  if (is3D()) {
    Common::Matrix3 cellMatrix = unitCell().cellMatrix();
    Common::Vector3 u1 = cellMatrix.row(0);
    Common::Vector3 u2 = cellMatrix.row(1);
    Common::Vector3 u3 = cellMatrix.row(2);

    for (int s_1 = -1; s_1 <= 1; s_1++)
      for (int s_2 = -1; s_2 <= 1; s_2++)
        for (int s_3 = -1; s_3 <= 1; s_3++)
          uVecs.append(s_1 * u1 + s_2 * u2 + s_3 * u3);
  }

  shortest = std::fabs((v1 - v2).norm());
  double distance;

  for (int i = 0; i < static_cast<int>(atomList.size()); i++) {
    v1 = atomPositions.at(i);
    for (int j = i + 1; j < static_cast<int>(atomList.size()); j++) {
      v2 = atomPositions.at(j);
      distance = std::fabs((v1 - v2).norm());
      if (distance < shortest)
        shortest = distance;
      for (int vecInd = 0; vecInd < uVecs.size(); vecInd++) {
        distance = std::fabs(((v1 + uVecs.at(vecInd)) - v2).norm());
        if (distance < shortest)
          shortest = distance;
      }
    }
  }
  return true;
}

bool Geometry::getShortestInteratomicDistancesBySpecies(QList<QString>& symbol1,
                                                        QList<QString>& symbol2,
                                                        QList<double>& distance) const
{
  symbol1.clear();
  symbol2.clear();
  distance.clear();

  // 3D or 0D.
  if (!is3D() && !is0D()) return false;
  const std::vector<Atom>& atomList = atoms();
  if (atomList.size() <= 1)
    return false;

  const QList<QString> symbols = getSymbols();
  const int nsymbs = symbols.size();
  if (nsymbs == 0)
    return false;

  // Species index of each atom.
  std::vector<int> speciesOf(atomList.size());
  for (size_t i = 0; i < atomList.size(); i++)
    speciesOf[i] = symbols.indexOf(
      Atoms::ElementInfo::getAtomicSymbol(atomList[i].atomicNumber()).c_str());

  // Add periodic images for a 3D structure (for a molecule this remains empty).
  QList<Common::Vector3> uVecs;
  if (is3D()) {
    Common::Matrix3 cellMatrix = unitCell().cellMatrix();
    Common::Vector3 u1 = cellMatrix.row(0);
    Common::Vector3 u2 = cellMatrix.row(1);
    Common::Vector3 u3 = cellMatrix.row(2);
    for (int s_1 = -1; s_1 <= 1; s_1++)
      for (int s_2 = -1; s_2 <= 1; s_2++)
        for (int s_3 = -1; s_3 <= 1; s_3++)
          uVecs.append(s_1 * u1 + s_2 * u2 + s_3 * u3);
  }

  // Store the shortest distance for each atom type pair.
  std::vector<std::vector<double> > shortest(
    nsymbs, std::vector<double>(nsymbs, DBL_MAX));

  for (size_t i = 0; i < atomList.size(); i++) {
    const Common::Vector3 v1 = atomList[i].pos();
    const int si = speciesOf[i];
    for (size_t j = i; j < atomList.size(); j++) {
      const Common::Vector3 v2 = atomList[j].pos();
      const int sj = speciesOf[j];
      const int a = std::min(si, sj);
      const int b = std::max(si, sj);

      // Different atoms: the plain distance also counts.
      if (i != j) {
        const double d = std::fabs((v1 - v2).norm());
        if (d < shortest[a][b])
          shortest[a][b] = d;
      }

      for (int vecInd = 0; vecInd < uVecs.size(); vecInd++) {
        // Skip the zero image of an atom with itself (distance 0).
        if (i == j && uVecs.at(vecInd).norm() < ZERO08)
          continue;
        const double d = std::fabs(((v1 + uVecs.at(vecInd)) - v2).norm());
        if (d < shortest[a][b])
          shortest[a][b] = d;
      }
    }
  }

  // Return the pairs in symbol order. A missing pair has zero distance.
  for (int a = 0; a < nsymbs; a++) {
    for (int b = a; b < nsymbs; b++) {
      symbol1.append(symbols.at(a));
      symbol2.append(symbols.at(b));
      distance.append(shortest[a][b] == DBL_MAX ? 0.0 : shortest[a][b]);
    }
  }

  return true;
}

bool Geometry::getNearestNeighborDistance(double x, double y, double z, double& shortest) const
{
  // 3D or 0D.
  if (!is3D() && !is0D()) return false;
  const std::vector<Atom>& atomList = atoms();
  if (atomList.size() < 1)
    return false;

  Common::Vector3 v1(x, y, z);
  // Add periodic images for a 3D structure (for a molecule this remains empty).
  QList<Common::Vector3> uVecs;
  if (is3D()) {
    Common::Vector3 aVec = unitCell().aVector();
    Common::Vector3 bVec = unitCell().bVector();
    Common::Vector3 cVec = unitCell().cVector();
    for (int s_1 = -1; s_1 <= 1; s_1++)
      for (int s_2 = -1; s_2 <= 1; s_2++)
        for (int s_3 = -1; s_3 <= 1; s_3++)
          uVecs.append(s_1 * aVec + s_2 * bVec + s_3 * cVec);
  }

  shortest = std::fabs((v1 - atom(0).pos()).norm());
  double distance;

  for (int j = 0; j < static_cast<int>(atomList.size()); j++) {
    const Common::Vector3& v2 = atomList.at(j).pos();
    distance = std::fabs((v1 - v2).norm());
    if (distance < shortest)
      shortest = distance;
    for (int vecInd = 0; vecInd < uVecs.size(); vecInd++) {
      distance = std::fabs(((v2 + uVecs.at(vecInd)) - v1).norm());
      if (distance < shortest)
        shortest = distance;
    }
  }
  return true;
}

bool Geometry::getSquaredAtomicDistancesToPoint(const Common::Vector3& coord,
                                                 QList<double>* distances) const
{
  // 3D or 0D.
  if (!is3D() && !is0D()) return false;
  int atmCount = static_cast<int>(numAtoms());
  if (atmCount < 1)
    return false;

  distances->clear();
  distances->reserve(atmCount);
  for (int i = 0; i < atmCount; ++i)
    distances->append(0.0);

  // Add periodic images for a 3D structure (for a molecule this remains empty).
  QList<Common::Vector3> uVecs;
  if (is3D()) {
    const Common::Vector3 aVec(unitCell().aVector());
    const Common::Vector3 bVec(unitCell().bVector());
    const Common::Vector3 cVec(unitCell().cVector());

    uVecs.reserve(27);
    for (short s1 = -1; s1 <= 1; ++s1)
      for (short s2 = -1; s2 <= 1; ++s2)
        for (short s3 = -1; s3 <= 1; ++s3)
          uVecs.append(s1 * aVec + s2 * bVec + s3 * cVec);
  } else {
    uVecs.append(Common::Vector3(0.0, 0.0, 0.0));
  }

  for (int i = 0; i < atmCount; ++i) {
    const Common::Vector3 pos = atom(i).pos();
    double shortest = DBL_MAX;
    for (auto it = uVecs.constBegin(), it_end = uVecs.constEnd(); it != it_end; ++it) {
      double current = ((*it + pos) - coord).squaredNorm();
      if (current < shortest)
        shortest = current;
    }
    (*distances)[i] = shortest;
  }

  return true;
}

bool Geometry::addAtomRandomly(unsigned int atomicNumber, double minIAD,
                               int maxAttempts)
{
  // 3D only.
  if (!is3D()) return false;
  double IAD = -1;
  int i = 0;
  Common::Vector3 coords;

  // For first atom, add to 0, 0, 0
  if (numAtoms() == 0) {
    coords = Common::Vector3(0, 0, 0);
  } else {
    do {
      // Generate fractional coordinates
      IAD = -1;
      double x = Common::getRandDouble();
      double y = Common::getRandDouble();
      double z = Common::getRandDouble();
      coords = Common::Vector3(x, y, z);

      // Convert to cartesian coordinates and store
      coords = unitCell().toCartesian(coords);

      if (minIAD >= 0.0) {
        // Check the distance at the actual (cartesian) placement point.
        getNearestNeighborDistance(coords[0], coords[1], coords[2], IAD);
      } else {
        break;
      }
      i++;
    } while (i < maxAttempts && IAD <= minIAD);

    // A negative minIAD means no distance check was requested.
    if (minIAD >= 0.0 && IAD <= minIAD)
      return false;
  }

  Atom& atom = addAtom();
  atom.setPos(coords);
  atom.setAtomicNumber(atomicNumber);
  return true;
}

QList<QString> Geometry::getAtomicSymbolsInOrder() const
{
  QList<QString> result;
  for (const auto& atom : atoms())
    result << Atoms::ElementInfo::getAtomicSymbol(atom.atomicNumber()).c_str();
  return result;
}

std::vector<Common::Vector3> Geometry::getAtomCoordsFrac() const
{
  // 3D only.
  if (!is3D()) return std::vector<Common::Vector3>();
  std::vector<Common::Vector3> list;
  QList<QString> symbols = getSymbols();
  for (const auto& symbolRef : symbols) {
    for (const auto& atom : atoms()) {
      QString symbolCur = Atoms::ElementInfo::getAtomicSymbol(atom.atomicNumber()).c_str();
      if (symbolCur == symbolRef)
        list.push_back(unitCell().toFractional(atom.pos()));
    }
  }
  return list;
}

bool Geometry::compareXtalComp(const Geometry& other, double lengthTol, double angleTol) const
{
  // 3D only.
  if (!is3D() || !other.is3D()) return false;
  const Common::Matrix3 thisCell(unitCell().cellMatrix());
  const Common::Matrix3 otherCell(other.unitCell().cellMatrix());
  XcMatrix thisCellXc(thisCell(0, 0), thisCell(0, 1), thisCell(0, 2),
                      thisCell(1, 0), thisCell(1, 1), thisCell(1, 2),
                      thisCell(2, 0), thisCell(2, 1), thisCell(2, 2));
  XcMatrix otherCellXc(otherCell(0, 0), otherCell(0, 1), otherCell(0, 2),
                       otherCell(1, 0), otherCell(1, 1), otherCell(1, 2),
                       otherCell(2, 0), otherCell(2, 1), otherCell(2, 2));

  std::vector<XcVector> thisCoords;
  std::vector<XcVector> otherCoords;
  std::vector<unsigned int> thisTypes;
  std::vector<unsigned int> otherTypes;
  thisCoords.reserve(numAtoms());
  thisTypes.reserve(numAtoms());
  otherCoords.reserve(other.numAtoms());
  otherTypes.reserve(other.numAtoms());

  for (const auto& atom : atoms()) {
    Common::Vector3 pos = cartToFrac(atom.pos());
    thisCoords.push_back(XcVector(pos.x(), pos.y(), pos.z()));
    thisTypes.push_back(atom.atomicNumber());
  }
  for (const auto& atom : other.atoms()) {
    Common::Vector3 pos = other.unitCell().toFractional(atom.pos());
    otherCoords.push_back(XcVector(pos.x(), pos.y(), pos.z()));
    otherTypes.push_back(atom.atomicNumber());
  }

  return XtalComp::compare(thisCellXc, thisTypes, thisCoords, otherCellXc,
                           otherTypes, otherCoords, nullptr, lengthTol,
                           angleTol);
}

bool Geometry::generateIADHistogram(std::vector<double>* distance,
                                     std::vector<double>* frequency, double min,
                                     double max, double step, const Atom& atom) const
{
  // 3D or 0D.
  if (!is3D() && !is0D()) return false;
  distance->clear();
  frequency->clear();

  if (min > max && step > 0)
    return false;
  if (step <= 0)
    return false;

  double halfstep = step / 2.0;

  double val = min;
  do {
    distance->push_back(val);
    frequency->push_back(0.0);
    val += step;
  } while (val < max);

  const std::vector<Atom>& atomList = atoms();
  if (atomList.empty())
    return false;
  if (atom.atomicNumber() == 0 && atomList.size() < 2)
    return false;

  std::vector<Common::Vector3> atomPositions;
  for (const auto& atm : atomList)
    atomPositions.push_back(atm.pos());

  // Periodic images exist only for 3D; for 0D the list stays empty and the
  //   plain distances below are all there is. Both loops below compute the
  //   plain distance first and skip the near-zero translation.
  std::vector<Common::Vector3> translations;
  if (is3D()) {
    Common::Matrix3 cellMatrix = unitCell().cellMatrix();
    Common::Vector3 u1 = cellMatrix.row(0);
    Common::Vector3 u2 = cellMatrix.row(1);
    Common::Vector3 u3 = cellMatrix.row(2);

    for (int s1 = -1; s1 <= 1; s1++)
      for (int s2 = -1; s2 <= 1; s2++)
        for (int s3 = -1; s3 <= 1; s3++)
          translations.push_back(s1 * u1 + s2 * u2 + s3 * u3);
  }

  Common::Vector3 v1;
  Common::Vector3 v2;
  double diff;

  if (atom.atomicNumber() == 0) {
    for (int i = 0; i < static_cast<int>(atomList.size()); i++) {
      NNHistMap m;
      m.i = i;
      m.halfstep = halfstep;
      m.atomPositions = &atomPositions;
      m.translations = &translations;
      m.dist = distance;

      reduceNNHistChunks(*frequency, calcNNHistChunk(m));
    }
  } else {
    v1 = atom.pos();
    for (int j = 0; j < static_cast<int>(atomList.size()); j++) {
      v2 = atomPositions.at(j);
      diff = std::fabs((v1 - v2).norm());

      for (int k = 0; k < static_cast<int>(distance->size()); k++) {
        double radius = distance->at(k);
        if (diff != 0 && Common::fuzzyCompare(diff, radius, halfstep))
          (*frequency)[k]++;
      }

      for (int t = 0; t < static_cast<int>(translations.size()); t++) {
        if (translations.at(t).norm() < ZERO08)
          continue;
        diff = std::fabs((v1 - (v2 + translations.at(t))).norm());
        for (int k = 0; k < static_cast<int>(distance->size()); k++) {
          double radius = distance->at(k);
          if (Common::fuzzyCompare(diff, radius, halfstep))
            (*frequency)[k]++;
        }
      }
    }
  }

  return true;
}

bool Geometry::compareIADDistributions(const std::vector<double>& d,
                                        const std::vector<double>& f1,
                                        const std::vector<double>& f2,
                                        double decay, double smear,
                                        double* error)
{
  // Check that smearing is possible
  if (smear != 0 && d.size() <= 1) {
    Common::error(QString("%1: Cannot smear with 1 or fewer points.")
                   .arg(__func__));
    return false;
  }
  // Check sizes
  if (d.size() != f1.size() || f1.size() != f2.size()) {
    Common::error(QString("%1: Vectors are not the same size.")
                   .arg(__func__));
    return false;
  }

  // Perform a boxcar smoothing over range set by "smear"
  // First determine step size of d, then convert smear to index units
  double stepSize = std::fabs(d.at(1) - d.at(0));
  int boxSize = std::ceil(smear / stepSize);

  if (boxSize > static_cast<int>(d.size())) {
    Common::error(QString("%1: Smear length is greater then d vector range.")
                   .arg(__func__));
    return false;
  }
  // Smear
  std::vector<double> f1s, f2s, ds; // smeared vectors

  if (smear != 0) {
    double f1t, f2t, dt; // temporary variables
    for (int i = 0; i < static_cast<int>(d.size()) - boxSize; i++) {
      f1t = f2t = dt = 0;
      for (int j = 0; j < boxSize; j++) {
        f1t += f1.at(i + j);
        f2t += f2.at(i + j);
        dt += d.at(i + j);
      }
      f1s.push_back(f1t / double(boxSize));
      f2s.push_back(f2t / double(boxSize));
      ds.push_back(dt / double(boxSize));
    }
  } else {
    for (int i = 0; i < static_cast<int>(d.size()) - boxSize; i++) {
      f1s.push_back(f1.at(i));
      f2s.push_back(f2.at(i));
      ds.push_back(d.at(i));
    }
  }

  // Calculate diff vector
  std::vector<double> diff;
  for (int i = 0; i < static_cast<int>(ds.size()); i++)
    diff.push_back(std::fabs(f1s.at(i) - f2s.at(i)));

  // Calculate decay function: Standard exponential decay with a
  // halflife of decay. If decay==0, no decay.
  double decayFactor = 0;
  // ln(2) / decay:
  if (decay != 0)
    decayFactor = 0.69314718055994530941723 / decay;

  // Calculate error:
  (*error) = 0;
  for (int i = 0; i < static_cast<int>(ds.size()); i++)
    (*error) += std::exp(-decayFactor * ds.at(i)) * diff.at(i);

  return true;
}

bool Geometry::calculateNearestNeighborLists(double cutoff)
{
  // 3D or 0D.
  if (!is3D() && !is0D()) return false;
  if (hasNearestNeighborLists() && cutoff == g_neighbor_list_cutoff)
    return true;

  QList<QString> symbols = getSymbols();
  int nsymbs = symbols.size();
  if (numAtoms() == 0 || nsymbs == 0)
    return false;

  std::vector<std::vector<std::pair<int, double> > > nnlist;
  nnlist.clear();
  nnlist.resize(numAtoms());

  std::vector<Common::Vector3> vecs = {
    unitCell().aVector(),
    unitCell().bVector(),
    unitCell().cVector()
  };

  // Find the periodic images to search (for 0D geometry loop reduces to 0,0,0).
  std::vector<int> expan = { 0, 0, 0 };
  if (is3D()) {
    double vol = vecs[0].dot(vecs[1].cross(vecs[2]));
    if (!GS_ISFINITE(vol) || std::fabs(vol) <= ZERO08)
      return false;
    for (int i = 0; i < 3; i++) {
      Common::Vector3 rec = vecs[(i + 1) % 3].cross(vecs[(i + 2) % 3]) / vol;
      expan[i] = std::ceil(cutoff * rec.norm());
    }
  }

  for (int i = 0; i < static_cast<int>(atoms().size()); i++) {
    const Common::Vector3& v0 = atoms()[i].pos();
    for (int j = 0; j < static_cast<int>(atoms().size()); j++) {
      const Common::Vector3& v1 = atoms()[j].pos();
      for (int s1 = -expan[0]; s1 <= expan[0]; s1++) {
        for (int s2 = -expan[1]; s2 <= expan[1]; s2++) {
          for (int s3 = -expan[2]; s3 <= expan[2]; s3++) {
            if ((s1 == 0 && s2 == 0 && s3 == 0) && i == j)
              continue;

            Common::Vector3 tVec = s1 * vecs[0] + s2 * vecs[1] + s3 * vecs[2];
            double distance = std::fabs((v1 + tVec - v0).norm());
            if (distance < cutoff) {
              nnlist.at(i).emplace_back(j, distance);
            }
          }
        }
      }
    }
    // Sort the neighbors list based on the distances (ascending)
    std::sort(nnlist.at(i).begin(), nnlist.at(i).end(),
              [](const std::pair<int,double>& a, const std::pair<int,double>& b) {
        return a.second < b.second;
    });
  }

  g_neighbor_list = nnlist;
  g_neighbor_list_cutoff = cutoff;

  return true;
}

bool Geometry::calculateNearestNeighborListsCellList(double cutoff)
{
  // 3D or 0D.
  if (!is3D() && !is0D()) return false;
  if (hasNearestNeighborLists() && cutoff == g_neighbor_list_cutoff)
    return true;

  QList<QString> symbols = getSymbols();
  int nsymbs = symbols.size();
  if (numAtoms() == 0 || nsymbs == 0)
    return false;

  const int nAtoms = static_cast<int>(atoms().size());
  std::vector<std::vector<std::pair<int, double> > > nnlist;
  nnlist.resize(nAtoms);

  std::vector<Common::Vector3> vecs = {
    unitCell().aVector(),
    unitCell().bVector(),
    unitCell().cVector()
  };

  // Add periodic images to the grid.
  std::vector<int> expan = { 0, 0, 0 };
  if (is3D()) {
    double vol = vecs[0].dot(vecs[1].cross(vecs[2]));
    if (!GS_ISFINITE(vol) || std::fabs(vol) <= ZERO08)
      return false;
    for (int i = 0; i < 3; i++) {
      Common::Vector3 rec = vecs[(i + 1) % 3].cross(vecs[(i + 2) % 3]) / vol;
      expan[i] = std::ceil(cutoff * rec.norm());
    }
  }

  // Add atoms and their periodic images.
  struct ImageAtom
  {
    int index;
    bool origin;
    Common::Vector3 pos;
  };
  std::vector<ImageAtom> images;
  images.reserve(static_cast<size_t>(nAtoms) * (2 * expan[0] + 1) *
                 (2 * expan[1] + 1) * (2 * expan[2] + 1));

  for (int j = 0; j < nAtoms; j++) {
    const Common::Vector3& v1 = atoms()[j].pos();
    for (int s1 = -expan[0]; s1 <= expan[0]; s1++) {
      for (int s2 = -expan[1]; s2 <= expan[1]; s2++) {
        for (int s3 = -expan[2]; s3 <= expan[2]; s3++) {
          const bool origin = (s1 == 0 && s2 == 0 && s3 == 0);
          images.push_back(
            { j, origin, v1 + s1 * vecs[0] + s2 * vecs[1] + s3 * vecs[2] });
        }
      }
    }
  }

  // Bounding box of all the image atoms.
  Common::Vector3 lo = images.front().pos;
  Common::Vector3 hi = images.front().pos;
  for (const ImageAtom& a : images) {
    for (int d = 0; d < 3; d++) {
      lo[d] = std::min(lo[d], a.pos[d]);
      hi[d] = std::max(hi[d], a.pos[d]);
    }
  }

  // Make a grid of cubic cells with sides >= cutoff: every neighboring atom within
  //   the cutoff will be in one of these cells.
  const double side = (cutoff > 0.0) ? cutoff : 1.0;
  const int nx = std::max(1, static_cast<int>((hi[0] - lo[0]) / side) + 1);
  const int ny = std::max(1, static_cast<int>((hi[1] - lo[1]) / side) + 1);
  const int nz = std::max(1, static_cast<int>((hi[2] - lo[2]) / side) + 1);

  auto cellOf = [&](const Common::Vector3& p, int& cx, int& cy, int& cz) {
    cx = std::min(nx - 1, std::max(0, static_cast<int>((p[0] - lo[0]) / side)));
    cy = std::min(ny - 1, std::max(0, static_cast<int>((p[1] - lo[1]) / side)));
    cz = std::min(nz - 1, std::max(0, static_cast<int>((p[2] - lo[2]) / side)));
  };

  auto flat = [&](int cx, int cy, int cz) {
    return (static_cast<size_t>(cz) * ny + cy) * nx + cx;
  };

  // Bin the image atoms into the grid.
  std::vector<std::vector<int> > grid(static_cast<size_t>(nx) * ny * nz);

  for (int k = 0; k < static_cast<int>(images.size()); k++) {
    int cx, cy, cz;
    cellOf(images[k].pos, cx, cy, cz);
    grid[flat(cx, cy, cz)].push_back(k);
  }

  // Check nearby cells for each atom.
  const double cutoff2 = cutoff * cutoff;
  for (int i = 0; i < nAtoms; i++) {
    const Common::Vector3& v0 = atoms()[i].pos();
    int cx, cy, cz;
    cellOf(v0, cx, cy, cz);

    for (int dx = -1; dx <= 1; dx++) {
      const int gx = cx + dx;
      if (gx < 0 || gx >= nx) continue;
      for (int dy = -1; dy <= 1; dy++) {
        const int gy = cy + dy;
        if (gy < 0 || gy >= ny) continue;
        for (int dz = -1; dz <= 1; dz++) {
          const int gz = cz + dz;
          if (gz < 0 || gz >= nz) continue;
          for (int k : grid[flat(gx, gy, gz)]) {
            const ImageAtom& a = images[k];
            // Skip the atom itself, but keep its periodic images.
            if (a.index == i && a.origin)
              continue;
            const double dist2 = (a.pos - v0).squaredNorm();
            if (dist2 < cutoff2)
              nnlist.at(i).emplace_back(a.index, std::sqrt(dist2));
          }
        }
      }
    }

    // Sort the neighbors list based on the distances (ascending)
    std::sort(nnlist.at(i).begin(), nnlist.at(i).end(),
              [](const std::pair<int, double>& a, const std::pair<int, double>& b) {
                return a.second < b.second;
              });
  }

  g_neighbor_list = nnlist;
  g_neighbor_list_cutoff = cutoff;

  return true;
}

bool Geometry::calculateNormalizedRDF(int nbins, double cutoff, double sigma)
{
  // 3D or 0D; the neighbor lists below handle the periodic images.
  if (!is3D() && !is0D()) return false;
  if (numAtoms() == 0 || getSymbols().size() == 0)
    return false;
  if (hasNormalizedRDF() && nbins == g_norm_rdf_nbins &&
      cutoff == g_norm_rdf_cutoff && sigma == g_norm_rdf_sigma)
    return true;

  // Timed after verifying the cache; so the call count shows real RDF calculations.
  Common::ScopedTimer _timer("Geometry::calculateRDF");

  QList<QString> symbols = getSymbols();
  int nsymbs = symbols.size();
  int npairs = nsymbs * (nsymbs + 1) / 2;

  double delta      = cutoff / nbins;
  int    del_bin    = 3 * std::ceil(sigma / delta);
  double gaus_fact  = -0.5 / (sigma * sigma);
  double softcutoff = 0.99 * cutoff;

  if (!calculateNearestNeighborLists(cutoff))
    return false;
  std::vector<std::vector<std::pair<int, double> > > nnlist = getNearestNeighborLists();

  // We calculate rdf as double (cached values are float). The flat vector is
  //   bin-major: value for bin "b" and species pair "p" is [b * npairs + p].
  std::vector<double> rdf(static_cast<size_t>(nbins) * npairs, 0.0);

  for (int i = 0; i < static_cast<int>(atoms().size()); i++) {
    int indx0 = symbols.indexOf(
      Atoms::ElementInfo::getAtomicSymbol(atoms()[i].atomicNumber()).c_str());
    for (int j = 0; j < static_cast<int>(nnlist.at(i).size()); j++) {
      int    a1 = nnlist.at(i).at(j).first;
      double d1 = nnlist.at(i).at(j).second;
      int indx1 = symbols.indexOf(
        Atoms::ElementInfo::getAtomicSymbol(atoms()[a1].atomicNumber()).c_str());
      if (indx1 < indx0)
        continue;
      // Index of the (indx0, indx1) species pair within the ordered unique pairs
      int pair = indx0 * nsymbs - (indx0 * (indx0 - 1)) / 2 + (indx1 - indx0);
      int binn = std::floor(d1 / delta);
      for (int b = binn - del_bin; b <= binn + del_bin; b++) {
        if (b < 0 || b >= nbins)
          continue;
        double coor = b * delta;
        double smooth = 1.0;
        if (coor >= softcutoff) {
          smooth = std::cos(0.5 * PI * (coor - softcutoff) / (cutoff - softcutoff));
        }
        rdf[static_cast<size_t>(b) * npairs + pair] += smooth * std::exp(gaus_fact * (d1 - coor) * (d1 - coor));
      }
    }
  }

  // Adjust for double-counting of same-species
  for (int j = 0; j < nsymbs; j++) {
    int pair = j * nsymbs - (j * (j - 1)) / 2;
    for (int i = 0; i < nbins; i++)
      rdf[static_cast<size_t>(i) * npairs + pair] *= 0.5;
  }

  double norm = 0.0;
  for (size_t i = 0; i < rdf.size(); i++)
    norm += rdf[i] * rdf[i];

  if (norm < ZERO06)
    return false;

  norm = 1.0 / std::sqrt(norm);
  for (size_t i = 0; i < rdf.size(); i++)
    rdf[i] *= norm;

  g_norm_rdf.assign(rdf.begin(), rdf.end());
  g_norm_rdf_nsymbs = nsymbs;
  g_norm_rdf_nbins = nbins;
  g_norm_rdf_cutoff = cutoff;
  g_norm_rdf_sigma = sigma;
  return true;
}

bool Geometry::calculateTotalNormalizedRDF(int nbins, double cutoff, double sigma,
                                            std::vector<double>& total)
{
  // 3D or 0D.
  if (!is3D() && !is0D()) return false;
  // This returns right away when the saved RDF matches these parameters.
  if (!calculateNormalizedRDF(nbins, cutoff, sigma))
    return false;

  int npairs = g_norm_rdf_nsymbs * (g_norm_rdf_nsymbs + 1) / 2;

  if (nbins <= 0 || npairs <= 0 ||
      g_norm_rdf.size() != static_cast<size_t>(nbins) * npairs)
    return false;

  total.assign(nbins, 0.0);

  for (int i = 0; i < nbins; i++) {
    for (int p = 0; p < npairs; p++)
      total[i] += g_norm_rdf[static_cast<size_t>(i) * npairs + p];
  }

  return true;
}

bool Geometry::compareRDF(Geometry& other, int nbins, double cutoff, double sigma, double tolerance,
                                       double& dotProduct)
{
  // 3D or 0D, and both must have the same dimension: a cluster and a
  //   crystal are never similar.
  if (!is3D() && !is0D()) return false;
  if (dimension() != other.dimension()) return false;
  dotProduct = 0.0;

  if (getSymbols() != other.getSymbols())
    return false;

  if (!calculateNormalizedRDF(nbins, cutoff, sigma) ||
      !other.calculateNormalizedRDF(nbins, cutoff, sigma))
    return false;

  const std::vector<float>& rdfi = g_norm_rdf;
  const std::vector<float>& rdfj = other.g_norm_rdf;
  if (rdfi.size() != rdfj.size())
    return false;

  for (size_t i = 0; i < rdfi.size(); i++)
    dotProduct += static_cast<double>(rdfi[i]) * rdfj[i];

  return (dotProduct >= tolerance);
}

bool Geometry::niggliReduce(const unsigned int iterations, double lenTol)
{
  // 3D only.
  if (!is3D()) {
    Common::error("Niggli reduction is applicable only to a crystal");
    return false;
  }
  // Cache volume for later sanity checks
  const double origVolume = getVolume();

  // Grab lattice vectors
  const Common::Vector3& v1 = unitCell().aVector();
  const Common::Vector3& v2 = unitCell().bVector();
  const Common::Vector3& v3 = unitCell().cVector();

  // Compute characteristic (step 0)
  double A = v1.squaredNorm();
  double B = v2.squaredNorm();
  double C = v3.squaredNorm();
  double xi = 2 * v2.dot(v3);
  double eta = 2 * v1.dot(v3);
  double zeta = 2 * v1.dot(v2);

  // Return value
  bool ret = false;
  // comparison tolerance
  double tol = 0.001 * lenTol * std::pow(getVolume(), 2.0 / 3.0);

  // Initialize change of basis matrices:
  //
  // Although the reduction algorithm produces quantities directly
  // relatable to a,b,c,alpha,beta,gamma, we will calculate a change
  // of basis matrix to use instead, and discard A, B, C, xi, eta,
  // zeta. By multiplying the change of basis matrix against the
  // current cell matrix, we avoid the problem of handling the
  // orientation matrix already present in the cell. The inverse of
  // this matrix can also be used later to convert the atomic
  // positions.
  // tmpMat is used to build other matrices
  Common::Matrix3 tmpMat;
  // Cache static matrices:
  // Swap x,y (Used in Step 1). Negatives ensure proper sign of final
  // determinant.
  tmpMat << 0, -1, 0, -1, 0, 0, 0, 0, -1;
  const Common::Matrix3 C1(tmpMat);
  // Swap y,z (Used in Step 2). Negatives ensure proper sign of final
  // determinant
  tmpMat << -1, 0, 0, 0, 0, -1, 0, -1, 0;
  const Common::Matrix3 C2(tmpMat);
  // For step 8:
  tmpMat << 1, 0, 1, 0, 1, 1, 0, 0, 1;
  const Common::Matrix3 C8(tmpMat);
  // initial change of basis matrix
  tmpMat << 1, 0, 0, 0, 1, 0, 0, 0, 1;
  Common::Matrix3 cob(tmpMat);

// Enable debugging output here:
/*
#define NIGGLI_DEBUG(step) qDebug() << iter << step << A << B << C << xi << eta
<< zeta << cob.determinant();
    std::cout << cob << std::endl;
*/
#define NIGGLI_DEBUG(step)

  unsigned int iter;
  for (iter = 0; iter < iterations; ++iter) {
    Q_ASSERT(std::fabs(cob.determinant() - 1.0) < ZERO05);

    // Step 1:
    if (Common::gt(A, B, tol) ||
        (Common::eq(A, B, tol) &&
         Common::gt(std::fabs(xi), std::fabs(eta), tol))) {
      cob *= C1;
      qSwap(A, B);
      qSwap(xi, eta);
      NIGGLI_DEBUG(1);
      ++iter;
    }

    // Step 2:
    if (Common::gt(B, C, tol) ||
        (Common::eq(B, C, tol) &&
         Common::gt(std::fabs(eta), std::fabs(zeta), tol))) {
      cob *= C2;
      qSwap(B, C);
      qSwap(eta, zeta);
      NIGGLI_DEBUG(2);
      continue;
    }

    // Step 3:
    // Use exact comparisons in steps 3 and 4.
    if (xi * eta * zeta > 0) {
      // Update change of basis matrix:
      tmpMat << Common::sign(xi), 0, 0, 0, Common::sign(eta), 0, 0, 0,
        Common::sign(zeta);
      cob *= tmpMat;

      // Update characteristic
      xi = std::fabs(xi);
      eta = std::fabs(eta);
      zeta = std::fabs(zeta);
      NIGGLI_DEBUG(3);
      ++iter;
    // Step 4:
    // Use exact comparisons for steps 3 and 4
    } else { // either step 3 or 4 should run
      // Update change of basis matrix:
      double* p = nullptr;
      double i = 1;
      double j = 1;
      double k = 1;
      if (xi > 0) {
        i = -1;
      } else if (!(xi < 0)) {
        p = &i;
      }
      if (eta > 0) {
        j = -1;
      } else if (!(eta < 0)) {
        p = &j;
      }
      if (zeta > 0) {
        k = -1;
      } else if (!(zeta < 0)) {
        p = &k;
      }
      if (i * j * k < 0) {
        if (!p) {
          Common::warning("Niggli reduction warning: one of the input structures "
                         "contains a lattice that is confusing the Niggli "
                         "reduction algorithm. Try making a small perturbation "
                         "(approx. 2 orders of magnitude smaller than the "
                         "tolerance) to the input lattices and try again. The "
                         "results of this comparison should not be relied upon.");
          return false;
        }
        *p = -1;
      }
      tmpMat << i, 0, 0, 0, j, 0, 0, 0, k;
      cob *= tmpMat;

      // Update characteristic
      xi = -std::fabs(xi);
      eta = -std::fabs(eta);
      zeta = -std::fabs(zeta);
      NIGGLI_DEBUG(4);
      ++iter;
    }

    // Step 5:
    if (Common::gt(std::fabs(xi), B, tol) ||
        (Common::eq(xi, B, tol) && Common::lt(2 * eta, zeta, tol)) ||
        (Common::eq(xi, -B, tol) && Common::lt(zeta, 0, tol))) {
      double signXi = Common::sign(xi);
      // Update change of basis matrix:
      tmpMat << 1, 0, 0, 0, 1, -signXi, 0, 0, 1;
      cob *= tmpMat;

      // Update characteristic
      C = B + C - xi * signXi;
      eta = eta - zeta * signXi;
      xi = xi - 2 * B * signXi;
      NIGGLI_DEBUG(5);
      continue;
    }

    // Step 6:
    if (Common::gt(std::fabs(eta), A, tol) ||
        (Common::eq(eta, A, tol) && Common::lt(2 * xi, zeta, tol)) ||
        (Common::eq(eta, -A, tol) && Common::lt(zeta, 0, tol))) {
      double signEta = Common::sign(eta);
      // Update change of basis matrix:
      tmpMat << 1, 0, -signEta, 0, 1, 0, 0, 0, 1;
      cob *= tmpMat;

      // Update characteristic
      C = A + C - eta * signEta;
      xi = xi - zeta * signEta;
      eta = eta - 2 * A * signEta;
      NIGGLI_DEBUG(6);
      continue;
    }

    // Step 7:
    if (Common::gt(std::fabs(zeta), A, tol) ||
        (Common::eq(zeta, A, tol) && Common::lt(2 * xi, eta, tol)) ||
        (Common::eq(zeta, -A, tol) && Common::lt(eta, 0, tol))) {
      double signZeta = Common::sign(zeta);
      // Update change of basis matrix:
      tmpMat << 1, -signZeta, 0, 0, 1, 0, 0, 0, 1;
      cob *= tmpMat;

      // Update characteristic
      B = A + B - zeta * signZeta;
      xi = xi - eta * signZeta;
      zeta = zeta - 2 * A * signZeta;
      NIGGLI_DEBUG(7);
      continue;
    }

    // Step 8:
    double sumAllButC = A + B + xi + eta + zeta;
    if (Common::lt(sumAllButC, 0, tol) ||
        (Common::eq(sumAllButC, 0, tol) &&
         Common::gt(2 * (A + eta) + zeta, 0, tol))) {
      // Update change of basis matrix:
      cob *= C8;

      // Update characteristic
      C = sumAllButC + C;
      xi = 2 * B + xi + zeta;
      eta = 2 * A + eta + zeta;
      NIGGLI_DEBUG(8);
      continue;
    }

    // Done!
    NIGGLI_DEBUG(999);
    ret = true;
    break;
  }

  // No change, already reduced. Just return.
  if (iter == 0)
    return true;

  // iterations exceeded
  if (!ret)
    return false;

  Q_ASSERT_X(cob.determinant() == 1, Q_FUNC_INFO,
             "Determinant of change of basis matrix must be 1.");

  // Update cell. This order is necessary for column vectors.
  // cob is built up in column-vector form (the cell as A = M^T, matching the
  //   standard Niggli matrices), while our cell is row-form, so we transpose
  //   cob to apply it: new = cob^T * M.
  setCellInfo(cob.transpose() * unitCell().cellMatrix());

  // Check that volume has not changed
  Q_ASSERT_X(Common::eq(origVolume, getVolume(), tol), Q_FUNC_INFO,
             "Cell volume changed during Niggli reduction.");

  // Rotate and wrap
  rotateCellAndCoordsToStandardOrientation();
  wrapAtomsToCell();
  return true;

#undef NIGGLI_DEBUG
}

bool Geometry::isNiggliReduced(double lenTol) const
{
  // 3D only.
  if (!is3D()) return false;
  return Geometry::isNiggliReduced(getA(), getB(), getC(), getAlpha(),
                                    getBeta(), getGamma(), lenTol);
}

bool Geometry::isNiggliReduced(const double a, const double b, const double c,
                                const double alpha, const double beta,
                                const double gamma, double lenTol)
{
  // Calculate characteristic
  double A = a * a;
  double B = b * b;
  double C = c * c;
  double xi = 2 * b * c * std::cos(alpha * DEG2RAD);
  double eta = 2 * a * c * std::cos(beta * DEG2RAD);
  double zeta = 2 * a * b * std::cos(gamma * DEG2RAD);

  // comparison tolerance
  // This may not be exactly the same as pow(origVolume, 2.0/3.0), but we'll
  // say that it's close enough...
  double tol = 0.001 * lenTol * std::pow((a + b + c) / 3.0, 2.0);

  // First check the Buerger conditions. Taken from: Gruber B.. Acta
  // Cryst. A. 1973;29(4):433-440. Available at:
  // http://scripts.iucr.org/cgi-bin/paper?S0567739473001063
  // [Accessed December 15, 2010].
  if (Common::gt(A, B, tol) || Common::gt(B, C, tol))
    return false;
  if (Common::eq(A, B, tol) && Common::gt(std::fabs(xi), std::fabs(eta), tol))
    return false;
  if (Common::eq(B, C, tol) && Common::gt(std::fabs(eta), std::fabs(zeta), tol))
    return false;
  if (!(Common::gt(xi, 0.0, tol) && Common::gt(eta, 0.0, tol) && Common::gt(zeta, 0.0, tol)) &&
      !(Common::leq(xi, 0.0, tol) && Common::leq(eta, 0.0, tol) && Common::leq(zeta, 0.0, tol)))
    return false;

  // Check against Niggli conditions (taken from Gruber 1973). The
  // logic of the second comparison is reversed from the paper to
  // simplify the algorithm.
  if (Common::eq(xi, B, tol) && Common::gt(zeta, 2 * eta, tol))
    return false;
  if (Common::eq(eta, A, tol) && Common::gt(zeta, 2 * xi, tol))
    return false;
  if (Common::eq(zeta, A, tol) && Common::gt(eta, 2 * xi, tol))
    return false;
  if (Common::eq(xi, -B, tol) && Common::neq(zeta, 0, tol))
    return false;
  if (Common::eq(eta, -A, tol) && Common::neq(zeta, 0, tol))
    return false;
  if (Common::eq(zeta, -A, tol) && Common::neq(eta, 0, tol))
    return false;

  if (Common::eq(xi + eta + zeta + A + B, 0, tol) &&
      Common::gt(2 * (A + eta) + zeta, 0, tol))
    return false;

  // all good!
  return true;
}

bool Geometry::isPrimitive(const double cartTol)
{
  // 3D only.
  if (!is3D()) return false;
  // Cache fractional coordinates and atomic nums
  QList<Common::Vector3> fcoords;
  QList<unsigned int> atomicNums;
  for (const auto& atom : atoms()) {
    fcoords.append(cartToFrac(atom.pos()));
    atomicNums.append(atom.atomicNumber());
  }
  size_t originalFCoordsSize = fcoords.size();

  // Get unit cell
  Common::Matrix3 cellMatrix = unitCell().cellMatrix();
  // Returns an unsigned int of the space group (in case we ever need it)
  reduceToPrimitive(&fcoords, &atomicNums, &cellMatrix, cartTol);

  return originalFCoordsSize == static_cast<size_t>(fcoords.size());
}

bool Geometry::reduceToPrimitive(const double prec)
{
  // 3D only.
  if (!is3D()) {
    Common::error("Reduction to primitive is applicable only to a crystal");
    return false;
  }

  // Cache fractional coordinates and atomic nums
  QList<Common::Vector3> fcoords;
  QList<unsigned int> atomicNums;
  for (const auto& atom : atoms()) {
    fcoords.append(cartToFrac(atom.pos()));
    atomicNums.append(atom.atomicNumber());
  }

  // Get unit cell
  Common::Matrix3 cellMatrix = unitCell().cellMatrix();
  unsigned int spg = reduceToPrimitive(&fcoords, &atomicNums, &cellMatrix, prec);

  // spg == 0 implies that reduceToPrimitive() failed
  if (spg == 0)
    return false;

  setCellInfo(cellMatrix);
  // Remove all atoms to simplify the change
  clearAtoms();

  // Add the atoms in
  for (size_t i = 0; i < static_cast<size_t>(fcoords.size()); i++) {
    Atom& newAtom = addAtom();
    newAtom.setAtomicNumber(atomicNums.at(static_cast<int>(i)));
    newAtom.setPos(fracToCart(fcoords.at(static_cast<int>(i))));
  }

  Q_ASSERT(fcoords.size() == atomicNums.size());
  Q_ASSERT(atoms().size() == static_cast<size_t>(fcoords.size()));

  return true;
}

unsigned int Geometry::reduceToPrimitive(QList<Common::Vector3>* fcoords,
                                          QList<unsigned int>* atomicNums,
                                          Common::Matrix3* cellMatrix, const double prec)
{
  Q_ASSERT(fcoords->size() == atomicNums->size());

  const int numberOfAtoms = fcoords->size();

  if (numberOfAtoms < 1) {
    Common::warning("Cannot determine spacegroup of empty cell.");
    return 0;
  }

  // Spglib expects column vecs, so fill with transpose
  double lattice[3][3];
  cellToColumnLatticeArray(*cellMatrix, lattice);

  // Build position list. Include space for 4*numAtoms for the
  // cell refinement
  double(*positions)[3] = new double[4 * numberOfAtoms][3];
  int* types = new int[4 * numberOfAtoms];
  const Common::Vector3* fracCoord;
  for (int i = 0; i < numberOfAtoms; ++i) {
    fracCoord = &(*fcoords)[i];
    types[i] = (*atomicNums)[i];
    positions[i][0] = fracCoord->x();
    positions[i][1] = fracCoord->y();
    positions[i][2] = fracCoord->z();
  }

  // find spacegroup for return value
  int spg;
  SpglibDataset* dataset = spg_get_dataset(lattice, positions, types, numberOfAtoms, prec);
  if (dataset == nullptr) {
    spg = 0;
  } else {
    spg = dataset->spacegroup_number;
    spg_free_dataset(dataset);
  }

  // Refine the structure
  int numBravaisAtoms = spg_refine_cell(lattice, positions, types, numberOfAtoms, prec);

  // if spglib cannot refine the cell, return 0.
  if (numBravaisAtoms <= 0) {
    delete[] positions;
    delete[] types;
    return 0;
  }

  // Find primitive cell. This updates lattice, positions, types
  // to primitive
  int numPrimitiveAtoms = spg_find_primitive(lattice, positions, types, numBravaisAtoms, prec);

  // If the cell was already a primitive cell, reset
  // numPrimitiveAtoms.
  if (numPrimitiveAtoms == 0)
    numPrimitiveAtoms = numBravaisAtoms;

  // Bail if everything failed
  if (numPrimitiveAtoms <= 0) {
    delete[] positions;
    delete[] types;
    return 0;
  }

  // Update passed objects
  // convert col vecs to row vecs
  *cellMatrix = columnLatticeArrayToCell(lattice);

  // Trim
  while (fcoords->size() > numPrimitiveAtoms) {
    fcoords->removeLast();
    atomicNums->removeLast();
  }
  while (fcoords->size() < numPrimitiveAtoms) {
    fcoords->append(Common::Vector3());
    atomicNums->append(0);
  }

  // Update
  Q_ASSERT(fcoords->size() == atomicNums->size());
  Q_ASSERT(fcoords->size() == numPrimitiveAtoms);
  for (int i = 0; i < numPrimitiveAtoms; ++i) {
    (*atomicNums)[i] = types[i];
    (*fcoords)[i] = Common::Vector3(positions[i]);
  }

  delete[] positions;
  delete[] types;

  if (spg >= 231 || spg < 0)
    spg = 0;

  return static_cast<unsigned int>(spg);
}

bool Geometry::standardizeToConventionalCell(const double prec)
{
  // 3D only.
  if (!is3D()) {
    Common::error("Standardizing to conventional is applicable only to a crystal");
    return false;
  }

  const int numberOfAtoms = static_cast<int>(numAtoms());
  if (numberOfAtoms < 1 || !is3D())
    return false;

  double lattice[3][3];
  cellToColumnLatticeArray(unitCell().cellMatrix(), lattice);

  double(*positions)[3] = new double[4 * numberOfAtoms][3];
  int* types = new int[4 * numberOfAtoms];
  for (int i = 0; i < numberOfAtoms; ++i) {
    const Common::Vector3 frac = cartToFrac(atom(i).pos());
    positions[i][0] = frac.x();
    positions[i][1] = frac.y();
    positions[i][2] = frac.z();
    types[i] = atom(i).atomicNumber();
  }

  const int standardizedAtoms =
    spg_standardize_cell(lattice, positions, types, numberOfAtoms, 0, 0, prec);
  if (standardizedAtoms <= 0) {
    delete[] positions;
    delete[] types;
    return false;
  }

  Common::Matrix3 standardizedCell = columnLatticeArrayToCell(lattice);

  UnitCell conventionalCell(standardizedCell);
  std::vector<Atom> standardizedAtomsList;
  standardizedAtomsList.reserve(static_cast<size_t>(standardizedAtoms));
  for (int i = 0; i < standardizedAtoms; ++i) {
    const Common::Vector3 frac(positions[i][0], positions[i][1], positions[i][2]);
    standardizedAtomsList.push_back(Atom(static_cast<unsigned short>(types[i]),
           conventionalCell.toCartesian(frac)));
  }

  delete[] positions;
  delete[] types;

  setUnitCell(conventionalCell);
  setAtoms(standardizedAtomsList);
  return true;
}

uint Geometry::getSpaceGroupNumber() const
{
  return g_spgNumber;
}

QString Geometry::getSpaceGroupSymbol() const
{
  return g_spgSymbol;
}

QString Geometry::getHTMLSpaceGroupSymbol() const
{
  QString s = g_spgSymbol;

  // Prepare HTML tags
  s.prepend("<HTML>");
  s.append("</HTML>");

  // "_X"  --> "<sub>X</sub>"
  int ind = s.indexOf("_");
  while (ind != -1) {
    s = s.insert(ind + 2, "</sub>");
    s = s.replace(ind, 1, "<sub>");
    ind = s.indexOf("_");
  }

  // "-X"  --> "<span style="text-decoration: overline">X</span>"
  ind = s.indexOf("-");
  while (ind != -1) {
    s = s.insert(ind + 2, "</span>");
    s = s.replace(ind, 1, "<span style=\"text-decoration: overline\">");
    ind = s.indexOf("-", ind + 35);
  }

  return s;
}

QString Geometry::getHMName(unsigned short spg)
{
  if (spg >= 231 || spg < 1) {
    Common::error(QString("%1: an invalid spg number of %2 was entered!")
                 .arg(__func__).arg(spg));
    return QString();
  }
  return QString::fromStdString(Atoms::_HMNames[spg]);
}

void Geometry::getSpglibFormat() const
{
  // 3D only.
  if (!is3D()) return;
  Common::Vector3 aVec = unitCell().aVector();
  Common::Vector3 bVec = unitCell().bVector();
  Common::Vector3 cVec = unitCell().cVector();

  QString t;
  QTextStream out(&t);

  out << "double lattice[3][3] = {\n"
      << "  {" << aVec.x() << ", " << bVec.x() << ", " << cVec.x() << "},\n"
      << "  {" << aVec.y() << ", " << bVec.y() << ", " << cVec.y() << "},\n"
      << "  {" << aVec.z() << ", " << bVec.z() << ", " << cVec.z() << "}};\n\n";

  out << "double position[][3] = {";
  for (unsigned int i = 0; i < numAtoms(); i++) {
    if (i != 0)
      out << ",";
    out << "\n";
    out << "  {" << atom(i).pos().x() << ", " << atom(i).pos().y() << ", "
        << atom(i).pos().z() << "}";
  }
  out << "};\n\n";
  out << "int types[] = { ";
  for (unsigned int i = 0; i < numAtoms(); i++) {
    if (i != 0)
      out << ", ";
    out << atom(i).atomicNumber();
  }
  out << " };\n";
  out << "int num_atom = " << numAtoms() << ";\n";
  Common::message(t);
}

void Geometry::findSpaceGroup(double prec) const
{
  // 3D only.
  // reset space group to 0 so we can exit if needed
  g_spgNumber = 0;
  g_spgSymbol = "Unknown";
  int numberOfAtoms = static_cast<int>(numAtoms());

  // if no unit cell or atoms, exit
  if (numberOfAtoms == 0)
    return;
  else if (!is3D()) {
    Common::warning(QString("%1: called on a structure with no cell!").arg(__func__));
    return;
  }

  // Get lattice matrix. Spglib expects column vectors.
  double lattice[3][3];
  cellToColumnLatticeArray(unitCell().cellMatrix(), lattice);

  // Get atom info
  double(*positions)[3] = new double[numberOfAtoms][3];
  int* types = new int[numberOfAtoms];
  const std::vector<Atom>& atomList = atoms();
  for (int i = 0; i < static_cast<int>(atomList.size()); i++) {
    Common::Vector3 fracCoords = cartToFrac(atomList.at(i).pos());
    types[i] = atomList.at(i).atomicNumber();
    positions[i][0] = fracCoords.x();
    positions[i][1] = fracCoords.y();
    positions[i][2] = fracCoords.z();
  }

  // find spacegroup
  char symbol[21];
  SpglibDataset* dataset = spg_get_dataset(lattice, positions, types, numberOfAtoms, prec);
  if (dataset == nullptr) {
    g_spgNumber = 0;
    symbol[0] = '\0';
  } else {
    g_spgNumber = dataset->spacegroup_number;
    std::snprintf(symbol, sizeof(symbol), "%s", dataset->international_symbol);
    spg_free_dataset(dataset);
  }

  delete[] positions;
  delete[] types;

  // Fail if g_spgNumber is still 0
  if (g_spgNumber == 0)
    return;

  // Set and clean up the symbol string
  g_spgSymbol = QString(symbol);
  g_spgSymbol.remove(" ");
}

QString Geometry::getPointGroupSymbol(double tolerance) const
{
  const QString unknown = "unknown";

  if (numAtoms() == 0)
    return unknown;

  // 0D only: the point group is for a finite cluster of atoms.
  if (!is0D()) {
    Common::warning(QString("%1: called on a structure that is not 0D!").arg(__func__));
    return unknown;
  }

  if (!GS_ISFINITE(tolerance) || tolerance < 0.0 || tolerance >= 1.0)
    return unknown;

  msym_context ctx = msymCreateContext();
  if (!ctx)
    return unknown;

  auto releaseAndReturn = [ctx](const QString& result) {
    msymReleaseContext(ctx);
    return result;
  };

  const msym_thresholds_t* defaults = msymGetDefaultThresholds();
  if (!defaults || defaults->geometry <= 0.0)
    return releaseAndReturn(unknown);

  const double scale = fabs(tolerance) == 0.0 ? 1.0 : tolerance / defaults->geometry;
  msym_thresholds_t thresholds = *defaults;
  thresholds.geometry *= scale;
  thresholds.zero *= scale;
  thresholds.angle *= scale;
  thresholds.equivalence *= scale;
  thresholds.eigfact *= scale;
  thresholds.permutation *= scale;
  thresholds.orthogonalization *= scale;

  msym_error_t ret = msymSetThresholds(ctx, &thresholds);
  if (ret != MSYM_SUCCESS)
    return releaseAndReturn(unknown);

  std::vector<msym_element_t> elements;
  elements.reserve(numAtoms());
  const std::vector<Atom>& atomList = atoms();
  for (size_t i = 0; i < atomList.size(); ++i) {
    const unsigned int atomicNum = atomList[i].atomicNumber();
    const std::string symbol = ElementInfo::getAtomicSymbol(atomicNum);

    msym_element_t element = {};
    element.n = static_cast<int>(atomicNum);
    element.m = ElementInfo::getAtomicMass(atomicNum);
    element.v[0] = atomList[i].pos().x();
    element.v[1] = atomList[i].pos().y();
    element.v[2] = atomList[i].pos().z();
    std::snprintf(element.name, sizeof(element.name), "%s", symbol.c_str());
    elements.push_back(element);
  }

  ret = msymSetElements(ctx, static_cast<int>(elements.size()), elements.data());
  if (ret == MSYM_SUCCESS)
    ret = msymFindSymmetry(ctx);

  char detectedName[32] = { '\0' };
  if (ret == MSYM_SUCCESS)
    ret = msymGetPointGroupName(ctx, sizeof(detectedName), detectedName);

  if (ret != MSYM_SUCCESS || detectedName[0] == '\0')
    return releaseAndReturn(unknown);

  return releaseAndReturn(QString::fromLatin1(detectedName));
}

// Atoms and bonds

void Geometry::appendCell(const Geometry& cell)
{
  // Offset for renumbering the appended bonds.
  size_t offset = numAtoms();
  for (const auto& atom : cell.atoms())
    addAtom(atom);
  for (const auto& bond : cell.bonds()) {
    addBond(bond.first() + offset, bond.second() + offset, bond.bondOrder());
  }
}

std::vector<Geometry> Geometry::getConnectedComponents() const
{
  std::vector<Geometry> ret;

  std::vector<bool> atomAlreadyUsed(numAtoms(), false);

  while (true) {
    auto it = std::find(atomAlreadyUsed.begin(), atomAlreadyUsed.end(), false);

    if (it == atomAlreadyUsed.end())
      break;

    Geometry component;
    component.setUnitCell(unitCell());

    size_t startInd = it - atomAlreadyUsed.begin();
    component.addAtom(atoms()[startInd]);
    atomAlreadyUsed[startInd] = true;

    std::vector<size_t> atomsToCheck(1, startInd);

    // Maps old atom indices to the new component indices.
    std::unordered_map<size_t, size_t> mapToNewIndices;
    mapToNewIndices[startInd] = 0;

    while (!atomsToCheck.empty()) {
      size_t checkInd = atomsToCheck[0];

      for (size_t i = startInd + 1; i < numAtoms(); ++i) {
        if (i == checkInd)
          continue;

        // -1 if no such bond exists.
        long long bondInd = bondBetweenAtoms(checkInd, i);
        if (bondInd != -1 && !atomAlreadyUsed[i]) {
          component.addAtom(atoms()[i]);
          atomAlreadyUsed[i] = true;
          atomsToCheck.push_back(i);
          mapToNewIndices[i] = component.numAtoms() - 1;
          component.addBond(mapToNewIndices[checkInd], mapToNewIndices[i],
                            bonds()[bondInd].bondOrder());
        }
        // Atom already added: still make sure the bond is recorded.
        else if (bondInd != -1 && atomAlreadyUsed[i]) {
          if (!component.areBonded(mapToNewIndices[checkInd], mapToNewIndices[i])) {
            component.addBond(mapToNewIndices[checkInd], mapToNewIndices[i],
                              bonds()[bondInd].bondOrder());
          }
        }
      }
      atomsToCheck.erase(atomsToCheck.begin());
    }

    ret.push_back(component);
  }

  return ret;
}

bool Geometry::removeAtom(size_t ind)
{
  if (ind >= g_atoms.size())
    return false;
  clearGeometryCaches();
  removeBondsFromAtom(ind);

  // Indicate to the bonds that they should decrement any indices greater
  // than ind.
  for (auto& bond : g_bonds)
    bond.atomIndexRemoved(ind);

  g_atoms.erase(g_atoms.begin() + ind);
  return true;
}

bool Geometry::removeAtom(const Atom& atom)
{
  long long index = atomIndex(atom);
  if (index == -1)
    return false;
  else
    removeAtom(index);
  return true;
}

// We pass by copy because we want to edit a copy of sortOrder...
void Geometry::sortAtoms(std::vector<size_t> sortOrder)
{
  assert(sortOrder.size() == g_atoms.size());

  // Nothing to sort; also keeps the size_t loop bound from underflowing.
  if (g_atoms.empty())
    return;

  // Only need to do g_atoms.size() - 1 since the last item will
  // automatically be in place.
  for (size_t i = 0; i < g_atoms.size() - 1; ++i) {
    assert(sortOrder[i] < g_atoms.size());

    // Keep swapping until the index is in the correct place
    while (sortOrder[i] != i) {
      size_t newInd = sortOrder[i];
      swapAtoms(i, newInd);
      std::swap(sortOrder[i], sortOrder[newInd]);
    }
  }
}

// We will implement this in terms of sortAtoms()
void Geometry::reorderAtoms(const std::vector<size_t>& newOrder)
{
  if (newOrder.size() != g_atoms.size()) {
    Common::error(QString("%1: atom order has the wrong size.").arg(__func__));
    return;
  }

  std::vector<bool> found(g_atoms.size(), false);
  for (size_t index : newOrder) {
    if (index >= g_atoms.size() || found[index]) {
      Common::error(QString("%1: atom order is incomplete.").arg(__func__));
      return;
    }
    found[index] = true;
  }

  std::vector<size_t> sortOrder(newOrder.size(), 0);
  for (size_t i = 0; i < newOrder.size(); ++i) {
    assert(newOrder[i] < sortOrder.size());
    sortOrder[newOrder[i]] = i;
  }
  sortAtoms(sortOrder);
}

void Geometry::removeBondBetweenAtoms(size_t ind1, size_t ind2)
{
  if (ind1 >= g_atoms.size() || ind2 >= g_atoms.size())
    return;
  for (size_t i = 0; i < g_bonds.size(); ++i) {
    if ((g_bonds[i].first() == ind1 && g_bonds[i].second() == ind2) ||
        (g_bonds[i].first() == ind2 && g_bonds[i].second() == ind1)) {
      removeBond(i);
      --i;
    }
  }
}

void Geometry::removeBondsFromAtom(size_t ind)
{
  if (ind >= g_atoms.size())
    return;
  for (size_t i = 0; i < g_bonds.size(); ++i) {
    if (g_bonds[i].first() == ind || g_bonds[i].second() == ind) {
      removeBond(i);
      --i;
    }
  }
}

long long Geometry::bondBetweenAtoms(size_t atomInd1, size_t atomInd2) const
{
  assert(atomInd1 < g_atoms.size());
  assert(atomInd2 < g_atoms.size());
  for (size_t i = 0; i < g_bonds.size(); ++i) {
    if ((g_bonds[i].first() == atomInd1 && g_bonds[i].second() == atomInd2) ||
        (g_bonds[i].second() == atomInd1 && g_bonds[i].first() == atomInd2)) {
      return i;
    }
  }
  return -1;
}

bool Geometry::isBonded(size_t ind) const
{
  assert(ind < g_atoms.size());
  for (const auto& bond : g_bonds) {
    if (bond.first() == ind || bond.second() == ind)
      return true;
  }
  return false;
}

bool Geometry::areBonded(size_t ind1, size_t ind2) const
{
  assert(ind1 < g_atoms.size());
  assert(ind2 < g_atoms.size());
  for (const auto& bond : g_bonds) {
    if ((bond.first() == ind1 && bond.second() == ind2) ||
        (bond.first() == ind2 && bond.second() == ind1)) {
      return true;
    }
  }
  return false;
}

std::vector<size_t> Geometry::bonds(size_t ind) const
{
  assert(ind < g_atoms.size());
  std::vector<size_t> ret;
  for (size_t i = 0; i < g_bonds.size(); ++i) {
    if (g_bonds[i].first() == ind || g_bonds[i].second() == ind)
      ret.push_back(i);
  }
  return ret;
}

std::vector<size_t> Geometry::bondedAtoms(size_t ind) const
{
  assert(ind < g_atoms.size());
  std::vector<size_t> ret;
  for (size_t i = 0; i < g_atoms.size(); ++i) {
    if (ind == i)
      continue;
    else if (areBonded(i, ind))
      ret.push_back(i);
  }
  return ret;
}

void Geometry::perceiveBonds()
{
  clearBonds();

  // The cutoff tolerance to be used
  const double tol = 0.1;
  const auto& atomList = atoms();
  double cutoff = 0.0;
  for (const Atom& atom : atomList)
    cutoff = std::max(cutoff, Atoms::ElementInfo::getCovalentRadius(atom.atomicNumber()));
  cutoff = 2.0 * cutoff + tol;

  if (!calculateNearestNeighborLists(cutoff))
    return;

  const auto& neighbors = getNearestNeighborLists();
  for (size_t i = 0; i < neighbors.size(); ++i) {
    for (const auto& neighbor : neighbors[i]) {
      if (neighbor.first < 0 || neighbor.first >= static_cast<int>(atomList.size()))
        continue;
      const size_t j = static_cast<size_t>(neighbor.first);
      if (i >= j || areBonded(i, j))
        continue;
      const double maxDistance =
              Atoms::ElementInfo::getCovalentRadius(atomList[i].atomicNumber()) +
              Atoms::ElementInfo::getCovalentRadius(atomList[j].atomicNumber()) + tol;
      if (neighbor.second < maxDistance)
        addBond(i, j);
    }
  }
}

QList<QString> Geometry::getSymbols() const
{
  // Returns the ordered list of structure symbols "as is"; i.e.,
  //   for a sub-system seed it might miss some of the symbols.
  QList<QString> list;
  for (const auto& atom : atoms()) {
    QString symbol = Atoms::ElementInfo::getAtomicSymbol(atom.atomicNumber()).c_str();
    if (!list.contains(symbol))
      list.append(symbol);
  }
  std::sort(list.begin(), list.end());
  return list;
}

unsigned int Geometry::getNumberOfAtomsOfSymbol(const QString& s) const
{
  // Retruns the number of atoms of a symbol (0 if that type
  //   doesn't exist; e.g., sub-system seed).
  QList<QString> symbols = getSymbols();
  int index = symbols.indexOf(s);
  if (index == -1)
    return 0;
  return getNumberOfAtomsAlpha().at(index);
}

std::vector<uint> Geometry::getNumberOfAtomsAlpha() const
{
  // Returns the ordered list of atom counts "as is"; i.e.,
  //   for a sub-system seed it might miss counts for some of the symbols.
  QList<QString> symbols = getSymbols();
  std::vector<uint> list(symbols.size(), 0);

  for (const auto& atom : atoms()) {
    QString symbol = Atoms::ElementInfo::getAtomicSymbol(atom.atomicNumber()).c_str();
    Q_ASSERT_X(symbols.contains(symbol), Q_FUNC_INFO,
               "getNumberOfAtomsAlpha found a symbol not in getSymbols.");
    ++list[symbols.indexOf(symbol)];
  }
  return list;
}

QString Geometry::getChemicalFormula() const
{
  // Returns the structure chemical formula "as is"; i.e.,
  //   for a sub-system seed it might miss some of the symbols.
  QList<QString> symbols = getSymbols();
  std::vector<uint> counts = getNumberOfAtomsAlpha();

  QString formula;
  for (int i = 0; i < symbols.size(); ++i)
    formula += QString("%1%2").arg(symbols.at(i)).arg(counts.at(i));
  return formula;
}

unsigned int Geometry::getFormulaUnits() const
{
  // Perform an atomistic formula unit calculation
  std::vector<uint> counts = getNumberOfAtomsAlpha();
  if (counts.empty())
    return 0;
  return std::accumulate(counts.begin(), counts.end(), counts[0], gcd);
}

QString Geometry::getCompositionString(bool reduceToEmpirical) const
{
  // Returns the structure composition id ("n:m:o") "as is"; i.e.,
  //   for a sub-system seed it might miss some of the symbols.
  uint formulaUnits = reduceToEmpirical ? getFormulaUnits() : 1;
  std::vector<uint> atomCounts = getNumberOfAtomsAlpha();

  QString id;
  if (atomCounts.size() == 0 || formulaUnits < 1)
    return id;

  for (const auto& count : atomCounts)
    id += QString("%1:").arg(count / formulaUnits);
  id.chop(1);

  return id;
}

double Geometry::angle(const Common::Vector3& A, const Common::Vector3& B, const Common::Vector3& C)
{
  const Common::Vector3& AB = A - B;
  const Common::Vector3& BC = C - B;

  return acos(AB.dot(BC) / (AB.norm() * BC.norm())) * RAD2DEG;
}

double Geometry::dihedral(const Common::Vector3& A, const Common::Vector3& B, const Common::Vector3& C,
                           const Common::Vector3& D)
{
  const Common::Vector3& AB = B - A;
  const Common::Vector3& BC = C - B;
  const Common::Vector3& CD = D - C;

  const Common::Vector3& n1 = AB.cross(BC);
  const Common::Vector3& n2 = BC.cross(CD);

  return atan2(n1.cross(n2).dot(BC / BC.norm()), n1.dot(n2)) * RAD2DEG;
}

} // namespace Atoms

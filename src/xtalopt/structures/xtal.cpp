/**********************************************************************
  Xtal - XtalOpt-specific Structure subclass.

  Copyright (C) 2009-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/structures/xtal.h>

#include <common/output.h>
#include <common/compatibility/qt_compat.h>
#include <common/numericutils.h>
#include <common/random.h>
#include <common/settings.h>

#include <QFile>
#include <QStringList>

#include <algorithm>
#include <cfloat> // For DBL_MAX
#include <limits>
#include <sstream>

using namespace std;
using namespace Search;

namespace XtalOpt {
namespace {

QString fixedWidthDecimal(double value, int width, int maxPrecision)
{
  QString text;
  for (int precision = maxPrecision; precision >= 0; --precision) {
    const double rounded = Common::roundToDecimalPlaces(value, precision);
    text = QString::number(rounded, 'f', precision);
    if (text.size() <= width)
      break;
  }

  if (text.size() > width) {
    for (int precision = maxPrecision; precision >= 0; --precision) {
      text = QString::number(value, 'e', precision);
      if (text.size() <= width)
        break;
    }
  }

  return QString("%1").arg(text, width);
}

} // namespace

Xtal::Xtal(QObject* parent)
  : Structure(parent), m_hasValidComposition(true),
    m_aboveHull(std::numeric_limits<double>::quiet_NaN())
{
}

Xtal::Xtal(double A, double B, double C, double Alpha, double Beta,
           double Gamma, QObject* parent)
  : Structure(parent), m_hasValidComposition(true),
    m_aboveHull(std::numeric_limits<double>::quiet_NaN())
{
  setCellInfo(A, B, C, Alpha, Beta, Gamma);
}

Xtal::Xtal(const Xtal& other)
  : Structure(other), m_hasValidComposition(other.m_hasValidComposition),
    m_aboveHull(other.m_aboveHull)
{
}

Xtal::Xtal(Xtal&& other) noexcept
  : Structure(std::move(other)),
    m_hasValidComposition(std::move(other.m_hasValidComposition)),
    m_aboveHull(std::move(other.m_aboveHull))
{
}

Xtal& Xtal::operator=(const Xtal& other)
{
  if (this != &other) {
    Structure::operator=(other);

    m_hasValidComposition = other.m_hasValidComposition;
    m_aboveHull = other.m_aboveHull;
  }

  return *this;
}

Xtal& Xtal::operator=(Xtal&& other) noexcept
{
  if (this != &other) {
    Structure::operator=(std::move(other));

    m_hasValidComposition = std::move(other.m_hasValidComposition);
    m_aboveHull = std::move(other.m_aboveHull);
  }

  return *this;
}

Xtal::~Xtal()
{
}



void Xtal::printLatticeInfo() const
{
  std::stringstream outs;
  outs << "a is " << this->getA() << "\n";
  outs << "b is " << this->getB() << "\n";
  outs << "c is " << this->getC() << "\n";
  outs << "alpha is " << this->getAlpha() << "\n";
  outs << "beta is " << this->getBeta() << "\n";
  outs << "gamma is " << this->getGamma() << "\n";
  outs << "volume is " << this->getVolume() << "\n";

  outs << "cellMatrix is (row vectors):\n";
  for (size_t i = 0; i < 3; ++i) {
    for (size_t j = 0; j < 3; ++j) {
      outs << unitCell().cellMatrix()(i, j) << "  ";
    }
    outs << "\n";
  }
  Common::message(outs.str().c_str());
}

void Xtal::printAtomInfo() const
{
  std::stringstream outs;
  outs << "Frac coords info (blank if none):\n";
  const std::vector<Atoms::Atom>& atoms = this->atoms();
  QList<Common::Vector3> fracCoords;

  for (const auto& atm : atoms)
    fracCoords.append(cartToFrac(atm.pos()));

  for (size_t i = 0; i < atoms.size(); i++) {
    outs << "  For atomic num " << atoms.at(i).atomicNumber()
         << ", coords are (" << fracCoords.at(i)[0] << ","
         << fracCoords.at(i)[1] << "," << fracCoords.at(i)[2] << ")\n";
  }
  Common::message(outs.str().c_str());
}

void Xtal::printXtalInfo() const
{
  printLatticeInfo();
  printAtomInfo();
}

bool Xtal::fixAngles(int attempts)
{
  // Perform niggli reduction
  if (!niggliReduce(attempts)) {
    Common::error(QString("Unable to perform cell reduction on Xtal %1"
                         " (%2,%3,%4,%5,%6,%7)")
                 .arg(getTag()).arg(getA()).arg(getB()).arg(getC())
                 .arg(getAlpha()).arg(getBeta()).arg(getGamma()));
    return false;
  }

  findSpaceGroup();
  return true;
}

bool Xtal::operator==(const Xtal& o) const
{
  // Compare coordinates using the default tolerance
  if (!compareXtalComp(o))
    return false;

  return true;
}

bool Xtal::addAtomRandomly(uint atomicNumber, double minIAD, int maxAttempts)
{
  double IAD = -1;
  int i = 0;
  Common::Vector3 cartCoords;

  // For first atom, add to 0, 0, 0
  if (numAtoms() == 0) {
    cartCoords = Common::Vector3(0, 0, 0);
  } else {
    do {
      // Generate fractional coordinates
      IAD = -1;
      double x = Common::getRandDouble();
      double y = Common::getRandDouble();
      double z = Common::getRandDouble();

      // Convert to cartesian coordinates and store
      Common::Vector3 fracCoords(x, y, z);
      cartCoords = fracToCart(fracCoords);
      if (minIAD >= 0.0) {
        getNearestNeighborDistance(cartCoords[0], cartCoords[1], cartCoords[2],
                                   IAD);
      } else {
        break;
      };
      i++;
    } while (i < maxAttempts && IAD <= minIAD);

    // Check the result, not the counter (negative minIAD: no distance check).
    if (minIAD >= 0.0 && IAD <= minIAD)
      return false;
  }
  Atoms::Atom& atm = addAtom();
  atm.setPos(cartCoords);
  atm.setAtomicNumber(atomicNumber);
  return true;
}

bool Xtal::addAtomRandomlyScaledIAD(unsigned int atomicNumber, const EleScaledRadii& limits, int maxAttempts)
{
  Common::Vector3 cartCoords;
  bool success;

  // For first atom, add to 0, 0, 0
  if (numAtoms() == 0) {
    cartCoords = Common::Vector3(0, 0, 0);
  } else {
    unsigned int i = 0;
    Common::Vector3 fracCoords;

    // Cache the minimum radius for the new atom
    const double newMinRadius = limits.getMinRadius(atomicNumber);

    // Compute a cut off distance -- atoms farther away than this value
    // will abort the check early.
    double maxCheckDistance = 0.0;
    for (const auto& atmn : limits.getRadiiAtomicNumbers()) {
      if (limits.getMinRadius(atmn) > maxCheckDistance) {
        maxCheckDistance = limits.getMinRadius(atmn);
      }
    }
    maxCheckDistance += newMinRadius;
    const double maxCheckDistSquared = maxCheckDistance * maxCheckDistance;

    do {
      // Reset sentinal
      success = true;

      // Generate fractional coordinates
      fracCoords = Common::Vector3(Common::getRandDouble(), Common::getRandDouble(), Common::getRandDouble());

      // Convert to cartesian coordinates and store
      cartCoords = Common::Vector3(this->fracToCart(fracCoords));

      // Compare distance to each atom in xtal with minimum radii
      QList<double> squaredDists;
      this->getSquaredAtomicDistancesToPoint(cartCoords, &squaredDists);
      Q_ASSERT_X(squaredDists.size() == static_cast<int>(this->numAtoms()), Q_FUNC_INFO,
                 "Size of distance list does not match number of atoms.");

      for (int dist_ind = 0; dist_ind < squaredDists.size(); ++dist_ind) {
        const double& curDistSquared = squaredDists[dist_ind];
        // Save a bit of time if distance is huge...
        if (curDistSquared > maxCheckDistSquared) {
          continue;
        }
        // Compare distance to minimum:
        const double minDist = newMinRadius + limits.getMinRadius(this->atom(dist_ind).atomicNumber());
        const double minDistWithTol = std::max(0.0, minDist - ZERO08);
        const double minDistSquared = minDistWithTol * minDistWithTol;

        if (Common::lt(curDistSquared, minDistSquared, 0.0)) {
          success = false;
          break;
        }
      }

    } while (++i < static_cast<unsigned int>(maxAttempts) && !success);

    // Check the result, not the counter.
    if (!success)
      return false;
  }
  Atoms::Atom& atom = addAtom();
  atom.setPos(cartCoords);
  atom.setAtomicNumber(atomicNumber);
  return true;
}

bool Xtal::moveAtomRandomlyScaledIAD(unsigned int atomicNumber,
  const EleScaledRadii& limits, int maxAttempts, Atoms::Atom* atom)
{
  Q_UNUSED(atomicNumber);
  if (!atom)
    return false;

  Common::Vector3 cartCoords;
  bool success;

  // For first atom, add to 0, 0, 0
  if (numAtoms() == 0) {
    cartCoords = Common::Vector3(0, 0, 0);
    atom->setPos(cartCoords);
    return true;
  } else {
    int movingAtomIndex = -1;
    for (int ind = 0; ind < static_cast<int>(this->numAtoms()); ++ind) {
      if (&this->atom(ind) == atom) {
        movingAtomIndex = ind;
        break;
      }
    }

    if (movingAtomIndex < 0)
      return false;

    const unsigned int movingAtomicNumber = this->atom(movingAtomIndex).atomicNumber();
    unsigned int i = 0;
    Common::Vector3 fracCoords;

    do {
      // Reset sentinal
      success = true;

      // Generate fractional coordinates
      fracCoords = Common::Vector3(Common::getRandDouble(), Common::getRandDouble(), Common::getRandDouble());

      // Convert to cartesian coordinates and store
      cartCoords = Common::Vector3(this->fracToCart(fracCoords));

      // Compare distance to each atom in xtal with minimum radii
      QList<double> squaredDists;
      this->getSquaredAtomicDistancesToPoint(cartCoords, &squaredDists);
      Q_ASSERT_X(squaredDists.size() == static_cast<int>(this->numAtoms()), Q_FUNC_INFO,
                 "Size of distance list does not match number of atoms.");

      for (int dist_ind = 0; dist_ind < squaredDists.size(); ++dist_ind) {
        // If a1 and a2 are the same, skip the comparison
        if (dist_ind == movingAtomIndex) {
          continue;
        }

        Atoms::Atom& a2 = this->atom(dist_ind);
        double& curDistSquared = squaredDists[dist_ind];

        double minDist = limits.getMinRadius(movingAtomicNumber);
        minDist = limits.getMinRadius(a2.atomicNumber()) + minDist;
        const double minDistWithTol = std::max(0.0, minDist - ZERO08);
        const double minDistSquared = minDistWithTol * minDistWithTol;

        if (Common::lt(curDistSquared, minDistSquared, 0.0)) {
          success = false;
          break;
        }
      }
    } while (++i < static_cast<unsigned int>(maxAttempts) && !success);

    // Check the result, not the counter.
    if (!success) {
      Common::error(QString("%1: failed to move an atom within the "
                            "interatomic distance after all attempts.").arg(__func__));
      return false;
    }

    this->atom(movingAtomIndex).setPos(cartCoords);
    notifyGeometryChanged();
    return true;
  }
}

bool Xtal::addAtomRandomlyCustomIAD(unsigned int atomicNumber,
  const PairCustomDistances& limitsIAD, int maxAttempts)
{
  Common::Vector3 cartCoords;
  bool success;

  // For first atom, add to 0, 0, 0
  if (numAtoms() == 0) {
    cartCoords = Common::Vector3(0, 0, 0);
  } else {
    unsigned int i = 0;
    Common::Vector3 fracCoords;

    do {
      // Reset sentinal
      success = true;

      // Generate fractional coordinates
      fracCoords = Common::Vector3(Common::getRandDouble(), Common::getRandDouble(), Common::getRandDouble());

      // Convert to cartesian coordinates and store
      cartCoords = Common::Vector3(this->fracToCart(fracCoords));

      // Compare distance to each atom using custom-IAD mode only.
      QList<double> squaredDists;
      this->getSquaredAtomicDistancesToPoint(cartCoords, &squaredDists);
      Q_ASSERT_X(squaredDists.size() == static_cast<int>(this->numAtoms()), Q_FUNC_INFO,
                 "Size of distance list does not match number of atoms.");

      for (int dist_ind = 0; dist_ind < squaredDists.size(); ++dist_ind) {
        double& curDistSquared = squaredDists[dist_ind];
        const double minDist =
          limitsIAD.getPairDistance(atomicNumber, this->atom(dist_ind).atomicNumber());
        if (minDist == PINF) {
          Common::error(QString("%1: missing custom interatomic distance "
                                "for atomic numbers %2 and %3.")
                       .arg(__func__)
                       .arg(atomicNumber)
                       .arg(this->atom(dist_ind).atomicNumber()));
          return false;
        }
        const double minDistWithTol = std::max(0.0, minDist - ZERO08);
        const double minDistSquared = minDistWithTol * minDistWithTol;

        if (Common::lt(curDistSquared, minDistSquared, 0.0)) {
          success = false;
          break;
        }
      }
    } while (++i < static_cast<unsigned int>(maxAttempts) && !success);

    // Check the result, not the counter.
    if (!success) {
      Common::error(QString("%1: failed to add an atom within the "
                            "interatomic distance after all attempts.").arg(__func__));
      return false;
    }
  }
  Atoms::Atom& atom = addAtom();
  atom.setPos(cartCoords);
  atom.setAtomicNumber(static_cast<int>(atomicNumber));
  return true;
}

bool Xtal::moveAtomRandomlyCustomIAD(unsigned int atomicNumber,
  const PairCustomDistances& limitsIAD, int maxAttempts, Atoms::Atom* atom)
{
  if (!atom)
    return false;

  Common::Vector3 cartCoords;
  bool success;

  // For first atom, add to 0, 0, 0
  if (numAtoms() == 0) {
    cartCoords = Common::Vector3(0, 0, 0);
    atom->setPos(cartCoords);
    return true;
  } else {
    int movingAtomIndex = -1;
    for (int ind = 0; ind < static_cast<int>(this->numAtoms()); ++ind) {
      if (&this->atom(ind) == atom) {
        movingAtomIndex = ind;
        break;
      }
    }

    if (movingAtomIndex < 0)
      return false;

    unsigned int i = 0;
    Common::Vector3 fracCoords;

    do {
      // Reset sentinal
      success = true;

      // Generate fractional coordinates
      fracCoords = Common::Vector3(Common::getRandDouble(), Common::getRandDouble(), Common::getRandDouble());

      // Convert to cartesian coordinates and store
      cartCoords = Common::Vector3(this->fracToCart(fracCoords));

      // Compare distance to each atom using custom-IAD mode only.
      QList<double> squaredDists;
      this->getSquaredAtomicDistancesToPoint(cartCoords, &squaredDists);
      Q_ASSERT_X(squaredDists.size() == static_cast<int>(this->numAtoms()), Q_FUNC_INFO,
                 "Size of distance list does not match number of atoms.");

      for (int dist_ind = 0; dist_ind < squaredDists.size(); ++dist_ind) {
        // If a1 and a2 are the same, skip the comparison
        if (dist_ind == movingAtomIndex) {
          continue;
        }

        double& curDistSquared = squaredDists[dist_ind];
        const double minDist =
          limitsIAD.getPairDistance(atomicNumber, this->atom(dist_ind).atomicNumber());
        if (minDist == PINF) {
          Common::error(QString("%1: missing custom interatomic distance "
                                "for atomic numbers %2 and %3.")
                       .arg(__func__)
                       .arg(atomicNumber)
                       .arg(this->atom(dist_ind).atomicNumber()));
          return false;
        }
        const double minDistWithTol = std::max(0.0, minDist - ZERO08);
        const double minDistSquared = minDistWithTol * minDistWithTol;

        if (Common::lt(curDistSquared, minDistSquared, 0.0)) {
          success = false;
          break;
        }
      }
    } while (++i < static_cast<unsigned int>(maxAttempts) && !success);

    // Check the result, not the counter.
    if (!success) {
      Common::error(QString("%1: failed to move an atom within the "
                            "interatomic distance after all attempts.").arg(__func__));
      return false;
    }

    this->atom(movingAtomIndex).setPos(cartCoords);
    notifyGeometryChanged();
    return true;
  }
}

bool Xtal::checkInterAtomicDistancesCustom(const PairCustomDistances& limitsIAD, int* atom1,
                                           int* atom2, double* IAD)
{
  // Check the custom IAD values (fails if any pairs of atoms are missing).
  // Iterate through all of the atoms in the geometry for "a1"
  for (auto a1 = atoms().begin(), a1_end = atoms().end(); a1 != a1_end; ++a1) {
    const int a1Index = static_cast<int>(a1 - atoms().begin());

    // Get list of minimum squared distances between each atom and a1
    QList<double> squaredDists;
    this->getSquaredAtomicDistancesToPoint(a1->pos(), &squaredDists);
    Q_ASSERT_X(squaredDists.size() == static_cast<int>(this->numAtoms()), Q_FUNC_INFO,
               "Size of distance list does not match number of atoms.");

    // Iterate through each distance
    for (int i = 0; i < squaredDists.size(); ++i) {

      // Grab the atom pointer at i, a2
      Atoms::Atom a2 = this->atom(i);

      // If a1 and a2 are the same, skip the comparison
      if (i == a1Index) {
        continue;
      }

      // Cache the squared distance between a1 and a2
      const double& curDistSquared = squaredDists[i];

      // Calculate the minimum distance for the atom pair
      const double minDist =
        limitsIAD.getPairDistance(a1->atomicNumber(), a2.atomicNumber());
      if (minDist == PINF) {
        if (atom1 != nullptr && atom2 != nullptr) {
          *atom1 = a1Index;
          *atom2 = i;
          if (IAD != nullptr)
            *IAD = sqrt(curDistSquared);
        }
        Common::error(QString("%1: missing custom interatomic distance "
                              "for atomic numbers %2 and %3.")
                     .arg(__func__)
                     .arg(a1->atomicNumber())
                     .arg(a2.atomicNumber()));
        return false;
      }
      const double minDistWithTol = std::max(0.0, minDist - ZERO08);
      const double minDistSquared = minDistWithTol * minDistWithTol;

      // If the distance is too small, set atom1/atom2 and return false
      if (Common::lt(curDistSquared, minDistSquared, 0.0)) {
        if (atom1 != nullptr && atom2 != nullptr) {
          *atom1 = a1Index;
          *atom2 = i;
          if (IAD != nullptr) {
            *IAD = sqrt(curDistSquared);
          }
        }
        return false;
      }

      // Atom a2 is ok with respect to a1
    }
    // Atom a1 is ok with all a2
  }
  // all distances check out; return true.
  if (atom1 != nullptr && atom2 != nullptr) {
    *atom1 = *atom2 = -1;
    if (IAD != nullptr) {
      *IAD = 0.0;
    }
  }
  return true;
}

bool Xtal::checkInterAtomicDistancesScaled(const EleScaledRadii& limits, int* atom1,
                                           int* atom2, double* IAD)
{
  // Compute a cut off distance -- atoms farther away than this value
  // will abort the check early.
  double maxCheckDistance = 0.0;
  for (const auto& atmn : limits.getRadiiAtomicNumbers()) {
    if (limits.getMinRadius(atmn) > maxCheckDistance)
      maxCheckDistance = limits.getMinRadius(atmn);
  }
  maxCheckDistance *= 2.0;

  const double maxCheckDistSquared = maxCheckDistance * maxCheckDistance;

  // Iterate through all of the atoms in the geometry for "a1"
  for (auto a1 = atoms().begin(), a1_end = atoms().end(); a1 != a1_end; ++a1) {
    const int a1Index = static_cast<int>(a1 - atoms().begin());

    // Get list of minimum squared distances between each atom and a1
    QList<double> squaredDists;
    this->getSquaredAtomicDistancesToPoint((*a1).pos(), &squaredDists);
    Q_ASSERT_X(squaredDists.size() == static_cast<int>(this->numAtoms()), Q_FUNC_INFO,
               "Size of distance list does not match number of atoms.");

    // Cache the minimum radius of a1
    const double minA1Radius = limits.getMinRadius((*a1).atomicNumber());

    // Iterate through each distance
    for (int i = 0; i < squaredDists.size(); ++i) {

      // Grab the atom at i, a2
      Atoms::Atom& a2 = this->atom(i);

      // If a1 and a2 are the same, skip the comparison
      if (i == a1Index) {
        continue;
      }

      // Cache the squared distance between a1 and a2
      const double& curDistSquared = squaredDists[i];

      // Skip comparison if the current distance exceeds the cutoff
      if (curDistSquared > maxCheckDistSquared) {
        continue;
      }

      // Calculate the minimum distance for the atom pair
      const double minDist =
        limits.getMinRadius(a2.atomicNumber()) + minA1Radius;
      const double minDistWithTol = std::max(0.0, minDist - ZERO08);
      const double minDistSquared = minDistWithTol * minDistWithTol;

      // If the distance is too small, set atom1/atom2 and return false
      if (Common::lt(curDistSquared, minDistSquared, 0.0)) {
        if (atom1 != nullptr && atom2 != nullptr) {
          *atom1 = a1Index;
          *atom2 = i;
          if (IAD != nullptr) {
            *IAD = sqrt(curDistSquared);
          }
        }
        return false;
      }

      // Atom a2 is ok with respect to a1
    }
    // Atom a1 is ok will all a2
  }
  // all distances check out; return true.
  if (atom1 != nullptr && atom2 != nullptr) {
    *atom1 = *atom2 = -1;
    if (IAD != nullptr) {
      *IAD = 0.0;
    }
  }
  return true;
}

QString Xtal::getResultsEntry(int objectives_num, int optstep, int objective_offset,
                              int constraints_num) const
{
  QString status = statusText(false);
  const State state = getStatus();
  switch (state) {
    case InProcess:
    case Submitted:
      status = "Step" + QString::number(optstep+1);
      break;
    default:
      break;
  }

  QString out = QString("%1 %2 %3 %4 %5 %6 %7 %8")
      .arg(getRank(), 5)
      .arg(getTag(), 9)
      .arg(getChemicalFormula(), 15)
      .arg(getCompositionString(), 10)
      .arg(getIndex(), 5)
      .arg(getEnthalpyPerAtom(), 12, 'f', 6)
      .arg(getParetoFront(), 5)
      .arg(getDistAboveHull(), 12, 'f', 6);
  for (int i = 0; i < objectives_num; i++) {
    const int objectiveIndex = objective_offset + i;
    if (objectiveIndex < getStrucObjNumber())
      out += fixedWidthDecimal(getStrucObjValues(objectiveIndex), 11, 6);
    else
      out += QString("%1").arg("-", 11);
  }
  for (int i = 0; i < constraints_num; i++) {
    if (i < getStrucConstraintNumber())
      out += fixedWidthDecimal(getStrucConstraintValues(i), 6, 0);
    else
      out += QString("%1").arg("-", 6);
  }
  out += QString("%1 %2")
      .arg(getSpaceGroupNumber(), 6)
      .arg(status, 16);

  return out;
}

QString Xtal::getHullHeader(const QList<QString>& chemSystem)
{
  QString out;
  for (const auto& sym : chemSystem)
    out += QString(" %1").arg(sym, 7);
  out += QString(" %1  # %2 %3 %4  %5")
           .arg("Enthalpy", 14)
           .arg("AboveHullAtm", 14)
           .arg("Pareto", 7)
           .arg("Index", 7)
           .arg("Tag");
  return out;
}

QString Xtal::getHullEntry(const QList<QString>& chemSystem) const
{
  QString out;
  for (const auto& sym : chemSystem)
    out += QString(" %1").arg(getNumberOfAtomsOfSymbol(sym), 7);
  out += QString(" %1").arg(getEnthalpy(), 14, 'f', 6);
  out += QString("  # %1").arg(getDistAboveHull(), 14, 'f', 6);
  out += QString(" %1").arg(getParetoFront(), 7);
  out += QString(" %1").arg(getIndex(), 7);
  out += QString("  %1").arg(getTag());
  return out;
}

// Initialize static members for COB list generation
QMutex Xtal::m_validCOBsGenMutex;
QList<Common::Matrix3> Xtal::m_transformationMatrices;
QList<Common::Matrix3> Xtal::m_mixMatrices;

void Xtal::generateValidCOBs()
{
  m_validCOBsGenMutex.lock();

  // Has another instance beat us to the punch?
  if (m_mixMatrices.size()) {
    m_validCOBsGenMutex.unlock();
    return;
  }

  Common::Matrix3 tmpMat;

  m_transformationMatrices.clear();
  m_transformationMatrices.reserve(32);
  m_mixMatrices.clear();
  m_mixMatrices.reserve(8);

  // Build list of transformation matrices
  // First build list of 90 degree rotations
  tmpMat << 1, 0, 0, 0, 1, 0, 0, 0, 1;
  m_transformationMatrices << tmpMat;
  tmpMat << 1, 0, 0, 0, 0, 1, 0, 1, 0;
  m_transformationMatrices << tmpMat;
  tmpMat << 0, 1, 0, 1, 0, 0, 0, 0, 1;
  m_transformationMatrices << tmpMat;
  tmpMat << 0, 0, 1, 0, 1, 0, 1, 0, 0;
  m_transformationMatrices << tmpMat;
  for (unsigned short int i = 0; i < 4; ++i) {
    // Now apply all possible reflections to 90 rotations
    tmpMat << -1, 0, 0, 0, 1, 0, 0, 0, 1;
    m_transformationMatrices << (tmpMat * m_transformationMatrices[i]);
    tmpMat << 1, 0, 0, 0, -1, 0, 0, 0, 1;
    m_transformationMatrices << (tmpMat * m_transformationMatrices[i]);
    tmpMat << 1, 0, 0, 0, 1, 0, 0, 0, -1;
    m_transformationMatrices << (tmpMat * m_transformationMatrices[i]);
    tmpMat << -1, 0, 0, 0, -1, 0, 0, 0, 1;
    m_transformationMatrices << (tmpMat * m_transformationMatrices[i]);
    tmpMat << -1, 0, 0, 0, 1, 0, 0, 0, -1;
    m_transformationMatrices << (tmpMat * m_transformationMatrices[i]);
    tmpMat << 1, 0, 0, 0, -1, 0, 0, 0, -1;
    m_transformationMatrices << (tmpMat * m_transformationMatrices[i]);
    tmpMat << -1, 0, 0, 0, -1, 0, 0, 0, -1;
    m_transformationMatrices << (tmpMat * m_transformationMatrices[i]);
  }

  // Now build list of mix matrices
  // Identity
  tmpMat << 1, 0, 0, 0, 1, 0, 0, 0, 1;
  m_mixMatrices.append(tmpMat);
  // Upper triangular mixes
  tmpMat << 1, 1, 0, 0, 1, 0, 0, 0, 1;
  m_mixMatrices.append(tmpMat);
  tmpMat << 1, 1, 1, 0, 1, 0, 0, 0, 1;
  m_mixMatrices.append(tmpMat);
  tmpMat << 1, 1, 0, 0, 1, 1, 0, 0, 1;
  m_mixMatrices.append(tmpMat);
  tmpMat << 1, 1, 1, 0, 1, 1, 0, 0, 1;
  m_mixMatrices.append(tmpMat);
  tmpMat << 1, 0, 1, 0, 1, 0, 0, 0, 1;
  m_mixMatrices.append(tmpMat);
  tmpMat << 1, 0, 1, 0, 1, 1, 0, 0, 1;
  m_mixMatrices.append(tmpMat);
  tmpMat << 1, 0, 0, 0, 1, 1, 0, 0, 1;
  m_mixMatrices.append(tmpMat);

  m_validCOBsGenMutex.unlock();
}

Xtal* Xtal::getRandomRepresentation() const
{
  generateValidCOBs();
  // Cache volume for later sanity checks
  const double origVolume = getVolume();

  // Randomly select a mix matrix to create a new cell matrix by
  // taking a linear combination of the current cell vectors
  const Common::Matrix3& mix(m_mixMatrices[Common::getRandUInt() % m_mixMatrices.size()]);

  // Build new Xtal with the new basis
  Xtal* nxtal = new Xtal(this->parent());
  // Set the new cell matrix (no transpose).
  nxtal->setCellInfo(mix * unitCell().cellMatrix());

  Q_ASSERT_X(Common::eq(origVolume, nxtal->getVolume()), Q_FUNC_INFO,
             "Randomized cell volume not "
             "equal to original structure.");

  // Generate a random translation (i.e. between 0 and 1)
  const double maxTranslation = getA() + getB() + getC();
  const Common::Vector3 randTranslation(Common::getRandDouble() * maxTranslation,
                                Common::getRandDouble() * maxTranslation,
                                Common::getRandDouble() * maxTranslation);

  // Add atoms
  for (auto it = atoms().begin(), it_end = atoms().end(); it != it_end; ++it) {
    Atoms::Atom& atom = nxtal->addAtom();
    atom.setAtomicNumber((*it).atomicNumber());
    atom.setPos((*it).pos() + randTranslation);
  }

  // rotate and wrap:
  nxtal->rotateCellAndCoordsToStandardOrientation();
  nxtal->wrapAtomsToCell();
  return nxtal;
}

} // end namespace XtalOpt

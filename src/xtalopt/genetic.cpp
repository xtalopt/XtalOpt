/**********************************************************************
  XtalOptGenetic - Tools necessary for genetic structure optimization

  Copyright (C) 2009-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/genetic.h>

#include <xtalopt/structures/xtal.h>

#include <common/constants.h>
#include <atoms/eleminfo.h>
#include <common/output.h>
#include <common/random.h>

#include <common/numericutils.h>

#include <QStringList>

using namespace std;
using namespace Search;

namespace XtalOpt {

namespace {

template <class T>
QString listToString(const QList<T>& values)
{
  QStringList strings;
  for (const auto& value : values)
    strings.append(QString::number(value));
  return "[" + strings.join(", ") + "]";
}

// Pick which axis is cut first, then select the other two randomly.
QList<int> randomAxisOrder()
{
  QList<int> axes;
  axes.append(static_cast<int>(floor(Common::getRandDouble() * 3)));
  if (axes.at(0) >= 3)
    axes[0] = 2;
  for (int i = 0; i < 3; ++i) {
    if (i != axes.at(0))
      axes.append(i);
  }
  if (Common::getRandDouble() <= 0.5) {
    const int tmp = axes[1];
    axes[1] = axes[2];
    axes[2] = tmp;
  }
  return axes;
}

bool transformCellAndCoordinates(Common::Matrix3& cell,
                                 QList<Common::Vector3>& fractionalCoordinates,
                                 const QList<int>& axes,
                                 bool standardOrientation = false)
{
  Common::Matrix3 transform = Common::Matrix3::Zero();
  for (int i = 0; i < 3; ++i) {
    const int sign = Common::getRandDouble() < 0.5 ? -1 : 1;
    if (axes.at(i) == 0)
      transform.setCol(i, Common::Vector3(sign, 0, 0));
    else if (axes.at(i) == 1)
      transform.setCol(i, Common::Vector3(0, sign, 0));
    else
      transform.setCol(i, Common::Vector3(0, 0, sign));
  }

  const Common::Matrix3 transpose = transform.transpose();
  cell = transpose * cell;
  for (auto& coordinates : fractionalCoordinates)
    coordinates = transpose * coordinates;

  if (!standardOrientation)
    return true;

  if (!Atoms::Geometry::rotateCellAndCoordsToStandardOrientation(cell, fractionalCoordinates, true))
    return false;

  // Shift the atoms randomly along each axis and wrap them back into the
  //   cell, so the cut does not always fall at the same place.
  const double shiftA = Common::getRandDouble();
  const double shiftB = Common::getRandDouble();
  const double shiftC = Common::getRandDouble();
  for (auto& fc : fractionalCoordinates) {
    fc[0] = fmod(fc[0] + shiftA + 100, 1);
    fc[1] = fmod(fc[1] + shiftB + 100, 1);
    fc[2] = fmod(fc[2] + shiftC + 100, 1);
  }

  return true;
}

bool averageCellComponents(const Common::Matrix3& cell1, const Common::Matrix3& cell2,
                           double weight, Common::Matrix3& cell)
{
  Common::Matrix3 result;
  for (uint row = 0; row < 3; ++row) {
    for (uint column = 0; column < 3; ++column) {
      result(row, column) =
        cell1(row, column) * weight + cell2(row, column) * (1.0 - weight);
      if (!GS_ISFINITE(result(row, column)))
        return false;
    }
  }

  if (result.determinant() <= 0.0)
    return false;

  cell = result;
  return true;
}

bool averageCellMetrics(const Common::Matrix3& cell1, const Common::Matrix3& cell2,
                        double weight, Common::Matrix3& cell)
{
  const Common::Matrix3 metric1 = cell1 * cell1.transpose();
  const Common::Matrix3 metric2 = cell2 * cell2.transpose();
  const Common::Matrix3 metric = weight * metric1 + (1.0 - weight) * metric2;
  Common::Matrix3 result = Common::Matrix3::Zero();
  const double scale = std::max(metric(0, 0), std::max(metric(1, 1), metric(2, 2)));
  if (!GS_ISFINITE(scale) || scale <= 0.0)
    return false;
  const double tolerance = ZERO12 * scale;

  double diagonal = metric(0, 0);
  if (!GS_ISFINITE(diagonal) || diagonal <= tolerance)
    return false;
  result(0, 0) = std::sqrt(diagonal);

  result(1, 0) = metric(1, 0) / result(0, 0);
  result(2, 0) = metric(2, 0) / result(0, 0);

  diagonal = metric(1, 1) - result(1, 0) * result(1, 0);
  if (!GS_ISFINITE(diagonal) || diagonal <= tolerance)
    return false;
  result(1, 1) = std::sqrt(diagonal);

  result(2, 1) =
    (metric(2, 1) - result(2, 0) * result(1, 0)) / result(1, 1);

  diagonal = metric(2, 2) - result(2, 0) * result(2, 0) -
             result(2, 1) * result(2, 1);
  if (!GS_ISFINITE(diagonal) || diagonal <= tolerance)
    return false;
  result(2, 2) = std::sqrt(diagonal);

  cell = result;
  return true;
}

inline int findClosestComposition(const QList<uint>& counts, const QList<CellComp>& compa)
{
  // A helper function to find the closest composition in the list
  //   to the given atom counts.
  // If anything goes wrong, return -1; but this shouldn't happen!
  if (compa.isEmpty())
    return -1;
  QList<QString> refSymbols = compa[0].getCompositionSymbols();
  std::vector<double> avglist;
  for (const auto& comp : compa) {
    double avg = 0.0;
    for (int i = 0; i < refSymbols.size(); i++) {
      double c = static_cast<double>(comp.getCount(refSymbols[i])) - static_cast<double>(counts[i]);
      avg += fabs(c);
    }
    avg /= refSymbols.size();
    avglist.push_back(avg);
  }

  return Common::findMinIndex(avglist);
}

// Change the counts to the desired total.
bool rebalanceCountsToTotal(QList<uint>& targetCounts, int desiredTotal)
{
  int Ntypes = targetCounts.size();
  if (Ntypes == 0 || desiredTotal < Ntypes)
    return false;

  int current = 0;
  for (const auto& count : targetCounts)
    current += count;
  int Ndiff = current - desiredTotal; // (-) add, (+) remove
  for (int pass = 0; Ndiff != 0; /*increment below*/) {
    int i = pass % Ntypes;
    if (Ndiff > 0 && targetCounts[i] > 1) {
      targetCounts[i]--; // remove one
      Ndiff -= 1;
    }
    else if (Ndiff < 0) {
      targetCounts[i]++; // add one
      Ndiff += 1;
    }
    pass++;
  }
  return true;
}

// Change the atom counts in the new crystal.
void applyCompositionDeltas(Xtal* nxtal, QList<int>& deltas, QList<uint>& nxtalCounts,
                            const QList<QString>& refSymbols,
                            const EleScaledRadii& elrad, bool useScaledIAD, bool useCustomIAD,
                            const PairCustomDistances* customIADs,
                            int maxAttempts, bool verbose,
                            const QString& tag, const char* context)
{
  for (int i = 0; i < deltas.size(); i++) {
    uint atomicnum = Atoms::ElementInfo::getAtomicNum(refSymbols.at(i).toStdString());
    // No fix is needed
    if (deltas[i] == 0)
      continue;
    // Delete extra atoms
    while (deltas[i] > 0) {
      const std::vector<Atoms::Atom>& atomList = nxtal->atoms();
      double odds = 0.5;
      for (int j = 0; j < static_cast<int>(atomList.size()); j++) {
        if (Common::getRandDouble() < odds &&
            atomList.at(j).atomicNumber() == atomicnum) {
          nxtal->removeAtom(atomList.at(j));
          deltas[i]--;
          nxtalCounts[i]--;
          break;
        }
      }
    }
    // Add atoms randomly
    int currentAttempt = 0;
    while (deltas[i] < 0 && currentAttempt < maxAttempts) {
      currentAttempt++;

      // Add an atom using the same distance checks that final validation uses.
      bool atomAdded = false;
      if (useCustomIAD && customIADs != nullptr) {
        atomAdded = nxtal->addAtomRandomlyCustomIAD(atomicnum, *customIADs);
      } else if (useScaledIAD) {
        atomAdded = nxtal->addAtomRandomlyScaledIAD(atomicnum, elrad);
      } else {
        atomAdded = nxtal->addAtomRandomly(atomicnum, -1.0);
      }

      if (atomAdded) {
        deltas[i]++;
        nxtalCounts[i]++;
      }
    }
    // Just see if we could fix the atom counts for this type
    if (deltas[i] != 0) {
      if (verbose) {
        Common::message(
          QString("   %1: failed to adjust remaining %2 atoms for %3 (%4)")
            .arg(context).arg(deltas[i]).arg(refSymbols[i]).arg(tag));
      }
    }
  }
}

void strain(Xtal* xtal, double sigma_lattice)
{
  // Build Voight strain matrix
  double volume = xtal->getVolume();
  Common::Matrix3 strainM;
  const double NV_MAGICCONST = 4 * exp(-0.5) / sqrt(2.0);
  for (uint row = 0; row < 3; row++) {
    for (uint col = row; col < 3; col++) {
      // Generate random value from a Gaussian distribution.
      // Ported from Python's standard random library.
      // Uses Kinderman and Monahan method. Reference: Kinderman,
      // A.J. and Monahan, J.F., "Computer generation of random
      // variables using the ratio of uniform deviates", ACM Trans
      // Math Software, 3, (1977), pp257-260.
      // mu = 0, sigma = sigma_lattice
      double z;
      while (true) {
        double u1 = Common::getRandDouble();
        double u2 = 1.0 - Common::getRandDouble();
        if (u2 == 0.0)
          continue; // happens a _lot_ with MSVC...
        z = NV_MAGICCONST * (u1 - 0.5) / u2;
        double zz = z * z / 4.0;
        if (zz <= -log(u2))
          break;
      }
      double epsilon = z * sigma_lattice;
      if (col == row) {
        strainM(row, col) = 1 + epsilon;
      } else {
        strainM(row, col) = epsilon / 2.0;
        strainM(col, row) = epsilon / 2.0;
      }
    }
  }

  // Store fractional coordinates
  std::vector<Atoms::Atom>& atomList = xtal->atoms();
  QList<Common::Vector3> fracCoordsList;
  for (const auto& atm : atomList)
    fracCoordsList.append(xtal->cartToFrac(atm.pos()));

  // Apply strain
  xtal->setCellInfo(xtal->unitCell().cellMatrix() * strainM);

  // Reset coordinates
  for (int i = 0; i < static_cast<int>(atomList.size()); i++)
    atomList.at(i).setPos(xtal->fracToCart(fracCoordsList.at(i)));

  // Rescale volume
  xtal->setVolume(volume);
  xtal->wrapAtomsToCell();
}

void ripple(Xtal* xtal, double rho, uint eta, uint mu)
{
  double phase1 = Common::getRandDouble() * 2 * PI;
  double phase2 = Common::getRandDouble() * 2 * PI;

  // Get random direction to shift atoms (x=0, y=1, z=2)
  int shiftAxis = 3, axis1 = 0, axis2 = 0;
  while (shiftAxis >= 3)
    shiftAxis = static_cast<uint>(Common::getRandDouble() * 3);
  switch (shiftAxis) {
    case 0:
      axis1 = 1;
      axis2 = 2;
      break;
    case 1:
      axis1 = 0;
      axis2 = 2;
      break;
    case 2:
      axis1 = 0;
      axis2 = 1;
      break;
    default:
      Common::warning(QString("Something is wrong in the periodic displacement "
                              "operator -- shiftAxis should not be %1")
                        .arg(shiftAxis));
      break;
  }

  std::vector<Atoms::Atom>& atoms = xtal->atoms();
  QList<Common::Vector3> fracCoordsList;

  for (const auto& atm : atoms)
    fracCoordsList.append(xtal->cartToFrac(atm.pos()));

  for (auto& fc : fracCoordsList) {
    Common::Vector3 v = fc;
    double shift = rho * cos(2 * PI * eta * v[axis1] + phase1) *
                   cos(2 * PI * mu * v[axis2] + phase2);
    v[shiftAxis] += shift;
    fc = v;
  }

  for (int i = 0; i < static_cast<int>(atoms.size()); i++) {
    Atoms::Atom& atm = atoms.at(i);
    atm.setPos(xtal->fracToCart(fracCoordsList.at(i)));
  }
  xtal->wrapAtomsToCell();
}

void exchange(Xtal* xtal, uint exchanges)
{
  // Check that there is more than 1 atom type present.
  // If not, print a warning and return input xtal:
  if (xtal->getSymbols().size() <= 1) {
    Common::warning("Cannot perform exchange with fewer than 2 atomic species.");
    return;
  }

  std::vector<Atoms::Atom>& atoms = xtal->atoms();
  // Swap <exchanges> number of atoms
  for (uint ex = 0; ex < exchanges; ex++) {
    // Generate some indices
    uint index1 = 0, index2 = 0;
    // Make sure we're swapping different atom types
    while (atoms.at(index1).atomicNumber() == atoms.at(index2).atomicNumber()) {
      index1 = index2 = 0;
      while (index1 == index2) {
        index1 = Common::getRandUInt(0, atoms.size() - 1);
        index2 = Common::getRandUInt(0, atoms.size() - 1);
      }
    }
    // Swap the atoms
    Common::Vector3 tmp = atoms.at(index1).pos();
    atoms[index1].setPos(atoms.at(index2).pos());
    atoms[index2].setPos(tmp);
  }
  xtal->notifyGeometryChanged();
  return;
}

} // namespace

Xtal* XtalOptGenetic::crossover(Xtal* xtal1, Xtal* xtal2, const QList<CellComp>& compa,
                                const EleScaledRadii& elrad, uint numCuts, double minContribution,
                                double& percent1, double& percent2, int minatoms, int maxatoms,
                                bool isVcSearch, bool verbose, bool useScaledIAD,
                                bool useCustomIAD,
                                const PairCustomDistances* customIADs)
{
  // Save the reference chemical system (i.e., full list of symbols)
  QList<QString> refSymbols = compa[0].getCompositionSymbols();

  // Get parents info: cells, lists of atoms, and fractional coordinates
  Common::Matrix3 cell1;
  QList<uint> xtalCounts1;
  QList<Common::Vector3> fracCoordsList1;
  QList<uint> atomicNumbers1;
  bool validComp1 = false;
  size_t numAtoms1 = 0;
  QString tag1;
  {
    QReadLocker locker(&xtal1->lock());
    cell1 = xtal1->unitCell().cellMatrix();
    for (const auto& symb : refSymbols)
      xtalCounts1.append(xtal1->getNumberOfAtomsOfSymbol(symb));
    const std::vector<Atoms::Atom>& atomList1 = xtal1->atoms();
    for (const auto& atm : atomList1) {
      fracCoordsList1.append(xtal1->cartToFrac(atm.pos()));
      atomicNumbers1.append(atm.atomicNumber());
    }
    validComp1 = xtal1->hasValidComposition();
    numAtoms1 = xtal1->numAtoms();
    tag1 = xtal1->getTag();
  }

  Common::Matrix3 cell2;
  QList<uint> xtalCounts2;
  QList<Common::Vector3> fracCoordsList2;
  QList<uint> atomicNumbers2;
  bool validComp2 = false;
  size_t numAtoms2 = 0;
  QString tag2;
  {
    QReadLocker locker(&xtal2->lock());
    cell2 = xtal2->unitCell().cellMatrix();
    for (const auto& symb : refSymbols)
      xtalCounts2.append(xtal2->getNumberOfAtomsOfSymbol(symb));
    const std::vector<Atoms::Atom>& atomList2 = xtal2->atoms();
    for (const auto& atm : atomList2) {
      fracCoordsList2.append(xtal2->cartToFrac(atm.pos()));
      atomicNumbers2.append(atm.atomicNumber());
    }
    validComp2 = xtal2->hasValidComposition();
    numAtoms2 = xtal2->numAtoms();
    tag2 = xtal2->getTag();
  }

  // Transform cells and atoms (relabel and orient axes, shift and wrap atoms)
  // Each parent is cut using its own random axis order.
  const QList<int> axes1 = randomAxisOrder();
  if (!transformCellAndCoordinates(cell1, fracCoordsList1, axes1, true))
    return nullptr;

  const QList<int> axes2 = randomAxisOrder();
  if (!transformCellAndCoordinates(cell2, fracCoordsList2, axes2, true))
    return nullptr;

  //
  // Build new xtal
  //

  // Average cell metrics by a weight
  //
  //Q_UNUSED(averageCellComponents);
  Q_UNUSED(averageCellMetrics);
  //
  double weight = Common::getRandDouble();
  Common::Matrix3 dims;
  if (!averageCellComponents(cell1, cell2, weight, dims))
    return nullptr;

  Xtal* nxtal = new Xtal();
  QWriteLocker nxtalLocker(&nxtal->lock());

  // Set the new xtal lattice cell
  nxtal->setCellInfo(dims.row(0), dims.row(1), dims.row(2));

  // Where to slice the parent xtals (in fractional units)?
  // If we have a "single-cut" crossover, we will use minimum contribution
  //   settings to partition the parent xtals.
  // In the "multi-cut" case, we start with uniformly distributing cut points
  //   to get ribbons of width "l", and distort those points randomly by up to
  //   "0.25*l" (so, no overlap and hopefully no too narrow ribbons).
  // E.g., with 3 cut points, we will have 4 ribbons with min and max width of
  //   0.125 and 0.375 (fractional coordinate [0,1]).

  int numPoints = numCuts + 2;               // all cut points (adding 2 fixed endpoints 0/1)
  QList<double> corPoints;                   // coordinate of all cut points
  corPoints.reserve(numPoints);
  for (int i = 0; i < numPoints; ++i)
    corPoints.append(0.0);
  corPoints[numPoints - 1] = 1.0;

  // Find the intermediate cut points' coordinates
  //   (we have already set the endpoints to 0.0 and 1.0)
  if (numCuts == 1) {
    // For single-cut, ie. simple crossover, we use the minimum contribution.
    corPoints[1] = ((100.0 - (2.0 * minContribution)) *
                    Common::getRandDouble() + minContribution) / 100.0;
  } else {
    // For multi-cut, we find coordinates for "numCuts" intermediate points.
    double rlen = 1.0 / (numCuts + 1); // initial uniform ribbons' length
    double rtol = rlen * 0.25;         // tolerance for displacing the cut points
    for (int i = 1; i <= static_cast<int>(numCuts); i++) {
      corPoints[i] = i * rlen + Common::getRandDouble(-rtol, rtol);
    }
  }

  // Before moving on, determine the "percentage" of each parent xtal
  //   that will be used in distributing atoms, so we can report it back.
  percent1 = percent2 = 0.0;
  for (int i = 0; i < numPoints - 1; i+=2)
    percent1 += corPoints[i+1] - corPoints[i];
  percent1 *= 100.0;
  percent2  = 100.0 - percent1;

  if (verbose) {
    QString tmp_par = QString("   %1: %2 + %3 cut on axes %4,%5 : ribbon markers %6% %7% : ")
                      .arg(__func__)
                      .arg(tag1)
                      .arg(tag2)
                      .arg(axes1.at(0))
                      .arg(axes2.at(0))
                      .arg(percent1, 5, 'f', 1)
                      .arg(percent2, 5, 'f', 1);
    for (int i = 0; i < numPoints; i++)
      tmp_par += QString(" %1 ").arg(corPoints[i], 10, 'f', 6);
    Common::message(tmp_par);
  }

  // Select atoms of parent xtals for the new xtal.
  // If we have "n" cut points, we will have a total of "n+1" ribbons in each parent
  //   cell (indexed as 1, 2, ..., n+1). Basically, we go over the atoms of each xtal,
  //   find to which ribbon they belong (variable "ribn"), and will pick the atoms of
  //   xtal1 if they are in "odd" ribbons, and those from xtal2 that are in "even" ribbons.
  // For simplicity, we assign the cut points to xtal1 set (note that 0 = 1 in frac coords!)
  //   So, atoms of xtal2 that are on cut points end up "ribbon-less", and discarded.
  // NOTE: although user can do it; but "even number of numCuts" will result in an
  //   uneven atom selection (xtal1 will have more ribbons from which we select atoms).
  //
  // Finally, we will save the index of discarded atoms for possible later use.
  // For extra atoms, if we needed to add them, we will convert their
  //   fractional coordinates to Cartesian in the nxtal cell.

  QMultiHash<uint, int> extraXtal1;
  QMultiHash<uint, int> extraXtal2;

  for (int i = 0; i < fracCoordsList1.size(); i++) {
    uint atmcn = atomicNumbers1.at(i);
    double coor = fracCoordsList1.at(i)[0];
    // In which ribbon this atom is located?
    int ribn = -1;
    for (int p = 1; p < numPoints; p++) {
      if (coor >= corPoints[p-1] && coor <= corPoints[p])
        {ribn = p; break;}
    }
    // Pick xtal1 atoms if they are in odd ribbons
    if (ribn % 2 == 1) {
      Atoms::Atom& newAtom = nxtal->addAtom();
      newAtom.setAtomicNumber(atmcn);
      newAtom.setPos(nxtal->fracToCart(fracCoordsList1.at(i)));
    } else {
      extraXtal1.insert(atmcn, i);
    }
  }

  for (int i = 0; i < fracCoordsList2.size(); i++) {
    uint atmcn = atomicNumbers2.at(i);
    double coor = fracCoordsList2.at(i)[0];
    // In which ribbon this atom is located?
    int ribn = -1;
    for (int p = 1; p < numPoints; p++) {
      if (coor > corPoints[p-1] && coor < corPoints[p])
        {ribn = p; break;}
    }
    // Pick xtal2 atoms if they are in even ribbons
    if (ribn % 2 == 0) {
      Atoms::Atom& newAtom = nxtal->addAtom();
      newAtom.setAtomicNumber(atmcn);
      newAtom.setPos(nxtal->fracToCart(fracCoordsList2.at(i)));
    } else {
      extraXtal2.insert(atmcn, i);
    }
  }

  // Find atom counts of nxtal and extra atom sets.
  // Any -possibly- missing species will have zero atom count
  //   since we check against the full reference symbols.
  QList<uint> nxtalCounts;
  QList<uint> extraCounts1;
  QList<uint> extraCounts2;
  for (const auto& symb : refSymbols) {
    uint atmcn = Atoms::ElementInfo::getAtomicNum(symb.toStdString());
    nxtalCounts.append(nxtal->getNumberOfAtomsOfSymbol(symb));
    extraCounts1.append(extraXtal1.values(atmcn).size());
    extraCounts2.append(extraXtal2.values(atmcn).size());
  }

  // Find "target atom counts" of the new cell by possibly needed
  //   adjustments to the current counts of nxtal:
  // CASE1: variable-composition search: just make sure no element is
  //   absent in the final cell. Here, we also need to make sure that
  //   the min/max atoms limits are maintained.
  // CASE2: fixed- or multi-composition search: if both/any parents have
  //   "valid" composition, we will chose the target from them. Otherwise,
  //   we will select the "best" composition from the list, i.e., the one
  //   that has the closest atom counts to the current nxtal. Either case,
  //   we don't need to be worried about the min/max atom limits here.

  QList<uint> targetCounts; // FC/MC/VC: final target counts?
  int targetParent = -1;    // FC/MC : which parent we choose?
  int chosenComp = -1;      // FC/MC : which composition from list (if needed)?

  if (isVcSearch) {
    int targetTotalCounts = 0;
    for (int i = 0; i < refSymbols.size(); i++) {
      uint numA = (nxtalCounts[i] == 0) ? 1 : nxtalCounts[i];
      targetCounts.append(numA);
      targetTotalCounts += numA;
    }
    // Make sure that the total target count is within the min/max
    //   atom count limits.
    int desiredTotal = targetTotalCounts;
    if      (desiredTotal < minatoms) desiredTotal = minatoms;
    else if (desiredTotal > maxatoms) desiredTotal = maxatoms;

    if (!rebalanceCountsToTotal(targetCounts, desiredTotal)) {
      Common::error("Could not adjust atom counts in crossover!");
      nxtalLocker.unlock();
      delete nxtal;
      return nullptr;
    }
    targetTotalCounts = desiredTotal;
    //
  } else { // fixed/multi-composition search
    // Find the target parent (if any)
    if (validComp1 && validComp2) {
      // If both parents are good, pick the one with larger num of atoms;
      //   then randomly if they are of the same size.
      if (numAtoms1 > numAtoms2)
        targetParent = 1;
      else if (numAtoms2 > numAtoms1)
        targetParent = 2;
      else
        targetParent = Common::getRandDouble() < 0.5 ? 1 : 2;
    } else if (validComp1) {
      targetParent = 1;
    } else if (validComp2) {
      targetParent = 2;
    }

    // Find the target counts: if no parent is found, select from comp list
    if (targetParent == 1) {
      targetCounts = xtalCounts1;
    } else if (targetParent == 2) {
      targetCounts = xtalCounts2;
    } else {
      // Select a target composition from list and use its counts
      chosenComp = findClosestComposition(nxtalCounts, compa);
      // Sanity check: this can't happen!
      if (chosenComp < 0) {
        Common::error("Could not select from composition list in crossover!");
        nxtalLocker.unlock();
        delete nxtal;
        return nullptr;
      }
      for (const auto& symb : refSymbols)
        targetCounts.append(compa[chosenComp].getCount(symb));
    }
  }

  // Now find the "deltas" list for all types, with each element of the
  //   list indicating how many atoms need to be added/removed to fix
  //   the composition:
  //   (deltas[i] > 0): type "i" has extra atoms; we need to remove
  //   (deltas[i] < 0): type "i" is short of atoms; we need to add
  //   (deltas[i] = 0): type "i" has a proper number of atoms

  QList<int> deltas;

  for (int i = 0; i < refSymbols.size(); i++) {
    deltas.append(nxtalCounts[i] - targetCounts[i]);
  }

  if (verbose) {
    Common::message(QString("   %1: counts initial %2 target %3 parent "
                          "%4 comp %5 deltas %6 - %7 %8")
                    .arg(__func__,
                         listToString(nxtalCounts),
                         listToString(targetCounts),
                         QString::number(targetParent),
                         QString::number(chosenComp),
                         listToString(deltas),
                         tag1,
                         tag2));
  }

  // First try to add missing atoms from discarded portions of the parents.
  int maxAttempts = 1000;

  for (int i = 0; i < deltas.size(); i++) {
    if (deltas[i] >= 0)
      continue;

    uint atomicnum = Atoms::ElementInfo::getAtomicNum(refSymbols[i].toStdString());
    int currentAttempt = 0;
    while (deltas[i] < 0 && currentAttempt < maxAttempts) {
      //
      currentAttempt++;
      //
      // Pick the parent: 1/2 chance for each if both have extra atoms of desired
      //   type; otherwise pick the one that has such atoms.
      // If none of the parents have extra atoms of this type; just abort the loop.
      uint parent = 0;
      if (extraCounts1[i] == 0 && extraCounts2[i] == 0)
        break;
      else if (extraCounts1[i] != 0 && extraCounts2[i] != 0)
        parent = (Common::getRandDouble() < 0.5) ? 1 : 2;
      else if (extraCounts1[i] != 0)
        parent = 1;
      else
        parent = 2;
      //
      // Whichever parent we have, it must have atoms of this type!
      //
      if (parent == 1) {
        double odds = 0.5;
        QList<int> extraAtoms = extraXtal1.values(atomicnum);
        for (int j = 0; j < extraAtoms.size(); j++) {
          if (Common::getRandDouble() < odds) {
            int posindx = extraAtoms.value(j);
            //
            Atoms::Atom& newAtom = nxtal->addAtom();
            newAtom.setAtomicNumber(atomicnum);
            newAtom.setPos(nxtal->fracToCart(fracCoordsList1[posindx]));
            //
            extraXtal1.remove(atomicnum, posindx);
            extraCounts1[i]--;
            //
            deltas[i]++;
            nxtalCounts[i]++;
            break;
          }
        }
      } else {
        double odds = 0.5;
        QList<int> extraAtoms = extraXtal2.values(atomicnum);
        for (int j = 0; j < extraAtoms.size(); j++) {
          if (Common::getRandDouble() < odds) {
            int posindx = extraAtoms.value(j);
            //
            Atoms::Atom& newAtom = nxtal->addAtom();
            newAtom.setAtomicNumber(atomicnum);
            newAtom.setPos(nxtal->fracToCart(fracCoordsList2[posindx]));
            //
            extraXtal2.remove(atomicnum, posindx);
            extraCounts2[i]--;
            //
            deltas[i]++;
            nxtalCounts[i]++;
            break;
          }
        }
      }
    }
  }

  // Remove extra atoms and add any remaining missing atoms randomly.
  applyCompositionDeltas(nxtal, deltas, nxtalCounts, refSymbols, elrad,
                         useScaledIAD, useCustomIAD, customIADs, maxAttempts, verbose,
                         tag1 + "+" + tag2, __func__);

  // Done!
  nxtal->wrapAtomsToCell();
  nxtal->setStatus(Xtal::WaitingForOptimization);
  return nxtal;
}

Xtal* XtalOptGenetic::stripple(Xtal* xtal, double sigma_lattice_min,
                               double sigma_lattice_max, double rho_min,
                               double rho_max, uint eta, uint mu,
                               double& sigma_lattice, double& rho)
{
  // lock parent xtal for reading
  QReadLocker locker(&xtal->lock());

  // Copy info over from parent to new xtal
  Xtal* nxtal = new Xtal;
  QWriteLocker nxtalLocker(&nxtal->lock());
  nxtal->setCellInfo(xtal->unitCell().cellMatrix());
  for (uint i = 0; i < xtal->numAtoms(); i++) {
    Atoms::Atom& atm = nxtal->addAtom();
    atm.setAtomicNumber(xtal->atom(i).atomicNumber());
    atm.setPos(xtal->atom(i).pos());
  }

  // unlock the parent xtal
  locker.unlock();

  sigma_lattice = 0;
  rho = 0;

  // Note that this will repeat until EITHER sigma OR rho is greater
  // than its respective minimum value, not both
  do {
    sigma_lattice = Common::getRandDouble();
    sigma_lattice *= sigma_lattice_max;
    rho = Common::getRandDouble();
    rho *= rho_max;
    // If values are fixed (min==max), check to see if they need to
    // be set manually, since it is unlikely that the above
    // randomization will produce an acceptable value. Randomize
    // which parameter to check to avoid biasing setting one value
    // over the other.
    double r = Common::getRandDouble();
    if (r < 0.5 && sigma_lattice_min == sigma_lattice_max && rho < rho_min) {
      sigma_lattice = sigma_lattice_max;
    }
    if (r >= 0.5 && rho_min == rho_max && sigma_lattice < sigma_lattice_min) {
      rho = rho_max;
    }
  } while (sigma_lattice < sigma_lattice_min && rho < rho_min);

  strain(nxtal, sigma_lattice);
  ripple(nxtal, rho, eta, mu);

  nxtal->setStatus(Xtal::WaitingForOptimization);
  return nxtal;
}

Xtal* XtalOptGenetic::permustrain(Xtal* xtal, double sigma_lattice_max,
                                  uint exchanges, double& sigma_lattice)
{
  // lock parent xtal for reading
  QReadLocker locker(&xtal->lock());

  // Copy info over from parent to new xtal
  Xtal* nxtal = new Xtal;
  QWriteLocker nxtalLocker(&nxtal->lock());
  nxtal->setCellInfo(xtal->unitCell().cellMatrix());
  const std::vector<Atoms::Atom>& atoms = xtal->atoms();
  for (const auto& srcAtom : atoms) {
    Atoms::Atom& atom = nxtal->addAtom();
    atom.setAtomicNumber(srcAtom.atomicNumber());
    atom.setPos(srcAtom.pos());
  }

  // unlock the parent xtal
  locker.unlock();

  // Perform lattice strain
  sigma_lattice = sigma_lattice_max * Common::getRandDouble();
  strain(nxtal, sigma_lattice);
  exchange(nxtal, exchanges);

  // Clean up
  nxtal->wrapAtomsToCell();
  nxtal->setStatus(Xtal::WaitingForOptimization);

  return nxtal;
}

Xtal* XtalOptGenetic::permutomic(Xtal* xtal, const CellComp& comp, const EleScaledRadii& elrad,
                                 int minatoms, int maxatoms, bool verbose, bool useScaledIAD,
                                 bool useCustomIAD,
                                 const PairCustomDistances* customIADs)
{
  // Save the reference chemical system (i.e., full list of symbols)
  QList<QString> refSymbols = comp.getCompositionSymbols();

  // lock parent xtal for reading
  QReadLocker locker(&xtal->lock());
  const QString parentTag = xtal->getTag();

  // Copy info over from parent to new xtal
  Xtal* nxtal = new Xtal;
  QWriteLocker nxtalLocker(&nxtal->lock());
  nxtal->setCellInfo(xtal->unitCell().cellMatrix());
  const std::vector<Atoms::Atom>& atoms = xtal->atoms();
  for (const auto& srcAtom : atoms) {
    Atoms::Atom& atom = nxtal->addAtom();
    atom.setAtomicNumber(srcAtom.atomicNumber());
    atom.setPos(srcAtom.pos());
  }

  // unlock the parent xtal
  locker.unlock();

  // First, apply a small "strain" to slightly distort the parent lattice.
  // We will use "half the default maximum strain stdev (= 0.5 * 0.5)"
  double sigma_lattice = 0.25 * Common::getRandDouble();
  strain(nxtal, sigma_lattice);

  // "Working" lists of symbols and atom counts in new xtal
  // We will use the full list of elements, so the output
  //   xtal won't be a sub-system even if the parent is one such.
  QList<uint> nxtalCounts;
  QList<uint> targetCounts;
  for (const auto& symb : refSymbols) {
    nxtalCounts.append(nxtal->getNumberOfAtomsOfSymbol(symb));
    targetCounts.append(nxtal->getNumberOfAtomsOfSymbol(symb));
  }

  int targetTotalCounts = nxtal->numAtoms();

  bool changedComp = false;

  // Start by fixing zero atom counts (if any)
  for (int i = 0; i < targetCounts.size(); i++) {
    if (targetCounts[i] == 0) {
      targetCounts[i]++;
      targetTotalCounts++;
      changedComp = true;
    }
  }

  // Now, we don't have any zero counts in the target composition. But we
  // don't know yet if we are above the max atoms, or we even have made
  //   any changes to the composition.
  // Let's postpone the "total count issue"; and try to alter the target
  //   counts by increasing/decreasing one of the target counts if we
  //   haven't done so in the above step.
  // We'll limit the attempts in fixing the target composition.

  int maxAttempts = 1000;
  int currentAttempt = 0;

  while (!changedComp && currentAttempt < maxAttempts) {
    currentAttempt++;
    // Should we increase (diff=+1) or decrease (diff=-1)?
    int diff;
    if (targetTotalCounts >= maxatoms)
      diff = -1;
    else if (targetTotalCounts <= minatoms)
      diff = +1;
    else
      diff = (Common::getRandDouble() < 0.5) ? -1 : +1;
    //
    for (int i = 0; i < targetCounts.size(); i++) {
      // To avoid any bias in the produced target counts, we
      //   will give equal chances of increasing/decreasing to
      //   every target count; regardless of its current value.
      double odds = 0.5;
      if (diff == -1 && targetCounts[i] > 1 && Common::getRandDouble() < odds) {
        targetCounts[i]--;
        targetTotalCounts--;
        changedComp = true;
        break;
      } else if (diff == +1 && Common::getRandDouble() < odds) {
        targetCounts[i]++;
        targetTotalCounts++;
        changedComp = true;
        break;
      }
    }
  }

  // If we weren't able to change the initial count up to this point,
  //   that's it! We just return the distorted lattice.
  if (!changedComp) {
    nxtal->setStatus(Xtal::WaitingForOptimization);
    return nxtal;
  }

  // Make sure that the total target count is within the min/max
  //   atom count limits.
  int desiredTotal = targetTotalCounts;
  if      (desiredTotal < minatoms) desiredTotal = minatoms;
  else if (desiredTotal > maxatoms) desiredTotal = maxatoms;

  if (!rebalanceCountsToTotal(targetCounts, desiredTotal)) {
    Common::error("Could not adjust atom counts in permutomic!");
    nxtalLocker.unlock();
    delete nxtal;
    return nullptr;
  }
  targetTotalCounts = desiredTotal;

  // So, we have a valid targetCounts that has a total within the
  //   atom count limits, and has no zero counts.

  // Now, find deltas
  // List "deltas" is for all types, with each element of the list:
  //   (deltas[i] > 0): type "i" has extra atoms; we need to remove
  //   (deltas[i] < 0): type "i" is short of atoms; we need to add
  //   (deltas[i] = 0): type "i" has a proper number of atoms
  QList<int> deltas;
  for (int i = 0; i < targetCounts.size(); i++) {
    deltas.append(nxtalCounts[i] - targetCounts[i]);
  }

  if (verbose) {
    Common::message(QString("   %1: counts initial %2 target %3 "
                          "deltas %4 - %5")
                    .arg(__func__,
                         listToString(nxtalCounts),
                         listToString(targetCounts),
                         listToString(deltas),
                         parentTag));
  }

  // Try to fix the atom counts according to the obtained values for deltas
  // For adding atoms, we will limit the attempts, as the radii limits might
  //   prevent us from being able to add them.
  applyCompositionDeltas(nxtal, deltas, nxtalCounts, refSymbols, elrad,
                         useScaledIAD, useCustomIAD, customIADs, maxAttempts,
                         verbose, parentTag, __func__);

  // We're done!
  nxtal->wrapAtomsToCell();
  nxtal->setStatus(Xtal::WaitingForOptimization);
  return nxtal;
}

Xtal* XtalOptGenetic::permucomp(Xtal* xtal, const CellComp& comp, const EleScaledRadii& elrad,
                                int minatoms, int maxatoms, bool verbose, bool useScaledIAD,
                                bool useCustomIAD,
                                const PairCustomDistances* customIADs)
{
  // Save the reference chemical system (i.e., full list of symbols)
  QList<QString> refSymbols = comp.getCompositionSymbols();

  // lock parent xtal for reading
  QReadLocker locker(&xtal->lock());
  const QString parentTag = xtal->getTag();

  // Copy info over from parent to new xtal
  Xtal* nxtal = new Xtal;
  QWriteLocker nxtalLocker(&nxtal->lock());
  nxtal->setCellInfo(xtal->unitCell().cellMatrix());
  const std::vector<Atoms::Atom>& atoms = xtal->atoms();
  for (const auto& srcAtom : atoms) {
    Atoms::Atom& atom = nxtal->addAtom();
    atom.setAtomicNumber(srcAtom.atomicNumber());
    atom.setPos(srcAtom.pos());
  }

  // unlock the parent xtal
  locker.unlock();

  // Initial lists of symbols and atom counts in new xtal
  QList<uint> nxtalCounts;
  for (const auto& symb : refSymbols)
    nxtalCounts.append(nxtal->getNumberOfAtomsOfSymbol(symb));

  // First, apply a small "strain" to slightly distort the parent lattice.
  // We will use "half the default maximum strain stdev (= 0.5 * 0.5)"
  double sigma_lattice = 0.25 * Common::getRandDouble();
  strain(nxtal, sigma_lattice);

  // Now, we create a "new random composition" as follows:
  //  (1) Initiate a list of random counts for all elements each
  //      ranging from "1" and up to "max atoms",
  //  (2) Randomly generate a new total target total atom count in
  //      the range of largest of minatoms/number of types (so we can
  //      have at least one atom per type while maintaining the minatoms)
  //      and up to the maxatoms.
  //  (3) In case the initialized total atom count is larger than the
  //      target total, reduce atom counts of species one by one (making
  //      sure we have at least 1 atom per type) until we reach the desired
  //      total atom count.

  // Initiate the new counts: randomly from 1 to max atom counts
  QList<uint> targetCounts;
  uint rng = static_cast<unsigned int>(maxatoms);
  for (int i = 0; i < nxtalCounts.size(); i++) {
    targetCounts.push_back(Common::getRandUInt(1, rng));
  }

  // Generate a target total atom count between the largest of
  //   "number of types"/"min atoms" and "max atoms"; such that
  //   we are within min/max total count limits, and can have
  //   at least one atom per type.
  // We will then adjust the current counts to match the desired
  //   total atom count.
  int lowestDesired = nxtalCounts.size();
  if (lowestDesired < minatoms) lowestDesired = minatoms;
  //
  int desiredTotal = Common::getRandUInt(lowestDesired, maxatoms);

  if (!rebalanceCountsToTotal(targetCounts, desiredTotal)) {
    Common::error("Could not adjust atom counts in permucomp!");
    nxtalLocker.unlock();
    delete nxtal;
    return nullptr;
  }
  // Now, find deltas
  // List "deltas" is for all types, with each element of the list:
  //   (deltas[i] > 0): type "i" has extra atoms; we need to remove
  //   (deltas[i] < 0): type "i" is short of atoms; we need to add
  //   (deltas[i] = 0): type "i" has a proper number of atoms
  QList<int> deltas;
  for (int i = 0; i < refSymbols.size(); i++) {
    deltas.append(nxtalCounts.at(i) - targetCounts.at(i));
  }

  if (verbose) {
    Common::message(QString("   %1: counts initial %2 target %3 "
                          "deltas %4 - %5")
                    .arg(__func__,
                         listToString(nxtalCounts),
                         listToString(targetCounts),
                         listToString(deltas),
                         parentTag));
  }

  // Correct for differences by inserting or removing atoms.
  // Because of the possible drastic changes in the composition,
  //   it might be impossible to adjust the counts when we need
  //   to add atoms. So, we put a limit on the number of tries.
  // If we reach the limit, we just leave it alone and move on
  //   with whatever count that we have been able to produce.
  applyCompositionDeltas(nxtal, deltas, nxtalCounts, refSymbols, elrad,
                         useScaledIAD, useCustomIAD, customIADs, 1000,
                         verbose, parentTag, __func__);

  // We're done!
  nxtal->wrapAtomsToCell();
  nxtal->setStatus(Xtal::WaitingForOptimization);
  return nxtal;
}

} // namespace XtalOpt

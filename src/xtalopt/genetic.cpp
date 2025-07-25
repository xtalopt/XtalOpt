/**********************************************************************
  XtalOptGenetic - Tools necessary for genetic structure optimization

  Copyright (C) 2009-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/genetic.h>

#include <xtalopt/structures/xtal.h>
#include <xtalopt/ui/dialog.h>

#include <globalsearch/eleminfo.h>
#include <globalsearch/random.h>

#include <globalsearch/utilities/utilityfunctions.h>

#include <QDebug>

using namespace std;
using namespace Eigen;
using namespace GlobalSearch;

namespace XtalOpt {

static inline int findClosestComposition(const QList<uint>& counts,
                                         const QList<CellComp>& compa)
{
  // A helper function to find the closest composition in the list
  //   to the given atom counts.
  // If anything goes wrong, return -1; but this shouldn't happen!
  if (compa.isEmpty())
    return -1;
  QList<QString> refSymbols = compa[0].getSymbols();
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

  return findMinIndex(avglist);
}

Xtal* XtalOptGenetic::crossover(Xtal* xtal1, Xtal* xtal2,
                                const QList<CellComp>& compa,
                                const EleRadii& elrad,
                                uint numCuts,
                                double minContribution,
                                double& percent1, double& percent2,
                                int minatoms,
                                int maxatoms,
                                bool isVcSearch,
                                bool verbose)
{
  // Save the reference chemical system (i.e., full list of symbols)
  QList<QString> refSymbols = compa[0].getSymbols();

  //
  // Random Assignments

  // Shift values = s_n_m:
  //  n = xtal (1,2)
  //  m = axes (1 = a_ch; 2,3 = secondary axes)
  double s_1_1, s_1_2, s_1_3, s_2_1, s_2_2, s_2_3;
  s_1_1 = getRandDouble();
  s_2_1 = getRandDouble();
  s_1_2 = getRandDouble();
  s_2_2 = getRandDouble();
  s_1_3 = getRandDouble();
  s_2_3 = getRandDouble();
  //
  // Transformation matrices
  //
  //  We will rotate and reflect the cell and coords randomly prior
  //  to slicing the cell. This will be done via transformation
  //  matrices.
  //
  //  First, generate a list of the numbers 0-2, using each number
  //  only once. Then construct the matrix via:
  //
  //  0: +/-(1 0 0)
  //  1: +/-(0 1 0)
  //  2: +/-(0 0 1)
  //
  // Column vectors are actually used instead of row vectors so that
  // both the cell matrix and coordinates can be transformed by
  //
  // new = old * xform
  //
  // provided that both new and old are (or are composed of) row
  // vectors. For column vecs/matrices:
  //
  // new = (old.transpose() * xform).transpose()
  //
  // should do the trick.
  //
  QList<int> list1;
  list1.append(static_cast<int>(floor(getRandDouble() * 3)));
  if (list1.at(0) == 3)
    list1[0] = 2;
  switch (list1.at(0)) {
    case 0:
      if (getRandDouble() > 0.5) {
        list1.append(1);
        list1.append(2);
      } else {
        list1.append(2);
        list1.append(1);
      }
      break;
    case 1:
      if (getRandDouble() > 0.5) {
        list1.append(0);
        list1.append(2);
      } else {
        list1.append(2);
        list1.append(0);
      }
      break;
    case 2:
      if (getRandDouble() > 0.5) {
        list1.append(0);
        list1.append(1);
      } else {
        list1.append(1);
        list1.append(0);
      }
      break;
  }

  QList<int> list2;
  list2.append(static_cast<int>(floor(getRandDouble() * 3)));
  if (list2.at(0) == 3)
    list2[0] = 2;
  switch (list2.at(0)) {
    case 0:
      if (getRandDouble() > 0.5) {
        list2.append(1);
        list2.append(2);
      } else {
        list2.append(2);
        list2.append(1);
      }
      break;
    case 1:
      if (getRandDouble() > 0.5) {
        list2.append(0);
        list2.append(2);
      } else {
        list2.append(2);
        list2.append(0);
      }
      break;
    case 2:
      if (getRandDouble() > 0.5) {
        list2.append(0);
        list2.append(1);
      } else {
        list2.append(1);
        list2.append(0);
      }
      break;
  }
  //
  //  Now populate the matrices:
  //
  Matrix3 xform1 = Matrix3::Zero();
  Matrix3 xform2 = Matrix3::Zero();
  for (int i = 0; i < 3; i++) {
    double r1 = getRandDouble() - 0.5;
    double r2 = getRandDouble() - 0.5;
    int s1 = int(r1 / fabs(r1));
    int s2 = int(r2 / fabs(r2));
    if (list1.at(i) == 0)
      xform1.block(0, i, 3, 1) << s1, 0, 0;
    else if (list1.at(i) == 1)
      xform1.block(0, i, 3, 1) << 0, s1, 0;
    else if (list1.at(i) == 2)
      xform1.block(0, i, 3, 1) << 0, 0, s1;
    if (list2.at(i) == 0)
      xform2.block(0, i, 3, 1) << s2, 0, 0;
    else if (list2.at(i) == 1)
      xform2.block(0, i, 3, 1) << 0, s2, 0;
    else if (list2.at(i) == 2)
      xform2.block(0, i, 3, 1) << 0, 0, s2;
  }

  // Get parents info: cells, lists of atoms, and fractional coordinates
  xtal1->lock().lockForRead();
  Matrix3 cell1 = xtal1->unitCell().cellMatrix();
  QList<uint> xtalCounts1;
  for (const auto& symb : refSymbols)
    xtalCounts1.append(xtal1->getNumberOfAtomsOfSymbol(symb));

  const std::vector<Atom>& atomList1 = xtal1->atoms();
  QList<Vector3> fracCoordsList1;
  for (int i = 0; i < atomList1.size(); i++)
    fracCoordsList1.append(xtal1->cartToFrac(atomList1.at(i).pos()));
  xtal1->lock().unlock();

  xtal2->lock().lockForRead();
  Matrix3 cell2 = xtal2->unitCell().cellMatrix();
  QList<uint> xtalCounts2;
  for (const auto& symb : refSymbols)
    xtalCounts2.append(xtal2->getNumberOfAtomsOfSymbol(symb));

  const std::vector<Atom>& atomList2 = xtal2->atoms();
  QList<Vector3> fracCoordsList2;
  for (int i = 0; i < atomList2.size(); i++)
    fracCoordsList2.append(xtal2->cartToFrac(atomList2.at(i).pos()));
  xtal2->lock().unlock();

  // Transform cells and atoms (reflect / rot)
  cell1 *= xform1;
  cell2 *= xform2;
  // Vector3 is a column vector, so transpose before and
  // after transforming them.
  for (int i = 0; i < fracCoordsList1.size(); i++)
    fracCoordsList1[i] = (fracCoordsList1[i].transpose() * xform1).transpose();
  for (int i = 0; i < fracCoordsList2.size(); i++)
    fracCoordsList2[i] = (fracCoordsList2[i].transpose() * xform2).transpose();

  // Shift coordinates:
  for (int i = 0; i < fracCoordsList1.size(); i++) {
    // <QList>[<QList index>][<0=x,1=y,2=z axes>]
    fracCoordsList1[i][0] += s_1_1;
    fracCoordsList1[i][1] += s_1_2;
    fracCoordsList1[i][2] += s_1_3;
  }
  for (int i = 0; i < fracCoordsList2.size(); i++) {
    fracCoordsList2[i][0] += s_2_1;
    fracCoordsList2[i][1] += s_2_2;
    fracCoordsList2[i][2] += s_2_3;
  }

  // Wrap coordinates
  for (int i = 0; i < fracCoordsList1.size(); i++) {
    fracCoordsList1[i][0] = fmod(fracCoordsList1[i][0] + 100, 1);
    fracCoordsList1[i][1] = fmod(fracCoordsList1[i][1] + 100, 1);
    fracCoordsList1[i][2] = fmod(fracCoordsList1[i][2] + 100, 1);
  }
  for (int i = 0; i < fracCoordsList2.size(); i++) {
    fracCoordsList2[i][0] = fmod(fracCoordsList2[i][0] + 100, 1);
    fracCoordsList2[i][1] = fmod(fracCoordsList2[i][1] + 100, 1);
    fracCoordsList2[i][2] = fmod(fracCoordsList2[i][2] + 100, 1);
  }

  //
  // Build new xtal
  //

  // Average cell matrices by a weight
  double weight = getRandDouble();
  Matrix3 dims;
  for (uint row = 0; row < 3; row++) {
    for (uint col = 0; col < 3; col++) {
      dims(row, col) =
        cell1(row, col) * weight + cell2(row, col) * (1 - weight);
    }
  }

  Xtal* nxtal = new Xtal();
  QWriteLocker nxtalLocker(&nxtal->lock());

  // Set the new xtal lattice cell
  nxtal->setCellInfo(dims.col(0), dims.col(1), dims.col(2));

  // Where to slice the parent xtals (in fractional units)?
  // If we have a "single-cut" crossover, we will use minimum contribution
  //   settings to partition the parent xtals.
  // In the "multi-cut" case, we start with uniformly distributing cut points
  //   to get ribbons of width "l", and distort those points randomly by up to
  //   "0.25*l" (so, no overlap and hopefully no too narrow ribbons).
  // E.g., with 3 cut points, we will have 4 ribbons with min and max width of
  //   0.125 and 0.375 (fractional coordinate [0,1]).

  int numPoints = numCuts + 2;               // all cut points (adding 2 fixed endpoints 0/1)
  QVector<double> corPoints(numPoints, 0.0); // coordinate of all cut points
  corPoints[numPoints - 1] = 1.0;

  // Find the intermediate cut points' coordinates
  //   (we have already set the endpoints to 0.0 and 1.0)
  if (numCuts == 1) {
    // For single-cut, ie. simple crossover, we use the minimum contribution.
    corPoints[1] = ((100.0 - (2.0 * minContribution)) *
                    getRandDouble() + minContribution) / 100.0;
  } else {
    // For multi-cut, we find coordinates for "numCuts" intermediate points.
    double rlen = 1.0 / (numCuts + 1); // initial uniform ribbons' length
    double rtol = rlen * 0.25;         // tolerance for displacing the cut points
    for (int i = 1; i <= numCuts; i++) {
      corPoints[i] = i * rlen + getRandDouble(-rtol, rtol);
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
    QString tmp_par = QString("   crossover: ribbon markers %1% %2% : ")
                      .arg(percent1, 5, 'f', 1).arg(percent2, 5, 'f', 1);
    for (int i = 0; i < numPoints; i++)
      tmp_par += QString(" %1 ").arg(corPoints[i], 10, 'f', 6);
    qDebug().noquote() << tmp_par;
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
    uint atmcn = atomList1.at(i).atomicNumber();
    double coor = fracCoordsList1.at(i)[0];
    // In which ribbon this atom is located?
    int ribn = -1;
    for (int p = 1; p < numPoints; p++) {
      if (coor >= corPoints[p-1] && coor <= corPoints[p])
        {ribn = p; break;}
    }
    // Pick xtal1 atoms if they are in odd ribbons
    if (ribn % 2 == 1) {
      Atom& newAtom = nxtal->addAtom();
      newAtom.setAtomicNumber(atmcn);
      newAtom.setPos(nxtal->fracToCart(fracCoordsList1.at(i)));
      //qDebug().noquote() << QString("   atomcell1 type %1 coord %2 ribbon %3 added %4")
      //                      .arg(atmcn, 3).arg(coor, 10,'f',6).arg(ribn, 2).arg("yes");
    } else {
      extraXtal1.insert(atmcn, i);
      //qDebug().noquote() << QString("   atomcell1 type %1 coord %2 ribbon %3 added %4")
      //                      .arg(atmcn, 3).arg(coor, 10,'f',6).arg(ribn, 2).arg("no");
    }
  }

  for (int i = 0; i < fracCoordsList2.size(); i++) {
    uint atmcn = atomList2.at(i).atomicNumber();
    double coor = fracCoordsList2.at(i)[0];
    // In which ribbon this atom is located?
    int ribn = -1;
    for (int p = 1; p < numPoints; p++) {
      if (coor > corPoints[p-1] && coor < corPoints[p])
        {ribn = p; break;}
    }
    // Pick xtal2 atoms if they are in even ribbons
    if (ribn % 2 == 0) {
      Atom& newAtom = nxtal->addAtom();
      newAtom.setAtomicNumber(atmcn);
      newAtom.setPos(nxtal->fracToCart(fracCoordsList2.at(i)));
      //qDebug().noquote() << QString("   atomcell2 type %1 coord %2 ribbon %3 added %4")
      //                      .arg(atmcn, 3).arg(coor, 10,'f',6).arg(ribn, 2).arg("yes");
    } else {
      extraXtal2.insert(atmcn, i);
      //qDebug().noquote() << QString("   atomcell2 type %1 coord %2 ribbon %3 added %4")
      //                      .arg(atmcn, 3).arg(coor, 10,'f',6).arg(ribn, 2).arg("no");
    }
  }

  // Find atom counts of nxtal and extra atom sets.
  // Any -possibly- missing species will have zero atom count
  //   since we check against the full reference symbols.
  QList<uint> nxtalCounts;
  QList<uint> extraCounts1;
  QList<uint> extraCounts2;
  for (const auto& symb : refSymbols) {
    uint atmcn = ElementInfo::getAtomicNum(symb.toStdString());
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

    int Ndiff = targetTotalCounts - desiredTotal; // (-) add, (+) remove
    int Ntypes = targetCounts.size();
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
    targetTotalCounts = desiredTotal;
    //
  } else { // fixed/multi-composition search
    // Find the target parent (if any)
    if (xtal1->hasValidComposition() && xtal2->hasValidComposition()) {
      // If both parents are good, pick the one with larger num of atoms;
      //   then randomly if they are of the same size.
      if (xtal1->numAtoms() > xtal2->numAtoms())
        targetParent = 1;
      else if (xtal2->numAtoms() > xtal1->numAtoms())
        targetParent = 2;
      else
        targetParent = getRandDouble() < 0.5 ? 1 : 2;
    } else if (xtal1->hasValidComposition()) {
      targetParent = 1;
    } else if (xtal2->hasValidComposition()) {
      targetParent = 2;
    }

    // Find the target counts: if no parent is found, select from comp list
    if (targetParent == 1) {
      targetCounts = xtalCounts1;
    } else if (targetParent == 2) {
      targetCounts = xtalCounts2;
    } else {
      // Select a target composition from list and use its counts
      chosenComp = findClosestComposition(nxtalCounts, compa); // getRandInt(0, compa.size() - 1);
      // Sanity check: this can't happen!
      if (chosenComp < 0) {
        qDebug() << "Error could not select from composition list in crossover!";
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
    qDebug() << "   crossover: counts initial" << nxtalCounts
             << "target" << targetCounts << "parent" << targetParent
             << "comp" << (chosenComp) << "deltas" << deltas
             << "-" << xtal1->getTag() << xtal2->getTag();
  }

  // Main loop to correct for differences by inserting atoms (from
  // discarded portions of parents) or removing random atoms.

  // We will be able to remove atoms, anyways. However, for adding,
  //   we might fail, e.g., sub-system parents might not have
  //   enough atoms of desired type at all!
  // So, we limit the number of attempts and will try to add any
  //   remaining number of required atoms randomly afterwards.
  int maxAttempts = 1000;
  int currentAttempt;

  for (int i = 0; i < deltas.size(); i++) {
    uint atomicnum = ElementInfo::getAtomicNum(refSymbols[i].toStdString());
    // Nothing to do for zero deltas
    if (deltas[i] == 0)
      continue;
    // Remove extra atoms
    while (deltas[i] > 0) {
      // Randomly delete atoms from nxtal;
      const std::vector<Atom>& atomList = nxtal->atoms();
      double odds = 0.5; //1.0 / static_cast<double>(nxtalCounts[i]);
      for (int j = 0; j < atomList.size(); j++) {
        if (getRandDouble() < odds &&
            atomList[j].atomicNumber() == atomicnum) {
          // If the atom type and odds are right, delete the atom and break loop to
          //   recheck condition. removeAtom(Atom*) takes care of deleting pointer.
          nxtal->removeAtom(atomList.at(j));
          deltas[i]--;
          nxtalCounts[i]--;
          break;
        }
      }
    }
    // Try to add atoms from discarded parts of the parent cells
    currentAttempt = 0;
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
        parent = (getRandDouble() < 0.5) ? 1 : 2;
      else if (extraCounts1[i] != 0)
        parent = 1;
      else
        parent = 2;
      //
      // Whichever parent we have, it must have atoms of this type!
      //
      if (parent == 1) {
        double odds = 0.5; //1.0 / static_cast<double>(extraCounts1[i]);
        QList<int> extraAtoms = extraXtal1.values(atomicnum);
        for (int j = 0; j < extraAtoms.size(); j++) {
          if (getRandDouble() < odds) {
            int posindx = extraAtoms.value(j);
            //
            Atom& newAtom = nxtal->addAtom();
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
        double odds = 0.5; //1.0 / static_cast<double>(extraCounts2[i]);
        QList<int> extraAtoms = extraXtal2.values(atomicnum);
        for (int j = 0; j < extraAtoms.size(); j++) {
          if (getRandDouble() < odds) {
            int posindx = extraAtoms.value(j);
            //
            Atom& newAtom = nxtal->addAtom();
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
    // Try to add atoms randomly
    currentAttempt = 0;
    while (deltas[i] < 0 && currentAttempt < maxAttempts) {
      currentAttempt++;
      if (nxtal->addAtomRandomly(atomicnum, elrad)) {
        deltas[i]++;
        nxtalCounts[i]++;
      }
    }
    // Just see if we could fix the atom counts for this type
    if (deltas[i] != 0) {
      if (verbose) {
        qDebug().noquote() <<
            QString("   crossover: failed to adjust remaining %1 atoms for %2 (%3)")
            .arg(deltas[i]).arg(refSymbols[i]).arg(nxtal->getTag());
      }
    }
  }

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
    Atom& atm = nxtal->addAtom();
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
    sigma_lattice = getRandDouble();
    sigma_lattice *= sigma_lattice_max;
    rho = getRandDouble();
    rho *= rho_max;
    // If values are fixed (min==max), check to see if they need to
    // be set manually, since it is unlikely that the above
    // randomization will produce an acceptable value. Randomize
    // which parameter to check to avoid biasing setting one value
    // over the other.
    double r = getRandDouble();
    if (r < 0.5 && sigma_lattice_min == sigma_lattice_max && rho < rho_min) {
      sigma_lattice = sigma_lattice_max;
    }
    if (r >= 0.5 && rho_min == rho_max && sigma_lattice < sigma_lattice_min) {
      rho = rho_max;
    }
  } while (sigma_lattice < sigma_lattice_min && rho < rho_min);

  XtalOptGenetic::strain(nxtal, sigma_lattice);
  XtalOptGenetic::ripple(nxtal, rho, eta, mu);

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
  const std::vector<Atom>& atoms = xtal->atoms();
  for (int i = 0; i < atoms.size(); i++) {
    Atom& atom = nxtal->addAtom();
    atom.setAtomicNumber(atoms.at(i).atomicNumber());
    atom.setPos(atoms.at(i).pos());
  }

  // unlock the parent xtal
  locker.unlock();

  // Perform lattice strain
  sigma_lattice = sigma_lattice_max * getRandDouble();
  XtalOptGenetic::strain(nxtal, sigma_lattice);
  XtalOptGenetic::exchange(nxtal, exchanges);

  // Clean up
  nxtal->wrapAtomsToCell();
  nxtal->setStatus(Xtal::WaitingForOptimization);

  return nxtal;
}

Xtal* XtalOptGenetic::permutomic(Xtal* xtal,
                                 const CellComp& comp,
                                 const EleRadii& elrad,
                                 int minatoms,
                                 int maxatoms, bool verbose)
{
  // Save the reference chemical system (i.e., full list of symbols)
  QList<QString> refSymbols = comp.getSymbols();

  // lock parent xtal for reading
  QReadLocker locker(&xtal->lock());

  // Copy info over from parent to new xtal
  Xtal* nxtal = new Xtal;
  QWriteLocker nxtalLocker(&nxtal->lock());
  nxtal->setCellInfo(xtal->unitCell().cellMatrix());
  const std::vector<Atom>& atoms = xtal->atoms();
  for (int i = 0; i < atoms.size(); i++) {
    Atom& atom = nxtal->addAtom();
    atom.setAtomicNumber(atoms.at(i).atomicNumber());
    atom.setPos(atoms.at(i).pos());
  }

  // unlock the parent xtal
  locker.unlock();

  // First, apply a small "strain" to slightly distort the parent lattice.
  // We will use "half the default maximum strain stdev (= 0.5 * 0.5)"
  double sigma_lattice = 0.25 * getRandDouble();
  XtalOptGenetic::strain(nxtal, sigma_lattice);

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
      diff = (getRandDouble() < 0.5) ? -1 : +1;
    //
    for (int i = 0; i < targetCounts.size(); i++) {
      // To avoid any bias in the produced target counts, we
      //   will give equal chances of increasing/decreasing to
      //   every target count; regardless of its current value.
      double odds = 0.5;
      if (diff == -1 && targetCounts[i] > 1 && getRandDouble() < odds) {
        targetCounts[i]--;
        targetTotalCounts--;
        changedComp = true;
        break;
      } else if (diff == +1 && getRandDouble() < odds) {
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
    return nxtal;
  }

  // Make sure that the total target count is within the min/max
  //   atom count limits.
  int desiredTotal = targetTotalCounts;
  if      (desiredTotal < minatoms) desiredTotal = minatoms;
  else if (desiredTotal > maxatoms) desiredTotal = maxatoms;

  int Ndiff = targetTotalCounts - desiredTotal; // (-) add, (+) remove
  int Ntypes = targetCounts.size();
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
    qDebug() << "   permutomic: counts initial" << nxtalCounts
             << "target" << targetCounts << "deltas"
             << deltas << "-" << xtal->getTag();
  }

  // Try to fix the atom counts according to the obtained values for deltas
  // For adding atoms, we will limit the attempts, as the radii limits might
  //   prevent us from being able to add them.
  for (int i = 0; i < deltas.size(); i++) {
    uint atomicnum = ElementInfo::getAtomicNum(refSymbols.at(i).toStdString());
    // No fix is needed
    if (deltas[i] == 0)
      continue;
    // Delete extra atoms
    while (deltas[i] > 0) {
      const std::vector<Atom>& atomList = nxtal->atoms();
      double odds = 0.5; //1.0 / static_cast<double>(nxtalCounts[i]);
      for (int j = 0; j < atomList.size(); j++) {
        if (getRandDouble() < odds &&
            atomList.at(j).atomicNumber() == atomicnum) {
          nxtal->removeAtom(atomList.at(j));
          deltas[i]--;
          nxtalCounts[i]--;
          break;
        }
      }
    }
    // Add atoms randomly
    currentAttempt = 0;
    while (deltas[i] < 0 && currentAttempt < maxAttempts) {
      currentAttempt++;
      if (nxtal->addAtomRandomly(atomicnum, elrad)) {
        deltas[i]++;
        nxtalCounts[i]++;
      }
    }
    // Just see if we could fix the atom counts for this type
    if (deltas[i] != 0) {
      if (verbose) {
        qDebug().noquote() <<
            QString("   permutomic: failed to adjust remaining %1 atoms for %2 (%3)")
            .arg(deltas[i]).arg(refSymbols[i]).arg(xtal->getTag());
      }
    }
  }

  // We're done!
  nxtal->wrapAtomsToCell();
  nxtal->setStatus(Xtal::WaitingForOptimization);
  return nxtal;
}

Xtal* XtalOptGenetic::permucomp(Xtal* xtal,
                                const CellComp& comp,
                                const EleRadii& elrad,
                                int minatoms,
                                int maxatoms, bool verbose)
{
  // Save the reference chemical system (i.e., full list of symbols)
  QList<QString> refSymbols = comp.getSymbols();

  // lock parent xtal for reading
  QReadLocker locker(&xtal->lock());

  // Copy info over from parent to new xtal
  Xtal* nxtal = new Xtal;
  QWriteLocker nxtalLocker(&nxtal->lock());
  nxtal->setCellInfo(xtal->unitCell().cellMatrix());
  const std::vector<Atom>& atoms = xtal->atoms();
  for (int i = 0; i < atoms.size(); i++) {
    Atom& atom = nxtal->addAtom();
    atom.setAtomicNumber(atoms.at(i).atomicNumber());
    atom.setPos(atoms.at(i).pos());
  }

  // unlock the parent xtal
  locker.unlock();

  // Initial lists of symbols and atom counts in new xtal
  QList<uint> nxtalCounts;
  for (const auto& symb : refSymbols)
    nxtalCounts.append(nxtal->getNumberOfAtomsOfSymbol(symb));

  // First, apply a small "strain" to slightly distort the parent lattice.
  // We will use "half the default maximum strain stdev (= 0.5 * 0.5)"
  double sigma_lattice = 0.25 * getRandDouble();
  XtalOptGenetic::strain(nxtal, sigma_lattice);

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
  uint targetTotalCounts = 0;
  uint rng = static_cast<unsigned int>(maxatoms);
  for (int i = 0; i < nxtalCounts.size(); i++) {
    targetCounts.push_back(getRandUInt(1, rng));
    targetTotalCounts += targetCounts[i];
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
  int desiredTotal = getRandUInt(lowestDesired, maxatoms);

  int Ndiff = targetTotalCounts - desiredTotal; // (-) add, (+) remove
  int Ntypes = targetCounts.size();
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
  targetTotalCounts = desiredTotal;

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
    qDebug() << "   permucomp: counts initial" << nxtalCounts
             << "target" << targetCounts << "deltas"
             << deltas << "-" << xtal->getTag();
  }

  // Correct for differences by inserting or removing atoms.

  // Because of the possible drastic changes in the composition,
  //   it might be impossible to adjust the counts when we need
  //   to add atoms. So, we put a limit on the number of tries.
  // If we reach the limit, we just leave it alone and move on
  //   with whatever count that we have been able to produce.
  int maxAttempts = 1000;
  int currentAttempt;

  for (int i = 0; i < deltas.size(); i++) {
    uint atomicnum = ElementInfo::getAtomicNum(refSymbols.at(i).toStdString());
    // No fix is needed
    if (deltas[i] == 0)
      continue;
    // Delete extra atoms
    while (deltas[i] > 0) {
      const std::vector<Atom>& atomList = nxtal->atoms();
      double odds = 0.5; //1.0 / static_cast<double>(nxtalCounts[i]);
      for (int j = 0; j < atomList.size(); j++) {
        if (getRandDouble() < odds &&
            atomList.at(j).atomicNumber() == atomicnum) {
          nxtal->removeAtom(atomList.at(j));
          deltas[i]--;
          nxtalCounts[i]--;
          break;
        }
      }
    }
    // Add atoms randomly
    currentAttempt = 0;
    while (deltas[i] < 0 && currentAttempt < maxAttempts) {
      currentAttempt++;
      if (nxtal->addAtomRandomly(atomicnum, elrad)) {
        deltas[i]++;
        nxtalCounts[i]++;
      }
    }
    // Just see if we could fix the atom counts for this type
    if (deltas[i] != 0) {
      if (verbose) {
        qDebug().noquote() <<
            QString("   permucomp: failed to adjust remaining %1 atoms for %2 (%3)")
            .arg(deltas[i]).arg(refSymbols[i]).arg(xtal->getTag());
      }
    }
  }

  // We're done!
  nxtal->wrapAtomsToCell();
  nxtal->setStatus(Xtal::WaitingForOptimization);
  return nxtal;
}

void XtalOptGenetic::exchange(Xtal* xtal, uint exchanges)
{
  // Check that there is more than 1 atom type present.
  // If not, print a warning and return input xtal:
  if (xtal->getSymbols().size() <= 1) {
    qDebug() << "Warning: Cannot perform exchange with fewer than 2 atomic species.";
    return;
  }

  std::vector<Atom>& atoms = xtal->atoms();
  // Swap <exchanges> number of atoms
  for (uint ex = 0; ex < exchanges; ex++) {
    // Generate some indices
    uint index1 = 0, index2 = 0;
    // Make sure we're swapping different atom types
    while (atoms.at(index1).atomicNumber() == atoms.at(index2).atomicNumber()) {
      index1 = index2 = 0;
      while (index1 == index2) {
        index1 = static_cast<uint>(getRandDouble() * atoms.size());
        index2 = static_cast<uint>(getRandDouble() * atoms.size());
      }
    }
    // Swap the atoms
    Vector3 tmp = atoms.at(index1).pos();
    atoms[index1].setPos(atoms.at(index2).pos());
    atoms[index2].setPos(tmp);
  }
  return;
}

void XtalOptGenetic::strain(Xtal* xtal, double sigma_lattice)
{
  // Build Voight strain matrix
  double volume = xtal->getVolume();
  Matrix3 strainM;
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
        double u1 = getRandDouble();
        double u2 = 1.0 - getRandDouble();
        if (u2 == 0.0)
          continue; // happens a _lot_ with MSVC...
        z = NV_MAGICCONST * (u1 - 0.5) / u2;
        double zz = z * z / 4.0;
        if (zz <= -log(u2))
          break;
      }
      double epsilon = z * sigma_lattice;
      // qDebug() << "epsilon(" << row << ", " << col << ") = " << epsilon;
      if (col == row) {
        strainM(row, col) = 1 + epsilon;
      } else {
        strainM(row, col) = epsilon / 2.0;
        strainM(col, row) = epsilon / 2.0;
      }
    }
  }

  // Store fractional coordinates
  std::vector<Atom>& atomList = xtal->atoms();
  QList<Vector3> fracCoordsList;
  for (int i = 0; i < atomList.size(); i++)
    fracCoordsList.append(xtal->cartToFrac(atomList.at(i).pos()));

  // Apply strain
  xtal->setCellInfo(xtal->unitCell().cellMatrix() * strainM);

  // Reset coordinates
  for (int i = 0; i < atomList.size(); i++)
    atomList.at(i).setPos(xtal->fracToCart(fracCoordsList.at(i)));

  // Rescale volume
  xtal->setVolume(volume);
  xtal->wrapAtomsToCell();
}

void XtalOptGenetic::ripple(Xtal* xtal, double rho, uint eta, uint mu)
{
  double phase1 = getRandDouble() * 2 * PI;
  double phase2 = getRandDouble() * 2 * PI;

  // Get random direction to shift atoms (x=0, y=1, z=2)
  int shiftAxis = 3, axis1, axis2;
  while (shiftAxis == 3)
    shiftAxis = static_cast<uint>(getRandDouble() * 3);
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
      qWarning() << "Something is wrong in the periodic displacement operator "
                    "-- shiftAxis should not be "
                 << shiftAxis;
      break;
  }

  std::vector<Atom>& atoms = xtal->atoms();
  QList<Vector3> fracCoordsList;

  for (int i = 0; i < atoms.size(); i++)
    fracCoordsList.append(xtal->cartToFrac(atoms.at(i).pos()));

  Vector3 v;
  double shift;
  for (int i = 0; i < fracCoordsList.size(); i++) {
    v = fracCoordsList.at(i);
    shift = rho * cos(2 * PI * eta * v[axis1] + phase1) *
            cos(2 * PI * mu * v[axis2] + phase2);
    // qDebug() << "Before: " << v.x() << " " << v.y() << " " << v.z();
    v[shiftAxis] += shift;
    // qDebug() << "After:  " << v.x() << " " << v.y() << " " << v.z();
    fracCoordsList[i] = v;
  }

  for (int i = 0; i < atoms.size(); i++) {
    Atom& atm = atoms.at(i);
    atm.setPos(xtal->fracToCart(fracCoordsList.at(i)));
  }
  xtal->wrapAtomsToCell();
}

} // end namespace XtalOpt

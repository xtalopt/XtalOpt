/**********************************************************************
  spgInit.cpp - Functions for spacegroup initizialization.

  Copyright (C) 2015 by Patrick S. Avery

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <xtalopt/spgInit/spgInit.h>
#include <xtalopt/spgInit/spgInitCombinatorics.h>
#include <xtalopt/spgInit/wyckoffDatabase.h>

// For XtalCompositionStruct
#include <xtalopt/xtalopt.h>

// For vector3
#include <openbabel/generic.h>

// For RANDDOUBLE()
#include <globalsearch/macros.h>

// For FunctionTracker
#include <globalsearch/utilities/functionTracker.h>

#include <tuple>
#include <iostream>

// Define this for debug output
//#define SPGINIT_DEBUG

// Uncomment the right side of this line to output function starts and endings
#define START_FT //FunctionTracker functionTracker(__FUNCTION__);

using namespace std;

#ifdef SPGINIT_DEBUG
static inline void printLatticeInfo(XtalOpt::Xtal* xtal)
{
  cout << "a is " << xtal->getA() << "\n";
  cout << "b is " << xtal->getB() << "\n";
  cout << "c is " << xtal->getC() << "\n";
  cout << "alpha is " << xtal->getAlpha() << "\n";
  cout << "beta is " << xtal->getBeta() << "\n";
  cout << "gamma is " << xtal->getGamma() << "\n";
  cout << "volume is " << xtal->getVolume() << "\n";
}

static inline void printAtomInfo(XtalOpt::Xtal* xtal)
{
  cout << "Frac coords info (blank if none):\n";
  QList<Avogadro::Atom*> atoms = xtal->atoms();
  QList<Eigen::Vector3d> fracCoords;

  for (size_t i = 0; i < atoms.size(); i++)
    fracCoords.append(*(xtal->cartToFrac(atoms.at(i)->pos())));

  for (size_t i = 0; i < atoms.size(); i++) {
    cout << "  For atomic num " <<  atoms.at(i)->atomicNumber() << ", coords are (" << fracCoords.at(i)[0] << "," << fracCoords.at(i)[1] << "," << fracCoords.at(i)[2] << ")\n";
  }
}
#endif

// Basic split of a string based upon a delimiter.
static inline vector<string> split(const string& s, char delim)
{
  vector<string> elems;
  stringstream ss(s);
  string item;
  while (getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

// Basic check to see if a string is a number
// Includes negative numbers
// If it runs into an "x", "y", or "z", it should return false
static inline bool isNumber(const string& s)
{
  std::string::const_iterator it = s.begin();
  while (it != s.end() && (isdigit(*it) || *it == '-' || *it == '.')) ++it;
  return !s.empty() && it == s.end();
}

// A simple function used in the std::sort in the function below
static inline bool greaterThan(const pair<uint, uint>& a,
                               const pair<uint, uint>& b)
{
  return a.first > b.first;
}

static inline bool numIsEven(int num)
{
  if (num % 2 == 0) return true;
  return false;
}

static inline bool numIsOdd(int num)
{
  return !numIsEven(num);
}

// Check if all the multiplicities of a spacegroup are even
static inline bool spgMultsAreAllEven(uint spg)
{
  START_FT;
  wyckoffPositions wyckVector = SpgInit::getWyckoffPositions(spg);
  // An error message should already be printed if this returns false
  if (wyckVector.size() == 0) return false;

  for (size_t i = 0; i < wyckVector.size(); i++) {
    if (numIsOdd(SpgInit::getMultiplicity(wyckVector.at(i)))) return false;
  }
  return true;
}

vector<numAndType> SpgInit::getNumOfEachType(const vector<uint>& atoms)
{
  START_FT;
  vector<uint> atomsAlreadyCounted;
  vector<numAndType> numOfEachType;
  for (size_t i = 0; i < atoms.size(); i++) {
    size_t size = 0;
    // If we already counted this one, just continue
    if (std::find(atomsAlreadyCounted.begin(), atomsAlreadyCounted.end(),
                  atoms.at(i)) != atomsAlreadyCounted.end()) continue;
    for (size_t j = 0; j < atoms.size(); j++)
      if (atoms.at(j) == atoms.at(i)) size++;
    numOfEachType.push_back(make_pair(size, atoms.at(i)));
    atomsAlreadyCounted.push_back(atoms.at(i));
  }
  // Sort from largest to smallest
  sort(numOfEachType.begin(), numOfEachType.end(), greaterThan);
  return numOfEachType;
}

// A unique position is a position that has no x, y, or z in it
bool SpgInit::containsUniquePosition(const wyckPos& pos)
{
  vector<string> xyzStrings = split(getWyckCoords(pos), ',');
  assert(xyzStrings.size() == 3);
  for (size_t i = 0; i < xyzStrings.size(); i++)
    if (!isNumber(xyzStrings.at(i))) return false;
  return true;
}

// This might be a little bit too long to be inline...
static double interpretComponent(const string& component,
                                        double x, double y, double z)
{
  START_FT;
  // If it's just a number, just return the float equivalent
  if (isNumber(component)) return stof(component);

  // '2x' throws off this alrogithm. Just add it here...
  // There is no '2y' or '2z', and when '2x' occurs, it is alone...
  if (component == "2x") return 2 * x;

  // If the position is not a number, there are 3 cases that need to be dealt with:
  // 1. Just a variable x, y, or z
  // 2. A negative x, y, or z
  // 3. a +/- x, y, or z with a float added or subtracted from it

  int i = 0;
  bool varIsNeg = false;

  double ret = 0;

  // Determine whether it is negative or not
  if (component.at(i) == '-') {
    varIsNeg = true;
    i++;
  }

  // Determine whether it is x, y, or z
  switch (component.at(i)) {
    case 'x':
      ret = (varIsNeg) ? -1 * x : x;
      break;
    case 'y':
      ret = (varIsNeg) ? -1 * y : y;
      break;
    case 'z':
      ret = (varIsNeg) ? -1 * z : z;
      break;
    default:
      cout << "Error reading string component: " << component
           << " in interpretComponent()\n";
      return -1;
  }

  // If it's just a single variable, return the double for that variable
  if (component.size() == i + 1) return ret;

  // If not, then we must be adding or subtracting a float to it
  i++;
  bool adding = true;
  if (component.at(i) == '+') adding = true;
  else if (component.at(i) == '-') adding = false;
  else {
    cout << "Error reading string component: " << component
         << " in interpretComponenet()\n";
    return -1;
  }

  // Find the float at the end
  i++;
  double f = stof(component.substr(i));
  ret += (adding) ? f : -1 * f;

  return ret;
}

const wyckoffPositions& SpgInit::getWyckoffPositions(uint spg)
{
  START_FT;
  if (spg < 1 || spg > 230) {
    cout << "Error. getWyckoffPositions() was called for a spacegroup "
         << "that does not exist! Given spacegroup is " << spg << endl;
    return wyckoffPositionsDatabase.at(0);
  }

  return wyckoffPositionsDatabase.at(spg);
}

bool SpgInit::addWyckoffAtomRandomly(XtalOpt::Xtal* xtal, wyckPos& position,
                                     uint atomicNum,
                                     const QHash<unsigned int,
                                                 XtalOpt::XtalCompositionStruct>& limits,
                                     int maxAttempts)
{
  START_FT;
#ifdef SPGINIT_DEBUG
  cout << "At beginning of addWyckoffAtomRandomly(), atom info is:\n";
  printAtomInfo(xtal);
  cout << "Attempting to add an atom at position " << getWyckCoords(position)
       << "\n";
#endif

  INIT_RANDOM_GENERATOR();
  double IAD = -1;

  int i = 0;
  // If this contains a unique position, we only need to try once
  // Otherwise, we'd be repeatedly trying the same thing...
  if (containsUniquePosition(position)) {
    maxAttempts = 1;
  }

  Eigen::Vector3d cartCoords;
  OpenBabel::vector3 fracCoords;
  bool success = true;

  // Cache the minimum radius for the new atom
  const double newMinRadius = limits.value(atomicNum).minRadius;

  // Compute a cut off distance -- atoms farther away than this value
  // will abort the check early.
  double maxCheckDistance = 0.0;
  for (QHash<unsigned int, XtalOpt::XtalCompositionStruct>::const_iterator
       it = limits.constBegin(), it_end = limits.constEnd(); it != it_end;
       ++it) {
    if (it.value().minRadius > maxCheckDistance) {
      maxCheckDistance = it.value().minRadius;
    }
  }
  maxCheckDistance += newMinRadius;
  const double maxCheckDistSquared = maxCheckDistance*maxCheckDistance;

  do {
    success = true;

    // Generate random coordinates in the wyckoff position
    IAD = -1;
    double x = RANDDOUBLE();
    double y = RANDDOUBLE();
    double z = RANDDOUBLE();

    vector<string> components = split(getWyckCoords(position), ',');

    double newX = interpretComponent(components[0], x, y, z);
    double newY = interpretComponent(components[1], x, y, z);
    double newZ = interpretComponent(components[2], x, y, z);

    // interpretComponenet() returns -1 if it failed to read the component
    if (newX == -1 || newY == -1 || newZ == -1) {
      cout << "addWyckoffAtomRandomly() failed due to a component not being "
           << "read successfully!\n";
      return false;
    }

    fracCoords.Set(newX, newY, newZ);

    // Convert to cartesian coordinates and store
    cartCoords = Eigen::Vector3d(xtal->fracToCart(fracCoords).AsArray());

    // If this is the first atom in the lattice, then there's no need to
    // check interatomic distances...
    if (xtal->numAtoms() == 0) break;

    // Compare distance to each atom in xtal with minimum radii
    QVector<double> squaredDists;
    xtal->getSquaredAtomicDistancesToPoint(cartCoords, &squaredDists);
    Q_ASSERT_X(squaredDists.size() == xtal->numAtoms(), Q_FUNC_INFO,
               "Size of distance list does not match number of atoms.");

    for (int dist_ind = 0; dist_ind < squaredDists.size(); ++dist_ind) {
      const double &curDistSquared = squaredDists[dist_ind];
      // Save a bit of time if distance is huge...
      if (curDistSquared > maxCheckDistSquared) {
        continue;
      }
      // Compare distance to minimum:
      const double minDist = newMinRadius + limits.value(
            xtal->atom(dist_ind)->atomicNumber()).minRadius;
      const double minDistSquared = minDist * minDist;

      if (curDistSquared < minDistSquared) {
        success = false;
        break;
      }
    }

    i++;
  } while (i < maxAttempts && !success);

  if (i > maxAttempts) return false;

  Avogadro::Atom* atom = xtal->addAtom();
  Eigen::Vector3d pos (cartCoords[0],cartCoords[1],cartCoords[2]);
  atom->setPos(pos);
  atom->setAtomicNumber(static_cast<int>(atomicNum));

#ifdef SPGINIT_DEBUG
    cout << "After an atom with atomic num " << atomicNum << " was added, the following is the lattice info:\n";
    printLatticeInfo(xtal);
    printAtomInfo(xtal);
#endif

  return true;
}

// vector<pair<wyckPos, atomic number>>
// returns an empty vector if the assignment failed
atomAssignments SpgInit::assignAtomsToWyckPos(uint spg, vector<uint> atoms)
{
  START_FT;
  // Not sure which one is better yet...
  return SpgInitCombinatorics::getRandomAtomAssignments(spg, atoms);
  // return SpgInitCombinatorics::getRandomAtomAssignmentsWithMostWyckLets(
  //                                              spg,
  //                                              atoms);
}

XtalOpt::Xtal* SpgInit::spgInitXtal(uint spg,
                                    const vector<uint>& atoms,
                                    const latticeStruct& latticeMins,
                                    const latticeStruct& latticeMaxes,
                                    const QHash<unsigned int,
                                                 XtalOpt::XtalCompositionStruct>& limits,
                                    int maxAttempts)
{
  START_FT;
  // First let's get a lattice...
  latticeStruct st = generateLatticeForSpg(spg, latticeMins, latticeMaxes);

  // Make sure it's a valid lattice
  if (st.a == 0 || st.b == 0 || st.c == 0 ||
      st.alpha == 0 || st.beta == 0 || st.gamma == 0) {
    cout << "Error in SpgInit::spgInitXtal(): an invalid lattice was "
         << "generated.\n";
    return NULL;
  }

  atomAssignments assignments = assignAtomsToWyckPos(spg, atoms);

  if (assignments.size() == 0) {
    cout << "Error in SpgInit::spgInitXtal(): atoms were not successfully"
         << " assigned positions in assignAtomsToWyckPos()\n";
    return NULL;
  }

#ifdef SPGINIT_DEBUG
  cout << "\natomAssignments are the following (atomicNum, wyckLet, wyckPos):"
       << "\n";
  for (size_t i = 0; i < assignments.size(); i++)
    cout << "  " << assignments.at(i).second << ", "
         << getWyckLet(assignments.at(i).first)
         << ", " << getWyckCoords(assignments.at(i).first) << "\n";
  cout << "\n";
#endif

  XtalOpt::Xtal* xtal = new XtalOpt::Xtal(st.a, st.b, st.c,
                                          st.alpha, st.beta, st.gamma);

  for (size_t i = 0; i < assignments.size(); i++) {
    wyckPos pos = assignments.at(i).first;
    uint atomicNum = assignments.at(i).second;
    if (!addWyckoffAtomRandomly(xtal, pos, atomicNum,
                                limits, maxAttempts)) {
      delete xtal;
      xtal = 0;
      return NULL;
    }
  }

#ifdef SPGINIT_DEBUG
  cout << "\n*********\nBefore fillUnitCell() is called, atom info is:\n";
  printAtomInfo(xtal);
#endif
  xtal->fillUnitCell(spg);
#ifdef SPGINIT_DEBUG
  cout << "\n*********\nAfter fillUnitCell() is called, atom info is:\n";
  printAtomInfo(xtal);
#endif

  // If the correct spacegroup isn't created (happens every once in a while),
  // delete the xtal and return NULL
  if (xtal->getSpaceGroupNumber() != spg) {
#ifdef SPGINIT_DEBUG
    cout << "Spacegroup num, '" << xtal->getSpaceGroupNumber()
         << "', isn't correct! It should be '" << spg << "'!\n";
#endif
    delete xtal;
    xtal = 0;
    return NULL;
  }

  // Otherwise, we succeeded!!
  return xtal;
}

bool SpgInit::isSpgPossible(uint spg, const vector<uint>& atoms)
{
  START_FT;

  if (spg < 1 || spg > 230) return false;

  // Add in a test here to shorten the time checking if a spg is possible
  // If a spacegroup contains all even number multiplicities (many do),
  // and there is an atom with an odd amount, then that spacegroup is not
  // not possible
  vector<numAndType> numOfEachType = getNumOfEachType(atoms);
  bool containsOdd = false;
  for (size_t i = 0; i < numOfEachType.size(); i++) {
    if (numIsOdd(numOfEachType.at(i).first)) {
      containsOdd = true;
      break;
    }
  }

  if (containsOdd && spgMultsAreAllEven(spg)) return false;

  // If the test failed, we must just try to assign atoms and see if it works
  // The third boolean parameter is telling it to find only one combination
  // This speeds it up significantly
  if (SpgInitCombinatorics::getSystemPossibilities(spg, atoms,
                                                   true, false).size() == 0)
    return false;

  return true;
}

template <typename T>
static inline T getSmallest(const T& a, const T& b, const T& c)
{
  if (a <= b && a <= c) return a;
  else if (b <= a && b <= c) return b;
  else return c;
}

template <typename T>
static inline T getLargest(const T& a, const T& b, const T& c)
{
  if (a >= b && a >= c) return a;
  else if (b >= a && b >= c) return b;
  else return c;
}

static inline double getRandDoubleInRange(double min, double max)
{
  // RANDDOUBLE() should generate a random double between 0 and 1
  return RANDDOUBLE() * (max - min) + min;
}

latticeStruct SpgInit::generateLatticeForSpg(uint spg,
                                             const latticeStruct& mins,
                                             const latticeStruct& maxes)
{
  START_FT;
  INIT_RANDOM_GENERATOR();

  latticeStruct st;
  if (spg < 1 || spg > 230) {
    cout << "Error: " << __FUNCTION__ << " was called for a "
         << "non-real spacegroup: " << spg << endl;
    // latticeStruct is initialized to have all "0" values. So just return a
    // "0" struct
    return st;
  }

  // Triclinic!
  else if (spg == 1 || spg == 2) {
    // There aren't really any constraints on a triclinic system...
    st.a     = getRandDoubleInRange(mins.a,     maxes.a);
    st.b     = getRandDoubleInRange(mins.b,     maxes.b);
    st.c     = getRandDoubleInRange(mins.c,     maxes.c);
    st.alpha = getRandDoubleInRange(mins.alpha, maxes.alpha);
    st.beta  = getRandDoubleInRange(mins.beta,  maxes.beta);
    st.gamma = getRandDoubleInRange(mins.gamma, maxes.gamma);
    return st;
  }

  // Monoclinic!
  else if (3 <= spg && spg <= 15) {
    // I am making beta unique here. This may or may not be the right angle
    // to make unique for the wyckoff positions in the database...

    // First make sure we can make alpha and gamma 90 degrees
    if (mins.alpha > 90 || maxes.alpha < 90 ||
        mins.gamma > 90 || maxes.gamma < 90) {
      cout << "Error: " << __FUNCTION__ << " was called for a spacegroup of "
           << spg << " which constrains alpha and gamma to be 90 degrees. The "
           << "input min and max values for the alpha and gamma do not allow "
           << "this. Please change their min and max values.\n";
      return st;
    }

    st.a     = getRandDoubleInRange(mins.a,     maxes.a);
    st.b     = getRandDoubleInRange(mins.b,     maxes.b);
    st.c     = getRandDoubleInRange(mins.c,     maxes.c);
    st.alpha = st.gamma = 90.0;
    st.beta  = getRandDoubleInRange(mins.beta,  maxes.beta);
    return st;
  }

  // Orthorhombic!
  else if (16 <= spg && spg <= 74) {
    // For orthorhombic, all angles must be 90 degrees.
    // Check to see if we can make 90 degree angles
    if (mins.alpha > 90 || maxes.alpha < 90 ||
        mins.beta  > 90 || maxes.beta  < 90 ||
        mins.gamma > 90 || maxes.gamma < 90) {
      cout << "Error: " << __FUNCTION__ << " was called for a spacegroup of "
           << spg << " which constrains all the angles to be 90 degrees. The "
           << "input min and max values for the angles do not allow this. "
           << "Please change their min and max values.\n";
      return st;
    }

    st.a     = getRandDoubleInRange(mins.a,     maxes.a);
    st.b     = getRandDoubleInRange(mins.b,     maxes.b);
    st.c     = getRandDoubleInRange(mins.c,     maxes.c);
    st.alpha = st.beta = st.gamma = 90.0;
    return st;
  }

  // Tetragonal!
  else if (75 <= spg && spg <= 142) {
    // For tetragonal, all angles must be 90 degrees.
    // Check to see if we can make 90 degree angles
    if (mins.alpha > 90 || maxes.alpha < 90 ||
        mins.beta  > 90 || maxes.beta  < 90 ||
        mins.gamma > 90 || maxes.gamma < 90) {
      cout << "Error: " << __FUNCTION__ << " was called for a spacegroup of "
           << spg << " which constrains all the angles to be 90 degrees. The "
           << "input min and max values for the angles do not allow this. "
           << "Please change their min and max values.\n";
      return st;
    }

    // a and b need to be able to be the same number
    // Find the larger min and smaller max of each
    double largerMin = (mins.a > mins.b) ? mins.a : mins.b;
    double smallerMax = (maxes.a < maxes.b) ? maxes.a : maxes.b;

    if (largerMin > smallerMax) {
      cout << "Error: " << __FUNCTION__ << " was called for a spacegroup of "
           << spg << " which constrains a and b to be equal. The "
           << "input min and max values for a and b do not allow this. "
           << "Please change their min and max values.\n";
      return st;
    }

    st.a     = st.b = getRandDoubleInRange(largerMin, smallerMax);
    st.c     = getRandDoubleInRange(mins.c, maxes.c);
    st.alpha = st.beta = st.gamma = 90.0;
    return st;
  }

  // Trigonal!
  // TODO: we are assuming here that all trigonal crystals can be
  // represented with hexagonal axes (and we are ignoring rhombohedral axes).
  // Is this correct?
  else if (143 <= spg && spg <= 167) {
    // For trigonal, alpha and beta must be 90 degrees, and gamma must be 120
    // Check to see if we can make these angles
    if (mins.alpha > 90 || maxes.alpha < 90 ||
        mins.beta  > 90 || maxes.beta  < 90) {
      cout << "Error: " << __FUNCTION__ << " was called for a spacegroup of "
           << spg << " which constrains alpha and beta to be 90 degrees. The "
           << "input min and max values for the angles do not allow this. "
           << "Please change their min and max values.\n";
      return st;
    }

    if (mins.gamma > 120 || maxes.gamma < 120) {
      cout << "Error: " << __FUNCTION__ << " was called for a spacegroup of "
           << spg << " which constrains gamma to be 120 degrees. The "
           << "input min and max values for gamma do not allow this. "
           << "Please change the min and max values.\n";
      return st;
    }

    // a and b need to be able to be the same number
    // Find the larger min and smaller max of each
    double largerMin = (mins.a > mins.b) ? mins.a : mins.b;
    double smallerMax = (maxes.a < maxes.b) ? maxes.a : maxes.b;

    if (largerMin > smallerMax) {
      cout << "Error: " << __FUNCTION__ << " was called for a spacegroup of "
           << spg << " which constrains a and b to be equal. The "
           << "input min and max values for a and b do not allow this. "
           << "Please change their min and max values.\n";
      return st;
    }

    st.a     = st.b = getRandDoubleInRange(largerMin, smallerMax);
    st.c     = getRandDoubleInRange(mins.c, maxes.c);
    st.alpha = st.beta = 90.0;
    st.gamma = 120.0;
    return st;
  }

  // Hexagonal!
  // Note that this is identical to trigonal since in trigonal we are using
  // hexagonal axes.
  else if (168 <= spg && spg <= 194) {
    // For hexagonal, alpha and beta must be 90 degrees, and gamma must be 120
    // Check to see if we can make these angles
    if (mins.alpha > 90 || maxes.alpha < 90 ||
        mins.beta  > 90 || maxes.beta  < 90) {
      cout << "Error: " << __FUNCTION__ << " was called for a spacegroup of "
           << spg << " which constrains alpha and beta to be 90 degrees. The "
           << "input min and max values for the angles do not allow this. "
           << "Please change their min and max values.\n";
      return st;
    }

    if (mins.gamma > 120 || maxes.gamma < 120) {
      cout << "Error: " << __FUNCTION__ << " was called for a spacegroup of "
           << spg << " which constrains gamma to be 120 degrees. The "
           << "input min and max values for gamma do not allow this. "
           << "Please change the min and max values.\n";
      return st;
    }

    // a and b need to be able to be the same number
    // Find the larger min and smaller max of each
    double largerMin = (mins.a > mins.b) ? mins.a : mins.b;
    double smallerMax = (maxes.a < maxes.b) ? maxes.a : maxes.b;

    if (largerMin > smallerMax) {
      cout << "Error: " << __FUNCTION__ << " was called for a spacegroup of "
           << spg << " which constrains a and b to be equal. The "
           << "input min and max values for a and b do not allow this. "
           << "Please change their min and max values.\n";
      return st;
    }

    st.a     = st.b = getRandDoubleInRange(largerMin, smallerMax);
    st.c     = getRandDoubleInRange(mins.c, maxes.c);
    st.alpha = st.beta = 90.0;
    st.gamma = 120.0;
    return st;
  }

  // Cubic!
  /*   ______
      /     /|
     /_____/ |
     |     | |
     |     | /
     |_____|/
  */
  else if (spg >= 195) {
    // We need to make sure that 90 degrees is an option and that a, b, and c
    // can be equal. Otherwise, it is impossible to generate this lattice
    if (mins.alpha > 90 || maxes.alpha < 90 ||
        mins.beta  > 90 || maxes.beta  < 90 ||
        mins.gamma > 90 || maxes.gamma < 90) {
      cout << "Error: " << __FUNCTION__ << " was called for a spacegroup of "
           << spg << " which constrains all the angles to be 90 degrees. The "
           << "input min and max values for the angles do not allow this. "
           << "Please change their min and max values.\n";
      return st;
    }
    // Can a, b, and c be equal?
    // Find the greatest min value and the smallest max value
    double largestMin = getLargest<double>(mins.a, mins.b, mins.c);
    double smallestMax = getSmallest<double>(maxes.a, maxes.b, maxes.c);

    // They can't be equal!
    if (largestMin > smallestMax) {
      cout << "Error: " << __FUNCTION__ << " was called for a spacegroup of "
           << spg << " which constrains a, b, and c to be equal. The "
           << "input min and max values for a, b, and c do not allow this. "
           << "Please change their min and max values.\n";
      return st;
    }

    // If we made it this far, we can set up the cell!
    st.alpha = st.beta = st.gamma = 90.0;
    st.a = st.b = st.c = getRandDoubleInRange(largestMin, smallestMax);

    return st;
  }

  // We shouldn't get to this point because one of the if statements should have
  // worked...
  cout << "Error: " << __FUNCTION__ << " has a problem identifying spg " << spg
       << "\n";
  return st;
}

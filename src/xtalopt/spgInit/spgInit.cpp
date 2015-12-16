/**********************************************************************
  SpgInit - Functions for spacegroup initizialization.

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
#include <xtalopt/spgInit/wyckoffDatabase.h>

// For vector3
#include <openbabel/generic.h>

// For RANDDOUBLE()
#include <globalsearch/macros.h>

#include <tuple>
#include <iostream>

// Define this for debug output
//#define SPGINIT_DEBUG

using namespace std;

// number of atoms and atomic number
typedef pair<uint, uint> numAndType;

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
static bool greaterThan(const pair<uint, uint>& a, const pair<uint, uint>& b) {
  return a.first > b.first;
}

static inline vector<numAndType> getNumOfEachType(const vector<uint> atoms)
{
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
static inline bool containsUniquePosition(const wyckPos& pos)
{
  vector<string> xyzStrings = split(get<2>(pos), ',');
  assert(xyzStrings.size() == 3);
  for (size_t i = 0; i < xyzStrings.size(); i++)
    if (!isNumber(xyzStrings.at(i))) return false;
  return true;
}

const wyckoffPositions& SpgInit::getWyckoffPositions(uint spg)
{
  if (spg < 1 || spg > 230) {
    cout << "Error. getWyckoffPositions() was called for a spacegroup "
         << "that does not exist! Given spacegroup is " << spg << endl;
    return wyckoffPositionsDatabase.at(0);
  }

  return wyckoffPositionsDatabase.at(spg);
}

static inline double interpretComponent(const string& component,
                                        double x, double y, double z)
{
  // If it's just a number, just return the float equivalent
  if (isNumber(component)) return stof(component);

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

bool SpgInit::addWyckoffAtomRandomly(XtalOpt::Xtal* xtal, wyckPos& position,
                                     uint atomicNum, double minIAD,
                                     int maxAttempts)
{

  // If the position is not a number, there are 3 cases that need to be dealt with:
  // 1. Just a variable x, y, or z
  // 2. A negative x, y, or z
  // 3. a +/- x, y, or z with a float added or subtracted from it

  INIT_RANDOM_GENERATOR();
  double IAD = -1;
  int i = 0;
  OpenBabel::vector3 coords;

  do {
    // Generate random coordinates in the wyckoff position
    IAD = -1;
    double x = RANDDOUBLE();
    double y = RANDDOUBLE();
    double z = RANDDOUBLE();

    vector<string> components = split(get<2>(position), ',');

    double newX = interpretComponent(components[0], x, y, z);
    double newY = interpretComponent(components[1], x, y, z);
    double newZ = interpretComponent(components[2], x, y, z);

    // interpretComponenet() returns -1 if it failed to read the component
    if (newX == -1 || newY == -1 || newZ == -1) {
      cout << "addWyckoffAtomRandomly() failed due to a component not being "
           << "read successfully!\n";
      return false;
    }

    coords.Set(newX, newY, newZ);
    if (minIAD != -1) {
      xtal->getNearestNeighborDistance(newX, newY, newZ, IAD);
    }
    else { break;};
    i++;
  } while (i < maxAttempts && IAD <= minIAD);

  if (i >= maxAttempts) return false;

  Avogadro::Atom* atom = xtal->addAtom();
  Eigen::Vector3d pos (coords[0],coords[1],coords[2]);
  atom->setPos(pos);
  atom->setAtomicNumber(static_cast<int>(atomicNum));
  return true;
}

// vector<pair<wyckPos, atomic number>>
// returns an empty vector if the assignment failed
atomAssignments SpgInit::assignAtomsToWyckPos(uint spg, vector<uint> atoms)
{
  // emptyRet is the empty return if the assignment failed
  atomAssignments emptyRet, ret;

  // vector<pair<atomicNum, num>>
  vector<numAndType> numOfEachType = getNumOfEachType(atoms);

  // The "uint" is the wyckPos and the "bool" is whether it's unique or not
  wyckoffPositions wyckVector = getWyckoffPositions(spg);

#ifdef SPGINIT_DEBUG
  cout << "<multiplicity> <unique?> is:\n";
  for (size_t i = 0; i < wyckVector.size(); i++)
    cout << get<1>(wyckVector.at(i)) << " " << containsUniquePosition(wyckVector.at(i)) << "\n";
#endif

  // Keep track of which wyckoff positions have been used
  vector<bool> wyckoffPositionUsed;
  wyckoffPositionUsed.reserve(wyckVector.size());
  for (size_t i = 0; i < wyckVector.size(); i++)
    wyckoffPositionUsed.push_back(false);

  // These are arranged from largest to smallest already
  for (size_t i = 0; i < numOfEachType.size(); i++) {
    bool foundAHome = false;
    uint atomicNum = numOfEachType.at(i).second;
    // Start with the highest wyckoff letter and work our way down
    // This will put as many atoms as possible into the general positions
    // while leaving unique positions for later
    for (int j = wyckVector.size() - 1; j >= 0; j--) {
      uint multiplicity = get<1>(wyckVector.at(j));
      bool unique = containsUniquePosition(wyckVector.at(j));
      // If it's not unique
      if (!unique &&
          // Then check to see if it CAN be used
          numOfEachType.at(i).first % multiplicity == 0) {
        ret.push_back(make_pair(wyckVector.at(j), atomicNum));
        wyckoffPositionUsed[j] = true;
        foundAHome = true;
        break;
      }
      // If it IS unique and hasn't been used
      else if (unique && !wyckoffPositionUsed.at(j) &&
               // Then check to see if they are equivalent
               numOfEachType.at(i).first == multiplicity) {
        ret.push_back(make_pair(wyckVector.at(j), atomicNum));
        wyckoffPositionUsed[j] = true;
        foundAHome = true;
        break;
      }
      // Finally, if it is unique and hasn't been used
      else if (unique && !wyckoffPositionUsed.at(j) &&
               numOfEachType.at(i).first % multiplicity == 0)
      {
        // If it failed the prior test, then this must be a multiple and NOT
        // equivalent (i. e., 4 and 2). Since this is the case, just find a home
        // for the atoms that CAN fit and just proceed to find a home for the
        // others
        wyckoffPositionUsed[j] = true;
        ret.push_back(make_pair(wyckVector.at(j), atomicNum));
        numOfEachType[i].first -= multiplicity;
        i--;
        foundAHome = true;
        break;
      }
    }
#ifdef SPGINIT_DEBUG
    cout << "wyckoffPositionUsed is:\n";
    for (size_t i = 0; i < wyckoffPositionUsed.size(); i++)
      cout << wyckoffPositionUsed[i] << "\n";
#endif
    if (!foundAHome) return emptyRet;
  }
  // If we made it here without returning, every atom type found a home
  return ret;

}

XtalOpt::Xtal* SpgInit::spgInitXtal(uint spg,
                                    const vector<uint>& atoms,
                                    const latticeStruct& latticeMins,
                                    const latticeStruct& latticeMaxes,
                                    double minIAD, int maxAttempts)
{
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

  XtalOpt::Xtal* xtal = new XtalOpt::Xtal(st.a, st.b, st.c,
                                          st.alpha, st.beta, st.gamma);

  for (size_t i = 0; i < assignments.size(); i++) {
    wyckPos pos = assignments.at(i).first;
    uint atomicNum = assignments.at(i).second;
    if (!addWyckoffAtomRandomly(xtal, pos, atomicNum,
                                minIAD, maxAttempts)) {
      delete xtal;
      xtal = 0;
      cout << "In spgInit::spgInitXtal(), failed to add atoms randomly for "
           << "a spg of " << spg << "\n";
      return NULL;
    }
  }

  xtal->fillUnitCell(spg);

  return xtal;
}

vector<atomStruct> SpgInit::generateInitWyckoffs(uint spg,
                                                 const vector<uint> atomTpes)
{

}

bool SpgInit::isSpgPossible(uint spg, const vector<uint>& atoms)
{
#ifdef SPGINIT_DEBUG
  cout << __FUNCTION__ << " called!\n";
  cout << "atoms is:\n";
  for (size_t i = 0; i < atoms.size(); i++) cout << atoms[i] << "\n";
#endif
  if (spg < 1 || spg > 230) return false;

  // If assignAtomsToWyckPos() returns an empty vector, the atoms could not
  // be assigned to produce the spacegroup
  if (assignAtomsToWyckPos(spg, atoms).size() == 0) return false;

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
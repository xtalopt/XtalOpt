/**********************************************************************
  randSpg.cpp - Functions for spacegroup generation.

  Copyright (C) 2015 - 2016 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#include "elemInfo.h"

#include "randSpg.h"
#include "randSpgCombinatorics.h"
#include "wyckoffDatabase.h"
#include "fillCellDatabase.h"
#include "utilityFunctions.h"

// For getRandDouble()
#include "rng.h"

// For FunctionTracker
#include "functionTracker.h"

#include <cassert>
#include <fstream>
#include <tuple>
#include <iostream>

// Define these for debug output
//#define RANDSPG_DEBUG
//#define RANDSPG_WYCK_DEBUG

// Uncomment the right side of this line to output function starts and endings
#define START_FT //FunctionTracker functionTracker(__FUNCTION__);

using namespace std;

// These are externs declared in randSpg.h
string e_logfilename = "randSpg.log";
char e_verbosity = 'r';

// Check if all the multiplicities of a spacegroup are even
static inline bool spgMultsAreAllEven(uint spg)
{
  START_FT;
  wyckoffPositions wyckVector = RandSpg::getWyckoffPositions(spg);
  // An error message should already be printed if this returns false
  if (wyckVector.size() == 0) return false;

  for (size_t i = 0; i < wyckVector.size(); i++) {
    if (numIsOdd(RandSpg::getMultiplicity(wyckVector[i]))) return false;
  }
  return true;
}

vector<numAndType> RandSpg::getNumOfEachType(const vector<uint>& atoms)
{
  START_FT;
  vector<uint> atomsAlreadyCounted;
  vector<numAndType> numOfEachType;
  for (size_t i = 0; i < atoms.size(); i++) {
    size_t size = 0;
    // If we already counted this one, just continue
    if (std::find(atomsAlreadyCounted.begin(), atomsAlreadyCounted.end(),
                  atoms[i]) != atomsAlreadyCounted.end()) continue;
    for (size_t j = 0; j < atoms.size(); j++)
      if (atoms[j] == atoms[i]) size++;
    numOfEachType.push_back(make_pair(size, atoms[i]));
    atomsAlreadyCounted.push_back(atoms[i]);
  }
  // Sort from largest to smallest
  sort(numOfEachType.begin(), numOfEachType.end(), greaterThan);
  return numOfEachType;
}

// Returns true on success and false on failure
bool getNumberInFirstTerm(const string& s, double& result, size_t& len)
{
  len = 0;
  size_t i = 0;

  if (s.size() == 0) {
    cout << "Error in " << __FUNCTION__ << ": the size is 0\n";
    return false;
  }

  bool isNeg = false;
  if (s[i] == '-') {
    isNeg = true;
    i++;
    len++;
  }

  // If it's just a variable, then the number is 1 or -1
  if (s[i] == 'x' || s[i] == 'y' || s[i] == 'z') {
    result = (isNeg) ? -1 : 1;
    return true;
  }

  // Find the length of the number
  size_t j = i;
  while (j < s.size() && (isDigit(s[j]) || s[j] == '.')) {
    len++;
    j++;
  }

  if (len == 0) {
    cout << "Error in " << __FUNCTION__ << ": invalid syntax: " << s << "\n";
    return false;
  }

  double ret;
  // This will throw if it fails to read the number
  try {
    ret = stof(s.substr(i, len));
  }
  catch (...) {
    cout << "Error in " << __FUNCTION__ << ": invalid syntax: " << s << "\n";
    return false;
  }

  result = (isNeg) ? -1 * ret : ret;
  return true;
}

// This might be a little bit too long to be inline...
double RandSpg::interpretComponent(const string& component,
                                   double x, double y, double z)
{
  START_FT;

  if (component.size() == 0) {
    cout << "Error in RandSpg::interpretComponent(): component is empty!\n";
    return -1;
  }

  int i = 0;
  double result = 0;
  while (i < component.size()) {
    // We assume we are adding unless told otherwise
    if (component[i] == '+') i++;

    double numInFront = 0;
    size_t len = 0;
    if (!getNumberInFirstTerm(component.substr(i), numInFront, len)) {
      cout << "Error in " << __FUNCTION__ << " getting number in term.\n";
      return 0;
    }

    // Shift i so that it is past the length of the number
    i += len;
    // If we reached the end, add it and break
    if (i >= component.size()) {
      result += numInFront;
      break;
    }

    // Figure out which variable we are dealing with...
    // or if we are dealing with one at all...
    switch (component[i]) {
      case 'x':
        result += numInFront * x;
        i++;
        break;
      case 'y':
        result += numInFront * y;
        i++;
        break;
      case 'z':
        result += numInFront * z;
        i++;
        break;
      default:
        result += numInFront;
        break;
    }
  }

  return result;
}

const wyckoffPositions& RandSpg::getWyckoffPositions(uint spg)
{
  START_FT;
  if (spg < 1 || spg > 230) {
    cout << "Error. getWyckoffPositions() was called for a spacegroup "
         << "that does not exist! Given spacegroup is " << spg << endl;
    return wyckoffPositionsDatabase[0];
  }

  return wyckoffPositionsDatabase[spg];
}

wyckPos RandSpg::getWyckPosFromWyckLet(uint spg, char wyckLet)
{
  const wyckoffPositions& wyckpos = getWyckoffPositions(spg);
  for (size_t i = 0; i < wyckpos.size(); i++) {
    if (getWyckLet(wyckpos[i]) == wyckLet) return wyckpos[i];
  }
  cout << "Error in " << __FUNCTION__ << ": wyckLet '" << wyckLet
       << "' not found in spg '" << spg  << "'!\n";
  return wyckPos();
}

const fillCellInfo& RandSpg::getFillCellInfo(uint spg)
{
  if (spg < 1 || spg > 230) {
    cout << "Error. getFillCellInfo() was called for a spacegroup "
         << "that does not exist! Given spacegroup is " << spg << endl;
    return fillCellVector[0];
  }
  return fillCellVector[spg];
}

vector<string> RandSpg::getVectorOfDuplications(uint spg)
{
  fillCellInfo fcInfo = getFillCellInfo(spg);
  string duplicateString = fcInfo.first;
  vector<string> ret = splitAndRemoveParenthesis(duplicateString);
  // 0,0,0 should always be the first duplicate -- i. e. identiy
  ret.insert(ret.begin(),"0,0,0");
  return ret;
}

vector<string> RandSpg::getVectorOfFillPositions(uint spg)
{
  fillCellInfo fcInfo = getFillCellInfo(spg);
  string positionsString = fcInfo.second;
  vector<string> ret = splitAndRemoveParenthesis(positionsString);
  return ret;
}

bool RandSpg::addWyckoffAtomRandomly(Crystal& crystal, const wyckPos& position,
                                     uint atomicNum, uint spg, int maxAttempts)
{
  START_FT;
#ifdef RANDSPG_WYCK_DEBUG
  cout << "At beginning of addWyckoffAtomRandomly(), atom info is:\n";
  crystal.printAtomInfo();
  cout << "Attempting to add an atom of atomicNum " << atomicNum
       << " at position " << getWyckCoords(position) << "\n";
#endif

  // If this contains a unique position, we only need to try once
  // Otherwise, we'd be repeatedly trying the same thing...
  if (containsUniquePosition(position)) {
    maxAttempts = 1;
  }

  int i = 0;
  bool success = false;
  do {
    // Generate random coordinates in the wyckoff position
    // Numbers are between 0 and 1
    double x = getRandDouble(0,1);
    double y = getRandDouble(0,1);
    double z = getRandDouble(0,1);

    vector<string> components = split(getWyckCoords(position), ',');

    // Interpret the three components of the Wyckoff position coordinates...
    double newX = interpretComponent(components[0], x, y, z);
    double newY = interpretComponent(components[1], x, y, z);
    double newZ = interpretComponent(components[2], x, y, z);

    // interpretComponenet() returns -1 if it failed to read the component
    if (newX == -1 || newY == -1 || newZ == -1) {
      cout << "addWyckoffAtomRandomly() failed due to a component not being "
           << "read successfully!\n";
      return false;
    }

    atomStruct newAtom(atomicNum, newX, newY, newZ);
    crystal.addAtom(newAtom);

    // Check the interatomic distances
    if (crystal.areIADsOkay(newAtom)) {
      // Now try to fill the cell using this new atom
      if (crystal.fillCellWithAtom(spg, newAtom)) success = true;
    }
    if (!success) {
      // Remove this atom and try again
      crystal.removeAtom(newAtom);
    }

    i++;
  } while (i < maxAttempts && !success);

  if (!success) return false;

#ifdef RANDSPG_WYCK_DEBUG
    cout << "After an atom with atomic num " << atomicNum << " was added and "
         << "the cell filled, the following is the atom info:\n";
    crystal.printAtomInfo();
#endif

  return true;
}

// This just converts the second element in the pair (the 'char') to a wyckPos
static vector<pair<uint, wyckPos>>
getModifiedForcedWyckVector(const vector<pair<uint, char>>& v, uint spg)
{
  vector<pair<uint, wyckPos>> ret;
  for (size_t i = 0; i < v.size(); i++)
    ret.push_back(make_pair(v[i].first,
                          RandSpg::getWyckPosFromWyckLet(spg, v[i].second)));
  return ret;
}

// Convenience function for counting the number of times a forced Wyck
// assignment is used and storing the information in a vector of tuples
static vector<tuple<uint, char, uint>>
getForcedWyckAssignmentsAndNumber(const vector<pair<uint, char>>& forcedWyckAssignments)
{
  vector<pair<uint, char>> alreadyUsedForcedWyckAssignments;
  // The tuple is as follows: <atomicNum, wyckLet, numTimesUsed>
  vector<tuple<uint, char, uint>> forcedWyckAssignmentsAndNumber;
  for (size_t i = 0; i < forcedWyckAssignments.size(); i++) {
    // Check to see if we've already looked at this assignment
    bool alreadyUsed = false;
    for (size_t j = 0; j < alreadyUsedForcedWyckAssignments.size(); j++) {
      if (forcedWyckAssignments[i] ==
          alreadyUsedForcedWyckAssignments[j]) {
        alreadyUsed = true;
        break;
      }
    }
    // If we've already looked at it, just continue
    if (alreadyUsed) continue;
    // Now count how many times we use this
    uint numTimesUsed = 1;
    for (size_t j = i + 1; j < forcedWyckAssignments.size(); j++) {
      if (forcedWyckAssignments[i] == forcedWyckAssignments[j])
        numTimesUsed++;
    }
    alreadyUsedForcedWyckAssignments.push_back(forcedWyckAssignments[i]);
    forcedWyckAssignmentsAndNumber.push_back(make_tuple(
      forcedWyckAssignments[i].first,
      forcedWyckAssignments[i].second,
      numTimesUsed));
  }
  return forcedWyckAssignmentsAndNumber;
}

Crystal createValidCrystal(uint spg, const latticeStruct& latticeMins,
                           const latticeStruct& latticeMaxes,
                           double minVolume, double maxVolume)
{
  Crystal ret;
  // If we fail to do this 1000 times, return an empty crystal
  size_t maxAttempts = 1000;
  size_t numAttempts = 0;
  bool validCrystal = false;
  while (maxAttempts > numAttempts && !validCrystal) {
    numAttempts++;

    // First let's get a lattice...
    latticeStruct st = RandSpg::generateLatticeForSpg(spg, latticeMins, latticeMaxes);
    Crystal crystal(st);

    // Make sure it's a valid lattice
    if (st.a == 0 || st.b == 0 || st.c == 0 ||
        st.alpha == 0 || st.beta == 0 || st.gamma == 0) {
      cout << "Error in RandSpg::createValidCrystal(): an invalid lattice was "
           << "generated.\n";
      return Crystal();
    }

    // Rescale the volume of the crystal if necessary
    if (maxVolume != -1 && crystal.getVolume() > maxVolume)
      // Pick a random number between the min and max volume and rescale to it
      crystal.rescaleVolume(getRandDouble(minVolume, maxVolume));
    else if (minVolume != -1 && crystal.getVolume() < minVolume)
      crystal.rescaleVolume(getRandDouble(minVolume, maxVolume));

    // After rescaling, check again to make sure a, b, and c are within
    // the correct limits
    st = crystal.getLattice();
    if (latticeMins.a <= st.a && st.a <= latticeMaxes.a &&
        latticeMins.b <= st.b && st.b <= latticeMaxes.b &&
        latticeMins.c <= st.c && st.c <= latticeMaxes.c) {
      ret = crystal;
      validCrystal = true;
    }
    // If the crystal is not valid, we'll try again
  }

  // If we get to the point without a valid crystal,
  // we exceeded the max attempts
  if (!validCrystal) {
    cerr << "After " << maxAttempts
         << " attempts, a valid crystal could not be made for "
         << "spg '" << spg << "' and the given latticeMins, latticeMaxes, "
         << "minVolume of '" << minVolume << "' and maxVolume of '"
         << maxVolume << "'\n";
    cerr << "Aborting this crystal.\n";
    return Crystal();
  }
  return ret;
}

Crystal RandSpg::randSpgCrystal(const randSpgInput& input)
{
  START_FT;

  // Convenience: so we don't have to say 'input.<option>' for every call
  uint spg                                                      = input.spg;
  const vector<uint>& atoms                                     = input.atoms;
  const latticeStruct& latticeMins                              = input.latticeMins;
  const latticeStruct& latticeMaxes                             = input.latticeMaxes;
  double IADScalingFactor                                       = input.IADScalingFactor;
  double minRadius                                              = input.minRadius;
  const std::vector<std::pair<uint, double>>& manualAtomicRadii = input.manualAtomicRadii;
  double minVolume                                              = input.minVolume;
  double maxVolume                                              = input.maxVolume;
  vector<pair<uint, char>> forcedWyckAssignments                = input.forcedWyckAssignments;
  char verbosity                                                = input.verbosity;
  int numAttempts                                               = input.maxAttempts;
  bool forceMostGeneralWyckPos                                  = input.forceMostGeneralWyckPos;

  // Change the atomic radii as necessary
  ElemInfo::applyScalingFactor(IADScalingFactor);

  // Set the min radius
  ElemInfo::setMinRadius(minRadius);

  // Set some explicit radii
  for (size_t i = 0; i < manualAtomicRadii.size(); i++) {
    uint atomicNum = manualAtomicRadii[i].first;
    double rad = manualAtomicRadii[i].second;
    ElemInfo::setRadius(atomicNum, rad);
  }

  systemPossibilities possibilities = RandSpgCombinatorics::getSystemPossibilities(spg, atoms);

  if (possibilities.size() == 0) {
    cout << "Error in RandSpg::" << __FUNCTION__ << "(): this spg '" << spg
         << "' cannot be generated with this composition\n";
    return Crystal();
  }

  // force the most general Wyckoff position to be used at least once?
  if (forceMostGeneralWyckPos)
    possibilities = RandSpgCombinatorics::removePossibilitiesWithoutGeneralWyckPos(possibilities, spg);

  if (possibilities.size() == 0) {
    cout << "Error in RandSpg::" << __FUNCTION__ << "(): this spg '" << spg
         << "' cannot be generated with this composition.\n";
    cout << "It can be generated if option 'forceMostGeneralWyckPos' is "
         << "turned off, but the correct spacegroup will not be guaranteed.\n";
    return Crystal();
  }

  // The tuple is as follows: <atomicNum, wyckLet, numTimesUsed>
  vector<tuple<uint, char, uint>> forcedWyckAssignmentsAndNumber = getForcedWyckAssignmentsAndNumber(forcedWyckAssignments);

  for (size_t i = 0; i < forcedWyckAssignmentsAndNumber.size(); i++) {
    possibilities =
      RandSpgCombinatorics::removePossibilitiesWithoutWyckPos(possibilities,
                  get<1>(forcedWyckAssignmentsAndNumber[i]),
                  get<2>(forcedWyckAssignmentsAndNumber[i]),
                  get<0>(forcedWyckAssignmentsAndNumber[i]));
  }

  if (possibilities.size() == 0) {
    cout << "Error in RandSpg::" << __FUNCTION__ << "(): this spg '" << spg
         << "' cannot be generated with this composition due to the forced "
         << "Wyckoff position constraints.\nPlease change them or remove them "
         << "if you wish to generate the space group.\n";
    return Crystal();
  }

  //RandSpgCombinatorics::printSystemPossibilities(possibilities);
  // If we desire verbose output, print the system possibility to the log file
  if (verbosity == 'v')
    appendToLogFile(RandSpgCombinatorics::getVerbosePossibilitiesString(possibilities));

  // Create a modified forced wyck vector for later...
  vector<pair<uint, wyckPos>> modifiedForcedWyckVector = getModifiedForcedWyckVector(forcedWyckAssignments, spg);

  // Begin the attempt loop!
  for (size_t i = 0; i < numAttempts; i++) {

    Crystal crystal = createValidCrystal(spg, latticeMins, latticeMaxes,
                                         minVolume, maxVolume);

    // Now, let's assign some atoms!
    atomAssignments assignments = RandSpgCombinatorics::getRandomAtomAssignments(possibilities, modifiedForcedWyckVector);

    //printAtomAssignments(assignments);
    // If we desire any output, print the atom assignments to the log file
    if (verbosity == 'r' || verbosity == 'v')
      appendToLogFile(getAtomAssignmentsString(assignments));

    if (assignments.size() == 0) {
      cout << "Error in RandSpg::randSpgXtal(): atoms were not successfully"
           << " assigned positions in assignAtomsToWyckPos()\n";
      continue;
    }

#ifdef RANDSPG_DEBUG
    cout << "\natomAssignments are the following (atomicNum, wyckLet, wyckPos):"
         << "\n";
    for (size_t j = 0; j < assignments.size(); j++)
      cout << "  " << assignments[j].second << ", "
           << getWyckLet(assignments[j].first)
           << ", " << getWyckCoords(assignments[j].first) << "\n";
    cout << "\n";
#endif

    bool assignmentsSuccessful = true;
    for (size_t j = 0; j < assignments.size(); j++) {
      const wyckPos& pos = assignments[j].first;
      uint atomicNum = assignments[j].second;
      if (!addWyckoffAtomRandomly(crystal, pos, atomicNum, spg)) {
        assignmentsSuccessful = false;
        break;
      }
    }

    // If we succeeded, and the number of atoms match, return the crystal!
    // There are rare cases where an atom may be placed on top of another
    // one and the essentially get merged into one. We check to make sure the
    // sizes of the atomic numbers match for this reason. We shouldn't have to
    // worry about types.
    if (assignmentsSuccessful &&
        atoms.size() == crystal.getVectorOfAtomicNums().size()) {
      if (verbosity != 'n') appendToLogFile("*** Success! ***\n");
      return crystal;
    }
    else {
      if (verbosity == 'r' || verbosity == 'v') {
        stringstream ss;
        ss << "Failed to add atoms to satisfy MinIAD.\nObtaining new atom "
           << "assignments and trying again. Failure count: " << i + 1 << "\n\n";
        appendToLogFile(ss.str());
      }
      continue;
    }
  }

  // If we made it here, we failed to generate the crystal
  stringstream errMsg;
  errMsg << "After " << numAttempts << " attempts: failed to generate "
         << "a crystal of spg " << spg << ".\n";
  if (verbosity != 'n') appendToLogFile(errMsg.str());
  cerr << errMsg.str();
  return Crystal();
}

bool RandSpg::isSpgPossible(uint spg, const vector<uint>& atoms)
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
    if (numIsOdd(numOfEachType[i].first)) {
      containsOdd = true;
      break;
    }
  }

  if (containsOdd && spgMultsAreAllEven(spg)) return false;

  // If the test failed, we must just try to assign atoms and see if it works
  // The third boolean parameter is telling it to find only one combination
  // This speeds it up significantly
  if (RandSpgCombinatorics::getSystemPossibilities(spg, atoms,
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

latticeStruct RandSpg::generateLatticeForSpg(uint spg,
                                             const latticeStruct& mins,
                                             const latticeStruct& maxes)
{
  START_FT;

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
    st.a     = getRandDouble(mins.a,     maxes.a);
    st.b     = getRandDouble(mins.b,     maxes.b);
    st.c     = getRandDouble(mins.c,     maxes.c);
    st.alpha = getRandDouble(mins.alpha, maxes.alpha);
    st.beta  = getRandDouble(mins.beta,  maxes.beta);
    st.gamma = getRandDouble(mins.gamma, maxes.gamma);
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

    st.a     = getRandDouble(mins.a,     maxes.a);
    st.b     = getRandDouble(mins.b,     maxes.b);
    st.c     = getRandDouble(mins.c,     maxes.c);
    st.alpha = st.gamma = 90.0;
    st.beta  = getRandDouble(mins.beta,  maxes.beta);
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

    st.a     = getRandDouble(mins.a,     maxes.a);
    st.b     = getRandDouble(mins.b,     maxes.b);
    st.c     = getRandDouble(mins.c,     maxes.c);
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

    st.a     = st.b = getRandDouble(largerMin, smallerMax);
    st.c     = getRandDouble(mins.c, maxes.c);
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

    st.a     = st.b = getRandDouble(largerMin, smallerMax);
    st.c     = getRandDouble(mins.c, maxes.c);
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

    st.a     = st.b = getRandDouble(largerMin, smallerMax);
    st.c     = getRandDouble(mins.c, maxes.c);
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
    st.a = st.b = st.c = getRandDouble(largestMin, smallestMax);

    return st;
  }

  // We shouldn't get to this point because one of the if statements should have
  // worked...
  cout << "Error: " << __FUNCTION__ << " has a problem identifying spg " << spg
       << "\n";
  return st;
}

string RandSpg::getAtomAssignmentsString(const atomAssignments& a)
{
  stringstream s;
  s << "printing atom assignments:\n";
  s << "Atomic num : Wyckoff letter\n";
  for (size_t i = 0; i < a.size(); i++) {
    s << a[i].second << " : " << getWyckLet(a[i].first) << "\n";
  }
  return s.str();
}

void RandSpg::printAtomAssignments(const atomAssignments& a)
{
  cout << getAtomAssignmentsString(a);
}

// The name of the log file is available in the header as an extern
void RandSpg::appendToLogFile(const std::string& text)
{
  fstream fs;
  fs.open(e_logfilename, std::fstream::out | std::fstream::app);

  if (!fs.is_open()) {
    cout << "Error opening log file, " << e_logfilename << ".\n"
         << "The program will keep running, but log info will not be written.\n";
    return;
  }

  fs << text;

  fs.close();
}

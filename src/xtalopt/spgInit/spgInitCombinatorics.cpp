/**********************************************************************
  spgInitCombinatorics.cpp - Functions for solving the complicated combinatorics
                             problems for spacegroup initialization

  Copyright (C) 2015 by Patrick S. Avery

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <iostream>

#include <xtalopt/spgInit/spgInit.h>
#include <xtalopt/spgInit/spgInitCombinatorics.h>
#include <xtalopt/spgInit/wyckPosTrackingInfo.h>

// For FunctionTracker
#include <globalsearch/utilities/functionTracker.h>

// Uncomment the right side of this line to output function starts and endings
#define START_FT //FunctionTracker functionTracker(__FUNCTION__);

using namespace std;

// Define this for debug output
//#define PRINT_SPG_INIT_COMB_DEBUG

// This is used in 'findAllCombinations()'
struct combinationSettings {
  uint numAtoms;
  bool findOnlyOne;
  bool findOnlyNonUnique;
  combinationSettings() :
    numAtoms(0),
    findOnlyOne(false),
    findOnlyNonUnique(false)
    {}
  combinationSettings(uint nAtoms, bool findOne, bool findNonUnique) :
    numAtoms(nAtoms),
    findOnlyOne(findOne),
    findOnlyNonUnique(findNonUnique)
    {}
};

typedef std::vector<WyckPosTrackingInfo> usageTracker;

#ifdef PRINT_SPG_INIT_COMB_DEBUG
static inline void printSingleAtomPossibility(const singleAtomPossibility&
                                              possib)
{
  cout << "   singleAtomPossibility is:\n";
  for (size_t i = 0; i < possib.size(); i++)
    cout << "    " << SpgInit::getWyckLet(possib.at(i)) << "\n";
}

static inline void printSingleAtomPossibilities(const singleAtomPossibilities&
                                                possibs)
{
  cout << "Printing single atom possibilities!\n";
  for (size_t i = 0; i < possibs.size(); i++) {
    cout << " at i == " << i << ":\n";
    printSingleAtomPossibility(possibs.at(i));
  }
  cout << "\n\n";
}

static inline void printSystemPossibility(const systemPossibility& possib)
{
  cout << " Printing system possibility\n";
  for (size_t i = 0; i < possib.size(); i++) {
    cout << "  at i == " << i << ":\n";
    cout << "   atomic num is " << possib.at(i).first << "\n";
    printSingleAtomPossibility(possib.at(i).second);
  }
}

static inline void printSystemPossibilities(const systemPossibilities& possib)
{
  cout << "Printing system possibilities\n";
  for (size_t i = 0; i < possib.size(); i++) {
    printSystemPossibility(possib.at(i));
  }
  cout << "\n\n";
}

static inline void printAtomAssignments(const atomAssignments& assigns)
{
  cout << "Printing atom assignments!\n";
  for (size_t i = 0; i < assigns.size(); i++) {
    cout << "  atomicNum is '" << assigns.at(i).second << "' and wyckLet is '"
         << SpgInit::getWyckLet(assigns.at(i).first) << "'.\n";
  }
  cout << "\n\n";
}
#endif

template<typename T>
static bool vecContains(const vector<T>& vec, const T& element)
{
  for (size_t i = 0; i < vec.size(); i++) {
    if (vec.at(i) == element) return true;
  }
  return false;
}

static inline uint getNumAtomsUsed(const usageTracker& tracker)
{
  size_t sum = 0;
  for (size_t i = 0; i < tracker.size(); i++)
    sum += ((tracker.at(i).multiplicity) * tracker.at(i).numTimesUsed);
  return sum;
}

static inline int getFirstAvailableIndex(const usageTracker& tracker)
{
  for (size_t i = 0; i < tracker.size(); i++) {
    if (tracker.at(i).keepUsing) return i;
  }
  // Could not find an available position
  return -1;
}

static inline uint getNumAtomsLeft(const usageTracker& tracker, uint numAtoms)
{
  uint numAtomsUsed = getNumAtomsUsed(tracker);
  uint numAtomsLeft = numAtoms - numAtomsUsed;
  return numAtomsLeft;
}

static inline bool positionIsUsable(const WyckPosTrackingInfo& info,
                                    uint numAtomsLeft, bool findOnlyNonUnique)
{
  size_t multiplicity = info.multiplicity;

  // If we are only looking for non unique positions and the position is unique,
  // return false
  if (findOnlyNonUnique && info.unique) return false;

  // If we have a smaller multiplicity than the number of atoms left
  if (multiplicity <= numAtomsLeft && info.keepUsing &&
      // And if it it is not unique or we are not already using it more than
      // we can
      (!info.unique || info.numTimesUsed < info.getNumPositions()))
    // Use it!
    return true;
  return false;
}

static inline singleAtomPossibility
convertToPossibility(const usageTracker& tempTracker)
{
  singleAtomPossibility poss;
  for (size_t i = 0; i < tempTracker.size(); i++) {
    for (size_t j = 0; j < tempTracker.at(i).numTimesUsed; j++) {
      // The positions in these objects must all have the same multiplicity
      // and NOT be unique. So we will just pick one from here at random for now
      // TODO: come up with a better way to randomly select these in the future.
      // Preferably, in a different function...
      poss.push_back(tempTracker.at(i).getRandomWyckPos());
    }
  }
  return poss;
}

atomAssignments
SpgInitCombinatorics::convertSysPossToAtomAssignments(
                                               const systemPossibility& poss)
{
  atomAssignments assignments;
  for (size_t i = 0; i < poss.size(); i++) {
    uint atomicNum = poss.at(i).first;
    const singleAtomPossibility& saPoss = poss.at(i).second;
    for (size_t j = 0; j < saPoss.size(); j++) {
      // For atom assignments, the wyckoff position is first and atomic number
      // is second
      assignments.push_back(make_pair(saPoss.at(j), atomicNum));
    }
  }
#ifdef PRINT_SPG_INIT_COMB_DEBUG
  printAtomAssignments(assignments);
#endif
  return assignments;
}

systemPossibility
SpgInitCombinatorics::convertAtomAssignmentsToSysPoss(
                                              const atomAssignments& assigns)
{
  vector<uint> atomicNumsCounted;
  systemPossibility sysPoss;
  for (size_t i = 0; i < assigns.size(); i++) {
    uint atomicNum = assigns.at(i).second;

    // Check to make sure this one hasn't already been counted
    bool counted = false;
    for (size_t j = 0; j < atomicNumsCounted.size(); j++) {
      if (atomicNum == atomicNumsCounted.at(j)) counted = true;
    }
    if (counted) continue;

    // Count all of them of this atomic number
    singleAtomPossibility saPoss;
    for (size_t j = 0; j < assigns.size(); j++) {
      if (assigns.at(j).second == atomicNum) {
        saPoss.push_back(assigns.at(j).first);
      }
    }
    sysPoss.push_back(make_pair(atomicNum, saPoss));
    atomicNumsCounted.push_back(atomicNum);
  }
  return sysPoss;
}

// For the purposes of 'groupSimilarWyckPositions()', we need the grouped
// Wyckoff positions to not be unique and to have the same multiplicity
static inline bool wyckPositionsAreSimilarAndNotUnique(const wyckPos& wyckPos1,
                                                       const wyckPos& wyckPos2)
{
  if (!SpgInit::containsUniquePosition(wyckPos1) &&
      !SpgInit::containsUniquePosition(wyckPos2) &&
      SpgInit::getMultiplicity(wyckPos1) ==
      SpgInit::getMultiplicity(wyckPos2)) return true;
  return false;
}

static vector<vector<wyckPos>> groupSimilarWyckPositions(
                                               const wyckoffPositions& wyckVec)
{
  vector<char> wyckPositionsUsed;
  vector<vector<wyckPos>> ret;
  for (size_t i = 0; i < wyckVec.size(); i++) {
    const wyckPos& pos_i = wyckVec.at(i);
    char wyckLet = SpgInit::getWyckLet(pos_i);
    // If we've already used this one, don't include it
    if (!vecContains<char>(wyckPositionsUsed, wyckLet)) {
      wyckPositionsUsed.push_back(wyckLet);
      vector<wyckPos> tempVec;
      tempVec.push_back(pos_i);
      // Add on similar positions
      for (size_t j = i + 1; j < wyckVec.size(); j++) {
        const wyckPos& pos_j = wyckVec.at(j);
        char wyckLet_j = SpgInit::getWyckLet(pos_j);
        // wyckPositionsUsed should not contain the character, but check
        // just in case
        if (wyckPositionsAreSimilarAndNotUnique(pos_i, pos_j) &&
            !vecContains<char>(wyckPositionsUsed, wyckLet_j)) {
          wyckPositionsUsed.push_back(wyckLet_j);
          tempVec.push_back(pos_j);
        }
      }
      ret.push_back(tempVec);
    }
  }
  return ret;
}

static usageTracker createUsageTracker(const wyckoffPositions& wyckVec)
{
  usageTracker tracker;
  tracker.reserve(wyckVec.size());
  vector<vector<wyckPos>> similarWyckPos = groupSimilarWyckPositions(wyckVec);
  for (size_t i = 0; i < similarWyckPos.size(); i++) {
    WyckPosTrackingInfo info(similarWyckPos.at(i));
    tracker.push_back(info);
  }
  return tracker;
}

static inline usageTracker createUsageTracker(uint spg)
{
  return createUsageTracker(SpgInit::getWyckoffPositions(spg));
}

// Check to see if a unique position was used twice in a single possibility
static inline bool sameUniquePositionUsedTwice(const singleAtomPossibility& poss)
{
  for (size_t i = 0; i < poss.size(); i++) {
    if (SpgInit::containsUniquePosition(poss.at(i))) {
      for (size_t j = i + 1; j < poss.size(); j++) {
        if (poss.at(i) == poss.at(j)) return true;
      }
    }
  }
}

// This should only be called to compare Wyckoff positions generated
// from the same spacegroup
// Be sure to not use the same poss1 and poss2...
static inline bool sameUniquePositionUsedInBoth(
                                            const singleAtomPossibility& poss1,
                                            const singleAtomPossibility& poss2)
{
  for (size_t i = 0; i < poss1.size(); i++) {
    // Only check the position if it actually contains a unique position
    if (SpgInit::containsUniquePosition(poss1.at(i))) {
      // If there is an identical position in the other one, then we used
      // the same unique position in both
      for (size_t j = 0; j < poss2.size(); j++) {
        if (poss1.at(i) == poss2.at(j)) return true;
      }
    }
  }
  return false;
}

// Check to see if a unique position was used twice in a system possibility
static inline bool uniquePositionUsedTwice(const systemPossibility& sysPoss)
{
  // Check to see if we used a unique position twice
  // Outer vector is a vector of pairs (first is uint for atomic num,
  // and second is a vector of the wyckoff positions)
  for (size_t i = 0; i < sysPoss.size(); i++) {

    singleAtomPossibility saPoss = sysPoss.at(i).second;

    // Check to see if a unique position is used twice in the same
    // single atom possibility
    if (sameUniquePositionUsedTwice(saPoss)) return true;

    // Check to see if a unique position is used twice in the different
    // atom possibilities
    for (size_t j = i + 1; j < sysPoss.size(); j++) {
      if (sameUniquePositionUsedInBoth(saPoss, sysPoss.at(j).second))
        return true;
    }
  }

  return false;
}

systemPossibilities joinSingleWithSystem(uint atomicNum,
                                         const singleAtomPossibilities& saPoss,
                                         const systemPossibilities& sysPoss)
{
  START_FT;
  systemPossibilities newSysPossibilities;

  // If sysPoss is empty, then our job is easy
  if (sysPoss.size() == 0) {
    for (size_t i = 0; i < saPoss.size(); i++) {
      systemPossibility tempSysPossibility;
      tempSysPossibility.push_back(make_pair(atomicNum, saPoss.at(i)));
      newSysPossibilities.push_back(tempSysPossibility);
    }
    return newSysPossibilities;
  }

  // We're going to add a single atom possibilities to all of the system
  // possibilities
  for (size_t i = 0; i < sysPoss.size(); i++) {
    for (size_t j = 0; j < saPoss.size(); j++) {
      systemPossibility tempSysPoss = sysPoss.at(i);
      // Add this to the system possibility
      tempSysPoss.push_back(make_pair(atomicNum, saPoss.at(j)));
      // Only add it if a unique position is NOT used twice
      if (!uniquePositionUsedTwice(tempSysPoss))
        newSysPossibilities.push_back(tempSysPoss);
    }
  }
  return newSysPossibilities;
}

// This will only throw if "findOnlyOne" is set to be true
// If that is the case, it may throw a single atom possibility (the possibility
// that it finds successfully)
// onlyNonUnique should typically be set to 'true' if findOnlyOne is true unless
// we are looking for the last atom combination
// This will ensure that a combination will be found
static void findAllCombinations(singleAtomPossibilities& appendVec,
                                usageTracker tracker,
                                const combinationSettings& sets)
{
  START_FT;
  if (sets.numAtoms == 0) return;

  uint numAtomsLeft = getNumAtomsLeft(tracker, sets.numAtoms);
  // Returns -1 if no index is available
  int firstAvailableIndex = getFirstAvailableIndex(tracker);

  if (firstAvailableIndex == -1) return;

  WyckPosTrackingInfo info = tracker.at(firstAvailableIndex);
  size_t firstMultiplicity = info.multiplicity;

  // Check to see if we can use the first available position ('again', if
  // it has already been used). Find all possible combinations while using it
  // if we can
  if (positionIsUsable(info, numAtomsLeft, sets.findOnlyNonUnique)) {
    usageTracker tempTracker = tracker;
    tempTracker[firstAvailableIndex].numTimesUsed += 1;

    // If we have used all the atoms, append this possibility to the vector
    if (getNumAtomsLeft(tempTracker, sets.numAtoms) == 0) {
      // If we are to only find one, we are done. Easiest way to get out
      // of here is to throw an exception and catch it on the outside.
      if (sets.findOnlyOne) throw convertToPossibility(tempTracker);
      appendVec.push_back(convertToPossibility(tempTracker));
    }

    // Otherwise, keep on checking for more possibilities!
    else findAllCombinations(appendVec, tempTracker, sets);
  }

  // Find all possible combinations without using this position ('again', if
  // it has already been used).
  tracker[firstAvailableIndex].keepUsing = false;
  findAllCombinations(appendVec, tracker, sets);
}

static void findOnlyOneCombinationIfPossible(singleAtomPossibilities& appendVec,
                                             usageTracker tracker,
                                             const combinationSettings& sets,
                                             bool finalAtom)
{
  START_FT;
  // If we use 'findOnlyOne', then findAllCombinations() will throw the
  // possibility when it is found
  try {
    // We will only look at unique possibilities if we are on the final
    // type of atom. This prevents us from getting accidentally 'stuck'
    // by filling up unique positions with something that doesn't need
    // them
    combinationSettings tempSets = sets;
    tempSets.findOnlyOne = true;
    tempSets.findOnlyNonUnique = true;
    // If the parameter of this function says to only look at non unique
    // items, then that overrides this
    if (sets.findOnlyNonUnique); // Do nothing
    // If we are looking at the final atom, we may use a non unique setup
    else if (finalAtom) tempSets.findOnlyNonUnique = false;
    findAllCombinations(appendVec, tracker, tempSets);
    // If we can't find any, then proceed with the normal algorithm
    if (appendVec.size() == 0) {
      tempSets.findOnlyOne = false;
      tempSets.findOnlyNonUnique = sets.findOnlyNonUnique;
      findAllCombinations(appendVec, tracker, tempSets);
    }
  }
  // If we caught a possibility, then that's the one we're gonna use!
  catch (singleAtomPossibility pos) {
    appendVec.clear();
    appendVec.push_back(pos);
  }
}

systemPossibilities
SpgInitCombinatorics::getSystemPossibilities(uint spg,
                                             const vector<uint>& atoms,
                                             bool findOnlyOne,
                                             bool findOnlyNonUnique)
{
  START_FT;
  vector<numAndType> numOfEachType = SpgInit::getNumOfEachType(atoms);

  systemPossibilities sysPossibilities;
  sysPossibilities.reserve(numOfEachType.size());

  for (size_t i = 0; i < numOfEachType.size(); i++) {

    uint atomicNum = numOfEachType.at(i).second;

    // Create inputs for 'findAllCombinations()'
    singleAtomPossibilities saPossibilities;
    usageTracker tracker = createUsageTracker(spg);
    uint numAtoms = numOfEachType.at(i).first;

    combinationSettings sets(numAtoms, findOnlyOne, findOnlyNonUnique);

    bool last = (i == numOfEachType.size() - 1);
    // This appends all possibilities found to 'saPossibilities'
    if (findOnlyOne)
      findOnlyOneCombinationIfPossible(saPossibilities, tracker, sets, last);
    else findAllCombinations(saPossibilities, tracker, sets);

    // If we didn't find any single atom possibilities, we won't find any
    // system possibilities either. Return empty
    if (saPossibilities.size() == 0) return systemPossibilities();

#ifdef PRINT_SPG_INIT_COMB_DEBUG
    cout << "For atomic num '" << atomicNum << "' calling "
         << "printSingleAtomPossibilities()\n";
    printSingleAtomPossibilities(saPossibilities);
#endif

    sysPossibilities = joinSingleWithSystem(atomicNum, saPossibilities,
                                            sysPossibilities);
  }

#ifdef PRINT_SPG_INIT_COMB_DEBUG
  printSystemPossibilities(sysPossibilities);
#endif

  return sysPossibilities;
}

static uint countNumberOfDifferentWyckLets(const systemPossibility& poss)
{
  vector<char> wyckLetterUsed;
  uint sum = 0;
  for (size_t i = 0; i < poss.size(); i++) {
    const singleAtomPossibility& saPoss = poss.at(i).second;
    for (size_t j = 0; j < saPoss.size(); j++) {
      char let = SpgInit::getWyckLet(saPoss.at(j));
      if (!vecContains<char>(wyckLetterUsed, let)) {
        sum++;
        wyckLetterUsed.push_back(let);
      }
    }
  }
  return sum;
}

systemPossibilities
SpgInitCombinatorics::getSystemPossibilitiesWithMostWyckLets(
                                             uint spg,
                                             const vector<uint>& atoms)
{
  START_FT;
  systemPossibilities sysPosses = getSystemPossibilities(spg, atoms);

  uint maxWyckLetsUsed = 0;
  // Now find the maximum number of different wyckoff letters used in one
  for (size_t i = 0; i < sysPosses.size(); i++) {
    uint numWyckLets = countNumberOfDifferentWyckLets(sysPosses.at(i));
    if (numWyckLets > maxWyckLetsUsed) {
      maxWyckLetsUsed = numWyckLets;
    }
  }

  // Now create a list of system possibilities that have the maximum number
  systemPossibilities newSysPossibilities;
  for (size_t i = 0; i < sysPosses.size(); i++) {
    uint numWyckLets = countNumberOfDifferentWyckLets(sysPosses.at(i));
    if (numWyckLets == maxWyckLetsUsed)
      newSysPossibilities.push_back(sysPosses.at(i));
  }

  return newSysPossibilities;
}

// Get a random system possibility from all possible ones
systemPossibility SpgInitCombinatorics::getRandomSystemPossibility(
                                             uint spg,
                                             const vector<uint>& atoms)
{
  START_FT;
  systemPossibilities temp = getSystemPossibilities(spg, atoms);
  // Make sure it isn't empty...
  // Return an empty sytemPossibility if it is
  if (temp.size() == 0) return systemPossibility();
  return temp.at(rand() % temp.size());
}

// Get a random system possibility from the ones with most wyckoff letters
systemPossibility
SpgInitCombinatorics::getRandomSystemPossibilityWithMostWyckLets(
                                             uint spg,
                                             const vector<uint>& atoms)
{
  START_FT;
  systemPossibilities temp = getSystemPossibilitiesWithMostWyckLets(spg, atoms);
  // Make sure it isn't empty...
  // Return an empty systemPossibility if it is
  if (temp.size() == 0) return systemPossibility();
  return temp.at(rand() % temp.size());
}

// Get random atom assignments from all the possible system probabilities
atomAssignments SpgInitCombinatorics::getRandomAtomAssignments(uint spg,
                                                const vector<uint>& atoms)
{
  START_FT;
  return convertSysPossToAtomAssignments(getRandomSystemPossibility(spg, atoms));
}

// Get random atom assignments from the possibilities with most wyckoff
// letters
atomAssignments SpgInitCombinatorics::getRandomAtomAssignmentsWithMostWyckLets(
                                                uint spg,
                                                const vector<uint>& atoms)
{
  START_FT;
  return convertSysPossToAtomAssignments(
                        getRandomSystemPossibilityWithMostWyckLets(spg, atoms));
}

atomAssignments SpgInitCombinatorics::getFirstPossibleAtomAssignment(
                                                    uint spg,
                                                    const vector<uint>& atoms)
{
  START_FT;
  // The 'true' is telling getSystemPossibilities() to only find one
  // There are some circumstances where more than one could be found.
  // So select one at random if they are.
  systemPossibilities poss = getSystemPossibilities(spg, atoms, true);
  uint ind = rand() % poss.size();

#ifdef PRINT_SPG_INIT_COMB_DEBUG
  printSystemPossibility(poss.at(ind));
#endif

  return convertSysPossToAtomAssignments(poss.at(ind));
}

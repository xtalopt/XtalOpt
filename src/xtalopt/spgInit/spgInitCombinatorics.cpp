/**********************************************************************
  SpgInitCombinatorics.cpp - Functions for solving the complicated combinatorics
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

using namespace std;

// Define this for debug output
//#define PRINT_SPG_INIT_COMB_DEBUG

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

static inline uint getNumAtomsUsed(const usageTracker& tracker)
{
  size_t sum = 0;
  for (size_t i = 0; i < tracker.size(); i++)
    sum += (SpgInit::getMultiplicity(tracker.at(i).pos) *
                                     tracker.at(i).numTimesUsed);
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

static inline bool positionIsUsable(const wyckPosTrackingInfo& info,
                                    uint numAtomsLeft)
{
  size_t multiplicity = SpgInit::getMultiplicity(info.pos);

  // If we have a smaller multiplicity than the number of atoms left
  if (multiplicity <= numAtomsLeft && info.keepUsing &&
      // And if it it is not unique or we are not already using it
      (!info.unique || info.numTimesUsed == 0))
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
      poss.push_back(tempTracker.at(i).pos);
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

static usageTracker createUsageTracker(const wyckoffPositions& wyckVec)
{
  usageTracker tracker;
  tracker.reserve(wyckVec.size());
  wyckPosTrackingInfo info;
  info.keepUsing = true;
  info.numTimesUsed = 0;
  for (size_t i = 0; i < wyckVec.size(); i++) {
    info.pos = wyckVec.at(i);
    info.unique = SpgInit::containsUniquePosition(info.pos);
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

static void findAllCombinations(singleAtomPossibilities& appendVec,
                                usageTracker tracker, uint numAtoms)
{
  if (numAtoms == 0) return;

  uint numAtomsLeft = getNumAtomsLeft(tracker, numAtoms);
  // Returns -1 if no index is available
  int firstAvailableIndex = getFirstAvailableIndex(tracker);

  if (firstAvailableIndex == -1) return;

  wyckPosTrackingInfo info = tracker.at(firstAvailableIndex);
  size_t firstMultiplicity = SpgInit::getMultiplicity(info.pos);

  // Check to see if we can use the first available position ('again', if
  // it has already been used). Find all possible combinations while using it
  // if we can
  if (positionIsUsable(info, numAtomsLeft)) {
    usageTracker tempTracker = tracker;
    tempTracker[firstAvailableIndex].numTimesUsed += 1;

    // If we have used all the atoms, append this possibility to the vector
    if (getNumAtomsLeft(tempTracker, numAtoms) == 0)
      appendVec.push_back(convertToPossibility(tempTracker));

    // Otherwise, keep on checking for more possibilities!
    else findAllCombinations(appendVec, tempTracker, numAtoms);
  }

  // Find all possible combinations without using this position ('again', if
  // it has already been used).
  tracker[firstAvailableIndex].keepUsing = false;
  findAllCombinations(appendVec, tracker, numAtoms);
}

systemPossibilities
SpgInitCombinatorics::getAllSystemPossibilities(uint spg,
                                                const vector<uint>& atoms)
{
  vector<numAndType> numOfEachType = SpgInit::getNumOfEachType(atoms);

  systemPossibilities sysPossibilities;
  sysPossibilities.reserve(numOfEachType.size());

  for (size_t i = 0; i < numOfEachType.size(); i++) {

    uint atomicNum = numOfEachType.at(i).second;

    // Create inputs for 'findAllCombinations()'
    singleAtomPossibilities saPossibilities;
    usageTracker tracker = createUsageTracker(spg);
    uint numAtoms = numOfEachType.at(i).first;

    // This appends all possibilities found to 'saPossibilities'
    findAllCombinations(saPossibilities, tracker, numAtoms);

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

template<typename T>
static bool vecContains(const vector<T>& vec, const T& element)
{
  for (size_t i = 0; i < vec.size(); i++) {
    if (vec.at(i) == element) return true;
  }
  return false;
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
  systemPossibilities sysPosses = getAllSystemPossibilities(spg, atoms);

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
  systemPossibilities temp = getAllSystemPossibilities(spg, atoms);
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
  return convertSysPossToAtomAssignments(getRandomSystemPossibility(spg, atoms));
}

// Get random atom assignments from the possibilities with most wyckoff
// letters
atomAssignments SpgInitCombinatorics::getRandomAtomAssignmentsWithMostWyckLets(
                                                uint spg,
                                                const vector<uint>& atoms)
{
  return convertSysPossToAtomAssignments(
                        getRandomSystemPossibilityWithMostWyckLets(spg, atoms));
}

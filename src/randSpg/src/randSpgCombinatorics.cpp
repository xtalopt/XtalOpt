/**********************************************************************
  randSpgCombinatorics.cpp - Functions for solving the complicated combinatorics
                            problems for spacegroup generation

  Copyright (C) 2015 - 2016 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#include <iostream>
#include <sstream>

#include "rng.h"
#include "randSpg.h"
#include "randSpgCombinatorics.h"
#include "wyckPosTrackingInfo.h"

// For FunctionTracker
#include "functionTracker.h"

// Uncomment the right side of this line to output function starts and endings
#define START_FT //FunctionTracker functionTracker(__FUNCTION__);

using namespace std;

// Define this for debug output
//#define PRINT_RAND_SPG_COMB_DEBUG

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

#ifdef PRINT_RAND_SPG_COMB_DEBUG
static inline void printSingleAtomPossibility(const singleAtomPossibility&
                                              possib)
{
  cout << "   singleAtomPossibility is:\n";
  for (size_t i = 0; i < possib.size(); i++)
    cout << "    " << RandSpg::getWyckLet(possib[i]) << "\n";
}

static inline void printSingleAtomPossibilities(const singleAtomPossibilities&
                                                possibs)
{
  cout << "Printing single atom possibilities!\n";
  for (size_t i = 0; i < possibs.size(); i++) {
    cout << " at i == " << i << ":\n";
    printSingleAtomPossibility(possibs[i]);
  }
  cout << "\n\n";
}

static inline void printSystemPossibility(const systemPossibility& possib)
{
  cout << " Printing system possibility\n";
  for (size_t i = 0; i < possib.size(); i++) {
    cout << "  at i == " << i << ":\n";
    cout << "   atomic num is " << possib[i].first << "\n";
    printSingleAtomPossibility(possib[i].second);
  }
}

static inline void printSystemPossibilities(const systemPossibilities& possib)
{
  cout << "Printing system possibilities\n";
  for (size_t i = 0; i < possib.size(); i++) {
    printSystemPossibility(possib[i]);
  }
  cout << "\n\n";
}

static inline void printAtomAssignments(const atomAssignments& assigns)
{
  cout << "Printing atom assignments!\n";
  for (size_t i = 0; i < assigns.size(); i++) {
    cout << "  atomicNum is '" << assigns[i].second << "' and wyckLet is '"
         << RandSpg::getWyckLet(assigns[i].first) << "'.\n";
  }
  cout << "\n\n";
}
#endif

template<typename T>
static bool vecContains(const vector<T>& vec, const T& element)
{
  for (size_t i = 0; i < vec.size(); i++) {
    if (vec[i] == element) return true;
  }
  return false;
}

static inline uint getNumAtomsUsed(const usageTracker& tracker)
{
  size_t sum = 0;
  for (size_t i = 0; i < tracker.size(); i++)
    sum += ((tracker[i].multiplicity) * tracker[i].numTimesUsed);
  return sum;
}

static inline int getFirstAvailableIndex(const usageTracker& tracker)
{
  for (size_t i = 0; i < tracker.size(); i++) {
    if (tracker[i].keepUsing) return i;
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
convertToPossibility(const usageTracker& tempTracker, uint atomicNum)
{
  singleAtomPossibility poss;
  poss.atomicNum = atomicNum;
  for (size_t i = 0; i < tempTracker.size(); i++) {
    const WyckPosTrackingInfo& info = tempTracker[i];
    if (info.numTimesUsed == 0) continue;

    similarWyckPosAndNumToChoose temp;
    temp.numToChoose = info.numTimesUsed;
    temp.choosablePositions = info.getPositions();
    poss.assigns.push_back(temp);
  }
  return poss;
}

// For the purposes of 'groupSimilarWyckPositions()', we need the grouped
// Wyckoff positions to have the same multiplicity and uniqueness
static inline bool wyckPositionsAreSimilar(const wyckPos& wyckPos1,
                                           const wyckPos& wyckPos2)
{
  if (RandSpg::containsUniquePosition(wyckPos1) ==
      RandSpg::containsUniquePosition(wyckPos2) &&
      RandSpg::getMultiplicity(wyckPos1) ==
      RandSpg::getMultiplicity(wyckPos2)) return true;
  return false;
}

// Takes a Wyckoff vector and groups similar Wyckoff positions together
// Stores the similar Wyckoff positions as a vector
static vector<vector<wyckPos>> groupSimilarWyckPositions(
                                               const wyckoffPositions& wyckVec)
{
  vector<char> wyckPositionsUsed;
  vector<vector<wyckPos>> ret;
  for (size_t i = 0; i < wyckVec.size(); i++) {
    const wyckPos& pos_i = wyckVec[i];
    char wyckLet = RandSpg::getWyckLet(pos_i);
    // If we've already used this one, don't include it
    if (!vecContains<char>(wyckPositionsUsed, wyckLet)) {
      wyckPositionsUsed.push_back(wyckLet);
      vector<wyckPos> tempVec;
      tempVec.push_back(pos_i);
      // Add on similar positions
      for (size_t j = i + 1; j < wyckVec.size(); j++) {
        const wyckPos& pos_j = wyckVec[j];
        char wyckLet_j = RandSpg::getWyckLet(pos_j);
        // wyckPositionsUsed should not contain the character, but check
        // just in case
        if (wyckPositionsAreSimilar(pos_i, pos_j) &&
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

// Create a basic usage tracker from the wyckoff positions
static usageTracker createUsageTracker(const wyckoffPositions& wyckVec)
{
  usageTracker tracker;
  tracker.reserve(wyckVec.size());
  vector<vector<wyckPos>> similarWyckPos = groupSimilarWyckPositions(wyckVec);
  for (size_t i = 0; i < similarWyckPos.size(); i++) {
    WyckPosTrackingInfo info(similarWyckPos[i]);
    tracker.push_back(info);
  }
  return tracker;
}

static inline usageTracker createUsageTracker(uint spg)
{
  return createUsageTracker(RandSpg::getWyckoffPositions(spg));
}

// Checks to see if the Wyckoff positions in a similar wyckoff vector
// are unique or not
static inline bool similarWyckPosAndNumToChooseIsUnique(const similarWyckPosAndNumToChoose& s)
{
  if (s.choosablePositions.size() == 0) return false;
  return RandSpg::containsUniquePosition(s.choosablePositions[0]);
}

// Check to make sure we don't use more unique positions than are available
// in a single atom possibility
static inline bool moreUniquePositionsUsedThanAvailable(const singleAtomPossibility& poss)
{
  const assignments& assigns = poss.assigns;
  for (size_t i = 0; i < assigns.size(); i++) {
    // Check to see if our similar Wyck positions are unique
    if (similarWyckPosAndNumToChooseIsUnique(assigns[i])) {
      // If they are, find the number to choose and the number available
      uint numToChoose = assigns[i].numToChoose;
      uint numAvailable = assigns[i].choosablePositions.size();
      // If we have to choose more than we have available, return true
      if (numToChoose > numAvailable) return true;
    }
  }
  return false;
}

static inline uint numTimesAPositionIsUsed(const singleAtomPossibility& sPoss,
                                           const similarWyckPositions& simPos)
{
  uint numTimesUsed = 0;
  for (size_t i = 0; i < sPoss.assigns.size(); i++) {
    if (sPoss.assigns[i].choosablePositions == simPos) {
      numTimesUsed += sPoss.assigns[i].numToChoose;
    }
  }
  return numTimesUsed;
}

// This should only be called to compare Wyckoff positions generated
// from the same spacegroup
// This assumes
static inline bool tooManyOfAUniquePositionUsed(const systemPossibility& s)
{
  for (size_t i = 0; i < s.size(); i++) {
    // Check to make sure the single atom possibility doesn't use
    // too many unique positions
    if (moreUniquePositionsUsedThanAvailable(s[i])) return true;
    const assignments& assigns = s[i].assigns;
    for (size_t j = 0; j < assigns.size(); j++) {
      const similarWyckPosAndNumToChoose& simPos = assigns[j];
      if (similarWyckPosAndNumToChooseIsUnique(simPos)) {
        uint numTimesUsed = simPos.numToChoose;
        // We need to loop through the other system possibilities and count the
        // number of times each one of them uses this position
        for (size_t k = i + 1; k < s.size(); k++) {
          numTimesUsed += numTimesAPositionIsUsed(s[k], simPos.choosablePositions);
        }
        // If, in total, we used too many of a unique position, return true
        if (numTimesUsed > simPos.choosablePositions.size()) return true;
      }
    }
  }
  return false;
}

systemPossibilities joinSingleWithSystem(const singleAtomPossibilities& saPoss,
                                         const systemPossibilities& sysPoss)
{
  START_FT;
  systemPossibilities newSysPossibilities;

  // If sysPoss is empty, then our job is easy
  if (sysPoss.size() == 0) {
    for (size_t i = 0; i < saPoss.size(); i++) {
      systemPossibility tempSysPossibility;
      tempSysPossibility.push_back(saPoss[i]);
      // We're assuming the single atom possibility has already been
      // checked internally for uniqueness violations
      newSysPossibilities.push_back(tempSysPossibility);
    }
    return newSysPossibilities;
  }

  // We're going to add a single atom possibilities to all of the system
  // possibilities
  for (size_t i = 0; i < sysPoss.size(); i++) {
    for (size_t j = 0; j < saPoss.size(); j++) {
      systemPossibility tempSysPoss = sysPoss[i];
      // Add this to the system possibility
      tempSysPoss.push_back(saPoss[j]);
      // Only add it if too many of a unique position is NOT used
      // If it violates the uniqueness rule, we can't use it
      if (!tooManyOfAUniquePositionUsed(tempSysPoss))
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
                                uint atomicNum,
                                const combinationSettings& sets)
{
  START_FT;
  if (sets.numAtoms == 0) return;

  uint numAtomsLeft = getNumAtomsLeft(tracker, sets.numAtoms);
  // Returns -1 if no index is available
  int firstAvailableIndex = getFirstAvailableIndex(tracker);

  if (firstAvailableIndex == -1) return;

  WyckPosTrackingInfo info = tracker[firstAvailableIndex];
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
      if (sets.findOnlyOne) throw convertToPossibility(tempTracker, atomicNum);
      appendVec.push_back(convertToPossibility(tempTracker, atomicNum));
    }

    // Otherwise, keep on checking for more possibilities!
    else findAllCombinations(appendVec, tempTracker, atomicNum, sets);
  }

  // Find all possible combinations without using this position ('again', if
  // it has already been used).
  tracker[firstAvailableIndex].keepUsing = false;
  findAllCombinations(appendVec, tracker, atomicNum, sets);
}

static void findOnlyOneCombinationIfPossible(singleAtomPossibilities& appendVec,
                                             usageTracker tracker,
                                             uint atomicNum,
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
    findAllCombinations(appendVec, tracker, atomicNum, tempSets);
    // If we can't find any, then proceed with the normal algorithm
    if (appendVec.size() == 0) {
      tempSets.findOnlyOne = false;
      tempSets.findOnlyNonUnique = sets.findOnlyNonUnique;
      findAllCombinations(appendVec, tracker, atomicNum, tempSets);
    }
  }
  // If we caught a possibility, then that's the one we're gonna use!
  catch (singleAtomPossibility pos) {
    appendVec.clear();
    appendVec.push_back(pos);
  }
}

// Returns all system possibilities that can be found
systemPossibilities
RandSpgCombinatorics::getSystemPossibilities(uint spg,
                                             const vector<uint>& atoms,
                                             bool findOnlyOne,
                                             bool findOnlyNonUnique)
{
  START_FT;
  vector<numAndType> numOfEachType = RandSpg::getNumOfEachType(atoms);

  systemPossibilities sysPossibilities;
  sysPossibilities.reserve(numOfEachType.size());

  for (size_t i = 0; i < numOfEachType.size(); i++) {

    uint atomicNum = numOfEachType[i].second;

    // Create inputs for 'findAllCombinations()'
    singleAtomPossibilities saPossibilities;
    usageTracker tracker = createUsageTracker(spg);
    uint numAtoms = numOfEachType[i].first;

    combinationSettings sets(numAtoms, findOnlyOne, findOnlyNonUnique);

    bool last = (i == numOfEachType.size() - 1);
    // This appends all possibilities found to 'saPossibilities'
    if (findOnlyOne)
      findOnlyOneCombinationIfPossible(saPossibilities, tracker, atomicNum, sets, last);
    else findAllCombinations(saPossibilities, tracker, atomicNum, sets);

    // If we didn't find any single atom possibilities, we won't find any
    // system possibilities either. Return empty
    if (saPossibilities.size() == 0) return systemPossibilities();

#ifdef PRINT_RAND_SPG_COMB_DEBUG
    cout << "For atomic num '" << atomicNum << "' calling "
         << "printSingleAtomPossibilities()\n";
    printSingleAtomPossibilities(saPossibilities);
#endif

    sysPossibilities = joinSingleWithSystem(saPossibilities, sysPossibilities);
  }

#ifdef PRINT_RAND_SPG_COMB_DEBUG
  printSystemPossibilities(sysPossibilities);
#endif

  return sysPossibilities;
}

uint countNumTimesWyckPosMayBeUsed(const systemPossibility& sysPos,
                                   char wyckLet)
{
  uint numTimesUsed = 0;
  for (size_t i = 0; i < sysPos.size(); i++) {
    const singleAtomPossibility& sinPos = sysPos[i];
    const assignments& assigns = sinPos.assigns;
    for (size_t j = 0; j < assigns.size(); j++) {
      uint numToChoose = assigns[j].numToChoose;
      const similarWyckPositions& cp = assigns[j].choosablePositions;
      for (size_t k = 0; k < cp.size(); k++) {
        const wyckPos& wp = cp[k];
        if (RandSpg::getWyckLet(wp) == wyckLet) {
          // If this is a unique wyckoff position, we may only use it once
          if (RandSpg::containsUniquePosition(wp)) return 1;
          // Otherwise, we can use it up to the maximum number of times
          else numTimesUsed += numToChoose;
        }
      }
    }
  }
  return numTimesUsed;
}

uint countNumTimesWyckPosMayBeUsedForSpecificAtom(
                                   const systemPossibility& sysPos,
                                   char wyckLet,
                                   uint atomicNum)
{
  uint numTimesUsed = 0;
  for (size_t i = 0; i < sysPos.size(); i++) {
    const singleAtomPossibility& sinPos = sysPos[i];
    if (sinPos.atomicNum != atomicNum) continue;
    const assignments& assigns = sinPos.assigns;
    for (size_t j = 0; j < assigns.size(); j++) {
      uint numToChoose = assigns[j].numToChoose;
      const similarWyckPositions& cp = assigns[j].choosablePositions;
      for (size_t k = 0; k < cp.size(); k++) {
        const wyckPos& wp = cp[k];
        if (RandSpg::getWyckLet(wp) == wyckLet) {
          // If this is a unique wyckoff position, we may only use it once
          if (RandSpg::containsUniquePosition(wp)) return 1;
          // Otherwise, we can use it up to the maximum number of times
          else numTimesUsed += numToChoose;
        }
      }
    }
  }
  return numTimesUsed;
}
systemPossibilities RandSpgCombinatorics::removePossibilitiesWithoutWyckPos(
                                          const systemPossibilities& sysPos,
                                          char wyckLet,
                                          uint minNumUses)
{
  systemPossibilities ret;
  for (size_t i = 0; i < sysPos.size(); i++) {
    uint numTimesUsed = countNumTimesWyckPosMayBeUsed(sysPos[i], wyckLet);
    if (numTimesUsed >= minNumUses) ret.push_back(sysPos[i]);
  }
  return ret;
}

// This version also specifies the atomic number
systemPossibilities RandSpgCombinatorics::removePossibilitiesWithoutWyckPos(
                                          const systemPossibilities& sysPos,
                                          char wyckLet,
                                          uint minNumUses,
                                          uint atomicNum)
{
  systemPossibilities ret;
  for (size_t i = 0; i < sysPos.size(); i++) {
    uint numTimesUsed = countNumTimesWyckPosMayBeUsedForSpecificAtom(sysPos[i], wyckLet, atomicNum);
    if (numTimesUsed >= minNumUses) ret.push_back(sysPos[i]);
  }
  return ret;
}

systemPossibilities RandSpgCombinatorics::removePossibilitiesWithoutGeneralWyckPos(const systemPossibilities& sysPos,
   uint spg,
   uint minNumUses)
{
  const wyckoffPositions& wp = RandSpg::getWyckoffPositions(spg);
  return removePossibilitiesWithoutWyckPos(sysPos, RandSpg::getWyckLet(wp[wp.size() - 1]), minNumUses);
}

// We call this when a unique position is assigned when getting
// atom assignments.
// It removes the unique position from the possibilities so that it will
// not be used
void removePositionFromSystemPossibility(systemPossibility& pos, char wyckLet)
{
  for (size_t i = 0; i < pos.size(); i++) {
    singleAtomPossibility& siPos = pos[i];
    for (size_t j = 0; j < siPos.assigns.size(); j++) {
      similarWyckPositions& simPos = siPos.assigns[j].choosablePositions;
      for (size_t k = 0; k < simPos.size(); k++) {
        if (RandSpg::getWyckLet(simPos[k]) == wyckLet) {
          simPos.erase(simPos.begin() + k);
          k--;
        }
      }
    }
  }
}

systemPossibility RandSpgCombinatorics::getRandomSystemPossibility(const systemPossibilities& sysPoss)
{
  return sysPoss[getRandInt(0, sysPoss.size() - 1)];
}

// Get random atom assignments from all the possible system possibilities
atomAssignments RandSpgCombinatorics::getRandomAtomAssignments(const systemPossibilities& sysPoss)
{
  // We will call the one with the "forcedWyckPositions" vector as an
  // empty vector
  return getRandomAtomAssignments(sysPoss, vector<pair<uint, wyckPos>>());
}

void decrementChoiceFromSystemPossibility(systemPossibility& sysPos, uint atomicNum, const wyckPos& wyckPos)
{
  // Remove this from the system possibility
  for (size_t i = 0; i < sysPos.size(); i++) {
    if (sysPos[i].atomicNum != atomicNum) continue;
    bool decrementComplete = false;
    for (size_t j = 0; j < sysPos[i].assigns.size(); j++) {
      similarWyckPosAndNumToChoose& simAndNum = sysPos[i].assigns[j];
      similarWyckPositions& simPos = simAndNum.choosablePositions;
      // If we find that the position is used here, decrement the choosable
      // positions and break. We are done.
      for (size_t k = 0; k < simPos.size(); k++) {
        if (simPos[k] == wyckPos) {
          simAndNum.numToChoose--;
          decrementComplete = true;
          break;
        }
      }
      // If we have decremented, we are done.
      if (decrementComplete) break;
    }
    if (decrementComplete) break;
  }
}

atomAssignments RandSpgCombinatorics::getRandomAtomAssignments(const systemPossibilities& sysPoss, const vector<pair<uint, wyckPos>>& forcedWyckPositions)
{
  START_FT;
  atomAssignments ret;
  // Pick a random system possibility to use
  systemPossibility tempPos = getRandomSystemPossibility(sysPoss);

  // Add the forced Wyckoff positions
  for (size_t i = 0; i < forcedWyckPositions.size(); i++) {
    wyckPos pos = forcedWyckPositions[i].second;
    uint atomicNum = forcedWyckPositions[i].first;
    ret.push_back(make_pair(pos, atomicNum));
    decrementChoiceFromSystemPossibility(tempPos, atomicNum, pos);
  }

  for (size_t i = 0; i < tempPos.size(); i++) {
    uint atomicNum = tempPos[i].atomicNum;
    for (size_t j = 0; j < tempPos[i].assigns.size(); j++) {
      similarWyckPosAndNumToChoose& simAndNum = tempPos[i].assigns[j];
      uint atomsLeft = simAndNum.numToChoose;
      similarWyckPositions& simPos = simAndNum.choosablePositions;
      // Keep adding atoms until there are none left
      while (atomsLeft > 0) {
        int rand = getRandInt(0, simPos.size() - 1);
        const wyckPos& wyckPos = simPos[rand];
        ret.push_back(make_pair(simPos[rand], atomicNum));
        atomsLeft--;
        // If we used a unique position, then remove all of this position from
        // the rest of the choices so we don't accidentally re-use it
        if (RandSpg::containsUniquePosition(wyckPos))
          removePositionFromSystemPossibility(tempPos, RandSpg::getWyckLet(wyckPos));
      }
    }
  }

  return ret;
}

string RandSpgCombinatorics::getSimilarWyckPosAndNumToChooseString(const similarWyckPosAndNumToChoose& simPos)
{
  stringstream s;
  s << "   printing similar Wyck pos and num to choose:\n";
  s << "   numToChoose is: " << simPos.numToChoose << "\n";
  if (simPos.choosablePositions.size() != 0)
    s << "   uniqueness is: "
      << (RandSpg::containsUniquePosition(simPos.choosablePositions[0]) ? "true\n" : "false\n");
  s << "   Wyckoff positions are:\n    { ";
  for (size_t i = 0; i < simPos.choosablePositions.size(); i++) {
    s << RandSpg::getWyckLet(simPos.choosablePositions[i]) << " ";
  }
  s << "}\n";
  return s.str();
}

void RandSpgCombinatorics::printSimilarWyckPosAndNumToChoose(const similarWyckPosAndNumToChoose& simPos)
{
  cout << getSimilarWyckPosAndNumToChooseString(simPos);
}

string RandSpgCombinatorics::getSingleAtomPossibilityString(const singleAtomPossibility& pos)
{
  stringstream s;
  s << "  Printing single atom possibility:\n";
  s << "  atomicNum is: " << pos.atomicNum << "\n";
  for (size_t i = 0; i < pos.assigns.size(); i++) {
    s << getSimilarWyckPosAndNumToChooseString(pos.assigns[i]);
  }
  return s.str();
}

void RandSpgCombinatorics::printSingleAtomPossibility(const singleAtomPossibility& pos)
{
  cout << getSingleAtomPossibilityString(pos);
}

string RandSpgCombinatorics::getSystemPossibilityString(const systemPossibility& pos)
{
  stringstream s;
  s << "\n Printing system possibility:\n";
  for (size_t i = 0; i < pos.size(); i++) s << getSingleAtomPossibilityString(pos[i]);
  return s.str();
}

void RandSpgCombinatorics::printSystemPossibility(const systemPossibility& pos)
{
  cout << getSystemPossibilityString(pos);
}

string RandSpgCombinatorics::getSystemPossibilitiesString(const systemPossibilities& pos)
{
  stringstream s;
  s << "Printing system possibilities:\n";
  for (size_t i = 0; i < pos.size(); i++) s << getSystemPossibilityString(pos[i]);
  return s.str();
}

string RandSpgCombinatorics::getVerbosePossibilitiesString(const systemPossibilities& pos)
{
  stringstream s;
  s << "Printing system possibilities:\n";
  for (size_t i = 0; i < pos.size(); i++) {
    const systemPossibility& sysPos = pos[i];
    s << "  Possibility " << i+1 << ":\n";
    for (size_t j = 0; j < sysPos.size(); j++) {
      const singleAtomPossibility& sinPos = sysPos[j];
      s << "    For atomicNum: " << sinPos.atomicNum << "\n";
      for (size_t k = 0; k < sinPos.assigns.size(); k++) {
        const similarWyckPosAndNumToChoose& simPos = sinPos.assigns[k];
        s << "      We will choose " << simPos.numToChoose 
          << " of the following positions:\n        { ";
        for (size_t l = 0; l < simPos.choosablePositions.size(); l++) {
          s << RandSpg::getWyckLet(simPos.choosablePositions[l]) << " ";
        }
        s << "}\n";
        if (simPos.choosablePositions.size() != 0)
          s << "        uniqueness is: "
            << (RandSpg::containsUniquePosition(simPos.choosablePositions[0]) ? "true - positions are not re-usable\n" : "false - positions are re-usable\n");
      }
    }
    s << "  End of possibility " << i+1 << "\n\n";
  }
  return s.str();
}

void RandSpgCombinatorics::printSystemPossibilities(const systemPossibilities& pos)
{
  cout << getSystemPossibilitiesString(pos);
}

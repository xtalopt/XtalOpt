
#include <iostream>

#include <xtalopt/spgInit/spgInit.h>
#include <xtalopt/spgInit/spgInitCombinatorics.h>

using namespace std;

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
}

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
    systemPossibility tempSysPoss = sysPoss.at(i);
    for (size_t j = 0; j < saPoss.size(); j++) {
      // Add this to the system possibility
      tempSysPoss.push_back(make_pair(atomicNum, saPoss.at(j)));
      // Only add it if a unique position is NOT used twice
      if (!uniquePositionUsedTwice(tempSysPoss))
        newSysPossibilities.push_back(tempSysPoss);
    }
  }
  return newSysPossibilities;
}

void findAllCombinations(singleAtomPossibilities& appendVec,
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
SpgInitCombinatorics::getSystemPossibilities(uint spg,
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

    printSingleAtomPossibilities(saPossibilities);

    sysPossibilities = joinSingleWithSystem(atomicNum, saPossibilities,
                                            sysPossibilities);
  }

  printSystemPossibilities(sysPossibilities);
  return sysPossibilities;
}

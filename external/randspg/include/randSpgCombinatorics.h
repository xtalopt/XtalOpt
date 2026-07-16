/**********************************************************************
  RandSpgCombinatorics.h - Functions for solving the complicated combinatorics
                           problems for spacegroup initialization

  Copyright (C) 2015 - 2016 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

/* You may feel free to skip over this explanation here. But this summarizes the
   purpose of these functions.

***********************************
You have a system that consists of rooms, boxes, and balls. There are 230 rooms total. There are many different kinds of boxes and balls.

Each room contains a certain number of boxes. Each box has a certain size and a property called 'uniqueness.' If a box is unique, then there is only one of them. If it is not unique, then there are an infinite number of them you can get. As for the size (unsigned integer), you MUST place exactly a number of balls inside that is equal to the size - no more, no less - and each ball MUST be the same type of ball.

There are many different types of balls. The list will be labelled as a, b, c, d, etc. In the scope of this problem, the size of the ball does not have an impact on whether it will fit in the box or not, so you may assume all balls are the same size.

A list of balls consists of their type, and how many of them there are (so we may have 4 of type a, 6 of type b, etc.). There aren't really any constraints on how many types there may be or how many balls of each type there may be.

Write a function with the following input/output (WITHOUT the use of trial and error methods):

input: list of balls (includes type of ball and number of each type) and room number (will identify how many different boxes there are, what each size is, and whether the box is unique or not).

output: a list of all possible arrangements of the balls so that every ball is placed inside a box. Not every box needs to be filled, but every ball must be inside a box. If the requirements are impossible to satisfy, return an empty list.

***********************************

These functions here solve this problem that I created.
The 'rooms' are spacegroups, the 'boxes' are Wyckoff positions, and the
'balls' are atoms. This problem needed to be solved to find all possible
combinations to put atoms in a given spacegroup.
*/

#ifndef RAND_SPG_COMBINATORICS_H
#define RAND_SPG_COMBINATORICS_H

// This is a vector of assignments
// in which to place atoms of a pre-known atomic number may be placed
// It doesn't mean "single" atom as in one atom, but one type of atom

// This is a vector of similar Wyckoff positions
// Wyckoff positions are considered similar if they share the same multiplicity
// and uniqueness
typedef std::vector<wyckPos> similarWyckPositions;

// This provides a set of choosable positions and how many we are supposed to
// choose
struct similarWyckPosAndNumToChoose {
  uint numToChoose;
  similarWyckPositions choosablePositions;
};

typedef std::vector<similarWyckPosAndNumToChoose> assignments;

// We have here an atomic number and the assignments to produce a single
// possibility for a given atomic number
struct singleAtomPossibility {
  uint atomicNum;
  assignments assigns;
};

// This represents all possible solutions of the system (for individual
// atomic numbers. Not all of them together). We assume
// everything in this vector has the same atomic number.
typedef std::vector<singleAtomPossibility> singleAtomPossibilities;

// This should be a vector of different atom combinations that give the final
// desired composition. We assume possibilities in this vector may
// have different atomic numbers
typedef std::vector<singleAtomPossibility> systemPossibility;
// This represents all possible systems from which a composition can be
// correctly reconstructed using Wyckoff positions
typedef std::vector<systemPossibility> systemPossibilities;

class RandSpgCombinatorics {
 public:
  // Returns all system possibilities that satisfy the constraints given
  // by the spacegroup and input atoms
  static systemPossibilities getSystemPossibilities(
                                             uint spg,
                                             const std::vector<uint>& atoms,
                                             bool findOnlyOne = false,
                                             bool onlyNonUnique = false);

  // This removes all possibilities for which the wyckLet is NOT in the possible
  // setup. This does not guarantee, however, that the wyckoff position will be
  // used. You must force it to be selected with a special function that
  // I'll be writing soon...
  static systemPossibilities removePossibilitiesWithoutWyckPos(
                                       const systemPossibilities& sysPos,
                                       char wyckLet,
                                       uint minNumUses = 1);

  // This one specifies the atomic number also
  static systemPossibilities removePossibilitiesWithoutWyckPos(
                                          const systemPossibilities& sysPos,
                                          char wyckLet,
                                          uint minNumUses,
                                          uint atomicNum);

  // This calls removePossibilitiesWithoutWyckPos() for the most general
  // Wyckoff position for the spacegroup
  static systemPossibilities removePossibilitiesWithoutGeneralWyckPos(
                                           const systemPossibilities& sysPos,
                                           uint spg,
                                           uint minNumUses = 1);

  // Pick a random system possibility from the system possibilities
  static systemPossibility getRandomSystemPossibility(const systemPossibilities& sysPoss);

  // Get a random set of atom assignments from all the system possibilities
  static atomAssignments getRandomAtomAssignments(const systemPossibilities& sysPoss);

  static atomAssignments getRandomAtomAssignments(
             const systemPossibilities& sysPoss,
             const std::vector<std::pair<uint, wyckPos>>& forcedWyckPositions);

  static std::string getSimilarWyckPosAndNumToChooseString(const similarWyckPosAndNumToChoose& simPos);

  static void printSimilarWyckPosAndNumToChoose(const similarWyckPosAndNumToChoose& simPos);

  static std::string getSingleAtomPossibilityString(const singleAtomPossibility& pos);

  static void printSingleAtomPossibility(const singleAtomPossibility& pos);

  static std::string getSystemPossibilityString(const systemPossibility& pos);

  static void printSystemPossibility(const systemPossibility& pos);

  static std::string getSystemPossibilitiesString(const systemPossibilities& pos);

  // To be added to the log file when the user specifies 'verbose' output
  static std::string getVerbosePossibilitiesString(const systemPossibilities& pos);

  static void printSystemPossibilities(const systemPossibilities& pos);
};

#endif

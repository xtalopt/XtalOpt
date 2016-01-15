/**********************************************************************
  SpgInitCombinatorics.h - Functions for solving the complicated combinatorics
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

#ifndef SPG_INIT_COMBINATORICS_H
#define SPG_INIT_COMBINATORICS_H

// This is a vector of assignments
// in which to place atoms of a pre-known atomic number may be placed
// It doesn't mean "single" atom as in one atom, but one type of atom
typedef std::vector<wyckPos> singleAtomPossibility;
// uint is the atomic number
typedef std::vector<singleAtomPossibility> singleAtomPossibilities;
// The uint is an atomic number
// This should be a vector of different atom combinations that give the final
// desired composition
typedef std::vector<std::pair<uint, singleAtomPossibility>> systemPossibility;
// This represents all possible systems from which a composition can be
// correctly reconstructed using Wyckoff positions
typedef std::vector<systemPossibility> systemPossibilities;

class SpgInitCombinatorics {
 public:
  // Returns all system possibilities that satisfty the constraints given
  // by the spacegroup and input atoms
  static systemPossibilities getSystemPossibilities(
                                             uint spg,
                                             const std::vector<uint>& atoms,
                                             bool findOnlyOne = false,
                                             bool onlyNonUnique = false);

  // Return only system possibilities that have the most variety in wyckoff
  // letters
  // These could potentially produce more stable structures
  static systemPossibilities getSystemPossibilitiesWithMostWyckLets(
                                             uint spg,
                                             const std::vector<uint>& atoms);

  // Get a random system possibility from all possible ones
  static systemPossibility getRandomSystemPossibility(
                                             uint spg,
                                             const std::vector<uint>& atoms);

  // Get a random system possibility from the ones with most wyckoff letters
  static systemPossibility getRandomSystemPossibilityWithMostWyckLets(
                                             uint spg,
                                             const std::vector<uint>& atoms);

  // Convert system possibility to atom assignment
  static atomAssignments convertSysPossToAtomAssignments(
                                             const systemPossibility& poss);

  // convert atom assignment to system possibility
  static systemPossibility convertAtomAssignmentsToSysPoss(
                                              const atomAssignments& assigns);

  // Get a random atom assignment from all the possibly system probabilities
  static atomAssignments getRandomAtomAssignments(uint spg,
                                                const std::vector<uint>& atoms);

  // Get a random atom assignment from the possibilities with most wyckoff
  // letters
  static atomAssignments getRandomAtomAssignmentsWithMostWyckLets(
                                                uint spg,
                                                const std::vector<uint>& atoms);

  // Returns the first atom assignment found
  // Should not be used to get random atom assignments because it will
  // always return the same one. It is used for isSpgPossible()
  static atomAssignments getFirstPossibleAtomAssignment(
                                                uint spg,
                                                const std::vector<uint>& atoms);
};

#endif

/**********************************************************************
  SpgInitCombinatorics.h - Functions for solving the complicated combinatorics
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

#ifndef SPG_INIT_COMBINATORICS_H
#define SPG_INIT_COMBINATORICS_H

// The 'wyckPos' is the wyckoff position
// The first 'bool' is whether to keep using this position or not
// The second 'bool' is whether it is unique or not
// The uint is the number of times it's been used
struct wyckPosTrackingInfo {
  wyckPos pos;
  bool keepUsing;
  bool unique;
  uint numTimesUsed;
};

typedef std::vector<wyckPosTrackingInfo> usageTracker;

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
  static systemPossibilities getAllSystemPossibilities(
                                             uint spg,
                                             const std::vector<uint>& atoms);

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
};

#endif

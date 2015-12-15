/**********************************************************************
  spgInit.h - Header file for spacegroup initialization functions

  Copyright (C) 2015 by Patrick S. Avery

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef SPG_INIT_H
#define SPG_INIT_H

#include <Eigen/LU>
#include <vector>
#include <tuple>
#include <utility>

#include <xtalopt/structures/xtal.h>

typedef struct {
  uint atomicNum;
  double x;
  double y;
  double z;
} atomStruct;

struct latticeStruct {
  double a;
  double b;
  double c;
  double alpha;
  double beta;
  double gamma;
  // Initialize all the values to be 0
  latticeStruct() : a(0), b(0), c(0), alpha(0), beta(0), gamma(0) {}
};

// wyckPos is a tuple of a char (representing the Wyckoff letter),
// an int (representing the multiplicity), and a string (that contains the first
// Wyckoff position)
typedef std::tuple<char, int, std::string> wyckPos;

// Each spacegroup has a variable number of wyckoff positions. So each
// spacegroup will have it's own vector of wyckoff positions.
typedef std::vector<wyckPos> wyckoffPositions;

// This assign an atom with a specific atomic number to be placed in a specific
// wyckoff position
// The vector is a vector of atom assignments
typedef std::pair<wyckPos, uint> atomAssignment;
typedef std::vector<atomAssignment> atomAssignments;

class SpgInit {
 public:
  /*
   * Obtain the wyckoff positions of a spacegroup from the database
   *
   * @param spg The spacegroup from which to obtain the wyckoff positions
   *
   * @return Returns a constant reference to a vector of wyckoff positions
   * for the spacegroup from the database in wyckoffDatabase.h. Returns an
   * empty vector if an invalid spg is entered.
   */
  static const wyckoffPositions& getWyckoffPositions(uint spg);

  /*
   *
   * Does nothing right now...
   *
   */
  static std::vector<atomStruct> generateInitWyckoffs(
                                             uint spg,
                                             const std::vector<uint> atomTypes);
  /*
   * Used to determine if a spacegroup is possible for a given set of atoms.
   * It is determined by using the multiplicities in the Wyckoff database.
   *
   * @param spg The spacegroup to check.
   * @param atomTypes A vector of atomic numbers (one for each atom). So if
   *                  our system were Ti1O2, it should be {22, 8, 8}
   *
   * @return True if the spacegroup may be generated. False if it cannot.
   *
   */
  static bool isSpgPossible(uint spg, const std::vector<uint>& atomTypes);

  /*
   * Generates a latticeStruct (contains a, b, c, alpha, beta, and gamma as
   * doubles) with randomly generated parameters for a given
   * spacegroup, mins, and maxes. If a parameter must be constrained due
   * to the spacegroup, it will be.
   *
   * @param spg The spacegroup for which to generate a lattice.
   * @param mins The minimum values for the lattice parameters
   * @param maxes The maximum values for the lattice parameters
   *
   * @return Returns the lattice struct that was generated. Returns a struct
   * with all zero values if a proper lattice struct cannot be generated
   * for a specific spacegroup due to invalid mins or maxes.
   * An error message will be printed to stdout with information if it fails.
   */
  static latticeStruct generateLatticeForSpg(uint spg,
                                             const latticeStruct& mins,
                                             const latticeStruct& maxes);
  /*
   * Attempts to add an atom randomly to a wyckoff position of a given xtal.
   * The position of the atom is constrained by the given wyckoff position.
   * It will attempt to add an atom randomly to satisfy minIDA for
   * maxAttempts times, and if it fails, it returns false. If a fixed wyckoff
   * position is given, it will just add the atom to the fixed wyckoff position.
   *
   * @param xtal The xtal for which an atom will be added
   * @param position The wyckoff position to add the atom to
   * @param atomicNum The atomic number of the atom to be added
   * @param minIAD The minimum interatomic distance that the atom needs to be
   *               from other atoms in the xtal. If -1, there is no min IAD.
   *               Default is -1.
   * @param maxAttempts The number of attempts to make to add the atom randomly
   *                    before the function returns false. Default is 1000.
   *
   * @return True if it succeeded, and false if it failed.
   */
  static bool addWyckoffAtomRandomly(XtalOpt::Xtal* xtal, wyckPos& position,
                                     uint atomicNum, double minIAD = -1,
                                     int maxAttempts = 1000);
  /*
   * Initialze and return a dynamically allocated xtal with a given spacegroup!
   * The lattice mins and lattice maxes provide constraints for the lattice
   * to be generated. The list of atom types tell it which atoms to be added.
   * Returns NULL if it failed to generate the xtal.
   *
   * @param spg The international number for the spacegroup to be generated
   * @param atomTypes A vector of atomic numbers (one for each atom). So if
   *                  our system were Ti1O2, it should be {22, 8, 8}
   * @param latticeMins A latticeStruct that contains the minima for a, b, c,
   *                    alpha, beta, and gamma.
   * @param latticeMaxes A latticeStruct that contains the maxima for a, b, c,
   *                     alpha, beta, and gamma.
   * @param minIAD The minimum interatomic distance that the atom needs to be
   *               from other atoms in the xtal. If -1, there is no min IAD.
   *               Default is -1. Used in addWyckoffAtomRandomly().
   * @param maxAttempts The number of attempts to make to add the atom randomly
   *                    before the function returns false. Default is 1000.
   *                    Used in addWyckoffAtomRandomly().
   *
   * @return A dynamically allocated xtal with the given spacegroup, atoms,
   * and lattice within the provided lattice constraints. Returns NULL
   * if the function failed to successfully generate the xtal.
   */
  static XtalOpt::Xtal* spgInitXtal(uint spg,
                                    const std::vector<uint>& atomTypes,
                                    const latticeStruct& latticeMins,
                                    const latticeStruct& latticeMaxes,
                                    double minIAD = -1, int maxAttempts = 1000);

  static atomAssignments assignAtomsToWyckPos(uint spg,
                                              std::vector<uint> atoms);

};

#endif

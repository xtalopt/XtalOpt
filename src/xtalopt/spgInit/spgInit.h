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
  static bool isSpgPossible(uint spg, const std::vector<uint> atomTypes);

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

  static bool addWyckoffAtomRandomly(XtalOpt::Xtal* xtal, wyckPos& position,
                                     uint atomicNum, double minIAD = -1,
                                     int maxAttempts = 1000);

  static XtalOpt::Xtal* spgInitXtal(uint spg,
                                    const std::vector<uint>& atomTypes,
                                    const latticeStruct& latticeMins,
                                    const latticeStruct& latticeMaxes,
                                    double minIAD, int maxAttempts);
};

#endif

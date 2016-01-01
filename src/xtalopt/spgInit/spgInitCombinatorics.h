

#ifndef SPG_INIT_COMBINATORICS_H
#define SPG_INIT_COMBINATORICS_H

#include <xtalopt/spgInit/spgInit.h>

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
  systemPossibilities getSystemPossibilities(uint spg,
                                             const std::vector<uint>& atoms);

};

#endif

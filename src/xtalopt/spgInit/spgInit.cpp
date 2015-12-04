#include <xtalopt/spgInit/spgInit.h>
#include <tuple>
#include <iostream>

// Define this for debug output
#define SPGINIT_DEBUG

using namespace std;

// Outer vector is the space group. So there are 230 of them
// Inner vector depends on the number of wyckoff elements that
// exist in each space group
// This contains the wyckoff letter, multiplicity, and x,y,z coordinates for
// the first wyckoff position of each spacegroup
static const vector<wyckoffPositions> wyckoffPositionsDatabase
{
  { // 0. Not a real space group...
    wyckInfo{'a',0," "}
  },

  { // 1
    wyckInfo{'a',1,"x,y,z"}
  },

  { // 2 - unique axis b
    wyckInfo{'a',1,"0,0,0"},
    wyckInfo{'b',1,"0,0,0.5"},
    wyckInfo{'c',1,"0,0.5,0"},
    wyckInfo{'d',1,"0.5,0,0"},
    wyckInfo{'e',1,"0.5,0.5,0"},
    wyckInfo{'f',1,"0.5,0,0.5"},
    wyckInfo{'g',1,"0,0.5,0.5"},
    wyckInfo{'h',1,"0.5,0.5,0.5"},
    wyckInfo{'i',2,"x,y,z"}
  },

  { // 3 - unique axis b
    wyckInfo{'a',1,"0,y,0"},
    wyckInfo{'b',1,"0,y,0.5"},
    wyckInfo{'c',1,"0.5,y,0"},
    wyckInfo{'d',1,"0.5,y,0.5"},
    wyckInfo{'e',2,"x,y,z"}
  },

  { // 4 - unique axis b
    wyckInfo{'a',2,"x,y,z"}
  },

  { // 5 - unique axis b
    wyckInfo{'a',2,"0,y,0"},
    wyckInfo{'b',2,"0,y,0.5"},
    wyckInfo{'c',4,"x,y,z"}
  },

  { // 6 - unique axis b
    wyckInfo{'a',1,"x,0,z"},
    wyckInfo{'b',1,"x,0.5,z"},
    wyckInfo{'c',2,"x,y,z"}
  },

  { // 7 - unique axis b
    wyckInfo{'a',2,"x,y,z"}
  },

  { // 8 - unique axis b
    wyckInfo{'a',2,"x,0,z"},
    wyckInfo{'b',4,"x,y,z"}
  },

  { // 9 - uniqe axis b
    wyckInfo{'a',4,"x,y,z"}
  },

  { // 10 - unique axis b
    wyckInfo{'a',1,"0,0,0"},
    wyckInfo{'b',1,"0,0.5,0"},
    wyckInfo{'c',1,"0,0,0.5"},
    wyckInfo{'d',1,"0.5,0,0"},
    wyckInfo{'e',1,"0.5,0.5,0"},
    wyckInfo{'f',1,"0,0.5,0.5"},
    wyckInfo{'g',1,"0.5,0,0.5"},
    wyckInfo{'h',1,"0.5,0.5,0.5"},
    wyckInfo{'i',2,"0,y,0"},
    wyckInfo{'j',2,"0.5,y,0"},
    wyckInfo{'k',2,"0,y,0.5"},
    wyckInfo{'l',2,"0.5,y,0.5"},
    wyckInfo{'m',2,"x,0,z"},
    wyckInfo{'n',2,"x,0.5,z"},
    wyckInfo{'o',4,"x,y,z"}
  },

  { // 11 - unique axis b
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0.5,0,0"},
    wyckInfo{'c',2,"0,0,0.5"},
    wyckInfo{'d',2,"0.5,0,0.5"},
    wyckInfo{'e',2,"x,0.25,z"},
    wyckInfo{'f',4,"x,y,z"}
  },

  { // 12 - unique axis b
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0,0.5,0"},
    wyckInfo{'c',2,"0,0,0.5"},
    wyckInfo{'d',2,"0,0.5,0.5"},
    wyckInfo{'e',4,"0.25,0.25,0"},
    wyckInfo{'f',4,"0.25,0.25,0.5"},
    wyckInfo{'g',4,"0,y,0"},
    wyckInfo{'h',4,"0,y,0.5"},
    wyckInfo{'i',4,"x,0,z"},
    wyckInfo{'j',8,"x,y,z"}
  },

  { // 13 - unique axis b
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0.5,0.5,0"},
    wyckInfo{'c',2,"0,0.5,0"},
    wyckInfo{'d',2,"0.5,0,0"},
    wyckInfo{'e',2,"0,y,0.25"},
    wyckInfo{'f',2,"0.5,y,0.25"},
    wyckInfo{'g',4,"x,y,z"}
  },

  { // 14 - unique axis b
    wyckInfo{'a',2,"0,0,0"},
    wyckInfo{'b',2,"0.5,0,0"},
    wyckInfo{'c',2,"0,0,0.5"},
    wyckInfo{'d',2,"0.5,0,0.5"},
    wyckInfo{'e',4,"x,y,z"}
  },

  { // 15 - unique axis b
    wyckInfo{'a',4,"0,0,0"},
    wyckInfo{'b',4,"0,0.5,0"},
    wyckInfo{'c',4,"0.25,0.25,0"},
    wyckInfo{'d',4,"0.25,0.25,0.5"},
    wyckInfo{'e',4,"0,y,0.25"},
    wyckInfo{'f',8,"x,y,z"}
  },

  { // 16
    wyckInfo{'a',1,"0,0,0"},
    wyckInfo{'b',1,"0.5,0,0"},
    wyckInfo{'c',1,"0,0.5,0"},
    wyckInfo{'d',1,"0,0,0.5"},
    wyckInfo{'e',1,"0.5,0.5,0"},
    wyckInfo{'f',1,"0.5,0,0.5"},
    wyckInfo{'g',1,"0,0.5,0.5"},
    wyckInfo{'h',1,"0.5,0.5,0.5"},
    wyckInfo{'i',2,"x,0,0"},
    wyckInfo{'j',2,"x,0,0.5"},
    wyckInfo{'k',2,"x,0.5,0"},
    wyckInfo{'l',2,"x,0.5,0.5"},
    wyckInfo{'m',2,"0,y,0"},
    wyckInfo{'n',2,"0,y,0.5"},
    wyckInfo{'o',2,"0.5,y,0"},
    wyckInfo{'p',2,"0.5,y,0.5"},
    wyckInfo{'q',2,"0,0,z"},
    wyckInfo{'r',2,"0.5,0,z"},
    wyckInfo{'s',2,"0,0.5,z"},
    wyckInfo{'t',2,"0.5,0.5,z"},
    wyckInfo{'u',4,"x,y,z"}
  },

  { // 17
    wyckInfo{'a',2,"x,0,0"},
    wyckInfo{'b',2,"x,0.5,0"},
    wyckInfo{'c',2,"0,y,0.25"},
    wyckInfo{'d',2,"0.5,y,0.25"},
    wyckInfo{'e',4,"x,y,z"}
  },

  { // 18
    wyckInfo{'a',2,"0,0,z"},
    wyckInfo{'b',2,"0,0.5,z"},
    wyckInfo{'c',4,"x,y,z"}
  },

  { // 19
    wyckInfo{'a',4,"x,y,z"}
  },

  { // 20
    wyckInfo{'a',4,"x,0,0"},
    wyckInfo{'b',4,"0,y,0.25"},
    wyckInfo{'c',8,"x,y,z"}
  },
/*
  { // 21

  },

  { // 22

  },

  { // 23

  },

  { // 24

  },

  { // 25

  },

  { // 26

  },

  { // 27

  },

  { // 28

  },

  { // 29

  },

  { // 30

  },

  { // 31

  },

  { // 32

  },

  { // 33

  },

  { // 34

  },

  { // 35

  },

  { // 36

  },

  { // 37

  },

  { // 38

  },

  { // 39

  },

  { // 40

  },

  { // 41

  },

  { // 42

  },

  { // 43

  },

  { // 44

  },

  { // 45

  },

  { // 46

  },

  { // 47

  },

  { // 48

  },

  { // 49

  },

  { // 50

  },

  { // 51

  },

  { // 52

  },

  { // 53

  },

  { // 54

  },

  { // 55

  },

  { // 56

  },

  { // 57

  },

  { // 58

  },

  { // 59

  },

  { // 60

  },

*/

  { // 230
    wyckInfo{'a',16, "0,0,0"},
    wyckInfo{'b',16, "0.125,0.125,0.125"},
    wyckInfo{'c',24, "0.125,0,0.25"},
    wyckInfo{'d',24, "0.375,0,0.25"},
    wyckInfo{'e',32, "x,x,x"},
    wyckInfo{'f',48, "x,0,0.25"},
    wyckInfo{'g',48, "0.125,y,-y+0.25"},
    wyckInfo{'h',96,"x,y,z"}
  }
};

const wyckoffPositions& SpgInit::getWyckoffPositions(uint spg)
{
  if (spg < 1 || spg > 230) {
    cout << "Error. getWyckoffPositions() was called for a spacegroup "
         << "that does not exist! Given spacegroup is " << spg << endl;
    return wyckoffPositionsDatabase.at(0);
  }

  return wyckoffPositionsDatabase.at(spg);
}

vector<atomStruct> SpgInit::generateInitWyckoffs(uint spg, vector<uint> atomTpes)
{

}

inline vector<uint> SpgInit::getNumOfEachType(vector<uint> atomTypes)
{
  vector<uint> atomsAlreadyCounted, numOfEachType;
  for (size_t i = 0; i < atomTypes.size(); i++) {
    size_t size = 0;
    // If we already counted this one, just continue
    if (std::find(atomsAlreadyCounted.begin(), atomsAlreadyCounted.end(),
                  atomTypes.at(i)) != atomsAlreadyCounted.end()) continue;
    for (size_t j = 0; j < atomTypes.size(); j++)
      if (atomTypes.at(j) == atomTypes.at(i)) size++;
    numOfEachType.push_back(size);
    atomsAlreadyCounted.push_back(atomTypes.at(i));
  }
  sort(numOfEachType.begin(), numOfEachType.end());
  return numOfEachType;
}

// Basic split of a string based upon a delimiter.
static inline vector<string> split(const string& s, char delim)
{
  vector<string> elems;
  stringstream ss(s);
  string item;
  while (getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

// Basic check to see if a string is a number
// Includes negative numbers
static inline bool isNumber(const string& s)
{
  std::string::const_iterator it = s.begin();
  while (it != s.end() && (isdigit(*it) || *it == '-')) ++it;
  return !s.empty() && it == s.end();
}

// A unique position is a position that has no x, y, or z in it
inline bool SpgInit::containsUniquePosition(wyckInfo& info)
{
  vector<string> xyzStrings = split(get<2>(info), ',');
  assert(xyzStrings.size() == 3);
  for (size_t i = 0; i < xyzStrings.size(); i++)
    if (!isNumber(xyzStrings.at(i))) return false;
  return true;
}

vector<pair<uint, bool> > SpgInit::getMultiplicityVector(wyckoffPositions& pos)
{
  std::vector<pair<uint, bool> > multiplicityVector;
  multiplicityVector.reserve(pos.size());
  for (size_t i = 0; i < pos.size(); i++)
    multiplicityVector.push_back(make_pair(get<1>(pos.at(i)), containsUniquePosition(pos.at(i))));
  return multiplicityVector;
}

static bool everyoneFoundAHome(vector<uint> numOfEachType, wyckoffPositions pos)
{
  // The "uint" is the multiplicity and the "bool" is whether it is unique or not
  vector<pair<uint, bool> > multiplicityVector = SpgInit::getMultiplicityVector(pos);

#ifdef SPGINIT_DEBUG
  cout << "multiplicity vector is:\n";
  for (size_t i = 0; i < multiplicityVector.size(); i++)
    cout << multiplicityVector.at(i).first << "\n";
#endif

  // Keep track of which wyckoff positions have been used
  vector<bool> wyckoffPositionUsed;
  wyckoffPositionUsed.reserve(multiplicityVector.size());
  for (size_t i = 0; i < multiplicityVector.size(); i++)
    wyckoffPositionUsed.push_back(false);

  // These are arranged from smallest to largest already
  for (size_t i = 0; i < numOfEachType.size(); i++) {
    bool foundAHome = false;
    for (size_t j = 0; j < multiplicityVector.size(); j++) {
      // If we found a potential home for the atom
      if (numOfEachType.at(i) % multiplicityVector.at(j).first == 0 &&
          // And either it's not unique or it hasn't been used yet
          (!multiplicityVector.at(j).second || !wyckoffPositionUsed.at(j))) {
        wyckoffPositionUsed[j] = true;
        foundAHome = true;
        break;
      }
    }
    if (!foundAHome) return false;
  }
  // If we made it here without returning false, every atom type found a home
  return true;
}

inline bool SpgInit::isSpgPossible(uint spg, vector<uint> atomTypes)
{
#ifdef SPGINIT_DEBUG
  cout << "atomTypes is:\n";
  for (size_t i = 0; i < atomTypes.size(); i++) cout << atomTypes[i] << "\n";
#endif

  wyckoffPositions pos = getWyckoffPositions(spg);
  size_t numAtoms = atomTypes.size();
  vector<uint> numOfEachType = getNumOfEachType(atomTypes);

#ifdef SPGINIT_DEBUG
  cout << "numAtoms is " << numAtoms;
  cout << "numOfEachType is:\n";
  for (size_t i = 0; i < numOfEachType.size(); i++)
    cout << numOfEachType.at(i) << "\n";
#endif

  if (!everyoneFoundAHome(numOfEachType, pos)) return false;

  return true;
}

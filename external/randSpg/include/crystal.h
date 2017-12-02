/**********************************************************************
  crystal.h - Custom crystal class for generating crystals with
              specific space groups.

  Copyright (C) 2015 - 2016 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#ifndef CRYSTAL_H
#define CRYSTAL_H

#include <cstdlib>
#include <vector>

// For some reason, uint isn't always defined on windows...
#ifdef _WIN32
#ifndef UNSIGNEDINT
#define UNSIGNEDINT
typedef unsigned int uint;
#endif
#endif

typedef unsigned int uint;

// Keep these as fractional coordinates
struct atomStruct {
  unsigned int atomicNum;
  double x;
  double y;
  double z;
  atomStruct() : atomicNum(0), x(0), y(0), z(0) {}
  atomStruct(unsigned int _aNum, double _x, double _y, double _z) :
    atomicNum(_aNum), x(_x), y(_y), z(_z) {}
};

// We need a comparison operator for atomStruct...
inline bool operator==(const atomStruct& lhs,
                       const atomStruct& rhs)
{
  if (lhs.atomicNum == rhs.atomicNum &&
      lhs.x  == rhs.x &&
      lhs.y == rhs.y &&
      lhs.z == rhs.z) return true;
  else return false;
}

struct latticeStruct {
  double a;
  double b;
  double c;
  double alpha;
  double beta;
  double gamma;
  // Initialize all the values to be 0
  latticeStruct() : a(0), b(0), c(0), alpha(0), beta(0), gamma(0) {}
  latticeStruct(double _a, double _b, double _c,
                double _alpha, double _beta, double _gamma) :
    a(_a), b(_b), c(_c), alpha(_alpha), beta(_beta), gamma(_gamma) {}
};

// Only use fractional coordinates for now...
class Crystal {
 public:
  /* Constructor.
   *
   * @param a The vector of atoms which shall be present in the crystal.
   * @param l The lattice of the crystal.
   * @param usingVdwRad Determines whether we will be using van der Waals radii
   *                    for interatomic distance checks or covalent radii.
   */
  explicit Crystal(latticeStruct l = latticeStruct(),
                   std::vector<atomStruct> a = std::vector<atomStruct>(),
                   bool usingVdwRad = false);

  /* Set the atoms in this crystal with a new vector of atoms.
   *
   * @param a The new vector of atoms.
   */
  void setAtoms(std::vector<atomStruct> a) {m_atoms = a;};

  /* Get a vector of the atom structs in this crystal.
   *
   * @return The atoms in this crystal.
   */
  std::vector<atomStruct> getAtoms() const {return m_atoms;};

  /* Get a vector of atomic numbers: one atomic number for each atom.
   *
   * @return The vector of atomic numbers.
   */
  std::vector<uint> getVectorOfAtomicNums() const;

  /* Get the number of atoms in this crystal.
   *
   * @return The number of atoms in this crystal.
   */
  uint numAtoms() const {return m_atoms.size();};

  /* Set the lattice struct for this cell. Also resets any cached lattice info.
   *
   * @param l The new lattice.
   */
  void setLattice(latticeStruct l) {m_lattice = l; resetLatticeCaches();};

  /* Get the lattice struct for this cell's lattice.
   *
   * @return The lattice struct for this cell.
   */
  latticeStruct getLattice() const {return m_lattice;};

  /* Set whether we are using van der Waals radii for interatomic distance
   * checks. If we are not, we are using covalent radii.
   *
   * @param b Whether we are using van der Waals radii or not
   */
  void setUsingVdwRadii(bool b) {m_usingVdwRadii = b;};

  /* Are we using van der Waals radii for interatomic distance checks?
   *
   * @return True if we are. False if we are not.
   */
  bool usingVdwRadii() {return m_usingVdwRadii;};

  /* Adds an atom to this crystal.
   *
   * @param atom The atom to be added.
   */
  void addAtom(atomStruct atom) {m_atoms.push_back(atom);};

  /* Checks to see if an atom is already at the position given by 'as'. Adds an
   * atom if one is not.
   *
   * @param as The atom to be added.
   *
   * @return Returns true if the atom was added. Returns false if the position
   *         is already occupied.
   */
  bool addAtomIfPositionIsEmpty(atomStruct& as);

  /* Removes all atoms greater than the index given by 'as'. This assumes
   * that all new atoms were appended to the end of the vector and that there
   * are no old atoms beyond this index.
   *
   * @param as The atom beyond which to remove all atoms.
   */
  void removeAllNewAtomsSince(const atomStruct& as);

  /* Removes an atom, as, that is currently a member of the cell.
   *
   * @param as The atom to be removed.
   */
  void removeAtom(const atomStruct& as);

  /* Removes an atom at index i.
   *
   * @param i The index of the atom to be removed.
   */
  void removeAtomAt(size_t i);

  /* Removes atoms that lie in the same position. This is convenient for
   * spg filling functions because there is a chance we will place an atom
   * on top of another.
   */
  void removeAtomsWithSameCoordinates();

  /* Wrap atoms to a unit cell that are located outside the cell.
   * Since we are using fractional coordinates, this is particularly easy and
   * is done by adding and subtracting 1.
   * (-0.5, 0, 0.5), for example, becomes (0.5, 0, 0.5)
   */
  void wrapAtomsToCell();

  /* Fill a unit cell with duplications of an atom required by a spacegroup. It
   * duplicates an atom according to the most general Wyckoff position, and
   * if it finds that it is placing an atom on top of another, it will not
   * add that atom (since it is probably a more specific Wyckoff position or
   * it has already been filled). It will also check interatomic distances,
   * and if IADs fail, it deletes all the new atoms and returns false.
   *
   * @param spg The spacegroup for which to duplicate the atom.
   * @param as The atom which we wish to duplicate (needs to already be
   *           present in the cell; it won't be added if it isn't present).
   *
   * @return true if successful. False if failed due to IAD failures. If false,
   *         it delete all the new atoms created in the process.
   */
  bool fillCellWithAtom(uint spg, const atomStruct& as);

  /* Calls fillCellWithAtom() for every atom that is currently in the cell.
   *
   * @return false if any of these return false.
   */
  bool fillUnitCell(uint spg);

  /* Get the volume of the cell assuming a, b, and c are all 1. This is
   * useful for converting atoms to Cartesian coordinates.
   *
   * @return The unit volume of the cell.
   */
  double getUnitVolume() const;

  /* Get the volume of the cell in Angstroms cubed.
   *
   * @return The volume of the cell.
   */
  double getVolume() const;

  /* Scales a, b, and c so that a different volume, 'newVolume', is the volume.
   *
   * @param newVolume The new volume to be set.
   */
  void rescaleVolume(double newVolume);

  /* Get a copy of an atom that has cartesian coordinates instead of fractional.
   *
   * @param as The atom of which to receive a copy in cart. coords.
   *
   * @return A copy of the atom with x, y, and z being in Angstroms.
   */
  atomStruct getAtomInCartCoords(const atomStruct& as) const;

  /* Calculates and returns the lattice vectors as a 3x3 vector of vectors
   *
   * @return The lattice vectors as a 3x3 vector of vectors
   */
  std::vector<std::vector<double>> getLatticeVecs() const;

  /* Returns the distance in Angstroms between two atoms. Does not take into
   * account periodicity effects (so please center one of the atoms in the
   * unit cell before calling this function).
   *
   * @param as1 The first atom.
   * @param as2 The second atom.
   *
   * @return The distance in Angstroms between the two atoms.
   */
  double getDistance(const atomStruct& as1, const atomStruct& as2) const;

  /* Find the nearest atom to parameter 'as' and set that neighbor to parameter
   * 'neighbor'. It also returns the distance between them in Angstroms.
   * In order to take into account periodicity effects, it creates a temporary
   * cell in which 'as' is at the center and finds all distances from there.
   * Note: function may not work if 'as' is closest to itself in another cell...
   *
   * @param as The atomstruct for which to find the nearest neighbor. It should
   *           already be an atom present in the crystal.
   * @param neighbor An atomstruct that will be set to be to that of the
   *                 nearest neighbor of as.
   *
   * @return Returns the distance in angstroms between the atoms.
   */
  double findNearestNeighborAtomAndDistance(const atomStruct& as,
                                            atomStruct& neighbor) const;

  /* Shift the cell and wrap the atoms so that an atom is at
   * the center (i. e. position (0.5, 0.5, 0.5)).
   *
   * @param as The atom to be centered. Needs to be an atom present in the cell.
   */
  void centerCellAroundAtom(const atomStruct& as);

  /* Shift the cell and wrap the atoms so that an atom at index ind is at
   * the center (i. e. position (0.5, 0.5, 0.5)).
   *
   * @param ind The index of the atom to be centered.
   */
  void centerCellAroundAtom(size_t ind);

  /* Finds the minimum interatomic distance between two atoms based upon
   * their atomic number and radii information in the ElemInfo class. Any
   * modifications to the radii (scaling or setting) should have been made
   * before this function is called.
   *
   * @param as1 The first atom.
   * @param as2 The second atom.
   *
   * @return The minimum interatomic distance between the two atoms.
   */
  double getMinIAD(const atomStruct& as1, const atomStruct& as2) const;

  /* Calls areIADsOkay(const atomStruct&) for every atom in the cell.
   *
   * @return true if all IADs are okay. False if not.
   */
  bool areIADsOkay() const;

  /* Checks to see if an atom has satisfactory interatomic distances (i. e.,
   * no atoms are too close to this one according to minIAD calculations).
   *
   * @param as The atom to check. It needs to be a member of the cell already.
   *
   * @return true if IADs are okay. False if not.
   */
  bool areIADsOkay(const atomStruct& as) const;

  /* Find the index number of an atom in the cell.
   *
   * @param as The atom for which to find an index number.
   *
   * @return Returns the index number of an atom. If it fails to find the atom,
   *         it prints an error message and returns -1.
   */
  int getAtomIndexNum(const atomStruct& as) const;

  /* Returns the crystal info as a string that has the format of a POSCAR
   *
   * @param title The title that will go on the first line of the POSCAR
   *
   * @return The string containing the POSCAR
   */
  std::string getPOSCARString(const std::string& title = " ") const;

  /* Writes the crystal info to a POSCAR that has filename of 'filename'
   *
   * @param filename The name of the POSCAR file to be written. You may include
   *                 the path if needed.
   * @param title The title that will go on the first line of the POSCAR
   *
   */
  void writePOSCAR(const std::string& filename,
                   const std::string& title = " ") const;

  /* Get a printable string that contains the atom info
   *
   * @param as The atom for which to obtain coords.
   *
   * @return The printable string that contains the atom info.
   */
  static std::string getAtomInfoString(const atomStruct& as);

  /* For debugging: print the atomic number and coordinates of a specific atom.
   *
   * @param as The atom for which to print coords.
   */
  static void printAtomInfo(const atomStruct& as);

  /* Get a printable string that contains atom info for all atoms in the cell.
   *
   * @return The printable string that contains the info on the atoms.
   */
  std::string getAtomInfoString() const;

  /* For debugging: print the atom info for every atom in the cell.
   */
  void printAtomInfo() const;

  /*  Get a printable string that contains the lattice info of the cell.
   */
  std::string getLatticeInfoString() const;

  /* For debugging: print the lattice info of the cell (a, b, c, alpha, beta,
   * and gamma)
   */
  void printLatticeInfo() const;
  /* For debugging: print the lattice vectors
   */
  void printLatticeVecs() const;

  /* Get a printable string that contains atom info and lattice info of a cell
   */
  std::string getCrystalInfoString() const;

  /* For debugging: print atom info and lattice info of a cell
   */
  void printCrystalInfo() const;

  /* For debugging: print IADs
   */
  void printIADs() const;

  /* Reset volume caches. -1.0 indicates that it is not cached
   */
  void resetLatticeCaches()
  {
    m_unitVolume = -1.0;
    m_volume = -1.0;
    m_cartConvMatCached = false;
  };

  // Cache the cartesian conversion matrix
  void cacheCartConvMat() const;

 private:
  latticeStruct m_lattice;
  std::vector<atomStruct> m_atoms;

  // A few cached values to reduce computation time
  // Unit volume
  mutable double m_unitVolume;
  // Volume
  mutable double m_volume;

  // Are we using vdw or covalent radii? We will use covalent by default
  bool m_usingVdwRadii;

  // More cached values
  // Matrix for conversion to cartesian coordinates
  // Since we have an upper triangle matrix, we don't need [1][0], [1][1], and [2][0]
  // They are all zero. So we will skip those values to avoid the extra memory usage
  // Instead, [0][0] is [0], [0][1] is [1], [0][2] is [2], [1][1] is [3], [1][2] is [4], and
  // [2][2] is [5]
  mutable bool m_cartConvMatCached;
  mutable double m_cartConvMat[6];
};

#endif

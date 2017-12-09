/**********************************************************************
  Molecule - a basic molecule class.

  Copyright (C) 2016-2017 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#ifndef GLOBALSEARCH_MOLECULE_H
#define GLOBALSEARCH_MOLECULE_H

#include <globalsearch/structures/atom.h>
#include <globalsearch/structures/bond.h>
#include <globalsearch/structures/unitcell.h>

#include <cassert>
#include <vector>

namespace GlobalSearch {

/**
 * @class Molecule molecule.h
 * @brief A basic molecule class. Contains a vector of atoms and a unit
 *        cell.
 */
class Molecule
{
public:
  /**
   * Molecule constructor. Default set of atoms is empty. Default
   * unit cell is invalid (everything is zero). An invalid unit
   * cell implies that the molecule does not have a unit cell.
   *
   * @param atoms A vector of atoms that the molecule will have. Default is
   *              an empty vector.
   * @param uc The unit cell that the molecule will have. Default is
   *           an invalid unit cell (all zeros for cell parameters).
   */
  explicit Molecule(const std::vector<Atom>& atoms = std::vector<Atom>(),
                    const UnitCell& uc = UnitCell());

  /* Default destructor */
  ~Molecule() = default;

  /* Default copy constructor */
  Molecule(const Molecule& other) = default;

  /* Default move constructor. Static assert noexcept later. */
  Molecule(Molecule&& other) = default;

  /* Default assignment operator */
  Molecule& operator=(const Molecule& other) = default;

  /* Default move assignment operator. Static assert noexcept later. */
  Molecule& operator=(Molecule&& other) = default;

  /**
   * Add an atom. A reference to the newly added atom is returned and
   * may be edited. The atomic number has a default value of 0, and the
   * position has a default value of (0,0,0).
   *
   * @param atomicNum The atomic number of the atom. Default value is 0.
   * @param pos The 3-dimensional Cartesian coordinates of the atom in
   *            Angstroms. The default is (0,0,0).
   *
   * @return A reference to the atom just created.
   */
  Atom& addAtom(unsigned short atomicNum = 0,
                const Vector3& pos = Vector3(0.0, 0.0, 0.0));

  /**
   * This function overloads addAtom. This adds a copy of the Atom object
   * passed to it.
   *
   * @param atom A copy of the Atom object to be added.
   *
   * @return A reference to the atom just created.
   */
  Atom& addAtom(const Atom& atom);

  /**
   * Add another molecule to this molecule. All atoms will be added, and
   * bonds of the newly added atoms will be preserved.
   *
   * @param mol The molecule to be added.
   */
  void addMolecule(const Molecule& mol);

  /**
   * A Molecule class can contain multiple "molecules." This function is
   * designed to separate these molecules by creating separate Molecules for
   * every group of bonded atoms and returning them in a vector. The
   * individual Molecules will all have the same unit cell as the parent
   * Molecule.
   *
   * @return The vector of indvidual molecules (groups of atoms bonded
   *         together).
   */
  std::vector<Molecule> getIndividualMolecules() const;

  /**
   * Set the atoms in the molecule. Any bonds will be cleared.
   *
   * @param atoms The atoms to be set in the molecule.
   */
  void setAtoms(const std::vector<Atom>& atoms);

  /**
   * Remove the atom with index "ind". Returns true on success. Returns
   * false if the index is out of range.
   *
   * @param ind The index of the atom to be removed.
   *
   * @return True on success. False if @p ind is out of range.
   */
  bool removeAtom(size_t ind);

  /**
   * Finds the atom equivalent to @p atom using operator==() and
   * then removes that atom. Returns true if the atom was found and removed.
   * Returns false if it was not.
   *
   * @param atom The atom to be removed.
   *
   * @return True on success. False if the atom was not found.
   */
  bool removeAtom(const Atom& atom);

  /* Clears all atoms from the molecule */
  void clearAtoms()
  {
    m_bonds.clear();
    m_atoms.clear();
  }

  /**
   * Returns the atom at index @p ind. An assertion makes sure that
   * @p ind is not beyond the bounds of the atom vector.
   *
   * @param ind The index of the atom to be returned.
   *
   * @return A reference to the atom that is returned.
   */
  Atom& atom(size_t ind);

  /**
   * Returns a const reference to the atom at index @p ind. An assertion
   * makes sure that @p ind is not beyond the bounds of the atom vector.
   *
   * @param ind The index of the atom to be returned.
   *
   * @return A const reference to the atom that is returned.
   */
  const Atom& atom(size_t ind) const;

  /**
   * Returns a reference to the atoms vector.
   *
   * @return A reference to the vector of atoms.
   */
  std::vector<Atom>& atoms() { return m_atoms; }

  /**
   * Returns a const reference to the atoms vector.
   *
   * @return A const reference to the vector of atoms.
   */
  const std::vector<Atom>& atoms() const { return m_atoms; }

  /**
   * Returns the index of the atom @p atom. Returns -1 if it was not found.
   *
   * @param atom The atom to find the index of.
   *
   * @return The index of the atom. Returns -1 if it was not found.
   */
  long long atomIndex(const Atom& atom) const;

  /**
   * Returns the number of atoms in the molecule.
   *
   * @return The number of atoms in the molecule.
   */
  size_t numAtoms() const { return m_atoms.size(); }

  /**
   * Returns a vector of atomic numbers of the molecule.
   *
   * @return The vector of atomic numbers of the molecule.
   */
  std::vector<unsigned short> atomicNumbers() const;

  /**
   * Returns the atomic number of the atom at index @p ind. An assertion
   * assures us that ind will not be beyond the bounds of the vector.
   *
   * @param ind The index of the atom whose atomic number we are obtaining.
   *
   * @return The atomic number of the atom at index @p.
   */
  unsigned short atomicNumber(size_t ind) const;

  /**
   * Swap the indices of two atoms. This will also ensure that the atom
   * indices in the bonds are properly changed.
   *
   * @param ind1 The index of the first atom to be swapped.
   * @param ind2 The index of the second atom to be swapped.
   */
  void swapAtoms(size_t ind1, size_t ind2);

  /**
   * Sort the atoms. The input, @p sortOrder, should
   * have a size equal to the number of atoms, and every number from
   * 0 to numAtoms() - 1 should be in the vector. This function will
   * automatically adjust the bonds as well so that the atoms remain
   * bonded correctly.
   *
   * @param sortOrder The sort order for the atoms.
   */
  void sortAtoms(std::vector<size_t> sortOrder);

  /**
   * Change the ordering of the atoms. The input, @p newOrder, should
   * have a size equal to the number of atoms, and every number from
   * 0 to numAtoms() - 1 should be in the vector. This function will
   * automatically adjust the bonds as well so that the atoms remain
   * bonded correctly.
   *
   * @param newOrder The new order for the atoms.
   */
  void reorderAtoms(const std::vector<size_t>& newOrder);

  /**
   * Get the cartesian distance between two points. If we have a valid
   * unit cell, we will take into account neighboring images.
   * Otherwise, we will just perform a regular distance calculation.
   *
   * @param A The first point.
   * @param B The second point.
   *
   * @return The distance.
   */
  double distance(const Vector3& A, const Vector3& B) const;

  /**
   * Get the cartesian distance between two atoms. If we have a valid
   * unit cell, we will take into account neighboring atom images.
   * Otherwise, we will just perform a regular distance calculation.
   *
   * @param atomInd1 The first atom index.
   * @param atomInd2 The second atom index.
   *
   * @return The distance.
   */
  double distance(size_t atomInd1, size_t atomInd2) const;

  /**
   * Get the angle (in degrees) between three points (where the second point
   * is the vertex).
   *
   * @param A The first point.
   * @param B The second point (the vertex of the angle).
   * @param C The third point.
   *
   * @return The angle in degrees.
   */
  static double angle(const Vector3& A, const Vector3& B, const Vector3& C);

  /**
   * Get the angle (in degrees) between three atoms (where the second atom is
   * the vertex).
   *
   * @param atomInd1 The first atom index.
   * @param atomInd2 The second atom index (the vertex of the angle).
   * @param atomInd3 The third atom index.
   *
   * @return The angle in degrees.
   */
  double angle(size_t atomInd1, size_t atomInd2, size_t atomInd3) const;

  /**
   * Get the dihedral angle (in degrees) created by four points where the
   * first three points form a plane and the last three points form a plane.
   *
   * @param A The first point.
   * @param B The second point.
   * @param C The third point.
   * @param D The fourth point.
   *
   * @return The dihedral angle in degrees.
   */
  static double dihedral(const Vector3& A, const Vector3& B, const Vector3& C,
                         const Vector3& D);

  /**
   * Get the dihedral angle (in degrees) created by four atoms where the
   * first three atoms form a plane and the last three atoms form a plane.
   *
   * @param atom1 The first atom index.
   * @param atom2 The second atom index.
   * @param atom3 The third atom index.
   * @param atom4 The fourth atom index.
   *
   * @return The dihedral angle (in degrees).
   */
  double dihedral(size_t atomInd1, size_t atomInd2, size_t atomInd3,
                  size_t atomInd4) const;

  /**
   * Does this molecule contain bonds? Returns true if !m_bonds.empty().
   *
   * @return Whether or not the molecule contains bonds.
   */
  bool hasBonds() const { return !m_bonds.empty(); }

  /**
   * How many bonds do we have?
   *
   * @return The number of bonds in the molecule.
   */
  size_t numBonds() const { return m_bonds.size(); }

  /**
   * Create a bond using the atom indices. Does nothing if the index is
   * out of range. There is a default bond order of 1.
   *
   * @param ind1 The index of the first atom in the bond.
   * @param ind2 The index of the second atom in the bond.
   * @param bondOrder The bond order of the bond.
   */
  void addBond(size_t ind1, size_t ind2, unsigned short bondOrder = 1);

  /**
   * Remove the bond at index @p bondInd. Does nothing if out of range.
   *
   * @param bondInd The index of the bond to be removed.
   */
  void removeBond(size_t bondInd);

  /**
   * Remove the bond between the two atoms. Does nothing if the atoms are
   * not bonded or are not in the molecule.
   *
   * @param ind1 The index of the first atom in the bond.
   * @param ind2 The index of the second atom in the bond.
   */
  void removeBondBetweenAtoms(size_t ind1, size_t ind2);

  /**
   * Remove all bonds connected to the atom @param ind.
   *
   * @param ind The index of the atom for which to remove bonds.
   */
  void removeBondsFromAtom(size_t ind);

  /**
   * Get the vector of bonds.
   *
   * @return The vector of bonds.
   */
  std::vector<Bond>& bonds() { return m_bonds; }

  /**
   * Get the vector of bonds. Const version.
   *
   * @return The vector of bonds.
   */
  const std::vector<Bond>& bonds() const { return m_bonds; }

  /**
   * Get the Bond at index @p bondInd.
   *
   * @param bondInd The index for which to get the bond.
   *
   * @return A reference to the Bond object.
   */
  Bond& bond(size_t bondInd);

  /**
   * Get the Bond at index @p bondInd. No edits are allowed here.
   *
   * @param bondInd The index for which to get the bond.
   *
   * @return A const reference to the Bond object.
   */
  const Bond& bond(size_t bondInd) const;

  /**
   * If the two atoms are bonded together, return the index of the bond that
   * is between them. If they are not bonded together, return -1.
   *
   * @param atomInd1 The first atom index of the two atoms bonded together.
   * @param atomInd2 The second atom index of the two atoms bonded together.
   *
   * @return The index of the bond between the atoms or -1 if no bond exists.
   */
  long long bondBetweenAtoms(size_t atomInd1, size_t atomInd2) const;

  /**
   * Is the atom at index @p ind bonded?
   *
   * @return True if it has at least one bond. False otherwise.
   */
  bool isBonded(size_t ind) const;

  /**
   * Are these two atoms bonded together?
   *
   * @param ind1 The index of the first atom.
   * @param ind2 The index of the second atom.
   *
   * @return True if the atoms are bonded together. False otherwise.
   */
  bool areBonded(size_t ind1, size_t ind2) const;

  /**
   * Get the indices of the bonds connected to atom @p ind.
   *
   * @param ind The index of the atom for which to get bonds.
   *
   * @return The indices of the bonds on the atom.
   */
  std::vector<size_t> bonds(size_t ind) const;

  /**
   * Get the indices of the atoms that are bonded to the atom at index
   * @p index.
   *
   * @param ind The index of the atom for which to get other bonded atoms.
   *
   * @return The indices of the atoms bonded to the atom.
   */
  std::vector<size_t> bondedAtoms(size_t ind) const;

  /**
   * Wrap the atoms in an attempt to make all the bonds between atoms as
   * short as possible in a unit cell. This will usually result in some
   * atoms being located outside the unit cell, but it is very helpful
   * for visualization.
   */
  void wrapMoleculesToSmallestBonds();

  /**
   * Remove all bonds.
   */
  void clearBonds() { m_bonds.clear(); }

  /**
   * Do we have a unit cell? Returns true if the unit cell is valid.
   * Returns false otherwise.
   *
   * @return True if the unit cell is valid and false otherwise.
   */
  bool hasUnitCell() const { return m_unitCell.isValid(); };

  /**
   * Set the unit cell of the molecule.
   *
   * @param uc The new unit cell of the molecule.
   */
  void setUnitCell(const UnitCell& uc) { m_unitCell = uc; };

  /**
   * Get a reference to the unit cell of the molcule.
   *
   * @return The unit cell of the molecule.
   */
  UnitCell& unitCell() { return m_unitCell; };

  /**
   * Get a const reference to the unit cell of the molcule.
   *
   * @return The unit cell of the molecule.
   */
  const UnitCell& unitCell() const { return m_unitCell; };

  /**
   * Clear the bonds and atoms, and zero the unit cell.
   */
  void clear()
  {
    m_bonds.clear();
    m_atoms.clear();
    m_unitCell.clear();
  };

private:
  std::vector<Atom> m_atoms;
  std::vector<Bond> m_bonds;
  UnitCell m_unitCell;
};

// Make sure the move constructor is noexcept
static_assert(std::is_nothrow_move_constructible<Molecule>::value,
              "Molecule should be noexcept move constructible.");

// Make sure the move assignment operator is noexcept
static_assert(std::is_nothrow_move_assignable<Molecule>::value,
              "Molecule should be noexcept move assignable.");

inline Molecule::Molecule(const std::vector<Atom>& atoms, const UnitCell& uc)
  : m_atoms(atoms), m_bonds(), m_unitCell(uc)
{
}

inline Atom& Molecule::addAtom(unsigned short atomicNum, const Vector3& pos)
{
  m_atoms.push_back(Atom(atomicNum, pos));
  return m_atoms.back();
}

inline Atom& Molecule::addAtom(const Atom& atom)
{
  m_atoms.push_back(atom);
  return m_atoms.back();
}

inline void Molecule::setAtoms(const std::vector<Atom>& atoms)
{
  m_bonds.clear();
  m_atoms = atoms;
}

inline Atom& Molecule::atom(size_t ind)
{
  assert(ind < m_atoms.size());
  return m_atoms[ind];
}

inline const Atom& Molecule::atom(size_t ind) const
{
  assert(ind < m_atoms.size());
  return m_atoms[ind];
}

inline long long Molecule::atomIndex(const Atom& atom) const
{
  for (size_t i = 0; i < m_atoms.size(); ++i) {
    if (atom == m_atoms[i])
      return i;
  }
  return -1;
}

inline std::vector<unsigned short> Molecule::atomicNumbers() const
{
  std::vector<unsigned short> atomicNums;
  for (size_t i = 0; i < m_atoms.size(); ++i)
    atomicNums.push_back(m_atoms[i].atomicNumber());
  return atomicNums;
}

inline unsigned short Molecule::atomicNumber(size_t ind) const
{
  assert(ind < m_atoms.size());
  return m_atoms[ind].atomicNumber();
}

inline void Molecule::swapAtoms(size_t ind1, size_t ind2)
{
  assert(ind1 < m_atoms.size());
  assert(ind2 < m_atoms.size());
  std::swap(m_atoms[ind1], m_atoms[ind2]);
  for (auto& bond : m_bonds)
    bond.swapIndices(ind1, ind2);
}

inline double Molecule::distance(const Vector3& A, const Vector3& B) const
{
  if (hasUnitCell())
    return m_unitCell.distance(A, B);
  return fabs((A - B).norm());
}

inline double Molecule::distance(size_t atomInd1, size_t atomInd2) const
{
  assert(atomInd1 < m_atoms.size());
  assert(atomInd2 < m_atoms.size());
  return distance(m_atoms[atomInd1].pos(), m_atoms[atomInd2].pos());
}

inline double Molecule::angle(size_t atomInd1, size_t atomInd2,
                              size_t atomInd3) const
{
  assert(atomInd1 < m_atoms.size());
  assert(atomInd2 < m_atoms.size());
  assert(atomInd3 < m_atoms.size());
  return angle(m_atoms[atomInd1].pos(), m_atoms[atomInd2].pos(),
               m_atoms[atomInd3].pos());
}

inline double Molecule::dihedral(size_t atomInd1, size_t atomInd2,
                                 size_t atomInd3, size_t atomInd4) const
{
  assert(atomInd1 < m_atoms.size());
  assert(atomInd2 < m_atoms.size());
  assert(atomInd3 < m_atoms.size());
  assert(atomInd4 < m_atoms.size());
  return dihedral(m_atoms[atomInd1].pos(), m_atoms[atomInd2].pos(),
                  m_atoms[atomInd3].pos(), m_atoms[atomInd4].pos());
}

inline void Molecule::addBond(size_t ind1, size_t ind2,
                              unsigned short bondOrder)
{
  assert(ind1 < m_atoms.size());
  assert(ind2 < m_atoms.size());
  // We will only allow one bond at a time between two atoms
  if (!areBonded(ind1, ind2))
    m_bonds.push_back(Bond(ind1, ind2, bondOrder));
}

inline void Molecule::removeBond(size_t bondInd)
{
  assert(bondInd < m_bonds.size());
  m_bonds.erase(m_bonds.begin() + bondInd);
}

inline Bond& Molecule::bond(size_t bondInd)
{
  assert(bondInd < m_bonds.size());
  return m_bonds[bondInd];
}

inline const Bond& Molecule::bond(size_t bondInd) const
{
  assert(bondInd < m_bonds.size());
  return m_bonds[bondInd];
}
}

#endif

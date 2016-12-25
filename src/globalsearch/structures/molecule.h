/**********************************************************************
  Molecule - a basic molecule class.

  Copyright (C) 2016 by Patrick S. Avery

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
#include <globalsearch/structures/unitcell.h>

#include <cassert>
#include <vector>

namespace GlobalSearch
{

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

    /* Copy constructor. Just copies the data. */
    Molecule(const Molecule& other);

    /* Assignment operator. Just copies the data. */
    Molecule& operator=(const Molecule& other);

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
     * Set the atoms in the molecule.
     *
     * @param atoms The atoms to be set in the molecule.
     */
    void setAtoms(const std::vector<Atom>& atoms) { m_atoms = atoms; };

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
    void clearAtoms() { m_atoms.clear(); };

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
    std::vector<Atom>& atoms() { return m_atoms; };

    /**
     * Returns a const reference to the atoms vector.
     *
     * @return A const reference to the vector of atoms.
     */
    const std::vector<Atom>& atoms() const { return m_atoms; };

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
    size_t numAtoms() const { return m_atoms.size(); };

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
     * Clear the atoms and zero the unit cell.
     */
    void clear() { m_atoms.clear(); m_unitCell.clear(); };

   private:
    std::vector<Atom> m_atoms;
    UnitCell m_unitCell;
  };

  inline Molecule::Molecule(const std::vector<Atom>& atoms,
                            const UnitCell& uc)
    : m_atoms(atoms),
      m_unitCell(uc)
  {
  }

  inline Molecule::Molecule(const Molecule& other)
    : m_atoms(other.m_atoms),
      m_unitCell(other.m_unitCell)
  {
  }

  inline Molecule& Molecule::operator=(const Molecule& other)
  {
    m_atoms = other.m_atoms;
    m_unitCell = other.m_unitCell;
    return *this;
  }

  inline Atom& Molecule::addAtom(unsigned short atomicNum, const Vector3& pos)
  {
    m_atoms.push_back(Atom(atomicNum, pos));
    return m_atoms.back();
  }

  inline bool Molecule::removeAtom(size_t ind)
  {
    if (ind >= m_atoms.size())
      return false;
    m_atoms.erase(m_atoms.begin() + ind);
    return true;
  }

  inline bool Molecule::removeAtom(const Atom& atom)
  {
    size_t index = atomIndex(atom);
    if (index == -1)
      return false;
    else
      removeAtom(index);
    return true;
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
}

#endif

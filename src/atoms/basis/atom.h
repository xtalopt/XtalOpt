/**********************************************************************
  Atom - A basic atom class.

  Copyright (C) 2016 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef ATOMS_ATOM_H
#define ATOMS_ATOM_H

#include <common/vector.h>

#include <type_traits>
#include <utility>

namespace Atoms {

/**
 * @class Atom atom.h
 * @brief A basic atom class. Each atom has an atomic number and a position.
 */
class Atom
{
public:
  /**
   * Atom constructor. Default atomic number is 0. Default position
   * is (0, 0, 0).
   *
   * @param atomicNum The atomic number. Default is zero.
   * @param pos The 3-dimensional Cartesian coordinates in Angstroms.
   *            Default is (0, 0, 0).
   */
  explicit Atom(unsigned short atomicNum = 0,
                const Common::Vector3& pos = Common::Vector3(0.0, 0.0, 0.0));

  /* Default destructor */
  ~Atom() = default;

  /* Default copy constructor */
  Atom(const Atom& other) = default;

  /**
   * Move constructor. Unfortunately, we must define it ourselves for it
   * to be noexcept.
   */
  Atom(Atom&& other) noexcept;

  /* Default assignment operator */
  Atom& operator=(const Atom& other) = default;

  /**
   * Move assignment operator. Unfortunately, we must define it ourselves
   * for it to be noexcept.
   */
  Atom& operator=(Atom&& other) noexcept;

  /* Comparison operator. Just compares the data. */
  bool operator==(const Atom& other) const;

  /**
   * Set the atomic number of the atom. It may be any number between 0 and
   * 255. However, real atoms will be be between 1 and 118.
   *
   * @param num The atomic number.
   */
  void setAtomicNumber(unsigned short num) { a_atomicNumber = num; };

  /**
   * Set the position of the atom using Cartesian coordinates in Angstroms.
   *
   * @param pos The 3-dimensional Cartesian coordinates in Angstroms.
   */
  void setPos(const Common::Vector3& pos) { a_pos = pos; };

  /**
   * Get the atomic number of the atom.
   *
   * @return The atomic number of the atom.
   */
  unsigned short atomicNumber() const { return a_atomicNumber; };

  /**
   * Get the position of the atom in 3-dimensional Cartesian coordinates
   * in Angstroms.
   *
   * @return The 3-dimensional Cartesian position in Angstroms.
   */
  Common::Vector3& pos() { return a_pos; };

  /**
   * Get the position of the atom in 3-dimensional Cartesian coordinates
   * in Angstroms.
   *
   * @return The 3-dimensional Cartesian position in Angstroms.
   */
  const Common::Vector3& pos() const { return a_pos; };

private:
  unsigned short a_atomicNumber;
  Common::Vector3 a_pos;
};

// Make sure the move constructor is noexcept
static_assert(std::is_nothrow_move_constructible<Atom>::value,
              "Atom should be noexcept move contructible.");

// Make sure the move assignment operator is noexcept
static_assert(std::is_nothrow_move_assignable<Atom>::value,
              "Atom should be noexcept move assignable.");

inline Atom::Atom(unsigned short atomicNum, const Common::Vector3& pos)
  : a_atomicNumber(atomicNum), a_pos(pos)
{
}

inline Atom::Atom(Atom&& other) noexcept
  : a_atomicNumber(std::move(other.a_atomicNumber)),
    a_pos(std::move(other.a_pos))
{
}

inline Atom& Atom::operator=(Atom&& other) noexcept
{
  if (this != &other) {
    a_atomicNumber = std::move(other.a_atomicNumber);
    a_pos = std::move(other.a_pos);
  }
  return *this;
}

inline bool Atom::operator==(const Atom& other) const
{
  if (a_atomicNumber == other.a_atomicNumber && a_pos == other.a_pos) {
    return true;
  }
  return false;
}
} // namespace Atoms

#endif

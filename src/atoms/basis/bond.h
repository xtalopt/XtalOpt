/**********************************************************************
  Bond - A basic bond class. Just stores two atom indices and a bond order.
         There is potential for more to be added in the future...

  Copyright (C) 2017 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef ATOMS_BOND_H
#define ATOMS_BOND_H

#include <cstddef>
#include <type_traits>

namespace Atoms {

/**
 * @class Bond bond.h
 * @brief A basic bond class. Just stores two atom indices and a bond order.
 *        There is potential for more to be added in the future...
 */
class Bond
{
public:
  /**
   * Bond constructor. Default bond order is 1.
   *
   * @param ind1 The index of the first atom in the bond.
   * @param ind2 The index of the second atom in the bond.
   * @param bondOrder The bond order. Default is 1.
   */
  explicit Bond(size_t ind1, size_t ind2, unsigned short bondOrder = 1);

  /* Default destructor */
  ~Bond() = default;

  /* Default copy constructor */
  Bond(const Bond& other) = default;

  /* Default move constructor */
  Bond(Bond&& other) = default;

  /* Default assignment operator */
  Bond& operator=(const Bond& other) = default;

  /* Default move assignment operator */
  Bond& operator=(Bond&& other) = default;

  /**
   * Get the index of the first atom in the bond.
   *
   * @return The index of the first atom in the bond.
   */
  size_t first() const { return b_ind1; }

  /**
   * Get the index of the second atom in the bond.
   *
   * @return The index of the second atom in the bond.
   */
  size_t second() const { return b_ind2; }

  /**
   * Swap atom indices in this bond: if the bond contains @p ind1 it becomes
   * @p ind2, and vice versa. Does nothing if the bond contains neither.
   * Useful when swapping atom indices in a geometry.
   *
   * @param ind1 The index (if it exists) to be swapped with @p ind2.
   * @param ind2 The index (if it exists) to be swapped with @p ind1.
   */
  void swapIndices(size_t ind1, size_t ind2);

  /**
   * Set the bond order. Should be 1, 2, 3, or 4...
   *
   * @param o The bond order.
   */
  void setBondOrder(unsigned short o) { b_bondOrder = o; }

  /**
   * Get the bond order.
   *
   * @return The bond order.
   */
  unsigned short bondOrder() const { return b_bondOrder; }

  /**
   * Indicate to this bond that the atom index, @p atomInd, has been
   * removed, so any indices higher than this should be decremented.
   * Because the index has been removed, all bonds to the removed atom
   * should have already been deleted. This bond SHOULD NOT have one
   * of its indices equal to @p atomInd.
   *
   * @param atomInd The atom index that has been removed.
   */
  void atomIndexRemoved(size_t atomInd);

private:
  size_t b_ind1, b_ind2;
  unsigned short b_bondOrder;
};

// Make sure the move constructor is noexcept
static_assert(std::is_nothrow_move_constructible<Bond>::value,
              "Bond should be noexcept move contructible.");

// Make sure the move assignment operator is noexcept
static_assert(std::is_nothrow_move_assignable<Bond>::value,
              "Bond should be noexcept move assignable.");

inline Bond::Bond(size_t ind1, size_t ind2, unsigned short bondOrder)
  : b_ind1(ind1), b_ind2(ind2), b_bondOrder(bondOrder)
{
}

inline void Bond::swapIndices(size_t ind1, size_t ind2)
{
  if (b_ind1 == ind1)
    b_ind1 = ind2;
  else if (b_ind1 == ind2)
    b_ind1 = ind1;

  if (b_ind2 == ind1)
    b_ind2 = ind2;
  else if (b_ind2 == ind2)
    b_ind2 = ind1;
}

inline void Bond::atomIndexRemoved(size_t atomInd)
{
  if (b_ind1 > atomInd)
    --b_ind1;
  if (b_ind2 > atomInd)
    --b_ind2;
}
} // namespace Atoms

#endif

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

#ifndef GLOBALSEARCH_ATOM_H
#define GLOBALSEARCH_ATOM_H

#include <globalsearch/vector.h>

namespace GlobalSearch
{

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
    explicit Atom(unsigned char atomicNum = 0,
                  const Vector3& pos = Vector3(0.0, 0.0, 0.0));

    /* Copy constructor. Just copies the data. */
    Atom(const Atom& other);

    /* Assignment operator. Just copies the data. */
    Atom& operator=(const Atom& other);

    /* Comparison operator. Just compares the data. */
    bool operator==(const Atom& other) const;

    /**
     * Set the atomic number of the atom. It may be any number between 0 and
     * 255. However, real atoms will be be between 1 and 118.
     *
     * @param num The atomic number.
     */
    void setAtomicNumber(unsigned char num) { m_atomicNumber = num; };

    /**
     * Set the position of the atom using Cartesian coordinates in Angstroms.
     *
     * @param pos The 3-dimensional Cartesian coordinates in Angstroms.
     */
    void setPos(const Vector3& pos) { m_pos = pos; };

    /**
     * Get the atomic number of the atom.
     *
     * @return The atomic number of the atom.
     */
    unsigned char atomicNumber() const { return m_atomicNumber; };

    /**
     * Get the position of the atom in 3-dimensional Cartesian coordinates
     * in Angstroms.
     *
     * @return The 3-dimensional Cartesian position in Angstroms.
     */
    Vector3 pos() const { return m_pos; };

   private:
    unsigned char m_atomicNumber;
    Vector3 m_pos;
  };

  inline Atom::Atom(unsigned char atomicNum, const Vector3& pos)
    : m_atomicNumber(atomicNum),
      m_pos(pos)
  {
  }

  inline Atom::Atom(const Atom& other)
    : m_atomicNumber(other.m_atomicNumber),
      m_pos(other.m_pos)
  {
  }

  inline Atom& Atom::operator=(const Atom& other)
  {
    m_atomicNumber = other.m_atomicNumber;
    m_pos = other.m_pos;
    return *this;
  }

  inline bool Atom::operator==(const Atom& other) const
  {
    if (m_atomicNumber == other.m_atomicNumber &&
        m_pos == other.m_pos) {
      return true;
    }
    return false;
  }

}

#endif

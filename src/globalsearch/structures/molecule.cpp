/**********************************************************************
  Molecule - a basic molecule class.

  Copyright (C) 2017 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#include <globalsearch/structures/molecule.h>

namespace GlobalSearch
{
  bool Molecule::removeAtom(size_t ind)
  {
    if (ind >= m_atoms.size())
      return false;
    removeBondsFromAtom(ind);

    // Indicate to the bonds that they should decrement any indices greater
    // than ind.
    for (auto& bond: m_bonds)
      bond.atomIndexRemoved(ind);

    m_atoms.erase(m_atoms.begin() + ind);
    return true;
  }

  bool Molecule::removeAtom(const Atom& atom)
  {
    long long index = atomIndex(atom);
    if (index == -1)
      return false;
    else
      removeAtom(index);
    return true;
  }

  void Molecule::removeBondBetweenAtoms(size_t ind1, size_t ind2)
  {
    assert(ind1 < m_atoms.size());
    assert(ind2 < m_atoms.size());
    for (size_t i = 0; i < m_bonds.size(); ++i) {
      if ((m_bonds[i].first() == ind1 && m_bonds[i].second() == ind2) ||
          (m_bonds[i].first() == ind2 && m_bonds[i].second() == ind1)) {
        removeBond(i);
        --i;
      }
    }
  }

  void Molecule::removeBondsFromAtom(size_t ind)
  {
    assert(ind < m_atoms.size());
    for (size_t i = 0; i < m_bonds.size(); ++i) {
      if (m_bonds[i].first() == ind || m_bonds[i].second() == ind) {
        removeBond(i);
        --i;
      }
    }
  }

  bool Molecule::isBonded(size_t ind) const
  {
    assert(ind < m_atoms.size());
    for (const auto& bond: m_bonds) {
      if (bond.first() == ind || bond.second() == ind)
        return true;
    }
    return false;
  }

  bool Molecule::areBonded(size_t ind1, size_t ind2) const
  {
    assert(ind1 < m_atoms.size());
    assert(ind2 < m_atoms.size());
    for (const auto& bond: m_bonds) {
      if ((bond.first() == ind1 && bond.second() == ind2) ||
          (bond.first() == ind2 && bond.second() == ind1)) {
        return true;
      }
    }
    return false;
  }

  std::vector<size_t> Molecule::bonds(size_t ind) const
  {
    assert(ind < m_atoms.size());
    std::vector<size_t> ret;
    for (size_t i = 0; i < m_bonds.size(); ++i) {
      if (m_bonds[i].first() == ind || m_bonds[i].second() == ind)
        ret.push_back(i);
    }
    return ret;
  }

  std::vector<size_t> Molecule::bondedAtoms(size_t ind) const
  {
    assert(ind < m_atoms.size());
    std::vector<size_t> ret;
    for (size_t i = 0; i < m_atoms.size(); ++i) {
      if (ind == i)
        continue;
      else if (areBonded(i, ind))
        ret.push_back(i);
    }
    return ret;
  }
}

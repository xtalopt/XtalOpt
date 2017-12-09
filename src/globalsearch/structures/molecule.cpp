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

#include <iostream>
#include <unordered_map>

namespace GlobalSearch {
void Molecule::addMolecule(const Molecule& mol)
{
  // First, get an offset for the bond numbering later
  size_t offset = numAtoms();
  // Add the new atoms
  for (const auto& atom : mol.atoms())
    addAtom(atom);
  // Add their bonds
  for (const auto& bond : mol.bonds()) {
    addBond(bond.first() + offset, bond.second() + offset, bond.bondOrder());
  }
}

std::vector<Molecule> Molecule::getIndividualMolecules() const
{
  std::vector<Molecule> ret;

  std::vector<bool> atomAlreadyUsed(numAtoms(), false);

  while (true) {
    // Find the first atom we haven't used yet
    auto it = std::find(atomAlreadyUsed.begin(), atomAlreadyUsed.end(), false);

    if (it == atomAlreadyUsed.end())
      break;

    Molecule newMol;
    newMol.setUnitCell(unitCell());

    size_t startInd = it - atomAlreadyUsed.begin();
    newMol.addAtom(atoms()[startInd]);
    atomAlreadyUsed[startInd] = true;

    // Atoms for which to check bonds
    std::vector<size_t> atomsToCheck(1, startInd);

    // A map of the old indices to the new molecule atom indices.
    std::unordered_map<size_t, size_t> mapToNewIndices;
    mapToNewIndices[startInd] = 0;

    // Find all atoms bonded to other atoms
    while (!atomsToCheck.empty()) {
      size_t checkInd = atomsToCheck[0];

      for (size_t i = startInd + 1; i < numAtoms(); ++i) {
        if (i == checkInd)
          continue;

        // bondInd will be -1 if no such bond exists
        long long bondInd = bondBetweenAtoms(checkInd, i);
        if (bondInd != -1 && !atomAlreadyUsed[i]) {
          newMol.addAtom(atoms()[i]);
          atomAlreadyUsed[i] = true;
          atomsToCheck.push_back(i);
          mapToNewIndices[i] = newMol.numAtoms() - 1;
          newMol.addBond(mapToNewIndices[checkInd], mapToNewIndices[i],
                         bonds()[bondInd].bondOrder());
        }
        // If we have already added the atom, make sure we have added the
        // bond
        else if (bondInd != -1 && atomAlreadyUsed[i]) {
          if (!newMol.areBonded(mapToNewIndices[checkInd],
                                mapToNewIndices[i])) {
            newMol.addBond(mapToNewIndices[checkInd], mapToNewIndices[i],
                           bonds()[bondInd].bondOrder());
          }
        }
      }
      atomsToCheck.erase(atomsToCheck.begin());
    }

    ret.push_back(newMol);
  }

  return ret;
}

bool Molecule::removeAtom(size_t ind)
{
  if (ind >= m_atoms.size())
    return false;
  removeBondsFromAtom(ind);

  // Indicate to the bonds that they should decrement any indices greater
  // than ind.
  for (auto& bond : m_bonds)
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

// We pass by copy because we want to edit a copy of newOrder...
void Molecule::sortAtoms(std::vector<size_t> sortOrder)
{
  assert(sortOrder.size() == m_atoms.size());

  // Only need to do m_atoms.size() - 1 since the last item will
  // automatically be in place.
  for (size_t i = 0; i < m_atoms.size() - 1; ++i) {
    assert(sortOrder[i] < m_atoms.size());

    // Keep swapping until the index is in the correct place
    while (sortOrder[i] != i) {
      size_t newInd = sortOrder[i];
      swapAtoms(i, newInd);
      std::swap(sortOrder[i], sortOrder[newInd]);
    }
  }
}

// We will implement this in terms of sortAtoms()
void Molecule::reorderAtoms(const std::vector<size_t>& newOrder)
{
  std::vector<size_t> sortOrder(newOrder.size(), 0);
  for (size_t i = 0; i < newOrder.size(); ++i) {
    assert(newOrder[i] < sortOrder.size());
    sortOrder[newOrder[i]] = i;
  }
  sortAtoms(sortOrder);
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

long long Molecule::bondBetweenAtoms(size_t atomInd1, size_t atomInd2) const
{
  assert(atomInd1 < m_atoms.size());
  assert(atomInd2 < m_atoms.size());
  for (size_t i = 0; i < m_bonds.size(); ++i) {
    if ((m_bonds[i].first() == atomInd1 && m_bonds[i].second() == atomInd2) ||
        (m_bonds[i].second() == atomInd1 && m_bonds[i].first() == atomInd2)) {
      return i;
    }
  }
  return -1;
}

bool Molecule::isBonded(size_t ind) const
{
  assert(ind < m_atoms.size());
  for (const auto& bond : m_bonds) {
    if (bond.first() == ind || bond.second() == ind)
      return true;
  }
  return false;
}

bool Molecule::areBonded(size_t ind1, size_t ind2) const
{
  assert(ind1 < m_atoms.size());
  assert(ind2 < m_atoms.size());
  for (const auto& bond : m_bonds) {
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

void Molecule::wrapMoleculesToSmallestBonds()
{
  if (!hasBonds() || !hasUnitCell())
    return;

  std::vector<bool> atomAlreadyMoved(numAtoms(), false);

  std::vector<size_t> atomsToCheck(1, 0);

  while (!atomsToCheck.empty()) {
    size_t checkInd = atomsToCheck[0];
    for (size_t i = 0; i < numAtoms(); ++i) {
      if (atomAlreadyMoved[i] || checkInd == i)
        continue;

      if (areBonded(checkInd, i)) {
        const auto& pos1 = atom(checkInd).pos();
        const auto& pos2 = atom(i).pos();
        atom(i).setPos(unitCell().minimumImage(pos2 - pos1) + pos1);
        atomAlreadyMoved[i] = true;
        atomsToCheck.push_back(i);
      }
    }
    atomsToCheck.erase(atomsToCheck.begin());

    // Move on to the next group of bonded atoms if this one is done
    if (atomsToCheck.empty()) {
      auto it =
        std::find(atomAlreadyMoved.begin(), atomAlreadyMoved.end(), false);

      // Break if we are done
      if (it == atomAlreadyMoved.end())
        break;

      // Otherwise, append the new atom to check and keep going
      size_t newInd = it - atomAlreadyMoved.begin();
      atomAlreadyMoved[newInd] = true;
      atomsToCheck.push_back(newInd);
    }
  }
}

double Molecule::angle(const Vector3& A, const Vector3& B, const Vector3& C)
{
  const Vector3& AB = A - B;
  const Vector3& BC = C - B;

  return acos(AB.dot(BC) / (AB.norm() * BC.norm())) * RAD2DEG;
}

double Molecule::dihedral(const Vector3& A, const Vector3& B, const Vector3& C,
                          const Vector3& D)
{
  const Vector3& AB = B - A;
  const Vector3& BC = C - B;
  const Vector3& CD = D - C;

  const Vector3& n1 = AB.cross(BC);
  const Vector3& n2 = BC.cross(CD);

  return atan2(n1.cross(n2).dot(BC / BC.norm()), n1.dot(n2)) * RAD2DEG;
}
}

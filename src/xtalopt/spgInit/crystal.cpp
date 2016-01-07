


#include <iostream>

#include "crystal.h"
#include "spgInit.h"
#include "utilityFunctions.h"

using namespace std;

//#define CRYSTAL_DEBUG

Crystal::Crystal(vector<atomStruct> a, latticeStruct l) :
  m_atoms(a),
  m_lattice(l)
{

}

void Crystal::removeAtomAt(size_t i)
{
  if (i - 1 > m_atoms.size()) std::cout << "Error: tried to remove an atom "
                                        << "at index " << i << " and the "
                                        << "size is only " << m_atoms.size()
                                        << "!\n";
  else m_atoms.erase(m_atoms.begin() + i);
}

static inline bool atomsHaveSamePosition(const atomStruct& a1,
                                         const atomStruct& a2)
{
  // min distance
  double minD = 0.00001;
  double dx = fabs(a1.x - a2.x);
  double dy = fabs(a1.y - a2.y);
  double dz = fabs(a1.z - a2.z);
  if (dx < 0.00001 && dy < 0.00001 && dz < 0.00001) return true;
  return false;
}

// We're assuming we're using fractional coordinates...
static inline void wrapUnitToCell(double& u)
{
  double minD = 0.00001;
  while (u < 0.0) u += 1.0;
  while (u >= 1.0 || fabs(u - 1.0) < minD) u -= 1.0;
}

static inline void wrapAtomToCell(atomStruct& as)
{
  wrapUnitToCell(as.x);
  wrapUnitToCell(as.y);
  wrapUnitToCell(as.z);
}

void Crystal::wrapAtomsToCell()
{
  for (size_t i = 0; i < m_atoms.size(); i++) wrapAtomToCell(m_atoms[i]);
}


void Crystal::removeAtomsWithSameCoordinates()
{
  for (size_t i = 0; i < m_atoms.size(); i++) {
    for (size_t j = i + 1; j < m_atoms.size(); j++) {
      if (atomsHaveSamePosition(m_atoms.at(i), m_atoms.at(j))) {
        removeAtomAt(j);
        j--;
      }
    }
  }
}

void Crystal::addAtomIfPositionIsEmpty(atomStruct& as)
{
  wrapAtomToCell(as);
  bool positionIsEmpty = true;
  for (size_t i = 0; i < m_atoms.size(); i++) {
    if (atomsHaveSamePosition(as, m_atoms.at(i)) &&
        as.atomicNum == m_atoms.at(i).atomicNum) {
      positionIsEmpty = false;
      break;
    }
  }
  if (positionIsEmpty) addAtom(as);
}

void Crystal::fillUnitCell(uint spg)
{
#ifdef CRYSTAL_DEBUG
  cout << "Crystall::fillUnitCell() was called for spg = " << spg
       << " for the following unit cell!\n";
  printCrystalInfo();
#endif
  // In case the atoms aren't already wrapped, go ahead and wrap them...
  wrapAtomsToCell();

  vector<string> dupVec = SpgInit::getVectorOfDuplications(spg);
  vector<string> fpVec = SpgInit::getVectorOfFillPositions(spg);

  // Keep a copy of what the atoms were initially
  vector<atomStruct> atoms = getAtoms();
  // Loop through all these atoms!
  for (size_t i = 0; i < atoms.size(); i++) {
    // cAtom stands for current atom
    const atomStruct& cAtom = atoms.at(i);
    double x = cAtom.x;
    double y = cAtom.y;
    double z = cAtom.z;
    uint atomicNum = cAtom.atomicNum;
    for (size_t j = 0; j < dupVec.size(); j++) {
      // First, we are going to set up the duplicate vector components

      vector<string> dupComponents = split(dupVec.at(j), ',');

      // These are all just numbers, so we can just convert them
      double dupX = stof(dupComponents.at(0));
      double dupY = stof(dupComponents.at(1));
      double dupZ = stof(dupComponents.at(2));

      // Next, we are going to loop through all fill positions
      // Skip the first one. It is always just (x,y,z)
      for (size_t k = 1; k < fpVec.size(); k++) {

        vector<string> fpComponents = split(fpVec.at(k), ',');

        double newX = SpgInit::interpretComponent(fpComponents.at(0), x, y, z) + dupX;
        double newY = SpgInit::interpretComponent(fpComponents.at(1), x, y, z) + dupY;
        double newZ = SpgInit::interpretComponent(fpComponents.at(2), x, y, z) + dupZ;

        atomStruct newAtom(atomicNum, newX, newY, newZ);
        addAtomIfPositionIsEmpty(newAtom);
      }
    }
  }
#ifdef CRYSTAL_DEBUG
  cout << "Filling is complete! Info is now:\n";
  printCrystalInfo();
#endif
}

void Crystal::printAtomInfo() const
{
  for (size_t i = 0; i < m_atoms.size(); i++) {
    const atomStruct& a = m_atoms.at(i);
    cout << "  For " << a.atomicNum << ", coords are: ("
         << a.x << "," << a.y << "," << a.z << ")\n";
  }
}

void Crystal::printLatticeInfo() const
{
  cout << "a: " << m_lattice.a << "\n";
  cout << "b: " << m_lattice.b << "\n";
  cout << "c: " << m_lattice.c << "\n";
  cout << "alpha: " << m_lattice.alpha << "\n";
  cout << "beta: " << m_lattice.beta << "\n";
  cout << "gamma: " << m_lattice.gamma << "\n";
}

void Crystal::printCrystalInfo() const
{
  cout << "\n**** Printing Crystal Info ****\n";
  printLatticeInfo();
  printAtomInfo();
  cout << "**** End Crystal Info ****\n\n";
}


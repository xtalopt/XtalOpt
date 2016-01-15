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
  explicit Crystal(std::vector<atomStruct> a, latticeStruct l,
                   bool usingVdwRad = true);

  void setAtoms(std::vector<atomStruct> a) {m_atoms = a;};
  std::vector<atomStruct> getAtoms() const {return m_atoms;};

  void setLattice(latticeStruct l) {m_lattice = l;};
  latticeStruct getLattice() const {return m_lattice;};

  void addAtom(atomStruct atom) {m_atoms.push_back(atom);};
  // Checks to see if an atom is already there. Adds an atom if one is not
  void addAtomIfPositionIsEmpty(atomStruct& as);
  void removeAtomAt(size_t i);
  void removeAtom(atomStruct& as);
  void removeAtomsWithSameCoordinates();
  void wrapAtomsToCell();
  void fillCellWithAtom(uint spg, const atomStruct& as);
  void fillUnitCell(uint spg);

  double getUnitVolume() const;
  double getVolume() const;
  void rescaleVolume(double newVolume);
  atomStruct getAtomInCartCoords(const atomStruct& as) const;

  double getDistance(const atomStruct& as1, const atomStruct& as2) const;

  // @param as The atomstruct for which to find the nearest neighbor. It should
  //            already be an atom present in the crystal.
  // @param neighbor An atomstruct that will be changed to that of the
  //                 nearest neighbor of as.
  double findNearestNeighborAtomAndDistance(const atomStruct& as,
                                            atomStruct& neighbor) const;

  void centerCellAroundAtom(const atomStruct& as);
  void centerCellAroundAtom(size_t ind);

  // IAD is interatomic distance
  // The radii in elemInfoDatabase.h should already be scaled by the scaling
  // factor before this is called.
  double getMinIAD(const atomStruct& as1, const atomStruct& as2) const;
  // This will be used for all atoms
  bool areIADsOkay() const;
  // This will be used for a single atom
  bool areIADsOkay(const atomStruct& as) const;

  size_t getAtomIndexNum(const atomStruct& as) const;

  static void printAtomInfo(const atomStruct& as);
  void printAtomInfo() const;
  void printLatticeInfo() const;
  void printCrystalInfo() const;

 private:
  std::vector<atomStruct> m_atoms;
  latticeStruct m_lattice;
  // Are we using vdw or covalent radii? We will use vdw by default
  bool m_usingVdwRadii;
};

#endif

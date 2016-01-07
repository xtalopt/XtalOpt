
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
  explicit Crystal(std::vector<atomStruct> a, latticeStruct l);

  void setAtoms(std::vector<atomStruct> a) {m_atoms = a;};
  std::vector<atomStruct> getAtoms() const {return m_atoms;};

  void setLattice(latticeStruct l) {m_lattice = l;};
  latticeStruct getLattice() const {return m_lattice;};

  void addAtom(atomStruct atom) {m_atoms.push_back(atom);};
  // Checks to see if an atom is already there. Adds an atom if one is not
  void addAtomIfPositionIsEmpty(atomStruct& as);
  void removeAtomAt(size_t i);
  void removeAtomsWithSameCoordinates();
  void wrapAtomsToCell();
  void fillUnitCell(uint spg);

  void printAtomInfo() const;
  void printLatticeInfo() const;
  void printCrystalInfo() const;

 private:
  std::vector<atomStruct> m_atoms;
  latticeStruct m_lattice;
};

#endif

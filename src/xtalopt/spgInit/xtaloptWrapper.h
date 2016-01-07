
#include <xtalopt/spgInit/spgInit.h>
#include <xtalopt/structures/xtal.h>
#include "crystal.h"

using namespace XtalOpt;

// Returns a dynamically allocated xtal
Xtal* crystal2Xtal(const Crystal& c)
{
  std::vector<atomStruct> atoms = c.getAtoms();
  latticeStruct lat = c.getLattice();
  Xtal* xtal = new Xtal(lat.a, lat.b, lat.c, lat.alpha, lat.beta, lat.gamma);
  for (size_t i = 0; i < atoms.size(); i++) {
    const atomStruct& as = atoms.at(i);
    Avogadro::Atom* atom = xtal->addAtom();
    // Need to convert these coordinates to cartesian...
    Eigen::Vector3d pos(as.x,as.y,as.z);
    pos = xtal->fracToCart(pos);
    atom->setPos(pos);
    atom->setAtomicNumber(static_cast<int>(as.atomicNum));
  }
  return xtal;
}

Crystal xtal2Crystal(Xtal* xtal)
{
  latticeStruct lat(xtal->getA(), xtal->getB(), xtal->getC(),
                    xtal->getAlpha(), xtal->getBeta(), xtal->getGamma());
  std::vector<atomStruct> atoms;
  QList<Avogadro::Atom*> xAtoms = xtal->atoms();

  for (size_t i = 0; i < xAtoms.size(); i++) {
    unsigned int atomicNum = xAtoms.at(i)->atomicNumber();
    Eigen::Vector3d fracCoords = *(xtal->cartToFrac(xAtoms.at(i)->pos()));
    atomStruct as(atomicNum, fracCoords[0], fracCoords[1], fracCoords[2]);
    atoms.push_back(as);
  }

  Crystal c(atoms, lat);
  return c;
}

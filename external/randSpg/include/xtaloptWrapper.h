/**********************************************************************
  xtaloptWrapper.h - Contains functions that convert between class 'Crystal'
                     and class 'Xtal'

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#ifndef XTALOPT_WRAPPER_H
#define XTALOPT_WRAPPER_H

#include "randSpg.h"
#include <xtalopt/structures/xtal.h>
#include "crystal.h"

namespace RandSpgXtalOptWrapper {
  // Returns a dynamically allocated xtal
  XtalOpt::Xtal* crystal2Xtal(const Crystal& c)
  {
    std::vector<atomStruct> atoms = c.getAtoms();
    latticeStruct lat = c.getLattice();
    XtalOpt::Xtal* xtal = new XtalOpt::Xtal(lat.a, lat.b, lat.c,
                                            lat.alpha, lat.beta, lat.gamma);
    for (size_t i = 0; i < atoms.size(); i++) {
      const atomStruct& as = atoms.at(i);
      GlobalSearch::Atom& atom = xtal->addAtom();
      // Need to convert these coordinates to cartesian...
      GlobalSearch::Vector3 pos(as.x,as.y,as.z);
      pos = xtal->fracToCart(pos);
      atom.setPos(pos);
      atom.setAtomicNumber(as.atomicNum);
    }
    return xtal;
  }

  Crystal xtal2Crystal(XtalOpt::Xtal* xtal)
  {
    latticeStruct lat(xtal->getA(), xtal->getB(), xtal->getC(),
                      xtal->getAlpha(), xtal->getBeta(), xtal->getGamma());
    std::vector<atomStruct> atoms;
    std::vector<GlobalSearch::Atom> xAtoms = xtal->atoms();

    for (size_t i = 0; i < xAtoms.size(); i++) {
      unsigned int atomicNum = xAtoms.at(i).atomicNumber();
      GlobalSearch::Vector3 fracCoords = xtal->cartToFrac(xAtoms.at(i).pos());
      atomStruct as(atomicNum, fracCoords[0], fracCoords[1], fracCoords[2]);
      atoms.push_back(as);
    }

    // Last parameter is false because xtalopt uses covalent radii, not vdw
    // radii
    Crystal c(lat, atoms, false);
    return c;
  }

  // Returns a dynamically allocated xtal
  XtalOpt::Xtal* randSpgXtal(const randSpgInput& input)
  {
    Crystal c = RandSpg::randSpgCrystal(input);
    // If the volume is zero, the generation failed
    if (c.getVolume() == 0) return NULL;
    else return crystal2Xtal(c);
  }
}
#endif

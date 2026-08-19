/**********************************************************************
  XtalOptGenetic - Tools necessary for genetic structure optimization

  Copyright (C) 2009-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef XTALOPTGENETIC_H
#define XTALOPTGENETIC_H

#include <xtalopt/types.h>

namespace XtalOpt {
class Xtal;

// Genetic operators

class XtalOptGenetic
{
public:
  static Xtal* crossover(Xtal* xtal1, Xtal* xtal2, const QList<CellComp>& compa,
                         const EleScaledRadii& elrad, uint numCuts, double minContribution,
                         double& percent1, double& percent2, int minatoms, int maxatoms,
                         bool isVcSearch, bool verbose, bool useScaledIAD = true,
                         bool useCustomIAD = false,
                         const PairCustomDistances* customIADs = nullptr);

  static Xtal* stripple(Xtal* xtal, double sigma_lattice_min,
                        double sigma_lattice_max, double rho_min,
                        double rho_max, uint eta, uint mu,
                        double& sigma_lattice, double& rho);

  static Xtal* permustrain(Xtal* xtal, double sigma_lattice_max, uint exchanges,
                           double& sigma_lattice);

  static Xtal* permutomic(Xtal* xtal, const CellComp& comp, const EleScaledRadii& elrad, int minatoms,
                          int maxatoms, bool verbose, bool useScaledIAD = true,
                          bool useCustomIAD = false,
                          const PairCustomDistances* customIADs = nullptr);

  static Xtal* permucomp(Xtal* xtal, const CellComp& comp, const EleScaledRadii& elrad, int minatoms,
                         int maxatoms, bool verbose, bool useScaledIAD = true,
                         bool useCustomIAD = false,
                         const PairCustomDistances* customIADs = nullptr);
};

} // end namespace XtalOpt

#endif

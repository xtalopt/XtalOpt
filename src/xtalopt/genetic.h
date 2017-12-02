/**********************************************************************
  XtalOptGenetic - Tools neccessary for genetic structure optimization

  Copyright (C) 2009-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef XTALOPTGENETIC_H
#define XTALOPTGENETIC_H

#include <QObject>

namespace XtalOpt {
class Xtal;
struct XtalCompositionStruct;

class XtalOptGenetic : public QObject
{
  Q_OBJECT

public:
  static Xtal* crossover(Xtal* xtal1, Xtal* xtal2, double minimumContribution,
                         double& percent1);
  static Xtal* FUcrossover(Xtal* xtal1, Xtal* xtal2, double minimumContribution,
                           double& percent1, double& percent2,
                           const QList<uint> formulaUnitsList,
                           QHash<uint, XtalCompositionStruct> comp);
  static Xtal* stripple(Xtal* xtal, double sigma_lattice_min,
                        double sigma_lattice_max, double rho_min,
                        double rho_max, uint eta, uint mu,
                        double& sigma_lattice, double& rho);
  static Xtal* permustrain(Xtal* xtal, double sigma_lattice_max, uint exchanges,
                           double& sigma_lattice);

  static void exchange(Xtal* xtal, uint exchanges);
  static void strain(Xtal* xtal, double sigma_lattice);
  static void ripple(Xtal* xtal, double rho, uint eta, uint mu);
};

} // end namespace XtalOpt

#endif

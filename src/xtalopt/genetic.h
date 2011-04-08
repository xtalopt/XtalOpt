/**********************************************************************
  XtalOptGenetic - Tools neccessary for genetic structure optimization

  Copyright (C) 2009-2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef XTALOPTGENETIC_H
#define XTALOPTGENETIC_H

#include <xtalopt/structures/xtal.h>

#include <openbabel/rand.h>

namespace XtalOpt {

  class XtalOptGenetic : public QObject
  {
    Q_OBJECT

   public:
    static Xtal* crossover(Xtal* xtal1, Xtal* xtal2, double minimumContribution, double &percent1);
    static Xtal* stripple(Xtal* xtal,
                          double sigma_lattice_min, double sigma_lattice_max,
                          double rho_min, double rho_max,
                          uint eta, uint mu,
                          double &sigma_lattice,
                          double &rho);
    static Xtal* permustrain(Xtal* xtal, double sigma_lattice_max, uint exchanges, double &sigma_lattice);

    static void exchange(Xtal *xtal, uint exchanges);
    static void strain(Xtal *xtal, double sigma_lattice);
    static void ripple(Xtal* xtal, double rho, uint eta, uint mu);
  };

} // end namespace XtalOpt

#endif

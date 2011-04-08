/**********************************************************************
  GAPC -- A genetic algorithm for protected clusters

  Copyright (C) 2009-2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef GENETIC_H
#define GENETIC_H

#include <gapc/structures/protectedcluster.h>

namespace GAPC {

  class GAPCGenetic
  {
   public:
    static ProtectedCluster* crossover(ProtectedCluster* pc1,
                                       ProtectedCluster* pc2);
    static ProtectedCluster* twist(ProtectedCluster* pc,
                                   double minimumRotation,
                                   double &rotationDeg);
    static ProtectedCluster* exchange(ProtectedCluster* pc,
                                      unsigned int exchanges);
    static ProtectedCluster* randomWalk(ProtectedCluster* pc,
                                        unsigned int numberAtoms,
                                        double minWalk,
                                        double maxWalk);
    static ProtectedCluster* anisotropicExpansion(ProtectedCluster *pc,
                                                  double amp);
  };

} // end namespace GAPC

#endif

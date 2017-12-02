/**********************************************************************
  GAPC -- A genetic algorithm for protected clusters

  Copyright (C) 2009-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
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
  static ProtectedCluster* twist(ProtectedCluster* pc, double minimumRotation,
                                 double& rotationDeg);
  static ProtectedCluster* exchange(ProtectedCluster* pc,
                                    unsigned int exchanges);
  static ProtectedCluster* randomWalk(ProtectedCluster* pc,
                                      unsigned int numberAtoms, double minWalk,
                                      double maxWalk);
  static ProtectedCluster* anisotropicExpansion(ProtectedCluster* pc,
                                                double amp);
};

} // end namespace GAPC

#endif

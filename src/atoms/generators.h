/**********************************************************************
  Generators - Crystal-structure generation functions (RandSpg, random,
               molecular crystals).

  Copyright (C) 2009-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef ATOMS_GENERATORS_H
#define ATOMS_GENERATORS_H

#include <common/constants.h>

#include <QString>

#include <cstddef>
#include <map>
#include <memory>
#include <utility>
#include <vector>

namespace Atoms {

class Geometry;

// Static helper functions that build and return a new Geometry.
class Generators
{
public:
  // Settings shared by the random and RandSpg crystal generators.
  struct CrystalGenerationOptions
  {
    CrystalGenerationOptions();

    // Shared crystal definition and cell bounds.
    std::vector<unsigned int> atomicNumbers;
    double aMin, bMin, cMin, alphaMin, betaMin, gammaMin;
    double aMax, bMax, cMax, alphaMax, betaMax, gammaMax;
    double minVolume, maxVolume;

    // Distance settings. RandSpg uses the the pair distances through a conversion;
    //   random generation uses the explicit radii or pair-distance maps.
    double minRadius;
    double iadScalingFactor;
    std::map<unsigned int, double> atomicRadii;
    std::map<std::pair<unsigned int, unsigned int>, double> pairMinDistances;

    // Generic random generation settings.
    int maxLatticeAttempts;
    int maxAtomPlacementAttempts;
    bool reduceCell;

    // RandSpg/space-group generation settings.
    unsigned int spaceGroup;
    int maxAttempts;
    char verbosity;
    bool forceMostGeneralWyckoffPosition;
    bool verifyWithSpglib;
    double spglibTolerance;
    size_t generationAttempts;
  };

  // Can RandSpg generate spaceGroup for the given atomic numbers?
  static bool canGenerateRandSpg(unsigned int spaceGroup,
    const std::vector<unsigned int>& atomicNumbers);

  // Generate with RandSpg; returns nullptr on failure.
  static std::unique_ptr<Geometry> generateRandSpg(const CrystalGenerationOptions& options);

  // Generate a random structure, no target space group; nullptr on failure.
  static std::unique_ptr<Geometry> generateRandom(const CrystalGenerationOptions& options);

  // Build a molecular crystal from one molecule (a 0D Geometry) placed on
  // symmetry-equivalent sites; nullptr on failure (error holds the reason).
  static std::unique_ptr<Geometry> generateMolecularCrystal(int spaceGroup,
    const Geometry& molecule, QString& error, double symprec = SPGLIB_TOL,
    double distanceScale = 1.0);
};

} // namespace Atoms

#endif

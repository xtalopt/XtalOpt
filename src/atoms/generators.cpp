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

#include <atoms/generators.h>

#include <atoms/basis/atom.h>
#include <atoms/basis/unitcell.h>
#include <atoms/eleminfo.h>
#include <atoms/geometry.h>
#include <common/compatibility/platform_compat.h>
#include <common/constants.h>
#include <common/makeunique.h>
#include <common/matrix.h>
#include <common/output.h>
#include <common/random.h>
#include <common/numericutils.h>
#include <common/vector.h>

#include <randspg/include/randSpg.h>

extern "C" {
#include <spglib/spglib.h>
}

#include <QString>

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <vector>

namespace Atoms {

// Functions for generate a geometry (structures). RandSpg makes symmetric cells,
// while the other functions make random cells or molecular crystals.

namespace {

bool randomLatticeIsAcceptable(const Geometry& structure,
  const Generators::CrystalGenerationOptions& options)
{
  if (!structure.is3D())
    return false;

  if (!GS_ISFINITE(structure.getA()) || std::fabs(structure.getA()) < ZERO08 ||
      !GS_ISFINITE(structure.getB()) || std::fabs(structure.getB()) < ZERO08 ||
      !GS_ISFINITE(structure.getC()) || std::fabs(structure.getC()) < ZERO08 ||
      !GS_ISFINITE(structure.getAlpha()) || std::fabs(structure.getAlpha()) < ZERO08 ||
      !GS_ISFINITE(structure.getBeta()) || std::fabs(structure.getBeta()) < ZERO08 ||
      !GS_ISFINITE(structure.getGamma()) || std::fabs(structure.getGamma()) < ZERO08) {
    return false;
  }

  if (options.reduceCell) {
    const double cutoff = 25.0;
    if (structure.getA() * cutoff < structure.getB() ||
        structure.getA() * cutoff < structure.getC() ||
        structure.getB() * cutoff < structure.getA() ||
        structure.getB() * cutoff < structure.getC() ||
        structure.getC() * cutoff < structure.getA() ||
        structure.getC() * cutoff < structure.getB()) {
      return false;
    }

    if (structure.getAlpha() * cutoff < structure.getBeta() ||
        structure.getAlpha() * cutoff < structure.getGamma() ||
        structure.getBeta() * cutoff < structure.getAlpha() ||
        structure.getBeta() * cutoff < structure.getGamma() ||
        structure.getGamma() * cutoff < structure.getAlpha() ||
        structure.getGamma() * cutoff < structure.getBeta()) {
      return false;
    }
  }

  return structure.getA() >= options.aMin && structure.getA() <= options.aMax &&
         structure.getB() >= options.bMin && structure.getB() <= options.bMax &&
         structure.getC() >= options.cMin && structure.getC() <= options.cMax &&
         structure.getAlpha() >= options.alphaMin && structure.getAlpha() <= options.alphaMax &&
         structure.getBeta() >= options.betaMin && structure.getBeta() <= options.betaMax &&
         structure.getGamma() >= options.gammaMin && structure.getGamma() <= options.gammaMax;
}

double maxRequiredDistance(const Generators::CrystalGenerationOptions& options,
                           unsigned int atomicNumber)
{
  double maxDistance = 0.0;
  if (!options.pairMinDistances.empty()) {
    for (const auto& entry : options.pairMinDistances) {
      if (entry.first.first == atomicNumber)
        maxDistance = std::max(maxDistance, entry.second);
    }
    return maxDistance;
  }

  auto r = options.atomicRadii.find(atomicNumber);
  if (r == options.atomicRadii.end())
    return std::numeric_limits<double>::infinity();

  double maxRadius = 0.0;
  for (const auto& entry : options.atomicRadii)
    maxRadius = std::max(maxRadius, entry.second);

  return r->second + maxRadius;
}

double minPairDistance(const Generators::CrystalGenerationOptions& options,
  unsigned int atomicNumber1, unsigned int atomicNumber2)
{
  // Check the distance for this ordered atom pair.
  const auto key = std::make_pair(atomicNumber1, atomicNumber2);
  auto pairIt = options.pairMinDistances.find(key);
  if (pairIt != options.pairMinDistances.end())
    return pairIt->second;

  if (!options.atomicRadii.empty()) {
    auto r1 = options.atomicRadii.find(atomicNumber1);
    auto r2 = options.atomicRadii.find(atomicNumber2);
    if (r1 == options.atomicRadii.end() || r2 == options.atomicRadii.end())
      return std::numeric_limits<double>::infinity();
    return r1->second + r2->second;
  }

  return 0.0;
}

bool atomPositionIsAllowed(const Geometry& structure, const Common::Vector3& cartCoords,
                           unsigned int atomicNumber,
                           const Generators::CrystalGenerationOptions& options)
{
  if (options.atomicRadii.empty() && options.pairMinDistances.empty())
    return true;

  const double maxCheckDistance = maxRequiredDistance(options, atomicNumber);
  const double maxCheckDistSquared = maxCheckDistance * maxCheckDistance;

  QList<double> squaredDists;
  structure.getSquaredAtomicDistancesToPoint(cartCoords, &squaredDists);
  Q_ASSERT_X(squaredDists.size() == static_cast<int>(structure.numAtoms()), Q_FUNC_INFO,
             "Size of distance list does not match number of atoms.");

  for (int i = 0; i < squaredDists.size(); ++i) {
    if (squaredDists[i] > maxCheckDistSquared)
      continue;

    const double minDistance =
      minPairDistance(options, atomicNumber, structure.atom(i).atomicNumber());
    const double minDistanceWithTol = std::max(0.0, minDistance - ZERO08);
    if (Common::lt(squaredDists[i], minDistanceWithTol * minDistanceWithTol, 0.0))
      return false;
  }

  return true;
}

bool addAtomRandomlyWithOptions(Geometry& structure, unsigned int atomicNumber,
  const Generators::CrystalGenerationOptions& options)
{
  Common::Vector3 cartCoords;
  if (structure.numAtoms() == 0) {
    cartCoords = Common::Vector3(0.0, 0.0, 0.0);
  } else {
    bool success = false;
    for (int i = 0; i < options.maxAtomPlacementAttempts; ++i) {
      const Common::Vector3 fracCoords(Common::getRandDouble(), Common::getRandDouble(),
                               Common::getRandDouble());
      cartCoords = structure.fracToCart(fracCoords);
      if (atomPositionIsAllowed(structure, cartCoords, atomicNumber, options)) {
        success = true;
        break;
      }
    }
    if (!success)
      return false;
  }

  structure.addAtom(static_cast<unsigned short>(atomicNumber), cartCoords);
  return true;
}

} // namespace

Generators::CrystalGenerationOptions::CrystalGenerationOptions()
  : atomicNumbers(),
    aMin(0.0), bMin(0.0), cMin(0.0),
    alphaMin(0.0), betaMin(0.0), gammaMin(0.0),
    aMax(0.0), bMax(0.0), cMax(0.0),
    alphaMax(0.0), betaMax(0.0), gammaMax(0.0),
    minVolume(-1.0), maxVolume(-1.0),
    minRadius(0.0),
    iadScalingFactor(1.0),
    atomicRadii(),
    pairMinDistances(),
    maxLatticeAttempts(100000),
    maxAtomPlacementAttempts(1000),
    reduceCell(true),
    spaceGroup(1),
    maxAttempts(100),
    verbosity('n'),
    forceMostGeneralWyckoffPosition(true),
    verifyWithSpglib(false),
    spglibTolerance(SPGLIB_TOL),
    generationAttempts(1)
{
}

bool Generators::canGenerateRandSpg(unsigned int spaceGroup,
  const std::vector<unsigned int>& atomicNumbers)
{
  return RandSpg::isSpgPossible(spaceGroup, atomicNumbers);
}

std::unique_ptr<Geometry> Generators::generateRandSpg(
  const Generators::CrystalGenerationOptions& options)
{
  latticeStruct latticeMins(options.aMin, options.bMin, options.cMin,
                            options.alphaMin, options.betaMin, options.gammaMin);
  latticeStruct latticeMaxes(options.aMax, options.bMax, options.cMax,
                             options.alphaMax, options.betaMax, options.gammaMax);
  randSpgInput input(options.spaceGroup, options.atomicNumbers, latticeMins, latticeMaxes);
  input.minRadius = options.minRadius;
  input.IADScalingFactor = options.iadScalingFactor;
  input.minVolume = options.minVolume;
  input.maxVolume = options.maxVolume;
  input.maxAttempts = options.maxAttempts;
  input.verbosity = options.verbosity;
  input.forceMostGeneralWyckPos = options.forceMostGeneralWyckoffPosition;

  const size_t attempts = options.generationAttempts > 0
                          ? options.generationAttempts : 1;
  for (size_t i = 0; i < attempts; ++i) {
    Crystal crystal = RandSpg::randSpgCrystal(input);
    if (crystal.getVolume() == 0)
      continue;

    auto structure = make_unique<Geometry>();
    const latticeStruct lattice = crystal.getLattice();
    structure->setCellInfo(lattice.a, lattice.b, lattice.c,
                           lattice.alpha, lattice.beta, lattice.gamma);

    const std::vector<atomStruct> atoms = crystal.getAtoms();
    for (const auto& atomData : atoms) {
      Common::Vector3 frac(atomData.x, atomData.y, atomData.z);
      structure->addAtom(atomData.atomicNum, structure->fracToCart(frac));
    }

    if (options.verifyWithSpglib) {
      structure->findSpaceGroup(options.spglibTolerance);
      if (structure->getSpaceGroupNumber() != options.spaceGroup) {
        continue;
      }
    }

    return structure;
  }

  return nullptr;
}

std::unique_ptr<Geometry> Generators::generateRandom(
  const Generators::CrystalGenerationOptions& options)
{
  if (options.atomicNumbers.empty())
    return nullptr;

  std::unique_ptr<Geometry> structure;
  for (int i = 0; i < options.maxLatticeAttempts; ++i) {
    structure = make_unique<Geometry>();

    const double a = Common::getRandDouble() * (options.aMax - options.aMin) + options.aMin;
    const double b = Common::getRandDouble() * (options.bMax - options.bMin) + options.bMin;
    const double c = Common::getRandDouble() * (options.cMax - options.cMin) + options.cMin;
    const double alpha = Common::getRandDouble() * (options.alphaMax - options.alphaMin) +
      options.alphaMin;
    const double beta = Common::getRandDouble() * (options.betaMax - options.betaMin) +
      options.betaMin;
    const double gamma = Common::getRandDouble() * (options.gammaMax - options.gammaMin) +
      options.gammaMin;

    structure->setCellInfo(a, b, c, alpha, beta, gamma);
    if (options.reduceCell)
      structure->niggliReduce();

    if (randomLatticeIsAcceptable(*structure, options))
      break;
  }

  if (!structure || !randomLatticeIsAcceptable(*structure, options)) {
    return nullptr;
  }

  if (options.minVolume >= 0.0 && options.maxVolume >= options.minVolume) {
    structure->setVolume(Common::getRandDouble(options.minVolume, options.maxVolume));
  }

  std::vector<unsigned int> atomicNumbers = options.atomicNumbers;
  if (!options.atomicRadii.empty()) {
    std::stable_sort(atomicNumbers.begin(), atomicNumbers.end(),
                     [&options](unsigned int lhs, unsigned int rhs) {
      auto l = options.atomicRadii.find(lhs);
      auto r = options.atomicRadii.find(rhs);
      const double lRadius = l != options.atomicRadii.end() ? l->second : 0.0;
      const double rRadius = r != options.atomicRadii.end() ? r->second : 0.0;
      return lRadius > rRadius;
    });
  }

  for (unsigned int atomicNumber : atomicNumbers) {
    if (!addAtomRandomlyWithOptions(*structure, atomicNumber, options)) {
      return nullptr;
    }
  }

  return structure;
}

std::unique_ptr<Geometry> Generators::generateMolecularCrystal(int spaceGroup,
  const Geometry& molecule, QString& error, double symprec, double distanceScale)
{
  // Make a molecular crystal and keep the smallest allowed cell.

  Q_ASSERT(molecule.is0D());
  error.clear();

  if (!molecule.is0D()) {
    error = "Molecular crystal generation requires a 0D molecule (no unit cell).";
    return nullptr;
  }
  if (spaceGroup < 1 || spaceGroup > 230) {
    error = "Space-group number must be in the range 1-230.";
    return nullptr;
  }
  if (distanceScale <= 0.0) {
    error = "Molecular-crystal distance scale must be positive.";
    return nullptr;
  }

  struct MolAtom
  {
    size_t sourceAtomIndex;
    unsigned short atomicNumber;
    double covalentRadius;
    Common::Vector3 frac;
    Common::Vector3 cart;
    int symopIndex;
  };

  struct SymOp
  {
    int rot[3][3];
    double trans[3];
  };

  if (molecule.atoms().empty()) {
    error = "Molecule cannot be empty.";
    return nullptr;
  }

  std::map<unsigned short, size_t> moleculeComposition;
  std::vector<MolAtom> asym;
  asym.reserve(molecule.atoms().size());
  for (size_t i = 0; i < molecule.atoms().size(); ++i) {
    const Atom& sourceAtom = molecule.atoms()[i];
    if (sourceAtom.atomicNumber() == 0) {
      error = "Molecule contains an atom with atomic number 0.";
      return nullptr;
    }
    ++moleculeComposition[sourceAtom.atomicNumber()];
    MolAtom atom;
    atom.sourceAtomIndex = i;
    atom.atomicNumber = sourceAtom.atomicNumber();
    atom.covalentRadius = ElementInfo::getCovalentRadius(atom.atomicNumber);
    atom.frac = sourceAtom.pos();
    atom.cart = sourceAtom.pos();
    atom.symopIndex = -1;
    asym.push_back(atom);
  }

  auto minImageDistance = [](const MolAtom& a, const MolAtom& b, const UnitCell& cell) -> double {
    const Common::Vector3 df = UnitCell::minimumImageFractional(b.frac - a.frac);
    return cell.toCartesian(df).norm();
  };

  auto chooseGenericCell = [](int sg) -> UnitCell {
    bool rhombohedralR = false;
    const int rGroups[] = {146, 148, 155, 160, 161, 166, 167};
    for (size_t i = 0; i < sizeof(rGroups) / sizeof(rGroups[0]); ++i) {
      if (rGroups[i] == sg) {
        rhombohedralR = true;
        break;
      }
    }

    if (sg <= 2)
      return UnitCell(1.0, 1.2, 1.4, 70.0, 80.0, 75.0);
    if (sg <= 15)
      return UnitCell(1.0, 1.2, 1.4, 90.0, 100.0, 90.0);
    if (sg <= 74)
      return UnitCell(1.0, 1.2, 1.4, 90.0, 90.0, 90.0);
    if (sg <= 142)
      return UnitCell(1.0, 1.0, 1.3, 90.0, 90.0, 90.0);
    if (sg <= 167)
      return UnitCell(1.0, 1.0, rhombohedralR ? 1.6 : 1.3, 90.0, 90.0, 120.0);
    if (sg <= 194)
      return UnitCell(1.0, 1.0, 1.3, 90.0, 90.0, 120.0);
    return UnitCell(1.0, 1.0, 1.0, 90.0, 90.0, 90.0);
  };

  int hallNumber = 0;
  for (int hall = 1; hall <= 530; ++hall) {
    const SpglibSpacegroupType type = spg_get_spacegroup_type(hall);
    if (type.number == spaceGroup) {
      hallNumber = hall;
      break;
    }
  }
  if (hallNumber == 0) {
    error = "Space group was not found in the spglib database.";
    return nullptr;
  }

  int rot[192][3][3];
  double trans[192][3];
  const int numOps = spg_get_symmetry_from_database(rot, trans, hallNumber);
  if (numOps <= 0) {
    error = "spglib failed to load symmetry operations.";
    return nullptr;
  }

  std::vector<SymOp> symops(static_cast<size_t>(numOps));
  for (int i = 0; i < numOps; ++i) {
    for (int r = 0; r < 3; ++r) {
      for (int c = 0; c < 3; ++c)
        symops[static_cast<size_t>(i)].rot[r][c] = rot[i][r][c];
      symops[static_cast<size_t>(i)].trans[r] = trans[i][r];
    }
  }

  const UnitCell baseCellShape = chooseGenericCell(spaceGroup);
  const Common::Vector3 refCart = asym[0].cart;
  double minimumScale = 1.0;
  for (size_t i = 0; i < asym.size(); ++i) {
    for (size_t j = i + 1; j < asym.size(); ++j) {
      const Common::Vector3 delta = baseCellShape.toFractional(asym[j].cart - asym[i].cart);
      minimumScale = std::max(minimumScale, std::fabs(delta.x()) / 0.45);
      minimumScale = std::max(minimumScale, std::fabs(delta.y()) / 0.45);
      minimumScale = std::max(minimumScale, std::fabs(delta.z()) / 0.45);
    }
  }

  UnitCell cell = baseCellShape;
  std::vector<MolAtom> sites;
  const double duplicateTolerance = ZERO05;

  auto setScaledCell = [&](double scale) {
    Common::Matrix3 matrix = baseCellShape.cellMatrix();
    matrix *= scale;
    cell.setCellMatrix(matrix);
  };

  auto placeFromCart = [&](const Common::Vector3& anchor) {
    for (size_t i = 0; i < asym.size(); ++i)
      asym[i].frac = anchor + cell.toFractional(asym[i].cart - refCart);
  };

  auto generateSites = [&]() {
    sites.clear();
    for (size_t atomIndex = 0; atomIndex < asym.size(); ++atomIndex) {
      const MolAtom& seed = asym[atomIndex];
      for (size_t symIndex = 0; symIndex < symops.size(); ++symIndex) {
        MolAtom structure = seed;
        structure.symopIndex = static_cast<int>(symIndex);
        for (int r = 0; r < 3; ++r) {
          double value = symops[symIndex].trans[r];
          for (int c = 0; c < 3; ++c)
            value += static_cast<double>(symops[symIndex].rot[r][c]) *
                     seed.frac[static_cast<size_t>(c)];
          structure.frac[static_cast<size_t>(r)] = value;
        }
        structure.frac = cell.wrapFractional(structure.frac);

        bool duplicate = false;
        for (size_t siteIndex = 0; siteIndex < sites.size(); ++siteIndex) {
          if (sites[siteIndex].sourceAtomIndex != structure.sourceAtomIndex)
            continue;
          const Common::Vector3 delta = UnitCell::minimumImageFractional(sites[siteIndex].frac -
                                             structure.frac);
          if (cell.toCartesian(delta).norm() <= duplicateTolerance) {
            duplicate = true;
            break;
          }
        }
        if (!duplicate)
          sites.push_back(structure);
      }
    }
  };

  auto minIntermolecularClearance = [&]() -> double {
    double best = std::numeric_limits<double>::max();
    bool havePair = false;
    for (size_t i = 0; i < sites.size(); ++i) {
      for (size_t j = i + 1; j < sites.size(); ++j) {
        if (sites[i].symopIndex >= 0 && sites[i].symopIndex == sites[j].symopIndex)
          continue;
        const double distance = minImageDistance(sites[i], sites[j], cell);
        const double requiredDistance = distanceScale *
          (sites[i].covalentRadius + sites[j].covalentRadius);
        best = std::min(best, distance - requiredDistance);
        havePair = true;
      }
    }
    return havePair ? best : 0.0;
  };

  auto updateSitesForScale = [&](const Common::Vector3& anchor, double scale) -> double {
    setScaledCell(scale);
    placeFromCart(anchor);
    generateSites();
    return minIntermolecularClearance();
  };

  auto hasCompleteMoleculeComposition = [&](const Geometry& structure) -> bool {
    std::map<unsigned short, size_t> structureComposition;
    for (size_t i = 0; i < structure.atoms().size(); ++i)
      ++structureComposition[structure.atoms()[i].atomicNumber()];

    size_t moleculeCount = 0;
    for (std::map<unsigned short, size_t>::const_iterator it = moleculeComposition.begin();
         it != moleculeComposition.end(); ++it) {
      const std::map<unsigned short, size_t>::const_iterator found =
        structureComposition.find(it->first);
      if (found == structureComposition.end() || found->second == 0 ||
          found->second % it->second != 0)
        return false;

      const size_t elementMoleculeCount = found->second / it->second;
      if (moleculeCount == 0)
        moleculeCount = elementMoleculeCount;
      else if (moleculeCount != elementMoleculeCount)
        return false;
    }

    for (std::map<unsigned short, size_t>::const_iterator it = structureComposition.begin();
         it != structureComposition.end(); ++it) {
      if (moleculeComposition.find(it->first) == moleculeComposition.end())
        return false;
    }

    return moleculeCount > 0;
  };

  Geometry bestStructure;
  bool haveBest = false;
  bool bestMatchesRequested = false;
  uint bestDetectedSpaceGroup = 0;
  QString bestDetectedSpaceGroupSymbol;
  double bestVolume = std::numeric_limits<double>::max();

  const int anchorGrid = 5;
  for (int ix = 1; ix <= anchorGrid; ++ix) {
    for (int iy = 1; iy <= anchorGrid; ++iy) {
      for (int iz = 1; iz <= anchorGrid; ++iz) {
        const Common::Vector3 anchor(static_cast<double>(ix) / static_cast<double>(anchorGrid + 1),
          static_cast<double>(iy) / static_cast<double>(anchorGrid + 1),
          static_cast<double>(iz) / static_cast<double>(anchorGrid + 1));

        double lo = minimumScale;
        double hi = minimumScale;
        double minClearance = updateSitesForScale(anchor, hi);

        bool bracketed = minClearance >= 0.0;
        for (int grow = 0; !bracketed && grow < 20; ++grow) {
          hi *= 2.0;
          minClearance = updateSitesForScale(anchor, hi);
          bracketed = minClearance >= 0.0;
        }

        if (!bracketed)
          continue;

        for (int bisect = 0; bisect < 32; ++bisect) {
          const double mid = 0.5 * (lo + hi);
          minClearance = updateSitesForScale(anchor, mid);
          if (minClearance >= 0.0)
            hi = mid;
          else
            lo = mid;
        }

        updateSitesForScale(anchor, hi);

        Geometry generated;
        generated.setCellInfo(cell.cellMatrix());
        for (size_t i = 0; i < sites.size(); ++i)
          generated.addAtom(sites[i].atomicNumber, generated.fracToCart(sites[i].frac));
        generated.wrapAtomsToCell();
        if (!hasCompleteMoleculeComposition(generated))
          continue;
        generated.findSpaceGroup(symprec);

        const uint detectedSpaceGroup = generated.getSpaceGroupNumber();
        const bool matchesRequested = detectedSpaceGroup == static_cast<uint>(spaceGroup);
        const double volume = generated.getVolume();

        if (!haveBest || (matchesRequested && !bestMatchesRequested) ||
            (matchesRequested == bestMatchesRequested && volume < bestVolume - ZERO08)) {
          bestStructure = generated;
          haveBest = true;
          bestMatchesRequested = matchesRequested;
          bestDetectedSpaceGroup = detectedSpaceGroup;
          bestDetectedSpaceGroupSymbol = generated.getSpaceGroupSymbol();
          bestVolume = volume;
        }
      }
    }
  }

  if (!haveBest) {
    error = "Molecular crystal generation failed: every tested molecular anchor "
      "produced overlapping or too-close symmetry copies.";
    return nullptr;
  }

  if (bestDetectedSpaceGroup != static_cast<uint>(spaceGroup)) {
    Common::warning(
      QString("Requested space group %1; generated structure is detected "
              "as %2 %3. Molecular symmetry can promote the final crystal "
              "to a higher-symmetry group.")
        .arg(spaceGroup)
        .arg(bestDetectedSpaceGroup)
        .arg(bestDetectedSpaceGroupSymbol));
  }

  return make_unique<Geometry>(bestStructure);
}

} // namespace Atoms

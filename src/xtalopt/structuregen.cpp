/**********************************************************************
  structuregen - XtalOpt structure generation workflow

  Copyright (C) 2009-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/xtalopt.h>

#include <xtalopt/constants.h>
#include <xtalopt/genetic.h>
#include <xtalopt/structures/xtal.h>

#include <common/compatibility/platform_compat.h>
#include <common/compatibility/qt_compat.h>
#include <common/chull.h>
#include <common/constants.h>
#include <common/fileutils.h>
#include <common/matrix.h>
#include <common/output.h>
#include <common/random.h>
#include <common/timing.h>
#include <common/numericutils.h>
#include <search/structure.h>
#include <search/optimizer.h>
#include <search/queueinterface.h>
#include <search/queueinterfaces/batch.h>
#include <search/queueinterfaces/queueinterfaces.h>
#include <search/queuemanager.h>
#include <search/search.h>
#include <search/slottedwaitcondition.h>
#include <search/ssh/sshmanager.h>
#include <search/tracker.h>
#include <atoms/eleminfo.h>
#include <atoms/formats/formats.h>
#include <atoms/formats/xyzformat.h>
#include <atoms/generators.h>
#include <atoms/geometry.h>

#include <QList>
#include <QReadLocker>
#include <QReadWriteLock>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>
#include <QTimer>
#include <QWriteLocker>
#include <QtConcurrent>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <utility>
#include <vector>

using namespace Search;

namespace XtalOpt {

namespace {

bool isMolUnitPossibleForComposition(const Atoms::Geometry& molecule, const CellComp& composition)
{
  Q_ASSERT(molecule.is0D());
  if (!molecule.is0D())
    return false;
  const QStringList comp_symbs = composition.getCompositionSymbols();
  const QStringList mole_symbs = molecule.getSymbols();

  for (auto symb : mole_symbs) {
    if (!comp_symbs.contains(symb))
      return false;
    if (composition.getCount(symb) < molecule.getNumberOfAtomsOfSymbol(symb))
      return false;
  }

  return true;
}

bool minDistanceForPolicy(unsigned int atomicNum1, unsigned int atomicNum2,
                          const EleScaledRadii& radii, const PairCustomDistances& customIADs,
                          bool useRadiiIADs, bool useCustomIADs, double* minDistance)
{
  // Custom IAD mode replaces (not augments) the scaled-radii rule; a missing
  //   custom pair means incomplete input.
  if (useCustomIADs) {
    *minDistance = customIADs.getPairDistance(atomicNum1, atomicNum2);
    return *minDistance != PINF;
  }

  if (useRadiiIADs) {
    *minDistance = radii.getMinRadius(atomicNum1) + radii.getMinRadius(atomicNum2);
  } else {
    *minDistance = 0.0;
  }
  return true;
}

bool moleculeUnitPlacementScale(const Atoms::Geometry& molecule, const EleScaledRadii& radii,
                                const PairCustomDistances& customIADs,
                                bool useRadiiIADs, bool useCustomIADs, double* moleculeScale)
{
  Q_ASSERT(molecule.is0D());
  if (!molecule.is0D()) {
    Common::debug(QString("%1: molecule unit must be a 0D geometry.")
                    .arg(__func__));
    return false;
  }
  *moleculeScale = 1.0;
  if (!useRadiiIADs && !useCustomIADs)
    return true;

  // MolUnits are stored at template scale. If the minimum IADs need a larger
  //   unit, expand uniformly by just enough to meet them (shape is preserved).
  double lowerScale = 1.0;
  const std::vector<Atoms::Atom>& atoms = molecule.atoms();
  for (size_t i = 0; i < atoms.size(); ++i) {
    const QString label1 = QString::fromStdString(Atoms::ElementInfo::getAtomicSymbol(atoms[i].atomicNumber()));
    for (size_t j = i + 1; j < atoms.size(); ++j) {
      const QString label2 = QString::fromStdString(Atoms::ElementInfo::getAtomicSymbol(atoms[j].atomicNumber()));
      double requiredDistance = 0.0;
      if (!minDistanceForPolicy(atoms[i].atomicNumber(), atoms[j].atomicNumber(),
                                radii, customIADs, useRadiiIADs, useCustomIADs,
                                &requiredDistance)) {
        Common::debug(QString("%1: missing IAD policy for molUnit type pair %2.")
                        .arg(__func__)
                        .arg(label1+"-"+label2));
        return false;
      }

      if (!Common::gt(requiredDistance, 0.0, ZERO08))
        continue;

      const double currentDistance = (atoms[i].pos() - atoms[j].pos()).norm();
      if (Common::leq(currentDistance, 0.0, ZERO08)) {
        Common::debug(QString("%1: molUnit type pair %2 has zero internal "
                              "distance and cannot satisfy IAD policy.")
                        .arg(__func__)
                        .arg(label1+"-"+label2));
        return false;
      }

      lowerScale = std::max(lowerScale, requiredDistance / currentDistance);
    }
  }

  *moleculeScale = lowerScale;

  return true;
}

bool positionsRespectIADs(const Xtal& xtal, const std::vector<Atoms::Atom>& atoms,
                          const EleScaledRadii& radii, const PairCustomDistances& customIADs,
                          bool useRadiiIADs, bool useCustomIADs)
{
  for (size_t i = 0; i < atoms.size(); ++i) {
    for (size_t j = i + 1; j < atoms.size(); ++j) {
      double minDist = 0.0;
      if (!minDistanceForPolicy(atoms[i].atomicNumber(), atoms[j].atomicNumber(),
                                radii, customIADs, useRadiiIADs, useCustomIADs, &minDist)) {
        return false;
      }
      if (Common::lt(xtal.distance(atoms[i].pos(), atoms[j].pos()), minDist, ZERO08))
        return false;
    }

    for (size_t j = 0; j < xtal.numAtoms(); ++j) {
      double minDist = 0.0;
      if (!minDistanceForPolicy(atoms[i].atomicNumber(), xtal.atom(j).atomicNumber(),
                                radii, customIADs, useRadiiIADs, useCustomIADs, &minDist)) {
        return false;
      }
      if (Common::lt(xtal.distance(atoms[i].pos(), xtal.atom(j).pos()), minDist, ZERO08))
        return false;
    }
  }
  return true;
}

// A molUnit is placed as a rigid body, so it must fit inside the cell without
//   reaching its own periodic images. Along each cell direction the unit's
//   extent plus the needed clearance has to stay within the interplanar
//   spacing; otherwise the unit folds onto a copy of itself.
bool moleculeUnitFitsCell(const std::vector<Common::Vector3>& positions,
                          const Atoms::UnitCell& cell, double clearance)
{
  const Common::Vector3 vectors[3] = { cell.aVector(), cell.bVector(), cell.cVector() };
  const double volume = cell.volume();
  for (int i = 0; i < 3; ++i) {
    const Common::Vector3 area = vectors[(i + 1) % 3].cross(vectors[(i + 2) % 3]);
    const double areaNorm = area.norm();
    if (!Common::gt(areaNorm, 0.0, ZERO08))
      return false;
    const Common::Vector3 normal = area.normalized();
    const double spacing = volume / areaNorm;
    double low = positions.front().dot(normal);
    double high = low;
    for (const auto& position : positions) {
      const double projection = position.dot(normal);
      low = std::min(low, projection);
      high = std::max(high, projection);
    }
    if (Common::gt((high - low) + clearance, spacing, ZERO08))
      return false;
  }
  return true;
}

// A convex-hull facet: a point p is inside the hull when normal.p + offset < 0.
struct HullPlane
{
  Common::Vector3 normal;
  double offset;
};

// A placed molUnit as a rigid body: a bounding sphere for a quick reject, plus
//   the atoms and convex-hull facets (in world coordinates) needed to check two
//   units for real overlap. hull is empty when the unit has no 3D hull (a flat
//   or linear unit); the sphere alone then guards it.
struct MoleculeUnitBody
{
  Common::Vector3 center;
  double radius;
  std::vector<Common::Vector3> atoms;
  std::vector<HullPlane> hull;
};

// True if a new unit overlaps an already-placed one. The bounding spheres give a
//   quick reject (minimum image, so periodic copies count). If they do touch and
//   both units have a hull, the exact shapes decide: each unit is brought to the
//   image of the other it is nearest, and its atoms are tested against the other's
//   hull (a point counts as inside only when it clears every facet by the margin).
//   A unit with no hull (flat or linear) is judged by the sphere alone.
bool moleculeUnitsOverlap(const Xtal& xtal, const MoleculeUnitBody& a, const MoleculeUnitBody& b)
{
  if (!Common::lt(xtal.distance(a.center, b.center), a.radius + b.radius, ZERO08))
    return false;
  if (a.hull.empty() || b.hull.empty())
    return true;

  const double margin = 0.1;
  auto inside = [margin](const std::vector<HullPlane>& hull,
                         const Common::Vector3& point) -> bool {
    for (const auto& plane : hull)
      if (plane.normal.dot(point) + plane.offset > -margin)
        return false;
    return true;
  };

  const Common::Vector3 delta = b.center - a.center;
  const Common::Vector3 shift = xtal.unitCell().minimumImage(delta) - delta;
  for (const auto& atom : b.atoms)
    if (inside(a.hull, atom + shift))
      return true;
  for (const auto& atom : a.atoms)
    if (inside(b.hull, atom - shift))
      return true;
  return false;
}

bool addMoleculeRandomly(Xtal& xtal, const Atoms::Geometry& molecule,
                         const EleScaledRadii& radii, const PairCustomDistances& customIADs,
                         bool useRadiiIADs, bool useCustomIADs,
                         double moleculeScale, int maxAttempts,
                         std::vector<MoleculeUnitBody>& placedUnits)
{
  // The molecule is a 0D unit (local Cartesian atoms); it is placed into the
  //   crystal's 3D cell, so its own dimensionality and the xtal's both matter.
  Q_ASSERT(molecule.is0D());
  Q_ASSERT(xtal.is3D());
  if (!molecule.is0D() || !xtal.is3D()) {
    Common::debug(QString("%1: molecule unit must be 0D and the crystal 3D.")
                    .arg(__func__));
    return false;
  }
  const std::vector<Atoms::Atom>& moleculeAtoms = molecule.atoms();

  // Clearance the unit keeps from its own periodic images: the largest of the
  //   minimum interatomic distances among its element pairs, so an image never
  //   sits closer than the IADs would allow between any of its atoms.
  double imageClearance = 0.0;
  for (size_t i = 0; i < moleculeAtoms.size(); ++i)
    for (size_t j = i; j < moleculeAtoms.size(); ++j) {
      double minDist = 0.0;
      if (minDistanceForPolicy(moleculeAtoms[i].atomicNumber(), moleculeAtoms[j].atomicNumber(),
                               radii, customIADs, useRadiiIADs, useCustomIADs, &minDist))
        imageClearance = std::max(imageClearance, minDist);
    }

  // The unit's convex hull in its own (template) frame, computed once here and
  //   moved to each trial placement below. Empty for a flat or linear unit, in
  //   which case its overlaps are judged by the bounding sphere alone.
  std::vector<HullPlane> referenceHull;
  {
    std::vector<double> points;
    points.reserve(moleculeAtoms.size() * 3);
    for (const auto& atom : moleculeAtoms) {
      points.push_back(atom.pos().x());
      points.push_back(atom.pos().y());
      points.push_back(atom.pos().z());
    }
    std::vector<double> normals, offsets;
    if (Common::convexHullPlanes(points, static_cast<int>(moleculeAtoms.size()), 3,
                                 normals, offsets)) {
      referenceHull.reserve(offsets.size());
      for (size_t i = 0; i < offsets.size(); ++i)
        referenceHull.push_back({ Common::Vector3(normals[i * 3], normals[i * 3 + 1],
                                                  normals[i * 3 + 2]), offsets[i] });
    }
  }

  for (int attempt = 0; attempt < maxAttempts; ++attempt) {
    const Common::Vector3 center = xtal.fracToCart(Common::Vector3(Common::getRandDouble(),
                                      Common::getRandDouble(), Common::getRandDouble()));
    const Common::Matrix3 rotation = Common::rotationMatrixRandom();

    // The rotated and scaled unit, before wrapping, is what the fit test needs.
    std::vector<Common::Vector3> positions;
    positions.reserve(moleculeAtoms.size());
    for (const auto& atom : moleculeAtoms)
      positions.push_back(center + Common::rotatePoint(rotation, atom.pos() * moleculeScale));

    if (!moleculeUnitFitsCell(positions, xtal.unitCell(), imageClearance))
      continue;

    // This placement's rigid body: bounding sphere plus the reference hull moved
    //   to the current rotation, scale, and center (world-frame facets).
    double radius = 0.0;
    for (const auto& position : positions)
      radius = std::max(radius, (position - center).norm());
    std::vector<HullPlane> worldHull;
    worldHull.reserve(referenceHull.size());
    for (const auto& plane : referenceHull) {
      const Common::Vector3 normal = Common::rotatePoint(rotation, plane.normal);
      worldHull.push_back({ normal, moleculeScale * plane.offset - normal.dot(center) });
    }
    const MoleculeUnitBody body{ center, radius, positions, std::move(worldHull) };

    bool overlapsPlaced = false;
    for (const auto& placed : placedUnits) {
      if (moleculeUnitsOverlap(xtal, body, placed)) {
        overlapsPlaced = true;
        break;
      }
    }
    if (overlapsPlaced)
      continue;

    std::vector<Atoms::Atom> placedAtoms;
    placedAtoms.reserve(positions.size());
    for (size_t i = 0; i < positions.size(); ++i)
      placedAtoms.push_back(Atoms::Atom(moleculeAtoms[i].atomicNumber(),
                                        xtal.unitCell().wrapCartesian(positions[i])));

    if (!positionsRespectIADs(xtal, placedAtoms, radii, customIADs, useRadiiIADs, useCustomIADs)) {
      continue;
    }

    for (const auto& atom : placedAtoms)
      xtal.addAtom(atom);
    placedUnits.push_back(body);
    return true;
  }

  return false;
}

QList<uint> sortedResidualAtoms(CellComp composition)
{
  QList<uint> atoms;
  QList<uint> atomicNums = composition.getCompositionAtomicNumbers();
  std::sort(atomicNums.begin(), atomicNums.end());
  for (const auto& atomicNum : atomicNums) {
    for (uint i = 0; i < composition.getCount(atomicNum); ++i)
      atoms.append(atomicNum);
  }
  return atoms;
}

QList<unsigned int> chemicalSystemAtomicNumbers(const QList<CellComp>& compList)
{
  QList<unsigned int> atomicNums;
  for (int compIndex = 0; compIndex < compList.size(); ++compIndex) {
    const QList<unsigned int> compAtomicNums = compList.at(compIndex).getCompositionAtomicNumbers();
    for (int i = 0; i < compAtomicNums.size(); ++i) {
      const unsigned int atomicNum = compAtomicNums.at(i);
      if (atomicNum != 0 && !atomicNums.contains(atomicNum))
        atomicNums.append(atomicNum);
    }
  }
  std::sort(atomicNums.begin(), atomicNums.end());
  return atomicNums;
}

} // namespace

void XtalOpt::buildInitialGenerationPlan(InitialGenerationPlan& plan, bool reportWarnings)
{
  plan = InitialGenerationPlan();
  plan.seedCount = static_cast<uint>(seedList().size());

  // The list of forced space-group counts (index = spg-1, 0 = not forced).
  plan.randSpgCounts.clear();
  for (uint spg = 1; spg <= 230; ++spg) {
    const int requested = (static_cast<int>(spg) <= minXtalsOfSpg().size())
        ? minXtalsOfSpg().at(spg - 1)
        : 0;
    plan.randSpgCounts.append(requested > 0 ? requested : 0);
  }
  for (int i = 0; i < plan.randSpgCounts.size(); ++i) {
    const int requested = plan.randSpgCounts.at(i);
    if (requested <= 0)
      continue;

    const uint spg = static_cast<uint>(i + 1);
    const int compatibleFormulas = randSpgCompatibleFormulaStrings(spg).size();
    if (compatibleFormulas == 0) {
      if (reportWarnings && isVerbose()) {
        Common::message(tr("   forced RandSpg space group %1 (requested %2 times) can't work "
                           "for any input formula.").arg(spg).arg(requested));
      }
      plan.randSpgCounts[i] = -1;
      continue;
    }

    plan.forcedRandSpgCount += static_cast<uint>(requested * compatibleFormulas);
  }

  plan.randomCount = getNumInitial();
  plan.totalTarget = plan.seedCount + plan.forcedRandSpgCount + plan.randomCount;
}

// Check and add an initial structure. Delete it when it is not accepted.
bool XtalOpt::acceptInitialXtal(Xtal* generated)
{
  if (!checkXtal(generated)) {
    delete generated;
    return false;
  }

  generated->findSpaceGroup(getTolSpg());
  if (isVerbose()) {
    Common::message(QString("   generated initial structure: %1")
                    .arg(generated->getParents()));
  }
  return initializeAndAddXtal(generated, 1, generated->getParents());
}

void XtalOpt::updateProgressBar(size_t goal, size_t attempted, size_t succeeded)
{
  // Update progress bar
  updateProgressValue(succeeded,
                      tr("%1 structures generated (%2 kept, %3 rejected)...")
                        .arg(attempted)
                        .arg(succeeded)
                        .arg(attempted - succeeded),
                      -1,
                      goal);
}

bool XtalOpt::generateInitialStructures()
{
  ////////////////////////////////////////////////////////////////////////
  /// This function generates "initial population". Generally, it generates:
  ///
  /// a) "forced structures" when applicable:
  ///   1) seed structures (if user provides any)
  ///   2) relevant spg#s to all input formulas (if forced randSpg is set)
  ///
  /// b) "random structures" up to "total random initial structures", from:
  ///   3) random structures using randSpg (if user sets usingRandSpg)
  ///   4) random structures using molUnit (if user sets molUnits)
  ///   5) random structures from compositions
  ////////////////////////////////////////////////////////////////////////

  InitialGenerationPlan plan;
  buildInitialGenerationPlan(plan);

  // Set up progress reporting
  beginProgressUpdate(tr("Generating structures..."), 0, 0);

  // Initialize loop variables
  int failed = 0;
  QString filename;

  // Use new xtal count in case "addXtal" falls behind so that we
  //   don't duplicate structures when switching from seeds -> random.
  uint newXtalCount = 0;

  // Load seeds...
  for (int i = 0; i < seedList().size(); i++) {
    filename = seedList().at(i);
    if (this->addSeed(filename)) {
      updateProgressBar(plan.totalTarget, newXtalCount + failed, newXtalCount);
      newXtalCount++;
    } else {
      failed++;
    }
  }

  // Generate requested RandSpg structures. The plan keeps the remaining count.
  if (plan.forcedRandSpgCount > 0) {
    QList<int> spgStillNeeded = plan.randSpgCounts;
    for (int i = 0; i < spgStillNeeded.size(); i++) {
      while (spgStillNeeded.at(i) > 0) {
        for (int compi = 0; compi < compList().size(); compi++) {
          uint spg = i + 1;

          // If the spacegroup isn't possible, just continue
          if (!isRandSpgPossibleForComposition(spg, compList()[compi]))
            continue;

          updateProgressBar(plan.totalTarget, newXtalCount + failed, newXtalCount);
          if (acceptInitialXtal(randSpgXtal(1, newXtalCount + 1, compList()[compi], spg)))
            newXtalCount++;
          else
            failed++;
        }
        spgStillNeeded[i]--;
      }
    }
  }

  // Generate the requested random initial structures (retry each until one is
  //   accepted, up to a fixed total count).
  const int maxRandomAttempts = 10000;
  uint randomGenerated = 0;
  while (randomGenerated < plan.randomCount) {
    bool accepted = false;
    for (int attempt = 0; attempt < maxRandomAttempts; ++attempt) {
      updateProgressBar(plan.totalTarget, newXtalCount + failed, newXtalCount);
      if (acceptInitialXtal(generateRandomXtal(1, newXtalCount + 1))) {
        newXtalCount++;
        accepted = true;
        break;
      }
      failed++;
    }
    if (!accepted) {
      Common::warning(QString("%1: failed too many times while generating %2. "
                              "Giving up.")
                        .arg(__func__)
                        .arg(tr("random initial structure")));
      endProgressUpdate();
      return false;
    }
    randomGenerated++;
  }

  // Wait for all structures to appear in tracker
  updateProgressValue(-1, tr("Waiting for structures to initialize..."), 0, newXtalCount);

  connect(tracker(), &Tracker::newStructureAdded, x_initWC, &SlottedWaitCondition::wakeAllSlot);

  x_initWC->prewaitLock();
  for (;;) {
    int initializedStructures = 0;
    {
      QReadLocker trackerLocker(tracker()->rwLock());
      initializedStructures = tracker()->size();
    }
    updateProgressValue(initializedStructures,
                        tr("Waiting for structures to initialize (%1 of %2)...")
                          .arg(initializedStructures)
                          .arg(newXtalCount));
    if (initializedStructures >= static_cast<int>(newXtalCount))
      break;
    // Do not wait here forever. A final signal can arrive before the wait.
    x_initWC->wait(INIT_WAIT_TIMEOUT);
  }
  x_initWC->postwaitUnlock();

  // We're done with x_initWC.
  disconnect(tracker(), &Tracker::newStructureAdded, x_initWC, &SlottedWaitCondition::wakeAllSlot);

  endProgressUpdate();

  return true;
}

void XtalOpt::generateNewStructure()
{
  // Generate in background thread:
  (void)QtConcurrent::run([this]() { this->generateNewStructure_(); });
}

void XtalOpt::generateNewStructure_()
{
  QReadLocker runtimeLocker(runtimeSettingsLock());

  // This function is being used to generate new structures.
  // We choose a random composition each time it's called.

  CellComp randomComp = pickRandomCompositionFromPossibleOnes();

  Xtal* newXtal = generateNewXtal(randomComp);
  if (!newXtal) {
    queue()->structureGenerationFailed();
    return;
  }

  initializeAndAddXtal(newXtal, newXtal->getGeneration(),
                       newXtal->getParents());
}

CellComp XtalOpt::pickRandomCompositionFromPossibleOnes()
{
  if (compList().isEmpty()) {
    Common::error(QString("%1: empty composition list.").arg(__func__));
    return CellComp();
  }

  if (compList().size() == 1)
    return compList()[0];

  return compList()[Common::getRandInt(0, compList().size() - 1)];
}

Xtal* XtalOpt::generateNewXtal(CellComp incomp)
{
  Common::ScopedTimer _timer("XtalOpt::generateNewXtal");
  QReadLocker runtimeLocker(runtimeSettingsLock());

  // A replacement for an invalid-composition seed must use one of the run's
  //   valid compositions rather than preserving the seed composition.
  if (incomp.getNumTypes() == 0) {
    if (compList().isEmpty()) {
      Common::error(QString("%1: empty composition list.").arg(__func__));
      return nullptr;
    }
    incomp = pickRandomCompositionFromPossibleOnes();
  }

  // Try to get it from the probability list only if we have large
  //   enough parents pool. Otherwise, generate randomly.
  if (getParentPoolSize() < 3) {
    Xtal* xtal = nullptr;

    int maxAttempts = 10000;
    int attemptCount = 0;
    while (!checkXtal(xtal)) {
      if (xtal)
        delete xtal;
      if (attemptCount >= maxAttempts) {
        Common::warning(QString("%1: failed too many times. Giving up.")
                         .arg(__func__));
        return nullptr;
      }
      ++attemptCount;
      xtal = generateRandomXtal(1, 0, incomp);
    }
    return xtal;
  }

  Xtal* xtal = generateEvolvedXtal();
  return xtal;
}

Xtal* XtalOpt::generateRandomXtal(uint generation, uint id, CellComp incomp)
{
  if (incomp.getNumTypes() == 0) {
    if (compList().isEmpty())
      return nullptr;
    incomp = pickRandomCompositionFromPossibleOnes();
  }

  bool randSpgPossible = false;
  if (getUsingRandSpg()) {
    // RandSpg draws from every space group that is geometrically possible for
    //   this composition. (Forced counts play no role in the random pool.)
    for (uint spg = 1; spg <= 230; ++spg) {
      if (isRandSpgPossibleForComposition(spg, incomp)) {
        randSpgPossible = true;
        break;
      }
    }
  }

  bool molUnitPossible = false;
  for (const auto& molecule : moleculeUnits()) {
    if (isMolUnitPossibleForComposition(molecule, incomp)) {
      molUnitPossible = true;
      break;
    }
  }

  // If both RandSpg and molUnit can run, try them in random order so neither
  //   dominates the random initial pool.
  if (randSpgPossible && molUnitPossible) {
    if (Common::getRandInt(0, 1) == 0) {
      if (Xtal* xtal = generateRandomRandSpgXtal(generation, id, incomp))
        return xtal;
      if (Xtal* xtal = generateRandomMolUnitXtal(generation, id, incomp))
        return xtal;
    } else {
      if (Xtal* xtal = generateRandomMolUnitXtal(generation, id, incomp))
        return xtal;
      if (Xtal* xtal = generateRandomRandSpgXtal(generation, id, incomp))
        return xtal;
    }
  } else if (randSpgPossible) {
    if (Xtal* xtal = generateRandomRandSpgXtal(generation, id, incomp))
      return xtal;
  } else if (molUnitPossible) {
    if (Xtal* xtal = generateRandomMolUnitXtal(generation, id, incomp))
      return xtal;
  }

  // Atomic random generation is the final fallback: no structural
  //   prerequisites beyond composition and lattice limits.
  return generateRandomAtomicXtal(generation, id, incomp);
}

Xtal* XtalOpt::generateRandomRandSpgXtal(uint generation, uint id, CellComp incomp)
{
  if (incomp.getNumTypes() == 0) {
    if (compList().isEmpty())
      return nullptr;
    incomp = pickRandomCompositionFromPossibleOnes();
  }

  QList<uint> possibleSpgs;
  for (uint spg = 1; spg <= 230; ++spg) {
    if (isRandSpgPossibleForComposition(spg, incomp))
      possibleSpgs.append(spg);
  }

  if (possibleSpgs.isEmpty())
    return nullptr;

  const uint spg = possibleSpgs.at(Common::getRandInt(0, possibleSpgs.size() - 1));
  Xtal* xtal = randSpgXtal(generation, id, incomp, spg);
  if (xtal) {
    const QString HM_spg = Atoms::Geometry::getHMName(spg);
    xtal->setParents(QString("random RandSpg: comp=%1 spg=%2 (%3)")
                       .arg(incomp.getFormula())
                       .arg(spg)
                       .arg(HM_spg));
  }
  return xtal;
}

Xtal* XtalOpt::randSpgXtal(uint generation, uint id, CellComp incomp,
                           uint spg, bool checkSpgWithSpglib)
{
  Atoms::Generators::CrystalGenerationOptions options;
  options.spaceGroup = spg;
  options.atomicNumbers = getStdVecOfAtomsComp(incomp);
  options.aMin = getAMin();
  options.bMin = getBMin();
  options.cMin = getCMin();
  options.alphaMin = getAlphaMin();
  options.betaMin = getBetaMin();
  options.gammaMin = getGammaMin();
  options.aMax = getAMax();
  options.bMax = getBMax();
  options.cMax = getCMax();
  options.alphaMax = getAlphaMax();
  options.betaMax = getBetaMax();
  options.gammaMax = getGammaMax();
  if (getUsingCustomIAD()) {
    // RandSpg needs one radius per element; the full custom table is checked later.
    options.minRadius = 0.0;
    options.iadScalingFactor = 0.0;
    const QList<uint> atomicNums = incomp.getCompositionAtomicNumbers();
    for (const auto& atomicNum : atomicNums) {
      double minDistance = PINF;
      for (const auto& otherAtomicNum : atomicNums) {
        const double distance =
          pairCustomDistances().getPairDistance(atomicNum, otherAtomicNum);
        if (distance == PINF)
          return nullptr;
        minDistance = std::min(minDistance, distance);
      }
      options.atomicRadii[atomicNum] = 0.5 * minDistance;
    }
  } else if (getUsingScaledIAD()) {
    options.minRadius = getMinRadius();
    options.iadScalingFactor = getScaleFactor();
  } else {
    options.minRadius = 0.0;
    options.iadScalingFactor = 0.0;
  }
  getCompositionVolumeLimits(incomp, options.minVolume, options.maxVolume);
  options.maxAttempts = 10;
  options.verbosity = 'n';
  // This removes the guarantee that we will generate the right
  //   space group, but we will just check it with spglib when requested.
  options.forceMostGeneralWyckoffPosition = false;
  options.verifyWithSpglib = checkSpgWithSpglib;
  options.spglibTolerance = getTolSpg();
  options.generationAttempts = 3;

  std::unique_ptr<Atoms::Geometry> generated = Atoms::Generators::generateRandSpg(options);
  Xtal* xtal = nullptr;
  if (generated) {
    xtal = new Xtal;
    xtal->setCellInfo(generated->unitCell().cellMatrix());
    for (size_t i = 0; i < generated->numAtoms(); ++i) {
      Atoms::Atom& atom = xtal->addAtom();
      atom = generated->atom(i);
    }
  }

  // We need to set these things before checkXtal() is called
  if (xtal) {
    xtal->setStatus(Xtal::WaitingForOptimization);
  } else {
    if (isVerbose()) {
      Common::message(QString("   failed to generate spg %1 for xtal %2 after %3 attempts")
                      .arg(spg).arg(incomp.getFormula()).arg(options.maxAttempts));
    }
    return nullptr;
  }

  QString HM_spg = Atoms::Geometry::getHMName(spg);

  // Set up xtal data
  xtal->setGeneration(generation);
  xtal->setIDNumber(id);
  xtal->setParents(QString("forced RandSpg: comp=%1 spg=%2 (%3)")
                       .arg(incomp.getFormula()).arg(spg).arg(HM_spg));
  return xtal;
}

Xtal* XtalOpt::generateRandomMolUnitXtal(uint generation, uint id, CellComp incomp)
{
  if (incomp.getNumTypes() == 0) {
    if (compList().isEmpty())
      return nullptr;
    incomp = pickRandomCompositionFromPossibleOnes();
  }

  if (incomp.getNumAtoms() > getMaxAtoms())
    return nullptr;

  CellComp remaining = incomp;
  std::vector<const Atoms::Geometry*> selectedMolecules;
  QStringList selectedMoleculeLabels;
  // moleculeUnitInputs() has the same order as moleculeUnits(): both are added to
  //   at the same time in processInputMoleculeUnit() and cleared together in clearMoleculeUnits().
  Q_ASSERT(moleculeUnitInputs().size() ==
           static_cast<int>(moleculeUnits().size()));
  std::vector<size_t> moleculeOrder;
  moleculeOrder.reserve(moleculeUnits().size());
  for (size_t i = 0; i < moleculeUnits().size(); ++i)
    moleculeOrder.push_back(i);
  std::stable_sort(moleculeOrder.begin(), moleculeOrder.end(),
                   [this](size_t lhs, size_t rhs) {
                     return moleculeUnits()[lhs].atoms().size() >
                            moleculeUnits()[rhs].atoms().size();
                   });

  for (const auto& moleculeIndex : moleculeOrder) {
    const Atoms::Geometry& molecule = moleculeUnits()[moleculeIndex];

    QHash<unsigned int, unsigned int> moleculeCounts;
    for (const auto& atom : molecule.atoms())
      ++moleculeCounts[atom.atomicNumber()];

    bool fits = true;
    for (auto it = moleculeCounts.constBegin(), itEnd = moleculeCounts.constEnd();
         it != itEnd; ++it) {
      if (remaining.getCount(it.key()) < it.value()) {
        fits = false;
        break;
      }
    }
    if (!fits)
      continue;

    selectedMolecules.push_back(&molecule);
    // Label is formula_pointgroup_template; the template name is the last field
    //   of the corresponding "<formula> <template>" molUnit input entry.
    const QStringList inputFields = moleculeUnitInputs()
        .value(static_cast<int>(moleculeIndex)).split(' ', QtCompat::SkipEmptyParts);
    QString label = molecule.getChemicalFormula() + "_" + molecule.getPointGroupSymbol();
    if (!inputFields.isEmpty())
      label += "_" + inputFields.last();
    selectedMoleculeLabels.append(label);
    for (auto it = moleculeCounts.constBegin(), itEnd = moleculeCounts.constEnd();
         it != itEnd; ++it) {
      const unsigned int atomicNum = it.key();
      const unsigned int count = remaining.getCount(atomicNum);
      remaining.setCompositionEntry(QString::fromStdString(
          Atoms::ElementInfo::getAtomicSymbol(atomicNum)), atomicNum, count - it.value());
    }
  }

  if (selectedMolecules.empty()) {
    Common::debug(QString("%1: no molecule unit fits target "
                          "composition %2.")
                    .arg(__func__)
                    .arg(incomp.getFormula()));
    return nullptr;
  }

  Xtal* xtal = generateEmptyXtalWithLattice(incomp);
  if (!xtal)
    return nullptr;

  QWriteLocker locker(&xtal->lock());
  xtal->setStatus(Xtal::Empty);

  QStringList placedMoleculeLabels;
  QList<double> usedMoleculeScales;
  std::vector<MoleculeUnitBody> placedUnits;

  // Size each molUnit for the current IADs, then place it as a rigid body. A
  //   unit that cannot fit the cell (or clashes with what is already placed) is
  //   returned to the residual pool, so its atoms are added individually below
  //   and the target composition is still met. If not a single unit can be
  //   placed, this is treated as a failed molUnit attempt.
  for (int i = 0; i < selectedMoleculeLabels.size(); ++i) {
    const Atoms::Geometry* molecule = selectedMolecules.at(i);
    double moleculeScale = 1.0;
    if (!moleculeUnitPlacementScale(*molecule, eleScaledRadii(), pairCustomDistances(), getUsingScaledIAD(),
                                    getUsingCustomIAD(), &moleculeScale)) {
      locker.unlock();
      delete xtal;
      Common::debug(QString("%1: failed to prepare molecule unit.")
                      .arg(__func__));
      return nullptr;
    }

    if (addMoleculeRandomly(*xtal, *molecule, eleScaledRadii(), pairCustomDistances(),
                            getUsingScaledIAD(), getUsingCustomIAD(), moleculeScale, 1000,
                            placedUnits)) {
      placedMoleculeLabels.append(selectedMoleculeLabels.at(i));
      usedMoleculeScales.append(moleculeScale);
    } else {
      // No room for this unit; return its atoms to the residual pool.
      QHash<unsigned int, unsigned int> moleculeCounts;
      for (const auto& atom : molecule->atoms())
        ++moleculeCounts[atom.atomicNumber()];
      for (auto it = moleculeCounts.constBegin(), itEnd = moleculeCounts.constEnd();
           it != itEnd; ++it) {
        const unsigned int atomicNum = it.key();
        remaining.setCompositionEntry(
          QString::fromStdString(Atoms::ElementInfo::getAtomicSymbol(atomicNum)),
          atomicNum, remaining.getCount(atomicNum) + it.value());
      }
    }
  }

  if (placedMoleculeLabels.isEmpty()) {
    locker.unlock();
    delete xtal;
    Common::debug(QString("%1: no molecule unit could be placed for "
                          "composition %2.")
                    .arg(__func__)
                    .arg(incomp.getFormula()));
    return nullptr;
  }

  QList<uint> atoms = sortedResidualAtoms(remaining);
  if (getUsingCustomIAD()) {
    std::stable_sort(atoms.begin(), atoms.end(),
                     [this](uint lhs, uint rhs) {
                       return pairCustomDistances().getMaxDistanceForAtom(lhs) >
                              pairCustomDistances().getMaxDistanceForAtom(rhs);
                     });
  } else if (getUsingScaledIAD()) {
    std::stable_sort(atoms.begin(), atoms.end(),
                     [this](uint lhs, uint rhs) {
                       return eleScaledRadii().getMinRadius(lhs) >
                              eleScaledRadii().getMinRadius(rhs);
                     });
  }
  for (const auto& atomicNum : atoms) {
    bool ok = false;
    if (getUsingCustomIAD())
      ok = xtal->addAtomRandomlyCustomIAD(atomicNum, pairCustomDistances(), 1000);
    else if (getUsingScaledIAD())
      ok = xtal->addAtomRandomlyScaledIAD(atomicNum, eleScaledRadii(), 1000);
    else
      ok = xtal->addAtomRandomly(atomicNum, -1.0, 1000);
    if (!ok) {
      locker.unlock();
      delete xtal;
      Common::debug(QString("%1: failed to add residual atom.")
                      .arg(__func__));
      return nullptr;
    }
  }

  xtal->setGeneration(generation);
  xtal->setIDNumber(id);
  xtal->setParents(tr("random molUnit: comp=%1 units=%2")
                     .arg(incomp.getFormula())
                     .arg(placedMoleculeLabels.join(",")));
  xtal->setStatus(Xtal::WaitingForOptimization);
  if (isVerbose()) {
    for (int i = 0; i < usedMoleculeScales.size(); ++i) {
      Common::message(QString("   molUnit used for %1: %2 scale=%3")
                              .arg(xtal->getTag())
                              .arg(placedMoleculeLabels.at(i))
                              .arg(usedMoleculeScales.at(i), 0, 'g', 8));
    }
  }
  return xtal;
}

Xtal* XtalOpt::generateEmptyXtalWithLattice(CellComp incomp)
{
  Xtal* xtal = nullptr;

  // There might be cases that lattice angles will not produce
  //   a proper cell; so we limit the number of attempts here
  //   to avoid infinite loops.
  const int maxAttempts = 100000;
  int attemptCount = 0;
  while (attemptCount < maxAttempts) {
    ++attemptCount;
    double a = Common::getRandDouble() * (getAMax() - getAMin()) + getAMin();
    double b = Common::getRandDouble() * (getBMax() - getBMin()) + getBMin();
    double c = Common::getRandDouble() * (getCMax() - getCMin()) + getCMin();
    double alpha = Common::getRandDouble() * (getAlphaMax() - getAlphaMin()) + getAlphaMin();
    double beta = Common::getRandDouble() * (getBetaMax() - getBetaMin()) + getBetaMin();
    double gamma = Common::getRandDouble() * (getGammaMax() - getGammaMin()) + getGammaMin();
    xtal = new Xtal(a, b, c, alpha, beta, gamma);
    if (checkLattice(xtal))
      break;
    delete xtal;
    xtal = nullptr;
  }

  if (!xtal) {
    Common::debug(QString("%1: %2 attempts failed to generate empty cell for composition %3")
                  .arg(__func__).arg(attemptCount).arg(incomp.getFormula()));
    return nullptr;
  }

  // checkLattice won't set volume for an empty lattice; so we
  //   set it here.
  double minvol, maxvol;
  getCompositionVolumeLimits(incomp, minvol, maxvol);
  xtal->setVolume(Common::getRandDouble(minvol, maxvol));

  return xtal;
}

Xtal* XtalOpt::generateRandomAtomicXtal(uint generation, uint id, CellComp incomp)
{
  if (incomp.getNumTypes() == 0) {
    if (compList().isEmpty())
      return nullptr;
    incomp = pickRandomCompositionFromPossibleOnes();
  }

  Atoms::Generators::CrystalGenerationOptions options;
  options.atomicNumbers = getStdVecOfAtomsComp(incomp);
  options.aMin = getAMin();
  options.bMin = getBMin();
  options.cMin = getCMin();
  options.alphaMin = getAlphaMin();
  options.betaMin = getBetaMin();
  options.gammaMin = getGammaMin();
  options.aMax = getAMax();
  options.bMax = getBMax();
  options.cMax = getCMax();
  options.alphaMax = getAlphaMax();
  options.betaMax = getBetaMax();
  options.gammaMax = getGammaMax();
  getCompositionVolumeLimits(incomp, options.minVolume, options.maxVolume);
  options.maxLatticeAttempts = 100000;
  options.maxAtomPlacementAttempts = 1000;

  // Parameters with nearly equal bounds are fixed; free cells are normalized
  //   before acceptance.
  const bool aFixed = Common::eq(getAMin(), getAMax(), LATT_LEN_COMP_TOL);
  const bool bFixed = Common::eq(getBMin(), getBMax(), LATT_LEN_COMP_TOL);
  const bool cFixed = Common::eq(getCMin(), getCMax(), LATT_LEN_COMP_TOL);
  const bool alphaFixed = Common::eq(getAlphaMin(), getAlphaMax(), LATT_ANG_COMP_TOL);
  const bool betaFixed = Common::eq(getBetaMin(), getBetaMax(), LATT_ANG_COMP_TOL);
  const bool gammaFixed = Common::eq(getGammaMin(), getGammaMax(), LATT_ANG_COMP_TOL);
  if (aFixed)
    options.aMax = options.aMin;
  if (bFixed)
    options.bMax = options.bMin;
  if (cFixed)
    options.cMax = options.cMin;
  if (alphaFixed)
    options.alphaMax = options.alphaMin;
  if (betaFixed)
    options.betaMax = options.betaMin;
  if (gammaFixed)
    options.gammaMax = options.gammaMin;
  options.reduceCell = !(aFixed || bFixed || cFixed || alphaFixed || betaFixed || gammaFixed);

  if (getUsingCustomIAD()) {
    // In custom mode the custom table is the only distance check.
    std::stable_sort(options.atomicNumbers.begin(), options.atomicNumbers.end(),
                     [this](unsigned int lhs, unsigned int rhs) {
      return pairCustomDistances().getMaxDistanceForAtom(lhs) >
             pairCustomDistances().getMaxDistanceForAtom(rhs);
    });
    for (const auto& pair : pairCustomDistances().getPairs()) {
      const double distance = pairCustomDistances().getPairDistance(pair.first, pair.second);
      options.pairMinDistances[std::make_pair(pair.first, pair.second)] = distance;
      if (pair.first != pair.second)
        options.pairMinDistances[std::make_pair(pair.second, pair.first)] = distance;
    }
  } else if (getUsingScaledIAD()) {
    for (const auto& atomicNumber : eleScaledRadii().getRadiiAtomicNumbers())
      options.atomicRadii[atomicNumber] = eleScaledRadii().getMinRadius(atomicNumber);
  }

  std::unique_ptr<Atoms::Geometry> generated = Atoms::Generators::generateRandom(options);
  Xtal* xtal = nullptr;
  if (generated) {
    xtal = new Xtal;
    xtal->setCellInfo(generated->unitCell().cellMatrix());
    for (size_t i = 0; i < generated->numAtoms(); ++i) {
      Atoms::Atom& atom = xtal->addAtom();
      atom = generated->atom(i);
    }
  }

  if (!xtal)
    return nullptr;

  xtal->setGeneration(generation);
  xtal->setIDNumber(id);
  xtal->setParents(tr("random atomic: comp=%1").arg(incomp.getFormula()));
  xtal->setStatus(Xtal::WaitingForOptimization);
  return xtal;
}

Xtal* XtalOpt::generateEvolvedXtal(Xtal* preselectedXtal)
{
  // preselectedXtal is nullptr by default

  // Initialize loop vars
  unsigned int gen = 0;
  QString parents;
  Xtal *xtal = nullptr, *selectedXtal = nullptr;

  // Shouldn't happen; but just in case ...
  if (!preselectedXtal && getParentPoolSize() == 0) {
    Common::warning(QString("%1: empty pool and no preselected xtal.")
                     .arg(__func__));
    return nullptr;
  }

  // Also, here we determine the chances of generating a random supercell.
  double wSupr;
  if (getPSupercell() > 100) {
    wSupr = 1.0;
  } else {
    wSupr = static_cast<double>(getPSupercell()) / 100.0;
  }

  // Perform operation until xtal is valid:
  int maxAttempts = 10000;
  int totalAttempts = 0;

  // Inner loop already does a check on xtal; this one is currently
  //   a defensive second check!
  while (!checkXtal(xtal)) {
    // First delete any previous failed structure in xtal
    if (xtal) {
      delete xtal;
      xtal = nullptr;
    }

    if (totalAttempts >= maxAttempts) {
      Common::warning(QString("%1: failed too many times. Giving up.")
                       .arg(__func__));
      return nullptr;
    }
    ++totalAttempts;

    // If an xtal hasn't been preselected, select one
    if (!preselectedXtal)
      selectedXtal = selectXtalFromProbabilityList();
    else
      selectedXtal = preselectedXtal;

    // Specially, the probability selection might fail and we get null pointer
    if (!selectedXtal) {
      Common::warning(QString("%1: selecting xtal failed.").arg(__func__));
      return nullptr;
    }

    // Decide operator
    // As of XtalOpt v14, we read "operation weights" instead of their percentages,
    //   and decide the chance of applying operators at the time of selection; based
    //   on the run condition (if variable-comp or if parent has invalid composition).
    // The following takes into account these conditions, and returns a randomly
    //   selected operator based on the user-specified relative weights.
    Operators op = selectOperation(selectedXtal->hasValidComposition());

    // Operator label for the log messages below.
    static const struct
    {
      Operators op;
      const char* label;
    } opLabels[] = {
      { OP_Crossover,   "crossover" },
      { OP_Stripple,    "stripple" },
      { OP_Permustrain, "permustrain" },
      { OP_Permutomic,  "permutomic" },
      { OP_Permucomp,   "permucomp" },
    };
    QString opStr = "(unknown)";
    for (const auto& entry : opLabels) {
      if (entry.op == op) {
        opStr = entry.label;
        break;
      }
    }
    if (isVerbose())
      Common::message(QString("   operator selected %1 for parent %2")
              .arg(opStr).arg(selectedXtal->getTag()));

    // Try 1000 times to get a good structure from the selected
    // operation. If not possible, send a warning to the log and
    // start anew.
    int operatorAttempts = 0;
    int maxOperatorAttempts = 1000;
    while (operatorAttempts < maxOperatorAttempts && !checkXtal(xtal)) {
      if (xtal) {
        delete xtal;
        xtal = nullptr;
      }

      ++operatorAttempts;

      // Operation specific set up:
      switch (op) {
        case OP_Crossover: {
          Xtal *xtal1 = nullptr, *xtal2 = nullptr;
          // Select structures
          double percent1 = 0.0, percent2 = 0.0;

          xtal1 = selectedXtal;
          xtal2 = selectXtalFromProbabilityList();

          // The probability selection might fail and we get null pointer
          if (!xtal2) {
            Common::warning(QString("%1: selecting crossover xtal failed.")
                             .arg(__func__));
            return nullptr;
          }

          // Perform operation
          const uint crossoverCuts = getCrossNcuts();
          xtal = XtalOptGenetic::crossover(xtal1, xtal2, this->compList(), this->eleScaledRadii(),
                                           crossoverCuts, getCrossMinimumContribution(),
                                           percent1, percent2,
                                           getMinAtoms(), getMaxAtoms(), getVcSearch(), isVerbose(),
                                           getUsingScaledIAD(), getUsingCustomIAD(),
                                           &this->pairCustomDistances());

          // Lock parents and get info from them
          uint gen1, gen2, id1, id2;
          {
            // Lock the lower-address structure first, so two threads locking
            //   the same pair always agree on the order (avoids a deadlock).
            QReadLocker locker1(&(xtal1 < xtal2 ? xtal1 : xtal2)->lock());
            QReadLocker locker2(&(xtal1 < xtal2 ? xtal2 : xtal1)->lock());
            gen1 = xtal1->getGeneration();
            gen2 = xtal2->getGeneration();
            id1 = xtal1->getIDNumber();
            id2 = xtal2->getIDNumber();
          }

          // We will set the parent xtal of this xtal to be
          // the parent that contributed the most
          if (xtal) {
            if (percent1 >= 50.0)
              xtal->setParentStructure(xtal1);
            else
              xtal->setParentStructure(xtal2);
          }

          // Determine generation number
          gen = (gen1 >= gen2) ? gen1 + 1 : gen2 + 1;

          parents = QString("Crossover: %1x%2 (%3%) + %4x%5 (%6%) cuts=%7")
                      .arg(gen1)
                      .arg(id1)
                      .arg(percent1, 0, 'f', 0)
                      .arg(gen2)
                      .arg(id2)
                      .arg(percent2, 0, 'f', 0)
                      .arg(crossoverCuts);
          continue;
        }
        case OP_Stripple: {
          // Perform stripple
          double amplitude = 0, stdev = 0;
          xtal = XtalOptGenetic::stripple(selectedXtal, getStripStrainStdevMin(),
                                          getStripStrainStdevMax(), getStripAmpMin(),
                                          getStripAmpMax(), getStripPer1(), getStripPer2(),
                                          stdev, amplitude);

          // Lock parent and extract info
          uint gen1, id1;
          {
            QReadLocker locker(&selectedXtal->lock());
            gen1 = selectedXtal->getGeneration();
            id1 = selectedXtal->getIDNumber();
            if (xtal)
              xtal->setParentStructure(selectedXtal);
          }

          // Determine generation number
          gen = gen1 + 1;
          // A regular mutation is being performed
          parents = QString("Stripple: %1x%2 stdev=%3 amp=%4 waves=%5,%6")
            .arg(gen1)
            .arg(id1)
            .arg(stdev, 0, 'f', 5)
            .arg(amplitude, 0, 'f', 5)
            .arg(getStripPer1())
            .arg(getStripPer2());
          continue;
        }
        case OP_Permustrain: {
          double stdev = 0;

          xtal = XtalOptGenetic::permustrain(selectedXtal, getPermStrainStdevMax(),
                                             getPermEx(), stdev);

          // Lock parent and extract info
          uint gen1, id1;
          {
            QReadLocker locker(&selectedXtal->lock());
            gen1 = selectedXtal->getGeneration();
            id1 = selectedXtal->getIDNumber();
            if (xtal)
              xtal->setParentStructure(selectedXtal);
          }

          // Determine generation number
          gen = gen1 + 1;
          // Set the ancestry like normal...
          parents = QString("Permustrain: %1x%2 stdev=%3 exch=%4")
            .arg(gen1)
            .arg(id1)
            .arg(stdev, 0, 'f', 5)
            .arg(getPermEx());
          continue;
        }
        case OP_Permutomic: {
          xtal = XtalOptGenetic::permutomic(selectedXtal, this->compList()[0], this->eleScaledRadii(),
                                            getMinAtoms(), getMaxAtoms(), isVerbose(),
                                            getUsingScaledIAD(), getUsingCustomIAD(),
                                            &this->pairCustomDistances());
          if (!xtal)
            continue;

          // Lock parent and extract info
          uint gen1, id1;
          QString parentComposition;
          {
            QReadLocker locker(&selectedXtal->lock());
            gen1 = selectedXtal->getGeneration();
            id1 = selectedXtal->getIDNumber();
            parentComposition = selectedXtal->getCompositionString(false);
            xtal->setParentStructure(selectedXtal);
          }

          // Determine generation number
          gen = gen1 + 1;
          // Set the ancestry like normal...
          parents = QString("Permutomic: %1x%2 (%3-%4)")
            .arg(gen1)
            .arg(id1)
            .arg(parentComposition)
            .arg(xtal->getCompositionString(false));
          continue;
        }
        case OP_Permucomp: {
          xtal = XtalOptGenetic::permucomp(selectedXtal, this->compList()[0], this->eleScaledRadii(),
                                           getMinAtoms(), getMaxAtoms(), isVerbose(),
                                           getUsingScaledIAD(), getUsingCustomIAD(),
                                           &this->pairCustomDistances());
          if (!xtal)
            continue;

          // Lock parent and extract info
          uint gen1, id1;
          QString parentComposition;
          {
            QReadLocker locker(&selectedXtal->lock());
            gen1 = selectedXtal->getGeneration();
            id1 = selectedXtal->getIDNumber();
            parentComposition = selectedXtal->getCompositionString(false);
            xtal->setParentStructure(selectedXtal);
          }

          // Determine generation number
          gen = gen1 + 1;
          // Set the ancestry like normal...
          parents = QString("Permucomp: %1x%2 (%3-%4)")
            .arg(gen1)
            .arg(id1)
            .arg(parentComposition)
            .arg(xtal->getCompositionString(false));
          continue;
        }
        default:
          Common::warning(QString("%1: attempt to use an invalid operator.")
                           .arg(__func__));
      }
    }
    if (operatorAttempts >= maxOperatorAttempts) {
      Common::warning(tr("Unable to perform operation %1 after 1000 tries. "
                 "Reselecting operator...")
                .arg(opStr));
    }
  }

  xtal->setGeneration(gen);
  xtal->setParents(parents);
  Xtal* parentXtal = qobject_cast<Xtal*>(xtal->getParentStructure());
  xtal->setParentStructure(parentXtal);

  // This is not a genetic operation, per se. By a user-defined chance (0-100)
  //   of "p_supercell", we try to generate a supercell with a randomly chosen
  //   expansion factor, with up to maximum number of atoms.
  // If this worked, we return the supercell. Otherwise, we just return the
  //   original generated cell.
  // We have already converted user input to a chance in [0,1] range in "wSupr".
  double s = Common::getRandDouble();
  if (s < wSupr) {
    Xtal* supercellXtal = generateSuperCell(xtal, 0, true);
    if (supercellXtal) {
      delete xtal;
      return supercellXtal;
    }
  }

  return xtal;
}

Xtal* XtalOpt::selectXtalFromProbabilityList()
{
  // Basically, this function is called only from generateEvolvedXtal
  //   (twice!). The engine selects the parent from its in-memory pool table:
  //   structures that are optimized and have their hull and objectives
  //   (if needed) calculated, and are not similar to another one.

  if (getParentsPoolSize() == 0) {
    Common::error("Parents pool size is zero for probability selection!");
    return nullptr;
  }

  // Select the parent xtal (nullptr if anything goes wrong).
  Structure* structure = selectParentStructure(getParentsPoolSize());

  // This shouldn't happen; but just in case ...
  if (!structure) {
    Common::error("Probability selection didn't return any results!");
    return nullptr;
  }

  return qobject_cast<Xtal*>(structure);
}

XtalOpt::Operators XtalOpt::selectOperation(bool validComp)
{
  // In this function we start by relative operation weights given
  //   by the user, and will try to find an appropriate genetic operation.
  //
  // Our general considerations are applied in the following order:
  // (1) non-vcSearch: we exclude permutomic and permucomp
  // (2) non-valid parent composition: we exclude stripple and permustrain
  //
  // Here, we:
  //   - decide about the fallback option,
  //   - create a list of operators, input weights (and set any negative
  //     weights to zero), and the above conditions,
  //   - combine the conditions to figure out what operations are allowed,
  //   - normalize the weights to 1.0 for allowed operations while zeroing the rest,
  //   - and finally choose a random number and select/return operator accordingly.

  // In case of any failure, we return crossover since it's always applicable.
  Operators fallback = OP_Crossover;

  // Build the list of operators. Each entry keeps the operator, its weight,
  //   and the two conditions together.
  // IMPORTANT: the verbose output and the selection below use this order.
  struct OpDescriptor
  {
    Operators op;
    double weight;
    bool allowSearch;
    bool allowValid;
  };
  std::vector<OpDescriptor> ops = {
    { OP_Stripple,    static_cast<double>(getPStrip()),  true,            validComp },
    { OP_Permustrain, static_cast<double>(getPPerm()),   true,            validComp },
    { OP_Permutomic,  static_cast<double>(getPAtomic()), getVcSearch(), true      },
    { OP_Permucomp,   static_cast<double>(getPComp()),   getVcSearch(), true      },
    { OP_Crossover,   static_cast<double>(getPCross()),  true,            true      }
  };

  int ops_num = ops.size();

  // Now apply conditions, find allowed ops, and zero out weights for the rest
  std::vector<bool> allowed(ops_num, false);
  int num_allowed = 0;
  for (int i = 0; i < ops_num; ++i) {
    allowed[i] = ops[i].allowSearch && ops[i].allowValid;
    if (allowed[i])
      num_allowed++;
    else
      ops[i].weight = 0.0;
  }

  // Sanity check: are we left with any allowed operations?
  if (num_allowed == 0) {
    Common::warning("Unexpected operation weights. Selecting crossover.");
    return fallback;
  }

  // Find total weight of allowed ops: since we have set the weight for
  //   the rest to zero, a simple sum is fine.
  double total = 0.0;
  for (int i = 0; i < ops_num; i++)
    total += ops[i].weight;

  // Normalize the weights for allowed ops, while leaving the rest zero.
  // If total weight of allowed ops is zero, make them normalized-equal;
  //   otherwise normalize their weight using the total.
  for (int i = 0; i < ops_num; i++) {
    if (allowed[i])
      ops[i].weight = (total > 0.0) ? ops[i].weight/total : 1.0/num_allowed;
  }

  if (isVerbose()) {
    Common::message(QString("   operation chances: stri %1 perm %2 atom %3 comp %4 cros %5")
            .arg(ops[0].weight, 5, 'f', 2)
            .arg(ops[1].weight, 5, 'f', 2)
            .arg(ops[2].weight, 5, 'f', 2)
            .arg(ops[3].weight, 5, 'f', 2)
            .arg(ops[4].weight, 5, 'f', 2));
  }

  // Perform the selection
  Operators op = fallback;

  double r = Common::getRandDouble();

  double cumr = 0.0;
  for (int i = 0; i < ops_num; i++) {
    cumr += ops[i].weight;
    if (r < cumr) {
      op = ops[i].op;
      break;
    }
  }

  // Return the selected operation
  return op;
}

// This always returns a dynamically allocated xtal
// Callers take ownership of the pointer
Xtal* XtalOpt::generateSuperCell(Xtal* inXtal, uint expansion, bool distort)
{
  // This function will generate a supercell out of the input cell.
  // It can be called in two modes:
  //   (1) expansion > 0: make unit cell with that many times the atoms,
  //   (2) expansion = 0: randomly choose a factor, and generate a supercell.
  //
  // If "distort" is true; randomly distorts an atom in the final supercell.
  //
  // If expansion is given, we check to see if it complies with max atoms.
  // If we pick this factor, we make sure it is compatible with max atoms.

  // A basic sanity check
  if (!inXtal)
    return nullptr;

  uint initNumAtoms = inXtal->numAtoms();
  uint finalExpansion = expansion;

  if (finalExpansion > 0) {
    // If pre-defined factor, make sure it's good.
    if (static_cast<double>(finalExpansion) * initNumAtoms > getMaxAtoms())
      return nullptr;
  } else {
    // If not, try to find a proper one randomly.
    int maxPossibleExpansion = std::floor(getMaxAtoms() / initNumAtoms);
    if (maxPossibleExpansion < 2)
      return nullptr;
    finalExpansion = Common::getRandInt(2, maxPossibleExpansion);
  }

  // This is the return xtal
  Xtal* xtal = new Xtal;

  // Lock the parent xtal for reading
  QReadLocker parentXtalLocker(&inXtal->lock());

  // Copy info over from input to new xtal
  xtal->setCellInfo(inXtal->unitCell().cellMatrix());
  const std::vector<Atoms::Atom>& atoms = inXtal->atoms();
  for (const auto& atom : atoms)
    xtal->addAtom(atom.atomicNumber(), atom.pos());
  uint gen = inXtal->getGeneration();
  QString parents = inXtal->getParents();
  Xtal* parentXtal = qobject_cast<Xtal*>(inXtal->getParentStructure());
  parentXtalLocker.unlock();

  // Keep performing the supercell generator until we are at the correct size.
  // We have already checked that the target cell will comply with the max atoms.

  // The current expansion factor of the generated supercell.
  uint factor = 1;
  while (factor != finalExpansion) {
    // This never happens; just in case!
    if (xtal->numAtoms() % initNumAtoms != 0) {
      delete xtal;
      return nullptr;
    }

    factor = xtal->numAtoms() / initNumAtoms;

    // Find the largest prime number multiple. We will expand
    // upon the shortest length with this number.
    uint remaining = finalExpansion / factor;
    uint divisor = 2;
    while (divisor < remaining) {
      if (remaining % divisor == 0)
        remaining /= divisor;
      else
        ++divisor;
    }
    const uint numberOfDuplicates = remaining;

    // a, b, and c are the number of duplicates in the A, B, and C
    // directions, respectively.
    uint a = 1;
    uint b = 1;
    uint c = 1;

    // Find the shortest length. We will expand upon this length.
    double A = xtal->getA();
    double B = xtal->getB();
    double C = xtal->getC();

    if (A <= B && A <= C)
      a = numberOfDuplicates;
    else if (B <= A && B <= C)
      b = numberOfDuplicates;
    else if (C <= A && C <= B)
      c = numberOfDuplicates;

    // Extract the old vectors
    const Common::Vector3& oldA = xtal->unitCell().aVector();
    const Common::Vector3& oldB = xtal->unitCell().bVector();
    const Common::Vector3& oldC = xtal->unitCell().cVector();

    const std::vector<Atoms::Atom> oldAtoms = xtal->atoms();

    // Add the extra atoms in
    for (int ind_a = 0; ind_a < static_cast<int>(a); ++ind_a) {
      for (int ind_b = 0; ind_b < static_cast<int>(b); ++ind_b) {
        for (int ind_c = 0; ind_c < static_cast<int>(c); ++ind_c) {
          if (ind_a == 0 && ind_b == 0 && ind_c == 0)
            continue;

          Common::Vector3 displacement = ind_a * oldA + ind_b * oldB + ind_c * oldC;
          for (const auto& atom : oldAtoms)
            xtal->addAtom(atom.atomicNumber(), atom.pos() + displacement);
        }
      }
    }

    // Scale the vectors used to place the added atoms, keeping the cell and
    //   atom coordinates in the same orientation.
    xtal->setCellInfo(a * oldA, b * oldB, c * oldC);
    factor = xtal->numAtoms() / initNumAtoms;
  }


  // Distort an atom?
  if (distort) {
    // pick a random atom
    uint ratom = Common::getRandUInt(0, xtal->numAtoms() - 1);
    Atoms::Atom& atom = xtal->atom(ratom);
    int atomicNumber = atom.atomicNumber();
    // try to distort it's position
    if (getUsingCustomIAD()) {
      xtal->moveAtomRandomlyCustomIAD(atomicNumber, this->pairCustomDistances(), 1000, &atom);
    } else if (getUsingScaledIAD()) {
      xtal->moveAtomRandomlyScaledIAD(atomicNumber, this->eleScaledRadii(), 1000, &atom);
    } else {
      atom.setPos(xtal->fracToCart(Common::Vector3(Common::getRandDouble(),
                                                   Common::getRandDouble(),
                                                   Common::getRandDouble())));
      xtal->notifyGeometryChanged();
    }
  }

  // Set the new xtal stuff
  xtal->setGeneration(gen);
  parents=QString("Supercell[%1]-").arg(factor)+parents;
  xtal->setParents(parents);
  xtal->setParentStructure(parentXtal);
  xtal->setStatus(Xtal::WaitingForOptimization);

  return xtal;
}

bool XtalOpt::initializeAndAddXtal(Xtal* xtal, uint generation, const QString& parents)
{
  if (!xtal)
    return false;

  {
    QtCompat::MutexLocker initLock(x_xtalInitMutex.get());
    QWriteLocker xtalLock(&xtal->lock());

    // If none of the cell parameters are fixed, perform a normalization
    //   on the lattice (currently a Niggli reduction)
    if (Common::neq(getAMin(), getAMax(), LATT_LEN_COMP_TOL) &&
        Common::neq(getBMin(), getBMax(), LATT_LEN_COMP_TOL) &&
        Common::neq(getCMin(), getCMax(), LATT_LEN_COMP_TOL) &&
        Common::neq(getAlphaMin(), getAlphaMax(), LATT_ANG_COMP_TOL) &&
        Common::neq(getBetaMin(), getBetaMax(), LATT_ANG_COMP_TOL) &&
        Common::neq(getGammaMin(), getGammaMax(), LATT_ANG_COMP_TOL)) {
      // A cell that can't be reduced is kept as it is; fixAngles reports it.
      xtal->fixAngles();
    }
    xtal->findSpaceGroup(getTolSpg());
  }

  return queue()->addNewStructure(xtal, generation, parents);
}

bool XtalOpt::addSeed(const QString& filename)
{
  Xtal* xtal = new Xtal;
  xtal->setLocpath(filename);
  xtal->setStatus(Xtal::WaitingForOptimization);

  // We will only display the warning once, so use a static bool for this
  // Use an atomic bool for thread safety
  static std::atomic_bool warningAlreadyDisplayed(false);
  if (!warningAlreadyDisplayed.load()) {
    Common::warning("XtalOpt no longer checks seed xtals for user-defined "
            "geometrical constraints.");
    warningAlreadyDisplayed = true;
  }

  // For seed structures, we call check composition with "isSeed = true"
  //   where we perform a basic check and increase max atoms if needed.
  const bool seedLoaded = Atoms::Formats::read(*xtal, filename) ||
    optimizer(0)->read(xtal, filename);
  if (!seedLoaded || !this->checkComposition(xtal, true)) {
    Common::error(tr("Could not load seed %1\n\n").arg(filename));
    delete xtal;
    return false;
  }

  // Seeds must be 3D periodic crystals; a structure read without a unit cell
  //   (e.g. XYZ with no Lattice entry) comes back 0D and is rejected.
  if (!xtal->is3D()) {
    Common::error(tr("Seed %1 is not a 3D periodic structure (no unit cell); "
                     "skipping it.").arg(filename));
    delete xtal;
    return false;
  }

  QString parents = QString("Seeded: %1").arg(filename);
  if (!initializeAndAddXtal(xtal, 1, parents))
    return false;
  Common::message(QString("   generated initial structure: seed from: %1")
                  .arg(filename));
  return true;
}

Structure* XtalOpt::replaceWithRandom(Structure* s, const QString& reason)
{
  Xtal* oldXtal = qobject_cast<Xtal*>(s);
  if (!oldXtal)
    return s;

  // Retrieve the composition of the original cell
  CellComp origComp;
  {
    QReadLocker locker(&oldXtal->lock());
    if (oldXtal->hasValidComposition())
      origComp = getXtalComposition(s);
  }

  uint generation, id;
  generation = s->getGeneration();
  id = s->getIDNumber();
  // Generate the replacement before locking the old xtal; generation can
  //   take a while and the queue thread needs this lock.
  Xtal* xtal = nullptr;
  int maxAttempts = 10000;
  int attemptCount = 0;
  // generateRandomXtal() doesn't perform routine xtal check; so we do it here.
  while (!checkXtal(xtal)) {
    if (xtal) {
      delete xtal;
      xtal = nullptr;
    }
    if (attemptCount >= maxAttempts) {
      Common::warning(QString("%1: failed too many times. Giving up.")
                       .arg(__func__));
      return nullptr;
    }
    ++attemptCount;
    xtal = generateRandomXtal(generation, id, origComp);
  }

  {
    // Copy info over
    QWriteLocker locker1(&oldXtal->lock());
    QWriteLocker locker2(&xtal->lock());
    // Randomly generated xtals do not have parent structures
    oldXtal->setParentStructure(nullptr);
    oldXtal->clear();
    oldXtal->setCellInfo(xtal->unitCell().cellMatrix());
    oldXtal->resetEnergy();
    oldXtal->resetEnthalpy();
    oldXtal->resetStrucObj();
    oldXtal->resetStrucConstraint();
    oldXtal->setCompositionValidity(xtal->hasValidComposition());
    oldXtal->setDistAboveHull(std::numeric_limits<double>::quiet_NaN());
    oldXtal->setPV(0);
    oldXtal->setCurrentOptStep(0);
    QString parents = xtal->getParents();
    if (parents.isEmpty()) {
      parents = QString("Randomly generated replacement (comp=%1)")
                  .arg(origComp.getFormula());
    }
    if (!reason.isEmpty())
      parents += " (" + reason + ")";
    oldXtal->setParents(parents);

    for (uint i = 0; i < xtal->numAtoms(); i++) {
      Atoms::Atom& atom1 = oldXtal->addAtom();
      Atoms::Atom& atom2 = xtal->atom(i);
      atom1.setPos(atom2.pos());
      atom1.setAtomicNumber(atom2.atomicNumber());
    }
    oldXtal->findSpaceGroup(getTolSpg());
    oldXtal->resetFailCount();
  }

  // Delete random xtal
  delete xtal;
  return qobject_cast<Structure*>(oldXtal);
}

Structure* XtalOpt::replaceWithOffspring(Structure* s, const QString& reason)
{
  Xtal* oldXtal = qobject_cast<Xtal*>(s);
  if (!oldXtal)
    return s;

  // Preserve valid compositions. Invalid seed compositions are replaced with
  //   one selected from the run's composition list by generateNewXtal().
  CellComp origComp;
  {
    QReadLocker locker(&oldXtal->lock());
    if (oldXtal->hasValidComposition())
      origComp = getXtalComposition(s);
  }

  // generateNewXtal() owns its retry budget and returns a checked structure.
  // Re-running an exhausted generator here only multiplies that budget.
  Xtal* xtal = generateNewXtal(origComp);
  if (!xtal) {
    Common::warning(QString("%1: could not generate a valid replacement.")
                     .arg(__func__));
    return nullptr;
  }

  // In vcSearch the offspring composition may differ from the old structure;
  //   that's fine, the copy below replaces the cell and all atoms anyway.

  {
    // Copy info over
    QWriteLocker locker1(&oldXtal->lock());
    QWriteLocker locker2(&xtal->lock());
    oldXtal->clear();
    oldXtal->setCellInfo(xtal->unitCell().cellMatrix());
    oldXtal->resetEnergy();
    oldXtal->resetEnthalpy();
    oldXtal->resetStrucObj();
    oldXtal->resetStrucConstraint();
    oldXtal->setCompositionValidity(xtal->hasValidComposition());
    oldXtal->setDistAboveHull(std::numeric_limits<double>::quiet_NaN());
    oldXtal->resetFailCount();
    oldXtal->setPV(0);
    oldXtal->setCurrentOptStep(0);
    QString parents = xtal->getParents();
    if (parents.isEmpty()) {
      parents = QString("Offspring generated replacement (comp=%1)")
                  .arg(origComp.getFormula());
    }
    if (!reason.isEmpty())
      parents += " (" + reason + ")";
    oldXtal->setParents(parents);

    oldXtal->setParentStructure(xtal->getParentStructure());

    for (uint i = 0; i < xtal->numAtoms(); ++i) {
      oldXtal->addAtom(xtal->atom(i));
    }
    oldXtal->findSpaceGroup(getTolSpg());
  }

  // Delete random xtal
  delete xtal;
  return static_cast<Structure*>(oldXtal);
}

bool XtalOpt::checkXtal(Xtal* xtal)
{
  Common::ScopedTimer _timer("XtalOpt::checkXtal");
  QReadLocker runtimeLocker(runtimeSettingsLock());

  // In this function, we always assume that we have a valid input xtal
  if (!xtal) {
    return false;
  }

  // Lock xtal
  QWriteLocker locker(&xtal->lock());

  if (xtal->getStatus() == Xtal::Empty) {
    return false;
  }

  if (!checkLattice(xtal))
    return false;

  if (!checkComposition(xtal))
    return false;

  // Sometimes atom positions come back non-finite for an unknown reason
  for (size_t i = 0; i < xtal->numAtoms(); ++i) {
    const Common::Vector3& pos = xtal->atom(i).pos();
    if (GS_IS_NAN_OR_INF(pos.x()) || GS_IS_NAN_OR_INF(pos.y()) ||
        GS_IS_NAN_OR_INF(pos.z())) {
      Common::debug(QString("Discarding structure %1: contains non-finite atom positions")
                      .arg(xtal->getTag()));
      return false;
    }
  }

  // Never accept the structure if two atoms are basically on top of one
  // another
  for (size_t i = 0; i < xtal->numAtoms(); ++i) {
    for (size_t j = i + 1; j < xtal->numAtoms(); ++j) {
      if (fuzzyCompare(xtal->atom(i).pos(), xtal->atom(j).pos())) {
        Common::debug(QString("Discarding structure %1: two atoms are basically on top "
                      "of one another. This can confuse some optimizers.")
                .arg(xtal->getTag()));
        return false;
      }
    }
  }

  // Check interatomic distances
  if (getUsingScaledIAD()) {
    int atom1, atom2;
    double IAD;
    if (!xtal->checkInterAtomicDistancesScaled(this->eleScaledRadii(), &atom1, &atom2, &IAD)) {
      Atoms::Atom& a1 = xtal->atom(atom1);
      Atoms::Atom& a2 = xtal->atom(atom2);
      const double minIAD = this->eleScaledRadii().getMinRadius(a1.atomicNumber()) +
                            this->eleScaledRadii().getMinRadius(a2.atomicNumber());

      Common::debug(QString("Discarding structure %1: bad IAD (%2 < %3)")
              .arg(xtal->getTag())
              .arg(IAD)
              .arg(minIAD));
      return false;
    }
  } else if (getUsingCustomIAD()) {
    int atom1, atom2;
    double IAD;
    if (!xtal->checkInterAtomicDistancesCustom(this->pairCustomDistances(), &atom1, &atom2, &IAD)) {
      Atoms::Atom& a1 = xtal->atom(atom1);
      Atoms::Atom& a2 = xtal->atom(atom2);
      const double minIAD =
        pairCustomDistances().getPairDistance(a1.atomicNumber(), a2.atomicNumber());
      Common::debug(QString("Discarding structure %1: bad custom IAD (%2 < %3)")
              .arg(xtal->getTag())
              .arg(IAD)
              .arg(minIAD));
      return false;
    }
  }

  // Xtal is OK!
  return true;
}

bool XtalOpt::checkLattice(Xtal* xtal)
{
  double minvol = -1.0;
  double maxvol = -1.0;
  if (xtal->numAtoms() > 0) {
    // Start with volume check, which is done only for non-empty xtals.
    //   Empty lattices have their volume adjusted separately.
    CellComp xtalComp = getXtalComposition(xtal);
    // The current (original) volume
    double org_vol = xtal->getVolume();

    // First, find volume limits for the structure
    getCompositionVolumeLimits(xtalComp, minvol, maxvol);

    // Check volume
    if (org_vol < minvol || org_vol > maxvol) { // PSA
      double adjvol = Common::getRandDouble(minvol, maxvol);
      // If the user has set vol_min to 0, we can end up with a null
      // volume. Fix this here. This is just to keep things stable
      // numerically during the rescaling -- it's unlikely that other
      // cells with small, nonzero volumes will pass the other checks
      // so long as other limits are reasonable.
      if (fabs(adjvol) < 1.0) {
        adjvol = (maxvol - minvol) * 0.5 + minvol; // PSA;
      }
      xtal->setVolume(adjvol);
    }

    if (isVerbose()) {
      double new_vol = xtal->getVolume();
      if (Common::neq(org_vol, new_vol, ZERO02))
        Common::message(QString("   volume fixed - ori %1   new %2   (%3 - %4) %5")
                .arg(org_vol,9,'f',2).arg(new_vol,9,'f',2)
                .arg(minvol,9,'f',2).arg(maxvol,9,'f',2)
                .arg(xtal->getCompositionString()));
    }
  }

  // Scale to any fixed parameters
  double a, b, c, alpha, beta, gamma;
  a = b = c = alpha = beta = gamma = 0;
  if (Common::eq(getAMin(), getAMax(), LATT_LEN_COMP_TOL))
    a = getAMin();
  if (Common::eq(getBMin(), getBMax(), LATT_LEN_COMP_TOL))
    b = getBMin();
  if (Common::eq(getCMin(), getCMax(), LATT_LEN_COMP_TOL))
    c = getCMin();
  if (Common::eq(getAlphaMin(), getAlphaMax(), LATT_ANG_COMP_TOL))
    alpha = getAlphaMin();
  if (Common::eq(getBetaMin(), getBetaMax(), LATT_ANG_COMP_TOL))
    beta = getBetaMin();
  if (Common::eq(getGammaMin(), getGammaMax(), LATT_ANG_COMP_TOL))
    gamma = getGammaMin();
  xtal->rescaleCell(a, b, c, alpha, beta, gamma);

  // Make sure that the current cell parameters are realistic. This is
  // needed here to guard the possible call to fixAngles (hence niggliReduce).
  if (GS_IS_NAN_OR_INF(xtal->getA()) || fabs(xtal->getA()) < ZERO08 ||
      GS_IS_NAN_OR_INF(xtal->getB()) || fabs(xtal->getB()) < ZERO08 ||
      GS_IS_NAN_OR_INF(xtal->getC()) || fabs(xtal->getC()) < ZERO08 ||
      GS_IS_NAN_OR_INF(xtal->getAlpha()) || fabs(xtal->getAlpha()) < ZERO08 ||
      GS_IS_NAN_OR_INF(xtal->getBeta()) || fabs(xtal->getBeta()) < ZERO08 ||
      GS_IS_NAN_OR_INF(xtal->getGamma()) || fabs(xtal->getGamma()) < ZERO08) {
    Common::debug(QString("Discarding structure %1: a cell parameter is either 0, nan, or inf.")
            .arg(xtal->getTag()));

    return false;
  }

  // If no cell parameters are fixed, normalize lattice
  if (a == 0.0 && b == 0.0 && c == 0.0 &&
      alpha == 0.0 && beta == 0.0 && gamma == 0.0) {
    // If one length is 25x shorter than another, it can sometimes
    // cause the spglib to crash in this function
    // If one is 25x shorter than another, discard it
    double cutoff = 25.0;
    if (xtal->getA() * cutoff < xtal->getB() ||
        xtal->getA() * cutoff < xtal->getC() ||
        xtal->getB() * cutoff < xtal->getA() ||
        xtal->getB() * cutoff < xtal->getC() ||
        xtal->getC() * cutoff < xtal->getA() ||
        xtal->getC() * cutoff < xtal->getB()) {
      Common::debug(QString("Discarding structure %1: ratio of two lengths is 25x or larger "
                    "(%2 %3 %4)")
              .arg(xtal->getTag())
              .arg(xtal->getA())
              .arg(xtal->getB())
              .arg(xtal->getC()));

      return false;
    }
    // Check that the angles aren't 25x different than the others as well
    if (xtal->getAlpha() * cutoff < xtal->getBeta() ||
        xtal->getAlpha() * cutoff < xtal->getGamma() ||
        xtal->getBeta() * cutoff < xtal->getAlpha() ||
        xtal->getBeta() * cutoff < xtal->getGamma() ||
        xtal->getGamma() * cutoff < xtal->getAlpha() ||
        xtal->getGamma() * cutoff < xtal->getBeta()) {
      Common::debug(QString("Discarding structure %1: ratio of two angles is 25x or larger "
                    "(%2 %3 %4)")
              .arg(xtal->getTag())
              .arg(xtal->getAlpha())
              .arg(xtal->getBeta())
              .arg(xtal->getGamma()));

      return false;
    }

    // Fix the angles
    if (!xtal->fixAngles())
      return false;
  }

  // Final lattice "sanity" checks

  // Check lattice limits
  if ((a == 0.0 && (Common::lt(xtal->getA(), getAMin(), LATT_LEN_COMP_TOL) ||
              Common::gt(xtal->getA(), getAMax(), LATT_LEN_COMP_TOL))) ||
      (b == 0.0 && (Common::lt(xtal->getB(), getBMin(), LATT_LEN_COMP_TOL) ||
              Common::gt(xtal->getB(), getBMax(), LATT_LEN_COMP_TOL))) ||
      (c == 0.0 && (Common::lt(xtal->getC(), getCMin(), LATT_LEN_COMP_TOL) ||
              Common::gt(xtal->getC(), getCMax(), LATT_LEN_COMP_TOL))) ||
      (alpha == 0.0 && (Common::lt(xtal->getAlpha(), getAlphaMin(), LATT_ANG_COMP_TOL) ||
                  Common::gt(xtal->getAlpha(), getAlphaMax(), LATT_ANG_COMP_TOL))) ||
      (beta == 0.0 && (Common::lt(xtal->getBeta(), getBetaMin(), LATT_ANG_COMP_TOL) ||
                 Common::gt(xtal->getBeta(), getBetaMax(), LATT_ANG_COMP_TOL))) ||
      (gamma == 0.0 && (Common::lt(xtal->getGamma(), getGammaMin(), LATT_ANG_COMP_TOL) ||
                  Common::gt(xtal->getGamma(), getGammaMax(), LATT_ANG_COMP_TOL)))) {
    QString out0 = QString("Discarding structure %1: bad lattice").arg(xtal->getTag());
    if (isVerbose()) {
      out0 += QString("\n       A:    %1  %2  %3\n       B:    %4  %5  %6"
                      "\n       C:    %7  %8  %9\n   Alpha:    %10  %11  %12"
                      "\n    Beta:    %13  %14  %15\n   Gamma:    %16  %17  %18\n")
              .arg(getAMin(),12,'f').arg(xtal->getA(),12,'f').arg(getAMax(),12,'f')
              .arg(getBMin(),12,'f').arg(xtal->getB(),12,'f').arg(getBMax(),12,'f')
              .arg(getCMin(),12,'f').arg(xtal->getC(),12,'f').arg(getCMax(),12,'f')
              .arg(getAlphaMin(),12,'f').arg(xtal->getAlpha(),12,'f').arg(getAlphaMax(),12,'f')
              .arg(getBetaMin(),12,'f').arg(xtal->getBeta(),12,'f').arg(getBetaMax(),12,'f')
              .arg(getGammaMin(),12,'f').arg(xtal->getGamma(),12,'f').arg(getGammaMax(),12,'f');
    }
    Common::debug(out0);
    return false;
  }

  // Reject the structure if the determinant of the cell matrix
  //   is zero, negative or non-finite.
  const double determinant = xtal->unitCell().cellMatrix().determinant();
  if (!GS_ISFINITE(determinant) || determinant <= ZERO08) {
    QString out0 =
       QString("Discarding structure %1: determinant of unit cell zero, negative, or non-finite")
       .arg(xtal->getTag());
    if (isVerbose()) {
      out0 += QString("\n");
      for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++)
          out0 += QString("   %1 ")
                      .arg(xtal->unitCell().cellMatrix()(i,j),12,'f',6);
        out0 += QString("\n");
      }
    }
    Common::debug(out0);
    return false;
  }

  // For non-empty cells; lastly, double check that volume is in
  //   the acceptable range.
  if (xtal->numAtoms() > 0) {
    double cur_vol = xtal->getVolume();
    if (Common::lt(cur_vol, minvol, ZERO08) ||
        Common::gt(cur_vol, maxvol, ZERO08)) {
      Common::debug(QString("Discarding structure %1: bad final volume (%2, min %3, max %4)")
            .arg(xtal->getTag()).arg(cur_vol).arg(minvol).arg(maxvol));
      return false;
    }
  }

  // We made it!
  return true;
}

bool XtalOpt::checkComposition(Xtal* xtal, bool isSeed)
{
  // This function checks if the composition (atom types and counts)
  //   of a given xtal is "valid".
  // Valid means that it doesn't have unknown element types, and
  //   depending on the search type it has a proper composition, i.e.,
  //   for FC/MC searches, the composition is listed.
  // Also, except than seed structures, it makes sure that the xtal
  //   does not have any zero atom counts, and the total counts does
  //   not exceed the max atom parameter.
  // For seed structures, we allow for zero atom counts, and raise the
  //   max atoms if needed.

  // Basic sanity checks
  if (!xtal || xtal->numAtoms() <= 0) {
    Common::debug(QString("%1: empty xtal.").arg(__func__));
    return false;
  }
  if (compList().isEmpty()) {
    Common::debug(QString("%1: composition is not set.").arg(__func__));
    return false;
  }

  QStringList chemSystem = getChemicalSystem();

  // Input xtal's info (composition, symbol list, atom counts, etc)
  CellComp comp = getXtalComposition(xtal);
  QList<QString> symbols = comp.getCompositionSymbols();
  uint numAtoms = comp.getNumAtoms();

  // Perform a series of checks
  bool hasExtraAtomCount = false;
  bool hasLowAtomCount = false;
  bool hasExtraTypes = false;
  bool hasMissingTypes = false;
  bool compositionIsNew = true;

  // Is there any species not defined in chemical system?
  for(int i = 0; i < symbols.size(); i++)
    if (!chemSystem.contains(symbols[i]))
      hasExtraTypes = true;

  // Does the total atoms count exceed the maximum number of atoms?
  if (numAtoms > static_cast<uint>(getMaxAtoms()))
    hasExtraAtomCount = true;

  // Does the total atoms count exceed the maximum number of atoms?
  if (numAtoms < static_cast<uint>(getMinAtoms()))
    hasLowAtomCount = true;

  // Is there any species of chemical system that is not present?
  for(int i = 0; i < chemSystem.size(); i++)
    if (!symbols.contains(chemSystem[i]))
      hasMissingTypes = true;

  // Is "chemical composition" equivalent to any on the initial list?
  // (We don't force it to be "exact" match with the list because
  //    random supercells generated in the run are accepted too).
  for(int i = 0; i < compList().size(); i++) {
    if (compareCompositions(compList()[i], comp) != 0.0) {
      compositionIsNew = false;
      break;
    }
  }

  // Now, according to run type and the above tests, see if xtal is ok.

  // Having extra types is always a no.
  if (hasExtraTypes) {
    Common::debug(QString("%1: unknown types in the xtal.").arg(__func__));
    return false;
  }

  // If a seed structure; we're done except than:
  // (1) if seed is a sub-system or it's composition is not on the list, mark it
  //     so we can manage for a proper genetic operation selection.
  // (2) if seed is acceptable, reset the min/max atom counts if needed
  if (isSeed) {
    if (hasMissingTypes || compositionIsNew)
      xtal->setCompositionValidity(false);
    adjustAtomCountLimits(static_cast<int>(numAtoms));
    return true;
  }

  // Except than seeds, for now, we won't handle "sub-system" structures.
  if (hasMissingTypes) {
    Common::debug(QString("%1: some atomic types are missing.").arg(__func__));
    return false;
  }

  // No structure can have more atoms than maxAtoms.
  if (hasExtraAtomCount) {
    Common::debug(QString("%1: number of atoms exceeds maxAtoms.").arg(__func__));
    return false;
  }

  // No structure can have fewer atoms than minAtoms.
  if (hasLowAtomCount) {
    Common::debug(QString("%1: number of atoms is lower than minAtoms.").arg(__func__));
    return false;
  }

  // For a variable-composition search, we're good.
  if (getVcSearch()) {
    // FIXME: this is a "mark"! In general, here we can add "new" compositions
    //   to the list of compositions and/or formulae if we wanted! E.g.,
    //formulas_input += "," + form;
    //compList.append(incomp);
    return true;
  }

  // So the search is either fixed-/multi-composition;
  //   that's, composition should be on the list.
  if (compositionIsNew) {
    Common::debug(QString("%1: composition does not match any list entry.")
                   .arg(__func__));
    return false;
  }

  // If we make it here, then xtal has an acceptable composition.
  return true;
}

void XtalOpt::adjustAtomCountLimits(int numAtoms)
{
  if (numAtoms > getMaxAtoms()) {
    setMaxAtoms(numAtoms);
    Common::warning(QString("Increased maxAtoms to %1 to fit a "
                            "structure/composition of that size.")
                      .arg(numAtoms));
  }
  if (numAtoms < getMinAtoms()) {
    setMinAtoms(numAtoms);
    Common::warning(QString("Decreased minAtoms to %1 to fit a "
                            "structure/composition of that size.")
                      .arg(numAtoms));
  }
}

bool XtalOpt::verifyCustomIADValues(bool reportError) const
{
  const QList<unsigned int> atomicNums = chemicalSystemAtomicNumbers(compList());
  if (atomicNums.isEmpty()) {
    if (reportError)
      Common::error("Custom IAD mode requires a non-empty chemical system.");
    return false;
  }

  for (int i = 0; i < atomicNums.size(); ++i) {
    for (int j = i; j < atomicNums.size(); ++j) {
      const double minDistance =
        pairCustomDistances().getPairDistance(atomicNums.at(i), atomicNums.at(j));
      if (minDistance == PINF) {
        if (reportError) {
          const QString label1 = QString::fromStdString(
            Atoms::ElementInfo::getAtomicSymbol(atomicNums.at(i)));
          const QString label2 = QString::fromStdString(
            Atoms::ElementInfo::getAtomicSymbol(atomicNums.at(j)));
          Common::error(QString("Missing customIAD for the pair %1")
                          .arg(label1 + "-" + label2));
        }
        return false;
      }
    }
  }

  return true;
}

bool XtalOpt::checkStepOptimizedStructure(Structure* s, QString* err)
{
  Xtal* xtal = qobject_cast<Xtal*>(s);
  if (xtal == nullptr) {
    return true;
  }

  // Normalize structures read from optimizer output unless a lattice
  //   parameter is fixed.
  if (Common::neq(getAMin(), getAMax(), LATT_LEN_COMP_TOL) &&
      Common::neq(getBMin(), getBMax(), LATT_LEN_COMP_TOL) &&
      Common::neq(getCMin(), getCMax(), LATT_LEN_COMP_TOL) &&
      Common::neq(getAlphaMin(), getAlphaMax(), LATT_ANG_COMP_TOL) &&
      Common::neq(getBetaMin(), getBetaMax(), LATT_ANG_COMP_TOL) &&
      Common::neq(getGammaMin(), getGammaMax(), LATT_ANG_COMP_TOL)) {
    xtal->fixAngles();
  }
  xtal->findSpaceGroup(getTolSpg());

  uint fixCount = xtal->getFixCount();

  // Check post-opt
  if (getUsingCheckStepOpt() && (getUsingCustomIAD() || getUsingScaledIAD())) {
    const bool customMode = getUsingCustomIAD();
    const int maxMoves = 100;
    int atom1 = -1;
    int atom2 = -1;
    double iad = 0.0;

    // One check more than the number of moves, so the last move is validated.
    for (int moves = 0; ; ++moves) {
      const bool ok = customMode
        ? xtal->checkInterAtomicDistancesCustom(pairCustomDistances(), &atom1, &atom2, &iad)
        : xtal->checkInterAtomicDistancesScaled(eleScaledRadii(), &atom1, &atom2, &iad);
      if (ok) {
        // Atoms were moved: redo the optimization with the repaired geometry.
        if (moves > 0) {
          s->setFixCount(fixCount + 1);
          s->setRestartOptStep(0);
        }
        return true;
      }
      if (moves >= maxMoves || fixCount >= 10)
        break;

      Atoms::Atom& atom = xtal->atom(atom2);
      const bool moved = customMode
        ? xtal->moveAtomRandomlyCustomIAD(atom.atomicNumber(), pairCustomDistances(), 1000, &atom)
        : xtal->moveAtomRandomlyScaledIAD(atom.atomicNumber(), eleScaledRadii(), 1000, &atom);
      if (!moved)
        break;
    }

    const double minIAD = customMode
      ? pairCustomDistances().getPairDistance(xtal->atom(atom1).atomicNumber(),
                                              xtal->atom(atom2).atomicNumber())
      : eleScaledRadii().getMinRadius(xtal->atom(atom1).atomicNumber()) +
        eleScaledRadii().getMinRadius(xtal->atom(atom2).atomicNumber());

    Common::debug(QString("Discarding structure %1: bad IAD (%2 < %3) - couldn't fix")
                    .arg(xtal->getTag()).arg(iad).arg(minIAD));

    if (err != nullptr)
      *err = "Two atoms are too close together.";
    return false;
  }

  // If early, don't check structure
  return true;
}

std::vector<double> XtalOpt::getReferenceEnergiesVector()
{
  // This is a convenience function to give SearchBase access to reference
  //   energies for hull calculations.
  // It returns a 1D vector of atom counts (sorted with symbols)
  //   and energies; ready to be added to the hull data input.
  // Reference structure might be a subsystem of chemical space;
  //   so we should return a vector with atom counts of "all symbols"
  //   including those that are -possibly- zero.
  std::vector<double> out;

  const QList<QString> chem_sys = getChemicalSystem();

  for (int i = 0; i < refEnergies().size(); i++) {
    QList<QString> symbols = refEnergies()[i].cell.getCompositionSymbols();
    double energy = refEnergies()[i].energy;
    for (int j = 0; j < chem_sys.size(); j++) {
      double natoms = 0.0;
      int ind = symbols.indexOf(chem_sys[j]);
      if (ind != -1) {
        natoms = static_cast<double>(refEnergies()[i].cell.getCount(symbols[ind]));
      }
      out.push_back(natoms);
    }
    out.push_back(energy);
  }

  return out;
}

void XtalOpt::refreshElementMinRadii()
{
  if (compList().isEmpty()) {
    eleScaledRadii().clearMinRadii();
    return;
  }

  eleScaledRadii().clearMinRadii();
  for (const auto& atomcn : compList()[0].getCompositionAtomicNumbers()) {
    double r = Atoms::ElementInfo::getCovalentRadius(atomcn) * getScaleFactor();
    if (r < getMinRadius())
      r = getMinRadius();
    eleScaledRadii().setMinRadius(atomcn, r);
  }
}

CellComp XtalOpt::getXtalComposition(Search::Structure* s)
{
  // Returns the "actual" composition, e.g., for a "sub-system" structure
  //   the output comp will have fewer atom types compared to the reference
  //   chemical system.
  CellComp out;

  if (s == nullptr || s->numAtoms() == 0)
    return out;

  QList<QString> symbs = s->getSymbols();
  std::vector<uint> count = s->getNumberOfAtomsAlpha();

  for (int i = 0; i < symbs.size(); i++) {
    uint atmcn = Atoms::ElementInfo::getAtomicNum(symbs[i].toStdString());
    out.setCompositionEntry(symbs[i], atmcn, count[i]);
  }

  return out;
}

CellComp XtalOpt::formulaToComposition(QString form)
{
  // Given a single input chemical formula (as a string); return
  //   the corresponding composition object.
  // This is done by adding symbol, atomic number, and atom count
  //   of each element to the composition object.

  CellComp compout;

  std::map<uint, uint> cmp;
  if (!Atoms::ElementInfo::readComposition(form.toStdString(), cmp)) {
    return compout;
  }

  for (const auto& elem : cmp) {
    compout.setCompositionEntry(Atoms::ElementInfo::getAtomicSymbol(elem.first).c_str(),
                elem.first, elem.second);
  }

  return compout;
}

double XtalOpt::compareCompositions(CellComp comp1, CellComp comp2)
{
  // This function determines if species of two comps match and if atom
  //   count of all species in comp2 are a fixed multiple "r > 0"
  //   of those in comp1.
  // So, the return value tells us that:
  // r > 0: comp2 has the same composition as comp1; with "r * num atoms",
  // r = 0: species don't match, or atom counts are not a fixed multiple, etc.

  // Some basic checks first
  if (comp1.getNumTypes() == 0 || comp1.getNumTypes() != comp2.getNumTypes())
    return 0;

  // Do species match?
  for (int i = 0; i < comp2.getNumTypes(); i++) {
    const QString symbol = comp2.getCompositionSymbols()[i];
    if (!comp1.getCompositionSymbols().contains(symbol) ||
        comp1.getCount(symbol) == 0 || comp2.getCount(symbol) == 0)
      return 0;
  }

  // Are counts equivalent: start by finding reference ratio.
  QString symb = comp1.getCompositionSymbols()[0];
  double ref1 = static_cast<double>(comp1.getCount(symb));
  double ref2 = static_cast<double>(comp2.getCount(symb));
  double refRatio = ref2 / ref1;

  for (int i = 0; i < comp2.getNumTypes(); i++) {
    symb = comp2.getCompositionSymbols()[i];
    double ele1 = static_cast<double>(comp1.getCount(symb));
    double ele2 = static_cast<double>(comp2.getCount(symb));
    double checkRatio = ele2 / ele1;
    if (Common::neq(checkRatio, refRatio, ZERO06))
      return 0;
  }

  // They are equivalent! Return the ratio.
  return refRatio;
}

void XtalOpt::getCompositionVolumeLimits(CellComp incomp, double& minvol, double& maxvol)
{
  // This function makes an estimate for min/max limits of volume corresponding to a composition.
  // From the composition, species types and their atom counts are obtained; and these
  //   info are then used to estimate volume limits according to one of the schemes:
  //   (1) elemental volumes, (2) scaled volume, or (3) absolute volume limits.

  // This shouldn't ever happen. Just in case ...
  if (incomp.getNumAtoms() == 0) {
    Common::warning("Unexpected empty composition in composition volume calculation.");
    minvol = 1;
    maxvol = 10000;
    return;
  }

  // Now: which scheme should be used calculate the volume limits?
  //   (1) If elemental volumes are given, use them,
  //   (2) If scaled volume is given, use that,
  //   (3) If none of the above, use absolute limits.
  bool useEleVol = (eleVolumes().getVolumeAtomicNumbers().size() == 0) ? false : true;
  bool useScaVol = (getVolScaleMin() > ZERO06 && getVolScaleMax() >= getVolScaleMin()) ? true : false;

  minvol = 0.0;
  maxvol = 0.0;

  QList<uint> atomicNums = incomp.getCompositionAtomicNumbers();

  for (int i = 0; i < atomicNums.size(); i++) {
    uint atomCount = incomp.getCount(atomicNums[i]);
    double atomVolum = Atoms::ElementInfo::getCovalentVolume(atomicNums[i]);
    if (useEleVol) {
      minvol += atomCount * eleVolumes().getMinVolume(atomicNums[i]);
      maxvol += atomCount * eleVolumes().getMaxVolume(atomicNums[i]);
    } else if (useScaVol) {
      minvol += atomCount * atomVolum * getVolScaleMin();
      maxvol += atomCount * atomVolum * getVolScaleMax();
    } else {
      minvol += atomCount * getVolMin();
      maxvol += atomCount * getVolMax();
    }
  }
}

void XtalOpt::warnVolumeLimitConflicts()
{
  if (compList().isEmpty())
    return;

  // If all three cell lengths are fixed, the cell volume can never be larger
  //   than a*b*c (a value which needs all three angles to be 90 degrees). So,
  //   a minimum volume above that can't be reached at all. A value of zero
  //   here means that the lengths are not all fixed and there is nothing to say.
  double maxCellVolume = 0.0;
  if (Common::eq(getAMin(), getAMax(), LATT_LEN_COMP_TOL) &&
      Common::eq(getBMin(), getBMax(), LATT_LEN_COMP_TOL) &&
      Common::eq(getCMin(), getCMax(), LATT_LEN_COMP_TOL))
    maxCellVolume = getAMin() * getBMin() * getCMin();

  // The element radii implied by the interatomic distance limits. These are
  //   used only if a distance limit is in use and, for the custom distances,
  //   the table has an entry for every pair.
  const double densePackingFraction = 0.74;
  const QList<uint> atomicNums = compList().first().getCompositionAtomicNumbers();
  QHash<uint, double> radii;
  bool haveRadii = getUsingScaledIAD() || getUsingCustomIAD();

  for (int i = 0; haveRadii && i < atomicNums.size(); ++i) {
    const uint atomicNum = atomicNums.at(i);
    if (getUsingScaledIAD()) {
      radii.insert(atomicNum, eleScaledRadii().getMinRadius(atomicNum));
      continue;
    }

    double minDistance = PINF;
    for (int j = 0; j < atomicNums.size(); ++j) {
      const double distance =
        pairCustomDistances().getPairDistance(atomicNum, atomicNums.at(j));
      if (distance == PINF) {
        haveRadii = false;
        break;
      }
      minDistance = std::min(minDistance, distance);
    }

    if (haveRadii)
      radii.insert(atomicNum, 0.5 * minDistance);
  }

  if (maxCellVolume <= 0.0 && !haveRadii)
    return;

  for (const auto& comp : compList()) {
    double minVolume = 0.0;
    double maxVolume = 0.0;
    getCompositionVolumeLimits(comp, minVolume, maxVolume);
    const QString formula = comp.getFormula();

    if (maxCellVolume > 0.0 && minVolume > maxCellVolume) {
      Common::warning(QString("The fixed cell lengths allow at most %2 A^3 for %1, "
                              "below the configured minimum volume of %3 A^3.")
                        .arg(formula)
                        .arg(maxCellVolume, 0, 'f', 3)
                        .arg(minVolume, 0, 'f', 3));
    }

    if (!haveRadii)
      continue;

    double sphereVolume = 0.0;
    for (int i = 0; i < atomicNums.size(); ++i) {
      const uint atomicNum = atomicNums.at(i);
      const double radius = radii.value(atomicNum);
      sphereVolume += comp.getCount(atomicNum) * (4.0 / 3.0) * PI * radius * radius * radius;
    }

    const double suggestedVolume = sphereVolume / densePackingFraction;
    const double suggestedVolumePerAtom =
      std::ceil((suggestedVolume / comp.getNumAtoms()) * 1000.0) / 1000.0;

    if (minVolume < sphereVolume) {
      Common::warning(
        QString("The configured minimum volume is too small to satisfy the IAD limits for %1 "
                "(suggested lower bound: %2 A^3/atom).")
          .arg(formula)
          .arg(suggestedVolumePerAtom, 0, 'f', 3));
    } else if (minVolume < suggestedVolume) {
      Common::warning(QString("The minimum volume may be too small for the IAD limits for %1 "
                              "(suggested lower bound: %2 A^3/atom).")
                        .arg(formula)
                        .arg(suggestedVolumePerAtom, 0, 'f', 3));
    }

    if (maxVolume < sphereVolume) {
      Common::warning(
        QString("The maximum volume cannot satisfy the IAD limits for %1 "
                "(suggested lower bound: %2 A^3/atom).")
          .arg(formula)
          .arg(suggestedVolumePerAtom, 0, 'f', 3));
    } else if (maxVolume < suggestedVolume) {
      Common::warning(QString("The maximum volume may be too small for the IAD limits for %1 "
                              "(suggested lower bound: %2 A^3/atom).")
                        .arg(formula)
                        .arg(suggestedVolumePerAtom, 0, 'f', 3));
    }
  }
  // Just a separator!
  Common::message("\n");
}

CellComp XtalOpt::getMinimalComposition()
{
  // Return the "minimum quantity composition": the one which
  //   has the smallest number of atoms of each symbol among
  //   all the compositions in the input list.
  // This is used whenever we need the smallest allowed composition.
  // NOTE: it is always assumed that the chemical elements
  //   in all compositions are the same; even if some have
  //   zero atom count!

  CellComp compMins;

  if (compList().isEmpty())
    return compMins;

  QList<QString> symbols = compList()[0].getCompositionSymbols();
  QList<uint> counts;
  for (int i = 0; i < symbols.size(); i++)
    counts.append(compList()[0].getCount(symbols[i]));

  for (int i = 1; i < compList().size(); i++) {
    for (int j = 0; j < symbols.size(); j++) {
      if (!compList()[i].getCompositionSymbols().contains(symbols[j]))
        return compMins;
      uint c = compList()[i].getCount(symbols[j]);
      if (c < counts[j])
        counts[j] = c;
    }
  }

  for (int i = 0; i < symbols.size(); i++) {
    compMins.setCompositionEntry(symbols[i],
                 Atoms::ElementInfo::getAtomicNum(symbols[i].toStdString()), counts[i]);
  }

  return compMins;
}

QList<QString> XtalOpt::getChemicalSystem() const
{
  // A small helper function: return sorted list of symbols in ref chemical system
  QList<QString> out = QList<QString>();

  if (compList().isEmpty())
    return out;

  out = compList()[0].getCompositionSymbols();

  return out;
}

QList<uint> XtalOpt::getListOfAtomsComp(CellComp incomp)
{
  if (incomp.getNumTypes() == 0) {
    Common::error(QString("%1: empty composition.").arg(__func__));
    return QList<uint>();
  }

  // Populate crystal
  QList<uint> atomicNums = incomp.getCompositionAtomicNumbers();
  // Sort atomic number by decreasing minimum radius. Adding the "larger"
  //   atoms first encourages a more even (and ordered) distribution
  for (int i = 0; i < atomicNums.size() - 1; ++i) {
    for (int j = i + 1; j < atomicNums.size(); ++j) {
      if (eleScaledRadii().getMinRadius(atomicNums[i]) < eleScaledRadii().getMinRadius(atomicNums[j])) {
        QtCompat::listSwapItemsAt(atomicNums, i, j);
      }
    }
  }

  QList<uint> atoms;
  for (int i = 0; i < atomicNums.size(); i++) {
    for (size_t j = 0; j < incomp.getCount(atomicNums[i]); j++) {
      atoms.push_back(atomicNums[i]);
    }
  }
  return atoms;
}

std::vector<uint> XtalOpt::getStdVecOfAtomsComp(CellComp incomp)
{
  auto list = getListOfAtomsComp(incomp);
  return std::vector<uint>(list.begin(), list.end());
}

bool XtalOpt::isRandSpgPossibleForComposition(uint spg, CellComp comp)
{
  return Atoms::Generators::canGenerateRandSpg(spg, getStdVecOfAtomsComp(comp));
}

QStringList XtalOpt::randSpgCompatibleFormulaStrings(uint spg)
{
  QStringList formulas;
  if (spg < 1 || spg > 230)
    return formulas;

  for (int i = 0; i < compList().size(); ++i) {
    if (isRandSpgPossibleForComposition(spg, compList().at(i)))
      formulas.append(compList().at(i).getFormula());
  }

  return formulas;
}

bool XtalOpt::setInputForcedSpgsString(const QString& value)
{
  QList<int> counts;
  for (int i = 0; i < 230; ++i)
    counts.append(0);

  QStringList list;
  if (!value.trimmed().isEmpty()) {
    list = value.trimmed().split(",", QtCompat::KeepEmptyParts);
    for (QString& item : list)
      item = item.trimmed();
  }
  for (const auto& item : list) {
    const int numhyphens = item.count(QLatin1Char('-'));
    if (numhyphens == 0) {
      bool ok = false;
      const unsigned int num = item.toUInt(&ok);
      if (!ok || num == 0 || num > 230)
        return false;
      ++counts[num - 1];
    } else if (numhyphens == 1) {
      QStringList num_list = item.split("-", QtCompat::SkipEmptyParts);
      if (num_list.size() != 2)
        return false;
      bool ok1, ok2;
      unsigned int min_num = num_list[0].toUInt(&ok1);
      unsigned int max_num = num_list[1].toUInt(&ok2);
      if (!ok1 || !ok2 || min_num == 0 || max_num > 230 || min_num > max_num)
        return false;
      for (unsigned int num = min_num; num <= max_num; num++)
        ++counts[num - 1];
    } else
      return false;
  }

  x_inputForcedSpgsString = value;
  if (list.isEmpty())
    minXtalsOfSpg().clear();
  else
    minXtalsOfSpg() = counts;
  return true;
}

} // namespace XtalOpt

/**********************************************************************
  MXtalOptGenetic - MolecularXtal genetic operators

  Copyright (C) 2009-2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include "mxtaloptgenetic.h"

#include <xtalopt/structures/molecularxtal.h>
#include <xtalopt/structures/submolecule.h>
#include <xtalopt/structures/submoleculesource.h>
#include <xtalopt/submoleculeranker.h>
#include <xtalopt/xtalopt.h>

#include <globalsearch/obeigenconv.h>

#include <openbabel/generic.h>
#include <openbabel/math/matrix3x3.h>
#include <openbabel/math/vector3.h>

#include <Eigen/Geometry>

#include <QtCore/QBitArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QVector>

#include <vector>

using namespace Avogadro;

namespace {

// Used in voight-generator
const double NV_MAGICCONST = 4 * exp(-0.5)/sqrt(2.0);

// Generate a random Voight strain matrix with elements randomly picked from
// a gaussian distribution G(x_0 = 0.0, sigma)
inline Eigen::Matrix3d randomVoight(const double sigma)
{
  Eigen::Matrix3d strainM;
  for (uint row = 0; row < 3; ++row) {
    for (uint col = row; col < 3; ++col) {
      // Generate random value from a Gaussian distribution.
      // Ported from Python's standard random library.
      // Uses Kinderman and Monahan method. Reference: Kinderman,
      // A.J. and Monahan, J.F., "Computer generation of random
      // variables using the ratio of uniform deviates", ACM Trans
      // Math Software, 3, (1977), pp257-260.
      // mu = 0, sigma = sigma_lattice
      double z;
      while (true) {
        double u1 = RANDDOUBLE();
        double u2 = 1.0 - RANDDOUBLE();
        if (u2 == 0.0) continue; // happens a _lot_ with MSVC...
        z = NV_MAGICCONST*(u1-0.5)/u2;
        double zz = z * z * 0.25;
        if (zz <= -log(u2))
          break;
      }
      double epsilon = z * sigma;
      if (col == row) {
        strainM(row, col) = 1 + epsilon;
      }
      else {
        strainM(col, row) = strainM(row, col) = epsilon * 0.5;
      }
    }
  }
  return strainM;
}

// Set the generation of MXtal to max(parent1.gen,parent2.gen)+1,
// or just parent1+1 if parent2 is omitted.
void assignGeneration(XtalOpt::MolecularXtal *mxtal,
                      const XtalOpt::MolecularXtal *parent1,
                      const XtalOpt::MolecularXtal *parent2 = NULL)
{
  const int gen1 = parent1->getGeneration();
  const int gen2 = (parent2 != NULL)
      ? parent2->getGeneration()
      : -1;

  mxtal->setGeneration( (gen1 > gen2) ? gen1 + 1 : gen2 + 1 );
}

void wrapFractionalCoordinate(Eigen::Vector3d *vec)
{
  if (((*vec)[0] = fmod((*vec)[0], 1.0)) < 0) ++(*vec)[0];
  if (((*vec)[1] = fmod((*vec)[1], 1.0)) < 0) ++(*vec)[1];
  if (((*vec)[2] = fmod((*vec)[2], 1.0)) < 0) ++(*vec)[2];
}

inline Eigen::AngleAxisd alignVectors(const Eigen::Vector3d &from,
                                      const Eigen::Vector3d &to)
{
  // Occasionally floating point round-off makes the argument of the acos
  // below fall slightly outside of [-1,1], creating a Nan that will blow up
  // the matrix. Check/correct that here.
  const double angleCosine = from.normalized().dot(to.normalized());
  double angle = 0;
  if (angleCosine > 1.0)
    angle = 0;
  else if (angleCosine < -1.0)
    angle = M_PI;
  else
    angle = acos(angleCosine);

  const Eigen::Vector3d axis(from.cross(to).normalized());

  return Eigen::AngleAxisd(angle, axis);
}

// Energies must be sorted low->high, must be >= 3 energies.
inline QList<double> getProbabilities(const QList<double> &energies)
{
  QList<double> probs;
#if QT_VERSION >= 0x040600
  probs.reserve(energies.size());
#endif

  const int numEnergies = energies.size();

  // Handle special cases
  switch (numEnergies)
  {
  // Special case: 0 energies
  case 0:
    qWarning() << "Can't calculate probabilities for a list of 0 energies.";
    return probs;
  // Special case: 1 energies
  case 1:
    probs << 1.0;
    return probs;
  // Special case: 2 energies
  case 2:
    probs << 0.75 << 1.0;
    return probs;
  // All others
  default:
    break;
  }

  // Similar technique as OptBase::getProbabilityList. See comments there
  // for details. Difference is that here we favor high energies.
  const double low = energies.first();
  const double high = energies.last();
  const double spread = high - low;
  if (spread < 1e-5) {
    const double dprob = 1.0 / static_cast<double>(numEnergies - 1);
    double prob = 0.0;
    for (int i = 0; i < numEnergies; ++i) {
      probs << prob;
      prob += dprob;
    }
  }
  double sum = 0.0;
  for (int i = 0; i < numEnergies; ++i) {
    probs << (energies.at(i) - low) / spread;
    // skip the subtraction from one -- this favors high energy.
    sum += probs.last();
  }
  for (int i = 0; i < numEnergies; ++i)
    probs[i] /= sum;
  sum = 0.0;
  for (int i = 0; i < numEnergies; ++i)
    sum = (probs[i] += sum);
  return probs;
}

} // end anonymous namespace

namespace XtalOpt
{
namespace MXtalOptGenetic
{

// Crossover
MolecularXtal * crossover(const MolecularXtal *parent1,
                          const MolecularXtal *parent2,
                          XtalOpt *opt)
{
  // Choose random variables

  // - Cut axes
  Axis cutAxis1, cutAxis2;
  do {
    cutAxis1 = static_cast<Axis>(3.0 * RANDDOUBLE());
    cutAxis2 = static_cast<Axis>(3.0 * RANDDOUBLE());
  }
  while (cutAxis1 > C_AXIS || cutAxis2 > C_AXIS); // RANDDOUBLE can return 1.0
  // - Fractional shifts
  const Eigen::Vector3d fracShift1 (Eigen::Vector3d::Random().normalized()
                                    * RANDDOUBLE());
  const Eigen::Vector3d fracShift2 (Eigen::Vector3d::Random().normalized()
                                    * RANDDOUBLE());
  // - cutPosition
  const double cutPos =
      (RANDDOUBLE() * (50.0 - opt->mga_cross_minimumContribution) +
      opt->mga_cross_minimumContribution) / 100.0;

  // Non random info:
  // - SubMoleculeSource quantites list sorted by sourceId
  QVector<unsigned int> quantities;
  quantities.reserve(opt->mcomp.size());
  for (QList<MolecularCompStruct>::const_iterator
       it = opt->mcomp.constBegin(), it_end = opt->mcomp.constEnd();
       it != it_end; ++it) {
    if (it->source->sourceId() == quantities.size()) {
      quantities.push_back(it->quantity);
    }
  }

  // Perform operation. Eight args feels too fortran-y :-(
  MolecularXtal *mxtal =
      crossover(parent1, parent2, cutAxis1, cutAxis2, fracShift1, fracShift2,
                cutPos, quantities);

  // Assign metadata
  assignGeneration(mxtal, parent1, parent2);
  mxtal->setParents(QString("Crossover: %1 (%3) + %2 (%4)")
                    .arg(parent1->getIDString())
                    .arg(parent2->getIDString())
                    .arg(cutPos * 100, 3)
                    .arg((1-cutPos) * 100, 3));
  mxtal->setStatus(MolecularXtal::WaitingForOptimization);

  return mxtal;
}

MolecularXtal * crossover(const MolecularXtal *parent1,
                          const MolecularXtal *parent2,
                          Axis cutAxis1,
                          Axis cutAxis2,
                          const Eigen::Vector3d fracShift1,
                          const Eigen::Vector3d fracShift2,
                          double cutPos,
                          // Sorted by submolecule sourceId
                          const QVector<unsigned int> &quantities)
{
  // Fetch lattice vectors
  std::vector<OpenBabel::vector3> obvecs1 =
      parent1->OBUnitCell()->GetCellVectors();
  std::vector<OpenBabel::vector3> obvecs2 =
      parent2->OBUnitCell()->GetCellVectors();
  const Eigen::Vector3d avec1 (obvecs1[0].AsArray());
  const Eigen::Vector3d bvec1 (obvecs1[1].AsArray());
  const Eigen::Vector3d cvec1 (obvecs1[2].AsArray());
  const Eigen::Vector3d avec2 (obvecs2[0].AsArray());
  const Eigen::Vector3d bvec2 (obvecs2[1].AsArray());
  const Eigen::Vector3d cvec2 (obvecs2[2].AsArray());
  const double a1 = avec1.norm();
  const double b1 = bvec1.norm();
  const double c1 = cvec1.norm();
  const double a2 = avec2.norm();
  const double b2 = bvec2.norm();
  const double c2 = cvec2.norm();

  // Construct transforms that will rotate the coordinates into place for
  // cutting.
  Eigen::Transform3d cartTransform1;
  Eigen::Transform3d fracTransform1;
  Eigen::Transform3d cartTransform2;
  Eigen::Transform3d fracTransform2;
  cartTransform1.setIdentity();
  fracTransform1.setIdentity();
  cartTransform2.setIdentity();
  fracTransform2.setIdentity();

  switch(cutAxis1)
  {
  case C_AXIS: // Rotate c -> b
    cartTransform1.prerotate(alignVectors(cvec1, bvec1));
    fracTransform1.prerotate(Eigen::AngleAxisd(0.5 * M_PI,
                                               Eigen::Vector3d(1.0, 0.0, 0.0)));
  case B_AXIS: // Rotate b -> a
    cartTransform1.rotate(alignVectors(bvec1, avec1));
    fracTransform1.rotate(Eigen::AngleAxisd(0.5 * M_PI,
                                               Eigen::Vector3d(0.0, 0.0, 1.0)));
  case A_AXIS:
  default:
    break;
  }

  switch(cutAxis2)
  {
  case C_AXIS: // Rotate c -> b
    cartTransform2.prerotate(alignVectors(cvec2, bvec2));
    fracTransform2.prerotate(Eigen::AngleAxisd(0.5 * M_PI,
                                               Eigen::Vector3d(1.0, 0.0, 0.0)));
  case B_AXIS: // Rotate b -> a
    cartTransform2.rotate(alignVectors(bvec2, cvec2));
    fracTransform2.rotate(Eigen::AngleAxisd(0.5 * M_PI,
                                               Eigen::Vector3d(0.0, 0.0, 1.0)));
  case A_AXIS:
  default:
    break;
  }

  // Cache lists of rotated fractional coordinates of the submolecule centers
  QList<SubMolecule*> submolecules1 = parent1->subMolecules();
  QVector<unsigned long> sourceIds1;
  sourceIds1.reserve(parent1->numSubMolecules());
  QVector<Eigen::Vector3d> fracCoords1;
  fracCoords1.reserve(parent1->numSubMolecules());
  QBitArray mask1 (parent1->numSubMolecules(), false);

  QList<SubMolecule*> submolecules2 = parent2->subMolecules();
  QVector<unsigned long> sourceIds2;
  sourceIds2.reserve(parent2->numSubMolecules());
  QVector<Eigen::Vector3d> fracCoords2;
  fracCoords2.reserve(parent2->numSubMolecules());
  QBitArray mask2 (parent2->numSubMolecules(), false);

  for (QList<SubMolecule*>::const_iterator it = submolecules1.constBegin(),
       it_end = submolecules1.constEnd(); it != it_end; ++it) {
    fracCoords1.push_back(fracTransform1 *
                          (fracShift1 + parent1->cartToFrac((*it)->center())));
    wrapFractionalCoordinate(&fracCoords1.back());
    sourceIds1.push_back((*it)->sourceId());
    if (fracCoords1.last().x() <= cutPos) {
      // This submolecule is a candidate for addition
      mask1.setBit(fracCoords1.size() - 1, true);
    }
  }

  for (QList<SubMolecule*>::const_iterator it = submolecules2.constBegin(),
       it_end = submolecules2.constEnd(); it != it_end; ++it) {
    fracCoords2.push_back(fracTransform2 *
                          (fracShift2 + parent2->cartToFrac((*it)->center())));
    wrapFractionalCoordinate(&fracCoords2.back());
    sourceIds2.push_back((*it)->sourceId());
    if (fracCoords2.last().x() > cutPos) {
      // This submolecule is a candidate for addition
      mask2.setBit(fracCoords2.size() - 1, true);
    }
  }

  // Count number of candidates of each type
  QVector<unsigned int> currentQuantities;
  currentQuantities.resize(quantities.size());
  currentQuantities.fill(0);

  for (int i = 0; i < sourceIds1.size(); ++i) {
    if (!mask1.testBit(i)) continue;
    ++currentQuantities[sourceIds1[i]];
  }

  for (int i = 0; i < sourceIds2.size(); ++i) {
    if (!mask2.testBit(i)) continue;
    ++currentQuantities[sourceIds2[i]];
  }

  // Compare quantities and adjust.
  for (int sourceId = 0; sourceId < quantities.size(); ++sourceId) {
    const unsigned int &target = quantities[sourceId];
    unsigned int &value  = currentQuantities[sourceId];
    // Set up a pseudo-random, yet reproducible method for deciding which
    // submols to add/remove:
    // - Parent to add/remove from
    bool useFirstParent = static_cast<bool>(target + value%2);
    const MolecularXtal *parent = (useFirstParent) ? parent1 : parent2;
    QBitArray *mask = (useFirstParent) ? &mask1 : &mask2;
    QVector<unsigned long> *sourceIds =
        (useFirstParent) ? &sourceIds1 : &sourceIds2;

    // - Index to begin iterating through the submolecules
    int startIndex = target * value + target * target + value * value + target;
    startIndex %= parent->numSubMolecules();

    bool adjusted;
    bool lastChance = false; // Need to switch parents if this becomes true
    while (target != value) {
      adjusted = false;
      // Add a submol of this type
      if (value < target) {
        for (; startIndex < mask->size(); ++startIndex) {
          if (mask->testBit(startIndex) ||
              sourceIds->at(startIndex) != sourceId) continue;
          mask->toggleBit(startIndex);
          ++value;
          adjusted = true;
          break;
        }
      }
      // Or remove a submol of this type
      else if (value > target) {
        for (; startIndex < mask->size(); ++startIndex) {
          if (!mask->testBit(startIndex) ||
              sourceIds->at(startIndex) != sourceId) continue;
          mask->toggleBit(startIndex);
          --value;
          adjusted = true;
          break;
        }
      }

      if (lastChance && !adjusted) {
        // Switch parents
        parent    = (!useFirstParent) ? parent1     : parent2;
        mask      = (!useFirstParent) ? &mask1      : &mask2;
        sourceIds = (!useFirstParent) ? &sourceIds1 : &sourceIds2;
        startIndex = target * value + target * target + value * value + value;
        startIndex %= parent->numSubMolecules();
      }

      if (adjusted) {
        startIndex += target * value + value;
        startIndex %= parent->numSubMolecules();
        continue;
      }
      else {
        startIndex = 0;
        lastChance = true;
      }
    }
  }

  // At this point, our composition should be correct. Build new Mxtal:
  MolecularXtal *mxtal = new MolecularXtal();
  // Average cells, weighted by cutPos
  OpenBabel::matrix3x3 cell1 = parent1->OBUnitCell()->GetCellMatrix();
  OpenBabel::matrix3x3 cell2 = parent2->OBUnitCell()->GetCellMatrix();
  OpenBabel::matrix3x3 cell;
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      cell.Set(i, j, cell1.Get(i,j) * cutPos + cell2.Get(i,j) * (1-cutPos));
    }
  }
  mxtal->setCellInfo(cell);

  // Now add SubMolecules
  Eigen::Vector3d translation;
  // From parent1:
  for (int i = 0; i < mask1.size(); ++i) {
    if (!mask1.testBit(i)) continue;
    SubMolecule *sub = parent1->subMolecule(i)->getCoherentClone();
    // Translate to origin, then move to new position
    translation  = -sub->center();
    translation += mxtal->fracToCart(fracCoords1[i]);
    sub->translate(translation);
    mxtal->addSubMolecule(sub);
  }
  // From parent2:
  for (int i = 0; i < mask2.size(); ++i) {
    if (!mask2.testBit(i)) continue;
    SubMolecule *sub = parent2->subMolecule(i)->getCoherentClone();
    // Translate to origin, then move to new position
    translation  = -sub->center();
    translation += mxtal->fracToCart(fracCoords2[i]);
    sub->translate(translation);
    mxtal->addSubMolecule(sub);
  }

  return mxtal;
}

// Reconf

MolecularXtal * reconf(const MolecularXtal *parent, XtalOpt *opt)
{
  // Create new mxtal as a copy of the parent
  MolecularXtal *mxtal = new MolecularXtal (*parent);

  // List of submolecule sources sorted by id
  QVector<SubMoleculeSource*> sources;
  sources.reserve(opt->mcomp.size());
  // mcomp is already sorted by id
  for (QList<MolecularCompStruct>::const_iterator it = opt->mcomp.constBegin(),
       it_end = opt->mcomp.constEnd(); it != it_end; ++it) {
    sources.push_back(it->source);
  }

  // Number of swaps to perform
  int numSwaps = static_cast<int>(
        ( opt->mga_reconf_maxSubMolsToReplace -
          opt->mga_reconf_minSubMolsToReplace ) * RANDDOUBLE() +
        opt->mga_reconf_minSubMolsToReplace);
  if (numSwaps > mxtal->numSubMolecules()) {
    numSwaps = mxtal->numSubMolecules();
  }

  // List of indices that will be swapped
  QVector<int> submolSwapIndices;
  submolSwapIndices.reserve(numSwaps);
  for (int i = 0; i < numSwaps; ++i) {
    int ind;
    do {
      ind = static_cast<int>(RANDDOUBLE() * mxtal->numSubMolecules());
    }
    while (ind > mxtal->numSubMolecules() ||
           submolSwapIndices.contains(ind));
    submolSwapIndices.append(ind);
  }

  // Conformer indices to swap in. Indexed by submolecule location.
  QVector<int> newConfIndices;
  newConfIndices.resize(mxtal->numSubMolecules());
  newConfIndices.fill(-1); // No swap by default
  // Replace the confIndices for the submols that we're re-confing
  for (QVector<int>::const_iterator it = submolSwapIndices.constBegin(),
       it_end = submolSwapIndices.constEnd(); it != it_end; ++it) {
    const int numConfs =
        sources[mxtal->subMolecule(*it)->sourceId()]->numConformers();
    int newConfIndex;
    do {
      newConfIndex = static_cast<int>(RANDDOUBLE() * numConfs);
    }
    while (newConfIndex >= numConfs);

    newConfIndices[*it] = newConfIndex;
  }

  // Perform reconf:
  reconf(mxtal, sources, newConfIndices);

  // Apply strain
  double sigma = (opt->mga_reconf_maxStrain - opt->mga_reconf_minStrain) *
      RANDDOUBLE() + opt->mga_reconf_minStrain;
  strain(mxtal, randomVoight(sigma));

  // Assign metadata
  assignGeneration(mxtal, parent);
  mxtal->setParents(QString("Reconf: %1, %2 replacements")
                    .arg(parent->getIDString())
                    .arg(numSwaps));
  mxtal->setStatus(MolecularXtal::WaitingForOptimization);

  return mxtal;
}

void reconf(MolecularXtal *mxtal,
            const QVector<SubMoleculeSource*> &sources,
            const QVector<int> &newConfIndices)
{
  QList<SubMolecule*> subs = mxtal->subMolecules();
  Q_ASSERT_X(newConfIndices.size() == subs.size(), Q_FUNC_INFO,
             "Conformer index vector size must match number of submolecules.");

  for (int i = 0; i < newConfIndices.size(); ++i) {
    int newConfIndex = newConfIndices[i];
    if (newConfIndex < 0) {
      continue;
    }

    SubMolecule *oldSub = subs[i];
    const Eigen::Vector3d center = oldSub->center();
    const Eigen::Vector3d normal = oldSub->normal();
    const Eigen::Vector3d farvec = oldSub->farthestAtomVector();

    Q_ASSERT(oldSub->sourceId() < sources.size());
    SubMoleculeSource *source = sources[oldSub->sourceId()];
    Q_ASSERT(newConfIndex < source->numConformers());
    SubMolecule *newSub = source->getSubMolecule(newConfIndex);
    newSub->setCenter(center);
    newSub->align(normal, farvec);

    mxtal->replaceSubMolecule(i, newSub);
  }
}

// Swirl
// TODO update the gui -- remove keep in plane param and min rot
MolecularXtal * swirl(const MolecularXtal *parent, XtalOpt *opt)
{
  // Create new mxtal as a copy of the parent
  MolecularXtal *mxtal = new MolecularXtal (*parent);

  // Apply strain
  double sigma = (opt->mga_swirl_maxStrain - opt->mga_swirl_minStrain) *
      RANDDOUBLE() + opt->mga_swirl_minStrain;
  strain(mxtal, randomVoight(sigma));

  // mxtal will be used during ranking to determine the best rotations.
  // offspring will be used in the unit-testable overload as the actual
  // returned offspring.
  MolecularXtal *offspring = new MolecularXtal (*mxtal);

  // Number of submolecules to rotate
  int numRots = static_cast<int>(
        ( opt->mga_swirl_maxSubMolsToRotate-
          opt->mga_swirl_minSubMolsToRotate ) * RANDDOUBLE() +
        opt->mga_swirl_minSubMolsToRotate);
  if (numRots > mxtal->numSubMolecules()) {
    numRots = mxtal->numSubMolecules();
  }

  // Angles (radian) and rotation axis for each submolecule. 0 angle indicates
  // no rotation.
  QVector<double> angles;
  QVector<Eigen::Vector3d> axes;
  angles.resize(mxtal->numSubMolecules());
  axes.resize(mxtal->numSubMolecules());
  // No rotation by default. Axes don't need initialization for no rotation.
  angles.fill(0.0);

  // Setup molecule ranker
  SubMoleculeRanker ranker (NULL, opt->sOBMutex);
  ranker.setDebug(true);
  ranker.setMXtal(mxtal); // TODO what if this returns false?

  // Debugging....
  const double preTotalEnergy = ranker.evaluateTotalEnergy();
  const QVector<double> preEnergies = ranker.evaluate();
  const QDateTime preTime = QDateTime::currentDateTime();

  // Create list of submolecules that can be rotated (after rotating, each is
  // removed)
  QList<SubMolecule*> submols = mxtal->subMolecules();
  QMap<double, SubMolecule*> sortedSubmols;
  QList<double> probabilities;

  // Loop through numRots
  for (int rot = 0; rot < numRots; ++rot) {
    sortedSubmols.clear();
    probabilities.clear();

    // Evaluate submolecule energies and sort
    foreach (SubMolecule* sub, submols) {
      double energy = ranker.evaluate(sub) /
          static_cast<double>(sub->numAtoms());
      sortedSubmols.insertMulti(energy, sub);
    }

    // Calculate energy-weighted probability list
    QList<double> energies = sortedSubmols.keys();
    probabilities = getProbabilities(energies);

    // Select one submolecule according to probability list
    const double r_sub = RANDDOUBLE();
    int ind;
    for (ind = 0; ind < probabilities.size(); ++ind)
      if (r_sub < probabilities.at(ind)) break;
    SubMolecule *sub = sortedSubmols.value(energies.at(ind));

    // Coin toss -- Use normal, normal orthogonal, or random vector as
    // rotation axis?
    Eigen::Vector3d axis;
    const double r_axis = RANDDOUBLE();
    if (r_axis < 0.33)
      axis = sub->normal();
    else if (r_axis < 0.67)
      axis = sub->normal().unitOrthogonal();
    else
      axis = Eigen::Vector3d::Random().normalized();

    // Scan rotations around axis, ending with molecule in original
    // configuration
    double angle = 0.0;
    double step = 30.0 * DEG_TO_RAD;
    QMap<double, double> energyAngleMap;
    while (angle < (2.0*M_PI + 0.5*step)) { // end up where we started
      if (angle > 1e-3) {
        sub->rotate(step, axis);
        ranker.updateGeometry(sub);
      }
      // Use total interatomic energy, since we're trying to minimize the
      // energy of the entire cell using a rigid body approximation.
      double energy = ranker.evaluateTotalEnergyInter();
      qDebug() << "Scanned angle:" << angle * RAD_TO_DEG
               << "total inter E:" << energy;
      energyAngleMap.insert(energy, angle);
      angle += step;
    }

    // Use an energy-weighted probability to select the rotation angle.
    // First trim off 1/2 of the worst rotations
    QMap<double, double>::iterator angleEraser = energyAngleMap.begin() +
        static_cast<int>(0.5 * energyAngleMap.size());
    while (angleEraser != energyAngleMap.end())
      angleEraser = energyAngleMap.erase(angleEraser);
    energies = energyAngleMap.keys();
    probabilities = getProbabilities(energies);
    qFatal("You didn't fix this, Dave! This probability distribution "
           "favors high energy configurations!!");
    const double r_angle = RANDDOUBLE();
    for (ind = 0; ind < probabilities.size(); ++ind)
      if (r_angle < probabilities.at(ind)) break;
    angle = energyAngleMap.value(energies.at(ind));

    // Store selected angle/axis
    const int submolInd = mxtal->subMolecules().indexOf(sub);
    axes.replace(submolInd, axis);
    angles.replace(submolInd, angle);

    // Apply selected rotation to prepare for next loop
    sub->rotate(angle, axis);
    ranker.updateGeometry(sub);

    // Remove current submolecule from list of candidates
    submols.removeOne(sub);
  }

  // Debugging...
  const int time = QDateTime::currentDateTime().secsTo(preTime);
  const double postTotalEnergy = ranker.evaluateTotalEnergy();
  const QVector<double> postEnergies = ranker.evaluate();
  const double diffTE = postTotalEnergy - preTotalEnergy;
  QVector<double> diffE (postEnergies.size());
  for (int i = 0; i < postEnergies.size(); ++i) {
    diffE[i] = postEnergies[i] - preEnergies[i];
  }
  qDebug() << "New swirl! Time in secs:" << time;
  qDebug() << "preEnergy postEnergy diff:"
           << preTotalEnergy << postTotalEnergy << diffTE;
  qDebug() << "preEnergies :" << preEnergies;
  qDebug() << "postEnergies:" << postEnergies;
  qDebug() << "diffEnergies:" << diffE;


  // Cleanup working mxtal now that we have our lists of axes/angles
  ranker.setMXtal(NULL);
  delete mxtal;
  mxtal = NULL;

  // Perform swirl
  swirl(offspring, axes, angles);

  // Assign metadata
  assignGeneration(offspring, parent);
  offspring->setParents(QString("Swirl: %1, %2 rotated submolecules")
                        .arg(parent->getIDString())
                        .arg(numRots));
  offspring->setStatus(MolecularXtal::WaitingForOptimization);

  return offspring;
}

void swirl(MolecularXtal *mxtal,
           const QVector<Eigen::Vector3d> &axes,
           const QVector<double> &angles)
{
  Q_ASSERT(mxtal->numSubMolecules() == axes.size() &&
           mxtal->numSubMolecules() == angles.size());

  for (int i = 0; i < mxtal->numSubMolecules(); ++i) {
    double angle = angles[i];
    if (fabs(angle) < 1e-5) continue;
    const Eigen::Vector3d &axis = axes[i];
    SubMolecule *sub = mxtal->subMolecule(i);

    sub->rotate(angle, axis);
  }
}

// Strain
void strain(MolecularXtal *mxtal, const Eigen::Matrix3d &voight)
{
  // Cache fractional submol centers for later
  QList<SubMolecule*> subs = mxtal->subMolecules();
  QVector<Eigen::Vector3d> fracCenters;
  fracCenters.reserve(mxtal->numSubMolecules());
  for (QList<SubMolecule*>::const_iterator it = subs.constBegin(),
       it_end = subs.constEnd(); it != it_end; ++it) {
    fracCenters.push_back(mxtal->cartToFrac((*it)->center()));
  }

  // Adjust cell
  mxtal->setCellInfo(mxtal->OBUnitCell()->GetCellMatrix() * Eigen2OB(voight));

  // Restore fractional centers of submolecules
  for (int i = 0; i < subs.size(); ++i) {
    subs[i]->setCenter(mxtal->fracToCart(fracCenters[i]));
  }
}

} // end namespace MXtalOptGenetic
} // end namespace XtalOpt

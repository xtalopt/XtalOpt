/**********************************************************************
  HoleFinder - Find low-density regions in three-dimensional space

  Copyright (C) 2012 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include "holefinder.h"

#include <globalsearch/macros.h> // for RANDDOUBLE()

#include <QtCore/QBitArray>
#include <QtCore/QDebug>
#include <QtCore/QMap>
#include <QtCore/QVector>

#include <Eigen/LU> // For MatrixBase::inverse();

#define M_PI 3.14159265358979323846
#include <limits>

#define DEBUGOUT(_funcnam_) \
  if (debug)    qDebug() << this << " " << _funcnam_ << ": " <<
#define DDEBUGOUT(_funcnam_) \
  if (d->debug) qDebug() << this << " " << _funcnam_ << ": " <<

namespace XtalOpt
{

/****************************************************************************
 * Hole class                                                               *
 ****************************************************************************/

class Hole
{
public:
  Hole()
    : center(0.0, 0.0, 0.0), radius(0.0), radiusSquared(0.0) {}
  Hole(const Eigen::Vector3d cen, double rad)
    : center(cen)
  {
    this->setRadius(rad);
  }

  void setRadius(double rad)
  {
    radius = rad;
    radiusSquared = rad * rad;
  }

  void setRadiusSquared(double radSq)
  {
    radius = sqrt(radSq);
    radiusSquared = radSq;
  }

  double volume() const
  {
    return 1.33333333333 * M_PI * radius * radius * radius;
  }

  Eigen::Vector3d center;
  double radius;
  double radiusSquared;
};

/****************************************************************************
 * HoleFinderPrivate class definition                                       *
 ****************************************************************************/

class HoleFinderPrivate
{
public:
  HoleFinderPrivate(HoleFinder *parent)
    : q_ptr(parent),
      debug(false),
      numPoints(0),
      resolution(1.0),
      minVolume(5.0),
      minDist(2.0)

  {}

  bool debug;
  int numPoints;
  // Initial scan setup
  double resolution;
  // Smallest allowable hole
  double minVolume;
  // Minimium distance from an existing point to a randomly generated point
  double minDist;
  // Translation vectors
  Eigen::Vector3d v1;
  Eigen::Vector3d v2;
  Eigen::Vector3d v3;
  double v1Norm;
  double v2Norm;
  double v3Norm;
  double cellVolume;
  // Fractionalization/cartesian matrices
  Eigen::Matrix3d fracMat;
  Eigen::Matrix3d cartMat;
  // Containers
  QVector<Eigen::Vector3d> points;
  QVector<Hole> holes;
  QMap<double, Hole> holeMap;
  QVector<double> holeProbs;

  // Change bases
  inline void cartToFrac(Eigen::Vector3d *vec) const
  {
    *vec = (this->fracMat * (*vec)).eval();
  }
  inline void fracToCart(Eigen::Vector3d *vec) const
  {
    *vec = (this->cartMat * (*vec)).eval();
  }

  // Build initial array of holes
  void buildGrid();
  // Update the whether or not hole is empty
  void updateHole(Hole *hole);
  // Reduce holes by merging neighbors
  void mergeHoles();
  // Merge the holes in vec into one, and return the result
  Hole reduceHoles(const QVector<Hole*> & holeVec);
  // Resize holes by checking distances
  void resizeHoles();
  // Populate holeMap, trim small holes
  void finalizeHoles();
  // Generate probability list from data
  void setProbabilities();
  // Get a random index into a ProbabilityVector
  const Hole &getRandomHole();

  // Print out all hole data
  void printHoles();

private:
  HoleFinder * const q_ptr;
  Q_DECLARE_PUBLIC(HoleFinder);
};

/****************************************************************************
 * HoleFinder methods                                                       *
 ****************************************************************************/

HoleFinder::HoleFinder(unsigned int numPoints)
  : d_ptr(new HoleFinderPrivate(this))
{
  Q_D(HoleFinder);
  d->numPoints = numPoints;
}

HoleFinder::~HoleFinder()
{
  delete d_ptr;
}

void HoleFinder::setDebug(bool b)
{
  Q_D(HoleFinder);
  d->debug = b;
}

void HoleFinder::setTranslations(const Eigen::Vector3d &v1,
                                 const Eigen::Vector3d &v2,
                                 const Eigen::Vector3d &v3)
{
  Q_D(HoleFinder);
  d->v1 = v1;
  d->v2 = v2;
  d->v3 = v3;
  d->cartMat.col(0) = v1;
  d->cartMat.col(1) = v2;
  d->cartMat.col(2) = v3;
  d->fracMat = d->cartMat.inverse();
  double v1SqrNorm = v1.squaredNorm();
  double v2SqrNorm = v2.squaredNorm();
  double v3SqrNorm = v3.squaredNorm();
  d->v1Norm = sqrt(v1SqrNorm);
  d->v2Norm = sqrt(v2SqrNorm);
  d->v3Norm = sqrt(v3SqrNorm);
  d->cellVolume = fabs(v1.dot(v2.cross(v3)));

  DDEBUGOUT("setTranslations")
      QString("Cart matrix:\n %L1 %L2 %L3\n %L4 %L5 %L6\n %L7 %L8 %L9")
      .arg(d->cartMat(0,0), 10)
      .arg(d->cartMat(0,1), 10)
      .arg(d->cartMat(0,2), 10)
      .arg(d->cartMat(1,0), 10)
      .arg(d->cartMat(1,1), 10)
      .arg(d->cartMat(1,2), 10)
      .arg(d->cartMat(2,0), 10)
      .arg(d->cartMat(2,1), 10)
      .arg(d->cartMat(2,2), 10);
  DDEBUGOUT("setTranslations")
      QString("Frac matrix:\n %L1 %L2 %L3\n %L4 %L5 %L6\n %L7 %L8 %L9")
      .arg(d->fracMat(0,0), 10)
      .arg(d->fracMat(0,1), 10)
      .arg(d->fracMat(0,2), 10)
      .arg(d->fracMat(1,0), 10)
      .arg(d->fracMat(1,1), 10)
      .arg(d->fracMat(1,2), 10)
      .arg(d->fracMat(2,0), 10)
      .arg(d->fracMat(2,1), 10)
      .arg(d->fracMat(2,2), 10);

  int numCells = 27;
  d->points.reserve(numCells * d->numPoints);

  DDEBUGOUT("setTranslations") "VectorLengths:"
      << d->v1Norm  << d->v2Norm  << d->v3Norm;
  DDEBUGOUT("setTranslations") "CellVolume" << d->cellVolume;
  DDEBUGOUT("setTranslations") "NumPoints:" << d->numPoints;
  DDEBUGOUT("setTranslations") "NumCells:" << numCells;
  DDEBUGOUT("setTranslations") "Allocated points:" << d->points.capacity();
}

void HoleFinder::setGridResolution(double res)
{
  Q_D(HoleFinder);
  d->resolution = res;
}

void HoleFinder::setMinimumHoleVolume(double volume)
{
  Q_D(HoleFinder);
  d->minVolume = volume;
}

void HoleFinder::setMinimumHoleRadius(double radius)
{
  Q_D(HoleFinder);
  d->minVolume = 1.33333333 * M_PI * radius * radius * radius;
}

void HoleFinder::setMinimumDistance(double min)
{
  Q_D(HoleFinder);
  d->minDist = min;
}

void HoleFinder::addPoint(const Eigen::Vector3d &pos)
{
  Q_D(HoleFinder);
  Eigen::Vector3d wrappedPos (pos);
  d->cartToFrac(&wrappedPos);
  if ((wrappedPos[0] = fmod(wrappedPos[0], 1.0)) < 0.0) ++wrappedPos.x();
  if ((wrappedPos[1] = fmod(wrappedPos[1], 1.0)) < 0.0) ++wrappedPos.y();
  if ((wrappedPos[2] = fmod(wrappedPos[2], 1.0)) < 0.0) ++wrappedPos.z();
  d->fracToCart(&wrappedPos);
  Eigen::Vector3d aTrans;
  Eigen::Vector3d bTrans;
  Eigen::Vector3d cTrans;
  for (double a = -1; a <= 1; ++a) {
    aTrans = a * d->v1;
    for (double b = -1; b <= 1; ++b) {
      bTrans = b * d->v2;
      for (double c = -1; c <= 1; ++c) {
        cTrans = c * d->v3;
        d->points.push_back(wrappedPos + aTrans + bTrans + cTrans);
      }
    }
  }
}

void HoleFinder::run()
{
  Q_D(HoleFinder);

  // Debugging...
  DDEBUGOUT("run") "Points considered:" << d->points.size();

  // Sample initial holes on grid
  d->buildGrid();
  d->resizeHoles();

#if 0 // Merging may not be necessary
  // Cache number of holes
  int numHoles;

  // Repeat merge procedure until the number of holes is constant
  int iter = 0;
  do {
    numHoles = d->holes.size(); // Cache number of holes
    d->mergeHoles();  // Combine degenerate holes
    d->resizeHoles(); // Determine radii of merged holes
    DDEBUGOUT("run") QString("Iteration %1: %2 holes before merge, %3 after.")
        .arg(++iter).arg(numHoles).arg(d->holes.size());
  }
  while (numHoles != d->holes.size());
#endif

  d->finalizeHoles();
  d->setProbabilities();

//  if (d->debug) d->printHoles();
}

Eigen::Vector3d HoleFinder::getRandomPoint()
{
  Q_D(HoleFinder);

  // Randomly select a hole
  const Hole &hole = d->getRandomHole();

  // Trim the radius of the sphere by 1.5 units. If the sphere is smaller than
  // 1.5 units, trim it to 0.5 units.
  const double rmax = (hole.radius >= d->minDist)
      ? hole.radius - d->minDist : 0.5;

  // Randomly select point from uniform distribution around sphere. The radius
  // is chosen such that
  //
  // P(r) [is prop. to] Area(r) = 4*pi*r^2, or
  // P(r) = N * 4*pi*r^2
  //
  // We want P(r) to lie between [0,1] and r to be between [0, rmax].
  //
  // Lower boundary conditions are trivially satisfied:
  //
  // P(0) = N * 4*pi*0^2 = 0
  //
  // Upper boundary conditions:
  //
  // P(rmax) = 1 = N * 4*pi*rmax^2, or
  // N = 1 / (4*pi*rmax^2)
  //
  // So P(r) simplifies to
  //
  // P(r) = (1/(4*pi*r^2)) * 4*pi*r^2 = r^2 / (rmax^2)
  //
  // So we can pick a "p" value from a uniform distribution from [0,1] and
  // transform it to a random radius that is uniformly distributed throughout
  // the sphere by
  const double p = RANDDOUBLE();
  const double r = sqrt(p) * rmax;

  // A random unit vector representing the displacement:
  Eigen::Vector3d displacement (2.0 * RANDDOUBLE() - 1.0,
                                2.0 * RANDDOUBLE() - 1.0,
                                2.0 * RANDDOUBLE() - 1.0);
  displacement.normalize();

  // Put the two together
  displacement *= r;

  // Shift by the hole's origin
  const Eigen::Vector3d ret (hole.center + displacement);

  // Ta-da!
  return ret;
}

/****************************************************************************
 * HoleFinderPrivate methods                                                *
 ****************************************************************************/

void HoleFinderPrivate::buildGrid()
{
  const Eigen::Vector3d v1Step (this->v1.normalized() * this->resolution);
  const Eigen::Vector3d v2Step (this->v2.normalized() * this->resolution);
  const Eigen::Vector3d v3Step (this->v3.normalized() * this->resolution);
  Eigen::Vector3d v1Base;
  Eigen::Vector3d v2Base;
  Eigen::Vector3d v3Base;
  const int v1Steps = floor(v1Norm / this->resolution);
  const int v2Steps = floor(v2Norm / this->resolution);
  const int v3Steps = floor(v3Norm / this->resolution);

  this->holes.clear();
  this->holes.reserve(v1Steps * v2Steps * v3Steps);

  for (int a = 0; a <= v1Steps; ++a) {
    v1Base = v1Step * a;
    for (int b = 0; b <= v2Steps; ++b) {
      v2Base = v2Step * b;
      for (int c = 0; c <= v3Steps; ++c) {
        v3Base = v3Step * c;
        this->holes.push_back(Hole(v1Base + v2Base + v3Base, 0.0));
      }
    }
  }
  DEBUGOUT("buildGrid") "Initial grid constructed. Number of holes:"
      << this->holes.size();
}

void HoleFinderPrivate::mergeHoles()
{
  // Make copy, clear original, add merged holes back into original
  QVector<Hole> holeCopy (this->holes);
  const int numHoles = this->holes.size();
  this->holes.clear();
  this->holes.reserve(numHoles);

  // If the bit is on, it has not been merged. If off, it has been.
  QBitArray mask (holeCopy.size(), true);

  // Check each pair of unmerged holes. If one contains the other, merge them.
  QVector<Hole*> toMerge;
  toMerge.reserve(256); // Way bigger than we need, but certainly sufficient

  // Temp vars
  Eigen::Vector3d diffVec;

  // "i" indexes the "base" hole
  for (int i = 0; i < numHoles; ++i) {
    if (!mask.testBit(i))
      continue;

    mask.clearBit(i);
    Hole &hole_i = holeCopy[i];

    toMerge.clear();
    toMerge.reserve(256);
    toMerge.push_back(&hole_i);

    // "j" indexes the compared holes
    for (int j = i+1; j < numHoles; ++j) {
      if (!mask.testBit(j))
        continue;

      Hole &hole_j = holeCopy[j];

      diffVec = hole_j.center - hole_i.center;

      // Use the greater of the two radii
      const double rad = (hole_i.radius > hole_j.radius)
          ? hole_i.radius : hole_j.radius;
      const double radSq = rad * rad;

      // Check periodic conditions
      // Convert diffVec to fractional units
      this->cartToFrac(&diffVec);
      // Adjust each component to range [-0.5, 0.5] (shortest representation)
      while (diffVec.x() < -0.5) ++diffVec.x();
      while (diffVec.y() < -0.5) ++diffVec.y();
      while (diffVec.z() < -0.5) ++diffVec.z();
      while (diffVec.x() > 0.5) --diffVec.x();
      while (diffVec.y() > 0.5) --diffVec.y();
      while (diffVec.z() > 0.5) --diffVec.z();
      // Back to cartesian
      this->fracToCart(&diffVec);

      // if j is within i's radius, add "j" to the merge list
      // and mark "j" as merged
      if (fabs(diffVec.x()) > rad ||
          fabs(diffVec.y()) > rad ||
          fabs(diffVec.z()) > rad ||
          fabs(diffVec.squaredNorm()) > radSq)
        continue; // no match

      // match:
      // Reset j's position to account for periodic wrap-around
      hole_j.center = hole_i.center + diffVec;
      mask.clearBit(j);
      toMerge.push_back(&hole_j);
    }

    if (toMerge.size() == 1)
      this->holes.push_back(hole_i);
    else
      this->holes.push_back(reduceHoles(toMerge));
  }
}

Hole HoleFinderPrivate::reduceHoles(const QVector<Hole *> &holeVec)
{
  // Average the positions, weighted by volume
  Eigen::Vector3d center (0.0, 0.0, 0.0);
  double denom = 0.0;
  foreach (Hole *hole, holeVec) {
    const double volume = hole->volume();
    center += hole->center * volume;
    denom += volume;
  }
  center /= denom;

  // Wrap center into the unit cell
  this->cartToFrac(&center);
  if ((center.x() = fmod(center.x(), 1.0)) < 0) ++center.x();
  if ((center.y() = fmod(center.y(), 1.0)) < 0) ++center.y();
  if ((center.z() = fmod(center.z(), 1.0)) < 0) ++center.z();
  this->fracToCart(&center);

  return Hole(center, 0.0); // radius is set during resizeHoles();
}

void HoleFinderPrivate::resizeHoles()
{
  if (this->points.size() == 0) {
    const double r = 2.0 * this->resolution;
    DEBUGOUT("resizeHoles") "No points -- setting each hole radius to" << r;
    for (QVector<Hole>::iterator it = this->holes.begin(),
         it_end = this->holes.end(); it != it_end; ++it) {
      it->setRadius(r);
    }
    return;
  }
  for (QVector<Hole>::iterator it = this->holes.begin(),
       it_end = this->holes.end(); it != it_end; ++it) {
// This is needed for windows, which seems to define a max() macro
#undef max
    it->radiusSquared = std::numeric_limits<double>::max();
    for (QVector<Eigen::Vector3d>::const_iterator
         pit = this->points.constBegin(),
         pit_end = this->points.constEnd();
         pit != pit_end; ++pit) {
      const double distSq = (it->center - *pit).squaredNorm();
      if (distSq < it->radiusSquared)
        it->setRadiusSquared(distSq);
    }
  }
  DEBUGOUT("resizeHoles") this->holes.size() << "resized.";
}

void HoleFinderPrivate::finalizeHoles()
{
  QMap<double, Hole> tmpMap;
  double totalVolume = 0.0;
  for (QVector<Hole>::const_iterator it = this->holes.constBegin(),
       it_end = this->holes.constEnd(); it != it_end; ++it) {
    double volume = it->volume();
    tmpMap.insertMulti(volume, *it);
    totalVolume += volume;
  }

  QMap<double, Hole>::const_iterator largestHole = tmpMap.constEnd() - 1;
  QMap<double, Hole>::const_iterator smallestHole =
      tmpMap.lowerBound(this->minVolume);
  if (smallestHole == tmpMap.constEnd()) {
    smallestHole = tmpMap.lowerBound(0.1 * largestHole.key());
    qWarning() << "HoleFinderPrivate::finalizeHoles No holes satisfying the "
                  "minimum specified volume of " << this->minVolume <<  "."
                  " Using a minimum volume that is 10% of the largest hole,"
               << smallestHole.key();
  }

  this->holeMap.clear();
  double trimmedVolume = 0.0;
  for (QMap<double, Hole>::const_iterator it = smallestHole,
       it_end = tmpMap.constEnd(); it != it_end; ++it) {
    trimmedVolume += it.key();
    this->holeMap.insertMulti(it.key(), it.value());
  }

  this->holes.clear();

  DEBUGOUT("finalizeHoles") "Total hole volume:" << totalVolume;
  DEBUGOUT("finalizeHoles") "Trimmed hole volume:" << trimmedVolume;
  DEBUGOUT("finalizeHoles") "Largest hole volume:" << largestHole.key();
  DEBUGOUT("finalizeHoles") "Smallest hole volume:" << smallestHole.key();
  DEBUGOUT("finalizeHoles") "Total holes:" << tmpMap.size();
  DEBUGOUT("finalizeHoles") "Trimmed holes:" << this->holeMap.size();
}

void HoleFinderPrivate::setProbabilities()
{
  const QList<double> volumes (this->holeMap.keys());

  this->holeProbs.clear();
  this->holeProbs.reserve(volumes.size());

  const int numVolumes = volumes.size();

  // Handle special cases
  switch (numVolumes)
  {
  // Special case: 0 volumes
  case 0:
    qWarning() << "Can't calculate probabilities for a list of 0 volumes.";
    return;
  // Special case: 1 volume
  case 1:
    this->holeProbs << 1.0;
    return;
  // Special case: 2 volumes
  case 2:
    this->holeProbs << 0.25 << 1.0; // favor highest 3:1
    return;
  // All others
  default:
    break;
  }

  // Similar technique as OptBase::getProbabilityList. See comments there
  // for details. Difference is that here we favor high energies.
  const double low = volumes.first();
  const double high = volumes.last();
  const double spread = high - low;
  if (spread < 1e-5) {
    const double dprob = 1.0 / static_cast<double>(numVolumes - 1);
    double prob = 0.0;
    for (int i = 0; i < numVolumes; ++i) {
      this->holeProbs << prob;
      prob += dprob;
    }
  }
  double sum = 0.0;
  for (int i = 0; i < numVolumes; ++i) {
    this->holeProbs << (volumes.at(i) - low) / spread;
    // skip the subtraction from one -- this favors high values.
    sum += this->holeProbs.last();
  }
  for (int i = 0; i < numVolumes; ++i)
    this->holeProbs[i] /= sum;
  sum = 0.0;
  for (int i = 0; i < numVolumes; ++i)
    sum = (this->holeProbs[i] += sum);
  return;
}

const Hole & HoleFinderPrivate::getRandomHole()
{
  Q_ASSERT(this->holeProbs.size() != 0);

  double r = RANDDOUBLE();
  int ind;
  for (ind = 0; ind < this->holeProbs.size(); ind++)
    if (r < this->holeProbs[ind]) break;

  QMap<double, Hole>::const_iterator retIt = this->holeMap.constBegin();
  for (int i = 0; i < ind; ++i) ++retIt;

  return retIt.value();
}

void HoleFinderPrivate::printHoles()
{
  std::cout << "Printing holes..." << std::endl;
  int i = 0;
  for (QMap<double, Hole>::const_iterator it = this->holeMap.constBegin(),
       it_end = this->holeMap.constEnd(); it != it_end; ++it) {
    std::cout << QString("Volume: %L1 Prob: %L2 Center: %L3 %L4 %L5")
                 .arg(it->volume(), 10)
                 .arg(this->holeProbs.at(i++), 10)
                 .arg(it->center.x(), 10)
                 .arg(it->center.y(), 10)
                 .arg(it->center.z(), 10)
                 .toStdString().c_str()
              << std::endl;
  }
}

}

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

#ifndef HOLEFINDER_H
#define HOLEFINDER_H

#include <QtCore/QObject>

#include <Eigen/Core>

namespace XtalOpt
{
class HoleFinderPrivate;

class HoleFinder : public QObject
{
public:
  HoleFinder(unsigned int numPoints);
  virtual ~HoleFinder();

  // Whether or not to print debugging output
  void setDebug(bool b);

  // Translation vectors used.
  void setTranslations(const Eigen::Vector3d &v1,
                       const Eigen::Vector3d &v2,
                       const Eigen::Vector3d &v3);

  // Grid resolution in data units
  void setGridResolution(double res);

  // Minimum hole size. Only call one -- both set the same variable internally
  // Default is a volume of 4 cubic units.
  void setMinimumHoleVolume(double volume);
  void setMinimumHoleRadius(double radius);

  // Minimum distance from an existing point to a randomly generated point.
  // Default is 2.0 units.
  void setMinimumDistance(double min);

  // Add a point to the system
  void addPoint(const Eigen::Vector3d &pos);

  // Find holes
  void run();

  // Return a point that favors proximity to a hole.
  Eigen::Vector3d getRandomPoint();

protected:
  HoleFinderPrivate * const d_ptr;

private:
  Q_DECLARE_PRIVATE(HoleFinder);
};

}
#endif // HOLEFINDER_H

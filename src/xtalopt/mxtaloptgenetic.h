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

#ifndef MXTALOPTGENETIC_H
#define MXTALOPTGENETIC_H

// @todo Implement energy evaluations and roulette selection

#include <Eigen/Core>

template <typename T> class QVector;

namespace XtalOpt
{
class MolecularXtal;
class SubMoleculeSource;
class XtalOpt;

namespace MXtalOptGenetic
{

enum Axis {
  A_AXIS = 0,
  B_AXIS,
  C_AXIS
};

// Crossover
MolecularXtal * crossover(const MolecularXtal *parent1,
                          const MolecularXtal *parent2,
                          XtalOpt *opt);
MolecularXtal * crossover(const MolecularXtal *parent1,
                          const MolecularXtal *parent2,
                          Axis cutAxis1,
                          Axis cutAxis2,
                          const Eigen::Vector3d fracShift1,
                          const Eigen::Vector3d fracShift2,
                          double cutPos,
                          // Sorted by submolecule sourceId
                          const QVector<unsigned int> &quantities);


// Reconf
MolecularXtal * reconf(const MolecularXtal *parent, XtalOpt *opt);
void reconf(MolecularXtal *mxtal,
            const QVector<SubMoleculeSource*> &sources,
            const QVector<int> &newConfIndices);

// Swirl
MolecularXtal * swirl(const MolecularXtal *parent, XtalOpt *opt);
void swirl(MolecularXtal *mxtal,
           const QVector<Eigen::Vector3d> &axes,
           const QVector<double> &angles);

// Strain
void strain(MolecularXtal *mxtal, const Eigen::Matrix3d &voight);

} // end namespace MXtalOptGenetic
} // end namespace XtalOpt

#endif // MXTALOPTGENETIC_H

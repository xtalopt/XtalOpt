/****************************************************************************
  MolecularXtalSuperCellGenerator
  Modifies a MolecularXtal by creating a supercell from the input geometry.

  Copyright (C) 2012 by David C. Lonie

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
  more details.
 ****************************************************************************/

#ifndef MOLECULARXTALSUPERCELLGENERATOR_H
#define MOLECULARXTALSUPERCELLGENERATOR_H

template<typename t> class QVector;

namespace XtalOpt
{
class MolecularXtal;
class MolecularXtalSuperCellGeneratorPrivate;

class MolecularXtalSuperCellGenerator
{
public:
  MolecularXtalSuperCellGenerator();
  virtual ~MolecularXtalSuperCellGenerator();

  void setDebug(bool debug);
  void setMXtal(const MolecularXtal &mxtal);
  // See the probability generation methods in MolecularXtalMutatorPrivate
  // for the format of the probability lists
  void setSubMolecularEnergies(const QVector<double> energies);

  const MolecularXtal & mxtal() const;
  const QVector<double> & subMolecularEnergies() const;
  bool debug() const;

  // Returns a list of pointers to new molecularXtals that contain the
  // generated supercells. The pointers remain valid until this class goes
  // out of scope or setMXtal is called again. The returned structures are
  // unique according to the MolecularXtal::operator== operator.
  const QVector<MolecularXtal *> & getSuperCells();

  // Return a list of submolecule ids that were not considered in the super
  // cell generation (e.g. an odd composition for a submolecule). These
  // should be relocated by the "movers" algorithm.
  const QVector<int> & getUnassignedSubMolecules() const;

protected:
  MolecularXtalSuperCellGeneratorPrivate * const d;
};

}

#endif // MOLECULARXTALSUPERCELLGENERATOR_H

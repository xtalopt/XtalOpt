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

#include "molecularxtalsupercellgenerator.h"

#include <xtalopt/structures/molecularxtal.h>
#include <xtalopt/structures/submolecule.h>

#include <globalsearch/macros.h>
#include <globalsearch/obeigenconv.h>

#include <QtCore/QDebug>
#include <QtCore/QVector>

#define DEBUGOUT(_funcnam_) \
  if (debug)    qDebug() << this << " " << _funcnam_ << ": " <<
#define DDEBUGOUT(_funcnam_) \
  if (d->debug) qDebug() << this << " " << _funcnam_ << ": " <<

namespace XtalOpt
{

//////////////////////////////////////////////////////////////////////////////
// MolecularXtalSuperCellGeneratorPrivate definition                        //
//////////////////////////////////////////////////////////////////////////////

class MolecularXtalSuperCellGeneratorPrivate
{
public:
  MolecularXtalSuperCellGeneratorPrivate()
    : debug(false)
  {}

  ~MolecularXtalSuperCellGeneratorPrivate()
  {
    this->clearSuperCells();
  }

  MolecularXtal parentMXtal;
  QVector<double> energies;
  QVector<MolecularXtal*> supercells;
  QVector<int> moverIndices;
  QVector<int> referenceIndices;
  QVector<int> unassignedIndices; // e.g. odd composition
  bool debug;

  enum Axis {
    A_Axis = 0,
    B_Axis,
    C_Axis
  };

  // Calculate a probability distribution for the energies provided
  QVector<double> calculateProbabilities(const QVector<double> &energies);

  // Randomly select an index from a probability distribution
  int selectIndexFromProbabilities(const QVector<double> probabilities);

  // Select and cache the indices of the submolecules that will be moved,
  // along with their reference submolecules, and any "left overs"
  void assignMovers();

  // Deletes and clears all supercells
  void clearSuperCells();

  // Checks if the mxtal is equivalent to any in the list. If not, it is
  // appended to the list and the function returns true. If it is a duplicate,
  // it will be deleted and the function returns false.
  bool addSuperCell(MolecularXtal *mxtal);

  // Create a supercell with the movers in the same positions exactly as the
  // references. Unassigned submols are in their original positions.
  MolecularXtal * generateStarterSuperCell();

  // generate various supercells
  MolecularXtal * generateStraightSuperCell(double translationFactor,
                                            Axis axis);
};

//////////////////////////////////////////////////////////////////////////////
// MolecularXtalSuperCellGenerator methods                                  //
//////////////////////////////////////////////////////////////////////////////

MolecularXtalSuperCellGenerator::MolecularXtalSuperCellGenerator()
  : d(new MolecularXtalSuperCellGeneratorPrivate())
{
}

MolecularXtalSuperCellGenerator::~MolecularXtalSuperCellGenerator()
{
  delete d;
}

void MolecularXtalSuperCellGenerator::setDebug(bool debug)
{
  d->debug = debug;
}

void MolecularXtalSuperCellGenerator::setMXtal(const MolecularXtal &mxtal)
{
  d->clearSuperCells();
  d->parentMXtal = mxtal;
}

void MolecularXtalSuperCellGenerator::
setSubMolecularEnergies(const QVector<double> energies)
{
  d->clearSuperCells();
  d->energies = energies;
}

const MolecularXtal &MolecularXtalSuperCellGenerator::mxtal() const
{
  return d->parentMXtal;
}

const QVector<double> &MolecularXtalSuperCellGenerator::
subMolecularEnergies() const
{
  return d->energies;
}

bool MolecularXtalSuperCellGenerator::debug() const
{
  return d->debug;
}

const QVector<MolecularXtal *> &MolecularXtalSuperCellGenerator::
getSuperCells()
{
  // return cached cells if available
  if (d->supercells.size() != 0) {
    return d->supercells;
  }

  // Assign movers
  d->assignMovers();

  // Generate straight translations
  MolecularXtal *sc;
  for (int i = 0; i < 3; ++i) {
    MolecularXtalSuperCellGeneratorPrivate::Axis axis =
        MolecularXtalSuperCellGeneratorPrivate::Axis(i);
//    sc = d->generateStraightSuperCell(0.25, axis);
//    d->addSuperCell(sc);
//    sc = d->generateStraightSuperCell(0.33, axis);
//    d->addSuperCell(sc);
    sc = d->generateStraightSuperCell(0.50, axis);
    d->addSuperCell(sc);
    // TODO more translations + (reflections, inversions), etc
  }

  return d->supercells;
}


const QVector<int> &MolecularXtalSuperCellGenerator::
getUnassignedSubMolecules() const
{
  return d->unassignedIndices;
}

//////////////////////////////////////////////////////////////////////////////
// MolecularXtalSuperCellGeneratorPrivate methods                           //
//////////////////////////////////////////////////////////////////////////////

QVector<double> MolecularXtalSuperCellGeneratorPrivate::
calculateProbabilities(const QVector<double> &energies)
{
  QVector<double> probs;
  probs.reserve(energies.size());

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
    probs << 0.75 << 1.0; // favor lowest 3:1
    return probs;
  // All others
  default:
    break;
  }

  // Similar technique as OptBase::getProbabilityList. See comments there
  // for details.
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
    return probs;
  }
  double sum = 0.0;
  for (int i = 0; i < numEnergies; ++i) {
    probs << 1.0 - ((energies.at(i) - low) / spread);
    sum += probs.last();
  }
  for (int i = 0; i < numEnergies; ++i)
    probs[i] /= sum;
  sum = 0.0;
  for (int i = 0; i < numEnergies; ++i)
    sum = (probs[i] += sum);
  return probs;
}

int MolecularXtalSuperCellGeneratorPrivate::
selectIndexFromProbabilities(const QVector<double> probs)
{
  if (probs.size() == 0) {
    return -1;
  }
  double r = RANDDOUBLE();
  int ind;
  for (ind = 0; ind < probs.size(); ind++)
    if (r < probs[ind]) break;
  return ind;
}

void MolecularXtalSuperCellGeneratorPrivate::assignMovers()
{
  // Determine composition by source id
  QVector<unsigned int> composition;
  foreach (const SubMolecule *sub, this->parentMXtal.subMolecules()) {
    unsigned long sourceId = sub->sourceId();
    Q_ASSERT_X(sourceId >= 0, Q_FUNC_INFO, "A submolecule has a negative "
               "sourceId. This usually means that the submolecule source "
               "used to generate it did not have a sourceId set. The "
               "supercell generator will *not* work without this!");
    while (composition.size() < sourceId + 1) {
      composition.push_back(0);
    }
    ++composition[sourceId];
  }

  // For each id found, select numSubMolsWithThatId / 2 as reference
  // molecules. This is the "good half". From the remaining submolecules, the
  // same number are selected as movers. Any leftovers are tracked in the
  // unassignedIndices vector.
  for (int sourceId = 0; sourceId < composition.size(); ++sourceId) {
    // Calculate number of reference molecules for this source id
    int stoich = composition[sourceId];
    int numRefSubMols = stoich / 2;
    int numMovers = numRefSubMols;

    // Build list of energies and a lookup table that maps energy list index
    // (key) to submolecule index (value)
    QVector<int> energyToSubMolLUT;
    QVector<double> energyList;
    energyToSubMolLUT.reserve(stoich);
    energyList.reserve(stoich);
    for (int i = 0; i < stoich; ++i) {
      energyToSubMolLUT.push_back(i);
      energyList.push_back(this->energies.at(i));
    }

    // Select references for this source id
    for (int i = 0; i < numRefSubMols; ++i) {
      // Calculate probabilities for current energy list
      QVector<double> probList = this->calculateProbabilities(energyList);

      // Randomly select reference from probabilitiy list
      int refEInd = this->selectIndexFromProbabilities(probList);

      // Translate energy index to submol index
      int refInd = energyToSubMolLUT[refEInd];

      // Update reference list, energy list, and lookup table
      this->referenceIndices.push_back(refInd);
      energyList.remove(refEInd);
      energyToSubMolLUT.remove(refEInd);
    }

    // The remaining submol indexes will be movers or unassigned
    for (QVector<int>::iterator it = energyToSubMolLUT.begin(),
         it_end = energyToSubMolLUT.begin() + numMovers;
         it != it_end; ++it) {
      this->moverIndices.push_back(*it);
    }
    // Any leftovers are unassigned
    if (energyToSubMolLUT.size() != numMovers) {
      this->unassignedIndices.push_back(energyToSubMolLUT.back());
    }
  }
}

void MolecularXtalSuperCellGeneratorPrivate::clearSuperCells()
{
  qDeleteAll(this->supercells);
  this->supercells.clear();
}

bool MolecularXtalSuperCellGeneratorPrivate::
addSuperCell(MolecularXtal *trialCell)
{
  const QString trialName = trialCell->property("supercellType").toString();
  if (*trialCell == this->parentMXtal) {
    DEBUGOUT("addSuperCell") "Rejecting supercell" << trialName
                                                   << "- matches parent";
    delete trialCell;
    trialCell = NULL;
    return false;
  }
  foreach (MolecularXtal *acceptedCell, this->supercells) {
    if (*trialCell == *acceptedCell) {
      DEBUGOUT("addSuperCell") "Rejecting supercell"
          << trialName << "- matches"
          << acceptedCell->property("supercellType").toString();

      delete trialCell;
      trialCell = NULL;
      return false;
    }
  }

  DEBUGOUT("addSuperCell") "Accepting supercell" << trialName;

  this->supercells.push_back(trialCell);
  return true;
}

MolecularXtal *MolecularXtalSuperCellGeneratorPrivate::
generateStarterSuperCell()
{
  // Start with a copy of the parent
  MolecularXtal *starter = new MolecularXtal (this->parentMXtal);

  // Adjust each mover so that it lies on top of the corresponding ref
  const int numMovers = this->moverIndices.size();
  Q_ASSERT(numMovers == this->referenceIndices.size());
  for (int i = 0; i < numMovers; ++i) {
    SubMolecule *ref  = starter->subMolecule(this->referenceIndices[i]);
    SubMolecule *move = starter->subMolecule(this->moverIndices[i]);
    const int numAtoms = ref->numAtoms();
    Q_ASSERT(numAtoms == move->numAtoms());
    for (int a = 0; a < numAtoms; ++a) {
      Avogadro::Atom *refAtom  = ref->atom(a);
      Avogadro::Atom *moveAtom = move->atom(a);
      Q_ASSERT(refAtom->atomicNumber() == moveAtom->atomicNumber());
      moveAtom->setPos(refAtom->pos());
    }
  }

  starter->setProperty("supercellType", QString("Unknown (starter)"));

  return starter;
}

MolecularXtal *MolecularXtalSuperCellGeneratorPrivate::
generateStraightSuperCell(double translationFactor, Axis axis)
{
  // Start with the starter supercell
  MolecularXtal *supercell = this->generateStarterSuperCell();

  Eigen::Vector3d trans;
  QString axisString;

  switch (axis) {
  case A_Axis:
    trans = Avogadro::OB2Eigen(
          supercell->OBUnitCell()->GetCellMatrix().GetRow(0));
    axisString = "v1";
    break;
  case B_Axis:
    trans = Avogadro::OB2Eigen(
          supercell->OBUnitCell()->GetCellMatrix().GetRow(1));
    axisString = "v2";
    break;
  case C_Axis:
    trans = Avogadro::OB2Eigen(
          supercell->OBUnitCell()->GetCellMatrix().GetRow(2));
    axisString = "v3";
    break;
  }

  // Apply translationFactor
  trans *= translationFactor;

  // Translate each atom in a mover
  foreach(const int moverInd, this->moverIndices) {
    supercell->subMolecule(moverInd)->translate(trans);
  }

  supercell->setProperty("supercellType", QString("%1 %2")
                         .arg(translationFactor)
                         .arg(axisString));

  return supercell;
}

}

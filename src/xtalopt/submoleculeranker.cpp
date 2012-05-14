/****************************************************************************
  SubMoleculeRanker: Evaluate energy of each SubMolecule in a MolecularXtal

  Copyright (C) 2012 by David C. Lonie

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
  more details.
 ****************************************************************************/

#include "submoleculeranker.h"

#include <xtalopt/structures/molecularxtal.h>
#include <xtalopt/structures/submolecule.h>

#include <avogadro/atom.h>
#include <avogadro/bond.h>

#include <openbabel/atom.h>
#include <openbabel/bond.h>
#include <openbabel/forcefield.h>
#include <openbabel/mol.h>
#include <openbabel/obconversion.h>

#include <QtCore/QDebug>
#include <QtCore/QHash>
#include <QtCore/QMutex>

#define DEBUGOUT(_funcnam_) \
  if (debug)    qDebug() << this << " " << _funcnam_ << ": " <<
#define DDEBUGOUT(_funcnam_) \
  if (d->debug) qDebug() << this << " " << _funcnam_ << ": " <<

using namespace OpenBabel;

namespace XtalOpt
{

class SubMoleculeRankerPrivate
{
public:
  SubMoleculeRankerPrivate(SubMoleculeRanker *parent)
    : q_ptr(parent),
      setupMutex(NULL),
      ff(NULL),
      mxtal(NULL),
      vdwCutoff(10.0),
      eleCutoff(10.0),
      imageAtomStartId(-1),
      debug(false)
  {}

  virtual ~SubMoleculeRankerPrivate()
  {
  }

  QHash<unsigned long, unsigned long> mxtal2OBMol; // atom id lookup
  OBMol obmol;
  OBBitVec intraGroup;
  OBBitVec intraGroupCache;
  OBBitVec interGroup;
  OBBitVec interGroupCache;
  OBBitVec originalAtoms;
  OBBitVec allAtoms;
  OBFFConstraints constraints;
  vector3 translations[26];
  QMutex *setupMutex;
  OBForceField *ff;
  MolecularXtal *mxtal;
  double vdwCutoff;
  double eleCutoff;
  int imageAtomStartId; // atom id where image atoms begin
  bool debug;

  bool setupForceField();
  bool initializeMMFF94s();
  bool initializeUFF();
  bool buildSuperCell();
  bool updateSuperCell();
  bool updateSuperCell(const SubMolecule *submol);
  void clearInteractions();
  bool updateInteractions();
  void cacheInteractions();
  void restoreInteractions();
  bool addSubMoleculeInterIntra(const SubMolecule *submol);
  bool addSubMoleculeInter(const SubMolecule *submol);
  bool addSubMoleculeIntra(const SubMolecule *submol);
  bool removeSubMoleculeInterIntra(const SubMolecule *submol);
  bool removeSubMoleculeInter(const SubMolecule *submol);
  bool removeSubMoleculeIntra(const SubMolecule *submol);
  bool addOriginalCellInterIntra();
  bool addOriginalCellInter();
  bool addOriginalCellIntra();
  double evaluateEnergy();
  bool cleanup();

private:
  SubMoleculeRanker * const q_ptr;
  Q_DECLARE_PUBLIC(SubMoleculeRanker);
};

SubMoleculeRanker::SubMoleculeRanker(QObject *parent,
                                     QMutex *setupMutex)
  : QObject(parent), d_ptr(new SubMoleculeRankerPrivate (this))
{
  Q_D(SubMoleculeRanker);
  DDEBUGOUT("ctor") "Created new instance @" << this;
  d->setupMutex = setupMutex;
}

SubMoleculeRanker::~SubMoleculeRanker()
{
  Q_D(SubMoleculeRanker);
  DDEBUGOUT("dtor") "Destroying instance @" << this;

  d->cleanup();

  delete d_ptr;
}

void SubMoleculeRanker::setDebug(bool b)
{
  Q_D(SubMoleculeRanker);
  d->debug = b;
}

bool SubMoleculeRanker::setMXtal(MolecularXtal *mxtal)
{
  Q_D(SubMoleculeRanker);
  if (mxtal == NULL) {
    d->cleanup();
    return true;
  }

  d->mxtal = mxtal;

  if (!d->setupForceField()) {
    qWarning() << "Could not set up forcefield for MXtal" << mxtal->getIDString();
    return false;
  }

  d->buildSuperCell();

  return true;
}

void SubMoleculeRanker::updateGeometry()
{
  Q_D(SubMoleculeRanker);
  DDEBUGOUT("updateGeometry") "Updating entire geometry.";
  d->updateSuperCell();
}

void SubMoleculeRanker::updateGeometry(const SubMolecule *subMol)
{
  Q_D(SubMoleculeRanker);
  DDEBUGOUT("updateGeometry") "Updating geometry for submol" << subMol;
  d->updateSuperCell(subMol);
}

void SubMoleculeRanker::setVDWCutoff(double cutoff)
{
  Q_D(SubMoleculeRanker);
  d->vdwCutoff = cutoff;
}

void SubMoleculeRanker::setElectrostaticCutoff(double cutoff)
{
  Q_D(SubMoleculeRanker);
  d->eleCutoff = cutoff;
}

bool SubMoleculeRanker::debug() const
{
  Q_D(const SubMoleculeRanker);
  return d->debug;
}

MolecularXtal *SubMoleculeRanker::mxtal() const
{
  Q_D(const SubMoleculeRanker);
  return d->mxtal;
}

double SubMoleculeRanker::vdwCutoff() const
{
  Q_D(const SubMoleculeRanker);
  return d->vdwCutoff;
}

double SubMoleculeRanker::electrostaticCutoff() const
{
  Q_D(const SubMoleculeRanker);
  return d->eleCutoff;
}

QVector<double> SubMoleculeRanker::evaluate()
{
  Q_D(const SubMoleculeRanker);
  DDEBUGOUT("evaluate") "Calculating energy of all submolecules:";
  QList<SubMolecule*> submols = d->mxtal->subMolecules();

  // evaluate and energies of each submolecule
  QVector<double> energies;
  energies.reserve(d->mxtal->numSubMolecules());
  foreach(SubMolecule *submol, submols) {
    energies.push_back(this->evaluate(submol));
  }

  return energies;
}

QVector<double> SubMoleculeRanker::evaluateInter()
{
  Q_D(const SubMoleculeRanker);
  DDEBUGOUT("evaluateInter") "Calculating energy of all submolecules:";
  QList<SubMolecule*> submols = d->mxtal->subMolecules();

  // evaluate and energies of each submolecule
  QVector<double> energies;
  energies.reserve(d->mxtal->numSubMolecules());
  foreach(SubMolecule *submol, submols) {
    energies.push_back(this->evaluateInter(submol));
  }

  return energies;
}

QVector<double> SubMoleculeRanker::evaluateIntra()
{
  Q_D(const SubMoleculeRanker);
  DDEBUGOUT("evaluateIntra") "Calculating energy of all submolecules:";
  QList<SubMolecule*> submols = d->mxtal->subMolecules();

  // evaluate and energies of each submolecule
  QVector<double> energies;
  energies.reserve(d->mxtal->numSubMolecules());
  foreach(SubMolecule *submol, submols) {
    energies.push_back(this->evaluateIntra(submol));
  }

  return energies;
}

double SubMoleculeRanker::evaluate(const SubMolecule *subMol)
{
  Q_D(SubMoleculeRanker);
  d->clearInteractions();
  d->addSubMoleculeInterIntra(subMol);
  const double energy = d->evaluateEnergy();
  DDEBUGOUT("evaluate") "SubMolecule" << subMol << "energy" << energy;
  return energy;
}

double SubMoleculeRanker::evaluateInter(const SubMolecule *subMol)
{
  Q_D(SubMoleculeRanker);
  d->clearInteractions();
  d->addSubMoleculeInter(subMol);
  const double energy = d->evaluateEnergy();
  DDEBUGOUT("evaluateInter") "SubMolecule" << subMol << "energy" << energy;
  return energy;
}

double SubMoleculeRanker::evaluateIntra(const SubMolecule *subMol)
{
  Q_D(SubMoleculeRanker);
  d->clearInteractions();
  d->addSubMoleculeIntra(subMol);
  const double energy = d->evaluateEnergy();
  DDEBUGOUT("evaluateIntra") "SubMolecule" << subMol << "energy" << energy;
  return energy;
}

double SubMoleculeRanker::evaluateTotalEnergy(
    const QVector<SubMolecule *> &submols, bool ignore)
{
  Q_D(SubMoleculeRanker);
  d->cacheInteractions();
  d->clearInteractions();
  double energy;

  // Are we limited to only certain submolecules?
  if (submols.size() == 0) {
    if (ignore) {
      d->addOriginalCellInterIntra();
      energy = d->evaluateEnergy();
    }
    else {
      energy = 0.0; // No interactions specified!
    }
  }
  // Set up specific interactions
  else {
    if (ignore) {
      d->addOriginalCellInterIntra();
      foreach (const SubMolecule *sub, submols) {
        d->removeSubMoleculeInterIntra(sub);
      }
    }
    else {
      foreach (const SubMolecule *sub, submols) {
        d->addSubMoleculeInterIntra(sub);
      }
    }
    energy = d->evaluateEnergy();
  }
  d->restoreInteractions();
  DDEBUGOUT("evaluateTotalEnergy") "Energy:" << energy;
  return energy;
}

double SubMoleculeRanker::evaluateTotalEnergyInter(
    const QVector<SubMolecule *> &submols, bool ignore)
{
  Q_D(SubMoleculeRanker);
  d->cacheInteractions();
  d->clearInteractions();
  double energy;

  // Are we limited to only certain submolecules?
  if (submols.size() == 0) {
    if (ignore) {
      d->addOriginalCellInter();
      energy = d->evaluateEnergy();
    }
    else {
      energy = 0.0; // No interactions specified!
    }
  }
  // Set up specific interactions
  else {
    if (ignore) {
      d->addOriginalCellInter();
      foreach (const SubMolecule *sub, submols) {
        d->removeSubMoleculeInter(sub);
      }
    }
    else {
      foreach (const SubMolecule *sub, submols) {
        d->addSubMoleculeInter(sub);
      }
    }
    energy = d->evaluateEnergy();
  }
  d->restoreInteractions();
  DDEBUGOUT("evaluateTotalEnergyInter") "Energy:" << energy;
  return energy;
}

double SubMoleculeRanker::evaluateTotalEnergyIntra(
    const QVector<SubMolecule *> &submols, bool ignore)
{
  Q_D(SubMoleculeRanker);
  d->cacheInteractions();
  d->clearInteractions();
  double energy;

  // Are we limited to only certain submolecules?
  if (submols.size() == 0) {
    if (ignore) {
      d->addOriginalCellIntra();
      energy = d->evaluateEnergy();
    }
    else {
      energy = 0.0; // No interactions specified!
    }
  }
  // Set up specific interactions
  else {
    if (ignore) {
      d->addOriginalCellIntra();
      foreach (const SubMolecule *sub, submols) {
        d->removeSubMoleculeIntra(sub);
      }
    }
    else {
      foreach (const SubMolecule *sub, submols) {
        d->addSubMoleculeIntra(sub);
      }
    }
    energy = d->evaluateEnergy();
  }
  d->restoreInteractions();
  DDEBUGOUT("evaluateTotalEnergyIntra") "Energy:" << energy;
  return energy;
}

/****************************************************************************
 *                     SubMoleculeRankerPrivate methods                     *
 ****************************************************************************/

bool SubMoleculeRankerPrivate::setupForceField()
{
  // Set OBMol so that we can test if the forcefield will work with the
  // target mxtal

  if (this->mxtal == NULL) {
    qWarning() << "Input geometry not set!";
    return false;
  }

  if (this->setupMutex != NULL) {
    this->setupMutex->lock();
  }
  this->obmol = this->mxtal->OBMol();
  if (this->setupMutex != NULL) {
    this->setupMutex->unlock();
  }

  if (!this->initializeMMFF94s()) {
    qWarning() << "Could not initialize MMFF94s force field. Attempting UFF.";
    if (!this->initializeUFF()) {
      qWarning() << "Could not initialize UFF force field. Aborting.";
      return false;
    }
    qWarning() << "Successfully setup UFF forcefield.";
  }

  return true;
}

bool SubMoleculeRankerPrivate::initializeMMFF94s()
{
  // HACK Need to instantiate OBConversion to get FindForceField to work
  OBConversion conv; Q_UNUSED(conv);
  ff = OBForceField::FindForceField("MMFF94s");
  if (!ff) {
    qWarning() << "Cannot locate MMFF94s forcefield.";
    return false;
  }

  // Don't modify the static instance -- make a clone:
  ff = ff->MakeNewInstance();

  if (!ff) {
    qWarning() << "Unable to clone MMFF94s forcefield.";
    return false;
  }

  ff->SetLogFile(NULL);
  ff->SetLogLevel(OBFF_LOGLVL_NONE);
  ff->SetLineSearchType(LineSearchType::Newton2Num);

  // Attempt to setup on obmol
  if (this->setupMutex != NULL) {
    this->setupMutex->lock();
  }
  if (!this->ff->Setup(this->obmol)) {
    if (this->setupMutex != NULL) {
      this->setupMutex->unlock();
    }
    qWarning() << "Cannot setup MMFF94s forcefield on target system.";
    return false;
  }
  if (this->setupMutex != NULL) {
    this->setupMutex->unlock();
  }

  return true;
}

bool SubMoleculeRankerPrivate::initializeUFF()
{
  // HACK Need to instantiate OBConversion to get FindForceField to work
  OBConversion conv; Q_UNUSED(conv);
  ff = OBForceField::FindForceField("UFF");
  if (!ff) {
    qWarning() << "Cannot locate UFF forcefield.";
    return false;
  }

  // Don't modify the static instance -- make a clone:
  ff = ff->MakeNewInstance();

  if (!ff) {
    qWarning() << "Unable to clone UFF forcefield.";
    return false;
  }

  ff->SetLogFile(NULL);
  ff->SetLogLevel(OBFF_LOGLVL_NONE);
  ff->SetLineSearchType(LineSearchType::Newton2Num);

  // Attempt to setup on obmol
  if (this->setupMutex != NULL) {
    this->setupMutex->lock();
  }
  if (!this->ff->Setup(this->obmol)) {
    if (this->setupMutex != NULL) {
      this->setupMutex->unlock();
    }
    qWarning() << "Cannot setup UFF forcefield on target system.";
    return false;
  }
  if (this->setupMutex != NULL) {
    this->setupMutex->unlock();
  }

  return true;
}

bool SubMoleculeRankerPrivate::buildSuperCell()
{
  mxtal2OBMol.clear();
  obmol.Clear();
  //  obmol.BeginModify();
  QList<SubMolecule*> subs = mxtal->subMolecules();
  for (QList<SubMolecule*>::const_iterator it = subs.constBegin(),
       it_end = subs.constEnd(); it != it_end; ++it) {
    QVector<Eigen::Vector3d> cohVecs = (*it)->getCoherentCoordinates();
    for (int i = 0; i < cohVecs.size(); ++i) {
      Avogadro::Atom *atom = (*it)->atom(i);
      const Eigen::Vector3d *pos = &cohVecs[i];
      OBAtom *obatom = obmol.NewAtom();
      obatom->SetAtomicNum(atom->atomicNumber());
      obatom->SetVector(pos->x(), pos->y(), pos->z());
      mxtal2OBMol.insert(atom->id(), obatom->GetId());
    }
    for (int i = 0; i < (*it)->numBonds(); ++i) {
      Avogadro::Bond *bond = (*it)->bond(i);
      OBBond *obbond = obmol.NewBond();
      obbond->SetBondOrder(bond->order());
      obbond->SetBegin(obmol.GetAtomById(
                         mxtal2OBMol.value(bond->beginAtomId())));
      obbond->SetEnd(  obmol.GetAtomById(
                         mxtal2OBMol.value(bond->endAtomId())));
    }
  }

  // Add images
  imageAtomStartId = obmol.NumAtoms();
  int numBonds = obmol.NumBonds();
  for (int i = 0; i < imageAtomStartId; ++i) {
    OBAtom *refAtom = obmol.GetAtomById(i);
    for (int j = 0; j < 26; ++j) { // 26 = 3x3x3 unit cells - 1
      OBAtom *newAtom = obmol.NewAtom();
      // Only set atomic num now, positions are set in updateSuperCell
      newAtom->SetAtomicNum(refAtom->GetAtomicNum());
      constraints.AddAtomConstraint(newAtom->GetIdx());
    }
  }
  for (int i = 0; i < numBonds; ++i) {
    OBBond *refBond = obmol.GetBondById(i);
    for (int j = 0; j < 26; ++j) { // 26 = 3x3x3 unit cells - 1
      OBBond *newBond = obmol.NewBond();
      unsigned long beginId = imageAtomStartId + j + 26 *
          refBond->GetBeginAtom()->GetId();
      unsigned long endId =   imageAtomStartId + j + 26 *
          refBond->GetEndAtom()->GetId();
      newBond->SetBegin(obmol.GetAtomById(beginId));
      newBond->SetEnd(  obmol.GetAtomById(endId));
    }
  }

  // Prevent openbabel from attempting to reperceive hybridization, etc
  //  obmol.EndModify();
  //  obmol.SetHybridizationPerceived();

  // Now the translations...
  std::vector<vector3> obvecs = mxtal->OBUnitCell()->GetCellVectors();
  int imageArray = -1;
  for (double i = -1; i < 1.5; ++i) {
    for (double j = -1; j < 1.5; ++j) {
      for (double k = -1; k < 1.5; ++k) {
        if (fabs(i) < 1e-10 && fabs(j) < 1e-10 && fabs(k) < 1e-10) {
          continue;
        }
        translations[++imageArray] =
            i * obvecs[0] + j * obvecs[1] + k * obvecs[2];
      }
    }
  }

  // Update the forcefield with the expanded system
  // Attempt to setup on obmol
  if (this->setupMutex != NULL) {
    this->setupMutex->lock();
  }
  if (!this->ff->Setup(this->obmol)) {
    if (this->setupMutex != NULL) {
      this->setupMutex->unlock();
    }
    qWarning() << "Cannot setup the forcefield on the expanded mxtal.";
    return false;
  }
  if (this->setupMutex != NULL) {
    this->setupMutex->unlock();
  }

  // Initialize bitplanes
  this->intraGroup.Resize(obmol.NumAtoms());
  this->intraGroup.Clear();
  this->interGroup = this->intraGroup;
  this->originalAtoms = this->intraGroup;
  this->originalAtoms.SetRangeOn(1, this->imageAtomStartId);
  this->allAtoms = this->intraGroup;
  this->allAtoms.SetRangeOn(1, obmol.NumAtoms());

  this->updateSuperCell();

  return true;
}

bool SubMoleculeRankerPrivate::updateSuperCell()
{
  Q_Q(SubMoleculeRanker);

  // Grab list of obmol ids (in the original cell, not images) that will need
  // updating. Image ids are calculated in the loop. Update the original cell
  // in this loop as well
  QVector<unsigned long> obmolIds;
  obmolIds.reserve(mxtal->numAtoms());
  foreach (const Avogadro::Atom *atom, mxtal->atoms()) {
    unsigned long id = mxtal2OBMol.value(atom->id(), -1);
    if (id == -1) {
      qWarning() << "MolecularXtal contains an unrecognized id.";
      return false;
    }
    obmolIds.push_back(id);
    const Eigen::Vector3d *pos = atom->pos();
    obmol.GetAtomById(id)->SetVector(pos->x(), pos->y(), pos->z());
  }

  // Update each image with the appropriate translation
  for (int i = 0; i < imageAtomStartId; ++i) {
    const vector3 &vec = obmol.GetAtomById(i)->GetVector();
    int transIndex = -1;
    for (int imageId = imageAtomStartId + i * 26,
         imageId_end = imageId + 26; imageId < imageId_end; ++imageId) {
      obmol.GetAtomById(imageId)->SetVector(vec + translations[++transIndex]);
    }
  }

  // Update the coordinates in the forcefield's obmol copy.
  if (!this->ff->SetConformers(this->obmol)) {
    qWarning() << "Unable to update forcefield's cached molecule.";
    return false;
  }

  // Force an update of the cutoffs
  this->ff->UpdatePairsSimple();

  return true;
}

bool SubMoleculeRankerPrivate::updateSuperCell(const SubMolecule *submol)
{
  Q_Q(SubMoleculeRanker);

  if (!this->mxtal->subMolecules().contains(const_cast<SubMolecule*>(submol))) {
    qWarning() << "MolecularXtal does not contain submol!";
    return false;
  }

  // Grab list of obmol ids (in the original cell, not images) that will need
  // updating. Image ids are calculated in the loop. Update the original cell
  // in this loop as well
  QVector<unsigned long> obmolIds;
  obmolIds.reserve(submol->numAtoms());
  foreach (const Avogadro::Atom *atom, submol->atoms()) {
    unsigned long id = mxtal2OBMol.value(atom->id(), -1);
    if (id == -1) {
      qWarning() << "SubMolecule contains an unrecognized id.";
      return false;
    }
    obmolIds.push_back(id);
    const Eigen::Vector3d *pos = atom->pos();
    obmol.GetAtomById(id)->SetVector(pos->x(), pos->y(), pos->z());
  }

  // Update each image with the appropriate translation
  foreach (unsigned long id, obmolIds) {
    const vector3 &vec = obmol.GetAtomById(id)->GetVector();
    int transIndex = -1;
    for (int imageId = imageAtomStartId + id * 26,
         imageId_end = imageId + 26; imageId < imageId_end; ++imageId) {
      obmol.GetAtomById(imageId)->SetVector(vec + translations[++transIndex]);
    }
  }

  // Update the coordinates in the forcefield's obmol copy.
  if (!this->ff->SetConformers(this->obmol)) {
    qWarning() << "Unable to update forcefield's cached molecule.";
    return false;
  }

  // Force an update of the cutoffs
  this->ff->UpdatePairsSimple();

  return true;
}

void SubMoleculeRankerPrivate::clearInteractions()
{
  this->ff->ClearGroups();
  this->interGroup.Clear();
  this->intraGroup.Clear();
}

bool SubMoleculeRankerPrivate::updateInteractions()
{
  // Set interaction groups
  ff->ClearGroups();
  ff->AddIntraGroup(this->intraGroup);
  ff->AddInterGroups(this->interGroup, this->allAtoms);

  // Reset forcefield setup
  if (this->setupMutex != NULL) {
    this->setupMutex->lock();
  }
  if (!ff->Setup(obmol, constraints)) {
    if (this->setupMutex != NULL) {
      this->setupMutex->unlock();
    }
    qWarning() << "Cannot setup forcefield on obmol.";
    return false;
  }
  if (this->setupMutex != NULL) {
    this->setupMutex->unlock();
  }

  return true;
}

void SubMoleculeRankerPrivate::cacheInteractions()
{
  this->intraGroupCache = this->intraGroup;
  this->interGroupCache = this->interGroup;
}

void SubMoleculeRankerPrivate::restoreInteractions()
{
  this->intraGroup = this->intraGroupCache;
  this->interGroup = this->interGroupCache;
}

bool SubMoleculeRankerPrivate::addSubMoleculeInterIntra(const SubMolecule *submol)
{
  Q_ASSERT(this->mxtal->subMolecules().contains(
             const_cast<SubMolecule *>(submol)));
  // Grab list of obmol ids
  QVector<unsigned long> obmolIds;
  obmolIds.reserve(submol->numAtoms());
  foreach (const Avogadro::Atom *atom, submol->atoms()) {
    obmolIds.push_back(mxtal2OBMol.value(atom->id(), -1));
  }

  // Create bitplanes for the submolecule of interest
  foreach (unsigned long id, obmolIds) {
    if (id == static_cast<unsigned long>(-1)) {
      qWarning() << "Bad id. Cannot set submolecule.";
      return false;
    }
    this->intraGroup.SetBitOn(id+1);
    this->interGroup.SetBitOn(id+1);
  }

  return this->updateInteractions();
}

bool SubMoleculeRankerPrivate::addSubMoleculeInter(const SubMolecule *submol)
{
  Q_ASSERT(this->mxtal->subMolecules().contains(
             const_cast<SubMolecule *>(submol)));
  // Grab list of obmol ids
  QVector<unsigned long> obmolIds;
  obmolIds.reserve(submol->numAtoms());
  foreach (const Avogadro::Atom *atom, submol->atoms()) {
    obmolIds.push_back(mxtal2OBMol.value(atom->id(), -1));
  }

  // Create bitplanes for the submolecule of interest
  foreach (unsigned long id, obmolIds) {
    if (id == static_cast<unsigned long>(-1)) {
      qWarning() << "Bad id. Cannot set submolecule.";
      return false;
    }
    this->interGroup.SetBitOn(id+1);
  }

  return this->updateInteractions();
}

bool SubMoleculeRankerPrivate::addSubMoleculeIntra(const SubMolecule *submol)
{
  Q_ASSERT(this->mxtal->subMolecules().contains(
             const_cast<SubMolecule *>(submol)));
  // Grab list of obmol ids
  QVector<unsigned long> obmolIds;
  obmolIds.reserve(submol->numAtoms());
  foreach (const Avogadro::Atom *atom, submol->atoms()) {
    obmolIds.push_back(mxtal2OBMol.value(atom->id(), -1));
  }

  // Create bitplanes for the submolecule of interest
  foreach (unsigned long id, obmolIds) {
    if (id == static_cast<unsigned long>(-1)) {
      qWarning() << "Bad id. Cannot set submolecule.";
      return false;
    }
    this->intraGroup.SetBitOn(id+1);
  }

  return this->updateInteractions();
}

bool SubMoleculeRankerPrivate::removeSubMoleculeInterIntra(
    const SubMolecule *submol)
{
  Q_ASSERT(this->mxtal->subMolecules().contains(
             const_cast<SubMolecule *>(submol)));
  // Grab list of obmol ids
  QVector<unsigned long> obmolIds;
  obmolIds.reserve(submol->numAtoms());
  foreach (const Avogadro::Atom *atom, submol->atoms()) {
    obmolIds.push_back(mxtal2OBMol.value(atom->id(), -1));
  }

  // Create bitplanes for the submolecule of interest
  foreach (unsigned long id, obmolIds) {
    if (id == static_cast<unsigned long>(-1)) {
      qWarning() << "Bad id. Cannot set submolecule.";
      return false;
    }
    this->intraGroup.SetBitOff(id+1);
    this->interGroup.SetBitOff(id+1);
  }

  return this->updateInteractions();
}

bool SubMoleculeRankerPrivate::removeSubMoleculeInter(
    const SubMolecule *submol)
{
  Q_ASSERT(this->mxtal->subMolecules().contains(
             const_cast<SubMolecule *>(submol)));
  // Grab list of obmol ids
  QVector<unsigned long> obmolIds;
  obmolIds.reserve(submol->numAtoms());
  foreach (const Avogadro::Atom *atom, submol->atoms()) {
    obmolIds.push_back(mxtal2OBMol.value(atom->id(), -1));
  }

  // Create bitplanes for the submolecule of interest
  foreach (unsigned long id, obmolIds) {
    if (id == static_cast<unsigned long>(-1)) {
      qWarning() << "Bad id. Cannot set submolecule.";
      return false;
    }
    this->interGroup.SetBitOff(id+1);
  }

  return this->updateInteractions();
}

bool SubMoleculeRankerPrivate::removeSubMoleculeIntra(
    const SubMolecule *submol)
{
  Q_ASSERT(this->mxtal->subMolecules().contains(
             const_cast<SubMolecule *>(submol)));
  // Grab list of obmol ids
  QVector<unsigned long> obmolIds;
  obmolIds.reserve(submol->numAtoms());
  foreach (const Avogadro::Atom *atom, submol->atoms()) {
    obmolIds.push_back(mxtal2OBMol.value(atom->id(), -1));
  }

  // Create bitplanes for the submolecule of interest
  foreach (unsigned long id, obmolIds) {
    if (id == static_cast<unsigned long>(-1)) {
      qWarning() << "Bad id. Cannot set submolecule.";
      return false;
    }
    this->intraGroup.SetBitOff(id+1);
  }

  return this->updateInteractions();
}

bool SubMoleculeRankerPrivate::addOriginalCellInterIntra()
{
  this->interGroup = this->originalAtoms;
  this->intraGroup = this->originalAtoms;
  return this->updateInteractions();
}

bool SubMoleculeRankerPrivate::addOriginalCellInter()
{
  this->interGroup = this->originalAtoms;
  return this->updateInteractions();
}

bool SubMoleculeRankerPrivate::addOriginalCellIntra()
{
  this->intraGroup = this->originalAtoms;
  return this->updateInteractions();
}

double SubMoleculeRankerPrivate::evaluateEnergy()
{
  return this->ff->Energy();
}

bool SubMoleculeRankerPrivate::cleanup()
{
  mxtal2OBMol.clear();
  obmol.Clear();
  constraints.Clear();
  delete ff;
  ff = NULL;
  return true;
}

}


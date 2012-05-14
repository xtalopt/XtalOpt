/****************************************************************************
  MolecularXtalOptimizer: Forcefield geometry optimizations of MolecularXtals

  Copyright (C) 2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
  more details.
 ****************************************************************************/

#include "molecularxtaloptimizer.h"

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
#include <QtCore/QThread>

#include <QtGui/QApplication>

#define DEBUGOUT(_funcnam_) \
  if (debug)    qDebug() << this << " " << _funcnam_ << ": " <<
#define DDEBUGOUT(_funcnam_) \
  if (d->debug) qDebug() << this << " " << _funcnam_ << ": " <<

using namespace OpenBabel;

namespace XtalOpt
{

class MolecularXtalOptimizerPrivate
{
public:
  MolecularXtalOptimizerPrivate(MolecularXtalOptimizer *parent)
    : q_ptr(parent),
      setupMutex(NULL),
      ff(NULL),
      mxtal(NULL),
      sCUpdateInterval(20),
      currentStep(0),
      numSteps(500),
      conv(1e-4),
      vdwCutoff(2.0),
      eleCutoff(2.0),
      cutoffUpdateInterval(-1),
      lastcutoffUpdate(0),
      imageAtomStartId(-1),
      converged(false),
      exceededSteps(false),
      aborted(false),
      debug(false)
  {}

  virtual ~MolecularXtalOptimizerPrivate()
  {
  }

  QHash<unsigned long, unsigned long> mxtal2OBMol; // atom id lookup
  OBMol obmol;
  OBFFConstraints constraints;
  vector3 translations[26];
  QMutex abortMutex;
  QMutex runningMutex;
  QMutex *setupMutex;
  OBForceField *ff;
  MolecularXtal *mxtal;
  int sCUpdateInterval;
  int currentStep;
  int numSteps;
  double conv;
  double vdwCutoff;
  double eleCutoff;
  int cutoffUpdateInterval;
  int lastcutoffUpdate;
  int imageAtomStartId; // atom id where image atoms begin
  bool converged;
  bool exceededSteps;
  bool aborted;
  bool debug;

  bool setup();
  bool initializeMMFF94s();
  bool initializeUFF();
  bool precalcSetup();
  bool cleanupUnitCell();
  bool updateSuperCell();
  bool updateCutoffsIfNeeded(bool force = false); // True if update performed
  bool optimizationIsComplete();
  void runUntilNextUpdate(int steps);
  bool cleanup();
  void releaseMXtal();

private:
  MolecularXtalOptimizer * const q_ptr;
  Q_DECLARE_PUBLIC(MolecularXtalOptimizer);
};

MolecularXtalOptimizer::MolecularXtalOptimizer(QObject *parent, QMutex *mut) :
  QObject(parent), d_ptr(new MolecularXtalOptimizerPrivate (this))
{
  Q_D(MolecularXtalOptimizer);
  DDEBUGOUT("ctor") "Created new instance @" << this;
  d->setupMutex = mut;
}

MolecularXtalOptimizer::~MolecularXtalOptimizer()
{
  Q_D(MolecularXtalOptimizer);
  DDEBUGOUT("dtor") "Destroying instance @" << this;

  d->cleanup();

  delete d_ptr;
}

void MolecularXtalOptimizer::setDebug(bool b)
{
  Q_D(MolecularXtalOptimizer);
  d->debug = b;
}

void MolecularXtalOptimizer::setMXtal(MolecularXtal *mxtal)
{
  Q_D(MolecularXtalOptimizer);
  d->mxtal = mxtal;
}

void MolecularXtalOptimizer::setSuperCellUpdateInterval(int interval)
{
  Q_D(MolecularXtalOptimizer);
  d->sCUpdateInterval = interval;
}

void MolecularXtalOptimizer::setNumberOfGeometrySteps(int steps)
{
  Q_D(MolecularXtalOptimizer);
  d->numSteps = steps;
}

void MolecularXtalOptimizer::setEnergyConvergence(double conv)
{
  Q_D(MolecularXtalOptimizer);
  d->conv = conv;
}

void MolecularXtalOptimizer::setVDWCutoff(double cutoff)
{
  Q_D(MolecularXtalOptimizer);
  d->vdwCutoff = cutoff;
}

void MolecularXtalOptimizer::setElectrostaticCutoff(double cutoff)
{
  Q_D(MolecularXtalOptimizer);
  d->eleCutoff = cutoff;
}

void MolecularXtalOptimizer::setCutoffUpdateInterval(int interval)
{
  Q_D(MolecularXtalOptimizer);
  d->cutoffUpdateInterval = interval;
}

bool MolecularXtalOptimizer::debug() const
{
  Q_D(const MolecularXtalOptimizer);
  return d->debug;
}

bool MolecularXtalOptimizer::setup()
{
  Q_D(MolecularXtalOptimizer);
  if (!d->setup()) {
    DDEBUGOUT("setup") "Setup failed!";
    d->cleanup();
    return false;
  }
  return true;
}

void MolecularXtalOptimizer::run()
{
  Q_D(MolecularXtalOptimizer);
  QMutexLocker runningLocker (&d->runningMutex);
  emit startingOptimization();
  DDEBUGOUT("run") "Starting optimization.";


  if (!d->cleanupUnitCell()) {
    DDEBUGOUT("run") "Unit Cell cleanup failed!";
    d->cleanup();
    emit finishedOptimization();
    return;
  }
  if (!d->updateSuperCell()) {
    DDEBUGOUT("run") "Super Cell update failed!";
    d->cleanup();
    emit finishedOptimization();
    return;
  }
  if (!d->precalcSetup()) {
    DDEBUGOUT("run") "Calculation setup failed!";
    d->cleanup();
    emit finishedOptimization();
    return;
  }

  while (!d->optimizationIsComplete()) {
    int stepsRemaining = d->numSteps - d->currentStep;
    int stepsToTake = (stepsRemaining < d->sCUpdateInterval)
        ? stepsRemaining : d->sCUpdateInterval;
    d->runUntilNextUpdate(stepsToTake);

    if (d->currentStep >= d->numSteps) {
      DDEBUGOUT("run") "Step limit exceeded.";
      d->exceededSteps = true;
      break;
    }

    d->abortMutex.lock();
    bool aborted = d->aborted;
    d->abortMutex.unlock();
    if (aborted) {
      DDEBUGOUT("run") "Aborted.";
      break;
    }

    DDEBUGOUT("run") "Updating Super Cell on step" << d->currentStep;
    if (!d->cleanupUnitCell()) {
      DDEBUGOUT("run") "Unit Cell cleanup failed!";
      d->cleanup();
      emit finishedOptimization();
      return;
    }
    if (!d->updateSuperCell()) {
      DDEBUGOUT("run") "Super Cell update failed!";
      d->cleanup();
      emit finishedOptimization();
      return;
    }
  }

  d->ff->GetCoordinates(d->obmol);
  d->obmol.SetEnergy(d->ff->Energy(false));

  emit finishedOptimization();
  return;
}

bool MolecularXtalOptimizer::waitForFinished(int ms_timeout) const
{
  Q_D(const MolecularXtalOptimizer);

  /* The runningMutex is locked at the beginning of run() and released when
   * run() returns. So just lock it and release it to make this thread sleep
   * until the calc is finished/aborts. */

  QMutex *mutableRunningMutex = const_cast<QMutex*>(&d->runningMutex);
  if (mutableRunningMutex->tryLock(ms_timeout)) {
    mutableRunningMutex->unlock();
    return true;
  }
  return false;
}

void MolecularXtalOptimizer::abort()
{
  Q_D(MolecularXtalOptimizer);
  d->abortMutex.lock();
  d->aborted = true;
  d->abortMutex.unlock();
}

bool MolecularXtalOptimizer::isRunning() const
{
  Q_D(const MolecularXtalOptimizer);
  QMutex *mutableRunningMutex = const_cast<QMutex*>(&d->runningMutex);
  if (mutableRunningMutex->tryLock()) {
    // Can lock mutex -- not running
    mutableRunningMutex->unlock();
    return false;
  }
  return true;
}

bool MolecularXtalOptimizer::isConverged() const
{
  Q_D(const MolecularXtalOptimizer);
  return d->converged;
}

bool MolecularXtalOptimizer::reachedStepLimit() const
{
  Q_D(const MolecularXtalOptimizer);
  return d->exceededSteps;
}

bool MolecularXtalOptimizer::isAborted() const
{
  Q_D(const MolecularXtalOptimizer);
  QMutex *mutableAbortMutex = const_cast<QMutex*>(&d->abortMutex);
  mutableAbortMutex->lock();
  bool ret = d->aborted;
  mutableAbortMutex->unlock();
  return ret;
}

void MolecularXtalOptimizer::updateMXtalCoords() const
{
  Q_D(const MolecularXtalOptimizer);
  if (d->mxtal == NULL) {
    qWarning() << "No mxtal set -- cannot update.";
    return;
  }

  QList<Avogadro::Atom*> atoms = d->mxtal->atoms();
  for (QList<Avogadro::Atom*>::const_iterator it = atoms.constBegin(),
      it_end = atoms.constEnd(); it != it_end; ++it) {
    unsigned long obid = d->mxtal2OBMol.value((*it)->id());
    OBAtom * obatom = d->obmol.GetAtomById(obid);
    if (obatom == NULL) {
      qWarning() << "Cannot update mxtal from OBMol. OBMol has no atom with "
                    "id" << obid;
      return;
    }
    (*it)->setPos(Eigen::Vector3d(obatom->GetVector().AsArray()));
  }
}

void MolecularXtalOptimizer::updateMXtalEnergy() const
{
  Q_D(const MolecularXtalOptimizer);
  if (d->mxtal == NULL) {
    qWarning() << "No mxtal set -- cannot update.";
    return;
  }

  d->mxtal->setEnergy(d->obmol.GetEnergy() * KCAL_TO_KJ);
}

void MolecularXtalOptimizer::releaseMXtal()
{
  Q_D(MolecularXtalOptimizer);
  d->releaseMXtal();
}

MolecularXtal * MolecularXtalOptimizer::mxtal() const
{
  Q_D(const MolecularXtalOptimizer);
  return d->mxtal;
}

int MolecularXtalOptimizer::superCellUpdateInterval() const
{
  Q_D(const MolecularXtalOptimizer);
  return d->sCUpdateInterval;
}

int MolecularXtalOptimizer::numberOfGeometrySteps() const
{
  Q_D(const MolecularXtalOptimizer);
  return d->numSteps;
}

int MolecularXtalOptimizer::energyConvergence() const
{
  Q_D(const MolecularXtalOptimizer);
  return d->conv;
}

double MolecularXtalOptimizer::vdwCutoff() const
{
  Q_D(const MolecularXtalOptimizer);
  return d->vdwCutoff;
}

double MolecularXtalOptimizer::electrostaticCutoff() const
{
  Q_D(const MolecularXtalOptimizer);
  return d->eleCutoff;
}

int MolecularXtalOptimizer::cutoffUpdateInterval() const
{
  Q_D(const MolecularXtalOptimizer);
  return d->cutoffUpdateInterval;
}

void MolecularXtalOptimizer::printSelf() const
{
  Q_D(const MolecularXtalOptimizer);
  // TODO
}

bool MolecularXtalOptimizerPrivate::setup()
{
  Q_Q(MolecularXtalOptimizer);

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

  //
  /// Force field is setup -- now for the geometry
  //

  mxtal->m_mxtalOpt = q; // q is a PIMPL pointer to the parent
  mxtal->m_preOptStepCount = this->numSteps;
  mxtal->m_preOptStep = 0;
  mxtal->connect(q, SIGNAL(finishedGeometryStep(int)),
                 SLOT(setPreOptStep(int)), Qt::DirectConnection);

  currentStep = 0;

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

  return true;
}

bool MolecularXtalOptimizerPrivate::initializeMMFF94s()
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

bool MolecularXtalOptimizerPrivate::initializeUFF()
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

bool MolecularXtalOptimizerPrivate::cleanupUnitCell()
{
  // Left open in case some processing is needed here, e.g. reselect which
  // images of which molecules are kept.
  return true;
}

bool MolecularXtalOptimizerPrivate::updateSuperCell()
{
  Q_Q(MolecularXtalOptimizer);

  // Get the current coordinates of the optimized system
  if (this->currentStep != 0) {
    this->ff->GetCoordinates(this->obmol);
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

  // If we haven't started the optimization yet and are still doing setup,
  // just return. Cutoffs will be updated during the minimizer initialization
  if (this->currentStep == 0) {
    return true;
  }

  // Update the coordinates in the forcefield's obmol copy.
  if (!this->ff->SetConformers(this->obmol)) {
    qWarning() << "Unable to update forcefield's cached molecule.";
    return false;
  }

  // Force an update of the cutoffs and reset the search direction
  ff->ConjugateGradientsInitialize(this->numSteps - this->currentStep,
                                   this->conv);
  this->lastcutoffUpdate = this->currentStep;

  DEBUGOUT("updateSuperCell")
      QString("Cutoffs updated, direction reset. step %1: vdw=%2 (%3 pairs), "
              "ele=%4 (%5 pairs)")
      .arg(this->currentStep)
      .arg(ff->GetVDWCutOff())
      .arg(ff->GetNumVDWPairs())
      .arg(ff->GetElectrostaticCutOff())
      .arg(ff->GetNumElectrostaticPairs());

  return true;
}

bool MolecularXtalOptimizerPrivate::updateCutoffsIfNeeded(bool force)
{
  if (!force && // Always update when forced
      ( ( this->cutoffUpdateInterval < 0 ||
          (this->currentStep - this->lastcutoffUpdate) <
          this->cutoffUpdateInterval
          ) || // Skip if too soon, or
        this->currentStep % this->sCUpdateInterval == 0 || // about to update SC
        this->currentStep == this->numSteps ) // or if just finished last step
      ) {
    return false;
  }

  ff->UpdatePairsSimple();
  this->lastcutoffUpdate = this->currentStep;

  DEBUGOUT("updateCutoffs")
      QString("Cutoffs updated, step %1: vdw=%2 (%3 pairs), ele=%4 (%5 pairs)")
      .arg(this->currentStep)
      .arg(ff->GetVDWCutOff())
      .arg(ff->GetNumVDWPairs())
      .arg(ff->GetElectrostaticCutOff())
      .arg(ff->GetNumElectrostaticPairs());

  return true;
}

bool MolecularXtalOptimizerPrivate::cleanup()
{
  mxtal2OBMol.clear();
  obmol.Clear();
  constraints.Clear();
  delete ff;
  ff = NULL;
  this->releaseMXtal();
  converged = false;
  exceededSteps = false;
  return true;
}

void MolecularXtalOptimizerPrivate::releaseMXtal()
{
  Q_Q(MolecularXtalOptimizer);
  // Remove the reference to this from the mxtal
  if (this->mxtal != NULL && this->mxtal->m_mxtalOpt == q) {
    this->mxtal->m_mxtalOpt = NULL;
  }
}

bool MolecularXtalOptimizerPrivate::optimizationIsComplete()
{
  return this->converged || this->exceededSteps;
}

bool MolecularXtalOptimizerPrivate::precalcSetup()
{
  // Abort if needed
  this->abortMutex.lock();
  bool needAbort = this->aborted;
  this->abortMutex.unlock();
  if (needAbort) {
    DEBUGOUT("run") "Aborting calculation.";
    return false;
  }

  Q_Q(MolecularXtalOptimizer);
  // Set electrostatic and VDW cutoffs
  ff->EnableCutOff(true);
  ff->SetElectrostaticCutOff(this->eleCutoff);
  ff->SetVDWCutOff(this->vdwCutoff);
  ff->SetUpdateFrequency(10000); // Cutoffs are updated manually.

  // These bitvectors use OBAtom::Idx(), which runs from 1 -> numAtoms()
  OBBitVec origCell (obmol.NumAtoms());
  origCell.SetRangeOn(1, imageAtomStartId);
  OBBitVec allAtoms (obmol.NumAtoms());
  allAtoms.SetRangeOn(1, obmol.NumAtoms());

  // Only calculate intramolecular penalties on our original unit cell
  ff->ClearGroups();
  ff->AddIntraGroup(origCell);

  // Don't calculate penalties for pairs of atoms that are both outside of the
  // original unit cell
  ff->AddInterGroups(origCell, allAtoms);

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

  // This call also initialized the cutoffs
  ff->ConjugateGradientsInitialize(this->numSteps, this->conv);

  DEBUGOUT("precalc")
      QString("Cutoffs updated, step %1: vdw=%2 (%3 pairs), ele=%4 (%5 pairs)")
      .arg(this->currentStep)
      .arg(ff->GetVDWCutOff())
      .arg(ff->GetNumVDWPairs())
      .arg(ff->GetElectrostaticCutOff())
      .arg(ff->GetNumElectrostaticPairs());

  return true;
}

void MolecularXtalOptimizerPrivate::runUntilNextUpdate(int steps)
{
  Q_Q(MolecularXtalOptimizer);

  // Abort if needed
  this->abortMutex.lock();
  bool needAbort = this->aborted;
  this->abortMutex.unlock();
  if (needAbort) {
    DEBUGOUT("cycle") "Aborting calculation.";
    return;
  }

  // Pause to emit signal and check for abort every 5 steps
  bool canTakeMoreSteps = true;
  int stepsTaken = 0;
  int stepsPerSignal = 5; // How many step to take before emitting the signal?
  double lastEnergy = ff->Energy(false);
  while (canTakeMoreSteps && stepsTaken < steps) {
    canTakeMoreSteps = ff->ConjugateGradientsTakeNSteps(stepsPerSignal);
    stepsTaken += stepsPerSignal;
    this->currentStep += stepsPerSignal;
    emit q->finishedGeometryStep(this->currentStep);
    double energy = ff->Energy(false);
    DEBUGOUT("cycle") QString("Completed step %1 of %2. Energy = %3, dE = %4 "
                              "dE/step = %5")
        .arg(currentStep).arg(numSteps).arg(energy).arg(energy - lastEnergy)
        .arg((energy - lastEnergy)/static_cast<double>(stepsPerSignal));
    lastEnergy = energy;

    // Abort if needed
    this->abortMutex.lock();
    bool needAbort = this->aborted;
    this->abortMutex.unlock();
    if (needAbort) {
      DEBUGOUT("cycle") "Aborting calculation.";
      return;
    }

    // Update cutoffs if it's time
    this->updateCutoffsIfNeeded();

    // Let the GUI catch up with a 1 second timeout
    qApp->processEvents(QEventLoop::AllEvents, 1000);
    QThread::yieldCurrentThread();
  }

  if (stepsTaken < steps) {
    DEBUGOUT("cycle") "Convergence reached in" << currentStep << "steps.";
    this->converged = true;
  }

  return;
}

}

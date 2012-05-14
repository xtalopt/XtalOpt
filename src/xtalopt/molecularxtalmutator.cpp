/****************************************************************************
  MolecularXtalMutator -- Perform smart mutations on MolecularXtals

  Copyright (C) 2012 by David C. Lonie

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
  more details.
 ****************************************************************************/

#include "molecularxtalmutator.h"

#include <xtalopt/holefinder.h>
#include <xtalopt/molecularxtalsupercellgenerator.h>
#include <xtalopt/structures/molecularxtal.h>
#include <xtalopt/structures/submolecule.h>
#include <xtalopt/submoleculeranker.h>

#include <globalsearch/macros.h>
#include <globalsearch/obeigenconv.h>

#include <Eigen/Core>

#include <QtCore/QDebug>
#include <QtCore/QMap>
#include <QtCore/QMutex>
#include <QtCore/QThread>
#include <QtCore/QVector>

#include <QtGui/QApplication>

#define DEBUGOUT(_funcnam_) \
  if (debug)    qDebug() << this << " " << _funcnam_ << ": " <<
#define DDEBUGOUT(_funcnam_) \
  if (d->debug) qDebug() << this << " " << _funcnam_ << ": " <<

using Avogadro::Eigen2OB;
using Avogadro::OB2Eigen;

namespace XtalOpt
{

/****************************************************************************
 * typedefs                                                                 *
 ****************************************************************************/

struct Transform_t
{
  double pos[3];
  double axis[3];
  double angle;
};

union ScanVar_t
{
  double real;
  int integer;
  Transform_t transform;
  double matrix3x3[9]; // row-major
};

enum ScanVarType
{
  REAL = 0,
  INTEGER,
  TRANSFORM,
  MATRIX3x3
};

typedef QMap<double, ScanVar_t> ScanMap;
typedef QMap<double, SubMolecule*> RankMap;
typedef QVector<double> ProbabilityVector;

/****************************************************************************
 * MolecularXtalMutatorPrivate class definition                             *
 ****************************************************************************/

class MolecularXtalMutatorPrivate
{
public:
  MolecularXtalMutatorPrivate(MolecularXtalMutator *parent)
    : q_ptr(parent),
      setupMutex(NULL),
      ranker(NULL), // Created when setupMutex is set
      aborted(false),
      debug(false),
      parentEnergy(0.0),
      offspringEnergy(0.0),
      createBest(false),
      startWithSuperCell(false),
      numCandidates(10),
      numStrains(10),
      numMovers(2),
      numDisplacements(10),
      rotationResRad(30*DEG_TO_RAD),
      numVolumeSamples(10),
      minVolumeFrac(0.75),
      maxVolumeFrac(1.25),
      numMutationSteps(0),
      currentStep(0)

  {
    this->strainSigmaRange[0] = 0.0;
    this->strainSigmaRange[1] = 0.5;
  }

  virtual ~MolecularXtalMutatorPrivate();

  /////////////
  // Methods //
  /////////////

  /////////////////
  // Helper methods
  //
  // Used to cache structure data during scans
  void copyCellAndCoords(const MolecularXtal *from, MolecularXtal *to);
  void cacheWorkingMXtal();
  void restoreWorkingMXtal();
  /// Probability calculators -- input energies must be sorted low->high
  // Get probabilities, favoring high energies
  ProbabilityVector getProbabilitiesHigh(const QList<double> &energies);
  // Get probabilities, favoring low energies
  ProbabilityVector getProbabilitiesLow(const QList<double> &energies);
  //
  /// Data generators
  // Get a random index into a ProbabilityVector
  int getRandomIndex(const ProbabilityVector &probs);
  // Get a random number from a range:
  double getRandomNumberFromRange(double min, double max);
  // Get a random voight matrix with random elements selected from a gaussian
  // distribution centered at zero with a width of sigma.
  Eigen::Matrix3d generateRandomVoightMatrix(double sigma);
  //
  /// Clean-up scans by removing bad data
  // Remove high energy scan data
  void trimHighEnergyScanData();
  // Remove low energy scan data
  void trimLowEnergyScanData();
  //
  /// Fill data structures
  // Rank the submolecules and populate rankMap, omitting those in 'omit'
  void rankSubMolecules(const QVector<SubMolecule *> &omit = QVector<SubMolecule*>());

  ///////////////////
  // Scanning Methods
  ///
  /// Must use a subMolecule from workingMXtal.
  /// workingMXtal will be modified during the course of the scan,
  /// but will be reset to starting geometry when finished
  /// These will use the scanMap ivar.
  ///
  //
  // Scan rotations of SubMolecule sub around axis with angular resolution
  // res (in radian)
  void rankSuperCells(const QVector<MolecularXtal*> &supercells, const QVector<SubMolecule *> &ignoredSubMols);
  void sampleStrains();
  void sampleDisplacements(SubMolecule *sub, unsigned int numDisplacements);
  void scanRotations(SubMolecule *sub, const Eigen::Vector3d &axis,
                     double res);
  void sampleVolumes();

  ////////////////////////////////////
  // workingMXtal modification methods
  ///
  /// Convenience functions for modifying the workingMXtal
  ///
  // Apply strain to the unit cell, preserve fractional submol centers
  void strain(const Eigen::Matrix3d &strainMatrix);
  // Move SubMolecule sub to position pos and rotate angle radians around axis
  void displace(SubMolecule *sub, const Eigen::Vector3d &pos,
                const Eigen::Vector3d &axis, double angle);
  // Rotate SubMolecule sub around axis by rot radians, and update ranker.
  void rotateSubMolecule(SubMolecule *sub, const Eigen::Vector3d &axis,
                         double rot);
  // Rescale the cell to the specified volume
  void scaleVolume(double volume);

  // Copy the workingMXtal into offspring and set metadata (except id number,
  // that is set by the queuemanager to prevent a race condition)
  void prepareOffspring();
  // Check for abort signal and yield the thread/allow application event loop
  // to run for a bit. Returns true if aborting.
  bool checkForAbort();

  // Increment this->currentStep and emit progress update
  void emitNextStep();

  ///////////////
  // Variables //
  ///////////////

  const MolecularXtal *parentMXtal;
  MolecularXtal offspring;
  MolecularXtal workingMXtal;
  MolecularXtal workingMXtalCache;
  ProbabilityVector scanProbabilities;
  ProbabilityVector rankProbabilities;
  QMutex abortMutex;
  QMutex runningMutex;
  QMutex *setupMutex;
  QList<double> probabilities;
  RankMap rankMap;
  ScanVarType scanType;
  ScanMap scanMap;
  SubMoleculeRanker *ranker;
  bool aborted;
  bool debug;
  double parentEnergy;
  double offspringEnergy;

  // Mutation parameters
  bool createBest;
  bool startWithSuperCell;
  unsigned int numCandidates;
  unsigned int numStrains;
  double strainSigmaRange[2];
  unsigned int numMovers;
  unsigned int numDisplacements;
  double rotationResRad;
  unsigned int numVolumeSamples;
  double minVolumeFrac;
  double maxVolumeFrac;

  // Progress updates
  int numMutationSteps;
  int currentStep;

private:
  MolecularXtalMutator * const q_ptr;
  Q_DECLARE_PUBLIC(MolecularXtalMutator);
};

/****************************************************************************
 * MolecularXtalMutator functions                                           *
 ****************************************************************************/

MolecularXtalMutator::MolecularXtalMutator(const MolecularXtal *mxtal,
                                           QMutex *setupMutex,
                                           QObject *parent)
  : d_ptr(new MolecularXtalMutatorPrivate(this))
{
  Q_D(MolecularXtalMutator);

  d->setupMutex = setupMutex;
  d->parentMXtal = mxtal;
  d->ranker = new SubMoleculeRanker (this, setupMutex);
}

MolecularXtalMutator::~MolecularXtalMutator()
{
  delete d_ptr;
}

void MolecularXtalMutator::setDebug(bool b)
{
  Q_D(MolecularXtalMutator);
  d->debug = b;
}

void MolecularXtalMutator::setCreateBest(bool b)
{
  Q_D(MolecularXtalMutator);
  d->createBest = b;
}

void MolecularXtalMutator::setStartWithSuperCell(bool b)
{
  Q_D(MolecularXtalMutator);
  d->startWithSuperCell = b;
}

void MolecularXtalMutator::setMaximumNumberOfCandidates(unsigned int num)
{
  Q_D(MolecularXtalMutator);
  d->numCandidates = num;
}

void MolecularXtalMutator::setNumberOfStrains(unsigned int strains)
{
  Q_D(MolecularXtalMutator);
  d->numStrains = strains;
}

void MolecularXtalMutator::setStrainSigmaRange(double min, double max)
{
  Q_D(MolecularXtalMutator);
  d->strainSigmaRange[0] = min;
  d->strainSigmaRange[1] = max;
}

void MolecularXtalMutator::setNumMovers(unsigned int numMovers)
{
  Q_D(MolecularXtalMutator);
  d->numMovers = numMovers;
}

void MolecularXtalMutator::setNumDisplacements(unsigned int numDisplacements)
{
  Q_D(MolecularXtalMutator);
  d->numDisplacements = numDisplacements;
}

void MolecularXtalMutator::setRotationResolution(double radians)
{
  Q_D(MolecularXtalMutator);
  d->rotationResRad = radians;
}

void MolecularXtalMutator::setNumberOfVolumeSamples(unsigned int numSamples)
{
  Q_D(MolecularXtalMutator);
  d->numVolumeSamples = numSamples;
}

void MolecularXtalMutator::setMinimumVolumeFraction(double frac)
{
  Q_D(MolecularXtalMutator);
  d->minVolumeFrac = frac;
}

void MolecularXtalMutator::setMaximumVolumeFraction(double frac)
{
  Q_D(MolecularXtalMutator);
  d->maxVolumeFrac = frac;
}

bool MolecularXtalMutator::mutate()
{
  Q_D(MolecularXtalMutator);
  QMutexLocker runningLocker (&d->runningMutex);
  emit startingMutation();
  emit progressUpdate(0);
  QDateTime startTime = QDateTime::currentDateTime();
  DDEBUGOUT("mutate") "Starting mutation at" << startTime << "createBest="
                                                << d->createBest;

  // Initialize data structures
  d->workingMXtal = *d->parentMXtal;
  foreach (SubMolecule *sub, d->workingMXtal.subMolecules())
    sub->makeCoherent();
  d->ranker->setMXtal(&d->workingMXtal);

  // Adjust the number of movers if it exceeds the number of submolecules
  if (d->numMovers > d->workingMXtal.numSubMolecules()) {
    d->numMovers = d->workingMXtal.numSubMolecules();
    DDEBUGOUT("mutate") "Reducing number of movers to" << d->numMovers;
  }

  // Figure out the number of steps used in the mutation:
  // numStrainSamples + 1 (orig lattice)
  // + numMovers * ( 1 submol rank +
  //                 numDisplacements + 1 (orig pos) +
  //                 floor(360 / rotResDeg) )
  // + numVolumeSamples + 1 (orig volume)
  // + 1 preparation of offspring
  d->numMutationSteps = d->numStrains + 1 + d->numMovers *
      (1 + d->numDisplacements + 1 +
       360 / static_cast<int>(d->rotationResRad * RAD_TO_DEG) )
      + d->numVolumeSamples + 1 + 1 + 1;
  d->currentStep = 0;

  // Debugging...
  if (d->debug) {
    d->parentEnergy = d->ranker->evaluateTotalEnergy();
    DDEBUGOUT("mutate") "Parent energy: " << d->parentEnergy;
  }

  if (d->checkForAbort())
    return false;

  // Generate supercells if needed
  if (d->startWithSuperCell) {
    // Evaluate energies of each submolecule for ranking
    QVector<double> submolEnergies = d->ranker->evaluate();

    // Setup generator
    MolecularXtalSuperCellGenerator sCGenerator;
    sCGenerator.setDebug(true);
    sCGenerator.setMXtal(d->workingMXtal);
    sCGenerator.setSubMolecularEnergies(submolEnergies);

    // Generate supercells
    const QVector<MolecularXtal*> &supercells = sCGenerator.getSuperCells();
    const QVector<int> &unassignedSubMolIndices =
        sCGenerator.getUnassignedSubMolecules();

    // update progress maximum
    d->numMutationSteps += supercells.size();

    // Convert indices to pointers
    QVector<SubMolecule *> unassignedSubMols;
    unassignedSubMols.reserve(unassignedSubMolIndices.size());
    foreach (int ind, unassignedSubMolIndices)
      unassignedSubMols.push_back(d->workingMXtal.subMolecule(ind));

    // Evaluate supercell energies
    d->rankSuperCells(supercells, unassignedSubMols);
    Q_ASSERT(d->scanType == INTEGER);
    DDEBUGOUT("mutate") "Generated" << d->scanMap.size() << "supercells:";
    d->trimHighEnergyScanData();
    DDEBUGOUT("mutate") "Trimmed to" << d->scanMap.size() << "supercells";
    QList<double> supercellEnergies (d->scanMap.keys());
    ProbabilityVector supercellProbs =
        d->getProbabilitiesLow(supercellEnergies);
    DDEBUGOUT("mutate") "Supercell energies:" << supercellEnergies;
    DDEBUGOUT("mutate") "Supercell Probabilities:" << supercellProbs;

    // Select supercell from distribution
    int selectedSupercell = 0;
    if (!d->createBest)
      selectedSupercell = d->getRandomIndex(supercellProbs);
    DDEBUGOUT("mutate") "Selected supercell:" << selectedSupercell;

    // Apply selected supercell
    ScanMap::const_iterator supercellIt = d->scanMap.constBegin();
    for (int i = 0; i < selectedSupercell; ++i) ++supercellIt;
    int supercellInd = supercellIt.value().integer;
    MolecularXtal *supercell = supercells.at(supercellInd);
    d->copyCellAndCoords(supercell, &d->workingMXtal);
    QString supercellType = supercell->property("supercellType").toString();
    d->workingMXtal.setProperty("supercellType", supercellType);
    DDEBUGOUT("mutate") "Selected" << supercellType;

    d->workingMXtal.wrapAtomsToCell();
    d->workingMXtal.makeCoherent();
    d->ranker->updateGeometry();

    // Increase the number of movers by the number of unassigned submolecules
    d->numMovers += unassignedSubMolIndices.size();
  }

  // Debugging
  double supercellEnergy = 0.0;
  if (d->startWithSuperCell && d->debug) {
    supercellEnergy = d->ranker->evaluateTotalEnergy();
    DDEBUGOUT("mutate") "SuperCell energy: " << supercellEnergy;
  }

  if (d->numStrains > 0) {
    // Sample strains
    d->sampleStrains();
    Q_ASSERT(d->scanType == MATRIX3x3);
    DDEBUGOUT("mutate") "Generated" << d->scanMap.size() << "strains:";
    d->trimHighEnergyScanData();
    DDEBUGOUT("mutate") "Trimmed to" << d->scanMap.size() << "strains";
    QList<double> strainEnergies (d->scanMap.keys());
    ProbabilityVector strainProbs = d->getProbabilitiesLow(strainEnergies);
    DDEBUGOUT("mutate") "Strain energies:" << strainEnergies;
    DDEBUGOUT("mutate") "Strain Probabilities:" << strainProbs;

    // Select strain from distribution
    int selectedStrain = 0;
    if (!d->createBest)
      selectedStrain = d->getRandomIndex(strainProbs);
    DDEBUGOUT("mutate") "Selected strain:" << selectedStrain;

    // Apply selected strain
    ScanMap::const_iterator strainIt = d->scanMap.constBegin();
    for (int i = 0; i < selectedStrain; ++i) ++strainIt;
    const double *strainData = strainIt.value().matrix3x3;
    Eigen::Matrix3d strainMatrix;
    strainMatrix(0,0) = strainData[0];
    strainMatrix(0,1) = strainData[1];
    strainMatrix(0,2) = strainData[2];
    strainMatrix(1,0) = strainData[3];
    strainMatrix(1,1) = strainData[4];
    strainMatrix(1,2) = strainData[5];
    strainMatrix(2,0) = strainData[6];
    strainMatrix(2,1) = strainData[7];
    strainMatrix(2,2) = strainData[8];
    d->strain(strainMatrix);
    d->ranker->updateGeometry();

  }
  else {
    d->emitNextStep();
    DDEBUGOUT("mutate") "Skipping strains... (numStrains is 0)";
  }

  // Debugging
  double strainedEnergy = 0.0;
  if (d->debug) {
    strainedEnergy = d->ranker->evaluateTotalEnergy();
    DDEBUGOUT("mutate") "Strained energy: " << strainedEnergy;
  }

  // Loop to move molecules
  SubMolecule *mover = NULL; // save each mover so it can be skipped next time
  double premoveEnergy = strainedEnergy;
  for (int moveInd = 0; moveInd < d->numMovers; ++moveInd) {
    if (d->checkForAbort())
      return false;

    // Rank SubMolecules by energy contribution
    if (mover == NULL)
      d->rankSubMolecules();
    else {
      QVector<SubMolecule*> omit (1);
      omit << mover;
      d->rankSubMolecules(omit);
    }

    if (d->rankMap.isEmpty())
      break;

    // Calculate probabilities from ranking
    QList<double> moverEnergies = d->rankMap.keys();
    ProbabilityVector moverProbs = d->getProbabilitiesHigh(moverEnergies);
    DDEBUGOUT("mutate") "SubMol energies:" << moverEnergies;
    DDEBUGOUT("mutate") "SubMol probabilities:" << moverProbs;

    // Select mover from distribution
    int moverInd = d->rankMap.size() - 1;
    if (!d->createBest)
      moverInd = d->getRandomIndex(moverProbs);
    DDEBUGOUT("mutate") "Selected mover" << moveInd+1 << ":" << moverInd;
    RankMap::const_iterator moverIt = d->rankMap.constBegin();
    for (int i = 0; i < moverInd; ++i) ++moverIt;
    mover = moverIt.value();

    d->emitNextStep();

    if (d->checkForAbort())
      return false;

    // Sample positions
    d->sampleDisplacements(mover, d->numDisplacements);
    Q_ASSERT(d->scanType == TRANSFORM);
    DDEBUGOUT("mutate") "Sampled" << d->scanMap.size() << "displacements.";
    d->trimHighEnergyScanData();
    DDEBUGOUT("mutate") "Trimmed to" << d->scanMap.size() << "displacements.";
    QList<double> displacementEnergies = d->scanMap.keys();
    ProbabilityVector displacementProbs =
        d->getProbabilitiesLow(displacementEnergies);
    DDEBUGOUT("mutate") "Displacement energies:" << displacementEnergies;
    DDEBUGOUT("mutate") "Displacement probabilities:" << displacementProbs;

    // Select position from distribution
    int displacementInd = 0;
    if (!d->createBest)
      displacementInd = d->getRandomIndex(displacementProbs);
    DDEBUGOUT("mutate") "Selected displacement:" << displacementInd;
    ScanMap::const_iterator displacementIt = d->scanMap.constBegin();
    for (int i = 0; i < displacementInd; ++i) ++displacementIt;
    const Eigen::Vector3d pos (displacementIt.value().transform.pos);
    const Eigen::Vector3d dispAxis (displacementIt.value().transform.axis);
    const double dispAngle = displacementIt.value().transform.angle;

    // Apply selected displacement
    d->displace(mover, pos, dispAxis, dispAngle);
    d->ranker->updateGeometry(mover);

    // Debugging
    double displacedEnergy = 0.0;
    if (d->debug) {
      displacedEnergy = d->ranker->evaluateTotalEnergy();
      DDEBUGOUT("mutate") "Displaced energy: " << displacedEnergy;
    }

    if (d->checkForAbort())
      return false;

    // Pick rotation axis
    Eigen::Vector3d axis;
    const double r_axis = RANDDOUBLE();
    if (r_axis < 0.33) {
      axis = mover->normal();
      DDEBUGOUT("mutate") "Using mover normal as rotation axis.";
    }
    else if (r_axis < 0.67) {
      axis = mover->normal().unitOrthogonal();
      DDEBUGOUT("mutate") "Using in-plane vector as rotation axis.";
    }
    else {
      axis = Eigen::Vector3d::Random().normalized();
      DDEBUGOUT("mutate") "Using random vector as rotation axis.";
    }

    // Scan rotations
    d->scanRotations(mover, axis, d->rotationResRad);
    Q_ASSERT(d->scanType == REAL);
    DDEBUGOUT("mutate") "Sampled" << d->scanMap.size() << "rotations.";
    d->trimHighEnergyScanData();
    DDEBUGOUT("mutate") "Trimmed to" << d->scanMap.size() << "rotations.";
    QList<double> rotationEnergies = d->scanMap.keys();
    ProbabilityVector rotationProbs = d->getProbabilitiesLow(rotationEnergies);
    DDEBUGOUT("mutate") "Rotation energies:" << rotationEnergies;
    DDEBUGOUT("mutate") "Rotation probabilities:" << rotationProbs;

    // Select rotation from distribution
    int rotationInd = 0;
    if (!d->createBest)
      rotationInd = d->getRandomIndex(rotationProbs);
    DDEBUGOUT("mutate") "Selected rotation:" << rotationInd;
    ScanMap::const_iterator rotationIt = d->scanMap.constBegin();
    for (int i = 0; i < rotationInd; ++i) ++rotationIt;
    double angle (rotationIt.value().real);

    // Apply selected rotation
    d->rotateSubMolecule(mover, axis, angle);
    d->ranker->updateGeometry(mover);

    // Debugging
    double rotatedEnergy = 0.0;
    if (d->debug) {
      rotatedEnergy = d->ranker->evaluateTotalEnergy();
      DDEBUGOUT("mutate") "Rotated energy: " << rotatedEnergy;
    }

    // Print summary
    DDEBUGOUT("mutate") QString("| %1 | %2 | %3 | %4 |")
        .arg("Move Summary", 15)
        .arg("Before", 10)
        .arg("After", 10)
        .arg("Diff", 10);
    DDEBUGOUT("mutate") QString("| %1 | %L2 | %L3 | %L4 |")
        .arg("Displacement", 15)
        .arg(premoveEnergy, 10)
        .arg(displacedEnergy, 10)
        .arg(displacedEnergy - premoveEnergy, 10);
    DDEBUGOUT("mutate") QString("| %1 | %L2 | %L3 | %L4 |")
        .arg("Rotation", 15)
        .arg(displacedEnergy, 10)
        .arg(rotatedEnergy, 10)
        .arg(rotatedEnergy - displacedEnergy, 10);
    DDEBUGOUT("mutate") QString("| %1 | %L2 | %L3 | %L4 |")
        .arg("Overall", 15)
        .arg(premoveEnergy, 10)
        .arg(rotatedEnergy, 10)
        .arg(rotatedEnergy - premoveEnergy, 10);

    // Prepare for next loop
    premoveEnergy = rotatedEnergy;

  } // end mover loop

  if (d->checkForAbort())
    return false;

  if (d->numVolumeSamples > 0) {
    // Sample Volumes
    d->sampleVolumes();
    Q_ASSERT(d->scanType == REAL);
    DDEBUGOUT("mutate") "Generated" << d->scanMap.size() << "volumes:";
    d->trimHighEnergyScanData();
    DDEBUGOUT("mutate") "Trimmed to" << d->scanMap.size() << "volumes";
    QList<double> volumeEnergies (d->scanMap.keys());
    ProbabilityVector volumeProbs = d->getProbabilitiesLow(volumeEnergies);
    DDEBUGOUT("mutate") "Volume energies:" << volumeEnergies;
    DDEBUGOUT("mutate") "Volume Probabilities:" << volumeProbs;

    // Select volume from distribution
    int selectedVolume = 0;
    if (!d->createBest)
      selectedVolume = d->getRandomIndex(volumeProbs);
    DDEBUGOUT("mutate") "Selected volume:" << selectedVolume;

    // Scale to selected volume
    ScanMap::const_iterator volumeIt = d->scanMap.constBegin();
    for (int i = 0; i < selectedVolume; ++i) ++volumeIt;
    const double volume = volumeIt.value().real;
    d->scaleVolume(volume);
    d->ranker->updateGeometry();

  }
  else {
    DDEBUGOUT("mutate") "Skipping volume sampling (numVolumeSamples is 0).";
  }

  // Debugging
  double scaledEnergy = 0.0;
  if (d->debug) {
    scaledEnergy = d->ranker->evaluateTotalEnergy();
    DDEBUGOUT("mutate") "Scaled energy: " << scaledEnergy;
  }

  d->offspringEnergy = scaledEnergy;

  QDateTime endTime = QDateTime::currentDateTime();
  DDEBUGOUT("mutate") "Mutation finished at" << endTime;
  DDEBUGOUT("mutate") "Time elapsed:    " << startTime.secsTo(endTime);
  DDEBUGOUT("mutate") "Parent energy:   " << d->parentEnergy;
  DDEBUGOUT("mutate") "Offspring energy:" << d->offspringEnergy;
  DDEBUGOUT("mutate") "Mutation diff:   "
      << d->offspringEnergy - d->parentEnergy;

  if (d->checkForAbort())
    return false;

  d->prepareOffspring();

  d->emitNextStep();

  if (d->currentStep != d->numMutationSteps) {
    DDEBUGOUT("mutate") "Not all steps taken? taken, total:"
        << d->currentStep << d->numMutationSteps;
  }

  emit finishedMutation();
  return true;
}

void MolecularXtalMutator::abort()
{
  Q_D(MolecularXtalMutator);
  d->abortMutex.lock();
  d->aborted = true;
  d->abortMutex.unlock();
}

bool MolecularXtalMutator::createBest() const
{
  Q_D(const MolecularXtalMutator);
  return d->createBest;
}

bool MolecularXtalMutator::startWithSuperCell() const
{
  Q_D(const MolecularXtalMutator);
  return d->startWithSuperCell;
}

unsigned int MolecularXtalMutator::maximumNumberOfCandidates() const
{
  Q_D(const MolecularXtalMutator);
  return d->numCandidates;
}

unsigned int MolecularXtalMutator::numberOfStrains() const
{
  Q_D(const MolecularXtalMutator);
  return d->numStrains;
}

void MolecularXtalMutator::strainSigmaRange(double range[]) const
{
  Q_D(const MolecularXtalMutator);
  range[0] = d->strainSigmaRange[0];
  range[1] = d->strainSigmaRange[1];
}

unsigned int MolecularXtalMutator::numberOfMovers() const
{
  Q_D(const MolecularXtalMutator);
  return d->numMovers;
}

unsigned int MolecularXtalMutator::numberOfDisplacements() const
{
  Q_D(const MolecularXtalMutator);
  return d->numDisplacements;
}

double MolecularXtalMutator::rotationResolution() const
{
  Q_D(const MolecularXtalMutator);
  return d->rotationResRad;
}

double MolecularXtalMutator::numberOfVolumeSamples() const
{
  Q_D(const MolecularXtalMutator);
  return d->numVolumeSamples;
}

double MolecularXtalMutator::minimumVolumeFraction() const
{
  Q_D(const MolecularXtalMutator);
  return d->minVolumeFrac;
}

double MolecularXtalMutator::maximumVolumeFraction() const
{
  Q_D(const MolecularXtalMutator);
  return d->maxVolumeFrac;
}

bool MolecularXtalMutator::waitForFinished(int ms_timeout) const
{
  Q_D(const MolecularXtalMutator);

  /* The runningMutex is locked at the beginning of the mutation and released
   * when finished. So just lock it and release it to make this thread sleep
   * until the calc is finished/aborts. */

  QMutex *mutableRunningMutex = const_cast<QMutex*>(&d->runningMutex);
  if (mutableRunningMutex->tryLock(ms_timeout)) {
    mutableRunningMutex->unlock();
    return true;
  }
  return false;
}

bool MolecularXtalMutator::isRunning() const
{
  Q_D(const MolecularXtalMutator);
  QMutex *mutableRunningMutex = const_cast<QMutex*>(&d->runningMutex);
  if (mutableRunningMutex->tryLock()) {
    // Can lock mutex -- not running
    mutableRunningMutex->unlock();
    return false;
  }
  return true;
}

bool MolecularXtalMutator::isAborted() const
{
  Q_D(const MolecularXtalMutator);
  QMutex *mutableAbortMutex = const_cast<QMutex*>(&d->abortMutex);
  mutableAbortMutex->lock();
  bool ret = d->aborted;
  mutableAbortMutex->unlock();
  return ret;
}

const MolecularXtal *MolecularXtalMutator::getParentMXtal() const
{
  Q_D(const MolecularXtalMutator);
  return d->parentMXtal;
}

MolecularXtal *MolecularXtalMutator::getOffspring() const
{
  Q_D(const MolecularXtalMutator);
  if (d->offspring.numAtoms() == 0) {
    return NULL;
  }
  return new MolecularXtal (d->offspring);
}

/****************************************************************************
 * MolecularXtalMutatorPrivate functions                                    *
 ****************************************************************************/

MolecularXtalMutatorPrivate::~MolecularXtalMutatorPrivate()
{
  delete this->ranker;
  this->ranker = NULL;
}

inline void MolecularXtalMutatorPrivate::copyCellAndCoords(
    const MolecularXtal *from, MolecularXtal *to)
{
  Q_ASSERT(from->numAtoms() == to->numAtoms());

  to->setCellInfo(from->getA(), from->getB(), from->getC(),
                  from->getAlpha(), from->getBeta(), from->getGamma());

  int numAtoms = from->numAtoms();
  for (int i = 0; i < numAtoms; ++i)
    to->atom(i)->setPos(from->atom(i)->pos());
}

void MolecularXtalMutatorPrivate::cacheWorkingMXtal()
{
  if (this->workingMXtal.numAtoms() != this->workingMXtalCache.numAtoms())
    this->workingMXtalCache = this->workingMXtal;
  else
    this->copyCellAndCoords(&this->workingMXtal, &this->workingMXtalCache);
}

void MolecularXtalMutatorPrivate::restoreWorkingMXtal()
{
  if (this->workingMXtal.numAtoms() != this->workingMXtalCache.numAtoms())
    this->workingMXtal = this->workingMXtalCache;
  else
    this->copyCellAndCoords(&this->workingMXtalCache, &this->workingMXtal);
}

ProbabilityVector MolecularXtalMutatorPrivate::getProbabilitiesHigh(
    const QList<double> &energies)
{
  ProbabilityVector probs;
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
    probs << 0.25 << 1.0; // favor highest 3:1
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
    return probs;
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

ProbabilityVector MolecularXtalMutatorPrivate::getProbabilitiesLow(
    const QList<double> &energies)
{
  ProbabilityVector probs;
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

inline int MolecularXtalMutatorPrivate::getRandomIndex(
    const ProbabilityVector &probs)
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

inline double
MolecularXtalMutatorPrivate::getRandomNumberFromRange(double min, double max)
{
  return RANDDOUBLE() * (max - min) + min;
}

const double NV_MAGICCONST = 4 * exp(-0.5)/sqrt(2.0);
inline Eigen::Matrix3d
MolecularXtalMutatorPrivate::generateRandomVoightMatrix(double sigma)
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

// remove all entries with an energy higher than the average energy
void MolecularXtalMutatorPrivate::trimHighEnergyScanData()
{
  if (this->scanMap.size() < 2)
    return;

  // If there are two energies, check if there is an order of magnitude
  // difference between them. If so, kick out the highest.
  if (this->scanMap.size() == 2) {
    double low = this->scanMap.begin().key();
    double high = (++this->scanMap.begin()).key();
    double ratio = high / low;
    if ((low > 0.0 && ratio > 10.0) ||
        (high < 0.0 && ratio < 0.1) ||
        ratio < 0) {
      this->scanMap.erase(this->scanMap.end() - 1);
    }
    return;
  }

  // If there is no spread to the energies, just return.
  if (fabs(this->scanMap.begin().key() - (this->scanMap.end()-1).key())
      < 1e-3)
    return;

  // Determine mean and median
  double mean = 0.0;
  int medianCounter = this->scanMap.size() / 2;
  // Enforce the maximum number of candidates
  if (medianCounter > this->numCandidates)
    medianCounter = this->numCandidates;
  double median;
  foreach (double energy, this->scanMap.keys()) {
    if (--medianCounter == 0)
      median = energy;
    mean += energy;
  }
  mean /= static_cast<double>(this->scanMap.size());

  double lowerBound = (mean < median) ? mean : median;

  ScanMap::iterator eraser = this->scanMap.lowerBound(lowerBound);

  if (eraser == this->scanMap.begin()) {
    DEBUGOUT("trimHighEnergyScanData") "Trimming would result in empty list; "
        << "Aborting. Lowest energy" << this->scanMap.begin().key()
        << "Cutoff energy" << eraser.key() << "Making return list length 1.";
    ++eraser;
  }

  while (eraser != this->scanMap.end()) {
    eraser = this->scanMap.erase(eraser);
  }
}

// remove all entries with an energy lower than the average energy
void MolecularXtalMutatorPrivate::trimLowEnergyScanData()
{
  if (this->scanMap.size() < 2)
    return;

  // If there are two energies, check if there is an order of magnitude
  // difference between them. If so, kick out the highest.
  if (this->scanMap.size() == 2) {
    double low = this->scanMap.begin().key();
    double high = (++this->scanMap.begin()).key();
    double ratio = high / low;
    if ((low > 0.0 && ratio > 10.0) ||
        (high < 0.0 && ratio < 0.1) ||
        ratio < 0) {
      this->scanMap.erase(this->scanMap.begin());
    }
    return;
  }

  // If there is no spread to the energies, just return.
  if (fabs(this->scanMap.begin().key() - (this->scanMap.end()-1).key())
      < 1e-3)
    return;

  double mean = 0.0;
  int medianCounter = this->scanMap.size() / 2;
  if (medianCounter < this->scanMap.size() - this->numCandidates - 1)
    medianCounter = this->scanMap.size() - this->numCandidates - 1;
  double median;
  foreach (double energy, this->scanMap.keys()) {
    if (--medianCounter == 0)
      median = energy;
    mean += energy;
  }
  mean /= static_cast<double>(this->scanMap.size());

  double upperBound = (mean > median) ? mean : median;

  if (upperBound > (this->scanMap.end()-1).key()) {
    DEBUGOUT("trimHighEnergyScanData") "Trimming would result in empty list;"
        << "Aborting. Highest energy" << (this->scanMap.end()-1).key()
        << "Cutoff energy" << upperBound;
    return;
  }

  while (this->scanMap.begin().key() < upperBound) {
    this->scanMap.remove(0);
  }
}

void MolecularXtalMutatorPrivate::rankSubMolecules(
    const QVector<SubMolecule*> &omit)
{
  this->rankMap.clear();
  foreach (SubMolecule* submol, this->workingMXtal.subMolecules()) {
    if (omit.contains(submol))
      continue;

    this->rankMap.insert(this->ranker->evaluateInter(submol)
                         / static_cast<double>(submol->numAtoms()), submol);
  }
}

void MolecularXtalMutatorPrivate::rankSuperCells(
    const QVector<MolecularXtal *> &supercells,
    const QVector<SubMolecule*> &ignoredSubMols)
{
  Q_ASSERT(this->ranker->mxtal() == &workingMXtal);

  // Initialize scan data
  this->scanMap.clear();
  this->scanType = INTEGER;

  // Loop through supercells
  for (int i = 0; i < supercells.size(); ++i) {
    this->emitNextStep();

    MolecularXtal *supercell = supercells[i];
    this->cacheWorkingMXtal();
    this->copyCellAndCoords(supercell, &this->workingMXtal);
    this->workingMXtal.wrapAtomsToCell();
    this->workingMXtal.makeCoherent();
    this->ranker->updateGeometry();
    double energy = this->ranker->evaluateTotalEnergy(ignoredSubMols, true);
    ScanVar_t scanVar;
    scanVar.integer = i;
    this->scanMap.insert(energy, scanVar);

    // Restore original structure
    this->restoreWorkingMXtal();
  }
}

void MolecularXtalMutatorPrivate::sampleStrains()
{
  Q_ASSERT(this->ranker->mxtal() == &workingMXtal);

  // Initialize scan data
  this->scanMap.clear();
  this->scanType = MATRIX3x3;

  // Loop over numStrains + 1 (to include input config)
  for (int i = 0; i < this->numStrains + 1; ++i) {
    this->emitNextStep();
    // Generate strain params
    double sigma = getRandomNumberFromRange(this->strainSigmaRange[0],
                                            this->strainSigmaRange[1]);
    Eigen::Matrix3d voight = this->generateRandomVoightMatrix(sigma);

    // First iteration -- make no change
    if (i == 0) {
      sigma = 0;
      voight.setIdentity();
    }

    // Save a copy of the working mxtal
    this->cacheWorkingMXtal();

    // Perform strain and evaluate energy. Update scanMap.
    this->strain(voight);
    this->ranker->updateGeometry();
    double energy = this->ranker->evaluateTotalEnergyInter();
    ScanVar_t scanVar;
    scanVar.matrix3x3[0] = voight(0,0);
    scanVar.matrix3x3[1] = voight(0,1);
    scanVar.matrix3x3[2] = voight(0,2);
    scanVar.matrix3x3[3] = voight(1,0);
    scanVar.matrix3x3[4] = voight(1,1);
    scanVar.matrix3x3[5] = voight(1,2);
    scanVar.matrix3x3[6] = voight(2,0);
    scanVar.matrix3x3[7] = voight(2,1);
    scanVar.matrix3x3[8] = voight(2,2);
    this->scanMap.insertMulti(energy, scanVar);

    // Restore previous working mxtal
    this->restoreWorkingMXtal();
  }
}

void MolecularXtalMutatorPrivate::sampleDisplacements(
    SubMolecule *sub, unsigned int numDisplacements)
{
  Q_ASSERT(this->ranker->mxtal() == &workingMXtal);

  // Initialize scan data
  this->scanMap.clear();
  this->scanType = TRANSFORM;

  // Initialize holefinder
  HoleFinder holeFinder (this->workingMXtal.numAtoms() - sub->numAtoms());
  OpenBabel::matrix3x3 obmat = this->workingMXtal.OBUnitCell()->GetCellMatrix();
  holeFinder.setGridResolution(0.5);
  holeFinder.setTranslations(OB2Eigen(obmat.GetRow(0)),
                             OB2Eigen(obmat.GetRow(1)),
                             OB2Eigen(obmat.GetRow(2)));
  foreach (const SubMolecule *submol, this->workingMXtal.subMolecules()) {
    if (sub == submol)
      continue;
    foreach (const Avogadro::Atom *atom, submol->atoms()) {
      holeFinder.addPoint(*atom->pos());
    }
  }
  holeFinder.run();

  // + 1 for input configuration
  const Eigen::Vector3d origPos (sub->center());
  for (int i = 0; i < numDisplacements + 1; ++i) {
    this->emitNextStep();
    Eigen::Vector3d pos (holeFinder.getRandomPoint());
    Eigen::Vector3d axis (RANDDOUBLE(), RANDDOUBLE(), RANDDOUBLE());
    axis.normalize();
    double angle = RANDDOUBLE() * 2.0 * M_PI;

    // Sample original configuration
    if (i == 0) {
      pos = origPos;
      angle = 0.0;
    }

    // Save a copy of the working mxtal
    this->cacheWorkingMXtal();

    // Perform strain and evaluate energy. Update scanMap.
    this->displace(sub, pos, axis, angle);
    this->ranker->updateGeometry(sub);
    double energy = this->ranker->evaluateTotalEnergyInter();
    ScanVar_t scanVar;
    scanVar.transform.pos[0] = pos[0];
    scanVar.transform.pos[1] = pos[1];
    scanVar.transform.pos[2] = pos[2];
    scanVar.transform.axis[0] = axis[0];
    scanVar.transform.axis[1] = axis[1];
    scanVar.transform.axis[2] = axis[2];
    scanVar.transform.angle = angle;
    this->scanMap.insertMulti(energy, scanVar);

    // Restore previous working mxtal
    this->restoreWorkingMXtal();
  }
}

void MolecularXtalMutatorPrivate::scanRotations(SubMolecule *sub,
                                                const Eigen::Vector3d &axis,
                                                double res)
{
  Q_ASSERT(this->ranker->mxtal() == &workingMXtal);
  Q_ASSERT(this->workingMXtal.subMolecules().contains(sub));

  // Initialize scan data
  this->scanMap.clear();
  this->scanType = REAL;

  for (double angle = 0.0; angle < 2.0 * M_PI; angle += res) {
    this->emitNextStep();
    // Save a copy of the working mxtal
    this->cacheWorkingMXtal();

    // Rotate, evaluate, store
    this->rotateSubMolecule(sub, axis, angle);
    this->ranker->updateGeometry(sub);
    double energy = this->ranker->evaluateTotalEnergyInter();
    ScanVar_t scanVar;
    scanVar.real = angle;
    this->scanMap.insertMulti(energy, scanVar);

    // Restore previous working mxtal
    this->restoreWorkingMXtal();
  }
}

void MolecularXtalMutatorPrivate::sampleVolumes()
{
  Q_ASSERT(this->ranker->mxtal() == &this->workingMXtal);

  // Initialize scan data
  this->scanMap.clear();
  this->scanType = REAL;

  double minVolume = this->workingMXtal.getVolume() * minVolumeFrac;
  double maxVolume = this->workingMXtal.getVolume() * maxVolumeFrac;

  // + 1 to sample input configuration as well
  for (int i = 0; i < this->numVolumeSamples+1; ++i) {
    this->emitNextStep();
    // Generate strain params
    double volume = getRandomNumberFromRange(minVolume,
                                             maxVolume);

    // Original config first
    if (i == 0)
      volume = this->workingMXtal.getVolume();

    // Save a copy of the working mxtal
    this->cacheWorkingMXtal();

    // Perform strain and evaluate energy. Update scanMap.
    this->scaleVolume(volume);
    this->ranker->updateGeometry();
    double energy = this->ranker->evaluateTotalEnergyInter();
    ScanVar_t scanVar;
    scanVar.real = volume;
    this->scanMap.insertMulti(energy, scanVar);

    // Restore previous working mxtal
    this->restoreWorkingMXtal();
  }
}

void MolecularXtalMutatorPrivate::strain(const Eigen::Matrix3d &strainMatrix)
{
  // Cache fractional submol centers for later
  QList<SubMolecule*> subs = this->workingMXtal.subMolecules();
  QVector<Eigen::Vector3d> fracCenters;
  fracCenters.reserve(this->workingMXtal.numSubMolecules());
  for (QList<SubMolecule*>::const_iterator it = subs.constBegin(),
       it_end = subs.constEnd(); it != it_end; ++it) {
    fracCenters.push_back(this->workingMXtal.cartToFrac((*it)->center()));
  }

  // Adjust cell
  this->workingMXtal.setCellInfo(
        this->workingMXtal.OBUnitCell()->GetCellMatrix()
        * Eigen2OB(strainMatrix));

  // Restore fractional centers of submolecules
  for (int i = 0; i < subs.size(); ++i) {
    subs[i]->setCenter(this->workingMXtal.fracToCart(fracCenters[i]));
  }
}

inline void MolecularXtalMutatorPrivate::displace(
    SubMolecule *sub, const Eigen::Vector3d &pos, const Eigen::Vector3d &axis,
    double angle)
{
  sub->setCenter(pos);
  sub->rotate(angle, axis);
}

inline void MolecularXtalMutatorPrivate::rotateSubMolecule(
    SubMolecule *sub, const Eigen::Vector3d &axis, double rot)
{
  sub->rotate(rot, axis);
}

void MolecularXtalMutatorPrivate::scaleVolume(double volume)
{
  this->workingMXtal.setVolume(volume);
}

void MolecularXtalMutatorPrivate::prepareOffspring()
{
  this->offspring = this->workingMXtal;

  this->offspring.setGeneration(this->parentMXtal->getGeneration() + 1);
  this->offspring.setParents(
        QString("Mutation: %1, %2 movers (-DE=%L3)%L4%L5")
        .arg(this->parentMXtal->getIDString())
        .arg(this->numMovers)
        .arg((this->parentEnergy - this->offspringEnergy))
        .arg((this->createBest) ? " (Best)" : "")
        .arg((this->startWithSuperCell)
             ? " (Supercell " +
               this->workingMXtal.property("supercellType").toString() + ")"
             : ""));
  this->offspring.setStatus(MolecularXtal::WaitingForOptimization);
}

bool MolecularXtalMutatorPrivate::checkForAbort()
{
  this->abortMutex.lock();
  bool needAbort = this->aborted;
  this->abortMutex.unlock();

  qApp->processEvents(QEventLoop::AllEvents, 1000);
  QThread::yieldCurrentThread();

  if (needAbort)
    DEBUGOUT("checkForAbort") "Abort requested.";

  return needAbort;
}

void MolecularXtalMutatorPrivate::emitNextStep()
{
  Q_Q(MolecularXtalMutator);

  emit q->progressUpdate(100 * ++this->currentStep / this->numMutationSteps);
}

}

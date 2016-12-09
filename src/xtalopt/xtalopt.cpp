/**********************************************************************
  XtalOpt - "Engine" for the optimization process

  Copyright (C) 2009-2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <xtalopt/xtalopt.h>

#include <xtalopt/structures/xtal.h>
#include <xtalopt/optimizers/vasp.h>
#include <xtalopt/optimizers/gulp.h>
#include <xtalopt/optimizers/pwscf.h>
#include <xtalopt/optimizers/castep.h>
#include <xtalopt/optimizers/siesta.h>
#include <xtalopt/ui/dialog.h>
#include <xtalopt/genetic.h>

#include <randSpg/include/xtaloptWrapper.h>
#include <xtalopt/ui/randSpgDialog.h>

#include <globalsearch/optbase.h>
#include <globalsearch/optimizer.h>
#include <globalsearch/queueinterface.h>
#include <globalsearch/queueinterfaces/local.h>
#include <globalsearch/queuemanager.h>
#include <globalsearch/slottedwaitcondition.h>
#include <globalsearch/macros.h>
#include <globalsearch/bt.h>
#include <globalsearch/utilities/fileutils.h>
#include <globalsearch/utilities/exceptionhandler.h>

#ifdef ENABLE_SSH
#include <globalsearch/sshmanager.h>
#include <globalsearch/queueinterfaces/remote.h>
#endif // ENABLE_SSH

#include <openbabel/generic.h>

#include <Eigen/LU>

#include <QtCore/QDir>
#include <QtCore/QList>
#include <QtCore/QFile>
#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtCore/QFileInfo>
#include <QtCore/QReadWriteLock>
#include <QtCore/QtConcurrentRun>
#include <QtCore/QtConcurrentMap>

#include <randSpg/include/randSpg.h>

#include <atomic> // For thread-safe types

#define ANGSTROM_TO_BOHR 1.889725989

using namespace GlobalSearch;
using namespace OpenBabel;
using namespace Avogadro;

namespace XtalOpt {

  XtalOpt::XtalOpt(XtalOptDialog *parent) :
    OptBase(parent),
    m_initWC(new SlottedWaitCondition (this)),
    using_randSpg(false),
    minXtalsOfSpgPerFU(QList<int>())
  {
    xtalInitMutex = new QMutex;
    m_idString = "XtalOpt";
    m_schemaVersion = 2;

    // Connections
    connect(m_tracker, SIGNAL(newStructureAdded(GlobalSearch::Structure*)),
            this, SLOT(checkForDuplicates()));
    connect(this, SIGNAL(sessionStarted()),
            this, SLOT(resetDuplicates()));
    connect(m_queue, SIGNAL(structureFinished(GlobalSearch::Structure*)),
            this, SLOT(updateLowestEnthalpyFUList(GlobalSearch::Structure*)));
    connect(m_queue, SIGNAL(structureFinished(GlobalSearch::Structure*)),
            this, SLOT(_incrementParentNumTotOffspring(GlobalSearch::Structure*)));

  }

  XtalOpt::~XtalOpt()
  {
    // Destructors should never throw exceptions. But use a try-catch block
    // just in case...
    try {
      // Stop queuemanager thread
      if (m_queueThread->isRunning()) {
        m_queueThread->disconnect();
        m_queueThread->quit();
        m_queueThread->wait();
      }

      // Delete queuemanager
      delete m_queue;
      m_queue = 0;

#ifdef ENABLE_SSH
      // Stop SSHManager
      delete m_ssh;
      m_ssh = 0;
#endif // ENABLE_SSH

      // Wait for save to finish
      unsigned int timeout = 30;
      while (timeout > 0 && savePending) {
        qDebug() << "Spinning on save before destroying XtalOpt ("
                 << timeout << "seconds until timeout).";
        timeout--;
        GS_SLEEP(1);
      };

      savePending = true;

      // Clean up various members
      m_initWC->deleteLater();
      m_initWC = 0;
    } // end of try{}
    catch(...) {
      ExceptionHandler::handleAllExceptions(__FUNCTION__);
    } // end of catch{}
  }

  void XtalOpt::startSearch()
  {
    // Populate crystal
    QList<uint> atomicNums = comp.keys();
    // Sort atomic number by decreasing minimum radius. Adding the "larger"
    // atoms first encourages a more even (and ordered) distribution
/*    qDebug() << "atomicNums are:";
    for (size_t i = 0; i < atomicNums.size(); ++i)
      qDebug() << QString::number(atomicNums.at(i));*/
    for (int i = 0; i < atomicNums.size()-1; ++i) {
      for (int j = i + 1; j < atomicNums.size(); ++j) {
        if (this->comp.value(atomicNums[i]).minRadius <
            this->comp.value(atomicNums[j]).minRadius) {
          atomicNums.swap(i,j);
        }
      }
    }

    // Settings checks
    // Check lattice parameters, volume, etc
    if (!XtalOpt::checkLimits()) {
      error("Cannot create structures. Check log for details.");
      return;
    }

    // Do we have a composition?
    if (comp.isEmpty()) {
      error("Cannot create structures. Composition is not set.");
      return;
    }

    // Check if xtalopt data is already saved at the filePath
    if (QFile::exists(filePath + "/xtalopt.state")) {
      bool proceed;
      needBoolean(tr("Warning: XtalOpt data is already saved at: %1"
                     "\nDo you wish to proceed and overwrite it?"
                     "\n\nIf no, please change the local working directory "
                     "under Queue configure located in the "
                     "'Optimization Settings' tab")
                  .arg(filePath),
                  &proceed);
      if (!proceed) return;
      else {
        bool result = FileUtils::removeDir(filePath);
        if (!result) {
          error(tr("Error removing directory at:\n %1").arg(filePath));
        }
      }
    }

    // Are the selected queueinterface and optimizer happy?
    QString err;
    if (!m_optimizer->isReadyToSearch(&err)) {
      error(tr("Optimizer is not fully initialized:") + "\n\n" + err);
      return;
    }

    if (!m_queueInterface->isReadyToSearch(&err)) {
      error(tr("QueueInterface is not fully initialized:") + "\n\n" + err);
      return;
    }

    // Warn user if runningJobLimit is 0
    if (limitRunningJobs && runningJobLimit == 0) {
      error(tr("Warning: the number of running jobs is currently set to 0."
               "\n\nYou will need to increase this value before the search "
               "can begin (The option is on the 'Search Settings' tab)."));
    };

    // Warn user if contStructs is 0
    if (contStructs == 0) {
      error(tr("Warning: the number of continuous structures is "
               "currently set to 0."
               "\n\nYou will need to increase this value before the search "
               "can move past the first generation (The option is on the "
               "'Search Settings' tab)."));
    };

    // VASP checks:
    if (m_optimizer->getIDString() == "VASP") {
      // Is the POTCAR generated? If not, warn user in log and launch
      // generator. Every POTCAR will be identical in this case!
      QList<uint> oldcomp, atomicNums = comp.keys();
      QList<QVariant> oldcomp_ = m_optimizer->getData("Composition").toList();
      for (int i = 0; i < oldcomp_.size(); i++)
        oldcomp.append(oldcomp_.at(i).toUInt());
      qSort(atomicNums);
      if (m_optimizer->getData("POTCAR info").toList().isEmpty() || // No info
          oldcomp != atomicNums // Composition has changed!
          ) {
        error("Using VASP and POTCAR is empty. Please select the "
              "pseudopotentials before continuing.");
        return;
      }

      // Build up the latest and greatest POTCAR compilation
      qobject_cast<VASPOptimizer*>(m_optimizer)->buildPOTCARs();
    }

#ifdef ENABLE_SSH
    // Create the SSHManager if running remotely
    if (qobject_cast<RemoteQueueInterface*>(m_queueInterface) != 0) {
      if (!this->createSSHConnections()) {
        error(tr("Could not create ssh connections."));
        return;
      }
    }
#endif // ENABLE_SSH

    // Here we go!
    debug("Starting optimization.");
    emit startingSession();

    // prepare pointers
    m_tracker->lockForWrite();
    m_tracker->deleteAllStructures();
    m_tracker->unlock();

    ///////////////////////////////////////////////
    // Generate random structures and load seeds //
    ///////////////////////////////////////////////

    // Set up progress bar
    m_dialog->startProgressUpdate(tr("Generating structures..."), 0, 0);

    // Initalize loop variables
    int failed = 0;
    QString filename;
    Xtal *xtal = 0;
    // Use new xtal count in case "addXtal" falls behind so that we
    // don't duplicate structures when switching from seeds -> random.
    uint newXtalCount = 0;

    // Load seeds...
    for (int i = 0; i < seedList.size(); i++) {
      filename = seedList.at(i);
      if (this->addSeed(filename)) {
        m_dialog->updateProgressLabel(
              tr("%1 structures generated (%2 kept, %3 rejected)...")
              .arg(i + failed).arg(i).arg(failed));
        newXtalCount++;
      }
    }

    // Perform a regular random generation
    if (!using_randSpg) {
      // Generation loop...
      while (newXtalCount < numInitial) {
        updateProgressBar(numInitial, newXtalCount + failed, newXtalCount);

        // Generate/Check xtal
        xtal = generateRandomXtal(1, newXtalCount+1);
        if (!checkXtal(xtal)) {
          delete xtal;
          failed++;
        }
        else {
          xtal->findSpaceGroup(tol_spg);
          initializeAndAddXtal(xtal, 1, xtal->getParents());
          newXtalCount++;
        }
      }
    }
    // Perform a spacegroup generation generation
    else {
      // If minXtalsOfSpgPerFU was never correctly generated, just generate one now
      if (minXtalsOfSpgPerFU.size() == 0) {
        for (size_t spg = 1; spg <= 230; spg++)
          minXtalsOfSpgPerFU.append(0);
      }

      QList<int> spgStillNeeded = minXtalsOfSpgPerFU;

      // Find the total number of xtals that will be generated
      // If minXtalsOfSpgPerFU specifies more xtals to be generated than
      // numInitial, then the total needed will be the sum from
      // minXtalsOfSpgPerFU

      size_t numXtalsToBeGenerated = 0;
      for (size_t i = 0; i < minXtalsOfSpgPerFU.size(); i++) {
        // The value in the vector is -1 if that spg is to not be used
        if (minXtalsOfSpgPerFU.at(i) != -1)
          numXtalsToBeGenerated += (minXtalsOfSpgPerFU.at(i) * formulaUnitsList.size());
      }

      // Now that minXtalsOfSpgPerFU is set up, proceed!
      newXtalCount = failed = 0;
      // Find spacegroups for which we have required a certain number of xtals
      // per formula unit
      for (size_t i = 0; i < spgStillNeeded.size(); i++) {
        while (spgStillNeeded.at(i) > 0) {
          for (size_t FU_ind = 0; FU_ind < formulaUnitsList.size(); FU_ind++) {
            // Update the progresss bar
            updateProgressBar((numXtalsToBeGenerated - failed > numInitial ?
                               numXtalsToBeGenerated - failed : numInitial),
                              newXtalCount + failed, newXtalCount);

            uint spg = i + 1;
            uint FU = formulaUnitsList.at(FU_ind);

            // If the spacegroup isn't possible for this FU, just continue
            if (!RandSpg::isSpgPossible(spg, getStdVecOfAtoms(FU))) {
              numXtalsToBeGenerated--;
              continue;
            }

            // Generate/Check xtal
            xtal = randSpgXtal(1, newXtalCount + 1, FU, spg);
            if (!checkXtal(xtal)) {
              delete xtal;
              qWarning() << "Failed to generate an xtal with spacegroup of"
                         << QString::number(spg) << "and FU of" << QString::number(FU);
              failed++;
            }
            else {
              xtal->findSpaceGroup(tol_spg);
              initializeAndAddXtal(xtal, 1, xtal->getParents());
              newXtalCount++;
            }
          }
          spgStillNeeded[i]--;
        }
      }
      // If we still haven't generated enough xtals, pick a random FU and spg
      // to be generated
      while (newXtalCount < numInitial) {
        // Let's keep the progress bar updated
        updateProgressBar(numInitial, newXtalCount + failed, newXtalCount);

        // Randomly select a formula unit
        uint randomFU = formulaUnitsList.at(rand()%int(formulaUnitsList.size()));
        // Randomly select a possible spg
        uint randomSpg = pickRandomSpgFromPossibleOnes();//pickRandomSpgFromPossibleOnes();
        // If it isn't possible, try again
        if (!RandSpg::isSpgPossible(randomSpg, getStdVecOfAtoms(randomFU))) continue;
        // Try it out
        xtal = randSpgXtal(1, newXtalCount + 1, randomFU, randomSpg);
        if (!checkXtal(xtal)) {
          delete xtal;
          qWarning() << "Failed to generate an xtal with spacegroup of"
                     << QString::number(randomSpg) << "and FU of" << QString::number(randomFU);
          failed++;
        }
        else {
          xtal->findSpaceGroup(tol_spg);
          initializeAndAddXtal(xtal, 1, xtal->getParents());
          newXtalCount++;
        }
        // If we failed, shake it off and try again
      }
    }

    // Wait for all structures to appear in tracker
    m_dialog->updateProgressLabel(
          tr("Waiting for structures to initialize..."));
    m_dialog->updateProgressMinimum(0);
    m_dialog->updateProgressMinimum(newXtalCount);

    connect(m_tracker, SIGNAL(newStructureAdded(GlobalSearch::Structure*)),
            m_initWC, SLOT(wakeAllSlot()));

    m_initWC->prewaitLock();
    do {
      m_dialog->updateProgressValue(m_tracker->size());
      m_dialog->updateProgressLabel(
            tr("Waiting for structures to initialize (%1 of %2)...")
            .arg(m_tracker->size()).arg(newXtalCount));
      // Don't block here forever -- there is a race condition where
      // the final newStructureAdded signal may be emitted while the
      // WC is not waiting. Since this is just trivial GUI updating
      // and we check the condition in the do-while loop, this is
      // acceptable. The following call will timeout in 250 ms.
      m_initWC->wait(250);
    }
    while (m_tracker->size() < newXtalCount);
    m_initWC->postwaitUnlock();

    // We're done with m_initWC.
    m_initWC->disconnect();

    m_dialog->stopProgressUpdate();

    m_dialog->saveSession();
    emit sessionStarted();
  }

  bool XtalOpt::addSeed(const QString &filename)
  {
    QString err;
    Xtal *xtal = new Xtal;
    xtal->setFileName(filename);
    xtal->setStatus(Xtal::WaitingForOptimization);
    // Create atoms
    for (QHash<unsigned int, XtalCompositionStruct>::const_iterator
         it = comp.constBegin(), it_end = comp.constEnd();
         it != it_end; ++it) {
      for (int i = 0; i < it.value().quantity; ++i) {
        xtal->addAtom();
      }
    }

    // We will only display the warning once, so use a static bool for this
    // Use an atomic bool for thread safety
    static std::atomic_bool warningAlreadyDisplayed(false);
    if (!warningAlreadyDisplayed.load()) {
      warning("XtalOpt no longer check to make sure seed "
              "xtals obey user-defined constraints (minimum "
              "interatomic distances, min/max volume, etc.). Be sure "
              "your seed structures are reasonable.");
      warningAlreadyDisplayed = true;
    }

    xtal->moveToThread(m_queue->thread());
    if ( !m_optimizer->read(xtal, filename) || !this->checkComposition(xtal, &err)) {
      error(tr("Error loading seed %1\n\n%2").arg(filename).arg(err));
      xtal->deleteLater();
      return false;
    }
    QString parents =tr("Seeded: %1", "1 is a filename").arg(filename);
    this->m_queue->addManualStructureRequest(1);
    initializeAndAddXtal(xtal, 1, parents);
    debug(tr("XtalOpt::addSeed: Loaded seed: %1",
             "1 is a filename").arg(filename));
    return true;
  }

  Structure* XtalOpt::replaceWithRandom(Structure *s, const QString & reason)
  {

    Xtal *oldXtal = qobject_cast<Xtal*>(s);
    QWriteLocker locker1 (oldXtal->lock());

    // Decrement the parent xtal that produced this deleted structure
    oldXtal->decrementParentOffspringCounts();

    // Randomly generated xtals do not have parent structures
    oldXtal->setParentStructure(NULL);

    uint FU = s->getFormulaUnits();

    uint generation, id;
    generation = s->getGeneration();
    id = s->getIDNumber();
    // Generate/Check new xtal
    Xtal *xtal = 0;
    while (!checkXtal(xtal)) {
      if (xtal) {
        delete xtal;
        xtal = 0;
      }

      xtal = generateRandomXtal(generation, id, FU);
    }

    // Copy info over
    QWriteLocker locker2 (xtal->lock());
    oldXtal->clear();
    oldXtal->setOBUnitCell(new OpenBabel::OBUnitCell);
    oldXtal->setCellInfo(xtal->OBUnitCell()->GetCellMatrix());
    oldXtal->resetEnergy();
    oldXtal->resetEnthalpy();
    oldXtal->setPV(0);
    oldXtal->setCurrentOptStep(1);
    QString parents = "Randomly generated";
    if (!reason.isEmpty())
      parents += " (" + reason + ")";
    oldXtal->setParents(parents);
    oldXtal->setFormulaUnits(oldXtal->getFormulaUnits());

    Atom *atom1, *atom2;
    for (uint i = 0; i < xtal->numAtoms(); i++) {
      atom1 = oldXtal->addAtom();
      atom2 = xtal->atom(i);
      atom1->setPos(atom2->pos());
      atom1->setAtomicNumber(atom2->atomicNumber());
    }
    oldXtal->findSpaceGroup(tol_spg);
    oldXtal->resetFailCount();

    // Delete random xtal
    xtal->deleteLater();
    return qobject_cast<Structure*>(oldXtal);
  }

  Structure* XtalOpt::replaceWithOffspring(Structure *s,
                                           const QString & reason)
  {
    Xtal *oldXtal = qobject_cast<Xtal*>(s);

    // Decrement the parent xtal that produced this deleted structure
    oldXtal->decrementParentOffspringCounts();

    uint FU = s->getFormulaUnits();
    // Generate/Check new xtal
    Xtal *xtal = 0;
    while (!checkXtal(xtal)) {
      if (xtal) {
        xtal->deleteLater();
        xtal = NULL;
      }
      xtal = generateNewXtal(FU);
    }

    // Just return xtal if the formula units are not equivalent.
    // This should theoretically not occur since generateNewXtal(FU) forces
    // xtal to have the correct FU.
    if (xtal->getFormulaUnits() != s->getFormulaUnits()) {
      return xtal;
    }

    // Copy info over
    QWriteLocker locker1 (oldXtal->lock());
    QWriteLocker locker2 (xtal->lock());
    oldXtal->setOBUnitCell(new OpenBabel::OBUnitCell);
    oldXtal->setCellInfo(xtal->OBUnitCell()->GetCellMatrix());
    oldXtal->resetEnergy();
    oldXtal->resetEnthalpy();
    oldXtal->resetFailCount();
    oldXtal->setPV(0);
    oldXtal->setCurrentOptStep(1);
    if (!reason.isEmpty()) {
      QString parents = xtal->getParents();
      parents += " (" + reason + ")";
      oldXtal->setParents(parents);
    }

    oldXtal->setParentStructure(xtal->getParentStructure());

    Q_ASSERT_X(xtal->numAtoms() == oldXtal->numAtoms(), Q_FUNC_INFO,
               "Number of atoms don't match. Cannot copy.");

    for (uint i = 0; i < xtal->numAtoms(); ++i) {
      (*oldXtal->atom(i)) = (*xtal->atom(i));
    }
    oldXtal->findSpaceGroup(tol_spg);

    // Delete random xtal
    xtal->deleteLater();
    return static_cast<Structure*>(oldXtal);
  }

  Xtal* XtalOpt::randSpgXtal(uint generation, uint id, uint FU, uint spg)
  {
    INIT_RANDOM_GENERATOR();

    Xtal* xtal = NULL;
    QString* err = NULL;

    // Let's make the spg input
    latticeStruct latticeMins, latticeMaxes;
    setLatticeMinsAndMaxes(latticeMins, latticeMaxes);

    // Create the input
    randSpgInput input(spg, getStdVecOfAtoms(FU), latticeMins, latticeMaxes);

    // Add various other input options
    input.IADScalingFactor = scaleFactor;
    input.minRadius = minRadius;
    input.minVolume = vol_min * static_cast<double>(FU);
    input.maxVolume = vol_max * static_cast<double>(FU);
    input.maxAttempts = 10;
    input.verbosity = 'n';
    // This removes the guarantee that we will generate the right space group,
    // but we will just check it with spglib
    input.forceMostGeneralWyckPos = false;

    // Let's try this 3 times
    size_t numAttempts = 0;
    do {
      numAttempts++;
      xtal = RandSpgXtalOptWrapper::randSpgXtal(input);
      // So that we don't crash the program, make sure the xtal exists
      // before attempting to get its spacegroup number
      if (xtal) {
        // If we succeed, we're done!
        if (xtal->getSpaceGroupNumber() == spg) break;
      }
    } while (numAttempts < 3);

    // Make sure we don't call xtal->getSpaceGroupNumber() until we know that
    // we have an xtal
    if (xtal) {
      if (xtal->getSpaceGroupNumber() != spg) {
        delete xtal;
        xtal = 0;
      }
    }

    // We need to set these things before checkXtal() is called
    if (xtal) {
      xtal->setStatus(Xtal::WaitingForOptimization);
      xtal->setFormulaUnits(FU);
      if (using_fixed_volume) xtal->setVolume(vol_fixed * FU);
    }
    else {
      qDebug() << "After" << QString::number(input.maxAttempts)
               << "attempts, failed to generate an xtal with spg of"
               << QString::number(spg);
      return NULL;
    }

    // Openbabel crashes when you try to retrieve the HM for spg = 230
    // for some reason...
    QString HM_spg = "";
    if (spg != 230)
      HM_spg = QString::fromStdString(OpenBabel::SpaceGroup::GetSpaceGroup(
                                                          spg)->GetHMName());
    else HM_spg = "Ia-3d";

    // Set up xtal data
    xtal->setGeneration(generation);
    xtal->setIDNumber(id);
    xtal->setParents(tr("Spg Init: %1 (%2)").arg(spg).arg(HM_spg));
    return xtal;
  }

  Xtal* XtalOpt::generateRandomXtal(uint generation, uint id, uint FU)
  {
    INIT_RANDOM_GENERATOR();

    // Set cell parameters
    double a, b, c, alpha, beta, gamma;

    // Create a valid crystal first
    Xtal *xtal = NULL;
    do {
      delete xtal;
      xtal = NULL;
      a            = RANDDOUBLE() * (a_max-a_min) + a_min;
      b            = RANDDOUBLE() * (b_max-b_min) + b_min;
      c            = RANDDOUBLE() * (c_max-c_min) + c_min;
      alpha        = RANDDOUBLE() * (alpha_max - alpha_min) + alpha_min;
      beta         = RANDDOUBLE() * (beta_max  - beta_min ) + beta_min;
      gamma        = RANDDOUBLE() * (gamma_max - gamma_min) + gamma_min;
      xtal = new Xtal(a, b, c, alpha, beta, gamma);
      xtal->setFormulaUnits(FU);
    } while (!checkLattice(xtal));

    QWriteLocker locker (xtal->lock());

    xtal->setStatus(Xtal::Empty);

    if (using_fixed_volume)
      xtal->setVolume(vol_fixed * FU);

    // In case we rescaled the cell, update a, b, and c
    a = xtal->getA();
    b = xtal->getB();
    c = xtal->getC();

    // Populate crystal
    QList<uint> atomicNums = comp.keys();
    // Sort atomic number by decreasing minimum radius. Adding the "larger"
    // atoms first encourages a more even (and ordered) distribution
    for (int i = 0; i < atomicNums.size()-1; ++i) {
      for (int j = i + 1; j < atomicNums.size(); ++j) {
        if (this->comp.value(atomicNums[i]).minRadius <
            this->comp.value(atomicNums[j]).minRadius) {
          atomicNums.swap(i,j);
        }
      }
    }

    unsigned int atomicNum;
    int qRand;
    int qTotal;
    int qRandPre;
    int qRandPost;

    //Mitosis = True
    if (using_mitosis){
      //  Unit Cell Vectors
      int A = ax;
      int B = bx;
      int C = cx;

      a = a / A;
      b = b / B;
      c = c / C;

      xtal->setCellInfo(a,
              b,
              c,
              xtal->getAlpha(),
              xtal->getBeta(),
              xtal->getGamma());

      //First check for "no center" MolUnits
      for (QHash<QPair<int, int>, MolUnit>::const_iterator it = this->compMolUnit.constBegin(), it_end = this->compMolUnit.constEnd(); it != it_end; it++) {
        QPair<int, int> key = const_cast<QPair<int, int> &>(it.key());
        if (key.first == 0) {
          for (int i = 0; i < it->numCenters * FU / divisions; i++) {
            if (!xtal->addAtomRandomly(key.first, key.second, this->comp,
                    this->compMolUnit, true)) {
              xtal->deleteLater();
              debug("XtalOpt::generateRandomXtal: Failed to add atoms with "
                    "specified interatomic distance.");
              return 0;
            }
          }
        }
      }

      for (int num_idx = 0; num_idx < atomicNums.size(); num_idx++) {
        atomicNum = atomicNums.at(num_idx);
        if (atomicNum ==0)
          continue;
        qTotal = comp.value(atomicNum).quantity * FU;

        // Do we use the MolUnit builder?
        bool addAtom = true;
        bool useMolUnit = false;
        for (QHash<QPair<int, int>, MolUnit>::const_iterator it = this->compMolUnit.constBegin(), it_end = this->compMolUnit.constEnd(); it != it_end; it++) {
          QPair<int, int> key = const_cast<QPair<int, int> &>(it.key());
          int first = key.first;
          if (atomicNum==first) {
            useMolUnit = true;
            break;
          }
        }

        // Do we add Atom or has it already been placed by MolUnit builder
        unsigned int qCenter = 0;
        unsigned int qNeighbor = 0;
        for (QHash<QPair<int, int>, MolUnit>::const_iterator it = this->compMolUnit.constBegin(), it_end = this->compMolUnit.constEnd(); it != it_end; it++) {
          QPair<int, int> key = const_cast<QPair<int, int> &>(it.key());
          if (atomicNum == key.first) {
            qCenter += it->numCenters * FU;
          }
          if (atomicNum == key.second) {
            qNeighbor += it->numCenters * it->numNeighbors * FU;
          }
        }

        if (qCenter / divisions == 0)
          useMolUnit = false;

        if (qTotal == qCenter + qNeighbor) {
          addAtom = false;
          qRandPre = 0;
        } else {
          qRandPre = (qTotal - (qCenter + qNeighbor)) / divisions;
        }

        //Initial atom placement
        for (uint i = 0; i < qRandPre; i++) {
          if (addAtom == true) {
            if (!xtal->addAtomRandomly(atomicNum, this->comp)) {
              xtal->deleteLater();
              debug("XtalOpt::generateRandomXtal: Failed to add atoms with "
                    "specified interatomic distance.");
              return 0;
            }
          }
        }

        // Add atom with MolUnit builder or Randomly
        if (useMolUnit == true) {
          for (QHash<QPair<int, int>, MolUnit>::const_iterator it = this->compMolUnit.constBegin(), it_end = this->compMolUnit.constEnd(); it != it_end; it++) {
            QPair<int, int> key = const_cast<QPair<int, int> &>(it.key());
            if (atomicNum == key.first) {
              for (int i = 0; i < it->numCenters * FU / divisions; i++) {
                if (!xtal->addAtomRandomly(atomicNum, key.second, this->comp, this->compMolUnit, useMolUnit)) {
                  xtal->deleteLater();
                  debug("XtalOpt::generateRandomXtal: Failed to add atoms with "
                        "specified interatomic distance.");
                  return 0;
                }
              }
            }
          }
        }
      }

      // Print subcell if checked
      if (using_subcellPrint) printSubXtal(xtal, generation, id);

      // Fill supercell by copying subcell according to parameters
      if (!xtal->fillSuperCell(A, B, C, xtal)) {
        xtal->deleteLater();
        debug("XtalOpt::generateRandomXtal: Failed to add atoms.");
        return 0;
      }


      // Randomly place the left over atoms
      for (int num_idx = 0; num_idx < atomicNums.size(); num_idx++) {
        atomicNum = atomicNums.at(num_idx);
        qTotal = (comp.value(atomicNum).quantity * FU);

        // Do we use the MolUnit builder?
        bool addAtom = true;
        bool useMolUnit = false;
        for (QHash<QPair<int, int>, MolUnit>::const_iterator it = this->compMolUnit.constBegin(), it_end = this->compMolUnit.constEnd(); it != it_end; it++) {
          QPair<int, int> key = const_cast<QPair<int, int> &>(it.key());
          int first = key.first;
          if (atomicNum==first) {
            useMolUnit = true;
            break;
          }
        }

        // Do we add Atom or has it already been placed by MolUnit builder
        unsigned int qCenter = 0;
        unsigned int qNeighbor = 0;
        for (QHash<QPair<int, int>, MolUnit>::const_iterator it = this->compMolUnit.constBegin(), it_end = this->compMolUnit.constEnd(); it != it_end; it++) {
          QPair<int, int> key = const_cast<QPair<int, int> &>(it.key());
          if (atomicNum == key.first) {
            qCenter += it->numCenters * FU;
          }
          if (atomicNum == key.second) {
            qNeighbor += it->numCenters * it->numNeighbors * FU;
          }
        }

        if (qTotal == qCenter + qNeighbor) {
          addAtom = false;
          qRandPost = 0;
        } else {
          qRandPre = (qTotal - (qCenter + qNeighbor)) / divisions;
          qRandPost = qTotal - (qCenter + qNeighbor) - (qRandPre * divisions);
        }

        if (qCenter / divisions > 0)
          qCenter -= (qCenter / divisions) * divisions;

        //Initial atom placement
        for (uint i = 0; i < qRandPost; i++) {
          if (addAtom == true) {
            if (!xtal->addAtomRandomly(atomicNum, this->comp)) {
              xtal->deleteLater();
              debug("XtalOpt::generateRandomXtal: Failed to add atoms with "
                    "specified interatomic distance.");
              return 0;
            }
          }
        }

        // Add atom with MolUnit builder or Randomly
        if (useMolUnit == true) {
          for (QHash<QPair<int, int>, MolUnit>::const_iterator it = this->compMolUnit.constBegin(), it_end = this->compMolUnit.constEnd(); it != it_end; it++) {
            QPair<int, int> key = const_cast<QPair<int, int> &>(it.key());
            if (atomicNum == key.first) {
              for (int i = 0; i < it->numCenters * FU % divisions; i++) {
                if (!xtal->addAtomRandomly(atomicNum, key.second, this->comp, this->compMolUnit, useMolUnit)) {
                  xtal->deleteLater();
                  debug("XtalOpt::generateRandomXtal: Failed to add atoms with "
                        "specified interatomic distance.");
                  return 0;
                }
              }
            }
          }
        }
      }

    //Mitosis = False
    } else {

      //First check for "no center" MolUnits
      for (QHash<QPair<int, int>, MolUnit>::const_iterator it = this->compMolUnit.constBegin(), it_end = this->compMolUnit.constEnd(); it != it_end; it++) {
        QPair<int, int> key = const_cast<QPair<int, int> &>(it.key());
        if (key.first == 0) {
          for (int i = 0; i < it->numCenters * FU; i++) {
            if (!xtal->addAtomRandomly(key.first, key.second, this->comp,
                    this->compMolUnit, true)) {
              xtal->deleteLater();
              debug("XtalOpt::generateRandomXtal: Failed to add atoms with "
                    "specified interatomic distance.");
              return 0;
            }
          }
        }
      }

      for (int num_idx = 0; num_idx < atomicNums.size(); num_idx++) {
        // To avoid messing up the stoichiometry with the MolUnit builder
        atomicNum = atomicNums.at(num_idx);
        qRand = comp.value(atomicNum).quantity * FU;

        if (atomicNum == 0)
          continue;

        bool addAtom = true;
        bool useMolUnit = false;

        for (QHash<QPair<int, int>, MolUnit>::const_iterator it = this->compMolUnit.constBegin(), it_end = this->compMolUnit.constEnd(); it != it_end; it++) {
          QPair<int, int> key = const_cast<QPair<int, int> &>(it.key());
          if (atomicNum == key.first) {
            useMolUnit = true;
            break;
          }
        }

        unsigned int qCenter = 0;
        unsigned int qNeighbor = 0;
        for (QHash<QPair<int, int>, MolUnit>::const_iterator it = this->compMolUnit.constBegin(), it_end = this->compMolUnit.constEnd(); it != it_end; it++) {
          QPair<int, int> key = const_cast<QPair<int, int> &>(it.key());
          if (atomicNum == key.first) {
            qCenter += it->numCenters * FU;
          }
          if (atomicNum == key.second) {
            qNeighbor += it->numCenters * it->numNeighbors * FU;
          }
        }

        if (qRand == qCenter + qNeighbor) {
          addAtom = false;
          qRand -= qCenter + qNeighbor;
        } else {
          qRand -= qCenter + qNeighbor;
        }

        //Initial atom placement
        for (uint i = 0; i < qRand; i++) {
          if (addAtom == true) {
            if (!xtal->addAtomRandomly(atomicNum, this->comp)) {
              xtal->deleteLater();
              debug("XtalOpt::generateRandomXtal: Failed to add atoms with "
                    "specified interatomic distance.");
              return 0;
            }
          }
        }


        if (useMolUnit == true) {
          for (QHash<QPair<int, int>, MolUnit>::const_iterator it = this->compMolUnit.constBegin(), it_end = this->compMolUnit.constEnd(); it != it_end; it++) {
            QPair<int, int> key = const_cast<QPair<int, int> &>(it.key());
            if (atomicNum == key.first) {
              for (int i = 0; i < it->numCenters * FU; i++) {
                if (!xtal->addAtomRandomly(atomicNum, key.second, this->comp, this->compMolUnit, useMolUnit)) {
                  xtal->deleteLater();
                  debug("XtalOpt::generateRandomXtal: Failed to add atoms with "
                        "specified interatomic distance.");
                  return 0;
                }
              }
            }
          }
        }
      }
    }

    // Set up geneology info
    xtal->setGeneration(generation);
    xtal->setIDNumber(id);
    xtal->setParents("Randomly generated");
    xtal->setFormulaUnits(FU);
    xtal->setStatus(Xtal::WaitingForOptimization);

    // Set up xtal data
    return xtal;
  }

  void XtalOpt::printSubXtal(Xtal *xtal, uint generation,
                                     uint id)
    {
    xtalInitMutex->lock();

    QString id_s, gen_s, locpath_s;
    id_s.sprintf("%05d",id);
    gen_s.sprintf("%05d",generation);
    locpath_s = filePath + "/subcells";
    QDir dir (locpath_s);
    if (!dir.exists()) {
      if (!dir.mkpath(locpath_s)) {
        error(tr("XtalOpt::initializeSubXtal: Cannot write to path: %1 "
                 "(path creation failure)", "1 is a file path.")
              .arg(locpath_s));
      }
    }
    QFile loc_subcell;
    loc_subcell.setFileName(locpath_s + "/" + gen_s + "x" + id_s + ".cml");

    if (!loc_subcell.open(QIODevice::WriteOnly)) {
                    error("XtalOpt::initializeSubXtal(): Error opening file "+loc_subcell.fileName()+" for writing...");
    }

    QTextStream out;
    out.setDevice(&loc_subcell);

    /*
    QStringList symbols = xtal->getSymbols();
    QList<unsigned int> atomCounts = xtal->getNumberOfAtomsAlpha();
    Q_ASSERT_X(symbols.size() == atomCounts.size(), Q_FUNC_INFO,
               "xtal->getSymbols is not the same size as xtal->getNumberOfAtomsAlpha.");
    for (unsigned int i = 0; i < symbols.size(); i++) {
      out << QString("%1%2").arg(symbols[i]).arg(atomCounts[i]);
    }
    out << " ";
    out << xtal->fileName();
    out << "\n";
    // Scaling factor. Just 1.0
    out << QString::number(1.0);
    out << "\n";
    // Unit Cell Vectors
    std::vector< vector3 > vecs = xtal->OBUnitCell()->GetCellVectors();
    for (uint i = 0; i < vecs.size(); i++) {
      out << QString("  %1 %2 %3\n")
        .arg(vecs[i].x(), 12, 'f', 8)
        .arg(vecs[i].y(), 12, 'f', 8)
        .arg(vecs[i].z(), 12, 'f', 8);
    }
    // Number of each type of atom (sorted alphabetically by symbol)
    for (int i = 0; i < atomCounts.size(); i++) {
      out << QString::number(atomCounts.at(i)) + " ";
    }
    out << "\n";
    // Use fractional coordinates:
    out << "Direct\n";
    // Coordinates of each atom (sorted alphabetically by symbol)
    QList<Eigen::Vector3d> coords = xtal->getAtomCoordsFrac();
    for (int i = 0; i < coords.size(); i++) {
      out << QString("  %1 %2 %3\n")
        .arg(coords[i].x(), 12, 'f', 8)
        .arg(coords[i].y(), 12, 'f', 8)
        .arg(coords[i].z(), 12, 'f', 8);
    }
    out << endl;
*/

// Print the subcells as .cml files
    QStringList symbols = xtal->getSymbols();
    QList<unsigned int> atomCounts = xtal->getNumberOfAtomsAlpha();
    out << "<molecule>\n";
    out << "\t<crystal>\n";

    // Unit Cell Vectors
    std::vector< vector3 > vecs = xtal->OBUnitCell()->GetCellVectors();
      out << QString("\t\t<scalar title=\"a\" units=\"units:angstrom\">%1</scalar>\n")
        .arg(vecs[0].x(), 12);
      out << QString("\t\t<scalar title=\"b\" units=\"units:angstrom\">%1</scalar>\n")
        .arg(vecs[1].y(), 12, 'f', 8);
      out << QString("\t\t<scalar title=\"c\" units=\"units:angstrom\">%1</scalar>\n")
        .arg(vecs[2].z(), 12, 'f', 8);

    // Unit Cell Angles
    out << QString("\t\t<scalar title=\"alpha\" units=\"units:degree\">%1</scalar>\n")
      .arg(xtal->OBUnitCell()->GetAlpha(), 12, 'f', 8);
    out << QString("\t\t<scalar title=\"beta\" units=\"units:degree\">%1</scalar>\n")
      .arg(xtal->OBUnitCell()->GetBeta(), 12, 'f', 8);
    out << QString("\t\t<scalar title=\"gamma\" units=\"units:degree\">%1</scalar>\n")
      .arg(xtal->OBUnitCell()->GetGamma(), 12, 'f', 8);

    out << "\t</crystal>\n";
    out << "\t<atomArray>\n";

    int symbolCount = 0;
    int j = 1;
    // Coordinates of each atom (sorted alphabetically by symbol)
    QList<Eigen::Vector3d> coords = xtal->getAtomCoordsFrac();
    for (int i = 0; i < coords.size(); i++) {
      if (j > atomCounts[symbolCount]) {
          symbolCount++;
          j = 0;
      }
      j++;
      out << QString("\t\t<atom id=\"a%1\" elementType=\"%2\" xFract=\"%3\" yFract=\"%4\" zFract=\"%5\"/>\n")
        .arg(i+1)
        .arg(symbols[symbolCount])
        .arg(coords[i].x(), 12, 'f', 8)
        .arg(coords[i].y(), 12, 'f', 8)
        .arg(coords[i].z(), 12, 'f', 8);
    }

    out << "\t</atomArray>\n";
    out << "</molecule>\n";
    out << endl;

    xtalInitMutex->unlock();
  }

  // Overloaded version of generateRandomXtal(uint generation, uint id, uint FU) without FU specified
  Xtal* XtalOpt::generateRandomXtal(uint generation, uint id)
  {
    INIT_RANDOM_GENERATOR();
    QList<uint> tempFormulaUnitsList = formulaUnitsList;
    if (using_mitotic_growth && !using_one_pool) {
      // Remove formula units on the list for which there is a smaller multiple that may be used to create a super cell.
      for (int i = 0; i < tempFormulaUnitsList.size(); i++) {
        for (int j = i + 1; j < tempFormulaUnitsList.size(); j++) {
          if (tempFormulaUnitsList.at(j) % tempFormulaUnitsList.at(i) == 0) {
            tempFormulaUnitsList.removeAt(j);
            j--;
          }
        }
      }
    }

    // We will assume modulo bias will be small since formula unit ranges are typically small. Pick random formula units.
    uint randomListIndex = rand()%int(tempFormulaUnitsList.size()); //PSA alter FU
    uint FU = tempFormulaUnitsList.at(randomListIndex);

    return generateRandomXtal(generation, id, FU);
  }

  void XtalOpt::initializeAndAddXtal(Xtal *xtal, uint generation,
                                     const QString &parents)
  {
    xtalInitMutex->lock();
    QList<Structure*> allStructures = m_queue->lockForNaming();
    Structure *structure;
    uint id = 1;
    for (int j = 0; j < allStructures.size(); j++) {
      structure = allStructures.at(j);
      structure->lock()->lockForRead();
      if (structure->getGeneration() == generation &&
          structure->getIDNumber() >= id) {
        id = structure->getIDNumber() + 1;
      }
      structure->lock()->unlock();
    }

    QWriteLocker xtalLocker (xtal->lock());
    xtal->moveToThread(m_queueThread);
    xtal->setIDNumber(id);
    xtal->setGeneration(generation);
    xtal->setParents(parents);

    // If the formula units haven't been set yet, Structure::getFormulaUnits()
    // finds the formula units with a formula-unit-finding algorithm. Setting
    // the formula units allows Structure::getFormulaUnits() to run faster.
    xtal->setFormulaUnits(xtal->getFormulaUnits());
    QString id_s, gen_s, locpath_s, rempath_s;
    id_s.sprintf("%05d",xtal->getIDNumber());
    gen_s.sprintf("%05d",xtal->getGeneration());
    locpath_s = filePath + "/" + gen_s + "x" + id_s + "/";
    rempath_s = rempath + "/" + gen_s + "x" + id_s + "/";
    QDir dir (locpath_s);
    if (!dir.exists()) {
      if (!dir.mkpath(locpath_s)) {
        error(tr("XtalOpt::initializeAndAddXtal: Cannot write to path: %1 "
                 "(path creation failure)", "1 is a file path.")
              .arg(locpath_s));
      }
    }
    //xtal->moveToThread(m_tracker->thread());
    xtal->setupConnections();
    xtal->setFileName(locpath_s);
    xtal->setRempath(rempath_s);
    xtal->setCurrentOptStep(1);
    // If none of the cell parameters are fixed, perform a normalization on
    // the lattice (currently a Niggli reduction)
    if (fabs(a_min     - a_max)     > 0.01 &&
        fabs(b_min     - b_max)     > 0.01 &&
        fabs(c_min     - c_max)     > 0.01 &&
        fabs(alpha_min - alpha_max) > 0.01 &&
        fabs(beta_min  - beta_max)  > 0.01 &&
        fabs(gamma_min - gamma_max) > 0.01) {
      xtal->fixAngles();
    }
    xtal->findSpaceGroup(tol_spg);
    xtalLocker.unlock();
    xtal->update();
    m_queue->unlockForNaming(xtal);
    xtalInitMutex->unlock();
  }

  void XtalOpt::generateNewStructure()
  {
    // Generate in background thread:
    QtConcurrent::run(this, &XtalOpt::generateNewStructure_);
  }

  void XtalOpt::generateNewStructure_()
  {
    Xtal *newXtal = generateNewXtal();
    initializeAndAddXtal(newXtal, newXtal->getGeneration(),
                         newXtal->getParents());
  }

  // Identical to the previous generateNewXtal() function except the number formula units to use have been specified
  Xtal* XtalOpt::generateNewXtal(uint FU)
  {
    QList<Structure*> structures;

    // If we are NOT using one pool. FU == 0 implies that we are using one pool
    if (!using_one_pool) {

      // We want to include supercell structures so that each individual formula
      // unit can build off of supercells in their own gene pool. One supercell
      // may be a duplicate of another supercell, though, and we don't want to
      // include duplicate supercells. This function fixes that for us.
      structures = m_queue->getAllOptimizedStructuresAndOneSupercellCopyForEachFormulaUnit();

      // Remove all structures that do not have formula units of FU
      for (size_t i = 0; i < structures.size(); i++) {
        if (structures.at(i)->getFormulaUnits() != FU) {
          structures.removeAt(i);
          i--;
        }
      }
    }

    // Just remove non-optimized structures if using_one_pool
    else if (using_one_pool) {
      structures = m_queue->getAllOptimizedStructures();
    }

    // Remove all structures that are not on the formula units list...
    for (size_t i = 0; i < structures.size(); i++) {
      if (!onTheFormulaUnitsList(structures.at(i)->getFormulaUnits())) {
        structures.removeAt(i);
        i--;
      }
    }

    // Check to see if there are enough optimized structure to perform
    // genetic operations
    if (structures.size() < 3) {
      Xtal *xtal = 0;

      // Check to see if a supercell should be formed by mitosis
      if (using_mitotic_growth && FU != 0) {
        QList<Structure*> tempStructures = m_queue->getAllOptimizedStructuresAndOneSupercellCopyForEachFormulaUnit();
        QList<uint> numberOfEachFormulaUnit = Structure::countStructuresOfEachFormulaUnit(&tempStructures, maxFU);

        // The number of formula units to use to make the super cell must be a multiple of the larger formula unit, and there must be as many at least five optimized structures. If there aren't, then generate more.
        for (int i = FU - 1; 0 < i; i--) {
          if (FU % i == 0 && numberOfEachFormulaUnit.at(i) >= 5 && onTheFormulaUnitsList(i) == true) {
            while (!checkXtal(xtal)) {
              if (xtal) {
                delete xtal;
                xtal = 0;
              }
            xtal = generateSuperCell(i, FU, 0, true, true);
            }
            return xtal;
          }

          // Generates more of a particular formula unit if everything checks except that there aren't enough parent structures. Must be on the formulaUnitsList.
          if (FU % i == 0 && numberOfEachFormulaUnit.at(i) < 5 && onTheFormulaUnitsList(i) == true) {
            xtal = generateNewXtal(i);
            QString parents = xtal->getParents();
            parents = parents + tr(" for mitosis to make %1 FU xtals").arg(QString::number(FU));
            xtal->setParents(parents);
            return xtal;
          }
        }
      }

      // If a supercell cannot be formed or if using_mitotic_growth == false
      while (!checkXtal(xtal)) {
        if (xtal) xtal->deleteLater();
        if (!using_one_pool) xtal = generateRandomXtal(1, 0, FU);
        else if (using_one_pool) xtal = generateRandomXtal(1, 0);
      }
      xtal->setParents(xtal->getParents() + " (too few optimized structures "
                       "to generate offspring)");
      return xtal;
    }

    Xtal* xtal = H_getMutatedXtal(structures, FU);
    return xtal;
  }

  // Overloaded function of generateNewXtal(uint FU)
  Xtal* XtalOpt::generateNewXtal()
  {
    // Check to see if there are any structures that need to be primitive
    // reduced or if there are supercells that needs to be generated. If
    // there are, then generate and return one.
    QList<Structure*> optimizedStructures =
                                           m_queue->getAllOptimizedStructures();
    Xtal *testXtal;
    if (supercellCheckLock.tryLockForWrite()) {
      for (size_t i = 0; i < optimizedStructures.size(); i++) {
        // generateNewXtal() runs concurrently. We don't want to make more than
        // one primitive/supercell xtal of a given xtal, so try to write for
        // lock. If it can't be done, then just continue to the next one.
        if (!optimizedStructures.at(i)->lock()->tryLockForWrite()) continue;

        // If the structure has been primitive checked, we don't need to check
        // it again. If it hasn't been duplicate checked, yet, let it be
        // duplicate checked first (so we don't check unnecessary structures).
        if (!optimizedStructures.at(i)->wasPrimitiveChecked() &&
            !optimizedStructures.at(i)->hasChangedSinceDupChecked()) {
          testXtal = qobject_cast<Xtal*>(optimizedStructures.at(i));
          // If testXtal is found to not be primitive, make a new xtal that is
          // the primitive of testXtal.
          if (!testXtal->isPrimitive(tol_spg)) {
            testXtal->setPrimitiveChecked(true);
            Xtal* nxtal = generatePrimitiveXtal(testXtal);
            testXtal->lock()->unlock();
            // This will continue the structure generation while simultaneously
            // unlocking supercellCheckLock in 0.1 second
            // This allows time for the structure to be finished before
            // checking to see if another supercell could be generated
            waitThenUnlockSupercellCheckLock();
            return nxtal;
          }
          testXtal->setPrimitiveChecked(true);
        }

        // Now let's check to see if a supercell should be generated from
        // the optimized structure
        if (!optimizedStructures.at(i)->wasSupercellGenerationChecked()) {
          // If the optimized structure's enthalpy is not the lowest enthalpy
          // of it's formula unit set, set the supercell generation to be
          // true and just continue to the next structure in the loop.
          // For some reason, even though the datatypes are all doubles, it
          // does not appear that we can do a direct comparison between the
          // lowestEnthalpyFUList and the structure's enthalpy. So, we will
          // just do a basic percent diff comparison instead. If the difference
          // is less than 0.001%, then we will assume they are the same
          uint FU = optimizedStructures.at(i)->getFormulaUnits();
          double percentDiff = fabs((optimizedStructures.at(i)->getEnthalpy() -
                                     lowestEnthalpyFUList.at(FU)) /
                                     lowestEnthalpyFUList.at(FU) * 100.00000);
          if (percentDiff > 0.001) {
            optimizedStructures.at(i)->setSupercellGenerationChecked(true);
            optimizedStructures.at(i)->lock()->unlock();
            continue;
          }
          double enthalpyPerAtom1 = optimizedStructures.at(i)->getEnthalpy() /
                                    static_cast<double>(
                                    optimizedStructures.at(i)->numAtoms());
          uint numAtomsPerFU = optimizedStructures.at(i)->numAtoms() /
                               optimizedStructures.at(i)->getFormulaUnits();
          for (size_t j = 1; j <= maxFU; j++) {
            if (!onTheFormulaUnitsList(j)) continue;
            // j represents a formula unit that is being checked.
            // If optimizedStructures.at(i) can create a supercell with formula
            // units of j and optimizedStructures.at(i)'s enthalpy/atom is
            // smaller than the smallest so-far discovered enthalpy/atom at that
            // FU, and if there is a difference greater than 3 meV/atom between
            // the two, build a supercell and add it to the gene pool
            double enthalpyPerAtom2 = (lowestEnthalpyFUList.at(j) /
                                      static_cast<double>(j)) /
                                      static_cast<double>(numAtomsPerFU);
            if (j != optimizedStructures.at(i)->getFormulaUnits() &&
                j %  optimizedStructures.at(i)->getFormulaUnits() == 0 &&
                (enthalpyPerAtom1 < enthalpyPerAtom2 || enthalpyPerAtom2 == 0)){
              // enthalpyDiff is in meV
              double enthalpyDiff = fabs(enthalpyPerAtom1 - enthalpyPerAtom2) *
                                    1000.0000000;
              if (enthalpyDiff <= 3.000000) continue;
              else {
                Xtal* xtal = qobject_cast<Xtal*>(optimizedStructures.at(i));
                // We may need to create more than one supercell from a given
                // xtal, so only update this if it is generating an xtal with
                // the maxFU
                if (j == maxFU) xtal->setSupercellGenerationChecked(true);
                xtal->lock()->unlock();
                Xtal* nxtal = generateSuperCell(xtal->getFormulaUnits(), j,
                                                xtal, true, false);
                xtal->lock()->lockForRead();
                nxtal->setParents(tr("Supercell generated from %1x%2")
                  .arg(xtal->getGeneration())
                  .arg(xtal->getIDNumber()));
                // We only want to perform offspring tracking for mutated
                // offspring.
                nxtal->setParentStructure(NULL);
                nxtal->setFormulaUnits(nxtal->getFormulaUnits());
                nxtal->setEnthalpy(xtal->getEnthalpy() *
                         nxtal->getFormulaUnits() /
                         xtal->getFormulaUnits());
                nxtal->setEnergy(xtal->getEnergy() *
                         nxtal->getFormulaUnits() /
                         xtal->getFormulaUnits() *
                         EV_TO_KJ_PER_MOL);
                nxtal->setPrimitiveChecked(true);
                nxtal->setSkippedOptimization(true);
                nxtal->setStatus(Xtal::Optimized);
                xtal->lock()->unlock();
                // This will continue the structure generation while
                // simultaneously unlocking supercellCheckLock in 0.1 second
                // This allows time for the structure to be finished before
                // checking to see if another supercell could be generated
                waitThenUnlockSupercellCheckLock();
                return nxtal;
              }
            }
          }
          optimizedStructures.at(i)->setSupercellGenerationChecked(true);
        }
        optimizedStructures.at(i)->lock()->unlock();
      }
      supercellCheckLock.unlock();
    }
    // Having finished doing primitive reduction and supercell generation
    // checks, we can now continue!

    // Inputing a formula unit of 0 implies using all formula units when
    // generating the probability list.
    if (using_one_pool) {
      return generateNewXtal(0);
    }

    // Get all structures to count numbers of each formula unit
    QList<Structure*> allStructures = m_queue->getAllStructures();

    // Count the number of structures of each formula unit
    QList<uint> numberOfEachFormulaUnit =
           Structure::countStructuresOfEachFormulaUnit(&allStructures,
                                                                maxFU);

    // If there are not yet at least 5 of any one FU, make more of that FU
    // Will generate smaller FU's first
    for (int i = minFU; i <= maxFU; ++i) {
      if ( (numberOfEachFormulaUnit.at(i) < 5) && (onTheFormulaUnitsList(i) == true)) {
        uint FU = i;
        return generateNewXtal(FU);
      }
    }

    // Find the formula unit with the smallest number of total structures.
    uint smallest = numberOfEachFormulaUnit.at(minFU);
    for (int i = minFU; i <= maxFU; ++i) {
      if ( (numberOfEachFormulaUnit.at(i) < smallest) && (onTheFormulaUnitsList(i) == true)) {
        smallest = numberOfEachFormulaUnit.at(i);
      }
    }

    // Pick the formula unit with the smallest number of optimized structures. If there are two or more formula units that have the smallest number of optimized structures, pick the smallest
    uint FU;
    for (int i = minFU; i <= maxFU; i++) {
      if ( (numberOfEachFormulaUnit.at(i) == smallest) && (onTheFormulaUnitsList(i) == true)) {
        FU = i;
        break;
      }
    }

    return generateNewXtal(FU);
  }

  // preselectedXtal is NULL by default
  // includeCrossover is true by default
  // includeMitosis is also true by default
  Xtal* XtalOpt::H_getMutatedXtal(QList<Structure*>& structures, int FU,
                                  Xtal* preselectedXtal, bool includeCrossover,
                                  bool includeMitosis, bool mitosisMutation)
  {

    INIT_RANDOM_GENERATOR();

    // Initialize loop vars
    double r;
    unsigned int gen;
    QString parents;
    Xtal *xtal = NULL, *selectedXtal = NULL;

    // Perform operation until xtal is valid:
    while (!checkXtal(xtal)) {
      // First delete any previous failed structure in xtal
      if (xtal) {
        xtal->deleteLater();
        xtal = 0;
      }

      // If an xtal hasn't been preselected, select one
      if (!preselectedXtal)
        selectedXtal = selectXtalFromProbabilityList(structures, FU);
      else
        selectedXtal = preselectedXtal;

      // Decide operator:
      r = RANDDOUBLE();

      // We will perform mitosis if:
      // 1: using_one_pool is enabled and
      // 2: the xtal selected can produce an FU on the list through mitosis and
      // 3: the probability succeeds
      if (includeMitosis && using_one_pool) {
       // Find candidate formula units to be created through mitosis
        QList<uint> possibleMitosisFU_index;
        for (int i = 0; i < formulaUnitsList.size(); i++) {
          if (formulaUnitsList.at(i) % selectedXtal->getFormulaUnits() == 0 &&
                formulaUnitsList.at(i) != selectedXtal->getFormulaUnits()) {
            possibleMitosisFU_index.append(i);
          }
        }

        // If no FU's may be created by mitosis, just continue
        if (!possibleMitosisFU_index.isEmpty()) {
          // If the probability fails, delete xtal and continue
          if (r <= chance_of_mitosis/100.0) {
            // Select an index randomly from the possibleMitosisFU_index
            uint randomListIndex = rand()%int(possibleMitosisFU_index.size());
            uint selectedIndex = possibleMitosisFU_index.at(randomListIndex);
            // Use that selected index to choose the formula units
            uint formulaUnits = formulaUnitsList.at(selectedIndex);
            // Perform mitosis
            Xtal *nxtal = NULL;
            while (!checkXtal(nxtal)) {
              if (nxtal) {
                delete nxtal;
                nxtal = 0;
              }

              nxtal = generateSuperCell(selectedXtal->getFormulaUnits(),
                                        formulaUnits, selectedXtal, true, true);
            }
            return nxtal;
          }
        }
      }

      Operators op;
      // Include the crossover in the selection
      if (includeCrossover) {
        if (r < p_cross/100.0)
          op = OP_Crossover;
        else if (r < (p_cross + p_strip)/100.0)
          op = OP_Stripple;
        else
          op = OP_Permustrain;
      }
      // In some cases (like in generateSuperCell()), we may
      // not want to perform crossover. Instead, renormalize
      // the percentages for stripple and permustrain and choose one
      else {
        double temp_p_strip = 100.0 * p_strip / (100.0 - p_cross);
        if (r < temp_p_strip / 100.0)
          op = OP_Stripple;
        else
          op = OP_Permustrain;
      }

      // Try 1000 times to get a good structure from the selected
      // operation. If not possible, send a warning to the log and
      // start anew.
      int attemptCount = 0;
      while (attemptCount < 1000 && !checkXtal(xtal)) {
        attemptCount++;
        if (xtal) {
          delete xtal;
          xtal = 0;
        }

        // Operation specific set up:
        switch (op) {
        case OP_Crossover: {
          Xtal *xtal1=0, *xtal2=0;
          // Select structures
          double percent1;
          double percent2;

          // If FU crossovers have been enabled, generate a new breeding pool
          // that includes multiple different formula units then run the
          // alternative crossover function
          bool enoughStructures = true;
          if (using_FU_crossovers) {
            // Get all optimized structures
            QList<Structure*> tempStructures =
                                           m_queue->getAllOptimizedStructures();

            // Trim all the structures that aren't of the allowed generation or
            // greater
            for (int i = 0; i < tempStructures.size(); i++) {
              if (tempStructures.at(i)->getGeneration() < FU_crossovers_generation) {
                tempStructures.removeAt(i);
                i--;
              }
            }

            // Only continue if there are at least 3 of the allowed generation or greater
            if (tempStructures.size() < 3) enoughStructures = false;
            if (enoughStructures) {

              xtal1 = selectXtalFromProbabilityList(tempStructures);
              xtal2 = selectXtalFromProbabilityList(tempStructures);

              // Perform operation
              xtal = XtalOptGenetic::FUcrossover(
                    xtal1, xtal2, cross_minimumContribution, percent1, percent2,
                    formulaUnitsList, this->comp);
            }
          }

          // Perform a regular crossover instead!
          if (!using_FU_crossovers || !enoughStructures) {
            xtal1 = selectedXtal;
            xtal2 = selectXtalFromProbabilityList(structures,
                                                  xtal1->getFormulaUnits());

            // Perform operation
            xtal = XtalOptGenetic::crossover(
                  xtal1, xtal2, cross_minimumContribution, percent1);
          }

          // Lock parents and get info from them
          xtal1->lock()->lockForRead();
          xtal2->lock()->lockForRead();
          uint gen1 = xtal1->getGeneration();
          uint gen2 = xtal2->getGeneration();
          uint id1 = xtal1->getIDNumber();
          uint id2 = xtal2->getIDNumber();
          xtal2->lock()->unlock();
          xtal1->lock()->unlock();

          // We will set the parent xtal of this xtal to be
          // the parent that contributed the most
          if (percent1 >= 50.0) xtal->setParentStructure(xtal1);
          else xtal->setParentStructure(xtal2);

          // Determine generation number
          gen = ( gen1 >= gen2 ) ?
            gen1 + 1 :
            gen2 + 1 ;

          // A regular crossover was performed. So percent2 will
          // simply be 100.0 - percent1. This may not be the case
          // for an FU Crossover
          if (!using_FU_crossovers || !enoughStructures)
            percent2 = 100.0 - percent1;

          parents = tr("Crossover: %1x%2 (%3%) + %4x%5 (%6%)")
            .arg(gen1)
            .arg(id1)
            .arg(percent1, 0, 'f', 0)
            .arg(gen2)
            .arg(id2)
            .arg(percent2, 0, 'f', 0);
          // To identify that a formula unit crossover was performed...
          if (using_FU_crossovers && enoughStructures)
            parents.prepend("FU ");
          continue;
        }
        case OP_Stripple: {

          // Perform stripple
          double amplitude=0, stdev=0;
          xtal = XtalOptGenetic::stripple(selectedXtal,
                                          strip_strainStdev_min,
                                          strip_strainStdev_max,
                                          strip_amp_min,
                                          strip_amp_max,
                                          strip_per1,
                                          strip_per2,
                                          stdev,
                                          amplitude);

          // Lock parent and extract info
          selectedXtal->lock()->lockForRead();
          uint gen1 = selectedXtal->getGeneration();
          uint id1 = selectedXtal->getIDNumber();

          // If it's a mitosis mutation, the parent xtal is already set
          if (!mitosisMutation)
            xtal->setParentStructure(selectedXtal);
          selectedXtal->lock()->unlock();

          // Determine generation number
          gen = gen1 + 1;
          // A regular mutation is being performed
          if (!mitosisMutation)
            parents = tr("Stripple: %1x%2 stdev=%3 amp=%4 waves=%5,%6")
              .arg(gen1)
              .arg(id1)
              .arg(stdev, 0, 'f', 5)
              .arg(amplitude, 0, 'f', 5)
              .arg(strip_per1)
              .arg(strip_per2);
          // Modified version of setting the parents.
          // selectedXtal->getParents() should be something like "mitosis of 1x1 followed by "
          else
            parents = selectedXtal->getParents() + tr("Stripple: stdev=%1 amp=%2 waves=%3,%4")
              .arg(stdev, 0, 'f', 5)
              .arg(amplitude, 0, 'f', 5)
              .arg(strip_per1)
              .arg(strip_per2);
          continue;
        }
        case OP_Permustrain: {

          double stdev=0;

          xtal = XtalOptGenetic::permustrain(
                selectedXtal, perm_strainStdev_max, perm_ex, stdev);

          // Lock parent and extract info
          selectedXtal->lock()->lockForRead();
          uint gen1 = selectedXtal->getGeneration();
          uint id1 = selectedXtal->getIDNumber();

          // If it's a mitosis mutation, the parent xtal is already set
          if (!mitosisMutation)
            xtal->setParentStructure(selectedXtal);
          selectedXtal->lock()->unlock();

          // Determine generation number
          gen = gen1 + 1;
          // Set the ancestry like normal...
          if (!mitosisMutation)
            parents = tr("Permustrain: %1x%2 stdev=%3 exch=%4")
              .arg(gen1)
              .arg(id1)
              .arg(stdev, 0, 'f', 5)
              .arg(perm_ex);
          // selectedXtal->getParents() should be something like "mitosis of 1x1 followed by "
          // modified settings for ancestry
          else {
            parents = selectedXtal->getParents() + tr("Permustrain: stdev=%1 exch=%2")
              .arg(stdev, 0, 'f', 5)
              .arg(perm_ex);
          }
          continue;
        }
        default:
          warning("XtalOpt::generateSingleOffspring: Attempt to use an "
                  "invalid operator.");
        }
      }
      if (attemptCount >= 1000) {
        QString opStr;
        switch (op) {
        case OP_Crossover:   opStr = "crossover"; break;
        case OP_Stripple:    opStr = "stripple"; break;
        case OP_Permustrain: opStr = "permustrain"; break;
        default:             opStr = "(unknown)"; break;
        }
        warning(tr("Unable to perform operation %1 after 1000 tries. "
                   "Reselecting operator...").arg(opStr));
      }
    }

    xtal->setGeneration(gen);
    xtal->setParents(parents);
    Xtal* parentXtal;
    if (!mitosisMutation)
      parentXtal = qobject_cast<Xtal*>(xtal->getParentStructure());
    else {
      parentXtal = qobject_cast<Xtal*>(selectedXtal->getParentStructure());
      xtal->setParentStructure(parentXtal);
    }

    return xtal;
  }

  Xtal* XtalOpt::generatePrimitiveXtal(Xtal* xtal)
  {
    Xtal* nxtal = new Xtal();
    // Copy cell over from xtal to nxtal
    nxtal->setCellInfo(xtal->OBUnitCell()->GetCellMatrix());
    Atom* newAtom;
    // Add the atoms in...
    for (size_t i = 0; i < xtal->numAtoms(); i++) {
      newAtom = nxtal->addAtom();
      newAtom->setAtomicNumber(xtal->atom(i)->atomicNumber());
      newAtom->setPos(xtal->atom(i)->pos());
    }
    // Reduce it to primitive...
    nxtal->reduceToPrimitive(tol_spg);
    uint gen = xtal->getGeneration() + 1;
    QString parents = tr("Primitive of %1x%2")
      .arg(xtal->getGeneration())
      .arg(xtal->getIDNumber());
    nxtal->setGeneration(gen);
    nxtal->setParents(parents);
    nxtal->setFormulaUnits(nxtal->getFormulaUnits());
    nxtal->setEnthalpy(xtal->getEnthalpy() *
                       nxtal->getFormulaUnits() /
                       xtal->getFormulaUnits());
    nxtal->setEnergy(xtal->getEnergy() *
                     nxtal->getFormulaUnits() /
                     xtal->getFormulaUnits() *
                     EV_TO_KJ_PER_MOL);
    nxtal->setPrimitiveChecked(true);
    nxtal->setSkippedOptimization(true);
    nxtal->setStatus(Xtal::Optimized);
    return nxtal;
  }

  // If myXtal is NULL, it returns a new dynamically allocated xtal
  Xtal* XtalOpt::generateSuperCell(uint initialFU, uint finalFU, Xtal *myXtal,
                                   bool setupNewXtal, bool mutate) {

    // If (myXtal == 0), select an xtal from the probability list
    if (setupNewXtal == true) {
      Xtal *xtal = NULL;
      if (myXtal == 0) {
        QList<Structure*> structures = m_queue->getAllOptimizedStructuresAndOneSupercellCopyForEachFormulaUnit();
        xtal = selectXtalFromProbabilityList(structures, initialFU);
      }
      // If !setupNewXtal, and the parent is already set,
      // transfer the parent over to xtal and set myXtal = 0 just for
      // ease later in function.
      else if (myXtal != 0) {
        xtal = myXtal;
        myXtal = 0;
      }

      unsigned int gen;
      QString parents;

      // lock parent xtal for reading
      QReadLocker locker (xtal->lock());

      // Copy info over from parent to new xtal
      myXtal = new Xtal;
      myXtal->setCellInfo(xtal->OBUnitCell()->GetCellMatrix());
      QWriteLocker nxtalLocker (myXtal->lock());
      QList<Atom*> atoms = xtal->atoms();
      for (int i = 0; i < atoms.size(); i++) {
        Atom* atom = myXtal->addAtom();
        atom->setAtomicNumber(atoms.at(i)->atomicNumber());
        atom->setPos(atoms.at(i)->pos());
      }

      // Lock parent and extract info
      xtal->lock()->lockForRead();
      uint gen1 = xtal->getGeneration();
      uint id1 = xtal->getIDNumber();
      xtal->lock()->unlock();

      // Determine generation number
      // This parent info will be erased and replaced with a different parent
      // if we are doing just a supercell generation. If we are doing
      // a mutation afterwards, the mutation description comes after
      // "followed by "
      parents = tr("%1x%2 mitosis followed by ")
        .arg(gen1)
        .arg(id1);
      myXtal->setParentStructure(xtal);
      myXtal->setParents(parents);
      myXtal->setGeneration(gen1 + 1);
    }

    // Find the largest prime number multiple. We will expand
    // upon the shortest length with this number. We will perform
    // the other duplications through recursion of this whole function.

    uint numberOfDuplicates = finalFU / initialFU;
    for (int i = 2; i < numberOfDuplicates; i++) {
      if (numberOfDuplicates % i == 0) {
        numberOfDuplicates = numberOfDuplicates / i;
        i = 2;
      }
    }

    // a, b, and c are the number of duplicates in the A, B, and C direction, respectively.
    uint a = 1;
    uint b = 1;
    uint c = 1;

    // Find the shortest length. We will expand upon this length.
    double A = myXtal->getA();
    double B = myXtal->getB();
    double C = myXtal->getC();

    if (A <= B && A <= C) {
      a = numberOfDuplicates;
    }
    else if (B <= A && B <= C) {
      b = numberOfDuplicates;
    }
    else if (C <= A && C <= B) {
      c = numberOfDuplicates;
    }

    QList<Atom*> oneFUatoms = myXtal->atoms();

    // Credit for the next 37 lines of this function goes to Zack Falls!
    // First get OB matrix, extract vectors, then convert to Eigen::Vector3d's
    matrix3x3 obcellMatrix = myXtal->OBUnitCell()->GetCellMatrix();
    OpenBabel::vector3 obU1 = obcellMatrix.GetRow(0);
    OpenBabel::vector3 obU2 = obcellMatrix.GetRow(1);
    OpenBabel::vector3 obU3 = obcellMatrix.GetRow(2);
    // Scale cell
    myXtal->setCellInfo(a * A,
                        b * B,
                        c * C,
                        myXtal->getAlpha(),
                        myXtal->getBeta(),
                        myXtal->getGamma());
    a--;
    b--;
    c--;

    for (int i = 0; i <= a; i++) {
      for (int j = 0; j <= b; j++) {
        for (int k = 0; k <= c; k++) {
          if (i == 0 && j == 0 && k == 0) continue;
          Eigen::Vector3d uVecs(
                  obU1.x() * i + obU2.x() * j + obU3.x() * k,
                  obU1.y() * i + obU2.y() * j + obU3.y() * k,
                  obU1.z() * i + obU2.z() * j + obU3.z() * k);
          // Add the atoms in
          foreach(Atom *atom, oneFUatoms) {
              Atom *newAtom = myXtal->addAtom();
              *newAtom = *atom;
              newAtom->setPos((*atom->pos())+uVecs);
              newAtom->setAtomicNumber(atom->atomicNumber());
          }
        }
      }
    }

    // Recursively perform mitosis until the final FU is reached
    if (myXtal->getFormulaUnits() != finalFU) {
      return generateSuperCell(myXtal->getFormulaUnits(), finalFU, myXtal,
                                 false, mutate);
      }

    // Perform genetic operation immediately after mitosis
    else {
      if (mutate) {
        // If xtal is already selected, no structure list is needed for
        // parameter 1. So we will use an empty list.
        // Technically, parameter 2 is not needed either.
        // Parameter 3 is the selected xtal to mutate.
        // Parameter 4 is includeCrossover and parameter 5 is includeMitosis.
        // Parameter 6 is mitosisMutation (changes the way the parents are set)
        QList<Structure*> temp;
        Xtal* xtal = H_getMutatedXtal(temp, FU, myXtal, false, false, true);
        return xtal;
      }
      else {
        return myXtal;
      }
    }
  }

  Xtal* XtalOpt::selectXtalFromProbabilityList(QList<Structure*> structures,
                                               uint FU) {
    INIT_RANDOM_GENERATOR();

    // Remove all structures that have an FU that ISN'T on the list
    for (size_t i = 0; i < structures.size(); i++) {
      if (!onTheFormulaUnitsList(structures.at(i)->getFormulaUnits())) {
        structures.removeAt(i);
        i--;
      }
    }

    if (FU != 0) {
      // Remove all structures that do not have formula units of FU
      for (int i = 0; i < structures.size(); i++) {
        if (structures.at(i)->getFormulaUnits() != FU) {
          structures.removeAt(i);
          i--;
        }
      }
    }

    // Sort structure list
    Structure::sortByEnthalpy(&structures);

    // Trim list
    // Remove all but (popSize + 1). The "+ 1" will be removed
    // during probability generation.
    while ( static_cast<uint>(structures.size()) > popSize + 1 ) {
      structures.removeLast();
    }

    QList<double> probs = getProbabilityList(structures);

    // Cast Structures into Xtals
    QList<Xtal*> xtals;
#if QT_VERSION >= 0x040700
    xtals.reserve(structures.size());
#endif // QT_VERSION
    for (int i = 0; i < structures.size(); ++i) {
      xtals.append(qobject_cast<Xtal*>(structures.at(i)));
    }

    // Initialize loop vars
    double r;
    Xtal *xtal = NULL;

    // Pick a parent
    int ind;
    r = RANDDOUBLE();
    for (ind = 0; ind < probs.size(); ind++)
      if (r < probs.at(ind)) break;
    xtal = xtals.at(ind);
    return xtal;
  }

  bool XtalOpt::checkLimits() {
    if (a_min > a_max) {
      warning("XtalOptRand::checkLimits error: Illogical A limits.");
      return false;
    }
    if (b_min > b_max) {
      warning("XtalOptRand::checkLimits error: Illogical B limits.");
      return false;
    }
    if (c_min > c_max) {
      warning("XtalOptRand::checkLimits error: Illogical C limits.");
      return false;
    }
    if (alpha_min > alpha_max) {
      warning("XtalOptRand::checkLimits error: Illogical Alpha limits.");
      return false;
    }
    if (beta_min > beta_max) {
      warning("XtalOptRand::checkLimits error: Illogical Beta limits.");
      return false;
    }
    if (gamma_min > gamma_max) {
      warning("XtalOptRand::checkLimits error: Illogical Gamma limits.");
      return false;
    }

    // Check to make sure at least one formula unit can be made with
    // the specified lengths and volume constraints
    bool anyFails = false;
    for (int i = 0; i < formulaUnitsList.size(); i++) {
      if (
          ( using_fixed_volume &&
            ( (a_min * b_min * c_min) > (vol_fixed * static_cast<double>(formulaUnitsList.at(i))) ||
              (a_max * b_max * c_max) < (vol_fixed * static_cast<double>(formulaUnitsList.at(i))) )
            ) ||
          ( !using_fixed_volume &&
            ( (a_min * b_min * c_min) > (vol_max * static_cast<double>(formulaUnitsList.at(i))) ||
              (a_max * b_max * c_max) < (vol_min * static_cast<double>(formulaUnitsList.at(i))) ||
              vol_min > vol_max)
            )) {
      warning(tr("XtalOptRand::checkLimits error: Illogical Volume limits for %1 FU. "
              "(Also check min/max volumes based on cell lengths)").arg(QString::number(formulaUnitsList.at(i))));
        anyFails = true;
      }
    }

    if (anyFails == true) return false;

    return true;
  }

  bool XtalOpt::checkComposition(Xtal *xtal, QString * err) {
    // Check composition
    QList<unsigned int> atomTypes = comp.keys();
    QList<unsigned int> atomCounts;
#if QT_VERSION >= 0x040700
    atomCounts.reserve(atomTypes.size());
#endif // QT_VERSION
    for (size_t i = 0; i < atomTypes.size(); ++i) {
      atomCounts.append(0);
    }

    // Count atoms of each type
    for (size_t i = 0; i < xtal->numAtoms(); ++i) {
      int typeIndex = atomTypes.indexOf(
            static_cast<unsigned int>(xtal->atom(i)->atomicNumber()));
      // Type not found:
      if (typeIndex == -1) {
        qDebug() << "XtalOpt::checkXtal: Composition incorrect.";
        if (err != NULL) {
          *err = "Bad composition.";
        }
        return false;
      }
      ++atomCounts[typeIndex];
    }

    // Check counts. Adjust for formula units.
    for (size_t i = 0; i < atomTypes.size(); ++i) {
      if (atomCounts[i] != comp[atomTypes[i]].quantity * xtal->getFormulaUnits()) { //PSA
        qDebug() << "atomCounts for atomic num "
                 << QString::number(atomTypes[i]) << "is"
                 << QString::number(atomCounts[i]) << ". It should be "
                 << QString::number(comp[atomTypes[i]].quantity * xtal->getFormulaUnits())
                 << "instead.";
        qDebug() << "FU is " << QString::number(xtal->getFormulaUnits()) << " and comp[atomTypes[i]].quantity is " << QString::number(comp[atomTypes[i]].quantity);
        // Incorrect count:
        qDebug() << "XtalOpt::checkXtal: Composition incorrect.";
        if (err != NULL) {
          *err = "Bad composition.";
        }
        return false;
      }
    }
    return true;
  }

  // Xtal should be write-locked before calling this function
  bool XtalOpt::checkLattice(Xtal *xtal, QString * err)
  {
    // Adjust max and min constraints depending on the formula unit
    new_vol_max = static_cast<double>(xtal->getFormulaUnits()) * vol_max;
    new_vol_min = static_cast<double>(xtal->getFormulaUnits()) * vol_min;

    // Check volume
    if (using_fixed_volume) {
      xtal->setVolume(vol_fixed * static_cast<double>(xtal->getFormulaUnits()));
    }
    else if ( xtal->getVolume() < new_vol_min || //PSA
              xtal->getVolume() > new_vol_max) { //PSA
      // I don't want to initialize a random number generator here, so
      // just use the modulus of the current volume as a random float.
      double newvol = fabs(fmod(xtal->getVolume(), 1)) *
          (new_vol_max - new_vol_min) + new_vol_min;
      // If the user has set vol_min to 0, we can end up with a null
      // volume. Fix this here. This is just to keep things stable
      // numerically during the rescaling -- it's unlikely that other
      // cells with small, nonzero volumes will pass the other checks
      // so long as other limits are reasonable.
      if (fabs(newvol) < 1.0) {
        newvol = (new_vol_max - new_vol_min)*0.5 + new_vol_min; //PSA;
      }
      //qDebug() << "XtalOpt::checkXtal: Rescaling volume from "
      //         << xtal->getVolume() << " to " << newvol;
      xtal->setVolume(newvol);
    }

    // Scale to any fixed parameters
    double a, b, c, alpha, beta, gamma;
    a = b = c = alpha = beta = gamma = 0;
    if (fabs(a_min - a_max) < 0.01) a = a_min;
    if (fabs(b_min - b_max) < 0.01) b = b_min;
    if (fabs(c_min - c_max) < 0.01) c = c_min;
    if (fabs(alpha_min - alpha_max) < 0.01) alpha = alpha_min;
    if (fabs(beta_min -  beta_max)  < 0.01)  beta = beta_min;
    if (fabs(gamma_min - gamma_max) < 0.01) gamma = gamma_min;
    xtal->rescaleCell(a, b, c, alpha, beta, gamma);

    // Reject the structure if using VASP and the determinant of the
    // cell matrix is negative (otherwise VASP complains about a
    // "negative triple product")
    if (qobject_cast<VASPOptimizer*>(m_optimizer) != 0 &&
        xtal->OBUnitCell()->GetCellMatrix().determinant() <= 0.0) {
      qDebug() << "Rejecting structure" << xtal->getIDString()
               << ": using VASP negative triple product.";
      if (err != NULL) {
        *err = "Unit cell matrix cannot have a negative triple product "
            "when using VASP.";
      }
      return false;
    }

    // Before fixing angles, make sure that the current cell
    // parameters are realistic
    if (GS_IS_NAN_OR_INF(xtal->getA()) || fabs(xtal->getA()) < 1e-8 ||
        GS_IS_NAN_OR_INF(xtal->getB()) || fabs(xtal->getB()) < 1e-8 ||
        GS_IS_NAN_OR_INF(xtal->getC()) || fabs(xtal->getC()) < 1e-8 ||
        GS_IS_NAN_OR_INF(xtal->getAlpha()) || fabs(xtal->getAlpha()) < 1e-8 ||
        GS_IS_NAN_OR_INF(xtal->getBeta())  || fabs(xtal->getBeta())  < 1e-8 ||
        GS_IS_NAN_OR_INF(xtal->getGamma()) || fabs(xtal->getGamma()) < 1e-8 ) {
      qDebug() << "XtalOpt::checkXtal: A cell parameter is either 0, nan, or "
                  "inf. Discarding.";
      if (err != NULL) {
        *err = "A cell parameter is too small (<10^-8) or not a number.";
      }
      return false;
    }

    // If no cell parameters are fixed, normalize lattice
    if (fabs(a + b + c + alpha + beta + gamma) < 1e-8) {
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
        qDebug() << "Error: one of the lengths is more than 25x shorter "
                 << "than another length. Crystals like these can sometimes "
                 << "cause spglib to crash the program. Discarding the xtal:";
        xtal->printXtalInfo();
        return false;
      }
      // Check that the angles aren't 25x different than the others as well
      if (xtal->getAlpha() * cutoff < xtal->getBeta() ||
          xtal->getAlpha() * cutoff < xtal->getGamma() ||
          xtal->getBeta()  * cutoff < xtal->getAlpha() ||
          xtal->getBeta()  * cutoff < xtal->getGamma() ||
          xtal->getGamma() * cutoff < xtal->getAlpha() ||
          xtal->getGamma() * cutoff < xtal->getBeta()) {
        qDebug() << "Error: one of the angles is more than 25x smaller "
                 << "than another angle. Crystals like these can sometimes "
                 << "cause spglib to crash the program. Discarding the xtal:";
        xtal->printXtalInfo();
        return false;
      }

      xtal->fixAngles();
    }

    // Check lattice
    if ( ( !a     && ( xtal->getA() < a_min         || xtal->getA() > a_max         ) ) ||
         ( !b     && ( xtal->getB() < b_min         || xtal->getB() > b_max         ) ) ||
         ( !c     && ( xtal->getC() < c_min         || xtal->getC() > c_max         ) ) ||
         ( !alpha && ( xtal->getAlpha() < alpha_min || xtal->getAlpha() > alpha_max ) ) ||
         ( !beta  && ( xtal->getBeta()  < beta_min  || xtal->getBeta()  > beta_max  ) ) ||
         ( !gamma && ( xtal->getGamma() < gamma_min || xtal->getGamma() > gamma_max ) ) )  {
      qDebug() << "Discarding structure -- Bad lattice:" <<endl
               << "A:     " << a_min << " " << xtal->getA() << " " << a_max << endl
               << "B:     " << b_min << " " << xtal->getB() << " " << b_max << endl
               << "C:     " << c_min << " " << xtal->getC() << " " << c_max << endl
               << "Alpha: " << alpha_min << " " << xtal->getAlpha() << " " << alpha_max << endl
               << "Beta:  " << beta_min  << " " << xtal->getBeta()  << " " << beta_max << endl
               << "Gamma: " << gamma_min << " " << xtal->getGamma() << " " << gamma_max;
      if (err != NULL) {
        *err = "The unit cell parameters do not fall within the specified "
            "limits.";
      }
      return false;
    }

    // We made it!
    return true;
  }

  bool XtalOpt::checkXtal(Xtal *xtal, QString * err) {
    if (!xtal) {
      if (err != NULL) {
        *err = "Xtal pointer is NULL.";
      }
      return false;
    }

    // Lock xtal
    QWriteLocker locker (xtal->lock());

    if (xtal->getStatus() == Xtal::Empty) {
      if (err != NULL) {
        *err = "Xtal status is empty.";
      }
      return false;
    }

    if (!checkComposition(xtal, err))
      return false;

    if (!checkLattice(xtal, err))
      return false;

    // Sometimes, all the atom positions are set to 'nan' for an unknown reason
    // Make sure that the position of the first atom is not nan
    if (GS_IS_NAN_OR_INF(xtal->atoms().at(0)->pos()->x())) {
      qDebug() << "Discarding structure -- contains 'nan' atom positions";
      return false;
    }

    // Check interatomic distances
    if (using_interatomicDistanceLimit) {
      int atom1, atom2;
      double IAD;
      if (!xtal->checkInteratomicDistances(this->comp, &atom1, &atom2, &IAD)){
        Atom *a1 = xtal->atom(atom1);
        Atom *a2 = xtal->atom(atom2);
        const double minIAD =
            this->comp.value(a1->atomicNumber()).minRadius +
            this->comp.value(a2->atomicNumber()).minRadius;

        qDebug() << "Discarding structure -- Bad IAD ("
                 << IAD << " < "
                 << minIAD << ")";
        if (err != NULL) {
          *err = "Two atoms are too close together.";
        }
        return false;
      }
    }

    // Xtal is OK!
    if (err != NULL) {
      *err = "";
    }
    return true;
  }

  QString XtalOpt::interpretTemplate(const QString & templateString,
                                     Structure* structure)
  {
    QStringList list = templateString.split("%");
    QString line;
    QString origLine;
    Xtal *xtal = qobject_cast<Xtal*>(structure);
    for (int line_ind = 0; line_ind < list.size(); line_ind++) {
      origLine = line = list.at(line_ind);
      interpretKeyword_base(line, structure);
      interpretKeyword(line, structure);
      if (line != origLine) { // Line was a keyword
        list.replace(line_ind, line);
      }
    }
    // Rejoin string
    QString ret = list.join("");
    ret += "\n";
    return ret;
  }

  void XtalOpt::interpretKeyword(QString &line, Structure* structure)
  {
    QString rep = "";
    Xtal *xtal = qobject_cast<Xtal*>(structure);

    // Xtal specific keywords
    if (line == "a")                    rep += QString::number(xtal->getA());
    else if (line == "b")               rep += QString::number(xtal->getB());
    else if (line == "c")               rep += QString::number(xtal->getC());
    else if (line == "alphaRad")        rep += QString::number(xtal->getAlpha() * DEG_TO_RAD);
    else if (line == "betaRad")         rep += QString::number(xtal->getBeta() * DEG_TO_RAD);
    else if (line == "gammaRad")        rep += QString::number(xtal->getGamma() * DEG_TO_RAD);
    else if (line == "alphaDeg")        rep += QString::number(xtal->getAlpha());
    else if (line == "betaDeg")         rep += QString::number(xtal->getBeta());
    else if (line == "gammaDeg")        rep += QString::number(xtal->getGamma());
    else if (line == "volume")          rep += QString::number(xtal->getVolume());
    else if (line == "block")           rep += QString("\%block");
    else if (line == "endblock")        rep += QString("\%endblock");
    else if (line == "coordsFrac") {
      QList<Atom*> atoms = structure->atoms();
      QList<Atom*>::const_iterator it;
      for (it  = atoms.begin();
           it != atoms.end();
           it++) {
        const Eigen::Vector3d coords = xtal->cartToFrac(*((*it)->pos()));
        rep += static_cast<QString>(OpenBabel::etab.GetSymbol((*it)->atomicNumber())) + " ";
        rep += QString::number(coords.x()) + " ";
        rep += QString::number(coords.y()) + " ";
        rep += QString::number(coords.z()) + "\n";
      }
    }
    else if (line == "chemicalSpeciesLabel") {
      QList<unsigned int> atomCounts = xtal->getNumberOfAtomsAlpha();
      QList<QString> symbol = xtal->getSymbols();
      for (int i = 0; i < atomCounts.size(); i++) {
        rep += " ";
        rep += QString::number(i+1) + " ";
        rep += QString::number(atomCounts.at(i)) + " ";
        rep += symbol.at(i) + "\n";
      }
    }
    else if (line == "atomicCoordsAndAtomicSpecies") {
      QList<Atom*> atoms = xtal->atoms();
      QList<Atom*>::const_iterator it;
      QList<QString> symbol = xtal->getSymbols();
      for (it  = atoms.begin();
           it != atoms.end();
           it++) {
        const Eigen::Vector3d coords = xtal->cartToFrac(*(*it)->pos());
        QString currAtom = static_cast<QString>(OpenBabel::etab.GetSymbol((*it)->atomicNumber()));
        int i = symbol.indexOf(currAtom)+1;
        rep += " ";
        QString inp;
        inp.sprintf("%4.8f", coords.x());
        rep += inp + "\t";
        inp.sprintf("%4.8f", coords.y());
        rep += inp + "\t";
        inp.sprintf("%4.8f", coords.z());
        rep += inp + "\t";
        rep += QString::number(i) + "\n";
      }
    }
    else if (line == "coordsFracId") {
      QList<Atom*> atoms = structure->atoms();
      QList<Atom*>::const_iterator it;
      for (it  = atoms.begin();
           it != atoms.end();
           it++) {
        const Eigen::Vector3d coords = xtal->cartToFrac(*(*it)->pos());
        rep += static_cast<QString>(OpenBabel::etab.GetSymbol((*it)->atomicNumber())) + " ";
        rep += QString::number((*it)->atomicNumber()) + " ";
        rep += QString::number(coords.x()) + " ";
        rep += QString::number(coords.y()) + " ";
        rep += QString::number(coords.z()) + "\n";
      }
    }
    else if (line == "gulpFracShell") {
      QList<Atom*> atoms = structure->atoms();
      QList<Atom*>::const_iterator it;
      for (it  = atoms.begin();
           it != atoms.end();
           it++) {
        const Eigen::Vector3d coords = xtal->cartToFrac(*((*it)->pos()));
        const char *symbol = OpenBabel::etab.GetSymbol((*it)->atomicNumber());
        rep += QString("%1 core %2 %3 %4\n")
            .arg(symbol).arg(coords.x()).arg(coords.y()).arg(coords.z());
        rep += QString("%1 shel %2 %3 %4\n")
            .arg(symbol).arg(coords.x()).arg(coords.y()).arg(coords.z());
        }
    }
    else if (line == "cellMatrixAngstrom") {
      matrix3x3 m = xtal->OBUnitCell()->GetCellMatrix();
      for (int i = 0; i < 3; i++) {
        rep += " ";
        for (int j = 0; j < 3; j++) {
          QString inp;
          inp.sprintf("%4.8f", m.Get(i,j));
          rep += inp + "\t";
        }
        rep += "\n";
      }
    }
    else if (line == "cellVector1Angstrom") {
      vector3 v = xtal->OBUnitCell()->GetCellVectors()[0];
      for (int i = 0; i < 3; i++) {
        rep += QString::number(v[i]) + "\t";
      }
    }
    else if (line == "cellVector2Angstrom") {
      vector3 v = xtal->OBUnitCell()->GetCellVectors()[1];
      for (int i = 0; i < 3; i++) {
        rep += QString::number(v[i]) + "\t";
      }
    }
    else if (line == "cellVector3Angstrom") {
      vector3 v = xtal->OBUnitCell()->GetCellVectors()[2];
      for (int i = 0; i < 3; i++) {
        rep += QString::number(v[i]) + "\t";
      }
    }
    else if (line == "cellMatrixBohr") {
      matrix3x3 m = xtal->OBUnitCell()->GetCellMatrix();
      for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
          rep += QString::number(m.Get(i,j) * ANGSTROM_TO_BOHR) + "\t";
        }
        rep += "\n";
      }
    }
    else if (line == "cellVector1Bohr") {
      vector3 v = xtal->OBUnitCell()->GetCellVectors()[0];
      for (int i = 0; i < 3; i++) {
        rep += QString::number(v[i] * ANGSTROM_TO_BOHR) + "\t";
      }
    }
    else if (line == "cellVector2Bohr") {
      vector3 v = xtal->OBUnitCell()->GetCellVectors()[1];
      for (int i = 0; i < 3; i++) {
        rep += QString::number(v[i] * ANGSTROM_TO_BOHR) + "\t";
      }
    }
    else if (line == "cellVector3Bohr") {
      vector3 v = xtal->OBUnitCell()->GetCellVectors()[2];
      for (int i = 0; i < 3; i++) {
        rep += QString::number(v[i] * ANGSTROM_TO_BOHR) + "\t";
      }
    }
    else if (line == "POSCAR") {
      // Comment line -- set to composition then filename
      // Construct composition
      QStringList symbols = xtal->getSymbols();
      QList<unsigned int> atomCounts = xtal->getNumberOfAtomsAlpha();
      Q_ASSERT_X(symbols.size() == atomCounts.size(), Q_FUNC_INFO,
                 "xtal->getSymbols is not the same size as xtal->getNumberOfAtomsAlpha.");
      for (unsigned int i = 0; i < symbols.size(); i++) {
        rep += QString("%1%2").arg(symbols[i]).arg(atomCounts[i]);
      }
      rep += " ";
      rep += xtal->fileName();
      rep += "\n";
      // Scaling factor. Just 1.0
      rep += QString::number(1.0);
      rep += "\n";
      // Unit Cell Vectors
      std::vector< vector3 > vecs = xtal->OBUnitCell()->GetCellVectors();
      for (uint i = 0; i < vecs.size(); i++) {
        rep += QString("  %1 %2 %3\n")
          .arg(vecs[i].x(), 12, 'f', 8)
          .arg(vecs[i].y(), 12, 'f', 8)
          .arg(vecs[i].z(), 12, 'f', 8);
      }
      // Number of each type of atom (sorted alphabetically by symbol)
      for (int i = 0; i < atomCounts.size(); i++) {
        rep += QString::number(atomCounts.at(i)) + " ";
      }
      rep += "\n";
      // Use fractional coordinates:
      rep += "Direct\n";
      // Coordinates of each atom (sorted alphabetically by symbol)
      QList<Eigen::Vector3d> coords = xtal->getAtomCoordsFrac();
      for (int i = 0; i < coords.size(); i++) {
        rep += QString("  %1 %2 %3\n")
          .arg(coords[i].x(), 12, 'f', 8)
          .arg(coords[i].y(), 12, 'f', 8)
          .arg(coords[i].z(), 12, 'f', 8);
      }
    } // End %POSCAR%

    if (!rep.isEmpty()) {
      // Remove any trailing newlines
      rep = rep.replace(QRegExp("\n$"), "");
      line = rep;
    }
  }

  QString XtalOpt::getTemplateKeywordHelp()
  {
    QString help = "";
    help.append(getTemplateKeywordHelp_base());
    help.append("\n");
    help.append(getTemplateKeywordHelp_xtalopt());
    return help;
  }

  QString XtalOpt::getTemplateKeywordHelp_xtalopt()
  {
    QString str;
    QTextStream out (&str);
    out
      << "Crystal specific information:\n"
      << "%POSCAR% -- VASP poscar generator\n"
      << "%coordsFrac% -- fractional coordinate data\n\t[symbol] [x] [y] [z]\n"
      << "%coordsFracId% -- fractional coordinate data with atomic number\n\t[symbol] [atomic number] [x] [y] [z]\n"
      << "%gulpFracShell% -- fractional coordinates for use in GULP core/shell calculations:\n"
         "\tBoth of the following are printed for each atom:\n"
         "\t[symbol] core [x] [y] [z]\n"
         "\t[symbol] shell [x] [y] [z]\n"
      << "%cellMatrixAngstrom% -- Cell matrix in Angstrom\n"
      << "%cellVector1Angstrom% -- First cell vector in Angstrom\n"
      << "%cellVector2Angstrom% -- Second cell vector in Angstrom\n"
      << "%cellVector3Angstrom% -- Third cell vector in Angstrom\n"
      << "%cellMatrixBohr% -- Cell matrix in Bohr\n"
      << "%cellVector1Bohr% -- First cell vector in Bohr\n"
      << "%cellVector2Bohr% -- Second cell vector in Bohr\n"
      << "%cellVector3Bohr% -- Third cell vector in Bohr\n"
      << "%a% -- Lattice parameter A\n"
      << "%b% -- Lattice parameter B\n"
      << "%c% -- Lattice parameter C\n"
      << "%alphaRad% -- Lattice parameter Alpha in rad\n"
      << "%betaRad% -- Lattice parameter Beta in rad\n"
      << "%gammaRad% -- Lattice parameter Gamma in rad\n"
      << "%alphaDeg% -- Lattice parameter Alpha in degrees\n"
      << "%betaDeg% -- Lattice parameter Beta in degrees\n"
      << "%gammaDeg% -- Lattice parameter Gamma in degrees\n"
      << "%volume% -- Unit cell volume\n"
      << "%gen% -- xtal generation number\n"
      << "%id% -- xtal id number\n";

    return str;
  }

  bool XtalOpt::load(const QString &filename, const bool forceReadOnly)
  {
    if (forceReadOnly) {
      readOnly = true;
    }
    else readOnly = false;

    loaded = true;

    // Attempt to open state file
    QFile file (filename);
    if (!file.open(QIODevice::ReadOnly)) {
      error("XtalOpt::load(): Error opening file "
            +file.fileName() + " for reading...");
      return false;
    }

    SETTINGS(filename);
    int loadedVersion = settings->value("xtalopt/version", 0).toInt();

    // Update config data. Be sure to bump m_schemaVersion in ctor if
    // adding updates.
    switch (loadedVersion) {
    case 0:
    case 1:
    case 2: // Tab edit bumped to V2. No change here.
      break;
    default:
      error("XtalOpt::load(): Settings in file "+file.fileName()+
            " cannot be opened by this version of XtalOpt. Please "
            "visit http://xtalopt.openmolecules.net to obtain a "
            "newer version.");
      return false;
    }

    bool stateFileIsValid = settings->value("xtalopt/saveSuccessful",
                                            false).toBool();
    if (!stateFileIsValid) {
      error("XtalOpt::load(): File " + file.fileName() +
            " is incomplete, corrupt, or invalid. (Try "
            + file.fileName() + ".old if it exists)");
      readOnly = true;
      return false;
    }

    DESTROY_SETTINGS(filename);

    // Get path and other info for later:
    QFileInfo stateInfo (file);
    // path to resume file
    QDir dataDir  = stateInfo.absoluteDir();
    QString dataPath = dataDir.absolutePath() + "/";
    // list of xtal dirs
    QStringList xtalDirs = dataDir.entryList(QStringList(),
                                             QDir::AllDirs, QDir::Size);
    xtalDirs.removeAll(".");
    xtalDirs.removeAll("..");
    for (int i = 0; i < xtalDirs.size(); i++) {
      // old versions of xtalopt used xtal.state, so still check for it.
      if (!QFile::exists(dataPath + "/" + xtalDirs.at(i)
                         + "/structure.state") &&
          !QFile::exists(dataPath + "/" + xtalDirs.at(i)
                         + "/xtal.state") ) {
          xtalDirs.removeAt(i);
          i--;
      }
    }

    // Set filePath:
    QString newFilePath = dataPath;
    QString newFileBase = filename;
    newFileBase.remove(newFilePath);
    newFileBase.remove("xtalopt.state.old");
    newFileBase.remove("xtalopt.state.tmp");
    newFileBase.remove("xtalopt.state");

    // TODO For some reason, the local view of "this" is not changed
    // when the settings are loaded in the following line. The tabs
    // are loading the settings and setting the variables in their
    // scope, but it isn't changing it here. Caching issue maybe?
    m_dialog->readSettings(filename);

#ifdef ENABLE_SSH
    // Create the SSHManager if running remotely
    if (qobject_cast<RemoteQueueInterface*>(m_queueInterface) != 0) {
      if (!this->createSSHConnections()) {
        error(tr("Could not create ssh connections."));
        return false;
      }
    }
#endif // ENABLE_SSH

    debug(tr("Resuming XtalOpt session in '%1' (%2) readOnly = %3")
          .arg(filename)
          .arg((m_optimizer) ? m_optimizer->getIDString()
                             : "No set optimizer")
          .arg( (readOnly) ? "true" : "false"));

    // Xtals
    // Initialize progress bar:
    m_dialog->updateProgressMaximum(xtalDirs.size());
    // If a local queue interface was used, all InProcess structures must be
    // Restarted.
    bool restartInProcessStructures = false;
    bool clearJobIDs = false;
    if (qobject_cast<LocalQueueInterface*>(m_queueInterface)) {
      restartInProcessStructures = true;
      clearJobIDs = true;
    }
    // Load xtals
    Xtal* xtal;
    QList<uint> keys = comp.keys();
    QList<Structure*> loadedStructures;
    QString xtalStateFileName;
    bool errorMsgAlreadyGiven = false;

    for (int i = 0; i < xtalDirs.size(); i++) {
      m_dialog->updateProgressLabel(tr("Loading structures(%1 of %2)...")
                                    .arg(i+1).arg(xtalDirs.size()));
      m_dialog->updateProgressValue(i);

      xtalStateFileName = dataPath + "/" + xtalDirs.at(i) + "/structure.state";
      debug(tr("Loading structure %1").arg(xtalStateFileName));
      // Check if this is an older session that used xtal.state instead.
      if ( !QFile::exists(xtalStateFileName) &&
           QFile::exists(dataPath + "/" + xtalDirs.at(i) + "/xtal.state") ) {
        xtalStateFileName = dataPath + "/" + xtalDirs.at(i) + "/xtal.state";
      }

      xtal = new Xtal();
      QWriteLocker locker (xtal->lock());
      xtal->moveToThread(m_tracker->thread());
      xtal->setupConnections();

      xtal->setFileName(dataPath + "/" + xtalDirs.at(i) + "/");
      // The "true" in the second parameter tells it to read current structure
      // info. This sets current cell info, atom info, enthalpy, energy, & PV
      xtal->readSettings(xtalStateFileName, true);

      // Store current state -- updateXtal will overwrite it.
      Xtal::State state = xtal->getStatus();
      // Set state from InProcess -> Restart if needed
      if (restartInProcessStructures && state == Structure::InProcess) {
        state = Structure::Restart;
      }
      QDateTime endtime = xtal->getOptTimerEnd();

      locker.unlock();

      // If the current settings were saved successfully, then the current
      // enthalpy,energy, atom types, atom positions, and cell info must be
      // set already
      SETTINGS(xtalStateFileName);

      // Store the memory address for a parent structure if one exists. Since
      // this resume loads state files in numerical order, the parent of a
      // given structure SHOULD have already been loaded.
      QString parentStructureString =
        settings->value("structure/parentStructure", "").toString();
      if (!parentStructureString.isEmpty()) {
        for (size_t i = 0; i < loadedStructures.size(); i++) {
          QString compare =
            QString::number(loadedStructures.at(i)->getGeneration()) +
            "x" +
            QString::number(loadedStructures.at(i)->getIDNumber());
          // Increment offspring counter
          // If the xtal skipped optimization, we don't want to count it
          // We also only want to count finished structures...
          if (parentStructureString == compare &&
              !xtal->skippedOptimization() &&
              (xtal->getStatus() == Xtal::Duplicate ||
               xtal->getStatus() == Xtal::Supercell ||
               xtal->getStatus() == Xtal::Optimized)) {
            xtal->setParentStructure(loadedStructures.at(i));
            xtal->incrementParentNumTotOffspring();
            break;
          }
        }
      }

      int version = settings->value("structure/version", -1).toInt();
      bool saveSuccessful = settings->value("structure/saveSuccessful",
                                            false).toBool();

      // The error message is given by a pop-up
      QString errorMsg = tr("Some structures were not loaded successfully. "
                            "These structures will be over-written if the "
                            "search is resumed."
                            "\n\nPlease check the log for details. ");

      // The warning message is given in the log
      QString warningMsg = tr("structure.state file was not saved "
                            "successfully for %1. This structure will be "
                            "excluded.").arg(xtal->fileName());

      // version == -1 implies that the save failed.
      if (version == -1) {
        if (!errorMsgAlreadyGiven) {
          error(errorMsg);
          errorMsgAlreadyGiven = true;
        }
        warning(warningMsg);
        continue;
      }

      if (version >= 3) {
        if (!saveSuccessful) {
          // Check the structure.state.old file if this was not saved
          // successfully.
          DESTROY_SETTINGS(xtalStateFileName);
          SETTINGS(xtalStateFileName + ".old");
          if (!settings->value("structure/saveSuccessful",
                               false).toBool()) {
            if (!errorMsgAlreadyGiven) {
              error(errorMsg);
              errorMsgAlreadyGiven = true;
            }
            warning(warningMsg);
            continue;
          }
          // Otherwise, just continue with the new settings in place
        }

        // Reset state
        locker.relock();
        xtal->setStatus(state);
        xtal->setOptTimerEnd(endtime);
        if (clearJobIDs) {
          xtal->setJobID(0);
        }
        // For some strange reason, setEnergy() does not appear to be
        // working in readSettings() in structure.cpp (even though all the
        // others including setEnthalpy() seem to work fine). So we will set it
        // here.
        double energy = settings->value("structure/current/energy", 0)
                                                                    .toDouble();
        xtal->setEnergy(energy * EV_TO_KJ_PER_MOL);

        DESTROY_SETTINGS(xtalStateFileName);
        locker.unlock();
        updateLowestEnthalpyFUList_(qobject_cast<Structure*>(xtal));
        loadedStructures.append(qobject_cast<Structure*>(xtal));
        continue;
      }
      DESTROY_SETTINGS(xtalStateFileName);
      // If we are loading a previous version,
      // attempt to load the xtal data from the output files
      if (!m_optimizer->load(xtal)) {
        error(tr("Error, no (or not appropriate for %1) xtal data in "
                 "%2.\n\nThis could be a result of resuming a structure "
                 "that has not yet done any local optimizations. If so, "
                 "safely ignore this message.")
              .arg(m_optimizer->getIDString())
              .arg(xtal->fileName()));
        delete xtal;
        continue;
      }

      // Reset state
      locker.relock();
      xtal->setStatus(state);
      xtal->setOptTimerEnd(endtime);
      if (clearJobIDs) {
        xtal->setJobID(0);
      }
      locker.unlock();
      updateLowestEnthalpyFUList_(qobject_cast<Structure*>(xtal));
      loadedStructures.append(qobject_cast<Structure*>(xtal));
      // Update the formula unit list. This is for loading older versions
      // of xtalopt
      if (i == 0) {
        maxFU = xtal->getFormulaUnits();
        minFU = xtal->getFormulaUnits();
        formulaUnitsList.clear();
        formulaUnitsList.append(xtal->getFormulaUnits());
        emit updateFormulaUnitsListUIText();
        emit updateVolumesToBePerFU(xtal->getFormulaUnits());
        error("Warning: an XtalOpt run from an older version is being "
              "loaded.\n\nIf you choose to resume the run, you will not "
              "be able to load it with the older version any longer.\n\n"
              "Upon resuming, the formula units will be set to what they were "
              "in the prior run, and the volumes will be adjusted to be per FU."             );
      }
    }

    m_dialog->updateProgressMinimum(0);
    m_dialog->updateProgressValue(0);
    m_dialog->updateProgressMaximum(loadedStructures.size());
    m_dialog->updateProgressLabel("Sorting and checking structures...");

    // Sort Xtals by index values
    int curpos = 0;
    //dialog->stopProgressUpdate();
    //dialog->startProgressUpdate("Sorting xtals...", 0, loadedStructures.size()-1);
    for (int i = 0; i < loadedStructures.size(); i++) {
      m_dialog->updateProgressValue(i);
      for (int j = 0; j < loadedStructures.size(); j++) {
        //dialog->updateProgressValue(curpos);
        if (loadedStructures.at(j)->getIndex() == i) {
          loadedStructures.swap(j, curpos);
          curpos++;
        }
      }
    }

    // Locate and assign memory addresses for parent structures

    m_dialog->updateProgressMinimum(0);
    m_dialog->updateProgressValue(0);
    m_dialog->updateProgressMaximum(loadedStructures.size());
    m_dialog->updateProgressLabel("Updating structure indices...");

    // Reassign indices (shouldn't always be necessary, but just in case...)
    for (int i = 0; i < loadedStructures.size(); i++) {
      m_dialog->updateProgressValue(i);
      loadedStructures.at(i)->setIndex(i);
    }

    m_dialog->updateProgressMinimum(0);
    m_dialog->updateProgressValue(0);
    m_dialog->updateProgressMaximum(loadedStructures.size());
    m_dialog->updateProgressLabel("Preparing GUI and tracker...");

    // Reset the local file path information in case the files have moved
    filePath = newFilePath;

    Structure *s= 0;
    for (int i = 0; i < loadedStructures.size(); i++) {
      s = loadedStructures.at(i);
      m_dialog->updateProgressValue(i);
      m_tracker->lockForWrite();
      m_tracker->append(s);
      m_tracker->unlock();
      if (s->getStatus() == Structure::WaitingForOptimization)
        m_queue->appendToJobStartTracker(s);
    }

    m_dialog->updateProgressLabel("Done!");

    // If no structures were loaded successfully, enter read-only mode.
    if (loadedStructures.size() == 0) {
      error(tr("Critical error! No structures were loaded successfully."
               "\nEntering read-only mode."));
      readOnly = true;
      return false;
    }

    // Check if user wants to resume the search
    if (!readOnly) {
      bool resume;
      needBoolean(tr("Session '%1' (%2) loaded. Would you like to start "
                     "submitting jobs and resume the search? (Answering "
                     "\"No\" will enter read-only mode.)")
                       .arg(description).arg(filePath),
                       &resume);

      readOnly = !resume;
      qDebug() << "Read only? " << readOnly;
    }

    return true;
  }

  bool XtalOpt::onTheFormulaUnitsList(uint FU) {
     for (int i = 0; i < formulaUnitsList.size(); i++) {
       if (FU == formulaUnitsList.at(i))
         return true;
     }
     return false;
  }

  void XtalOpt::resetSpacegroups() {
    if (isStarting) {
      return;
    }
    QtConcurrent::run(this, &XtalOpt::resetSpacegroups_);
  }

  void XtalOpt::resetSpacegroups_() {
    const QList<Structure*> structures = *(m_tracker->list());
    for (QList<Structure*>::const_iterator it = structures.constBegin(),
         it_end = structures.constEnd(); it != it_end; ++it)
    {
      (*it)->lock()->lockForWrite();
      qobject_cast<Xtal*>(*it)->findSpaceGroup(tol_spg);
      (*it)->lock()->unlock();
    }
  }

  void XtalOpt::resetDuplicates() {
    if (isStarting) {
      return;
    }
    QtConcurrent::run(this, &XtalOpt::resetDuplicates_);
  }

  void XtalOpt::resetDuplicates_() {
    const QList<Structure*> *structures = m_tracker->list();
    Xtal *xtal = 0;
    for (int i = 0; i < structures->size(); i++) {
      xtal = qobject_cast<Xtal*>(structures->at(i));
      xtal->lock()->lockForWrite();
      // Let's reset supercells here too
      if (xtal->getStatus() == Xtal::Duplicate ||
          xtal->getStatus() == Xtal::Supercell) {
        xtal->setStatus(Xtal::Optimized);
        // Reset the duplicate counting for the parents
        // If the xtal does not have a parent, this skip over it automatically
        xtal->decrementParentNumDupOffspring();
      }
      xtal->structureChanged(); // Reset cached comparisons
      xtal->lock()->unlock();
    }
    checkForDuplicates();
  }

  // Helper struct for the map below
  struct dupCheckStruct
  {
    Xtal *i, *j;
    double tol_len, tol_ang;
  };

  // Helper Supercell Check Struct is defined in the header

  void checkIfDups(dupCheckStruct & st)
  {
    Xtal *kickXtal, *keepXtal;
    st.i->lock()->lockForRead();
    st.j->lock()->lockForRead();
    // if they are already both duplicates, just return.
    if (st.i->getStatus() == Xtal::Duplicate &&
        st.j->getStatus() == Xtal::Duplicate) {
      st.i->lock()->unlock();
      st.j->lock()->unlock();
      return;
    }
    if (st.i->compareCoordinates(*st.j, st.tol_len, st.tol_ang)) {
      // Mark the newest xtal as a duplicate of the oldest. This keeps the
      // lowest-energy plot trace accurate.
      // For some reason, primitive structures do not always update their
      // indices immediately, and they remain the default "-1". So, if one
      // of the indices is -1, set that to be the kickXtal
      if (st.i->getIndex() == -1) {
        kickXtal = st.i;
        keepXtal = st.j;
      }
      else if (st.j->getIndex() == -1) {
        kickXtal = st.j;
        keepXtal = st.i;
      }
      else if (st.i->getIndex() > st.j->getIndex()) {
        kickXtal = st.i;
        keepXtal = st.j;
      }
      else {
        kickXtal = st.j;
        keepXtal = st.i;
      }
      // If the kickXtal is already a duplicate, just return
      if (kickXtal->getStatus() == Xtal::Duplicate ||
          kickXtal->getStatus() == Xtal::Supercell) {
        kickXtal->lock()->unlock();
        keepXtal->lock()->unlock();
        return;
      }
      kickXtal->lock()->unlock();
      kickXtal->lock()->lockForWrite();
      kickXtal->setStatus(Xtal::Duplicate);
      kickXtal->setDuplicateString(QString("%1x%2")
                                   .arg(keepXtal->getGeneration())
                                   .arg(keepXtal->getIDNumber()));

      // Increment the offspring duplicate counter for the parent
      kickXtal->incrementParentNumDupOffspring();
      if (kickXtal->hasParentStructure()) {
        Structure* parentXtal = kickXtal->getParentStructure();
        /*qDebug() << "Duplicate: parentXtal->getNumDupOffspring() is now"
                   << parentXtal->getNumDupOffspring()
                   << "\nand parentXtal->getNumTotOffspring() is"
                   << parentXtal->getNumTotOffspring() << "\nfor"
                   << parentXtal->getGeneration() << "x"
                   << parentXtal->getIDNumber();*/
      }
    }
    st.i->lock()->unlock();
    st.j->lock()->unlock();
  }

  void XtalOpt::checkIfSups(supCheckStruct & st)
  {
    Xtal *smallerFormulaUnitXtal, *largerFormulaUnitXtal;
    st.i->lock()->lockForRead();
    st.j->lock()->lockForRead();

    // Determine the larger formula unit structure and the smaller formula unit
    // structure.
    if (st.i->getFormulaUnits() > st.j->getFormulaUnits()) {
      largerFormulaUnitXtal = st.i;
      smallerFormulaUnitXtal = st.j;
    }
    else {
      largerFormulaUnitXtal = st.j;
      smallerFormulaUnitXtal = st.i;
    }

    // if the larger formula unit xtal is already a supercell, skip over it.
    if (largerFormulaUnitXtal->getStatus() == Xtal::Supercell) {
      largerFormulaUnitXtal->lock()->unlock();
      smallerFormulaUnitXtal->lock()->unlock();
      return;
    }

    // We're going to create a temporary xtal that is an expanded version
    // of the smaller formula unit xtal AND has the same formula units of
    // the larger formula unit xtal. They will then be compared with xtalcomp.
    Xtal xtalObject;
    Xtal *tempXtal = &xtalObject;
    tempXtal->setCellInfo(
                        smallerFormulaUnitXtal->OBUnitCell()->GetCellMatrix());

    QList<Atom*> atoms = smallerFormulaUnitXtal->atoms();
    for (size_t i = 0; i < atoms.size(); i++) {
      Atom* atom = tempXtal->addAtom();
      atom->setAtomicNumber(atoms.at(i)->atomicNumber());
      atom->setPos(atoms.at(i)->pos());
    }

    Xtal* tempXtal2 = generateSuperCell(
                                 smallerFormulaUnitXtal->getFormulaUnits(),
                                 largerFormulaUnitXtal->getFormulaUnits(),
                                 tempXtal, false, false);

    if (tempXtal2->compareCoordinates(*largerFormulaUnitXtal, st.tol_len,
                                     st.tol_ang)) {

      // We're going to label the larger formula unit structure a supercell
      // of the smaller. The smaller structure is more fundamental and should
      // remain in the gene pool.
      largerFormulaUnitXtal->lock()->unlock();
      largerFormulaUnitXtal->lock()->lockForWrite();
      largerFormulaUnitXtal->setStatus(Xtal::Supercell);
      // If the smaller formula unit xtal is already a duplicate, make the
      // supercell a supercell the structure that the smaller formula unit
      // duplicate points to.
      if (smallerFormulaUnitXtal->getStatus() == Xtal::Duplicate)
        largerFormulaUnitXtal->setSupercellString(
                    smallerFormulaUnitXtal->getDuplicateString());
      else if (smallerFormulaUnitXtal->getStatus() == Xtal::Supercell)
        largerFormulaUnitXtal->setSupercellString(
                    smallerFormulaUnitXtal->getSupercellString());
      // Otherwise, just make it a supercell of the smaller formula unit xtal
      else largerFormulaUnitXtal->setSupercellString(QString("%1x%2")
                                   .arg(smallerFormulaUnitXtal->getGeneration())
                                   .arg(smallerFormulaUnitXtal->getIDNumber()));

      // Increment the numDupOffspring of the parent xtal if it hasn't been
      // incremented already
      largerFormulaUnitXtal->incrementParentNumDupOffspring();
      if (largerFormulaUnitXtal->hasParentStructure()) {
        Structure* parentXtal = largerFormulaUnitXtal->getParentStructure();
        /*qDebug() << "Supercell: parentXtal->getNumDupOffspring() is now"
                 << parentXtal->getNumDupOffspring()
                 << "\nand parentXtal->getNumTotOffspring() is"
                 << parentXtal->getNumTotOffspring() << "\nfor"
                 << parentXtal->getGeneration() << "x"
                 << parentXtal->getIDNumber();*/
      }
    }
    tempXtal->deleteLater();
    tempXtal = 0;
    st.i->lock()->unlock();
    st.j->lock()->unlock();
  }

  void XtalOpt::checkForDuplicates() {
    if (isStarting) {
      return;
    }
    QtConcurrent::run(this, &XtalOpt::checkForDuplicates_);
  }

  void XtalOpt::checkForDuplicates_() {
    m_tracker->lockForRead();
    const QList<Structure*> *structures = m_tracker->list();
    QList<Xtal*> xtals;
    Xtal *xtal;
    for (int i = 0; i < structures->size(); i++) {
      xtal = qobject_cast<Xtal*>(structures->at(i));
      xtals.append(xtal);
    }
    m_tracker->unlock();

    // Build helper structs
    QList<dupCheckStruct> dupSts;
    dupCheckStruct dupSt;
    QList<supCheckStruct> supSts;
    supCheckStruct supSt;

    for (QList<Xtal*>::iterator xi = xtals.begin();
         xi != xtals.end(); xi++) {
      (*xi)->lock()->lockForRead();
      if ((*xi)->getStatus() != Xtal::Optimized) {
        (*xi)->lock()->unlock();
        continue;
      }

      for (QList<Xtal*>::iterator xj = xi + 1;
           xj != xtals.end(); xj++) {
        (*xj)->lock()->lockForRead();
        if ((*xj)->getStatus() != Xtal::Optimized) {
          (*xj)->lock()->unlock();
          continue;
        }
        if (((*xi)->hasChangedSinceDupChecked() ||
             (*xj)->hasChangedSinceDupChecked()) &&
            // Perform a course enthalpy screening to cut down on number of
            // comparisons
            fabs(((*xi)->getEnthalpy() /
                 static_cast<double>((*xi)->getFormulaUnits())) -
                 ((*xj)->getEnthalpy() /
                 static_cast<double>((*xj)->getFormulaUnits()))) < 1.0 &&
            // Screen out options that CANNOT be supercells

            (((*xi)->getFormulaUnits() % (*xj)->getFormulaUnits() == 0) ||
             ((*xj)->getFormulaUnits() % (*xi)->getFormulaUnits() == 0)))
        {
          // Append the duplicate structs list
          if ((*xi)->getFormulaUnits() == (*xj)->getFormulaUnits()) {
            dupSt.i = (*xi);
            dupSt.j = (*xj);
            dupSt.tol_len = this->tol_xcLength;
            dupSt.tol_ang = this->tol_xcAngle;
            dupSts.append(dupSt);
          }
          // Append the supercell structs list. One has to be a formula unit
          // multiple of the other to be a candidate supercell. In addition,
          // their formula units cannot equal.
          else if ((((*xi)->getFormulaUnits() % (*xj)->getFormulaUnits() == 0)
                     ||
                    ((*xj)->getFormulaUnits() % (*xi)->getFormulaUnits() == 0))
                     &&
                    ((*xj)->getFormulaUnits() != (*xi)->getFormulaUnits())) {

            supSt.i = (*xi);
            supSt.j = (*xj);
            supSt.tol_len = this->tol_xcLength;
            supSt.tol_ang = this->tol_xcAngle;
            supSts.append(supSt);
          }
        }
        (*xj)->lock()->unlock();
      }
      // Nothing else should be setting this, so just update under a
      // read lock
      (*xi)->setChangedSinceDupChecked(false);
      (*xi)->lock()->unlock();
    }

    // If a supercell is matched as a duplicate in the checkIfDups function,
    // it is okay because it will be overwritten with the checkIfSups function
    // following it.
    for (size_t i = 0; i < dupSts.size(); i++) checkIfDups(dupSts[i]);

    // Tried to run this concurrently. Would freeze upon resuming for some
    // reason, though...
    // QtConcurrent::blockingMap(supSts, checkIfSups);
    for (size_t i = 0; i < supSts.size(); i++) checkIfSups(supSts[i]);

    // Label supercells that primitive xtals came from as such
    for (size_t i = 0; i < xtals.size(); i++) {
      xtals.at(i)->lock()->lockForRead();
      if (xtals.at(i)->skippedOptimization()) {
        for (size_t j = 0; j < xtals.size(); j++) {
          if (i == j) continue;
          xtals.at(j)->lock()->lockForRead();
          // If the xtal is optimized, a duplicate, or a supercell, overwrite
          // the previous settings with what it should be for the primitive...
          if ((xtals.at(i)->getStatus() == Xtal::Optimized ||
               xtals.at(i)->getStatus() == Xtal::Duplicate ||
               xtals.at(i)->getStatus() == Xtal::Supercell) &&
              xtals.at(i)->getParents() == tr("Primitive of %1x%2")
                                            .arg((xtals.at(j))->getGeneration())
                                            .arg(xtals.at(j)->getIDNumber())) {
            // Just in case the parent's numDupOffspring was incremented...
            // Will not decrement if it did not increment
            xtals.at(j)->decrementParentNumDupOffspring();
            xtals.at(j)->setStatus(Xtal::Supercell);
            xtals.at(j)->setSupercellString(QString("%1x%2")
                                            .arg(xtals.at(i)->getGeneration())
                                            .arg(xtals.at(i)->getIDNumber()));
          }
          xtals.at(j)->lock()->unlock();
        }
      }
      xtals.at(i)->lock()->unlock();
    }

    emit refreshAllStructureInfo();
  }

  void XtalOpt::updateLowestEnthalpyFUList(GlobalSearch::Structure* s)
  {
    // Perform this in a background thread
    QtConcurrent::run(this, &XtalOpt::updateLowestEnthalpyFUList_, s);
  }

  void XtalOpt::updateLowestEnthalpyFUList_(GlobalSearch::Structure* s)
  {
    // Thankfully, the enthalpy appears to get updated before it reaches this
    // point
    s->lock()->lockForRead();
    // This is to prevent segmentation faults...
    while (lowestEnthalpyFUList.size() - 1 < s->getFormulaUnits()) {
      lowestEnthalpyFUList.append(0);
    }
    if (lowestEnthalpyFUList.at(s->getFormulaUnits()) == 0 ||
        lowestEnthalpyFUList.at(s->getFormulaUnits()) > s->getEnthalpy()) {
      lowestEnthalpyFUList[s->getFormulaUnits()] = s->getEnthalpy();
    }
    s->lock()->unlock();
  }

  // No naming trickery here...
  void XtalOpt::_incrementParentNumTotOffspring(GlobalSearch::Structure* s)
  {
    s->incrementParentNumTotOffspring();
  }

  void XtalOpt::setLatticeMinsAndMaxes(latticeStruct& latticeMins,
                                       latticeStruct& latticeMaxes)
  {
    latticeMins.a = a_min;
    latticeMins.b = b_min;
    latticeMins.c = c_min;
    latticeMins.alpha = alpha_min;
    latticeMins.beta = beta_min;
    latticeMins.gamma = gamma_min;

    latticeMaxes.a = a_max;
    latticeMaxes.b = b_max;
    latticeMaxes.c = c_max;
    latticeMaxes.alpha = alpha_max;
    latticeMaxes.beta = beta_max;
    latticeMaxes.gamma = gamma_max;
  }

  QList<uint> XtalOpt::getListOfAtoms(uint FU)
  {
    // Populate crystal
    QList<uint> atomicNums = comp.keys();
    // Sort atomic number by decreasing minimum radius. Adding the "larger"
    // atoms first encourages a more even (and ordered) distribution
    for (int i = 0; i < atomicNums.size()-1; ++i) {
      for (int j = i + 1; j < atomicNums.size(); ++j) {
        if (this->comp.value(atomicNums[i]).minRadius <
            this->comp.value(atomicNums[j]).minRadius) {
          atomicNums.swap(i,j);
        }
      }
    }

    QList<uint> atoms;
    for (size_t i = 0; i < atomicNums.size(); i++) {
      for (size_t j = 0; j < comp.value(atomicNums[i]).quantity * FU; j++) {
        atoms.push_back(atomicNums[i]);
      }
    }
    return atoms;
  }

  std::vector<uint> XtalOpt::getStdVecOfAtoms(uint FU)
  {
    return getListOfAtoms(FU).toVector().toStdVector();
  }

  // minXtalsOfSpgPerFU should already be set up by now
  uint XtalOpt::pickRandomSpgFromPossibleOnes()
  {
    if (minXtalsOfSpgPerFU.size() == 0) {
      qDebug() << "Error! pickRandomSpgFromPossibleOnes() was called before"
               << "minXtalsOfSpgPerFU was set up!";
      return 1;
    }

    QList<uint> possibleSpgs;
    for (size_t i = 0; i < minXtalsOfSpgPerFU.size(); i++) {
      uint spg = i + 1;
      if (minXtalsOfSpgPerFU.at(i) != -1) possibleSpgs.append(spg);
    }

    // If they are all impossible, print an error and return 1
    if (possibleSpgs.size() == 0) {
      qDebug() << "Error! In pickRandomSpgFromPossibleOnes(), no spacegroups"
               << "were selected to be allowed! We will be generating a"
               << "structure with a spacegroup of 1";
      return 1;
    }

    // Pick a random index from the list
    size_t idx = rand()%int(possibleSpgs.size());

    // Return the spacegroup at this index
    return possibleSpgs.at(idx);
  }

  void XtalOpt::updateProgressBar(size_t goal, size_t attempted,
                                  size_t succeeded)
  {
    // Update progress bar
    m_dialog->updateProgressMaximum(goal);
    m_dialog->updateProgressValue(succeeded);
    m_dialog->updateProgressLabel(
              tr("%1 structures generated (%2 kept, %3 rejected)...")
              .arg(attempted).arg(succeeded).arg(attempted - succeeded));
  }

} // end namespace XtalOpt

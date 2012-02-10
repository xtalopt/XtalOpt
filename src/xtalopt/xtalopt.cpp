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

#include <xtalopt/structures/molecularxtal.h>
#include <xtalopt/structures/submolecule.h>
#include <xtalopt/structures/submoleculesource.h>
#include <xtalopt/structures/xtal.h>
#include <xtalopt/optimizers/vasp.h>
#include <xtalopt/optimizers/gulp.h>
#include <xtalopt/optimizers/pwscf.h>
#include <xtalopt/optimizers/castep.h>
#include <xtalopt/optimizers/openbabeloptimizer.h>
#include <xtalopt/ui/dialog.h>
#include <xtalopt/genetic.h>
#include <xtalopt/molecularxtaloptimizer.h>
#include <xtalopt/queueinterfaces/openbabel.h>

#include <xtalopt/molecularxtalmutator.h>
#include <xtalopt/mxtaloptgenetic.h>

#include <globalsearch/optbase.h>
#include <globalsearch/optimizer.h>
#include <globalsearch/queueinterface.h>
#include <globalsearch/queueinterfaces/local.h>
#include <globalsearch/queuemanager.h>
#include <globalsearch/slottedwaitcondition.h>
#include <globalsearch/macros.h>
#include <globalsearch/bt.h>

#ifdef ENABLE_SSH
#include <globalsearch/sshmanager.h>
#include <globalsearch/queueinterfaces/remote.h>
#endif // ENABLE_SSH

#include <avogadro/bond.h>

#include <QtGui/QApplication>

#include <QtCore/QDir>
#include <QtCore/QList>
#include <QtCore/QFile>
#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtCore/QFileInfo>
#include <QtCore/QReadWriteLock>
#include <QtCore/QtConcurrentRun>
#include <QtCore/QtConcurrentMap>

#define ANGSTROM_TO_BOHR 1.889725989

using namespace GlobalSearch;
using namespace OpenBabel;
using namespace Avogadro;

namespace XtalOpt {

  XtalOpt::XtalOpt(XtalOptDialog *parent) :
    OptBase(parent),
    mga_warnedNoStrainOnFixedCell(false),
    mga_warnedNoVolumeSamplesOnFixedCell(false),
    mga_warnedNoVolumeSamplesOnFixedVolume(false),
    m_initWC(new SlottedWaitCondition (this)),
    m_isMolecular(false),
    m_currentSubMolSourceProgress(-1)
  {
    xtalInitMutex = new QMutex;
    m_idString = "XtalOpt";
    m_schemaVersion = 2;

    // Connections
    connect(m_tracker, SIGNAL(newStructureAdded(GlobalSearch::Structure*)),
            this, SLOT(checkForDuplicates()));
    connect(this, SIGNAL(sessionStarted()),
            this, SLOT(resetDuplicates()));
  }

  XtalOpt::~XtalOpt()
  {
    m_isDestroying = true;

    // Send abort signal to mutators
    m_mutatorsMutex.lock();
    foreach (MolecularXtalMutator *mutator, m_mutators) {
      mutator->abort();
      while (!mutator->waitForFinished(500)) {
        qDebug() << "Waiting for mutator to abort...";
        QApplication::processEvents(QEventLoop::AllEvents, 500);
      }
    }
    m_mutatorsMutex.unlock();

    // Openbabel needs a bit of extra time to clean up its threads.
    OpenBabelQueueInterface *obqi = qobject_cast<OpenBabelQueueInterface*>
        (m_queueInterface);
    if (obqi != NULL) {
      obqi->prepareForDestroy();
    }

    // Wait for save to finish
    unsigned int timeout = 30;
    while (timeout > 0 && savePending) {
      qDebug() << "Spinning on save before destroying XtalOpt ("
               << timeout << "seconds until timeout).";
      timeout--;
      GS_SLEEP(1);
      QApplication::processEvents(QEventLoop::AllEvents, 500);
    };

    qDebug() << "Waiting for main event loop to clear...";
    QApplication::processEvents(QEventLoop::AllEvents);
    qDebug() << "Main event loop cleared.";

    savePending = true;

    // Stop queuemanager thread
    if (m_queueThread->isRunning()) {
      m_queueThread->disconnect();
      m_queueThread->quit();
      m_queueThread->wait();
    }

    // Delete queuemanager
    m_queue->deleteLater();
    m_queue = 0;

#ifdef ENABLE_SSH
    // Stop SSHManager
    m_ssh->deleteLater();
    m_ssh = 0;
#endif // ENABLE_SSH

    // Clean up various members
    m_initWC->deleteLater();
    m_initWC = 0;
  }

  void XtalOpt::startSearch()
  {

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

    // If using the openbabel optimizer/queueinterface, warn the user that
    // these are intended for testing only, and that the results will be
    // neither useful nor reliable
    if (qobject_cast<OpenBabelOptimizer*>(m_optimizer) != NULL ||
        qobject_cast<OpenBabelQueueInterface*>(m_queueInterface) != NULL) {
      error(tr("The OpenBabel queue and OpenBabel optimizer are used for "
               "testing the XtalOpt algorithm only, and any structures "
               "obtained while using them are unlikely to be meaningful.\n\n"
               "Switch to another optimizer and/or queue if you are "
               "interested in finding quality structures.\n\n"
               "You have been warned!"));
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

    // Molecular search prep
    if (this->isMolecularXtalSearch()) {
      // Reassign source ids now that list is final
      for (int i = 0; i < this->mcomp.size(); ++i) {
        mcomp[i].source->setSourceId(i);
      }

      // Generate conformers for submolecules, if needed
      bool notify = m_dialog->startProgressUpdate("Generating conformers...",
                                                  0, 100);
      if (!this->initializeSubMoleculeSources(notify)) {
        error("Error generating submolecule conformers. Aborting.");
        if (notify)
          m_dialog->stopProgressUpdate();
        return;
      }

      if (notify) {
        m_dialog->updateProgressValue(100);
        m_dialog->stopProgressUpdate();
      }
    }

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
    uint progCount = 0;
    QString filename;
    // Use new xtal count in case "addXtal" falls behind so that we
    // don't duplicate structures when switching from seeds -> random.
    uint newXtalCount=0;

    // Load seeds...
    if (!this->isMolecularXtalSearch()) {
      for (int i = 0; i < seedList.size(); i++) {
        filename = seedList.at(i);
        if (this->addSeed(filename)) {
          m_dialog->updateProgressLabel(
                tr("%1 structures generated (%2 kept, %3 rejected)...")
                .arg(i + failed).arg(i).arg(failed));
          newXtalCount++;
        }
      }
    }

    // Generation loop...
    for (uint i = newXtalCount; i < numInitial; i++) {
      // Update progress bar
      m_dialog->updateProgressMaximum(
            (i == 0) ? 0 : int(progCount/static_cast<double>(i))*numInitial);
      m_dialog->updateProgressValue(progCount);
      progCount++;
      m_dialog->updateProgressLabel(
            tr("%1 structures generated (%2 kept, %3 rejected)...")
            .arg(i + failed).arg(i).arg(failed));

      // Generate/Check xtal
      if (this->isMolecularXtalSearch()) {
        MolecularXtal *mxtal = NULL;
        mxtal = generateRandomMXtal(1, i+1);
        if (!checkXtal(mxtal)) {
          delete mxtal;
          i--;
          failed++;
        }
        else {
          mxtal->findSpaceGroup(tol_spg);
          initializeAndAddXtal(mxtal, 1, mxtal->getParents());
          newXtalCount++;
        }
      }
      else {
        Xtal *xtal;
        xtal = generateRandomXtal(1, i+1);
        if (!checkXtal(xtal)) {
          delete xtal;
          i--;
          failed++;
        }
        else {
          xtal->findSpaceGroup(tol_spg);
          initializeAndAddXtal(xtal, 1, xtal->getParents());
          newXtalCount++;
        }
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

  bool XtalOpt::initializeSubMoleculeSources(bool notify)
  {
    m_currentSubMolSourceProgress = 0;
    for (QList<MolecularCompStruct>::const_iterator
         it = this->mcomp.constBegin(), it_end = this->mcomp.constEnd();
         it != it_end; ++it) {
      if (notify)
        this->connect(it->source, SIGNAL(conformerGenerated(int,int)),
                      SLOT(initializeSMSProgressUpdate(int,int)),
                      Qt::BlockingQueuedConnection);
      it->source->setMaxConformers(this->maxConf);
      it->source->findAndSetConformers();
      if (notify)
        disconnect(it->source, SIGNAL(conformerGenerated(int,int)),
                   this, SLOT(initializeSMSProgressUpdate(int,int)));

      ++m_currentSubMolSourceProgress;
    }
    m_currentSubMolSourceProgress = -1;

    return true;
  }

  void XtalOpt::initializeSMSProgressUpdate(int finished, int total)
  {
    // Total number of conformers assuming all sources generate "total"
    int allConfCount = total * this->mcomp.size();
    // Account for all sources already processed
    int alreadyDone = total * (m_currentSubMolSourceProgress);
    // Calculate percent completed
    int percent = 100 * (alreadyDone + finished) / allConfCount;
    // Update progress bar:
    m_dialog->updateProgressValue(percent);
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
    xtal->moveToThread(m_queue->thread());
    if ( !m_optimizer->read(xtal, filename) || !this->checkXtal(xtal, &err)) {
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
    if (this->isMolecularXtalSearch()) {
      MolecularXtal *mxtal =  qobject_cast<MolecularXtal*>(s);
      return static_cast<Structure*>(
            this->replaceWithRandomMXtal(mxtal, reason));
    }
    else {
      Xtal *xtal =  qobject_cast<Xtal*>(s);
      return static_cast<Structure*>(
            this->replaceWithRandomXtal(xtal, reason));
    }
    // Shouldn't happen, but some compilers aren't that bright...
    return NULL;
  }

  Xtal* XtalOpt::replaceWithRandomXtal(Xtal *oldXtal,
                                       const QString & reason)
  {
    QWriteLocker locker1 (oldXtal->lock());

    // Generate/Check new xtal
    Xtal *xtal = 0;
    while (!checkXtal(xtal)) {
      if (xtal) {
        delete xtal;
        xtal = 0;
      }

      xtal = generateRandomXtal(0, 0);
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
    return oldXtal;
  }

  MolecularXtal* XtalOpt::replaceWithRandomMXtal(MolecularXtal *oldMXtal,
                                                 const QString & reason)
  {
    QWriteLocker locker1 (oldMXtal->lock());

    // Generate/Check new mxtal
    MolecularXtal *mxtal = NULL;
    while (!checkXtal(mxtal)) {
      if (mxtal) {
        delete mxtal;
        mxtal = NULL;
      }
      mxtal = generateRandomMXtal(0, 0);
    }

    // Copy info over
    QWriteLocker locker2 (mxtal->lock());
    //! @todo Verify that this assignment doesn't do anything unsual.
    oldMXtal->copyStructure(*mxtal);
    oldMXtal->resetEnergy();
    oldMXtal->resetEnthalpy();
    oldMXtal->setPV(0);
    oldMXtal->setCurrentOptStep(1);
    QString parents = "Randomly generated";
    if (!reason.isEmpty())
      parents += " (" + reason + ")";
    oldMXtal->setParents(parents);
    oldMXtal->findSpaceGroup(tol_spg);
    oldMXtal->resetFailCount();

    // Flag for preoptimization
    if (this->usePreopt) {
      oldMXtal->setNeedsPreoptimization(true);
    }

    // Delete random xtal
    mxtal->deleteLater();
    return oldMXtal;
  }

  Structure* XtalOpt::replaceWithOffspring(Structure *s,
                                           const QString & reason)
  {
    if (this->isMolecularXtalSearch()) {
      MolecularXtal *mxtal =  qobject_cast<MolecularXtal*>(s);
      return static_cast<Structure*>(
            this->replaceWithOffspringMXtal(mxtal, reason));
    }
    else {
      Xtal *xtal =  qobject_cast<Xtal*>(s);
      return static_cast<Structure*>(
            this->replaceWithOffspringXtal(xtal, reason));
    }
    // Shouldn't happen, but some compilers aren't that bright...
    return NULL;
  }

  Xtal* XtalOpt::replaceWithOffspringXtal(Xtal *oldXtal, const QString &reason)
  {
    // Generate/Check new xtal
    Xtal *xtal = 0;
    while (!checkXtal(xtal)) {
      if (xtal) {
        xtal->deleteLater();
        xtal = NULL;
      }
      xtal = generateNewXtal();
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

    Q_ASSERT_X(xtal->numAtoms() == oldXtal->numAtoms(), Q_FUNC_INFO,
               "Number of atoms don't match. Cannot copy.");

    for (uint i = 0; i < xtal->numAtoms(); ++i) {
      (*oldXtal->atom(i)) = (*xtal->atom(i));
    }
    oldXtal->findSpaceGroup(tol_spg);

    // Delete random xtal
    xtal->deleteLater();
    return oldXtal;
  }

  MolecularXtal* XtalOpt::replaceWithOffspringMXtal(
      MolecularXtal *oldMXtal, const QString &reason)
  {
    // Generate/Check new mxtal
    MolecularXtal *mxtal = NULL;
    while (!checkXtal(mxtal)) {
      if (mxtal) {
        mxtal->deleteLater();
        mxtal = NULL;
      }
      mxtal = generateNewMXtal();
    }

    // Copy info over
    QWriteLocker locker (oldMXtal->lock());
    QWriteLocker locker2 (mxtal->lock());
    //! @todo Verify that this assignment doesn't do anything unusual.
    oldMXtal->copyStructure(*mxtal);
    oldMXtal->resetEnergy();
    oldMXtal->resetEnthalpy();
    oldMXtal->setPV(0);
    oldMXtal->setCurrentOptStep(1);
    QString parents = mxtal->getParents();
    if (!reason.isEmpty())
      parents += " (" + reason + ")";
    oldMXtal->setParents(parents);
    oldMXtal->findSpaceGroup(tol_spg);
    oldMXtal->resetFailCount();

    // Flag for preoptimization
    if (this->usePreopt) {
      oldMXtal->setNeedsPreoptimization(true);
    }

    // Delete offspring mxtal
    mxtal->deleteLater();
    return oldMXtal;
  }

  Xtal* XtalOpt::generateRandomXtal(uint generation, uint id)
  {
    INIT_RANDOM_GENERATOR();
    // Set cell parameters
    double a            = RANDDOUBLE() * (a_max-a_min) + a_min;
    double b            = RANDDOUBLE() * (b_max-b_min) + b_min;
    double c            = RANDDOUBLE() * (c_max-c_min) + c_min;
    double alpha        = RANDDOUBLE() * (alpha_max - alpha_min) + alpha_min;
    double beta         = RANDDOUBLE() * (beta_max  - beta_min ) + beta_min;
    double gamma        = RANDDOUBLE() * (gamma_max - gamma_min) + gamma_min;

    // Create crystal
    Xtal *xtal	= new Xtal(a, b, c, alpha, beta, gamma);
    QWriteLocker locker (xtal->lock());

    xtal->setStatus(Xtal::Empty);

    if (using_fixed_volume)
      xtal->setVolume(vol_fixed);

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
    unsigned int q;
    for (int num_idx = 0; num_idx < atomicNums.size(); num_idx++) {
      atomicNum = atomicNums.at(num_idx);
      q = comp.value(atomicNum).quantity;
      for (uint i = 0; i < q; i++) {
        if (!xtal->addAtomRandomly(atomicNum, this->comp)) {
          xtal->deleteLater();
          debug("XtalOpt::generateRandomXtal: Failed to add atoms with "
                "specified interatomic distance.");
          return 0;
        }
      }
    }

    // Set up geneology info
    xtal->setGeneration(generation);
    xtal->setIDNumber(id);
    xtal->setParents("Randomly generated");
    xtal->setStatus(Xtal::WaitingForOptimization);

    // Set up xtal data
    return xtal;
  }

  MolecularXtal* XtalOpt::generateRandomMXtal(uint generation, uint id)
  {
    INIT_RANDOM_GENERATOR();
    // Set cell parameters
    double a      = RANDDOUBLE() * (a_max-a_min) + a_min;
    double b      = RANDDOUBLE() * (b_max-b_min) + b_min;
    double c      = RANDDOUBLE() * (c_max-c_min) + c_min;
    double alpha  = RANDDOUBLE() * (alpha_max - alpha_min) + alpha_min;
    double beta   = RANDDOUBLE() * (beta_max  - beta_min ) + beta_min;
    double gamma  = RANDDOUBLE() * (gamma_max - gamma_min) + gamma_min;

    // Create crystal
    MolecularXtal *mxtal	= new MolecularXtal(a, b, c, alpha, beta, gamma);
    QWriteLocker locker (mxtal->lock());

    mxtal->setStatus(MolecularXtal::Empty);

    if (using_fixed_volume) {
      mxtal->setVolume(vol_fixed);
    }

    // Populate crystal
    //  Calculate maximum translation (cell diagonal length)
    const std::vector<OpenBabel::vector3> obvecs =
        mxtal->OBUnitCell()->GetCellVectors();
    Q_ASSERT(obvecs.size() == 3);
    const OpenBabel::vector3 obcellDiagonal = obvecs[0]+obvecs[1]+obvecs[2];
    const double unitCellDiagonal = obcellDiagonal.length();

    Eigen::Matrix3d rowVectors;
    rowVectors.row(0) = Eigen::Vector3d(obvecs[0].AsArray());
    rowVectors.row(1) = Eigen::Vector3d(obvecs[1].AsArray());
    rowVectors.row(2) = Eigen::Vector3d(obvecs[2].AsArray());

    for (QList<MolecularCompStruct>::const_iterator
         it = mcomp.constBegin(), it_end = mcomp.constEnd();
         it != it_end; ++it) {
      for (int i = 0; i < it->quantity; ++i) {
        SubMolecule * sub = it->source->getRandomSubMolecule();
        QList<Atom*> sAtoms = sub->atoms();
        // Attempt to add submolecule using various translations
        //! @todo There needs to be a limit on the number of iterations here
        while (true) { // Use break to pop out of this loop
          Eigen::Vector3d trans (RANDDOUBLE(), RANDDOUBLE(), RANDDOUBLE());
          trans = mxtal->fracToCart(trans);
          sub->setCenter(trans);

          // Compare the distances between the atoms in sub with the atoms in
          // mxtal. If they meet the minimum radius restrictions, add sub.
          if (this->using_interatomicDistanceLimit) {
            int atom1, atom2;
            double IAD;
            if (mxtal->checkInteratomicDistances(
                  this->comp, sAtoms, &atom1, &atom2, &IAD)) {
              mxtal->addSubMolecule(sub);
              break;
            }
            else /* mxtal->checkInteratomicDistances(...) */ {
              qDebug() << "Cannot add submolecule; bad IAD:" << IAD;
            }
          }
          // If we aren't using interatomic distance limits, just add sub.
          else /* (!this->using_interatomicDistanceLimit) */ {
            mxtal->addSubMolecule(sub);
            break;
          }
        } // end while(true)
      } // end for quantity
    } // end for source

    // Set up geneology info
    mxtal->setGeneration(generation);
    mxtal->setIDNumber(id);
    mxtal->setParents("Randomly generated");
    mxtal->setStatus(MolecularXtal::WaitingForOptimization);

    return mxtal;
  }

  void XtalOpt::initializeAndAddXtal(Xtal *xtal, uint generation,
                                     const QString &parents)
  {
    MolecularXtal *mxtal = qobject_cast<MolecularXtal*>(xtal);

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
    xtal->setIDNumber(id);
    xtal->setGeneration(generation);
    xtal->setParents(parents);
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
    xtal->moveToThread(m_tracker->thread());
    xtal->setupConnections();
    xtal->setFileName(locpath_s);
    xtal->setRempath(rempath_s);
    xtal->setCurrentOptStep(1);
    xtal->resetOptimizerLookupTable();
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

    // If this is a molecular xtal, flag it for preoptimization
    if (mxtal != NULL) {
      if (this->usePreopt) {
        mxtal->setNeedsPreoptimization(true);
      }
    }

    xtalLocker.unlock();
    xtal->update();
    m_queue->unlockForNaming(xtal);
    xtalInitMutex->unlock();

  }

  void XtalOpt::generateNewStructure()
  {
    if (m_isDestroying)
      return;

    // Generate in background thread:
    QtConcurrent::run(this, &XtalOpt::generateNewStructure_);
  }

  void XtalOpt::generateNewStructure_()
  {
    if (this->isMolecularXtalSearch()) {
      MolecularXtal *newMXtal = generateNewMXtal();
      if (newMXtal == NULL || m_isDestroying)
        return;
      initializeAndAddXtal(newMXtal, newMXtal->getGeneration(),
                           newMXtal->getParents());
    }
    else {
      Xtal *newXtal = generateNewXtal();
      initializeAndAddXtal(newXtal, newXtal->getGeneration(),
                           newXtal->getParents());
    }
  }

  void XtalOpt::preoptimizeStructure(Structure *s)
  {
    if (s == NULL) {
      qWarning() << Q_FUNC_INFO << "NULL argument.";
      return;
    }

    MolecularXtal *mxtal = qobject_cast<MolecularXtal*>(s);
    if (mxtal == NULL) {
      qWarning() << "No preoptimization method implemented for"
                 << s->metaObject()->className();
      return;
    }

    QtConcurrent::run(this, &XtalOpt::preoptimizeMXtal, mxtal);
  }

  void XtalOpt::preoptimizeMXtal(MolecularXtal *mxtal)
  {
    QWriteLocker locker (mxtal->lock());

    if (!mxtal->needsPreoptimization())
      return;

    mxtal->emitPreoptimizationStarted();

    mxtal->setNeedsPreoptimization(false);

    mxtal->wrapAtomsToCell();

    // Rough preoptimization
    MolecularXtalOptimizer mxtalOpt (this, sOBMutex);
    mxtalOpt.setDebug(this->mpo_debug);
    mxtalOpt.setMXtal(mxtal);
    mxtalOpt.setEnergyConvergence(this->mpo_econv);
    mxtalOpt.setNumberOfGeometrySteps(this->mpo_maxSteps);
    mxtalOpt.setSuperCellUpdateInterval(this->mpo_sCUpdateInterval);
    mxtalOpt.setVDWCutoff(this->mpo_vdwCut);
    mxtalOpt.setElectrostaticCutoff(this->mpo_eleCut);
    mxtalOpt.setCutoffUpdateInterval(this->mpo_cutoffUpdateInterval);
    mxtalOpt.setup();

    mxtal->startOptTimer();
    locker.unlock();
    mxtalOpt.run();
    locker.relock();
    mxtal->stopOptTimer();

    mxtal->setStatus(MolecularXtal::Updating);

    if (mxtalOpt.isConverged() || mxtalOpt.reachedStepLimit()) {
      mxtalOpt.updateMXtalCoords();
      mxtal->wrapAtomsToCell();
    }
    else {
      qWarning() << "Preoptimization failed for" << mxtal->getIDString()
                 << ". Continuing with full optimization.";
    }

    mxtalOpt.releaseMXtal(); // Under writelock on mxtal->lock(), ok to call

    mxtal->emitPreoptimizationFinished();
  }

  Xtal* XtalOpt::generateNewXtal()
  {
    // Get all optimized structures
    QList<Structure*> structures = m_queue->getAllOptimizedStructures();

    // Check to see if there are enough optimized structure to perform
    // genetic operations
    if (structures.size() < 3) {
      Xtal *xtal = 0;
      while (!checkXtal(xtal)) {
        if (xtal) xtal->deleteLater();
        xtal = generateRandomXtal(1, 0);
      }
      xtal->setParents(xtal->getParents() + " (too few optimized structures "
                       "to generate offspring)");
      return xtal;
    }

    // Sort structure list
    Structure::sortByEnthalpy(&structures);

    // Trim list
    // Remove all but (popSize + 1). The "+ 1" will be removed
    // during probability generation.
    while ( static_cast<uint>(structures.size()) > popSize + 1 ) {
      structures.removeLast();
    }

    // Make list of weighted probabilities based on enthalpy values
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
    unsigned int gen;
    QString parents;
    Xtal *xtal = NULL;

    // Perform operation until xtal is valid:
    while (!checkXtal(xtal)) {
      // First delete any previous failed structure in xtal
      if (xtal) {
        xtal->deleteLater();
        xtal = 0;
      }

      // Decide operator:
      r = RANDDOUBLE();
      Operators op;
      if (r < p_cross/100.0)
        op = OP_Crossover;
      else if (r < (p_cross + p_strip)/100.0)
        op = OP_Stripple;
      else
        op = OP_Permustrain;

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
          int ind1, ind2;
          Xtal *xtal1=0, *xtal2=0;
          // Select structures
          ind1 = ind2 = 0;
          double r1 = RANDDOUBLE();
          double r2 = RANDDOUBLE();
          for (ind1 = 0; ind1 < probs.size(); ind1++)
            if (r1 < probs.at(ind1)) break;
          for (ind2 = 0; ind2 < probs.size(); ind2++)
            if (r2 < probs.at(ind2)) break;

          xtal1 = xtals.at(ind1);
          xtal2 = xtals.at(ind2);

          // Perform operation
          double percent1;
          xtal = XtalOptGenetic::crossover(
                xtal1, xtal2, cross_minimumContribution, percent1);

          // Lock parents and get info from them
          xtal1->lock()->lockForRead();
          xtal2->lock()->lockForRead();
          uint gen1 = xtal1->getGeneration();
          uint gen2 = xtal2->getGeneration();
          uint id1 = xtal1->getIDNumber();
          uint id2 = xtal2->getIDNumber();
          xtal2->lock()->unlock();
          xtal1->lock()->unlock();

          // Determine generation number
          gen = ( gen1 >= gen2 ) ?
            gen1 + 1 :
            gen2 + 1 ;
          parents = tr("Crossover: %1x%2 (%3%) + %4x%5 (%6%)")
            .arg(gen1)
            .arg(id1)
            .arg(percent1, 0, 'f', 0)
            .arg(gen2)
            .arg(id2)
            .arg(100.0 - percent1, 0, 'f', 0);
          continue;
        }
        case OP_Stripple: {
          // Pick a parent
          int ind;
          double r = RANDDOUBLE();
          for (ind = 0; ind < probs.size(); ind++)
            if (r < probs.at(ind)) break;
          Xtal *xtal1 = xtals.at(ind);

          // Perform stripple
          double amplitude=0, stdev=0;
          xtal = XtalOptGenetic::stripple(xtal1,
                                          strip_strainStdev_min,
                                          strip_strainStdev_max,
                                          strip_amp_min,
                                          strip_amp_max,
                                          strip_per1,
                                          strip_per2,
                                          stdev,
                                          amplitude);

          // Lock parent and extract info
          xtal1->lock()->lockForRead();
          uint gen1 = xtal1->getGeneration();
          uint id1 = xtal1->getIDNumber();
          xtal1->lock()->unlock();

          // Determine generation number
          gen = gen1 + 1;
          parents = tr("Stripple: %1x%2 stdev=%3 amp=%4 waves=%5,%6")
            .arg(gen1)
            .arg(id1)
            .arg(stdev, 0, 'f', 5)
            .arg(amplitude, 0, 'f', 5)
            .arg(strip_per1)
            .arg(strip_per2);
          continue;
        }
        case OP_Permustrain: {
          int ind;
          double r = RANDDOUBLE();
          for (ind = 0; ind < probs.size(); ind++)
            if (r < probs.at(ind)) break;

          Xtal *xtal1 = xtals.at(ind);
          double stdev=0;
          xtal = XtalOptGenetic::permustrain(
                xtals.at(ind), perm_strainStdev_max, perm_ex, stdev);

          // Lock parent and extract info
          xtal1->lock()->lockForRead();
          uint gen1 = xtal1->getGeneration();
          uint id1 = xtal1->getIDNumber();
          xtal1->lock()->unlock();

          // Determine generation number
          gen = gen1 + 1;
          parents = tr("Permustrain: %1x%2 stdev=%3 exch=%4")
            .arg(gen1)
            .arg(id1)
            .arg(stdev, 0, 'f', 5)
            .arg(perm_ex);
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
    return xtal;
  }

  MolecularXtal* XtalOpt::generateNewMXtal()
  {
    // Get all optimized structures
    QList<Structure*> structures = m_queue->getAllOptimizedStructures();

    // Check to see if there are enough optimized structure to perform
    // genetic operations
    if (structures.size() < 3) {
      MolecularXtal *mxtal = 0;
      while (!checkXtal(mxtal)) {
        if (mxtal) {
          mxtal->deleteLater();
          mxtal = NULL;
        }
        mxtal = generateRandomMXtal(1, 0);
      }
      mxtal->setParents(mxtal->getParents() + " (too few optimized structures "
                       "to generate offspring)");
      return mxtal;
    }

    // Sort structure list
    Structure::sortByEnthalpy(&structures);

    // Trim list
    // Remove all but (popSize + 1). The "+ 1" will be removed
    // during probability generation.
    while ( static_cast<uint>(structures.size()) > popSize + 1 ) {
      structures.removeLast();
    }

    // Make list of weighted probabilities based on enthalpy values
    QList<double> probs = getProbabilityList(structures);

    // Cast Structures into MXtals
    QList<MolecularXtal*> mxtals;
#if QT_VERSION >= 0x040700
    mxtals.reserve(structures.size());
#endif // QT_VERSION
    for (int i = 0; i < structures.size(); ++i) {
      mxtals.append(qobject_cast<MolecularXtal*>(structures.at(i)));
    }

    // Initialize loop vars
    MolecularXtal *mxtal = NULL;

    // Perform operation until xtal is valid:
    while (!checkXtal(mxtal)) {
      // First delete any previous failed structure in xtal
      if (mxtal) {
        mxtal->deleteLater();
        mxtal = NULL;
      }

      // Try 5 times to get a good structure from the selected
      // operation. If not possible, send a warning to the log and continue
      // trying
      int attemptCount = 0;
      while (attemptCount < 5 && !checkXtal(mxtal)) {
        attemptCount++;
        if (mxtal) {
          delete mxtal;
          mxtal = NULL;
        }

        // Select parent
        int ind;
        MolecularXtal *parentMXtal = NULL;
        // Select structures
        double r = RANDDOUBLE();
        for (ind = 0; ind < probs.size(); ind++)
          if (r < probs.at(ind)) break;

        parentMXtal  = mxtals.at(ind);

        // Determine if the cell is fixed
        bool cellIsFixed = false;
        if (fabs(a_min     - a_max)     < 0.01 ||
            fabs(b_min     - b_max)     < 0.01 ||
            fabs(c_min     - c_max)     < 0.01 ||
            fabs(alpha_min - alpha_max) < 0.01 ||
            fabs(beta_min  - beta_max)  < 0.01 ||
            fabs(gamma_min - gamma_max) < 0.01)
          cellIsFixed = true;

        // Only allow one mutation at a time. The forcefields can consume a
        // *lot* of memory for these large supercells.
        QMutexLocker limitLocker (&m_mxtalMutationLimiter);

        // Bail out early if we're destroying the process
        if (m_isDestroying)
          return NULL;

        MolecularXtalMutator mutator (parentMXtal, this->sOBMutex);
        m_mutatorsMutex.lock();
        m_mutators.append(&mutator);
        m_mutatorsMutex.unlock();

        mutator.setDebug(true);
        if (!cellIsFixed)
          mutator.setNumberOfStrains(this->mga_numLatticeSamples);
        else {
          if (this->mga_numLatticeSamples != 0 ||
              !this->mga_warnedNoStrainOnFixedCell) {
            this->warning("Refusing to mutate lattice, at least one cell "
                          "parameter is fixed.");
            this->mga_warnedNoStrainOnFixedCell = true;
          }
          mutator.setNumberOfStrains(0);
        }
        mutator.setStrainSigmaRange(this->mga_strainMin,
                                    this->mga_strainMax);
        mutator.setNumMovers(this->mga_numMovers);
        mutator.setNumDisplacements(this->mga_numDisplacements);
        mutator.setRotationResolution(this->mga_rotResDeg * DEG_TO_RAD);
        if (cellIsFixed || this->using_fixed_volume) {
          if (this->mga_numVolSamples != 0) {
            if (cellIsFixed && !this->mga_warnedNoVolumeSamplesOnFixedCell) {
              this->warning("Refusing to mutate cell volume, at least one cell "
                            "parameter is fixed.");
              this->mga_warnedNoVolumeSamplesOnFixedCell = true;
            }
            if (this->using_fixed_volume &&
                !this->mga_warnedNoVolumeSamplesOnFixedVolume) {
              this->warning("Refusing to mutate cell volume, volume is fixed.");
              this->mga_warnedNoVolumeSamplesOnFixedVolume = true;
            }
          }
          mutator.setNumberOfVolumeSamples(0);
        }
        else {
          mutator.setNumberOfVolumeSamples(this->mga_numVolSamples);
          mutator.setMinimumVolumeFraction(this->mga_volMinFrac);
          mutator.setMaximumVolumeFraction(this->mga_volMaxFrac);
        }

        // Attempt to make the best mutation possible if it hasn't been done
        // yet
        if (!parentMXtal->hasBestOffspring()) {
          mutator.setCreateBest(true);
          parentMXtal->setHasBestOffspring(true);
        }

        // Build a supercell to begin with or not
        bool startWithSuperCell = (RANDDOUBLE() < 0.5);
        mutator.setStartWithSuperCell(startWithSuperCell);

        // Setup progress bar
        bool notify = m_dialog->startProgressUpdate(
              tr("Mutating structure %1...").arg(parentMXtal->getIDString()),
              0, 100);
        if (notify) {
          connect(&mutator, SIGNAL(progressUpdate(int)),
                  m_dialog, SLOT(updateProgressValue(int)),
                  Qt::DirectConnection); // slot handles threading internally
        }

        mutator.mutate();
        if (notify) {
          mutator.disconnect(m_dialog);
          m_dialog->stopProgressUpdate();
        }

        m_mutatorsMutex.lock();
        m_mutators.removeOne(&mutator);
        m_mutatorsMutex.unlock();

        // We're exiting if the mutator has aborted
        if (mutator.isAborted() || m_isDestroying)
          return NULL;

        mxtal = mutator.getOffspring();

        // If the offspring is identical to the parent, retry:
        if (*mxtal == *parentMXtal) {
          this->warning("Offspring mxtal is identical to parent. Retrying.");
          delete mxtal;
          mxtal = NULL;
          continue;
        }

        // Assign id
        int gen = mxtal->getGeneration();
        int id = 0;
        for (QList<MolecularXtal*>::const_iterator it = mxtals.constBegin(),
             it_end = mxtals.constEnd(); it != it_end; ++it) {
          (*it)->lock()->lockForRead();
          const int curGen = (*it)->getGeneration();
          const int curId = (*it)->getIDNumber();
          (*it)->lock()->unlock();
          if (curGen == gen &&
              curId  >= id) {
            id = curId + 1;
          }
        }
        mxtal->setIDNumber(id);
      }
      if (attemptCount >= 5) {
#if 0 // old operators...
        QString opStr;
        switch (op) {
        case MXOP_Crossover: opStr = "crossover"; break;
        case MXOP_Reconf:    opStr = "reconf"; break;
        case MXOP_Swirl:     opStr = "swirl"; break;
        default:             opStr = "(unknown)"; break;
        }
        warning(tr("Unable to perform operation %1 after 1000 tries. "
                   "Reselecting operator...").arg(opStr));
#endif
        this->warning("Five failed attempts to create offspring logged."
                      " Check/loosen the system constraints.");
        if (this->using_interatomicDistanceLimit && this->usePreopt)
          this->warning("Note: Interatomic distance contraints are usually "
                        "unnecessary when using a preoptimization step.");
      }
    }

    return mxtal;
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
    if (
        ( using_fixed_volume &&
          ( (a_min * b_min * c_min) > vol_fixed ||
            (a_max * b_max * c_max) < vol_fixed )
          ) ||
        ( !using_fixed_volume &&
          ( (a_min * b_min * c_min) > vol_max ||
            (a_max * b_max * c_max) < vol_min ||
            vol_min > vol_max)
          )) {
      warning("XtalOptRand::checkLimits error: Illogical Volume limits. "
              "(Also check min/max volumes based on cell lengths)");
      return false;
    }
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

    // Check composition
    QList<unsigned int> atomTypes = comp.keys();
    QList<unsigned int> atomCounts;
#if QT_VERSION >= 0x040700
    atomCounts.reserve(atomTypes.size());
#endif // QT_VERSION
    for (int i = 0; i < atomTypes.size(); ++i) {
      atomCounts.append(0);
    }
    // Count atoms of each type
    for (int i = 0; i < xtal->numAtoms(); ++i) {
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
    // Check counts
    for (int i = 0; i < atomTypes.size(); ++i) {
      if (atomCounts[i] != comp[atomTypes[i]].quantity) {
        // Incorrect count:
        qDebug() << "XtalOpt::checkXtal: Composition incorrect.";
        if (err != NULL) {
          *err = "Bad composition.";
        }
        return false;
      }
    }

    // Check volume
    if (using_fixed_volume) {
      locker.unlock();
      xtal->setVolume(vol_fixed);
      locker.relock();
    }
    else if ( xtal->getVolume() < vol_min ||
              xtal->getVolume() > vol_max ) {
      // I don't want to initialize a random number generator here, so
      // just use the modulus of the current volume as a random float.
      double newvol = fabs(fmod(xtal->getVolume(), 1)) *
          (vol_max - vol_min) + vol_min;
      // If the user has set vol_min to 0, we can end up with a null
      // volume. Fix this here. This is just to keep things stable
      // numerically during the rescaling -- it's unlikely that other
      // cells with small, nonzero volumes will pass the other checks
      // so long as other limits are reasonable.
      if (fabs(newvol) < 1.0) {
        newvol = (vol_max - vol_min)*0.5 + vol_min;
      }
      qDebug() << "XtalOpt::checkXtal: Rescaling volume from "
               << xtal->getVolume() << " to " << newvol;
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
    // Also check atom positions
    foreach (const Atom *atom, xtal->atoms()) {
      if (GS_IS_NAN_OR_INF(atom->pos()->x()) ||
          GS_IS_NAN_OR_INF(atom->pos()->y()) ||
          GS_IS_NAN_OR_INF(atom->pos()->z()) ) {
        qDebug() << "XtalOpt::checkXtal: A coordinate is either nan or "
                    "inf. Discarding.";
        if (err != NULL) {
          *err = "A coordinate is infinite or not a number.";
        }
        return false;
      }
    }

    // If no cell parameters are fixed, normalize lattice
    if (fabs(a + b + c + alpha + beta + gamma) < 1e-8) {
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
      // If this is a molecularxtal, also check that all atoms are
      // sufficiently far from each bond:
      if (MolecularXtal *mxtal = qobject_cast<MolecularXtal*>(xtal)) {
        if (!mxtal->checkAtomToBondDistances(0.25)) {
          qDebug() << "Discarding structure -- an atom is too close to a bond.";
          if (err != NULL) {
            *err = "A non-bonded atom is too close to a bond.";
          }
          return false;
        }
      }
    }

    // Xtal is OK!
    if (err != NULL) {
      *err = "";
    }
    return true;
  }

  bool XtalOpt::checkStepOptimizedStructure(Structure *s, QString *err)
  {
    if (s == NULL) {
      if (err != NULL) {
        *err = "NULL pointer give for structure.";
      }
      return false;
    }
    // Only currently implemented for molecular xtals:
    MolecularXtal *mxtal = qobject_cast<MolecularXtal*>(s);

    if (mxtal == NULL) {
      return true;
    }

    // Check continuity of submolecular units
    if (!mxtal->verifySubMolecules()) {
      if (err != NULL) {
        *err = "Molecular Xtal exploded (unreasonable bonds post-optimization)";
      }
      return false;
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
      // Try XtalOpt first:
      interpretKeyword(line, structure);
      // If no match, try OptBase:
      if (line == origLine) {
        interpretKeyword_base(line, structure);
      }
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
    MolecularXtal *mxtal = qobject_cast<MolecularXtal*>(xtal);

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
    else if (line == "coordsFrac") {
      QList<Atom*> atoms = structure->atoms();
      int optIndex = -1;
      QHash<int, int> *lut = structure->getOptimizerLookupTable();
      lut->clear();
      // If this is a molecularxtal, use the coherent coordinates of each
      // submolecule
      QVector<Eigen::Vector3d> cohVecs;
      if (mxtal != NULL) {
        atoms.clear();
#if QT_VERSION > 0x040700
        atoms.reserve(mxtal->numAtoms());
#endif
        cohVecs.resize(mxtal->numAtoms());
        QVector<Eigen::Vector3d> subVecs;
        subVecs.reserve(mxtal->numAtoms());
        for (int i = 0; i < mxtal->numSubMolecules(); ++i) {
          SubMolecule *sub = mxtal->subMolecule(i);
          subVecs = sub->getCoherentCoordinates();
          Q_ASSERT_X(subVecs.size() == sub->numAtoms(), Q_FUNC_INFO,
                     "sub->getCoherentCoordinates() did not return the "
                     "correct number of vectors.");
          qCopy(subVecs.constBegin(), subVecs.constEnd(),
                cohVecs.begin() + atoms.size());
          atoms.append(sub->atoms());
        }
      }
      for (int i = 0; i < atoms.size(); ++i) {
        Atom *atom = atoms[i];
        const Eigen::Vector3d coords = (mxtal == NULL)
            ? xtal->cartToFrac(*atom->pos())
            : mxtal->cartToFrac(cohVecs.at(i));
        rep += QString(OpenBabel::etab.GetSymbol(atom->atomicNumber())) + " ";
        rep += QString::number(coords.x()) + " ";
        rep += QString::number(coords.y()) + " ";
        rep += QString::number(coords.z()) + "\n";
        lut->insert(++optIndex, atom->index());
      }
    }
    else if (line == "coordsFracId") {
      QList<Atom*> atoms = structure->atoms();
      int optIndex = -1;
      QHash<int, int> *lut = structure->getOptimizerLookupTable();
      lut->clear();
      // If this is a molecularxtal, use the coherent coordinates of each
      // submolecule
      QVector<Eigen::Vector3d> cohVecs;
      if (mxtal != NULL) {
        atoms.clear();
#if QT_VERSION > 0x040700
        atoms.reserve(mxtal->numAtoms());
#endif
        cohVecs.resize(mxtal->numAtoms());
        QVector<Eigen::Vector3d> subVecs;
        subVecs.reserve(mxtal->numAtoms());
        for (int i = 0; i < mxtal->numSubMolecules(); ++i) {
          SubMolecule *sub = mxtal->subMolecule(i);
          subVecs = sub->getCoherentCoordinates();
          Q_ASSERT_X(subVecs.size() == sub->numAtoms(), Q_FUNC_INFO,
                     "sub->getCoherentCoordinates() did not return the "
                     "correct number of vectors.");
          qCopy(subVecs.constBegin(), subVecs.constEnd(),
                cohVecs.begin() + atoms.size());
          atoms.append(sub->atoms());
        }
      }
      for (int i = 0; i < atoms.size(); ++i) {
        Atom *atom = atoms[i];
        const Eigen::Vector3d coords = (mxtal == NULL)
            ? xtal->cartToFrac(*atom->pos())
            : mxtal->cartToFrac(cohVecs.at(i));
        rep += QString(OpenBabel::etab.GetSymbol(atom->atomicNumber())) +" ";
        rep += QString::number(atom->atomicNumber()) + " ";
        rep += QString::number(coords.x()) + " ";
        rep += QString::number(coords.y()) + " ";
        rep += QString::number(coords.z()) + "\n";
        lut->insert(++optIndex, atom->index());
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
        for (int j = 0; j < 3; j++) {
          rep += QString::number(m.Get(i,j)) + "\t";
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
    else if (line == "gulpConnect") {
      QList<Bond*> bonds = xtal->bonds();
      const char *singleBond = "single";
      const char *doubleBond = "double";
      const char *tripleBond = "triple";
      const char *bondOrder = NULL;
      for (QList<Bond*>::const_iterator it = bonds.constBegin(),
           it_end = bonds.constEnd(); it != it_end; ++it) {
        switch ((*it)->order()) {
        case 1:
          bondOrder = singleBond;
          break;
        case 2:
          bondOrder = doubleBond;
          break;
        case 3:
          bondOrder = tripleBond;
          break;
        default:
          this->warning("Unrecognized bond order (" +
                        QString::number((*it)->order()) + ")");
          bondOrder = NULL;
          break;
        }

        rep += QString("connect %1 %2 %3\n")
            .arg((*it)->beginAtom()->index() + 1)
            .arg((*it)->endAtom()->index() + 1)
            .arg((bondOrder) ? QString(bondOrder) : "unrecognized");
      }
    }
    else if (line == "mopacCoordsAndCell") {
      std::vector<OpenBabel::vector3> obVecs =
          xtal->OBUnitCell()->GetCellVectors();
      const Eigen::Vector3d *firstAtomPos = NULL;
      Eigen::Vector3d v1 (obVecs.at(0).AsArray());
      Eigen::Vector3d v2 (obVecs.at(1).AsArray());
      Eigen::Vector3d v3 (obVecs.at(2).AsArray());
      int optIndex = -1;
      QHash<int, int> *lut = structure->getOptimizerLookupTable();
      lut->clear();
      for (int i = 0; i < xtal->numAtoms(); ++i) {
        Atom *atom = xtal->atom(i);
        lut->insert(++optIndex, atom->index());

        const Eigen::Vector3d *tmpVec = atom->pos();
        if (firstAtomPos == NULL)
          firstAtomPos = tmpVec;
        rep += QString("%1  %2  %3  %4\n").arg(atom->atomicNumber(), 3)
            .arg(tmpVec->x(), 12, 'f', 8)
            .arg(tmpVec->y(), 12, 'f', 8)
            .arg(tmpVec->z(), 12, 'f', 8);
      }
      if (firstAtomPos != NULL) {
        v1 += *firstAtomPos;
        v2 += *firstAtomPos;
        v3 += *firstAtomPos;
        rep += QString("Tv  %1  %2  %3\n"
                       "Tv  %4  %5  %6\n"
                       "Tv  %7  %8  %9\n")
            .arg(v1.x(), 12, 'f', 8)
            .arg(v1.y(), 12, 'f', 8)
            .arg(v1.z(), 12, 'f', 8)
            .arg(v2.x(), 12, 'f', 8)
            .arg(v2.y(), 12, 'f', 8)
            .arg(v2.z(), 12, 'f', 8)
            .arg(v3.x(), 12, 'f', 8)
            .arg(v3.y(), 12, 'f', 8)
            .arg(v3.z(), 12, 'f', 8);
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
      QList<Atom*> atoms = xtal->getAtomsSortedBySymbol();
      int optIndex = -1;
      QHash<int, int> *lut = structure->getOptimizerLookupTable();
      lut->clear();
      Eigen::Vector3d vec;
      for (int i = 0; i < atoms.size(); i++) {
        vec = xtal->cartToFrac(*atoms[i]->pos());
        rep += QString("  %1 %2 %3\n")
          .arg(vec.x(), 12, 'f', 8)
          .arg(vec.y(), 12, 'f', 8)
          .arg(vec.z(), 12, 'f', 8);
        lut->insert(i, atoms[i]->index());
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
      << "%mopacCoordsAndCell% -- MOPAC atom and unit cell specification\n"
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
      << "%gulpConnect% -- bonding information for GULP\n"
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
      return false;
    }

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

    // Is this a molecular search? Check the setting here, see note above --
    // The ivars of this aren't always updated immediately
    bool isMolSearch =
        settings->value("xtalopt/init/isMolecularXtalSearch", false).toBool();

    debug(tr("Resuming %1 XtalOpt session in '%2' (%3) readOnly = %4")
          .arg((isMolSearch) ? "molecular" : "ionic")
          .arg(filename)
          .arg((m_optimizer) ? m_optimizer->getIDString()
                             : "No set optimizer")
          .arg( (readOnly) ? "true" : "false"));

    // Load submoleculesources for molecular searches before reading other
    // settings. Otherwise Bad Things(TM) will happen, like VASP POTCARs
    // getting clear.
    if (isMolSearch) {
      this->readSubMoleculeSources(filename);
    }

    // Read settings
    m_dialog->readSettings(filename);

    QApplication::processEvents(QEventLoop::AllEvents);

#ifdef ENABLE_SSH
    // Create the SSHManager if running remotely
    if (qobject_cast<RemoteQueueInterface*>(m_queueInterface) != 0) {
      if (!this->createSSHConnections()) {
        error(tr("Could not create ssh connections."));
        return false;
      }
    }
#endif // ENABLE_SSH

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
    unsigned int numAtoms = 0;
    if (this->isMolecularXtalSearch()) {
      foreach (const MolecularCompStruct cur, this->mcomp) {
        numAtoms += cur.quantity * cur.source->numAtoms();
      }
    }
    else {
      foreach (int key, this->comp.keys()) {
        numAtoms += comp.value(key).quantity;
      }
    }
    QList<uint> keys = comp.keys();
    QList<Structure*> loadedStructures;
    QString xtalStateFileName;
    for (int i = 0; i < xtalDirs.size(); i++) {
      Xtal *xtal = NULL;
      MolecularXtal *mxtal = NULL;
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

      if (this->isMolecularXtalSearch()) {
        xtal = mxtal = new MolecularXtal();
      }
      else {
        xtal = new Xtal();
      }
      QWriteLocker locker (xtal->lock());
      xtal->moveToThread(m_tracker->thread());
      xtal->setupConnections();
      // Add empty atoms to xtal, updateXtal will populate it
      for (int j = 0; j < numAtoms; j++) {
        xtal->addAtom();
      }
      xtal->setFileName(dataPath + "/" + xtalDirs.at(i) + "/");
      xtal->readSettings(xtalStateFileName);

      // Store current state -- updateXtal will overwrite it.
      Xtal::State state = xtal->getStatus();
      // Set state from InProcess -> Restart if needed
      if (restartInProcessStructures && state == Structure::InProcess) {
        state = Structure::Restart;
      }
      QDateTime endtime = xtal->getOptTimerEnd();

      locker.unlock();

      if (!m_optimizer->load(xtal)) {
        error(tr("Error, no (or not appropriate for %1) xtal data in "
                 "%2.\n\nThis could be a result of resuming a structure "
                 "that has not yet done any local optimizations. If so, "
                 "safely ignore this message.")
              .arg(m_optimizer->getIDString())
              .arg(xtal->fileName()));
        xtal->deleteLater();
        xtal = mxtal = NULL;
        continue;
      }

      // Reset state
      locker.relock();
      xtal->setStatus(state);
      xtal->setOptTimerEnd(endtime);
      if (clearJobIDs) {
        xtal->setJobID(0);
      }
      if (mxtal != NULL) {
        mxtal->readMolecularXtalSettings(xtalStateFileName);
        if (mxtal->numSubMolecules() == 0) {
          error(tr("Error, molecular subunit data not found in "
                   "%1.\n\nThis could be a result of resuming a structure "
                   "that has not yet done any local optimizations. If so, "
                   "safely ignore this message.")
                .arg(filename));
          mxtal->deleteLater();
          xtal = mxtal = NULL;
          continue;
        }
      }
      locker.unlock();
      loadedStructures.append(qobject_cast<Structure*>(xtal));
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

  void XtalOpt::readSubMoleculeSources(const QString &filename)
  {
    if (filename.isEmpty()) {
      return;
    }

    SETTINGS(filename);

    settings->beginGroup("xtalopt");
    this->mcomp.clear();
    int numSubMolSources = settings->beginReadArray("subMoleculeSources");
    for (int i = 0; i < numSubMolSources; ++i) {
      settings->setArrayIndex(i);
      MolecularCompStruct cur;
      cur.source = new SubMoleculeSource ();
      cur.quantity = settings->value("quantity").toUInt();
      settings->beginGroup("source");
      cur.source->readFromSettings(settings);
      settings->endGroup();
      mcomp.append(cur);
    }
    settings->endArray();
    settings->endGroup();
  }

  void XtalOpt::writeSubMoleculeSources(const QString &filename)
  {
    if (filename.isEmpty()) {
      return;
    }

    SETTINGS(filename);

    int numSubMolSources = this->mcomp.size();

    settings->beginGroup("xtalopt");
    settings->beginWriteArray("subMoleculeSources", numSubMolSources);
    for (int i = 0; i < numSubMolSources; ++i) {
      settings->setArrayIndex(i);
      settings->setValue("quantity", mcomp[i].quantity);
      settings->beginGroup("source");
      mcomp[i].source->writeToSettings(settings);
      settings->endGroup();
    }
    settings->endArray();
    settings->endGroup();
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
      if (xtal->getStatus() == Xtal::Duplicate)
        xtal->setStatus(Xtal::Optimized);
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

  void checkIfDupXtals(dupCheckStruct & st)
  {
    Xtal *kickXtal, *keepXtal;
    st.i->lock()->lockForRead();
    st.j->lock()->lockForRead();
    if (st.i->compareCoordinates(*st.j, st.tol_len, st.tol_ang)) {
      // Mark the newest xtal as a duplicate of the oldest. This keeps the
      // lowest-energy plot trace accurate.
      if (st.i->getIndex() > st.j->getIndex()) {
        kickXtal = st.i;
        keepXtal = st.j;
      }
      else {
        kickXtal = st.j;
        keepXtal = st.i;
      }
      kickXtal->lock()->unlock();
      kickXtal->lock()->lockForWrite();
      kickXtal->setStatus(Xtal::Duplicate);
      kickXtal->setDuplicateString(QString("%1x%2")
                                   .arg(keepXtal->getGeneration())
                                   .arg(keepXtal->getIDNumber()));
    }
    st.i->lock()->unlock();
    st.j->lock()->unlock();
  }

  void checkIfDupMXtals(dupCheckStruct & st)
  {
    MolecularXtal *kickXtal;
    MolecularXtal *keepXtal;
    MolecularXtal *mx_i = static_cast<MolecularXtal*>(st.i);
    MolecularXtal *mx_j = static_cast<MolecularXtal*>(st.j);

    mx_i->lock()->lockForRead();
    mx_j->lock()->lockForRead();
    if (mx_i->compareCoordinates(*mx_j, st.tol_len, st.tol_ang)) {
      // Mark the newest mxtal as a duplicate of the oldest. This keeps the
      // lowest-energy plot trace accurate.
      if (mx_i->getIndex() > mx_j->getIndex()) {
        kickXtal = mx_i;
        keepXtal = mx_j;
      }
      else {
        kickXtal = mx_j;
        keepXtal = mx_i;
      }
      kickXtal->lock()->unlock();
      kickXtal->lock()->lockForWrite();
      kickXtal->setStatus(Xtal::Duplicate);
      kickXtal->setDuplicateString(QString("%1x%2")
                                   .arg(keepXtal->getGeneration())
                                   .arg(keepXtal->getIDNumber()));
    }
    mx_i->lock()->unlock();
    mx_j->lock()->unlock();
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
    QList<dupCheckStruct> sts;
    dupCheckStruct st;
    for (QList<Xtal*>::iterator xi = xtals.begin();
         xi != xtals.end(); xi++) {
      (*xi)->lock()->lockForRead();
      if ((*xi)->getStatus() == Xtal::Duplicate) {
        (*xi)->lock()->unlock();
        continue;
      }

      for (QList<Xtal*>::iterator xj = xi + 1;
           xj != xtals.end(); xj++) {
        (*xj)->lock()->lockForRead();
        if ((*xj)->getStatus() == Xtal::Duplicate) {
          (*xj)->lock()->unlock();
          continue;
        }
        if (((*xi)->hasChangedSinceDupChecked() ||
             (*xj)->hasChangedSinceDupChecked()) &&
            // Perform a course enthalpy screening to cut down on number of
            // comparisons
            fabs((*xi)->getEnthalpy() - (*xj)->getEnthalpy()) < 1.0)
        {
          st.i = (*xi);
          st.j = (*xj);
          st.tol_len = this->tol_xcLength;
          st.tol_ang = this->tol_xcAngle;
          sts.append(st);
        }
        (*xj)->lock()->unlock();
      }
      // Nothing else should be setting this, so just update under a
      // read lock
      (*xi)->setChangedSinceDupChecked(false);
      (*xi)->lock()->unlock();
    }

    if (this->isMolecularXtalSearch())
      QtConcurrent::blockingMap(sts, checkIfDupMXtals);
    else
      QtConcurrent::blockingMap(sts, checkIfDupXtals);

    emit refreshAllStructureInfo();
  }

} // end namespace XtalOpt

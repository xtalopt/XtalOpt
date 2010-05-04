/**********************************************************************
  XtalOpt - Holds all data for genetic optimization

  Copyright (C) 2009-2010 by David C. Lonie

  This file is part of the Avogadro molecular editor project.
  For more information, see <http://avogadro.openmolecules.net/>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include "xtalopt.h"

#include "../generic/xtal.h"
#include "../generic/optimizer.h"
#include "optimizers/vasp.h"
#include "optimizers/gulp.h"
#include "optimizers/pwscf.h"
#include "ui/dialog.h"
#include "../generic/queuemanager.h"
#include "../generic/templates.h"
#include "../generic/macros.h"
#include "genetic.h"
#include "../generic/bt.h"

#include <openbabel/rand.h>

#include <QDir>
#include <QList>
#include <QFile>
#include <QDebug>
#include <QTimer>
#include <QFileInfo>
#include <QReadWriteLock>
#include <QMessageBox>
#include <QtConcurrentRun>

using namespace std;
using namespace OpenBabel;
using namespace Eigen;

namespace Avogadro {

  XtalOpt::XtalOpt(XtalOptDialog *parent) :
    QObject(parent)
  {
    m_tracker   = new Tracker (this);
    m_queue     = new QueueManager(this, m_tracker);
    m_optimizer = 0; // This will be set when the GUI is initialized
    sOBMutex = new QMutex;
    stateFileMutex = new QMutex;
    backTraceMutex = new QMutex;
    xtalInitMutex = new QMutex;
    m_dialog = parent;

    savePending = false;

    testingMode = false;
    test_nRunsStart = 1;
    test_nRunsEnd = 100;
    test_nStructs = 600;

    // Connections
    connect(m_tracker, SIGNAL(newStructureAdded(Structure*)),
            this, SLOT(checkForDuplicates()));
    connect(this, SIGNAL(sessionStarted()),
            this, SLOT(resetDuplicates()));
    connect(this, SIGNAL(startingSession()),
            this, SLOT(setIsStartingTrue()));
    connect(this, SIGNAL(sessionStarted()),
            this, SLOT(setIsStartingFalse()));
  }

  XtalOpt::~XtalOpt() {
    // Wait for save to finish
    while (savePending) {};
    savePending = true;
    delete m_queue;
    delete m_tracker;
  }

  void XtalOpt::reset() {
    m_tracker->deleteAllStructures();
    m_tracker->reset();
    m_queue->reset();
  }

  void XtalOpt::startOptimization() {
    debug("Starting optimization.");
    emit startingSession();

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

    // VASP checks:
    if (m_optimizer->getIDString() == "VASP") {
      // Is the POTCAR generated? If not, warn user in log and launch generator.
      // Every POTCAR will be identical in this case!
      QList<uint> oldcomp, atomicNums = comp.keys();
      QList<QVariant> oldcomp_ = m_optimizer->getData("Composition").toList();
      for (int i = 0; i < oldcomp_.size(); i++)
        oldcomp.append(oldcomp_.at(i).toUInt());
      qSort(atomicNums);
      if (m_optimizer->getData("POTCAR info").toList().isEmpty() || // No info at all!
          oldcomp != atomicNums // Composition has changed!
          ) {
        error("Using VASP and POTCAR is empty. Please select the pseudopotentials before continuing.");
        return;
      }

      // Build up the latest and greatest POTCAR compilation
      qobject_cast<VASPOptimizer*>(m_optimizer)->buildPOTCARs();
    }

    // prepare pointers
    m_tracker->deleteAllStructures();

    ///////////////////////////////////////////////
    // Generate random structures and load seeds //
    ///////////////////////////////////////////////

    // Set up progress bar
    m_dialog->startProgressUpdate(tr("Generating structures..."), 0, 0);

    // Initalize loop variables
    int failed = 0;
    uint progCount = 0;
    QString filename;
    Xtal *xtal = 0;
    // Use new xtal count in case "addXtal" falls behind so that we
    // don't duplicate structures when switching from seeds -> random.
    uint newXtalCount=0;

    // Load seeds...
    for (int i = 0; i < seedList.size(); i++) {
      filename = seedList.at(i);
      Xtal *xtal = new Xtal;
      xtal->setFileName(filename);
      if ( !m_optimizer->read(xtal, filename) || (xtal == 0) ) {
        m_tracker->deleteAllStructures();
        error(tr("Error loading seed %1").arg(filename));
        return;
      }
      QString parents =tr("Seeded: %1", "1 is a filename").arg(filename);
      initializeAndAddXtal(xtal, 1, parents);
      debug(tr("XtalOpt::StartOptimization: Loaded seed: %1", "1 is a filename").arg(filename));
      m_dialog->updateProgressLabel(tr("%1 structures generated (%2 kept, %3 rejected)...").arg(i + failed).arg(i).arg(failed));
      newXtalCount++;
    }

    // Generation loop...
    for (uint i = newXtalCount; i < numInitial; i++) {
      // Update progress bar
      m_dialog->updateProgressMaximum( (i == 0)
                                        ? 0
                                        : int(progCount / static_cast<double>(i)) * numInitial );
      m_dialog->updateProgressValue(progCount);
      progCount++;
      m_dialog->updateProgressLabel(tr("%1 structures generated (%2 kept, %3 rejected)...").arg(i + failed).arg(i).arg(failed));

      // Generate/Check xtal
      xtal = generateRandomXtal(1, i+1);
      if (!checkXtal(xtal)) {
        delete xtal;
        i--;
        failed++;
      }
      else {
        xtal->findSpaceGroup();
        initializeAndAddXtal(xtal, 1, xtal->getParents());
        newXtalCount++;
      }
    }

    m_dialog->stopProgressUpdate();

    m_dialog->saveSession();
    emit sessionStarted();
  }

  void XtalOpt::sortByEnthalpy(QList<Xtal*> *xtals) {
    uint numStructs = xtals->size();

    // Simple selection sort
    Xtal *xtal_i=0, *xtal_j=0, *tmp=0;
    for (uint i = 0; i < numStructs-1; i++) {
      xtal_i = xtals->at(i);
      xtal_i->lock()->lockForRead();
      for (uint j = i+1; j < numStructs; j++) {
        xtal_j = xtals->at(j);
        xtal_j->lock()->lockForRead();
        if (xtal_j->getEnthalpy() < xtal_i->getEnthalpy()) {
          xtals->swap(i,j);
          tmp = xtal_i;
          xtal_i = xtal_j;
          xtal_j = tmp;
        }
        xtal_j->lock()->unlock();
      }
      xtal_i->lock()->unlock();
    }
  }

  void XtalOpt::rankEnthalpies(QList<Xtal*> *xtals) {
    uint numStructs = xtals->size();
    QList<Xtal*> rxtals;

    // Copy xtals to a temporary list (don't modify input list!)
    for (uint i = 0; i < numStructs; i++)
      rxtals.append(xtals->at(i));

    // Simple selection sort
    Xtal *xtal_i=0, *xtal_j=0, *tmp=0;
    for (uint i = 0; i < numStructs-1; i++) {
      xtal_i = rxtals.at(i);
      xtal_i->lock()->lockForRead();
      for (uint j = i+1; j < numStructs; j++) {
        xtal_j = rxtals.at(j);
        xtal_j->lock()->lockForRead();
        if (xtal_j->getEnthalpy() < xtal_i->getEnthalpy()) {
          rxtals.swap(i,j);
          tmp = xtal_i;
          xtal_i = xtal_j;
          xtal_j = tmp;
        }
        xtal_j->lock()->unlock();
      }
      xtal_i->lock()->unlock();
    }

    // Set rankings
    for (uint i = 0; i < numStructs; i++) {
      xtal_i = rxtals.at(i);
      xtal_i->lock()->lockForWrite();
      xtal_i->setRank(i+1);
      xtal_i->lock()->unlock();
    }
  }

  Structure* XtalOpt::replaceWithRandom(Structure *s, const QString & reason) {
    Xtal *oldXtal = qobject_cast<Xtal*>(s);
    QWriteLocker locker1 (oldXtal->lock());

    // Generate/Check new xtal
    Xtal *xtal = 0;
    while (!checkXtal(xtal))
      xtal = generateRandomXtal(0, 0);

    // Copy info over
    QWriteLocker locker2 (xtal->lock());
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
      atom1 = oldXtal->atom(i);
      atom2 = xtal->atom(i);
      atom1->setPos(atom2->pos());
      atom1->setAtomicNumber(atom2->atomicNumber());
    }
    oldXtal->findSpaceGroup();
    oldXtal->resetFailCount();

    // Delete random xtal
    xtal->deleteLater();
    return qobject_cast<Structure*>(oldXtal);
  }

  Xtal* XtalOpt::generateRandomXtal(uint generation, uint id) {
    // Random number generator
    OpenBabel::OBRandom rand (true);    // "true" uses system random numbers. OB's version isn't too good...
    rand.TimeSeed();

    // Set cell parameters
    double a            = rand.NextFloat() * (a_max-a_min) + a_min;
    double b            = rand.NextFloat() * (b_max-b_min) + b_min;
    double c            = rand.NextFloat() * (c_max-c_min) + c_min;
    double alpha	= rand.NextFloat() * (alpha_max - alpha_min) + alpha_min;
    double beta         = rand.NextFloat() * (beta_max  - beta_min ) + beta_min;
    double gamma	= rand.NextFloat() * (gamma_max - gamma_min) + gamma_min;

    // Create crystal
    Xtal *xtal	= new Xtal(a, b, c, alpha, beta, gamma);
    QWriteLocker locker (xtal->lock());

    xtal->setStatus(Xtal::Empty);

    if (using_fixed_volume)
      xtal->setVolume(vol_fixed);

    // Populate crystal
    QList<uint> atomicNums = comp.keys();
    uint atomicNum;
    uint q;
    for (int num_idx = 0; num_idx < atomicNums.size(); num_idx++) {
      atomicNum = atomicNums.at(num_idx);
      q = comp.value(atomicNum);
      double IAD = (using_shortestInteratomicDistance)
                ? shortestInteratomicDistance
                : -1.0;
      for (uint i = 0; i < q; i++) {
        if (!xtal->addAtomRandomly(atomicNum, IAD)) {
          xtal->deleteLater();
          debug("XtalOpt::generateRandomXtal: Failed to add atoms with specified interatomic distance.");
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

  QList<double> XtalOpt::getProbabilityList(QList<Xtal*> *xtals) {
    // IMPORTANT: xtals must contain one more xtal than needed -- the last xtal in the
    // list will be removed from the probability list!
    if (xtals->size() <= 1) {
      qDebug() << "XtalOpt::getProbabilityList: Structure list too small -- bailing out.";
      return QList<double>();
    }
    QList<double> probs;
    Xtal *xtal=0, *first=0, *last=0;
    first = xtals->first();
    last = xtals->last();
    first->lock()->lockForRead();
    last->lock()->lockForRead();
    double lowest = first->getEnthalpy();
    double highest = last->getEnthalpy();;
    double spread = highest - lowest;
    last->lock()->unlock();
    first->lock()->unlock();
    // If all structures are at the same enthalpy, lets save some time...
    if (spread <= 1e-5) {
      double v = 1.0/static_cast<double>(xtals->size());
      double p = v;
      for (int i = 0; i < xtals->size(); i++) {
        probs.append(v);
        v += p;
      }
      return probs;
    }
    // Generate a list of floats from 0->1 proportional to the enthalpies;
    // E.g. if enthalpies are:
    // -5   -2   -1   3   5
    // We'll have:
    // 0   0.3  0.4  0.8  1
    for (int i = 0; i < xtals->size(); i++) {
      xtal = xtals->at(i);
      xtal->lock()->lockForRead();
      probs.append( ( xtal->getEnthalpy() - lowest ) / spread);
      xtal->lock()->unlock();
    }
    // Subtract each value from one, and find the sum of the resulting list
    // We'll end up with:
    // 1  0.7  0.6  0.2  0   --   sum = 2.5
    double sum = 0;
    for (int i = 0; i < probs.size(); i++){
      probs[i] = 1.0 - probs.at(i);
      sum += probs.at(i);
    }
    // Normalize with the sum so that the list adds to 1
    // 0.4  0.28  0.24  0.08  0
    for (int i = 0; i < probs.size(); i++){
      probs[i] /= sum;
    }
    // Then replace each entry with a cumulative total:
    // 0.4 0.68 0.92 1 1
    sum = 0;
    for (int i = 0; i < probs.size(); i++){
      sum += probs.at(i);
      probs[i] = sum;
    }
    // Pop off the last entry (remember the n_popSize + 1 earlier?)
    // 0.4 0.68 0.92 1
    probs.removeLast();
    // And we have a enthalpy weighted probability list! To use:
    //
    //   double r = rand.NextFloat();
    //   uint ind;
    //   for (ind = 0; ind < probs.size(); ind++)
    //     if (r < probs.at(ind)) break;
    //
    // ind will hold the chosen index.

    return probs;
  }

  void XtalOpt::initializeAndAddXtal(Xtal *xtal, uint generation, const QString &parents) {
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
        error(tr("XtalOpt::initializeAndAddXtal: Cannot write to path: %1 (path creation failure)",
                 "1 is a file path.")
              .arg(locpath_s));
      }
    }
    xtal->setFileName(locpath_s);
    xtal->setRempath(rempath_s);
    xtal->setCurrentOptStep(1);
    xtal->findSpaceGroup();
    m_queue->unlockForNaming(xtal);
    xtalInitMutex->unlock();
  }

  void XtalOpt::generateNewStructure() {
    // Get all optimized structures
    QList<Structure*> structures = m_queue->getAllOptimizedStructures();

    // Check to see if there are enough optimized structure to perform
    // genetic operations
    if (structures.size() < 3) {
      Xtal *xtal = generateRandomXtal(1, 0);
      initializeAndAddXtal(xtal, 1, xtal->getParents());
      return;
    }

    QList<Xtal*> xtals;
    for (int i = 0; i < structures.size(); i++)
      xtals.append(qobject_cast<Xtal*>(structures.at(i)));


    // return xtal
    Xtal *xtal = 0;

    // temporary use xtal
    Xtal *txtal;

    // Setup random engine
    OpenBabel::OBRandom rand (true);    // "true" uses system random
                                        // numbers. OB's version isn't
                                        // too good...
    rand.TimeSeed();

    // Trim and sort list
    XtalOpt::sortByEnthalpy(&xtals);
    // Remove all but (n_consider + 1). The "+ 1" will be removed
    // during probability generation.
    while ( static_cast<uint>(xtals.size()) > popSize + 1 )
      xtals.removeLast();

    // Make list of weighted probabilities based on enthalpy values
    QList<double> probs = getProbabilityList(&xtals);

    // Initialize loop vars
    double r;
    unsigned int gen;
    QString parents;

    // Perform operation until xtal is valid:
    while (!checkXtal(xtal)) {
      // First delete any previous failed structure in xtal
      if (xtal) {
        delete xtal;
        xtal = 0;
      }

      // Decide operator:
      r = rand.NextFloat();
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
          while (ind1 == ind2) {
            for (ind1 = 0; ind1 < probs.size(); ind1++)
              if (rand.NextFloat() < probs.at(ind1)) break;
            for (ind2 = 0; ind2 < probs.size(); ind2++)
              if (rand.NextFloat() < probs.at(ind2)) break;
          }

          xtal1 = xtals.at(ind1);
          xtal2 = xtals.at(ind2);

          // Perform operation
          double percent1;
          xtal = XtalOptGenetic::crossover(xtal1, xtal2, cross_minimumContribution, percent1);

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
          for (ind = 0; ind < probs.size(); ind++)
            if (rand.NextFloat() < probs.at(ind)) break;
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
          for (ind = 0; ind < probs.size(); ind++)
            if (rand.NextFloat() < probs.at(ind)) break;

          Xtal *xtal1 = xtals.at(ind);
          double stdev=0;
          xtal = XtalOptGenetic::permustrain(xtals.at(ind), perm_strainStdev_max, perm_ex, stdev);

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
          warning("XtalOpt::generateSingleOffspring: Attempt to use an invalid operator.");
        }
      }
      if (attemptCount >= 1000) {
        QString opStr;
        switch (op) {
        case OP_Crossover:                 opStr = "crossover"; break;
        case OP_Stripple:                  opStr = "stripple"; break;
        case OP_Permustrain:               opStr = "permustrain"; break;
        default:                        opStr = "(unknown)"; break;
        }
        warning(tr("Unable to perform operation %1 after 1000 tries. Reselecting operator...").arg(opStr));
      }
    }
    initializeAndAddXtal(xtal, gen, parents);
    return;
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
      warning("XtalOptRand::checkLimits error: Illogical Volume limits. (Also check min/max volumes based on cell lengths)");
      return false;
    }
    return true;
  }

  bool XtalOpt::checkXtal(Xtal *xtal) {
    if (!xtal) {
      return false;
    }

    // Lock xtal
    QWriteLocker locker (xtal->lock());

    if (xtal->getStatus() == Xtal::Empty) return false;

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
      double newvol = abs(fmod(xtal->getVolume(), 1)) * (vol_max - vol_min) + vol_min;
      qDebug() << "XtalOpt::checkXtal: Rescaling volume from " << xtal->getVolume() << " to " << newvol;
      xtal->setVolume(newvol);
    }

    // Scale to any fixed parameters
    double a, b, c, alpha, beta, gamma;
    a = b = c = alpha = beta = gamma = 0;
    if (a_min == a_max) a = a_min;
    if (b_min == b_max) b = b_min;
    if (c_min == c_max) c = c_min;
    if (alpha_min ==	alpha_max)	alpha = alpha_min;
    if (beta_min ==	beta_max)	beta = beta_min;
    if (gamma_min ==	gamma_max)	gamma = gamma_min;
    xtal->rescaleCell(a, b, c, alpha, beta, gamma);

    // Ensure that all angles are between 60 and 120:
    xtal->fixAngles();

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
      return false;
    }

    // Check interatomic distances
    if (using_shortestInteratomicDistance) {
      double distance = 0;
      if (xtal->getShortestInteratomicDistance(distance))
        if (distance < shortestInteratomicDistance) {
          qDebug() << "Discarding structure -- Bad IAD (" << distance << " < " << shortestInteratomicDistance << ")";
          return false;
        }
    }

    // Xtal is OK!
    return true;
  }

  bool XtalOpt::save() {
    //qDebug() << "XtalOpt::save() called";
    if (isStarting) {
      savePending = false;
      return false;
    }
    QReadLocker trackerLocker (m_tracker->rwLock());
    QMutexLocker locker (stateFileMutex);
    QString filename = filePath + "/xtalopt.state";
    QString tmpfilename = filename + ".tmp";
    QString oldfilename = filename + ".old";

    // Save data to tmp
    m_dialog->writeSettings(tmpfilename);
    SETTINGS(tmpfilename);
    settings->sync();

    // Move xtalopt.state -> xtalopt.state.old
    if (QFile::exists(filename) ) {
      if (QFile::exists(oldfilename)) {
        qDebug() << "rm old:    " << QFile::remove(oldfilename);
      }
      qDebug() << "Rename"
               << filename
               << oldfilename
               << QFile::rename(filename, oldfilename);
    }

    // Move xtalopt.state.tmp to xtalopt.state
      qDebug() << "Rename"
               << tmpfilename
               << filename
               << QFile::rename(tmpfilename, filename);

    // Loop over xtals and save them
    QFile xfile;
    QList<Structure*> *structures = m_tracker->list();

    Xtal* xtal;
    QTextStream xout;
    for (int i = 0; i < structures->size(); i++) {
      xtal = qobject_cast<Xtal*>(structures->at(i));
      xtal->lock()->lockForRead();
      // Set index here -- this is the only time these are written, so
      // this is ok under a read lock because of the savePending logic
      xtal->setIndex(i);
      xfile.setFileName(xtal->fileName() + "/xtal.state");
      if (!xfile.open(QIODevice::WriteOnly)) {
        error(tr("XtalOpt::save(): Error opening file %1 for writing (Structure %2)...")
              .arg(xfile.fileName())
              .arg(xtal->getIDString()));
        xtal->lock()->unlock();
        savePending = false;
        return false;
      }
      xout.setDevice(&xfile);
      xtal->save(xout);
      xtal->lock()->unlock();
      xfile.close();
    }

    /////////////////////////
    // Print results files //
    /////////////////////////

    QFile file (filePath + "/results.txt");
    QFile oldfile (filePath + "/results_old.txt");
    if (oldfile.open(QIODevice::ReadOnly))
      oldfile.remove();
    if (file.open(QIODevice::ReadOnly))
      file.copy(oldfile.fileName());
    file.close();
    if (!file.open(QIODevice::WriteOnly)) {
      error("XtalOpt::save(): Error opening file "+file.fileName()+" for writing...");
      savePending = false;
      return false;
    }
    QTextStream out (&file);

    QList<Xtal*> sortedXtals;

    for (int i = 0; i < structures->size(); i++)
      sortedXtals.append(qobject_cast<Xtal*>(structures->at(i)));
    if (sortedXtals.size() != 0) sortByEnthalpy(&sortedXtals);

    // Print the data to the file:
    out << "Rank\tGen\tID\tEnthalpy\tSpaceGroup\tStatus\n";
    for (int i = 0; i < sortedXtals.size(); i++) {
      xtal = sortedXtals.at(i);
      if (!xtal) continue; // In case there was a problem copying.
      xtal->lock()->lockForRead();
      out << i << "\t"
          << xtal->getGeneration() << "\t"
          << xtal->getIDNumber() << "\t"
          << xtal->getEnthalpy() << "\t\t"
          << xtal->getSpaceGroupNumber() << ": " << xtal->getSpaceGroupSymbol() << "\t\t";
      // Status:
      switch (xtal->getStatus()) {
      case Xtal::Optimized:
        out << "Optimized";
        break;
      case Xtal::Killed:
      case Xtal::Removed:
        out << "Killed";
        break;
      case Xtal::Duplicate:
        out << "Duplicate";
        break;
      case Xtal::Error:
        out << "Error";
        break;
      case Xtal::StepOptimized:
      case Xtal::WaitingForOptimization:
      case Xtal::InProcess:
      case Xtal::Empty:
      case Xtal::Updating:
      case Xtal::Submitted:
      default:
        out << "In progress";
        break;
      }
      xtal->lock()->unlock();
      out << endl;
    }
    savePending = false;
    return true;
  }

  bool XtalOpt::load(const QString &filename) {
    // Attempt to open state file
    QFile file (filename);
    if (!file.open(QIODevice::ReadOnly)) {
      error("XtalOpt::load(): Error opening file "+file.fileName()+" for reading...");
      return false;
    }

    // Get path and other info for later:
    QFileInfo stateInfo (file);
    // path to resume file
    QDir dataDir  = stateInfo.absoluteDir();
    QString dataPath = dataDir.absolutePath() + "/";
    // list of xtal dirs
    QStringList xtalDirs = dataDir.entryList(QStringList(), QDir::AllDirs, QDir::Size);
    xtalDirs.removeAll(".");
    xtalDirs.removeAll("..");
    for (int i = 0; i < xtalDirs.size(); i++) {
      if (!QFile::exists(dataPath + "/" + xtalDirs.at(i) + "/xtal.state")) {
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

    m_dialog->readSettings(filename);

    SETTINGS(filename);

    // Set optimizer
    setOptimizer(OptTypes(settings->value("xtalopt/edit/optType").toInt()));
    qDebug() << "Resuming XtalOpt session in '" << filename 
             << "' (" << m_optimizer->getIDString() << ")";

    // Xtals
    // Initialize progress bar:
    m_dialog->updateProgressMaximum(xtalDirs.size());
    Xtal* xtal;
    QList<uint> keys = comp.keys();
    QList<Structure*> loadedStructures;
    QFile xfile;
    QTextStream xin;
    uint count = 0;
    int numDirs = xtalDirs.size();
    for (int i = 0; i < numDirs; i++) {
      count++;
      m_dialog->updateProgressLabel(tr("Loading structures(%1 of %2)...").arg(count).arg(numDirs));
      m_dialog->updateProgressValue(count-1);

      xfile.setFileName(dataPath + "/" + xtalDirs.at(i) + "/xtal.state");
      if (!xfile.open(QIODevice::ReadOnly)) {
        error("Error, cannot open file for reading: "+xfile.fileName());
        return false;
      }
      xin.setDevice(&xfile);

      xtal = new Xtal();
      QWriteLocker locker (xtal->lock());
      // Add empty atoms to xtal, updateXtal will populate it
      for (int j = 0; j < keys.size(); j++) {
        for (uint k = 0; k < comp.value(keys.at(j)); k++)
          xtal->addAtom();
      }
      xtal->setFileName(dataPath + "/" + xtalDirs.at(i) + "/");
      xtal->load(xin);

      // Store current state -- updateXtal will overwrite it.
      Xtal::State state = xtal->getStatus();
      QDateTime endtime = xtal->getOptTimerEnd();

      locker.unlock();

      if (!m_optimizer->load(xtal)) {
        error(tr("Error, no (or not appropriate for %1) xtal data in %2.\n\nThis could be a result of resuming a structure that has not yet done any local optimizations. If so, safely ignore this message.")
              .arg(m_optimizer->getIDString())
              .arg(xtal->fileName()));
        continue;
      }

      // Reset state
      locker.relock();
      xtal->setStatus(state);
      xtal->setOptTimerEnd(endtime);
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
    m_dialog->updateProgressLabel("Updating  structure indices...");

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
      m_tracker->append(s);
      if (s->getStatus() == Structure::WaitingForOptimization)
        m_queue->appendToJobStartTracker(s);
    }

    m_dialog->updateProgressLabel("Done!");

    return true;
  }

  void XtalOpt::resetDuplicates() {
    if (isStarting) {
      return;
    }
    QtConcurrent::run(this, &XtalOpt::resetDuplicates_);
  }

  void XtalOpt::resetDuplicates_() {
    QList<Structure*> *structures = m_tracker->list();
    Xtal *xtal = 0;
    for (int i = 0; i < structures->size(); i++) {
      xtal = qobject_cast<Xtal*>(structures->at(i));
      xtal->lock()->lockForWrite();
      if (xtal->getStatus() == Xtal::Duplicate)
        xtal->setStatus(Xtal::Optimized);
      xtal->lock()->unlock();
    }
    checkForDuplicates();
  }

  void XtalOpt::checkForDuplicates() {
    if (isStarting) {
      return;
    }
    QtConcurrent::run(this, &XtalOpt::checkForDuplicates_);
  }

  void XtalOpt::checkForDuplicates_() {
    QHash<QString, double> limits;
    limits.insert("enthalpy", tol_enthalpy);
    limits.insert("volume", tol_volume);

    QList<QString> keys = limits.keys();
    QList<QHash<QString, double> > fps;
    QList<Xtal::State> states;

    m_tracker->lockForRead();
    QList<Structure*> *structures = m_tracker->list();

    Xtal *xtal=0, *xtal_i=0, *xtal_j=0;
    for (int i = 0; i < structures->size(); i++) {
      xtal = qobject_cast<Xtal*>(structures->at(i));
      xtal->lock()->lockForRead();
      fps.append(xtal->getFingerprint());
      states.append(xtal->getStatus());
      xtal->lock()->unlock();
    }

    // Iterate over all xtals
    QHash<QString, double> fp_i, fp_j;
    QString key;
    for (int i = 0; i < fps.size(); i++) {
      if ( states.at(i) != Xtal::Optimized ) continue;
      fp_i = fps.at(i);
      for (int j = i+1; j < fps.size(); j++) {
        if (states.at(j) != Xtal::Optimized ) continue;
        fp_j = fps.at(j);
        // If xtals do not have the same spacegroup number, break
        if (fp_i.value("spacegroup") != fp_j.value("spacegroup")) {
          continue;
        }
        // Check limits
        bool match = true;
        for (int k = 0; k < keys.size(); k++) {
          key = keys.at(k);
          // If values do not match, skip to next pair of xtals.
          if (fabs(fp_i.value(key) - fp_j.value(key) ) > limits.value(key)) {
            match = false;
            break;
          }
        }
        if (!match) continue;
        // If we get here, all the fingerprint values match,
        // and we have a duplicate. Mark the xtal with the
        // highest enthalpy as a duplicate of the other.
        xtal_i = qobject_cast<Xtal*>(structures->at(i));
        xtal_j = qobject_cast<Xtal*>(structures->at(j));
        if (fp_i["enthalpy"] > fp_j["enthalpy"]) {
          xtal_i->lock()->lockForWrite();
          xtal_j->lock()->lockForRead();
          xtal_i->setStatus(Xtal::Duplicate);
          xtal_i->setDuplicateString(QString("%1x%2")
                                     .arg(xtal_j->getGeneration())
                                     .arg(xtal_j->getIDNumber()));
          xtal_i->lock()->unlock();
          xtal_j->lock()->unlock();
          break; // If xtals->at(i) is now a duplicate, don't bother comparing it anymore
        }
        else {
          xtal_j->lock()->lockForWrite();
          xtal_i->lock()->lockForRead();
          xtal_j->setStatus(Xtal::Duplicate);
          xtal_j->setDuplicateString(QString("%1x%2")
                                     .arg(xtal_i->getGeneration())
                                     .arg(xtal_i->getIDNumber()));
          xtal_j->lock()->unlock();
          xtal_i->lock()->unlock();
        }
      }
    }
    m_tracker->unlock();
    emit updateAllInfo();
  }

  void XtalOpt::warning(const QString & s) {
    qWarning() << "Warning: " << s;
    m_dialog->log("Warning: " + s);
  }

  void XtalOpt::debug(const QString & s) {
    qDebug() << "Debug: " << s;
    m_dialog->log("Debug: " + s);
  }

  void XtalOpt::error(const QString & s) {
    qWarning() << "Error: " << s;
    m_dialog->log("Error: " + s);
    m_dialog->errorBox(s);
  }

  void XtalOpt::printBackTrace
() {
    backTraceMutex->lock();
    QStringList l = getBackTrace();
    backTraceMutex->unlock();
    for (int i = 0; i < l.size();i++)
      qDebug() << l.at(i);
  }

  void XtalOpt::setOptimizer(Optimizer *o) {
    Optimizer *old = m_optimizer;
    if (m_optimizer) {
      // Save settings explicitly. This is called in the destructer, but
      // we may need some settings in the new optimizer.
      old->writeSettings();
      old->deleteLater();
    }
    m_optimizer = o;
    emit optimizerChanged(o);
  }

  void XtalOpt::setOptimizer(const QString &IDString)
  {
    if (IDString.toLower() == "vasp")
      setOptimizer(new VASPOptimizer (this));
    else if (IDString.toLower() == "gulp")
      setOptimizer(new GULPOptimizer (this));
    else if (IDString.toLower() == "pwscf")
      setOptimizer(new PWscfOptimizer (this));
    else
      error(tr("XtalOpt::setOptimizer: unable to determine optimizer from '%1'")
            .arg(IDString));
  }
        

  void XtalOpt::setOptimizer(OptTypes opttype)
  {
    switch (opttype) {
    case OT_VASP:
      setOptimizer(new VASPOptimizer (this));
      break;
    case OT_GULP:
      setOptimizer(new GULPOptimizer (this));
      break;
    case OT_PWscf:
      setOptimizer(new PWscfOptimizer (this));
      break;
    default:
      error(tr("XtalOpt::setOptimizer: unable to determine optimizer from '%1'")
            .arg(QString::number((int)opttype)));
      break;
    }
  }
  

} // end namespace Avogadro

#include "xtalopt.moc"

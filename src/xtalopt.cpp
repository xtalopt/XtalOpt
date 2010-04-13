/**********************************************************************
  XtalOpt - Holds all data for genetic optimization

  Copyright (C) 2009 by David C. Lonie

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

#include "xtal.h"
#include "optimizers.h"
#include "ui/dialog.h"
#include "templates.h"
#include "genetic.h"
#include "bt.h"

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
    m_xtals     = new QList<Xtal*>;
    m_rwLock	= new QReadWriteLock;
    comp = new QHash<uint, uint>; // <atomic #, quantity>
    stupidOpenBabelMutex = new QMutex;
    stateFileMutex = new QMutex;
    backTraceMutex = new QMutex;
    dialog = parent;

    checkPopulationPending = false;
    savePending = false;
    runCheckPending = false;

    testingMode = false;
    test_nRunsStart = 1;
    test_nRunsEnd = 100;
    test_nStructs = 600;

    // Initialize tracker objects
    errorPendingTracker = new XtalPendingTracker;
    restartPendingTracker = new XtalPendingTracker;
    startPendingTracker  = new XtalPendingTracker;
    submissionPendingTracker  = new XtalPendingTracker;
    updatePendingTracker = new XtalPendingTracker;
    nextOptStepPendingTracker = new XtalPendingTracker;
    progressInfoUpdateTracker = new XtalPendingTracker;
    runningTracker = new XtalPendingTracker;

    // Connections
    connect(this, SIGNAL(newStructureAdded()),
            this, SLOT(checkForDuplicates()));
    connect(this, SIGNAL(sessionStarted()),
            this, SLOT(resetDuplicates()));
    connect(this, SIGNAL(xtalReadyToStart()),
            this, SLOT(startXtal()));
    connect(this, SIGNAL(startingSession()),
            this, SLOT(setIsStartingTrue()));
    connect(this, SIGNAL(sessionStarted()),
            this, SLOT(setIsStartingFalse()));
  }

  XtalOpt::~XtalOpt() {
    // will NOT delete structures by default!
    m_xtals->clear();
    delete m_xtals;
    delete m_rwLock;
  }

  void XtalOpt::deleteAllStructures() {
    m_rwLock->lockForWrite();
    Xtal *xtal = 0;
    for (int i = 0; i < m_xtals->size(); i++) {
      xtal = m_xtals->at(i);
      xtal->lock()->lockForWrite();
      xtal->deleteLater();
      xtal->lock()->unlock();
    }
    m_xtals->clear();
    emit structuresCleared();
    m_rwLock->unlock();
  }

  void XtalOpt::clearStructures() {
    m_rwLock->lockForWrite();
    m_xtals->clear();
    m_rwLock->unlock();
    emit structuresCleared();
  }

  void XtalOpt::reset() {
    deleteAllStructures();

    // Reset tracker objects
    m_rwLock->lockForWrite();
    errorPendingTracker->reset();
    restartPendingTracker->reset();
    startPendingTracker->reset();
    submissionPendingTracker->reset();
    updatePendingTracker->reset();
    nextOptStepPendingTracker->reset();
    progressInfoUpdateTracker->reset();
    runningTracker->reset();
    m_rwLock->unlock();
  }

  QReadWriteLock* XtalOpt::rwLock() {
    return m_rwLock;
  }

  void XtalOpt::startOptimization() {
    //qDebug() << "XtalOpt::startOptimization() called";
    debug("Starting optimization.");
    emit startingSession();

    // Settings checks
    // Check lattice parameters, volume, etc
    if (!XtalOpt::checkLimits()) {
      error("Cannot create structures. Check log for details.");
      return;
    }

    // Do we have a composition?
    if (comp->isEmpty()) {
      error("Cannot create structures. Composition is not set.");
      return;
    }

    // VASP checks:
    if (optType == XtalOpt::OptType_VASP) {
      // Is the POTCAR generated? If not, warn user in log and launch generator.
      // Every POTCAR will be identical in this case!
      QList<uint> atomicNums = comp->keys();
      qSort(atomicNums);
      if (VASP_POTCAR_info.isEmpty() || // No info at all!
          VASP_POTCAR_comp != atomicNums // Composition has changed!
          ) {
        error("Using VASP and POTCAR is empty. Please select the pseudopotentials before continuing.");
        return;
      }

      // Build up the latest and greatest POTCAR compilation
      XtalOptTemplate::buildVASP_POTCAR(this);
    }

    // prepare pointers
    deleteAllStructures();

    ///////////////////////////////////////////////
    // Generate random structures and load seeds //
    ///////////////////////////////////////////////

    // Set up progress bar
    dialog->startProgressUpdate(tr("Generating structures..."), 0, 0);

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
      if ( !Optimizer::readXtal(xtal, this, filename) || (xtal == 0) ) {
        deleteAllStructures();
        error(tr("Error loading seed %1").arg(filename));
        return;
      }
      xtal->setGeneration(1);
      xtal->setXtalNumber(i+1);
      xtal->setParents(tr("Seeded: %1", "1 is a filename").arg(filename));
      dialog->updateProgressLabel(tr("%1 structures generated (%2 kept, %3 rejected)...").arg(i + failed).arg(i).arg(failed));
      debug(tr("Loaded seed: %1", "1 is a filename").arg(filename));
      addXtal(xtal);
      newXtalCount++;
    }

    // Generation loop...
    for (uint i = newXtalCount; i < numInitial; i++) {
      // Update progress bar
      dialog->updateProgressMaximum( (i == 0)
                                        ? 0
                                        : int(progCount / static_cast<double>(i)) * numInitial );
      dialog->updateProgressValue(progCount);
      progCount++;
      dialog->updateProgressLabel(tr("%1 structures generated (%2 kept, %3 rejected)...").arg(i + failed).arg(i).arg(failed));

      // Generate/Check xtal
      xtal = generateRandomXtal(1, i+1);
      if (!checkXtal(xtal)) {
        delete xtal;
        i--;
        failed++;
      }
      else {
        xtal->findSpaceGroup();
        addXtal(xtal);
        newXtalCount++;
      }
    }

    dialog->stopProgressUpdate();

    dialog->saveSession();
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

  Xtal* XtalOpt::generateRandomXtal(uint generation, uint id) {
    //qDebug() << "XtalOpt::generateRandomXtal( " << ", " << generation << ", " << id << " ) called";

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
    QList<uint> atomicNums = comp->keys();
    uint atomicNum;
    uint q;
    for (int num_idx = 0; num_idx < atomicNums.size(); num_idx++) {
      atomicNum = atomicNums.at(num_idx);
      q = comp->value(atomicNum);
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
    xtal->setXtalNumber(id);
    xtal->setParents("Randomly generated");
    xtal->setStatus(Xtal::WaitingForOptimization);

    // Set up xtal data
    return xtal;
  }

  QList<double> XtalOpt::getProbabilityList(QList<Xtal*> *xtals) {
    //qDebug() << "XtalOpt::getProbabilityList( " << xtals << " called";
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
    first->lock()->unlock();
    last->lock()->unlock();
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

  Xtal* XtalOpt::generateSingleOffspring(QList<Xtal*> *inxtals) {
    // return xtal
    Xtal *xtal = 0;
    // temporary use xtal
    Xtal *txtal;

    // Setup random engine
    OpenBabel::OBRandom rand (true);    // "true" uses system random numbers. OB's version isn't too good...
    rand.TimeSeed();

    // Trim and sort list
    QList<Xtal*> *xtals = new QList<Xtal*>;
    for (int i = 0; ( i < inxtals->size()); i++) {
      txtal = inxtals->at(i);
      txtal->lock()->lockForRead();
      if (inxtals->at(i)->getStatus() == Xtal::Optimized)
        xtals->append(inxtals->at(i));
      txtal->lock()->unlock();
    }
    XtalOpt::sortByEnthalpy(xtals);
    // Remove all but (n_consider + 1). The "+ 1" will be removed during probability generation.
    while ( static_cast<uint>(xtals->size()) > popSize + 1 ) xtals->removeLast();

    // Make list of weighted probabilities based on enthalpy values
    QList<double> probs = getProbabilityList(xtals);

    // Initialize loop vars
    double r;

    // Perform operation until xtal is valid:
    while (!checkXtal(xtal)) {
      // First delete any previous failed structure in xtal
      if (xtal) {
        delete xtal;
        xtal = 0;
      }

      // Decide mutation type:
      r = rand.NextFloat();
      Operators op;
      if (r < p_her/100.0)
        op = Crossover;
      else if (r < (p_her + p_mut)/100.0)
        op = Stripple;
      else
        op = Permustrain;

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
        case Crossover: {
          //qDebug() << "Performing Crossover...";
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

          xtal1 = xtals->at(ind1);
          xtal2 = xtals->at(ind2);

          // Perform operation
          double percent1;
          xtal = XtalOptGenetic::crossover(xtal1, xtal2, her_minimumContribution, percent1);

          // Lock parents and get info from them
          xtal1->lock()->lockForRead();
          xtal2->lock()->lockForRead();
          uint gen1 = xtal1->getGeneration();
          uint gen2 = xtal2->getGeneration();
          uint id1 = xtal1->getXtalNumber();
          uint id2 = xtal2->getXtalNumber();
          xtal1->lock()->unlock();
          xtal2->lock()->unlock();

          // Determine generation number
          uint gen = ( gen1 >= gen2 ) ?
            gen1 + 1 :
            gen2 + 1 ;
          xtal->setGeneration(gen);
          // Determine xtal number
          uint id = 1;
          // Write lock here so that numbers aren't assigned twice
          m_rwLock->lockForWrite();
          startPendingTracker->lock();
          for (int j = 0; j < inxtals->size(); j++) {
            txtal = inxtals->at(j);
            txtal->lock()->lockForRead();
            if (txtal->getGeneration() == gen &&
                txtal->getXtalNumber() >= id) {
              id = txtal->getXtalNumber() + 1;
            }
            txtal->lock()->unlock();
          }
          // Check pending xtal list, too!
          QList<Xtal*> pendList = startPendingTracker->list();
          for (int j = 0; j < pendList.size(); j++) {
            txtal = pendList.at(j);
            txtal->lock()->lockForRead();
            if (txtal->getGeneration() == gen &&
                txtal->getXtalNumber() >= id) {
              id = txtal->getXtalNumber() + 1;
            }
            txtal->lock()->unlock();
          }
          startPendingTracker->unlock();
          m_rwLock->unlock();
          xtal->setXtalNumber(id);
          xtal->setParents(QString("Crossover: %1x%2 (%3%) + %4x%5 (%6%)")
                           .arg(gen1)
                           .arg(id1)
                           .arg(percent1, 0, 'f', 0)
                           .arg(gen2)
                           .arg(id2)
                           .arg(100.0 - percent1, 0, 'f', 0)
                           );
          continue;
        }
        case Stripple: {
          //qDebug() << "Stripplating";
          // Pick a parent
          int ind;
          for (ind = 0; ind < probs.size(); ind++)
            if (rand.NextFloat() < probs.at(ind)) break;
          Xtal *xtal1 = xtals->at(ind);

          // Perform mutation
          double amplitude=0, stdev=0;
          xtal = XtalOptGenetic::stripple(xtal1,
                                          mut_strainStdev_min,
                                          mut_strainStdev_max,
                                          mut_amp_min,
                                          mut_amp_max,
                                          mut_per1,
                                          mut_per2,
                                          stdev,
                                          amplitude);
          QWriteLocker locker (xtal->lock());

          // Lock parent and extract info
          xtal1->lock()->lockForRead();
          uint gen1 = xtal1->getGeneration();
          uint id1 = xtal1->getXtalNumber();
          xtal1->lock()->unlock();

          // Determine generation number
          xtal->setGeneration(gen1 + 1);

          // Write lock here so that numbers aren't assigned twice
          m_rwLock->lockForRead();
          startPendingTracker->lock();

          // Determine xtal number
          uint id = 1;
          for (int j = 0; j < inxtals->size(); j++) {
            txtal = inxtals->at(j);
            txtal->lock()->lockForRead();
            if (txtal->getGeneration() == xtal->getGeneration() &&
                txtal->getXtalNumber() >= id) {
              id = txtal->getXtalNumber() + 1;
            }
            txtal->lock()->unlock();
          }
          // Check pending xtal list, too!
          QList<Xtal*> pendList = startPendingTracker->list();
          for (int j = 0; j < pendList.size(); j++) {
            txtal = pendList.at(j);
            txtal->lock()->lockForRead();
            if (txtal->getGeneration() == xtal->getGeneration() &&
                txtal->getXtalNumber() >= id) {
              id = txtal->getXtalNumber() + 1;
            }
            txtal->lock()->unlock();
          }
          startPendingTracker->unlock();
          m_rwLock->unlock();
          xtal->setXtalNumber(id);
          xtal->setParents(QString("Stripple: %1x%2 stdev=%3 amp=%4 waves=%5,%6")
                           .arg(gen1)
                           .arg(id1)
                           .arg(stdev, 0, 'f', 5)
                           .arg(amplitude, 0, 'f', 5)
                           .arg(mut_per1)
                           .arg(mut_per2)
                           );
          continue;
        }
        case Permustrain: {
          //qDebug() << "Performing permustrain";
          int ind;
          for (ind = 0; ind < probs.size(); ind++)
            if (rand.NextFloat() < probs.at(ind)) break;

          Xtal *xtal1 = xtals->at(ind);
          double stdev=0;
          xtal = XtalOptGenetic::permustrain(xtals->at(ind), perm_strainStdev_max, perm_ex, stdev);
          QWriteLocker locker (xtal->lock());

          // Lock parent and extract info
          xtal1->lock()->lockForRead();
          uint gen1 = xtal1->getGeneration();
          uint id1 = xtal1->getXtalNumber();
          xtal1->lock()->unlock();

          // Determine generation number
          uint gen = gen1 + 1;
          xtal->setGeneration(gen);

          // Lock here so that numbers aren't assigned twice
          m_rwLock->lockForRead();
          startPendingTracker->lock();

          // Determine xtal number
          uint id = 1;
          for (int j = 0; j < inxtals->size(); j++) {
            txtal = inxtals->at(j);
            txtal->lock()->lockForRead();
            if (txtal->getGeneration() == xtal->getGeneration() &&
                txtal->getXtalNumber() >= id) {
              id = txtal->getXtalNumber() + 1;
            }
            txtal->lock()->unlock();
          }
          // Check pending xtal list, too!
          QList<Xtal*> pendList = startPendingTracker->list();
          for (int j = 0; j < pendList.size(); j++) {
            txtal = pendList.at(j);
            txtal->lock()->lockForRead();
            if (txtal->getGeneration() == gen &&
                txtal->getXtalNumber() >= id) {
              id = txtal->getXtalNumber() + 1;
            }
            txtal->lock()->unlock();
          }
          startPendingTracker->unlock();
          m_rwLock->unlock();
          xtal->setXtalNumber(id);
          xtal->setParents(QString("Permustrain: %1x%2 stdev=%3 exch=%4")
                           .arg(gen1)
                           .arg(id1)
                           .arg(stdev, 0, 'f', 5)
                           .arg(perm_ex)
                           );
          continue;
        }
        default:
          warning("XtalOpt::generateSingleOffspring: Attempt to use an invalid operator.");
        }
      }
      if (attemptCount >= 100) {
        QString opStr;
        switch (op) {
        case Crossover:                 opStr = "crossover"; break;
        case Stripple:                  opStr = "stripple"; break;
        case Permustrain:               opStr = "permustrain"; break;
        default:                        opStr = "(unknown)"; break;
        }
        warning(tr("Unable to perform operation %1 after 1000 tries. Reselecting operator...").arg(opStr));
      }
    }

    //qDebug() << "New structure generated!";
    delete xtals;
    xtal->findSpaceGroup();
    return xtal;
  }

  QList<Xtal*> * XtalOpt::newGeneration(QList<Xtal*> *inxtals, uint numberNew, uint generationNumber, uint idSeed) {
    // qDebug() << "XtalOpt::newGeneration( "
    //          << inxtals << ", "
    //          << numberNew << ", "
    //          << generationNumber << ", "
    //          << idSeed << " ) called";

    // Return list and xtal pointer
    QList<Xtal*> *ret = new QList<Xtal*>;
    Xtal *cxtal=0, *xtal=0;

    // Setup random engine
    OpenBabel::OBRandom rand (true);    // "true" uses system random numbers. OB's version isn't too good...
    rand.TimeSeed();

    // Convert percentages into counts
    uint n_her          = static_cast<uint>( (p_her/100.0) * numberNew);
    uint n_mut		= static_cast<uint>( (p_mut/100.0) * numberNew);
    uint n_perm		= static_cast<uint>( (p_perm/100.0) * numberNew);

    // Trim and sort list
    QList<Xtal*> *xtals = new QList<Xtal*>;
    for (int i = 0; ( i < inxtals->size()); i++) {
      xtal = inxtals->at(i);
      xtal->lock()->lockForRead();
      if (xtal->getStatus() == Xtal::Optimized)
        xtals->append(xtal);
      xtal->lock()->unlock();
    }

    if (xtals->size() <= 1) {
      error("XtalOpt::newGeneration: Not enough xtals are optimized to make a new generation!");
      return ret;
    }

    XtalOpt::sortByEnthalpy(xtals);
    // Remove all but (n_consider + 1). The "+ 1" will be removed after probability generation.
    while ( static_cast<uint>(xtals->size()) > popSize + 1 ) xtals->removeLast();

    // Make list of weighted probabilities based on enthalpy values
    QList<double> probs = getProbabilityList(xtals);

    // Perform crossover
    if (xtals->size() < 2) {
      qWarning() << "Skipping crossover -- too few structures.";
    }
    else {
      //qDebug() << "Starting Crossover";
      int ind1, ind2;
      for (uint i = 0; i < n_her; i++) {
        // Select structures
        ind1 = ind2 = 0;
        while (ind1 == ind2) {
          double r = rand.NextFloat();
          for (ind1 = 0; ind1 < probs.size(); ind1++)
            if (r < probs.at(ind1)) break;
          r = rand.NextFloat();
          for (ind2 = 0; ind2 < probs.size(); ind2++)
            if (r < probs.at(ind2)) break;
        }
        double percent1;
        cxtal = XtalOptGenetic::crossover(xtals->at(ind1), xtals->at(ind2), her_minimumContribution, percent1);
        if (!checkXtal(cxtal)) {
          i--;
          delete cxtal;
          continue;
        }
        cxtal->lock()->lockForWrite();
        xtals->at(ind1)->lock()->lockForRead();
        xtals->at(ind2)->lock()->lockForRead();
        cxtal->setGeneration(generationNumber);
        cxtal->setXtalNumber(idSeed++);
        cxtal->setParents(QString("Crossover: %1x%2 (%3%) + %4x%5 (%6%)")
                          .arg(xtals->at(ind1)->getGeneration())
                          .arg(xtals->at(ind1)->getXtalNumber())
                          .arg(percent1, 0, 'f', 0)
                          .arg(xtals->at(ind2)->getGeneration())
                          .arg(xtals->at(ind2)->getXtalNumber())
                          .arg(100.0 - percent1, 0, 'f', 0)
                          );
        cxtal->lock()->unlock();
        xtals->at(ind1)->lock()->unlock();
        xtals->at(ind2)->lock()->unlock();
        ret->append(cxtal);
      }
    }

    // Perform stripples
    //qDebug() << "Starting Stripples";
    int ind;
    for (uint i = 0; i < n_mut; i++) {
      double r = rand.NextFloat();
      for (ind = 0; ind < probs.size(); ind++)
        if (r < probs.at(ind)) break;
      double amplitude=0;
      double stdev=0;
      cxtal = XtalOptGenetic::stripple(xtals->at(ind),
                                       mut_strainStdev_min, mut_strainStdev_max,
                                       mut_amp_min, mut_amp_max,
                                       mut_per1, mut_per2,
                                       stdev,
                                       amplitude);
      if (!checkXtal(cxtal)) {
        i--;
        delete cxtal;
        continue;
      }
      cxtal->lock()->lockForWrite();
      xtals->at(ind)->lock()->lockForRead();
      cxtal->setGeneration(generationNumber);
      cxtal->setXtalNumber(idSeed++);
      cxtal->setParents(QString("Stripple:: %1x%2 stdev=%3 amp=%4 waves=%5,%6")
                        .arg(xtals->at(ind)->getGeneration())
                        .arg(xtals->at(ind)->getXtalNumber())
                        .arg(stdev, 0, 'f', 5)
                        .arg(amplitude, 0, 'f', 5)
                        .arg(mut_per1)
                        .arg(mut_per2)
                        );
      cxtal->lock()->unlock();
      xtals->at(ind)->lock()->unlock();
      ret->append(cxtal);
    }

    // Perform permustrains
    //qDebug() << "Starting Permustrains";
    for (uint i = 0; i < n_perm; i++) {
      double r = rand.NextFloat();
      for (ind = 0; ind < probs.size(); ind++)
        if (r < probs.at(ind)) break;
      double stdev = 0;
      cxtal = XtalOptGenetic::permustrain(xtals->at(ind), perm_strainStdev_max, perm_ex, stdev);
      if (!checkXtal(cxtal)) {
        i--;
        delete cxtal;
        continue;
      }
      cxtal->lock()->lockForWrite();
      xtals->at(ind)->lock()->lockForRead();
      cxtal->setGeneration(generationNumber);
      cxtal->setXtalNumber(idSeed++);
      cxtal->setParents(QString("Permustrain:: %1x%2 stdev=%3 exch=%4")
                        .arg(xtals->at(ind)->getGeneration())
                        .arg(xtals->at(ind)->getXtalNumber())
                        .arg(stdev, 0, 'f', 5)
                        .arg(perm_ex)
                        );
      cxtal->lock()->unlock();
      xtals->at(ind)->lock()->unlock();
      ret->append(cxtal);
    }

    //qDebug() << "Generation complete!";

    delete xtals;
    return ret;
  }

  bool XtalOpt::checkLimits() {
    //qDebug() << "XtalOptRand::checkLimits() called";
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
      //qDebug() << "XtalOpt::checkXtal() called on a null pointer";
      return false;
    }
    //qDebug() << "XtalOpt::checkXtal( " << xtal << " ) called";

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
      qDebug() << "Rescaling volume from " << xtal->getVolume() << " to " << newvol;
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
      qDebug() << "Not saving while session is starting";
      savePending = false;
      return false;
    }
    QMutexLocker locker (stateFileMutex);
    // Back up xtalopt.state
    QFile file (filePath + "/" + fileBase + "xtalopt.state");
    QFile oldfile (filePath + "/" + fileBase + "xtalopt.state.old");
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
    // Store values
    out << "optType: " << optType << endl
        << "numInitial: " << numInitial << endl
        << "popSize: " << popSize << endl
        << "genTotal: " << genTotal << endl
        << "p_her: " << p_her << endl
        << "p_mut: " << p_mut << endl
        << "p_perm: " << p_perm << endl
        << "her_minimumContribution: " << her_minimumContribution << endl
        << "perm_ex: " << perm_ex << endl
        << "perm_strainStdev_max: " << perm_strainStdev_max << endl
        << "mut_strainStdev_min: " << mut_strainStdev_min << endl
        << "mut_strainStdev_max: " << mut_strainStdev_max << endl
        << "mut_amp_min: " << mut_amp_min << endl
        << "mut_amp_max: " << mut_amp_max << endl
        << "mut_per1: " << mut_per1 << endl
        << "mut_per2: " << mut_per2 << endl
        << "A_min: " << a_min << endl
        << "B_min: " << b_min << endl
        << "C_min: " << c_min << endl
        << "alpha_min: " << alpha_min << endl
        << "beta_min: " << beta_min << endl
        << "gamma_min: " << gamma_min << endl
        << "A_max: " << a_max << endl
        << "B_max: " << b_max << endl
        << "C_max: " << c_max << endl
        << "alpha_max: " << alpha_max << endl
        << "beta_max: " << beta_max << endl
        << "gamma_max: " << gamma_max << endl
        << "vol_min: " << vol_min << endl
        << "vol_max: " << vol_max << endl
        << "vol_fixed: " << vol_fixed << endl
        << "shortestInteratomicDistance: " << shortestInteratomicDistance << endl
        << "tol_enthalpy: " << tol_enthalpy << endl
        << "tol_volume: " << tol_volume << endl
        << "using_fixed_volume: " << using_fixed_volume << endl
        << "using_shortestInteratomicDistance: " << using_shortestInteratomicDistance << endl
        << "using_remote: " << using_remote << endl
        << "limitRunningJobs: " << limitRunningJobs << endl
        << "runningJobLimit: " << runningJobLimit << endl
        << "failLimit: " << failLimit << endl
        << "failAction: " << failAction << endl
        << "launchCommand: " << launchCommand << endl
        << "queueCheck: " << queueCheck << endl
        << "queueDelete: " << queueDelete << endl
        << "host: " << host << endl
        << "username: " << username << endl
        << "rempath: " << rempath << endl
        << "VASPUser1: " << VASPUser1 << endl
        << "VASPUser2: " << VASPUser2 << endl
        << "VASPUser3: " << VASPUser3 << endl
        << "VASPUser4: " << VASPUser4 << endl
        << "GULPUser1: " << GULPUser1 << endl
        << "GULPUser2: " << GULPUser2 << endl
        << "GULPUser3: " << GULPUser3 << endl
        << "GULPUser4: " << GULPUser4 << endl
        << "PWscfUser1: " << PWscfUser1 << endl
        << "PWscfUser2: " << PWscfUser2 << endl
        << "PWscfUser3: " << PWscfUser3 << endl
        << "PWscfUser4: " << PWscfUser4 << endl
      //
      // VASP File lists
      //
        << "### Begin VASP_INCAR_list ###" << endl;
    for (int i = 0; i < VASP_INCAR_list.size(); i++)
      out << "### NEXT INCAR:" << endl
          << VASP_INCAR_list.at(i) << endl;
    out << "### End VASP_INCAR_list ###" << endl
        << "### Begin VASP_qScript_list ###" << endl;
    for (int i = 0; i < VASP_qScript_list.size(); i++)
      out << "### NEXT VASP_qScript:" << endl
          << VASP_qScript_list.at(i) << endl;
    out << "### End VASP_qScript_list ###" << endl
        << "### Begin VASP_KPOINTS_list ###" << endl;
    for (int i = 0; i < VASP_KPOINTS_list.size(); i++)
      out << "### NEXT KPOINTS:" << endl
          << VASP_KPOINTS_list.at(i) << endl;
    out << "### End VASP_KPOINTS_list ###" << endl
        << "### Begin VASP_POTCAR_info ###" << endl;
    for (int i = 0; i < VASP_POTCAR_info.size(); i++) {
      out << "### NEXT POTCAR_info:" << endl;
      QStringList symbols = VASP_POTCAR_info.at(i).keys();
      qSort(symbols);
      for (int j = 0; j < symbols.size(); j++) {
        out << symbols.at(j) << " " << VASP_POTCAR_info.at(i)[symbols.at(j)] << endl;
      }
    }
    out << "### End VASP_POTCAR_info ###" << endl
      // VASP_POTCAR_list will be regenerated on load up.

      //
      // GULP file lists
      //
        << "### Begin GULP_gin_list ###" << endl;
    for (int i = 0; i < GULP_gin_list.size(); i++)
      out << "### NEXT GULP_gin:" << endl
          << GULP_gin_list.at(i) << endl;
    out << "### End GULP_gin_list ###" << endl;

    //
    // PWscf file lists
    //
    out << "### Begin PWscf_in_list ###" << endl;
    for (int i = 0; i < PWscf_in_list.size(); i++)
      out << "### NEXT in:" << endl
          << PWscf_in_list.at(i) << endl;
    out << "### End PWscf_in_list ###" << endl
        << "### Begin PWscf_qScript_list ###" << endl;
    for (int i = 0; i < PWscf_qScript_list.size(); i++)
      out << "### NEXT PWscf_qScript:" << endl
          << PWscf_qScript_list.at(i) << endl;
    out << "### End PWscf_qScript_list ###" << endl;

    // Save composition:
    out << "Composition:";
    QList<uint> keys = comp->keys();
    for (int i = 0; i < keys.size(); i++)
      out << " " << QString::number(keys.at(i)) << " " << QString::number(comp->value(keys.at(i)));
    out << endl;

    // VASP_POTCAR_comp
    out << "VASP_POTCAR_comp:";
    for (int i = 0; i < VASP_POTCAR_comp.size(); i++)
      out << " " << QString::number(VASP_POTCAR_comp.at(i));
    out << endl;

    // Loop over xtals and save them
    QFile xfile;
    QReadLocker structLocker (rwLock());
    QList<Xtal*>* xtals = getStructures();
    // out << "numXtals: " << xtals->size() << endl;
    Xtal* xtal;
    QTextStream xout;
    // out << "### Begin Xtals ###" << endl;
    for (int i = 0; i < xtals->size(); i++) {
      xtal = xtals->at(i);
      xtal->lock()->lockForRead();
      xfile.setFileName(xtal->fileName() + "/xtal.state");
      if (!xfile.open(QIODevice::WriteOnly)) {
        error("XtalOpt::save(): Error opening file "+xfile.fileName()+" for writing...");
        xtal->lock()->unlock();
        savePending = false;
        return false;
      }
      xout.setDevice(&xfile);
      // out << xfile.fileName() << endl;
      xtal->saveXtal(xout);
      xtal->lock()->unlock();
      xfile.close();
    }
    // out << "### End Xtals ###" << endl;

    out.flush();
    file.close();
    oldfile.close();

    /////////////////////////
    // Print results files //
    /////////////////////////

    file.setFileName(filePath + "/" + fileBase + "results.txt");
    oldfile.setFileName(filePath + "/" + fileBase + "results_old.txt");
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
    out.setDevice(&file);

    QList<Xtal*> sortedXtals;

    xtals = getStructures();
    for (int i = 0; i < xtals->size(); i++)
      sortedXtals.append(xtals->at(i));
    if (sortedXtals.size() != 0) sortByEnthalpy(&sortedXtals);

    // Print the data to the file:
    out << "Rank\tGen\tID\tEnthalpy\tSpaceGroup\tStatus\n";
    for (int i = 0; i < sortedXtals.size(); i++) {
      xtal = sortedXtals.at(i);
      if (!xtal) continue; // In case there was a problem copying.
      xtal->lock()->lockForRead();
      out << i << "\t"
          << xtal->getGeneration() << "\t"
          << xtal->getXtalNumber() << "\t"
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
    //qDebug() << "XtalOpt::load( " << filename << " ) called";

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

    // Set filePath, figure out and set fileBase:
    filePath = dataPath;
    fileBase = filename;
    fileBase.remove(filePath);
    fileBase.remove("xtalopt.state");

    // Set up stream for reading
    QTextStream in (&file);
    QString line, str;
    QStringList strl;
    while (!in.atEnd()) {
      line = in.readLine();
      //qDebug() << line;
      strl = line.split(QRegExp("\\s+"));

      if (line.contains("optType:") && strl.size() > 1)
        optType = XtalOpt::OptTypes(strl.at(1).toInt());
      if (line.contains("numInitial:") && strl.size() > 1)
        numInitial = strl.at(1).toUInt();
      if (line.contains("popSize:") && strl.size() > 1)
        popSize = strl.at(1).toUInt();
      if (line.contains("genTotal:") && strl.size() > 1)
        genTotal = strl.at(1).toUInt();
      if (line.contains("p_her:") && strl.size() > 1)
        p_her = strl.at(1).toUInt();
      if (line.contains("p_mut:") && strl.size() > 1)
        p_mut = strl.at(1).toUInt();
      if (line.contains("p_perm:") && strl.size() > 1)
        p_perm = strl.at(1).toUInt();
      if (line.contains("her_minimumContribution:") && strl.size() > 1)
        her_minimumContribution = strl.at(1).toUInt();
      if (line.contains("perm_ex:") && strl.size() > 1)
        perm_ex = strl.at(1).toUInt();
      if (line.contains("perm_strainStdev_max:") && strl.size() > 1)
        perm_strainStdev_max = strl.at(1).toDouble();
      if (line.contains("mut_strainStdev_min:") && strl.size() > 1)
        mut_strainStdev_min = strl.at(1).toDouble();
      if (line.contains("mut_strainStdev_max:") && strl.size() > 1)
        mut_strainStdev_max = strl.at(1).toDouble();
      if (line.contains("mut_amp_min:") && strl.size() > 1)
        mut_amp_min = strl.at(1).toDouble();
      if (line.contains("mut_amp_max:") && strl.size() > 1)
        mut_amp_max = strl.at(1).toDouble();
      if (line.contains("mut_per1:") && strl.size() > 1)
        mut_per1 = strl.at(1).toUInt();
      if (line.contains("mut_per2:") && strl.size() > 1)
        mut_per2 = strl.at(1).toUInt();
      if (line.contains("A_min:") && strl.size() > 1)
        a_min = strl.at(1).toDouble();
      if (line.contains("B_min:") && strl.size() > 1)
        b_min = strl.at(1).toDouble();
      if (line.contains("C_min:") && strl.size() > 1)
        c_min = strl.at(1).toDouble();
      if (line.contains("alpha_min:") && strl.size() > 1)
        alpha_min = strl.at(1).toDouble();
      if (line.contains("beta_min:") && strl.size() > 1)
        beta_min = strl.at(1).toDouble();
      if (line.contains("gamma_min:") && strl.size() > 1)
        gamma_min = strl.at(1).toDouble();
      if (line.contains("A_max:") && strl.size() > 1)
        a_max = strl.at(1).toDouble();
      if (line.contains("B_max:") && strl.size() > 1)
        b_max = strl.at(1).toDouble();
      if (line.contains("C_max:") && strl.size() > 1)
        c_max = strl.at(1).toDouble();
      if (line.contains("alpha_max:") && strl.size() > 1)
        alpha_max = strl.at(1).toDouble();
      if (line.contains("beta_max:") && strl.size() > 1)
        beta_max = strl.at(1).toDouble();
      if (line.contains("gamma_max:") && strl.size() > 1)
        gamma_max = strl.at(1).toDouble();
      if (line.contains("vol_min:") && strl.size() > 1)
        vol_min = strl.at(1).toDouble();
      if (line.contains("vol_max:") && strl.size() > 1)
        vol_max = strl.at(1).toDouble();
      if (line.contains("vol_fixed:") && strl.size() > 1)
        vol_fixed = strl.at(1).toDouble();
      if (line.contains("tol_enthalpy:") && strl.size() > 1)
        tol_enthalpy = strl.at(1).toDouble();
      if (line.contains("tol_volume:") && strl.size() > 1)
        tol_volume = strl.at(1).toDouble();
      if (line.contains("using_fixed_volume:") && strl.size() > 1)
        using_fixed_volume = strl.at(1).toInt();
      if (line.contains("using_shortestInteratomicDistance:") && strl.size() > 1)
        using_shortestInteratomicDistance = strl.at(1).toInt();
      if (line.contains("using_remote:") && strl.size() > 1)
        using_remote = strl.at(1).toInt();
      if (line.contains("limitRunningJobs:") && strl.size() > 1)
        limitRunningJobs = strl.at(1).toInt();
      if (line.contains("runningJobLimit:") && strl.size() > 1)
        runningJobLimit = strl.at(1).toInt();
      if (line.contains("failLimit:") && strl.size() > 1)
        failLimit = strl.at(1).toUInt();
      if (line.contains("failAction:") && strl.size() > 1)
        failAction = strl.at(1).toUInt();
      // These get determined auto-magically now.
      // if (line.contains("filePath:") && strl.size() > 1) {
      //   strl.removeFirst();
      //   filePath = strl.join(" ");
      // }
      // if (line.contains("fileBase:") && strl.size() > 1) {
      //   strl.removeFirst();
      //   fileBase = strl.join(" ");
      // }
      if (line.contains("launchCommand:") && strl.size() > 1) {
        strl.removeFirst();
        launchCommand = strl.join(" ");
      }
      if (line.contains("queueCheck:") && strl.size() > 1) {
        strl.removeFirst();
        queueCheck = strl.join(" ");
      }
      if (line.contains("queueDelete:") && strl.size() > 1) {
        strl.removeFirst();
        queueDelete = strl.join(" ");
      }
      if (line.contains("host:") && strl.size() > 1) {
        strl.removeFirst();
        host = strl.join(" ");
      }
      if (line.contains("username:") && strl.size() > 1) {
        strl.removeFirst();
        username = strl.join(" ");
      }
      if (line.contains("rempath:") && strl.size() > 1) {
        strl.removeFirst();
        rempath = strl.join(" ");
      }
      if (line.contains("VASPUser1:") && strl.size() > 1) {
        strl.removeFirst();
        VASPUser1 = strl.join(" ");
      }
      if (line.contains("VASPUser2:") && strl.size() > 1) {
        strl.removeFirst();
        VASPUser2 = strl.join(" ");
      }
      if (line.contains("VASPUser3:") && strl.size() > 1) {
        strl.removeFirst();
        VASPUser3 = strl.join(" ");
      }
      if (line.contains("VASPUser4:") && strl.size() > 1) {
        strl.removeFirst();
        VASPUser4 = strl.join(" ");
      }
      if (line.contains("GULPUser1:") && strl.size() > 1) {
        strl.removeFirst();
        GULPUser1 = strl.join(" ");
      }
      if (line.contains("GULPUser2:") && strl.size() > 1) {
        strl.removeFirst();
        GULPUser2 = strl.join(" ");
      }
      if (line.contains("GULPUser3:") && strl.size() > 1) {
        strl.removeFirst();
        GULPUser3 = strl.join(" ");
      }
      if (line.contains("GULPUser4:") && strl.size() > 1) {
        strl.removeFirst();
        GULPUser4 = strl.join(" ");
      }
      if (line.contains("PWscfUser1:") && strl.size() > 1) {
        strl.removeFirst();
        PWscfUser1 = strl.join(" ");
      }
      if (line.contains("PWscfUser2:") && strl.size() > 1) {
        strl.removeFirst();
        PWscfUser2 = strl.join(" ");
      }
      if (line.contains("PWscfUser3:") && strl.size() > 1) {
        strl.removeFirst();
        PWscfUser3 = strl.join(" ");
      }
      if (line.contains("PWscfUser4:") && strl.size() > 1) {
        strl.removeFirst();
        PWscfUser4 = strl.join(" ");
      }
      if (line.contains("### Begin VASP_INCAR_list ###")) {
        // Find start of first INCAR:
        while (!line.contains("### NEXT INCAR:") && !line.contains("### End VASP_INCAR_list ###")) {
          //qDebug() << "DISCARDING (INCAR): " << line;
          line = in.readLine();
        }
        uint i = 0;
        VASP_INCAR_list.clear();
        while (!in.atEnd() && !line.contains("### End VASP_INCAR_list ###")) {
          line = in.readLine(); // Get rid of the ### NEXT INCAR line
          VASP_INCAR_list.append("");
          QTextStream s (&(VASP_INCAR_list[i]));
          while (!in.atEnd() && !line.contains("### NEXT INCAR:") && !line.contains("### End VASP_INCAR_list ###")) {
            s << line;
            line = in.readLine();
            if (!in.atEnd() && !line.contains("### NEXT INCAR:") &&  !line.contains("### End VASP_INCAR_list ###"))
              s << endl;
          }
          i++;
        }
      }
      if (line.contains("### Begin queueScript_list ###") || line.contains("### Begin VASP_qScript_list ###")) {
        // Find start of first queueScript:
        while (!line.contains("### NEXT queueScript:") && !line.contains("### NEXT VASP_qScript:") &&
               !line.contains("### End queueScript_list ###") && !line.contains("### End VASP_qScript_list ###")) {
          //qDebug() << "DISCARDING (queueScript): " << line;
          line = in.readLine();
        }
        uint i = 0;
        VASP_qScript_list.clear();
        while (!in.atEnd() && !line.contains("### End queueScript_list ###") && !line.contains("### End VASP_qScript_list ###")) {
          line = in.readLine(); // Get rid of the ### NEXT INCAR line
          VASP_qScript_list.append("");
          QTextStream s (&(VASP_qScript_list[i]));
          while (!in.atEnd() && !line.contains("### NEXT queueScript:") && !line.contains("### End queueScript_list ###") &&
                 !line.contains("### NEXT VASP_qScript:") && !line.contains("### End VASP_qScript_list ###")) {
            s << line;
            line = in.readLine();
            if (!in.atEnd() && !line.contains("### NEXT queueScript:") &&  !line.contains("### End queueScript_list ###") &&
                 !line.contains("### NEXT VASP_qScript:") && !line.contains("### End VASP_qScript_list ###"))
              s << endl;
          }
          i++;
        }
      }
      if (line.contains("### Begin VASP_KPOINTS_list ###")) {
        // Find start of first KPOINTS:
        while (!line.contains("### NEXT KPOINTS:") && !line.contains("### End VASP_KPOINTS_list ###")) {
          //qDebug() << "DISCARDING (KPOINTS): " << line;
          line = in.readLine();
        }
        uint i = 0;
        VASP_KPOINTS_list.clear();
        while (!in.atEnd() && !line.contains("### End VASP_KPOINTS_list ###")) {
          line = in.readLine(); // Get rid of the ### NEXT INCAR line
          VASP_KPOINTS_list.append("");
          QTextStream s (&(VASP_KPOINTS_list[i]));
          while (!in.atEnd() && !line.contains("### NEXT KPOINTS:") && !line.contains("### End VASP_KPOINTS_list ###")) {
            s << line;
            line = in.readLine();
            if (!in.atEnd() && !line.contains("### NEXT KPOINTS:") &&  !line.contains("### End VASP_KPOINTS_list ###"))
              s << endl;
          }
          i++;
        }
      }
      if (line.contains("### Begin VASP_POTCAR_info ###")) {
        // Find start of first POTCAR:
        while (!line.contains("### NEXT POTCAR_info:") && !line.contains("### End VASP_POTCAR_info ###")) {
          //qDebug() << "DISCARDING (POTCAR): " << line;
          line = in.readLine();
        }
        uint i = 0;
        VASP_POTCAR_info.clear();
        while (!in.atEnd() && !line.contains("### End VASP_POTCAR_info ###")) {
          line = in.readLine(); // Get rid of the ### NEXT POTCAR line
          QHash<QString, QString> hash;
          VASP_POTCAR_info.append(hash);
          //qDebug() << "New POTCAR hash created...";
          while (!in.atEnd() && !line.contains("### NEXT POTCAR_info:") && !line.contains("### End VASP_POTCAR_info ###")) {
            strl = line.split(QRegExp("\\s+"));
            //qDebug() << "POTCAR info: " << line;
            if (strl.size() >= 2)
              (VASP_POTCAR_info[i]).insert(strl.at(0), strl.at(1));
            line = in.readLine();
          }
          i++;
        }
        XtalOptTemplate::buildVASP_POTCAR(this);
      }
      if (line.contains("### Begin GULP_gin_list ###")) {
        // Find start of first GULP_gin:
        while (!line.contains("### NEXT GULP_gin:") && !line.contains("### End GULP_gin_list ###")) {
          //qDebug() << "DISCARDING (GULP_gin): " << line;
          line = in.readLine();
        }
        uint i = 0;
        GULP_gin_list.clear();
        while (!in.atEnd() && !line.contains("### End GULP_gin_list ###")) {
          line = in.readLine(); // Get rid of the ### NEXT GULP_gin line
          GULP_gin_list.append("");
          QTextStream s (&(GULP_gin_list[i]));
          while (!in.atEnd() && !line.contains("### NEXT GULP_gin:") && !line.contains("### End GULP_gin_list ###")) {
            s << line;
            line = in.readLine();
            if (!in.atEnd() && !line.contains("### NEXT GULP_gin:") &&  !line.contains("### End GULP_gin_list ###"))
              s << endl;
          }
          i++;
        }
      }
      // PWscf
      if (line.contains("### Begin PWscf_in_list ###")) {
        // Find start of first PWscf in:
        while (!line.contains("### NEXT in:") && !line.contains("### End PWscf_in_list ###")) {
          //qDebug() << "DISCARDING (PWscf in): " << line;
          line = in.readLine();
        }
        uint i = 0;
        PWscf_in_list.clear();
        while (!in.atEnd() && !line.contains("### End PWscf_in_list ###")) {
          line = in.readLine(); // Get rid of the "### NEXT in" line
          PWscf_in_list.append("");
          QTextStream s (&(PWscf_in_list[i]));
          while (!in.atEnd() && !line.contains("### NEXT in:") && !line.contains("### End PWscf_in_list ###")) {
            s << line;
            line = in.readLine();
            if (!in.atEnd() && !line.contains("### NEXT in:") &&  !line.contains("### End PWscf_in_list ###"))
              s << endl;
          }
          i++;
        }
      }
      if (line.contains("### Begin PWscf_qScript_list ###")) {
        // Find start of first qScript:
        while (!line.contains("### NEXT PWscf_qScript:") && !line.contains("### End PWscf_qScript_list ###")) {
          //qDebug() << "DISCARDING (PWscf qScript): " << line;
          line = in.readLine();
        }
        uint i = 0;
        PWscf_qScript_list.clear();
        while (!in.atEnd() && !line.contains("### End PWscf_qScript_list ###")) {
          line = in.readLine(); // Get rid of the ### NEXT qScript line
          PWscf_qScript_list.append("");
          QTextStream s (&(PWscf_qScript_list[i]));
          while (!in.atEnd() &&
                 !line.contains("### NEXT PWscf_qScript:") &&
                 !line.contains("### End PWscf_qScript_list ###")) {
            s << line;
            line = in.readLine();
            if (!in.atEnd() &&
                !line.contains("### NEXT PWscf_qScript:") &&
                !line.contains("### End PWscf_qScript_list ###"))
              s << endl;
          }
          i++;
        }
      }

      // Composition
      if (line.contains("Composition:") && strl.size() > 1 ) {
        if (comp) delete comp;
        comp = new QHash<uint,uint>;
        for (int i = 1; i <= strl.size() - 2; i += 2)
          comp->insert(strl.at(i).toUInt(), strl.at(i+1).toUInt());
      }

      // VASP_POTCAR_comp
      if (line.contains("VASP_POTCAR_comp:") && strl.size() > 1 ) {
        VASP_POTCAR_comp.clear();
        for (int i = 1; i < strl.size(); i++)
          VASP_POTCAR_comp.append(strl.at(i).toUInt());
      }

      // Xtals
      // Old method: this has been replace with a method that
      // doesn't require absolute paths.
      //
      // if (line.contains("numXtals:") && strl.size() > 1) {
      //   numXtals = strl.at(1).toInt();
      //   dialog->updateProgressMaximum(numXtals);
      // }
      // if (line.contains("### Begin Xtals ###")) {
      //   line = in.readLine();
      //   //qDebug() << line;
      //   Xtal* xtal;
      //   QList<uint> keys = comp->keys();
      //   QFile xfile;
      //   QTextStream xin;
      //   uint count = 0;

      //   while (!in.atEnd() && !line.contains("### End Xtals ###")) {
      //     count++;
      //     dialog->updateProgressLabel(tr("Loading structures(%1 of %2)...").arg(count).arg(numXtals));
      //     dialog->updateProgressValue(count-1);

      //     xfile.setFileName(line);
      //     if (!xfile.open(QIODevice::ReadOnly)) {
      //       error("Error, cannot open file for reading: "+xfile.fileName());
      //       return false;
      //     }
      //     xin.setDevice(&xfile);

      //     xtal = new Xtal();
      //     QWriteLocker locker (xtal->lock());
      //     // Add empty atoms to xtal, updateXtal will populate it
      //     for (int i = 0; i < keys.size(); i++) {
      //       for (uint j = 0; j < comp->value(keys.at(i)); j++)
      //         xtal->addAtom();
      //     }
      //     xtal->loadXtal(xin);

      //     // Try to load CONTCAR first
      //     // Disable remote checking for now -- just load in local cache.
      //     bool usingRemote = using_remote;
      //     using_remote = false;
      //     // Store current state -- updateXtal will overwrite it.
      //     Xtal::State state = xtal->getStatus();
      //     QDateTime endtime = xtal->getOptTimerEnd();

      //     locker.unlock();


      //     if (!Optimizer::loadXtal(xtal, this)) {
      //       error("Error, no (or not appropriate) xtal data in "+xfile.fileName());
      //       return false;
      //     }

      //     // Reenable remote checking if needed
      //     using_remote = usingRemote;

      //     // Reset state
      //     locker.relock();
      //     xtal->setStatus(state);
      //     if (state != Xtal::Optimized &&
      //         state != Xtal::Killed    &&
      //         state != Xtal::Removed   &&
      //         state != Xtal::Duplicate ){
      //       submissionPendingTracker->removeXtal(xtal);
      //       runningTracker->addXtal(xtal);
      //     }
      //     if (state == Xtal::WaitingForOptimization) {
      //       submissionPendingTracker->addXtal(xtal);
      //       runningTracker->removeXtal(xtal);
      //     }
      //     xtal->setOptTimerEnd(endtime);
      //     locker.unlock();
      //     m_rwLock->lockForWrite();
      //     appendStructure(xtal);
      //     m_rwLock->unlock();

      //     line = in.readLine();
      //   }
    }

    // Xtals
    // New method:
    //
    // Initialize progress bar:
    dialog->updateProgressMaximum(xtalDirs.size());
    Xtal* xtal;
    QList<uint> keys = comp->keys();
    QList<Xtal*> loadedXtals;
    QFile xfile;
    QTextStream xin;
    uint count = 0;
    int numDirs = xtalDirs.size();
    for (int i = 0; i < numDirs; i++) {
      count++;
      dialog->updateProgressLabel(tr("Loading structures(%1 of %2)...").arg(count).arg(numDirs));
      dialog->updateProgressValue(count-1);

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
        for (uint k = 0; k < comp->value(keys.at(j)); k++)
          xtal->addAtom();
      }
      xtal->setFileName(dataPath + "/" + xtalDirs.at(i) + "/");
      xtal->loadXtal(xin);

      // Disable remote checking for now -- just load in local cache.
      bool usingRemote = using_remote;
      using_remote = false;
      // Store current state -- updateXtal will overwrite it.
      Xtal::State state = xtal->getStatus();
      QDateTime endtime = xtal->getOptTimerEnd();

      locker.unlock();

      if (!Optimizer::loadXtal(xtal, this)) {
        error("Error, no (or not appropriate) xtal data in "+xfile.fileName() + ".\n\n" +
              "This could be a result of resuming a structure that has not yet done any" +
              "local optimizations. If so, safely ignore this message.");
        continue;
      }

      // Reenable remote checking if needed
      using_remote = usingRemote;

      // Reset state
      locker.relock();
      xtal->setStatus(state);
      if (state != Xtal::Optimized &&
          state != Xtal::Killed    &&
          state != Xtal::Removed   &&
          state != Xtal::Duplicate ){
        submissionPendingTracker->removeXtal(xtal);
        runningTracker->addXtal(xtal);
      }
      if (state == Xtal::WaitingForOptimization) {
        submissionPendingTracker->addXtal(xtal);
        runningTracker->removeXtal(xtal);
      }
      xtal->setOptTimerEnd(endtime);
      locker.unlock();
      m_rwLock->lockForWrite();
      loadedXtals.append(xtal);
      m_rwLock->unlock();
    }

    // Sort Xtals by index values
    int curpos = 0;
    //dialog->stopProgressUpdate();
    //dialog->startProgressUpdate("Sorting xtals...", 0, loadedXtals.size()-1);
    for (int i = 0; i < loadedXtals.size(); i++) {
      for (int j = 0; j < loadedXtals.size(); j++) {
        //dialog->updateProgressValue(curpos);
        if (loadedXtals.at(j)->getIndex() == i) {
          loadedXtals.swap(j, curpos);
          curpos++;
        }
      }
    }

    // Reassign indices (shouldn't always be necessary, but just in case...)
    for (int i = 0; i < loadedXtals.size(); i++) {
      loadedXtals.at(i)->setIndex(i);
    }


    appendStructures(&loadedXtals);
    return true;
  }

  QList<Xtal*>* XtalOpt::getStructures() {
    return m_xtals;
  }

  Xtal* XtalOpt::getStructureAt(int i) {
    return m_xtals->at(i);
  }

  void XtalOpt::checkPopulation() {
    //qDebug() << "XtalOpt::checkPopulation() called.";
    if (isStarting) {
      qDebug() << "Not checking population while session is starting";
      return;
    }

    if (checkPopulationPending) {
      qDebug() << "XtalOpt::checkPopulation(): ignoring spurious call.";
      return;
    }
    checkPopulationPending = true;

    // Count jobs
    uint running = 0;
    uint optimized = 0;
    uint submitted = 0;
    rwLock()->lockForRead();
    running += startPendingTracker->countAndLock();
    // Check to see that the number of running jobs is >= that specified:
    Xtal *xtal = 0;
    Xtal::State state;
    int fail=0;
    for (int i = 0; i < getStructures()->size(); i++) {
      xtal = getStructures()->at(i);
      xtal->lock()->lockForRead();
      state = xtal->getStatus();
      if (xtal->getFailCount() != 0) fail++;
      xtal->lock()->unlock();
      if ( state == Xtal::Submitted ||
           state == Xtal::InProcess ){
        submitted++;
        running++;
        submissionPendingTracker->removeXtal(xtal);
        runningTracker->addXtal(xtal);
      }
      else if ( state != Xtal::Optimized &&
                state != Xtal::Duplicate &&
                state != Xtal::WaitingForOptimization &&
                state != Xtal::Killed &&
                state != Xtal::Removed ) {
        running++;
        submissionPendingTracker->removeXtal(xtal);
        runningTracker->addXtal(xtal);
      }
      else if ( state == Xtal::Optimized ) {
        optimized++;
      }
      else if ( state == Xtal::WaitingForOptimization ) {
        running++;
        submissionPendingTracker->addXtal(xtal);
        runningTracker->removeXtal(xtal);
      }
      else {
        submissionPendingTracker->removeXtal(xtal);
        runningTracker->removeXtal(xtal);
      }
    }
    startPendingTracker->unlock();
    rwLock()->unlock();
    dialog->updateStatus(optimized, running, fail);

    // Submit any jobs if needed
    int pending = submissionPendingTracker->countAndLock();
    while (pending != 0 &&
           (
            limitRunningJobs == false ||
            submitted < runningJobLimit
            )
           ) {
      submitJob();
      submitted++;
      pending--;
    }
    submissionPendingTracker->unlock();

    // Generate new offspring from pool
    while (running < genTotal) {
      if ( optimized >= 3) {
        // Add a new structure to replace the one that optimized
        xtal = XtalOpt::generateSingleOffspring(getStructures());
      } else {// There aren't enough optimized structures to contribute to a genetic matching
        //  Add random structures instead!
        uint id = 0;
        rwLock()->lockForRead();
        startPendingTracker->lock();
        for (int i = 0; i < getStructures()->size(); i++) {
          xtal = getStructures()->at(i);
          xtal->lock()->lockForRead();
          if (xtal->getGeneration() == 1 && xtal->getXtalNumber() >= id)
            id = xtal->getXtalNumber() + 1;
          xtal->lock()->unlock();
        }
        QList<Xtal*> pendList = startPendingTracker->list();
        for (int i = 0; i < pendList.size(); i++) {
          xtal = pendList.at(i);
          xtal->lock()->lockForRead();
          if (xtal->getGeneration() == 1 && xtal->getXtalNumber() >= id)
            id = xtal->getXtalNumber() + 1;
          xtal->lock()->unlock();
        }
        startPendingTracker->unlock();
        rwLock()->unlock();
        xtal = generateRandomXtal(1, id);
        if (!checkXtal(xtal)) {
          delete xtal;
          continue;
        }
      }
      qDebug() << "Adding Xtal "<<xtal->getIDString();
      addXtal(xtal);
      running++;
    }

    checkPopulationPending = false;
  }

  void XtalOpt::updateXtal() {
    //qDebug() << "XtalOpt::updateXtal() called";
    QtConcurrent::run(this, &XtalOpt::updateXtal_);
  }

  void XtalOpt::updateXtal_() {
    //qDebug() << "XtalOpt::updateXtal_() called";
    Xtal *xtal = 0;
    if (!updatePendingTracker->popFirst(xtal)) return;
    Optimizer::updateXtal(xtal, this);
  }

  void XtalOpt::errorXtal(const QStringList & queueData) {
    //qDebug() << "XtalOpt::errorXtal() called";
    QtConcurrent::run(this, &XtalOpt::errorXtal_, queueData);
  }

  void XtalOpt::errorXtal_(const QStringList & queueData) {
    //qDebug() << "XtalOpt::errorXtal_() called";
    Xtal *xtal = 0;
    if (!errorPendingTracker->popFirst(xtal)) return;;
    bool exists = false;
    int jobID;

    // Check if the job is running under a different JobID (Stranger things have happened!)
    jobID = Optimizer::checkIfJobNameExists(xtal, this, queueData, exists);
    QWriteLocker locker (xtal->lock());
    if (exists) { // Mark the job as running and update the jobID
      warning(tr("Reclaiming jobID %1 for xtal %2")
              .arg(QString::number(jobID))
              .arg(xtal->getIDString()));
      xtal->setStatus(Xtal::InProcess);
      xtal->setJobID(jobID);
    }
    else { //restart job
      // Rewrite input files
      locker.unlock();
      Optimizer::writeInputFiles(xtal, this);
      runningTracker->removeXtal(xtal);
      submissionPendingTracker->addXtal(xtal);
    }
  }

  void XtalOpt::submitJob() {
    //qDebug() << "XtalOpt::submitJob() called";
    QtConcurrent::run(this, &XtalOpt::submitJob_);
  }

  void XtalOpt::submitJob_() {
    //qDebug() << "XtalOpt::submitJob_() called";
    Xtal *xtal = 0;
    if (!submissionPendingTracker->popFirstAndLock(xtal)) {
      submissionPendingTracker->unlock();
      return;
    }
    // Return if xtal is already running
    if (runningTracker->hasXtal(xtal)) {
      submissionPendingTracker->unlock();
      return;
    }
    xtal->lock()->lockForWrite();
    xtal->setStatus(Xtal::Submitted);
    xtal->lock()->unlock();
    runningTracker->addXtal(xtal);
    submissionPendingTracker->unlock();

    if (!Optimizer::startOptimization(xtal, this)) {
      warning(tr("XtalOpt::submitJob_: Job did not run successfully for xtal %1-%2.")
              .arg(xtal->getIDString())
              .arg(xtal->getCurrentOptStep()));
      xtal->lock()->lockForWrite();
      xtal->addFailure();
      xtal->setStatus(Xtal::Error);
      xtal->lock()->unlock();
      return;
    }
  }

  void XtalOpt::startXtal() {
    //qDebug() << "XtalOpt::startXtal() called";
    QtConcurrent::run(this, &XtalOpt::startXtal_);
  }

  void XtalOpt::startXtal_() {
    //qDebug() << "XtalOpt::startXtal_() called";
    rwLock()->lockForWrite();
    Xtal *xtal = 0;
    if (!startPendingTracker->popFirstAndLock(xtal)) {
      rwLock()->unlock();
      startPendingTracker->unlock();
      return;
    }

    // Get info
    QString id, gen_s;
    QWriteLocker locker (xtal->lock());
    id.sprintf("%05d",xtal->getXtalNumber());
    gen_s.sprintf("%05d",xtal->getGeneration());
    xtal->setFileName(filePath + "/" + fileBase + gen_s + "x" + id + "/");
    xtal->setRempath(rempath + "/" + fileBase + gen_s + "x" + id + "/");
    xtal->setCurrentOptStep(1);

    // Write input files and create directory
    locker.unlock();
    Optimizer::writeInputFiles(xtal, this);

    // Append to opt
    appendStructure(xtal);

    // Actually start the job.
    runningTracker->removeXtal(xtal);
    submissionPendingTracker->addXtal(xtal);

    // Unlock mutexes
    startPendingTracker->unlock();
    rwLock()->unlock();
  }

  void XtalOpt::restartXtal() {
    //qDebug() << "XtalOpt::restartXtal() called";
    QtConcurrent::run(this, &XtalOpt::restartXtal_);
  }

  void XtalOpt::restartXtal_() {
    //qDebug() << "XtalOpt::restartXtal_() called";
    Xtal *xtal = 0;
    if (!restartPendingTracker->popFirst(xtal)) {
      return;
    }
    // Rewrite input files
    Optimizer::writeInputFiles(xtal, this);
    runningTracker->removeXtal(xtal);
    submissionPendingTracker->addXtal(xtal);
  }

  void XtalOpt::nextOptStep() {
    //qDebug() << "XtalOpt::nextOptStep() called";
    QtConcurrent::run(this, &XtalOpt::nextOptStep_);
  }

  void XtalOpt::nextOptStep_() {
    //qDebug() << "XtalOpt::nextOptStep_() called";
    Xtal* xtal;
    if (!nextOptStepPendingTracker->popFirstAndLock(xtal)) {
      nextOptStepPendingTracker->unlock();
      return;
    }
    QWriteLocker locker (xtal->lock());

    // update optstep and relaunch if necessary
    if (xtal->getCurrentOptStep() < static_cast<uint>(Optimizer::totalOptSteps(this))) {
      xtal->setCurrentOptStep(xtal->getCurrentOptStep() + 1);

      // Update input files
      locker.unlock();
      Optimizer::writeInputFiles(xtal, this);
      runningTracker->removeXtal(xtal);
      submissionPendingTracker->addXtal(xtal);
    }
    // Otherwise, it's done
    else {
      xtal->setStatus(Xtal::Optimized);
      runningTracker->removeXtal(xtal);
    }
    nextOptStepPendingTracker->unlock();
    updateInfo(xtal);
  }

  void XtalOpt::deleteJob(int ind) {
    //qDebug() << "XtalOpt::deleteJob( " << ind << " ) called";
    if (ind < 0 || ind > getStructures()->size()-1) return;
    QtConcurrent::run(&Optimizer::deleteJob, getStructures()->at(ind), this);
  }

  void XtalOpt::checkRunningJobs() {
    //qDebug() << "XtalOpt::checkRunningJobs() called";
    if (isStarting) {
      qDebug() << "Not checking jobs while session is starting";
      return;
    }
    if (runCheckPending) {
      qDebug() << "XtalOpt::checkRunningJobs: Ignoring extra call.";
      return;
    }
    runCheckPending = true;
    QtConcurrent::run(this, &XtalOpt::checkRunningJobs_);
  }

  void XtalOpt::checkRunningJobs_() {
    //qDebug() << "XtalOpt::checkRunningJobs_() called";

    if (runningTracker->count() == 0) {
      runCheckPending = false;
      return;
    }

    // Get list of running xtals
    QList<Xtal*> xtals = runningTracker->list();

    xtals.append(submissionPendingTracker->list());

    // prep variables
    Xtal *xtal = 0;
    QList<Xtal*> doneList;

    // Get queue data
    updateQueue();

    for (int i = 0; i < xtals.size(); i++) {
      xtal = xtals.at(i);

      // If the xtal is in one of the following trackers, don't update
      // (or lock) it until it's done:
      if (errorPendingTracker->hasXtal(xtal) ||
          restartPendingTracker->hasXtal(xtal) ||
          startPendingTracker->hasXtal(xtal) ||
          updatePendingTracker->hasXtal(xtal) ||
          nextOptStepPendingTracker->hasXtal(xtal) ) {
        qDebug() << tr("XtalOpt::checkRunningJobs_: Not checking xtal %1 until it's free of all pendingTrackers.")
          .arg(xtal->getIDString());
        continue;
      }

      int ind = getStructures()->indexOf(xtal);
      QReadLocker xtalLocker (xtal->lock());
      //qDebug() << tr("Checking xtal %1x%2").arg(xtal->getGeneration()).arg(xtal->getXtalNumber());

      // Check failure count
      if (xtal->getFailCount() > failLimit) {
        switch (FailActions(failAction)) {
        case FA_DoNothing:
        default:
          break;
        case FA_KillIt:
          xtalLocker.unlock();
          xtal->lock()->lockForWrite();
          xtal->setStatus(Xtal::Killed);
          xtal->lock()->unlock();
          xtalLocker.relock();
          break;
        case FA_Randomize:
          xtalLocker.unlock();
          QWriteLocker locker (xtal->lock());
          // End job if currently running
          if (xtal->getJobID()) {
            locker.unlock();
            Optimizer::deleteJob(xtal, this);
            locker.relock();
          }
          // Generate/Check new xtal
          Xtal *nxtal = 0;
          while (!checkXtal(nxtal))
            nxtal = generateRandomXtal(0, 0);
          // Copy info over
          QWriteLocker locker2 (nxtal->lock());
          xtal->setCellInfo(nxtal->OBUnitCell()->GetCellMatrix());
          xtal->setEnergy(0);
          xtal->setEnthalpy(0);
          xtal->setPV(0);
          xtal->setParents(tr("Randomly generated (excessive failures)"));
          // Copy atoms
          Atom *atom1, *atom2;
          for (uint i = 0; i < nxtal->numAtoms(); i++) {
            atom1 = xtal->atom(i);
            atom2 = nxtal->atom(i);
            atom1->setPos(atom2->pos());
            atom1->setAtomicNumber(atom2->atomicNumber());
          }
          // Reset member data
          xtal->findSpaceGroup();
          xtal->resetFailCount();
          xtal->setCurrentOptStep(1);
          xtal->setStatus(Xtal::WaitingForOptimization);
          // Delete random xtal
          nxtal->deleteLater();
          // Relock xtal
          locker.unlock();
          xtalLocker.relock();
          // Misc
          updateInfo(xtal);
          runningTracker->addXtal(xtal);
          break;
        }
      }

      // Check status
      switch (xtal->getStatus()) {
      case Xtal::InProcess: {
        xtalLocker.unlock();
        Optimizer::JobState state = Optimizer::getStatus(xtal, this);
        xtalLocker.relock();
        switch (state) {
        case Optimizer::Running:
        case Optimizer::Queued:
        case Optimizer::CommunicationError:
        case Optimizer::Unknown:
        case Optimizer::Pending:
        case Optimizer::Started:
          break;
        case Optimizer::Success:
          updatePendingTracker->addXtal(xtal);
          updateXtal();
          break;
        case Optimizer::Error:
          debug(tr("Structure %1 has failed to perform local optimization (JobID = %2)")
                .arg(xtal->getIDString())
                .arg(xtal->getJobID()));
          errorPendingTracker->addXtal(xtal);
          errorXtal(queueData);
          xtal->addFailure();
          break;
        }
        break;
      }
      case Xtal::Submitted: {
        xtalLocker.unlock();
        Optimizer::JobState substate = Optimizer::getStatus(xtal, this);
        xtalLocker.relock();
        switch (substate) {
        case Optimizer::Running:
        case Optimizer::Queued:
        case Optimizer::Error:
        case Optimizer::Success:
        case Optimizer::Started:
          xtal->setStatus(Xtal::InProcess);
          break;
        case Optimizer::CommunicationError:
        case Optimizer::Unknown:
        case Optimizer::Pending:
        default:
          break;
        }
        break;
      }
      case Xtal::Killed:
      case Xtal::Removed:
      case Xtal::Duplicate:
      case Xtal::Optimized:
        deleteJob(ind);
        doneList.append(xtal);
        break;
      case Xtal::StepOptimized:
        // Check to see if there are any more OptSteps to run on the xtal
        nextOptStepPendingTracker->addXtal(xtal);
        nextOptStep();
        break;
      case Xtal::Error:
        errorPendingTracker->addXtal(xtal);
        errorXtal(queueData);
        break;
      case Xtal::Restart:
        restartPendingTracker->addXtal(xtal);
        restartXtal();
        break;
      case Xtal::WaitingForOptimization:
      case Xtal::Updating:
      case Xtal::Empty:
        break;
      }

      updateInfo(xtal);
    }

    // Remove xtals that have finished
    for (int i = 0; i < doneList.size(); i++) {
      xtal = doneList.at(i);
      runningTracker->removeXtal(doneList.at(i));
    }

    runCheckPending = false;
  }

  void XtalOpt::resetDuplicates() {
    //qDebug() << "XtalOpt::resetDuplicates( ) called";
    if (isStarting) {
      qDebug() << "Not resetting duplicates while session is starting";
      return;
    }
    QtConcurrent::run(this, &XtalOpt::resetDuplicates_);
  }

  void XtalOpt::resetDuplicates_() {
    //qDebug() << "XtalOpt::resetDuplicates_( ) called";
    QList<Xtal*> *xtals = getStructures();
    Xtal *xtal = 0;
    for (int i = 0; i < xtals->size(); i++) {
      xtal = xtals->at(i);
      xtal->lock()->lockForWrite();
      if (xtal->getStatus() == Xtal::Duplicate)
        xtal->setStatus(Xtal::Optimized);
      xtal->lock()->unlock();
    }
    checkForDuplicates();
  }

  void XtalOpt::checkForDuplicates() {
    //qDebug() << "XtalOpt::checkForDuplicates( ) called";
    if (isStarting) {
      qDebug() << "Not checking for duplicates while session is starting";
      return;
    }
    QtConcurrent::run(this, &XtalOpt::checkForDuplicates_);
  }

  void XtalOpt::checkForDuplicates_() {
    //qDebug() << "XtalOpt::checkForDuplicates_( ) called";
    QList<Xtal*> *xtals = getStructures();

    QHash<QString, double> limits;
    limits.insert("enthalpy", tol_enthalpy);
    limits.insert("volume", tol_volume);

    QList<QString> keys = limits.keys();
    QList<QHash<QString, double> > fps;
    QList<Xtal::State> states;

    rwLock()->lockForRead();

    Xtal *xtal=0, *xtal_i=0, *xtal_j=0;
    for (int i = 0; i < xtals->size(); i++) {
      xtal = xtals->at(i);
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
        xtal_i = xtals->at(i);
        xtal_j = xtals->at(j);
        if (fp_i["enthalpy"] > fp_j["enthalpy"]) {
          xtal_i->lock()->lockForWrite();
          xtal_j->lock()->lockForRead();
          xtal_i->setStatus(Xtal::Duplicate);
          xtal_i->setDuplicateString(QString("%1x%2")
                                     .arg(xtal_j->getGeneration())
                                     .arg(xtal_j->getXtalNumber()));
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
                                     .arg(xtal_i->getXtalNumber()));
          xtal_j->lock()->unlock();
          xtal_i->lock()->unlock();
        }
      }
    }
    rwLock()->unlock();
    emit updateAllInfo();
  }

  void XtalOpt::addXtal(Xtal *xtal) {
    startPendingTracker->addXtal(xtal);
    emit xtalReadyToStart();
  }

  void XtalOpt::updateInfo(Xtal *xtal) {
    progressInfoUpdateTracker->addXtal(xtal);
    emit newInfoUpdate();
  }

  void XtalOpt::updateQueue(int time) {
    //qDebug() << "XtalOpt::updateQueue( " << time << " ) called";
    if (isStarting) {
      //qDebug() << "Not updating queue while session is starting";
      return;
    }
    int elapsed = queueTimeStamp.secsTo(QDateTime::currentDateTime());
    if (elapsed < 0 || time < elapsed) {
      //qDebug() << "XtalOpt::updateQueue: Checking queue... " << elapsed;
      if (!Optimizer::getQueueList(this, queueData)) {
        warning("Cannot fetch queue -- check stderr");
      }
      queueTimeStamp = QDateTime::currentDateTime();
      //qDebug() << "Queue:\n" << queueData;
    }
    else {
      //qDebug() << "XtalOpt::updateQueue: Skipping queue check... " << elapsed;
    }
  }

  void XtalOpt::warning(const QString & s) {
    qWarning() << "Warning: " << s;
    dialog->log("Warning: " + s);
  }

  void XtalOpt::debug(const QString & s) {
    qDebug() << "Debug: " << s;
    dialog->log("Debug: " + s);
  }

  void XtalOpt::error(const QString & s) {
    qWarning() << "Error: " << s;
    dialog->log("Error: " + s);
    dialog->errorBox(s);
  }

  void XtalOpt::printBackTrace() {
    backTraceMutex->lock();
    QStringList l = getBackTrace();
    backTraceMutex->unlock();
    for (int i = 0; i < l.size();i++)
      qDebug() << l.at(i);
  }

} // end namespace Avogadro

#include "xtalopt.moc"

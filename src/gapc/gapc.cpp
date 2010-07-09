/**********************************************************************
  GAPC -- A genetic algorithm for protected clusters

  Copyright (C) 2010 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <gapc/gapc.h>

#include <gapc/ui/dialog.h>
#include <gapc/structures/protectedcluster.h>

#include <globalsearch/structure.h>
#include <globalsearch/tracker.h>
#include <globalsearch/queuemanager.h>

#include <QDir>

#include <cstdlib>
#include <ctime>

namespace GAPC {

  OptGAPC::OptGAPC(GAPCDialog *parent) :
    OptBase(parent)
  {
    m_idString = "GAPC";
  }

  OptGAPC:: ~OptGAPC()
  {
  }

  Structure* OptGAPC::replaceWithRandom(Structure *s, const QString & reason)
  {
    ProtectedCluster *oldPC = qobject_cast<ProtectedCluster*>(s);
    QWriteLocker locker1 (oldPC->lock());

    // Generate/Check new cluster
    ProtectedCluster *PC = 0;
    while (!checkPC(PC)) {
      if (PC) delete PC;
      PC = generateRandomPC();
    }

    // Copy info over
    QWriteLocker locker2 (PC->lock());
    oldPC->resetEnergy();
    oldPC->resetEnthalpy();
    oldPC->setPV(0);
    oldPC->setCurrentOptStep(1);
    QString parents = "Randomly generated";
    if (!reason.isEmpty())
      parents += " (" + reason + ")";
    oldPC->setParents(parents);

    Atom *atom1, *atom2;
    for (uint i = 0; i < PC->numAtoms(); i++) {
      atom1 = oldPC->atom(i);
      atom2 = PC->atom(i);
      atom1->setPos(atom2->pos());
      atom1->setAtomicNumber(atom2->atomicNumber());
    }
    oldPC->resetFailCount();

    // TODO Perceive bonds?

    // Delete random PC
    PC->deleteLater();
    return qobject_cast<Structure*>(oldPC);
  }

  bool OptGAPC::checkLimits()
  {
    // Call error() and return false if there's a problem
    // TODO
  }

  bool OptGAPC::checkPC(ProtectedCluster *pc)
  {
    // TODO
  }

  bool OptGAPC::save(const QString & filename, bool notify)
  {
    // TODO
  }

  bool OptGAPC::load(const QString & filename)
  {
    // TODO
  }

  void OptGAPC::startSearch()
  {
    debug("Starting optimization.");
    emit startingSession();

    // Settings checks
    // Check lattice parameters, volume, etc
    if (!checkLimits()) {
      return;
    }

    // Do we have a composition?
    if (comp.core.isEmpty()) {
      error("Cannot create structures. Core composition is not set.");
      return;
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
    ProtectedCluster *pc = 0;
    // Use newPCCount in case "addXtal" falls behind so that we don't
    // duplicate structures when switching from seeds -> random.
    uint newPCCount=0;

    // Load seeds...
    for (int i = 0; i < seedList.size(); i++) {
      filename = seedList.at(i);
      pc = new ProtectedCluster;
      pc->setFileName(filename);
      if ( !m_optimizer->read(pc, filename) || (pc == 0) ) {
        m_tracker->deleteAllStructures();
        error(tr("Error loading seed %1").arg(filename));
        return;
      }
      QString parents = tr("Seeded: %1", "1 is a filename").arg(filename);
      initializeAndAddPC(pc, 1, parents);
      debug(tr("GAPC::StartOptimization: Loaded seed: %1", "1 is a filename").arg(filename));
      m_dialog->updateProgressLabel(tr("%1 structures generated (%2 kept, %3 rejected)...").arg(i + failed).arg(i).arg(failed));
      newPCCount++;
    }

    // Generation loop...
    for (uint i = newPCCount; i < numInitial; i++) {
      // Update progress bar
      m_dialog->updateProgressMaximum( (i == 0)
                                        ? 0
                                        : int(progCount / static_cast<double>(i)) * numInitial );
      m_dialog->updateProgressValue(progCount);
      progCount++;
      m_dialog->updateProgressLabel(tr("%1 structures generated (%2 kept, %3 rejected)...").arg(i + failed).arg(i).arg(failed));

      // Generate/Check xtal
      pc = generateRandomPC(1, i+1);
      if (!checkPC(pc)) {
        delete pc;
        i--;
        failed++;
      }
      else {
        initializeAndAddPC(pc, 1, pc->getParents());
        newPCCount++;
      }
    }

    m_dialog->stopProgressUpdate();

    m_dialog->saveSession();
    emit sessionStarted();
  }

  void OptGAPC::initializeAndAddPC(ProtectedCluster *pc,
                                   uint generation,
                                   const QString &parents) {
    initMutex.lock();
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

    QWriteLocker pcLocker (pc->lock());
    pc->setIDNumber(id);
    pc->setGeneration(generation);
    pc->setParents(parents);
    QString id_s, gen_s, locpath_s, rempath_s;
    id_s.sprintf("%05d",pc->getIDNumber());
    gen_s.sprintf("%05d",pc->getGeneration());
    locpath_s = filePath + "/" + gen_s + "x" + id_s + "/";
    rempath_s = rempath + "/" + gen_s + "x" + id_s + "/";
    QDir dir (locpath_s);
    if (!dir.exists()) {
      if (!dir.mkpath(locpath_s)) {
        error(tr("OptGAPC::initializeAndAddXtal: Cannot write to path: %1 (path creation failure)",
                 "1 is a file path.")
              .arg(locpath_s));
      }
    }
    pc->setFileName(locpath_s);
    pc->setRempath(rempath_s);
    pc->setCurrentOptStep(1);
    m_queue->unlockForNaming(pc);
    initMutex.unlock();
  }

  void OptGAPC::generateNewStructure()
  {
    // Get all optimized structures
    QList<Structure*> structures = m_queue->getAllOptimizedStructures();

    // Check to see if there are enough optimized structure to perform
    // genetic operations
    // TODO -- use genetic operators -- the following if is short circuited:
    //    if (structures.size() < 3) {
    if (true) {
      ProtectedCluster *pc = generateRandomPC(1, 0);
      initializeAndAddPC(pc, 1, pc->getParents());
      return;
    }
  }

  ProtectedCluster* OptGAPC::generateRandomPC(unsigned int gen, unsigned int id)
  {
    // Create cluster
    ProtectedCluster *pc = new ProtectedCluster();
    QWriteLocker locker (pc->lock());

    pc->setStatus(ProtectedCluster::Empty);

    // Populate cluster
    pc->constructRandomCluster(comp.core, minIAD, maxIAD);

    // Set up geneology info
    pc->setGeneration(gen);
    pc->setIDNumber(id);
    pc->setParents("Randomly generated");
    pc->setStatus(ProtectedCluster::WaitingForOptimization);

    return pc;
  }

  void OptGAPC::setOptimizer_string(const QString &s, const QString &filename)
  {
    // TODO
  }

}

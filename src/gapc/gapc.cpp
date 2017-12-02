/**********************************************************************
  GAPC -- A genetic algorithm for protected clusters

  Copyright (C) 2010-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <gapc/gapc.h>

#include <gapc/genetic.h>
#include <gapc/optimizers/adf.h>
#include <gapc/optimizers/gulp.h>
#include <gapc/structures/protectedcluster.h>
#include <gapc/ui/dialog.h>

#include <globalsearch/macros.h>
#include <globalsearch/queuemanager.h>
#include <globalsearch/slottedwaitcondition.h>
#include <globalsearch/structure.h>
#include <globalsearch/tracker.h>

#ifdef ENABLE_SSH
#include <globalsearch/queueinterfaces/remote.h>
#include <globalsearch/sshmanager.h>
#endif // ENABLE_SSH

#include <QDir>
#include <QtConcurrentMap>
#include <QtConcurrentRun>

#include <vector>

using namespace std;
using namespace Avogadro;
using namespace GlobalSearch;

namespace GAPC {

OptGAPC::OptGAPC(GAPCDialog* parent)
  : OptBase(parent), m_initWC(new SlottedWaitCondition(this))
{
  m_idString = "GAPC";
  m_schemaVersion = 2;

  connect(m_queue, SIGNAL(structureFinished(GlobalSearch::Structure*)), this,
          SLOT(checkForDuplicates()));
  connect(m_queue, SIGNAL(structureFinished(GlobalSearch::Structure*)), this,
          SLOT(checkOptimizedPC(GlobalSearch::Structure*)));
  connect(this, SIGNAL(sessionStarted()), this, SLOT(resetDuplicates()));
}

OptGAPC::~OptGAPC()
{
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
  while (savePending) {
    qDebug() << "Spinning on save before destroying GAPC...";
    GS_SLEEP(1);
  };
  savePending = true;

  // Clean up various members
  m_initWC->deleteLater();
  m_initWC = 0;
}

Structure* OptGAPC::replaceWithRandom(Structure* s, const QString& reason)
{
  ProtectedCluster* oldPC = qobject_cast<ProtectedCluster*>(s);
  QWriteLocker locker1(oldPC->lock());

  // Generate/Check new cluster
  ProtectedCluster* PC = 0;
  while (!checkPC(PC)) {
    if (PC) {
      delete PC;
      PC = 0;
    }

    PC = generateRandomPC();
  }

  // Copy info over
  QWriteLocker locker2(PC->lock());
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
  // Nothing to do here now -- limits cannot conflict.
  return true;
}

bool OptGAPC::checkPC(ProtectedCluster* pc)
{
  if (!pc)
    return false;

  QReadLocker locker(pc->lock());

  // Check that no two atoms are too close together
  double shortest = 0;
  if (pc->getShortestInteratomicDistance(shortest)) {
    if (shortest < minIAD) {
      qDebug() << "Discarding structure: IAD check failed: " << shortest
               << " < " << minIAD;
      return false;
    }
  }

  // Check that the cluster hasn't already exploded
  if (!pc->checkForExplosion(explodeLimit)) {
    qDebug() << "Discarding structure: Explosion detected!";
    return false;
  }

  return true;
}

void OptGAPC::checkOptimizedPC(Structure* s)
{
  if (!s)
    return;

  ProtectedCluster* pc = qobject_cast<ProtectedCluster*>(s);
  if (!pc)
    return;

  QReadLocker locker(pc->lock());

  // Explode check
  if (!pc->checkForExplosion(explodeLimit)) {
    qDebug() << "Cluster " << pc->getIDString() << " exploded!";
    switch (explodeAction) {
      case EA_Kill:
        locker.unlock();
        m_queue->killStructure(pc);
        return;
      case EA_Randomize:
        pc->setStatus(ProtectedCluster::Updating);
        locker.unlock();
        replaceWithRandom(pc, tr("Cluster exploded"));
        pc->lock()->lockForWrite();
        pc->setStatus(ProtectedCluster::Restart);
        pc->lock()->unlock();
        return;
    }
  }
}

bool OptGAPC::load(const QString& filename, const bool forceReadOnly)
{
  if (forceReadOnly) {
    readOnly = true;
  }

  // Attempt to open state file
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly)) {
    error("OptGAPC::load(): Error opening file " + file.fileName() +
          " for reading...");
    return false;
  }

  SETTINGS(filename);
  int loadedVersion =
    settings->value(m_idString.toLower() + "/version", 0).toInt();

  // Update config data. Be sure to bump m_schemaVersion in ctor if
  // adding updates.
  switch (loadedVersion) {
    case 0: // Initial version
    case 1: // Edit tab bumped to V2. No change here.
    case 2: // Current version
    default:
      break;
  }

  bool stateFileIsValid =
    settings->value(m_idString.toLower() + "/saveSuccessful", false).toBool();
  if (!stateFileIsValid) {
    error("OptGAPC::load(): File " + file.fileName() +
          " is incomplete, corrupt, or invalid. (Try " + file.fileName() +
          ".old if it exists)");
    return false;
  }

  // Get path and other info for later:
  QFileInfo stateInfo(file);
  // path to resume file
  QDir dataDir = stateInfo.absoluteDir();
  QString dataPath = dataDir.absolutePath() + "/";
  // list of structure dirs
  QStringList structureDirs =
    dataDir.entryList(QStringList(), QDir::AllDirs, QDir::Size);
  structureDirs.removeAll(".");
  structureDirs.removeAll("..");
  for (int i = 0; i < structureDirs.size(); i++) {
    if (!QFile::exists(dataPath + "/" + structureDirs.at(i) +
                       "/structure.state")) {
      structureDirs.removeAt(i);
      i--;
    }
  }

  // Set filePath:
  QString newFilePath = dataPath;
  QString newFileBase = filename;
  newFileBase.remove(newFilePath);
  newFileBase.remove(m_idString.toLower() + ".state.old");
  newFileBase.remove(m_idString.toLower() + ".state.tmp");
  newFileBase.remove(m_idString.toLower() + ".state");

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

  debug(tr("Resuming %1 session in '%2' (%3) readOnly = %4")
          .arg(m_idString)
          .arg(filename)
          .arg(m_optimizer->getIDString())
          .arg((readOnly) ? "true" : "false"));

  // Structures
  // Initialize progress bar:
  m_dialog->updateProgressMaximum(structureDirs.size());
  ProtectedCluster* pc;
  QList<uint> keys = comp.core.keys();
  QList<Structure*> loadedStructures;
  QString pcStateFileName;
  uint count = 0;
  int numDirs = structureDirs.size();
  for (int i = 0; i < numDirs; i++) {
    count++;
    m_dialog->updateProgressLabel(
      tr("Loading structures(%1 of %2)...").arg(count).arg(numDirs));
    m_dialog->updateProgressValue(count - 1);

    pcStateFileName = dataPath + "/" + structureDirs.at(i) + "/structure.state";
    debug(tr("Loading structure %1").arg(pcStateFileName));

    pc = new ProtectedCluster();
    QWriteLocker locker(pc->lock());
    // Add empty atoms to pc, updatePC will populate it
    for (int j = 0; j < keys.size(); j++) {
      for (uint k = 0; k < comp.core.value(keys.at(j)); k++)
        pc->addAtom();
    }
    pc->setFileName(dataPath + "/" + structureDirs.at(i) + "/");
    pc->readSettings(pcStateFileName);

    // Store current state -- updatePC will overwrite it.
    ProtectedCluster::State state = pc->getStatus();
    QDateTime endtime = pc->getOptTimerEnd();

    locker.unlock();

    if (!m_optimizer->load(pc)) {
      error(
        tr("Error, no (or not appropriate for %1) structural data in %2.\n\n\
This could be a result of resuming a structure that has not yet done any local \
optimizations. If so, safely ignore this message.")
          .arg(m_optimizer->getIDString())
          .arg(pc->fileName()));
      continue;
    }

    // Reset state
    locker.relock();
    pc->setStatus(state);
    pc->setOptTimerEnd(endtime);
    pc->generateDefaultHistogram();
    pc->enableAutoHistogramGeneration(true);
    locker.unlock();
    loadedStructures.append(qobject_cast<Structure*>(pc));
  }

  m_dialog->updateProgressMinimum(0);
  m_dialog->updateProgressValue(0);
  m_dialog->updateProgressMaximum(loadedStructures.size());
  m_dialog->updateProgressLabel("Sorting and checking structures...");

  // Sort structures by index values
  int curpos = 0;
  for (int i = 0; i < loadedStructures.size(); i++) {
    m_dialog->updateProgressValue(i);
    for (int j = 0; j < loadedStructures.size(); j++) {
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

  Structure* s = 0;
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

  return true;
}

void OptGAPC::startSearch()
{
  // Settings checks
  // Check lattice parameters, volume, etc
  if (!checkLimits()) {
    return;
  }

  // Do we have a composition?
  if (comp.core.isEmpty()) {
    error(tr("Cannot create structures. Core composition is not set."));
    return;
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
             "can begin (The option is on the 'Optimization Settings' tab)."));
  };

  // Warn user if contStructs is 0
  if (contStructs == 0) {
    error(tr("Warning: the number of continuous structures is "
             "currently set to 0."
             "\n\nYou will need to increase this value before the search "
             "can move past the first generation (The option is on the "
             "'Optimization Settings' tab)."));
  };

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
  uint progCount = 0;
  QString filename;
  ProtectedCluster* pc = 0;
  // Use newPCCount in case the tracker falls behind so that we
  // don't duplicate structures when switching from seeds -> random.
  uint newPCCount = 0;

  // Load seeds...
  for (int i = 0; i < seedList.size(); i++) {
    filename = seedList.at(i);
    pc = new ProtectedCluster;
    pc->setFileName(filename);
    if (!m_optimizer->read(pc, filename) || (pc == 0)) {
      m_tracker->lockForWrite();
      m_tracker->deleteAllStructures();
      m_tracker->unlock();
      error(tr("Error loading seed %1").arg(filename));
      return;
    }
    QString parents = tr("Seeded: %1", "1 is a filename").arg(filename);
    initializeAndAddPC(pc, 1, parents);
    debug(tr("GAPC::StartOptimization: Loaded seed: %1", "1 is a filename")
            .arg(filename));
    m_dialog->updateProgressLabel(
      tr("%1 structures generated (%2 kept, %3 rejected)...")
        .arg(i + failed)
        .arg(i)
        .arg(failed));
    newPCCount++;
  }

  // Generation loop...
  for (uint i = newPCCount; i < numInitial; i++) {
    // Update progress bar
    m_dialog->updateProgressMaximum(
      (i == 0) ? 0 : int(progCount / static_cast<double>(i)) * numInitial);
    m_dialog->updateProgressValue(progCount);
    progCount++;
    m_dialog->updateProgressLabel(
      tr("%1 structures generated (%2 kept, %3 rejected)...")
        .arg(i + failed)
        .arg(i)
        .arg(failed));

    // Generate/Check cluster
    pc = generateRandomPC(1, i + 1);
    if (!checkPC(pc)) {
      delete pc;
      i--;
      failed++;
    } else {
      initializeAndAddPC(pc, 1, pc->getParents());
      newPCCount++;
    }
  }

  // Wait for all structures to appear in tracker
  m_dialog->updateProgressLabel(tr("Waiting for structures to initialize..."));
  m_dialog->updateProgressMinimum(0);
  m_dialog->updateProgressMinimum(newPCCount);

  connect(m_tracker, SIGNAL(newStructureAdded(GlobalSearch::Structure*)),
          m_initWC, SLOT(wakeAllSlot()));

  m_initWC->prewaitLock();
  do {
    m_dialog->updateProgressValue(m_tracker->size());
    m_dialog->updateProgressLabel(
      tr("Waiting for structures to initialize (%1 of %2)...")
        .arg(m_tracker->size())
        .arg(newPCCount));
    // Don't block here forever -- there is a race condition where
    // the final newStructureAdded signal may be emitted while the
    // WC is not waiting. Since this is just trivial GUI updating
    // and we check the condition in the do-while loop, this is
    // acceptable. The following call will timeout in 250 ms.
    m_initWC->wait(250);
  } while (m_tracker->size() < newPCCount);
  m_initWC->postwaitUnlock();

  // We're done with m_initWC.
  m_initWC->disconnect();

  m_dialog->stopProgressUpdate();

  m_dialog->saveSession();
  emit sessionStarted();
}

void OptGAPC::initializeAndAddPC(ProtectedCluster* pc, uint generation,
                                 const QString& parents)
{
  initMutex.lock();
  QList<Structure*> allStructures = m_queue->lockForNaming();
  Structure* structure;
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

  QWriteLocker pcLocker(pc->lock());
  pc->moveToThread(m_queueThread);
  pc->setIDNumber(id);
  pc->setGeneration(generation);
  pc->setParents(parents);
  QString id_s, gen_s, locpath_s, rempath_s;
  id_s.sprintf("%05d", pc->getIDNumber());
  gen_s.sprintf("%05d", pc->getGeneration());
  locpath_s = filePath + "/" + gen_s + "x" + id_s + "/";
  rempath_s = rempath + "/" + gen_s + "x" + id_s + "/";
  QDir dir(locpath_s);
  if (!dir.exists()) {
    if (!dir.mkpath(locpath_s)) {
      error(tr("OptGAPC::initializeAndAddPC: Cannot write to path: %1 (path "
               "creation failure)",
               "1 is a file path.")
              .arg(locpath_s));
    }
  }
  pc->setFileName(locpath_s);
  pc->setRempath(rempath_s);
  pc->setCurrentOptStep(1);
  pc->setupConnections();
  pc->enableAutoHistogramGeneration(true);
  pc->update();
  pcLocker.unlock();
  m_queue->unlockForNaming(pc);
  initMutex.unlock();
}

void OptGAPC::generateNewStructure()
{
  QtConcurrent::run(this, &OptGAPC::generateNewStructure_);
}

void OptGAPC::generateNewStructure_()
{
  INIT_RANDOM_GENERATOR();
  // Get all optimized structures
  QList<Structure*> structures = m_queue->getAllOptimizedStructures();

  // Check to see if there are enough optimized structure to perform
  // genetic operations
  if (structures.size() < 3) {
    ProtectedCluster* pc = generateRandomPC(1, 0);
    initializeAndAddPC(pc, 1, pc->getParents());
    return;
  }

  // return pc
  ProtectedCluster* pc = 0;

  // temporary use pc
  ProtectedCluster* tpc;

  // Trim and sort list
  Structure::sortByEnthalpy(&structures);
  // Remove all but (n_consider + 1). The "+ 1" will be removed
  // during probability generation.
  while (static_cast<uint>(structures.size()) > popSize + 1)
    structures.removeLast();

  // Make list of weighted probabilities based on enthalpy values
  QList<double> probs = getProbabilityList(structures);

  // Convert stuctures to pcs
  QList<ProtectedCluster*> pcs;
  for (int i = 0; i < structures.size(); i++)
    pcs.append(qobject_cast<ProtectedCluster*>(structures.at(i)));

  // Initialize loop vars
  double r;
  unsigned int gen;
  QString parents;

  // Perform operation until pc is valid:
  while (!checkPC(pc)) {
    // First delete any previous failed structure in pc
    if (pc) {
      delete pc;
      pc = 0;
    }

    // Decide operator:
    r = RANDDOUBLE() * 100.0;
    Operators op;
    if (r < p_cross)
      op = OP_Crossover;
    else if (r < p_cross + p_twist)
      op = OP_Twist;
    else if (r < p_cross + p_twist + p_exch)
      op = OP_Exchange;
    else if (r < p_cross + p_twist + p_exch + p_randw)
      op = OP_RandomWalk;
    else
      op = OP_AnisotropicExpansion;

    // Try 1000 times to get a good structure from the selected
    // operation. If not possible, send a warning to the log and
    // start anew.
    int attemptCount = 0;
    while (attemptCount < 1000 && !checkPC(pc)) {
      attemptCount++;
      if (pc) {
        delete pc;
        pc = 0;
      }

      // Operation specific set up:
      switch (op) {
        case OP_Crossover: {
          int ind1, ind2;
          ProtectedCluster *pc1 = 0, *pc2 = 0;
          // Select structures
          ind1 = ind2 = 0;
          double r1 = RANDDOUBLE();
          double r2 = RANDDOUBLE();
          for (ind1 = 0; ind1 < probs.size(); ind1++)
            if (r1 < probs.at(ind1))
              break;
          for (ind2 = 0; ind2 < probs.size(); ind2++)
            if (r2 < probs.at(ind2))
              break;

          pc1 = pcs.at(ind1);
          pc2 = pcs.at(ind2);

          // Perform operation
          pc = GAPCGenetic::crossover(pc1, pc2);

          // Lock parents and get info from them
          pc1->lock()->lockForRead();
          pc2->lock()->lockForRead();
          unsigned int gen1 = pc1->getGeneration();
          unsigned int gen2 = pc2->getGeneration();
          unsigned int id1 = pc1->getIDNumber();
          unsigned int id2 = pc2->getIDNumber();
          pc2->lock()->unlock();
          pc1->lock()->unlock();

          // Determine generation number
          gen = (gen1 >= gen2) ? gen1 + 1 : gen2 + 1;
          parents = tr("Crossover: %1x%2 + %4x%5")
                      .arg(gen1)
                      .arg(id1)
                      .arg(gen2)
                      .arg(id2);
          continue;
        }

        case OP_Twist: {
          int ind = 0;
          ProtectedCluster* pc1 = 0;
          // Select structures
          double r = RANDDOUBLE();
          for (ind = 0; ind < probs.size(); ind++)
            if (r < probs.at(ind))
              break;

          pc1 = pcs.at(ind);

          // Perform operation
          double rotation;
          pc = GAPCGenetic::twist(pc1, twist_minRot, rotation);

          // Lock parents and get info from them
          pc1->lock()->lockForRead();
          unsigned int gen1 = pc1->getGeneration();
          unsigned int id1 = pc1->getIDNumber();
          pc1->lock()->unlock();

          // Determine generation number
          gen = gen1 + 1;
          parents =
            tr("Twist: %1x%2 (%3 deg)").arg(gen1).arg(id1).arg(rotation);
          continue;
        }

        case OP_Exchange: {
          int ind = 0;
          ProtectedCluster* pc1 = 0;
          // Select structures
          double r = RANDDOUBLE();
          for (ind = 0; ind < probs.size(); ind++)
            if (r < probs.at(ind))
              break;

          pc1 = pcs.at(ind);

          // Perform operation
          pc = GAPCGenetic::exchange(pc1, exch_numExch);

          // Lock parents and get info from them
          pc1->lock()->lockForRead();
          unsigned int gen1 = pc1->getGeneration();
          unsigned int id1 = pc1->getIDNumber();
          pc1->lock()->unlock();

          // Determine generation number
          gen = gen1 + 1;
          parents = tr("Exchange: %1x%2 (%3 swaps)")
                      .arg(gen1)
                      .arg(id1)
                      .arg(exch_numExch);
          continue;
        }

        case OP_RandomWalk: {
          int ind = 0;
          ProtectedCluster* pc1 = 0;
          // Select structures
          double r = RANDDOUBLE();
          for (ind = 0; ind < probs.size(); ind++)
            if (r < probs.at(ind))
              break;

          pc1 = pcs.at(ind);

          // Perform operation
          pc = GAPCGenetic::randomWalk(pc1, randw_numWalkers, randw_minWalk,
                                       randw_maxWalk);

          // Lock parents and get info from them
          pc1->lock()->lockForRead();
          unsigned int gen1 = pc1->getGeneration();
          unsigned int id1 = pc1->getIDNumber();
          pc1->lock()->unlock();

          // Determine generation number
          gen = gen1 + 1;
          parents = tr("RandomWalk: %1x%2 (%3 walkers, %4-%5)")
                      .arg(gen1)
                      .arg(id1)
                      .arg(randw_numWalkers)
                      .arg(randw_minWalk)
                      .arg(randw_maxWalk);
          continue;
        }

        case OP_AnisotropicExpansion: {
          int ind = 0;
          ProtectedCluster* pc1 = 0;
          // Select structures
          double r = RANDDOUBLE();
          for (ind = 0; ind < probs.size(); ind++)
            if (r < probs.at(ind))
              break;

          pc1 = pcs.at(ind);

          // Perform operation
          pc = GAPCGenetic::anisotropicExpansion(pc1, aniso_amp);

          // Lock parents and get info from them
          pc1->lock()->lockForRead();
          unsigned int gen1 = pc1->getGeneration();
          unsigned int id1 = pc1->getIDNumber();
          pc1->lock()->unlock();

          // Determine generation number
          gen = gen1 + 1;
          parents =
            tr("AnisoExp: %1x%2 (%3 amp)").arg(gen1).arg(id1).arg(aniso_amp);
          continue;
        }

      } // end switch
    }
    if (attemptCount >= 1000) {
      QString opStr;
      switch (op) {
        case OP_Crossover:
          opStr = "crossover";
          break;
        case OP_Twist:
          opStr = "twist";
          break;
        case OP_Exchange:
          opStr = "exchange";
          break;
        case OP_RandomWalk:
          opStr = "random walk";
          break;
        case OP_AnisotropicExpansion:
          opStr = "anisotropic expansion";
          break;
        default:
          opStr = "(unknown)";
          break;
      }
      warning(tr("Unable to perform operation %1 after 1000 tries. Reselecting "
                 "operator...")
                .arg(opStr));
    }
  }
  initializeAndAddPC(pc, gen, parents);
  return;
}

ProtectedCluster* OptGAPC::generateRandomPC(unsigned int gen, unsigned int id)
{
  // Create cluster
  ProtectedCluster* pc = new ProtectedCluster();
  QWriteLocker locker(pc->lock());

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

void OptGAPC::resetDuplicates()
{
  if (isStarting) {
    return;
  }
  QtConcurrent::run(this, &OptGAPC::resetDuplicates_);
}

void OptGAPC::resetDuplicates_()
{
  QList<Structure*>* structures = m_tracker->list();
  ProtectedCluster* pc;
  for (int i = 0; i < structures->size(); i++) {
    pc = qobject_cast<ProtectedCluster*>(structures->at(i));
    pc->lock()->lockForWrite();
    pc->setChangedSinceDupChecked(true);
    if (pc->getStatus() == ProtectedCluster::Duplicate)
      pc->setStatus(ProtectedCluster::Optimized);
    pc->lock()->unlock();
  }
  checkForDuplicates();
}

void OptGAPC::checkForDuplicates()
{
  if (isStarting) {
    return;
  }
  QtConcurrent::run(this, &OptGAPC::checkForDuplicates_);
}

// Helper function for QtConcurrent::blockingMapped below
QHash<QString, QVariant> getFingerprint(Structure* s)
{
  return s->getFingerprint();
}

// Helper function/struct for QtConcurrent::blockingMap below
struct checkForDupsStruct
{
  unsigned int i, j, numAtoms;
  double scale;
  QList<QHash<QString, QVariant>>* fps;
  Structure *s_i, *s_j;
  vector<double>* dist;
  QList<vector<double>>* freqs;
  OptGAPC* opt;
};

void checkIfDuplicates(checkForDupsStruct& st)
{
  QHash<QString, QVariant> *fp_i = 0, *fp_j = 0;
  vector<double> *freq_i, *freq_j;

  fp_i = &((*st.fps)[st.i]);
  fp_j = &((*st.fps)[st.j]);
  freq_i = &((*st.freqs)[st.i]);
  freq_j = &((*st.freqs)[st.j]);

  double error = 0;
  if (!ProtectedCluster::compareIADDistributions((*st.dist), (*freq_i),
                                                 (*freq_j), 0, 0.1, &error)) {
    st.opt->warning("Geometric fingerprint comparison failed. Aborting...");
    return;
  }
  error /= st.scale;
  // qDebug() << error;
  if (error >= st.opt->tol_geo)
    return;
  if (fabs(fp_i->value("enthalpy").toDouble() -
           fp_j->value("enthalpy").toDouble()) /
        st.scale >=
      st.opt->tol_enthalpy)
    return;
  // If we get here, all the fingerprint values match,
  // and we have a duplicate. Mark the xtal with the
  // highest enthalpy as a duplicate of the other.
  if (fp_i->value("enthalpy").toDouble() > fp_j->value("enthalpy").toDouble()) {
    st.s_i->lock()->lockForWrite();
    st.s_j->lock()->lockForRead();
    st.s_i->setStatus(Structure::Duplicate);
    st.s_i->setDuplicateString(QString("%1x%2 (%3)")
                                 .arg(st.s_j->getGeneration())
                                 .arg(st.s_j->getIDNumber())
                                 .arg(error, 5, 'g'));
    st.s_i->lock()->unlock();
    st.s_j->lock()->unlock();
  } else {
    st.s_j->lock()->lockForWrite();
    st.s_i->lock()->lockForRead();
    st.s_j->setStatus(Structure::Duplicate);
    st.s_j->setDuplicateString(QString("%1x%2 (%3)")
                                 .arg(st.s_i->getGeneration())
                                 .arg(st.s_i->getIDNumber())
                                 .arg(error, 5, 'g'));
    st.s_j->lock()->unlock();
    st.s_i->lock()->unlock();
  }
}

long factorial(long a)
{
  if (a > 1)
    return (a * factorial(a - 1));
  else
    return (1);
}

void OptGAPC::checkForDuplicates_()
{
  QTime alltimer = QTime::currentTime();
  m_tracker->lockForRead();
  QList<Structure*>* structures = m_tracker->list();

  if (structures->size() == 0)
    return;
  // getFingerprint is defined above
  QTime gentimer = QTime::currentTime();
  QList<QHash<QString, QVariant>> fps =
    QtConcurrent::blockingMapped((*structures), getFingerprint);
  double gentime = gentimer.msecsTo(QTime::currentTime()) / (double)1000;

  m_tracker->unlock();

  QVariantList distv = fps.first().value("IADDist").toList();
  vector<double> dist;
  QList<vector<double>> freqs;
  QVariantList freqv;
  vector<double>* freq;

  QTime convtimer = QTime::currentTime();
  // Convert QVariant lists to doubles
  dist.reserve(distv.size());
  for (int i = 0; i < distv.size(); i++) {
    dist.push_back(distv.at(i).toDouble());
  }
  for (int i = 0; i < fps.size(); i++) {
    freqv = fps.at(i).value("IADFreq").toList();
    freqs.append(vector<double>());
    freq = &(freqs[i]);
    freq->reserve(freqv.size());
    for (int i = 0; i < freqv.size(); i++) {
      freq->push_back(freqv.at(i).toDouble());
    }
  }

  double convtime = convtimer.msecsTo(QTime::currentTime()) / (double)1000;

  QTime comptimer = QTime::currentTime();
  // compute tol scaling factor (number of atoms)
  double scale = 1;
  if (structures->size() > 0 && structures->first()->numAtoms() != 0) {
    scale = structures->first()->numAtoms();
  }
  // Create helper structs
  QList<checkForDupsStruct> sts;
  Structure *s_i, *s_j;
  for (int i = 0; i < structures->size() - 1; i++) {
    s_i = structures->at(i);
    if (s_i->getStatus() != Structure::Optimized)
      continue;
    for (int j = i + 1; j < structures->size(); j++) {
      s_j = structures->at(j);
      if (s_j->getStatus() != Structure::Optimized)
        continue;
      if (s_i->hasChangedSinceDupChecked() ||
          s_j->hasChangedSinceDupChecked()) {
        checkForDupsStruct st;
        st.i = i;
        st.j = j;
        st.fps = &fps;
        st.s_i = s_i;
        st.s_j = s_j;
        st.scale = scale;
        st.dist = &dist;
        st.freqs = &freqs;
        st.opt = this;
        sts.append(st);
      }
    }
    s_i->setChangedSinceDupChecked(false);
  }
  QtConcurrent::blockingMap(sts, checkIfDuplicates);
  double comptime = comptimer.msecsTo(QTime::currentTime()) / (double)1000;
  double alltime = alltimer.msecsTo(QTime::currentTime()) / (double)1000;
  qDebug() << QString("Fingerprint timings: %1 structs | %2 (gen) + %3 (conv) "
                      "+ %4 (comp) = %5 (tot). %6 comps")
                .arg(fps.size())
                .arg(gentime, 5, 'g')
                .arg(convtime, 5, 'g')
                .arg(comptime, 5, 'g')
                .arg(alltime, 5, 'g')
                .arg(sts.size());

  emit refreshAllStructureInfo();
}
}

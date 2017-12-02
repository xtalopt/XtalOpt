/**********************************************************************
  RandomDock

  Copyright (C) 2009-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <randomdock/randomdock.h>

#include <randomdock/optimizers/gamess.h>
#include <randomdock/structures/matrix.h>
#include <randomdock/structures/scene.h>
#include <randomdock/structures/substrate.h>
#include <randomdock/ui/dialog.h>

#include <globalsearch/optbase.h>
#include <globalsearch/queueinterfaces/remote.h>
#include <globalsearch/queuemanager.h>
#include <globalsearch/slottedwaitcondition.h>
#include <globalsearch/tracker.h>
#ifdef ENABLE_SSH
#include <globalsearch/sshmanager.h>
#endif // ENABLE_SSH
#include <globalsearch/macros.h>

#include <avogadro/atom.h>
#include <avogadro/bond.h>

#include <openbabel/mol.h>

#include <QDir>
#include <QFile>
#include <QStringList>
#include <QThread>

using namespace std;
using namespace Avogadro;
using namespace GlobalSearch;

namespace RandomDock {

RandomDock::RandomDock(RandomDockDialog* parent)
  : OptBase(parent), substrate(0), m_initWC(new SlottedWaitCondition(this)),
    strictHBonds(false)
{
  m_idString = "RandomDock";
  m_schemaVersion = 2;
  sceneInitMutex = new QMutex;
  limitRunningJobs = true;
  // By default, just replace with random when a scene fails.
  failLimit = 1;
  failAction = FA_Randomize;
}

RandomDock::~RandomDock()
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
    qDebug() << "Spinning on save before destroying RandomDock...";
    GS_SLEEP(1);
  };
  savePending = true;

  // Clean up various members
  m_initWC->deleteLater();
  m_initWC = 0;
}

void RandomDock::startSearch()
{
  // Check that everything is in place
  if (!substrate) {
    error("Cannot begin search without specifying substrate.");
    setIsStartingFalse();
    return;
  }
  if (matrixList.size() == 0) {
    error("Cannot begin search without specifying matrix molecules.");
    setIsStartingFalse();
    return;
  }
  if (!checkLimits()) {
    setIsStartingFalse();
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

  ////////////////////////////
  // Generate random scenes //
  ////////////////////////////

  // Set up progress bar
  m_dialog->startProgressUpdate(tr("Generating structures..."), 0, 0);

  // Initalize loop variables
  Scene* scene;
  int progCount = 0;

  // Generation loop...
  for (uint i = 0; i < runningJobLimit; i++) {
    m_dialog->updateProgressMaximum(
      (i == 0) ? 0 : int(progCount / static_cast<double>(i)) * runningJobLimit);
    m_dialog->updateProgressValue(progCount++);
    m_dialog->updateProgressLabel(
      tr("%1 scenes generated of (%2)...").arg(i).arg(runningJobLimit));

    // Generate/Check molecule
    scene = generateRandomScene();
    initializeAndAddScene(scene);
  }

  // Wait for all structures to appear in tracker
  m_dialog->updateProgressLabel(tr("Waiting for structures to initialize..."));
  m_dialog->updateProgressMinimum(0);
  m_dialog->updateProgressMinimum(runningJobLimit);

  connect(m_tracker, SIGNAL(newStructureAdded(GlobalSearch::Structure*)),
          m_initWC, SLOT(wakeAllSlot()));

  m_initWC->prewaitLock();
  do {
    m_dialog->updateProgressValue(m_tracker->size());
    m_dialog->updateProgressLabel(
      tr("Waiting for structures to initialize (%1 of %2)...")
        .arg(m_tracker->size())
        .arg(runningJobLimit));
    // Don't block here forever -- there is a race condition where
    // the final newStructureAdded signal may be emitted while the
    // WC is not waiting. Since this is just trivial GUI updating
    // and we check the condition in the do-while loop, this is
    // acceptable. The following call will timeout in 250 ms.
    m_initWC->wait(250);
  } while (m_tracker->size() < runningJobLimit);
  m_initWC->postwaitUnlock();

  // We're done with m_initWC.
  m_initWC->disconnect();

  m_dialog->stopProgressUpdate();

  emit sessionStarted();
  m_dialog->saveSession();
}

static inline bool isHBondAcceptor(int atomicNum)
{
  if (atomicNum == 9 || // Flourine
      atomicNum == 8 || // Oxygen
      atomicNum == 7)   // Nitrogen
    return true;
  return false;
}

static inline bool isHBond(Atom* satom, int an, QList<int> nbrsANs)
{
  // Check that either atom is a hydrogen bondable H
  if (satom->isHydrogen() && isHBondAcceptor(an)) {
    QList<unsigned long> snbrs = satom->neighbors();
    for (int i = 0; i < snbrs.size(); ++i) {
      if (isHBondAcceptor(
            satom->molecule()->atomById(snbrs[i])->atomicNumber())) {
        return true;
      }
    }
  }
  if (an == 1 && isHBondAcceptor(satom->atomicNumber())) {
    for (int i = 0; i < nbrsANs.size(); ++i) {
      if (isHBondAcceptor(nbrsANs[i])) {
        return true;
      }
    }
  }
  return false;
}

Scene* RandomDock::generateRandomScene()
{
  INIT_RANDOM_GENERATOR();
  // Here we build a scene by extracting coordinates of the atoms
  // from a random conformer of a substrate and the specified number
  // of matrix molecules. The coordinates are rotated, translated,
  // and checked before populating the new scene.

  // Initialize vars
  Atom* atom;
  Bond* newbond;
  Bond* oldbond;
  Matrix* mat;
  Scene* scene = new Scene;
  QWriteLocker sceneLocker(scene->lock());
  QHash<ulong, ulong> idMap; // Old id, new id
  QList<Atom*> atomList;
  QList<Eigen::Vector3d> positions;
  QList<int> atomicNums;

  // Select random conformer of substrate
  substrate->lock()->lockForWrite(); // Write lock prevents
                                     // conformer from changing
  int conformer = substrate->getRandomConformerIndex();
  substrate->setConformer(conformer);

  // Extract information from substrate
  atomList = substrate->atoms();
  for (int j = 0; j < atomList.size(); j++) {
    atomicNums.append(atomList.at(j)->atomicNumber());
    positions.append(*(atomList.at(j)->pos()));
  }

  substrate->lock()->unlock();

  // Place substrate's geometric center at origin
  RandomDock::centerCoordinatesAtOrigin(positions);

  // Add atoms to the scene
  for (int i = 0; i < positions.size(); i++) {
    atom = scene->addAtom();
    idMap.insert(atomList.at(i)->id(), atom->id());
    atom->setAtomicNumber(atomicNums.at(i));
    atom->setPos(positions.at(i));
  }

  // Attach bonds
  for (uint i = 0; i < substrate->numBonds(); i++) {
    newbond = scene->addBond();
    oldbond = substrate->bonds().at(i);
    newbond->setAtoms(idMap[oldbond->beginAtomId()],
                      idMap[oldbond->endAtomId()], oldbond->order());
    newbond->setAromaticity(oldbond->isAromatic());
  }

  // Get matrix elements
  // Generate probability list
  QList<double> probs;
  double total = 0; // leave as double for division below

  // Probability based on stoichiometry
  for (int i = 0; i < matrixStoich.size(); i++)
    total += matrixStoich.at(i);
  for (int i = 0; i < matrixStoich.size(); i++) {
    if (i == 0)
      probs.append(matrixStoich.at(0) / total);
    else
      probs.append(matrixStoich.at(i) / total + probs.at(i - 1));
  }

  // Pick and add matrix elements
  QList<QList<int>> neighborAtomicNums;
  for (uint i = 0; i < numMatrixMol; i++) {
    // Add random conformers of matrix molecule in random locations,
    // orientations
    double r = RANDDOUBLE();
    int ind;
    for (ind = 0; ind < probs.size(); ind++)
      if (r < probs.at(ind))
        break;

    mat = matrixList.at(ind);

    atomicNums.clear();
    neighborAtomicNums.clear();
    positions.clear();
    idMap.clear();

    mat->lock()->lockForWrite(); // Write lock prevents conformer
                                 // from changing
    conformer = mat->getRandomConformerIndex();
    mat->setConformer(conformer);

    // Extract information from matrix
    atomList = mat->atoms();
    for (int j = 0; j < atomList.size(); j++) {
      atomicNums.append(atomList.at(j)->atomicNumber());
      positions.append(*(atomList.at(j)->pos()));
      QList<unsigned long> nbrs = atomList.at(j)->neighbors();
      neighborAtomicNums.append(QList<int>());
      for (int ni = 0; ni < nbrs.size(); ++ni)
        neighborAtomicNums.last().append(
          mat->atomById(nbrs[ni])->atomicNumber());
    }

    mat->lock()->unlock();

    // Calculate radii if we're in cluster mode, otherwise use user
    // specified limits
    double r_min;
    double r_max;
    if (cluster_mode) {
      double sceneRadius = scene->radius();
      double substrateRadius = substrate->radius();
      // Find shortest and longest matrix radii
      double mat_short;
      double mat_long;
      mat_short = mat_long = matrixList.first()->radius();
      for (int m = 0; m < matrixList.size(); m++) {
        Matrix* mat = matrixList.at(m);
        mat->lock()->lockForWrite();
        for (uint i = 0; i < mat->numConformers(); i++) {
          mat->setConformer(i);
          mat->updateMolecule();
          double tmp = mat->radius();
          if (tmp < mat_short)
            mat_short = tmp;
          if (tmp > mat_long)
            mat_long = tmp;
        }
        mat->lock()->unlock();
      }
      r_min = (mat_short < substrateRadius) ? mat_short : substrateRadius;
      r_max = mat_long + substrateRadius + IAD_max;
    } else {
      r_min = radius_min;
      r_max = radius_max;
    }

    // Make A 2D network
    if (build2DNetwork) {
      RandomDock::DRotateCoordinates(positions);
      RandomDock::DDisplaceCoordinates(positions, r_min, r_max);
    } else {
      // Rotate, translate positions
      RandomDock::randomlyRotateCoordinates(positions);
      RandomDock::randomlyDisplaceCoordinates(positions, r_min, r_max);
    }

    // Check interatomic distances
    bool ok = true;
    double shortest, distance;
    shortest = -1;
    for (uint mi = 0; mi < mat->numAtoms(); mi++) {
      for (uint si = 0; si < scene->numAtoms(); si++) {
        distance =
          abs((*(scene->atoms().at(si)->pos()) - positions.at(mi)).norm());
        // Go ahead and bail if the atoms are too close
        if (distance < IAD_min) {
          ok = false;
          break;
        }
        // Screen for H bonds -- function is static inline above
        if (this->strictHBonds &&
            !isHBond(scene->atoms().at(si), atomicNums.at(mi),
                     neighborAtomicNums.at(mi))) {
          continue;
        }
        // Check distances
        if (shortest < 0)
          shortest = distance; // initialize...
        else if (distance < shortest)
          shortest = distance; // update
      }
      if (!ok)
        break;
    }
    if (!ok || shortest > IAD_max || shortest < IAD_min) {
      qDebug() << "Bad IAD: " << shortest;
      i--;
      continue;
    }

    // If IAD checks out, add the atoms to the scene
    for (int j = 0; j < positions.size(); j++) {
      atom = scene->addAtom();
      idMap.insert(atomList.at(j)->id(), atom->id());
      atom->setAtomicNumber(atomicNums.at(j));
      atom->setPos(positions.at(j));
    }

    // Attach bonds
    for (uint j = 0; j < mat->numBonds(); j++) {
      newbond = scene->addBond();
      oldbond = mat->bonds().at(j);
      newbond->setAtoms(idMap[oldbond->beginAtomId()],
                        idMap[oldbond->endAtomId()], oldbond->order());
      newbond->setAromaticity(oldbond->isAromatic());
    }

  } // end for i in numMatrixMol
  return scene;
}

Structure* RandomDock::replaceWithRandom(Structure* s, const QString& reason)
{
  Scene* oldScene = qobject_cast<Scene*>(s);
  QWriteLocker locker1(oldScene->lock());

  // Generate/Check new scene
  Scene* scene = 0;
  while (!checkScene(scene)) {
    if (scene) {
      delete scene;
      scene = 0;
    }

    scene = generateRandomScene();
  }

  // Copy info over
  QWriteLocker locker2(scene->lock());
  oldScene->copyStructure(*scene);
  oldScene->resetEnergy();
  oldScene->resetEnthalpy();
  oldScene->setPV(0);
  oldScene->setCurrentOptStep(1);
  QString parents = "Randomly generated";
  if (!reason.isEmpty())
    parents += " (" + reason + ")";
  oldScene->setParents(parents);
  oldScene->resetFailCount();

  // Delete random scene
  scene->deleteLater();
  return qobject_cast<Structure*>(oldScene);
}

void RandomDock::initializeAndAddScene(Scene* scene)
{
  // Initialize vars
  QString id_s;
  QString locpath_s;
  QString rempath_s;

  // So as to not assign duplicate ids, ensure only one assignment
  // is made at a time
  sceneInitMutex->lock();

  // lockForNaming returns a list of all structures, both accepted
  // and pending, so it's size+1 is the id of the new structure.
  int id = m_queue->lockForNaming().size() + 1;

  // Generate locations using id number
  id_s.sprintf("%05d", id);
  locpath_s = filePath + "/" + id_s + "/";
  rempath_s = rempath + "/" + id_s + "/";

  // Create path
  QDir dir(locpath_s);
  if (!dir.exists()) {
    if (!dir.mkpath(locpath_s)) {
      error(tr("RandomDock::initializeAndAddScene: Cannot write to path: %1 "
               "(path creation failure)",
               "1 is a file path.")
              .arg(locpath_s));
    }
  }

  // Assign data to scene
  scene->lock()->lockForWrite();
  scene->moveToThread(m_queueThread);
  scene->setIDNumber(id);
  scene->setIndex(id - 1);
  scene->setFileName(locpath_s);
  scene->setRempath(rempath_s);
  scene->setCurrentOptStep(1);
  scene->setStatus(Structure::WaitingForOptimization);
  scene->lock()->unlock();

  // unlockForNaming will append the scene
  m_queue->unlockForNaming(scene);

  // Done!
  sceneInitMutex->unlock();
}

void RandomDock::generateNewStructure()
{
  initializeAndAddScene(generateRandomScene());
}

bool RandomDock::checkLimits()
{
  // TODO Are there any input parameters that need to be verified?
  return true;
}

bool RandomDock::checkScene(Scene* scene)
{
  // Check if null
  if (!scene)
    return false;

  return true;
}

void RandomDock::sortAndRankByEnergy(QList<Scene*>* scenes)
{
  uint numStructs = scenes->size();

  // Simple selection sort
  Scene *scene_i, *scene_j, *tmp;
  for (uint i = 0; i < numStructs; i++) {
    scene_i = scenes->at(i);
    scene_i->lock()->lockForRead();
    for (uint j = i + 1; j < numStructs; j++) {
      scene_j = scenes->at(j);
      scene_j->lock()->lockForRead();
      if (scene_j->getEnergy() < scene_i->getEnergy()) {
        scenes->swap(i, j);
        tmp = scene_i;
        scene_i = scene_j;
        scene_j = tmp;
      }
      scene_j->lock()->unlock();
    }
    scene_i->lock()->unlock();
  }

  for (uint i = 0; i < numStructs; i++) {
    scene_i = scenes->at(i);
    scene_i->lock()->lockForWrite();
    scene_i->setRank(i + 1);
    scene_i->lock()->unlock();
  }
}

void RandomDock::centerCoordinatesAtOrigin(QList<Eigen::Vector3d>& coords)
{
  // Find center of coordinates:
  Eigen::Vector3d center(0, 0, 0);
  for (int i = 0; i < coords.size(); i++)
    center += coords.at(i);
  center /= static_cast<float>(coords.size());

  // Translate coords
  for (int i = 0; i < coords.size(); i++) {
    coords[i] -= center;
  }
}

void RandomDock::randomlyRotateCoordinates(QList<Eigen::Vector3d>& coords)
{
  INIT_RANDOM_GENERATOR();
  // Find center of coordinates:
  Eigen::Vector3d center(0, 0, 0);
  for (int i = 0; i < coords.size(); i++)
    center += coords.at(i);
  center /= static_cast<float>(coords.size());

  // Get random angles
  double X = RANDDOUBLE() * 2 * 3.14159265;
  double Y = RANDDOUBLE() * 2 * 3.14159265;
  double Z = RANDDOUBLE() * 2 * 3.14159265;

  // Build rotation matrix
  Eigen::Matrix3d rx, ry, rz, rot;
  rx << 1, 0, 0, 0, cos(X), -sin(X), 0, sin(X), cos(X);
  ry << cos(Y), 0, sin(Y), 0, 1, 0, -sin(Y), 0, cos(Y);
  rz << cos(Z), -sin(Z), 0, sin(Z), cos(Z), 0, 0, 0, 1;
  rot = rx * ry * rz;

  // Perform operations
  for (int i = 0; i < coords.size(); i++) {
    // Center coords
    coords[i] -= center;
    coords[i] = rot * coords.at(i);
  }
}

void RandomDock::randomlyDisplaceCoordinates(QList<Eigen::Vector3d>& coords,
                                             double radiusMin, double radiusMax)
{
  INIT_RANDOM_GENERATOR();
  // Get random spherical coordinates
  double rho = RANDDOUBLE() * (radiusMax - radiusMin) + radiusMin;
  double theta = RANDDOUBLE() * 2 * 3.14159265;
  double phi = RANDDOUBLE() * 2 * 3.14159265;

  // convert to cartesian coordinates
  double x = rho * sin(phi) * cos(theta);
  double y = rho * sin(phi) * sin(theta);
  double z = rho * cos(phi);

  // Make into vector
  Eigen::Vector3d t;
  t << x, y, z;

  // Transform coords
  for (int i = 0; i < coords.size(); i++)
    coords[i] += t;
}

void RandomDock::DRotateCoordinates(QList<Eigen::Vector3d>& coords)
{
  INIT_RANDOM_GENERATOR();
  // Find center of coordinates:
  Eigen::Vector3d center(0, 0, 0);
  for (int i = 0; i < coords.size(); i++)
    center += coords.at(i);
  center /= static_cast<float>(coords.size());

  // Get random angles
  double theta = RANDDOUBLE() * 2 * 3.14159265;

  // Build rotation matrix
  Eigen::Matrix3d rot;
  rot << cos(theta), -sin(theta), 0, sin(theta), cos(theta), 0, 0, 0, 1;

  // Perform operations
  for (int i = 0; i < coords.size(); i++) {
    // Center coords
    coords[i] -= center;
    coords[i] = rot * coords.at(i);
  }
}

void RandomDock::DDisplaceCoordinates(QList<Eigen::Vector3d>& coords,
                                      double radiusMin, double radiusMax)
{
  INIT_RANDOM_GENERATOR();
  // Get random 2D coordinates
  double pi = RANDDOUBLE() * 2 * 3.14159265;
  double phi = RANDDOUBLE() * 2 * 3.14159265;
  double dx = cos(phi) * (radiusMax - radiusMin) + radiusMin;
  double dy = sin(pi) * (radiusMax - radiusMin) + radiusMin;

  // convert to cartesian coordinates
  double x = dx;
  double y = dy;
  double z = 0;

  // Make into vector
  Eigen::Vector3d t;
  t << x, y, z;

  // Transform coords
  for (int i = 0; i < coords.size(); i++)
    coords[i] += t;
}

} // end namespace RandomDock

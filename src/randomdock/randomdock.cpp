/**********************************************************************
  RandomDock - Holds all data for genetic optimization

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

#include "randomdock.h"

#include "ui/dialog.h"
#include "optimizers/gamess.h"
#include "structures/matrix.h"
#include "structures/substrate.h"
#include "structures/scene.h"

#include "../generic/tracker.h"
#include "../generic/optbase.h"
#include "../generic/queuemanager.h"
#include "../generic/macros.h"

#include <avogadro/atom.h>
#include <avogadro/bond.h>
#include <openbabel/rand.h>
#include <openbabel/mol.h>

#include <QDir>
#include <QFile>
#include <QStringList>

using namespace std;
using namespace Avogadro;

namespace RandomDock {

  RandomDock::RandomDock(RandomDockDialog *parent) :
    OptBase(parent),
    m_dialog(parent),
    substrate(0)
  {
    sceneInitMutex = new QMutex;
    limitRunningJobs = true;
  }

  RandomDock::~RandomDock()
  {
  }

  void RandomDock::startOptimization() {
    debug("Starting optimization.");
    emit startingSession();

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

    // prepare pointers
    m_tracker->deleteAllStructures();

    ////////////////////////////
    // Generate random scenes //
    ////////////////////////////

    // Set up progress bar
    m_dialog->startProgressUpdate(tr("Generating structures..."), 0, 0);

    // Initalize loop variables
    QString filename;
    Scene *scene;
    int progCount=0;

    // Generation loop...
    for (uint i = 0; i < runningJobLimit; i++) {
      m_dialog->updateProgressMaximum( (i == 0)
                                        ? 0
                                        : int(progCount / static_cast<double>(i)) * runningJobLimit );
      m_dialog->updateProgressValue(progCount);
      progCount++;
      m_dialog->updateProgressLabel(tr("%1 scenes generated of (%2)...").arg(i).arg(runningJobLimit));
 
      // Generate/Check molecule
      scene = generateRandomScene();
      initializeAndAddScene(scene);
    }

    m_dialog->stopProgressUpdate();

    m_dialog->saveSession();
    emit sessionStarted();
  }

  Scene* RandomDock::generateRandomScene() {
    // Here we build a scene by extracting coordinates of the atoms
    // from a random conformer of a substrate and the specified number
    // of matrix molecules. The coordinates are rotated, translated,
    // and checked before populating the new scene.

    // Initialize vars
    Atom *atom;
    Bond *newbond;
    Bond *oldbond;
    Matrix *mat;
    Substrate *sub;
    Scene *scene = new Scene;
    QHash<ulong, ulong> idMap; // Old id, new id
    QList<Atom*> atomList;
    QList<Eigen::Vector3d> positions;
    QList<int> atomicNums;
    OpenBabel::OBRandom rand (true);    // "true" uses system random numbers.
    rand.TimeSeed();

    // Select random conformer of substrate
    sub = substrate->getRandomConformer();

    // Extract information from sub
    atomList = sub->atoms();
    for (int j = 0; j < atomList.size(); j++) {
      atomicNums.append(atomList.at(j)->atomicNumber());
      positions.append( *(atomList.at(j)->pos()));
    }

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
    for (uint i = 0; i < sub->numBonds(); i++) {
      newbond = scene->addBond();
      oldbond = sub->bonds().at(i);
      newbond->setAtoms(idMap[oldbond->beginAtomId()],
                        idMap[oldbond->endAtomId()],
                        oldbond->order());
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
      if (i == 0) probs.append(matrixStoich.at(0)/total);
      else probs.append(matrixStoich.at(i)/total + probs.at(i-1));
    }

    // Pick and add matrix elements
    for (uint i = 0; i < numMatrixMol; i++) {
      // Add random conformers of matrix molecule in random locations,
      // orientations
      double r = rand.NextFloat();
      int ind;
      for (ind = 0; ind < probs.size(); ind++)
        if (r < probs.at(ind)) break;

      mat = matrixList.at(ind);

      // Extract information from matrix
      atomList = mat->atoms();
      atomicNums.clear();
      positions.clear();
      idMap.clear();
      for (int j = 0; j < atomList.size(); j++) {
        atomicNums.append(atomList.at(j)->atomicNumber());
        positions.append( *(atomList.at(j)->pos()));
      }

      // Rotate, translate positions
      RandomDock::randomlyRotateCoordinates(positions);
      RandomDock::randomlyDisplaceCoordinates(positions, radius_min, radius_max);

      // Check interatomic distances
      double shortest, distance;
      shortest = -1;
      for (uint mi = 0; mi < mat->numAtoms(); mi++) {
        for (uint si = 0; si < scene->numAtoms(); si++) {
          distance = abs((
                          *(scene->atoms().at(si)->pos()) -
                          positions.at(mi)
                          ).norm()
                         );
          if (shortest < 0)
            shortest = distance; // initialize...
          else {
            if (distance < shortest) shortest = distance;
          }
        }
      }
      if (shortest > IAD_max || shortest < IAD_min) {
        qDebug() << "Bad IAD: "  << shortest;
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
                          idMap[oldbond->endAtomId()],
                          oldbond->order());
        newbond->setAromaticity(oldbond->isAromatic());
      }

    } // end for i in numMatrixMol
    return scene;
  }

  Structure* RandomDock::replaceWithRandom(Structure *s, const QString & reason)
  {
    Scene *oldScene = qobject_cast<Scene*>(s);
    QWriteLocker locker1 (oldScene->lock());

    // Generate/Check new scene
    Scene *scene = 0;
    while (!checkScene(scene))
      scene = generateRandomScene();

    // Copy info over
    QWriteLocker locker2 (scene->lock());
    OpenBabel::OBMol oldOBMol = scene->OBMol();
    oldScene->setOBMol(&oldOBMol);
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

  void RandomDock::initializeAndAddScene(Scene *scene)
  {
    // Initialize vars
    QString id_s;
    QString locpath_s;
    QString rempath_s;

    // So as to not assign duplicate ids, ensure only one assignment
    // is made at a time
    sceneInitMutex->lock();

    // lockForNaming returns a list of all structures, both accepted
    // and pending, so it's size is the id of the new structure.
    int id = m_queue->lockForNaming().size();

    // Generate locations using id number
    id_s.sprintf("%05d",id);
    locpath_s = filePath + "/" + id_s + "/";
    rempath_s = rempath + "/" + id_s + "/";

    // Create path
    QDir dir (locpath_s);
    if (!dir.exists()) {
      if (!dir.mkpath(locpath_s)) {
        error(tr("RandomDock::initializeAndAddScene: Cannot write to path: %1 (path creation failure)",
                 "1 is a file path.")
              .arg(locpath_s));
      }
    }

    // Assign data to scene
    scene->lock()->lockForWrite();
    scene->setIDNumber(id);
    scene->setFileName(locpath_s);
    scene->setRempath(rempath_s);
    scene->setCurrentOptStep(1);
    scene->lock()->unlock();

    // unlockForNaming will append the scene
    m_queue->unlockForNaming(scene);

    // Done!
    sceneInitMutex->unlock();
  }

  void RandomDock::generateNewStructure() {
    initializeAndAddScene(generateRandomScene());
  }

  bool RandomDock::checkLimits() {
    // TODO Are there any input parameters that need to be verified?
    return true;
  }

  bool RandomDock::checkScene(Scene *scene) {
    // TODO Do we need anything here?
    Q_UNUSED(scene);
    return true;
  }

  bool RandomDock::save() {
    if (isStarting) {
      savePending = false;
      return false;
    }
    QReadLocker trackerLocker (m_tracker->rwLock());
    QMutexLocker locker (stateFileMutex);
    QString filename = filePath + "/randomdock.state";
    QString tmpfilename = filename + ".tmp";
    QString oldfilename = filename + ".old";

    // Save data to tmp
    m_dialog->writeSettings(tmpfilename);
    SETTINGS(tmpfilename);
    settings->setValue("randomdock/saveSuccessful", true);
    settings->sync();

    // Move randomdock.state -> randomdock.state.old
    if (QFile::exists(filename) ) {
      if (QFile::exists(oldfilename)) {
        QFile::remove(oldfilename);
      }
      QFile::rename(filename, oldfilename);
    }

    // Move randomdock.state.tmp to randomdock.state
    QFile::rename(tmpfilename, filename);

    // TODO Check that settings are written correctly

    savePending = false;
    return true;
  }

  bool RandomDock::load(const QString &filename) {
    // Attempt to open state file
    QFile file (filename);
    if (!file.open(QIODevice::ReadOnly)) {
      error("RandomDock::load(): Error opening file "+file.fileName()+" for reading...");
      return false;
    }

    SETTINGS(filename);
    bool stateFileIsValid = settings->value("randomdock/saveSuccessful", false).toBool();
    if (!stateFileIsValid) {
      error("RandomDock::load(): File "+file.fileName()+" is incomplete, corrupt, or invalid.");
      return false;
    }
    
    // Get path and other info for later:
    QFileInfo stateInfo (file);
    // path to resume file
    QDir dataDir  = stateInfo.absoluteDir();
    QString dataPath = dataDir.absolutePath() + "/";
    // list of structure dirs
    QStringList dirs = dataDir.entryList(QStringList(), QDir::AllDirs, QDir::Size);
    dirs.removeAll(".");
    dirs.removeAll("..");
    for (int i = 0; i < dirs.size(); i++) {
      if (!QFile::exists(dataPath + "/" + dirs.at(i) + "/scene.state") &&
          !QFile::exists(dataPath + "/" + dirs.at(i) + "/matrix.state") &&
          !QFile::exists(dataPath + "/" + dirs.at(i) + "/substrate.state")
          ) {
          dirs.removeAt(i);
          i--;
      }
    }

    // Set filePath:
    QString newFilePath = dataPath;
    QString newFileBase = filename;
    newFileBase.remove(newFilePath);
    newFileBase.remove("randomdock.state.old");
    newFileBase.remove("randomdock.state.tmp");
    newFileBase.remove("randomdock.state");

    m_dialog->readSettings(filename);

    // Set optimizer
    setOptimizer(OptTypes(settings->value("randomdock/edit/optType").toInt()));
    debug(tr("Resuming RandomDock session in '%1' (%2)")
          .arg(filename)
          .arg(m_optimizer->getIDString()));

    // TODO load scenes, matrix, and substrate

    return true;
  }

  void RandomDock::rankByEnergy(QList<Scene*> *scenes) {
    uint numStructs = scenes->size();
    QList<Scene*> rscenes;

    // Copy scenes to a temporary list (don't modify input list!)
    for (uint i = 0; i < numStructs; i++)
      rscenes.append(scenes->at(i));

    // Simple selection sort
    for (uint i = 0; i < numStructs; i++) {
      for (uint j = i + 1; j < numStructs; j++) {
        if (rscenes.at(j)->getEnergy() < rscenes.at(i)->getEnergy()) {
          rscenes.swap(i,j);
        }
      }
    }

    for (uint i = 0; i < numStructs; i++)
      rscenes.at(i)->setRank(i+1);
  }

  void RandomDock::centerCoordinatesAtOrigin(QList<Eigen::Vector3d> & coords) {
    // Find center of coordinates:
    Eigen::Vector3d center (0,0,0);
    for (int i = 0; i < coords.size(); i++)
      center += coords.at(i);
    center /= static_cast<float>(coords.size());

    // Translate coords
    for (int i = 0; i < coords.size(); i++) {
      coords[i] -= center;
    }
  }

  void RandomDock::randomlyRotateCoordinates(QList<Eigen::Vector3d> & coords) {
    // Find center of coordinates:
    Eigen::Vector3d center (0,0,0);
    for (int i = 0; i < coords.size(); i++)
      center += coords.at(i);
    center /= static_cast<float>(coords.size());

    // Get random angles
    OpenBabel::OBRandom rand (true); 	// "true" uses system random numbers. OB's version isn't too good...
    rand.TimeSeed();
    double X = rand.NextFloat() * 2 * 3.14159265;
    double Y = rand.NextFloat() * 2 * 3.14159265;
    double Z = rand.NextFloat() * 2 * 3.14159265;

    // Build rotation matrix
    Eigen::Matrix3d rx, ry, rz, rot;
    rx <<
      1, 	0, 	0,
      0, 	cos(X),	-sin(X),
      0, 	sin(X), cos(X);
    ry <<
      cos(Y),	0, 	sin(Y),
      0,	1, 	0,
      -sin(Y),	0,	cos(Y);
    rz <<
      cos(Z),	-sin(Z),0,
      sin(Z),	cos(Z),	0,
      0,	0,	1;
    rot = rx * ry * rz;

    // Perform operations
    for (int i = 0; i < coords.size(); i++) {
      // Center coords
      coords[i] -= center;
      coords[i] = rot * coords.at(i);
    }
  }

  void RandomDock::randomlyDisplaceCoordinates(QList<Eigen::Vector3d> & coords, double radiusMin, double radiusMax) {
    // Get random spherical coordinates
    OpenBabel::OBRandom rand (true); 	// "true" uses system random numbers. OB's version isn't too good...
    rand.TimeSeed();
    double rho 	= rand.NextFloat() * (radiusMax - radiusMin) + radiusMin;
    double theta= rand.NextFloat() * 2 * 3.14159265;
    double phi	= rand.NextFloat() * 2 * 3.14159265;
    
    // convert to cartesian coordinates
    double x = rho * sin(phi) * cos(theta);
    double y = rho * sin(phi) * sin(theta);
    double z = rho * cos(phi);

    // Make into vector
    Eigen::Vector3d t;
    t << x,y,z;

    // Transform coords
    for (int i = 0; i < coords.size(); i++)
      coords[i] += t;
  }

  void RandomDock::setOptimizer_string(const QString &IDString)
  {
    if (IDString.toLower() == "gamess")
      setOptimizer(new GAMESSOptimizer (this));
    else
      error(tr("RandomDock::setOptimizer: unable to determine optimizer from '%1'")
            .arg(IDString));
  }

  void RandomDock::setOptimizer_enum(OptTypes opttype)
  {
    switch (opttype) {
    case OT_GAMESS:
      setOptimizer(new GAMESSOptimizer (this));
      break;
    default:
      error(tr("RandomDock::setOptimizer: unable to determine optimizer from '%1'")
            .arg(QString::number((int)opttype)));
      break;
    }
  }
} // end namespace RandomDock

//#include "randomdock.moc"

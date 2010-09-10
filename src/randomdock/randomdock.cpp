/**********************************************************************
  RandomDock - Holds all data for genetic optimization

  Copyright (C) 2009 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <randomdock/randomdock.h>

#include <randomdock/ui/dialog.h>
#include <randomdock/optimizers/gamess.h>
#include <randomdock/structures/matrix.h>
#include <randomdock/structures/substrate.h>
#include <randomdock/structures/scene.h>

#include <globalsearch/tracker.h>
#include <globalsearch/optbase.h>
#include <globalsearch/queuemanager.h>
#include <globalsearch/sshmanager.h>
#include <globalsearch/macros.h>

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
    substrate(0)
  {
    m_idString = "RandomDock";
    sceneInitMutex = new QMutex;
    limitRunningJobs = true;
    // By default, just replace with random when a scene fails.
    failLimit = 1;
    failAction = FA_Randomize;
  }

  RandomDock::~RandomDock()
  {
  }

  void RandomDock::startSearch() {
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

    // Create the SSHManager
    QString pw = "";
    for (;;) {
      try {
        m_ssh->makeConnections(host, username, pw, port);
      }
      catch (SSHConnection::SSHConnectionException e) {
        QString err;
        switch (e) {
        case SSHConnection::SSH_CONNECTION_ERROR:
        case SSHConnection::SSH_UNKNOWN_HOST_ERROR:
        case SSHConnection::SSH_UNKNOWN_ERROR:
        default:
          err = "There was a problem connection to the ssh server at "
            + username + "@" + host + ":" + QString::number(port) + ". "
            + "Please check that all provided information is correct, "
            + "and attempt to log in outside of Avogadro before trying again.";
          error(err);
          return;
        case SSHConnection::SSH_BAD_PASSWORD_ERROR:
          // Chances are that the pubkey auth was attempted but failed,
          // so just prompt user for password.
            err = "Please enter a password for "
              + username + "@" + host + ":" + QString::number(port)
              + ":";
            bool ok;
            QString newPassword;
            // This is a BlockingQueuedConnection, which blocks until
            // the slot returns.
            emit needPassword(err, &newPassword, &ok);
            if (!ok) { // user cancels
              return;
            }
            pw = newPassword;
            continue;
        } // end switch
      } // end catch
      break;
    } // end forever

    // prepare pointers
    m_tracker->deleteAllStructures();

    ////////////////////////////
    // Generate random scenes //
    ////////////////////////////

    // Set up progress bar
    m_dialog->startProgressUpdate(tr("Generating structures..."), 0, 0);

    // Initalize loop variables
    Scene *scene;
    int progCount=0;

    // Generation loop...
    for (uint i = 0; i < runningJobLimit; i++) {
      m_dialog->updateProgressMaximum( (i == 0)
                                        ? 0
                                        : int(progCount
                                              / static_cast<double>(i))
                                       * runningJobLimit );
      m_dialog->updateProgressValue(progCount++);
      m_dialog->updateProgressLabel(tr("%1 scenes generated of (%2)...")
                                    .arg(i)
                                    .arg(runningJobLimit));

      // Generate/Check molecule
      scene = generateRandomScene();
      initializeAndAddScene(scene);
    }

    m_dialog->stopProgressUpdate();

    emit sessionStarted();
    m_dialog->saveSession();
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
    Scene *scene = new Scene;
    QWriteLocker sceneLocker (scene->lock());
    QHash<ulong, ulong> idMap; // Old id, new id
    QList<Atom*> atomList;
    QList<Eigen::Vector3d> positions;
    QList<int> atomicNums;
    OpenBabel::OBRandom rand (true);    // "true" uses system random numbers.
    rand.TimeSeed();

    // Select random conformer of substrate
    substrate->lock()->lockForWrite(); // Write lock prevents
                                       // conformer from changing
    int conformer = substrate->getRandomConformerIndex();
    substrate->setConformer(conformer);

    // Extract information from substrate
    atomList = substrate->atoms();
    for (int j = 0; j < atomList.size(); j++) {
      atomicNums.append(atomList.at(j)->atomicNumber());
      positions.append( *(atomList.at(j)->pos()));
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

      atomicNums.clear();
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
        positions.append( *(atomList.at(j)->pos()));
      }

      mat->lock()->unlock();

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
    oldScene->copyStructure(scene);
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
    // and pending, so it's size+1 is the id of the new structure.
    int id = m_queue->lockForNaming().size() + 1;

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
    scene->setIndex(id-1);
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

  void RandomDock::generateNewStructure() {
    initializeAndAddScene(generateRandomScene());
  }

  bool RandomDock::checkLimits() {
    // TODO Are there any input parameters that need to be verified?
    return true;
  }

  bool RandomDock::checkScene(Scene *scene) {
    // Check if null
    if (!scene) return false;

    return true;
  }

  bool RandomDock::save(const QString &stateFilename, bool notify) {
    Q_UNUSED(notify); //TODO!
    if (isStarting ||
        readOnly) {
      savePending = false;
      return false;
    }
    QString filename;
    QReadLocker trackerLocker (m_tracker->rwLock());
    QMutexLocker locker (stateFileMutex);
    if (stateFilename.isEmpty()) {
      filename = filePath + "/randomdock.state";
    }
    else {
      filename = stateFilename;
    }
    QString tmpfilename = filename + ".tmp";
    QString oldfilename = filename + ".old";

    // Save data to tmp
    m_dialog->writeSettings(tmpfilename);
    SETTINGS(tmpfilename);
    const int VERSION = 1;
    settings->setValue("randomdock/version",     VERSION);

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

    /////////////////////////
    // Print results files //
    /////////////////////////

    QFile file (filePath + "/results.txt");
    QFile oldfile (filePath + "/results_old.txt");
    // if (notify) {
    //   m_dialog->updateProgressLabel(tr("Saving: Writing %1...")
    //                                 .arg(file.fileName()));
    // }
    if (oldfile.open(QIODevice::ReadOnly))
      oldfile.remove();
    if (file.open(QIODevice::ReadOnly))
      file.copy(oldfile.fileName());
    file.close();
    if (!file.open(QIODevice::WriteOnly)) {
      error("RandomDock::save(): Error opening file "+file.fileName()+" for writing...");
      savePending = false;
      return false;
    }
    QTextStream out (&file);

    QList<Structure*> *structures = m_tracker->list();
    QList<Scene*> sortedScenes;
    Scene *scene;

    for (int i = 0; i < structures->size(); i++)
      sortedScenes.append(qobject_cast<Scene*>(structures->at(i)));
    if (sortedScenes.size() != 0) {
      sortAndRankByEnergy(&sortedScenes);
    }

    // Print the data to the file:
    out << "Rank\tID\tEnergy\tStatus\n";
    for (int i = 0; i < sortedScenes.size(); i++) {
      scene = sortedScenes.at(i);
      if (!scene) continue; // In case there was a problem copying.
      scene->lock()->lockForRead();
      out << i << "\t"
          << scene->getIDNumber() << "\t"
          << scene->getEnergy() << "\t\t";
      // Status:
      switch (scene->getStatus()) {
      case Scene::Optimized:
        out << "Optimized";
        break;
      case Scene::Killed:
      case Scene::Removed:
        out << "Killed";
        break;
      case Scene::Duplicate:
        out << "Duplicate";
        break;
      case Scene::Error:
        out << "Error";
        break;
      case Scene::StepOptimized:
      case Scene::WaitingForOptimization:
      case Scene::InProcess:
      case Scene::Empty:
      case Scene::Updating:
      case Scene::Submitted:
      default:
        out << "In progress";
        break;
      }
      scene->lock()->unlock();
      out << endl;
      // if (notify) {
      //   m_dialog->stopProgressUpdate();
      // }
    }

    // Mark operation successful
    settings->setValue("randomdock/saveSuccessful", true);

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
    // Update config data
    int loadedVersion = settings->value("randomdock/version", 0).toInt();
    switch (loadedVersion) {
    case 0:
    case 1:
    default:
      break;
    }

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
    setOptimizer(OptTypes(settings->value("randomdock/edit/optType").toInt()),
                 filename);
    debug(tr("Resuming RandomDock session in '%1' (%2)")
          .arg(filename)
          .arg(m_optimizer->getIDString()));

    // TODO load scenes, matrix, and substrate

    return true;
  }

  void RandomDock::sortAndRankByEnergy(QList<Scene*> *scenes) {
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
          scenes->swap(i,j);
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
      scene_i->setRank(i+1);
      scene_i->lock()->unlock();
    }
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
    OpenBabel::OBRandom rand (true);    // "true" uses system random numbers. OB's version isn't too good...
    rand.TimeSeed();
    double X = rand.NextFloat() * 2 * 3.14159265;
    double Y = rand.NextFloat() * 2 * 3.14159265;
    double Z = rand.NextFloat() * 2 * 3.14159265;

    // Build rotation matrix
    Eigen::Matrix3d rx, ry, rz, rot;
    rx <<
      1,        0,      0,
      0,        cos(X),	-sin(X),
      0,        sin(X), cos(X);
    ry <<
      cos(Y),	0,      sin(Y),
      0,	1,      0,
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
    OpenBabel::OBRandom rand (true);    // "true" uses system random numbers. OB's version isn't too good...
    rand.TimeSeed();
    double rho  = rand.NextFloat() * (radiusMax - radiusMin) + radiusMin;
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

  void RandomDock::setOptimizer_string(const QString &IDString, const QString &filename)
  {
    if (IDString.toLower() == "gamess")
      setOptimizer(new GAMESSOptimizer (this, filename));
    else
      error(tr("RandomDock::setOptimizer: unable to determine optimizer from '%1'")
            .arg(IDString));
  }

  void RandomDock::setOptimizer_enum(OptTypes opttype, const QString &filename)
  {
    switch (opttype) {
    case OT_GAMESS:
      setOptimizer(new GAMESSOptimizer (this, filename));
      break;
    default:
      error(tr("RandomDock::setOptimizer: unable to determine optimizer from '%1'")
            .arg(QString::number((int)opttype)));
      break;
    }
  }
} // end namespace RandomDock

//#include "randomdock.moc"

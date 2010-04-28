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
#include "matrixmol.h"
#include "substratemol.h"
#include "scenemol.h"

#include <avogadro/atom.h>
#include <avogadro/bond.h>
#include <openbabel/rand.h>

#include <QFile>
#include <QStringList>

using namespace std;

namespace Avogadro {

  bool RandomDock::save(RandomDockParams *p) {
    qDebug() << "RandomDock::save( " << p << " ) called";
    // Back up randomdock.state
    QFile file (p->filePath + "/" + p->fileBase + "randomdock.state");
    QFile oldfile (p->filePath + "/" + p->fileBase + "randomdock.state.old");
    if (oldfile.open(QIODevice::ReadOnly))
      oldfile.remove();
    if (file.open(QIODevice::ReadOnly))
      file.copy(oldfile.fileName());
    file.close();
    if (!file.open(QIODevice::WriteOnly)) {
      qWarning() << "RandomDock::save(): Error opening file " << file.fileName() << " for writing...";
      return false;
    }
    QTextStream out (&file);

    // Store values in p
    out << "TODO!";
    return true;
  }

  bool RandomDock::load(RandomDockParams *p, const QString &filename) {
    qDebug() << "RandomDock::load( " << p << ", " << filename << " ) called";
    QFile file (filename);
    if (!file.open(QIODevice::ReadOnly)) {
      qWarning() << "RandomDock::load(): Error opening file " << file.fileName() << " for writing...";
      return false;
    }
    QTextStream in (&file);
    QString line, str;
    QStringList strl;
    while (!in.atEnd()) {
      line = in.readLine();
      qDebug() << line;
      strl = line.split(QRegExp("\\s+"));

      // TODO....

    }
    return true;
  }

  void RandomDock::rankByEnergy(QList<Scene*> *scenes) {
    qDebug() << "RandomDock::rankByEnergy( " << scenes << " ) called.";
    uint numStructs = scenes->size();
    QList<Scene*> rscenes;

    // Copy xtals to a temporary list (don't modify input list!)
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
      rscenes.at(i)->setEnergyRank(i+1);
  }

  void RandomDock::centerCoordinatesAtOrigin(QList<Eigen::Vector3d> & coords) {
    qDebug() << "RandomDock::centerCoordinatesAtOrigin() called;";

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
    qDebug() << "RandomDock::randomlyRotateCoordinates() called;";

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
    qDebug() << "RandomDock::randomlyDisplaceCoordinates() called.";

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


  RandomDockParams::RandomDockParams(RandomDockDialog* d) :
    dialog(d), substrate(0)
  {
    rwLock = new QReadWriteLock;
    scenes = new QList<Scene*>;
    matrixList = new QList<Matrix*>;
  }

  RandomDockParams::~RandomDockParams()
  {
    delete rwLock;
    deleteAllScenes();
    delete scenes;
  }

  void RandomDockParams::reset() {
    clearAllScenes();
  }

  void RandomDockParams::deleteAllScenes() {
    qDebug() << "RandomDockParams::deleteAllScenes() called";
    qDeleteAll(*scenes);
    clearAllScenes();
  }

  void RandomDockParams::clearAllScenes() {
    qDebug() << "RandomDockParams::clearAllScenes() called";
    scenes->clear();
    emit sceneCountChanged();
  }

  Scene* RandomDockParams::generateNewScene() {
    qDebug() << "RandomDockParams::addNewScene() called";

    // Initialize
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

    // Set scene id number
    scene->setSceneNumber(getScenes()->size() + 1);

    // Select random conformer of substrate
    sub = substrate->getRandomConformer();

    // Extract information from sub
    atomList = sub->atoms();
    atomicNums.clear();
    positions.clear();
    idMap.clear();
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

    for (int i = 0; i < matrixStoich.size(); i++)
      total += matrixStoich.at(i);
    for (int i = 0; i < matrixStoich.size(); i++) {
      if (i == 0) probs.append(matrixStoich.at(0)/total);
      else probs.append(matrixStoich.at(i)/total + probs.at(i-1));
    }

    // Pick and add matrix elements
    for (uint i = 0; i < numMatrixMol; i++) {
      // Add random conformers of matrix molecule in random locations, orientations
      double r = rand.NextFloat();
      int ind;
      for (ind = 0; ind < probs.size(); ind++)
        if (r < probs.at(ind)) break;

      mat = matrixList->at(ind);

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

      // Check IADs
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
      // If IAD checks work out ok, add the atoms to the scene
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
    appendScene(scene);
    return scene;
  }
} // end namespace Avogadro

#include "randomdock.moc"

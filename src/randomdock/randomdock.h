/**********************************************************************
  RandomDock - Holds parameters and static functions for random 
		docking analysis

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

#ifndef RANDOMDOCK_H
#define RANDOMDOCK_H

#include "../generic/optbase.h"

#include <Eigen/Geometry>

#include <QDebug>
#include <QStringList>
#include <QReadWriteLock>

using namespace Avogadro;

namespace Avogadro {
  class Structure;
}

namespace RandomDock {
  class RandomDockDialog;
  class RandomDockParams;
  class Scene;
  class Substrate;
  class Matrix;

  class RandomDock : public OptBase
  {
    Q_OBJECT

   public:
    explicit RandomDock(RandomDockDialog *parent);
    virtual ~RandomDock();

    enum OptTypes {
      OT_GAMESS = 0,
    };

    Scene* generateRandomScene();
    Structure* replaceWithRandom(Structure *s, const QString & reason);
    bool checkLimits();
    bool save();
    bool load(const QString & filename);

    bool checkScene(Scene *scene);
    RandomDockDialog* dialog() {return m_dialog;};
    static void rankByEnergy(QList<Scene*> *scenes);

    //TODO move to structure-derived classes, or incorporate into scene generation
    static void centerCoordinatesAtOrigin(QList<Eigen::Vector3d> & coords);
    static void randomlyRotateCoordinates(QList<Eigen::Vector3d> & coords);
    static void randomlyDisplaceCoordinates(QList<Eigen::Vector3d> & coords, double radiusMin, double radiusMax);

    QString substrateFile;	// Filename of the substrate
    Substrate *substrate;	// Pointer to the substrate
    QStringList matrixFiles;	// List of filenames
    QList<Matrix*> matrixList;	// List of pointers to the matrix molecules
    QList<int> matrixStoich;	// Stoichiometry of the matrix elements
    uint numMatrixMol;		// Number of matrix molecules to be placed around the substrate
    double IAD_min;		// Minimum allowed interatomic distance
    double IAD_max;		// Maximum allowed interatomic distance
    double radius_min;		// Minimum distance from origin to place matrix molecules
    double radius_max;		// Maximum distance from origin to place matrix molecules
    bool radius_auto;		// Whether to automatically calculate the matrix radius

    uint cutoff;		// Number of searches to perform in total

    // sOBMutex is here because OB likes to implement singleton
    // classes that aren't thread safe.
    QMutex *sceneInitMutex;
    // These were mutexes, but Qt suddenly started to complain...
    bool savePending, isStarting;


   signals:
    void newInfoUpdate();
    void updateAllInfo();

   public slots:
    void startOptimization();
    void generateNewStructure();
    void initializeAndAddScene(Scene *scene);
    void setOptimizer(Optimizer *o) {setOptimizer_opt(o);};
    void setOptimizer(const QString &IDString) {setOptimizer_string(IDString);};
    void setOptimizer(OptTypes opttype) {setOptimizer_enum(opttype);};

   private:
    RandomDockDialog *m_dialog;

    void setOptimizer_string(const QString &s);
    void setOptimizer_enum(OptTypes opttype);

  };

} // end namespace Avogadro

#endif

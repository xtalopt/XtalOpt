/**********************************************************************
  RandomDock

  Copyright (C) 2009-2011 by David C. Lonie

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

#include <globalsearch/optbase.h>

#include <Eigen/Geometry>

#include <QtCore/QDebug>
#include <QtCore/QStringList>
#include <QtCore/QReadWriteLock>

#include <QtGui/QInputDialog>

namespace GlobalSearch {
  class SlottedWaitCondition;
  class Structure;
}

namespace RandomDock {
  class RandomDockDialog;
  class RandomDockParams;
  class Scene;
  class Substrate;
  class Matrix;

  class RandomDock : public GlobalSearch::OptBase
  {
    Q_OBJECT

   public:
    explicit RandomDock(RandomDockDialog *parent);
    virtual ~RandomDock();

    enum OptTypes {
      OT_GAMESS = 0,
      OT_ADF,
      OT_MOPAC
    };

    enum QueueInterfaces {
      QI_LOCAL = 0
#ifdef ENABLE_SSH
      ,
      QI_PBS,
      QI_SGE
#endif // ENABLE_SSH
    };

    Scene* generateRandomScene();
    GlobalSearch::Structure* replaceWithRandom(GlobalSearch::Structure *s,
                                               const QString & reason = "");
    bool checkLimits();

    bool checkScene(Scene *scene);
    static void sortAndRankByEnergy(QList<Scene*> *scenes);

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
    bool cluster_mode;
    bool strictHBonds;

    QMutex *sceneInitMutex;

  public slots:
    void startSearch();
    void generateNewStructure();
    void initializeAndAddScene(Scene *scene);

  private:
    GlobalSearch::SlottedWaitCondition *m_initWC;
  };

} // end namespace RandomDock

#endif

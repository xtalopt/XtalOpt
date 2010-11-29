/**********************************************************************
  RandomDock - Holds parameters and static functions for random
                docking analysis

  Copyright (C) 2009 by David C. Lonie

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
      OT_ADF
    };

    Scene* generateRandomScene();
    GlobalSearch::Structure* replaceWithRandom(GlobalSearch::Structure *s,
                                               const QString & reason = "");
    bool checkLimits();
    bool save(const QString & filename = "", bool notify = false);
    bool load(const QString & filename);

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

    QMutex *sceneInitMutex;

   signals:
    void newInfoUpdate();
    void updateAllInfo();

   public slots:
    void startSearch();
    void generateNewStructure();
    void initializeAndAddScene(Scene *scene);
    void setOptimizer(GlobalSearch::Optimizer *o) {
      setOptimizer_opt(o);};
    void setOptimizer(const QString &IDString, const QString &filename = "") {
      setOptimizer_string(IDString, filename);};
    void setOptimizer(OptTypes opttype, const QString &filename = "") {
      setOptimizer_enum(opttype, filename);};

   private:
    void setOptimizer_string(const QString &s, const QString &filename = "");
    void setOptimizer_enum(OptTypes opttype, const QString &filename = "");

  };

} // end namespace RandomDock

#endif

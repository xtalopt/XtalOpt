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

#include <Eigen/Geometry>

#include <QDebug>
#include <QStringList>
#include <QReadWriteLock>

namespace Avogadro {
  class RandomDockDialog;
  class RandomDockParams;
  class Scene;
  class Substrate;
  class Matrix;

  class RandomDock : public QObject
  {
    Q_OBJECT

   public:
    explicit RandomDock(QObject *parent = 0) : QObject(parent) {};
    virtual ~RandomDock() {};

    enum OptTypes {
      OT_GAMESS = 0,
    }

    enum FailActions {
      FA_DoNothing = 0,
      FA_KillIt,
      FA_Randomize
    };

    Scene* generateRandomScene();
    Structure* replaceWithRandom(Structure *s, const QString & reason);
    bool checkLimits();
    bool checkScene(Scene *scene);
    bool save();
    bool load(const QString & filename);
    Tracker* tracker(){return m_tracker;};
    QueueManager* queue(){return m_queue;};
    Optimizer* optimizer() {return m_optimizer;};
    RandomDockDialog* dialog() {return m_dialog;};
    static void rankByEnergy(QList<Scene*> *scenes);

    //TODO move to structure-derived classes, or incorporate into scene generation
    static void centerCoordinatesAtOrigin(QList<Eigen::Vector3d> & coords);
    static void randomlyRotateCoordinates(QList<Eigen::Vector3d> & coords);
    static void randomlyDisplaceCoordinates(QList<Eigen::Vector3d> & coords, double radiusMin, double radiusMax);

    QString filePath;		// Path where scenes are saved
    QString rempath;		// Path on remote server to use
    QString qsub;		// Command used to submit jobs
    QString qstat;		// Command used to check queue
    QString qdel;		// Command used to delete jobs
    QString host;		// Name of remote server
    QString username;		// Name of user on remote server

    QString substrateFile;	// Filename of the substrate
    Substrate *substrate;	// Pointer to the substrate
    QStringList matrixFiles;	// List of filenames
    QList<Matrix*> *matrixList;	// List of pointers to the matrix molecules
    QList<int> matrixStoich;	// Stoichiometry of the matrix elements
    uint numMatrixMol;		// Number of matrix molecules to be placed around the substrate
    double IAD_min;		// Minimum allowed interatomic distance
    double IAD_max;		// Maximum allowed interatomic distance
    double radius_min;		// Minimum distance from origin to place matrix molecules
    double radius_max;		// Maximum distance from origin to place matrix molecules
    bool radius_auto;		// Whether to automatically calculate the matrix radius

    uint runningJobLimit;	// Number of searches to perform simultaneously
    uint cutoff;		// Number of searches to perform in total

    // sOBMutex is here because OB likes to implement singleton
    // classes that aren't thread safe.
    QMutex *sOBMutex, *stateFileMutex, *backTraceMutex, *xtalInitMutex;
    // These were mutexes, but Qt suddenly started to complain...
    bool savePending;


   signals:
    void newInfoUpdate();
    void updateAllInfo();
    void sessionStarted();
    void startingSession();
    void structuresCleared();
    void optimizerChanged(Optimizer*);

   public slots:
    void reset();
    void startOptimization();
    void generateNewStructure();
    void initializeAndAddScene(Scene *scene);
    void warning(const QString & s);
    void debug(const QString & s);
    void error(const QString & s);
    void emitSessionStarted() {emit sessionStarted();};
    void emitStartingSession() {emit startingSession();};
    void setIsStartingTrue() {isStarting = true;};
    void setIsStartingFalse() {isStarting = false;};
    void resetDuplicates();
    void checkForDuplicates();
    void printBackTrace();
    void setOptimizer(Optimizer* o);
    void setOptimizer(const QString &IDString);
    void setOptimizer(OptTypes opttype);

   private:
    Tracker *m_tracker;
    QueueManager *m_queue;
    Optimizer *m_optimizer;
    XtalOptDialog *m_dialog;

    void resetDuplicates_();
    void checkForDuplicates_();

  };


  signals:
    void sceneCountChanged();
  };

} // end namespace Avogadro

#endif

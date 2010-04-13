/**********************************************************************
  XtalOpt - Holds all data for genetic optimization

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

#ifndef XTALOPT_H
#define XTALOPT_H

#include "xtal.h"
#include "genetic.h"

#include <QDebug>
#include <QMutex>
#include <QStringList>
#include <QReadWriteLock>

#include <openbabel/generic.h>
#include <openbabel/mol.h>

namespace Avogadro {
  class XtalOptDialog;
  class XtalPendingTracker;

  class XtalOpt : public QObject
  {
    Q_OBJECT

   public:
    explicit XtalOpt(XtalOptDialog *parent);
    virtual ~XtalOpt();

    enum Operators {
      Crossover = 0,
      Stripple,
      Permustrain
    };

    enum OptTypes {
      OptType_VASP = 0,
      OptType_GULP,
      OptType_PWscf
    };

    enum FailActions {
      FA_DoNothing = 0,
      FA_KillIt,
      FA_Randomize
    };

    Xtal* generateSingleOffspring(QList<Xtal*> *inxtals);
    QList<Xtal*>* newGeneration(QList<Xtal*> *inxtals, uint numberNew, uint generationNumber, uint idSeed);
    Xtal* generateRandomXtal(uint generation, uint id);
    bool checkLimits();
    bool checkXtal(Xtal *xtal);
    bool save();
    bool load(const QString & filename);
    QList<Xtal*>* getStructures();
    Xtal* getStructureAt(int i);
    QReadWriteLock* rwLock();
    static void sortByEnthalpy(QList<Xtal*> *xtals);
    static void rankEnthalpies(QList<Xtal*> *xtals);
    static QList<double> getProbabilityList(QList<Xtal*> *xtals);

    uint numInitial;                    // Number of initial structures
    uint runningJobLimit;		// Number of concurrent jobs allowed.
    uint test_nRunsStart;		// Starting run number
    uint test_nRunsEnd;			// Ending run number
    uint test_nStructs;			// Number of structures per run when testing
    uint popSize;                       // Population size
    uint genTotal;                      // New structures per generation

    uint p_her;                         // Percentage of new structures by heredity
    uint p_mut;                         // Percentage of new structures by mutation
    uint p_perm;                        // Percentage of new structures by permutation

    uint her_minimumContribution;	// Minimum contribution each parent in heredity

    double mut_amp_min;			// Minimum amplitude of periodic displacement
    double mut_amp_max;			// Maximum amplitude of periodic displacement
    uint mut_per1;			// Number of cosine waves in direction 1
    uint mut_per2;			// Number of cosine waves in direction 2
    double mut_strainStdev_min;		// Minimum standard deviation of epsilon in the mutation strain matrix
    double mut_strainStdev_max;		// Maximum standard deviation of epsilon in the mutation strain matrix

    uint perm_ex;                       // Number of times atoms are swapped in permutation
    double perm_strainStdev_max;	// Max standard deviation of epsilon in the permutation strain matrix

    double
      a_min,            a_max,		// Limits for lattice
      b_min,            b_max,
      c_min,            c_max,
      alpha_min,	alpha_max,
      beta_min,         beta_max,
      gamma_min,	gamma_max,
      vol_min,		vol_max,        vol_fixed,
      shortestInteratomicDistance;

    double tol_enthalpy, tol_volume;	// Duplicate matching tolerances

    uint failLimit, failAction;

    bool using_fixed_volume, using_shortestInteratomicDistance, using_remote, limitRunningJobs, isStarting, testingMode;
    QString filePath, fileBase, launchCommand, queueCheck, queueDelete, gulpPath, host, username, rempath,
      VASPUser1, VASPUser2, VASPUser3, VASPUser4,
      GULPUser1, GULPUser2, GULPUser3, GULPUser4,
      PWscfUser1, PWscfUser2, PWscfUser3, PWscfUser4;
    QHash<uint, uint> *comp;
    QDateTime queueTimeStamp;
    QStringList
      seedList,
      queueData,
      VASP_INCAR_list, VASP_qScript_list, VASP_KPOINTS_list, VASP_POTCAR_list,
      GULP_gin_list,
      PWscf_qScript_list, PWscf_in_list;
    QList<uint> VASP_POTCAR_comp;
    QList<QHash<QString, QString> > VASP_POTCAR_info;
    XtalOpt *opt;
    OptTypes optType;
    XtalOptDialog *dialog;

    // stupidOpenBabelMutex is stupid because OB likes to implement
    // singleton classes that aren't thread safe.
    QMutex *stupidOpenBabelMutex, *stateFileMutex, *backTraceMutex;
    // These were mutexes, but Qt suddenly started to complain...
    bool checkPopulationPending, savePending, runCheckPending;

    XtalPendingTracker *errorPendingTracker, *restartPendingTracker,
      *startPendingTracker, *updatePendingTracker, *nextOptStepPendingTracker,
      *runningTracker, *progressInfoUpdateTracker, *submissionPendingTracker;

   signals:
    void newStructureAdded();
    void newInfoUpdate();
    void updateAllInfo();
    void xtalReadyToStart();
    void sessionStarted();
    void startingSession();
    void structuresCleared();

   public slots:
    void reset();
    void startOptimization();
    void checkPopulation();
    void updateQueue(int time = 15);
    void updateInfo(Xtal *xtal);
    void addXtal(Xtal *xtal);
    void warning(const QString & s);
    void debug(const QString & s);
    void error(const QString & s);
    void deleteAllStructures();
    void clearStructures();
    void appendStructures(QList<Xtal*> *xtals) {for (int i=0; i < xtals->size(); i++) appendStructure(xtals->at(i));};
    void appendStructure(Xtal* xtal) {m_xtals->append(xtal); emit newStructureAdded();};
    void emitNewInfoUpdate() {emit newInfoUpdate();};
    void emitXtalReadyToStart() {emit xtalReadyToStart();};
    void emitSessionStarted() {emit sessionStarted();};
    void emitStartingSession() {emit startingSession();};
    void setIsStartingTrue() {isStarting = true;};
    void setIsStartingFalse() {isStarting = false;};
    void updateXtal();
    void startXtal();
    void submitJob();
    void restartXtal();
    void errorXtal(const QStringList & queueData);
    void deleteJob(int);
    void nextOptStep();
    void resetDuplicates();
    void checkForDuplicates();
    void checkRunningJobs();
    void printBackTrace();

   private slots:

   private:
    QList<Xtal*>	*m_xtals;
    QReadWriteLock	*m_rwLock;

    void errorXtal_(const QStringList & queueData);
    void startXtal_();
    void submitJob_();
    void restartXtal_();
    void updateXtal_();
    void nextOptStep_();
    void resetDuplicates_();
    void checkForDuplicates_();
    void checkRunningJobs_();
  };

  // Class to hold a thread safe list of xtals that have operations pending
  class XtalPendingTracker
  {
  private:
    QMutex m_mutex;
    QList<Xtal*> m_list;
  public:
    XtalPendingTracker() {};
    void lock(){m_mutex.lock();};
    void unlock(){m_mutex.unlock();};
    void addXtal(Xtal* xtal) {
      lock();
      if (m_list.contains(xtal)) {
        unlock();
        return;
      }
      m_list.append(xtal);
      unlock();
    };
    bool popFirst(Xtal* &xtal) {
      lock();
      if (m_list.isEmpty()) {
        unlock();
        return false;
      }
      xtal = m_list.takeFirst();
      unlock();
      return true;
    };
    bool popFirstAndLock(Xtal* &xtal) {
      lock();
      if (m_list.isEmpty())
        return false;
      xtal = m_list.takeFirst();
      return true;
    };
    void removeXtal(Xtal* xtal) {lock();m_list.removeAll(xtal);unlock();};
    bool hasXtal(Xtal* xtal) {lock(); bool b = m_list.contains(xtal);unlock(); return b;};
    QList<Xtal*> list() {return m_list;};
    int count() {lock(); int s = m_list.size(); unlock(); return s;};
    int countAndLock() {lock(); return m_list.size();};
    void reset() {lock(); m_list.clear(); unlock();};
  };
} // end namespace Avogadro

#endif

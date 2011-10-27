/**********************************************************************
  XtalOpt - Holds all data for genetic optimization

  Copyright (C) 2009-2011 by David C. Lonie

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

#include <xtalopt/structures/xtal.h>
#include <xtalopt/genetic.h>

#include <globalsearch/optbase.h>
#include <globalsearch/queuemanager.h>
#include <globalsearch/macros.h>

#include <QtCore/QDebug>
#include <QtCore/QMutex>
#include <QtCore/QFuture>
#include <QtCore/QStringList>
#include <QtCore/QReadWriteLock>

#include <QtGui/QInputDialog>

namespace GlobalSearch {
  class SlottedWaitCondition;
}

namespace XtalOpt {
  class MolecularXtal;
  class SubMoleculeSource;
  class XtalOptDialog;

  struct XtalCompositionStruct
  {
    double minRadius;
    unsigned int quantity;
  };

  struct MolecularCompStruct
  {
    SubMoleculeSource *source;
    unsigned int quantity;
  };

  class XtalOpt : public GlobalSearch::OptBase
  {
    Q_OBJECT

   public:
    explicit XtalOpt(XtalOptDialog *parent);
    virtual ~XtalOpt();

    enum OptTypes {
      OT_VASP = 0,
      OT_GULP,
      OT_PWscf,
      OT_CASTEP
    };

    enum QueueInterfaces {
      QI_LOCAL = 0
#ifdef ENABLE_SSH
      ,
      QI_PBS,
      QI_SGE,
      QI_SLURM,
      QI_LSF,
      QI_LOADLEVELER
#endif // ENABLE_SSH
    };

    enum Operators {
      OP_Crossover = 0,
      OP_Stripple,
      OP_Permustrain
    };

    Xtal* generateRandomXtal(uint generation, uint id);
    MolecularXtal* generateRandomMXtal(uint generation, uint id);
    bool addSeed(const QString & filename);
    GlobalSearch::Structure* replaceWithRandom(GlobalSearch::Structure *s,
                                               const QString & reason = "");
    GlobalSearch::Structure* replaceWithOffspring(GlobalSearch::Structure *s,
                                                  const QString &reason = "");
    bool checkLimits();
    bool checkXtal(Xtal *xtal, QString * err = NULL);
    QString interpretTemplate(const QString & templateString, GlobalSearch::Structure* structure);
    QString getTemplateKeywordHelp();
    bool load(const QString & filename, const bool forceReadOnly = false);

    uint numInitial;                    // Number of initial structures

    uint popSize;                       // Population size

    uint p_cross;                       // Percentage of new structures by crossover
    uint p_strip;	                // Percentage of new structures by stripple
    uint p_perm;                        // Percentage of new structures by permustrain

    uint cross_minimumContribution;	// Minimum contribution each parent in crossover

    double strip_amp_min;		// Minimum amplitude of periodic displacement
    double strip_amp_max;		// Maximum amplitude of periodic displacement
    uint strip_per1;			// Number of cosine waves in direction 1
    uint strip_per2;			// Number of cosine waves in direction 2
    double strip_strainStdev_min;	// Minimum standard deviation of epsilon in the stripple strain matrix
    double strip_strainStdev_max;	// Maximum standard deviation of epsilon in the stripple strain matrix

    uint perm_ex;                       // Number of times atoms are swapped in permustrain
    double perm_strainStdev_max;	// Max standard deviation of epsilon in the permustrain strain matrix

    double
      a_min,            a_max,		// Limits for lattice
      b_min,            b_max,
      c_min,            c_max,
      alpha_min,        alpha_max,
      beta_min,         beta_max,
      gamma_min,        gamma_max,
      vol_min, vol_max, vol_fixed,
      scaleFactor, minRadius;

    double tol_xcLength;        	// Duplicate matching tolerances
    double tol_xcAngle;
    double tol_spg;

    // MXtalOptGenetic params
    // - Crossover
    int mga_p_cross;                      //! [0, 100]
    double mga_cross_minimumContribution; //! [0.0, 50.0]
    // - Reconf
    int mga_p_reconf;                     //! [0, 100]
    int mga_reconf_minSubMolsToReplace;   //! [0, sum(i, mcomp[i].quantity)]
    int mga_reconf_maxSubMolsToReplace;   //! [0, sum(i, mcomp[i].quantity)]
    double mga_reconf_minStrain;          //! [0, 1]
    double mga_reconf_maxStrain;          //! [0, 1]
    // - Swirl
    int mga_p_swirl;                      //! [0, 100]
    int mga_swirl_minSubMolsToRotate;     //! [0, sum(i, mcomp[i].quantity)]
    int mga_swirl_maxSubMolsToRotate;     //! [0, sum(i, mcomp[i].quantity)]
    int mga_swirl_minRotationDegree;      //! [0, 180]
    double mga_swirl_fracInPlane;         //! [0, 1]
    double mga_swirl_minStrain;           //! [0, 1]
    double mga_swirl_maxStrain;           //! [0, 1]

    bool using_fixed_volume;
    bool using_interatomicDistanceLimit;

    QHash<uint, XtalCompositionStruct> comp;
    QList<MolecularCompStruct> mcomp;
    QStringList seedList;

    bool isMolecularXtalSearch() {return m_isMolecular;}

    QMutex *xtalInitMutex;

  public slots:
    void startSearch();
    bool initializeSubMoleculeSources(bool notify);
    void initializeSMSProgressUpdate(int finished, int total);
    void generateNewStructure();
    Xtal* generateNewXtal();
    void initializeAndAddXtal(Xtal *xtal,
                              unsigned int generation,
                              const QString &parents);
    void resetSpacegroups();
    void resetDuplicates();
    void checkForDuplicates();
    void setMolecularXtalSearch(bool b)
    {
      if (m_isMolecular == b) return;
      m_isMolecular = b;
      emit isMolecularXtalSearchChanged(b);
    }

  Q_SIGNALS:
    void isMolecularXtalSearchChanged(bool);

   protected:
    friend class XtalOptUnitTest;
    void resetSpacegroups_();
    void resetDuplicates_();
    void checkForDuplicates_();
    void generateNewStructure_();

    void interpretKeyword(QString &keyword, GlobalSearch::Structure* structure);
    QString getTemplateKeywordHelp_xtalopt();

    GlobalSearch::SlottedWaitCondition *m_initWC;
    bool m_isMolecular;
    // Used for progress updates during initializeSubMoleculeSource
    int m_currentSubMolSourceProgress;
  };

} // end namespace XtalOpt

#endif

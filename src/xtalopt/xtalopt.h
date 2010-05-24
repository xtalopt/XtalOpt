/**********************************************************************
  XtalOpt - Holds all data for genetic optimization

  Copyright (C) 2009-2010 by David C. Lonie

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

#include "../generic/optbase.h"
#include "../generic/xtal.h"
#include "../generic/queuemanager.h"
#include "../generic/tracker.h"
#include "../generic/macros.h"
#include "genetic.h"

#include <QDebug>
#include <QMutex>
#include <QStringList>
#include <QReadWriteLock>

namespace Avogadro {
  class XtalOptDialog;

  class XtalOpt : public OptBase
  {
    Q_OBJECT

   public:
    explicit XtalOpt(XtalOptDialog *parent);
    virtual ~XtalOpt();

    enum OptTypes {
      OT_VASP = 0,
      OT_GULP,
      OT_PWscf
    };

    enum Operators {
      OP_Crossover = 0,
      OP_Stripple,
      OP_Permustrain
    };

    Xtal* generateRandomXtal(uint generation, uint id);
    Structure* replaceWithRandom(Structure *s, const QString & reason);
    bool checkLimits();
    bool checkXtal(Xtal *xtal);
    bool save();
    bool load(const QString & filename);
    XtalOptDialog* dialog() {return m_dialog;};
    static void sortByEnthalpy(QList<Xtal*> *xtals);
    static void rankEnthalpies(QList<Xtal*> *xtals);
    static QList<double> getProbabilityList(QList<Xtal*> *xtals);

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
      alpha_min,	alpha_max,
      beta_min,         beta_max,
      gamma_min,	gamma_max,
      vol_min,		vol_max,        vol_fixed,
      shortestInteratomicDistance;

    double tol_enthalpy, tol_volume;	// Duplicate matching tolerances

    bool using_fixed_volume, using_shortestInteratomicDistance;
    QHash<uint, uint> comp;
    QStringList seedList;

    QMutex *xtalInitMutex;

   signals:
    void newInfoUpdate();
    void updateAllInfo();

   public slots:
    void startOptimization();
    void generateNewStructure();
    void initializeAndAddXtal(Xtal *xtal, uint generation, const QString &parents);
    void resetDuplicates();
    void checkForDuplicates();
    void setOptimizer(Optimizer *o) {setOptimizer_opt(o);};
    void setOptimizer(const QString &IDString) {setOptimizer_string(IDString);};
    void setOptimizer(OptTypes opttype) {setOptimizer_enum(opttype);};

   private:
    XtalOptDialog *m_dialog;

    void resetDuplicates_();
    void checkForDuplicates_();

    void setOptimizer_string(const QString &s);
    void setOptimizer_enum(OptTypes opttype);
  };

} // end namespace Avogadro

#endif

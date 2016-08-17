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
#include <QtCore/QtConcurrentRun>

#include <QtGui/QInputDialog>

// Forward declarations...
struct latticeStruct;

namespace GlobalSearch {
  class SlottedWaitCondition;
}

namespace XtalOpt {
  class XtalOptDialog;

  struct XtalCompositionStruct
  {
    double minRadius;
    unsigned int quantity;
  };

  // comparison of two XtalCompositionStructs
  static inline bool operator==(const XtalCompositionStruct& lhs,
                                const XtalCompositionStruct& rhs)
  {
    if (lhs.minRadius == rhs.minRadius &&
        lhs.quantity  == rhs.quantity) return true;
    else return false;
  }

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
      OT_CASTEP,
      OT_SIESTA
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

    Xtal* randSpgXtal(uint generation, uint id, uint FU, uint spg);
    Xtal* generateRandomXtal(uint generation, uint id);

    //Identical to the previous generateRandomXtal except the number of formula units has been determined elsewhere
    Xtal* generateRandomXtal(uint generation, uint id, uint FU);
    // The H_ indicates that it returns a dynamically allocated xtal.
    // H stands for 'heap'
    // mitosisMutation is set just to help with naming the ancestry
    Xtal* H_getMutatedXtal(QList<GlobalSearch::Structure*>& structures, int FU,
                           Xtal* preselectedXtal = NULL,
                           bool includeCrossover = true,
                           bool includeMitosis = true,
                           bool mitosisMutation = false);
    bool addSeed(const QString & filename);
    GlobalSearch::Structure* replaceWithRandom(GlobalSearch::Structure *s,
                                               const QString & reason = "");
    GlobalSearch::Structure* replaceWithOffspring(GlobalSearch::Structure *s,
                                                  const QString &reason = "");
    bool checkLimits();
    bool checkComposition(Xtal *xtal, QString * err = NULL);
    bool checkXtal(Xtal *xtal, QString * err = NULL);
    QString interpretTemplate(const QString & templateString, GlobalSearch::Structure* structure);
    QString getTemplateKeywordHelp();
    bool load(const QString & filename, const bool forceReadOnly = false);

    bool loaded;

    uint numInitial;                    // Number of initial structures

    uint popSize;                       // Population size

    uint FU_crossovers_generation;      // Generation to begin crossovers between different formula units
    uint chance_of_mitosis;             // Chance of performing mitosis after "using one pool?" has been checked

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
      new_a_min,        new_a_max,      //new_min and new_max are formula unit corrected
      new_b_min,        new_b_max,
      new_c_min,        new_c_max,
      alpha_min,        alpha_max,
      beta_min,         beta_max,
      gamma_min,        gamma_max,
      vol_min, vol_max, vol_fixed,
      new_vol_min,      new_vol_max,
      scaleFactor, minRadius,
      minFU,            maxFU;

    int
        divisions,                   // Number of divisions for mitosis
        ax,                          // Number of divisions for cell vector 'a'
        bx,                          // Number of divisions for cell vector 'b'
        cx;                          // Number of divisions for cell vector 'c'

    double tol_xcLength;        	// Duplicate matching tolerances
    double tol_xcAngle;
    double tol_spg;

    bool using_fixed_volume;
    bool using_interatomicDistanceLimit;
    bool using_mitotic_growth;
    bool using_FU_crossovers;
    bool using_one_pool;

    // Generate a new formula unit.
    int  FU;
    QList<uint> formulaUnitsList;
    // List that contains the lowest enthalpy discovered so far at a particular
    // formula unit. The position on the list is equivalent to the formula unit
    // being analyzed (i. e., lowestEnthalpyFUList.at(i) is the lowest enthalpy
    // at an FU of i
    QList<double> lowestEnthalpyFUList;

    bool using_mitosis;
    bool using_subcellPrint;

    QHash<uint, XtalCompositionStruct> comp;
    QStringList seedList;

    QMutex *xtalInitMutex;

    // Spacegroup generation
    bool using_randSpg;
    // If the number is -1, that spg is not allowed
    // Otherwise, it represents the minimum number of xtals for that spacegroup
    // per formula unit. The spacegroup it represents is index + 1
    QList<int> minXtalsOfSpgPerFU;

  public slots:
    void startSearch();
    void generateNewStructure();
    Xtal* generateNewXtal();
    // Identical to generateNewXtal() except the number of formula units has been specified already
    Xtal* generateNewXtal(uint FU);
    // setupNewXtal should be true if we are setting up a new dynamically
    // allocated xtal. It copies over cell info and atom positions from xtal.
    // if xtal == 0, one is selected from the probability list with formula
    // units of initialFU.
    // mutate should be true if you wish to perform a stripple/permustrain
    // on the xtal immediately after generating the supercell.
    Xtal* generateSuperCell(uint initialFU, uint finalFU, Xtal *xtal,
                            bool setupNewXtal, bool mutate);
    // Returns a dynamically allocated xtal that has undergone a primitive
    // reduction of the xtal that was input
    Xtal* generatePrimitiveXtal(Xtal *xtal);
    void initializeAndAddXtal(Xtal *xtal,
                              unsigned int generation,
                              const QString &parents);
    bool onTheFormulaUnitsList(uint FU);
    void printSubXtal(Xtal *xtal,
                              unsigned int generation,
                              uint id);
    void resetSpacegroups();
    void resetDuplicates();
    void checkForDuplicates();
    void updateLowestEnthalpyFUList(GlobalSearch::Structure* s);
    void _incrementParentNumTotOffspring(GlobalSearch::Structure* s);

   protected:
    friend class XtalOptUnitTest;
    void resetSpacegroups_();
    void resetDuplicates_();
    void checkForDuplicates_();
    void generateNewStructure_();
    void updateLowestEnthalpyFUList_(GlobalSearch::Structure* s);
    struct supCheckStruct
    {
      Xtal *i, *j;
      double tol_len, tol_ang;
    };
    void checkIfSups(supCheckStruct &st);

    Xtal* selectXtalFromProbabilityList(
                               QList<GlobalSearch::Structure*> structures,
                               uint FU = 0);
    void interpretKeyword(QString &keyword, GlobalSearch::Structure* structure);
    QString getTemplateKeywordHelp_xtalopt();

    GlobalSearch::SlottedWaitCondition *m_initWC;
    // This lock is to prevent multiple threads from generating the same
    // supercell in XtalOpt::generateNewXtal()
    QReadWriteLock supercellCheckLock;
    // This creates a background thread that waits 0.1 seconds before unlocking
    void waitThenUnlockSupercellCheckLock() {
      QtConcurrent::run(this, &XtalOpt::waitThenUnlockSupercellCheckLock_);};
    void waitThenUnlockSupercellCheckLock_() {
      GS_SLEEP(0.1);
      supercellCheckLock.unlock();
    };

    // Sets a_min, b_min, c_min, ... to the given lattice structs
    void setLatticeMinsAndMaxes(latticeStruct& latticeMins,
                                latticeStruct& latticeMaxes);
    // If composition is Ti1O2, returns {22, 8, 8}
    QList<uint> getListOfAtoms(uint FU);
    std::vector<uint> getStdVecOfAtoms(uint FU);
    uint pickRandomSpgFromPossibleOnes();
    void updateProgressBar(size_t goal, size_t attempted, size_t succeeded);

   signals:
    void updateFormulaUnitsListUIText();
    void updateVolumesToBePerFU(uint FU);
  };

} // end namespace XtalOpt

#endif

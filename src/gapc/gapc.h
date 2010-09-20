/**********************************************************************
  GAPC -- A genetic algorithm for protected clusters

  Copyright (C) 2010 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef OPTGAPC_H
#define OPTGAPC_H

#include <globalsearch/optbase.h>

#include <QStringList>
#include <QHash>

namespace GlobalSearch {
  class Structure;
  class Optimizer;
}

using namespace GlobalSearch;

namespace GAPC {
  class GAPCDialog;
  class ProtectedCluster;

  struct GAPC_Comp {
    // Atomic number / quantity
    QHash <unsigned int, unsigned int> core;
    // TODO ligand / quantity
    //QHash <Ligand, unsigned int> ligands;
  };

    enum Operators {
      OP_Crossover = 0,
      OP_Twist,
      OP_Exchange,
      OP_RandomWalk
    };

  class OptGAPC : public GlobalSearch::OptBase
  {
    Q_OBJECT

   public:
    // Variables for GAPC
    QMutex initMutex;
    GAPC_Comp comp;
    QStringList seedList;
    unsigned int numInitial;
    unsigned int popSize;
    float tol_enthalpy;
    float minIAD;
    float maxIAD;
    int p_cross;
    int p_twist;
    int p_exch;
    int p_randw;
    int twist_minRot;
    int exch_numExch;
    int randw_numWalkers;
    float randw_minWalk;
    float randw_maxWalk;

    enum OptTypes {
      OT_OpenBabel = 0,
      OT_ADF
    };

    /**
     * Constructor
     *
     * @param parent Dialog window of GUI.
     */
    explicit OptGAPC(GAPCDialog *parent);

    /**
     * Destructor
     */
    virtual ~OptGAPC();

    /**
     * Replace the Structure with an appropriate random Structure.
     *
     * @param s The Structure to be replaced. This pointer remains
     * valid -- the structure it points will be modified.
     * @param reason Reason for replacing. This will appear in the
     * Structure::getParents() string. (Optional)
     *
     * @return The pointer to the structure (same as s).
     */
    GlobalSearch::Structure* replaceWithRandom(Structure *s, const QString & reason = "");

    /**
     * Before starting an optimization, this function will check the
     * parameters of the search to ensure that they are within a
     * reasonable range.
     *
     * @return True if the search parameters are valid, false otherwise.
     */
    bool checkLimits();

    /**
     * Check a protected cluster to ensure that it conforms to the
     * specified limits.
     *
     * @return True if the protected cluster is valid, false
     * otherwise.
     */
    bool checkPC(ProtectedCluster *pc);

    /**
     * Save the current search. If filename is omitted, default to
     * m_filePath + "/[search name].state". Must set
     * OptBase::savePending = true before calling.
     *
     * @param filename Filename to write to. Optional.
     * @param notify Whether to display a user-visible notification
     *
     * @return True if successful, false otherwise.
     */
    bool save(const QString & filename = "", bool notify = false);

    /**
     * Load a search session from the specified filename.
     *
     * @param filename State file to resume.
     *
     * @return True is successful, false otherwise.
     */
    bool load(const QString & filename);

    ProtectedCluster* generateRandomPC(unsigned int gen = 1, unsigned int id = 0);
    static void sortByEnthalpy(QList<ProtectedCluster*> *pcs);
    static void rankEnthalpies(QList<ProtectedCluster*> *pcs);
    static QList<double> getProbabilityList(QList<ProtectedCluster*> *xtals);

   signals:
    void updateAllInfo();
    void newInfoUpdate();

   public slots:
    /**
     * Begin the search.
     */
    void startSearch();

    /**
     * Called when the QueueManager requests more Structures.
     * @sa QueueManager
     */
    void generateNewStructure();

    void initializeAndAddPC(ProtectedCluster *pc,
                            uint gen, const QString &parents);

    void setOptimizer(Optimizer *o) {
      setOptimizer_opt(o);};
    void setOptimizer(const QString &IDString, const QString &filename = "") {
      setOptimizer_string(IDString, filename);};
    void setOptimizer(OptTypes opttype, const QString &filename = "") {
      setOptimizer_enum(opttype, filename);};

   protected:
    /// Hidden calls to setOptimizer
    void setOptimizer_string(const QString &s, const QString &filename = "");
    void setOptimizer_enum(OptTypes opttype, const QString &filename = "");

  };

} // end namespace GAPC

#endif

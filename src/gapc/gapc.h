/**********************************************************************
  GAPC -- A genetic algorithm for protected clusters

  Copyright (C) 2010-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef OPTGAPC_H
#define OPTGAPC_H

#include <globalsearch/optbase.h>

#include <QHash>
#include <QStringList>

namespace GlobalSearch {
class Structure;
class SlottedWaitCondition;
}

namespace GAPC {
class GAPCDialog;
class ProtectedCluster;

struct GAPC_Comp
{
  // Atomic number / quantity
  QHash<unsigned int, unsigned int> core;
  // TODO ligand / quantity
  // QHash <Ligand, unsigned int> ligands;
};

enum Operators
{
  OP_Crossover = 0,
  OP_Twist,
  OP_Exchange,
  OP_RandomWalk,
  OP_AnisotropicExpansion
};

class OptGAPC : public GlobalSearch::OptBase
{
  Q_OBJECT

public:
  enum OptTypes
  {
    OT_ADF = 0,
    OT_GULP
  };

  enum QueueInterfaces
  {
    QI_LOCAL = 0
#ifdef ENABLE_SSH
    ,
    QI_PBS,
    QI_SGE
#endif // ENABLE_SSH
  };

  enum ExplodeActions
  {
    EA_Randomize = 0,
    EA_Kill
  };

  // Variables for GAPC
  QMutex initMutex;
  GAPC_Comp comp;
  QStringList seedList;
  unsigned int numInitial;
  unsigned int popSize;
  double tol_enthalpy;
  double tol_geo;
  double minIAD;
  double maxIAD;
  double explodeLimit;
  int p_cross;
  int p_twist;
  int p_exch;
  int p_randw;
  int p_aniso;
  int twist_minRot;
  int exch_numExch;
  int randw_numWalkers;
  float randw_minWalk;
  float randw_maxWalk;
  float aniso_amp;
  ExplodeActions explodeAction;

  /**
   * Constructor
   *
   * @param parent Dialog window of GUI.
   */
  explicit OptGAPC(GAPCDialog* parent);

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
  GlobalSearch::Structure* replaceWithRandom(GlobalSearch::Structure* s,
                                             const QString& reason = "");

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
  bool checkPC(ProtectedCluster* pc);

  /**
   * Load a search session from the specified filename.
   *
   * @param filename State file to resume.
   *
   * @return True is successful, false otherwise.
   */
  bool load(const QString& filename, const bool forceReadOnly = false);

  ProtectedCluster* generateRandomPC(unsigned int gen = 1, unsigned int id = 0);

public slots:
  /**
   * Begin the search.
   */
  void startSearch();

  /**
   * Check a protected cluster after all optimization steps are
   * completed to ensure that it conforms to the specified limits.
   *
   * This currently checks to see if the structure has exploded. If
   * it has, the action described by explodeAction is taken.
   *
   */
  void checkOptimizedPC(GlobalSearch::Structure* s);

  /**
   * Called when the QueueManager requests more Structures.
   * @sa QueueManager
   */
  void generateNewStructure();

  void initializeAndAddPC(ProtectedCluster* pc, uint gen,
                          const QString& parents);
  void resetDuplicates();
  void checkForDuplicates();

protected:
  void resetDuplicates_();
  void checkForDuplicates_();
  void generateNewStructure_();

  GlobalSearch::SlottedWaitCondition* m_initWC;
};

} // end namespace GAPC

#endif

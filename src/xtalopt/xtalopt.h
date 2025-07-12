/**********************************************************************
  XtalOpt - Holds all data for genetic optimization

  Copyright (C) 2009-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef XTALOPT_H
#define XTALOPT_H

#include <globalsearch/macros.h>
#include <globalsearch/searchbase.h>
#include <globalsearch/constants.h>

#include <QtConcurrent>

#include <memory>
#include <mutex>

// Forward declarations...
struct latticeStruct;

namespace GlobalSearch {
class AbstractDialog;
class Molecule;
class SlottedWaitCondition;
}

namespace XtalOpt {
class Xtal;
class XtalOptRpc;

// As of XtalOpt 14, we use the user-provided "chemical formula" strings
//   to obtain the list of chemical composition, elemental volumes, and
//   reference energies.
// The "Cell Composition" object will be used to store the information about
//   the chemical composition of a cell (symbol, atomic number, number of atoms
//   of a symbol for all elements in the cell); and is obtained by parsing a
//   (full) chemical formula string in "formulaToComposition" function.
//
// We maintain a list of these objects as the list of user-provided formula, and
//   use them to generate new cells.
// Further, we use them as a convenience tool to parse elemental volume
//   and reference energy entries.
//
// The set of "get..." function are interfaces to access various information.
//   Since composition object stores elemental data as a qMap; these functions
//   return "sorted" lists: symbols are alphabetically sorted, and the rest are
//   sorted accordingly. Although, we generally don't rely on this order in the code.
//
class CellComp
{
public:
  void clear() {m_data.clear();}
  // Set an element's entry in cell composition using symbol, atomic number, atom count
  void set(QString symb, uint atomicn, uint acount)
    {m_data[symb] = qMakePair(atomicn, acount);}
  // Access atom count of "symbol" or "atomic number"
  uint getCount(const QString& s) const
    {return (m_data.contains(s) ? m_data.value(s).second : 0);}
  uint getCount(const uint& i) const
    {for (const auto& ed : m_data) {if (ed.first == i) return ed.second;} return 0;}
  // Access total number of atoms and types
  int getNumAtoms() const
    {int n = 0; for (const auto& ed : m_data) n += ed.second; return n;}
  int getNumTypes() const
    {return m_data.size();}
  QString getFormula() const
    {QString f = "";
    for (const auto& key : m_data.keys())
      f += QString("%1%2").arg(key).arg(m_data.value(key).second);
    return f;}
  // Access lists: the symbols are always sorted alphabetically and the
  // rest of the lists are sorted accordingly.
  QList<QString> getSymbols() const
    {return m_data.keys();}
  QList<uint> getAtomicNumbers() const
    {QList<uint> a; for (const auto &ed : m_data) a.append(ed.first); return a;}
  QList<uint> getCounts() const
    {QList<uint> c; for (const auto &ed : m_data) c.append(ed.second); return c;}
private:
  // This is a map of  <"element symbol" , "atomic number , atom count">
  QMap<QString, QPair<uint, uint> > m_data;
};

// Reference energy object: for each formula in the reference energies input
//   we construct a cell composition object, and assign to it the corresponding
//   energy. A list of this "RefEnergy" objects will be used in the code to
//   produce the "reference energies vector" for convex hull calculation.
struct RefEnergy
{
  CellComp cell;
  double   energy;
};

// Minimum radii of elements: an instance of this class is created once the
//   user's input formula are processed, by assigning the values for all
//   elements in the search space.
// Note: prior to XtalOpt14, this information was stored in the composition
//   object. It is separated now to simplify it's runtime update by eliminating
//   the need to update all composition objects every time radii-related stuff
//   are being updated.
class EleRadii
{
public:
  void clear() {m_data.clear();}
  // Set an element's entry with atomic number and minimum radius
  void set(const uint& atomcn, const double& minradius)
    {m_data[atomcn] = minradius;}
  // Access the list of elements stored in the object
  QList<uint> getAtomicNumbers() const
    {return m_data.keys();}
  // Access the min radius for an atomic number (if element is not there, return 1e300)
  double getMinRadius(uint a) const
    {if (m_data.contains(a)) return m_data.value(a); return PINF;}
private:
  QMap<uint, double> m_data;
};

// Elemental volume object (as of XtalOpt14): user can provide a list of min/max
//   values for elemental volumes; besides the absolute and scaled volume limit
//   options. An instance of this class is initialized once the input is processed,
//   and being updated at runtime if user provides new values.
class EleVolume
{
public:
  void clear() {m_data.clear();}
  // Set an element's entry with atomic number and minimum/maximum volumes
  void set(const uint& atomcn, const double& min, const double& max)
    {m_data[atomcn] = qMakePair(min, max);}
  // Access the list of elements stored in the object
  QList<uint> getAtomicNumbers() const
    {return m_data.keys();}
  // Access the min/max volumes for an atomic number
  double getMinVolume(uint a) const
    {if (m_data.contains(a)) return m_data.value(a).first; return 0.0;}
  double getMaxVolume(uint a) const
    {if (m_data.contains(a)) return m_data.value(a).second; return 0.0;}
private:
  QMap<uint, QPair<double, double> > m_data;
};

struct MolUnit
{
  unsigned int numCenters;
  unsigned int numNeighbors;
  double dist;
  unsigned int geom;
};

struct IAD
{
  double minIAD;
};

// A simple minIADs class that uses unordered atomic numbers for
// the key and a double for the value. In order to set a value,
// you must use set() and not ().
class minIADs
{
public:
  // Set a specific atomic number pair to have a specific IAD
  void set(short i, short j, double d) { m_data[std::minmax(i, j)] = d; }

  void clear() { m_data.clear(); }

  // Get the IAD value for a specific atomic number pair, or
  // 1e300 if the value does not exist.
  double operator()(short i, short j) const
  {
    if (m_data.count(std::minmax(i, j)) != 1)
      return PINF;
    return m_data.at(std::minmax(i, j));
  }

private:
  std::map<std::pair<short, short>, double> m_data;
};

class XtalOpt : public GlobalSearch::SearchBase
{
  Q_OBJECT

public:
  explicit XtalOpt(GlobalSearch::AbstractDialog* parent = nullptr);
  virtual ~XtalOpt() override;

  enum OptTypes
  {
    OT_VASP = 0,
    OT_GULP,
    OT_PWscf,
    OT_CASTEP,
    OT_SIESTA,
    OT_MTP,
    OT_GENERIC
  };

  enum QueueInterfaces
  {
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

  enum Operators
  {
    OP_Stripple = 0,
    OP_Permustrain,
    OP_Permutomic,
    OP_Permucomp,
    OP_Crossover
  };

  // Helper struct for similarity check with XtalComp
  struct simCheckStruct
  {
    Xtal *i, *j;
    double tol_len, tol_ang;
  };

  virtual void readRuntimeOptions() override;

  Xtal* randSpgXtal(uint generation, uint id, CellComp incomp,
                    uint spg, bool checkSpgWithSpglib = true);
  Xtal* generateRandomXtal(uint generation, uint id, CellComp incomp = {});

  // Starting from XtalOpt 14, the user defines genetic operation relative
  //   weights, can add sub-system seeds or define various search types.
  // The choice of genetic operation depends on all these conditions; and
  //   for simplicity this function is introduced that takes everything
  //   into account and returns a randomly selected genetic operation.
  Operators selectOperation(bool valid);

  // The _H indicates that it returns a dynamically allocated xtal.
  // H stands for 'heap'
  Xtal* generateEvolvedXtal_H(QList<GlobalSearch::Structure*>& structures,
                            Xtal* preselectedXtal = nullptr);

  Xtal* generateEmptyXtalWithLattice(CellComp incomp = {});

  bool addSeed(const QString& filename);
  GlobalSearch::Structure* replaceWithRandom(
    GlobalSearch::Structure* s, const QString& reason = "") override;
  GlobalSearch::Structure* replaceWithOffspring(
    GlobalSearch::Structure* s, const QString& reason = "") override;
  bool checkStepOptimizedStructure(GlobalSearch::Structure* s,
                                   QString* err = NULL) override;
  bool checkLimits() override;
  bool checkComposition(Xtal* xtal, bool isSeed = false);
  bool checkLattice(Xtal* xtal);
  bool checkXtal(Xtal* xtal);

  // Returns true if all IAD checks passed, and false otherwise
  static bool checkIntramolecularIADs(const GlobalSearch::Molecule& mol,
                                      const minIADs& iads,
                                      bool ignoreBondedAtoms);

  // These two molecules under comparison should have the same unit cell
  // Returns true if all IAD checks passed, and false otherwise
  // Does not check intramolecular IADs.
  static bool checkIntermolecularIADs(const GlobalSearch::Molecule& mol1,
                                      const GlobalSearch::Molecule& mol2,
                                      const minIADs& iads);

  QString interpretTemplate(const QString& templateString,
                            GlobalSearch::Structure* structure) override;
  QString getTemplateKeywordHelp() override;

  std::unique_ptr<GlobalSearch::QueueInterface> createQueueInterface(
    const std::string& queueName) override;

  std::unique_ptr<GlobalSearch::Optimizer> createOptimizer(
    const std::string& optName) override;

  bool save(QString filename = "", bool notify = false) override;
  bool load(const QString& filename, const bool forceReadOnly = false) override;

  bool writeEditSettings(const QString& filename = "");
  bool readEditSettings(const QString& filename = "");
  bool readSettings(const QString& filename = "");

  // This function will load all the xtals in the data directory in a
  // read-only fashion so that a plot may be displayed. This is intended
  // to be used for generating a plot in the CLI mode.
  bool plotDir(const QDir& dataDir);

  void checkIfSimilar(simCheckStruct& st);

  // This function parses the objective-related input and initializes relevant variables
  bool processInputObjectives(QString s);

  // An override function to give searchbase access to reference energies for hull calculations
  virtual std::vector<double> getReferenceEnergiesVector() override;

  // Returns the composition object for an xtal/structure (considering the full
  //   chemical system, so, might include zero counts!).
  CellComp getXtalComposition(GlobalSearch::Structure *s);

  // Convert a string of chemical formula to composition object
  CellComp formulaToComposition(QString form);

  // Compare two composition object if they are equivalent/supercell or not
  double compareCompositions(CellComp comp1, CellComp comp2);

  // Get the estimated min/max volume limits for a composition
  void getCompositionVolumeLimits(CellComp incomp, double& vol_min, double& vol_max);

  // Get the composition that has the smallest atom counts for all elements
  CellComp getMinimalComposition();

  // Get the composition that has the largest atom counts for all elements
  CellComp getMaximalComposition();

  // Get the sorted full list of chemical element in the current run (reference chemical system)
  // Also, overrides a searchbase function so access to this info is provided there for hull calcs.
  QList<QString> getChemicalSystem() const override;

  // Process input formulas string and produce composition objects
  bool processInputChemicalFormulas(QString s);

  // Process input reference energy string
  bool processInputReferenceEnergies(QString s);

  // Process input elemental volumes string
  bool processInputElementalVolumes(QString s);

  // Variables

  // Input strings to be processed for main internal variables
  // NOTE: To keep the state/runtime files shorter; reading and saving
  //   of the chemical formula, reference energies, elemental volumes
  //   will be "based on a string entry". That's, for example, we save
  //   the "input entry for chemical formulas" as is to the state file
  //   and at the time of resuming a run, we read that and process it
  //   to obtain the actual composition list.
  QString input_formulas_string;  // Input string for chemical formulas
  QString input_ene_refs_string;  // Input string for reference energies
  QString input_ele_volm_string;  // Input string for elemental volumes

  QList<CellComp> compList;    // Cell compositions
  QList<RefEnergy> refEnergies;// Reference energies
  EleRadii  eleMinRadii;       // Elemental minimum radii
  EleVolume eleVolumes;        // Elemental volumes

  int maxAtoms;                // Maximum number of atoms in the run
  bool vcSearch;               // Is the search variable-composition?

  bool loaded;

  uint numInitial;             // Number of initial structures

  uint parentsPoolSize;        // Parents pool size

  uint p_cross;       // Relative weight of new structures by crossover
  uint p_strip;       // Relative weight of new structures by stripple
  uint p_perm;        // Relative weight of new structures by permustrain
  uint p_atomic;      // Relative weight of new structures by permutomic
  uint p_comp;        // Relative weight of new structures by permucomp
  double p_supercell; // Percent chances of expanding a new xtal to a random supercell

  uint
    cross_minimumContribution; // Minimum contribution each parent in crossover
  uint  cross_ncuts;           // Number of cut points in crossover

  double strip_amp_min;         // Minimum amplitude of periodic displacement
  double strip_amp_max;         // Maximum amplitude of periodic displacement
  uint strip_per1;              // Number of cosine waves in direction 1
  uint strip_per2;              // Number of cosine waves in direction 2
  double strip_strainStdev_min; // Minimum standard deviation of epsilon in the
                                // stripple strain matrix
  double strip_strainStdev_max; // Maximum standard deviation of epsilon in the
                                // stripple strain matrix

  uint perm_ex;      // Number of times atoms are swapped in permustrain
  double perm_strainStdev_max; // Max standard deviation of epsilon in the
                               // permustrain strain matrix

  double a_min, a_max, // Limits for lattice
    b_min, b_max, c_min, c_max, new_a_min,
    new_a_max, // new_min and new_max are formula unit corrected
    new_b_min, new_b_max, new_c_min, new_c_max, alpha_min, alpha_max, beta_min,
    beta_max, gamma_min, gamma_max, vol_min, vol_max, vol_fixed,
    scaleFactor, minRadius, vol_scale_min, vol_scale_max;

  double tol_xcLength;  // XtalComp similarity tolerance: length
  double tol_xcAngle;   // XtalComp similarity tolerance: angle
  double tol_spg;       // spglib tolerance (default value is in constants.h file)
  double tol_rdf;       // tolerance for RDF similarity (0.0 to 1.0, default = 0.0: ignore)
  double tol_rdf_sigma; // gaussian spread for RDF calculations (default = 0.008)
  double tol_rdf_cutoff;// distance cutoff for RDF calculations (default = 6.0)
  int    tol_rdf_nbins; // number of bins for RDF calculations (default = 3000)

  bool using_molUnit;

  bool using_customIAD;
  bool using_checkStepOpt;
  QHash<QPair<int, int>, IAD> interComp;

  bool using_interatomicDistanceLimit;

  QHash<QPair<int, int>, MolUnit> compMolUnit;

  QStringList seedList;

  QMutex* xtalInitMutex;

  // Spacegroup generation
  bool using_randSpg;
  // If the number is -1, that spg is not allowed
  // Otherwise, it represents the minimum number of xtals for that spacegroup
  // per formula unit. The spacegroup it represents is index + 1
  QList<int> minXtalsOfSpg;

  std::unique_ptr<XtalOptRpc> m_rpcClient;

public slots:
  bool startSearch() override;
  void generateNewStructure() override;
  Xtal* generateNewXtal(CellComp incomp);
  // Returns a dynamically allocated xtal that has undergone a primitive
  // reduction of the xtal that was input
  Xtal* generatePrimitiveXtal(Xtal* xtal);
  Xtal* generateSuperCell(Xtal* parentXtal, uint expansion, bool distort);
  void initializeAndAddXtal(Xtal* xtal, unsigned int generation,
                            const QString& parents);
  void resetSpacegroups();
  void resetSimilarities();
  void checkForSimilarities();
  CellComp pickRandomCompositionFromPossibleOnes();
  uint pickRandomSpgFromPossibleOnes();

  QString CLIRuntimeFile()
  {
    return locWorkDir + QDir::separator() + "cli-runtime-options.txt";
  }

  // Import/Export settings in GUI from/to CLI
  static bool importSettings_(QString filename, XtalOpt& x);
  static bool exportSettings_(QString filename, XtalOpt* x);

  // Prints all the options to @p stream
  static void printOptionSettings(QTextStream& stream, XtalOpt* x);

  void setupRpcConnections();
  void sendRpcUpdate(GlobalSearch::Structure* s);

  // If composition is Ti1O2, returns {22, 8, 8}
  QList<uint> getListOfAtomsComp(CellComp incomp);
  std::vector<uint> getStdVecOfAtomsComp(CellComp incomp);

protected:
  friend class XtalOptUnitTest;
  void resetSpacegroups_();
  void resetSimilarities_();
  void checkForSimilarities_();
  void generateNewStructure_();

  Xtal* selectXtalFromProbabilityList(
    QList<GlobalSearch::Structure*> structures);
  void interpretKeyword(QString& keyword, GlobalSearch::Structure* structure);
  QString getTemplateKeywordHelp_xtalopt();

  GlobalSearch::SlottedWaitCondition* m_initWC;

  // Sets a_min, b_min, c_min, ... to the given lattice structs
  void setLatticeMinsAndMaxes(latticeStruct& latticeMins,
                              latticeStruct& latticeMaxes);
  void updateProgressBar(size_t goal, size_t attempted, size_t succeeded);

  static void setGeom(unsigned int& geom, QString strGeom);
  static QString getGeom(int numNeighbors, int geom);

signals:
  void updatePlot();
  void enablePlotUpdate();
  void disablePlotUpdate();
};
} // end namespace XtalOpt

#endif

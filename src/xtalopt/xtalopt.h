/**********************************************************************
  XtalOpt - XtalOpt application search workflow implementation

  Copyright (C) 2009-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef XTALOPT_H
#define XTALOPT_H

#include <common/fileutils.h>
#include <common/settings.h>
#include <search/search.h>
#include <atoms/geometry.h>

#include <xtalopt/types.h>
#include <xtalopt/settings.h>

#include <QtConcurrent>
#include <QElapsedTimer>
#include <QPair>
#include <QSet>

#include <atomic>
#include <memory>
#include <mutex>

class QTimer;

namespace Search {
class SlottedWaitCondition;
}

namespace XtalOpt {
class Xtal;

// State files written by XtalOpt use this format version.
const int CurrentStateSchemaVersion = 5;

// Write a structure state file.
void writeStructureStateFile(Xtal& xtal, const QString& filename);

class XtalOpt : public Search::SearchBase
{
  Q_OBJECT

public:
  explicit XtalOpt(QObject* parent = nullptr);

  // Values obtained from the structured input entries.
  const QList<CellComp>& compList() const { return x_compList; }
  QList<CellComp>& compList() { return x_compList; }
  const QHash<QPair<int, int>, IAD>& interComp() const { return x_interComp; }
  QHash<QPair<int, int>, IAD>& interComp() { return x_interComp; }
  const EleRadii& eleMinRadii() const { return x_eleMinRadii; }
  EleRadii& eleMinRadii() { return x_eleMinRadii; }
  const EleVolume& eleVolumes() const { return x_eleVolumes; }
  EleVolume& eleVolumes() { return x_eleVolumes; }
  const QList<RefEnergy>& refEnergies() const { return x_refEnergies; }
  QList<RefEnergy>& refEnergies() { return x_refEnergies; }
  const QStringList& moleculeUnitInputs() const { return x_moleculeUnitInputs; }
  QStringList& moleculeUnitInputs() { return x_moleculeUnitInputs; }
  const std::vector<Atoms::Geometry>& moleculeUnits() const { return x_moleculeUnits; }
  std::vector<Atoms::Geometry>& moleculeUnits() { return x_moleculeUnits; }
  const QStringList& seedList() const { return x_seedList; }
  QStringList& seedList() { return x_seedList; }
  const QList<int>& minXtalsOfSpg() const { return x_minXtalsOfSpg; }
  QList<int>& minXtalsOfSpg() { return x_minXtalsOfSpg; }

  // Repeated entries' values.
  QStringList objectiveLines() const;
  QStringList constraintLines() const;
  QStringList customIADLines() const;
  QStringList molUnitLines() const;
  void clearCustomIADs() { x_interComp.clear(); }

  // Change the atom limits (if needed) with a warning to user.
  void adjustAtomCountLimits(int numAtoms);

  virtual ~XtalOpt() override;

  // Run mode, set when the program starts.
  enum RunMode {
    RunModeUnknown,   // not yet set (constructor default)
    RunModeCliStart,  // CLI fresh start from xtalopt.in
    RunModeCliResume, // CLI resume from xtalopt.state
    RunModeGui,       // interactive GUI (start or resume, user-driven)
    RunModeReadOnly   // read-only plot / inspection session
  };

  // Genetic operators available for offspring generation.
  enum Operators
  {
    OP_Stripple = 0,
    OP_Permustrain,
    OP_Permutomic,
    OP_Permucomp,
    OP_Crossover
  };

  // Set the run mode.
  void setRunMode(RunMode mode)
  {
    x_runMode = mode;
    setReadOnly(mode == RunModeReadOnly);
  }
  RunMode getRunMode() const { return x_runMode; }

  //
  // Accessors for scalar settings
  //

  // Composition limits
  int getMaxAtoms() const { return x_maxAtoms; }
  void setMaxAtoms(int v) { x_maxAtoms = v; }
  int getMinAtoms() const { return x_minAtoms; }
  void setMinAtoms(int v) { x_minAtoms = v; }
  bool getVcSearch() const { return x_vcSearch; }
  void setVcSearch(bool v) { x_vcSearch = v; }
  bool getSaveHullSnapshots() const { return x_saveHullSnapshots; }
  void setSaveHullSnapshots(bool v) { x_saveHullSnapshots = v; }
  // Lattice limits
  double getAMin() const { return x_aMin; }
  void setAMin(double v) { x_aMin = v; }
  double getBMin() const { return x_bMin; }
  void setBMin(double v) { x_bMin = v; }
  double getCMin() const { return x_cMin; }
  void setCMin(double v) { x_cMin = v; }
  double getAMax() const { return x_aMax; }
  void setAMax(double v) { x_aMax = v; }
  double getBMax() const { return x_bMax; }
  void setBMax(double v) { x_bMax = v; }
  double getCMax() const { return x_cMax; }
  void setCMax(double v) { x_cMax = v; }
  double getAlphaMin() const { return x_alphaMin; }
  void setAlphaMin(double v) { x_alphaMin = v; }
  double getBetaMin() const { return x_betaMin; }
  void setBetaMin(double v) { x_betaMin = v; }
  double getGammaMin() const { return x_gammaMin; }
  void setGammaMin(double v) { x_gammaMin = v; }
  double getAlphaMax() const { return x_alphaMax; }
  void setAlphaMax(double v) { x_alphaMax = v; }
  double getBetaMax() const { return x_betaMax; }
  void setBetaMax(double v) { x_betaMax = v; }
  double getGammaMax() const { return x_gammaMax; }
  void setGammaMax(double v) { x_gammaMax = v; }
  // Volume limits
  double getVolMin() const { return x_volMin; }
  void setVolMin(double v) { x_volMin = v; }
  double getVolMax() const { return x_volMax; }
  void setVolMax(double v) { x_volMax = v; }
  double getVolScaleMin() const { return x_volScaleMin; }
  void setVolScaleMin(double v) { x_volScaleMin = v; }
  double getVolScaleMax() const { return x_volScaleMax; }
  void setVolScaleMax(double v) { x_volScaleMax = v; }
  // Interatomic distance settings
  bool getUsingScaledIAD() const { return x_usingScaledIAD; }
  void setUsingScaledIAD(bool v) { x_usingScaledIAD = v; }
  bool getUsingCustomIAD() const { return x_usingCustomIAD; }
  void setUsingCustomIAD(bool v) { x_usingCustomIAD = v; }
  bool getUsingCheckStepOpt() const { return x_usingCheckStepOpt; }
  void setUsingCheckStepOpt(bool v) { x_usingCheckStepOpt = v; }
  double getScaleFactor() const { return x_scaleFactor; }
  void setScaleFactor(double v) { x_scaleFactor = v; }
  double getMinRadius() const { return x_minRadius; }
  void setMinRadius(double v) { x_minRadius = v; }
  // RandSpg
  bool getUsingRandSpg() const { return x_usingRandSpg; }
  void setUsingRandSpg(bool v) { x_usingRandSpg = v; }
  // Run parameters
  uint getNumInitial() const { return x_numInitial; }
  void setNumInitial(uint v) { x_numInitial = v; }
  uint getParentsPoolSize() const { return x_parentsPoolSize; }
  void setParentsPoolSize(uint v) { x_parentsPoolSize = v; }
  // Operator weights
  uint getPStrip() const { return x_pStrip; }
  void setPStrip(uint v) { x_pStrip = v; }
  uint getPPerm() const { return x_pPerm; }
  void setPPerm(uint v) { x_pPerm = v; }
  uint getPAtomic() const { return x_pAtomic; }
  void setPAtomic(uint v) { x_pAtomic = v; }
  uint getPComp() const { return x_pComp; }
  void setPComp(uint v) { x_pComp = v; }
  uint getPCross() const { return x_pCross; }
  void setPCross(uint v) { x_pCross = v; }
  uint getPSupercell() const { return x_pSupercell; }
  void setPSupercell(uint v) { x_pSupercell = v; }
  // Operator parameters
  double getStripAmpMin() const { return x_stripAmpMin; }
  void setStripAmpMin(double v) { x_stripAmpMin = v; }
  double getStripAmpMax() const { return x_stripAmpMax; }
  void setStripAmpMax(double v) { x_stripAmpMax = v; }
  uint getStripPer1() const { return x_stripPer1; }
  void setStripPer1(uint v) { x_stripPer1 = v; }
  uint getStripPer2() const { return x_stripPer2; }
  void setStripPer2(uint v) { x_stripPer2 = v; }
  double getStripStrainStdevMin() const { return x_stripStrainStdevMin; }
  void setStripStrainStdevMin(double v) { x_stripStrainStdevMin = v; }
  double getStripStrainStdevMax() const { return x_stripStrainStdevMax; }
  void setStripStrainStdevMax(double v) { x_stripStrainStdevMax = v; }
  uint getPermEx() const { return x_permEx; }
  void setPermEx(uint v) { x_permEx = v; }
  double getPermStrainStdevMax() const { return x_permStrainStdevMax; }
  void setPermStrainStdevMax(double v) { x_permStrainStdevMax = v; }
  uint getCrossNcuts() const { return x_crossNcuts; }
  void setCrossNcuts(uint v) { x_crossNcuts = v; }
  uint getCrossMinimumContribution() const { return x_crossMinimumContribution; }
  void setCrossMinimumContribution(uint v) { x_crossMinimumContribution = v; }
  // Tolerances
  double getTolXcLength() const { return x_tolXcLength; }
  void setTolXcLength(double v) { x_tolXcLength = v; }
  double getTolXcAngle() const { return x_tolXcAngle; }
  void setTolXcAngle(double v) { x_tolXcAngle = v; }
  double getTolSpg() const { return x_tolSpg; }
  void setTolSpg(double v) { x_tolSpg = v; }
  double getTolRdf() const { return x_tolRdf; }
  void setTolRdf(double v) { x_tolRdf = v; }
  double getTolRdfCutoff() const { return x_tolRdfCutoff; }
  void setTolRdfCutoff(double v) { x_tolRdfCutoff = v; }
  int getTolRdfNbins() const { return x_tolRdfNbins; }
  void setTolRdfNbins(int v) { x_tolRdfNbins = v; }
  double getTolRdfSigma() const { return x_tolRdfSigma; }
  void setTolRdfSigma(double v) { x_tolRdfSigma = v; }
  // Structured single-line settings
  QString getInputFormulasString() const { return x_inputFormulasString; }
  void setInputFormulasString(const QString& v) { x_inputFormulasString = v; }
  QString getInputEneRefsString() const { return x_inputEneRefsString; }
  void setInputEneRefsString(const QString& v) { x_inputEneRefsString = v; }
  QString getInputEleVolmString() const { return x_inputEleVolmString; }
  void setInputEleVolmString(const QString& v) { x_inputEleVolmString = v; }
  QString getInputForcedSpgsString() const { return x_inputForcedSpgsString; }
  bool setInputForcedSpgsString(const QString& v);


  bool addSeed(const QString& filename);

  Xtal* randSpgXtal(uint generation, uint id, CellComp incomp,
                    uint spg, bool checkSpgWithSpglib = true);

  Xtal* generateRandomXtal(uint generation, uint id, CellComp incomp = {});

  // This returns a dynamically allocated xtal.
  Xtal* generateEvolvedXtal(Xtal* preselectedXtal = nullptr);

  // Starting from XtalOpt 14, the user defines genetic operation relative
  //   weights, can add sub-system seeds or define various search types.
  // The choice of genetic operation depends on all these conditions; and
  //   for simplicity this function is introduced that takes everything
  //   into account and returns a randomly selected genetic operation.
  Operators selectOperation(bool valid);

  Search::Structure* replaceWithRandom(
    Search::Structure* s, const QString& reason = "") override;

  Search::Structure* replaceWithOffspring(
    Search::Structure* s, const QString& reason = "") override;

  bool checkStepOptimizedStructure(Search::Structure* s, QString* err = nullptr) override;

  bool checkLimits();

  bool checkCustomIADs(bool reportError = true) const;

  bool checkComposition(Xtal* xtal, bool isSeed = false);

  bool checkLattice(Xtal* xtal);

  bool checkXtal(Xtal* xtal);

  // Returns true if all IAD checks passed, and false otherwise
  static bool checkInternalIADs(const Atoms::Geometry& geometry, const minIADs& iads,
                                bool ignoreBondedAtoms);

  // These two geometries under comparison should have the same unit cell
  // Returns true if all IAD checks passed, and false otherwise
  // Does not check internal IADs.
  static bool checkBetweenGeometriesIADs(const Atoms::Geometry& geometry1,
                                         const Atoms::Geometry& geometry2, const minIADs& iads);

  // Save session
  bool saveSessionState(QString filename, bool notify = false);

  // Write the state file only.
  bool saveStateFile(const QString& filename);

  // Load settings/state from a file (doesn't start a session by itself).
  bool importStateFile(const QString& filename);

  // Resume a saved session using the SearchBase session functions.
  bool resumeSearch(const QString& filename, bool* settingsOnlyLoaded = nullptr);

  // Write job/search settings to a scheme file.
  bool saveSchemeFile(const QString& filename);

  // Read job and search settings from a scheme file.
  bool loadSchemeFile(const QString& filename, bool fullState = false);

  // Convert an old settings file.
  bool convertLegacyFileToCurrent(const QString& filename);

  // Read shared settings from a state file.
  bool readStateFile(const QString& filename, bool fullState,
                     bool* stateWasConverted = nullptr);

  // Absolute path to this session's state file.
  QString stateFilePath() const;

  // Path to the CLI runtime file.
  QString runtimeFilePath()
  {
    return Common::localPath(getLocWorkDir(), "cli-runtime-options.txt");
  }

  // Check for changes to the runtime file.
  void checkRuntimeFile();

  // Request a state file save after a runtime setting change.
  void requestStateFileSave();

  // Request a save of the current results table.
  void requestResultsFileSave(bool alsoHullFile = false);

  void requestStructureEvaluation(Search::Structure* structure);
  void handleOptimizedDeparture(Search::Structure* structure);

  // Process input formulas string and produce composition objects
  bool processInputChemicalFormulas(QString s);

  // Process input reference energy string
  bool processInputReferenceEnergies(QString s);

  // Process input elemental volumes string
  bool processInputElementalVolumes(QString s);

  // Process the saved single-line inputs again.
  bool rebuildDerivedSettings();

  // Parse one molecule-unit entry.
  bool processInputMoleculeUnit(QString s);

  // Parse and append one user-defined objective entry.
  bool processInputObjectives(QString s);

  // Parse and append one constrained-search entry.
  bool processInputConstraint(QString s);

  // Parse one "<symbol>, <symbol>, <minDistance>" custom-IAD entry and store it
  //   (symmetrically) in the interatomic-distance table.
  bool processInputCustomIAD(QString s);

  // Text accessors for settings with a complicated type (eg, jobFailAction).
  QString failActionText() const;
  bool setFailActionText(const QString& v);
  QString optimizationTypeText() const;
  bool setOptimizationTypeText(const QString& v);
  // Read and write the seed structure list.
  QString seedStructuresText() const;
  void setSeedStructuresText(const QString& v);

  // Read the input and runtime files.
  bool loadInputFile(const QString& filename, bool bestEffort,
                     bool loadAndVerifyAssets = true);

  // Write an input file; warn if this fails.
  bool saveInputFile(const QString& filename);

  // Read and apply the runtime file. Do nothing if it cannot be read.
  void loadRuntimeFile();

  // Apply settings from runtime text.
  void applyRuntimeText(const QString& runtimeText);

  // Write the initial runtime file for a CLI run.
  void saveRuntimeFile();

  // Convert multi-entry (repeated) lists to text.
  QString objectiveEntryToText(int objectiveIndex) const;
  QString constraintEntryToText(int constraintIndex) const;
  static QString customIADEntryToText(int atomicNumber1, int atomicNumber2, double minIAD);

  // Get the composition that has the smallest atom counts for all elements
  CellComp getMinimalComposition();

  // Get the sorted full list of chemical element in the current run (reference chemical system)
  QList<QString> getChemicalSystem() const;

  // If composition is Ti1O2, returns {22, 8, 8}
  QList<uint> getListOfAtomsComp(CellComp incomp);

  std::vector<uint> getStdVecOfAtomsComp(CellComp incomp);

  // Recalculate per-element minimum radii from current scale/custom settings.
  void refreshElementMinRadii();

  // Check whether RandSpg can generate spg for the given comp.
  bool isRandSpgPossibleForComposition(uint spg, CellComp comp);

  // Return input formulas compatible with a RandSpg space group.
  QStringList randSpgCompatibleFormulaStrings(uint spg);

  //
  // Objective and constraint functions
  // The engine doesn't distinguish the built-in and user objectives, so these
  //   functions translate the user objective numbers when needed.
  //

  // Remove an objective (the built-in above-hull one can't be removed).
  bool removeUserObjective(int index);

  // Remove a constrained-search entry.
  bool removeConstraint(int index);

  // Update the above-hull objective weight considering user-defined objectives' weights.
  void refreshBuiltinObjectiveWeight();

  // Engine objective index of the built-in above-hull value
  static int getBuiltinObjectiveIndex() { return 0; }

  // First engine objective index available to user-defined objectives
  static int getFirstUserObjectiveIndex() { return 1; }

  // Number of user-defined objectives (excludes the always present above hull).
  int getUserObjectivesNum() const;

  // Return the engine objective index for a zero-based user-defined objective.
  int getUserObjectiveIndex(int userObjectiveNumber) const;

  // Whether any user-defined objectives are configured.
  bool hasUserObjectives() const;

  // Whether external objective or constraint scripts are needed.
  bool needsObjectiveOrConstraintCalculations() const;

private:
  // Returns the structure that was marked similar, or nullptr if none was.
  Xtal* checkIfSimilar(Xtal* a, Xtal* b, const QList<QString>& aSymbols, const QList<QString>& bSymbols);

  void ensureBuiltinObjective();
  bool validateUserObjectiveDefinition(ObjType objtyp, const QString& objexe, const QString& objout,
                                       double objwgt, QString* errorMessage = nullptr) const;
  bool validateConstraintDefinition(const QString& exe, const QString& out, QString* errorMessage = nullptr) const;

  bool runSearch(const QString& stateFile, bool* settingsOnlyLoaded);
  bool checkLocalInputFiles(bool includeSeeds, QString* errorMessage) const;
  bool checkOptimizerAndQueue(const QString& readinessAction, QString* errorMessage);
  bool canRequestStateFileSave() const;
  void requestStructureStateFileSave(Search::Structure* structure);
  void requestStructureStateFileSave(const QList<Search::Structure*>& structures);
  void markResultsFileNeedsSave();
  void retryPendingFileSaves();
  void clearPendingRequests();
  QList<Search::Structure*> trackedStructuresSnapshot();
  bool savePendingStateFiles(const QString& filename, bool saveAll, bool showProgress);
  bool saveRequestedOutputFiles(bool saveAll, bool showProgress);
  QSet<Search::Structure*> saveStructureStateFiles(const QList<Search::Structure*>& structures,
                                                   const QSet<Search::Structure*>& structuresToSave,
                                                   bool showProgress);
  void finishSearch();
  void requestFullEvaluation();
  void requestEvaluationAfterKill(Search::Structure* structure);
  bool evaluateStructuresIncrementally(const QSet<Search::Structure*>& structures);
  // Write all XtalOpt state groups to filename.
  bool writeStateFileContents(const QString& filename);
  bool writeFreshStateFile(const QString& filename);
  QStringList readStructureStateDirectories(const QString& stateFile) const;
  bool restorePopulation(const QString& stateFile, const QStringList& xtalDirs);

  //
  // Generation functions.
  //

  Xtal* generateRandomRandSpgXtal(uint generation, uint id, CellComp incomp = {});
  Xtal* generateRandomAtomicXtal(uint generation, uint id, CellComp incomp = {});
  Xtal* generateRandomMolUnitXtal(uint generation, uint id, CellComp incomp = {});
  Xtal* generateEmptyXtalWithLattice(CellComp incomp = {});

  //
  // XtalOpt-specific run variables/settings.
  //

  std::unique_ptr<QMutex> x_xtalInitMutex;
  std::mutex x_stateSaveMutex;
  std::mutex x_outputSaveMutex;

  std::mutex x_filesNeedingSaveMutex;
  QSet<Search::Structure*> x_structuresNeedingSave;
  bool x_settingsStateNeedsSave;
  bool x_resultsFileNeedsSave;
  bool x_hullFileNeedsSave;
  QList<QPair<QString, QString>> x_pendingHullSnapshots;
  QSet<Search::Structure*> x_structuresNeedingEvaluation;
  bool x_fullEvaluationNeeded;

  std::atomic<bool> x_resultsFileSaveScheduled;
  std::atomic<unsigned long long> x_hullSnapshotSequence;
  std::atomic<qint64> x_lastOutputWriteMs;
  std::atomic<qint64> x_lastOutputWriteEndMs;
  QElapsedTimer x_saveClock;
  std::vector<double> x_hullPointsCache;

  BackgroundJob x_fileSaveJob;
  BackgroundJob x_outputSaveJob;

  BackgroundJob x_similarityCheckJob;
  BackgroundJob x_spacegroupResetJob;
  std::atomic<bool> x_similaritiesNeedReset;

  RunMode x_runMode;
  QString x_lastRuntimeText;

  QTimer* x_resultsSaveTimer;

  QTimer* x_saveRetryTimer;

  QTimer* x_runtimeTimer;

  //
  // Scalar settings
  //
  // Composition limits
  int x_maxAtoms = 0;                // Maximum number of atoms in the run
  int x_minAtoms = 0;                // Minimum number of atoms in the run
  bool x_vcSearch = false;           // Is the search variable-composition?
  bool x_saveHullSnapshots = false;
  // Limits for lattice
  double x_aMin = 0.0;
  double x_bMin = 0.0;
  double x_cMin = 0.0;
  double x_aMax = 0.0;
  double x_bMax = 0.0;
  double x_cMax = 0.0;
  double x_alphaMin = 0.0;
  double x_betaMin = 0.0;
  double x_gammaMin = 0.0;
  double x_alphaMax = 0.0;
  double x_betaMax = 0.0;
  double x_gammaMax = 0.0;
  // Volume limits
  double x_volMin = 0.0;
  double x_volMax = 0.0;
  double x_volScaleMin = 0.0;
  double x_volScaleMax = 0.0;
  // Interatomic distance settings
  bool x_usingScaledIAD = false;
  bool x_usingCustomIAD = false;
  bool x_usingCheckStepOpt = false;
  double x_scaleFactor = 0.0;
  double x_minRadius = 0.0;
  // RandSpg
  bool x_usingRandSpg = false;
  // Run parameters
  uint x_numInitial = 0;             // Number of initial structures
  uint x_parentsPoolSize = 0;        // Parents pool size
  // Operator weights
  uint x_pStrip = 0;                 // Relative weight of new structures by stripple
  uint x_pPerm = 0;                  // Relative weight of new structures by permustrain
  uint x_pAtomic = 0;                // Relative weight of new structures by permutomic
  uint x_pComp = 0;                  // Relative weight of new structures by permucomp
  uint x_pCross = 0;                 // Relative weight of new structures by crossover
  uint x_pSupercell = 0;             // Percent chances of expanding to a random supercell
  // Operator parameters
  double x_stripAmpMin = 0.0;        // Minimum amplitude of periodic displacement
  double x_stripAmpMax = 0.0;        // Maximum amplitude of periodic displacement
  uint x_stripPer1 = 0;              // Number of cosine waves in direction 1
  uint x_stripPer2 = 0;              // Number of cosine waves in direction 2
  double x_stripStrainStdevMin = 0.0; // Minimum standard deviation of epsilon in the
                                      // stripple strain matrix
  double x_stripStrainStdevMax = 0.0; // Maximum standard deviation of epsilon in the
                                      // stripple strain matrix
  uint x_permEx = 0;                 // Number of times atoms are swapped in permustrain
  double x_permStrainStdevMax = 0.0; // Max standard deviation of epsilon in the
                                     // permustrain strain matrix
  uint x_crossNcuts = 0;             // Number of cut points in crossover
  uint x_crossMinimumContribution = 0; // Minimum contribution each parent in crossover
  // Tolerances
  double x_tolXcLength = 0.0;  // XtalComp similarity tolerance: length
  double x_tolXcAngle = 0.0;   // XtalComp similarity tolerance: angle
  double x_tolSpg = 0.0;       // spglib tolerance (default value is in constants.h file)
  double x_tolRdf = 0.98;      // tolerance for RDF similarity (0.0 to 1.0, default = 0.98)
  double x_tolRdfCutoff = 0.0; // distance cutoff for RDF calculations (default = 6.0)
  int x_tolRdfNbins = 0;       // number of bins for RDF calculations (default = 3000)
  double x_tolRdfSigma = 0.0;  // gaussian spread for RDF calculations (default = 0.008)
  // Input strings to be processed for main internal variables
  // NOTE: To keep the state/runtime files shorter; reading and saving
  //   of the chemical formula, reference energies, elemental volumes,
  //   and forced space groups will be "based on a string entry". That's,
  //   for example, we save the "input entry for chemical formulas" as is
  //   to the state file and at the time of resuming a run, we read that and
  //   process it to obtain the actual composition list.
  QString x_inputFormulasString;
  QString x_inputEneRefsString;
  QString x_inputEleVolmString;
  QString x_inputForcedSpgsString;
  QList<CellComp> x_compList;
  QHash<QPair<int, int>, IAD> x_interComp;
  EleRadii x_eleMinRadii;
  EleVolume x_eleVolumes;
  QList<RefEnergy> x_refEnergies;
  QStringList x_moleculeUnitInputs;
  std::vector<Atoms::Geometry> x_moleculeUnits;
  QStringList x_seedList;
  // Spacegroup generation
  // If the number is -1, that spg is not allowed
  // Otherwise, it represents the minimum number of xtals for that spacegroup
  // per formula unit. The spacegroup it represents is index + 1
  QList<int> x_minXtalsOfSpg;


public:
  // Clear molecule units.
  void clearMoleculeUnits()
  {
    moleculeUnitInputs().clear();
    moleculeUnits().clear();
  }

public slots:

  //
  // Search slots
  //

  // Start an active XtalOpt search.
  bool startSearch() override;

  // QueueManager calls this when it needs a new structure.
  void generateNewStructure() override;

  void resetSpacegroups();

  // Clear similarity information for tracked structures.
  void resetSimilarities();

protected:
  friend class XtalOptUnitTest;

  // Counts for planning the initial generation.
  struct InitialGenerationPlan
  {
    InitialGenerationPlan()
      : seedCount(0),
        forcedRandSpgCount(0),
        randomCount(0),
        totalTarget(0)
    {
    }

    uint seedCount;
    uint forcedRandSpgCount;
    uint randomCount;
    uint totalTarget;
    QList<int> randSpgCounts; // list of forced spgs
  };

  //
  // Composition handlers.
  //

  // Build the reference-energy table used for hull calculations.
  std::vector<double> getReferenceEnergiesVector();

  // Returns the composition object for an xtal/structure (considering the full
  //   chemical system, so, might include zero counts!).
  CellComp getXtalComposition(Search::Structure *s);

  // Convert a string of chemical formula to composition object
  CellComp formulaToComposition(QString form);

  // Compare two composition object if they are equivalent/supercell or not
  double compareCompositions(CellComp comp1, CellComp comp2);

  // Get the estimated min/max volume limits for a composition
  void getCompositionVolumeLimits(CellComp incomp, double& vol_min, double& vol_max);

  //
  // Output files and hull movie.
  //

  // Refresh per-structure evaluation data from current objective/hull state.
  bool refreshStructureEvaluationData();

  // Emit notifications that structure evaluation data changed.
  void updateStructureEvaluationInfo();

  // Write the main XtalOpt results file.
  bool writeResultsFile(const QList<Search::Structure*>& structures, bool notify);

  // Record one convex-hull snapshot (the movie option)
  void queueHullSnapshot();

  // The text content of a hull data file for the structures.
  QString hullFileContents(const QList<Search::Structure*>& structures);

  // Write a hull data file for the supplied structures.
  bool writeHullFile(const QList<Search::Structure*>& structures, const QString& filename);

  //
  // Structure generation
  //

  void resetSpacegroups_();

  // Build the initial-generation plan from current settings.
  void buildInitialGenerationPlan(InitialGenerationPlan& plan,
                                  bool reportWarnings = true);

  // Generate and register the initial structures for a fresh search.
  bool generateInitialStructures();

  void generateNewStructure_();

  Xtal* generateNewXtal(CellComp incomp);

  Xtal* generateSuperCell(Xtal* parentXtal, uint expansion, bool distort);

  void initializeAndAddXtal(Xtal* xtal, unsigned int generation,
                             const QString& parents);

  // Check and add a generated initial structure.
  bool acceptInitialXtal(Xtal* generated);

  CellComp pickRandomCompositionFromPossibleOnes();

  Xtal* selectXtalFromProbabilityList();

  //
  // Similarity check handlers.
  //

  void resetSimilarities_();

  void checkForSimilarities();

  void checkForSimilarities_();

  //
  // Optimizer/Queue Template stuff.
  //

  // Register XtalOpt "queue/optimizer"s.
  void registerXtalOptOptimizerAndQueue();

  // Register XtalOpt-specific template keywords.
  void registerXtalOptKeywords();

  // Report generation progress to the user interface.
  void updateProgressBar(size_t goal, size_t attempted, size_t succeeded);

  Search::SlottedWaitCondition* x_initWC;
};
} // end namespace XtalOpt

#endif

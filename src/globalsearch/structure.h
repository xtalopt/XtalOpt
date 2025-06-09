/**********************************************************************
  Structure - Generic wrapper for Avogadro's molecule class

  Copyright (C) 2009-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef STRUCTURE_H
#define STRUCTURE_H

#include <globalsearch/structures/molecule.h>

#include <QDateTime>
#include <QObject>
#include <QReadWriteLock>

#include <atomic>
#include <vector>

class QTextStream;

namespace GlobalSearch {

/**
 * @class Structure structure.h <globalsearch/structure.h>
 * @brief Generic molecule object.
 * @author David C. Lonie
 *
 * The Structure class provides a generic data object for storing
 * information about a molecule. It derives from GlobalSearch::Molecule,
 * adding new functionality to help with common tasks during a
 * global structure search.
 */
class Structure : public QObject, public GlobalSearch::Molecule
{
  Q_OBJECT

public:
  /**
   * Constructor.
   *
   * @param parent The object parent.
   */
  Structure(QObject* parent = 0);

  /**
   * Copy constructor.
   */
  Structure(const Structure& other);

  /**
   * Move constructor. Calls the move assignment operator.
   */
  Structure(Structure&& other) noexcept;

  /**
   * Explicit copy constructor for Molecules.
   */
  Structure(const GlobalSearch::Molecule& other);

  /**
   * Destructor.
   */
  virtual ~Structure() override;

  /**
   * Assignment operator. Makes a new structure with all Structure
   * specific information copied from \a other.
   */
  Structure& operator=(const Structure& other);

  /**
   * Move assignment operator.
   */
  Structure& operator=(Structure&& other) noexcept;

  /**
   * Assignment operator. Makes a new structure with all Molecule
   * specific information copied from \a other.
   */
  Structure& operator=(const GlobalSearch::Molecule& other);

  /**
   * Enum possible objective calculation outcomes for a structure
   */
  enum ObjectivesState
  {
    /** Objectives are not calculated */
    Os_NotCalculated = 0,
    /** Objectives are calculated, structure should be kept */
    Os_Retain,
    /** Objectives calculated, structure dismissed by a filtration objective */
    Os_Dismiss,
    /** Objective calculations failed (bad or missing output file, ...) */
    Os_Fail
  };

  /**
   * Enum containing possible optimization statuses.
   * @sa setStatus
   * @sa getStatus
   */
  enum State
  {
    /** Structure has completed all optimization steps */
    Optimized = 0,
    /** Structure has completed an optimization step but may still
     * have some to complete. getCurrentOptStep() shows the step
     * that has just completed. */
    StepOptimized,
    /** Structure is waiting to start an optimization
     * step. getCurrentOptStep() shows the step it will start
     * next. */
    WaitingForOptimization,
    /** Structure is currently queued or running an optimization
     * step on the PBS server (if applicable). */
    InProcess,
    /** Structure has just been generated, and has not yet been
     * initialized */
    Empty,
    /** The Structure has completed it's current optimization step,
     * and the results of the calculation are being transferred
     * and applied.*/
    Updating,
    /** The optimization is failing. */
    Error,
    /** The Structure has been submitted to the PBS server, but has
     * not appeared in the queue yet. */
    Submitted,
    /** The Structure has been killed before finishing all
     * optimization steps. */
    Killed,
    /** The Structure has been killed after finishing all
     * optimization steps. */
    Removed,
    /** The Structure has been found to be similar to
     * another. The other structure's information can be found in
     * getSimilarityString(). */
    Similar,
    /** The Structure is about to restart it's current optimization step. */
    Restart,
    /** Structure marked as dismissed in objective calculations. */
    ObjectiveDismiss,
    /** Objective calculations for the structure have failed. */
    ObjectiveFail,
    /** All objective calculations for structure are successfully finished. */
    ObjectiveRetain,
    /** Structure is in the process of objective calculations */
    ObjectiveCalculation
  };

  /** Functions to set or retrieve the "composition validity"
   *  for a structure. Validity here means that the structure
   *  has a chemical composition that matches one of the user's
   *  input formulas list, or -at least- is a supercell of one of
   *  those formulas. So, e.g., a "sub-system" seed structure
   *  will not have a valid composition.
   *  For now, this is being used in genetic operation selection
   *  to filter out structures with "unknown" comp from stripple/permustrain
   *  in a fixed/multi composition search, where we already
   *  know the output of gen opt will not be acceptable.
   *  By default, this is set to true as internally generated
   *  structures match the list (or in vc search are acceptable
   *  anyways). Currently, we only set it to "false" in the
   *  "checkComposition" for off-composition seeds.
   */
  bool hasValidComposition() {return m_hasValidComposition;};
  void setCompositionValidity(bool v) {m_hasValidComposition = v;};

  /** Functions to set or retrieve the distance above hull
   *    (in energy per atom units).
   */
  double getDistAboveHull() const {return m_aboveHull;};
  void   setDistAboveHull(double i) {m_aboveHull = i;};

  /**
   * Set and retrieve the Pareto front index for structure
   */
  int  getParetoFront() const {return m_paretoFront; };
  void setParetoFront(int i)  {m_paretoFront = i; };

  /**
   * Multi-objective read/write functions for a structure
   */
  int          getStrucObjNumber() const       {return m_strucObjValues.size();};
  double       getStrucObjValues(int i) const  {return m_strucObjValues.at(i);};
  int          getStrucObjFailCt() const       {return m_strucObjFailCt;};
  void         setStrucObjValues(double v)     {m_strucObjValues.push_back(v);};
  void         setStrucObjFailCt(int i)        {m_strucObjFailCt = i;};
  void         setStrucObjState(ObjectivesState v) {m_strucObjState = v;};
  ObjectivesState getStrucObjState() const {return m_strucObjState;};
  // we don't reset objective fail count; it might be a redone structure which shouldn't be repeated!
  void          resetStrucObj() {m_strucObjValues.clear(); m_strucObjState = Structure::Os_NotCalculated;};
  //
  void          setStrucObjValuesVec(QList<double> v) {m_strucObjValues = v;};
  QList<double> getStrucObjValuesVec()                {return m_strucObjValues;};
  //
  void resetStrucHistObj() {m_hist_strucObjValues.clear();
    m_hist_strucObjState.clear(); m_hist_strucObjFailCt.clear();};
  //
  int           getStrucHistObjNumber()      {return m_hist_strucObjValues.size();};
  QList<double> getStrucHistObjValues(int i) {return m_hist_strucObjValues.at(i);};
  int           getStrucHistObjFailCt(int i) {return m_hist_strucObjFailCt.at(i);};
  void          setStrucHistObjValues(QList<double> v) {m_hist_strucObjValues.push_back(v);};
  void          setStrucHistObjState(ObjectivesState v) {m_hist_strucObjState.push_back(v);};
  void          setStrucHistObjFailCt(int v) {m_hist_strucObjFailCt.push_back(v);};
  ObjectivesState  getStrucHistObjState(int i) {return m_hist_strucObjState.at(i);};

  /** Whether the Structure has an enthalpy value set.
   * @return true if enthalpy has been set, false otherwise
   * @sa setEnthalpy
   * @sa getEnthalpy
   * @sa setPV
   * @sa getPV
   * @sa setEnergy
   * @sa getEnergy
   */
  bool hasEnthalpy() const { return m_hasEnthalpy; };

  /** Return whether or not this structure has a parent structure saved.
   * @return Returns true if a parent structure is saved, and false if
   * a parent structure is not saved.
   */
  bool hasParentStructure() const
  {
    if (m_parentStructure)
      return true;
    else
      return false;
  }

  /** Return the number of times the structure has
   * had the atoms moved to fix the structure to pass
   * the IAD check.
   *@sa setFixCount
   */
  int getFixCount() const { return m_fixCount; };

  /** Return the energy value of the structure in eV.
   *
   * @return The energy of the structure in eV.
   * @sa setEnthalpy
   * @sa hasEnthalpy
   * @sa setPV
   * @sa getPV
   * @sa setEnergy
   * @sa getEnergy
   */
  double getEnergy() const { return m_energy; };

  /** Return the enthalpy value of the structure.
   *
   * @note If the enthalpy is not set but the energy is set, this
   * function assumes that the system is at zero-pressure and
   * returns the energy.
   *
   * @return The enthalpy of the structure.
   * @sa getEnthalpyPerAtom
   * @sa setEnthalpy
   * @sa hasEnthalpy
   * @sa setPV
   * @sa getPV
   * @sa setEnergy
   * @sa getEnergy
   */
  double getEnthalpy() const
  {
    if (!m_hasEnthalpy)
      return getEnergy();
    return m_enthalpy;
  };

  /** Return the enthalpy per atom value of the structure.
   *
   * @note If the enthalpy is not set but the energy is set, this
   * function assumes that the system is at zero-pressure and
   * returns the energy.
   *
   * @return The enthalpy of the structure.
   * @sa getEnthalpy
   * @sa setEnthalpy
   * @sa hasEnthalpy
   * @sa setPV
   * @sa getPV
   * @sa setEnergy
   * @sa getEnergy
   */
  double getEnthalpyPerAtom() const
  {
    return getEnthalpy() / static_cast<double>(numAtoms());
  }

  /** Returns the value PV term from an enthalpy calculation (H = U
   * + PV) in eV.
   *
   * @return The PV term in eV.
   * @sa getEnthalpy
   * @sa setEnthalpy
   * @sa hasEnthalpy
   * @sa setPV
   * @sa setEnergy
   * @sa getEnergy
   */
  double getPV() const { return m_PV; };

  /** Returns an energetic ranking set by setRank(uint).
   * @return the energetic ranking.
   * @sa setRank
   */
  uint getRank() const { return m_rank; };

  /** Returns the Job ID of the Structure's current running
   * optimization. Returns zero is not running.
   * @return Job ID of the structure's optimization process.
   * @sa setJobID
   */
  uint getJobID() const { return m_jobID; };

  /** Returns the generation number of the structure. Only useful
   * for genetic/evolutionary algorithms.
   * @return Generation number
   * @sa setGeneration
   * @sa getIDNumber
   * @sa getIndex
   * @sa setIDNumber
   * @sa setIndex
   * @sa getTag
   */
  uint getGeneration() const { return m_generation; };

  /** Returns an ID number associated with the Structure.
   *
   * @note If a generation number is used as well, this may not be
   * unique.
   * @return Identification number
   * @sa setGeneration
   * @sa getGeneration
   * @sa getIndex
   * @sa setIDNumber
   * @sa setIndex
   * @sa getTag
   */
  uint getIDNumber() const { return m_id; };

  /** Returns a unique ID number associated with the Structure. This
   * is typically assigned in order of introduction to a tracker.
   *
   * @return Unique identification number
   * @sa setGeneration
   * @sa getGeneration
   * @sa setIndex
   * @sa setIDNumber
   * @sa getIDNumber
   * @sa getTag
   */
  int getIndex() const { return m_index; };

  /**
   * Provides locking. Should be used before reading or writing to the
   * structure.
   */
  QReadWriteLock& lock() { return m_lock; };

  /** @return A string naming the Structure that this Structure is
   * similar to.
   * @sa setSimilarityString
   */
  QString getSimilarityString() const { return m_simString; };

  /** @return a string describing the ancestory of the Structure.
   * @sa setParents
   */
  QString getParents() const { return m_parents; };

  /** @return The path on the remote server for the Structure
   * @sa setRempath
   */
  QString getRempath() const { return m_rempath; };

  /** @return The local path of the structure.
   * @sa setLocpath
   */
  QString getLocpath() const { return m_locpath; };

  /** @return The current status of the Structure.
   * @sa setStatus
   * @sa State
   */
  State getStatus() const { return m_status; };

  /** @return The current optimization step of the Structure.
   * @sa setCurrentOptStep
   */
  uint getCurrentOptStep() { return m_currentOptStep; };

  /** @return The number of times this Structure has failed the
   * current optimization step.
   * @sa setFailCount
   * @sa addFailure
   * @sa resetFailCount
   */
  uint getFailCount() { return m_failCount; };

  // Calculate and return the number of formula units
  uint getFormulaUnits() const;

  /** @return A pointer for the parent structure of a given structure
   */
  Structure* getParentStructure() const { return m_parentStructure; };

  /** @return The time that the current optimization step started.
  * @sa getOptTimerEnd
  * @sa startOptTimer
  * @sa stopOptTimer
  * @sa setOptTimerStart
  * @sa setOptTimerEnd
  * @sa getOptElapsed
  */
  QDateTime getOptTimerStart() const { return m_optStart; };

  /** @return The time that the current optimization step ended.
   * @sa getOptTimerStart
   * @sa startOptTimer
   * @sa stopOptTimer
   * @sa setOptTimerStart
   * @sa setOptTimerEnd
   * @sa getOptElapsed
   */
  QDateTime getOptTimerEnd() const { return m_optEnd; };

  /** Returns a unique identification string. Defaults to
   * [generation]x[IDNumber]. Handy for debugging/error output.
   * @return Unique identification string.
   * @sa setGeneration
   * @sa getGeneration
   * @sa setIndex
   * @sa getIndex
   * @sa setIDNumber
   * @sa getIDNumber
   */
  QString getTag() const
  {
    return tr("%1x%2").arg(getGeneration()).arg(getIDNumber());
  };

  /** @return A header line for a results printout
   * @sa getResultsEntry
   * @sa SearchBase::save
   */
  virtual QString getResultsHeader(int objectives_num) const
  {
    QString out = QString("%1 %2 %3 %4 %5")
      .arg("Rank", 6)
      .arg("Tag", 8)
      .arg("Formula", 12)
      .arg("INDX", 6)
      .arg("Enthalpy", 10);
    for (int i = 0; i< objectives_num; i++)
      out += QString("%1").arg("Objective"+QString::number(i+1), 11);
    out += QString("%1")
      .arg("Status", 11);

    return out;
  }

  /** Add objectives info to history of the structure
   * @param s Structure whose objective-related info is added to history
   */
  void updateAndAddObjectivesToHistory(Structure* s);

  /** This function is changed for multi-objective case // SH
   * @return A structure-specific entry for a results printout
   * @sa getResultsHeader
   * @sa SearchBase::save
   */
  virtual QString getResultsEntry(int objectives_num, int optstep,
                                  QList<QString> chemSys) const;

  /** Find the smallest separation between all atoms in the
   * Structure.
   *
   * @return true if the operation makes sense for this Structure,
   * false otherwise (i.e. fewer than two atoms present)
   *
   * @param list list of distances in Angstrom
   * @sa getNearestNeighborDistance
   * @sa getNearestNeighborHistogram
   * @sa getNeighbors
   */
  virtual bool getNearestNeighborDistances(QList<double>* list) const;

  /** Return a list of nearest neighbor distances for each atom in
   * the Structure
   *
   * @return true if the operation makes sense for this Structure,
   * false otherwise (i.e. fewer than two atoms present)
   *
   * @param shortest An empty double to be overwritten with the
   * shortest interatomic distance.
   * @sa getNearestNeighborDistance
   * @sa getNearestNeighborHistogram
   * @sa getNeighbors
   */
  virtual bool getShortestInteratomicDistance(double& shortest) const;

  /** Find the distance to the nearest atom from a specified
   * point.
   *
   * Useful for checking if an atom will be too close to another
   * atom before adding it.
   *
   * @return true if the operation makes sense for this Structure,
   * false otherwise (i.e. fewer than one atom present)
   *
   * @param x Cartesian coordinate
   * @param y Cartesian coordinate
   * @param z Cartesian coordinate
   * @param shortest An empty double to be overwritten with the
   * nearest neighbor distance.
   * @sa getShortestInteratomicDistance
   * @sa getNearestNeighborHistogram
   * @sa getNeighbors
   */
  virtual bool getNearestNeighborDistance(const double x, const double y,
                                          const double z,
                                          double& shortest) const;

  /** Find the nearest neighbor distance of a specified atom.
   *
   * @return true if the operation makes sense for this Structure,
   * false otherwise (i.e. fewer than one atom present)
   *
   * @param atom Atom of interest
   * @param shortest An empty double to be overwritten with the
   * nearest neighbor distance.
   * @sa getShortestInteratomicDistance
   * @sa getNearestNeighborHistogram
   * @sa getNeighbors
   */
  virtual bool getNearestNeighborDistance(const GlobalSearch::Atom& atom,
                                          double& shortest) const;

  /**
   * @return a list of all atoms within \a cutoff of
   * (\a x,\a y,\a z) and, optionally, their \a distances.
   */
  QList<GlobalSearch::Atom> getNeighbors(const double x, const double y,
                                         const double z, const double cutoff,
                                         QList<double>* distances = 0) const;

  /**
   * @overload
   *
   * @return a list of all atoms within \a cutoff of \a atom and,
   *  optionally, their \a distances.
   */
  QList<GlobalSearch::Atom> getNeighbors(const GlobalSearch::Atom& atom,
                                         const double cutoff,
                                         QList<double>* distances = 0) const;

  /** Get the default histogram data.
   */
  virtual void getDefaultHistogram(QList<double>* dist,
                                   QList<double>* freq) const;

  /** Get the default histogram data.
   */
  virtual void getDefaultHistogram(QList<QVariant>* dist,
                                   QList<QVariant>* freq) const;

  /**
   * @return True is histogram generation is pending.
   */
  virtual bool isHistogramGenerationPending() const
  {
    return m_histogramGenerationPending;
  };

  /** Generate data for a histogram of the distances between all
   * atoms, or between one atom and all others.
   *
   * If the parameter atom is specified, the resulting data will
   * represent the distance distribution between that atom and all
   * others. If omitted (or nullptr), a histogram of all interatomic
   * distances is calculated.
   *
   * Useful for estimating the coordination number of an atom from
   * a plot.
   *
   * @warning This algorithm is not thoroughly tested and should not
   * be relied upon. It is merely an estimation.
   *
   * @return true if the operation makes sense for this Structure,
   * false otherwise (i.e. fewer than one atom present)
   *
   * @param distance List of distance values for the histogram
   * bins.
   *
   * @param frequency Number of Atoms within the corresponding
   * distance bin.
   *
   * @param min Value of starting histogram distance.
   * @param max Value of ending histogram distance.
   * @param step Increment between bins.
   * @param atom Optional: Atom to calculate distances from.
   *
   * @sa getShortestInteratomicDistance
   * @sa requestHistogramGeneration
   * @sa getNearestNeighborDistance
   */
  virtual bool generateIADHistogram(
    QList<double>* distance, QList<double>* frequency, double min = 0.0,
    double max = 10.0, double step = 0.01,
    const GlobalSearch::Atom& atom = Atom()) const;

  /** Generate data for a histogram of the distances between all
   * atoms, or between one atom and all others.
   *
   * If the parameter atom is specified, the resulting data will
   * represent the distance distribution between that atom and all
   * others. If omitted (or nullptr), a histogram of all interatomic
   * distances is calculated.
   *
   * Useful for estimating the coordination number of an atom from
   * a plot.
   *
   * @warning This algorithm is not thoroughly tested and should not
   * be relied upon. It is merely an estimation.
   *
   * @return true if the operation makes sense for this Structure,
   * false otherwise (i.e. fewer than one atom present)
   *
   * @param distance List of distance values for the histogram
   * bins.
   *
   * @param frequency Number of Atoms within the corresponding
   * distance bin.
   *
   * @param min Value of starting histogram distance.
   * @param max Value of ending histogram distance.
   * @param step Increment between bins.
   * @param atom Optional: Atom to calculate distances from.
   *
   * @sa getShortestInteratomicDistance
   * @sa requestHistogramGeneration
   * @sa getNearestNeighborDistance
   */
  virtual bool generateIADHistogram(
    QList<QVariant>* distance, QList<QVariant>* frequency, double min = 0.0,
    double max = 10.0, double step = 0.01,
    const GlobalSearch::Atom& atom = Atom()) const;

  /** Add an atom to a random position in the Structure. If no other
   * atoms exist in the Structure, the new atom is placed at
   * (0,0,0).
   *
   * @return true if the atom was sucessfully added within the
   * specified interatomic distances.
   *
   * @param atomicNumber Atomic number of atom to add.
   *
   * @param minIAD Smallest interatomic distance allowed (nullptr or
   * omit for no limit)
   *
   * @param maxIAD Largest interatomic distance allowed (nullptr or
   * omit for no limit)
   *
   * @param maxAttempts Maximum number of tries before giving up.
   */
  virtual bool addAtomRandomly(uint atomicNumber, double minIAD = 0.0,
                               double maxIAD = 0.0, int maxAttempts = 1000);
  /**
   * Use the atoms' covalent radii to automatically generate bonds.
   */
  void perceiveBonds();

  /** The collection of functions to calculate and set, clear, and retrieve the
   * "Normalized pairwise" RDF vector for the structure. The normalization
   * of pairwise entries is done so they are ready for dot product.
   *
   * This normalized RDF object is a "nbin*nelem*nelem" size matrix; however
   * the relevant pairwise entries are unique pairs in their natural order.
   * For examples:
   * 1) For a ternary system such as "A-B-C", the relevant entries are:
   * "distance" "AA" "AB" "AC" "BB" "BC" "CC".
   * 2) For a quaternary "A-B-C-D", they are:
   * "distance" "AA" "AB" "AC" "AD" "BB" "BC" "BD" "CC" "CD" "DD".
   *
   * So, to read the meaningful RDF columns, these kind of loops are needed:
   *     i=0,nbins -> j=0,nelem -> k=j,nelem ---> rdf[i][j][k]
   */
  std::vector<std::vector<std::vector<double> > >
       getNormalizedRDF() const { return m_norm_rdf; }
  void clearNormalizedRDF() {m_norm_rdf.clear();}
  bool hasNormalizedRDF() {return (!m_norm_rdf.empty());}
  bool calculateNormalizedRDF(int nbins, double cutoff, double sigma);

  /** @return An alphabetized list of the atomic symbols for the
   * atomic species present in the Structure.
   * @sa getNumberOfAtomsAlpha
   */
  QList<QString> getSymbols() const;

  /** @return The number of atoms of species
   * given by the variable s.
   */
  uint getNumberOfAtomsOfSymbol(QString s) const;

  /** @return A list of the number of species present that
   * corresponds to the symbols listed in getSymbols().
   * @sa getSymbols
   */
  QList<uint> getNumberOfAtomsAlpha() const;

  /** @return The string with chemical formula
   */
  QString getChemicalFormula() const;

  /** Return the "unique" composition of the structure
   * With default true argument, the output is "empirical composition",
   * otherwise, it will be exact atom counts.
   */
  QString getCompositionString(bool reduceToEmpirical = true) const;

  /** @return Fractional atom coordinates. The atoms are ordered in
   * the same ordering you would get from getSymbols().
   */
  QList<Vector3> getAtomCoordsFrac() const;

  /** @return A string formatted "HH:MM:SS" indicating the amount of
   * time spent in the current optimization step
   *
   * @sa setOptTimerStart
   * @sa getOptTimerStart
   * @sa setOptTimerEnd
   * @sa getOptTimerEnd
   * @sa startOptTimer
   * @sa stopOptTimer
   */
  QString getOptElapsed() const;

  /** @return Get the amount of seconds elapsed as an int.
   */
  int getOptElapsedSeconds() const;

  /** @return Get the amount of hours elapsed as a double.
   */
  double getOptElapsedHours() const;

  /** A "fingerprint" hash of the structure. Returns "enthalpy" key
   * with the enthalpy value as a double wrapped in a QVariant. May
   * be extended in derived classes.
   *
   * Used for checking if two Structures are similar enough to be
   * marked as similarities.
   *
   * @return A hash of key/value pairs containing data that is
   * representative of the Structure.
   */
  virtual QHash<QString, QVariant> getFingerprint() const;

  /**
   * Structure can track if it has changed since it was last checked
   * in a similarity finding routine. This is useful for cutting down
   * on the number of comparisons needed.
   *
   * Must call setupConnections() before using this function.
   * @sa setChangedSinceSimChecked()
   */
  bool hasChangedSinceSimChecked() { return m_updatedSinceSimChecked; };

  /** Structure tracks if it has been primitive-checked or not. Primitive
   * checking involves running the primitive reduction function to see if
   * a smaller FU primitive structure can be made.
   *
   * @sa setPrimitiveChecked()
   */
  bool wasPrimitiveChecked() const { return m_primitiveChecked; };

  /** If the structure was created by primitive reduction, then it does
   * not proceed through the optimizer. This bool indicates if it was created
   * by primitive reduction.
   *
   * @sa setSkippedOptimization()
   */
  bool skippedOptimization() const { return m_skippedOptimization; };

  /** Sort the listed structures by their enthalpies
   *
   * @param structures List of structures to sort
   * @sa sortByAboveHull
   * @sa sortAndRankStructures
   */
  static void sortByEnthalpyPerAtom(QList<Structure*>* structures);

  /** Sort the listed structures by their distance above hull
   *
   * @param structures List of structures to sort
   * @sa sortByEnthalpyPerAtom
   * @sa sortAndRankStructures
   */
  static void sortByDistanceAboveHull(QList<Structure*>* structures);

  /** Sort and rank the listed structures by their above hull (formerly: enthalpy)
   *
   * @param structures List of structures to sort and assign rank
   * @sa sortByEnthalpyPerAtom
   * @sa sortByDistanceAboveHull
   * @sa setRank
   * @sa getRank
   */
  static void sortAndRankStructures(QList<Structure*>* structures);

  /**
   * Get the extra files to be copied to the working dir.
   *
   * @return A vector of the names (including paths) of files to be
   *         copied to the structure's working directory.
   */
  std::vector<std::string> copyFiles() const { return m_copyFiles; }

  /**
   * Append a file to be copied to this structure's working directory.
   * This will only append the file if it does not already exist, so it
   * is safe to append the same file multiple times.
   *
   * @param f The name of the file to be copied.
   */
  void appendCopyFile(const std::string& f)
  {
    if (std::find(m_copyFiles.begin(), m_copyFiles.end(), f) ==
        m_copyFiles.end()) {
      m_copyFiles.push_back(f);
    }
  }

  /**
   * Clears the list of files to be copied to a structure's working dir.
   */
  void clearCopyFiles() { m_copyFiles.clear(); }

  /**
   * Whether or not to re-use the bonding information from before the
   * optimization occurred. The atom order after optimization MUST be
   * the same as the input atom order for optimization.
   *
   * @param b True if we are to use pre-optimization bonding. False if
   *           we should not.
   */
  void setReusePreoptBonding(bool b) { m_reusePreoptBonding = b; }

  /**
   * Whether or not to re-use the bonding information from before the
   * optimization occurred. The atom order after optimization MUST be
   * the same as the input atom order for optimization.
   *
   * @return True if we are to use pre-optimization bonding. False if
   *         we should not.
   */
  bool reusePreoptBonding() { return m_reusePreoptBonding; }

  /**
   * Clear pre-optimization bonding information.
   */
  void clearPreoptBonding() { m_preoptBonds.clear(); }

  /**
   * Set the preopt bonding information to be used when updating the
   * structure.
   *
   * @param v The vector of bonding information via atom indices.
   */
  void setPreoptBonding(const std::vector<Bond>& v) { m_preoptBonds = v; }

  /**
   * Get the vector of preoptimization bonding information. This will be
   * used to assign bonds to be the same as that before optimization. Note
   * that the pre-optimization atom ordering MUST be the same as
   * the post-optimization atom ordering.
   *
   * @return The vector of preoptimization bonding information.
   */
  const std::vector<Bond>& getPreoptBonding() const { return m_preoptBonds; }

signals:

  /**
   * Emitted when a objective's output file is found for a structure
   *
   */
  void objectiveOutputExists();

public slots:

  /**
   * Connect slots/signals within the molecule. This must be called
   * AFTER moving the Structure to it's final thread.
   */
  virtual void setupConnections();

  /**
   * Set whether the default histogram generation should be
   * performed (default is off)
   */
  virtual void enableAutoHistogramGeneration(bool);

  /**
   * Request that histogram data be regenerated. This is connected
   * to Molecule::update() and calls
   * generateDefaultHistogram(). This function is throttled to only
   * run every 250 ms.
   */
  virtual void requestHistogramGeneration();

  /**
   * Generate default histogram data (0:10 A, 0.01 A step)
   * @sa isHistogramGenerationPending()
   * @sa getDefaultHistogram()
   */
  virtual void generateDefaultHistogram();

  /**
   * After calling setupConnections(), this will be called when the
   * structure is update, atoms moved, added, etc...
   */
  virtual void structureChanged();

  /**
   * Compare two IAD histograms.
   *
   * Given two histograms over the same range with the same step,
   * this function calculates an error value to measure the
   * differences between the two. A boxcar smoothing is performed
   * using a width of "smear", and an optional weight can be
   * applied. The weight is a standard exponential decay with a
   * halflife of "decay".
   *
   * @param d List of distances
   * @param f1 First list of frequencies
   * @param f2 Second list of frequencies
   * @param decay Exponential decay parameter for lowering weight of large
   * IADs
   * @param smear Boxcar smoothing width in Angstroms
   * @param error Return error value
   *
   * @return Whether or not the operation could be performed.
   */
  static bool compareIADDistributions(const std::vector<double>& d,
                                      const std::vector<double>& f1,
                                      const std::vector<double>& f2,
                                      double decay, double smear,
                                      double* error);
  /**
   * Compare two IAD histograms.
   *
   * Given two histograms over the same range with the same step,
   * this function calculates an error value to measure the
   * differences between the two. A boxcar smoothing is performed
   * using a width of "smear", and an optional weight can be
   * applied. The weight is a standard exponential decay with a
   * halflife of "decay".
   *
   * @param d List of distances
   * @param f1 First list of frequencies
   * @param f2 Second list of frequencies
   * @param decay Exponential decay parameter for lowering weight of large
   * IADs
   * @param smear Boxcar smoothing width in Angstroms
   * @param error Return error value
   *
   * @return Whether or not the operation could be performed.
   */
  static bool compareIADDistributions(const QList<double>& d,
                                      const QList<double>& f1,
                                      const QList<double>& f2, double decay,
                                      double smear, double* error);

  /**
   * Compare two IAD histograms.
   *
   * Given two histograms over the same range with the same step,
   * this function calculates an error value to measure the
   * differences between the two. A boxcar smoothing is performed
   * using a width of "smear", and an optional weight can be
   * applied. The weight is a standard exponential decay with a
   * halflife of "decay".
   *
   * @param d List of distances
   * @param f1 First list of frequencies
   * @param f2 Second list of frequencies
   * @param decay Exponential decay parameter for lowering weight of large
   * IADs
   * @param smear Boxcar smoothing width in Angstroms
   * @param error Return error value
   *
   * @return Whether or not the operation could be performed.
   */
  static bool compareIADDistributions(const QList<QVariant>& d,
                                      const QList<QVariant>& f1,
                                      const QList<QVariant>& f2, double decay,
                                      double smear, double* error);

  /**
   * Write supplementary data about this Structure to a file. All
   * data that is not stored in the readable optimizer output file
   * should be written here.
   *
   * If reimplementing this in a derived class, call
   * writeStructureSettings(filename) to write inherited data.
   * @param filename Filename to write data to.
   * @sa writeStructureSettings
   * @sa readSettings
   */
  virtual void writeSettings(const QString& filename)
  {
    writeStructureSettings(filename);
  };

  /**
   * Read supplementary data about this Structure from a file. All
   * data that is not stored in the readable optimizer output file
   * should be read here.
   *
   * If reimplementing this in a derived class, call
   * readStructureSettings(filename) to read inherited data.
   * @param filename Filename to read data from.
   * @param readCurrentInfo Update the current info of the structure?
   * Note: readCurrentInfo will also set a unit cell. The code may need to
   * be changed slightly for reading current info for non-periodic structures
   *
   * @sa readStructureSettings
   * @sa writeSettings
   */
  virtual void readSettings(const QString& filename,
                            const bool readCurrentInfo = false)
  {
    readStructureSettings(filename, readCurrentInfo);
  };

  /**
   * Update the coordinates, enthalpy and/or energy, and optionally
   * unit cell of the structure, without adding the data to the
   * structure's history.
   *
   * @param atomicNums List of atomic numbers
   * @param coords List of cartesian coordinates
   * @param energy in eV
   * @param enthalpy in eV
   * @param cell Matrix of cell vectors (row vectors)
   */
  virtual void updateAndSkipHistory(const QList<unsigned int>& atomicNums,
                                    const QList<Vector3>& coords,
                                    const double energy = 0,
                                    const double enthalpy = 0,
                                    const Matrix3& cell = Matrix3::Zero());

  /**
   * Update the coordinates, enthalpy and/or energy, and optionally
   * unit cell of the structure, appending the data to the
   * structure's history.
   *
   * @param atomicNums List of atomic numbers
   * @param coords List of cartesian coordinates
   * @param energy in eV
   * @param enthalpy in eV
   * @param cell Matrix of cell vectors (row vectors)
   */
  virtual void updateAndAddToHistory(const QList<unsigned int>& atomicNums,
                                     const QList<Vector3>& coords,
                                     const double energy = 0,
                                     const double enthalpy = 0,
                                     const Matrix3& cell = Matrix3::Zero());

  /**
   * @param index Index of entry to remove from structure's history.
   */
  virtual void deleteFromHistory(unsigned int index);

  /**
   * This function is used to retrieve data from the structure's
   * history. All non-zero pointers will be modified to contain the
   * information at the specified index of the history.
   *
   * @param index Entry in history to return
   *
   * @param atomicNums Pointer to a list that will be filled with
   * atomic numbers. Can be zero if this is not needed.
   *
   * @param coords Pointer to a list that will be filled with
   * cartesian atomic coordinates. Can be zero if this is not
   * needed.
   *
   * @param energy Pointer to a double that will contain the entry's
   * energy in eV. Can be zero if this is not needed.
   *
   * @param enthalpy Pointer to a double that will contain the
   * entry's enthalpy in eV. Can be zero if this is not needed.
   *
   * @param cell Pointer to an Eigen::Matrix3f filled with the unit
   * cell vectors (row vectors). Can be zero if this is not needed.
   *
   * @note If the system is not periodic, the cell matrix will be a
   * zero matrix. Use Matrix3::isZero() to test for a valid
   * cell.
   */
  virtual void retrieveHistoryEntry(unsigned int index,
                                    QList<unsigned int>* atomicNums,
                                    QList<Vector3>* coords, double* energy,
                                    double* enthalpy, Matrix3* cell);

  /**
   * @return Number of history entries available
   */
  virtual unsigned int sizeOfHistory() { return m_histEnergies.size(); };

  /** Set the number of times the structures has
   * had the atoms moved to pass the
   * IAD check.
   * @sa getFixCount
   */
  void setFixCount(int fixCount) { m_fixCount = fixCount; };

  /** Set the energy in eV.
   * @param energy The Structure's energy in eV.
   * @sa getEnergy
   */
  void setEnergy(double energy) { m_energy = energy; };

  /** Set the enthalpy of the Structure.
   * @param enthalpy The Structure's enthalpy
   * @sa getEnthalpy
   */
  void setEnthalpy(double enthalpy)
  {
    m_hasEnthalpy = true;
    m_enthalpy = enthalpy;
  };

  /** Set the PV term of the Structure's enthalpy (see getPV()).
   * @param pv The PV term
   * @sa getPV
   */

  void setPV(double pv) { m_PV = pv; };

  /** Reset the Structure's enthalpy and PV term to zero and clear
   * hasEnthalpy()
   * @sa setEnthalpy
   * @sa getEnthalpy
   * @sa hasEnthalpy
   * @sa setPV
   * @sa getPV
   */
  void resetEnthalpy()
  {
    m_enthalpy = 0;
    m_PV = 0;
    m_hasEnthalpy = false;
  };

  /** Reset the Structure's energy to zero
   * @sa setEnergy
   * @sa getEnergy
   */
  void resetEnergy() { m_energy = 0.0; };

  /** Set the Structure's energetic ranking.
   * @param rank The Structure's energetic ranking.
   * @sa getRank
   */
  void setRank(uint rank) { m_rank = rank; };

  /** Set the Job ID of the current optimization process.
   * @param id The current optimization process's Job ID.
   * @sa getJobID
   */
  void setJobID(uint id) { m_jobID = id; };

  /** Set the generation number of the Structure.
   * @param gen The generation number.
   * @sa setGeneration
   * @sa getGeneration
   * @sa setIndex
   * @sa getIndex
   * @sa setIDNumber
   * @sa getIDNumber
   * @sa getTag
   */
  void setGeneration(uint gen) { m_generation = gen; };

  /** Set the ID number associated with the Structure.
   *
   * @note If a generation number is used as well, this may not be
   * unique.
   * @return Identification number
   * @sa setGeneration
   * @sa getGeneration
   * @sa setIndex
   * @sa getIndex
   * @sa getIDNumber
   * @sa getTag
   */
  void setIDNumber(uint id) { m_id = id; };

  /** Set a unique ID number associated with the Structure. This
   * is typically assigned in order of introduction to a tracker.
   *
   * @note If a generation number is used as well, this may not be
   * unique.
   * @param index Identification number
   * @sa setGeneration
   * @sa getGeneration
   * @sa getIndex
   * @sa setIDNumber
   * @sa getIDNumber
   * @sa getTag
   */
  void setIndex(int index) { m_index = index; };

  /** @param p A string describing the ancestory of the Structure.
   * @sa getParents
   */
  void setParents(const QString& p) { m_parents = p; };

  /** @param p The path on the remote server to the Structure.
   * @sa getRempath
   */
  void setRempath(const QString& p) { m_rempath = p; };

  /** @param p The local path to the structure.
   * @sa getLocpath
   */
  void setLocpath(const QString& p) { m_locpath = p; };

  /** @param status The current status of the Structure.
   * @sa getStatus
   * @sa State
   */
  void setStatus(State status) { m_status = status; };

  /** @param i The current optimization step of the Structure.
   * @sa getCurrentOptStep
   */
  void setCurrentOptStep(uint i) { m_currentOptStep = i; };

  /** @param count The number of times this Structure has failed the
   * current optimization step.
   * @sa addFailure
   * @sa getFailCount
   * @sa resetFailCount
   */
  void setFailCount(uint count) { m_failCount = count; };

  /** Set the parent structure for this structure
   */
  void setParentStructure(Structure* structure)
  {
    m_parentStructure = structure;
  };

  /** Reset the number of times this Structure has failed the
   * current optimization step.
   *
   * @sa addFailure
   * @sa setFailCount
   * @sa getFailCount
   */
  void resetFailCount() { setFailCount(0); };

  /** Increase the number of times this Structure has failed the
   * current optimization step by one.
   *
   * @sa resetFailCount
   * @sa setFailCount
   * @sa getFailCount
   */
  void addFailure() { setFailCount(getFailCount() + 1); };

  /** @param s A string naming the Structure that this Structure is a
   * similar to.
   * @sa getSimilarityString
   */
  void setSimilarityString(const QString& s) { m_simString = s; };

  /**
   * Structure can track if it has changed since it was last checked
   * in a similarity finding routine. This is useful for cutting down
   * on the number of comparisons needed.
   *
   * Must call setupConnections() before using this function.
   * @sa hasChangedSinceSimChecked()
   */
  void setChangedSinceSimChecked(bool b) { m_updatedSinceSimChecked = b; };

  /**
   * Structure tracks if it has been primitive-checked or not. Primitive
   * checking involves running the primitive reduction function to see if
   * a smaller FU primitive structure can be made.
   *
   * @sa wasPrimitiveChecked()
   */
  void setPrimitiveChecked(bool b) { m_primitiveChecked = b; };

  /** If the structure was created by primitive reduction, then it does
   * not proceed through the optimizer. This bool indicates if it was created
   * by primitive reduction.
   *
   * @sa skippedOptimization()
   */
  void setSkippedOptimization(bool b) { m_skippedOptimization = b; };

  /** Record the current time as when the current optimization
   * process started.
   *
   * @sa setOptTimerStart
   * @sa getOptTimerStart
   * @sa setOptTimerEnd
   * @sa getOptTimerEnd
   * @sa stopOptTimer
   * @sa getOptElapsed
   */
  void startOptTimer()
  {
    m_optStart = QDateTime::currentDateTime();
    m_optEnd = QDateTime();
  };

  /** Record the current time as when the current optimization
   * process stopped.
   *
   * @sa setOptTimerStart
   * @sa getOptTimerStart
   * @sa setOptTimerEnd
   * @sa getOptTimerEnd
   * @sa startOptTimer
   * @sa getOptElapsed
   */
  void stopOptTimer()
  {
    if (m_optEnd.isNull())
      m_optEnd = QDateTime::currentDateTime();
  };

  /** @param d The time that the current optimization
   * process started.
   *
   * @sa getOptTimerStart
   * @sa setOptTimerEnd
   * @sa getOptTimerEnd
   * @sa startOptTimer
   * @sa stopOptTimer
   * @sa getOptElapsed
   */
  void setOptTimerStart(const QDateTime& d) { m_optStart = d; };

  /** @param d The time that the current optimization
   * process stopped.
   *
   * @sa setOptTimerStart
   * @sa getOptTimerStart
   * @sa getOptTimerEnd
   * @sa startOptTimer
   * @sa stopOptTimer
   * @sa getOptElapsed
   */
  void setOptTimerEnd(const QDateTime& d) { m_optEnd = d; };

  /** Load data into Structure.
   * @attention Do not use this function in new code, as it has been
   * replaced by readSettings. Old code should be rewritten to use
   * readSettings as well.
   * @deprecated Use readSettings instead, and call this only as a
   * backup for outdates .state files
   * @param in QTextStream containing load data.
   * @sa readSettings
   */
  virtual void load(QTextStream& in);

protected slots:
  /**
   * Write data from the Structure class to a file.
   * @param filename Filename to write data to.
   * @sa writeSettings
   * @sa readSettings
   */
  void writeStructureSettings(const QString& filename);

  /**
   * Read data concerning the Structure class from a file.
   * @param filename Filename to read data from.
   * @param readCurrentInfo Update the current info of the structure?
   * Note: this will also set a unit cell. The code may need to be changed
   * slightly for reading current info for non-periodic structures
   *
   * @sa writeSettings
   * @sa readSettings
   */
  void readStructureSettings(const QString& filename,
                             const bool readCurrentInfo = false);

  /**
   * Write current data for a structure to a file.
   * Data includes enthalpy, energy, cell vectors, and atom info
   * @param filename Filename to write data to.
   * @sa writeStructureSettings
   * @sa readStructureSettings
   */
  void writeCurrentStructureInfo(const QString& filename);

  /**
   * Read current data concerning a structure from a file.
   * Data includes enthalpy, energy, cell vectors, and atom info
   * @param filename Filename to read data from.
   * @sa writeSettings
   * @sa readSettings
   */
  void readCurrentStructureInfo(const QString& filename);

protected:

  // Valid composition: false if: (1) sub-system structure
  // (2) a composition not on the list (for FC/MC searches)
  bool m_hasValidComposition;

  // Multi-objective parameters for a structure
  QList<double> m_strucObjValues;
  int           m_strucObjFailCt;
  std::atomic<ObjectivesState> m_strucObjState;
  // Objective history parameters
  QList<QList<double>> m_hist_strucObjValues;
  QList<int>           m_hist_strucObjFailCt;
  QList<ObjectivesState> m_hist_strucObjState;

  // skip Doxygen parsing
  /// \cond
  bool m_hasEnthalpy;
  std::vector<std::vector<std::vector<double> > > m_norm_rdf;
  std::atomic_bool m_updatedSinceSimChecked, m_primitiveChecked,
    m_skippedOptimization;
  bool m_histogramGenerationPending;
  uint m_generation, m_id, m_rank, m_jobID, m_currentOptStep, m_failCount,
    m_fixCount;
  QString m_parents, m_simString, m_rempath, m_locpath;
  double m_energy, m_enthalpy, m_PV, m_paretoFront, m_aboveHull;
  std::atomic<State> m_status;
  QDateTime m_optStart, m_optEnd;
  int m_index;
  QList<QVariant> m_histogramDist, m_histogramFreq;
  QReadWriteLock m_lock;

  // History
  QList<QList<unsigned int>> m_histAtomicNums;
  QList<double> m_histEnthalpies;
  QList<double> m_histEnergies;
  QList<QList<Vector3>> m_histCoords;
  QList<Matrix3> m_histCells;

  // Pointer to parent structure if one is saved.
  Structure* m_parentStructure;

  // A list of extra files to be copied from their location to this
  // structure's working directory.
  std::vector<std::string> m_copyFiles;

  // Whether or not to use the pre-optimization bonding information to set
  // the bonds after optimization. The atoms MUST remain in the same order
  // before and after optimization.
  bool m_reusePreoptBonding;

  // The pre-optimization bonding information.
  std::vector<Bond> m_preoptBonds;

  // End doxygen skip:
  /// \endcond
};

} // end namespace GlobalSearch

#endif

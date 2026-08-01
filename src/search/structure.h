/**********************************************************************
  Structure - Search lifecycle structure object

  Copyright (C) 2009-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef SEARCH_STRUCTURE_H
#define SEARCH_STRUCTURE_H

#include <atoms/geometry.h>

#include <common/matrix.h>

#include <QDateTime>
#include <QObject>
#include <QReadWriteLock>

#include <algorithm>
#include <atomic>
#include <vector>

namespace Search {

/**
 * @class Structure structure.h <search/structure.h>
 * @brief A single structure in the search.
 * @author David C. Lonie
 *
 * The Structure class provides a generic data object for storing
 * information about a structure. It derives from Atoms::Geometry,
 * adding new functionality to help with common tasks during a
 * global structure search.
 */
class Structure : public QObject, public Atoms::Geometry
{
  Q_OBJECT

public:
  Structure(QObject* parent = nullptr);
  // Copies start parentless; see structure.cpp for the double-delete reason.
  Structure(const Structure& other);
  Structure(Structure&& other) noexcept;
  virtual ~Structure() override;
  Structure& operator=(const Structure& other);
  Structure& operator=(Structure&& other) noexcept;

  // Objective calculation status.
  enum ObjectivesState
  {
    Os_NotCalculated = 0,
    Os_Retain,
    Os_Dismiss, // kept only for old state files
    Os_Fail
  };

  // Constraint calculation status.
  enum ConstraintState
  {
    Cs_NotCalculated = 0,
    Cs_Retain,
    Cs_Dismiss,
    Cs_Fail
  };

  /**
   * Structure's status during the search. Saved as integers in structure.state, so
   * do not renumber existing values without converting old state files. Empty
   * is a placeholder, not a runnable queue state.
   * @sa getStatus
   */
  enum State
  {
    /** Completed all optimization steps */
    Optimized = 0,
    /** Completed a step; getCurrentOptStep() is the step just finished */
    StepOptimized = 1,
    /** Waiting to start a step; getCurrentOptStep() is the next step */
    WaitingForOptimization = 2,
    /** Queued or running a step through the queue interface */
    InProcess = 3,
    /** Just generated, not yet initialized */
    Empty = 4,
    /** Step finished; results are being transferred and applied */
    Updating = 5,
    /** The optimization is failing */
    Error = 6,
    /** Submitted but not yet visible in the queue */
    Submitted = 7,
    /** Killed before finishing all steps */
    Killed = 8,
    /** Killed after finishing all steps */
    Removed = 9,
    /** About to restart the current step */
    Restart = 10,
    /** Running objective calculations */
    ObjectiveCalculation = 11,
    /** Running post-optimization processing before finalization */
    Postprocessing = 12,
    /** Running constrained-search calculations */
    ConstraintCalculation = 13,
    /** Dismissed by constraint calculations */
    Dismissed = 14,
    /** Objective calculations failed */
    ObjcFailed = 15,
    /** Constraint calculations failed */
    ConsFailed = 16
  };

  // Pareto front index.
  int  getParetoFront() const {return m_paretoFrontIndex; };
  void setParetoFront(int i)  {m_paretoFrontIndex = i; };

  /**
   * Multi-objective read/write functions for a structure
   */
  int          getStrucObjNumber() const       {return m_strucObjValues.size();};
  double       getStrucObjValues(int i) const  {return m_strucObjValues.at(i);};
  void         setStrucObjValues(double v)     {m_strucObjValues.push_back(v);};
  void         setStrucObjState(ObjectivesState v) {m_strucObjState = v;};
  ObjectivesState getStrucObjState() const {return m_strucObjState;};
  void          resetStrucObj() {m_strucObjValues.clear(); setParetoFront(-1);
                                 m_strucObjState = Structure::Os_NotCalculated;};
  //
  void          setStrucObjValuesVec(QList<double> v) {m_strucObjValues = v;};
  QList<double> getStrucObjValuesVec() const          {return m_strucObjValues;};

  ConstraintState getStrucConstraintState() const { return m_strucConstraintState; }
  void setStrucConstraintState(ConstraintState v) { m_strucConstraintState = v; }
  int getStrucConstraintNumber() const { return m_strucConstraintValues.size(); }
  int getStrucConstraintRedoCount() const { return m_strucConstraintRedoCount; }
  void setStrucConstraintRedoCount(int i) { m_strucConstraintRedoCount = i; }
  double getStrucConstraintValues(int i) const
  {
    return m_strucConstraintValues.at(i);
  }
  void setStrucConstraintValues(double v) { m_strucConstraintValues.push_back(v); }
  void setStrucConstraintValuesVec(QList<double> v)
  {
    m_strucConstraintValues = v;
  }
  QList<double> getStrucConstraintValuesVec() const
  {
    return m_strucConstraintValues;
  }
  void resetStrucConstraint()
  {
    m_strucConstraintValues.clear();
    m_strucConstraintState = Structure::Cs_NotCalculated;
  }

  /** Whether the Structure has an enthalpy value set.
   * @return true if enthalpy has been set, false otherwise
   * @sa setEnthalpy
   * @sa getEnthalpy
   * @sa setPV
   * @sa getPV
   * @sa setEnergy
   * @sa getEnergy
   */
  bool hasEnthalpy() const { return m_hasEnthalpy; }

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
  double getEnergy() const { return m_energy; }

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
    return m_hasEnthalpy ? m_enthalpy : m_energy;
  }
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
  double getPV() const { return m_PV; }

  /** Set the energy in eV.
   * @param energy The Structure's energy in eV.
   * @sa getEnergy
   */
  void setEnergy(double energy) { m_energy = energy; }

  /** Set the enthalpy of the Structure.
   * @param enthalpy The Structure's enthalpy
   * @sa getEnthalpy
   */
  void setEnthalpy(double enthalpy)
  {
    m_hasEnthalpy = true;
    m_enthalpy = enthalpy;
  }
  /** Set the PV term of the Structure's enthalpy (see getPV()).
   * @param pv The PV term
   * @sa getPV
   */
  void setPV(double pv) { m_PV = pv; }

  /** Reset the Structure's energy to zero
   * @sa setEnergy
   * @sa getEnergy
   */
  void resetEnergy() { m_energy = 0.0; }

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
    m_enthalpy = 0.0;
    m_PV = 0.0;
    m_hasEnthalpy = false;
  }
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

  /** Returns a search ranking set by setRank(uint).
   * @return the search ranking.
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
  QReadWriteLock& lock() const { return m_lock; };

  /** @return if this structure is similar to another structure. */
  bool isSimilar() const { return !m_simString.isEmpty(); };

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

  // Check groups of related structure states.
  static bool isQueueTerminalState(State state);
  bool isQueueTerminalState() const;

  static bool isQueueInProgressState(State state);
  bool isQueueInProgressState() const;

  static bool isPostOptimizationCalculationState(State state);
  bool isPostOptimizationCalculationState() const;

  static bool isOptimizedState(State state);
  bool isOptimizedState() const;

  static bool isFailedFinalState(State state);
  bool isFailedFinalState() const;

  static bool isDismissedFinalState(State state);
  bool isDismissedFinalState() const;

  static bool isKilledOrRemovedState(State state);
  bool isKilledOrRemovedState() const;

  static bool isStoppedFinalState(State state);
  bool isStoppedFinalState() const;

  static bool isQueueErrorRecoveryState(State state);
  bool isQueueErrorRecoveryState() const;

  static bool isActiveState(State state);
  bool isActiveState() const;

  static bool isTerminalFailureState(State state);
  bool isTerminalFailureState() const;

  /** @return Display text for a state; the long form is proper for GUI labels.
   *  This does not include queue status details.
   */
  static QString statusText(State state, bool longText = false);
  QString statusText(bool longText = false) const;

  /** @return The current optimization step of the Structure.
   * @sa setCurrentOptStep
   */
  uint getCurrentOptStep() const { return m_currentOptStep; };

  /** @return The number of times this Structure has failed the
   * current optimization step.
   * @sa setFailCount
   * @sa addFailure
   * @sa resetFailCount
   */
  uint getFailCount() const { return m_failCount; };

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

  /** @return Zero-padded directory tag for structure work folders. */
  QString getDirectoryTag() const
  {
    return QString("%1").arg(getGeneration(), 5, 10, QChar('0')) + "x" +
           QString("%1").arg(getIDNumber(), 5, 10, QChar('0'));
  };

  /** @return Generic header line for a results printout.
   *  Application can override this.
   *  @sa getResultsEntry
   */
  virtual QString getResultsHeader(int objectives_num, int objective_offset = 0,
                                   int constraints_num = 0) const
  {
    Q_UNUSED(objective_offset);
    QString out = QString("%1 %2 %3 %4 %5")
      .arg("Rank", 6)
      .arg("Tag", 8)
      .arg("Formula", 12)
      .arg("Index", 6)
      .arg("Enthalpy", 10);
    for (int i = 0; i< objectives_num; i++)
      out += QString("%1").arg("Objc"+QString::number(i+1), 11);
    for (int i = 0; i < constraints_num; i++)
      out += QString("%1").arg(QString("Cons%1").arg(i + 1), 11);
    out += QString("%1")
      .arg("Status", 11);

    return out;
  }

  /** @return Generic fallback results entry. XtalOpt overrides this in Xtal.
   * @sa getResultsHeader */
  virtual QString getResultsEntry(int objectives_num, int optstep, int objective_offset = 0,
                                  int constraints_num = 0) const;

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

  /**
   * Structure can track if it has changed since it was last checked
   * in a similarity finding routine. This is useful for cutting down
   * on the number of comparisons needed.
   *
   * Must call setupConnections() before using this function.
   * @sa setChangedSinceSimChecked()
   */
  bool hasChangedSinceSimChecked() const { return m_updatedSinceSimChecked; };

  /** Sort and rank the listed structures by their first objective
   *
   * Structures without usable data or in non-result states go to the end.
   * @param structures List of structures to sort and assign rank
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
  void setPreoptBonding(const std::vector<Atoms::Bond>& v)
  {
    m_preoptBonds = v;
  }
  /**
   * Get the vector of preoptimization bonding information. This will be
   * used to assign bonds to be the same as that before optimization. Note
   * that the pre-optimization atom ordering MUST be the same as
   * the post-optimization atom ordering.
   *
   * @return The vector of preoptimization bonding information.
   */
  const std::vector<Atoms::Bond>& getPreoptBonding() const
  {
    return m_preoptBonds;
  }


public slots:

  /**
   * Connect slots/signals within the structure. This must be called
   * AFTER moving the Structure to it's final thread.
   */
  virtual void setupConnections();

  /**
   * After calling setupConnections(), this will be called when the
   * structure is update, atoms moved, added, etc...
   */
  virtual void structureChanged();

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
  virtual bool updateAndSkipHistory(const QList<unsigned int>& atomicNums,
                                    const QList<Common::Vector3>& coords,
                                    const double energy = 0,
                                    const double enthalpy = 0,
                                    const Common::Matrix3& cell = Common::Matrix3::Zero());

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
  virtual bool updateAndAddToHistory(const QList<unsigned int>& atomicNums,
                                     const QList<Common::Vector3>& coords,
                                     const double energy = 0,
                                     const double enthalpy = 0,
                                     const Common::Matrix3& cell = Common::Matrix3::Zero());

  /** Update from a Geometry, appending to history. */
  virtual bool updateAndAddToHistory(const Atoms::Geometry& structure, const double energy = 0,
                                     const double enthalpy = 0);

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
   * @param cell Pointer to a Common::Matrix3 filled with the unit
   * cell vectors (row vectors). Can be zero if this is not needed.
   *
   * @note If the system is not periodic, the cell matrix will be a
   * zero matrix. Use Matrix3::isZero() to test for a valid
   * cell.
   */
  virtual void retrieveHistoryEntry(unsigned int index,
                                    QList<unsigned int>* atomicNums,
                                    QList<Common::Vector3>* coords, double* energy,
                                    double* enthalpy, Common::Matrix3* cell);

  /**
   * @return Number of history entries available
   */
  virtual unsigned int sizeOfHistory() { return m_histEnergies.size(); };

  /** Clear all history without changing the current structure. */
  void clearHistory()
  {
    m_histAtomicNums.clear();
    m_histCoords.clear();
    m_histEnergies.clear();
    m_histEnthalpies.clear();
    m_histCells.clear();
  }

  /** Append one history entry without changing the current structure. */
  void appendHistoryEntry(const QList<unsigned int>& atomicNums,
                          const QList<Common::Vector3>& coords, double energy, double enthalpy,
                          const Common::Matrix3& cell)
  {
    m_histAtomicNums.append(atomicNums);
    m_histCoords.append(coords);
    m_histEnergies.append(energy);
    m_histEnthalpies.append(enthalpy);
    m_histCells.append(cell);
  }

  /** Set the number of times the structures has
   * had the atoms moved to pass the
   * IAD check.
   * @sa getFixCount
   */
  void setFixCount(int fixCount) { m_fixCount = fixCount; };

  /** Set the Structure's search ranking.
   * @param rank The Structure's search ranking.
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
  void setStatus(State status)
  {
    if (status != Optimized)
      m_simString.clear();
    m_status = status;
  };

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

protected slots:

protected:

  bool m_hasEnthalpy = false;
  double m_energy = 0.0;
  double m_enthalpy = 0.0;
  double m_PV = 0.0;

  // Multi-objective parameters for a structure
  QList<double> m_strucObjValues;
  QList<double> m_strucConstraintValues;
  int           m_strucConstraintRedoCount;
  std::atomic<ObjectivesState> m_strucObjState;
  std::atomic<ConstraintState> m_strucConstraintState;

  // skip Doxygen parsing
  /// \cond
  std::atomic_bool m_updatedSinceSimChecked;
  uint m_generation, m_id, m_rank, m_jobID, m_currentOptStep, m_failCount,
    m_fixCount;
  QString m_parents, m_simString, m_rempath, m_locpath;
  int m_paretoFrontIndex;
  std::atomic<State> m_status;
  QDateTime m_optStart, m_optEnd;
  std::atomic<int> m_index;
  mutable QReadWriteLock m_lock;

  // History
  QList<QList<unsigned int>> m_histAtomicNums;
  QList<double> m_histEnthalpies;
  QList<double> m_histEnergies;
  QList<QList<Common::Vector3>> m_histCoords;
  QList<Common::Matrix3> m_histCells;

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
  std::vector<Atoms::Bond> m_preoptBonds;

  // End doxygen skip:
  /// \endcond

};

} // end namespace Search

#endif

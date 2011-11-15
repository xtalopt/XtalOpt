/**********************************************************************
  Structure - Generic wrapper for Avogadro's molecule class

  Copyright (C) 2009-2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef STRUCTURE_H
#define STRUCTURE_H

#include <avogadro/molecule.h>
#include <avogadro/atom.h>

#include <openbabel/math/vector3.h>
#include <openbabel/mol.h>
#include <openbabel/generic.h>

#include <QtCore/QDebug>
#include <QtCore/QDateTime>
#include <QtCore/QHash>
#include <QtCore/QTextStream>

#include <vector>

// source: http://en.wikipedia.org/wiki/Electronvolt
#define EV_TO_KJ_PER_MOL 96.4853365
#define KJ_PER_MOL_TO_EV 0.0103642692

namespace GlobalSearch {

  /**
   * @class Structure structure.h <globalsearch/structure.h>
   * @brief Generic molecule object.
   * @author David C. Lonie
   *
   * The Structure class provides a generic data object for storing
   * information about a molecule. It derives from Avogadro::Molecule,
   * adding new functionality to help with common tasks during a
   * global structure search.
   */
  class Structure : public Avogadro::Molecule
  {
    Q_OBJECT

   public:

    /**
     * Constructor.
     *
     * @param parent The object parent.
     */
    Structure(QObject *parent = 0);

    /**
     * Copy constructor.
     */
    Structure(const Structure &other);

    /**
     * Explicit copy constructor for Molecules.
     */
    Structure(const Avogadro::Molecule &other);

    /**
     * Destructor.
     */
    virtual ~Structure();

    /**
     * Assignment operator. Makes a new structure with all Structure
     * specific information copied from \a other.
     * @sa copyStructure
     */
    Structure& operator=(const Structure& other);

    /**
     * Assignment operator. Makes a new structure with all Molecule
     * specific information copied from \a other.
     * @sa copyStructure
     */
    Structure& operator=(const Avogadro::Molecule& other);

    /**
     * Only update this structure's atoms, bonds, and residue information
     * from \a other.
     * @sa operator=
     */
    virtual Structure& copyStructure(const Structure &other);

    /**
     * Enum containing possible optimization statuses.
     * @sa setStatus
     * @sa getStatus
     */
    enum State {
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
      /** The Structure has been found to be a duplicate of
       * another. The other structure's information can be found in
       * getDuplicateString(). */
      Duplicate,
      /** The Structure is about to restart it's current optimization
       * step. */
      Restart,
      /** The Structure is undergoing a preoptimization step. */
      Preoptimizing
    };

    /** @return Whether or not the "best" offspring (an optimized mutation)
     * has been generated for this structure.
     */
    bool hasBestOffspring() const {return m_hasBestOffspring;}

    /** Whether the Structure has an enthalpy value set.
     * @return true if enthalpy has been set, false otherwise
     * @sa setEnthalpy
     * @sa getEnthalpy
     * @sa setPV
     * @sa getPV
     * @sa setEnergy
     * @sa getEnergy
     */
    bool hasEnthalpy()	const {return m_hasEnthalpy;};

    /** Return the energy value of the first conformer in eV. This is
     * a convenience function.
     *
     * @note The energies of the other conformers are still available
     * using energy(int). Be aware that energy(int) returns
     * kcal/mol. The multiplicative factor EV_TO_KCAL_PER_MOL has been
     * defined to aid conversion.
     *
     * @return The energy of the first conformer in eV.
     * @sa setEnthalpy
     * @sa hasEnthalpy
     * @sa setPV
     * @sa getPV
     * @sa setEnergy
     * @sa getEnergy
     */
    double getEnergy()	const {return energy(0) * KJ_PER_MOL_TO_EV;};

    /** Return the enthalpy value of the first conformer in eV.
     *
     * @note If the enthalpy is not set but the energy is set, this
     * function assumes that the system is at zero-pressure and
     * returns the energy.
     *
     * @return The enthalpy of the first conformer in eV.
     * @sa setEnthalpy
     * @sa hasEnthalpy
     * @sa setPV
     * @sa getPV
     * @sa setEnergy
     * @sa getEnergy
     */
    double getEnthalpy()const {if (!m_hasEnthalpy) return getEnergy(); return m_enthalpy;};

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
    double getPV()	const {return m_PV;};

    /** Returns an energetic ranking set by setRank(uint).
     * @return the energetic ranking.
     * @sa setRank
     */
    uint getRank()	const {return m_rank;};

    /** Returns the Job ID of the Structure's current running
     * optimization. Returns zero is not running.
     * @return Job ID of the structure's optimization process.
     * @sa setJobID
     */
    uint getJobID()	const {return m_jobID;};

    /** Returns the generation number of the structure. Only useful
     * for genetic/evolutionary algorithms.
     * @return Generation number
     * @sa setGeneration
     * @sa getIDNumber
     * @sa getIndex
     * @sa setIDNumber
     * @sa setIndex
     * @sa getIDString
     */
    uint getGeneration() const {return m_generation;};

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
     * @sa getIDString
     */
    uint getIDNumber() const {return m_id;};

    /** Returns a unique ID number associated with the Structure. This
     * is typically assigned in order of introduction to a tracker.
     *
     * @return Unique identification number
     * @sa setGeneration
     * @sa getGeneration
     * @sa getIndex
     * @sa setIDNumber
     * @sa getIDNumber
     * @sa getIDString
     */
    int getIndex() const {return m_index;};

    /** @return A string naming the Structure that this Structure is a
     * duplicate of.
     * @sa setDuplicateString
     */
    QString getDuplicateString() const {return m_dupString;};

    /** @return a string describing the ancestory of the Structure.
     * @sa setParents
     */
    QString getParents() const {return m_parents;};

    /** @return The path on the remote server to write this Structure
     * for optimization.
     * @sa setRempath
     */
    QString getRempath() const {return m_rempath;};

    /** @return The current status of the Structure.
     * @sa setStatus
     * @sa State
     */
    State getStatus()	const {return m_status;};

    /** @return The current optimization step of the Structure.
     * @sa setCurrentOptStep
     */
    uint getCurrentOptStep() {return m_currentOptStep;};

    /** @return true if running, false otherwise. */
    virtual bool isPreoptimizing() const {return false;}

    /** @return The percentage completion of the preoptimization step.
      * -1 if @a this is not Preoptimizing.
      */
    virtual int getPreOptProgress() const {return -1;}

    /** @return Whether or not @a this needs to be preoptimized. */
    virtual bool needsPreoptimization() const {return false;}

    /** @return The number of times this Structure has failed the
     * current optimization step.
     * @sa setFailCount
     * @sa addFailure
     * @sa resetFailCount
     */
    uint getFailCount() { return m_failCount;};

    /** @return The time that the current optimization step started.
     * @sa getOptTimerEnd
     * @sa startOptTimer
     * @sa stopOptTimer
     * @sa setOptTimerStart
     * @sa setOptTimerEnd
     * @sa getOptElapsed
     */
    QDateTime getOptTimerStart() const {return m_optStart;};

    /** @return The time that the current optimization step ended.
     * @sa getOptTimerStart
     * @sa startOptTimer
     * @sa stopOptTimer
     * @sa setOptTimerStart
     * @sa setOptTimerEnd
     * @sa getOptElapsed
     */
    QDateTime getOptTimerEnd() const {return m_optEnd;};

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
    QString getIDString() const {
      return tr("%1x%2").arg(getGeneration()).arg(getIDNumber());};

    /** @return A header line for a results printout
     * @sa getResultsEntry
     * @sa OptBase::save
     */
    virtual QString getResultsHeader() const {
      return QString("%1 %2 %3 %4 %5")
        .arg("Rank", 6)
        .arg("Gen", 6)
        .arg("ID", 6)
        .arg("Enthalpy", 10)
        .arg("Status", 11);};

    /** @return A structure-specific entry for a results printout
     * @sa getResultsHeader
     * @sa OptBase::save
     */
    virtual QString getResultsEntry() const;

    /** @return a lookup table for mapping atoms indices between
     * structure index (value) and the optimizer index (key).
     */
    QHash<int, int> * getOptimizerLookupTable()
    {
      return &m_optimizerLookup;
    }

    /** Reset the optimizer lookup table to set the optimizer indicies
     * to the structure indices.
     */
    void resetOptimizerLookupTable()
    {
      m_optimizerLookup.clear();
      for (int i = 0; i < m_atomList.size(); ++i)
        m_optimizerLookup.insert(i,i);
    }

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
    virtual bool getNearestNeighborDistances(QList<double> * list) const;

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
    virtual bool getShortestInteratomicDistance(double & shortest) const;

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
    virtual bool getNearestNeighborDistance(const double x,
                                            const double y,
                                            const double z,
                                            double & shortest) const;

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
    virtual bool getNearestNeighborDistance(const Avogadro::Atom *atom,
                                            double & shortest) const;

    /**
     * @return a list of all atoms within \a cutoff of
     * (\a x,\a y,\a z) and, optionally, their \a distances.
     */
    QList<Avogadro::Atom*> getNeighbors (const double x,
                                         const double y,
                                         const double z,
                                         const double cutoff,
                                         QList<double> *distances = 0) const;

    /**
     * @overload
     *
     * @return a list of all atoms within \a cutoff of \a atom and,
     *  optionally, their \a distances.
     */
    QList<Avogadro::Atom*> getNeighbors (const Avogadro::Atom *atom,
                                         const double cutoff,
                                         QList<double> *distances = 0) const;

    /** Get the default histogram data.
     */
    virtual void getDefaultHistogram(QList<double> *dist, QList<double> *freq) const;

    /** Get the default histogram data.
     */
    virtual void getDefaultHistogram(QList<QVariant> *dist, QList<QVariant> *freq) const;

    /**
     * @return True is histogram generation is pending.
     */
    virtual bool isHistogramGenerationPending() const {
      return m_histogramGenerationPending;};

    /** Generate data for a histogram of the distances between all
     * atoms, or between one atom and all others.
     *
     * If the parameter atom is specified, the resulting data will
     * represent the distance distribution between that atom and all
     * others. If omitted (or NULL), a histogram of all interatomic
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
    virtual bool generateIADHistogram(QList<double> * distance,
                                      QList<double> * frequency,
                                      double min = 0.0,
                                      double max = 10.0,
                                      double step = 0.01,
                                      Avogadro::Atom *atom = 0) const;

    /** Generate data for a histogram of the distances between all
     * atoms, or between one atom and all others.
     *
     * If the parameter atom is specified, the resulting data will
     * represent the distance distribution between that atom and all
     * others. If omitted (or NULL), a histogram of all interatomic
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
    virtual bool generateIADHistogram(QList<QVariant> * distance,
                                      QList<QVariant> * frequency,
                                      double min = 0.0,
                                      double max = 10.0,
                                      double step = 0.01,
                                      Avogadro::Atom *atom = 0) const;

    /** Add an atom to a random position in the Structure. If no other
     * atoms exist in the Structure, the new atom is placed at
     * (0,0,0).
     *
     * @return true if the atom was sucessfully added within the
     * specified interatomic distances.
     *
     * @param atomicNumber Atomic number of atom to add.
     *
     * @param minIAD Smallest interatomic distance allowed (NULL or
     * omit for no limit)
     *
     * @param maxIAD Largest interatomic distance allowed (NULL or
     * omit for no limit)
     *
     * @param maxAttempts Maximum number of tries before giving up.
     *
     * @param atom Returns a pointer to the new atom.
     */
    virtual bool addAtomRandomly(uint atomicNumber,
                                 double minIAD = 0.0,
                                 double maxIAD = 0.0,
                                 int maxAttempts = 1000,
                                 Avogadro::Atom **atom = 0);

    /** @return An alphabetized list of the atomic symbols for the
     * atomic species present in the Structure.
     * @sa getNumberOfAtomsAlpha
     */
    QList<QString> getSymbols() const;

    /** @return A list of the number of species present that
     * corresponds to the symbols listed in getSymbols().
     * @sa getSymbols
     */
    QList<uint> getNumberOfAtomsAlpha() const;

    /** @return A string formated "HH:MM:SS" indicating the amount of
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

    /** A "fingerprint" hash of the structure. Returns "enthalpy" key
     * with the enthalpy value as a double wrapped in a QVariant. May
     * be extended in derived classes.
     *
     * Used for checking if two Structures are similar enough to be
     * marked as duplicates.
     *
     * @return A hash of key/value pairs containing data that is
     * representative of the Structure.
     */
    virtual QHash<QString, QVariant> getFingerprint() const;

    /**
     * Structure can track if it has changed since it was last checked
     * in a duplicate finding routine. This is useful for cutting down
     * on the number of comparisons needed.
     *
     * Must call setupConnections() before using this function.
     * @sa setChangedSinceDupChecked()
     */
    bool hasChangedSinceDupChecked() {return m_updatedSinceDupChecked;};

    /** Sort the listed structures by their enthalpies
     *
     * @param structures List of structures to sort
     * @sa rankEnthalpies
     * @sa sortAndRankByEnthalpy
     */
    static void sortByEnthalpy(QList<Structure*> *structures);

    /** Rank the listed structures by their enthalpies
     *
     * @param structures List of structures to assign ranks
     * @sa sortEnthalpies
     * @sa sortAndRankByEnthalpy
     * @sa setRank
     * @sa getRank
     */
    static void rankByEnthalpy(const QList<Structure*> &structures);

    /** Sort and rank the listed structures by their enthalpies
     *
     * @param structures List of structures to sort and assign rank
     * @sa sortByEnthalpy
     * @sa rankEnthalpies
     * @sa setRank
     * @sa getRank
     */
    static void sortAndRankByEnthalpy(QList<Structure*> *structures);

  signals:
    void preoptimizationStarted();
    void preoptimizationFinished();

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
    static bool compareIADDistributions(const std::vector<double> &d,
                                        const std::vector<double> &f1,
                                        const std::vector<double> &f2,
                                        double decay,
                                        double smear,
                                        double *error);
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
    static bool compareIADDistributions(const QList<double> &d,
                                        const QList<double> &f1,
                                        const QList<double> &f2,
                                        double decay,
                                        double smear,
                                        double *error);

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
    static bool compareIADDistributions(const QList<QVariant> &d,
                                        const QList<QVariant> &f1,
                                        const QList<QVariant> &f2,
                                        double decay,
                                        double smear,
                                        double *error);

    /**
     * Write supplementary data about this Structure to a file. All
     * data that is not stored in the OpenBabel-readable optimizer
     * output file should be written here.
     *
     * If reimplementing this in a derived class, call
     * writeStructureSettings(filename) to write inherited data.
     * @param filename Filename to write data to.
     * @sa writeStructureSettings
     * @sa readSettings
     */
    virtual void writeSettings(const QString &filename) {
      writeStructureSettings(filename);};

    /**
     * Read supplementary data about this Structure from a file. All
     * data that is not stored in the OpenBabel-readable optimizer
     * output file should be read here.
     *
     * If reimplementing this in a derived class, call
     * readStructureSettings(filename) to read inherited data.
     * @param filename Filename to read data from.
     * @sa readStructureSettings
     * @sa writeSettings
     */
    virtual void readSettings(const QString &filename) {
      readStructureSettings(filename);};

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
    virtual void updateAndSkipHistory(const QList<unsigned int> &atomicNums,
                                      const QList<Eigen::Vector3d> &coords,
                                      const double energy = 0,
                                      const double enthalpy = 0,
                                      const Eigen::Matrix3d &cell = Eigen::Matrix3d::Zero());

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
    virtual void updateAndAddToHistory(const QList<unsigned int> &atomicNums,
                                       const QList<Eigen::Vector3d> &coords,
                                       const double energy = 0,
                                       const double enthalpy = 0,
                                       const Eigen::Matrix3d &cell = Eigen::Matrix3d::Zero());

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
     * zero matrix. Use Eigen::Matrix3d::isZero() to test for a valid
     * cell.
     */
    virtual void retrieveHistoryEntry(unsigned int index,
                                      QList<unsigned int> *atomicNums,
                                      QList<Eigen::Vector3d> *coords,
                                      double *energy,
                                      double *enthalpy,
                                      Eigen::Matrix3d *cell);

      /**
       * @return Number of history entries available
       */
    virtual unsigned int sizeOfHistory() {return m_histEnergies.size();};

    /** @param b Whether or not the "best" offspring (an optimized mutation)
     * has been generated for this structure.
     */
    void setHasBestOffspring(bool b = true)
    {
      m_hasBestOffspring = b;
    }

    /** Set the enthalpy of the Structure.
     * @param enthalpy The Structure's enthalpy
     * @sa getEnthalpy
     */
    void setEnthalpy(double enthalpy) {m_hasEnthalpy = true; m_enthalpy = enthalpy;};

    /** Set the PV term of the Structure's enthalpy (see getPV()).
     * @param pv The PV term
     * @sa getPV
     */
    void setPV(double pv) {m_PV = pv;};

    /** Reset the Structure's enthalpy and PV term to zero and clear
     * hasEnthalpy()
     * @sa setEnthalpy
     * @sa getEnthalpy
     * @sa hasEnthalpy
     * @sa setPV
     * @sa getPV
     */
    void resetEnthalpy() {m_enthalpy=0; m_PV=0; m_hasEnthalpy=false;};

    /** Reset the Structure's energy to zero
     * @sa setEnergy
     * @sa getEnergy
     */
    void resetEnergy() {std::vector<double> E; E.push_back(0); setEnergies(E);};

    /** Determine and set the energy using a forcefield method from
     * OpenBabel.
     * @param ff A string identifying the forcefield to use (default: UFF).
     */
    void setOBEnergy(const QString &ff = QString("UFF"));

    /** Set the Structure's energetic ranking.
     * @param rank The Structure's energetic ranking.
     * @sa getRank
     */
    void setRank(uint rank) {m_rank = rank;};

    /** Set the Job ID of the current optimization process.
     * @param id The current optimization process's Job ID.
     * @sa getJobID
     */
    void setJobID(uint id) {m_jobID = id;};

    /** Set the generation number of the Structure.
     * @param gen The generation number.
     * @sa setGeneration
     * @sa getGeneration
     * @sa setIndex
     * @sa getIndex
     * @sa setIDNumber
     * @sa getIDNumber
     * @sa getIDString
     */
    void setGeneration(uint gen) {m_generation = gen;};

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
     * @sa getIDString
     */
    void setIDNumber(uint id) {m_id = id;};

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
     * @sa getIDString
     */
    void setIndex(int index) {m_index = index;};

    /** @param p A string describing the ancestory of the Structure.
     * @sa getParents
     */
    void setParents(const QString & p) {m_parents = p;};

    /** @param p The path on the remote server to write this Structure
     * for optimization.
     * @sa getRempath
     */
    void setRempath(const QString & p) {m_rempath = p;};

    /** @param status The current status of the Structure.
     * @sa getStatus
     * @sa State
     */
    void setStatus(State status) {m_status = status;};

    /** @param i The current optimization step of the Structure.
     * @sa getCurrentOptStep
     */
    void setCurrentOptStep(uint i) {m_currentOptStep = i;};

    /** @param count The number of times this Structure has failed the
     * current optimization step.
     * @sa addFailure
     * @sa getFailCount
     * @sa resetFailCount
     */
    void setFailCount(uint count) {m_failCount = count;};

    /** Reset the number of times this Structure has failed the
     * current optimization step.
     *
     * @sa addFailure
     * @sa setFailCount
     * @sa getFailCount
     */
    void resetFailCount() {setFailCount(0);};

    /** Increase the number of times this Structure has failed the
     * current optimization step by one.
     *
     * @sa resetFailCount
     * @sa setFailCount
     * @sa getFailCount
     */
    void addFailure() {setFailCount(getFailCount() + 1);};

    /** @param s A string naming the Structure that this Structure is a
     * duplicate of.
     * @sa getDuplicateString
     */
    void setDuplicateString(const QString & s) {m_dupString = s;};

    /**
     * Structure can track if it has changed since it was last checked
     * in a duplicate finding routine. This is useful for cutting down
     * on the number of comparisons needed.
     *
     * Must call setupConnections() before using this function.
     * @sa hasChangedSinceDupChecked()
     */
    void setChangedSinceDupChecked(bool b) {m_updatedSinceDupChecked = b;};

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
    void startOptTimer() {
      m_optStart = QDateTime::currentDateTime(); m_optEnd = QDateTime();};

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
    void stopOptTimer() {
      if (m_optEnd.isNull()) m_optEnd = QDateTime::currentDateTime();};

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
    void setOptTimerStart(const QDateTime &d) {m_optStart = d;};

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
    void setOptTimerEnd(const QDateTime &d) {m_optEnd = d;};


    /** @param b Whether or not @a this needs to be preoptimized. */
    virtual void setNeedsPreoptimization(bool b)
    {
      Q_UNUSED(b);
      qWarning() << "Preoptimization is not implemented for all structure "
                    "types. Call to Structure::setNeedsPreoptimization "
                    "ignored.";
    }

    /** Abort the preoptimization running on this structure. */
    virtual void abortPreoptimization() const {};

    /** Emits the preoptimizationStarted signal */
    void emitPreoptimizationStarted()
    {
      emit preoptimizationStarted();
    }

    /** Emits the preoptimizationFinished signal */
    void emitPreoptimizationFinished()
    {
      emit preoptimizationFinished();
    }

    /** Load data into Structure.
     * @attention Do not use this function in new code, as it has been
     * replaced by readSettings. Old code should be rewritten to use
     * readSettings as well.
     * @deprecated Use readSettings instead, and call this only as a
     * backup for outdates .state files
     * @param in QTextStream containing load data.
     * @sa readSettings
     */
    virtual void load(QTextStream &in);

   protected slots:
    /**
     * Write data from the Structure class to a file.
     * @param filename Filename to write data to.
     * @sa writeSettings
     * @sa readSettings
     */
    void writeStructureSettings(const QString &filename);

    /**
     * Read data concerning the Structure class from a file.
     * @param filename Filename to read data from.
     * @sa writeSettings
     * @sa readSettings
     */
    void readStructureSettings(const QString &filename);

  protected:
    // skip Doxygen parsing
    /// \cond
    bool m_hasEnthalpy, m_updatedSinceDupChecked;
    bool m_histogramGenerationPending;
    bool m_hasBestOffspring;
    uint m_generation, m_id, m_rank, m_jobID, m_currentOptStep, m_failCount;
    QString m_parents, m_dupString, m_rempath;
    double m_enthalpy, m_PV;
    State m_status;
    QDateTime m_optStart, m_optEnd;
    int m_index;
    QList<QVariant> m_histogramDist, m_histogramFreq;

    // History
    QList<QList<unsigned int> > m_histAtomicNums;
    QList<double> m_histEnthalpies;
    QList<double> m_histEnergies;
    QList<QList<Eigen::Vector3d> > m_histCoords;
    QList<Eigen::Matrix3d> m_histCells;

    // Map <Structure atom index, optimizer atom index>
    QHash<int, int> m_optimizerLookup;

    // End doxygen skip:
    /// \endcond
  };

} // end namespace GlobalSearch

#endif

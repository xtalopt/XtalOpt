/**********************************************************************
  Geometry - Reusable structure/geometry API.

  Copyright (C) 2009-2011 by David C. Lonie
  Copyright (C) 2016-2017 by Patrick S. Avery
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef ATOMS_GEOMETRY_H
#define ATOMS_GEOMETRY_H

#include <atoms/basis/atom.h>
#include <atoms/basis/bond.h>
#include <atoms/basis/unitcell.h>
#include <common/constants.h>

#include <QList>
#include <QString>

#include <cassert>
#include <cstddef>
#include <map>
#include <type_traits>
#include <utility>
#include <vector>

namespace Atoms {

// Convert between the row cell matrix and the column lattice array (eg, used by spglib).
void cellToColumnLatticeArray(const Common::Matrix3& cell, double lattice[3][3]);
Common::Matrix3 columnLatticeArrayToCell(const double lattice[3][3]);

/**
 * @class Geometry geometry.h
 * @brief A structure containing atoms, bonds, and a unit cell.
 *
 * Geometry may be a 3D crystal or a 0D molecule. Crystal functions check for
 * a 3D cell. Distance and RDF functions also work for molecules; they just
 * do not use periodic images.
 */
class Geometry
{
public:
  explicit Geometry(const std::vector<Atom>& atoms = std::vector<Atom>(),
                    const UnitCell& unitCell = UnitCell())
    : g_atoms(atoms), g_bonds(), g_unitCell(unitCell)
  {
  }

  virtual ~Geometry() = default;
  Geometry(const Geometry& other) = default;
  Geometry(Geometry&& other) = default;
  Geometry& operator=(const Geometry& other) = default;
  Geometry& operator=(Geometry&& other) = default;

  // Conversion convenience. A non-periodic cell gives a zero vector.
  Common::Vector3 fracToCart(const Common::Vector3& v) const {
    return unitCell().toCartesian(v);
  }

  Common::Vector3 cartToFrac(const Common::Vector3& v) const {
    return unitCell().toFractional(v);
  }

  // Cell data. These functions make a 3D cell.
  void setCellInfo(double a, double b, double c,
                   double alpha, double beta, double gamma);
  void setCellInfo(const Common::Matrix3& m) {
    g_unitCell.setCellMatrix(m);
    clearGeometryCaches();
  }

  void setCellInfo(const Common::Vector3& a, const Common::Vector3& b, const Common::Vector3& c);
  void setVolume(double volume);

  // rescale cell can be used to "fix" any cell parameter at a particular value.
  // Simply pass the fixed values and use "0" for any non-fixed parameters.
  // Volume will be preserved.
  void rescaleCell(double a, double b, double c,
                   double alpha, double beta, double gamma);

  // Rotate the cell vectors and atomic coordinates so that v1 is parallel
  // to x and v2 is in the xy plane
  bool rotateCellAndCoordsToStandardOrientation();
  static bool rotateCellAndCoordsToStandardOrientation(Common::Matrix3& cell,
                                                       QList<Common::Vector3>& fractionalCoordinates,
                                                       bool positiveHandedness);

  // Calculate the matrix used in the above function. Matrix has row vectors.
  // If the current cell cannot be rotated in a numerically stable
  // manner, this will return Matrix3::Zeros;
  Common::Matrix3 getCellMatrixInStandardOrientation() const;

  void wrapAtomsToCell();

  // Move bonded atoms to their shortest periodic images. 3D only. Atoms may
  // end up outside the cell after this.
  void wrapBondedComponentsToSmallestBonds();

  /** Return a list of nearest neighbor distances for each atom in
   * the Structure
   *
   * @return true if the operation makes sense for this Structure,
   * false otherwise (i.e. fewer than two atoms present)
   *
   * @param shortest An empty double to be overwritten with the
   * shortest interatomic distance.
   * @sa getNearestNeighborDistance
   * @sa generateIADHistogram
   * @sa getNeighbors
   */
  virtual bool getShortestInteratomicDistance(double& shortest) const;

  // Find the shortest distance for each pair of atom types. Missing pairs are zero.
  bool getShortestInteratomicDistancesBySpecies(QList<QString>& symbol1,
                                                QList<QString>& symbol2,
                                                QList<double>& distance) const;

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
   * @sa generateIADHistogram
   * @sa getNeighbors
   */
  virtual bool getNearestNeighborDistance(double x, double y, double z, double& shortest) const;

  /** Generate data for a histogram of the distances between all
   * atoms, or between one atom and all others.
   *
   * If the parameter atom is specified, the resulting data will
   * represent the distance distribution between that atom and all
   * others. If omitted, a histogram of all interatomic distances
   * is calculated.
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
   * @param distance List of distance values for the histogram bins.
   * @param frequency Number of Atoms within the corresponding distance bin.
   * @param min Value of starting histogram distance.
   * @param max Value of ending histogram distance.
   * @param step Increment between bins.
   * @param atom Optional: Atom to calculate distances from.
   *
   * @sa getShortestInteratomicDistance
   * @sa getNearestNeighborDistance
   */
  virtual bool generateIADHistogram(std::vector<double>* distance, std::vector<double>* frequency,
                                    double min = 0.0, double max = 10.0, double step = 0.01,
                                    const Atom& atom = Atom()) const;

  bool getSquaredAtomicDistancesToPoint(const Common::Vector3& coord,
                                        QList<double>* distances) const;

  /** Add an atom to a random position in the Structure. If no other
   * atoms exist in the Structure, the new atom is placed at
   * (0,0,0).
   *
   * @return true if the atom was sucessfully added within the
   * specified interatomic distances.
   *
   * @param atomicNumber Atomic number of atom to add.
   *
   * @param minIAD Smallest interatomic distance allowed. A negative
   * value disables this check.
   *
   * @param maxAttempts Maximum number of tries before giving up.
   */
  virtual bool addAtomRandomly(unsigned int atomicNumber, double minIAD = 0.0,
                               int maxAttempts = 1000);

  QList<QString> getAtomicSymbolsInOrder() const;

  /** @return Fractional atom coordinates. The atoms are ordered in
   * the same ordering you would get from getSymbols().
   */
  std::vector<Common::Vector3> getAtomCoordsFrac() const;

  bool compareXtalComp(const Geometry& other, double lengthTol = 0.1, double angleTol = 2.0) const;

  /** The collection of functions to calculate and set, clear, and retrieve the
   * "Normalized pairwise" RDF vector for the structure. The normalization
   * of pairwise entries is done so they are ready for dot product.
   *
   * The normalized RDF is a flat "float" vector (memory considerations!).
   * Its size is "nbins*npairs" vector, with npairs = nelem*(nelem+1)/2.
   * The vector is bin-major: for each bin, the values for unique pairs
   *   are saved in their natural order (getSymbols() order); eg,
   * 1) For a ternary system such as "A-B-C", the pair entries are:
   * "AA" "AB" "AC" "BB" "BC" "CC" --index--> {0,...,5}.
   * 2) For a quaternary "A-B-C-D", they are:
   * "AA" "AB" "AC" "AD" "BB" "BC" "BD" "CC" "CD" "DD" --index--> {0,...,9}.
   *
   * So, the value for bin "b" and unique pair index "p" is:
   *     rdf[b * npairs + p]
   */
  ///@{
  const std::vector<float>& getNormalizedRDF() const { return g_norm_rdf; }

  void clearNormalizedRDF()
  {
    g_norm_rdf.clear();
    g_norm_rdf_nsymbs = 0;
    g_norm_rdf_nbins = 0;
    g_norm_rdf_cutoff = 0.0;
    g_norm_rdf_sigma = 0.0;
  }

  bool hasNormalizedRDF() const { return !g_norm_rdf.empty(); }
  bool calculateNormalizedRDF(int nbins, double cutoff, double sigma);
  bool calculateTotalNormalizedRDF(int nbins, double cutoff, double sigma,
                                   std::vector<double>& total);
  bool compareRDF(Geometry& other, int nbins, double cutoff,
                  double sigma, double tolerance, double& dotProduct);

  std::vector<std::vector<std::pair<int, double> > >
       getNearestNeighborLists() const { return g_neighbor_list; }

  void clearNearestNeighborLists()
  {
    g_neighbor_list.clear();
    g_neighbor_list_cutoff = 0.0;
  }

  void notifyGeometryChanged() { clearGeometryCaches(); }
  bool hasNearestNeighborLists() const { return !g_neighbor_list.empty(); }
  bool calculateNearestNeighborLists(double cutoff);
  // Find nearest neighbors using cell lists instead of every atom pair.
  bool calculateNearestNeighborListsCellList(double cutoff);
  ///@}

  // Spacegroup
  void findSpaceGroup(double prec = SPGLIB_TOL) const;
  uint getSpaceGroupNumber() const;
  QString getSpaceGroupSymbol() const;
  QString getHTMLSpaceGroupSymbol() const;
  // Static function for getting a Hermann-Mauguin name from a spg number
  static QString getHMName(unsigned short spg);
  // Debugging
  void getSpglibFormat() const;

  // Find the point group. 0D only; a periodic structure gives unknown.
  QString getPointGroupSymbol(double tolerance = 0.0) const;

  // Reduce cell. See member function fixAngles()
  // Returns true if successful, false otherwise
  // Angles are in degrees. Algorithm is based on Grosse-Kunstleve
  // RW, Sauter NK, Adams PD. Numerically stable algorithms for the
  // computation of reduced unit cells. Acta Crystallographica Section A
  // Foundations of Crystallography. 2003;60(1):1-6. Available at:
  // http://scripts.iucr.org/cgi-bin/paper?S010876730302186X [Accessed
  // November 24, 2010].
  bool niggliReduce(const unsigned int iterations = 100, double lenTol = NIGGLI_TOL);

  static bool isNiggliReduced(const double a, const double b, const double c,
                              const double alpha, const double beta, const double gamma,
                              double lenTol = NIGGLI_TOL);

  bool isNiggliReduced(double lenTol = NIGGLI_TOL) const;

  // Checks to see if an xtal is primitive or not. If a primitive reduction
  // results in a smaller FU xtal, the function returns true
  bool isPrimitive(const double prec = SPGLIB_TOL);

  bool reduceToPrimitive(const double prec = SPGLIB_TOL);

  // Standardize to spglib's conventional cell.
  bool standardizeToConventionalCell(const double prec = SPGLIB_TOL);

  // Convenience functions for cell parameters
  // A non-3D cell gives zero.
  ///@{
  double getA() const { return unitCell().a(); }
  double getB() const { return unitCell().b(); }
  double getC() const { return unitCell().c(); }
  double getAlpha() const { return unitCell().alpha(); }
  double getBeta() const { return unitCell().beta(); }
  double getGamma() const { return unitCell().gamma(); }
  ///@}

  // Return the cell volume. For a molecule this gives raw hull volume.
  double getVolume() const;
  double getVolumePerAtom() const;

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
   * @param distance List of distances
   * @param frequency1 First list of frequencies
   * @param frequency2 Second list of frequencies
   * @param decay Exponential decay parameter for lowering weight of large IADs
   * @param smear Boxcar smoothing width in Angstroms
   * @param error Return error value
   *
   * @return Whether or not the operation could be performed.
   */
  static bool compareIADDistributions(const std::vector<double>& distance,
                                      const std::vector<double>& frequency1,
                                      const std::vector<double>& frequency2,
                                      double decay, double smear, double* error);

  // Atoms and bonds
  ///@{

  /**
   * Add an atom. A reference to the newly added atom is returned and
   * may be edited. The atomic number has a default value of 0, and the
   * position has a default value of (0,0,0).
   *
   * @param atomicNum The atomic number of the atom. Default value is 0.
   * @param pos The 3-dimensional Cartesian coordinates of the atom in
   *            Angstroms. The default is (0,0,0).
   *
   * @return A reference to the atom just created.
   */
  Atom& addAtom(unsigned short atomicNum = 0,
                const Common::Vector3& pos = Common::Vector3(0.0, 0.0, 0.0));

  /**
   * This function overloads addAtom. This adds a copy of the Atom object
   * passed to it.
   *
   * @param atom A copy of the Atom object to be added.
   *
   * @return A reference to the atom just created.
   */
  Atom& addAtom(const Atom& atom);

  /**
   * Add another geometry to this geometry. All atoms will be added, and
   * bonds of the newly added atoms will be preserved.
   *
   * @param other The geometry to be added.
   */
  void appendCell(const Geometry& other);

  /**
   * A Geometry class can contain multiple "molecules." This function is
   * designed to separate these molecules by creating separate Geometries for
   * every group of bonded atoms and returning them in a vector. The
   * individual Geometries will all have the same unit cell as the parent
   * Geometry.
   *
   * @return The vector of indvidual molecules (groups of atoms bonded
   *         together).
   */
  std::vector<Geometry> getConnectedComponents() const;

  /**
   * Set the atoms in the geometry. Any bonds will be cleared.
   *
   * @param atoms The atoms to be set in the geometry.
   */
  void setAtoms(const std::vector<Atom>& atoms);

  /**
   * Remove the atom with index "ind". Returns true on success. Returns
   * false if the index is out of range.
   *
   * @param ind The index of the atom to be removed.
   *
   * @return True on success. False if @p ind is out of range.
   */
  bool removeAtom(size_t ind);

  /**
   * Finds the atom equivalent to @p atom using operator==() and
   * then removes that atom. Returns true if the atom was found and removed.
   * Returns false if it was not.
   *
   * @param atom The atom to be removed.
   *
   * @return True on success. False if the atom was not found.
   */
  bool removeAtom(const Atom& atom);

  /* Clears all atoms from the geometry */
  void clearAtoms()
  {
    g_bonds.clear();
    g_atoms.clear();
    clearGeometryCaches();
  }

  /**
   * Returns the atom at index @p ind. An assertion makes sure that
   * @p ind is not beyond the bounds of the atom vector.
   *
   * @param ind The index of the atom to be returned.
   *
   * @return A reference to the atom that is returned.
   */
  Atom& atom(size_t ind);

  /**
   * Returns a const reference to the atom at index @p ind. An assertion
   * makes sure that @p ind is not beyond the bounds of the atom vector.
   *
   * @param ind The index of the atom to be returned.
   *
   * @return A const reference to the atom that is returned.
   */
  const Atom& atom(size_t ind) const;

  /**
   * Returns a reference to the atoms vector.
   *
   * @return A reference to the vector of atoms.
   */
  std::vector<Atom>& atoms()
  {
    return g_atoms;
  }

  /**
   * Returns a const reference to the atoms vector.
   *
   * @return A const reference to the vector of atoms.
   */
  const std::vector<Atom>& atoms() const { return g_atoms; }

  /**
   * Returns the index of the atom @p atom. Returns -1 if it was not found.
   *
   * @param atom The atom to find the index of.
   *
   * @return The index of the atom. Returns -1 if it was not found.
   */
  long long atomIndex(const Atom& atom) const;

  /**
   * Returns the number of atoms in the geometry.
   *
   * @return The number of atoms in the geometry.
   */
  size_t numAtoms() const { return g_atoms.size(); }

  /**
   * Returns a vector of atomic numbers of the geometry.
   *
   * @return The vector of atomic numbers of the geometry.
   */
  std::vector<unsigned short> atomicNumbers() const;

  /**
   * Returns the atomic number of the atom at index @p ind. An assertion
   * assures us that ind will not be beyond the bounds of the vector.
   *
   * @param ind The index of the atom whose atomic number we are obtaining.
   *
   * @return The atomic number of the atom at index @p.
   */
  unsigned short atomicNumber(size_t ind) const;

  /**
   * Swap the indices of two atoms. This will also ensure that the atom
   * indices in the bonds are properly changed.
   *
   * @param ind1 The index of the first atom to be swapped.
   * @param ind2 The index of the second atom to be swapped.
   */
  void swapAtoms(size_t ind1, size_t ind2);

  /**
   * Sort the atoms. The input, @p sortOrder, should
   * have a size equal to the number of atoms, and every number from
   * 0 to numAtoms() - 1 should be in the vector. This function will
   * automatically adjust the bonds as well so that the atoms remain
   * bonded correctly.
   *
   * @param sortOrder The sort order for the atoms.
   */
  void sortAtoms(std::vector<size_t> sortOrder);

  /**
   * Change the ordering of the atoms. The input, @p newOrder, should
   * have a size equal to the number of atoms, and every number from
   * 0 to numAtoms() - 1 should be in the vector. This function will
   * automatically adjust the bonds as well so that the atoms remain
   * bonded correctly.
   *
   * @param newOrder The new order for the atoms.
   */
  void reorderAtoms(const std::vector<size_t>& newOrder);

  /**
   * Get the cartesian distance between two points. If we have a valid
   * unit cell, we will take into account neighboring images.
   * Otherwise, we will just perform a regular distance calculation.
   *
   * @param A The first point.
   * @param B The second point.
   *
   * @return The distance.
   */
  double distance(const Common::Vector3& A, const Common::Vector3& B) const;

  /**
   * Get the cartesian distance between two atoms. If we have a valid
   * unit cell, we will take into account neighboring atom images.
   * Otherwise, we will just perform a regular distance calculation.
   *
   * @param atomInd1 The first atom index.
   * @param atomInd2 The second atom index.
   *
   * @return The distance.
   */
  double distance(size_t atomInd1, size_t atomInd2) const;

  /**
   * Get the angle (in degrees) between three points (where the second point
   * is the vertex).
   *
   * @param A The first point.
   * @param B The second point (the vertex of the angle).
   * @param C The third point.
   *
   * @return The angle in degrees.
   */
  static double angle(const Common::Vector3& A, const Common::Vector3& B, const Common::Vector3& C);

  /**
   * Get the angle (in degrees) between three atoms (where the second atom is
   * the vertex).
   *
   * @param atomInd1 The first atom index.
   * @param atomInd2 The second atom index (the vertex of the angle).
   * @param atomInd3 The third atom index.
   *
   * @return The angle in degrees.
   */
  double angle(size_t atomInd1, size_t atomInd2, size_t atomInd3) const;

  /**
   * Get the dihedral angle (in degrees) created by four points where the
   * first three points form a plane and the last three points form a plane.
   *
   * @param A The first point.
   * @param B The second point.
   * @param C The third point.
   * @param D The fourth point.
   *
   * @return The dihedral angle in degrees.
   */
  static double dihedral(const Common::Vector3& A, const Common::Vector3& B,
                         const Common::Vector3& C, const Common::Vector3& D);

  /**
   * Get the dihedral angle (in degrees) created by four atoms where the
   * first three atoms form a plane and the last three atoms form a plane.
   *
   * @param atomInd1 The first atom index.
   * @param atomInd2 The second atom index.
   * @param atomInd3 The third atom index.
   * @param atomInd4 The fourth atom index.
   *
   * @return The dihedral angle (in degrees).
   */
  double dihedral(size_t atomInd1, size_t atomInd2,
                  size_t atomInd3, size_t atomInd4) const;

  /**
   * Does this geometry contain bonds? Returns true if !g_bonds.empty().
   *
   * @return Whether or not the geometry contains bonds.
   */
  bool hasBonds() const { return !g_bonds.empty(); }

  /**
   * How many bonds do we have?
   *
   * @return The number of bonds in the geometry.
   */
  size_t numBonds() const { return g_bonds.size(); }

  /**
   * Create a bond using the atom indices. Does nothing if the index is
   * out of range. There is a default bond order of 1.
   *
   * @param ind1 The index of the first atom in the bond.
   * @param ind2 The index of the second atom in the bond.
   * @param bondOrder The bond order of the bond.
   */
  void addBond(size_t ind1, size_t ind2, unsigned short bondOrder = 1);

  /**
   * Remove the bond at index @p bondInd. Does nothing if out of range.
   *
   * @param bondInd The index of the bond to be removed.
   */
  void removeBond(size_t bondInd);

  /**
   * Remove the bond between the two atoms. Does nothing if the atoms are
   * not bonded or are not in the geometry.
   *
   * @param ind1 The index of the first atom in the bond.
   * @param ind2 The index of the second atom in the bond.
   */
  void removeBondBetweenAtoms(size_t ind1, size_t ind2);

  /**
   * Remove all bonds connected to the atom @param ind.
   *
   * @param ind The index of the atom for which to remove bonds.
   */
  void removeBondsFromAtom(size_t ind);

  /**
   * Get the vector of bonds.
   *
   * @return The vector of bonds.
   */
  std::vector<Bond>& bonds() { return g_bonds; }

  /**
   * Get the vector of bonds. Const version.
   *
   * @return The vector of bonds.
   */
  const std::vector<Bond>& bonds() const { return g_bonds; }

  /**
   * Get the Bond at index @p bondInd.
   *
   * @param bondInd The index for which to get the bond.
   *
   * @return A reference to the Bond object.
   */
  Bond& bond(size_t bondInd);

  /**
   * Get the Bond at index @p bondInd. No edits are allowed here.
   *
   * @param bondInd The index for which to get the bond.
   *
   * @return A const reference to the Bond object.
   */
  const Bond& bond(size_t bondInd) const;

  /**
   * If the two atoms are bonded together, return the index of the bond that
   * is between them. If they are not bonded together, return -1.
   *
   * @param atomInd1 The first atom index of the two atoms bonded together.
   * @param atomInd2 The second atom index of the two atoms bonded together.
   *
   * @return The index of the bond between the atoms or -1 if no bond exists.
   */
  long long bondBetweenAtoms(size_t atomInd1, size_t atomInd2) const;

  /**
   * Is the atom at index @p ind bonded?
   *
   * @return True if it has at least one bond. False otherwise.
   */
  bool isBonded(size_t ind) const;

  /**
   * Are these two atoms bonded together?
   *
   * @param ind1 The index of the first atom.
   * @param ind2 The index of the second atom.
   *
   * @return True if the atoms are bonded together. False otherwise.
   */
  bool areBonded(size_t ind1, size_t ind2) const;

  /**
   * Get the indices of the bonds connected to atom @p ind.
   *
   * @param ind The index of the atom for which to get bonds.
   *
   * @return The indices of the bonds on the atom.
   */
  std::vector<size_t> bonds(size_t ind) const;

  /**
   * Get the indices of the atoms that are bonded to the atom at index
   * @p index.
   *
   * @param ind The index of the atom for which to get other bonded atoms.
   *
   * @return The indices of the atoms bonded to the atom.
   */
  std::vector<size_t> bondedAtoms(size_t ind) const;

  /**
   * Use the atoms' covalent radii to automatically generate bonds.
   */
  void perceiveBonds();

  /** @return An alphabetized list of the atomic symbols for the
   * atomic species present in the Structure.
   * @sa getNumberOfAtomsAlpha
   */
  QList<QString> getSymbols() const;

  /** @return The number of atoms of species
   * given by the variable s.
   */
  unsigned int getNumberOfAtomsOfSymbol(const QString& s) const;

  /** @return A list of the number of species present that
   * corresponds to the symbols listed in getSymbols().
   * @sa getSymbols
   */
  std::vector<uint> getNumberOfAtomsAlpha() const;

  /** @return The string with chemical formula
   */
  QString getChemicalFormula() const;

  // Greatest common divisor of the atom counts.
  unsigned int getFormulaUnits() const;

  /** Return the "unique" composition of the structure
   * With default true argument, the output is "empirical composition",
   * otherwise, it will be exact atom counts.
   */
  QString getCompositionString(bool reduceToEmpirical = true) const;

  /**
   * Remove all bonds.
   */
  void clearBonds() { g_bonds.clear(); }

  bool isPeriodic(int axis) const { return g_unitCell.isPeriodic(axis); }

  // Number of periodic axes (0..3).
  int dimension() const { return g_unitCell.dimension(); }

  bool is3D() const { return g_unitCell.is3D(); }
  bool is2D() const { return g_unitCell.is2D(); }
  bool is1D() const { return g_unitCell.is1D(); }
  bool is0D() const { return g_unitCell.is0D(); }

  /**
   * Set the unit cell of the geometry.
   *
   * @param uc The new unit cell of the geometry.
   */
  void setUnitCell(const UnitCell& uc)
  {
    g_unitCell = uc;
    clearGeometryCaches();
  };

  /**
   * Get a reference to the unit cell of the geometry.
   *
   * @return The unit cell of the geometry.
   */
  UnitCell& unitCell()
  {
    return g_unitCell;
  };

  /**
   * Get a const reference to the unit cell of the geometry.
   *
   * @return The unit cell of the geometry.
   */
  const UnitCell& unitCell() const { return g_unitCell; };

  /**
   * Clear the bonds and atoms, and zero the unit cell.
   */
  void clear()
  {
    g_bonds.clear();
    g_atoms.clear();
    g_unitCell.clear();
    clearGeometryCaches();
  };

  ///@}

private:
  std::vector<Atom> g_atoms;
  std::vector<Bond> g_bonds;
  UnitCell g_unitCell;

  mutable unsigned short g_spgNumber = 0;
  mutable QString g_spgSymbol = "Unknown";
  mutable std::vector<float> g_norm_rdf;
  // Parameters used to make the RDF.
  mutable int g_norm_rdf_nsymbs = 0;
  mutable int g_norm_rdf_nbins = 0;
  mutable double g_norm_rdf_cutoff = 0.0;
  mutable double g_norm_rdf_sigma = 0.0;
  mutable std::vector<std::vector<std::pair<int, double> > > g_neighbor_list;
  // Cutoff used to make the neighbor lists.
  mutable double g_neighbor_list_cutoff = 0.0;

  void clearGeometryCaches()
  {
    clearNormalizedRDF();
    clearNearestNeighborLists();
    g_spgNumber = 0;
    g_spgSymbol = "Unknown";
  }

  unsigned int reduceToPrimitive(QList<Common::Vector3>* fcoords, QList<unsigned int>* atomicNums,
                                 Common::Matrix3* cellMatrix, const double prec = SPGLIB_TOL);
};

static_assert(std::is_nothrow_move_constructible<Geometry>::value,
              "Geometry should be noexcept move constructible.");

static_assert(std::is_nothrow_move_assignable<Geometry>::value,
              "Geometry should be noexcept move assignable.");

inline Atom& Geometry::addAtom(unsigned short atomicNum, const Common::Vector3& pos)
{
  clearGeometryCaches();
  g_atoms.push_back(Atom(atomicNum, pos));
  return g_atoms.back();
}

inline Atom& Geometry::addAtom(const Atom& atom)
{
  clearGeometryCaches();
  g_atoms.push_back(atom);
  return g_atoms.back();
}

inline void Geometry::setAtoms(const std::vector<Atom>& atoms)
{
  g_bonds.clear();
  g_atoms = atoms;
  clearGeometryCaches();
}

inline Atom& Geometry::atom(size_t ind)
{
  assert(ind < g_atoms.size());
  return g_atoms[ind];
}

inline const Atom& Geometry::atom(size_t ind) const
{
  assert(ind < g_atoms.size());
  return g_atoms[ind];
}

inline long long Geometry::atomIndex(const Atom& atom) const
{
  for (size_t i = 0; i < g_atoms.size(); ++i) {
    if (atom == g_atoms[i])
      return i;
  }
  return -1;
}

inline std::vector<unsigned short> Geometry::atomicNumbers() const
{
  std::vector<unsigned short> atomicNums;
  atomicNums.reserve(g_atoms.size());
  for (size_t i = 0; i < g_atoms.size(); ++i)
    atomicNums.push_back(g_atoms[i].atomicNumber());
  return atomicNums;
}

inline unsigned short Geometry::atomicNumber(size_t ind) const
{
  assert(ind < g_atoms.size());
  return g_atoms[ind].atomicNumber();
}

inline void Geometry::swapAtoms(size_t ind1, size_t ind2)
{
  assert(ind1 < g_atoms.size());
  assert(ind2 < g_atoms.size());
  clearGeometryCaches();
  std::swap(g_atoms[ind1], g_atoms[ind2]);
  for (auto& bond : g_bonds)
    bond.swapIndices(ind1, ind2);
}

inline double Geometry::distance(const Common::Vector3& A, const Common::Vector3& B) const
{
  if (is3D())
    return g_unitCell.distance(A, B);
  return fabs((A - B).norm());
}

inline double Geometry::distance(size_t atomInd1, size_t atomInd2) const
{
  assert(atomInd1 < g_atoms.size());
  assert(atomInd2 < g_atoms.size());
  return distance(g_atoms[atomInd1].pos(), g_atoms[atomInd2].pos());
}

inline double Geometry::angle(size_t atomInd1, size_t atomInd2,
                               size_t atomInd3) const
{
  assert(atomInd1 < g_atoms.size());
  assert(atomInd2 < g_atoms.size());
  assert(atomInd3 < g_atoms.size());
  return angle(g_atoms[atomInd1].pos(), g_atoms[atomInd2].pos(),
               g_atoms[atomInd3].pos());
}

inline double Geometry::dihedral(size_t atomInd1, size_t atomInd2,
                                  size_t atomInd3, size_t atomInd4) const
{
  assert(atomInd1 < g_atoms.size());
  assert(atomInd2 < g_atoms.size());
  assert(atomInd3 < g_atoms.size());
  assert(atomInd4 < g_atoms.size());
  return dihedral(g_atoms[atomInd1].pos(), g_atoms[atomInd2].pos(),
                  g_atoms[atomInd3].pos(), g_atoms[atomInd4].pos());
}

inline void Geometry::addBond(size_t ind1, size_t ind2,
                               unsigned short bondOrder)
{
  if (ind1 >= g_atoms.size() || ind2 >= g_atoms.size())
    return;
  // We will only allow one bond at a time between two atoms
  if (!areBonded(ind1, ind2))
    g_bonds.push_back(Bond(ind1, ind2, bondOrder));
}

inline void Geometry::removeBond(size_t bondInd)
{
  if (bondInd >= g_bonds.size())
    return;
  g_bonds.erase(g_bonds.begin() + bondInd);
}

inline Bond& Geometry::bond(size_t bondInd)
{
  assert(bondInd < g_bonds.size());
  return g_bonds[bondInd];
}

inline const Bond& Geometry::bond(size_t bondInd) const
{
  assert(bondInd < g_bonds.size());
  return g_bonds[bondInd];
}

} // namespace Atoms

#endif

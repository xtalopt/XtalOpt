/**********************************************************************
  Xtal - Wrapper for Structure to ease work with crystals.

  Copyright (C) 2009-2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef XTAL_H
#define XTAL_H

#include <globalsearch/structure.h>

#include <QtCore/QDebug>
#include <QtCore/QDateTime>
#include <QtCore/QMutex>
#include <QtCore/QTextStream>
#include <QtCore/QVector>

#define EV_TO_KCAL_PER_MOL 23.060538

class QFile;

namespace XtalOpt {
  struct XtalCompositionStruct;

  class Xtal : public GlobalSearch::Structure
  {
    Q_OBJECT

   public:
    Xtal(QObject *parent = 0);
    Xtal(double A, double B, double C,
         double Alpha, double Beta, double Gamma,
         QObject *parent = 0);
    virtual ~Xtal();

    // Virtuals from structure
    bool getShortestInteratomicDistance(double & shortest) const;
    bool getSquaredAtomicDistancesToPoint(const Eigen::Vector3d &coord,
                                          QVector<double> *distances);
    bool getNearestNeighborDistance(const double x, const double y, const double z,
                                    double & shortest) const;
    bool getIADHistogram(QList<double> * distance,
                         QList<double> * frequency,
                         double min, double max, double step,
                         Avogadro::Atom *atom = 0) const;
    bool addAtomRandomly(uint atomicNumber,
                         double minIAD = 0.0,
                         double maxIAD = 0.0,
                         int maxAttempts = 100.0,
                         Avogadro::Atom **atom = 0); //maxIAD is not used.
    // Uses the minRadius constraints in @a limits to restrict atom placement
    bool addAtomRandomly(unsigned int atomicNumber,
                         const QHash<unsigned int, XtalCompositionStruct> & limits,
                         int maxAttempts = 100.0,
                         Avogadro::Atom **atom = 0);
    // Use the minRadius constraints in @a limits to check the interatomic
    // distances in the xtal. atom1 and atom2 are overwritten with the indexes
    // of the first set of offending atom, if any, that are found. The bad IAD
    // is written to IAD if a double pointer is provided.
    bool checkInteratomicDistances(const QHash<unsigned int, XtalCompositionStruct> &limits,
                                   int *atom1 = NULL, int *atom2 = NULL,
                                   double *IAD = NULL);
    // Same as above, but check the atoms in "atoms" against those in "this".
    // atom1 is the index into "atoms", atom2 is the index into this->m_atomList
    // (use Xtal::atom(atom2) to snag a pointer to it).
    bool checkInteratomicDistances(const QHash<unsigned int, XtalCompositionStruct> &limits,
                                   const QList<Avogadro::Atom*> atoms,
                                   int *atom1 = NULL, int *atom2 = NULL,
                                   double *IAD = NULL);
    QHash<QString, QVariant> getFingerprint();
    virtual QString getResultsEntry() const;
    virtual QString getResultsHeader() const {
      return QString("%1 %2 %3 %4 %5 %6")
        .arg("Rank", 6)
        .arg("Gen", 6)
        .arg("ID", 6)
        .arg("Enthalpy", 10)
        .arg("SpaceGroup", 10)
        .arg("Status", 11);};

    // Cell paramters
    double getA()       const {return cell()->GetA();};
    double getB()       const {return cell()->GetB();};
    double getC()       const {return cell()->GetC();};
    double getAlpha()   const {return cell()->GetAlpha();};
    double getBeta()    const {return cell()->GetBeta();};
    double getGamma()   const {return cell()->GetGamma();};
    double getVolume()  const {return cell()->GetCellVolume();};

    // Debugging
    void getSpglibFormat() const;

    // Rotate the cell vectors (and atomic coordinates in the second
    // function) so that v1 is parallel to x and v2 is in the xy plane
    bool rotateCellToStandardOrientation();
    bool rotateCellAndCoordsToStandardOrientation();

    // Calculate the matrix used in the above function. Matrix has row vectors.
    // If the current cell cannot be rotated in a numerically stable
    // manner, this will return Eigen::Matrix3d::Zeros;
    // Use matrix @a m
    static Eigen::Matrix3d getCellMatrixInStandardOrientation(const Eigen::Matrix3d &m);
    // Use this's cell
    Eigen::Matrix3d getCellMatrixInStandardOrientation() const;

    // Randomly skew the lattice and translate the coordinates. Coordinates
    // may be reflected, but the structures should be energetically
    // equivalent
    Xtal * getRandomRepresentation() const;

    // Conversion convenience
    OpenBabel::vector3 fracToCart(const OpenBabel::vector3 & fracCoords) const {
      return cell()->FractionalToCartesian(fracCoords);}
    OpenBabel::vector3* fracToCart(const OpenBabel::vector3* fracCoords) const {
      return new OpenBabel::vector3 (cell()->FractionalToCartesian(*fracCoords));}
    Eigen::Vector3d fracToCart(const Eigen::Vector3d & fracCoords) const;
    Eigen::Vector3d* fracToCart(const Eigen::Vector3d* fracCoords) const;

    OpenBabel::vector3 cartToFrac(const OpenBabel::vector3 & cartCoords) const {
      return cell()->CartesianToFractional(cartCoords);}
    OpenBabel::vector3* cartToFrac(const OpenBabel::vector3* cartCoords) const {
      return new OpenBabel::vector3 (cell()->CartesianToFractional(*cartCoords));}
    Eigen::Vector3d cartToFrac(const Eigen::Vector3d & cartCoords) const;
    Eigen::Vector3d* cartToFrac(const Eigen::Vector3d* cartCoords) const;

    // Convenience retreval
    QList<Avogadro::Atom*> getAtomsSortedBySymbol() const;

    // Spacegroup
    uint getSpaceGroupNumber();
    QString getSpaceGroupSymbol();
    QString getHTMLSpaceGroupSymbol();

    // Reduce cell. See member function fixAngles()
    // Returns true if successful, false otherwise
    // Angles are in degrees. Algorithm is based on Grosse-Kunstleve
    // RW, Sauter NK, Adams PD. Numerically stable algorithms for the
    // computation of reduced unit cells. Acta Crystallographica Section A
    // Foundations of Crystallography. 2003;60(1):1-6. Available at:
    // http://scripts.iucr.org/cgi-bin/paper?S010876730302186X [Accessed
    // November 24, 2010].
    bool niggliReduce(const unsigned int iterations = 100);
    static bool isNiggliReduced(const double a, const double b, const double c,
                                const double alpha, const double beta,
                                const double gamma);
    bool isNiggliReduced() const;

    bool operator==(const Xtal &other) const;
    bool operator!=(const Xtal &other) const {return !operator==(other);};

    // Tolerances in angstrom and degree:
    bool compareCoordinates(const Xtal &other, const double tol = 0.1,
                            const double angleTol = 2.0) const;

    // Testing use only
    /**
     * Given a QString containing a POSCAR formatted structure, create
     * a generic Xtal object.
     *
     * \note The atom types will not be correct here -- this function
     * should only be used in testing. To actually read in a QM
     * output, see the OpenBabel::OBConversion documentation.
     */
    static Xtal* POSCARToXtal(const QString &poscar);

    /**
     * Given a QFile handle containing a POSCAR formatted structure,
     * create a generic Xtal object.
     *
     * \note The atom types will not be correct here -- this function
     * should only be used in testing. To actually read in a QM
     * output, see the OpenBabel::OBConversion documentation.
     */
    static Xtal* POSCARToXtal(QFile *file);

    // Find the shortest equivalent vector accounting for translational
    // symmetry
    void shortenCartesianVector(Eigen::Vector3d *cartVec);
    static void shortenCartesianVector(Eigen::Vector3d *cartVec,
                                       const Eigen::Matrix3d &cellColMatrix);

    // Find the shortest equivalent vector accounting for translational
    // symmetry.
    void shortenFractionalVector(Eigen::Vector3d *fracVec);

    // For random representation generation
    static void generateValidCOBs();
    static QVector<Eigen::Matrix3d> m_transformationMatrices;
    static QVector<Eigen::Matrix3d> m_mixMatrices;
    // Ensure that only one thread generates the COB vectors
    static QMutex m_validCOBsGenMutex;

   signals:
    void dimensionsChanged();

   public slots:
    // Cell data
    void setCellInfo(double A, double B, double C,
                     double Alpha, double Beta, double Gamma) {
      cell()->SetData(A,B,C,Alpha,Beta,Gamma);};
    void setCellInfo(const OpenBabel::matrix3x3 &m) {
      cell()->SetData(m);};
    void setCellInfo(const OpenBabel::vector3 &v1, const OpenBabel::vector3 &v2, const OpenBabel::vector3 &v3) {
      cell()->SetData(v1, v2, v3);};
    virtual void setVolume(double Volume);
    // rescale cell can be used to "fix" any cell parameter at a particular value.
    // Simply pass the fixed values and use "0" for any non-fixed parameters.
    // Volume will be preserved.
    virtual void rescaleCell(double a, double b, double c,
                             double alpha, double beta, double gamma);

    // Self-correction
    bool fixAngles(int attempts = 100);
    void wrapAtomsToCell();

    // Spacegroup
    virtual void findSpaceGroup(double prec = 0.05);

   protected:
    void ctor(QObject *parent=0);
    OpenBabel::OBUnitCell* cell() const;
    uint m_spgNumber;
    QString m_spgSymbol;
  };

} // end namespace XtalOpt

#endif

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

#include <QtCore/QMutex>
#include <QtCore/QVector>

#define EV_TO_KCAL_PER_MOL 23.060538

class QFile;

namespace XtalOpt {
  struct XtalCompositionStruct;
  struct MolUnit;

  using GlobalSearch::Matrix3;
  using GlobalSearch::Vector3;

  class Xtal : public GlobalSearch::Structure
  {
    Q_OBJECT

   public:
    Xtal(QObject *parent = 0);
    Xtal(double A, double B, double C,
         double Alpha, double Beta, double Gamma,
         QObject *parent = 0);

    /* Copy constructor */
    Xtal(const Xtal& other);

    /* Move constructor */
    Xtal(Xtal&& other) noexcept;

    /* Assignment operator */
    Xtal& operator=(const Xtal& other);

    /* Move assignment operator */
    Xtal& operator=(Xtal&& other) noexcept;

    virtual ~Xtal() override;

    // Virtuals from structure
    bool getShortestInteratomicDistance(double & shortest) const override;
    bool getSquaredAtomicDistancesToPoint(const Vector3 &coord,
                                          QVector<double> *distances);
    bool getNearestNeighborDistance(const double x,
                                    const double y,
                                    const double z,
                                    double & shortest) const override;
    bool getIADHistogram(QList<double> * distance,
                         QList<double> * frequency,
                         double min, double max, double step,
                         GlobalSearch::Atom *atom = 0) const;
    bool addAtomRandomly(uint atomicNumber,
                         double minIAD = 0.0,
                         double maxIAD = 0.0,
                         int maxAttempts = 100.0) override;//maxIAD is not used.
    // Uses the minRadius constraints in @a limits to restrict atom placement
    bool addAtomRandomly(unsigned int atomicNumber,
                         const QHash<unsigned int, XtalCompositionStruct> & limits,
                         int maxAttempts = 100.0);
    bool addAtomRandomly(unsigned int atomicNumber,
                         unsigned int neighbor,
                         const QHash<unsigned int, XtalCompositionStruct> & limits,
                         const QHash<QPair<int, int>, MolUnit> & limitsMolUnit,
                         bool useMolUnit,
                         int maxAttempts = 100.0);

    // Build molUnit given a tempMolecule with a center atom defined
    bool molUnitBuilder(Vector3 centerCoords, unsigned int atomicNum, int valence, double dist, int hyb);

    // Fills a supercell for the mitosis process
    bool fillSuperCell(int a, int b, int c, Xtal * myXtal);

    // Use the minRadius constraints in @a limits to check the interatomic
    // distances in the xtal. atom1 and atom2 are overwritten with the indexes
    // of the first set of offending atom, if any, that are found. The bad IAD
    // is written to IAD if a double pointer is provided.
    bool checkInteratomicDistances(const QHash<unsigned int, XtalCompositionStruct> &limits,
                                   int *atom1 = nullptr, int *atom2 = nullptr,
                                   double *IAD = nullptr);
    QHash<QString, QVariant> getFingerprint();
    virtual QString getResultsEntry() const override;
    virtual QString getResultsHeader() const override {
      return QString("%1 %2 %3 %4 %5 %6 %7")
        .arg("Rank", 6)
        .arg("Gen", 6)
        .arg("ID", 6)
        .arg("Enthalpy/FU", 13)
        .arg("FU", 6)
        .arg("SpaceGroup", 10)
        .arg("Status", 11);};

    // Convencience functions for cell parameters
    double getA()       const { return unitCell().a(); };
    double getB()       const { return unitCell().b(); };
    double getC()       const { return unitCell().c(); };
    double getAlpha()   const { return unitCell().alpha(); };
    double getBeta()    const { return unitCell().beta(); };
    double getGamma()   const { return unitCell().gamma(); };
    double getVolume()  const { return unitCell().volume(); };

    // Debugging
    void getSpglibFormat() const;

    // Rotate the cell vectors (and atomic coordinates in the second
    // function) so that v1 is parallel to x and v2 is in the xy plane
    bool rotateCellToStandardOrientation();
    bool rotateCellAndCoordsToStandardOrientation();

    // Calculate the matrix used in the above function. Matrix has row vectors.
    // If the current cell cannot be rotated in a numerically stable
    // manner, this will return Matrix3::Zeros;
    // Use matrix @a m
    static Matrix3 getCellMatrixInStandardOrientation(const Matrix3 &m);
    // Use this's cell
    Matrix3 getCellMatrixInStandardOrientation() const;

    // Randomly skew the lattice and translate the coordinates. Coordinates
    // may be reflected, but the structures should be energetically
    // equivalent
    Xtal * getRandomRepresentation() const;

    // Conversion convenience
    Vector3 fracToCart(const Vector3& v) const;
    Vector3 cartToFrac(const Vector3& v) const;

    // Convenience retreval
    QList<Vector3> getAtomCoordsFrac() const;

    // Spacegroup
    uint getSpaceGroupNumber();
    QString getSpaceGroupSymbol();
    QString getHTMLSpaceGroupSymbol();

    // Static function for getting a Hermann-Mauguin name from a spg number
    static QString getHMName(unsigned short spg);


    // Reduce cell. See member function fixAngles()
    // Returns true if successful, false otherwise
    // Angles are in degrees. Algorithm is based on Grosse-Kunstleve
    // RW, Sauter NK, Adams PD. Numerically stable algorithms for the
    // computation of reduced unit cells. Acta Crystallographica Section A
    // Foundations of Crystallography. 2003;60(1):1-6. Available at:
    // http://scripts.iucr.org/cgi-bin/paper?S010876730302186X [Accessed
    // November 24, 2010].
    bool niggliReduce(const unsigned int iterations = 100, double lenTol = 0.01);
    static bool isNiggliReduced(const double a, const double b, const double c,
                                const double alpha, const double beta,
                                const double gamma, double lenTol = 0.01);
    bool isNiggliReduced(double lenTol = 0.01) const;

    // Checks to see if an xtal is primitive or not. If a primitive reduction
    // results in a smaller FU xtal, the function returns true
    bool isPrimitive(const double cartTol = 0.05);
    bool reduceToPrimitive(const double cartTol = 0.05);

    QList<QString> currentAtomicSymbols();
    inline void updateMolecule(const QList<QString> &ids,
                               const QList<Vector3> &coords);
    void setCurrentFractionalCoords(const QList<QString> &ids,
                                    const QList<Vector3> &fcoords);

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
     * should only be used in testing.
     */
    static Xtal* POSCARToXtal(const QString &poscar);

    /**
     * Given a QFile handle containing a POSCAR formatted structure,
     * create a generic Xtal object.
     *
     * \note The atom types will not be correct here -- this function
     * should only be used in testing.
     */
    static Xtal* POSCARToXtal(QFile *file);

    // For random representation generation
    static void generateValidCOBs();
    static QVector<Matrix3> m_transformationMatrices;
    static QVector<Matrix3> m_mixMatrices;
    // Ensure that only one thread generates the COB vectors
    static QMutex m_validCOBsGenMutex;

   signals:
    void dimensionsChanged();

   public slots:
    // Cell data
    void setCellInfo(double a, double b, double c,
                     double alpha, double beta, double gamma);
    void setCellInfo(const Matrix3& m) { unitCell().setCellMatrix(m); };
    void setCellInfo(const Vector3& a,
                     const Vector3& b,
                     const Vector3& c);
    void setVolume(double Volume);
    // rescale cell can be used to "fix" any cell parameter at a particular value.
    // Simply pass the fixed values and use "0" for any non-fixed parameters.
    // Volume will be preserved.
    void rescaleCell(double a, double b, double c, double alpha, double beta, double gamma);

    // Self-correction
    bool fixAngles(int attempts = 100);
    void wrapAtomsToCell();

    // Spacegroup
    void findSpaceGroup(double prec = 0.05);

    // Printing debug output
    void printLatticeInfo() const;
    void printAtomInfo() const;
    void printXtalInfo() const;

   private slots:

   private:
    // This function is called by the public overloaded function:
    // bool reduceToPrimitive(const double cartTol = 0.05)
    unsigned int reduceToPrimitive(QList<Vector3> *fcoords,
                                   QList<unsigned int> *atomicNums,
                                   Matrix3 *cellMatrix,
                                   const double cartTol = 0.05);
    unsigned short m_spgNumber;
    QString m_spgSymbol;

  };

  inline Vector3 Xtal::fracToCart(const Vector3& v) const
  {
    return unitCell().toCartesian(v);
  }

  inline Vector3 Xtal::cartToFrac(const Vector3& v) const
  {
    return unitCell().toFractional(v);
  }

  inline void Xtal::setCellInfo(double a, double b, double c,
                                double alpha, double beta, double gamma)
  {
    unitCell().setCellParameters(a, b, c, alpha, beta, gamma);
  }

  inline void Xtal::setCellInfo(const Vector3& a, const Vector3& b,
                                const Vector3& c)
  {
    unitCell().setCellVectors(a, b, c);
  }

} // end namespace XtalOpt

#endif

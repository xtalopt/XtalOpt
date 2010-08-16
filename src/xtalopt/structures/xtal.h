/**********************************************************************
  Xtal - Wrapper for Structure to ease work with crystals.

  Copyright (C) 2009-2010 by David C. Lonie

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

#include <QDebug>
#include <QDateTime>
#include <QTextStream>

#define EV_TO_KCAL_PER_MOL 23.060538

using namespace GlobalSearch;

namespace XtalOpt {
  class Xtal : public Structure
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
    bool getNearestNeighborDistance(double x, double y, double z, double & shortest) const;
    bool getNearestNeighborHistogram(QList<double> & distance, QList<double> & frequency, double min, double max, double step, Atom *atom = 0) const;
    bool addAtomRandomly(uint atomicNumber, double minIAD = 0.0, double maxIAD = 0.0, int maxAttempts = 100.0, Atom **atom = 0); //maxIAD is not used.
    QHash<QString, double> getFingerprint();

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
    QList<Eigen::Vector3d> getAtomCoordsFrac() const;

    // Spacegroup
    uint getSpaceGroupNumber();
    QString getSpaceGroupSymbol();
    QString getHTMLSpaceGroupSymbol();


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
    void setVolume(double Volume);
    // rescale cell can be used to "fix" any cell parameter at a particular value.
    // Simply pass the fixed values and use "0" for any non-fixed parameters.
    // Volume will be preserved.
    void rescaleCell(double a, double b, double c, double alpha, double beta, double gamma);

    // Self-correction
    bool fixAngles(int attempts = 20);
    void wrapAtomsToCell();

    // Spacegroup
    void findSpaceGroup(double prec = 0.05);

   private slots:

   private:
    void ctor(QObject *parent=0);
    OpenBabel::OBUnitCell* cell() const;
    uint m_spgNumber;
    QString m_spgSymbol;
  };

} // end namespace Avogadro

#endif

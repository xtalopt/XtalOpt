/**********************************************************************
  XtalOptMolecule - Wrapper for Avogadro's molecule class to ease work
                    with crystals.

  Copyright (C) 2009 by David C. Lonie

  This file is part of the Avogadro molecular editor project.
  For more information, see <http://avogadro.openmolecules.net/>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef XTALOPTMOL_H
#define XTALOPTMOL_H

#include <avogadro/molecule.h>

#include <openbabel/generic.h>
#include <openbabel/mol.h>
#include <openbabel/math/vector3.h>

#include <QDebug>
#include <QDateTime>
#include <QTextStream>

#define EV_TO_KCAL_PER_MOL 23.060538

namespace Avogadro {
  class XtalOpt;
  class Xtal : public Molecule
  {
    Q_OBJECT

   public:
    Xtal(QObject *parent = 0);
    Xtal(double A, double B, double C,
         double Alpha, double Beta, double Gamma,
         QObject *parent = 0);
    virtual ~Xtal();

    enum State {Optimized = 0,
                StepOptimized,
                WaitingForOptimization,
                InProcess,
                Empty,
                Updating,
                Error,
                Submitted,
                Killed,
                Removed,
                Duplicate,
                Restart
    };

    double getA()       const {return cell()->GetA();};
    double getB()       const {return cell()->GetB();};
    double getC()       const {return cell()->GetC();};
    double getAlpha()   const {return cell()->GetAlpha();};
    double getBeta()    const {return cell()->GetBeta();};
    double getGamma()   const {return cell()->GetGamma();};
    double getVolume()  const {return cell()->GetCellVolume();};
    bool hasEnergy()	const {return m_hasEnergy;};
    bool hasEnthalpy()	const {return m_hasEnthalpy;};
    double getEnergy()	const {return energy(0)/EV_TO_KCAL_PER_MOL;};
    double getEnthalpy()const {if (!m_hasEnthalpy&&m_hasEnergy) return getEnergy(); return m_enthalpy;};
    double getPV()	const {return m_PV;};
    uint getRank()	const {return m_rank;};
    uint getJobID()	const {return m_jobID;};
    uint getGeneration() const {return m_generation;};
    uint getXtalNumber() const {return m_id;};
    int getIndex() const {return m_index;};
    QString getDuplicateString() const {return m_dupString;};
    QString getParents() const {return m_parents;};
    QString getRempath() const {return m_rempath;};
    State getStatus()	const {return m_status;};
    uint getCurrentOptStep() {return m_currentOptStep;};
    uint getFailCount() { return m_failCount;};
    QDateTime getOptTimerStart() const {return m_optStart;};
    QDateTime getOptTimerEnd() const {return m_optEnd;};
    QString getIDString() const {return tr("%1x%2").arg(getGeneration()).arg(getXtalNumber());};
    void getSpglibFormat() const;

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

    bool getShortestInteratomicDistance(double & shortest) const;
    bool getNearestNeighborDistance(double x, double y, double z, double & shortest) const;
    bool getNearestNeighborHistogram(QList<double> & distance, QList<double> & frequency, double min, double max, double step, Atom *atom = 0) const;
    bool addAtomRandomly(uint atomicNumber, double minIAD = 0.0, double maxAttempts = 100.0);

    uint getSpaceGroupNumber();
    QString getSpaceGroupSymbol();
    QString getHTMLSpaceGroupSymbol();

    QList<QString> getSymbols() const;
    QList<uint> getNumberOfAtomsAlpha() const; // Number of each type of atom (sorted alphabetically by symbol)
    QList<Eigen::Vector3d> getAtomCoordsFrac() const;

    QString getOptElapsed() const;

    QHash<QString, double> getFingerprint();

   signals:
    void dimensionsChanged();

   public slots:
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
    bool fixAngles(int attempts = 20);
    void setEnthalpy(double enthalpy) {m_hasEnthalpy = true; m_enthalpy = enthalpy;};
    void setPV(double pv) {m_PV = pv;};
    void setEnergy(double e) {m_hasEnergy = true; std::vector<double> E; E.push_back(e); setEnergies(E);};
    void setOBEnergy(const QString &ff = QString("UFF"));
    void setRank(uint rank) {m_rank = rank;};
    void setJobID(uint id) {m_jobID = id;};
    void setGeneration(uint gen) {m_generation = gen;};
    void setXtalNumber(uint id) {m_id = id;};
    void setIndex(int index) {m_index = index;};
    void setParents(const QString & p) {m_parents = p;};
    void setRempath(const QString & p) {m_rempath = p;};
    void setStatus(State status) {m_status = status;};
    void setCurrentOptStep(uint i) {m_currentOptStep = i;};
    void setFailCount(uint count) {m_failCount = count;};
    void resetFailCount() {setFailCount(0);};
    void addFailure() {setFailCount(getFailCount() + 1);};
    void setDuplicateString(const QString & s) {m_dupString = s;};
    void setOpt(XtalOpt *p) {m_opt = p;};
    void startOptTimer() {m_optStart = QDateTime::currentDateTime(); m_optEnd = QDateTime();};
    void stopOptTimer() {if (m_optEnd.isNull()) m_optEnd = QDateTime::currentDateTime();};
    void setOptTimerStart(const QDateTime &d) {m_optStart = d;};
    void setOptTimerEnd(const QDateTime &d) {m_optEnd = d;};
    void findSpaceGroup(double prec = 5e-2);
    // Rotate the crystal 90 degrees around the given axis:
    void rotateX();
    void rotateY();
    void rotateZ();
    void rotate(const Eigen::Matrix3d &rot);
    void wrapAtomsToCell();
    void loadXtal(QTextStream &in);
    void saveXtal(QTextStream &in);

   private slots:

   private:
    bool m_hasEnergy, m_hasEnthalpy;
    void initializeCell();
    OpenBabel::OBUnitCell* cell() const;
    uint m_generation, m_id, m_rank, m_jobID, m_currentOptStep, m_failCount, m_spgNumber;
    int m_index;
    QString m_parents, m_dupString, m_rempath, m_spgSymbol;
    double m_enthalpy, m_PV;
    State m_status;
    QDateTime m_optStart, m_optEnd;
    XtalOpt *m_opt;
  };

} // end namespace Avogadro

#endif

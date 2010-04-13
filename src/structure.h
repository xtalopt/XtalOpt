/**********************************************************************
  Structure - Generic wrapper for Avogadro's molecule class

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

#ifndef STRUCTURE_H
#define STRUCTURE_H

#include <avogadro/molecule.h>

#include <openbabel/generic.h>
#include <openbabel/mol.h>
#include <openbabel/math/vector3.h>

#include <QDebug>
#include <QDateTime>
#include <QTextStream>

#define EV_TO_KCAL_PER_MOL 23.060538

namespace Avogadro {
  class Structure : public Molecule
  {
    Q_OBJECT

   public:
    Structure(QObject *parent = 0);
    virtual ~Structure();

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

    bool hasEnergy()	const {return m_hasEnergy;};
    bool hasEnthalpy()	const {return m_hasEnthalpy;};
    double getEnergy()	const {return energy(0)/EV_TO_KCAL_PER_MOL;};
    double getEnthalpy()const {if (!m_hasEnthalpy&&m_hasEnergy) return getEnergy(); return m_enthalpy;};
    double getPV()	const {return m_PV;};
    uint getRank()	const {return m_rank;};
    uint getJobID()	const {return m_jobID;};
    uint getGeneration() const {return m_generation;};
    uint getIDNumber() const {return m_id;};
    int getIndex() const {return m_index;};
    QString getDuplicateString() const {return m_dupString;};
    QString getParents() const {return m_parents;};
    QString getRempath() const {return m_rempath;};
    State getStatus()	const {return m_status;};
    uint getCurrentOptStep() {return m_currentOptStep;};
    uint getFailCount() { return m_failCount;};
    QDateTime getOptTimerStart() const {return m_optStart;};
    QDateTime getOptTimerEnd() const {return m_optEnd;};
    QString getIDString() const {return tr("%1x%2").arg(getGeneration()).arg(getIDNumber());};

    virtual bool getShortestInteratomicDistance(double & shortest) const;
    virtual bool getNearestNeighborDistance(double x, double y, double z, double & shortest) const;
    virtual bool getNearestNeighborHistogram(QList<double> & distance, QList<double> & frequency, double min, double max, double step, Atom *atom = 0) const;
    virtual bool addAtomRandomly(uint atomicNumber, double minIAD = 0.0, double maxIAD = 0.0, double maxAttempts = 100.0);

    QList<QString> getSymbols() const;
    QList<uint> getNumberOfAtomsAlpha() const; // Number of each type of atom (sorted alphabetically by symbol)

    QString getOptElapsed() const;

    virtual QHash<QString, double> getFingerprint();

   signals:

   public slots:
    void setEnthalpy(double enthalpy) {m_hasEnthalpy = true; m_enthalpy = enthalpy;};
    void setPV(double pv) {m_PV = pv;};
    void setEnergy(double e) {m_hasEnergy = true; std::vector<double> E; E.push_back(e); setEnergies(E);};
    void setOBEnergy(const QString &ff = QString("UFF"));
    void setRank(uint rank) {m_rank = rank;};
    void setJobID(uint id) {m_jobID = id;};
    void setGeneration(uint gen) {m_generation = gen;};
    void setIDNumber(uint id) {m_id = id;};
    void setIndex(int index) {m_index = index;};
    void setParents(const QString & p) {m_parents = p;};
    void setRempath(const QString & p) {m_rempath = p;};
    void setStatus(State status) {m_status = status;};
    void setCurrentOptStep(uint i) {m_currentOptStep = i;};
    void setFailCount(uint count) {m_failCount = count;};
    void resetFailCount() {setFailCount(0);};
    void addFailure() {setFailCount(getFailCount() + 1);};
    void setDuplicateString(const QString & s) {m_dupString = s;};
    void startOptTimer() {m_optStart = QDateTime::currentDateTime(); m_optEnd = QDateTime();};
    void stopOptTimer() {if (m_optEnd.isNull()) m_optEnd = QDateTime::currentDateTime();};
    void setOptTimerStart(const QDateTime &d) {m_optStart = d;};
    void setOptTimerEnd(const QDateTime &d) {m_optEnd = d;};
    virtual void load(QTextStream &in);
    virtual void save(QTextStream &in);

   private slots:

   private:
    bool m_hasEnergy, m_hasEnthalpy;
    uint m_generation, m_id, m_rank, m_jobID, m_currentOptStep, m_failCount;
    QString m_parents, m_dupString, m_rempath;
    double m_enthalpy, m_PV;
    State m_status;
    QDateTime m_optStart, m_optEnd;
    int m_index;
  };

} // end namespace Avogadro

#endif

/**********************************************************************
  Structure - Wrapper for Avogadro's molecule class

  Copyright (C) 2009-2010 by David C. Lonie

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

#include "structure.h"

#include <avogadro/primitive.h>
#include <avogadro/molecule.h>
#include <avogadro/atom.h>

#include <openbabel/generic.h>
#include <openbabel/rand.h>
#include <openbabel/forcefield.h>

#include <QDebug>
#include <QRegExp>
#include <QStringList>

using namespace std;
using namespace OpenBabel;
using namespace Eigen;

namespace Avogadro {

  Structure::Structure(QObject *parent) :
    Molecule(parent),
    m_generation(0),
    m_id(0),
    m_jobID(0),
    m_PV(0),
    m_optStart(QDateTime()),
    m_optEnd(QDateTime()),
    m_index(-1)
  {
    m_hasEnergy = m_hasEnthalpy = false;
    m_currentOptStep = 1;
    setStatus(Empty);
    resetFailCount();
  }

  Structure::~Structure() {
  }

  bool Structure::addAtomRandomly(uint atomicNumber, double minIAD, double maxIAD, double maxAttempts) {
    Q_UNUSED(atomicNumber);
    Q_UNUSED(minIAD);
    Q_UNUSED(maxIAD);
    Q_UNUSED(maxAttempts);
    // Treat this function as pure virtual for now. Should eventually
    // be used for non-extended systems.
    qWarning() << "WARNING: Structure::addAtomRandomly called. Should be pure virtual for now. This is a bug.";
    return false;
  }

  void Structure::setOBEnergy(const QString &ff) {
    OBForceField* pFF = OBForceField::FindForceField(ff.toStdString().c_str());
    if (!pFF) return;
    OpenBabel::OBMol obmol = OBMol();
    if (!pFF->Setup(obmol)) {
      qWarning() << "Structure::setOBEnergy: could not setup force field " << ff << ".";
      return;
    }
    std::vector<double> E;
    E.push_back(pFF->Energy());
    setEnergies(E);
    m_hasEnergy = true;
  }

  bool Structure::getShortestInteratomicDistance(double & shortest) const {
    Q_UNUSED(shortest);
    // Treat this function as pure virtual for now. Should eventually
    // be used for non-extended systems.
    qWarning() << "WARNING: Structure::getShortestInteratomicDistance called. Should be pure virtual for now. This is a bug.";
    return false;
  }

  bool Structure::getNearestNeighborDistance(double x, double y, double z, double & shortest) const {
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(z);
    Q_UNUSED(shortest);
    // Treat this function as pure virtual for now. Should eventually
    // be used for non-extended systems.
    qWarning() << "WARNING: Structure::getNearestNeighborDistance called. Should be pure virtual for now. This is a bug.";
    return false;
  }

  bool Structure::getNearestNeighborHistogram(QList<double> & distance, QList<double> & frequency, double min, double max, double step, Atom *atom) const {
    Q_UNUSED(distance);
    Q_UNUSED(frequency);
    Q_UNUSED(min);
    Q_UNUSED(max);
    Q_UNUSED(step);
    Q_UNUSED(atom);
    // Treat this function as pure virtual for now. Should eventually
    // be used for non-extended systems.
    qWarning() << "WARNING: Structure::getNearestNeighborHistogram called. Should be pure virtual for now. This is a bug.";
    return false;
  }

  QList<QString> Structure::getSymbols() const {
    QList<QString> list;
    OpenBabel::OBMol obmol = OBMol();
    FOR_ATOMS_OF_MOL(atom,obmol) {
      QString symbol = QString(OpenBabel::etab.GetSymbol(atom->GetAtomicNum()));
      if (!list.contains(symbol)) {
        list.append(symbol);
      }
    }
    qSort(list);
    return list;
  }

  QList<uint> Structure::getNumberOfAtomsAlpha() const {
    QList<uint> list;
    QList<QString> symbols = getSymbols();
    for (int i = 0; i < symbols.size(); i++)
      list.append(0);
    OpenBabel::OBMol obmol = OBMol();
    int ind, tmp;
    FOR_ATOMS_OF_MOL(atom,obmol) {
      QString symbol            = QString(OpenBabel::etab.GetSymbol(atom->GetAtomicNum()));
      ind = symbols.indexOf(symbol);
      tmp = list.at(ind);
      tmp++;
      list.replace(ind, tmp);
    }
    return list;
  }

  QString Structure::getOptElapsed() const {
    int secs;
    if (m_optStart.toString() == "") return "0:00:00";
    if (m_optEnd.toString() == "")
      secs = m_optStart.secsTo(QDateTime::currentDateTime());
    else
      secs = m_optStart.secsTo(m_optEnd);
    int hours   = static_cast<int>(secs/3600);
    int mins    = static_cast<int>( (secs - hours * 3600) / 60);
    secs        = secs % 60;
    QString ret;
    ret.sprintf("%d:%02d:%02d", hours, mins, secs);
    return ret;
  }

  void Structure::save(QTextStream &out) {
    out << "Generation: " << getGeneration() << endl
        << "ID#: " << getIDNumber() << endl
        << "Index: " << getIndex() << endl
        << "Enthalpy Rank: " << getRank() << endl
        << "Job ID: " << getJobID() << endl
        << "Current OptStep: " << getCurrentOptStep() << endl
        << "Ancestry: " << getParents() << endl
        << "Remote Path: " << getRempath() << endl
        << "Status: " << getStatus() << endl
        << "failCount: " << getFailCount() << endl
        << "Start Time: " << getOptTimerStart().toString() << endl
        << "End Time: " << getOptTimerEnd().toString() << endl;
  }

  void Structure::load(QTextStream &in) {
    QString line, str;
    QStringList strl;
    setStatus(InProcess); // Override later if status is available
    while (!in.atEnd()) {
      line = in.readLine();
      strl = line.split(QRegExp(" "));
      //qDebug() << strl;

      if (line.contains("Generation:") && strl.size() > 1)
        setGeneration( (strl.at(1)).toUInt() );
      if (line.contains("ID#:") && strl.size() > 1)
        setIDNumber( (strl.at(1)).toUInt() );
      if (line.contains("Index:") && strl.size() > 1)
        setIndex( (strl.at(1)).toUInt() );
      if (line.contains("Enthalpy Rank:") && strl.size() > 2)
        setRank( (strl.at(2)).toUInt() );
      if (line.contains("Job ID:") && strl.size() > 2)
        setJobID( (strl.at(2)).toUInt() );
      if (line.contains("Current INCAR:") && strl.size() > 2)
        setCurrentOptStep( (strl.at(2)).toUInt() );
      if (line.contains("Current OptStep:") && strl.size() > 2)
        setCurrentOptStep( (strl.at(2)).toUInt() );
      if (line.contains("Ancestry:") && strl.size() > 1) {
        strl.removeFirst();
        setParents( strl.join(" ") );
      }
      if (line.contains("Remote Path:") && strl.size() > 2) {
        strl.removeFirst(); strl.removeFirst();
        setRempath( strl.join(" ") );
      }
      if (line.contains("Start Time:") && strl.size() > 2) {
        strl.removeFirst(); strl.removeFirst();
        str = strl.join(" ");
        setOptTimerStart(QDateTime::fromString(str));
      }
      if (line.contains("Status:") && strl.size() > 1) {
        setStatus( Structure::State((strl.at(1)).toInt() ));
      }
      if (line.contains("failCount:") && strl.size() > 1) {
        setFailCount((strl.at(1)).toUInt());
      }
      if (line.contains("End Time:") && strl.size() > 2) {
        strl.removeFirst(); strl.removeFirst();
        str = strl.join(" ");
        setOptTimerEnd(QDateTime::fromString(str));
      }
    }
  }

  QHash<QString, double> Structure::getFingerprint() {
    // Treat this function as pure virtual for now. Should eventually
    // be used for non-extended systems.
    qWarning() << "WARNING: Structure::getFingerprint called. Should be pure virtual for now. This is a bug.";
    return QHash<QString, double> ();
  }
} // end namespace Avogadro

#include "structure.moc"

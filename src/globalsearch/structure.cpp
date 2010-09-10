/**********************************************************************
  Structure - Wrapper for Avogadro's molecule class

  Copyright (C) 2009-2010 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <globalsearch/structure.h>
#include <globalsearch/macros.h>

#include <avogadro/primitive.h>
#include <avogadro/molecule.h>
#include <avogadro/atom.h>

#include <openbabel/generic.h>
#include <openbabel/rand.h>
#include <openbabel/forcefield.h>

#include <QFile>
#include <QDebug>
#include <QRegExp>
#include <QStringList>

using namespace OpenBabel;
using namespace Eigen;

namespace GlobalSearch {

  Structure::Structure(QObject *parent) :
    Molecule(parent),
    m_generation(0),
    m_id(0),
    m_rank(0),
    m_jobID(0),
    m_PV(0),
    m_optStart(QDateTime()),
    m_optEnd(QDateTime()),
    m_index(-1)
  {
    m_hasEnthalpy = false;
    m_currentOptStep = 1;
    setStatus(Empty);
    resetFailCount();
  }

  Structure::Structure(const Structure &other) :
    Molecule(other),
    m_generation(0),
    m_id(0),
    m_rank(0),
    m_jobID(0),
    m_PV(0),
    m_optStart(QDateTime()),
    m_optEnd(QDateTime()),
    m_index(-1)
  {
    *this = other;
  }


  Structure::~Structure() {
  }

  Structure& Structure::operator=(const Structure& other)
  {
    Molecule::operator=(other);

    // Set properties
    m_hasEnthalpy    = other.m_hasEnthalpy;
    m_generation     = other.m_generation;
    m_id             = other.m_id;
    m_rank           = other.m_rank;
    m_jobID          = other.m_jobID;
    m_currentOptStep = other.m_currentOptStep;
    m_failCount      = other.m_failCount;
    m_parents        = other.m_parents;
    m_dupString      = other.m_dupString;
    m_rempath        = other.m_rempath;
    m_enthalpy       = other.m_enthalpy;
    m_PV             = other.m_PV;
    m_status         = other.m_status;
    m_optStart       = other.m_optStart;
    m_optEnd         = other.m_optEnd;
    m_index          = other.m_index;

    return *this;
  }

  Structure& Structure::copyStructure(Structure *other)
  {
    Molecule::operator=(*other);
    return *this;
  }

  void Structure::writeStructureSettings(const QString &filename)
  {
    SETTINGS(filename);
    const int VERSION = 1;
    settings->beginGroup("structure");
    settings->setValue("version",     VERSION);
    settings->setValue("generation", getGeneration());
    settings->setValue("id", getIDNumber());
    settings->setValue("index", getIndex());
    settings->setValue("rank", getRank());
    settings->setValue("jobID", getJobID());
    settings->setValue("currentOptStep", getCurrentOptStep());
    settings->setValue("parents", getParents());
    settings->setValue("rempath", getRempath());
    settings->setValue("status", int(getStatus()));
    settings->setValue("failCount", getFailCount());
    settings->setValue("startTime", getOptTimerStart().toString());
    settings->setValue("endTime", getOptTimerEnd().toString());
    settings->endGroup();
    DESTROY_SETTINGS(filename);
  }

  void Structure::readStructureSettings(const QString &filename)
  {
    SETTINGS(filename);
    settings->beginGroup("structure");
    int loadedVersion = settings->value("version", 0).toInt();
    if (loadedVersion >= 1) { // Version 0 uses save(QTextStream)
      setGeneration(     settings->value("generation",     0).toInt());
      setIDNumber(       settings->value("id",             0).toInt());
      setIndex(          settings->value("index",          0).toInt());
      setRank(           settings->value("rank",           0).toInt());
      setJobID(          settings->value("jobID",          0).toInt());
      setCurrentOptStep( settings->value("currentOptStep", 0).toInt());
      setFailCount(      settings->value("failCount",      0).toInt());
      setParents(        settings->value("parents",        "").toString());
      setRempath(        settings->value("rempath",        "").toString());
      setStatus(   State(settings->value("status",         -1).toInt()));

      setOptTimerStart( QDateTime::fromString(settings->value("startTime", "").toString()));
      setOptTimerEnd(   QDateTime::fromString(settings->value("endTime",   "").toString()));
    }
    settings->endGroup();

    // Update config data
    switch (loadedVersion) {
    case 0: {
      // Call load(QTextStream) to update
      qDebug() << "Updating "
               << filename
               << " from Version 0 -> 1";
      QFile file (filename);
      file.open(QIODevice::ReadOnly);
      QTextStream stream (&file);
      load(stream);
    }
    case 1:
    default:
      break;
    }
  }

  bool Structure::addAtomRandomly(uint atomicNumber, double minIAD, double maxIAD, int maxAttempts, Atom **atom) {
    OpenBabel::OBRandom rand (true);    // "true" uses system random numbers.
    rand.TimeSeed();

    double IAD = -1;
    int i = 0;
    vector3 coords;

    // For first atom, add to 0, 0, 0
    if (numAtoms() == 0) {
      coords = vector3 (0,0,0);
    }
    else {
      do {
        // Generate random coordinates
        IAD = -1;
        double x = rand.NextFloat() * radius() + maxIAD;
        double y = rand.NextFloat() * radius() + maxIAD;
        double z = rand.NextFloat() * radius() + maxIAD;

        coords.Set(x,y,z);
        if (minIAD != -1) {
          getNearestNeighborDistance(x, y, z, IAD);
        }
        else { break;};
        i++;
      } while (i < maxAttempts && IAD <= minIAD);

      if (i >= maxAttempts) return false;
    }

    Atom *atm = addAtom();
    atom = &atm;
    Eigen::Vector3d pos (coords[0],coords[1],coords[2]);
    (*atom)->setPos(pos);
    (*atom)->setAtomicNumber(static_cast<int>(atomicNumber));
    return true;
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
  }

  bool Structure::getShortestInteratomicDistance(double & shortest) const
  {
    QList<Atom*> atomList = atoms();
    if (atomList.size() <= 1) return false; // Need at least two atoms!
    QList<Vector3d> atomPositions;
    for (int i = 0; i < atomList.size(); i++)
      atomPositions.push_back(*(atomList.at(i)->pos()));

    Vector3d v1= atomPositions.at(0);
    Vector3d v2= atomPositions.at(1);
    shortest = abs((v1-v2).norm());
    double distance;

    // Find shortest distance
    for (int i = 0; i < atomList.size(); i++) {
      v1 = atomPositions.at(i);
      for (int j = i+1; j < atomList.size(); j++) {
        v2 = atomPositions.at(j);
        // Intercell
        distance = abs((v1-v2).norm());
        if (distance < shortest) shortest = distance;
      }
    }

    return true;
  }

  bool Structure::getNearestNeighborDistance(double x, double y, double z, double & shortest) const {
    QList<Atom*> atomList = atoms();
    if (atomList.size() < 1) return false; // Need at least one atom!
    QList<Vector3d> atomPositions;
    for (int i = 0; i < atomList.size(); i++)
      atomPositions.push_back(*(atomList.at(i)->pos()));

    Vector3d v1 (x, y, z);
    Vector3d v2 = atomPositions.at(0);
    shortest = abs((v1-v2).norm());
    double distance;

    // Find shortest distance
    for (int j = 0; j < atomList.size(); j++) {
      v2 = atomPositions.at(j);
      // Intercell
      distance = abs((v1-v2).norm());
      if (distance < shortest) shortest = distance;
    }
    return true;
  }

  bool Structure::getNearestNeighborHistogram(QList<double> & distance, QList<double> & frequency, double min, double max, double step, Atom *atom) const
  {
    if (min > max && step > 0) {
      qWarning() << "Structure::getNearestNeighborHistogram: min cannot be greater than max!";
      return false;
    }
    if (step < 0 || step == 0) {
      qWarning() << "Structure::getNearestNeighborHistogram: invalid step size:" << step;
      return false;
    }
    if (numAtoms() < 1) return false; // Need at least one atom!

    // Populate distance list
    distance.clear();
    frequency.clear();
    double val = min;
    do {
      distance.append(val);
      frequency.append(0);
      val += step;
    } while (val < max);

    QList<Atom*> atomList = atoms();
    QList<Vector3d> atomPositions;
    for (int i = 0; i < atomList.size(); i++)
      atomPositions.push_back(*(atomList.at(i)->pos()));

    Vector3d v1= atomPositions.at(0);
    Vector3d v2= atomPositions.at(1);
    double diff;

    // build histogram
    // Loop over all atoms
    if (atom == 0) {
      for (int i = 0; i < atomList.size(); i++) {
        v1 = atomPositions.at(i);
        for (int j = i+1; j < atomList.size(); j++) {
          v2 = atomPositions.at(j);

          diff = abs((v1-v2).norm());
          for (int k = 0; k < distance.size(); k++) {
            double radius = distance.at(k);
            if (abs(diff-radius) < step/2) {
              frequency[k]++;
            }
          }
        }
      }
    }
    // Or, just the one requested
    else {
      v1 = *atom->pos();
      for (int j = 0; j < atomList.size(); j++) {
        if (atomList.at(j) == atom) continue;
        v2 = atomPositions.at(j);
        // Intercell
        diff = abs((v1-v2).norm());
        for (int k = 0; k < distance.size(); k++) {
          double radius = distance.at(k);
          if (diff != 0 && abs(diff-radius) < step/2) {
            frequency[k]++;
          }
        }
      }
    }

    return true;
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

  void Structure::load(QTextStream &in) {
    QString line, str;
    QStringList strl;
    setStatus(InProcess); // Override later if status is available
    in.seek(0);
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

//#include "structure.moc"

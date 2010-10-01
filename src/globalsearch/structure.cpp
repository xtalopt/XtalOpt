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
#include <openbabel/forcefield.h>

#include <QtCore/QFile>
#include <QtCore/QDebug>
#include <QtCore/QRegExp>
#include <QtCore/QStringList>
#include <QtCore/QtConcurrentMap>

using namespace OpenBabel;
using namespace Eigen;
using namespace std;

namespace GlobalSearch {

  Structure::Structure(QObject *parent) :
    Molecule(parent),
    m_hasEnthalpy(false),
    m_histogramGenerationPending(false),
    m_generation(0),
    m_id(0),
    m_rank(0),
    m_jobID(0),
    m_PV(0),
    m_optStart(QDateTime()),
    m_optEnd(QDateTime()),
    m_index(-1)
  {
    m_currentOptStep = 1;
    setStatus(Empty);
    resetFailCount();
  }

  Structure::Structure(const Structure &other) :
    Molecule(other),
    m_histogramGenerationPending(false),
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
    m_histogramGenerationPending = other.m_histogramGenerationPending;
    m_hasEnthalpy                = other.m_hasEnthalpy;
    m_generation                 = other.m_generation;
    m_id                         = other.m_id;
    m_rank                       = other.m_rank;
    m_jobID                      = other.m_jobID;
    m_currentOptStep             = other.m_currentOptStep;
    m_failCount                  = other.m_failCount;
    m_parents                    = other.m_parents;
    m_dupString                  = other.m_dupString;
    m_rempath                    = other.m_rempath;
    m_enthalpy                   = other.m_enthalpy;
    m_PV                         = other.m_PV;
    m_status                     = other.m_status;
    m_optStart                   = other.m_optStart;
    m_optEnd                     = other.m_optEnd;
    m_index                      = other.m_index;

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
    INIT_RANDOM_GENERATOR();
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
        double x = RANDDOUBLE() * radius() + maxIAD;
        double y = RANDDOUBLE() * radius() + maxIAD;
        double z = RANDDOUBLE() * radius() + maxIAD;

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

  QString Structure::getResultsEntry() const
  {
    QString status;
    switch (getStatus()) {
    case Optimized:
      status = "Optimized";
      break;
    case Killed:
    case Removed:
      status = "Killed";
      break;
    case Duplicate:
      status = "Duplicate";
      break;
    case Error:
      status = "Error";
      break;
    case StepOptimized:
    case WaitingForOptimization:
    case InProcess:
    case Empty:
    case Updating:
    case Submitted:
    default:
      status = "In progress";
      break;
    }
    return QString("%1 %2 %3 %4 %5")
      .arg(getRank(), 6)
      .arg(getGeneration(), 6)
      .arg(getIDNumber(), 6)
      .arg(getEnthalpy(), 10)
      .arg(status, 11);
  };

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
    shortest = fabs((v1-v2).norm());
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

  void Structure::enableAutoHistogramGeneration(bool b) {
    if (b) {
      connect(this, SIGNAL(updated()),
              this, SLOT(requestHistogramGeneration()),
              Qt::QueuedConnection);
      connect(this, SIGNAL(atomAdded(Atom*)),
              this, SLOT(requestHistogramGeneration()),
              Qt::QueuedConnection);
      connect(this, SIGNAL(atomUpdated(Atom*)),
              this, SLOT(requestHistogramGeneration()),
              Qt::QueuedConnection);
      connect(this, SIGNAL(atomRemoved(Atom*)),
              this, SLOT(requestHistogramGeneration()),
              Qt::QueuedConnection);
      connect(this, SIGNAL(bondAdded(Bond*)),
              this, SLOT(requestHistogramGeneration()),
              Qt::QueuedConnection);
      connect(this, SIGNAL(bondUpdated(Bond*)),
              this, SLOT(requestHistogramGeneration()),
              Qt::QueuedConnection);
      connect(this, SIGNAL(bondRemoved(Bond*)),
              this, SLOT(requestHistogramGeneration()),
              Qt::QueuedConnection);
      connect(this, SIGNAL(primitiveAdded(Primitive*)),
              this, SLOT(requestHistogramGeneration()),
              Qt::QueuedConnection);
      connect(this, SIGNAL(primitiveUpdated(Primitive*)),
              this, SLOT(requestHistogramGeneration()),
              Qt::QueuedConnection);
      connect(this, SIGNAL(primitiveRemoved(Primitive*)),
              this, SLOT(requestHistogramGeneration()),
              Qt::QueuedConnection);
    } else {
      disconnect(this, SIGNAL(updated()),
                 this, SLOT(requestHistogramGeneration()));
      disconnect(this, SIGNAL(atomAdded(Atom*)),
                 this, SLOT(requestHistogramGeneration()));
      disconnect(this, SIGNAL(atomUpdated(Atom*)),
                 this, SLOT(requestHistogramGeneration()));
      disconnect(this, SIGNAL(atomRemoved(Atom*)),
                 this, SLOT(requestHistogramGeneration()));
      disconnect(this, SIGNAL(bondAdded(Bond*)),
                 this, SLOT(requestHistogramGeneration()));
      disconnect(this, SIGNAL(bondUpdated(Bond*)),
                 this, SLOT(requestHistogramGeneration()));
      disconnect(this, SIGNAL(bondRemoved(Bond*)),
                 this, SLOT(requestHistogramGeneration()));
      disconnect(this, SIGNAL(primitiveAdded(Primitive*)),
                 this, SLOT(requestHistogramGeneration()));
      disconnect(this, SIGNAL(primitiveUpdated(Primitive*)),
                 this, SLOT(requestHistogramGeneration()));
      disconnect(this, SIGNAL(primitiveRemoved(Primitive*)),
                 this, SLOT(requestHistogramGeneration()));
    }
  }

  void Structure::requestHistogramGeneration()
  {
    if (!m_histogramGenerationPending) {
      m_histogramGenerationPending = true;
      // Wait 250 ms before requesting to limit number of requests
      QTimer::singleShot(250, this, SLOT(generateDefaultHistogram()));
    }
  }

  void Structure::generateDefaultHistogram()
  {
    generateIADHistogram(&m_histogramDist, &m_histogramFreq, 0, 10, 0.01);
    m_histogramGenerationPending = false;
  }

  void Structure::getDefaultHistogram(QList<double> *dist, QList<double> *freq) const
  {
    dist->clear();
    freq->clear();
    for (int i = 0; i < m_histogramDist.size(); i++) {
      dist->append(m_histogramDist.at(i).toDouble());
      freq->append(m_histogramFreq.at(i).toDouble());
    }
  }

  void Structure::getDefaultHistogram(QList<QVariant> *dist, QList<QVariant> *freq) const
  {
    (*dist) = m_histogramDist;
    (*freq) = m_histogramFreq;
  }


  bool Structure::generateIADHistogram(QList<double> * dist,
                                       QList<double> * freq,
                                       double min,
                                       double max,
                                       double step,
                                       Atom *atom) const
  {
    QList<QVariant> distv, freqv;
    if (!generateIADHistogram(&distv, &freqv, min, max, step, atom)) {
      return false;
    }
    dist->clear();
    freq->clear();
    for (int i = 0; i < distv.size(); i++) {
      dist->append(distv.at(i).toDouble());
      freq->append(freqv.at(i).toDouble());
    }
    return true;
  }

  // Helper functions and structs for the histogram generator
  struct NNHistMap {
    int i;
    double step;
    QList<Vector3d> *atomPositions;
    QList<QVariant> *dist;
  };

  // Returns the frequencies for this chunk
  QList<int> calcNNHistChunk(const NNHistMap &m)
  {
    const Vector3d *v1 = &(m.atomPositions->at(m.i));
    const Vector3d *v2;
    QList<int> freq;
    double diff, radius;
    for (int ind = 0; ind < m.dist->size(); ind++) {
      freq.append(0);
    }
    for (int j = m.i+1; j < m.atomPositions->size(); j++) {
      v2 = &(m.atomPositions->at(j));
      diff = fabs(((*v1)-(*v2)).norm());
      for (int k = 0; k < m.dist->size(); k++) {
        if (fabs(diff-(m.dist->at(k).toDouble())) < m.step/2) {
          freq[k]++;
        }
      }
    }
    return freq;
  }

  QList<QVariant> reduceNNHistChunks(QList<QVariant> &final, const QList<int> &tmp)
  {
    if (final.size() != tmp.size()) {
      final.clear();
      for (int i = 0; i < tmp.size(); i++) {
        final.append(tmp.at(i));
      }
    }
    else {
      double d;
      for (int i = 0; i < final.size(); i++) {
        d = final.at(i).toDouble();
        d += tmp.at(i);
        final.replace(i, d);
      }
    }
    return final;
  }

  bool Structure::generateIADHistogram(QList<QVariant> * distance,
                                       QList<QVariant> * frequency,
                                       double min, double max, double step,
                                       Atom *atom) const
  {
    distance->clear();
    frequency->clear();

    if (min > max && step > 0) {
      qWarning() << "Structure::getNearestNeighborHistogram: min cannot be greater than max!";
      return false;
    }
    if (step <= 0) {
      qWarning() << "Structure::getNearestNeighborHistogram: invalid step size:" << step;
      return false;
    }

    double val = min;
    do {
      distance->append(val);
      frequency->append(0);
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
    // Loop over all atoms -- use map-reduce
    if (atom == 0) {
      QList<NNHistMap> ml;
      for (int i = 0; i < atomList.size(); i++) {
        NNHistMap m;
        m.i = i; m.step = step; m.atomPositions = &atomPositions; m.dist = distance;
        ml.append(m);
      }
      (*frequency) = QtConcurrent::blockingMappedReduced(ml, calcNNHistChunk, reduceNNHistChunks);
    }
    // Or, just the one requested
    else {
      v1 = *atom->pos();
      for (int j = 0; j < atomList.size(); j++) {
        if (atomList.at(j) == atom) continue;
        v2 = atomPositions.at(j);
        // Intercell
        diff = fabs((v1-v2).norm());
        for (int k = 0; k < distance->size(); k++) {
          double radius = distance->at(k).toDouble();
          double d;
          if (diff != 0 && fabs(diff-radius) < step/2) {
            d = frequency->at(k).toDouble();
            d++;
            frequency->replace(k, d);
          }
        }
      }
    }

    return true;
  }

  bool Structure::compareIADDistributions(const vector<double> &d,
                                          const vector<double> &f1,
                                          const vector<double> &f2,
                                          double decay,
                                          double smear,
                                          double *error)
  {
    // Check that smearing is possible
    if (smear != 0 && d.size() <= 1) {
      qWarning() << "Cluster::compareNNDist: Cannot smear with 1 or fewer points.";
      return false;
    }
    // Check sizes
    if (d.size() != f1.size() || f1.size() != f2.size()) {
      qWarning() << "Cluster::compareNNDist: Vectors are not the same size.";
      return false;
    }

    // Perform a boxcar smoothing over range set by "smear"
    // First determine step size of d, then convert smear to index units
    double stepSize = fabs(d.at(1) - d.at(0));
    int boxSize = ceil(smear/stepSize);
    if (boxSize > d.size()) {
      qWarning() << "Cluster::compareNNDist: Smear length is greater then d vector range.";
      return false;
    }
    // Smear
    vector<double> f1s, f2s, ds; // smeared vectors
    if (smear != 0) {
      double f1t, f2t, dt; // temporary variables
      for (int i = 0; i < d.size() - boxSize; i++) {
        f1t = f2t = dt = 0;
        for (int j = 0; j < boxSize; j++) {
          f1t += f1.at(i+j);
          f2t += f2.at(i+j);
        }
        f1s.push_back(f1t / double(boxSize));
        f2s.push_back(f2t / double(boxSize));
        ds.push_back(dt / double(boxSize));
      }
    } else {
      for (int i = 0; i < d.size() - boxSize; i++) {
        f1s.push_back(f1.at(i));
        f2s.push_back(f2.at(i));
        ds.push_back(d.at(i));
      }
    }

    // Calculate diff vector
    vector<double> diff;
    for (int i = 0; i < ds.size(); i++) {
      diff.push_back(fabs(f1s.at(i) - f2s.at(i)));
    }

    // Calculate decay function: Standard exponential decay with a
    // halflife of decay. If decay==0, no decay.
    double decayFactor = 0;
    // ln(2) / decay:
    if (decay != 0) {
      decayFactor = 0.69314718055994530941723 / decay;
    }

    // Calculate error:
    (*error) = 0;
    for (int i = 0; i < ds.size(); i++) {
      (*error) += exp(-decayFactor * ds.at(i)) * diff.at(i);
    }

    return true;
  }

  bool Structure::compareIADDistributions(const QList<double> &d,
                                          const QList<double> &f1,
                                          const QList<double> &f2,
                                          double decay,
                                          double smear,
                                          double *error)
  {
    // Check sizes
    if (d.size() != f1.size() || f1.size() != f2.size()) {
      qWarning() << "Cluster::compareIADDist: Vectors are not the same size.";
      return false;
    }
    vector<double> dd, f1d, f2d;
    for (int i = 0; i < d.size(); i++) {
      dd.push_back(d.at(i));
      f1d.push_back(f1.at(i));
      f2d.push_back(f2.at(i));
    }
    return compareIADDistributions(dd, f1d, f2d, decay, smear, error);
  }

  bool Structure::compareIADDistributions(const QList<QVariant> &d,
                                          const QList<QVariant> &f1,
                                          const QList<QVariant> &f2,
                                          double decay,
                                          double smear,
                                          double *error)
  {
    // Check sizes
    if (d.size() != f1.size() || f1.size() != f2.size()) {
      qWarning() << "Cluster::compareIADDist: Vectors are not the same size.";
      return false;
    }
    vector<double> dd, f1d, f2d;
    for (int i = 0; i < d.size(); i++) {
      dd.push_back(d.at(i).toDouble());
      f1d.push_back(f1.at(i).toDouble());
      f2d.push_back(f2.at(i).toDouble());
    }
    return compareIADDistributions(dd, f1d, f2d, decay, smear, error);
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

  QHash<QString, QVariant> Structure::getFingerprint() const
  {
    QHash<QString, QVariant> fp;
    fp.insert("enthalpy", getEnthalpy());
    return fp;
  }

  void Structure::sortByEnthalpy(QList<Structure*> *structures)
  {
    uint numStructs = structures->size();

    // Simple selection sort
    Structure *structure_i=0, *structure_j=0, *tmp=0;
    for (uint i = 0; i < numStructs-1; i++) {
      structure_i = structures->at(i);
      structure_i->lock()->lockForRead();
      for (uint j = i+1; j < numStructs; j++) {
        structure_j = structures->at(j);
        structure_j->lock()->lockForRead();
        if (structure_j->getEnthalpy() < structure_i->getEnthalpy()) {
          structures->swap(i,j);
          tmp = structure_i;
          structure_i = structure_j;
          structure_j = tmp;
        }
        structure_j->lock()->unlock();
      }
      structure_i->lock()->unlock();
    }
  }

  void rankInPlace(const QList<Structure*> &structures)
  {
    Structure *s;
    for (uint i = 0; i < structures.size(); i++) {
      s = structures.at(i);
      s->lock()->lockForWrite();
      s->setRank(i+1);
      s->lock()->unlock();
    }
  }

  void Structure::rankByEnthalpy(const QList<Structure*> &structures)
  {
    uint numStructs = structures.size();
    QList<Structure*> rstructures;

    // Copy structures to a temporary list (don't modify input list!)
    for (uint i = 0; i < numStructs; i++)
      rstructures.append(structures.at(i));

    // Simple selection sort
    Structure *structure_i=0, *structure_j=0, *tmp=0;
    for (uint i = 0; i < numStructs-1; i++) {
      structure_i = rstructures.at(i);
      structure_i->lock()->lockForRead();
      for (uint j = i+1; j < numStructs; j++) {
        structure_j = rstructures.at(j);
        structure_j->lock()->lockForRead();
        if (structure_j->getEnthalpy() < structure_i->getEnthalpy()) {
          rstructures.swap(i,j);
          tmp = structure_i;
          structure_i = structure_j;
          structure_j = tmp;
        }
        structure_j->lock()->unlock();
      }
      structure_i->lock()->unlock();
    }

    rankInPlace(rstructures);
  }

  void Structure::sortAndRankByEnthalpy(QList<Structure*> *structures)
  {
    sortByEnthalpy(structures);
    rankInPlace(*structures);
  }


} // end namespace GlobalSearch

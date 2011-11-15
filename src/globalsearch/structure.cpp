/**********************************************************************
  Structure - Wrapper for Avogadro's molecule class

  Copyright (C) 2009-2011 by David C. Lonie

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
#include <QtCore/QTimer>
#include <QtCore/QRegExp>
#include <QtCore/QStringList>
#include <QtCore/QtConcurrentMap>

#define KCAL_PER_MOL_TO_EV 0.043364122

using namespace Avogadro;
using namespace OpenBabel;
using namespace Eigen;
using namespace std;

namespace GlobalSearch {

  Structure::Structure(QObject *parent) :
    Molecule(parent),
    m_hasEnthalpy(false),
    m_updatedSinceDupChecked(true),
    m_histogramGenerationPending(false),
    m_hasBestOffspring(false),
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
    m_updatedSinceDupChecked(true),
    m_hasBestOffspring(false),
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

  Structure::Structure(const Avogadro::Molecule &other) :
    Molecule(other),
    m_histogramGenerationPending(false),
    m_updatedSinceDupChecked(true),
    m_hasBestOffspring(false),
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

  void Structure::setupConnections()
  {
    // List of slots to be called each time the structure changes
    QList<const char*> slotlist;
    slotlist.append(SLOT(structureChanged()));
    for (int i = 0; i < slotlist.size(); i++) {
      connect(this, SIGNAL(updated()),
              this, slotlist.at(i),
              Qt::QueuedConnection);
      connect(this, SIGNAL(atomAdded(Atom*)),
              this, slotlist.at(i),
              Qt::QueuedConnection);
      connect(this, SIGNAL(atomUpdated(Atom*)),
              this, slotlist.at(i),
              Qt::QueuedConnection);
      connect(this, SIGNAL(atomRemoved(Atom*)),
              this, slotlist.at(i),
              Qt::QueuedConnection);
      connect(this, SIGNAL(bondAdded(Bond*)),
              this, slotlist.at(i),
              Qt::QueuedConnection);
      connect(this, SIGNAL(bondUpdated(Bond*)),
              this, slotlist.at(i),
              Qt::QueuedConnection);
      connect(this, SIGNAL(bondRemoved(Bond*)),
              this, slotlist.at(i),
              Qt::QueuedConnection);
      connect(this, SIGNAL(primitiveAdded(Primitive*)),
              this, slotlist.at(i),
              Qt::QueuedConnection);
      connect(this, SIGNAL(primitiveUpdated(Primitive*)),
              this, slotlist.at(i),
              Qt::QueuedConnection);
      connect(this, SIGNAL(primitiveRemoved(Primitive*)),
              this, slotlist.at(i),
              Qt::QueuedConnection);
    }
  }

  void Structure::enableAutoHistogramGeneration(bool b)
  {
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

  Structure::~Structure()
  {
  }

  Structure& Structure::operator=(const Structure& other)
  {
    this->copyStructure(other);

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

  Structure& Structure::operator=(const Avogadro::Molecule &mol)
  {
    this->copyStructure(mol);
    return *this;
  }

  Structure& Structure::copyStructure(const Structure &other)
  {
    Molecule::operator=(other);

    if (other.OBUnitCell()) {
      OpenBabel::OBUnitCell *cell = new OpenBabel::OBUnitCell(*other.OBUnitCell());
      setOBUnitCell(cell);
    }

    return *this;
  }

  void Structure::writeStructureSettings(const QString &filename)
  {
    SETTINGS(filename);
    const int VERSION = 2;
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
    settings->setValue("needsPreoptimization", needsPreoptimization());
    settings->setValue("hasBestOffspring", hasBestOffspring());

    // History
    settings->beginGroup("history");
    //  Atomic nums
    settings->beginWriteArray("atomicNums");
    for (int i = 0; i < m_histAtomicNums.size(); i++) {
      settings->setArrayIndex(i);
      const QList<unsigned int> *ptr = &(m_histAtomicNums.at(i));
      settings->beginWriteArray(QString("atomicNums-%1").arg(i));
      for (int j = 0; j < ptr->size(); j++) {
        settings->setArrayIndex(j);
        settings->setValue("value", ptr->at(j));
      }
      settings->endArray();
    }
    settings->endArray();

    //  Coords
    settings->beginWriteArray("coords");
    for (int i = 0; i < m_histCoords.size(); i++) {
      settings->setArrayIndex(i);
      const QList<Eigen::Vector3d> *ptr = &(m_histCoords.at(i));
      settings->beginWriteArray(QString("coords-%1").arg(i));
      for (int j = 0; j < ptr->size(); j++) {
        settings->setArrayIndex(j);
        settings->setValue("x", ptr->at(j).x());
        settings->setValue("y", ptr->at(j).y());
        settings->setValue("z", ptr->at(j).z());
      }
      settings->endArray();
    }
    settings->endArray();

    //  Energies
    settings->beginWriteArray("energies");
    for (int i = 0; i < m_histEnergies.size(); i++) {
      settings->setArrayIndex(i);
      settings->setValue("value", m_histEnergies.at(i));
    }
    settings->endArray();

    //  Enthalpies
    settings->beginWriteArray("enthalpies");
    for (int i = 0; i < m_histEnthalpies.size(); i++) {
      settings->setArrayIndex(i);
      settings->setValue("value", m_histEnthalpies.at(i));
    }
    settings->endArray();

    //  Cells
    settings->beginWriteArray("cells");
    for (int i = 0; i < m_histCells.size(); i++) {
      settings->setArrayIndex(i);
      const Eigen::Matrix3d *ptr = &m_histCells.at(i);
      settings->setValue("00", (*ptr)(0, 0));
      settings->setValue("01", (*ptr)(0, 1));
      settings->setValue("02", (*ptr)(0, 2));
      settings->setValue("10", (*ptr)(1, 0));
      settings->setValue("11", (*ptr)(1, 1));
      settings->setValue("12", (*ptr)(1, 2));
      settings->setValue("20", (*ptr)(2, 0));
      settings->setValue("21", (*ptr)(2, 1));
      settings->setValue("22", (*ptr)(2, 2));
    }
    settings->endArray();

    settings->endGroup(); // history

    // Optimizer index lookup table
    settings->beginWriteArray("optimizerLookup");
    QList<int> optKeys = m_optimizerLookup.keys();
    for (int i = 0; i < optKeys.size(); ++i) {
      settings->setArrayIndex(i);
      settings->setValue("key", optKeys.at(i));
      settings->setValue("value", m_optimizerLookup.value(optKeys.at(i)));
    }
    settings->endArray();

    settings->endGroup(); // structure
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

      setNeedsPreoptimization(settings->value("needsPreoptimization", true).toBool());
      setHasBestOffspring(settings->value("hasBestOffspring", false).toBool());

      // History
      settings->beginGroup("history");
      //  Atomic nums
      int size, size2;
      size = settings->beginReadArray("atomicNums");
      m_histAtomicNums.clear();
      for (int i = 0; i < size; i++) {
        settings->setArrayIndex(i);
        size2 = settings->beginReadArray(QString("atomicNums-%1").arg(i));
        QList<unsigned int> cur;
        for (int j = 0; j < size2; j++) {
          settings->setArrayIndex(j);
          cur.append(settings->value("value").toUInt());
        }
        settings->endArray();
        m_histAtomicNums.append(cur);
      }
      settings->endArray();

      //  Coords
      size = settings->beginReadArray("coords");
      m_histCoords.clear();
      for (int i = 0; i < size; i++) {
        settings->setArrayIndex(i);
        size2 = settings->beginReadArray(QString("coords-%1").arg(i));
        QList<Eigen::Vector3d> cur;
        for (int j = 0; j < size2; j++) {
          settings->setArrayIndex(j);
          double x = settings->value("x").toDouble();
          double y = settings->value("y").toDouble();
          double z = settings->value("z").toDouble();
          cur.append(Eigen::Vector3d(x, y, z));
        }
        settings->endArray();
        m_histCoords.append(cur);
      }
      settings->endArray();

      //  Energies
      size = settings->beginReadArray("energies");
      m_histEnergies.clear();
      for (int i = 0; i < size; i++) {
        settings->setArrayIndex(i);
        m_histEnergies.append(settings->value("value").toDouble());
      }
      settings->endArray();

      //  Enthalpies
      size = settings->beginReadArray("enthalpies");
      m_histEnthalpies.clear();
      for (int i = 0; i < size; i++) {
        settings->setArrayIndex(i);
        m_histEnthalpies.append(settings->value("value").toDouble());
      }
      settings->endArray();

      //  Cells
      size = settings->beginReadArray("cells");
      m_histCells.clear();
      for (int i = 0; i < size; i++) {
        settings->setArrayIndex(i);
        Eigen::Matrix3d cur;
        cur(0, 0) = settings->value("00").toDouble();
        cur(0, 1) = settings->value("01").toDouble();
        cur(0, 2) = settings->value("02").toDouble();
        cur(1, 0) = settings->value("10").toDouble();
        cur(1, 1) = settings->value("11").toDouble();
        cur(1, 2) = settings->value("12").toDouble();
        cur(2, 0) = settings->value("20").toDouble();
        cur(2, 1) = settings->value("21").toDouble();
        cur(2, 2) = settings->value("22").toDouble();
        m_histCells.append(cur);
      }
      settings->endArray();

      settings->endGroup(); // history

      // Optimizer index lookup table
      size = settings->beginReadArray("optimizerLookup");
      m_optimizerLookup.clear();
      for (int i = 0; i < size; ++i) {
        settings->setArrayIndex(i);
        int key = settings->value("key", -1).toInt();
        int value = settings->value("value", -1).toInt();
        if (key < 0 || value < 0) {
          qDebug() << "Missing key (" << key << ") value (" << value << ")"
                      "pair for structure" << this->getIDString();
          continue;
        }
        m_optimizerLookup.insert(key, value);
      }
      settings->endArray();

    } // end if version >= 1
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
    case 1: // Histories added. Nothing to do, structures loaded will
            // have empty histories
    case 2:
    default:
      break;
    }
  }

  void Structure::structureChanged()
  {
    m_updatedSinceDupChecked = true;
  }

  void Structure::updateAndSkipHistory(const QList<unsigned int> &atomicNums,
                                       const QList<Eigen::Vector3d> &coords,
                                       const double energy,
                                       const double enthalpy,
                                       const Eigen::Matrix3d &cell)
  {
    Q_ASSERT_X(atomicNums.size() == coords.size() && coords.size() == numAtoms(),
               Q_FUNC_INFO,
               "Lengths of atomicNums and coords must match numAtoms().");

    // Update atoms
    Atom *atm;
    bool useLUT = !m_optimizerLookup.isEmpty();
    for (int optIndex = 0; optIndex < numAtoms(); ++optIndex) {
      int atomIndex = optIndex;
      if (useLUT) {
        atomIndex = m_optimizerLookup.value(optIndex);
      }
      atm = atom(atomIndex);
      atm->setAtomicNumber(atomicNums.at(optIndex));
      atm->setPos(coords.at(optIndex));
    }

    // Update energy/enthalpy
    if (fabs(enthalpy) < 1e-6) {
      m_hasEnthalpy = false;
      m_PV = 0.0;
    }
    else {
      m_hasEnthalpy = true;
      m_PV = enthalpy - energy;
    }
    m_enthalpy = enthalpy;
    setEnergy(energy * EV_TO_KJ_PER_MOL);

    // Update cell if necessary
    if (!cell.isZero()) {
      OpenBabel::matrix3x3 obmat;
      for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
          obmat.Set(i,j,cell(i,j));
        }
      }
      OpenBabel::OBUnitCell *obcell = OBUnitCell();
      obcell->SetData(obmat);
      setOBUnitCell(obcell);
    }

    emit moleculeChanged();
    update();
  }

  void Structure::updateAndAddToHistory(const QList<unsigned int> &atomicNums,
                                        const QList<Eigen::Vector3d> &coords,
                                        const double energy,
                                        const double enthalpy,
                                        const Eigen::Matrix3d &cell)
  {
    Q_ASSERT_X(atomicNums.size() == coords.size() && coords.size() == numAtoms(),
               Q_FUNC_INFO,
               "Lengths of atomicNums and coords must match numAtoms().");

    // Update history
    m_histAtomicNums.append(atomicNums);
    m_histCoords.append(coords);
    m_histEnergies.append(energy);
    m_histEnthalpies.append(enthalpy);
    m_histCells.append(cell);

    // Update atoms
    Atom *atm;
    bool useLUT = !m_optimizerLookup.isEmpty();
    for (int optIndex = 0; optIndex < numAtoms(); ++optIndex) {
      int atomIndex = optIndex;
      if (useLUT) {
        atomIndex = m_optimizerLookup.value(optIndex);
      }
      atm = atom(atomIndex);
      atm->setAtomicNumber(atomicNums.at(optIndex));
      atm->setPos(coords.at(optIndex));
    }

    // Update energy/enthalpy
    if (fabs(enthalpy) < 1e-6) {
      m_hasEnthalpy = false;
      m_PV = 0.0;
    }
    else {
      m_hasEnthalpy = true;
      m_PV = enthalpy - energy;
    }
    m_enthalpy = enthalpy;
    setEnergy(energy * EV_TO_KJ_PER_MOL);

    // Update cell if necessary
    if (!cell.isZero()) {
      OpenBabel::matrix3x3 obmat;
      for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
          obmat.Set(i,j,cell(i,j));
        }
      }
      OpenBabel::OBUnitCell *obcell = OBUnitCell();
      obcell->SetData(obmat);
      setOBUnitCell(obcell);
    }

    emit moleculeChanged();
    update();
  }

  void Structure::deleteFromHistory(unsigned int index) {
    Q_ASSERT_X(index <= sizeOfHistory() - 1, Q_FUNC_INFO,
               "Requested history index greater than the number of available entries.");

    m_histAtomicNums.removeAt(index);
    m_histEnthalpies.removeAt(index);
    m_histEnergies.removeAt(index);
    m_histCoords.removeAt(index);
    m_histCells.removeAt(index);
  }

  void Structure::retrieveHistoryEntry(unsigned int index,
                                       QList<unsigned int> *atomicNums,
                                       QList<Eigen::Vector3d> *coords,
                                       double *energy,
                                       double *enthalpy,
                                       Eigen::Matrix3d *cell)
  {
    Q_ASSERT_X(index <= sizeOfHistory() - 1, Q_FUNC_INFO,
               "Requested history index greater than the number of available entries.");

    if (atomicNums != NULL) {
      *atomicNums = m_histAtomicNums.at(index);
    }
    if (coords != NULL) {
      *coords = m_histCoords.at(index);
    }
    if (energy != NULL) {
      *energy = m_histEnergies.at(index);
    }
    if (enthalpy != NULL) {
      *enthalpy = m_histEnthalpies.at(index);
    }
    if (cell != NULL) {
      *cell = m_histCells.at(index);
    }

  }

  bool Structure::addAtomRandomly(uint atomicNumber, double minIAD, double maxIAD, int maxAttempts, Atom **atom)
  {
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
    case Restart:
      status = "In progress";
      break;
    case Preoptimizing:
      status = "Preoptimizing";
      break;
    default:
      status = "Unknown";
      break;
    }
    return QString("%1 %2 %3 %4 %5")
      .arg(getRank(), 6)
      .arg(getGeneration(), 6)
      .arg(getIDNumber(), 6)
      .arg(getEnthalpy(), 10)
      .arg(status, 11);
  };

  bool Structure::getNearestNeighborDistances(QList<double> * list) const
  {
    list->clear();
    QList<Atom *> atomList = atoms();
    if (atomList.size() < 2) return false;

#if QT_VERSION >= 0x040700
    list->reserve(atomList.size());
#endif // QT_VERSION

    double shortest;
    for (QList<Atom*>::const_iterator it = atomList.constBegin(),
         it_end = atomList.constEnd(); it != it_end; ++it) {
      getNearestNeighborDistance((*it), shortest);
      list->append(shortest);
    }
    return true;
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

  bool Structure::getNearestNeighborDistance(const double x,
                                             const double y,
                                             const double z,
                                             double & shortest) const
  {
    QList<Atom*> atomList = atoms();
    if (atomList.size() < 2) return false; // Need at least two atoms!
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

  bool Structure::getNearestNeighborDistance(const Avogadro::Atom *atom,
                                             double & shortest) const
  {
    const Eigen::Vector3d *position = atom->pos();
    return getNearestNeighborDistance(position->x(),
                                      position->y(),
                                      position->z(), shortest);
  }

  QList<Atom*> Structure::getNeighbors(const double x,
                                       const double y,
                                       const double z,
                                       const double cutoff,
                                       QList<double> *distances) const
  {
    QList<Atom*> neighbors;
    if (distances) {
      distances->clear();
    }
    Eigen::Vector3d vec (x,y,z);
    double cutoffSquared = cutoff*cutoff;
    for (QList<Atom*>::const_iterator it = m_atomList.constBegin(),
           it_end = m_atomList.constEnd(); it != it_end; ++it) {
      double distSq = ((*(*it)->pos()) - vec).squaredNorm();
      if (distSq <= cutoffSquared) {
        neighbors << *it;
        if (distances) {
          *distances << sqrt(distSq);
        }
      }
    }
    return neighbors;
  }

  QList<Atom*> Structure::getNeighbors(const Avogadro::Atom *atom,
                                       const double cutoff,
                                       QList<double> *distances) const
  {
    const Eigen::Vector3d *position = atom->pos();
    return getNeighbors(position->x(),
                        position->y(),
                        position->z(), cutoff, distances);
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
                                       Avogadro::Atom *atom) const
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
  // Skip doxygenation:
  /// \cond
  struct NNHistMap {
    int i;
    double halfstep;
    QList<Vector3d> *atomPositions;
    QList<QVariant> *dist;
  };

  // End doxygenation skip:
  /// \endcond

  // Returns the frequencies for this chunk
  QList<int> calcNNHistChunk(const NNHistMap &m)
  {
    const Vector3d *v1 = &(m.atomPositions->at(m.i));
    const Vector3d *v2;
    QList<int> freq;
    double diff;
    for (int ind = 0; ind < m.dist->size(); ind++) {
      freq.append(0);
    }
    for (int j = m.i+1; j < m.atomPositions->size(); j++) {
      v2 = &(m.atomPositions->at(j));
      diff = fabs(((*v1)-(*v2)).norm());
      for (int k = 0; k < m.dist->size(); k++) {
        if (fabs(diff-(m.dist->at(k).toDouble())) < m.halfstep) {
          freq[k]++;
        }
      }
    }
    return freq;
  }

  void reduceNNHistChunks(QList<QVariant> &final, const QList<int> &tmp)
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
    return;
  }

  bool Structure::generateIADHistogram(QList<QVariant> * distance,
                                       QList<QVariant> * frequency,
                                       double min, double max, double step,
                                       Avogadro::Atom *atom) const
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

    double halfstep = step / 2.0;

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
        m.i = i; m.halfstep = halfstep; m.atomPositions = &atomPositions; m.dist = distance;
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
          if (diff != 0 && fabs(diff-radius) < halfstep) {
            d = frequency->at(k).toDouble();
            d++;
            frequency->replace(k, d);
          }
        }
      }
    }

    return true;
  }

  bool Structure::compareIADDistributions(const std::vector<double> &d,
                                          const std::vector<double> &f1,
                                          const std::vector<double> &f2,
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
    for (QList<Atom*>::const_iterator
           it = m_atomList.begin(),
           it_end = m_atomList.end();
         it != it_end;
         ++it) {
      QString symbol = QString(OpenBabel::etab.GetSymbol((*it)->atomicNumber()));
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
    int ind;
    for (QList<Atom*>::const_iterator
           it = m_atomList.begin(),
           it_end = m_atomList.end();
         it != it_end;
         ++it) {
      QString symbol = QString(OpenBabel::etab.GetSymbol((*it)->atomicNumber()));
      Q_ASSERT_X(symbols.contains(symbol), Q_FUNC_INFO,
                 "getNumberOfAtomsAlpha found a symbol not in getSymbols.");
      ind = symbols.indexOf(symbol);
      ++list[ind];
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

    if (numStructs <= 1) return;

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
    if (structures.size() == 0) return;
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

    if (numStructs == 0) return;

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

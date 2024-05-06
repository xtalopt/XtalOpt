/**********************************************************************
  Structure - Wrapper for the molecule class

  Copyright (C) 2009-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <globalsearch/structure.h>

#include <globalsearch/constants.h>
#include <globalsearch/bt.h>
#include <globalsearch/eleminfo.h>
#include <globalsearch/macros.h>
#include <globalsearch/random.h>
#include <globalsearch/structures/molecule.h>

#include <QDebug>
#include <QFile>
#include <QRegExp>
#include <QStringList>
#include <QTimer>
#include <QtConcurrent>

using namespace Eigen;
using namespace std;

namespace GlobalSearch {

Structure::Structure(QObject* parent)
  : Molecule(), m_hasEnthalpy(false), m_updatedSinceDupChecked(true),
    m_primitiveChecked(false), m_skippedOptimization(false),
    m_supercellGenerationChecked(false), m_histogramGenerationPending(false),
    m_generation(0), m_id(0), m_rank(0), m_jobID(0), m_energy(0), m_enthalpy(0),
    m_PV(0), m_optStart(QDateTime()), m_optEnd(QDateTime()), m_index(-1),
    m_lock(QReadWriteLock::Recursive),
    m_parentStructure(nullptr), m_copyFiles(), m_reusePreoptBonding(true),
    m_bulkModulus(-1.0), m_shearModulus(-1.0), m_vickersHardness(-1.0),
    m_strucObjState(Structure::Os_NotCalculated), m_strucObjFailCt(0)
{
  m_currentOptStep = 0;
  setStatus(Empty);
  resetFailCount();
}

Structure::Structure(const Structure& other)
  : Molecule(other), m_updatedSinceDupChecked(true), m_primitiveChecked(false),
    m_skippedOptimization(false), m_supercellGenerationChecked(false),
    m_histogramGenerationPending(false), m_generation(0), m_id(0), m_rank(0),
    m_jobID(0), m_energy(0), m_enthalpy(0), m_PV(0), m_optStart(QDateTime()),
    m_optEnd(QDateTime()), m_index(-1), m_lock(QReadWriteLock::Recursive),
    m_parentStructure(nullptr), m_copyFiles(), m_reusePreoptBonding(true),
    m_bulkModulus(-1.0), m_shearModulus(-1.0), m_vickersHardness(-1.0),
    m_strucObjState(Structure::Os_NotCalculated), m_strucObjFailCt(0)
{
  *this = other;
}

Structure::Structure(Structure&& other) noexcept
{
  *this = std::move(other);
}

Structure::Structure(const GlobalSearch::Molecule& other)
  : Molecule(other), m_updatedSinceDupChecked(true), m_primitiveChecked(false),
    m_skippedOptimization(false), m_supercellGenerationChecked(false),
    m_histogramGenerationPending(false), m_generation(0), m_id(0), m_rank(0),
    m_jobID(0), m_energy(0), m_enthalpy(0), m_PV(0), m_optStart(QDateTime()),
    m_optEnd(QDateTime()), m_index(-1), m_lock(QReadWriteLock::Recursive),
    m_parentStructure(nullptr), m_copyFiles(), m_reusePreoptBonding(true),
    m_bulkModulus(-1.0), m_shearModulus(-1.0), m_vickersHardness(-1.0),
    m_strucObjState(Structure::Os_NotCalculated), m_strucObjFailCt(0)
{
  *this = other;
}

void Structure::setupConnections()
{
  // List of slots to be called each time the structure changes
  QList<const char*> slotlist;
  slotlist.append(SLOT(structureChanged()));
}

void Structure::enableAutoHistogramGeneration(bool b)
{
  // FIXME: This does nothing after the removal of dependence on
  // Avogadro and OpenBabel...
}

Structure::~Structure()
{
}

Structure& Structure::operator=(const Structure& other)
{
  if (this != &other) {
    Molecule::operator=(other);

    // Set properties
    m_hasEnthalpy = other.m_hasEnthalpy;
    m_updatedSinceDupChecked = other.m_updatedSinceDupChecked.load();
    m_primitiveChecked = other.m_primitiveChecked.load();
    m_skippedOptimization = other.m_skippedOptimization.load();
    m_supercellGenerationChecked = other.m_supercellGenerationChecked.load();
    m_histogramGenerationPending = other.m_histogramGenerationPending;
    m_generation = other.m_generation;
    m_id = other.m_id;
    m_rank = other.m_rank;
    m_jobID = other.m_jobID;
    m_currentOptStep = other.m_currentOptStep;
    m_failCount = other.m_failCount;
    m_parents = other.m_parents;
    m_dupString = other.m_dupString;
    m_rempath = other.m_rempath;
    m_locpath = other.m_locpath;
    m_energy = other.m_energy;
    m_enthalpy = other.m_enthalpy;
    m_PV = other.m_PV;
    m_status = other.m_status.load();
    m_optStart = other.m_optStart;
    m_optEnd = other.m_optEnd;
    m_index = other.m_index;
    m_parentStructure = other.m_parentStructure;
    m_copyFiles = other.m_copyFiles;
    m_reusePreoptBonding = other.m_reusePreoptBonding;
    m_bulkModulus = other.m_bulkModulus;
    m_shearModulus = other.m_shearModulus;
    m_vickersHardness = other.m_vickersHardness;
    m_strucObjValues = other.m_strucObjValues;
    m_strucObjState = ObjectivesState(other.m_strucObjState);
    m_strucObjFailCt = other.m_strucObjFailCt;
  }

  return *this;
}

Structure& Structure::operator=(Structure&& other) noexcept
{
  if (this != &other) {
    Molecule::operator=(std::move(other));

    // Set properties
    m_hasEnthalpy = std::move(other.m_hasEnthalpy);
    m_updatedSinceDupChecked = other.m_updatedSinceDupChecked.load();
    m_primitiveChecked = other.m_primitiveChecked.load();
    m_skippedOptimization = other.m_skippedOptimization.load();
    m_supercellGenerationChecked = other.m_supercellGenerationChecked.load();
    m_histogramGenerationPending =
      std::move(other.m_histogramGenerationPending);
    m_generation = std::move(other.m_generation);
    m_id = std::move(other.m_id);
    m_rank = std::move(other.m_rank);
    m_jobID = std::move(other.m_jobID);
    m_currentOptStep = std::move(other.m_currentOptStep);
    m_failCount = std::move(other.m_failCount);
    m_parents = std::move(other.m_parents);
    m_dupString = std::move(other.m_dupString);
    m_rempath = std::move(other.m_rempath);
    m_locpath = std::move(other.m_locpath);
    m_energy = std::move(other.m_energy);
    m_enthalpy = std::move(other.m_enthalpy);
    m_PV = std::move(other.m_PV);
    m_status = std::move(other.m_status.load());
    m_optStart = std::move(other.m_optStart);
    m_optEnd = std::move(other.m_optEnd);
    m_index = std::move(other.m_index);
    m_parentStructure = std::move(other.m_parentStructure);

    other.m_parentStructure = nullptr;
    m_copyFiles = std::move(other.m_copyFiles);
    m_reusePreoptBonding = std::move(other.m_reusePreoptBonding);
    m_bulkModulus = std::move(other.m_bulkModulus);
    m_shearModulus = std::move(other.m_shearModulus);
    m_vickersHardness = std::move(other.m_vickersHardness);
    m_strucObjValues = std::move(other.m_strucObjValues);
    m_strucObjState = std::move(ObjectivesState(other.m_strucObjState));
    m_strucObjFailCt = std::move(other.m_strucObjFailCt);
  }

  return *this;
}

Structure& Structure::operator=(const GlobalSearch::Molecule& mol)
{
  Molecule::operator=(mol);
  return *this;
}

void Structure::writeStructureSettings(const QString& filename)
{
  SETTINGS(filename);
  const int version = 4;
  settings->beginGroup("structure");
  settings->setValue("saveSuccessful", false);
  settings->setValue("version", version);
  settings->setValue("generation", getGeneration());
  settings->setValue("id", getIDNumber());
  settings->setValue("index", getIndex());
  settings->setValue("rank", getRank());
  settings->setValue("primitiveChecked", wasPrimitiveChecked());
  settings->setValue("skippedOptimization", skippedOptimization());
  settings->setValue("supercellGenerationChecked",
                     wasSupercellGenerationChecked());
  settings->setValue("jobID", getJobID());
  settings->setValue("currentOptStep", getCurrentOptStep());
  settings->setValue("parents", getParents());
  settings->setValue("rempath", getRempath());
  settings->setValue("locpath", getLocpath());
  settings->setValue("status", int(getStatus()));
  settings->setValue("failCount", getFailCount());
  settings->setValue("startTime", getOptTimerStart().toString());
  settings->setValue("endTime", getOptTimerEnd().toString());
  settings->beginWriteArray("copyFiles");
  for (size_t i = 0; i < m_copyFiles.size(); ++i) {
    settings->setArrayIndex(i);
    settings->setValue("value", m_copyFiles[i].c_str());
  }
  settings->endArray();

  // Objectives (multi-objective run): write current info to structure.state file
  settings->beginWriteArray("objectives");
  for (size_t i = 0; i < getStrucObjNumber(); i++) {
    settings->setArrayIndex(i);
    settings->setValue("value", QString::number(getStrucObjValues(i)));
  }
  settings->endArray();
  settings->setValue("objectivesState", getStrucObjState());
  settings->setValue("objectivesFailCount", getStrucObjFailCt());

  settings->setValue("reusePreoptBonding", reusePreoptBonding());
  settings->beginWriteArray("preoptBonds");
  for (size_t i = 0; i < m_preoptBonds.size(); ++i) {
    settings->setArrayIndex(i);
    QString entry = QString::number(m_preoptBonds[i].first()) + "," +
                    QString::number(m_preoptBonds[i].second()) + ":" +
                    QString::number(m_preoptBonds[i].bondOrder());
    settings->setValue("value", entry);
  }
  settings->endArray();

  // Check if a parent structure is saved
  // This is NOT a variable that can be read in readSettings().
  if (this->hasParentStructure()) {
    QString parentStructure = m_parentStructure->getTag();
    settings->setValue("parentStructure", parentStructure);
  }

  // Aflow ML stuff
  settings->setValue("bulkModulus", bulkModulus());
  settings->setValue("shearModulus", shearModulus());
  settings->setValue("vickersHardness", vickersHardness());

  // History
  settings->beginGroup("history");
  //  Atomic nums
  settings->beginWriteArray("atomicNums");
  for (int i = 0; i < m_histAtomicNums.size(); i++) {
    settings->setArrayIndex(i);
    const QList<unsigned int>* ptr = &(m_histAtomicNums.at(i));
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
    const QList<Vector3>* ptr = &(m_histCoords.at(i));
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

  // Objectives (multi-objective run): write history to structure.state file
  settings->beginWriteArray("objectives");
  for (int i = 0; i < getStrucHistObjNumber(); i++) {
    settings->setArrayIndex(i);
    settings->beginWriteArray("value");
    for (int j = 0; j < getStrucHistObjValues(i).size(); j++) {
      settings->setArrayIndex(j);
      settings->setValue("value", getStrucHistObjValues(i).at(j));
    }
    settings->endArray();
    settings->setValue("state", getStrucHistObjState(i));
    settings->setValue("failcount", getStrucHistObjFailCt(i));
  }
  settings->endArray();

  //  Cells
  settings->beginWriteArray("cells");
  for (int i = 0; i < m_histCells.size(); i++) {
    settings->setArrayIndex(i);
    const Matrix3* ptr = &m_histCells.at(i);
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
  settings->setValue("saveSuccessful", true);
  settings->endGroup(); // structure

  // This will write the current enthalpy, energy, cell information, atom
  // types, and atom positions for a cell that skipped optimization
  writeCurrentStructureInfo(filename);
}

void Structure::readStructureSettings(const QString& filename,
                                      const bool readCurrentInfo)
{
  SETTINGS(filename);
  settings->beginGroup("structure");
  int loadedVersion = settings->value("version", 0).toInt();
  if (loadedVersion >= 1) { // Version 0 uses save(QTextStream)
    setGeneration(settings->value("generation", 0).toInt());
    setIDNumber(settings->value("id", 0).toInt());
    setIndex(settings->value("index", 0).toInt());
    setRank(settings->value("rank", 0).toInt());
    setPrimitiveChecked(settings->value("primitiveChecked", 0).toBool());
    setSkippedOptimization(settings->value("skippedOptimization", 0).toBool());
    setSupercellGenerationChecked(
      settings->value("supercellGenerationChecked", 0).toBool());
    setJobID(settings->value("jobID", 0).toInt());

    setCurrentOptStep(settings->value("currentOptStep", 0).toInt());

    // We changed the indexing to be zero-based instead of one-based in the
    // latest scheme update.
    if (loadedVersion < 4 && getCurrentOptStep() != 0)
      setCurrentOptStep(getCurrentOptStep() - 1);

    setFailCount(settings->value("failCount", 0).toInt());
    setParents(settings->value("parents", "").toString());
    setRempath(settings->value("rempath", "").toString());
    setLocpath(settings->value("locpath", m_locpath).toString());
    setStatus(State(settings->value("status", -1).toInt()));

    setOptTimerStart(
      QDateTime::fromString(settings->value("startTime", "").toString()));
    setOptTimerEnd(
      QDateTime::fromString(settings->value("endTime", "").toString()));

    int size = settings->beginReadArray("copyFiles");
    m_copyFiles.clear();
    for (int i = 0; i < size; ++i) {
      settings->setArrayIndex(i);
      m_copyFiles.push_back(settings->value("value").toString().toStdString());
    }
    settings->endArray();

    // Objectives (multi-objective run): read current info from structure.state file
    resetStrucObj();
    size = settings->beginReadArray("objectives");
    for (int i = 0; i < size; i++) {
      settings->setArrayIndex(i);
      setStrucObjValues(settings->value("value").toDouble());
    }
    settings->endArray();
    setStrucObjState(ObjectivesState(settings->value("objectivesState", 0).toInt()));
    setStrucObjFailCt(settings->value("objectivesFailCount", 0).toInt());

    setReusePreoptBonding(
      settings->value("reusePreoptBonding", false).toBool());
    size = settings->beginReadArray("preoptBonds");
    std::vector<Bond> preoptBonds;
    for (int i = 0; i < size; ++i) {
      settings->setArrayIndex(i);
      QString entry = settings->value("value").toString();
      if (!entry.contains(',') || !entry.contains(':'))
        continue;
      size_t ind1 = entry.split(',')[0].toUInt();
      size_t ind2 = entry.split(',')[1].split(':')[0].toUInt();
      size_t bondOrder = entry.split(',')[1].split(':')[1].toUInt();
      preoptBonds.push_back(Bond(ind1, ind2, bondOrder));
    }
    setPreoptBonding(preoptBonds);
    settings->endArray();

    setBulkModulus(settings->value("bulkModulus", "-1.0").toDouble());
    setShearModulus(settings->value("shearModulus", "-1.0").toDouble());
    setVickersHardness(settings->value("vickersHardness", "-1.0").toDouble());

    // History
    settings->beginGroup("history");
    //  Atomic nums
    int size2;
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
      QList<Vector3> cur;
      for (int j = 0; j < size2; j++) {
        settings->setArrayIndex(j);
        double x = settings->value("x").toDouble();
        double y = settings->value("y").toDouble();
        double z = settings->value("z").toDouble();
        cur.append(Vector3(x, y, z));
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

    // Objectives (multi-objective run): read history from structure.state file
    size = settings->beginReadArray("objectives");
    resetStrucHistObj();
    for (int i = 0; i < size; i++) {
      settings->setArrayIndex(i);
      int size2 = settings->beginReadArray("value");
      QList<double> tmpvalue;
      for (int j = 0; j < size2; j++) {
        settings->setArrayIndex(j);
        tmpvalue.push_back(settings->value("value").toDouble());
      }
      setStrucHistObjValues(tmpvalue);
      settings->endArray();
      setStrucHistObjState(ObjectivesState(settings->value("state").toInt()));
      setStrucHistObjFailCt(settings->value("failcount").toInt());
    }
    settings->endArray();

    //  Cells
    size = settings->beginReadArray("cells");
    m_histCells.clear();
    for (int i = 0; i < size; i++) {
      settings->setArrayIndex(i);
      Matrix3 cur;
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
  }
  settings->endGroup();

  // Update config data
  switch (loadedVersion) {
    case 0: {
      // Call load(QTextStream) to update
      qDebug() << "Updating " << filename << " from Version 0 -> 1";
      QFile file(filename);
      file.open(QIODevice::ReadOnly);
      QTextStream stream(&file);
      load(stream);
    }
    case 1: // Histories added. Nothing to do, structures loaded will
            // have empty histories
    case 2:
    case 3:
    case 4:
    default:
      break;
  }
  // This will read the current enthalpy, energy, cell information, atom
  // types, and atom positions for the structure.
  if (readCurrentInfo)
    readCurrentStructureInfo(filename);
}

void Structure::writeCurrentStructureInfo(const QString& filename)
{
  SETTINGS(filename);
  // May set versions in the future
  // const int version = 1;
  settings->beginGroup("structure/current");
  settings->setValue("enthalpy", this->getEnthalpy());
  settings->setValue("energy", this->getEnergy());
  settings->setValue("PV", this->getPV());

  // Atomic numbers
  settings->beginWriteArray("atomicNums");
  for (size_t i = 0; i < numAtoms(); i++) {
    settings->setArrayIndex(i);
    settings->setValue("value", QString::number(atom(i).atomicNumber()));
  }
  settings->endArray();

  // Cartesian coords
  Vector3 cartCoords;
  settings->beginWriteArray("coords");
  for (size_t i = 0; i < numAtoms(); i++) {
    cartCoords = atom(i).pos();
    settings->setArrayIndex(i);
    settings->setValue("x", QString::number(cartCoords[0]));
    settings->setValue("y", QString::number(cartCoords[1]));
    settings->setValue("z", QString::number(cartCoords[2]));
  }
  settings->endArray();

  // Check to see if cell info exists before saving it...
  // This is important for non-periodic structures
  if (hasUnitCell()) {
    settings->setValue("hasCellInfo", true);
    // Cell info
    Matrix3 uc = unitCell().cellMatrix();
    settings->beginGroup("cell");
    settings->setValue("00", (uc(0, 0)));
    settings->setValue("01", (uc(0, 1)));
    settings->setValue("02", (uc(0, 2)));
    settings->setValue("10", (uc(1, 0)));
    settings->setValue("11", (uc(1, 1)));
    settings->setValue("12", (uc(1, 2)));
    settings->setValue("20", (uc(2, 0)));
    settings->setValue("21", (uc(2, 1)));
    settings->setValue("22", (uc(2, 2)));
    settings->endGroup(); // cell
  }
  settings->endGroup(); // structure/current
}

void Structure::readCurrentStructureInfo(const QString& filename)
{
  SETTINGS(filename);
  settings->beginGroup("structure/current");
  setEnthalpy(settings->value("enthalpy", 0).toDouble());
  setEnergy(settings->value("energy", 0).toDouble());
  setPV(settings->value("PV", 0).toDouble());

  //  Atomic nums
  size_t size;
  size = settings->beginReadArray("atomicNums");
  QList<unsigned int> atomicNums;
  for (int i = 0; i < size; i++) {
    settings->setArrayIndex(i);
    atomicNums.append(settings->value("value").toUInt());
  }
  settings->endArray();

  size = settings->beginReadArray("coords");
  QList<Vector3> cartCoords;
  double x, y, z;
  for (int i = 0; i < size; i++) {
    settings->setArrayIndex(i);
    x = settings->value("x").toDouble();
    y = settings->value("y").toDouble();
    z = settings->value("z").toDouble();
    cartCoords.append(Vector3(x, y, z));
  }
  settings->endArray();

  if (settings->value("hasCellInfo", false).toBool()) {
    Matrix3 cellMatrix;
    settings->beginGroup("cell");
    cellMatrix(0, 0) = settings->value("00").toDouble();
    cellMatrix(0, 1) = settings->value("01").toDouble();
    cellMatrix(0, 2) = settings->value("02").toDouble();
    cellMatrix(1, 0) = settings->value("10").toDouble();
    cellMatrix(1, 1) = settings->value("11").toDouble();
    cellMatrix(1, 2) = settings->value("12").toDouble();
    cellMatrix(2, 0) = settings->value("20").toDouble();
    cellMatrix(2, 1) = settings->value("21").toDouble();
    cellMatrix(2, 2) = settings->value("22").toDouble();
    settings->endGroup();

    // Set the cell info
    unitCell().setCellMatrix(cellMatrix);
  }

  // Just in case there were atoms set elsewhere for some reason...
  clearAtoms();

  // Now let's add in the atoms...
  for (size_t i = 0; i < atomicNums.size(); i++) {
    Atom& newAtom = addAtom();
    newAtom.setAtomicNumber(atomicNums.at(i));
    newAtom.setPos(cartCoords.at(i));
  }
  settings->endGroup();
}

void Structure::perceiveBonds()
{
  clearBonds();

  // The cutoff tolerance to be used
  double tol = 0.1;

  const auto& atoms = this->atoms();
  for (size_t i = 0; i < atoms.size(); ++i) {
    for (size_t j = i + 1; j < atoms.size(); ++j) {
      const auto& rad1 = ElemInfo::getCovalentRadius(atoms[i].atomicNumber());
      const auto& rad2 = ElemInfo::getCovalentRadius(atoms[j].atomicNumber());
      if (distance(atoms[i].pos(), atoms[j].pos()) < rad1 + rad2 + tol)
        addBond(i, j);
    }
  }
}

void Structure::structureChanged()
{
  m_updatedSinceDupChecked = true;
}

void Structure::updateAndSkipHistory(const QList<unsigned int>& atomicNums,
                                     const QList<Vector3>& coords,
                                     const double energy, const double enthalpy,
                                     const Matrix3& cell)
{
  Q_ASSERT_X(atomicNums.size() == coords.size() && coords.size() == numAtoms(),
             Q_FUNC_INFO,
             "Lengths of atomicNums and coords must match numAtoms().");

  // Update atoms
  Atom atom;
  for (int i = 0; i < numAtoms(); i++) {
    atom = atoms()[i];
    atom.setAtomicNumber(atomicNums.at(i));
    atom.setPos(coords.at(i));
  }

  // Update energy/enthalpy
  if (fabs(enthalpy) < ZERO6) {
    m_hasEnthalpy = false;
    m_PV = 0.0;
  } else {
    m_hasEnthalpy = true;
    m_PV = enthalpy - energy;
  }
  m_enthalpy = enthalpy;
  m_energy = energy;

  // Update cell if necessary
  if (!cell.isZero())
    unitCell().setCellMatrix(cell);
}

void Structure::updateAndAddObjectivesToHistory(Structure* s)
{
  s->setStrucHistObjValues(s->getStrucObjValuesVec());
  s->setStrucHistObjState(s->getStrucObjState());
  s->setStrucHistObjFailCt(s->getStrucObjFailCt());
}

void Structure::updateAndAddToHistory(const QList<unsigned int>& atomicNums,
                                      const QList<Vector3>& coords,
                                      const double energy,
                                      const double enthalpy,
                                      const Matrix3& cell)
{
  Q_ASSERT_X(atomicNums.size() == coords.size(), Q_FUNC_INFO,
             "Lengths of atomicNums and coords must match numAtoms().");

  // Update history
  m_histAtomicNums.append(atomicNums);
  m_histCoords.append(coords);
  m_histEnergies.append(energy);
  m_histEnthalpies.append(enthalpy);
  m_histCells.append(cell);

  // Reset atoms
  clearAtoms();
  for (int i = 0; i < atomicNums.size() && i < coords.size(); ++i)
    addAtom(atomicNums[i], coords[i]);

  // Are we to use the same bonds as we used in pre-optimization? If true,
  // use them and clear it.
  if (reusePreoptBonding() && !getPreoptBonding().empty()) {
    // bonds() returns a reference. So we can set it.
    bonds() = getPreoptBonding();
    clearPreoptBonding();
  }

  // Update energy/enthalpy
  if (fabs(enthalpy) < ZERO6) {
    m_hasEnthalpy = false;
    m_PV = 0.0;
  } else {
    m_hasEnthalpy = true;
    m_PV = enthalpy - energy;
  }
  m_enthalpy = enthalpy;
  m_energy = energy;

  // Update cell if necessary
  if (!cell.isZero())
    unitCell().setCellMatrix(cell);
}

void Structure::deleteFromHistory(unsigned int index)
{
  Q_ASSERT_X(
    index <= sizeOfHistory() - 1, Q_FUNC_INFO,
    "Requested history index greater than the number of available entries.");

  m_histAtomicNums.removeAt(index);
  m_histEnthalpies.removeAt(index);
  m_histEnergies.removeAt(index);
  m_histCoords.removeAt(index);
  m_histCells.removeAt(index);
}

void Structure::retrieveHistoryEntry(unsigned int index,
                                     QList<unsigned int>* atomicNums,
                                     QList<Vector3>* coords, double* energy,
                                     double* enthalpy, Matrix3* cell)
{
  Q_ASSERT_X(
    index <= sizeOfHistory() - 1, Q_FUNC_INFO,
    "Requested history index greater than the number of available entries.");

  if (atomicNums != nullptr) {
    *atomicNums = m_histAtomicNums.at(index);
  }
  if (coords != nullptr) {
    *coords = m_histCoords.at(index);
  }
  if (energy != nullptr) {
    *energy = m_histEnergies.at(index);
  }
  if (enthalpy != nullptr) {
    *enthalpy = m_histEnthalpies.at(index);
  }
  if (cell != nullptr) {
    *cell = m_histCells.at(index);
  }
}

bool Structure::addAtomRandomly(uint atomicNumber, double minIAD, double maxIAD,
                                int maxAttempts)
{
  double IAD = -1;
  int i = 0;
  Vector3 coords;

  // For first atom, add to 0, 0, 0
  if (numAtoms() == 0) {
    coords = Vector3(0, 0, 0);
  } else {
    do {
      // Generate random coordinates
      IAD = -1;
      double x = getRandDouble();
      double y = getRandDouble();
      double z = getRandDouble();
      coords = Vector3(x, y, z);

      coords = unitCell().toCartesian(coords);
      coords[0] += maxIAD;
      coords[1] += maxIAD;
      coords[2] += maxIAD;

      if (minIAD != -1) {
        getNearestNeighborDistance(x, y, z, IAD);
      } else {
        break;
      };
      i++;
    } while (i < maxAttempts && IAD <= minIAD);

    if (i >= maxAttempts)
      return false;
  }

  Atom& atom = addAtom();
  atom.setPos(coords);
  atom.setAtomicNumber(atomicNumber);
  return true;
}

QString Structure::getResultsEntry(bool includeHardness, int objectives_num, int optstep) const
{
  QString status;
  switch (getStatus()) {
    case Optimized:
      if (skippedOptimization())
        status = "Skipped Optimization";
      else
        status = "Optimized";
      break;
    case Killed:
    case Removed:
      status = "Killed";
      break;
    case Duplicate:
      status = "Duplicate";
      break;
    case Supercell:
      status = "Supercell";
      break;
    case Error:
      status = "Error";
      break;
    case ObjectiveDismiss:
      status = "ObjectiveDismiss";
      break;
    case ObjectiveFail:
      status = "ObjectiveFail";
      break;
    case ObjectiveRetain:
    case ObjectiveCalculation:
      status = "ObjectiveCalculation";
      break;
    case StepOptimized:
    case WaitingForOptimization:
    case Submitted:
    case InProcess:
    case Empty:
    case Updating:
      status = "Opt Step " + QString::number(optstep);
      break;
    default:
      status = "In progress";
      break;
  }

  QString out = QString("%1 %2 %3 %4 %5")
      .arg(getRank(), 6)
      .arg(getGeneration(), 6)
      .arg(getIDNumber(), 6)
      .arg(getIndex(), 6)
      .arg(getEnthalpy(), 10);
  if (includeHardness)
    out += QString("%1")
      .arg(vickersHardness(), 10);
  for (int i = 0; i < objectives_num; i++) {
    if (i < getStrucObjNumber())
      out += " "+QString("%1").arg(getStrucObjValues(i), 10);
    else
      out += QString("%1").arg("-", 11);
  }
  out += QString("%1")
      .arg(status, 11);

  return out;
}

bool Structure::getNearestNeighborDistances(QList<double>* list) const
{
  list->clear();
  const std::vector<Atom>& atomList = atoms();
  if (atomList.size() < 2)
    return false;

#if QT_VERSION >= 0x040700
  list->reserve(atomList.size());
#endif // QT_VERSION

  double shortest;
  for (std::vector<Atom>::const_iterator it = atomList.begin(),
                                         it_end = atomList.end();
       it != it_end; ++it) {
    getNearestNeighborDistance((*it), shortest);
    list->append(shortest);
  }
  return true;
}

bool Structure::getShortestInteratomicDistance(double& shortest) const
{
  const std::vector<Atom>& atomList = atoms();
  if (atomList.size() <= 1)
    return false; // Need at least two atoms!
  QList<Vector3> atomPositions;
  for (int i = 0; i < atomList.size(); i++)
    atomPositions.push_back(atomList.at(i).pos());

  Vector3 v1 = atomPositions.at(0);
  Vector3 v2 = atomPositions.at(1);
  shortest = std::fabs((v1 - v2).norm());
  double distance;

  // Find shortest distance
  for (int i = 0; i < atomList.size(); i++) {
    v1 = atomPositions.at(i);
    for (int j = i + 1; j < atomList.size(); j++) {
      v2 = atomPositions.at(j);
      // Intercell
      distance = std::fabs((v1 - v2).norm());
      if (distance < shortest)
        shortest = distance;
    }
  }
  return true;
}

bool Structure::getNearestNeighborDistance(const double x, const double y,
                                           const double z,
                                           double& shortest) const
{
  const std::vector<Atom>& atomList = atoms();
  if (atomList.size() < 2)
    return false; // Need at least two atoms!
  QList<Vector3> atomPositions;
  for (int i = 0; i < atomList.size(); i++)
    atomPositions.push_back(atomList.at(i).pos());

  Vector3 v1(x, y, z);
  Vector3 v2 = atomPositions.at(0);
  shortest = fabs((v1 - v2).norm());
  double distance;

  // Find shortest distance
  for (int j = 0; j < atomList.size(); j++) {
    v2 = atomPositions.at(j);
    // Intercell
    distance = fabs((v1 - v2).norm());
    if (distance < shortest)
      shortest = distance;
  }
  return true;
}

bool Structure::getNearestNeighborDistance(const GlobalSearch::Atom& atom,
                                           double& shortest) const
{
  Vector3 position = atom.pos();
  return getNearestNeighborDistance(position.x(), position.y(), position.z(),
                                    shortest);
}

QList<Atom> Structure::getNeighbors(const double x, const double y,
                                    const double z, const double cutoff,
                                    QList<double>* distances) const
{
  QList<Atom> neighbors;
  if (distances) {
    distances->clear();
  }
  Vector3 vec(x, y, z);
  double cutoffSquared = cutoff * cutoff;
  const std::vector<Atom>& atomList = atoms();
  for (std::vector<Atom>::const_iterator it = atomList.begin(),
                                         it_end = atomList.end();
       it != it_end; ++it) {
    double distSq = ((*it).pos() - vec).squaredNorm();
    if (distSq <= cutoffSquared) {
      neighbors << *it;
      if (distances) {
        *distances << sqrt(distSq);
      }
    }
  }
  return neighbors;
}

QList<Atom> Structure::getNeighbors(const GlobalSearch::Atom& atom,
                                    const double cutoff,
                                    QList<double>* distances) const
{
  Vector3 position = atom.pos();
  return getNeighbors(position.x(), position.y(), position.z(), cutoff,
                      distances);
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

void Structure::getDefaultHistogram(QList<double>* dist,
                                    QList<double>* freq) const
{
  dist->clear();
  freq->clear();
  for (int i = 0; i < m_histogramDist.size(); i++) {
    dist->append(m_histogramDist.at(i).toDouble());
    freq->append(m_histogramFreq.at(i).toDouble());
  }
}

void Structure::getDefaultHistogram(QList<QVariant>* dist,
                                    QList<QVariant>* freq) const
{
  (*dist) = m_histogramDist;
  (*freq) = m_histogramFreq;
}

bool Structure::generateIADHistogram(QList<double>* dist, QList<double>* freq,
                                     double min, double max, double step,
                                     const GlobalSearch::Atom& atom) const
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
struct NNHistMap
{
  int i;
  double halfstep;
  QList<Vector3>* atomPositions;
  QList<QVariant>* dist;
};

// End doxygenation skip:
/// \endcond

// Returns the frequencies for this chunk
QList<int> calcNNHistChunk(const NNHistMap& m)
{
  const Vector3* v1 = &(m.atomPositions->at(m.i));
  const Vector3* v2;
  QList<int> freq;
  double diff;
  for (int ind = 0; ind < m.dist->size(); ind++) {
    freq.append(0);
  }
  for (int j = m.i + 1; j < m.atomPositions->size(); j++) {
    v2 = &(m.atomPositions->at(j));
    diff = fabs(((*v1) - (*v2)).norm());
    for (int k = 0; k < m.dist->size(); k++) {
      if (fabs(diff - (m.dist->at(k).toDouble())) < m.halfstep) {
        freq[k]++;
      }
    }
  }
  return freq;
}

void reduceNNHistChunks(QList<QVariant>& final, const QList<int>& tmp)
{
  if (final.size() != tmp.size()) {
    final.clear();
    for (int i = 0; i < tmp.size(); i++) {
      final.append(tmp.at(i));
    }
  } else {
    double d;
    for (int i = 0; i < final.size(); i++) {
      d = final.at(i).toDouble();
      d += tmp.at(i);
      final.replace(i, d);
    }
  }
  return;
}

bool Structure::generateIADHistogram(QList<QVariant>* distance,
                                     QList<QVariant>* frequency, double min,
                                     double max, double step,
                                     const GlobalSearch::Atom& atom) const
{
  distance->clear();
  frequency->clear();

  if (min > max && step > 0) {
    qWarning() << "Structure::getNearestNeighborHistogram: min cannot be "
                  "greater than max!";
    return false;
  }
  if (step <= 0) {
    qWarning() << "Structure::getNearestNeighborHistogram: invalid step size:"
               << step;
    return false;
  }

  double halfstep = step / 2.0;

  double val = min;
  do {
    distance->append(val);
    frequency->append(0);
    val += step;
  } while (val < max);

  const std::vector<Atom>& atomList = atoms();
  QList<Vector3> atomPositions;
  for (int i = 0; i < atomList.size(); i++)
    atomPositions.push_back(atomList.at(i).pos());

  Vector3 v1 = atomPositions.at(0);
  Vector3 v2 = atomPositions.at(1);
  double diff;

  // build histogram
  // Loop over all atoms -- use map-reduce
  if (atom.atomicNumber() == 0) {
    QList<NNHistMap> ml;
    for (int i = 0; i < atomList.size(); i++) {
      NNHistMap m;
      m.i = i;
      m.halfstep = halfstep;
      m.atomPositions = &atomPositions;
      m.dist = distance;
      ml.append(m);
    }
    (*frequency) = QtConcurrent::blockingMappedReduced(ml, calcNNHistChunk,
                                                       reduceNNHistChunks);
  }
  // Or, just the one requested
  else {
    v1 = atom.pos();
    for (int j = 0; j < atomList.size(); j++) {
      if (atomList.at(j) == atom)
        continue;
      v2 = atomPositions.at(j);
      // Intercell
      diff = fabs((v1 - v2).norm());
      for (int k = 0; k < distance->size(); k++) {
        double radius = distance->at(k).toDouble();
        double d;
        if (diff != 0 && fabs(diff - radius) < halfstep) {
          d = frequency->at(k).toDouble();
          d++;
          frequency->replace(k, d);
        }
      }
    }
  }

  return true;
}

bool Structure::compareIADDistributions(const std::vector<double>& d,
                                        const std::vector<double>& f1,
                                        const std::vector<double>& f2,
                                        double decay, double smear,
                                        double* error)
{
  // Check that smearing is possible
  if (smear != 0 && d.size() <= 1) {
    qWarning()
      << "Cluster::compareNNDist: Cannot smear with 1 or fewer points.";
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
  int boxSize = ceil(smear / stepSize);
  if (boxSize > d.size()) {
    qWarning()
      << "Cluster::compareNNDist: Smear length is greater then d vector range.";
    return false;
  }
  // Smear
  vector<double> f1s, f2s, ds; // smeared vectors
  if (smear != 0) {
    double f1t, f2t, dt; // temporary variables
    for (int i = 0; i < d.size() - boxSize; i++) {
      f1t = f2t = dt = 0;
      for (int j = 0; j < boxSize; j++) {
        f1t += f1.at(i + j);
        f2t += f2.at(i + j);
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

bool Structure::compareIADDistributions(const QList<double>& d,
                                        const QList<double>& f1,
                                        const QList<double>& f2, double decay,
                                        double smear, double* error)
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

bool Structure::compareIADDistributions(const QList<QVariant>& d,
                                        const QList<QVariant>& f1,
                                        const QList<QVariant>& f2, double decay,
                                        double smear, double* error)
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

QList<QString> Structure::getSymbols() const
{
  QList<QString> list;
  for (std::vector<Atom>::const_iterator it = atoms().begin(),
                                         it_end = atoms().end();
       it != it_end; ++it) {
    QString symbol = ElemInfo::getAtomicSymbol((*it).atomicNumber()).c_str();
    if (!list.contains(symbol)) {
      list.append(symbol);
    }
  }
  qSort(list);
  return list;
}

QList<uint> Structure::getNumberOfAtomsAlpha() const
{
  QList<uint> list;
  QList<QString> symbols = getSymbols();
  for (int i = 0; i < symbols.size(); i++)
    list.append(0);
  int ind;
  for (std::vector<Atom>::const_iterator it = atoms().begin(),
                                         it_end = atoms().end();
       it != it_end; ++it) {
    QString symbol = ElemInfo::getAtomicSymbol((*it).atomicNumber()).c_str();
    Q_ASSERT_X(symbols.contains(symbol), Q_FUNC_INFO,
               "getNumberOfAtomsAlpha found a symbol not in getSymbols.");
    ind = symbols.indexOf(symbol);
    ++list[ind];
  }
  return list;
}

QList<Vector3> Structure::getAtomCoordsFrac() const
{
  QList<Vector3> list;
  // Sort by symbols
  QList<QString> symbols = getSymbols();
  QString symbol_ref;
  QString symbol_cur;
  std::vector<Atom>::const_iterator it;
  for (int i = 0; i < symbols.size(); i++) {
    symbol_ref = symbols.at(i);
    for (it = atoms().begin(); it != atoms().end(); it++) {
      symbol_cur = ElemInfo::getAtomicSymbol((*it).atomicNumber()).c_str();
      if (symbol_cur == symbol_ref) {
        list.append(unitCell().toFractional((*it).pos()));
      }
    }
  }
  return list;
}

QString Structure::getOptElapsed() const
{
  int secs = getOptElapsedSeconds();
  int hours = static_cast<int>(secs / 3600);
  int mins = static_cast<int>((secs - hours * 3600) / 60);
  secs = secs % 60;
  QString ret;
  ret.sprintf("%d:%02d:%02d", hours, mins, secs);
  return ret;
}

int Structure::getOptElapsedSeconds() const
{
  if (m_optStart.toString() == "")
    return 0;
  if (m_optEnd.toString() == "")
    return m_optStart.secsTo(QDateTime::currentDateTime());

  return m_optStart.secsTo(m_optEnd);
}

double Structure::getOptElapsedHours() const
{
  return getOptElapsedSeconds() / 3600.0;
}

void Structure::load(QTextStream& in)
{
  QString line, str;
  QStringList strl;
  setStatus(InProcess); // Override later if status is available
  in.seek(0);
  while (!in.atEnd()) {
    line = in.readLine();
    strl = line.split(QRegExp(" "));
    // qDebug() << strl;

    if (line.contains("Generation:") && strl.size() > 1)
      setGeneration((strl.at(1)).toUInt());
    if (line.contains("ID#:") && strl.size() > 1)
      setIDNumber((strl.at(1)).toUInt());
    if (line.contains("Index:") && strl.size() > 1)
      setIndex((strl.at(1)).toUInt());
    if (line.contains("Enthalpy Rank:") && strl.size() > 2)
      setRank((strl.at(2)).toUInt());
    if (line.contains("Job ID:") && strl.size() > 2)
      setJobID((strl.at(2)).toUInt());
    if (line.contains("Current INCAR:") && strl.size() > 2)
      setCurrentOptStep((strl.at(2)).toUInt());
    if (line.contains("Current OptStep:") && strl.size() > 2)
      setCurrentOptStep((strl.at(2)).toUInt());
    if (line.contains("Ancestry:") && strl.size() > 1) {
      strl.removeFirst();
      setParents(strl.join(" "));
    }
    if (line.contains("Remote Path:") && strl.size() > 2) {
      strl.removeFirst();
      strl.removeFirst();
      setRempath(strl.join(" "));
    }
    if (line.contains("Start Time:") && strl.size() > 2) {
      strl.removeFirst();
      strl.removeFirst();
      str = strl.join(" ");
      setOptTimerStart(QDateTime::fromString(str));
    }
    if (line.contains("Status:") && strl.size() > 1) {
      setStatus(Structure::State((strl.at(1)).toInt()));
    }
    if (line.contains("failCount:") && strl.size() > 1) {
      setFailCount((strl.at(1)).toUInt());
    }
    if (line.contains("End Time:") && strl.size() > 2) {
      strl.removeFirst();
      strl.removeFirst();
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

void Structure::sortByEnthalpy(QList<Structure*>* structures)
{
  uint numStructs = structures->size();
  if (numStructs <= 1)
    return;

  // Simple selection sort
  Structure *structure_i = 0, *structure_j = 0, *tmp = 0;
  for (uint i = 0; i < numStructs - 1; i++) {
    structure_i = structures->at(i);
    QReadLocker iLocker(&structure_i->lock());

    for (uint j = i + 1; j < numStructs; j++) {
      structure_j = structures->at(j);
      QReadLocker jLocker(&structure_j->lock());
      if (structure_j->getEnthalpy() /
            static_cast<double>(structure_j->numAtoms()) <
          structure_i->getEnthalpy() /
            static_cast<double>(
              structure_i->numAtoms())) { // PSA Enthalpy per atom
        structures->swap(i, j);
        tmp = structure_i;
        structure_i = structure_j;
        structure_j = tmp;
      }
    }
  }
}

void Structure::sortByVickersHardness(QList<Structure*>* structures)
{
  uint numStructs = structures->size();
  if (numStructs <= 1)
    return;

  // Simple selection sort
  Structure *structure_i = 0, *structure_j = 0, *tmp = 0;
  for (uint i = 0; i < numStructs - 1; i++) {
    structure_i = structures->at(i);
    QReadLocker iLocker(&structure_i->lock());

    for (uint j = i + 1; j < numStructs; j++) {
      structure_j = structures->at(j);
      QReadLocker jLocker(&structure_j->lock());
      if (structure_j->vickersHardness() < structure_i->vickersHardness()) {
        structures->swap(i, j);
        tmp = structure_i;
        structure_i = structure_j;
        structure_j = tmp;
      }
    }
  }
}

void rankInPlace(const QList<Structure*>& structures)
{
  if (structures.size() == 0)
    return;
  Structure* s;
  for (uint i = 0; i < structures.size(); i++) {
    s = structures.at(i);
    QWriteLocker sLocker(&s->lock());
    s->setRank(i + 1);
  }
}

void Structure::rankByEnthalpy(const QList<Structure*>& structures)
{
  uint numStructs = structures.size();

  if (numStructs == 0)
    return;

  QList<Structure*> rstructures;

  // Copy structures to a temporary list (don't modify input list!)
  for (uint i = 0; i < numStructs; i++)
    rstructures.append(structures.at(i));

  // Simple selection sort
  Structure *structure_i = 0, *structure_j = 0, *tmp = 0;
  for (uint i = 0; i < numStructs - 1; i++) {
    structure_i = rstructures.at(i);
    QReadLocker iLocker(&structure_i->lock());

    for (uint j = i + 1; j < numStructs; j++) {
      structure_j = rstructures.at(j);
      QReadLocker jLocker(&structure_j->lock());
      if (structure_j->getEnthalpy() /
            static_cast<double>(structure_j->numAtoms()) <
          structure_i->getEnthalpy() /
            static_cast<double>(
              structure_i->numAtoms())) { // PSA Enthalpy per atom
        rstructures.swap(i, j);
        tmp = structure_i;
        structure_i = structure_j;
        structure_j = tmp;
      }
    }
  }

  rankInPlace(rstructures);
}

void Structure::sortAndRankByEnthalpy(QList<Structure*>* structures)
{
  sortByEnthalpy(structures);
  rankInPlace(*structures);
}

// greatest common divisor
uint gcd(uint a, uint b)
{
  return b == 0 ? a : gcd(b, a % b);
}

uint Structure::getFormulaUnits() const
{
  // Perform an atomistic formula unit calculation
  QList<uint> counts = getNumberOfAtomsAlpha();
  if (counts.empty())
    return 0;
  return std::accumulate(counts.begin(), counts.end(), counts[0], gcd);
}

// Returns the number of structures of each formula unit up to the
// user-specified maximum formula units numberOfEachFormulaUnit.at(n) is the
// number of structures with formula units n.
QList<uint> Structure::countStructuresOfEachFormulaUnit(
  QList<Structure*>* structures, int maxFU)
{
  QList<uint> numberOfEachFormulaUnit;
  uint numStructs = structures->size();
  Structure* structure_j = 0;
  for (int i = 0; i <= maxFU; i++) {
    int numbers = 0;
    for (uint j = 0; j < numStructs; j++) {
      structure_j = structures->at(j);
      QReadLocker(&structure_j->lock());
      if (structure_j->getFormulaUnits() == i)
        numbers += 1;
    }
    numberOfEachFormulaUnit.append(numbers);
  }
  return numberOfEachFormulaUnit;
}
} // end namespace GlobalSearch

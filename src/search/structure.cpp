/**********************************************************************
  Structure - Search lifecycle structure object

  Copyright (C) 2009-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/structure.h>

#include <common/compatibility/platform_compat.h>
#include <common/output.h>
#include <common/constants.h>

#include <QFile>
#include <QStringList>

#include <algorithm>
#include <cmath>

using namespace std;

namespace Search {

namespace {
bool hasUsablePrimaryObjective(const Search::Structure* structure)
{
  return structure->getStrucObjNumber() > 0 && !GS_ISNAN(structure->getStrucObjValues(0)) &&
         !GS_ISINF(structure->getStrucObjValues(0));
}

// Return the results sort group.
int sortGroupRank(Search::Structure::State state)
{
  if (Search::Structure::isOptimizedState(state))      return 0;
  if (Search::Structure::isActiveState(state))            return 1;
  if (state == Search::Structure::WaitingForOptimization) return 2;
  if (Search::Structure::isDismissedFinalState(state))    return 3;
  if (Search::Structure::isTerminalFailureState(state))   return 4;
  return 5;
}

// An structure's sorting data, copied under its lock before the sort.
struct SortEntry
{
  Search::Structure* structure;
  int group;
  bool hasObjective;
  double objective;
  int index;
};

bool shouldSortBeforeByPrimaryObjective(const SortEntry& lhs, const SortEntry& rhs)
{
  // Sort by the major grouping first; a lower group number comes first.
  if (lhs.group != rhs.group)
    return lhs.group < rhs.group;

  // Sort optimized structures by the objective.
  if (lhs.hasObjective != rhs.hasObjective)
    return lhs.hasObjective;
  if (lhs.hasObjective && rhs.hasObjective) {
    if (lhs.objective < rhs.objective)
      return true;
    if (rhs.objective < lhs.objective)
      return false;
  }

  return lhs.index < rhs.index;
}

void rankInPlace(const QList<Structure*>& structures)
{
  if (structures.size() == 0)
    return;
  Structure* s;
  for (int i = 0; i < structures.size(); i++) {
    s = structures.at(i);
    QWriteLocker sLocker(&s->lock());
    s->setRank(i + 1);
  }
}

} // namespace


bool Structure::isQueueTerminalState(State state)
{
  switch (state) {
    case Optimized:
    case Killed:
    case Removed:
    case Dismissed:
    case ObjcFailed:
    case ConsFailed:
      return true;
    default:
      return false;
  }
}

bool Structure::isQueueTerminalState() const
{
  return Structure::isQueueTerminalState(getStatus());
}

bool Structure::isQueueInProgressState(State state)
{
  switch (state) {
    case Submitted:
    case InProcess:
    case ObjectiveCalculation:
    case ConstraintCalculation:
      return true;
    default:
      return false;
  }
}

bool Structure::isQueueInProgressState() const
{
  return Structure::isQueueInProgressState(getStatus());
}

bool Structure::isPostOptimizationCalculationState(State state)
{
  switch (state) {
    case ObjectiveCalculation:
    case ConstraintCalculation:
      return true;
    default:
      return false;
  }
}

bool Structure::isPostOptimizationCalculationState() const
{
  return Structure::isPostOptimizationCalculationState(getStatus());
}

bool Structure::isOptimizedState(State state)
{
  switch (state) {
    case Optimized:
      return true;
    default:
      return false;
  }
}

bool Structure::isOptimizedState() const
{
  return Structure::isOptimizedState(getStatus());
}

bool Structure::isFailedFinalState(State state)
{
  switch (state) {
    case ObjcFailed:
    case ConsFailed:
      return true;
    default:
      return false;
  }
}

bool Structure::isFailedFinalState() const
{
  return Structure::isFailedFinalState(getStatus());
}

bool Structure::isDismissedFinalState(State state)
{
  switch (state) {
    case Dismissed:
      return true;
    default:
      return false;
  }
}

bool Structure::isDismissedFinalState() const
{
  return Structure::isDismissedFinalState(getStatus());
}

bool Structure::isKilledOrRemovedState(State state)
{
  switch (state) {
    case Killed:
    case Removed:
      return true;
    default:
      return false;
  }
}

bool Structure::isKilledOrRemovedState() const
{
  return Structure::isKilledOrRemovedState(getStatus());
}

bool Structure::isStoppedFinalState(State state)
{
  return Structure::isKilledOrRemovedState(state) || Structure::isDismissedFinalState(state) ||
         Structure::isFailedFinalState(state);
}

bool Structure::isStoppedFinalState() const
{
  return Structure::isStoppedFinalState(getStatus());
}

bool Structure::isQueueErrorRecoveryState(State state)
{
  switch (state) {
    case Error:
    case Restart:
      return true;
    default:
      return false;
  }
}

bool Structure::isQueueErrorRecoveryState() const
{
  return Structure::isQueueErrorRecoveryState(getStatus());
}

bool Structure::isActiveState(State state)
{
  switch (state) {
    case StepOptimized:
    case InProcess:
    case Empty:
    case Updating:
    case Submitted:
    case Error:
    case Restart:
    case ObjectiveCalculation:
    case Postprocessing:
    case ConstraintCalculation:
      return true;
    default:
      return false;
  }
}

bool Structure::isActiveState() const
{
  return Structure::isActiveState(getStatus());
}

bool Structure::isTerminalFailureState(State state)
{
  switch (state) {
    case Killed:
    case Removed:
    case ObjcFailed:
    case ConsFailed:
      return true;
    default:
      return false;
  }
}

bool Structure::isTerminalFailureState() const
{
  return Structure::isTerminalFailureState(getStatus());
}

QString Structure::statusText(State state, bool longText)
{
  switch (state) {
    case Optimized:
      return "Optimized";
    case StepOptimized:
      return longText ? "Checking status..." : "Checking";
    case WaitingForOptimization:
      return longText ? "Waiting for optimization" : "Waiting";
    case InProcess:
      return longText ? "In process" : "InProcess";
    case Empty:
      return longText ? "Structure empty..." : "Empty";
    case Updating:
      return longText ? "Updating structure..." : "Updating";
    case Submitted:
      return longText ? "Job submitted" : "Submitted";
    case Error:
      return longText ? "Job error" : "Error";
    case Killed:
      return "Killed";
    case Removed:
      return "Removed";
    case Restart:
      return longText ? "Restarting job..." : "Restart";
    case Postprocessing:
      return longText ? "Postprocessing..." : "Postproc";
    case ObjectiveCalculation:
      return longText ? "Calculating objectives..." : "ObjcCalcs";
    case ConstraintCalculation:
      return longText ? "Calculating constraints..." : "ConsCalcs";
    case Dismissed:
      return "Dismissed";
    case ObjcFailed:
      return longText ? "Objective failed" : "ObjcFailed";
    case ConsFailed:
      return longText ? "Constraint failed" : "ConsFailed";
    default:
      return "Unknown";
  }
}

QString Structure::statusText(bool longText) const
{
  if (isSimilar()) {
    if (longText)
      return QString("Similar to %1").arg(getSimilarityString());
    return QString("Sim(%1)").arg(getSimilarityString());
  }

  return Structure::statusText(getStatus(), longText);
}

Structure::Structure(QObject* parent)
  : QObject(parent), Atoms::Geometry(),
    m_strucConstraintRedoCount(0),
    m_strucObjState(Structure::Os_NotCalculated),
    m_strucConstraintState(Structure::Cs_NotCalculated),
    m_updatedSinceSimChecked(true),
    m_generation(0), m_id(0), m_rank(0), m_jobID(0),
    m_paretoFrontIndex(-1),
    m_optStart(QDateTime()), m_optEnd(QDateTime()), m_index(-1),
    m_lock(QReadWriteLock::Recursive),
    m_parentStructure(nullptr), m_copyFiles(), m_reusePreoptBonding(true)
{
  m_currentOptStep = 0;
  m_fixCount = 0;
  m_failCount = 0;
  setStatus(Empty);
  resetFailCount();
}

// Make a copy without a parent.
Structure::Structure(const Structure& other)
  : QObject(nullptr), Atoms::Geometry(),
    m_lock(QReadWriteLock::Recursive)
{
  *this = other;
}

Structure::Structure(Structure&& other) noexcept
  : QObject(nullptr), Atoms::Geometry(),
    m_lock(QReadWriteLock::Recursive)
{
  *this = std::move(other);
}

void Structure::setupConnections()
{
}

Structure::~Structure()
{
}

Structure& Structure::operator=(const Structure& other)
{
  if (this != &other) {
    Atoms::Geometry::operator=(static_cast<const Atoms::Geometry&>(other));

    m_hasEnthalpy = other.m_hasEnthalpy;
    m_energy = other.m_energy;
    m_enthalpy = other.m_enthalpy;
    m_PV = other.m_PV;
    m_updatedSinceSimChecked = other.m_updatedSinceSimChecked.load();
    m_generation = other.m_generation;
    m_id = other.m_id;
    m_rank = other.m_rank;
    m_jobID = other.m_jobID;
    m_currentOptStep = other.m_currentOptStep;
    m_failCount = other.m_failCount;
    m_fixCount = other.m_fixCount;
    m_parents = other.m_parents;
    m_simString = other.m_simString;
    m_rempath = other.m_rempath;
    m_locpath = other.m_locpath;
    m_status = other.m_status.load();
    m_optStart = other.m_optStart;
    m_optEnd = other.m_optEnd;
    m_index = other.m_index.load();
    m_histAtomicNums = other.m_histAtomicNums;
    m_histEnthalpies = other.m_histEnthalpies;
    m_histEnergies = other.m_histEnergies;
    m_histCoords = other.m_histCoords;
    m_histCells = other.m_histCells;
    m_parentStructure = other.m_parentStructure;
    m_copyFiles = other.m_copyFiles;
    m_reusePreoptBonding = other.m_reusePreoptBonding;
    m_preoptBonds = other.m_preoptBonds;
    m_strucObjValues = other.m_strucObjValues;
    m_strucConstraintValues = other.m_strucConstraintValues;
    m_strucObjState = ObjectivesState(other.m_strucObjState);
    m_strucConstraintState = ConstraintState(other.m_strucConstraintState);
    m_strucConstraintRedoCount = other.m_strucConstraintRedoCount;
    m_paretoFrontIndex = other.m_paretoFrontIndex;
  }

  return *this;
}

Structure& Structure::operator=(Structure&& other) noexcept
{
  if (this != &other) {
    Atoms::Geometry::operator=(static_cast<Atoms::Geometry&&>(other));

    m_hasEnthalpy = other.m_hasEnthalpy;
    m_energy = other.m_energy;
    m_enthalpy = other.m_enthalpy;
    m_PV = other.m_PV;
    m_updatedSinceSimChecked = other.m_updatedSinceSimChecked.load();
    m_generation = std::move(other.m_generation);
    m_id = std::move(other.m_id);
    m_rank = std::move(other.m_rank);
    m_jobID = std::move(other.m_jobID);
    m_currentOptStep = std::move(other.m_currentOptStep);
    m_failCount = std::move(other.m_failCount);
    m_fixCount = std::move(other.m_fixCount);
    m_parents = std::move(other.m_parents);
    m_simString = std::move(other.m_simString);
    m_rempath = std::move(other.m_rempath);
    m_locpath = std::move(other.m_locpath);
    m_status = std::move(other.m_status.load());
    m_optStart = std::move(other.m_optStart);
    m_optEnd = std::move(other.m_optEnd);
    m_index = other.m_index.load();
    m_histAtomicNums = std::move(other.m_histAtomicNums);
    m_histEnthalpies = std::move(other.m_histEnthalpies);
    m_histEnergies = std::move(other.m_histEnergies);
    m_histCoords = std::move(other.m_histCoords);
    m_histCells = std::move(other.m_histCells);
    m_parentStructure = std::move(other.m_parentStructure);

    // We never delete the parent pointer; just clear it in the moved-from
    //   object.
    other.m_parentStructure = nullptr;
    m_copyFiles = std::move(other.m_copyFiles);
    m_reusePreoptBonding = std::move(other.m_reusePreoptBonding);
    m_preoptBonds = std::move(other.m_preoptBonds);
    m_strucObjValues = std::move(other.m_strucObjValues);
    m_strucConstraintValues = std::move(other.m_strucConstraintValues);
    m_strucObjState = std::move(ObjectivesState(other.m_strucObjState));
    m_strucConstraintState = std::move(ConstraintState(other.m_strucConstraintState));
    m_strucConstraintRedoCount = std::move(other.m_strucConstraintRedoCount);
    m_paretoFrontIndex = std::move(other.m_paretoFrontIndex);
  }

  return *this;
}








void Structure::structureChanged()
{
  m_simString.clear();
  m_updatedSinceSimChecked = true;
}

bool Structure::updateAndSkipHistory(const QList<unsigned int>& atomicNums,
                                     const QList<Common::Vector3>& coords,
                                     const double energy, const double enthalpy,
                                     const Common::Matrix3& cell)
{
  Q_ASSERT_X(atomicNums.size() == coords.size(), Q_FUNC_INFO,
             "Lengths of atomicNums and coords must match.");
  if (atomicNums.size() != coords.size() ||
      (!cell.isZero() && !Atoms::Geometry::isCellMatrixUsable(cell)))
    return false;

  clearAtoms();
  for (int i = 0; i < atomicNums.size(); ++i)
    addAtom(atomicNums.at(i), coords.at(i));

  // enthalpy ~0 means none was reported
  if (fabs(enthalpy) < ZERO06) {
    resetEnthalpy();
  } else {
    setEnthalpy(enthalpy);
    setPV(enthalpy - energy);
  }
  setEnergy(energy);

  if (!cell.isZero())
    setCellInfo(cell);

  // Mark the structure for the next similarity check.
  structureChanged();
  return true;
}

bool Structure::updateAndAddToHistory(const QList<unsigned int>& atomicNums,
                                      const QList<Common::Vector3>& coords,
                                      const double energy,
                                      const double enthalpy,
                                      const Common::Matrix3& cell)
{
  Q_ASSERT_X(atomicNums.size() == coords.size(), Q_FUNC_INFO,
             "Lengths of atomicNums and coords must match numAtoms().");
  if (atomicNums.size() != coords.size() || (!cell.isZero() && !Atoms::Geometry::isCellMatrixUsable(cell)))
    return false;

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
  // enthalpy ~0 means none was reported
  if (fabs(enthalpy) < ZERO06) {
    resetEnthalpy();
  } else {
    setEnthalpy(enthalpy);
    setPV(enthalpy - energy);
  }
  setEnergy(energy);

  // Update cell if necessary
  if (!cell.isZero())
    setCellInfo(cell);

  // Mark the structure for the next similarity check.
  structureChanged();
  return true;
}

bool Structure::updateAndAddToHistory(const Atoms::Geometry& structure, const double energy,
  const double enthalpy)
{
  QList<unsigned int> atomicNums;
  QList<Common::Vector3> coords;
  for (const auto& atom : structure.atoms()) {
    atomicNums.append(atom.atomicNumber());
    coords.append(atom.pos());
  }

  return updateAndAddToHistory(atomicNums, coords, energy, enthalpy,
    structure.is3D() ? structure.unitCell().cellMatrix() : Common::Matrix3::Zero());
}

void Structure::deleteFromHistory(unsigned int index)
{
  Q_ASSERT_X(index < sizeOfHistory(), Q_FUNC_INFO,
    "Requested history index greater than the number of available entries.");

  m_histAtomicNums.removeAt(index);
  m_histEnthalpies.removeAt(index);
  m_histEnergies.removeAt(index);
  m_histCoords.removeAt(index);
  m_histCells.removeAt(index);
}

void Structure::retrieveHistoryEntry(unsigned int index,
                                     QList<unsigned int>* atomicNums,
                                     QList<Common::Vector3>* coords, double* energy,
                                     double* enthalpy, Common::Matrix3* cell)
{
  Q_ASSERT_X(index < sizeOfHistory(), Q_FUNC_INFO,
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

QString Structure::getResultsEntry(int objectives_num, int optstep, int objective_offset,
                                   int constraints_num) const
{
  // Generic fallback only; XtalOpt uses Xtal::getResultsEntry().
  QString status;
  const State state = getStatus();
  switch (state) {
    case StepOptimized:
    case WaitingForOptimization:
    case Submitted:
    case InProcess:
    case Updating:
      status = "Opt Step " + QString::number(optstep + 1);
      break;
    default:
      status = statusText(false);
      break;
  }

  QString out = QString("%1 %2 %3 %4 %5")
      .arg(getRank(), 6)
      .arg(getTag(), 8)
      .arg(getChemicalFormula(), 12)
      .arg(getIndex(), 6)
      .arg(getEnthalpy(), 10);
  for (int i = 0; i < objectives_num; i++) {
    const int objectiveIndex = objective_offset + i;
    if (objectiveIndex < getStrucObjNumber())
      out += " " + QString("%1").arg(getStrucObjValues(objectiveIndex), 10, 'g', 6);
    else
      out += QString("%1").arg("-", 11);
  }
  for (int i = 0; i < constraints_num; i++) {
    if (i < getStrucConstraintNumber())
      out += " " + QString("%1").arg(getStrucConstraintValues(i), 10, 'g', 6);
    else
      out += QString("%1").arg("-", 11);
  }
  out += QString("%1")
      .arg(status, 11);

  return out;
}

QString Structure::getOptElapsed() const
{
  int secs = getOptElapsedSeconds();
  int hours = static_cast<int>(secs / 3600);
  int mins = static_cast<int>((secs - hours * 3600) / 60);
  secs = secs % 60;
  QString ret;
  ret = QString("%1:%2:%3")
          .arg(hours)
          .arg(mins, 2, 10, QChar('0'))
          .arg(secs, 2, 10, QChar('0'));
  return ret;
}

int Structure::getOptElapsedSeconds() const
{
  if (m_optStart.isNull())
    return 0;
  if (m_optEnd.isNull())
    return m_optStart.secsTo(QDateTime::currentDateTime());

  return m_optStart.secsTo(m_optEnd);
}

double Structure::getOptElapsedHours() const
{
  return getOptElapsedSeconds() / 3600.0;
}

void Structure::sortAndRankStructures(QList<Structure*>* structures)
{
  // Copy every structure's sorting data first to avoid real time changes
  std::vector<SortEntry> entries;
  entries.reserve(structures->size());
  for (int i = 0; i < structures->size(); ++i) {
    Structure* structure = structures->at(i);
    QReadLocker structureLocker(&structure->lock());
    SortEntry entry;
    entry.structure = structure;
    const State state = structure->getStatus();
    entry.group = sortGroupRank(state);
    entry.hasObjective =
      isOptimizedState(state) && hasUsablePrimaryObjective(structure);
    entry.objective = entry.hasObjective ? structure->getStrucObjValues(0) : 0.0;
    entry.index = structure->getIndex();
    entries.push_back(entry);
  }

  std::stable_sort(entries.begin(), entries.end(), shouldSortBeforeByPrimaryObjective);

  for (int i = 0; i < structures->size(); ++i)
    (*structures)[i] = entries[i].structure;
  rankInPlace(*structures);
}

} // end namespace Search

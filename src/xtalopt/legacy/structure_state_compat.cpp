/**********************************************************************
  structure_state_compat - Old structure.state compatibility.

  Copyright (C) 2009-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/legacy/structure_state_compat.h>
#include <xtalopt/legacy/state_compat.h>

#include <xtalopt/xtalopt.h>

#include <common/output.h>

#include <QSettings>

#include <limits>

// Convert XtalOpt v14 structure states.

namespace XtalOpt {
using Search::Structure;

namespace {

// Structure status values written by XtalOpt v14.
enum Version14State
{
  Version14Optimized = 0,
  Version14StepOptimized = 1,
  Version14WaitingForOptimization = 2,
  Version14InProcess = 3,
  Version14Empty = 4,
  Version14Updating = 5,
  Version14Error = 6,
  Version14Submitted = 7,
  Version14Killed = 8,
  Version14Removed = 9,
  Version14Similar = 10,
  Version14Restart = 11,
  Version14ObjectiveDismiss = 12,
  Version14ObjectiveFail = 13,
  Version14ObjectiveRetain = 14,
  Version14ObjectiveCalculation = 15
};

enum StructureStateLayout
{
  UnsupportedStructureState,
  Version14StructureState,
  CurrentStructureState
};

StructureStateLayout structureStateLayout(const QSettings& settings)
{
  const int version = settings.value("structure/version", -1).toInt();
  if (version == CurrentStateSchemaVersion)
    return CurrentStructureState;
  if (version == Legacy::Version4StateSchemaVersion &&
      settings.contains("structure/hasValidComposition"))
    return Version14StructureState;
  return UnsupportedStructureState;
}

bool currentStateFromVersion14Status(int statusValue, Structure::State& state)
{
  switch (statusValue) {
    case Version14Optimized:
      state = Structure::Optimized;
      break;
    case Version14StepOptimized:
      state = Structure::StepOptimized;
      break;
    case Version14WaitingForOptimization:
      state = Structure::WaitingForOptimization;
      break;
    case Version14InProcess:
      state = Structure::InProcess;
      break;
    case Version14Empty:
      state = Structure::Empty;
      break;
    case Version14Updating:
      state = Structure::Updating;
      break;
    case Version14Error:
      state = Structure::Error;
      break;
    case Version14Submitted:
      state = Structure::Submitted;
      break;
    case Version14Killed:
      state = Structure::Killed;
      break;
    case Version14Removed:
      state = Structure::Removed;
      break;
    case Version14Similar:
      state = Structure::Optimized;
      break;
    case Version14Restart:
      state = Structure::Restart;
      break;
    case Version14ObjectiveDismiss:
      state = Structure::Dismissed;
      break;
    case Version14ObjectiveFail:
      state = Structure::ObjcFailed;
      break;
    case Version14ObjectiveRetain:
      state = Structure::Optimized;
      break;
    case Version14ObjectiveCalculation:
      state = Structure::ObjectiveCalculation;
      break;
    default:
      return false;
  }

  return true;
}

bool splitVersion4ObjectiveValues(const QList<double>& loadedValues,
                                  int currentFullCount, int currentUserCount,
                                  const QList<int>& constraintObjectiveIndices,
                                  QList<double>& objectiveValues,
                                  QList<double>* constraintValues)
{
  const int movedObjectiveCount = constraintObjectiveIndices.size();
  const bool valuesIncludeBuiltinObjective =
    loadedValues.size() == currentFullCount + movedObjectiveCount;
  const bool valuesAreUserObjectivesOnly =
    loadedValues.size() == currentUserCount + movedObjectiveCount;
  if (!valuesIncludeBuiltinObjective && !valuesAreUserObjectivesOnly)
    return false;

  objectiveValues.clear();
  objectiveValues.reserve(valuesIncludeBuiltinObjective ? currentFullCount : currentUserCount);
  if (constraintValues) {
    constraintValues->clear();
    constraintValues->reserve(movedObjectiveCount);
  }

  for (int i = 0; i < loadedValues.size(); ++i) {
    const int loadedUserObjectiveIndex = valuesIncludeBuiltinObjective
      ? i - XtalOpt::getFirstUserObjectiveIndex() : i;
    if (loadedUserObjectiveIndex >= 0 &&
        constraintObjectiveIndices.contains(loadedUserObjectiveIndex)) {
      if (constraintValues)
        constraintValues->append(loadedValues.at(i));
      continue;
    }
    objectiveValues.append(loadedValues.at(i));
  }

  return objectiveValues.size() ==
           (valuesIncludeBuiltinObjective ? currentFullCount : currentUserCount) &&
           (!constraintValues || constraintValues->size() == movedObjectiveCount);
}

void normalizeStatusAfterLegacyConstraintMove(Search::Structure& structure)
{
  if (structure.getStatus() == Structure::ObjcFailed &&
      structure.getStrucConstraintState() == Structure::Cs_Fail)
    structure.setStatus(Structure::ConsFailed);
}

} // end anonymous namespace

namespace Legacy {

bool convertAndReadStructureState(Search::Structure& structure,
                                  const QString& structureStateFilename,
                                  const QString& mainStateFilename,
                                  bool& currentInfoRead)
{
  currentInfoRead = false;

  QSettings settings(structureStateFilename, QSettings::IniFormat);
  const StructureStateLayout layout = structureStateLayout(settings);
  if (layout == CurrentStructureState)
    return true;
  if (layout != Version14StructureState)
    return false;

  Structure::State currentState;
  if (!currentStateFromVersion14Status(static_cast<int>(structure.getStatus()), currentState))
    return false;
  structure.setStatus(currentState);

  if (structure.sizeOfHistory() > 0) {
    QList<unsigned int> atomicNums;
    QList<Common::Vector3> coords;
    double energy = 0.0;
    double enthalpy = 0.0;
    Common::Matrix3 cell;
    structure.retrieveHistoryEntry(structure.sizeOfHistory() - 1, &atomicNums, &coords,
                                   &energy, &enthalpy, &cell);
    if (!atomicNums.isEmpty() && atomicNums.size() == coords.size())
      currentInfoRead = structure.updateAndSkipHistory(atomicNums, coords, energy, enthalpy, cell);
  }

  QString error;
  QList<int> constraintObjectiveIndices;
  int legacyObjectiveCount = 0;
  if (mainStateFilename.isEmpty() ||
      !readVersion4Objectives(mainStateFilename, constraintObjectiveIndices,
                              legacyObjectiveCount, &error)) {
    Common::error(QString("The old structure state file %1 cannot be read: %2")
                  .arg(structureStateFilename).arg(error));
    return false;
  }

  // The objectives that are not moved to constraints are the current ones, and
  //   the built-in objective is added before them.
  const int currentUserCount = legacyObjectiveCount - constraintObjectiveIndices.size();
  const int currentFullCount = currentUserCount + XtalOpt::getFirstUserObjectiveIndex();

  QList<double> legacyValues;
  settings.beginGroup("structure");
  const int legacyConstraintRedoCount = settings.value("objectivesFailCount", 0).toInt();
  const int objectiveSize = settings.beginReadArray("objectives");
  for (int i = 0; i < objectiveSize; ++i) {
    settings.setArrayIndex(i);
    legacyValues.append(settings.value("value").toDouble());
  }
  settings.endArray();
  settings.endGroup();

  structure.setStrucConstraintRedoCount(legacyConstraintRedoCount);
  QList<double> values = legacyValues.isEmpty()
    ? structure.getStrucObjValuesVec() : legacyValues;
  if (!legacyValues.isEmpty())
    structure.setStrucObjValuesVec(values);

  if (!constraintObjectiveIndices.isEmpty() && !legacyValues.isEmpty()) {
    QList<double> normalized;
    QList<double> constraintValues;
    if (!splitVersion4ObjectiveValues(values, currentFullCount, currentUserCount,
                                      constraintObjectiveIndices, normalized,
                                      &constraintValues)) {
      Common::error(QString("The objective information in old structure state file %1 does "
                            "not match the main state file.").arg(structureStateFilename));
      return false;
    }

    structure.setStrucObjValuesVec(normalized);
    structure.setStrucConstraintValuesVec(constraintValues);
    values = normalized;
    if (structure.getStrucConstraintState() == Structure::Cs_NotCalculated) {
      if (structure.getStrucObjState() == Structure::Os_Dismiss)
        structure.setStrucConstraintState(Structure::Cs_Dismiss);
      else if (structure.getStrucObjState() == Structure::Os_Fail)
        structure.setStrucConstraintState(Structure::Cs_Fail);
      else
        structure.setStrucConstraintState(Structure::Cs_Retain);
      normalizeStatusAfterLegacyConstraintMove(structure);
    }
  }

  if (!legacyValues.isEmpty() && values.size() != currentFullCount &&
      values.size() != currentUserCount) {
    Common::error(QString("The objective information in old structure state file %1 does "
                          "not match the main state file.").arg(structureStateFilename));
    return false;
  }

  if (currentFullCount > currentUserCount && values.size() == currentUserCount) {
    QList<double> expanded;
    expanded.reserve(currentFullCount);
    for (int i = 0; i < currentFullCount; ++i)
      expanded.append(std::numeric_limits<double>::quiet_NaN());
    for (int i = 0; i < currentUserCount; ++i)
      expanded[XtalOpt::getFirstUserObjectiveIndex() + i] = values.at(i);
    structure.setStrucObjValuesVec(expanded);
  } else if (values.size() == currentFullCount && currentFullCount > currentUserCount) {
    values[XtalOpt::getBuiltinObjectiveIndex()] = std::numeric_limits<double>::quiet_NaN();
    structure.setStrucObjValuesVec(values);
  }

  return true;
}

} // namespace Legacy
} // namespace XtalOpt

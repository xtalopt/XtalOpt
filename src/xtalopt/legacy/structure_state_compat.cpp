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

#include <xtalopt/xtalopt.h>

#include <QSettings>

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
  if (version == FloorStateSchemaVersion &&
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

} // end anonymous namespace

namespace Legacy {

bool normalizeStructureStateAfterRead(Search::Structure& structure, const QString& filename)
{
  QSettings settings(filename, QSettings::IniFormat);
  if (structureStateLayout(settings) != Version14StructureState)
    return false;

  Structure::State currentState;
  if (!currentStateFromVersion14Status(static_cast<int>(structure.getStatus()), currentState))
    return false;
  structure.setStatus(currentState);

  if (structure.sizeOfHistory() == 0)
    return false;

  QList<unsigned int> atomicNums;
  QList<Common::Vector3> coords;
  double energy = 0.0;
  double enthalpy = 0.0;
  Common::Matrix3 cell;
  structure.retrieveHistoryEntry(structure.sizeOfHistory() - 1, &atomicNums, &coords, &energy, &enthalpy, &cell);
  if (atomicNums.isEmpty() || atomicNums.size() != coords.size())
    return false;

  return structure.updateAndSkipHistory(atomicNums, coords, energy, enthalpy, cell);
}

} // namespace Legacy

bool isStateFileLoadable(const QString& filename)
{
  QSettings settings(filename, QSettings::IniFormat);
  const bool saveSuccessful =
    settings.value("structure/saveSuccessful", false).toBool();
  const StructureStateLayout layout = structureStateLayout(settings);
  if (layout == UnsupportedStructureState || !saveSuccessful)
    return false;

  if (layout == CurrentStructureState) {
    const int status = settings.value("structure/status", -1).toInt();
    if (status < static_cast<int>(Structure::Optimized) ||
        status > static_cast<int>(Structure::ConsFailed))
      return false;
  }

  if (layout == Version14StructureState) {
    Structure::State currentState;
    const int status = settings.value("structure/status", -1).toInt();
    if (!currentStateFromVersion14Status(status, currentState))
      return false;
  }

  return true;
}

} // namespace XtalOpt

/**********************************************************************
  PWSCFOptimizer - Optimizer interface for PWscf (Quantum ESPRESSO).

  Copyright (C) 2010-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/optimizers/pwscfoptimizer.h>

#include <search/constants.h>

#include <common/constants.h>
#include <common/fileutils.h>
#include <common/output.h>
#include <common/stringutils.h>
#include <search/structure.h>
#include <atoms/formats/pwscfformat.h>
#include <atoms/geometry.h>

#include <cmath>
#include <cstring>
#include <sstream>

namespace Search {
namespace {

bool readPwscfEnergy(const QString& filename, double& energy, double& enthalpy)
{
  std::string text;
  if (!Common::readFileToString(filename, &text)) {
    Common::error(QString("PWSCF output, %1, could not be opened!")
                 .arg(filename));
    return false;
  }
  std::istringstream ifs(text);

  bool energyFound = false;
  std::string line;
  std::vector<std::string> lineSplit;
  while (getline(ifs, line)) {
    if (strstr(line.c_str(), "Final enthalpy")) {
      lineSplit = Common::split(line, ' ');
      if (lineSplit.size() < 4 || !Common::parseDoubleString(lineSplit[3], enthalpy)) {
        Common::error(QString("Could not read final enthalpy in PWSCF output! %1")
                     .arg(line.c_str()));
        return false;
      }
      enthalpy *= RY2EV;
      if (std::fabs(energy) < ZERO08)
        energy = enthalpy;
      energyFound = true;
    } else if (strstr(line.c_str(), "!    total energy")) {
      lineSplit = Common::split(line, ' ');
      if (lineSplit.size() < 5 || !Common::parseDoubleString(lineSplit[4], energy)) {
        Common::error(QString("Could not read final energy in PWSCF output! %1")
                     .arg(line.c_str()));
        return false;
      }
      energy *= RY2EV;
      energyFound = true;
    }
  }

  if (!energyFound)
    Common::error("Energy not found in PWSCF output!");
  return energyFound;
}

} // namespace

const OptimizerDefaults& PWSCFOptimizer::defaults()
{
  static const char* const templates[] = { "xtal.in", nullptr };
  static const char* const assets[] = { nullptr };
  static const char* const outputs[] = { "xtal.out", nullptr };
  // clang-format off
  static const OptimizerDefaults s{
    "PWscf", templates, assets,
    "xtal.out", "JOB DONE", outputs,
    "pw.x", "xtal.in", "xtal.out", "xtal.err" };
  // clang-format on
  return s;
}

PWSCFOptimizer::PWSCFOptimizer(SearchBase* parent)
  : Optimizer(parent)
{
  m_optimizerDefaults = &defaults();
}

PWSCFOptimizer::~PWSCFOptimizer()
{
}

bool PWSCFOptimizer::readOutput(Structure* s, const QString& filename) const
{
  Atoms::Geometry structure = *s;
  if (!Atoms::PwscfFormat::readOutput(structure, filename))
    return false;

  double energy = 0.0;
  double enthalpy = 0.0;
  if (!readPwscfEnergy(filename, energy, enthalpy))
    return false;

  return s->updateAndAddToHistory(structure, energy, enthalpy);
}
} // namespace Search

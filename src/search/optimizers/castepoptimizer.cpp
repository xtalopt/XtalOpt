/**********************************************************************
  CASTEPOptimizer - Optimizer interface for CASTEP.

  Copyright (C) 2010-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/optimizers/castepoptimizer.h>

#include <common/constants.h>
#include <common/fileutils.h>
#include <common/output.h>
#include <common/stringutils.h>
#include <search/structure.h>
#include <atoms/formats/castepformat.h>
#include <atoms/geometry.h>

#include <cmath>
#include <cstring>
#include <sstream>

namespace Search {
namespace {

bool readCastepEnergy(const QString& filename, double& energy, double& enthalpy)
{
  std::string text;
  if (!Common::readFileToString(filename, &text)) {
    Common::error(QString("CASTEP output, %1, could not be opened!")
                 .arg(filename));
    return false;
  }
  std::istringstream ifs(text);

  bool energyFound = false;
  std::string line;
  std::vector<std::string> lineSplit;
  while (getline(ifs, line)) {
    if (strstr(line.c_str(), "Final Enthalpy")) {
      lineSplit = Common::split(line, '=');
      const std::vector<std::string> valueFields =
        lineSplit.size() >= 2 ? Common::split(lineSplit[1], ' ') : std::vector<std::string>();
      if (valueFields.empty() || !Common::parseDoubleString(valueFields[0], enthalpy)) {
        Common::error(QString("Could not read final enthalpy in CASTEP output! %1")
                     .arg(line.c_str()));
        return false;
      }
      if (std::fabs(energy) < ZERO08)
        energy = enthalpy;
      energyFound = true;
    } else if (strstr(line.c_str(), "Final energy, E")) {
      lineSplit = Common::split(line, ' ');
      if (lineSplit.size() < 5 || !Common::parseDoubleString(lineSplit[4], energy)) {
        Common::error(QString("Could not read final energy in CASTEP output! %1")
                     .arg(line.c_str()));
        return false;
      }
      energyFound = true;
    }
  }

  if (!energyFound)
    Common::error("Energy not found in CASTEP output!");
  return energyFound;
}

} // namespace

const OptimizerDefaults& CASTEPOptimizer::defaults()
{
  static const char* const templates[] = { "xtal.param", "xtal.cell", nullptr };
  static const char* const assets[] = { nullptr };
  static const char* const outputs[] = { "xtal.castep", nullptr };
  static const OptimizerDefaults s{
    "CASTEP", templates, assets, "xtal.castep",
    "Geometry optimization completed successfully.", outputs,
    "castep xtal", "", "", "" };
  return s;
}

CASTEPOptimizer::CASTEPOptimizer(SearchBase* parent)
  : Optimizer(parent)
{
  m_optimizerDefaults = &defaults();
}

CASTEPOptimizer::~CASTEPOptimizer()
{
}

bool CASTEPOptimizer::readOutput(Structure* s, const QString& filename) const
{
  Atoms::Geometry structure;
  if (!Atoms::CastepFormat::readOutput(&structure, filename))
    return false;

  double energy = 0.0;
  double enthalpy = 0.0;
  if (!readCastepEnergy(filename, energy, enthalpy))
    return false;

  s->updateAndAddToHistory(structure, energy, enthalpy);
  return true;
}
} // namespace Search

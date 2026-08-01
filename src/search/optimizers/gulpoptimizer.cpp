/**********************************************************************
  GULPOptimizer - Optimizer interface for GULP.

  Copyright (C) 2010-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/optimizers/gulpoptimizer.h>

#include <common/fileutils.h>
#include <common/output.h>
#include <common/stringutils.h>
#include <search/structure.h>
#include <atoms/formats/gulpformat.h>
#include <atoms/geometry.h>

#include <cstring>
#include <sstream>

namespace Search {
namespace {

bool readGulpEnergy(const QString& filename, double& energy)
{
  std::string text;
  if (!Common::readFileToString(filename, &text)) {
    Common::error(QString("GULP output, %1, could not be opened!")
                 .arg(filename));
    return false;
  }
  std::istringstream ifs(text);

  bool energyFound = false;
  std::string line;
  std::vector<std::string> lineSplit;
  while (getline(ifs, line)) {
    if (!strstr(line.c_str(), "Total lattice energy") || !strstr(line.c_str(), "eV"))
      continue;

    lineSplit = Common::split(line, ' ');
    if (lineSplit.size() < 5 || !Common::parseDoubleString(lineSplit[4], energy)) {
      Common::error(QString("Incomplete energy line in GULP output: %1")
                   .arg(line.c_str()));
      return false;
    }
    energyFound = true;
  }

  if (!energyFound)
    Common::error("Energy not found in GULP output!");
  return energyFound;
}

} // namespace

const OptimizerDefaults& GULPOptimizer::defaults()
{
  static const char* const templates[] = { "xtal.gin", nullptr };
  static const char* const assets[] = { nullptr };
  static const char* const outputs[] = { "xtal.got", nullptr };
  static const OptimizerDefaults s{
    "GULP", templates, assets, "xtal.got", "**** Optimisation achieved ****",
    outputs, "gulp", "xtal.gin", "xtal.got", "xtal.ger" };
  return s;
}

GULPOptimizer::GULPOptimizer(SearchBase* parent)
  : Optimizer(parent)
{
  m_optimizerDefaults = &defaults();
}

GULPOptimizer::~GULPOptimizer()
{
}

bool GULPOptimizer::readOutput(Structure* s, const QString& filename) const
{
  Atoms::Geometry structure;
  if (!Atoms::GulpFormat::readOutput(&structure, filename))
    return false;

  double energy = 0.0;
  double enthalpy = 0.0;
  if (!readGulpEnergy(filename, energy))
    return false;

  return s->updateAndAddToHistory(structure, energy, enthalpy);
}
} // namespace Search

/**********************************************************************
  MTPOptimizer - Optimizer interface for MTP (machine-learned potentials).

  Copyright (C) 2010-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/optimizers/mtpoptimizer.h>

#include <common/fileutils.h>
#include <common/output.h>
#include <common/stringutils.h>
#include <search/structure.h>
#include <atoms/formats/mtpformat.h>
#include <atoms/geometry.h>

#include <cstring>
#include <sstream>

namespace Search {
namespace {

bool readMtpResultState(const QString& filename, const QString& tag, double& energy)
{
  std::string text;
  if (!Common::readFileToString(filename, &text)) {
    Common::error(QString("MTP output, %1, could not be opened!")
                 .arg(filename));
    return false;
  }
  std::istringstream ifs(text);

  bool energyFound = false;
  bool relaxOK = false;
  std::string line;
  std::vector<std::string> lineSplit;
  while (getline(ifs, line)) {
    if (strstr(line.c_str(), "BEGIN_CFG")) {
      // A new block starts; keep only the last block's results.
      energyFound = false;
      relaxOK = false;
    } else if (strstr(line.c_str(), "Energy")) {
      getline(ifs, line);
      lineSplit = Common::split(line, ' ');
      if (lineSplit.size() != 1 || !Common::parseDoubleString(lineSplit[0], energy)) {
        Common::error(QString("Could not read the energy in MTP output! %1")
                     .arg(line.c_str()));
        return false;
      }
      energyFound = true;
    } else if (strstr(line.c_str(), "relaxation_OK")) {
      relaxOK = true;
    }
  }

  if (!relaxOK) {
    Common::error(QString("Unsuccessful relaxation in MTP output for %1")
                 .arg(tag));
    return false;
  }
  if (!energyFound) {
    Common::error("Energy not found in MTP output!");
    return false;
  }
  return true;
}

} // namespace

const OptimizerDefaults& MTPOptimizer::defaults()
{
  static const char* const templates[] = { "mtp.relax", "mtp.pot", "mtp.cfg",
                                            nullptr };
  static const char* const assets[] = { nullptr };
  static const char* const outputs[] = { "xtal.mot_0", nullptr };
  // clang-format off
  static const OptimizerDefaults s{
    "MTP", templates, assets,
    "xtal.mot_0", "Energy", outputs,
    "mlp relax mtp.relax --cfg-filename=mtp.cfg --save-relaxed=xtal.mot",
    "mtp.cfg", "mtp.out", "" };
  // clang-format on
  return s;
}

MTPOptimizer::MTPOptimizer(SearchBase* parent)
  : Optimizer(parent)
{
  m_optimizerDefaults = &defaults();
}

MTPOptimizer::~MTPOptimizer()
{
}

bool MTPOptimizer::readOutput(Structure* s, const QString& filename) const
{
  Atoms::Geometry structure;
  if (!Atoms::MtpFormat::read(structure, filename))
    return false;

  double energy = 0.0;
  double enthalpy = 0.0;
  if (!readMtpResultState(filename, s->getTag(), energy))
    return false;

  return s->updateAndAddToHistory(structure, energy, enthalpy);
}
} // namespace Search

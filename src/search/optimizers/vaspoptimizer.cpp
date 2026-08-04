/**********************************************************************
  VASPOptimizer - Optimizer interface for VASP.

  Copyright (C) 2010-2011 by David C. Lonie
  Copyright (C) 2017 by Patrick Avery
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/optimizers/vaspoptimizer.h>

#include <atoms/eleminfo.h>
#include <atoms/formats/poscarformat.h>
#include <common/fileutils.h>
#include <common/output.h>
#include <common/stringutils.h>
#include <search/search.h>
#include <search/structure.h>

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>

#include <QDir>
#include <QFileInfo>
#include <QStringList>

namespace Search {
namespace {

bool getOUTCAREnergy(std::istream& in, double& energy)
{
  if (!in)
    return false;

  std::string line;
  // We will read backwards and stop as soon as we find the energy
  in.seekg(0, std::ios::end);
  while (in.tellg() >= 0) {
    Common::reverseGetline(in, line);
    if (strstr(line.c_str(), "free  energy   TOTEN")) {
      std::vector<std::string> stringSplit = Common::split(line, ' ');
      // Make sure the line is long enough. If not, just keep reading.
      if (stringSplit.size() < 5)
        continue;

      // Ignore an invalid energy value (eg, "*****" chars): don't use previous read-in value!
      return Common::parseDoubleString(stringSplit[4], energy);
    } else if (strstr(line.c_str(), "free  energy ML TOTEN")) { // VASP ML output
      std::vector<std::string> stringSplit = Common::split(line, ' ');
      // Make sure the line is long enough. If not, just keep reading.
      if (stringSplit.size() < 6)
        continue;

      return Common::parseDoubleString(stringSplit[5], energy);
    }
  }
  return false;
}

bool getOUTCAREnthalpy(std::istream& in, double& enthalpy)
{
  if (!in)
    return false;

  std::string line;
  // We will read backwards and stop as soon as we find the enthalpy
  in.seekg(0, std::ios::end);
  while (in.tellg() >= 0) {
    Common::reverseGetline(in, line);
    if (strstr(line.c_str(), "enthalpy is  TOTEN")) {
      std::vector<std::string> stringSplit = Common::split(line, ' ');
      // Make sure the line is long enough. If not, just keep reading.
      if (stringSplit.size() < 5)
        continue;

      // Ignore an invalid enthalpy value (eg, "*****" chars): don't use previous read-in value!
      return Common::parseDoubleString(stringSplit[4], enthalpy);
    } else if (strstr(line.c_str(), "enthalpy is ML TOTEN")) { // VASP ML output
      std::vector<std::string> stringSplit = Common::split(line, ' ');
      // Make sure the line is long enough. If not, just keep reading.
      if (stringSplit.size() < 6)
        continue;

      return Common::parseDoubleString(stringSplit[5], enthalpy);
    }
  }
  return false;
}

} // namespace

const OptimizerDefaults& VASPOptimizer::defaults()
{
  static const char* const templates[] = { "INCAR", "KPOINTS", nullptr };
  static const char* const assets[] = { "POTCAR", nullptr };
  static const char* const outputs[] = { "CONTCAR", "POSCAR", nullptr };
  static const OptimizerDefaults s{
    "VASP", templates, assets, "OUTCAR", "Total CPU time", outputs,
    "vasp", "", "", "" };
  return s;
}

VASPOptimizer::VASPOptimizer(SearchBase* parent)
  : Optimizer(parent)
{
  m_optimizerDefaults = &defaults();
}

VASPOptimizer::~VASPOptimizer()
{
}

bool VASPOptimizer::addOptimizerInputFiles(Structure* s, int optStep,
                                           QHash<QString, QString>& files) const
{
  std::stringstream poscar;
  if (Atoms::PoscarFormat::write(*s, poscar, s->getLocpath()))
    files.insert("POSCAR", QString::fromStdString(poscar.str()));

  const QString rawPotcarAsset =
    QString::fromStdString(m_search->getOptimizerInputAsset(optStep, "POTCAR"));
  const QHash<QString, QString> potcarAssets = inputAssetTextToMap(rawPotcarAsset);
  if (!rawPotcarAsset.trimmed().isEmpty() && potcarAssets.isEmpty()) {
    Common::error(QString("Could not read the saved POTCAR input asset for %1.")
                    .arg(s->getTag()));
    return false;
  }
  const QString systemPotcarAsset = potcarAssets.value("system");
  if (!systemPotcarAsset.isEmpty()) {
    QString potcar;
    if (!readSavedInputAssetValue(systemPotcarAsset, potcar))
      return false;
    files.insert("POTCAR", potcar);
  } else if (!potcarAssets.isEmpty()) {
    QString potcar;
    const QStringList symbols = s->getSymbols();
    for (const auto& symbol : symbols) {
      const QString speciesAsset = potcarAssets.value(symbol);
      if (speciesAsset.isEmpty()) {
        Common::error(QString("No POTCAR input asset for species %1 in %2.")
                        .arg(symbol)
                        .arg(s->getTag()));
        return false;
      }
      QString speciesPotcar;
      if (!readSavedInputAssetValue(speciesAsset, speciesPotcar))
        return false;
      potcar += speciesPotcar;
      if (!potcar.endsWith('\n'))
        potcar += '\n';
    }
    files.insert("POTCAR", potcar);
  }

  return true;
}

bool VASPOptimizer::readOutput(Structure* s, const QString& filename) const
{
  std::string contcarText;
  if (!Common::readFileToString(filename, &contcarText)) {
    Common::error(QString("CONTCAR, %1, could not be opened!").arg(filename));
    return false;
  }
  std::istringstream ifs(contcarText);

  // First, read the POSCAR file
  Atoms::Geometry structure;
  if (!Atoms::PoscarFormat::read(structure, ifs)) {
    Common::error("VASPOptimizer: failed to read POSCAR file!");
    return false;
  }

  // Now find the energy in the OUTCAR.
  QString outcarFile = QFileInfo(filename).dir().filePath("OUTCAR");

  // Open it and make sure it exists
  std::string outcarText;
  std::istringstream outcar_ifs;
  const bool outcarRead = Common::readFileToString(outcarFile, &outcarText);
  if (outcarRead)
    outcar_ifs.str(outcarText);

  // We don't want to print a warning here if the OUTCAR file doesn't exist
  // because sometimes (like when the user is loading seeds), there is no
  // OUTCAR. We do not want the user to be concerned about it.

  double energy = 0.0, enthalpy = 0.0;

  bool energyFound = getOUTCAREnergy(outcar_ifs, energy);
  getOUTCAREnthalpy(outcar_ifs, enthalpy);

  if (outcarRead && !energyFound) {
    Common::warning("The energy could not be found in the OUTCAR file!");
    return false;
  }

  return s->updateAndAddToHistory(structure, energy, enthalpy);
}
} // namespace Search

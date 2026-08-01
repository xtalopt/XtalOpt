/**********************************************************************
  SIESTAOptimizer - Optimizer interface for SIESTA.

  Copyright (C) 2010-2011 by David C. Lonie
  Copyright (C) 2016 by Patrick Avery
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/optimizers/siestaoptimizer.h>

#include <common/fileutils.h>
#include <common/output.h>
#include <common/stringutils.h>
#include <search/search.h>
#include <search/structure.h>
#include <atoms/formats/siestaformat.h>
#include <atoms/geometry.h>

#include <cstring>
#include <sstream>

#include <QDir>
#include <QFileInfo>
#include <QStringList>

namespace Search {
namespace {

bool readSiestaEnergy(const QString& filename, double& energy, double& enthalpy)
{
  QFileInfo info(filename);
  const QString basisEnthalpyFile = QDir(info.absolutePath()).filePath("BASIS_ENTHALPY");
  std::string basisEnthalpyText;
  if (Common::readFileToString(basisEnthalpyFile, &basisEnthalpyText)) {
    std::istringstream basisEnthalpyIfs(basisEnthalpyText);
    std::string line;
    getline(basisEnthalpyIfs, line);
    // Read the optional enthalpy file (malformed or absent file leaves enthalpy unset).
    const std::vector<std::string> entries = Common::split(line, ' ');
    if (entries.empty() || !Common::parseDoubleString(entries[0], enthalpy)) {
      Common::warning(QString("Could not parse BASIS_ENTHALPY value: %1")
                     .arg(line.c_str()));
    }
  }

  std::string text;
  if (!Common::readFileToString(filename, &text)) {
    Common::error(QString("SIESTA output, %1, could not be opened!")
                 .arg(filename));
    return false;
  }
  std::istringstream ifs(text);

  bool energyFound = false;
  std::string line;
  std::vector<std::string> lineSplit;
  while (getline(ifs, line)) {
    if (!strstr(line.c_str(), "siesta: Final energy (eV)"))
      continue;

    getline(ifs, line);
    line = Common::trim(line);
    while (strstr(line.c_str(), "siesta")) {
      if (strstr(line.c_str(), "Total =")) {
        lineSplit = Common::split(line, ' ');
        if (lineSplit.size() != 4 || !Common::parseDoubleString(lineSplit[3], energy)) {
          Common::error("Could not read energy in SIESTA output.");
          Common::error("Please contact the XtalOpt developers.");
          Common::error(QString("Faulty line is: %1").arg(line.c_str()));
          return false;
        }
        energyFound = true;
        break;
      }
      getline(ifs, line);
      line = Common::trim(line);
    }
  }

  if (!energyFound)
    Common::error("Energy not found in SIESTA output!");
  return energyFound;
}

} // namespace

const OptimizerDefaults& SIESTAOptimizer::defaults()
{
  static const char* const templates[] = { "xtal.fdf", nullptr };
  static const char* const assets[] = { "PSF", nullptr };
  static const char* const outputs[] = { "xtal.out", nullptr };
  static const OptimizerDefaults s{
    "SIESTA", templates, assets, "xtal.out", "siesta: Final energy (eV):",
    outputs, "siesta", "xtal.fdf", "xtal.out", "" };
  return s;
}

SIESTAOptimizer::SIESTAOptimizer(SearchBase* parent)
  : Optimizer(parent)
{
  m_optimizerDefaults = &defaults();
}

SIESTAOptimizer::~SIESTAOptimizer()
{
}

bool SIESTAOptimizer::addOptimizerInputFiles(Structure* s, int optStep,
                                             QHash<QString, QString>& files) const
{
  const QString rawPsfAsset =
    QString::fromStdString(m_search->getOptimizerInputAsset(optStep, "PSF"));
  const QHash<QString, QString> psfAssets = inputAssetTextToMap(rawPsfAsset);
  if (psfAssets.isEmpty())
    return true;

  const QStringList symbols = s->getSymbols();
  for (const auto& symbol : symbols) {
    const QString psfAsset = psfAssets.value(symbol);
    if (psfAsset.isEmpty()) {
      Common::error(QString("No PSF input asset for species %1 in %2.")
                      .arg(symbol)
                      .arg(s->getTag()));
      return false;
    }

    QString contents;
    if (!readSavedInputAssetValue(psfAsset, contents))
      return false;
    files.insert(symbol + ".psf", contents);
  }

  return true;
}

bool SIESTAOptimizer::readOutput(Structure* s, const QString& filename) const
{
  Atoms::Geometry structure;
  if (!Atoms::SiestaFormat::readOutput(&structure, filename))
    return false;

  double energy = 0.0;
  double enthalpy = 0.0;
  if (!readSiestaEnergy(filename, energy, enthalpy))
    return false;

  return s->updateAndAddToHistory(structure, energy, enthalpy);
}
} // namespace Search

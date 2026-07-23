/**********************************************************************
  settings - XtalOpt settings: one table defines every keyword.

  Copyright (C) 2017 by Patrick S. Avery
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/settings.h>

#include <xtalopt/xtalopt.h>

#include <common/constants.h>
#include <common/compatibility/platform_compat.h>
#include <common/output.h>
#include <common/stringutils.h>

#include <QHash>
#include <QSet>

#include <algorithm>
#include <functional>
#include <limits>
#include <type_traits>
#include <utility>

namespace XtalOpt {
namespace Settings {

namespace {

// Functions for scalar values
typedef std::function<QString(const XtalOpt&)> GetFn;
typedef std::function<bool(XtalOpt&, const QString&)> SetFn;
// Functions for repeated values
typedef std::function<QStringList(const XtalOpt&)> ListFn;
typedef std::function<void(XtalOpt&)> ClearFn;

// A settings table entry
struct Row
{
  const char* keyword;
  const char* defaultValue; // compute at startup if null
  bool required;
  bool runtimeChangeable;
  GetFn get;     // scalar: read the value as text
  SetFn set;     // scalar: parse text and store the value
  ListFn list;   // repeated: parse the stored entries as text lines
  SetFn add;     // repeated: parse one entry line and append it
  ClearFn clear; // repeated: drop all entries (before a re-read)
};

// Basic table entry: keyword, default, and flags (no setter/getter).
Row baseRow(const char* kw, const char* def, bool req, bool rt)
{
  Row r;
  r.keyword = kw;
  r.defaultValue = def;
  r.required = req;
  r.runtimeChangeable = rt;
  return r;
}

// Add an "optscheme" entry: per-step optimizer, queue, templates, etc.
// Saved to state in [xtalopt/optscheme] group.
Row optscheme(const char* kw, const char* def, bool req, bool rt)
{
  return baseRow(kw, def, req, rt);
}

// Add a "repeated" keyword.
// Saved to state in [xtalopt/input] group.
template <class ListM, class AddM, class ClearM>
Row repeated(const char* kw, const char* def, bool req, bool rt, ListM listFn, AddM addFn, ClearM clearFn)
{
  Row r = baseRow(kw, def, req, rt);
  r.list = [listFn](const XtalOpt& o) -> QStringList { return (o.*listFn)(); };
  r.add = [addFn](XtalOpt& o, const QString& v) -> bool { return (o.*addFn)(v); };
  r.clear = [clearFn](XtalOpt& o) { (o.*clearFn)(); };
  return r;
}

// Call a setting's setter function
template <class S, class T> bool callSetter(XtalOpt& o, S s, const T& v, std::true_type) { return (o.*s)(v); }
template <class S, class T> bool callSetter(XtalOpt& o, S s, const T& v, std::false_type) { (o.*s)(v); return true; }

template <class S, class T> bool callSetter(XtalOpt& o, S s, const T& v)
{
  return callSetter(o, s, v, std::is_same<decltype((o.*s)(v)), bool>{});
}

// Add a "scalar" entry and its XtalOpt/SearchBase setter and getter.
// Settings such as enums use their text functions (eg, failActionText()).
template <class Getter, class Setter, typename std::enable_if<std::is_member_function_pointer<Getter>::value, int>::type = 0>
Row scalar(const char* kw, const char* def, bool req, bool rt, Getter g, Setter s)
{
  using T = typename std::decay<
    decltype((std::declval<const XtalOpt&>().*g)())>::type;
  Row r = baseRow(kw, def, req, rt);
  r.get = [g](const XtalOpt& o) -> QString {
    return Common::valueToText((o.*g)());
  };
  r.set = [s](XtalOpt& o, const QString& v) -> bool {
    T tmp;
    if (!Common::textToValue(v, tmp))
      return false;
    return callSetter(o, s, tmp);
  };
  return r;
}

// The complete settings table: one row per keyword, with its default and
//   whether it is required or can change during a run.

const QList<Row>& rows()
{
  static const QList<Row> table = []() {
    QList<Row> t;

    //  Required inputs
    t << scalar("chemicalFormulas", "", true, false, &XtalOpt::getInputFormulasString, &XtalOpt::setInputFormulasString);
    t << optscheme("queueInterface", "none", true, false);
    t << optscheme("optimizer", "gulp", true, false);

    //  Domain
    t << scalar("description", "", false, false, &XtalOpt::getDescription, &XtalOpt::setDescription);
    t << scalar("referenceEnergies", "", false, false, &XtalOpt::getInputEneRefsString, &XtalOpt::setInputEneRefsString);
    t << scalar("elementalVolumes", "", false, true, &XtalOpt::getInputEleVolmString, &XtalOpt::setInputEleVolmString);
    t << scalar("seedStructures", "", false, false, &XtalOpt::seedStructuresText, &XtalOpt::setSeedStructuresText);

    //  Composition limits
    t << scalar("maxAtoms", "20", false, false, &XtalOpt::getMaxAtoms, &XtalOpt::setMaxAtoms);
    t << scalar("minAtoms", "1", false, false, &XtalOpt::getMinAtoms, &XtalOpt::setMinAtoms);
    t << scalar("vcSearch", "false", false, false, &XtalOpt::getVcSearch, &XtalOpt::setVcSearch);
    t << scalar("saveHullSnapshots", "false", false, true, &XtalOpt::getSaveHullSnapshots, &XtalOpt::setSaveHullSnapshots);

    //  Lattice limits
    t << scalar("aMin", "3.0", false, true, &XtalOpt::getAMin, &XtalOpt::setAMin);
    t << scalar("bMin", "3.0", false, true, &XtalOpt::getBMin, &XtalOpt::setBMin);
    t << scalar("cMin", "3.0", false, true, &XtalOpt::getCMin, &XtalOpt::setCMin);
    t << scalar("aMax", "10.0", false, true, &XtalOpt::getAMax, &XtalOpt::setAMax);
    t << scalar("bMax", "10.0", false, true, &XtalOpt::getBMax, &XtalOpt::setBMax);
    t << scalar("cMax", "10.0", false, true, &XtalOpt::getCMax, &XtalOpt::setCMax);
    t << scalar("alphaMin", "60.0", false, true, &XtalOpt::getAlphaMin, &XtalOpt::setAlphaMin);
    t << scalar("betaMin", "60.0", false, true, &XtalOpt::getBetaMin, &XtalOpt::setBetaMin);
    t << scalar("gammaMin", "60.0", false, true, &XtalOpt::getGammaMin, &XtalOpt::setGammaMin);
    t << scalar("alphaMax", "120.0", false, true, &XtalOpt::getAlphaMax, &XtalOpt::setAlphaMax);
    t << scalar("betaMax", "120.0", false, true, &XtalOpt::getBetaMax, &XtalOpt::setBetaMax);
    t << scalar("gammaMax", "120.0", false, true, &XtalOpt::getGammaMax, &XtalOpt::setGammaMax);

    //  Volume limits
    t << scalar("minVolume", "1.0", false, true, &XtalOpt::getVolMin, &XtalOpt::setVolMin);
    t << scalar("maxVolume", "100.0", false, true, &XtalOpt::getVolMax, &XtalOpt::setVolMax);
    t << scalar("minVolumeScale", "0.0", false, true, &XtalOpt::getVolScaleMin, &XtalOpt::setVolScaleMin);
    t << scalar("maxVolumeScale", "0.0", false, true, &XtalOpt::getVolScaleMax, &XtalOpt::setVolScaleMax);

    //  Interatomic distances
    t << scalar("usingScaledIADs", "true", false, true, &XtalOpt::getUsingScaledIAD, &XtalOpt::setUsingScaledIAD);
    t << scalar("usingCustomIADs", "false", false, true, &XtalOpt::getUsingCustomIAD, &XtalOpt::setUsingCustomIAD);
    t << scalar("checkIADPostOptimization", "false", false, true, &XtalOpt::getUsingCheckStepOpt, &XtalOpt::setUsingCheckStepOpt);
    t << scalar("radiiScalingFactor", "0.5", false, true, &XtalOpt::getScaleFactor, &XtalOpt::setScaleFactor);
    t << scalar("minRadius", "0.25", false, true, &XtalOpt::getMinRadius, &XtalOpt::setMinRadius);
    t << repeated("customIAD", "", false, false, &XtalOpt::customIADLines, &XtalOpt::processInputCustomIAD, &XtalOpt::clearCustomIADs);

    //  RandSpg
    t << scalar("usingRandSpg", "false", false, false, &XtalOpt::getUsingRandSpg, &XtalOpt::setUsingRandSpg);
    t << scalar("forcedSpgs", "", false, false, &XtalOpt::getInputForcedSpgsString, &XtalOpt::setInputForcedSpgsString);

    //  MolUnit
    t << repeated("molUnit", "", false, false, &XtalOpt::molUnitLines, &XtalOpt::processInputMoleculeUnit, &XtalOpt::clearMoleculeUnits);

    //  Search limits
    t << scalar("numInitial", "20", false, false, &XtalOpt::getNumInitial, &XtalOpt::setNumInitial);
    t << scalar("parentsPoolSize", "20", false, true, &XtalOpt::getParentsPoolSize, &XtalOpt::setParentsPoolSize);
    t << scalar("limitRunningJobs", "true", false, true, &XtalOpt::isLimitRunningJobs, &XtalOpt::setLimitRunningJobs);
    t << scalar("runningJobLimit", "1", false, true, &XtalOpt::getRunningJobLimit, &XtalOpt::setRunningJobLimit);
    t << scalar("continuousStructures", "15", false, true, &XtalOpt::getContStructs, &XtalOpt::setContStructs);
    t << scalar("jobFailLimit", "1", false, true, &XtalOpt::getFailLimit, &XtalOpt::setFailLimit);
    t << scalar("jobFailAction", "kill", false, true, &XtalOpt::failActionText, &XtalOpt::setFailActionText);
    t << scalar("maxNumStructures", "100", false, true, &XtalOpt::getMaxNumStructures, &XtalOpt::setMaxNumStructures);
    t << scalar("softExit", "false", false, true, &XtalOpt::isSoftExit, &XtalOpt::setSoftExit);
    t << scalar("hardExit", "false", false, true, &XtalOpt::isHardExit, &XtalOpt::setHardExit);

    //  Objectives
    t << scalar("optimizationType", "basic", false, true, &XtalOpt::optimizationTypeText, &XtalOpt::setOptimizationTypeText);
    t << scalar("tournamentSelection", "true", false, true, &XtalOpt::isTournamentSelection, &XtalOpt::setTournamentSelection);
    t << scalar("restrictedPool", "false", false, true, &XtalOpt::isRestrictedPool, &XtalOpt::setRestrictedPool);
    t << scalar("crowdingDistance", "true", false, true, &XtalOpt::isCrowdingDistance, &XtalOpt::setCrowdingDistance);
    t << scalar("paretoFilterZeroWeights", "false", false, true, &XtalOpt::isParetoFilterZeroWeights, &XtalOpt::setParetoFilterZeroWeights);
    t << scalar("objectivePrecision", "-1", false, true, &XtalOpt::getObjectivePrecision, &XtalOpt::setObjectivePrecision);
    t << scalar("constraintsReDo", "false", false, true, &XtalOpt::isConstraintsReDo, &XtalOpt::setConstraintsReDo);
    t << repeated("objective", "", false, false, &XtalOpt::objectiveLines, &XtalOpt::processInputObjectives, &XtalOpt::resetObjectives);
    t << repeated("constraint", "", false, false, &XtalOpt::constraintLines, &XtalOpt::processInputConstraint, &XtalOpt::resetConstraints);

    //  Operator weights
    t << scalar("weightPermutomic", "15", false, true, &XtalOpt::getPAtomic, &XtalOpt::setPAtomic);
    t << scalar("weightPermucomp", "5", false, true, &XtalOpt::getPComp, &XtalOpt::setPComp);
    t << scalar("weightStripple", "25", false, true, &XtalOpt::getPStrip, &XtalOpt::setPStrip);
    t << scalar("weightPermustrain", "25", false, true, &XtalOpt::getPPerm, &XtalOpt::setPPerm);
    t << scalar("weightCrossover", "35", false, true, &XtalOpt::getPCross, &XtalOpt::setPCross);
    t << scalar("randomSuperCell", "0", false, true, &XtalOpt::getPSupercell, &XtalOpt::setPSupercell);

    //  Operator parameters
    t << scalar("strippleAmplitudeMin", "0.5", false, true, &XtalOpt::getStripAmpMin, &XtalOpt::setStripAmpMin);
    t << scalar("strippleAmplitudeMax", "1.0", false, true, &XtalOpt::getStripAmpMax, &XtalOpt::setStripAmpMax);
    t << scalar("strippleNumWavesAxis1", "1", false, true, &XtalOpt::getStripPer1, &XtalOpt::setStripPer1);
    t << scalar("strippleNumWavesAxis2", "1", false, true, &XtalOpt::getStripPer2, &XtalOpt::setStripPer2);
    t << scalar("strippleStrainStdevMin", "0.5", false, true, &XtalOpt::getStripStrainStdevMin, &XtalOpt::setStripStrainStdevMin);
    t << scalar("strippleStrainStdevMax", "0.5", false, true, &XtalOpt::getStripStrainStdevMax, &XtalOpt::setStripStrainStdevMax);
    t << scalar("permustrainNumExchanges", "4", false, true, &XtalOpt::getPermEx, &XtalOpt::setPermEx);
    t << scalar("permustrainStrainStdevMax", "0.5", false, true, &XtalOpt::getPermStrainStdevMax, &XtalOpt::setPermStrainStdevMax);
    t << scalar("crossoverCuts", "1", false, true, &XtalOpt::getCrossNcuts, &XtalOpt::setCrossNcuts);
    t << scalar("crossoverMinContribution", "25", false, true, &XtalOpt::getCrossMinimumContribution, &XtalOpt::setCrossMinimumContribution);

    //  Tolerances
    t << scalar("xtalcompToleranceLength", "0.1", false, true, &XtalOpt::getTolXcLength, &XtalOpt::setTolXcLength);
    t << scalar("xtalcompToleranceAngle", "2.0", false, true, &XtalOpt::getTolXcAngle, &XtalOpt::setTolXcAngle);
    t << scalar("spglibTolerance", nullptr, false, true, &XtalOpt::getTolSpg, &XtalOpt::setTolSpg);
    t << scalar("rdfTolerance", "0.98", false, true, &XtalOpt::getTolRdf, &XtalOpt::setTolRdf);
    t << scalar("rdfCutoff", "6.0", false, false, &XtalOpt::getTolRdfCutoff, &XtalOpt::setTolRdfCutoff);
    t << scalar("rdfNumBins", "3000", false, false, &XtalOpt::getTolRdfNbins, &XtalOpt::setTolRdfNbins);
    t << scalar("rdfSigma", "0.008", false, false, &XtalOpt::getTolRdfSigma, &XtalOpt::setTolRdfSigma);

    //  Output
    t << scalar("verboseOutput", "false", false, true, &XtalOpt::isVerbose, &XtalOpt::setVerbose);
    t << scalar("debugOutput", "false", false, true, &XtalOpt::isDebugOutput, &XtalOpt::setDebugOutput);

    //  Optimizer/Queue and connection
    t << optscheme("numOptimizationSteps", "1", false, false);
    t << optscheme("templatesDirectory", ".", false, false);
    t << scalar("localWorkingDirectory", "local", false, false, &XtalOpt::getLocWorkDir, &XtalOpt::setLocWorkDir);
    t << scalar("remoteQueue", "false", false, false, &XtalOpt::isRemoteQueue, &XtalOpt::setRemoteQueue);
    t << scalar("sshMethod", nullptr, false, false, &XtalOpt::sshMethod, &XtalOpt::setSshMethod);
    t << scalar("host", "", false, false, &XtalOpt::getHost, &XtalOpt::setHost);
    t << scalar("port", "22", false, false, &XtalOpt::getPort, &XtalOpt::setPort);
    t << scalar("user", "", false, false, &XtalOpt::getUsername, &XtalOpt::setUsername);
    t << scalar("remoteWorkingDirectory", "", false, false, &XtalOpt::getRemWorkDir, &XtalOpt::setRemWorkDir);
    t << optscheme("submitCommand", "", false, false);
    t << optscheme("cancelCommand", "", false, false);
    t << optscheme("statusCommand", "", false, false);
    t << scalar("queueRefreshInterval", "10", false, true, &XtalOpt::queueRefreshInterval, &XtalOpt::setQueueRefreshInterval);
    t << scalar("cleanRemoteDirs", "false", false, false, &XtalOpt::cleanRemoteOnStop, &XtalOpt::setCleanRemoteOnStop);
    t << scalar("logErrorDirectories", "false", false, false, &XtalOpt::logErrorDirs, &XtalOpt::setLogErrorDirs);
    t << scalar("autoCancelJobAfterTime", "false", false, true, &XtalOpt::cancelJobAfterTime, &XtalOpt::setCancelJobAfterTime);
    t << scalar("hoursForAutoCancelJob", "100.0", false, true, &XtalOpt::hoursForCancelJobAfterTime, &XtalOpt::setHoursForCancelJobAfterTime);
    t << scalar("autoCancelScriptAfterTime", "true", false, true, &XtalOpt::cancelScriptAfterTime, &XtalOpt::setCancelScriptAfterTime);
    t << scalar("hoursForAutoCancelScript", "2.0", false, true, &XtalOpt::hoursForCancelScriptAfterTime, &XtalOpt::setHoursForCancelScriptAfterTime);
    t << optscheme("directRunCommand", "", false, false);
    t << optscheme(queueTemplateKeyword(), "", false, false);

    //  Optimizer templates
    t << optscheme("ginTemplates", "", false, false);
    t << optscheme("incarTemplates", "", false, false);
    t << optscheme("kpointsTemplates", "", false, false);
    t << optscheme("pwscfTemplates", "", false, false);
    t << optscheme("castepCellTemplates", "", false, false);
    t << optscheme("castepParamTemplates", "", false, false);
    t << optscheme("fdfTemplates", "", false, false);
    t << optscheme("mtpCellTemplates", "", false, false);
    t << optscheme("mtpRelaxTemplates", "", false, false);
    t << optscheme("mtpPotTemplates", "", false, false);

    //  Input assets (per-species files)
    t << optscheme("potcarFile", "", false, false);
    t << optscheme("psfFile", "", false, false);

    //  User information
    t << scalar("user1", "", false, false, &XtalOpt::getUser1, &XtalOpt::setUser1);
    t << scalar("user2", "", false, false, &XtalOpt::getUser2, &XtalOpt::setUser2);
    t << scalar("user3", "", false, false, &XtalOpt::getUser3, &XtalOpt::setUser3);
    t << scalar("user4", "", false, false, &XtalOpt::getUser4, &XtalOpt::setUser4);

    return t;
  }();
  return table;
}

// Use a lowercase keyword to find its table index.
const QHash<QString, int>& keywordIndex()
{
  static const QHash<QString, int> index = []() {
    QHash<QString, int> m;
    for (int i = 0; i < rows().size(); ++i)
      m.insert(QString(rows().at(i).keyword).toLower(), i);
    return m;
  }();
  return index;
}

// Find a keyword in the table.
int lookupRow(const QString& raw)
{
  const QString lkey = raw.trimmed().toLower();
  auto it = keywordIndex().constFind(lkey);
  return it != keywordIndex().constEnd() ? it.value() : -1;
}

// Return a table entry (returns null on failure).
const Row* findRow(const QString& keyword)
{
  const int idx = lookupRow(keyword);
  return idx < 0 ? nullptr : &rows().at(idx);
}

// Conversion of an optimizer keyword and a file name
struct OptimizerFileKeyword
{
  const char* keyword;
  const char* filename;
};

// Optimizer template files and their input keywords.
static const OptimizerFileKeyword optimizerTemplateKeywords[] = {
  { "ginTemplates",         "xtal.gin"   },
  { "incarTemplates",       "INCAR"      },
  { "kpointsTemplates",     "KPOINTS"    },
  { "pwscfTemplates",       "xtal.in"    },
  { "castepCellTemplates",  "xtal.cell"  },
  { "castepParamTemplates", "xtal.param" },
  { "fdfTemplates",         "xtal.fdf"   },
  { "mtpCellTemplates",     "mtp.cfg"    },
  { "mtpRelaxTemplates",    "mtp.relax"  },
  { "mtpPotTemplates",      "mtp.pot"    }
};

// Optimizer input assets and their input keywords.
static const OptimizerFileKeyword optimizerInputAssets[] = {
  { "potcarFile", "POTCAR" },
  { "psfFile",    "PSF"    }
};

// Return the keyword for a file name (empty if no filename doesn't match any keyword).
template <size_t N> QString keywordForOptimizerFile(const OptimizerFileKeyword (&table)[N], const QString& file)
{
  for (const auto& entry : table) {
    if (file.compare(entry.filename, Qt::CaseInsensitive) == 0)
      return entry.keyword;
  }
  return QString();
}

// Return the file name for a keyword (empty if keywords doesn't match any filename).
template <size_t N> QString optimizerFileForKeyword(const OptimizerFileKeyword (&table)[N], const QString& keyword)
{
  for (const auto& entry : table) {
    if (keyword.compare(entry.keyword, Qt::CaseInsensitive) == 0)
      return entry.filename;
  }
  return QString();
}


} // anonymous namespace

QString keywordForOptimizerTemplateFile(const QString& filename)
{
  return keywordForOptimizerFile(optimizerTemplateKeywords, filename);
}

QString filenameForOptimizerTemplateKeyword(const QString& keyword)
{
  return optimizerFileForKeyword(optimizerTemplateKeywords, keyword);
}

QString keywordForOptimizerInputAsset(const QString& assetName)
{
  return keywordForOptimizerFile(optimizerInputAssets, assetName);
}

const char* queueTemplateKeyword()
{
  return "jobTemplates";
}

bool isOptimizerAndQueueFileKeyword(const QString& keyword)
{
  return !optimizerFileForKeyword(optimizerTemplateKeywords, keyword).isEmpty() ||
         !optimizerFileForKeyword(optimizerInputAssets, keyword).isEmpty() ||
         keyword == queueTemplateKeyword();
}


QString findKeywordName(const QString& raw)
{
  const int idx = lookupRow(raw);
  return idx < 0 ? QString() : rows().at(idx).keyword;
}

QString defaultValue(const QString& keyword)
{
  const Row* row = findRow(keyword);
  if (!row)
    return QString();
  if (row->defaultValue != nullptr)
    return row->defaultValue;

  // Computed defaults: these cannot be compile-time string literals.
  const QString kw = QString(row->keyword);
  if (kw == "spglibTolerance")
    return QString::number(SPGLIB_TOL);
  if (kw == "sshMethod")
    return Search::SearchBase::defaultSshMethod();
  return QString();
}

bool isRequired(const QString& keyword)
{
  const Row* row = findRow(keyword);
  return row && row->required;
}

bool isRuntimeChangeable(const QString& keyword)
{
  const Row* row = findRow(keyword);
  return row && row->runtimeChangeable;
}

bool isRepeatableInput(const QString& keyword)
{
  // Collect repeated entries' values (and per-step optimizer stuff).
  const Row* row = findRow(keyword);
  if (row && row->add)
    return true;
  return !optimizerFileForKeyword(optimizerInputAssets, keyword).isEmpty();
}

QStringList allKeywords()
{
  QStringList keywords;
  for (const auto& row : rows())
    keywords << row.keyword;
  return keywords;
}

QStringList requiredKeywords()
{
  QStringList keywords;
  for (const auto& row : rows()) {
    if (row.required)
      keywords << row.keyword;
  }
  return keywords;
}

QStringList runtimeKeywords()
{
  QStringList keywords;
  for (const auto& row : rows()) {
    if (row.runtimeChangeable)
      keywords << row.keyword;
  }
  return keywords;
}

bool hasScalarBinding(const QString& keyword)
{
  const Row* row = findRow(keyword);
  return row && static_cast<bool>(row->set);
}

bool applyScalar(XtalOpt& opt, const QString& keyword, const QString& value)
{
  const Row* row = findRow(keyword);
  if (!row || !row->set)
    return false;
  return row->set(opt, value);
}

QString scalarValue(const XtalOpt& opt, const QString& keyword)
{
  const Row* row = findRow(keyword);
  if (!row || !row->get)
    return QString();
  return row->get(opt);
}

bool isRepeated(const QString& keyword)
{
  const Row* row = findRow(keyword);
  return row && static_cast<bool>(row->add);
}

QStringList repeatedEntries(const XtalOpt& opt, const QString& keyword)
{
  const Row* row = findRow(keyword);
  if (!row || !row->list)
    return QStringList();
  return row->list(opt);
}

void clearRepeated(XtalOpt& opt, const QString& keyword)
{
  const Row* row = findRow(keyword);
  if (row && row->clear)
    row->clear(opt);
}

bool addRepeatedEntry(XtalOpt& opt, const QString& keyword, const QString& entry)
{
  const Row* row = findRow(keyword);
  if (!row || !row->add)
    return false;
  return row->add(opt, entry);
}

QString keywordSummaryText()
{
  QString text;
  text += "XtalOpt input file keywords (xtalopt.in):\n\n";
  text += "  R = required   C = changeable-runtime   M = multi-line\n\n";
  text += QString("  %1 %2 %3\n").arg("keyword", -28).arg("default", -24).arg("type");
  text += QString("  %1 %2 %3\n").arg(QString(27, '-') + " ", -28).arg(QString(23, '-') + " ", -24).arg("----");
  for (const auto& row : rows()) {
    QString type;
    if (row.required)
      type += "R";
    if (row.runtimeChangeable)
      type += "C";
    if (isRepeatableInput(row.keyword))
      type += "M";
    // Placeholders keep empty columns visibly aligned.
    QString def = defaultValue(row.keyword);
    if (def.isEmpty())
      def = "-";
    if (type.isEmpty())
      type = "-";
    text += QString("  %1 %2 %3\n")
              .arg(QString(row.keyword), -28)
              .arg(def, -24)
              .arg(type);
  }
  return text;
}

void applyAllDefaults(XtalOpt& opt)
{
  for (const auto& row : rows()) {
    if (!row.set)
      continue;
    applyScalar(opt, row.keyword, defaultValue(row.keyword));
  }
}

ScalarSnapshot captureScalars(const XtalOpt& opt)
{
  ScalarSnapshot snapshot;
  for (const auto& row : rows()) {
    if (row.get)
      snapshot.insert(row.keyword, row.get(opt));
  }
  return snapshot;
}

// Handle an invalid value according to "invalidAction", which depends on run mode:
//   reject the change, reset to default, restore the pre-edit value.
static bool handleInvalidSetting(XtalOpt& opt, InvalidSettingAction invalidAction,
                                 const ScalarSnapshot* base,
                                 const QString& message, const QStringList& keywords)
{
  switch (invalidAction) {
    case InvalidSettingAction::Reject:
      Common::error(message);
      return false;
    case InvalidSettingAction::ResetToDefault:
      Common::warning(message + " Resetting to default.");
      for (const QString& kw : keywords)
        applyScalar(opt, kw, defaultValue(kw));
      return true;
    case InvalidSettingAction::KeepPrevious:
      Common::warning(message + " Keeping the previous value.");
      if (base) {
        for (const QString& kw : keywords)
          applyScalar(opt, kw, base->value(kw));
      }
      return true;
  }
  return true;
}

bool validateSettings(XtalOpt& opt, InvalidSettingAction invalidAction, const ScalarSnapshot* base)
{
  bool valid = true;

  //
  // Values checked according to invalidAction
  //

  // Absolute volume limits should be both positive and max >= min.
  if (opt.getVolMin() < ZERO06 || opt.getVolMax() < ZERO06 || opt.getVolMax() < opt.getVolMin()) {
    if (!handleInvalidSetting(opt, invalidAction, base, "Absolute volume limits are invalid "
                 "(both must be positive and max >= min).", { "minVolume", "maxVolume" }))
      valid = false;
  }

  // Scaled volume limits should always have min >= 0 (0 disables them) and max >= min.
  if (opt.getVolScaleMin() < 0.0 || opt.getVolScaleMax() < opt.getVolScaleMin()) {
    if (!handleInvalidSetting(opt, invalidAction, base, "Scaled volume limits are invalid (min >= 0 and max >= min).",
                 { "minVolumeScale", "maxVolumeScale" }))
      valid = false;
  }

  // Atom limits should always have min >= 1 and min <= max.
  if (opt.getMinAtoms() < 1 || opt.getMinAtoms() > opt.getMaxAtoms()) {
    if (!handleInvalidSetting(opt, invalidAction, base, "Atom limits are invalid (min >= 1 and min <= max).",
                 { "minAtoms", "maxAtoms" }))
      valid = false;
  }

  // Make sure in VC search atom limits allows at least one atom per type.
  if (opt.getVcSearch() && !opt.compList().isEmpty() &&
      opt.getMaxAtoms() < opt.compList().first().getCompositionSymbols().size()) {
    if (!handleInvalidSetting(opt, invalidAction, base,
                               "maxAtoms is too small to include "
                               "one atom of every element in the composition (must be >= "
                               "the number of element types).",
                               {"maxAtoms" }))
      valid = false;
  }

  // The two interatomic-distance settings can't be used at the same time.
  if (opt.getUsingScaledIAD() && opt.getUsingCustomIAD()) {
    if (!handleInvalidSetting(opt, invalidAction, base,
                               "usingScaledIADs and usingCustomIADs cannot both be true.",
                               {"usingCustomIADs" }))
      valid = false;
  }

  // Lattice limits should always have acceptable values and min <= max.
  const struct
  {
    const char* axis;
    double mn;
    double mx;
    const char* kwMin;
    const char* kwMax;
    bool angle;
  } lattice[] = {
    { "a", opt.getAMin(), opt.getAMax(), "aMin", "aMax", false },
    { "b", opt.getBMin(), opt.getBMax(), "bMin", "bMax", false },
    { "c", opt.getCMin(), opt.getCMax(), "cMin", "cMax", false },
    { "alpha", opt.getAlphaMin(), opt.getAlphaMax(), "alphaMin", "alphaMax", true },
    { "beta", opt.getBetaMin(), opt.getBetaMax(), "betaMin", "betaMax", true },
    { "gamma", opt.getGammaMin(), opt.getGammaMax(), "gammaMin", "gammaMax", true },
  };
  for (const auto& lim : lattice) {
    const bool badValues = lim.angle ? lim.mn <= 0.0 || lim.mn >= 180.0 || lim.mx <= 0.0 || lim.mx >= 180.0
                                     : lim.mn <= 0.0 || lim.mx <= 0.0;
    if (badValues || lim.mn > lim.mx) {
      const QString condition = lim.angle ? "both must be between 0 and 180"
                                          : "both must be positive";

      if (!handleInvalidSetting(opt, invalidAction, base,
                                 QString("%1 lattice limits are invalid (%2 and min <= max).").arg(lim.axis, condition),
                                 {lim.kwMin, lim.kwMax }))
        valid = false;
    }
  }

  if (opt.getScaleFactor() < 0.0 || opt.getScaleFactor() > 1.0) {
    if (!handleInvalidSetting(opt, invalidAction, base,
                               "radiiScalingFactor is invalid (must be between 0 and 1).",
                               {"radiiScalingFactor"}))
      valid = false;
  }

  if (opt.getMinRadius() < 0.0) {
    if (!handleInvalidSetting(opt, invalidAction, base,
                               "minRadius is invalid (must be >= 0).",
                               {"minRadius"}))
      valid = false;
  }

  if (opt.getParentsPoolSize() < 1) {
    if (!handleInvalidSetting(opt, invalidAction, base,
                               "parentsPoolSize is invalid (must be >= 1).",
                               {"parentsPoolSize"}))
      valid = false;
  }

  if (opt.getMaxNumStructures() < 1) {
    if (!handleInvalidSetting(opt, invalidAction, base,
                               "maxNumStructures is invalid (must be >= 1).",
                               {"maxNumStructures"}))
      valid = false;
  }

  if (opt.getFailLimit() < 1) {
    if (!handleInvalidSetting(opt, invalidAction, base,
                               "jobFailLimit is invalid (must be >= 1).",
                               {"jobFailLimit"}))
      valid = false;
  }

  if (opt.getObjectivePrecision() < -1 || opt.getObjectivePrecision() > 24) {
    if (!handleInvalidSetting(opt, invalidAction, base,
                               "objectivePrecision is invalid (must be between -1 and 24).",
                               {"objectivePrecision"}))
      valid = false;
  }

  if (opt.getPSupercell() > 100) {
    if (!handleInvalidSetting(opt, invalidAction, base,
                               "randomSuperCell is invalid (must be between 0 and 100).",
                               {"randomSuperCell"}))
      valid = false;
  }

  if (opt.getStripAmpMin() < 0.0 || opt.getStripAmpMax() > 1.0 ||
      opt.getStripAmpMin() > opt.getStripAmpMax()) {
    if (!handleInvalidSetting(opt, invalidAction, base,
                               "Stripple amplitude limits are invalid "
                               "(both must be between 0 and 1 and min <= max).",
                               {"strippleAmplitudeMin", "strippleAmplitudeMax"}))
      valid = false;
  }

  if (opt.getStripStrainStdevMin() < 0.0 || opt.getStripStrainStdevMax() > 2.0 ||
      opt.getStripStrainStdevMin() > opt.getStripStrainStdevMax()) {
    if (!handleInvalidSetting(opt, invalidAction, base,
                               "Stripple strain limits are invalid "
                               "(both must be between 0 and 2 and min <= max).",
                               {"strippleStrainStdevMin", "strippleStrainStdevMax"}))
      valid = false;
  }

  if (opt.getStripPer1() < 1 || opt.getStripPer2() < 1) {
    if (!handleInvalidSetting(opt, invalidAction, base,
                               "Stripple wave counts are invalid (both must be >= 1).",
                               {"strippleNumWavesAxis1", "strippleNumWavesAxis2"}))
      valid = false;
  }

  if (opt.getPermStrainStdevMax() < 0.0) {
    if (!handleInvalidSetting(opt, invalidAction, base,
                               "permustrainStrainStdevMax is invalid (must be >= 0).",
                               {"permustrainStrainStdevMax"}))
      valid = false;
  }

  if (opt.getCrossNcuts() < 1 || opt.getCrossNcuts() > 10) {
    if (!handleInvalidSetting(opt, invalidAction, base,
                               "crossoverCuts is invalid (must be between 1 and 10).",
                               {"crossoverCuts"}))
      valid = false;
  }

  if (opt.getCrossMinimumContribution() < 25 || opt.getCrossMinimumContribution() > 50) {
    if (!handleInvalidSetting(opt, invalidAction, base,
                               "crossoverMinContribution is invalid (must be between 25 and 50).",
                               {"crossoverMinContribution"}))
      valid = false;
  }

  if (opt.getTolXcLength() < 0.0) {
    if (!handleInvalidSetting(opt, invalidAction, base,
                               "xtalcompToleranceLength is invalid (must be >= 0).",
                               {"xtalcompToleranceLength"}))
      valid = false;
  }

  if (opt.getTolXcAngle() < 0.0 || opt.getTolXcAngle() > 100.0) {
    if (!handleInvalidSetting(opt, invalidAction, base,
                               "xtalcompToleranceAngle is invalid (must be between 0 and 100).",
                               {"xtalcompToleranceAngle"}))
      valid = false;
  }

  if (opt.getTolSpg() <= 0.0) {
    if (!handleInvalidSetting(opt, invalidAction, base,
                               "spglibTolerance is invalid (must be positive).",
                               {"spglibTolerance"}))
      valid = false;
  }

  if (opt.getTolRdf() < 0.0 || opt.getTolRdf() > 1.0) {
    if (!handleInvalidSetting(opt, invalidAction, base,
                               "rdfTolerance is invalid (must be between 0 and 1).",
                               {"rdfTolerance"}))
      valid = false;
  }

  if (opt.getTolRdfCutoff() < 0.1 || opt.getTolRdfCutoff() > 20.0) {
    if (!handleInvalidSetting(opt, invalidAction, base,
                               "rdfCutoff is invalid (must be between 0.1 and 20).",
                               {"rdfCutoff"}))
      valid = false;
  }

  if (opt.getTolRdfNbins() < 100 || opt.getTolRdfNbins() > 10000) {
    if (!handleInvalidSetting(opt, invalidAction, base,
                               "rdfNumBins is invalid (must be between 100 and 10000).",
                               {"rdfNumBins"}))
      valid = false;
  }

  if (opt.getTolRdfSigma() < 0.001 || opt.getTolRdfSigma() > 2.0) {
    if (!handleInvalidSetting(opt, invalidAction, base,
                               "rdfSigma is invalid (must be between 0.001 and 2).",
                               {"rdfSigma"}))
      valid = false;
  }

  if (opt.queueRefreshInterval() < 1 || opt.queueRefreshInterval() > 99999) {
    if (!handleInvalidSetting(opt, invalidAction, base,
                               "queueRefreshInterval is invalid (must be between 1 and 99999).",
                               {"queueRefreshInterval"}))
      valid = false;
  }

  if (opt.hoursForCancelJobAfterTime() < 0.0 ||
      opt.hoursForCancelJobAfterTime() > 10000.0 ||
      (opt.cancelJobAfterTime() && opt.hoursForCancelJobAfterTime() <= 0.0)) {
    if (!handleInvalidSetting(opt, invalidAction, base,
                               "Automatic job cancellation settings are invalid "
                               "(hours must be between 0 and 10000, and positive when enabled).",
                               {"autoCancelJobAfterTime", "hoursForAutoCancelJob"}))
      valid = false;
  }

  if (opt.hoursForCancelScriptAfterTime() < 0.0 ||
      opt.hoursForCancelScriptAfterTime() > 10000.0 ||
      (opt.cancelScriptAfterTime() && opt.hoursForCancelScriptAfterTime() <= 0.0)) {
    if (!handleInvalidSetting(opt, invalidAction, base,
                               "Automatic script cancellation settings are invalid "
                               "(hours must be between 0 and 10000, and positive when enabled).",
                               {"autoCancelScriptAfterTime", "hoursForAutoCancelScript"}))
      valid = false;
  }

  if (opt.getPort() < 1 || opt.getPort() > 65535) {
    if (!handleInvalidSetting(opt, invalidAction, base,
                               "port is invalid (must be between 1 and 65535).",
                               {"port"}))
      valid = false;
  }

  //
  // Automatic fixes (always applied; there is no valid alternative)
  //

  // Atom limits must cover all input formulas' compositions.
  for (const auto& comp : opt.compList())
    opt.adjustAtomCountLimits(comp.getNumAtoms());

  return valid;
}

} // namespace Settings
} // namespace XtalOpt

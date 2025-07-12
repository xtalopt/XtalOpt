/**********************************************************************
  cliOptions.cpp - Static options class for command-line interface for XtalOpt.

  Copyright (C) 2017 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#include <memory>

#include <QFile>
#include <QString>

#include <globalsearch/constants.h>
#include <globalsearch/eleminfo.h>
#include <globalsearch/queueinterfaces/queueinterfaces.h>
#include <globalsearch/utilities/fileutils.h>
#include <globalsearch/utilities/makeunique.h>

#include <xtalopt/optimizers/optimizers.h>
#include <xtalopt/xtalopt.h>

#include <xtalopt/cliOptions.h>

using namespace std;
using namespace GlobalSearch;

namespace XtalOpt {

// To avoid multi-line complications; we define this variable
// here which stores the values for "objective" entries in the
// input file for later processing.
static QStringList objectives_input = {};

static const QStringList keywords = { "minVolumeScale",
                                      "maxVolumeScale",
                                      "elementalVolumes",
                                      "maxAtoms",
                                      "vcSearch",
                                      "saveHullSnapshots",
                                      "verboseOutput",
                                      "referenceEnergies",
                                      "randomSuperCell",
                                      "optimizationType",
                                      "tournamentSelection",
                                      "restrictedPool",
                                      "rdfTolerance",
                                      "rdfCutoff",
                                      "rdfNumBins",
                                      "rdfSigma",
                                      "user1",
                                      "user2",
                                      "user3",
                                      "user4",
                                      "crowdingDistance",
                                      "objectivePrecision",
                                      "objective",
                                      "objectivesReDo",
                                      "softExit",
                                      "hardExit",
                                      "localQueue",
                                      "seedStructures",
                                      "chemicalFormulas",
                                      "aMin",
                                      "bMin",
                                      "cMin",
                                      "aMax",
                                      "bMax",
                                      "cMax",
                                      "alphaMin",
                                      "betaMin",
                                      "gammaMin",
                                      "alphaMax",
                                      "betaMax",
                                      "gammaMax",
                                      "minVolume",
                                      "maxVolume",
                                      "usingRadiiInteratomicDistanceLimit",
                                      "usingCustomIADs",
                                      "checkIADPostOptimization",
                                      "radiiScalingFactor",
                                      "minRadius",
                                      "usingMolecularUnits",
                                      "usingRandSpg",
                                      "forcedSpgsWithRandSpg",
                                      "numInitial",
                                      "parentsPoolSize",
                                      "limitRunningJobs",
                                      "runningJobLimit",
                                      "continuousStructures",
                                      "jobFailLimit",
                                      "jobFailAction",
                                      "maxNumStructures",
                                      "weightPermutomic",
                                      "weightPermucomp",
                                      "weightStripple",
                                      "weightPermustrain",
                                      "weightCrossover",
                                      "strippleAmplitudeMin",
                                      "strippleAmplitudeMax",
                                      "strippleNumWavesAxis1",
                                      "strippleNumWavesAxis2",
                                      "strippleStrainStdevMin",
                                      "strippleStrainStdevMax",
                                      "permustrainNumExchanges",
                                      "permustrainStrainStdevMax",
                                      "crossoverCuts",
                                      "crossoverMinContribution",
                                      "xtalcompToleranceLength",
                                      "xtalcompToleranceAngle",
                                      "spglibTolerance",
                                      "templatesDirectory",
                                      "queueInterface",
                                      "localWorkingDirectory",
                                      "logErrorDirectories",
                                      "autoCancelJobAfterTime",
                                      "hoursForAutoCancelJob",
                                      "numOptimizationSteps",
                                      "host",
                                      "port",
                                      "user",
                                      "remoteWorkingDirectory",
                                      "submitCommand",
                                      "cancelCommand",
                                      "statusCommand",
                                      "queueRefreshInterval",
                                      "cleanRemoteDirs",
                                      "jobTemplates",
                                      "optimizer",
                                      "exeLocation",
                                      "castepCellTemplates",
                                      "castepParamTemplates",
                                      "ginTemplates",
                                      "pwscfTemplates",
                                      "fdfTemplates",
                                      "incarTemplates",
                                      "kpointsTemplates",
                                      "mtpCellTemplates",
                                      "mtpRelaxTemplates",
                                      "mtpPotTemplates" };

static const QStringList requiredKeywords = { "chemicalFormulas",
                                              "queueInterface", "optimizer" };

static const QStringList validQueueInterfaces = { "loadleveler", "local",
                                                  "lsf",         "pbs",
                                                  "sge",         "slurm" };

static const QStringList requiredRemoteKeywords = { "host", "user",
                                                    "remoteWorkingDirectory",
                                                    "jobTemplates" };

static const QStringList requiredLocalQueueKeywords = { "jobTemplates" };

static const QStringList validOptimizers = { "gulp", "castep", "pwscf",
                                             "siesta", "vasp" , "generic", "mtp" };

static const QHash<QString, QStringList> requiredOptimizerKeywords = {
  { "gulp", { "ginTemplates" } },

  { "vasp", { "incarTemplates", "kpointsTemplates" } },

  { "pwscf", { "pwscfTemplates" } },

  { "castep", { "castepCellTemplates", "castepParamTemplates" } },

  { "siesta", { "fdfTemplates" } },

  { "mtp" , { "mtpCellTemplates", "mtpRelaxTemplates", "mtpPotTemplates" } }
};

QString XtalOptCLIOptions::xtaloptHeaderString()
{
  QString out = QString("\n====================================================\n")
              + QString("   XtalOpt Multi-Objective Evolutionary Algorithm   \n")
              + QString("\n Version %1").arg(XTALOPT_VER)
              + QString("\n Zurek Group, University at Buffalo")
              + QString("\n====================================================\n\n");
  return out;
}

bool XtalOptCLIOptions::isKeyword(const QString& s, QString& csString)
{
  size_t ind = keywords.indexOf(QRegExp(s, Qt::CaseInsensitive));
  if (ind == -1)
    return false;
  csString = keywords[ind];
  return true;
}

bool XtalOptCLIOptions::isPotFile(const QString& s)
{
  if (s.trimmed().startsWith("potcarfile") ||
      s.trimmed().startsWith("psffile")) {
    return true;
  }

  return false;
}

bool XtalOptCLIOptions::isMolecularUnitsLine(const QString& s)
{
  if (s.trimmed().startsWith("molecularunits"))
    return true;

  return false;
}

bool XtalOptCLIOptions::isCustomIADLine(const QString& s)
{
  if (s.trimmed().startsWith("customiad"))
    return true;

  return false;
}

bool XtalOptCLIOptions::isMultiLineEntry(const QString& s)
{
  if (isPotFile(s) || isMolecularUnitsLine(s) || isCustomIADLine(s))
    return true;
  return false;
}

void XtalOptCLIOptions::processLine(const QString& tmpLine,
                                    QHash<QString, QString>& options, XtalOpt& xtalopt)
{
  QString line = tmpLine.trimmed();

  // Remove everything to the right of '#' (including '#') since it is a comment
  line.remove(QRegExp(" *#.*"));
  // Simplify 'space' characters (e.g., prevent issues in reading 'potcar element')
  //line.replace(QRegExp("\\s+"), " ");
  line = line.simplified();

  if (line.isEmpty())
    return;

  // We might have additional "=" signs in the value (e.g., arguments
  //   of the local run command). So, we split the input based on the
  //   "leftmost '=' sign", to obtain the key and value.

  // Get the key and the value
  QString key = line.section('=', 0, 0).trimmed().toLower();
  QString value = line.section('=', 1).trimmed();

  if (key.isEmpty()) {
    qDebug() << "Warning: invalid line '" << tmpLine
             << "' was read in options file.";
    return;
  }

  if (value.isEmpty()) {
    qDebug() << "Warning: invalid line '" << tmpLine
             << "' was read in options file.";
    return;
  }

  // Case sensitive key
  QString csKey = key;
  if (!isKeyword(key, csKey) && !isMultiLineEntry(key)) {
    qDebug() << "Warning: ignoring unrecognized option in line '" << tmpLine
             << "'";
    return;
  }

  // Multi-objective related entries are treated separately. The reason is that
  //   there might be multiple of these entries and each have multiple fields.
  // So, we won't assign actual variables here. Rather, add them all to a list
  //   to process them later on.
  if (csKey == "objective" )
    objectives_input.append(value);
  else
    options[csKey] = value;
}

bool XtalOptCLIOptions::requiredOptionsSet(
  const QHash<QString, QString>& options)
{
  // Do we have all the required keywords?
  for (const auto& requiredKeyword : requiredKeywords) {
    if (options[requiredKeyword].isEmpty()) {
      qDebug() << "Error: required option, '" << requiredKeyword
               << "', was not set in the options file.\n"
               << "Required options for every run are: " << requiredKeywords;
      return false;
    }
  }

  // Make sure that the queue interface is valid
  if (!validQueueInterfaces.contains(options["queueInterface"].toLower())) {
    qDebug() << "Error: unrecognized queue interface, '"
             << options["queueInterface"] << "', was entered.\n"
             << "Valid queue interfaces are: " << validQueueInterfaces;
    return false;
  }

  // Make sure that the optimizer is valid
  if (!validOptimizers.contains(options["optimizer"].toLower())) {
    qDebug() << "Error: unrecognized optimizer, '" << options["optimizer"]
             << "', was entered.\n"
             << "Valid optimizers are: " << validOptimizers;
    return false;
  }

#ifdef ENABLE_SSH
  // If we are local queue or remote, several entries are required
  if (toBool(options["localQueue"]) && options["queueInterface"].toLower() != "local") {
    for (const auto& requiredKeyword : requiredLocalQueueKeywords) {
      if (options[requiredKeyword].isEmpty()) {
        qDebug() << "Error: required option for local queue interfaces, '"
                 << requiredKeyword << "', was not set in the options file.\n"
                 << "Required options for local queue interfaces are: "
                 << requiredLocalQueueKeywords;
        return false;
      }
    }
  } else if (options["queueInterface"].toLower() != "local") {
    for (const auto& requiredKeyword : requiredRemoteKeywords) {
      if (options[requiredKeyword].isEmpty()) {
        qDebug() << "Error: required option for remote queue interfaces, '"
                 << requiredKeyword << "', was not set in the options file.\n"
                 << "Required options for remote queue interfaces are: "
                 << requiredRemoteKeywords;
        return false;
      }
    }
  }
#endif

  // Make sure we have the required templates for whichever optimizer we are
  // using
  const QStringList& requiredOptKeys =
    requiredOptimizerKeywords[options["optimizer"].toLower()];

  for (const auto& requiredKeyword : requiredOptKeys) {
    if (options[requiredKeyword].isEmpty()) {
      qDebug() << "Error: required option for" << options["optimizer"] << ",'"
               << requiredKeyword << "', was not set in the options file.\n"
               << "Required options for" << options["optimizer"]
               << "are: " << requiredOptKeys;
      return false;
    }
  }

  // Everything that was required was set!
  return true;
}

bool XtalOptCLIOptions::processOptions(const QHash<QString, QString>& options,
                                       XtalOpt& xtalopt)
{
  // Process the input parameters for XtalOpt run. Basically, the first thing
  //   to process and initialize is the chemical formula (as it is needed to
  //   initialize elemental volumes and reference energies). However, since
  //   construction of composition object needs the knowledge of radii, we
  //   start by reading them.

  // Start by figuring out if the user wants a verbose output?
  xtalopt.m_verbose = toBool(options.value("verboseOutput", "false"));

  // First, let's set the radii options so we can use them in
  // processing the input compositions.
  xtalopt.scaleFactor = options.value("radiiScalingFactor", "0.5").toFloat();
  xtalopt.minRadius = options.value("minRadius", "0.25").toFloat();

  // Now process the initial chemical formulae list.
  xtalopt.input_formulas_string = options["chemicalFormulas"];
  if (!xtalopt.processInputChemicalFormulas(xtalopt.input_formulas_string)) {
    qDebug() << "Error: input compositions were not read in successfully!";
    return false;
  } else {

    if (xtalopt.m_verbose) {
      QString outstr = "   Final list of input compositions:\n";
      for (int i = 0; i < xtalopt.compList.size(); i++) {
        outstr += QString("%1").arg(xtalopt.compList[i].getFormula(), 20);
        if ((i+1) % 3 == 0)
          outstr += "\n";
      }
      outstr += "\n\n";
      outstr += "   Chemical System: " + xtalopt.getChemicalSystem().join(" ");
      outstr += "\n\n";
      outstr += "   Initial atomic min radii: \n";
      for (const auto& el : xtalopt.eleMinRadii.getAtomicNumbers())
        outstr += QString("      %1 : %2\n").arg(el).arg(xtalopt.eleMinRadii.getMinRadius(el));
      qDebug().noquote() << outstr;
    }

  }

  // Is this a variable-composition search?
  xtalopt.vcSearch = toBool(options.value("vcSearch", "false"));

  // Should we save hull snapshots?
  xtalopt.m_saveHullSnapshots = toBool(options.value("saveHullSnapshots", "false"));

  // Read maximum number of atoms.
  xtalopt.maxAtoms = options.value("maxAtoms", "20").toInt();
  // Increase "maxAtoms" if it's smaller than the largest initial cell.
  int maximum_atoms_in_compositions = 0;
  for (int i = 0; i < xtalopt.compList.size(); i++) {
    if (xtalopt.compList[i].getNumAtoms() > maximum_atoms_in_compositions)
      maximum_atoms_in_compositions = xtalopt.compList[i].getNumAtoms();
  }
  if (maximum_atoms_in_compositions > xtalopt.maxAtoms) {
    qDebug() << "\nWarning: maximum atom count in formulas larger"
             << "than maxAtoms; resetting it to "
             << maximum_atoms_in_compositions;
    xtalopt.maxAtoms = maximum_atoms_in_compositions;
  }

  // Process the seed structures list input.
  QStringList sl =
      options.value("seedStructures", "").split(",", QString::SkipEmptyParts);
  for (int i = 0; i < sl.size(); i++) {
    if (!sl[i].simplified().isEmpty())
    xtalopt.seedList.append(sl[i].simplified());
  }

  // Process the reference energies (for convex hull formation energy).
  // If user provides "non-empty" reference entries, it is processed here and
  //   the default values are set inside this function.
  // If the input is empty, this function initiates a list of "zero" energy
  //   for elements only; as the default values.
  // If this function returns false, it means user has entered "something", but
  //   we couldn't read it! True means input was empty or was read successfully.
  // Appropriate error messages will be printed inside the function.
  //
  // Now read the input list (if any).
  xtalopt.input_ene_refs_string = options.value("referenceEnergies", "");
  if (!xtalopt.processInputReferenceEnergies(xtalopt.input_ene_refs_string)) {
    return false;
  } else {
    if (xtalopt.m_verbose) {
      QString outstr = "\n";
      for(int i = 0; i < xtalopt.refEnergies.size(); i++) {
        outstr += QString("   Reference energy %1 : %2\n")
                      .arg(xtalopt.refEnergies[i].cell.getFormula(), 10)
                      .arg(xtalopt.refEnergies[i].energy, 12, 'f', 6);
      }
      qDebug().noquote() << outstr;
    }
  }

  // Process the volume limits.
  // Volume limits are used in the code in the following order:
  //   (1) if elemental volumes are given, they are used,
  //   (2) if scaled volumes are given, then use them,
  //   (3) otherwise, use the pair of vol_min/vol_max.
  //
  // Start with the absolute volume limits. Since these are
  //  the "base" values to be used if other "optional" volume
  //  schemes are not given, we return false with any error here.
  //  So, we make sure user knows what they're doing.
  //
  xtalopt.vol_min = options.value("minVolume", "1.0").toFloat();
  xtalopt.vol_max = options.value("maxVolume", "100.0").toFloat();
  if (xtalopt.vol_min < ZERO6 || xtalopt.vol_max < ZERO6) {
    qDebug() << "Error: input values for volume min and max limit "
                "can't be negative!";
    return false;
  }
  if (xtalopt.vol_max < xtalopt.vol_min) {
    qDebug() << "Error: max volume limit is smaller than the min "
                " volume limit!";
    return false;
  }
  // Now, process the elemental volumes (if any)
  // If they are properly given, the "element_vol" variable will be initiated.
  // Elemental volumes are used if "given for all elements, min > 0, and max >= min"
  xtalopt.input_ele_volm_string = options.value("elementalVolumes", "");
  if (!xtalopt.processInputElementalVolumes(xtalopt.input_ele_volm_string)) {
    qDebug() << "\nWarning: Ignoring elemental volume input.";
  } else {
    if (xtalopt.m_verbose) {
      QString outstr = "\n";
      for (const auto& atomcn : xtalopt.eleVolumes.getAtomicNumbers()) {
        outstr += QString("   Elemental volume %1 : %2 %3\n")
                      .arg(ElementInfo::getAtomicSymbol(atomcn).c_str(), 10)
                      .arg(xtalopt.eleVolumes.getMinVolume(atomcn), 12, 'f', 6)
                      .arg(xtalopt.eleVolumes.getMaxVolume(atomcn), 12, 'f', 6);
      }
      qDebug().noquote() << outstr;
    }
  }
  // Finally, check for scaled volumes (if any)
  // These will be used only if "min > 0 and max >= min"
  xtalopt.vol_scale_min = options.value("minVolumeScale", "0.0").toFloat();
  xtalopt.vol_scale_max = options.value("maxVolumeScale", "0.0").toFloat();

  // We put default values in all of these
  // Other initialization settings
  xtalopt.a_min = options.value("aMin", "3.0").toFloat();
  xtalopt.b_min = options.value("bMin", "3.0").toFloat();
  xtalopt.c_min = options.value("cMin", "3.0").toFloat();
  xtalopt.a_max = options.value("aMax", "10.0").toFloat();
  xtalopt.b_max = options.value("bMax", "10.0").toFloat();
  xtalopt.c_max = options.value("cMax", "10.0").toFloat();
  xtalopt.alpha_min = options.value("alphaMin", "60.0").toFloat();
  xtalopt.beta_min = options.value("betaMin", "60.0").toFloat();
  xtalopt.gamma_min = options.value("gammaMin", "60.0").toFloat();
  xtalopt.alpha_max = options.value("alphaMax", "120.0").toFloat();
  xtalopt.beta_max = options.value("betaMax", "120.0").toFloat();
  xtalopt.gamma_max = options.value("gammaMax", "120.0").toFloat();

  xtalopt.using_interatomicDistanceLimit =
    toBool(options.value("usingRadiiInteratomicDistanceLimit", "true"));
  xtalopt.using_customIAD = toBool(options.value("usingCustomIADs", "false"));

  if (xtalopt.using_interatomicDistanceLimit && xtalopt.using_customIAD) {
    qDebug() << "Error: usingRadiiInteratomicDistanceLimit (default is true)"
             << "and usingCustomIADs (default is false) cannot both be set to"
             << "true.\nPlease set one to false and try again";
    return false;
  }

  xtalopt.using_checkStepOpt =
    toBool(options.value("checkIADPostOptimization", "false"));

  xtalopt.using_molUnit = toBool(options.value("usingMolecularUnits", "false"));

  if (xtalopt.using_molUnit && !processMolUnits(options, xtalopt)) {
    qDebug() << "Error: Invalid settings entered for molecular units."
             << "Please check your input and try again.";
    return false;
  }

  if (xtalopt.using_customIAD && !processCustomIADs(options, xtalopt)) {
    qDebug() << "Error: Invalid settings entered for custom IADs."
             << "Please check your input and try again.";
    return false;
  }

  xtalopt.using_randSpg = toBool(options.value("usingRandSpg", "false"));
  if (xtalopt.using_randSpg) {
    // Create the list of space groups for generation
    for (int spg = 1; spg <= 230; spg++)
      xtalopt.minXtalsOfSpg.append(0);

    QStringList list = toList(options.value("forcedSpgsWithRandSpg", ""));
    for (const auto& item : qAsConst(list)) {
      int numhyphens = item.count(QLatin1Char('-'));
      if (numhyphens == 0) {
        unsigned int num = item.toUInt();
        if (num != 0 && num <= 230)
          ++xtalopt.minXtalsOfSpg[num - 1];
      } else if (numhyphens == 1) {
        QStringList num_list = item.split("-", QString::SkipEmptyParts);
        bool ok1, ok2;
        unsigned int min_num = num_list[0].toUInt(&ok1);
        unsigned int max_num = num_list[1].toUInt(&ok2);
        if (!ok1 || !ok2)
          continue;
        for (unsigned int num = min_num; num <= max_num; num++) {
          if (num != 0 && num <= 230)
            ++xtalopt.minXtalsOfSpg[num - 1];
        }
      }
    }
  }

  if (xtalopt.using_randSpg && xtalopt.using_molUnit) {
    qDebug() << "Error: randSpg cannot be used with molUnit."
             << " Please turn this off if you wish to "
             << "use randSpg.";
    return false;
  }

  // Search settings
  xtalopt.numInitial = options.value("numInitial", "0").toUInt();
  xtalopt.parentsPoolSize = options.value("parentsPoolSize", "20").toUInt();
  xtalopt.limitRunningJobs = toBool(options.value("limitRunningJobs", "true"));
  xtalopt.runningJobLimit = options.value("runningJobLimit", "1").toUInt();
  xtalopt.contStructs = options.value("continuousStructures", "15").toUInt();
  xtalopt.failLimit = options.value("jobFailLimit", "1").toUInt();
  if (xtalopt.failLimit < 1)
    xtalopt.failLimit = 1;

  QString failAction = options.value("jobFailAction", "replaceWithRandom");
  if (failAction.toLower() == "keeptrying")
    xtalopt.failAction = SearchBase::FA_DoNothing;
  else if (failAction.toLower() == "kill")
    xtalopt.failAction = SearchBase::FA_KillIt;
  else if (failAction.toLower() == "replacewithrandom")
    xtalopt.failAction = SearchBase::FA_Randomize;
  else if (failAction.toLower() == "replacewithoffspring")
    xtalopt.failAction = SearchBase::FA_NewOffspring;
  else {
    qDebug() << "\nWarning: unrecognized jobFailAction: " << failAction;
    qDebug() << "Using default option: replaceWithRandom";
    xtalopt.failAction = SearchBase::FA_Randomize;
  }

  xtalopt.maxNumStructures = options.value("maxNumStructures", "100").toUInt();
  xtalopt.m_softExit = toBool(options.value("softExit", "false"));
  xtalopt.m_localQueue = toBool(options.value("localQueue", "false"));

  // Multi-Objective related entries.
  // Process the optimization objectives (and weights) for energy/objectives.
  // For this process, objectives list will be used that contains all objective
  //   related input provided by the user.
  if (objectives_input.size() > 0) {
    for (int i = 0; i < objectives_input.size(); i++)
      if (!xtalopt.processInputObjectives(objectives_input[i]))
        return false;
  }
  xtalopt.m_objectivesReDo = toBool(options.value("objectivesReDo", "false"));

  // Optimization type: Basic (scalar fitness), Pareto
  xtalopt.m_optimizationType = options.value("optimizationType", "basic").toLower();
  // Tournament selection (applies only to Pareto optimization)
  xtalopt.m_tournamentSelection = toBool(options.value("tournamentSelection", "true"));
  // Restrict the pool size in tournament selection
  xtalopt.m_restrictedPool = toBool(options.value("restrictedPool", "false"));
  // Crowding distance
  xtalopt.m_crowdingDistance = toBool(options.value("crowdingDistance", "true"));
  // Objectives' value precision
  xtalopt.m_objectivePrecision = options.value("objectivePrecision", "-1").toInt();

  // Mutators
  xtalopt.p_cross = options.value("weightCrossover", "35").toUInt();
  xtalopt.p_strip = options.value("weightStripple", "25").toUInt();
  xtalopt.p_perm = options.value("weightPermustrain", "25").toUInt();
  xtalopt.p_atomic = options.value("weightPermutomic", "15").toUInt();
  xtalopt.p_comp = options.value("weightPermucomp", "5").toUInt();
  xtalopt.p_supercell = options.value("randomSuperCell", "0").toUInt();

  // Stripple options
  xtalopt.strip_amp_min =
    options.value("strippleAmplitudeMin", "0.5").toFloat();
  xtalopt.strip_amp_max =
    options.value("strippleAmplitudeMax", "1.0").toFloat();
  xtalopt.strip_per1 = options.value("strippleNumWavesAxis1", "1").toUInt();
  xtalopt.strip_per2 = options.value("strippleNumWavesAxis2", "1").toUInt();
  xtalopt.strip_strainStdev_min =
    options.value("strippleStrainStdevMin", "0.5").toFloat();
  xtalopt.strip_strainStdev_max =
    options.value("strippleStrainStdevMax", "0.5").toFloat();

  // Permustrain options
  xtalopt.perm_ex = options.value("permustrainNumExchanges", "4").toUInt();
  xtalopt.perm_strainStdev_max =
    options.value("permustrainStrainStdevMax", "0.5").toFloat();

  // Crossover options
  xtalopt.cross_ncuts  = options.value("crossoverCuts", "1").toUInt();
  xtalopt.cross_minimumContribution =
    options.value("crossoverMinContribution", "25").toUInt();

  // RDF similarity check
  xtalopt.tol_rdf = options.value("rdfTolerance", "0.0").toDouble();
  xtalopt.tol_rdf_cutoff = options.value("rdfCutoff", "6.0").toDouble();
  xtalopt.tol_rdf_nbins = options.value("rdfNumBins", "3000").toInt();
  xtalopt.tol_rdf_sigma = options.value("rdfSigma", "0.008").toDouble();

  // XtalComp similarity check
  xtalopt.tol_xcLength =
    options.value("xtalcompToleranceLength", "0.1").toFloat();
  xtalopt.tol_xcAngle =
    options.value("xtalcompToleranceAngle", "2.0").toFloat();

  // Spglib tolerance
  //   (for consistency; default "tolerances" are defined in constants.h)
  xtalopt.tol_spg = options.value("spglibTolerance", QString::number(SPGTOLDEF)).toFloat();

  // FIXME: We will use this later
  QString templatesDir = options.value("templatesDirectory", ".");

  // Read the "custom" user input entries
  xtalopt.setUser1(options.value("user1", "").toStdString());
  xtalopt.setUser2(options.value("user2", "").toStdString());
  xtalopt.setUser3(options.value("user3", "").toStdString());
  xtalopt.setUser4(options.value("user4", "").toStdString());

  // Are there any queue interfaces that are remote?
  bool anyRemote = false;

  size_t numOptSteps = options.value("numOptimizationSteps", "1").toUInt();

  xtalopt.clearOptSteps();

  for (size_t i = 0; i < numOptSteps; ++i) {

    xtalopt.appendOptStep();

#ifdef ENABLE_SSH
    xtalopt.setQueueInterface(
      i, options["queueInterface"].toLower().toStdString());
#else
    if (options["queueInterface"].toLower() != "local") {
      qDebug() << "Error: SSH is disabled, so only 'local' interface is"
               << "allowed.";
      qDebug() << "Please use the option 'queueInterface <optStep> = local'";
      return false;
    }
    xtalopt.setQueueInterface(i, "local");
#endif

#ifdef ENABLE_SSH

    bool remote = (options["queueInterface"].toLower() != "local");

    // We have additional things to set if we are remote
    if (remote) {
      anyRemote = true;
      RemoteQueueInterface* remoteQueue =
        qobject_cast<RemoteQueueInterface*>(xtalopt.queueInterface(i));

      if (!options["submitCommand"].isEmpty())
        remoteQueue->setSubmitCommand(options["submitCommand"]);
      if (!options["cancelCommand"].isEmpty())
        remoteQueue->setCancelCommand(options["cancelCommand"]);
      if (!options["statusCommand"].isEmpty())
        remoteQueue->setStatusCommand(options["statusCommand"]);
    }
#endif

    QString optimizerName = options["optimizer"].toLower();
    QString queueName = xtalopt.queueInterface(i)->getIDString().toLower();
    xtalopt.setOptimizer(i, optimizerName.toStdString());
    XtalOptOptimizer* optimizer =
      static_cast<XtalOptOptimizer*>(xtalopt.optimizer(i));
    if (optimizerName == "castep") {
      if (!addOptimizerTemplate(xtalopt, "castepCellTemplates",
                                queueName, i, options)) {
        return false;
      }

      if (!addOptimizerTemplate(xtalopt, "castepParamTemplates",
                                queueName, i, options)) {
        return false;
      }
    } else if (options["optimizer"].toLower() == "gulp") {
      if (!addOptimizerTemplate(xtalopt, "ginTemplates", queueName, i,
                                options)) {
        return false;
      }
    } else if (options["optimizer"].toLower() == "pwscf") {
      if (!addOptimizerTemplate(xtalopt, "pwscfTemplates", queueName,
                                i, options)) {
        return false;
      }
    } else if (options["optimizer"].toLower() == "mtp") {
      if (!addOptimizerTemplate(xtalopt, "mtpCellTemplates", queueName,
                                i, options)) {
        return false;
      }
      if (!addOptimizerTemplate(xtalopt, "mtpRelaxTemplates", queueName,
                                i, options)) {
        return false;
      }
      if (!addOptimizerTemplate(xtalopt, "mtpPotTemplates", queueName,
                                i, options)) {
        return false;
      }
    } else if (options["optimizer"].toLower() == "siesta") {
      if (!addOptimizerTemplate(xtalopt, "fdfTemplates", queueName, i,
                                options)) {
        return false;
      }

      // Generating the PSF files require a little bit more work...
      // Generate list of symbols
      QVariantList psfInfo;
      QStringList symbols;
      QList<uint> atomicNums = xtalopt.compList[0].getAtomicNumbers();
      std::sort(atomicNums.begin(), atomicNums.end());

      for (const auto& atomicNum : atomicNums) {
        if (atomicNum != 0)
          symbols.append(ElementInfo::getAtomicSymbol(atomicNum).c_str());
      }
      std::sort(symbols.begin(), symbols.end());
      QVariantHash hash;
      for (const auto& symbol : symbols) {
        QString filename =
          options["psffile " + symbol.toLower()];
        if (filename.isEmpty()) {
          qDebug() << "Error: no PSF file found for atom type" << symbol;
          qDebug() << "You must set the PSF file in the options like so:";
          QString tmp = "psfFile " + symbol +
                        " = /path/to/siesta_psfs/symbol.psf";
          qDebug() << tmp;
          return false;
        }

        // Check if psf file exists
        QFileInfo check_file(filename);
        if (!check_file.exists() || !check_file.isFile()) {
          qDebug() << "Error: the PSF file for atom type" << symbol
                   << "was not found at " << filename;
          return false;
        }

        hash.insert(symbol, QVariant(filename));
      }

      psfInfo.append(QVariant(hash));

      // Set composition in optimizer
      QVariantList toOpt;
      for (const auto& atomicNum : atomicNums)
        toOpt.append(atomicNum);

      optimizer->setData("Composition", toOpt);

      // Set PSF info
      optimizer->setData("PSF info", QVariant(psfInfo));
    } else if (options["optimizer"].toLower() == "vasp") {
      if (!addOptimizerTemplate(xtalopt, "incarTemplates", queueName,
                                i, options)) {
        return false;
      }

      if (!addOptimizerTemplate(xtalopt, "kpointsTemplates",
                                queueName, i, options)) {
        return false;
      }

      // Generating the POTCAR file requires a little bit more work...
      // Generate list of symbols
      QVariantList potcarInfo;
      QStringList symbols;
      QList<uint> atomicNums = xtalopt.compList[0].getAtomicNumbers();
      std::sort(atomicNums.begin(), atomicNums.end());

      for (const auto& atomicNum : atomicNums) {
        if (atomicNum != 0)
          symbols.append(ElementInfo::getAtomicSymbol(atomicNum).c_str());
      }
      std::sort(symbols.begin(), symbols.end());
      QVariantHash hash;
      std::string potcarStr;

      // We first check if a "single" POTCAR for the "system" is given
      //   by the user as "potcarfile system" in the setup file!
      // If so, then single potcarfile entries for the elements are
      //   ignored and this single file will be used "as is".
      // If not, then a traditional element-by-element check is performed
      //   to see all required POTCAR files are present.
      QString filename =
        options["potcarfile system"];
      if (!filename.isEmpty()) {
        // Check if potcar file exists
        QFileInfo check_file(filename);
        if (!check_file.exists() || !check_file.isFile()) {
          qDebug() << "Error: the POTCAR file for the system"
            << "was not found at " << filename;
          return false;
        }
        hash.insert("system", QVariant(filename));
        potcarStr += "%fileContents:" + filename.toStdString() + "%\n";
      } else {
        for (const auto& symbol : symbols) {
          filename =
            options["potcarfile " + symbol.toLower()];
          if (filename.isEmpty()) {
            qDebug() << "Error: no POTCAR file found for atom type" << symbol;
            qDebug() << "You must set the POTCAR file in the options like so:";
            QString tmp = "potcarFile " + symbol +
              " = /path/to/vasp_potcars/symbol/POTCAR";
            qDebug() << tmp;
            return false;
          }
          // Check if potcar file exists
          QFileInfo check_file(filename);
          if (!check_file.exists() || !check_file.isFile()) {
            qDebug() << "Error: the POTCAR file for atom type" << symbol
              << "was not found at " << filename;
            return false;
          }
          hash.insert(symbol, QVariant(filename));
          potcarStr += "%fileContents:" + filename.toStdString() + "%\n";
        }
      }

      potcarInfo.append(QVariant(hash));

      xtalopt.setTemplate(i, "POTCAR", potcarStr);

      // Set composition in optimizer
      QVariantList toOpt;
      for (const auto& atomicNum : atomicNums)
        toOpt.append(atomicNum);

      optimizer->setData("Composition", toOpt);

      // Set POTCAR info
      optimizer->setData("POTCAR info", QVariant(potcarInfo));
    } else {
      qDebug() << "Error: unknown optimizer:" << options["optimizer"];
      return false;
    }

#ifdef ENABLE_SSH
    if (remote) {
      // We need to add the job templates if we are remote
      if (!addOptimizerTemplate(xtalopt, "jobTemplates", queueName, i,
                                options)) {
        return false;
      }
    }
#endif

    // This will only get used if we are local
    if (!options["exeLocation"].isEmpty())
      optimizer->setLocalRunCommand(options["exeLocation"]);
  }

  xtalopt.locWorkDir =
    options.value("localWorkingDirectory", "localWorkingDirectory");
  // Make relative paths become absolute paths
  xtalopt.locWorkDir = QDir(xtalopt.locWorkDir).absolutePath();

  xtalopt.m_logErrorDirs =
    toBool(options.value("logErrorDirectories", "false"));

  xtalopt.m_cancelJobAfterTime =
    toBool(options.value("autoCancelJobAfterTime", "false"));

  if (xtalopt.m_cancelJobAfterTime) {
    xtalopt.m_hoursForCancelJobAfterTime =
      options.value("hoursForAutoCancelJob", "100.0").toDouble();
  }

  if (anyRemote) {
    xtalopt.setQueueRefreshInterval(
      options.value("queueRefreshInterval", "10").toUInt());
    xtalopt.setCleanRemoteOnStop(
      toBool(options.value("cleanRemoteDirs", "false")));
    xtalopt.host = options["host"];
    xtalopt.port = options.value("port", "22").toUInt();
    xtalopt.username = options["user"];
    xtalopt.remWorkDir = options["remoteWorkingDirectory"];
  }

  return true;
}

bool XtalOptCLIOptions::printOptions(const QHash<QString, QString>& options,
                                     XtalOpt& xtalopt)
{
  QStringList keys = options.keys();
  std::sort(keys.begin(), keys.end());

  QString output;
  QTextStream stream(&output);

  // Former: Manually set options
  stream << "\n=== Manually Set Options\n\n";
  for (const auto& key : keys)
    stream << key << " = " << options[key] << "\n";

  // Former: All options
  stream << "\n=== All Run Options\n";
  xtalopt.printOptionSettings(stream, &xtalopt);

  // We need to convert to c string to properly print newlines
  qDebug() << output.toUtf8().data();

  // Check if xtalopt data is already saved at the local working directory.
  // Up to r12, xtalopt would ask the user if they want the code to clean up the folder.
  // But there were cases which user had run data stored in an important directory
  //   such as Desktop/ or Documents/, and then mistakenly let the code to clean up which
  //   results in permanent deletion of data!
  // So, now we only let the user know that the directory is not empty and quit (in the
  //   GUI there is dialog for this).
  if (QFile::exists(xtalopt.locWorkDir + QDir::separator() + "xtalopt.state")) {
    QString msg = QString("Error: XtalOpt data is already saved at:\n") +
                          xtalopt.locWorkDir +
                          "\n\nEmpty the directory to proceed or "
                          "select a new 'Local working directory'!"
                          "\n\nQuitting now ...";
    qDebug().noquote() << msg;
    return false;
  }
  // Make the directory if it doesn't exist...
  if (!QFile::exists(xtalopt.locWorkDir))
    QDir().mkpath(xtalopt.locWorkDir);

  // Try to write to a log file also
  QFile file(xtalopt.locWorkDir + QDir::separator() + "settings.log");
  if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    file.write(output.toStdString().c_str());

  return true;
}

bool XtalOptCLIOptions::readOptions_(const QString& filename, XtalOpt& x)
{
  // This is a slightly-modified version of the below function; to be
  //   used in reading a CLI input file to populate the GUI entries.
  // It does not produce output files by its own, and we will try
  //   to read the stuff as much as we can (not insisting on having
  //   a prefectly OK input file). If the file is not OK, we return
  //   false, which eventually prompts a warning message in the GUI.

  // Attempt to open the file
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "Error: could not open file '" << filename << "'.";
    return false;
  }

  // The options QHash
  QHash<QString, QString> options;

  bool results = true;

  // Read the file line-by-line
  while (!file.atEnd()) {
    QString line = file.readLine();
    processLine(line, options, x);
  }

  // Check to make sure all required options were set
  if (!requiredOptionsSet(options))
    results = false;

  // Process the options
  if (!processOptions(options, x))
    results = false;

  return results;

}

bool XtalOptCLIOptions::readOptions(const QString& filename, XtalOpt& xtalopt)
{
  // Start by printing the program header
  qDebug().noquote() << xtaloptHeaderString();

  // Attempt to open the file
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "Error: could not open file '" << filename << "'.";
    return false;
  }

  // The options QHash
  QHash<QString, QString> options;

  // Read the file line-by-line
  while (!file.atEnd()) {
    QString line = file.readLine();
    processLine(line, options, xtalopt);
  }

  // Check to make sure all required options were set
  if (!requiredOptionsSet(options))
    return false;

  // Process the options
  if (!processOptions(options, xtalopt))
    return false;

  // Print out all options to the terminal and a file
  if (!printOptions(options, xtalopt))
    return false;

  // Write the initial run-time file in the local working directory
  writeInitialRuntimeFile(xtalopt);

  return true;
}

QString XtalOptCLIOptions::fromBool(bool b)
{
  if (b)
    return "true";
  return "false";
}

bool XtalOptCLIOptions::toBool(const QString& s)
{
  QString qs = s.trimmed();
  // Should be true if it begins with 't' or it is any number other than zero
  if (qs.startsWith("t", Qt::CaseInsensitive) || qs.toInt() != 0)
    return true;
  return false;
}

QStringList XtalOptCLIOptions::toList(const QString& s)
{
  QStringList qsl = s.trimmed().split(",", QString::SkipEmptyParts);
  // Trim every QString in the list
  std::for_each(qsl.begin(), qsl.end(), [](QString& qs) { qs = qs.trimmed(); });
  return qsl;
}

// Convert the template names to the name used by the optimizer
QString convertedTemplateName(const QString& s, const QString& queueName)
{
  if (s.compare("castepCellTemplates", Qt::CaseInsensitive) == 0)
    return "xtal.cell";
  if (s.compare("castepParamTemplates", Qt::CaseInsensitive) == 0)
    return "xtal.param";
  if (s.compare("ginTemplates", Qt::CaseInsensitive) == 0)
    return "xtal.gin";
  if (s.compare("pwscfTemplates", Qt::CaseInsensitive) == 0)
    return "xtal.in";
  if (s.compare("fdfTemplates", Qt::CaseInsensitive) == 0)
    return "xtal.fdf";
  if (s.compare("incarTemplates", Qt::CaseInsensitive) == 0)
    return "INCAR";
  if (s.compare("kpointsTemplates", Qt::CaseInsensitive) == 0)
    return "KPOINTS";
  if (s.compare("mtpCellTemplates", Qt::CaseInsensitive) == 0)
    return "mtp.cell";
  if (s.compare("mtpRelaxTemplates", Qt::CaseInsensitive) == 0)
    return "mtp.relax";
  if (s.compare("mtpPotTemplates", Qt::CaseInsensitive) == 0)
    return "mtp.pot";

  if (s.compare("jobTemplates", Qt::CaseInsensitive) == 0) {
    if (queueName.compare("LoadLeveler", Qt::CaseInsensitive) == 0)
      return "job.ll";
    else if (queueName.compare("LSF", Qt::CaseInsensitive) == 0)
      return "job.lsf";
    else if (queueName.compare("PBS", Qt::CaseInsensitive) == 0)
      return "job.pbs";
    else if (queueName.compare("SGE", Qt::CaseInsensitive) == 0)
      return "job.sh";
    else if (queueName.compare("SLURM", Qt::CaseInsensitive) == 0)
      return "job.slurm";
    qDebug() << "Unknown queue id:" << queueName;
    return "job.sh";
  }

  qDebug() << "Unknown template name:" << s;
  return "";
}

bool XtalOptCLIOptions::addOptimizerTemplate(
  XtalOpt& xtalopt, const QString& templateName, const QString& queueName,
  size_t optStep, const QHash<QString, QString>& options)
{
  QString optStepStr = QString::number(optStep + 1);
  QStringList fileList = options[templateName].split(",");

  if (fileList.size() <= optStep) {
    qDebug() << "Error in" << __FUNCTION__ << ": " << templateName
             << "does not contain a template for opt step " << optStepStr;
    return false;
  }

  QString filename = fileList[optStep].trimmed();
  if (filename.isEmpty()) {
    qDebug() << "Error in" << __FUNCTION__ << ": " << templateName
             << "is missing!";
    return false;
  }

  // Now we have to open the files and read their contents
  QFile file(options.value("templatesDirectory", ".") + "/" + filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "Error: could not open file '" << filename << "' in"
             << "the templates directory: "
             << options.value("templatesDirectory", ".");
    return false;
  }
  QString text = file.readAll();
  xtalopt.setTemplate(
    optStep, convertedTemplateName(templateName, queueName).toStdString(),
    text.toStdString());

  return true;
}

// This was copied and pasted directly from TabInit::setGeom, except
// that they are all lower-case for case-insensitivity...
void setGeom(unsigned int& geom, const QString& strGeom)
{
  // Two neighbors
  if (strGeom.contains("linear")) {
    geom = 1;
  } else if (strGeom.contains("bent")) {
    geom = 2;
    // Three neighbors
  } else if (strGeom.contains("trigonal planar")) {
    geom = 2;
  } else if (strGeom.contains("trigonal pyramidal")) {
    geom = 3;
  } else if (strGeom.contains("t-shaped")) {
    geom = 4;
    // Four neighbors
  } else if (strGeom.contains("tetrahedral")) {
    geom = 3;
  } else if (strGeom.contains("see-saw")) {
    geom = 5;
  } else if (strGeom.contains("square planar")) {
    geom = 4;
    // Five neighbors
  } else if (strGeom.contains("trigonal bipyramidal")) {
    geom = 5;
  } else if (strGeom.contains("square pyramidal")) {
    geom = 6;
    // Six neighbors
  } else if (strGeom.contains("octahedral")) {
    geom = 6;
    // Default
  } else {
    geom = 0;
  }
}

bool XtalOptCLIOptions::processMolUnits(const QHash<QString, QString>& options,
                                        XtalOpt& xtalopt)
{
  QHash<QString, unsigned int> atomCounts;

  for (const auto& option : options.keys()) {
    // Find the molecular units lines
    if (option.trimmed().toLower().startsWith("molecularunits")) {
      QStringList splitLine =
        options[option].split(",", QString::SkipEmptyParts);
      if (splitLine.size() != 6) {
        qDebug() << "Error: molecularUnits line must have 6 comma-delimited"
                 << "items on the right-hand side of the equals sign."
                 << "\nFaulty option is as follows: "
                 << option + " = " + options[option];
        return false;
      }
      QString centerSymbol = splitLine[0].trimmed(),
              neighborSymbol = splitLine[2].trimmed(),
              geometry = splitLine[4].trimmed();
      size_t numCenters = splitLine[1].toUInt(),
             numNeighbors = splitLine[3].toUInt();
      double distance = splitLine[5].toDouble();

      // Make sure the data is valid
      unsigned short centerAtomicNum =
        ElementInfo::getAtomicNum(centerSymbol.toStdString());

      // If the centerSymbol == "None", then 0 is the correct number for it
      if (centerSymbol.toLower() != "none" && centerAtomicNum == 0) {
        qDebug() << "Error processing molecularUnits line:"
                 << "Invalid atomic symbol:" << centerSymbol;
        qDebug() << "Proper format is as follows: "
                 << "<centerSymbol>, <numCenters>, <neighborSymbol>,"
                 << "<numNeighbors>, <geometry>, <distances>";
        return false;
      }

      unsigned short neighborAtomicNum =
        ElementInfo::getAtomicNum(neighborSymbol.toStdString());

      if (neighborAtomicNum == 0) {
        qDebug() << "Error processing molecularUnits line:"
                 << "Invalid atomic symbol:" << neighborSymbol;
        qDebug() << "Proper format is as follows: "
                 << "<centerSymbol>, <numCenters>, <neighborSymbol>,"
                 << "<numNeighbors>, <geometry>, <distances>";
        return false;
      }

      if (numCenters == 0) {
        qDebug() << "Error processing molecularUnits line:"
                 << "numCenters cannot be zero.";
        return false;
      }

      switch (numNeighbors) {
        case 1:
          if (geometry.toLower() != "linear") {
            qDebug() << "Error reading molecularUnits:"
                     << "for numNeighbors ==" << numNeighbors << ", possible"
                     << "geometries are:"
                     << "linear.";
            return false;
          }
          break;
        case 2:
          if (geometry.toLower() != "linear" && geometry.toLower() != "bent") {
            qDebug() << "Error reading molecularUnits:"
                     << "for numNeighbors ==" << numNeighbors << ", possible"
                     << "geometries are:"
                     << "linear and bent.";
            return false;
          }
          break;
        case 3:
          if (geometry.toLower() != "trigonal planar" &&
              geometry.toLower() != "trigonal pyramidal" &&
              geometry.toLower() != "t-shaped") {
            qDebug() << "Error reading molecularUnits:"
                     << "for numNeighbors ==" << numNeighbors << ", possible"
                     << "geometries are:"
                     << "trigonal planar, trigonal pyramidal, and t-shaped.";
            return false;
          }
          break;
        case 4:
          if (geometry.toLower() != "tetrahedral" &&
              geometry.toLower() != "see-saw" &&
              geometry.toLower() != "square planar") {
            qDebug() << "Error reading molecularUnits:"
                     << "for numNeighbors ==" << numNeighbors << ", possible"
                     << "geometries are:"
                     << "tetrahedral, see-saw, and square planar.";
            return false;
          }
          break;
        case 5:
          if (geometry.toLower() != "trigonal bipyramidal" &&
              geometry.toLower() != "square pyramidal") {
            qDebug() << "Error reading molecularUnits:"
                     << "for numNeighbors ==" << numNeighbors << ", possible"
                     << "geometries are:"
                     << "trigonal bipyramidal and square pyramidal.";
            return false;
          }
          break;
        case 6:
          if (geometry.toLower() != "octahedral") {
            qDebug() << "Error reading molecularUnits:"
                     << "for numNeighbors ==" << numNeighbors << ", possible"
                     << "geometries are:"
                     << "octahedral.";
            return false;
          }
          break;
        default:
          qDebug() << "Error reading molecularUnits: invalid numNeighbors"
                   << "was entered. Valid numbers are 1 - 6";
          return false;
      }

      unsigned int geom = 0;
      setGeom(geom, geometry.toLower());

      // Now make the struct and add it to XtalOpt
      MolUnit entry;
      entry.numCenters = numCenters;
      entry.numNeighbors = numNeighbors;
      entry.geom = geom;
      entry.dist = distance;

      xtalopt.compMolUnit.insert(
        qMakePair<int, int>(centerAtomicNum, neighborAtomicNum), entry);

      atomCounts[centerSymbol.toLower()] += numCenters;
      atomCounts[neighborSymbol.toLower()] += (numNeighbors * numCenters);
    }
  }

  // Considering the case of multi-/variable-composition; we will compare
  //   molunits against the "minimum quantities in all input compositions"
  //   If they are appropriate, we will proceed. So we use "comp" variable.
  for (const auto& countKey : atomCounts.keys()) {
    if (xtalopt.getMinimalComposition().getCount(countKey) < atomCounts[countKey]) {
      QString elemName = countKey;
      if (!elemName.isEmpty())
        elemName[0] = elemName[0].toUpper();
      qDebug() << "Error reading molecularUnits: for atom" << elemName << ","
               << "there are more atoms predicted to be generated with"
               << "the molUnit settings than those allowed by the input compoisitions.";
      return false;
    }
  }

  return true;
}

bool XtalOptCLIOptions::processCustomIADs(
  const QHash<QString, QString>& options, XtalOpt& xtalopt)
{
  for (const auto& option : options.keys()) {
    // Find the custom IAD lines
    if (option.trimmed().toLower().startsWith("customiad")) {
      QStringList splitLine =
        options[option].split(",", QString::SkipEmptyParts);
      if (splitLine.size() != 3) {
        qDebug() << "Error: customIAD line must have 3 comma-delimited"
                 << "items on the right-hand side of the equals sign."
                 << "\nFaulty option is as follows: "
                 << option + " = " + options[option];
        qDebug() << "Proper format is as follows: "
                 << "<firstSymbol>, <secondSymbol>, <minDistance>";
        return false;
      }
      QString firstSymbol = splitLine[0].trimmed(),
              secondSymbol = splitLine[1].trimmed();
      double minDist = splitLine[2].toDouble();

      // Make sure the data is valid
      unsigned short firstAtomicNum =
        ElementInfo::getAtomicNum(firstSymbol.toStdString());

      // If the atomic number is 0, the symbol is invalid
      if (firstAtomicNum == 0) {
        qDebug() << "Error processing customIAD line:"
                 << "Invalid atomic symbol:" << firstSymbol;
        qDebug() << "Proper format is as follows: "
                 << "<firstSymbol>, <secondSymbol>, <minDistance>";
        return false;
      }

      unsigned short secondAtomicNum =
        ElementInfo::getAtomicNum(secondSymbol.toStdString());

      // If the atomic number is 0, the symbol is invalid
      if (secondAtomicNum == 0) {
        qDebug() << "Error processing customIAD line:"
                 << "Invalid atomic symbol:" << secondSymbol;
        qDebug() << "Proper format is as follows: "
                 << "<firstSymbol>, <secondSymbol>, <minDistance>";
        return false;
      }

      // Now make the struct and add it to XtalOpt
      IAD entry;
      entry.minIAD = minDist;

      xtalopt.interComp.insert(
        qMakePair<int, int>(firstAtomicNum, secondAtomicNum), entry);

      // We need to make a struct for the other way around for some reason
      // now... it's not my design...
      xtalopt.interComp.insert(
        qMakePair<int, int>(secondAtomicNum, firstAtomicNum), entry);
    }
  }

  return true;
}

void XtalOptCLIOptions::writeInitialRuntimeFile(XtalOpt& xtalopt)
{
  // Attempt to open the file. If we can't, just return.
  QFile file(xtalopt.CLIRuntimeFile());
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    return;

  QString text = "# XtalOpt Run-Time File\n";
  text += "# Change options here during the CLI run to "
          "change options in the program\n\n";

  text += "\n# Lattice constraints\n";
  text += QString("aMin = ") + QString::number(xtalopt.a_min) + "\n";
  text += QString("bMin = ") + QString::number(xtalopt.b_min) + "\n";
  text += QString("cMin = ") + QString::number(xtalopt.c_min) + "\n";
  text += QString("aMax = ") + QString::number(xtalopt.a_max) + "\n";
  text += QString("bMax = ") + QString::number(xtalopt.b_max) + "\n";
  text += QString("cMax = ") + QString::number(xtalopt.c_max) + "\n";

  text += QString("alphaMin = ") + QString::number(xtalopt.alpha_min) + "\n";
  text += QString("betaMin = ") + QString::number(xtalopt.beta_min) + "\n";
  text += QString("gammaMin = ") + QString::number(xtalopt.gamma_min) + "\n";
  text += QString("alphaMax = ") + QString::number(xtalopt.alpha_max) + "\n";
  text += QString("betaMax = ") + QString::number(xtalopt.beta_max) + "\n";
  text += QString("gammaMax = ") + QString::number(xtalopt.gamma_max) + "\n";
  text += QString("minVolume = ") + QString::number(xtalopt.vol_min) + "\n";
  text += QString("maxVolume = ") + QString::number(xtalopt.vol_max) + "\n";
  text += QString("maxVolumeScale = ") +
          QString::number(xtalopt.vol_scale_max) + "\n";
  text += QString("minVolumeScale = ") +
          QString::number(xtalopt.vol_scale_min) + "\n";
  // The "," below is just to avoid annoying "empty line/value" warning!
  text += QString("elementalVolumes = ,") + xtalopt.input_ele_volm_string + "\n";
  text += QString("usingRadiiInteratomicDistanceLimit = ") +
          fromBool(xtalopt.using_interatomicDistanceLimit) + "\n";
  text += QString("radiiScalingFactor = ") +
          QString::number(xtalopt.scaleFactor) + "\n";
  text += QString("minRadius = ") + QString::number(xtalopt.minRadius) + "\n";
  text +=
    QString("usingCustomIADs = ") + fromBool(xtalopt.using_customIAD) + "\n";
  text += QString("checkIADPostOptimization = ") +
          fromBool(xtalopt.using_checkStepOpt) + "\n";

  text += "\n# Optimization Settings\n";
  text += QString("maxAtoms = ") + QString::number(xtalopt.maxAtoms) + "\n";
  text += QString("localQueue = ") + fromBool(xtalopt.m_localQueue) + "\n";
  text += QString("objectivesReDo = ") +
          fromBool(xtalopt.m_objectivesReDo) + "\n";
  // Optimization Type
  text += QString("optimizationType = ") + xtalopt.m_optimizationType + "\n";
  text += QString("tournamentSelection = ") +
          fromBool(xtalopt.m_tournamentSelection) + "\n";
  text += QString("restrictedPool = ") +
          fromBool(xtalopt.m_restrictedPool) + "\n";
  text += QString("crowdingDistance = ") +
          fromBool(xtalopt.m_crowdingDistance) + "\n";
  text += QString("objectivePrecision = ") +
          QString::number(xtalopt.m_objectivePrecision) + "\n";

  text += QString("jobFailLimit = ") +
          QString::number(xtalopt.failLimit) + "\n";
  text += QString("jobFailAction = ");
  if (xtalopt.failAction == SearchBase::FA_DoNothing)
    text += "keepTrying\n";
  else if (xtalopt.failAction == SearchBase::FA_KillIt)
    text += "kill\n";
  else if (xtalopt.failAction == SearchBase::FA_Randomize)
    text += "replaceWithRandom\n";
  else if (xtalopt.failAction == SearchBase::FA_NewOffspring)
    text += "replaceWithOffspring\n";
  else
    text += "unknown\n";

  text += QString("limitRunningJobs = ") +
          fromBool(xtalopt.limitRunningJobs) + "\n";
  text += QString("runningJobLimit = ") +
          QString::number(xtalopt.runningJobLimit) + "\n";
  text += QString("continuousStructures = ") +
          QString::number(xtalopt.contStructs) + "\n";
  text += QString("parentsPoolSize = ") +
          QString::number(xtalopt.parentsPoolSize) + "\n";
  text += QString("maxNumStructures = ") +
          QString::number(xtalopt.maxNumStructures) + "\n";
  text += QString("softExit = ") + fromBool(xtalopt.m_softExit) + "\n";

  text += QString("\n# Search and Mutator Settings\n");
  text += QString("vcSearch = ") + fromBool(xtalopt.vcSearch) + "\n";
  text += QString("saveHullSnapshots = ") + fromBool(xtalopt.m_saveHullSnapshots) + "\n";
  text += QString("weightStripple = ") +
          QString::number(xtalopt.p_strip) + "\n";
  text += QString("weightPermustrain = ") +
          QString::number(xtalopt.p_perm) + "\n";
  text += QString("weightPermutomic = ") +
          QString::number(xtalopt.p_atomic) + "\n";
  text += QString("weightPermucomp = ") +
          QString::number(xtalopt.p_comp) + "\n";
  text += QString("weightCrossover = ") +
          QString::number(xtalopt.p_cross) + "\n";


  text += QString("\n# Mutator Details Settings\n");
  text += QString("randomSuperCell = ") +
          QString::number(xtalopt.p_supercell) + "\n";

  text += QString("strippleAmplitudeMin = ") +
          QString::number(xtalopt.strip_amp_min) + "\n";
  text += QString("strippleAmplitudeMax = ") +
          QString::number(xtalopt.strip_amp_max) + "\n";
  text += QString("strippleNumWavesAxis1 = ") +
          QString::number(xtalopt.strip_per1) + "\n";
  text += QString("strippleNumWavesAxis2 = ") +
          QString::number(xtalopt.strip_per2) + "\n";

  text += QString("strippleStrainStdevMin = ") +
          QString::number(xtalopt.strip_strainStdev_min) + "\n";
  text += QString("strippleStrainStdevMax = ") +
          QString::number(xtalopt.strip_strainStdev_max) + "\n";

  text += QString("permustrainNumExchanges = ") +
          QString::number(xtalopt.perm_ex) + "\n";
  text += QString("permustrainStrainStdevMax = ") +
          QString::number(xtalopt.perm_strainStdev_max) + "\n";

  text += QString("crossoverMinContribution = ") +
          QString::number(xtalopt.cross_minimumContribution) + "\n";
  text += QString("crossoverCuts = ") +
          QString::number(xtalopt.cross_ncuts) + "\n";

  text += QString("\n# Similarity Check and SpgLib Settings\n");
  text += QString("xtalcompToleranceLength = ") +
          QString::number(xtalopt.tol_xcLength) + "\n";
  text += QString("xtalcompToleranceAngle = ") +
          QString::number(xtalopt.tol_xcAngle) + "\n";
  text += QString("spglibTolerance = ") +
          QString::number(xtalopt.tol_spg) + "\n";
  text += QString("rdfTolerance = ") + QString::number(xtalopt.tol_rdf) + "\n";
  /* FIXME: this is a mark! For now, no runtime update of rdf details
  text += QString("rdfCutoff = ") + QString::number(xtalopt.tol_rdf_cutoff) + "\n";
  text += QString("rdfNumBins = ") + QString::number(xtalopt.tol_rdf_nbins) + "\n";
  text += QString("rdfSigma = ") + QString::number(xtalopt.tol_rdf_sigma) + "\n";
  */

  text += QString("\n# Queue Interface Settings\n");
  text += QString("queueRefreshInterval = ") +
          QString::number(xtalopt.queueRefreshInterval()) + "\n";
  text += QString("autoCancelJobAfterTime = ") +
          fromBool(xtalopt.m_cancelJobAfterTime) + "\n";

  if (xtalopt.m_cancelJobAfterTime) {
    text += QString("hoursForAutoCancelJob = ") +
            QString::number(xtalopt.m_hoursForCancelJobAfterTime) + "\n";
  }

  text += QString("verboseOutput = ") + fromBool(xtalopt.m_verbose) + "\n";

  file.write(text.toLocal8Bit().data());
}

void XtalOptCLIOptions::readRuntimeOptions(XtalOpt& xtalopt)
{
  // Attempt to open the file. If we can't, just return.
  QFile file(xtalopt.CLIRuntimeFile());
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return;

  // The options QHash
  QHash<QString, QString> options;

  // Read the file line-by-line
  while (!file.atEnd()) {
    QString line = file.readLine();
    processLine(line, options, xtalopt);
  }

  processRuntimeOptions(options, xtalopt);
}

inline bool CICompare(const QString& s1, const QString& s2)
{
  return s1.toLower() == s2.toLower();
}

void XtalOptCLIOptions::processRuntimeOptions(
  const QHash<QString, QString>& options, XtalOpt& xtalopt)
{
  // Change in some values at the runtime requires verification
  //   or further processing. This is a list of temporary
  //   variables that are updated if a runtime-adjusted value
  //   is given. We will verify them at the end and will update
  //   the "main" parameters only if changes are valid.
  double scal_vmin  = xtalopt.vol_scale_min;
  double scal_vmax  = xtalopt.vol_scale_max;
  double absl_vmin  = xtalopt.vol_min;
  double absl_vmax  = xtalopt.vol_max;
  QString elem_vols = xtalopt.input_ele_volm_string;

  for (const auto& option : options.keys()) {
    // Start with those that need further verification.
    if (CICompare("minVolume", option)) {
      absl_vmin = options[option].toFloat();
    } else if (CICompare("maxVolume", option)) {
      absl_vmax = options[option].toFloat();
    } else if (CICompare("maxVolumeScale", option)) {
      scal_vmax = options[option].toFloat();
    } else if (CICompare("minVolumeScale", option)) {
      scal_vmin = options[option].toFloat();
    } else if (CICompare("elementalVolumes", option)) {
      elem_vols = options[option];
    // Then, the rest of entries.
    } else if (CICompare("radiiScalingFactor", option)) {
      xtalopt.scaleFactor = options[option].toFloat();
      for (const auto& atomcn : xtalopt.eleMinRadii.getAtomicNumbers()) {
        double  minr = ElementInfo::getCovalentRadius(atomcn) * xtalopt.scaleFactor;
        minr = (minr > xtalopt.minRadius) ? minr : xtalopt.minRadius;
        xtalopt.eleMinRadii.set(atomcn, minr);
      }
    } else if (CICompare("minRadius", option)) {
      xtalopt.minRadius = options[option].toFloat();
      for (const auto& atomcn : xtalopt.eleMinRadii.getAtomicNumbers()) {
        double  minr = ElementInfo::getCovalentRadius(atomcn) * xtalopt.scaleFactor;
        minr = (minr > xtalopt.minRadius) ? minr : xtalopt.minRadius;
        xtalopt.eleMinRadii.set(atomcn, minr);
      }
    } else if (CICompare("verboseOutput", option)) {
      xtalopt.m_verbose = toBool(options[option]);
    } else if (CICompare("weightStripple", option)) {
      xtalopt.p_strip = options[option].toUInt();
    } else if (CICompare("weightPermustrain", option)) {
      xtalopt.p_perm = options[option].toUInt();
    } else if (CICompare("weightPermutomic", option)) {
      xtalopt.p_atomic = options[option].toUInt();
    } else if (CICompare("weightPermucomp", option)) {
      xtalopt.p_comp = options[option].toUInt();
    } else if (CICompare("weightCrossover", option)) {
      xtalopt.p_cross = options[option].toUInt();
    } else if (CICompare("crossoverCuts", option)) {
      xtalopt.cross_ncuts = options[option].toUInt();
    } else if (CICompare("vcSearch", option)) {
      xtalopt.vcSearch = toBool(options[option]);
    } else if (CICompare("saveHullSnapshots", option)) {
      xtalopt.m_saveHullSnapshots = toBool(options[option]);
    } else if (CICompare("randomSuperCell", option)) {
      xtalopt.p_supercell = options[option].toUInt();
    } else if (CICompare("maxAtoms", option)) {
      xtalopt.maxAtoms = options[option].toUInt();
    } else if (CICompare("aMin", option)) {
      xtalopt.a_min = options[option].toFloat();
    } else if (CICompare("bMin", option)) {
      xtalopt.b_min = options[option].toFloat();
    } else if (CICompare("cMin", option)) {
      xtalopt.c_min = options[option].toFloat();
    } else if (CICompare("aMax", option)) {
      xtalopt.a_max = options[option].toFloat();
    } else if (CICompare("bMax", option)) {
      xtalopt.b_max = options[option].toFloat();
    } else if (CICompare("cMax", option)) {
      xtalopt.c_max = options[option].toFloat();
    } else if (CICompare("alphaMin", option)) {
      xtalopt.alpha_min = options[option].toFloat();
    } else if (CICompare("betaMin", option)) {
      xtalopt.beta_min = options[option].toFloat();
    } else if (CICompare("gammaMin", option)) {
      xtalopt.gamma_min = options[option].toFloat();
    } else if (CICompare("alphaMax", option)) {
      xtalopt.alpha_max = options[option].toFloat();
    } else if (CICompare("betaMax", option)) {
      xtalopt.beta_max = options[option].toFloat();
    } else if (CICompare("gammaMax", option)) {
      xtalopt.gamma_max = options[option].toFloat();
    } else if (CICompare("usingRadiiInteratomicDistanceLimit", option)) {
      xtalopt.using_interatomicDistanceLimit = toBool(options[option]);
    } else if (CICompare("usingCustomIADs", option)) {
      xtalopt.using_customIAD = toBool(options[option]);
      if (xtalopt.using_customIAD && xtalopt.using_interatomicDistanceLimit) {
        qDebug() << "Warning: usingRadiiInteratomicDistanceLimit and"
                 << "usingCustomIADs cannot both be set to true.\n"
                 << "Switching off usingCustomIADs";
        xtalopt.using_customIAD = false;
      }
    } else if (CICompare("checkIADPostOptimization", option)) {
      xtalopt.using_checkStepOpt = toBool(options[option]);
    } else if (CICompare("parentsPoolSize", option)) {
      xtalopt.parentsPoolSize = options[option].toUInt();
    } else if (CICompare("limitRunningJobs", option)) {
      xtalopt.limitRunningJobs = toBool(options[option]);
    } else if (CICompare("runningJobLimit", option)) {
      xtalopt.runningJobLimit = options[option].toUInt();
    } else if (CICompare("continuousStructures", option)) {
      xtalopt.contStructs = options[option].toUInt();
    } else if (CICompare("jobFailLimit", option)) {
      xtalopt.failLimit = options[option].toUInt();
    } else if (CICompare("jobFailAction", option)) {
      QString failAction = options[option];
      if (failAction.toLower() == "keeptrying")
        xtalopt.failAction = SearchBase::FA_DoNothing;
      else if (failAction.toLower() == "kill")
        xtalopt.failAction = SearchBase::FA_KillIt;
      else if (failAction.toLower() == "replacewithrandom")
        xtalopt.failAction = SearchBase::FA_Randomize;
      else if (failAction.toLower() == "replacewithoffspring")
        xtalopt.failAction = SearchBase::FA_NewOffspring;
      else {
        qDebug() << "Warning: unrecognized jobFailAction: " << failAction;
        qDebug() << "Ignoring change in jobFailAction.";
      }
    } else if (CICompare("maxNumStructures", option)) {
      xtalopt.maxNumStructures = options[option].toUInt();
    } else if (CICompare("optimizationType", option)) {
      xtalopt.m_optimizationType = options[option].toLower();
    } else if (CICompare("tournamentSelection", option)) {
      xtalopt.m_tournamentSelection = toBool(options[option]);
    } else if (CICompare("restrictedPool", option)) {
      xtalopt.m_restrictedPool = toBool(options[option]);
    } else if (CICompare("crowdingDistance", option)) {
      xtalopt.m_crowdingDistance = toBool(options[option]);
    } else if (CICompare("objectivePrecision", option)) {
      xtalopt.m_objectivePrecision = options[option].toInt();
    } else if (CICompare("strippleAmplitudeMin", option)) {
      xtalopt.strip_amp_min = options[option].toFloat();
    } else if (CICompare("strippleAmplitudeMax", option)) {
      xtalopt.strip_amp_max = options[option].toFloat();
    } else if (CICompare("strippleNumWavesAxis1", option)) {
      xtalopt.strip_per1 = options[option].toUInt();
    } else if (CICompare("strippleNumWavesAxis2", option)) {
      xtalopt.strip_per2 = options[option].toUInt();
    } else if (CICompare("strippleStrainStdevMin", option)) {
      xtalopt.strip_strainStdev_min = options[option].toFloat();
    } else if (CICompare("strippleStrainStdevMax", option)) {
      xtalopt.strip_strainStdev_max = options[option].toFloat();
    } else if (CICompare("permustrainNumExchanges", option)) {
      xtalopt.perm_ex = options[option].toUInt();
    } else if (CICompare("permustrainStrainStdevMax", option)) {
      xtalopt.perm_strainStdev_max = options[option].toFloat();
    } else if (CICompare("crossoverMinContribution", option)) {
      xtalopt.cross_minimumContribution = options[option].toUInt();
      if (xtalopt.cross_minimumContribution < 25) {
        qDebug() << "Warning: crossover minimum contribution must be"
                    "at least 25. Setting it to 25.";
        xtalopt.cross_minimumContribution = 25;
      } else if (xtalopt.cross_minimumContribution > 50) {
        qDebug() << "Warning: crossover minimum contribution must not"
                 << "be greater than 50. Setting it to 50";
        xtalopt.cross_minimumContribution = 50;
      }
    } else if (CICompare("xtalcompToleranceLength", option)) {
      xtalopt.tol_xcLength = options[option].toFloat();
    } else if (CICompare("xtalcompToleranceAngle", option)) {
      xtalopt.tol_xcAngle = options[option].toFloat();
    } else if (CICompare("spglibTolerance", option)) {
      xtalopt.tol_spg = options[option].toFloat();
    } else if (CICompare("rdfTolerance", option)) {
      xtalopt.tol_rdf = options[option].toDouble();
    } else if (CICompare("rdfCutoff", option)) {
      xtalopt.tol_rdf_cutoff = options[option].toDouble();
    } else if (CICompare("rdfNumBins", option)) {
      xtalopt.tol_rdf_nbins = options[option].toInt();
    } else if (CICompare("rdfSigma", option)) {
      xtalopt.tol_rdf_sigma = options[option].toDouble();
    } else if (CICompare("autoCancelJobAfterTime", option)) {
      xtalopt.m_cancelJobAfterTime = toBool(options[option]);
    } else if (CICompare("hoursForAutoCancelJob", option)) {
      xtalopt.m_hoursForCancelJobAfterTime = options[option].toDouble();
    } else if (CICompare("queueRefreshInterval", option)) {
      xtalopt.setQueueRefreshInterval(options[option].toUInt());
    } else if (CICompare("softExit", option)) {
      xtalopt.m_softExit = toBool(options[option]);
    } else if (CICompare("hardExit", option)) {
      xtalopt.m_hardExit = toBool(options[option]);
    } else if (CICompare("localQueue", option)) {
      xtalopt.m_localQueue = toBool(options[option]);
    } else if (CICompare("objectivesReDo", option)) {
      xtalopt.m_objectivesReDo = toBool(options[option]);
    } else {
      qDebug() << "Warning: option," << option << ", is not a valid runtime"
               << "option! It is being ignored.";
    }
  }

  // Sanity checks and post-processing: absolute volume limits.
  if (absl_vmin < ZERO6 || absl_vmax < ZERO6 || absl_vmax < absl_vmin) {
    qDebug() << "Warning: ignored incorrect runtime values for volume limits!";
  } else {
    xtalopt.vol_min = absl_vmin;
    xtalopt.vol_max = absl_vmax;
  }
  // Sanity checks and post-processing: elemental volume limits.
  if (!xtalopt.processInputElementalVolumes(elem_vols)) {
    qDebug() << "Warning: ignored incorrect runtime values for elemental volume limits!";
  } else {
    xtalopt.input_ele_volm_string = elem_vols;
  }
  // Sanity checks and post-processing: scaled volume limits.
  // The lower limit is set to 0.0 (instead of ZERO...) to let the user set them
  //   to "zero" to not use them, without a repeating warning.
  if (scal_vmin < 0.0 || scal_vmax < scal_vmin) {
    qDebug() << "Warning: ignored incorrect runtime values for scaled volume limits!";
  } else {
    xtalopt.vol_scale_min = scal_vmin;
    xtalopt.vol_scale_max = scal_vmax;
  }
}

} // end namespace XtalOpt

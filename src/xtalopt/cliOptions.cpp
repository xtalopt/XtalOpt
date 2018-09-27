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

static const QStringList keywords = { "empiricalFormula",
                                      "formulaUnits",
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
                                      "volumeMin",
                                      "volumeMax",
                                      "usingRadiiInteratomicDistanceLimit",
                                      "usingCustomIADs",
                                      "checkIADPostOptimization",
                                      "radiiScalingFactor",
                                      "minRadius",
                                      "usingSubcellMitosis",
                                      "printSubcell",
                                      "mitosisDivisions",
                                      "mitosisA",
                                      "mitosisB",
                                      "mitosisC",
                                      "usingMolecularUnits",
                                      "usingRandSpg",
                                      "forcedSpgsWithRandSpg",
                                      "numInitial",
                                      "popSize",
                                      "limitRunningJobs",
                                      "runningJobLimit",
                                      "continuousStructures",
                                      "jobFailLimit",
                                      "jobFailAction",
                                      "maxNumStructures",
                                      "usingMitoticGrowth",
                                      "usingFormulaUnitCrossovers",
                                      "formulaUnitCrossoversGen",
                                      "usingOneGenePool",
                                      "chanceOfFutureMitosis",
                                      "percentChanceStripple",
                                      "percentChancePermustrain",
                                      "percentChanceCrossover",
                                      "strippleAmplitudeMin",
                                      "strippleAmplitudeMax",
                                      "strippleNumWavesAxis1",
                                      "strippleNumWavesAxis2",
                                      "strippleStrainStdevMin",
                                      "strippleStrainStdevMax",
                                      "permustrainNumExchanges",
                                      "permustrainStrainStdevMax",
                                      "crossoverMinContribution",
                                      "xtalcompToleranceLength",
                                      "xtalcompToleranceAngle",
                                      "spglibTolerance",
                                      "templatesDirectory",
                                      "queueInterface",
                                      "localWorkingDirectory",
                                      "logErrorDirectories",
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
                                      "kpointsTemplates" };

static const QStringList requiredKeywords = { "empiricalFormula",
                                              "queueInterface", "optimizer" };

static const QStringList validQueueInterfaces = { "loadleveler", "local",
                                                  "lsf",         "pbs",
                                                  "sge",         "slurm" };

static const QStringList requiredRemoteKeywords = { "host", "user",
                                                    "remoteWorkingDirectory",
                                                    "jobTemplates" };

static const QStringList validOptimizers = { "gulp", "castep", "pwscf",
                                             "siesta", "vasp" , "generic" };

static const QHash<QString, QStringList> requiredOptimizerKeywords = {
  { "gulp", { "ginTemplates" } },

  { "vasp", { "incarTemplates", "kpointsTemplates" } },

  { "pwscf", { "pwscfTemplates" } },

  { "castep", { "castepCellTemplates", "castepParamTemplates" } },

  { "siesta", { "fdfTemplates" } }
};

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
                                    QHash<QString, QString>& options)
{
  QString line = tmpLine.trimmed();

  // Remove everything to the right of '#' (including '#') since it is a comment
  line.remove(QRegExp(" *#.*"));

  if (line.isEmpty())
    return;

  // Get the key and the value
  QString key = line.section('=', 0, 0).trimmed().toLower();
  QString value = line.section('=', 1, 1).trimmed();

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
  // If we are not local, several remote files are required
  if (options["queueInterface"].toLower() != "local") {
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
  // First, let's set the radii options so we can use them in the
  // XtalCompositionStruct
  xtalopt.scaleFactor = options.value("radiiScalingFactor", "0.5").toFloat();
  xtalopt.minRadius = options.value("minRadius", "0.25").toFloat();

  // Next, read the composition
  // First uint is the atomic number and second uint is the quantity
  map<uint, uint> comp;
  if (!ElemInfo::readComposition(options["empiricalFormula"].toStdString(),
                                 comp)) {
    qDebug() << "Error reading composition: " << options["empiricalFormula"];
    return false;
  } else {
    for (const auto& elem : comp) {
      XtalCompositionStruct compStruct;
      // Set the radius - taking into account scaling factor and minRadius
      compStruct.minRadius =
        ElemInfo::getCovalentRadius(elem.first) * xtalopt.scaleFactor;
      if (compStruct.minRadius < xtalopt.minRadius)
        compStruct.minRadius = xtalopt.minRadius;

      compStruct.quantity = elem.second;
      xtalopt.comp[elem.first] = compStruct;
    }
  }

  // Next, the formula units
  QString unused;
  xtalopt.formulaUnitsList =
    FileUtils::parseUIntString(options["formulaUnits"], unused);
  if (xtalopt.formulaUnitsList.isEmpty())
    xtalopt.formulaUnitsList = { 1 };

  // We put default values in all of these
  // Initialization settings
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
  xtalopt.vol_min = options.value("volumeMin", "1.0").toFloat();
  xtalopt.vol_max = options.value("volumeMax", "100000.0").toFloat();

  // Check for fixed volume
  if (fabs(xtalopt.vol_min - xtalopt.vol_max) < 1.e-5) {
    xtalopt.using_fixed_volume = true;
    xtalopt.vol_fixed = xtalopt.vol_min;
  } else {
    xtalopt.using_fixed_volume = false;
  }

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

  xtalopt.using_mitosis = toBool(options.value("usingSubcellMitosis", "false"));
  xtalopt.using_subcellPrint = toBool(options.value("printSubcell", "false"));
  xtalopt.divisions = options.value("mitosisDivisions", "1").toUInt();
  xtalopt.ax = options.value("mitosisA", "1").toUInt();
  xtalopt.bx = options.value("mitosisB", "1").toUInt();
  xtalopt.cx = options.value("mitosisC", "1").toUInt();
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

  if (xtalopt.using_mitosis && !isMitosisOk(xtalopt)) {
    qDebug() << "Error: Invalid numbers entered for mitosis. Please check"
             << "your input and try again.";
    return false;
  }

  xtalopt.using_randSpg = toBool(options.value("usingRandSpg", "false"));
  if (xtalopt.using_randSpg) {
    // Create the list of space groups for generation
    for (int spg = 1; spg <= 230; spg++)
      xtalopt.minXtalsOfSpgPerFU.append(0);

    QStringList list = toList(options.value("forcedSpgsWithRandSpg", ""));
    for (const auto& item : list) {
      unsigned int num = item.toUInt();
      if (num != 0 && num <= 230)
        ++xtalopt.minXtalsOfSpgPerFU[num - 1];
    }
  }

  if ((xtalopt.using_randSpg && xtalopt.using_molUnit) ||
      (xtalopt.using_randSpg && xtalopt.using_mitosis)) {
    qDebug() << "Error: randSpg cannot be used with molUnit or subcell"
             << "mitosis. Please turn both of these off if you wish to"
             << "use randSpg.";
    return false;
  }

  // Search setings
  xtalopt.numInitial = options.value("numInitial", "20").toUInt();
  xtalopt.popSize = options.value("popSize", "20").toUInt();
  xtalopt.limitRunningJobs = toBool(options.value("limitRunningJobs", "true"));
  xtalopt.runningJobLimit = options.value("runningJobLimit", "2").toUInt();
  xtalopt.contStructs = options.value("continuousStructures", "3").toUInt();
  xtalopt.failLimit = options.value("jobFailLimit", "2").toUInt();

  QString failAction = options.value("jobFailAction", "replaceWithRandom");
  if (failAction.toLower() == "keeptrying")
    xtalopt.failAction = OptBase::FA_DoNothing;
  else if (failAction.toLower() == "kill")
    xtalopt.failAction = OptBase::FA_KillIt;
  else if (failAction.toLower() == "replacewithrandom")
    xtalopt.failAction = OptBase::FA_Randomize;
  else if (failAction.toLower() == "replacewithoffspring")
    xtalopt.failAction = OptBase::FA_NewOffspring;
  else {
    qDebug() << "Warning: unrecognized jobFailAction: " << failAction;
    qDebug() << "Using default option: replaceWithRandom";
    xtalopt.failAction = OptBase::FA_Randomize;
  }

  xtalopt.cutoff = options.value("maxNumStructures", "10000").toUInt();
  xtalopt.using_mitotic_growth =
    toBool(options.value("usingMitoticGrowth", "false"));
  xtalopt.using_FU_crossovers =
    toBool(options.value("usingFormulaUnitCrossovers", "false"));
  xtalopt.FU_crossovers_generation =
    options.value("formulaUnitCrossoversGen", "4").toUInt();
  xtalopt.using_one_pool = toBool(options.value("usingOneGenePool", "false"));
  xtalopt.chance_of_mitosis =
    options.value("chanceOfFutureMitosis", "50").toUInt();

  // Mutators
  xtalopt.p_strip = options.value("percentChanceStripple", "50").toUInt();
  xtalopt.p_perm = options.value("percentChancePermustrain", "35").toUInt();
  xtalopt.p_cross = options.value("percentChanceCrossover", "15").toUInt();

  // Sanity check
  if (xtalopt.p_strip + xtalopt.p_perm + xtalopt.p_cross != 100) {
    qDebug() << "Error: percentChanceStripple + percentChancePermustrain"
             << "+ percentChanceCrossover must equal 100!";
    return false;
  }

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
  xtalopt.cross_minimumContribution =
    options.value("crossoverMinContribution", "25").toUInt();

  // Duplicate matching
  xtalopt.tol_xcLength =
    options.value("xtalcompToleranceLength", "0.1").toFloat();
  xtalopt.tol_xcAngle =
    options.value("xtalcompToleranceAngle", "2.0").toFloat();

  // Spglib tolerance
  xtalopt.tol_spg = options.value("spglibTolerance", "0.05").toFloat();

  // We will use this later
  QString templatesDir = options.value("templatesDirectory", ".");

  // Are there any queue interfaces that are remote?
  bool anyRemote = false;

  size_t numOptSteps = options.value("numOptimizationSteps", "1").toUInt();

  for (size_t i = 0; i < numOptSteps; ++i) {

    xtalopt.appendOptStep();

    QString optInd = QString::number(i + 1);

#ifdef ENABLE_SSH
    xtalopt.setQueueInterface(
      i, options["queueInterface " + optInd].toLower().toStdString());
#else
    if (options["queueInterface " + optInd].toLower() != "local") {
      qDebug() << "Error: SSH is disabled, so only 'local' interface is"
               << "allowed.";
      qDebug() << "Please use the option 'queueInterface <optStep> = local'";
      return false;
    }
    xtalopt.setQueueInterface(i, "local");
#endif

#ifdef ENABLE_SSH

    bool remote = (options["queueInterface " + optInd].toLower() != "local");

    // We have additional things to set if we are remote
    if (remote) {
      anyRemote = true;
      RemoteQueueInterface* remoteQueue =
        qobject_cast<RemoteQueueInterface*>(xtalopt.queueInterface(i + 1));

      if (!options["submitCommand " + optInd].isEmpty())
        remoteQueue->setSubmitCommand(options["submitCommand " + optInd]);
      if (!options["cancelCommand " + optInd].isEmpty())
        remoteQueue->setCancelCommand(options["cancelCommand " + optInd]);
      if (!options["statusCommand " + optInd].isEmpty())
        remoteQueue->setStatusCommand(options["statusCommand " + optInd]);
    }
#endif

    QString optimizerName = options["optimizer " + optInd].toLower();
    QString queueName = xtalopt.queueInterface(i)->getIDString().toLower();
    xtalopt.setOptimizer(i, optimizerName.toStdString());
    XtalOptOptimizer* optimizer =
      static_cast<XtalOptOptimizer*>(xtalopt.optimizer(i));
    if (optimizerName == "castep") {
      if (!addOptimizerTemplate(xtalopt, "castepCellTemplates " + optInd,
                                queueName, i, options)) {
        return false;
      }

      if (!addOptimizerTemplate(xtalopt, "castepCellTemplates " + optInd,
                                queueName, i, options)) {
        return false;
      }
    } else if (options["optimizer"].toLower() == "gulp") {
      if (!addOptimizerTemplate(xtalopt, "ginTemplates " + optInd, queueName, i,
                                options)) {
        return false;
      }
    } else if (options["optimizer"].toLower() == "pwscf") {
      if (!addOptimizerTemplate(xtalopt, "pwscfTemplates " + optInd, queueName,
                                i, options)) {
        return false;
      }
    } else if (options["optimizer"].toLower() == "siesta") {
      if (!addOptimizerTemplate(xtalopt, "fdfTemplates " + optInd, queueName, i,
                                options)) {
        return false;
      }

      // Generating the PSF files require a little bit more work...
      // Generate list of symbols
      QVariantList psfInfo;
      QStringList symbols;
      QList<uint> atomicNums = xtalopt.comp.keys();
      qSort(atomicNums);

      for (const auto& atomicNum : atomicNums) {
        if (atomicNum != 0)
          symbols.append(ElemInfo::getAtomicSymbol(atomicNum).c_str());
      }
      qSort(symbols);
      QVariantHash hash;
      for (const auto& symbol : symbols) {
        QString filename =
          options["psffile " + symbol.toLower() + " " + optInd];
        if (filename.isEmpty()) {
          qDebug() << "Error: no PSF file found for atom type" << symbol
                   << "and opt step:" << optInd;
          qDebug() << "You must set the PSF file in the options like so:";
          QString tmp = "psfFile " + symbol + " " + optInd +
                        " = /path/to/siesta_psfs/symbol.psf";
          qDebug() << tmp;
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
      if (!addOptimizerTemplate(xtalopt, "incarTemplates " + optInd, queueName,
                                i, options)) {
        return false;
      }

      if (!addOptimizerTemplate(xtalopt, "kpointsTemplates " + optInd,
                                queueName, i, options)) {
        return false;
      }

      // Generating the POTCAR file requires a little bit more work...
      // Generate list of symbols
      QVariantList potcarInfo;
      QStringList symbols;
      QList<uint> atomicNums = xtalopt.comp.keys();
      qSort(atomicNums);

      for (const auto& atomicNum : atomicNums) {
        if (atomicNum != 0)
          symbols.append(ElemInfo::getAtomicSymbol(atomicNum).c_str());
      }
      qSort(symbols);
      QVariantHash hash;
      for (const auto& symbol : symbols) {
        QString filename =
          options["potcarfile " + symbol.toLower() + " " + optInd];
        if (filename.isEmpty()) {
          qDebug() << "Error: no POTCAR file found for atom type" << symbol
                   << "and opt step" << optInd;
          qDebug() << "You must set the POTCAR file in the options like so:";
          QString tmp = "potcarFile " + symbol + " " + optInd +
                        " = /path/to/vasp_potcars/symbol/POTCAR";
          qDebug() << tmp;
          return false;
        }

        hash.insert(symbol, QVariant(filename));
      }

      potcarInfo.append(QVariant(hash));

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
      if (!addOptimizerTemplate(xtalopt, "jobTemplates " + optInd, queueName, i,
                                options)) {
        return false;
      }
    }
#endif

    // This will only get used if we are local
    if (!options["exeLocation"].isEmpty())
      optimizer->setLocalRunCommand(options["exeLocation"]);
  }

  xtalopt.filePath =
    options.value("localWorkingDirectory", "localWorkingDirectory");
  // Make relative paths become absolute paths
  xtalopt.filePath = QDir(xtalopt.filePath).absolutePath();

  xtalopt.m_logErrorDirs =
    toBool(options.value("logErrorDirectories", "false"));

  if (anyRemote) {
    xtalopt.setQueueRefreshInterval(
      options.value("queueRefreshInterval", "10").toUInt());
    xtalopt.setCleanRemoteOnStop(
      toBool(options.value("cleanRemoteDirs", "false")));
    xtalopt.host = options["host"];
    xtalopt.port = options.value("port", "22").toUInt();
    xtalopt.username = options["user"];
    xtalopt.rempath = options["remoteWorkingDirectory"];
  }

  return true;
}

bool XtalOptCLIOptions::printOptions(const QHash<QString, QString>& options,
                                     XtalOpt& xtalopt)
{
  QStringList keys = options.keys();
  qSort(keys);

  QString output;
  QTextStream stream(&output);
  stream << "**** Manually set options: ****\n";
  for (const auto& key : keys)
    stream << key << ": " << options[key] << "\n";

  stream << "\n**** All options: ****\n";
  xtalopt.printOptionSettings(stream);

  // We need to convert to c string to properly print newlines
  qDebug() << output.toUtf8().data();

  // Let's clean out the directory if old data is here - before we write logs
  // Check if xtalopt data is already saved at the filePath
  if (QFile::exists(xtalopt.filePath + QDir::separator() + "xtalopt.state")) {
    bool proceed;
    QString msg = QString("Warning: XtalOpt data is already saved at: ") +
                  xtalopt.filePath +
                  "\nDo you wish to proceed and overwrite it?"
                  "\n\nIf no, please change the "
                  "'localWorkingDirectory' option in the input file";

    xtalopt.promptForBoolean(msg, &proceed);
    if (!proceed) {
      return false;
    } else {
      bool result = FileUtils::removeDir(xtalopt.filePath);
      if (!result) {
        qDebug() << "Error removing directory at" << xtalopt.filePath;
        return false;
      }
    }
  }
  // Make the directory if it doesn't exist...
  if (!QFile::exists(xtalopt.filePath))
    QDir().mkpath(xtalopt.filePath);

  // Try to write to a log file also
  QFile file(xtalopt.filePath + QDir::separator() + "xtaloptSettings.log");
  if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    file.write(output.toStdString().c_str());

  return true;
}

bool XtalOptCLIOptions::readOptions(const QString& filename, XtalOpt& xtalopt)
{
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
    processLine(line, options);
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
  QString filename = options[templateName];
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

bool XtalOptCLIOptions::isMitosisOk(XtalOpt& xtalopt)
{
  if (xtalopt.ax * xtalopt.bx * xtalopt.cx != xtalopt.divisions) {
    qDebug() << "Error: mitosisDivisions must equal"
             << "mitosisA * mitosisB * mitosisC";
    return false;
  }

  size_t minNumAtoms =
    std::min_element(
      xtalopt.comp.cbegin(), xtalopt.comp.cend(),
      [](const XtalCompositionStruct& lhs, const XtalCompositionStruct& rhs) {
        return lhs.quantity < rhs.quantity;
      })
      ->quantity;

  if (minNumAtoms == 0) {
    qDebug() << "Error: no atoms were found when checking mitosis!";
    return false;
  }

  minNumAtoms *= xtalopt.minFU();

  if (minNumAtoms < xtalopt.divisions) {
    qDebug() << "Error: mitosisDivisions cannot be greater than the smallest"
             << "formula unit times the smallest number of atoms of one type";
    qDebug() << "With the current composition and formula unit, the largest"
             << "number of divisions possible is:" << minNumAtoms;
    return false;
  }

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
        ElemInfo::getAtomicNum(centerSymbol.toStdString());

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
        ElemInfo::getAtomicNum(neighborSymbol.toStdString());

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

  // Check to see if we exceeded the number of atoms in any cases.
  for (const auto& countKey : atomCounts.keys()) {
    if (xtalopt.comp[ElemInfo::getAtomicNum(countKey.toStdString().c_str())]
            .quantity *
          xtalopt.minFU() <
        atomCounts[countKey]) {
      QString elemName = countKey;
      if (!elemName.isEmpty())
        elemName[0] = elemName[0].toUpper();
      qDebug() << "Error reading molecularUnits: for atom" << elemName << ","
               << "there are more atoms predicted to be generated with"
               << "the molUnit settings than there are for minFU * numAtoms";
      qDebug() << "You must make sure that for each atom, minFU * numAtoms is"
               << "greater than the number of atoms to be generated with"
               << "MolUnits.";
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
        ElemInfo::getAtomicNum(firstSymbol.toStdString());

      // If the atomic number is 0, the symbol is invalid
      if (firstAtomicNum == 0) {
        qDebug() << "Error processing customIAD line:"
                 << "Invalid atomic symbol:" << firstSymbol;
        qDebug() << "Proper format is as follows: "
                 << "<firstSymbol>, <secondSymbol>, <minDistance>";
        return false;
      }

      unsigned short secondAtomicNum =
        ElemInfo::getAtomicNum(secondSymbol.toStdString());

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

  QString tmpText;
  for (const auto& e : xtalopt.formulaUnitsList)
    tmpText += (QString::number(e) + ", ");
  tmpText.chop(2);

  QString result;
  FileUtils::parseUIntString(tmpText, result);
  QString text = "# XtalOpt Run-Time File\n";
  text += "# Change options here during the CLI run to "
          "change options in the program\n\n";
  text += QString("formulaUnits = ") + result + "\n";

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
  text += QString("volumeMin = ") + QString::number(xtalopt.vol_min) + "\n";
  text += QString("volumeMax = ") + QString::number(xtalopt.vol_max) + "\n";
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
  text += QString("popSize = ") + QString::number(xtalopt.popSize) + "\n";
  text +=
    QString("limitRunningJobs = ") + fromBool(xtalopt.limitRunningJobs) + "\n";
  text += QString("runningJobLimit = ") +
          QString::number(xtalopt.runningJobLimit) + "\n";
  text += QString("continuousStructures = ") +
          QString::number(xtalopt.contStructs) + "\n";
  text +=
    QString("jobFailLimit = ") + QString::number(xtalopt.failLimit) + "\n";
  text += QString("jobFailAction = ");
  if (xtalopt.failAction == OptBase::FA_DoNothing)
    text += "keepTrying\n";
  else if (xtalopt.failAction == OptBase::FA_KillIt)
    text += "kill\n";
  else if (xtalopt.failAction == OptBase::FA_Randomize)
    text += "replaceWithRandom\n";
  else if (xtalopt.failAction == OptBase::FA_NewOffspring)
    text += "replaceWithOffspring\n";
  else
    text += "unknown\n";

  text +=
    QString("maxNumStructures = ") + QString::number(xtalopt.cutoff) + "\n";
  text += QString("usingMitoticGrowth = ") +
          fromBool(xtalopt.using_mitotic_growth) + "\n";
  text += QString("usingFormulaUnitCrossovers = ") +
          fromBool(xtalopt.using_FU_crossovers) + "\n";
  text += QString("formulaUnitCrossoversGen = ") +
          QString::number(xtalopt.FU_crossovers_generation) + "\n";
  text +=
    QString("usingOneGenePool = ") + fromBool(xtalopt.using_one_pool) + "\n";
  text += QString("chanceOfFutureMitosis = ") +
          QString::number(xtalopt.chance_of_mitosis) + "\n";

  text += QString("\n# Mutator Settings\n");
  text += QString("percentChanceStripple = ") +
          QString::number(xtalopt.p_strip) + "\n";
  text += QString("percentChancePermustrain = ") +
          QString::number(xtalopt.p_perm) + "\n";
  text += QString("percentChanceCrossover = ") +
          QString::number(xtalopt.p_cross) + "\n";

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

  text += QString("\n# Duplicate Matching and SpgLib Settings\n");
  text += QString("xtalcompToleranceLength = ") +
          QString::number(xtalopt.tol_xcLength) + "\n";
  text += QString("xtalcompToleranceAngle = ") +
          QString::number(xtalopt.tol_xcAngle) + "\n";
  text +=
    QString("spglibTolerance = ") + QString::number(xtalopt.tol_spg) + "\n";

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
    processLine(line, options);
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
  for (const auto& option : options.keys()) {
    if (CICompare("formulaUnits", option)) {
      QString unused;
      xtalopt.formulaUnitsList =
        FileUtils::parseUIntString(options["formulaUnits"], unused);
      if (xtalopt.formulaUnitsList.isEmpty()) {
        qDebug()
          << "Warning: in runtime options, formula units unsuccessfully read."
          << "\nSetting formula units to be 1.";
        xtalopt.formulaUnitsList = { 1 };
      }
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
    } else if (CICompare("volumeMin", option)) {
      xtalopt.vol_min = options[option].toFloat();
    } else if (CICompare("volumeMax", option)) {
      xtalopt.vol_max = options[option].toFloat();
    } else if (CICompare("usingRadiiInteratomicDistanceLimit", option)) {
      xtalopt.using_interatomicDistanceLimit = toBool(options[option]);
    } else if (CICompare("radiiScalingFactor", option)) {
      xtalopt.scaleFactor = options[option].toFloat();
      for (const auto& key : xtalopt.comp.keys()) {
        xtalopt.comp[key].minRadius =
          ElemInfo::getCovalentRadius(key) * xtalopt.scaleFactor;
        if (xtalopt.comp[key].minRadius < xtalopt.minRadius)
          xtalopt.comp[key].minRadius = xtalopt.minRadius;
      }
    } else if (CICompare("minRadius", option)) {
      xtalopt.minRadius = options[option].toFloat();
      for (const auto& key : xtalopt.comp.keys()) {
        xtalopt.comp[key].minRadius =
          ElemInfo::getCovalentRadius(key) * xtalopt.scaleFactor;
        if (xtalopt.comp[key].minRadius < xtalopt.minRadius)
          xtalopt.comp[key].minRadius = xtalopt.minRadius;
      }
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
    } else if (CICompare("popSize", option)) {
      xtalopt.popSize = options[option].toUInt();
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
        xtalopt.failAction = OptBase::FA_DoNothing;
      else if (failAction.toLower() == "kill")
        xtalopt.failAction = OptBase::FA_KillIt;
      else if (failAction.toLower() == "replacewithrandom")
        xtalopt.failAction = OptBase::FA_Randomize;
      else if (failAction.toLower() == "replacewithoffspring")
        xtalopt.failAction = OptBase::FA_NewOffspring;
      else {
        qDebug() << "Warning: unrecognized jobFailAction: " << failAction;
        qDebug() << "Ignoring change in jobFailAction.";
      }
    } else if (CICompare("maxNumStructures", option)) {
      xtalopt.cutoff = options[option].toUInt();
    } else if (CICompare("usingMitoticGrowth", option)) {
      xtalopt.using_mitotic_growth = toBool(options[option]);
    } else if (CICompare("usingFormulaUnitCrossovers", option)) {
      xtalopt.using_FU_crossovers = toBool(options[option]);
    } else if (CICompare("formulaUnitCrossoversGen", option)) {
      xtalopt.FU_crossovers_generation = options[option].toUInt();
    } else if (CICompare("usingOneGenePool", option)) {
      xtalopt.using_one_pool = toBool(options[option]);
    } else if (CICompare("chanceOfFutureMitosis", option)) {
      xtalopt.chance_of_mitosis = options[option].toUInt();
      if (xtalopt.chance_of_mitosis > 100) {
        qDebug() << "Warning: chanceOfFutureMitosis must not be greater"
                 << "than 100. Setting chanceOfFutureMitosis to 100";
        xtalopt.chance_of_mitosis = 100;
      }
    } else if (CICompare("percentChanceStripple", option)) {
      xtalopt.p_strip = options[option].toUInt();
    } else if (CICompare("percentChancePermustrain", option)) {
      xtalopt.p_perm = options[option].toUInt();
    } else if (CICompare("percentChanceCrossover", option)) {
      xtalopt.p_cross = options[option].toUInt();
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
    } else {
      qDebug() << "Warning: option," << option << ", is not a valid runtime"
               << "option! It is being ignored.";
    }
  }

  // Sanity checks
  if (xtalopt.p_strip + xtalopt.p_perm + xtalopt.p_cross != 100) {
    qDebug() << "Error: percentChanceStripple + percentChancePermustrain"
             << "+ percentChanceCrossover must equal 100!";
    qDebug() << "Setting them to default values of 50, 35, 15, respectively";
    xtalopt.p_strip = 50;
    xtalopt.p_perm = 35;
    xtalopt.p_cross = 15;
  }
}

} // end namespace XtalOpt

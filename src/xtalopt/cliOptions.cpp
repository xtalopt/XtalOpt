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

static const QStringList keywords =
{
  "empiricalFormula",
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
  "usingInteratomicDistanceLimit",
  "radiiScalingFactor",
  "minRadius",
  "usingSubcellMitosis",
  "printSubcell",
  "mitosisDivisions",
  "mitosisA",
  "mitosisB",
  "mitosisC",
  "usingmolecularUnits",
  "usingRandSpg",
  "numInitial",
  "popSize",
  "limitRunningJobs",
  "runningJobLimit",
  "continuousStructures",
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
  "kpointsTemplates",
  "potcarFile"
};

static const QStringList requiredKeywords =
{
  "empiricalFormula",
  "queueInterface",
  "optimizer"
};

static const QStringList validQueueInterfaces =
{
  "loadleveler",
  "local",
  "lsf",
  "pbs",
  "sge",
  "slurm"
};

static const QStringList requiredRemoteKeywords =
{
  "host",
  "user",
  "remoteWorkingDirectory",
  "jobTemplates"
};

static const QStringList validOptimizers =
{
  "gulp",
  "castep",
  "pwscf",
  "siesta",
  "vasp"
};

static const QHash<QString, QStringList> requiredOptimizerKeywords =
{
  {
    "gulp",
    {
      "ginTemplates"
    }
  },

  {
    "vasp",
    {
      "incarTemplates",
      "kpointsTemplates",
      "potcarFile"
    }
  },

  {
    "pwscf",
    {
      "pwscfTemplates"
    }
  },

  {
    "castep",
    {
      "castepCellTemplates",
      "castepParamTemplates"
    }
  },

  {
    "siesta",
    {
      "fdfTemplates"
    }
  }
};

bool XtalOptCLIOptions::isKeyword(const QString& s,
                                  QString& csString)
{
  size_t ind = keywords.indexOf(QRegExp(s, Qt::CaseInsensitive));
  if (ind == -1)
    return false;
  csString = keywords[ind];
  return true;
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
             << "' was read in input file.";
    return;
  }

  if (value.isEmpty()) {
    qDebug() << "Warning: invalid line '" << tmpLine
             << "' was read in input file.";
    return;
  }

  // Case sensitive key
  QString csKey;
  if (!isKeyword(key, csKey)) {
    qDebug() << "Warning: ignoring unrecognized option in line '"
             << tmpLine << "'";
    return;
  }

  options[csKey] = value;
}

bool XtalOptCLIOptions::requiredOptionsSet(const QHash<QString,
                                                       QString>& options)
{
  // Do we have all the required keywords?
  for (const auto& requiredKeyword: requiredKeywords) {
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
    qDebug() << "Error: unrecognized optimizer, '"
             << options["optimizer"] << "', was entered.\n"
             << "Valid optimizers are: " << validOptimizers;
    return false;
  }

#ifdef ENABLE_SSH
  // If we are not local, several remote files are required
  if (options["queueInterface"].toLower() != "local") {
    for (const auto& requiredKeyword: requiredRemoteKeywords) {
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

  for (const auto& requiredKeyword: requiredOptKeys) {
    if (options[requiredKeyword].isEmpty()) {
      qDebug() << "Error: required option for" << options["optimizer"] << ",'"
               << requiredKeyword << "', was not set in the options file.\n"
               << "Required options for" << options["optimizer"] << "are: "
               << requiredOptKeys;
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
  }
  else {
    for (const auto& elem: comp) {
      XtalCompositionStruct compStruct;
      // Set the radius - taking into account scaling factor and minRadius
      compStruct.minRadius = ElemInfo::getCovalentRadius(elem.first) *
                             xtalopt.scaleFactor;
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
    xtalopt.formulaUnitsList = {1};

  // We put default values in all of these
  // Initialization settings
  xtalopt.a_min = options.value("aMin", "3.0").toFloat();
  xtalopt.b_min = options.value("bMin", "3.0").toFloat();
  xtalopt.c_min = options.value("cMin", "3.0").toFloat();
  xtalopt.a_max = options.value("aMax", "10.0").toFloat();
  xtalopt.b_max = options.value("bMax", "10.0").toFloat();
  xtalopt.c_max = options.value("cMax", "10.0").toFloat();
  xtalopt.alpha_min = options.value("alphaMin", "60.0").toFloat();
  xtalopt.beta_min  = options.value("betaMin",  "60.0").toFloat();
  xtalopt.gamma_min = options.value("gammaMin", "60.0").toFloat();
  xtalopt.alpha_max = options.value("alphaMax", "120.0").toFloat();
  xtalopt.beta_max  = options.value("betaMax",  "120.0").toFloat();
  xtalopt.gamma_max = options.value("gammaMax", "120.0").toFloat();
  xtalopt.vol_min = options.value("volumeMin", "1.0").toFloat();
  xtalopt.vol_max = options.value("volumeMax", "100000.0").toFloat();
  xtalopt.using_interatomicDistanceLimit =
    toBool(options.value("usingInteratomicDistanceLimit", "true"));
  xtalopt.using_mitosis = toBool(options.value("usingSubcellMitosis", "false"));
  xtalopt.using_subcellPrint = toBool(options.value("printSubcell", "false"));
  xtalopt.divisions = options.value("mitosisDivisions", "1").toUInt();
  xtalopt.ax = options.value("mitosisA", "1").toUInt();
  xtalopt.bx = options.value("mitosisB", "1").toUInt();
  xtalopt.cx = options.value("mitosisC", "1").toUInt();
  xtalopt.using_molUnit = toBool(options.value("usingMolcularUnits", "false"));
  xtalopt.using_randSpg = toBool(options.value("usingRandSpg", "false"));

  // Search setings
  xtalopt.numInitial = options.value("numInitial", "20").toUInt();
  xtalopt.popSize = options.value("popSize", "20").toUInt();
  xtalopt.limitRunningJobs = toBool(options.value("limitRunningJobs", "true"));
  xtalopt.runningJobLimit = options.value("runningJobLimit", "2").toUInt();
  xtalopt.contStructs = options.value("continuousStructures", "3").toUInt();
  xtalopt.using_mitotic_growth =
    toBool(options.value("usingMitoticGrowth", "false"));
  xtalopt.using_FU_crossovers =
    toBool(options.value("usingFormulaUnitCrossovers", "false"));
  xtalopt.FU_crossovers_generation =
    options.value("formulaUnitCrossoversGen", "4").toUInt();
  xtalopt.using_one_pool =
    toBool(options.value("usingOneGenePool", "false"));
  xtalopt.chance_of_mitosis =
    options.value("chanceOfFutureMitosis", "50").toUInt();

  // Mutators
  xtalopt.p_strip = options.value("percentChanceStripple", "50").toUInt();
  xtalopt.p_perm  = options.value("percentChancePermustrain", "35").toUInt();
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

  unique_ptr<QueueInterface> queue(nullptr);

#ifdef ENABLE_SSH
  if (options["queueInterface"].toLower() == "loadleveler") {
    queue = make_unique<LoadLevelerQueueInterface>(&xtalopt);
  }
  else if (options["queueInterface"].toLower() == "local") {
    queue = make_unique<LocalQueueInterface>(&xtalopt);
  }
  else if (options["queueInterface"].toLower() == "lsf") {
    queue = make_unique<LsfQueueInterface>(&xtalopt);
  }
  else if (options["queueInterface"].toLower() == "pbs") {
    queue = make_unique<PbsQueueInterface>(&xtalopt);
  }
  else if (options["queueInterface"].toLower() == "sge") {
    queue = make_unique<SgeQueueInterface>(&xtalopt);
  }
  else if (options["queueInterface"].toLower() == "slurm") {
    queue = make_unique<SlurmQueueInterface>(&xtalopt);
  }
  else {
    qDebug() << "Error: unknown queue interface: " << options["queueInterface"];
    return false;
  }
#else
  if (options["queueInterface"].toLower() == "local") {
    queue = make_unique<LocalQueueInterface>(&xtalopt);
  }
  else {
    qDebug() << "Error: SSH is disabled, so only 'local' interface is allowed.";
    qDebug() << "Please use the option 'queueInterface = local'";
    return false;
  }
#endif

  xtalopt.filePath = options.value("localWorkingDirectory",
                                   "localWorkingDirectory");
  // Make relative paths become absolute paths
  xtalopt.filePath = QDir(xtalopt.filePath).absolutePath();

  xtalopt.m_logErrorDirs =
    toBool(options.value("logErrorDirectories", "false"));

  size_t numOptSteps = options.value("numOptimizationSteps", "1").toUInt();

#ifdef ENABLE_SSH
  bool remote = (options["queueInterface"].toLower() != "local");

  // We have additional things to set if we are remote
  if (remote) {
    xtalopt.host = options["host"];
    xtalopt.port = options.value("port", "22").toUInt();
    xtalopt.username = options["user"];

    xtalopt.rempath = options["remoteWorkingDirectory"];

    RemoteQueueInterface* remoteQueue =
      qobject_cast<RemoteQueueInterface*>(queue.get());

    if (!options["submitCommand"].isEmpty())
      remoteQueue->setSubmitCommand(options["submitCommand"]);
    if (!options["cancelCommand"].isEmpty())
      remoteQueue->setCancelCommand(options["cancelCommand"]);
    if (!options["statusCommand"].isEmpty())
      remoteQueue->setStatusCommand(options["statusCommand"]);

    remoteQueue->setInterval(
      options.value("queueRefreshInterval", "10").toUInt());
    remoteQueue->setCleanRemoteOnStop(
      toBool(options.value("cleanRemoteDirs", "false")));
  }
#endif

  xtalopt.setQueueInterface(queue.release());

  std::unique_ptr<XtalOptOptimizer> optimizer(nullptr);
  if (options["optimizer"].toLower() == "castep") {
    optimizer = make_unique<CASTEPOptimizer>(&xtalopt);

    if (!addOptimizerTemplates("castepCellTemplates", options, numOptSteps,
                               *optimizer, *xtalopt.queueInterface())) {
      return false;
    }

    if (!addOptimizerTemplates("castepParamTemplates", options, numOptSteps,
                               *optimizer, *xtalopt.queueInterface())) {
      return false;
    }
  }
  else if (options["optimizer"].toLower() == "gulp") {
    optimizer = make_unique<GULPOptimizer>(&xtalopt);

    if (!addOptimizerTemplates("ginTemplates", options, numOptSteps,
                               *optimizer, *xtalopt.queueInterface())) {
      return false;
    }
  }
  else if (options["optimizer"].toLower() == "pwscf") {
    optimizer = make_unique<PWscfOptimizer>(&xtalopt);

    if (!addOptimizerTemplates("pwscfTemplates", options, numOptSteps,
                               *optimizer, *xtalopt.queueInterface())) {
      return false;
    }
  }
  else if (options["optimizer"].toLower() == "siesta") {
    optimizer = make_unique<SIESTAOptimizer>(&xtalopt);

    if (!addOptimizerTemplates("fdfTemplates", options, numOptSteps,
                               *optimizer, *xtalopt.queueInterface())) {
      return false;
    }
  }
  else if (options["optimizer"].toLower() == "vasp") {
    optimizer = make_unique<VASPOptimizer>(&xtalopt);

    if (!addOptimizerTemplates("incarTemplates", options, numOptSteps,
                               *optimizer, *xtalopt.queueInterface())) {
      return false;
    }

    if (!addOptimizerTemplates("kpointsTemplates", options, numOptSteps,
                               *optimizer, *xtalopt.queueInterface())) {
      return false;
    }

    // For the POTCAR file, let's just add the same thing to every step
    QFile file(templatesDir + "/" + options["potcarFile"]);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      qDebug() << "Error: could not open file '" << options["potcarFile"]
               << "' in the templates directory: " << templatesDir;
      return false;
    }
    for (size_t i = 0; i < numOptSteps; ++i)
      optimizer->appendTemplate("POTCAR", file.readAll());
  }
  else {
    qDebug() << "Error: unknown optimizer:" << options["optimizer"];
    return false;
  }

#ifdef ENABLE_SSH
  if (remote) {
    // We need to add the job templates if we are remote
    if (!addOptimizerTemplates("jobTemplates", options, numOptSteps,
                               *optimizer, *xtalopt.queueInterface())) {
      return false;
    }
  }
#endif

  // The first opt step should just be a blank one. Remove it.
  optimizer->removeAllTemplatesForOptStep(0);

  // Let us make sure all the optimization steps have the same number of steps
  QStringList templateNames = optimizer->getTemplateNames();
  for (const auto& templateName: templateNames) {
    if (optimizer->getTemplate(templateName).size() != numOptSteps) {
      qDebug() << "Error: template '" << templateName << "' does not"
               << "have the correct number of optimization steps ("
               << numOptSteps << ")!";
      qDebug() << templateName << " has "
               << optimizer->getTemplate(templateName).size() << " steps";
      return false;
    }
  }

  // This will only get used if we are local
  if (!options["exeLocation"].isEmpty())
    optimizer->setLocalRunCommand(options["exeLocation"]);

  xtalopt.setOptimizer(optimizer.release());

  return true;
}

void XtalOptCLIOptions::printOptions(const QHash<QString, QString>& options,
                                     const XtalOpt& xtalopt)
{
  QStringList keys = options.keys();
  qSort(keys);

  QString output;
  QTextStream stream(&output);
  stream << "**** Manually set options: ****\n";
  for (const auto& key: keys)
    stream << key << ": " << options[key] << "\n";

  stream << "\n**** All options: ****\n";
  xtalopt.printOptionSettings(stream);

  // We need to convert to c string to properly print newlines
  qDebug() << output.toUtf8().data();

  // Try to write to a log file also
  QFile file("xtaloptSettings.log");
  if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    file.write(output.toStdString().c_str());
}

bool XtalOptCLIOptions::readOptions(const QString& filename,
                                    XtalOpt& xtalopt)
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
  printOptions(options, xtalopt);

  return true;
}

bool XtalOptCLIOptions::toBool(const QString& s)
{
  QString qs = s.trimmed();
  // Should be true if it begins with 't' or it is any number other than zero
  if (qs.startsWith("t", Qt::CaseInsensitive) || qs.toInt() != 0)
    return true;
  return false;
}

QString XtalOptCLIOptions::toString(bool b)
{
  if (b)
    return "true";
  return "false";
}

QStringList XtalOptCLIOptions::toList(const QString& s)
{
  QStringList qsl = s.trimmed().split(",", QString::SkipEmptyParts);
  // Trim every QString in the list
  std::for_each(qsl.begin(), qsl.end(), [](QString& qs) { qs = qs.trimmed(); });
  return qsl;
}

// Convert the template names to the name used by the optimizer
QString convertedTemplateName(const QString& s, const QueueInterface& queue)
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
  if (s.compare("potcarFile", Qt::CaseInsensitive) == 0)
    return "POTCAR";

  if (s.compare("jobTemplates", Qt::CaseInsensitive) == 0) {
    QString id = queue.getIDString();
    if (id.compare("LoadLeveler", Qt::CaseInsensitive) == 0)
      return "job.ll";
    else if (id.compare("LSF", Qt::CaseInsensitive) == 0)
      return "job.lsf";
    else if (id.compare("PBS", Qt::CaseInsensitive) == 0)
      return "job.pbs";
    else if (id.compare("SGE", Qt::CaseInsensitive) == 0)
      return "job.sh";
    else if (id.compare("SLURM", Qt::CaseInsensitive) == 0)
      return "job.slurm";
    qDebug() << "Unknown queue id:" << id;
    return "job.sh";
  }

  qDebug() << "Unknown template name:" << s;
  return "";
}


bool XtalOptCLIOptions::addOptimizerTemplates(
                                    const QString& templateName,
                                    const QHash<QString, QString>& options,
                                    size_t numOptSteps,
                                    XtalOptOptimizer& optimizer,
                                    QueueInterface& queue)
{
  QStringList filenames = toList(options[templateName]);
  if (options[templateName].isEmpty() || filenames.size() != numOptSteps) {
    qDebug() << "Error: the number of " << templateName << " files must be"
             << "equal to the number of optimization steps.";
    return false;
  }

  // Now we have to open the files and read their contents
  for (const auto& filename: filenames) {
    QFile file(options.value("templatesDirectory", ".") + "/" + filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      qDebug() << "Error: could not open file '" << filename << "' in"
               << "the templates directory: "
               << options.value("templatesDirectory", ".");
      return false;
    }
    QString text = file.readAll();
    optimizer.appendTemplate(convertedTemplateName(templateName, queue),
                             text);
  }

  return true;
}

} // end namespace XtalOpt

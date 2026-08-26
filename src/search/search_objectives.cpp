/**********************************************************************
  search_objectives - Objective execution helpers for global search

  Copyright (C) 2010-2011 by David C. Lonie
  Copyright (C) 2024 Samad Hajinazar

  This source code is released under the New BSD License.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/constants.h>
#include <search/search.h>

#include <common/compatibility/platform_compat.h>
#include <common/compatibility/qt_compat.h>

#include <common/fileutils.h>
#include <common/output.h>
#include <search/queueinterface.h>
#include <search/structure.h>
#include <atoms/formats/vaspformat.h>

#include <QFile>
#include <QHash>
#include <QElapsedTimer>
#include <QObject>
#include <QStringList>
#include <QTextStream>
#include <QThread>

#include <cmath>
#include <climits>
#include <limits>
#include <string>
#include <vector>

namespace Search {

namespace {

bool usesLocalExecutionHost(const SearchBase* search, const QueueInterface* queue)
{
  // Direct-run jobs are always executed locallly.
  return !queue->isBatchQueue() || !search->isRemoteQueue();
}

struct ScriptCalculationContext
{
  const SearchBase* search;
  QueueInterface* queue;
  bool localRun;
  QString localDir;
  QString workDir;
};

struct ExternalScript
{
  ExternalScript(int displayIndex_, const QString& executable_, const QString& outputFile_)
    : displayIndex(displayIndex_),
      executable(executable_),
      outputFile(outputFile_)
  {
  }

  int displayIndex;
  QString executable;
  QString outputFile;
};

ScriptCalculationContext calculationContext(SearchBase* search, Structure* s)
{
  ScriptCalculationContext context;
  context.search = search;
  context.queue = search->queueInterface(s->getCurrentOptStep());
  context.localRun = usesLocalExecutionHost(search, context.queue);
  context.localDir = s->getLocpath();
  context.workDir = context.localRun ? context.localDir : s->getRempath();
  return context;
}

QString workPath(const ScriptCalculationContext& context, const QString& filename)
{
  return context.localRun ? Common::localPath(context.workDir, filename)
                          : Common::remotePath(context.workDir, filename);
}

bool writeOutputPoscar(Structure* s, const QString& filename)
{
  const std::string poscar = Atoms::VaspFormat::writeToString(*s, s->getLocpath()).toStdString();
  if (poscar.empty())
    return false;

  // Write with plain newlines on every platform: this file may be copied
  // to the cluster for the user scripts to read.
  QFile file(Common::localPath(s->getLocpath(), filename));
  if (file.open(QIODevice::WriteOnly) &&
      file.write(poscar.c_str(), static_cast<qint64>(poscar.size())) ==
                                 static_cast<qint64>(poscar.size())) {
    file.close();
    return true;
  }
  return false;
}

bool removeOldOutputFiles(Structure* s, const ScriptCalculationContext& context,
                          const std::vector<ExternalScript>& scripts)
{
  QStringList outputFiles;
  outputFiles.reserve(static_cast<int>(scripts.size()));
  for (const auto& script : scripts)
    outputFiles.append(script.outputFile);
  if (outputFiles.isEmpty())
    return true;

  QHash<QString, bool> existingFiles;
  if (!context.queue->checkIfFilesExist(s, outputFiles, &existingFiles))
    return false;

  for (const auto& script : scripts) {
    if (existingFiles.value(script.outputFile, false)) {
      if (!context.queue->removeAFile(s, script.outputFile)) {
        Common::error(QObject::tr("Failed to remove file %1!")
                                  .arg(script.outputFile));
        return false;
      }
    }
  }
  return true;
}

bool runExternalScripts(Structure* s, const ScriptCalculationContext& context,
                        const std::vector<ExternalScript>& scripts, const QString& scriptKind)
{
  int timeoutMs = -1;
  if (context.search->cancelScriptAfterTime()) {
    const double requested = context.search->hoursForCancelScriptAfterTime() * 3600000.0;
    timeoutMs = requested >= INT_MAX ? INT_MAX : static_cast<int>(requested);
  }

  QElapsedTimer timer;
  timer.start();

  for (const auto& script : scripts) {
    if (context.search->isShuttingDown())
      return false;

    const int remaining = timeoutMs < 0
                          ? -1
                          : qMax(0, timeoutMs - static_cast<int>(qMin<qint64>(timer.elapsed(), INT_MAX)));
    if (remaining == 0)
      return false;

    // The command is run by a shell (remote) or by QProcess (local); quote the
    //   path so whitespaces in it cannot split the command.
    const QString executable = context.localRun
                               ? "\"" + script.executable + "\""
                               : Common::quoteRemotePath(script.executable);

    const QueueInterface::CommandResult result =
      context.queue->runACommand(context.workDir, executable, remaining);

    if (!result.succeeded()) {
      Common::error(QObject::tr("Failed to run the user script for %1 %2 for structure %3")
                                .arg(scriptKind).arg(script.displayIndex).arg(s->getTag()));
      return false;
    }
  }
  return true;
}

bool checkAndGetScriptOutputs(Structure* s, const ScriptCalculationContext& context,
                              const std::vector<ExternalScript>& scripts,
                              const QString& scriptKind)
{
  QStringList outputFiles;
  outputFiles.reserve(static_cast<int>(scripts.size()));

  for (const auto& script : scripts)
    outputFiles.append(script.outputFile);

  if (outputFiles.isEmpty())
    return true;

  QHash<QString, bool> existingFiles;
  if (!context.queue->checkIfFilesExist(s, outputFiles, &existingFiles))
    return false;

  for (const auto& script : scripts) {
    if (!existingFiles.value(script.outputFile, false))
      return false;
  }

  // A script may create its output file before writing it. Give the
  //   writer a short delay to finish before the files are read.
  QThread::msleep(OBJECTIVE_CHECK_MS);

  const QString lowerKind = scriptKind.toLower();

  for (const auto& script : scripts) {
    const QString localOutput = Common::localPath(context.localDir, script.outputFile);
    if (!context.localRun)
      QFile::remove(localOutput);

    // Report a failed copy of output files: it will be retried.
    if (!context.queue->copyFileFromExecutionHost(
          workPath(context, script.outputFile), localOutput)) {
      Common::error(QObject::tr("Failed to copy output for %1 %2 for structure %3 from remote!")
                                .arg(lowerKind).arg(script.displayIndex).arg(s->getTag()));
      return false;
    }
  }

  return true;
}

// Read the first field of a script output file. Constraints also accept
//   "true"/"false" input. A missing, unreadable, or non-finite value is not valid.
bool readScriptValue(const QString& filename, const QString& localDir,
                     bool acceptBoolean, double& value)
{
  bool valid = false;
  value = 0.0;

  QFile file(Common::localPath(localDir, filename));

  if (file.open(QIODevice::ReadOnly)) {
    QTextStream in(&file);
    const QString firstLine = in.readLine();
    const QStringList fields = firstLine.split(" ", QtCompat::SkipEmptyParts);
    if (!fields.isEmpty()) {
      const QString str = fields.at(0);
      if (acceptBoolean && str.compare("true", Qt::CaseInsensitive) == 0) {
        value = 1.0;
        valid = true;
      } else if (acceptBoolean && str.compare("false", Qt::CaseInsensitive) == 0) {
        value = 0.0;
        valid = true;
      } else {
        value = str.toDouble(&valid);
      }
    }
    file.close();
  }

  if (GS_ISNAN(value) || GS_ISINF(value)) {
    value = 0.0;
    valid = false;
  }

  return valid;
}

} // namespace

bool SearchBase::startConstraintCalculations(Structure* s)
{
  const ScriptCalculationContext context = calculationContext(this, s);

  std::vector<ExternalScript> scripts;
  scripts.reserve(getConstraintsNum());

  for (int i = 0; i < getConstraintsNum(); ++i)
    scripts.push_back(ExternalScript(i + 1, getConstraintExe(i), getConstraintOut(i)));

  Common::message(tr("Constraint calculations for %1 started.").arg(s->getTag()));

  const QString outputStructure = "output.POSCAR";
  if (!writeOutputPoscar(s, outputStructure)) {
    Common::error(tr("Failed writing output.POSCAR file for structure %1")
                     .arg(s->getTag()));
    return false;
  }

  if (!context.queue->copyFileToExecutionHost(Common::localPath(context.localDir, outputStructure),
        workPath(context, outputStructure))) {
    Common::error(tr("Failed to copy the output.POSCAR file for structure %1 to remote!")
                     .arg(s->getTag()));
    return false;
  }
  return runExternalScripts(s, context, scripts, tr("constraint"));
}

bool SearchBase::finishConstraintCalculations(Structure* s)
{
  const ScriptCalculationContext context = calculationContext(this, s);
  std::vector<ExternalScript> scripts;
  scripts.reserve(getConstraintsNum());
  for (int i = 0; i < getConstraintsNum(); ++i)
    scripts.push_back(ExternalScript(i + 1, getConstraintExe(i), getConstraintOut(i)));

  if (!checkAndGetScriptOutputs(s, context, scripts, tr("Constraint")))
    return false;

  int failedCount = 0;
  int dismissedCount = 0;
  QList<double> values;
  values.reserve(getConstraintsNum());
  for (int i = 0; i < getConstraintsNum(); i++) {
    double value = 0.0;
    const QString constraintFile = getConstraintOut(i);
    const bool valid = readScriptValue(constraintFile, context.localDir, true, value);
    values.append(value);
    if (!valid) {
      Common::error(tr("Failed to read any results from output file for constraint %1 for structure %2")
                       .arg(i + 1).arg(s->getTag()));
      failedCount += 1;
    } else if (value == 0.0) {
      dismissedCount += 1;
    }
  }

  QString constext;
  QWriteLocker structureLocker(&s->lock());
  s->setStrucConstraintValuesVec(values);
  if (failedCount == 0 && dismissedCount == 0) {
    s->setStrucConstraintState(Structure::Cs_Retain);
    constext = "retain";
  } else if (dismissedCount > 0) {
    // Check dismissed structures first; so they remain accessible for constraint redo (if any).
    s->setStrucConstraintState(Structure::Cs_Dismiss);
    constext = "dismiss";
  } else {
    s->setStrucConstraintState(Structure::Cs_Fail);
    constext = "fail";
  }
  structureLocker.unlock();

  Common::message(tr("Constraint calculations for %1 finished (status = %2).")
                     .arg(s->getTag()).arg(constext));

  return true;
}

bool SearchBase::startObjectiveCalculations(Structure* s)
{
  const ScriptCalculationContext context = calculationContext(this, s);
  std::vector<ExternalScript> scripts;
  scripts.reserve(getObjectivesNum());
  for (int i = 0; i < getObjectivesNum(); ++i) {
    if (objectiveNeedsExternalCalculation(i)) {
      scripts.push_back(ExternalScript(i + 1, getObjectivesExe(i), getObjectivesOut(i)));
    }
  }

  Common::message(tr("Objective calculations for %1 started.").arg(s->getTag()));

  const QString outputStructure = "output.POSCAR";
  if (!writeOutputPoscar(s, outputStructure)) {
    Common::error(tr("Failed writing output.POSCAR file for structure %1")
                     .arg(s->getTag()));
    return false;
  }

  if (!context.queue->copyFileToExecutionHost(Common::localPath(context.localDir, outputStructure),
        workPath(context, outputStructure))) {
    Common::error(tr("Failed to copy the output.POSCAR file for structure %1 to remote!")
                     .arg(s->getTag()));
    return false;
  }
  return runExternalScripts(s, context, scripts, tr("objective"));
}

bool SearchBase::finishObjectiveCalculations(Structure* s)
{
  const ScriptCalculationContext context = calculationContext(this, s);
  std::vector<ExternalScript> scripts;
  scripts.reserve(getObjectivesNum());
  for (int i = 0; i < getObjectivesNum(); ++i) {
    if (objectiveNeedsExternalCalculation(i)) {
      scripts.push_back(ExternalScript(i + 1, getObjectivesExe(i), getObjectivesOut(i)));
    }
  }

  if (!checkAndGetScriptOutputs(s, context, scripts, tr("Objective")))
    return false;

  QList<double> values;
  values.reserve(getObjectivesNum());
  // Set the values from the external objectives only.
  for (int i = 0; i < getObjectivesNum(); ++i)
    values.append(std::numeric_limits<double>::quiet_NaN());
  int failedCount = 0;
  for (int i = 0; i < getObjectivesNum(); i++) {
    if (!objectiveNeedsExternalCalculation(i))
      continue;

    double value = 0.0;
    const QString objectiveFile = getObjectivesOut(i);
    const bool valid = readScriptValue(objectiveFile, context.localDir, false, value);
    if (!valid) {
      Common::error(tr("Failed to read any results from output file for objective %1 for structure %2")
                       .arg(i + 1).arg(s->getTag()));
      failedCount += 1;
    }

    values[i] = value;
  }

  QString objctext;
  QWriteLocker structureLocker(&s->lock());
  s->setStrucObjValuesVec(values);
  if (failedCount == 0) {
    s->setStrucObjState(Structure::Os_Retain);
    objctext = "retain";
  } else {
    s->setStrucObjState(Structure::Os_Fail);
    objctext = "fail";
  }
  structureLocker.unlock();

  Common::message(tr("Objective calculations for %1 finished (status = %2).")
                     .arg(s->getTag()).arg(objctext));

  return true;
}

bool SearchBase::removeOldScriptOutputs(Structure* s, bool constraints)
{
  const ScriptCalculationContext context = calculationContext(this, s);
  std::vector<ExternalScript> scripts;
  if (constraints) {
    scripts.reserve(getConstraintsNum());
    for (int i = 0; i < getConstraintsNum(); ++i)
      scripts.push_back(ExternalScript(i + 1, getConstraintExe(i), getConstraintOut(i)));
  } else {
    scripts.reserve(getObjectivesNum());
    for (int i = 0; i < getObjectivesNum(); ++i) {
      if (objectiveNeedsExternalCalculation(i)) {
        scripts.push_back(ExternalScript(i + 1, getObjectivesExe(i), getObjectivesOut(i)));
      }
    }
  }
  return removeOldOutputFiles(s, context, scripts);
}

} // namespace Search

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

#include <common/timing.h>
#include <common/compatibility/platform_compat.h>
#include <common/compatibility/qt_compat.h>

#include <common/fileutils.h>
#include <common/output.h>
#include <search/queueinterface.h>
#include <search/structure.h>
#include <atoms/formats/poscarformat.h>

#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QObject>
#include <QStringList>
#include <QTextStream>
#include <QThreadPool>
#include <QtConcurrent>

#include <cmath>
#include <limits>
#include <string>
#include <vector>

namespace Search {

namespace {

bool isSafeOutputBasename(const QString& filename)
{
  const QString trimmed = filename.trimmed();
  if (trimmed.isEmpty() || trimmed != filename)
    return false;
  if (trimmed == "." || trimmed == "..")
    return false;
  if (trimmed.contains('/') || trimmed.contains('\\'))
    return false;
  if (trimmed.contains(QChar::fromLatin1('\0')))
    return false;
  return QFileInfo(trimmed).fileName() == trimmed;
}

bool usesLocalExecutionHost(const SearchBase* search, const QueueInterface* queue)
{
  // Direct-run jobs are always executed locallly.
  return queue->getIDString().toLower() == "none" || !search->isRemoteQueue();
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
  const std::string poscar = Atoms::PoscarFormat::writeToString(*s, s->getLocpath()).toStdString();
  if (poscar.empty())
    return false;

  QFile file(Common::localPath(s->getLocpath(), filename));
  if (file.open(QIODevice::WriteOnly | QIODevice::Text) &&
      file.write(poscar.c_str(), static_cast<qint64>(poscar.size())) ==
        static_cast<qint64>(poscar.size())) {
    file.close();
    return true;
  }
  return false;
}

void removeOldOutputFiles(Structure* s, const ScriptCalculationContext& context,
                          const std::vector<ExternalScript>& scripts)
{
  QStringList outputFiles;
  outputFiles.reserve(static_cast<int>(scripts.size()));
  for (const auto& script : scripts) {
    if (!isSafeOutputBasename(script.outputFile)) {
      Common::error(QObject::tr("Refusing unsafe output filename for structure %1: %2")
                      .arg(s->getTag())
                      .arg(script.outputFile));
      continue;
    }
    outputFiles.append(script.outputFile);
  }
  if (outputFiles.isEmpty())
    return;

  QHash<QString, bool> existingFiles;
  if (!context.queue->checkIfFilesExist(s, outputFiles, &existingFiles))
    return;

  for (const auto& script : scripts) {
    if (!isSafeOutputBasename(script.outputFile))
      continue;
    if (existingFiles.value(script.outputFile, false)) {
      if (!context.queue->removeAFile(s, script.outputFile))
        Common::error(QObject::tr("Failed to remove file %1!")
                        .arg(script.outputFile));
    }
  }
}

void runExternalScripts(Structure* s, const ScriptCalculationContext& context,
                        const std::vector<ExternalScript>& scripts, const QString& scriptKind)
{
  for (const auto& script : scripts) {
    const QueueInterface::CommandResult result =
      context.queue->runACommand(context.workDir, script.executable);
    if (!result.launched || result.exitCode != 0) {
      Common::error(QObject::tr("Failed to run the user script for %1 %2 for structure %3")
                      .arg(scriptKind)
                      .arg(script.displayIndex)
                      .arg(s->getTag()));
    }
  }
}

bool waitForOutputFiles(Structure* s, const ScriptCalculationContext& context,
                        const std::vector<ExternalScript>& scripts, const QString& scriptKind,
                        int queueRefreshInterval)
{
  bool ok = false;
  int waitCycles = 0;
  QStringList missingFiles;
  QStringList outputFiles;
  outputFiles.reserve(static_cast<int>(scripts.size()));
  for (const auto& script : scripts) {
    if (!isSafeOutputBasename(script.outputFile)) {
      // Check the output file name to be acceptable.
      Common::error(QObject::tr("Refusing unsafe output filename for structure %1: %2")
                      .arg(s->getTag())
                      .arg(script.outputFile));
      return false;
    }
    outputFiles.append(script.outputFile);
  }
  if (outputFiles.isEmpty())
    return false;

  while (!ok) {
    ok = true;
    missingFiles.clear();
    QHash<QString, bool> existingFiles;
    const bool checkedFiles = context.queue->checkIfFilesExist(s, outputFiles, &existingFiles);
    if (!checkedFiles) {
      ok = false;
      missingFiles = outputFiles;
    }
    if (checkedFiles) {
      for (const auto& script : scripts) {
        if (!existingFiles.value(script.outputFile, false)) {
          ok = false;
          missingFiles.append(script.outputFile);
        }
      }
    }
    if (!ok) {
      ++waitCycles;
      if (waitCycles % 120 == 0) {
        Common::debug(QObject::tr("%1 calculations for %2 waiting for output file(s): %3")
                        .arg(scriptKind)
                        .arg(s->getTag())
                        .arg(missingFiles.join(", ")));
      }
      if (waitCycles >= OBJECTIVE_WAIT_CYCLES) {
        Common::error(QObject::tr("%1 calculations for %2 timed out waiting for output file(s): %3")
                        .arg(scriptKind)
                        .arg(s->getTag())
                        .arg(missingFiles.join(", ")));
        return false;
      }
      // Wait one second at a time.
      const int sleepSeconds = qMax(1, queueRefreshInterval);
      for (int slept = 0; slept < sleepSeconds; ++slept) {
        if (context.search->isShuttingDown()) {
          Common::debug(QObject::tr("%1 calculations for %2 abandoned: engine is shutting down.")
                          .arg(scriptKind)
                          .arg(s->getTag()));
          return false;
        }
        GS_SLEEP(1);
      }
    }
  }
  return true;
}

bool waitForAndFetchScriptOutputs(Structure* s, const ScriptCalculationContext& context,
                                  const std::vector<ExternalScript>& scripts,
                                  const QString& scriptKind, int queueRefreshInterval)
{
  if (!waitForOutputFiles(s, context, scripts, scriptKind, queueRefreshInterval)) {
    return false;
  }

  const QString lowerKind = scriptKind.toLower();
  for (const auto& script : scripts) {
    const QString localOutput = Common::localPath(context.localDir, script.outputFile);
    if (!context.localRun)
      QFile::remove(localOutput);

    // Report a failed copy of output files.
    if (!context.queue->copyFileFromExecutionHost(
          workPath(context, script.outputFile), localOutput)) {
      Common::error(QObject::tr("Failed to copy output for %1 %2 for structure %3 from remote!")
                      .arg(lowerKind)
                      .arg(script.displayIndex)
                      .arg(s->getTag()));
    }
  }

  return true;
}

bool readObjectiveValue(const QString& filename, const QString& localDir, double& value)
{
  bool valid = false;
  value = 0.0;

  QFile file(Common::localPath(localDir, filename));
  if (file.open(QIODevice::ReadOnly)) {
    QTextStream in(&file);
    const QString firstLine = in.readLine();
    const QStringList fields = firstLine.split(" ", QtCompat::SkipEmptyParts);
    if (!fields.isEmpty())
      value = fields.at(0).toDouble(&valid);
    file.close();
  }

  if (GS_ISNAN(value) || GS_ISINF(value)) {
    value = 0.0;
    valid = false;
  }

  return valid;
}

bool readConstraintValue(const QString& filename, const QString& localDir, double& value)
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
      if (str.compare("true", Qt::CaseInsensitive) == 0) {
        value = 1.0;
        valid = true;
      } else if (str.compare("false", Qt::CaseInsensitive) == 0) {
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

void SearchBase::calculateConstraints(Structure* s)
{
  if (getConstraintsNum() == 0)
    return;

  Common::ScopedTimer _timer("SearchBase::calculateConstraints");

  (void)QtConcurrent::run(m_objectiveThreadPool.get(),
    [this, s]() { this->startConstraintCalculations(s); });
}

void SearchBase::startConstraintCalculations(Structure* s)
{
  const ScriptCalculationContext context = calculationContext(this, s);
  std::vector<ExternalScript> scripts;
  scripts.reserve(getConstraintsNum());
  for (int i = 0; i < getConstraintsNum(); ++i)
    scripts.push_back(ExternalScript(i + 1, getConstraintExe(i), getConstraintOut(i)));

  Common::message(tr("Constraint calculations for %1 started.").arg(s->getTag()));

  {
    QWriteLocker structureLocker(&s->lock());
    s->setStrucConstraintState(Structure::Cs_Fail);
  }

  const QString outputStructure = "output.POSCAR";
  if (!writeOutputPoscar(s, outputStructure)) {
    Common::error(tr("Failed writing output.POSCAR file for structure %1")
                    .arg(s->getTag()));
    emit doneWithConstraints(s);
    return;
  }

  removeOldOutputFiles(s, context, scripts);
  if (!context.queue->copyFileToExecutionHost(Common::localPath(context.localDir, outputStructure),
        workPath(context, outputStructure))) {
    Common::error(tr("Failed to copy the output.POSCAR file for structure %1 to remote!")
                    .arg(s->getTag()));
  }
  runExternalScripts(s, context, scripts, tr("constraint"));

  finishConstraintCalculations(s);
}

void SearchBase::finishConstraintCalculations(Structure* s)
{
  const ScriptCalculationContext context = calculationContext(this, s);
  std::vector<ExternalScript> scripts;
  scripts.reserve(getConstraintsNum());
  for (int i = 0; i < getConstraintsNum(); ++i)
    scripts.push_back(ExternalScript(i + 1, getConstraintExe(i), getConstraintOut(i)));

  if (!waitForAndFetchScriptOutputs(s, context, scripts, tr("Constraint"),
                                    queueRefreshInterval())) {
    emit doneWithConstraints(s);
    return;
  }

  int failedCount = 0;
  int dismissedCount = 0;
  QList<double> values;
  values.reserve(getConstraintsNum());
  for (int i = 0; i < getConstraintsNum(); i++) {
    double value = 0.0;
    const QString constraintFile = getConstraintOut(i);
    const bool valid = readConstraintValue(constraintFile, context.localDir, value);
    values.append(value);
    if (!valid) {
      Common::error(tr("Failed to read any results from output file for constraint %1 for structure %2")
              .arg(i + 1)
              .arg(s->getTag()));
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

  emit doneWithConstraints(s);
}

void SearchBase::calculateObjectives(Structure* s)
{
  // Objective calculations for each structure are handled in two steps:
  //   (1) startObjectiveCalculations writes output.POSCAR and runs the
  //       user scripts,
  //   (2) finishObjectiveCalculations waits for the output files, reads
  //       the values, updates the structure, and signals the finish.

  // Just start by checking if objective calculations are requested
  if (!hasExternalObjectiveCalculations())
    return;

  Common::ScopedTimer _timer("SearchBase::calculateObjectives");

  (void)QtConcurrent::run(m_objectiveThreadPool.get(),
    [this, s]() { this->startObjectiveCalculations(s); });
}

void SearchBase::startObjectiveCalculations(Structure* s)
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

  {
    QWriteLocker structureLocker(&s->lock());
    s->setStrucObjState(Structure::Os_Fail);
  }

  const QString outputStructure = "output.POSCAR";
  if (!writeOutputPoscar(s, outputStructure)) {
    Common::error(tr("Failed writing output.POSCAR file for structure %1")
                    .arg(s->getTag()));
    emit doneWithObjectives(s);
    return;
  }

  removeOldOutputFiles(s, context, scripts);
  if (!context.queue->copyFileToExecutionHost(Common::localPath(context.localDir, outputStructure),
        workPath(context, outputStructure))) {
    Common::error(tr("Failed to copy the output.POSCAR file for structure %1 to remote!")
                    .arg(s->getTag()));
  }
  runExternalScripts(s, context, scripts, tr("objective"));

  finishObjectiveCalculations(s);
}

void SearchBase::finishObjectiveCalculations(Structure* s)
{
  const ScriptCalculationContext context = calculationContext(this, s);
  std::vector<ExternalScript> scripts;
  scripts.reserve(getObjectivesNum());
  for (int i = 0; i < getObjectivesNum(); ++i) {
    if (objectiveNeedsExternalCalculation(i)) {
      scripts.push_back(ExternalScript(i + 1, getObjectivesExe(i), getObjectivesOut(i)));
    }
  }

  if (!waitForAndFetchScriptOutputs(s, context, scripts, tr("Objective"), queueRefreshInterval())) {
    emit doneWithObjectives(s);
    return;
  }

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
    const bool valid = readObjectiveValue(objectiveFile, context.localDir, value);
    if (!valid) {
      Common::error(tr("Failed to read any results from output file for objective %1 for structure %2")
              .arg(i + 1)
              .arg(s->getTag()));
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

  emit doneWithObjectives(s);
}

} // namespace Search

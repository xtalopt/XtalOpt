/**********************************************************************
  queue_command_compat - Old batch-queue command-key compatibility.

  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/legacy/queue_command_compat.h>

#include <QSettings>
#include <QVariant>

// Convert old queue commands.

namespace XtalOpt {
namespace Legacy {

namespace {

const int OriginalSearchStateVersion = 4;

struct LegacyBatchQueueDefinition
{
  // Queue id.
  const char* queueId;
  // Old settings group.
  const char* settingsGroup;
  const char* submitKey;
  const char* statusKey;
  const char* cancelKey;
};

const LegacyBatchQueueDefinition LegacyBatchQueues[] = {
  { "pbs", "pbsqueueinterface", "qsub", "qstat", "qdel" },
  { "sge", "sgequeueinterface", "qsub", "qstat", "qdel" },
  { "slurm", "slurmqueueinterface", "sbatch", "squeue", "scancel" },
  { "lsf", "lsfqueueinterface", "bsub", "bjobs", "bkill" },
  { "loadleveler", "loadlevelerqueueinterface", "llsubmit", "llq", "llcancel" }
};

void appendNote(QStringList& notes, const QString& note)
{
  QString value = note;
  value.replace('\n', "\\n");
  notes.append(value);
}

const LegacyBatchQueueDefinition* legacyBatchQueueDefinition(const QString& queueId)
{
  const QString normalized = queueId.trimmed().toLower();
  for (size_t i = 0; i < sizeof(LegacyBatchQueues) / sizeof(LegacyBatchQueues[0]); ++i) {
    if (normalized == LegacyBatchQueues[i].queueId)
      return &LegacyBatchQueues[i];
  }
  return 0;
}

void setCurrentBatchCommandIfMissing(QSettings& settings, const QString& key, const QVariant& value)
{
  if (!settings.contains(key) && value.isValid() && !value.toString().isEmpty())
    settings.setValue(key, value);
}

int legacyNumOptSteps(const QSettings& settings, const QString& root)
{
  int numOptSteps = settings.value(root + "/edit/numOptSteps", 1).toInt();
  return numOptSteps < 1 ? 1 : numOptSteps;
}

const LegacyBatchQueueDefinition* legacyStepQueueDefinition(const QSettings& settings,
                                                            const QString& root, int step)
{
  const QString queueId = settings.value(root + "/edit/queueInterface/" + QString::number(step),
                   "none").toString();
  return legacyBatchQueueDefinition(queueId);
}

// Return the current command group.
QString currentBatchCommandBase(const QString& root, int step,
                                const LegacyBatchQueueDefinition& definition)
{
  return root + "/optscheme/queue/" + QString::number(step) + "/commands/" + definition.queueId;
}

// Convert old per-step queue commands.
void normalizeLegacyPerStepBatchQueueCommands(QSettings& settings, const QString& searchId,
                                              QStringList& notes)
{
  const QString root = searchId.toLower();

  bool converted = false;
  for (int i = 0; i < legacyNumOptSteps(settings, root); ++i) {
    const LegacyBatchQueueDefinition* definition = legacyStepQueueDefinition(settings, root, i);
    if (!definition)
      continue;

    const QString legacyBase = root + "/queueinterface/" + definition->settingsGroup + "/" +
      QString::number(i) + "/paths/";
    const QString commandBase = currentBatchCommandBase(root, i, *definition);
    setCurrentBatchCommandIfMissing(settings, commandBase + "/submit",
                                    settings.value(legacyBase + definition->submitKey));
    setCurrentBatchCommandIfMissing(settings, commandBase + "/status",
                                    settings.value(legacyBase + definition->statusKey));
    setCurrentBatchCommandIfMissing(settings, commandBase + "/cancel",
                                    settings.value(legacyBase + definition->cancelKey));
    converted = true;
  }

  settings.remove(root + "/queueinterface");
  if (converted) {
    appendNote(notes,
               QString("converted legacy %1/queueinterface batch commands to "
                       "current queue command paths")
                 .arg(root));
  }
}

// Convert old shared queue commands.
void normalizeLegacyBatchQueueCommands(QSettings& settings, const QString& searchId,
                                       QStringList& notes)
{
  const QString root = searchId.toLower();
  const QString legacyBase = root + "/sys/queue";
  const QVariant legacySubmit = settings.value(legacyBase + "/qsub");
  const QVariant legacyStatus = settings.value(legacyBase + "/qstat");
  const QVariant legacyCancel = settings.value(legacyBase + "/qdel");

  if (!legacySubmit.isValid() && !legacyStatus.isValid() && !legacyCancel.isValid())
    return;

  bool converted = false;
  for (int i = 0; i < legacyNumOptSteps(settings, root); ++i) {
    const LegacyBatchQueueDefinition* definition = legacyStepQueueDefinition(settings, root, i);
    if (!definition)
      continue;
    const QString queueId = definition->queueId;
    if (queueId != "pbs" && queueId != "sge")
      continue;

    // Set the current queue commands.
    const QString commandBase = currentBatchCommandBase(root, i, *definition);
    setCurrentBatchCommandIfMissing(settings, commandBase + "/submit", legacySubmit);
    setCurrentBatchCommandIfMissing(settings, commandBase + "/status", legacyStatus);
    setCurrentBatchCommandIfMissing(settings, commandBase + "/cancel", legacyCancel);
    converted = true;
  }

  settings.remove(legacyBase);
  if (converted) {
    appendNote(notes,
               QString("converted legacy %1/sys/queue batch commands to "
                       "current queue command paths")
                 .arg(root));
  } else {
    appendNote(notes,
               QString("ignored legacy %1/sys/queue batch commands because "
                       "no PBS/SGE queue step is selected")
                 .arg(root));
  }
}

} // end anonymous namespace

void normalizeSearchState(QSettings& settings, const QString& searchId, int loadedVersion,
                          QStringList& notes)
{
  if (loadedVersion != OriginalSearchStateVersion)
    return;

  // Convert the old queue commands.
  normalizeLegacyPerStepBatchQueueCommands(settings, searchId, notes);
  normalizeLegacyBatchQueueCommands(settings, searchId, notes);
}

} // namespace Legacy
} // namespace XtalOpt

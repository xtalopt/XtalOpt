/**********************************************************************
  state_compat - Old XtalOpt state-file compatibility.

  Copyright (C) 2017 by Patrick S. Avery
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/xtalopt.h>

#include <common/fileutils.h>
#include <common/output.h>
#include <atoms/eleminfo.h>

#include <search/structure.h>
#include <xtalopt/legacy/input_compat.h>
#include <xtalopt/legacy/legacy_helpers.h>
#include <xtalopt/legacy/queue_command_compat.h>
#include <xtalopt/structures/xtal.h>

#include <QDir>
#include <QFile>
#include <QHash>
#include <QReadWriteLock>
#include <QSettings>
#include <QStringList>
#include <QTemporaryFile>
#include <QTextStream>
#include <QVariant>

#include <algorithm>
#include <limits>
#include <map>

// Convert old version=4 state files.

namespace XtalOpt {

using Search::Structure;

namespace {

using namespace Legacy;

int loadedXtalOptStateVersion(QSettings& settings)
{
  const int rootVersion = settings.value("xtalopt/version", 0).toInt();
  const int initVersion = settings.value("xtalopt/init/version", 0).toInt();
  const int editVersion = settings.value("xtalopt/edit/version", 0).toInt();

  if (rootVersion == FloorStateSchemaVersion || initVersion == FloorStateSchemaVersion ||
      editVersion == FloorStateSchemaVersion)
    return FloorStateSchemaVersion;

  if (rootVersion != 0)
    return rootVersion;
  if (initVersion != 0)
    return initVersion;
  if (editVersion != 0)
    return editVersion;

  return 0;
}

void markAsCurrentState(QSettings& settings)
{
  settings.setValue("xtalopt/version", CurrentStateSchemaVersion);
  settings.remove("xtalopt/init/version");
  settings.remove("xtalopt/edit/version");
}

QString noteValue(QString value)
{
  value.replace('\n', "\\n");
  return value;
}

void appendNote(QStringList& notes, const QString& note)
{
  notes.append(noteValue(note));
}

void normalizeEditSettings(QSettings& settings, QStringList& notes)
{
  settings.beginGroup("xtalopt/edit");
  const bool hasNumOptSteps = settings.contains("numOptSteps");
  const bool hasOldNumOptSteps = settings.contains("numOptimizationSteps");
  if (!hasNumOptSteps && hasOldNumOptSteps) {
    settings.setValue("numOptSteps", settings.value("numOptimizationSteps"));
    appendNote(notes, "replaced xtalopt/edit/numOptimizationSteps with "
               "xtalopt/edit/numOptSteps");
  } else if (hasNumOptSteps && hasOldNumOptSteps) {
    appendNote(notes, "ignored xtalopt/edit/numOptimizationSteps because "
               "xtalopt/edit/numOptSteps is present");
  }
  settings.remove("numOptimizationSteps");

  int numOptSteps = settings.value("numOptSteps", 1).toInt();
  if (numOptSteps < 1)
    numOptSteps = 1;

  if (settings.contains("queueInterface")) {
    appendNote(notes, "ignored unindexed xtalopt/edit/queueInterface entry");
    settings.remove("queueInterface");
  }

  for (int i = 0; i < numOptSteps; ++i) {
    const QString queueKey = "queueInterface/" + QString::number(i);
    QString queueInterface = settings.value(queueKey, "none").toString().trimmed().toLower();
    if (queueInterface == "local") {
      settings.setValue(queueKey, "none");
      appendNote(notes,
                 QString("replaced xtalopt/edit/%1=local with none")
                   .arg(queueKey));
    }

    const QString optimizerPrefix = "optimizer/" + QString::number(i);
    const QString currentKey = optimizerPrefix + "/directRunCommand";
    const QString oldKey = optimizerPrefix + "/exeLocation";
    const bool hasCurrentCommand = settings.contains(currentKey);
    const bool hasOldCommand = settings.contains(oldKey);
    if (!hasCurrentCommand && hasOldCommand) {
      settings.setValue(currentKey, settings.value(oldKey));
      appendNote(notes,
                 QString("replaced xtalopt/edit/%1 with xtalopt/edit/%2")
                   .arg(oldKey)
                   .arg(currentKey));
    } else if (hasCurrentCommand && hasOldCommand) {
      appendNote(notes,
                 QString("ignored xtalopt/edit/%1 because xtalopt/edit/%2 "
                         "is present")
                   .arg(oldKey)
                   .arg(currentKey));
    }
    settings.remove(oldKey);
  }

  settings.endGroup();
}

bool settingToBool(const QVariant& variant)
{
  const QString value = variant.toString().trimmed();
  return value.startsWith("t", Qt::CaseInsensitive) || value.toInt() != 0;
}

struct LegacyMolUnitConversion
{
  QStringList molUnitEntries;
  QHash<unsigned int, unsigned int> atomCounts;
};

void addAtomCount(QHash<unsigned int, unsigned int>& counts, unsigned int atomicNum,
                  unsigned int count)
{
  if (atomicNum == 0 || count == 0)
    return;
  counts[atomicNum] = counts.value(atomicNum, 0) + count;
}

bool convertLegacyMolUnit(const LegacyMolUnitFields& fields, LegacyMolUnitConversion& conversion,
                          QString& error)
{
  conversion.molUnitEntries.clear();
  conversion.atomCounts.clear();

  const bool noCenter = isNoCenterSymbol(fields.centerSymbol);
  unsigned int centerAtomicNum = 0;
  QString centerSymbol;
  if (!noCenter) {
    centerAtomicNum = Atoms::ElementInfo::getAtomicNum(fields.centerSymbol.trimmed().toStdString());
    if (centerAtomicNum == 0) {
      error = QString("invalid legacy compMolUnit center symbol: %1")
                .arg(fields.centerSymbol);
      return false;
    }
    centerSymbol = Atoms::ElementInfo::getAtomicSymbol(centerAtomicNum).c_str();
  }

  const unsigned int neighborAtomicNum = Atoms::ElementInfo::getAtomicNum(
      fields.neighborSymbol.trimmed().toStdString());
  if (neighborAtomicNum == 0) {
    error = QString("invalid legacy compMolUnit neighbor symbol: %1")
              .arg(fields.neighborSymbol);
    return false;
  }
  const QString neighborSymbol = Atoms::ElementInfo::getAtomicSymbol(neighborAtomicNum).c_str();

  if (fields.numCenters <= 0) {
    error = "legacy compMolUnit numCenters must be positive.";
    return false;
  }

  const LegacyMolUnitGeometry geometry = parseLegacyGeometry(fields.geometry);
  if (!geometryFitsNeighborCount(fields.numNeighbors, geometry)) {
    error = QString("invalid legacy compMolUnit geometry '%1' for " "numNeighbors=%2")
              .arg(fields.geometry)
              .arg(fields.numNeighbors);
    return false;
  }

  QString formula;
  QString templateName;
  if (noCenter) {
    if (fields.numNeighbors == 1 && geometry == LegacyGeomLinear) {
      // An old single-atom molunit won't be converted; only checked for composition!
      addAtomCount(conversion.atomCounts, neighborAtomicNum,
                   static_cast<unsigned int>(fields.numCenters));
      return true;
    }

    formula = formulaEntry(neighborSymbol, fields.numNeighbors);
    templateName = shellOnlyTemplate(fields.numNeighbors, geometry);
    addAtomCount(conversion.atomCounts, neighborAtomicNum,
                 static_cast<unsigned int>(fields.numNeighbors * fields.numCenters));
  } else if (centerAtomicNum == neighborAtomicNum) {
    const int atomCount = fields.numNeighbors + 1;
    formula = formulaEntry(neighborSymbol, atomCount);
    templateName = centeredHomonuclearTemplate(fields.numNeighbors, geometry);
    addAtomCount(conversion.atomCounts, neighborAtomicNum,
                 static_cast<unsigned int>(atomCount * fields.numCenters));
  } else {
    formula = formulaEntry(centerSymbol, 1) + formulaEntry(neighborSymbol, fields.numNeighbors);
    templateName = centeredHeteroTemplate(fields.numNeighbors, geometry);
    addAtomCount(conversion.atomCounts, centerAtomicNum,
                 static_cast<unsigned int>(fields.numCenters));
    addAtomCount(conversion.atomCounts, neighborAtomicNum,
                 static_cast<unsigned int>(fields.numNeighbors * fields.numCenters));
  }

  if (templateName.isEmpty()) {
    error = QString("unsupported legacy compMolUnit geometry '%1' for " "numNeighbors=%2")
              .arg(fields.geometry)
              .arg(fields.numNeighbors);
    return false;
  }

  const QString entry = QString("%1 %2").arg(formula).arg(templateName);
  for (int i = 0; i < fields.numCenters; ++i)
    conversion.molUnitEntries.append(entry);

  return true;
}

// Read an old composition.
CellComp compatFormulaToComposition(const QString& formula)
{
  CellComp composition;

  std::map<uint, uint> parsed;
  if (!Atoms::ElementInfo::readComposition(formula.toStdString(), parsed)) {
    return composition;
  }

  for (const auto& elem : parsed) {
    composition.setCompositionEntry(Atoms::ElementInfo::getAtomicSymbol(elem.first).c_str(),
      elem.first, elem.second);
  }

  return composition;
}

bool compatCompositionsAreIntegerMultiples(const CellComp& first, const CellComp& second)
{
  if (first.getNumTypes() == 0 || first.getNumTypes() != second.getNumTypes())
    return false;

  uint ratio = 0;
  const QList<uint> atomicNums = first.getCompositionAtomicNumbers();
  for (const auto& atomicNum : atomicNums) {
    const uint firstCount = first.getCount(atomicNum);
    const uint secondCount = second.getCount(atomicNum);
    if (firstCount == 0 || secondCount == 0 || secondCount % firstCount != 0) {
      return false;
    }

    const uint entryRatio = secondCount / firstCount;
    if (entryRatio == 0)
      return false;
    if (ratio == 0)
      ratio = entryRatio;
    else if (ratio != entryRatio)
      return false;
  }

  return true;
}

bool compatCompositionsHaveSameSystem(const QList<CellComp>& compositions)
{
  if (compositions.isEmpty())
    return false;

  const QList<uint> reference = compositions.first().getCompositionAtomicNumbers();
  for (int i = 1; i < compositions.size(); ++i) {
    if (compositions.at(i).getCompositionAtomicNumbers() != reference)
      return false;
  }

  return true;
}

bool compatInputCompositions(const QString& formulaString, QList<CellComp>& compositions)
{
  compositions.clear();

  const QStringList formulaList = formulaString.split(',');
  for (const auto& entry : formulaList) {
    QString formula = entry.simplified();
    formula.replace(" ", "");

    if (!formula.contains("-")) {
      const CellComp composition = compatFormulaToComposition(formula);
      if (composition.getNumTypes() == 0)
        return false;
      compositions.append(composition);
      continue;
    }

    const QStringList range = formula.split("-");
    if (range.size() != 2)
      return false;

    const CellComp first = compatFormulaToComposition(range.at(0));
    const CellComp second = compatFormulaToComposition(range.at(1));
    if (!compatCompositionsAreIntegerMultiples(first, second))
      return false;

    compositions.append(first);
  }

  return compatCompositionsHaveSameSystem(compositions);
}

CellComp compatMinimalComposition(const QList<CellComp>& compositions)
{
  CellComp minimal;

  if (compositions.isEmpty())
    return minimal;

  const QList<QString> symbols = compositions.first().getCompositionSymbols();
  for (const auto& symbol : symbols) {
    uint count = compositions.first().getCount(symbol);
    for (int i = 1; i < compositions.size(); ++i)
      count = std::min(count, compositions.at(i).getCount(symbol));

    minimal.setCompositionEntry(symbol, Atoms::ElementInfo::getAtomicNum(symbol.toStdString()),
      count);
  }

  return minimal;
}

bool validateLegacyMolUnitCounts(QSettings& settings,
                                 const QHash<unsigned int, unsigned int>& counts, QString& error)
{
  if (counts.isEmpty())
    return true;

  const QString formulaString =
    settings.value("xtalopt/init/chemical_formulas", QString()).toString();

  QList<CellComp> compositions;
  if (!compatInputCompositions(formulaString, compositions)) {
    error = "failed to read chemical_formulas while converting compMolUnit.";
    return false;
  }

  const CellComp minimal = compatMinimalComposition(compositions);
  for (auto it = counts.constBegin(), itEnd = counts.constEnd(); it != itEnd; ++it) {
    if (minimal.getCount(it.key()) >= it.value())
      continue;

    const QString symbol = Atoms::ElementInfo::getAtomicSymbol(it.key()).c_str();
    error = QString("invalid legacy compMolUnit settings: requested %1 "
                    "atoms of %2, but the minimum input composition contains " "%3.")
              .arg(it.value())
              .arg(symbol)
              .arg(minimal.getCount(it.key()));
    return false;
  }

  return true;
}

bool normalizeMoleculeUnits(QSettings& settings, QStringList& notes, QString& error)
{
  settings.beginGroup("xtalopt/init");

  const int currentMolUnitSize = settings.beginReadArray("molUnit");
  settings.endArray();
  const bool hasCurrentMolUnits = currentMolUnitSize > 0;
  const bool usingOldMolUnits = settingToBool(settings.value("using/molUnit", false));

  if (hasCurrentMolUnits) {
    if (settings.contains("using/molUnit")) {
      appendNote(notes, "ignored legacy compMolUnit data because current molUnit "
                 "entries are present");
    }
    settings.remove("compMolUnit");
    settings.remove("using/molUnit");
    settings.endGroup();
    return true;
  }

  if (!usingOldMolUnits) {
    if (settings.contains("using/molUnit")) {
      appendNote(notes, "ignored disabled legacy compMolUnit data");
    }
    settings.remove("compMolUnit");
    settings.remove("using/molUnit");
    settings.endGroup();
    return true;
  }

  QStringList convertedEntries;
  QHash<unsigned int, unsigned int> atomCounts;
  int droppedEntries = 0;
  const int legacyMolUnitSize = settings.beginReadArray("compMolUnit");
  for (int i = 0; i < legacyMolUnitSize; ++i) {
    settings.setArrayIndex(i);
    LegacyMolUnitFields fields;
    fields.centerSymbol = settings.value("center").toString();
    fields.numCenters = settings.value("number_of_centers").toInt();
    fields.neighborSymbol = settings.value("neighbor").toString();
    fields.numNeighbors = settings.value("number_of_neighbors").toInt();
    fields.geometry = settings.value("geometry").toString();

    LegacyMolUnitConversion conversion;
    if (!convertLegacyMolUnit(fields, conversion, error)) {
      settings.endArray();
      settings.endGroup();
      error = QString("failed to read legacy compMolUnit entry %1: %2")
                .arg(i)
                .arg(error);
      return false;
    }

    if (conversion.molUnitEntries.isEmpty())
      ++droppedEntries;
    convertedEntries.append(conversion.molUnitEntries);
    for (auto atomIt = conversion.atomCounts.constBegin(),
         atomItEnd = conversion.atomCounts.constEnd(); atomIt != atomItEnd; ++atomIt) {
      atomCounts[atomIt.key()] = atomCounts.value(atomIt.key(), 0) + atomIt.value();
    }
  }
  settings.endArray();
  settings.endGroup();

  if (!validateLegacyMolUnitCounts(settings, atomCounts, error))
    return false;

  settings.beginGroup("xtalopt/init");
  settings.remove("compMolUnit");
  settings.remove("using/molUnit");
  if (!convertedEntries.isEmpty()) {
    settings.beginWriteArray("molUnit");
    for (int i = 0; i < convertedEntries.size(); ++i) {
      settings.setArrayIndex(i);
      settings.setValue("entry", convertedEntries.at(i));
    }
    settings.endArray();
    appendNote(notes, "converted enabled legacy compMolUnit data to current " "molUnit entries");
  }
  if (droppedEntries > 0) {
    appendNote(notes, "dropped legacy compMolUnit entries with a single "
               "neighbor and no center (a lone atom needs no molUnit)");
  }
  settings.endGroup();
  return true;
}

QStringList chemicalSystemFromState(QSettings& settings)
{
  const QString formulaString =
    settings.value("xtalopt/init/chemical_formulas", QString()).toString();
  if (formulaString.trimmed().isEmpty())
    return QStringList();

  QList<CellComp> compositions;
  if (!compatInputCompositions(formulaString, compositions))
    return QStringList();

  QStringList symbols = compositions.first().getCompositionSymbols();
  std::sort(symbols.begin(), symbols.end());
  return symbols;
}

int currentNumOptSteps(QSettings& settings)
{
  int numOptSteps = settings.value("xtalopt/edit/numOptSteps", 1).toInt();
  if (numOptSteps < 1)
    numOptSteps = 1;
  return numOptSteps;
}

bool legacyPotcarAssetMap(const QString& potcarTemplate, const QStringList& symbols,
                          QString& normalized)
{
  if (potcarTemplate.trimmed().isEmpty())
    return false;

  QStringList entries;
  const QStringList lines = potcarTemplate.split('\n');
  for (const auto& line : lines) {
    const QString trimmed = line.trimmed();
    if (trimmed.isEmpty())
      continue;
    if (!trimmed.startsWith("%fileContents:") || !trimmed.endsWith("%") ||
        trimmed.mid(1, trimmed.size() - 2).indexOf('%') != -1) {
      return false;
    }
    entries.append(trimmed);
  }
  if (entries.isEmpty())
    return false;

  if (entries.size() == 1 || entries.size() != symbols.size()) {
    normalized = "system=" + entries.first().trimmed();
    return true;
  }

  QStringList parsedEntries;
  for (int i = 0; i < symbols.size() && i < entries.size(); ++i)
    parsedEntries.append(symbols.at(i) + "=" + entries.at(i).trimmed());
  normalized = parsedEntries.join("; ");
  return true;
}

void normalizeVaspPotcarAssets(QSettings& settings, QStringList& notes)
{
  const QStringList symbols = chemicalSystemFromState(settings);
  const int numOptSteps = currentNumOptSteps(settings);
  for (int i = 0; i < numOptSteps; ++i) {
    const QString optimizerStr =
      settings.value("xtalopt/edit/optimizer/" + QString::number(i), "gulp")
        .toString().trimmed();
    if (optimizerStr.compare("vasp", Qt::CaseInsensitive) != 0)
      continue;

    const QString optimizerGroup = optimizerStr.toLower();
    const QString potcarKey = QString("xtalopt/edit/optimizer/%1/%2/POTCAR")
        .arg(i)
        .arg(optimizerGroup);
    const QString potcarTemplate = settings.value(potcarKey).toString();

    QString normalized;
    if (!legacyPotcarAssetMap(potcarTemplate, symbols, normalized))
      continue;

    settings.setValue(potcarKey, normalized);
    appendNote(notes,
               QString("normalized legacy VASP POTCAR asset at %1")
                 .arg(potcarKey));
  }
}

void removeLegacyOptimizerSettings(QSettings& settings, QStringList& notes)
{
  settings.beginGroup("xtalopt");
  const bool hasLegacyCache = settings.childGroups().contains("optimizer");
  settings.endGroup();

  if (!hasLegacyCache)
    return;

  settings.remove("xtalopt/optimizer");
  appendNote(notes, "removed legacy xtalopt/optimizer saved state");
}

void normalizeRemoteQueue(QSettings& settings, QStringList& notes)
{
  settings.beginGroup("xtalopt/opt");
  const bool hasCurrent = settings.contains("opt/remoteQueue");
  const bool hasOld = settings.contains("opt/localQueue");
  if (!hasCurrent && hasOld) {
    settings.setValue("opt/remoteQueue", !settingToBool(settings.value("opt/localQueue")));
    appendNote(notes, "replaced xtalopt/opt/opt/localQueue with inverse "
               "xtalopt/opt/opt/remoteQueue");
  } else if (hasCurrent && hasOld) {
    appendNote(notes, "ignored xtalopt/opt/opt/localQueue because "
               "xtalopt/opt/opt/remoteQueue is present");
  }
  settings.remove("opt/localQueue");
  settings.endGroup();
}

struct StateObjectiveEntry
{
  QVariant typ;
  QString exe;
  QString out;
  QVariant wgt;
};

struct StateConstraintEntry
{
  QString exe;
  QString out;
};

QList<StateConstraintEntry> readConstraintEntries(QSettings& settings)
{
  QList<StateConstraintEntry> constraints;
  settings.beginGroup("xtalopt/obj");
  const int constraintSize = settings.beginReadArray("constraints");
  for (int i = 0; i < constraintSize; ++i) {
    settings.setArrayIndex(i);
    StateConstraintEntry entry;
    entry.exe = settings.value("exe", "").toString();
    entry.out = settings.value("out", "").toString();
    constraints.append(entry);
  }
  settings.endArray();
  settings.endGroup();
  return constraints;
}

void normalizeObjectiveSettings(QSettings& settings, QList<int>& constraintObjectiveIndices,
                                QStringList& notes)
{
  settings.beginGroup("xtalopt/obj");

  const bool hasCurrentRedo = settings.contains("constraintsReDo");
  const bool hasOldRedo = settings.contains("objectivesReDo");
  if (!hasCurrentRedo && hasOldRedo) {
    settings.setValue("constraintsReDo", settings.value("objectivesReDo"));
    appendNote(notes, "replaced xtalopt/obj/objectivesReDo with " "xtalopt/obj/constraintsReDo");
  } else if (hasCurrentRedo && hasOldRedo) {
    appendNote(notes, "ignored xtalopt/obj/objectivesReDo because "
               "xtalopt/obj/constraintsReDo is present");
  }
  settings.remove("objectivesReDo");

  QList<StateObjectiveEntry> currentObjectives;
  QList<StateConstraintEntry> legacyConstraints;
  const int objectiveSize = settings.beginReadArray("objectives");
  for (int i = 0; i < objectiveSize; ++i) {
    settings.setArrayIndex(i);
    StateObjectiveEntry entry;
    entry.typ = settings.value("typ", "");
    entry.exe = settings.value("exe", "").toString();
    entry.out = settings.value("out", "").toString();
    entry.wgt = settings.value("wgt", 0.0);

    bool okType = false;
    bool okWeight = false;
    const int typeValue = entry.typ.toInt(&okType);
    entry.wgt.toDouble(&okWeight);
    if (okType && okWeight && typeValue == 2) {
      StateConstraintEntry constraint;
      constraint.exe = entry.exe;
      constraint.out = entry.out;
      legacyConstraints.append(constraint);
      constraintObjectiveIndices.append(i);
      continue;
    }

    currentObjectives.append(entry);
  }
  settings.endArray();
  settings.endGroup();

  if (legacyConstraints.isEmpty())
    return;

  QList<StateConstraintEntry> allConstraints = legacyConstraints;
  allConstraints.append(readConstraintEntries(settings));

  settings.beginGroup("xtalopt/obj");
  settings.remove("objectives");
  settings.beginWriteArray("objectives");
  for (int i = 0; i < currentObjectives.size(); ++i) {
    settings.setArrayIndex(i);
    const StateObjectiveEntry& entry = currentObjectives.at(i);
    settings.setValue("typ", entry.typ);
    settings.setValue("exe", entry.exe);
    settings.setValue("out", entry.out);
    settings.setValue("wgt", entry.wgt);
  }
  settings.endArray();

  settings.remove("constraints");
  settings.beginWriteArray("constraints");
  for (int i = 0; i < allConstraints.size(); ++i) {
    settings.setArrayIndex(i);
    settings.setValue("exe", allConstraints.at(i).exe);
    settings.setValue("out", allConstraints.at(i).out);
  }
  settings.endArray();
  settings.endGroup();

  appendNote(notes, "converted legacy filtration objectives to current constraints");
}

void normalizeRandSpgCounts(QSettings& settings, QStringList& notes)
{
  settings.beginGroup("xtalopt/init");

  QList<int> denseCounts;
  const int randSpgSize = settings.beginReadArray("minXtalsOfSpg");
  if (randSpgSize > 0) {
    settings.setArrayIndex(0);
    if (!settings.contains("spg")) {
      for (int i = 0; i < randSpgSize; ++i) {
        settings.setArrayIndex(i);
        denseCounts.append(settings.value("count", 0).toInt());
      }
    }
  }
  settings.endArray();

  if (denseCounts.isEmpty()) {
    settings.endGroup();
    return;
  }

  settings.remove("minXtalsOfSpg");
  int written = 0;
  settings.beginWriteArray("minXtalsOfSpg");
  for (int i = 0; i < denseCounts.size(); ++i) {
    if (denseCounts.at(i) > 0) {
      settings.setArrayIndex(written++);
      settings.setValue("spg", i + 1);
      settings.setValue("count", denseCounts.at(i));
    }
  }
  settings.endArray();
  if (written == 0)
    settings.remove("minXtalsOfSpg");

  settings.endGroup();
  appendNote(notes, "converted dense xtalopt/init/minXtalsOfSpg array to sparse " "current format");
}

void copyValueIfPresent(QSettings& settings, const QString& oldKey, const QString& newKey)
{
  if (settings.contains(oldKey))
    settings.setValue(newKey, settings.value(oldKey));
}

// Convert old seed structures.
void convertSeedStructuresToScalar(QSettings& settings)
{
  settings.beginGroup("xtalopt/init");
  QStringList seeds;
  const int size = settings.beginReadArray("seedStructures");
  for (int i = 0; i < size; ++i) {
    settings.setArrayIndex(i);
    const QString path = settings.value("path").toString().trimmed();
    if (!path.isEmpty())
      seeds.append(path);
  }
  settings.endArray();
  settings.endGroup();
  if (!seeds.isEmpty())
    settings.setValue("xtalopt/input/seedStructures", seeds.join(","));
}

// Convert old forced space groups (v4 minXtalsOfSpg to current forcedSpgs comma string).
void convertForcedSpgsToScalar(QSettings& settings)
{
  settings.beginGroup("xtalopt/init");
  QStringList forced;
  const int size = settings.beginReadArray("minXtalsOfSpg");
  for (int i = 0; i < size; ++i) {
    settings.setArrayIndex(i);
    const int spg = settings.value("spg", 0).toInt();
    const int count = settings.value("count", 0).toInt();
    if (spg >= 1 && spg <= 230) {
      for (int n = 0; n < count; ++n)
        forced.append(QString::number(spg));
    }
  }
  settings.endArray();
  settings.endGroup();
  if (!forced.isEmpty())
    settings.setValue("xtalopt/input/forcedSpgs", forced.join(","));
}

void copySettingsGroup(QSettings& settings, const QString& oldGroup, const QString& newGroup)
{
  settings.beginGroup(oldGroup);
  const QStringList keys = settings.childKeys();
  const QStringList groups = settings.childGroups();
  QList<QVariant> values;
  for (const auto& key : keys)
    values.append(settings.value(key));
  settings.endGroup();

  for (int i = 0; i < keys.size(); ++i)
    settings.setValue(newGroup + "/" + keys.at(i), values.at(i));

  for (const auto& group : groups)
    copySettingsGroup(settings, oldGroup + "/" + group, newGroup + "/" + group);
}

// Write a v5 repeated keyword array.
void writeRepeatedEntryArray(QSettings& settings, const QString& name, const QStringList& entries)
{
  settings.remove("xtalopt/input/" + name);
  if (entries.isEmpty())
    return;
  settings.beginGroup("xtalopt/input");
  settings.beginWriteArray(name);
  for (int i = 0; i < entries.size(); ++i) {
    settings.setArrayIndex(i);
    settings.setValue("entry", entries.at(i));
  }
  settings.endArray();
  settings.endGroup();
}

// Convert old custom IAD values.
void convertCustomIADsToRepeated(QSettings& settings)
{
  settings.beginGroup("xtalopt/init");
  QStringList entries;
  const int size = settings.beginReadArray("customIAD");
  for (int i = 0; i < size; ++i) {
    settings.setArrayIndex(i);
    const int a1 = settings.value("atomicNumber1", 0).toInt();
    const int a2 = settings.value("atomicNumber2", 0).toInt();
    const double minIAD = settings.value("minInteratomicDist", 0.0).toDouble();
    if (a1 > 0 && a2 > 0) {
      entries.append(QString("%1, %2, %3")
                       .arg(Atoms::ElementInfo::getAtomicSymbol(a1).c_str())
                       .arg(Atoms::ElementInfo::getAtomicSymbol(a2).c_str())
                       .arg(minIAD));
    }
  }
  settings.endArray();
  settings.endGroup();
  writeRepeatedEntryArray(settings, "customIAD", entries);
}

QStringList optimizerAssetNames(const QString& optimizerId)
{
  const QString id = optimizerId.trimmed().toLower();
  if (id == "vasp")
    return QStringList() << "POTCAR";
  if (id == "siesta")
    return QStringList() << "PSF";
  return QStringList();
}

void copyOptimizerInputs(QSettings& settings, int optStep, const QString& optimizerId)
{
  const QString oldGroup = QString("xtalopt/edit/optimizer/%1/%2")
      .arg(optStep)
      .arg(optimizerId);
  const QString templateGroup = QString("xtalopt/optscheme/optimizer/%1/templates/%2")
      .arg(optStep)
      .arg(optimizerId);
  const QString assetGroup = QString("xtalopt/optscheme/optimizer/%1/assets/%2")
      .arg(optStep)
      .arg(optimizerId);
  const QStringList assets = optimizerAssetNames(optimizerId);

  settings.beginGroup(oldGroup);
  const QStringList keys = settings.childKeys();
  QList<QVariant> values;
  for (const auto& key : keys)
    values.append(settings.value(key));
  settings.endGroup();

  for (int i = 0; i < keys.size(); ++i) {
    const QString key = keys.at(i);
    const QString targetGroup =
      assets.contains(key, Qt::CaseInsensitive) ? assetGroup : templateGroup;
    settings.setValue(targetGroup + "/" + key, values.at(i));
  }
}

// Convert old objectives.
void convertObjectivesToRepeated(QSettings& settings)
{
  settings.beginGroup("xtalopt/obj");
  QStringList entries;
  const int size = settings.beginReadArray("objectives");
  for (int i = 0; i < size; ++i) {
    settings.setArrayIndex(i);
    const int typ = settings.value("typ", 0).toInt();
    const QString exe = settings.value("exe", "").toString();
    const QString out = settings.value("out", "").toString();
    const double wgt = settings.value("wgt", 0.0).toDouble();
    const QString type = (typ == 0) ? "min" : "max";
    entries.append(type + " " + exe + " " + out + " " + QString::number(wgt));
  }
  settings.endArray();
  settings.endGroup();
  writeRepeatedEntryArray(settings, "objective", entries);
}

// Convert old constraints.
void convertConstraintsToRepeated(QSettings& settings)
{
  settings.beginGroup("xtalopt/obj");
  QStringList entries;
  const int size = settings.beginReadArray("constraints");
  for (int i = 0; i < size; ++i) {
    settings.setArrayIndex(i);
    const QString exe = settings.value("exe", "").toString();
    const QString out = settings.value("out", "").toString();
    entries.append(exe + " " + out);
  }
  settings.endArray();
  settings.endGroup();
  writeRepeatedEntryArray(settings, "constraint", entries);
}

// Convert the old state layout.
void convertVersion4LayoutToCurrent(QSettings& settings, QStringList& notes)
{
  // init/* scalars -> scalar/<keyword>
  copyValueIfPresent(settings, "xtalopt/init/vcSearch", "xtalopt/input/vcSearch");
  copyValueIfPresent(settings, "xtalopt/init/using/randSpg", "xtalopt/input/usingRandSpg");
  copyValueIfPresent(settings, "xtalopt/init/minAtoms", "xtalopt/input/minAtoms");
  copyValueIfPresent(settings, "xtalopt/init/maxAtoms", "xtalopt/input/maxAtoms");
  copyValueIfPresent(settings, "xtalopt/init/limits/a/min", "xtalopt/input/aMin");
  copyValueIfPresent(settings, "xtalopt/init/limits/a/max", "xtalopt/input/aMax");
  copyValueIfPresent(settings, "xtalopt/init/limits/b/min", "xtalopt/input/bMin");
  copyValueIfPresent(settings, "xtalopt/init/limits/b/max", "xtalopt/input/bMax");
  copyValueIfPresent(settings, "xtalopt/init/limits/c/min", "xtalopt/input/cMin");
  copyValueIfPresent(settings, "xtalopt/init/limits/c/max", "xtalopt/input/cMax");
  copyValueIfPresent(settings, "xtalopt/init/limits/alpha/min", "xtalopt/input/alphaMin");
  copyValueIfPresent(settings, "xtalopt/init/limits/alpha/max", "xtalopt/input/alphaMax");
  copyValueIfPresent(settings, "xtalopt/init/limits/beta/min", "xtalopt/input/betaMin");
  copyValueIfPresent(settings, "xtalopt/init/limits/beta/max", "xtalopt/input/betaMax");
  copyValueIfPresent(settings, "xtalopt/init/limits/gamma/min", "xtalopt/input/gammaMin");
  copyValueIfPresent(settings, "xtalopt/init/limits/gamma/max", "xtalopt/input/gammaMax");
  copyValueIfPresent(settings, "xtalopt/init/limits/volume/min", "xtalopt/input/minVolume");
  copyValueIfPresent(settings, "xtalopt/init/limits/volume/max", "xtalopt/input/maxVolume");
  copyValueIfPresent(settings, "xtalopt/init/limits/volume/scale_min",
                     "xtalopt/input/minVolumeScale");
  copyValueIfPresent(settings, "xtalopt/init/limits/volume/scale_max",
                     "xtalopt/input/maxVolumeScale");
  copyValueIfPresent(settings, "xtalopt/init/using/interatomicDistanceLimit",
                     "xtalopt/input/usingScaledIADs");
  copyValueIfPresent(settings, "xtalopt/init/limits/scaleFactor",
                     "xtalopt/input/radiiScalingFactor");
  copyValueIfPresent(settings, "xtalopt/init/limits/minRadius", "xtalopt/input/minRadius");
  copyValueIfPresent(settings, "xtalopt/init/using/customIAD", "xtalopt/input/usingCustomIADs");
  copyValueIfPresent(settings, "xtalopt/init/using/checkStepOpt",
                     "xtalopt/input/checkIADPostOptimization");
  copyValueIfPresent(settings, "xtalopt/init/saveHullSnapshots", "xtalopt/input/saveHullSnapshots");
  copyValueIfPresent(settings, "xtalopt/init/verboseOutput", "xtalopt/input/verboseOutput");

  // Convert the input values.
  copyValueIfPresent(settings, "xtalopt/init/chemical_formulas", "xtalopt/input/chemicalFormulas");
  copyValueIfPresent(settings, "xtalopt/init/referenceEnergies", "xtalopt/input/referenceEnergies");
  copyValueIfPresent(settings, "xtalopt/init/limits/volume/elemental",
                     "xtalopt/input/elementalVolumes");
  convertSeedStructuresToScalar(settings);
  convertForcedSpgsToScalar(settings);

  // Convert values with more than one line.
  copySettingsGroup(settings, "xtalopt/init/molUnit", "xtalopt/input/molUnit");
  convertCustomIADsToRepeated(settings);

  // edit/* scalars -> scalar/<keyword>
  copyValueIfPresent(settings, "xtalopt/edit/locWorkDir", "xtalopt/input/localWorkingDirectory");
  copyValueIfPresent(settings, "xtalopt/edit/description", "xtalopt/input/description");
  copyValueIfPresent(settings, "xtalopt/edit/logErrorDirs", "xtalopt/input/logErrorDirectories");
  copyValueIfPresent(settings, "xtalopt/edit/user1", "xtalopt/input/user1");
  copyValueIfPresent(settings, "xtalopt/edit/user2", "xtalopt/input/user2");
  copyValueIfPresent(settings, "xtalopt/edit/user3", "xtalopt/input/user3");
  copyValueIfPresent(settings, "xtalopt/edit/user4", "xtalopt/input/user4");
  copyValueIfPresent(settings, "xtalopt/edit/remote/host", "xtalopt/input/host");
  copyValueIfPresent(settings, "xtalopt/edit/remote/port", "xtalopt/input/port");
  copyValueIfPresent(settings, "xtalopt/edit/remote/username", "xtalopt/input/user");
  copyValueIfPresent(settings, "xtalopt/edit/remote/sshMethod", "xtalopt/input/sshMethod");
  copyValueIfPresent(settings, "xtalopt/edit/remote/remWorkDir",
                     "xtalopt/input/remoteWorkingDirectory");
  copyValueIfPresent(settings, "xtalopt/edit/remote/cancelJobAfterTime",
                     "xtalopt/input/autoCancelJobAfterTime");
  copyValueIfPresent(settings, "xtalopt/edit/remote/hoursForCancelJobAfterTime",
                     "xtalopt/input/hoursForAutoCancelJob");
  copyValueIfPresent(settings, "xtalopt/edit/remote/queueRefreshInterval",
                     "xtalopt/input/queueRefreshInterval");
  copyValueIfPresent(settings, "xtalopt/edit/remote/cleanRemoteOnStop",
                     "xtalopt/input/cleanRemoteDirs");

  // Convert the number of optimization steps.
  copyValueIfPresent(settings, "xtalopt/edit/numOptSteps", "xtalopt/optscheme/numOptSteps");

  const int numOptSteps = currentNumOptSteps(settings);
  for (int i = 0; i < numOptSteps; ++i) {
    const QString step = QString::number(i);
    const QString queueId = settings.value("xtalopt/edit/queueInterface/" + step, "none")
        .toString().trimmed().toLower();
    settings.setValue("xtalopt/optscheme/queue/" + step + "/interface", queueId);
    copySettingsGroup(settings, "xtalopt/edit/queueInterface/" + step + "/" + queueId,
                      "xtalopt/optscheme/queue/" + step + "/templates/" + queueId);

    const QString optimizerId = settings.value("xtalopt/edit/optimizer/" + step, "gulp")
        .toString().trimmed().toLower();
    settings.setValue("xtalopt/optscheme/optimizer/" + step + "/interface", optimizerId);
    copyValueIfPresent(settings, "xtalopt/edit/optimizer/" + step + "/directRunCommand",
                       "xtalopt/optscheme/optimizer/" + step + "/directRunCommand");
    copyOptimizerInputs(settings, i, optimizerId);
  }

  // opt/opt/* run parameters -> scalar/<keyword>
  copyValueIfPresent(settings, "xtalopt/opt/opt/p_supercell", "xtalopt/input/randomSuperCell");
  copyValueIfPresent(settings, "xtalopt/opt/opt/numInitial", "xtalopt/input/numInitial");
  copyValueIfPresent(settings, "xtalopt/opt/opt/parentsPoolSize", "xtalopt/input/parentsPoolSize");
  copyValueIfPresent(settings, "xtalopt/opt/opt/limitRunningJobs",
                     "xtalopt/input/limitRunningJobs");
  copyValueIfPresent(settings, "xtalopt/opt/opt/runningJobLimit", "xtalopt/input/runningJobLimit");
  copyValueIfPresent(settings, "xtalopt/opt/opt/contStructs", "xtalopt/input/continuousStructures");
  copyValueIfPresent(settings, "xtalopt/opt/opt/failLimit", "xtalopt/input/jobFailLimit");
  copyValueIfPresent(settings, "xtalopt/opt/opt/maxNumStructures",
                     "xtalopt/input/maxNumStructures");
  copyValueIfPresent(settings, "xtalopt/opt/opt/p_atomic", "xtalopt/input/weightPermutomic");
  copyValueIfPresent(settings, "xtalopt/opt/opt/p_comp", "xtalopt/input/weightPermucomp");
  copyValueIfPresent(settings, "xtalopt/opt/opt/p_strip", "xtalopt/input/weightStripple");
  copyValueIfPresent(settings, "xtalopt/opt/opt/p_perm", "xtalopt/input/weightPermustrain");
  copyValueIfPresent(settings, "xtalopt/opt/opt/p_cross", "xtalopt/input/weightCrossover");
  copyValueIfPresent(settings, "xtalopt/opt/opt/strip_amp_min",
                     "xtalopt/input/strippleAmplitudeMin");
  copyValueIfPresent(settings, "xtalopt/opt/opt/strip_amp_max",
                     "xtalopt/input/strippleAmplitudeMax");
  copyValueIfPresent(settings, "xtalopt/opt/opt/strip_per1", "xtalopt/input/strippleNumWavesAxis1");
  copyValueIfPresent(settings, "xtalopt/opt/opt/strip_per2", "xtalopt/input/strippleNumWavesAxis2");
  copyValueIfPresent(settings, "xtalopt/opt/opt/strip_strainStdev_min",
                     "xtalopt/input/strippleStrainStdevMin");
  copyValueIfPresent(settings, "xtalopt/opt/opt/strip_strainStdev_max",
                     "xtalopt/input/strippleStrainStdevMax");
  copyValueIfPresent(settings, "xtalopt/opt/opt/perm_ex", "xtalopt/input/permustrainNumExchanges");
  copyValueIfPresent(settings, "xtalopt/opt/opt/perm_strainStdev_max",
                     "xtalopt/input/permustrainStrainStdevMax");
  copyValueIfPresent(settings, "xtalopt/opt/opt/cross_ncuts", "xtalopt/input/crossoverCuts");
  copyValueIfPresent(settings, "xtalopt/opt/opt/cross_minimumContribution",
                     "xtalopt/input/crossoverMinContribution");
  copyValueIfPresent(settings, "xtalopt/opt/opt/softExit", "xtalopt/input/softExit");
  copyValueIfPresent(settings, "xtalopt/opt/opt/remoteQueue", "xtalopt/input/remoteQueue");
  copyValueIfPresent(settings, "xtalopt/opt/opt/saveHullSnapshots",
                     "xtalopt/input/saveHullSnapshots");

  // Convert the job failure action.
  if (settings.contains("xtalopt/opt/opt/failAction")) {
    const int failAction = settings.value("xtalopt/opt/opt/failAction").toInt();
    QString name = "replaceWithRandom";
    if (failAction == 0)
      name = "keepTrying";
    else if (failAction == 1)
      name = "kill";
    else if (failAction == 3)
      name = "replaceWithOffspring";
    settings.setValue("xtalopt/input/jobFailAction", name);
  }

  // opt/tol/* tolerances -> scalar/<keyword>
  copyValueIfPresent(settings, "xtalopt/opt/tol/rdf/tolerance", "xtalopt/input/rdfTolerance");
  copyValueIfPresent(settings, "xtalopt/opt/tol/rdf/cutoff", "xtalopt/input/rdfCutoff");
  copyValueIfPresent(settings, "xtalopt/opt/tol/rdf/nbins", "xtalopt/input/rdfNumBins");
  copyValueIfPresent(settings, "xtalopt/opt/tol/rdf/sigma", "xtalopt/input/rdfSigma");
  copyValueIfPresent(settings, "xtalopt/opt/tol/xtalcomp/length",
                     "xtalopt/input/xtalcompToleranceLength");
  copyValueIfPresent(settings, "xtalopt/opt/tol/xtalcomp/angle",
                     "xtalopt/input/xtalcompToleranceAngle");
  copyValueIfPresent(settings, "xtalopt/opt/tol/spg", "xtalopt/input/spglibTolerance");

  // obj/* fitness parameters -> scalar/<keyword>, repeated -> repeated/
  copyValueIfPresent(settings, "xtalopt/obj/optimizationType", "xtalopt/input/optimizationType");
  copyValueIfPresent(settings, "xtalopt/obj/tournamentSelection",
                     "xtalopt/input/tournamentSelection");
  copyValueIfPresent(settings, "xtalopt/obj/restrictedPool", "xtalopt/input/restrictedPool");
  copyValueIfPresent(settings, "xtalopt/obj/crowdingDistance", "xtalopt/input/crowdingDistance");
  copyValueIfPresent(settings, "xtalopt/obj/paretoFilterZeroWeights",
                     "xtalopt/input/paretoFilterZeroWeights");
  copyValueIfPresent(settings, "xtalopt/obj/objectivePrecision",
                     "xtalopt/input/objectivePrecision");
  copyValueIfPresent(settings, "xtalopt/obj/constraintsReDo", "xtalopt/input/constraintsReDo");
  convertObjectivesToRepeated(settings);
  convertConstraintsToRepeated(settings);

  // Remove old settings.
  settings.beginGroup("xtalopt");
  const QStringList keepGroups = { "input", "optscheme" };
  const QStringList keepKeys = { "version", "saveSuccessful" };
  for (const QString& group : settings.childGroups()) {
    if (!keepGroups.contains(group))
      settings.remove(group);
  }
  for (const QString& key : settings.childKeys()) {
    if (!keepKeys.contains(key))
      settings.remove(key);
  }
  settings.endGroup();

  appendNote(notes, "converted original v4 state layout to current v5 settings");
}

bool rewriteVersion4XtalOptStateFile(QSettings& settings, bool fullState, int loadedVersion,
                                      QList<int>& constraintObjectiveIndices, QStringList& notes,
                                      QString& error)
{
  markAsCurrentState(settings);
  normalizeEditSettings(settings, notes);
  Legacy::normalizeLegacySearchState(settings, "xtalopt", loadedVersion, notes);

  if (fullState) {
    if (!normalizeMoleculeUnits(settings, notes, error))
      return false;
    normalizeVaspPotcarAssets(settings, notes);
    removeLegacyOptimizerSettings(settings, notes);
    normalizeRemoteQueue(settings, notes);
    normalizeObjectiveSettings(settings, constraintObjectiveIndices, notes);
    normalizeRandSpgCounts(settings, notes);
  }

  convertVersion4LayoutToCurrent(settings, notes);
  return true;
}

bool appendCompatibilityNotes(const QString& filename, const QStringList& notes)
{
  QFile file(filename);
  if (!file.open(QIODevice::Append | QIODevice::Text))
    return false;

  QTextStream out(&file);
  out << "\n# XtalOpt state compatibility notes\n";
  out << "# This file was generated while reading an old version 4 "
         "XtalOpt state file.\n";
  out << "# It is a read-side copy only; active runs save to the original "
         "state file.\n";
  for (const auto& note : notes)
    out << "# " << note << "\n";
  return true;
}

int structureStateVersion(const QString& filename)
{
  // Return the structure state version.
  QSettings settings(filename, QSettings::IniFormat);
  settings.beginGroup("structure");
  const int version = settings.value("version", -1).toInt();
  settings.endGroup();
  return version;
}

void normalizeStatusAfterLegacyConstraintMove(Structure* structure)
{
  if (structure->getStatus() == Structure::ObjcFailed &&
      structure->getStrucConstraintState() == Structure::Cs_Fail)
    structure->setStatus(Structure::ConsFailed);
}

bool splitVersion4ObjectiveValues(const QList<double>& loadedValues,
                                  int currentFullCount, int currentUserCount,
                                  const QList<int>& constraintObjectiveIndices,
                                  QList<double>& objectiveValues,
                                  QList<double>* constraintValues = nullptr)
{
  const int movedObjectiveCount = constraintObjectiveIndices.size();
  const bool valuesIncludeBuiltinObjective =
    loadedValues.size() == currentFullCount + movedObjectiveCount;
  const bool valuesAreUserObjectivesOnly =
    loadedValues.size() == currentUserCount + movedObjectiveCount;
  if (!valuesIncludeBuiltinObjective && !valuesAreUserObjectivesOnly)
    return false;

  objectiveValues.clear();
  objectiveValues.reserve(valuesIncludeBuiltinObjective ? currentFullCount : currentUserCount);
  if (constraintValues) {
    constraintValues->clear();
    constraintValues->reserve(movedObjectiveCount);
  }

  for (int i = 0; i < loadedValues.size(); ++i) {
    const int loadedUserObjectiveIndex = valuesIncludeBuiltinObjective ? i - XtalOpt::getFirstUserObjectiveIndex() : i;
    if (loadedUserObjectiveIndex >= 0 &&
        constraintObjectiveIndices.contains(loadedUserObjectiveIndex)) {
      if (constraintValues)
        constraintValues->append(loadedValues.at(i));
      continue;
    }
    objectiveValues.append(loadedValues.at(i));
  }

  return objectiveValues.size() ==
           (valuesIncludeBuiltinObjective ? currentFullCount : currentUserCount) &&
           (!constraintValues || constraintValues->size() == movedObjectiveCount);
}

} // end anonymous namespace

bool XtalOpt::prepareXtalOptStateFileForRead(const QString& filename, bool fullState,
                                             QString& readFilename,
                                             bool keepCompatibilityCopy)
{
  readFilename = filename;
  x_loadedStateConstraintObjectiveIndices.clear();
  x_loadedVersion4State = false;

  QSettings originalSettings(filename, QSettings::IniFormat);
  const int loadedVersion = loadedXtalOptStateVersion(originalSettings);

  // Read the current version or convert version 4.
  if (loadedVersion == CurrentStateSchemaVersion)
    return true; // already current; read in place
  if (loadedVersion != FloorStateSchemaVersion) {
    Common::error(QString("%1: %2 is schema version %3; this XtalOpt reads "
                          "version %4 and converts version %5. Older sessions "
                          "cannot be resumed.")
                    .arg(__func__)
                    .arg(filename)
                    .arg(loadedVersion)
                    .arg(CurrentStateSchemaVersion)
                    .arg(FloorStateSchemaVersion));
    return false;
  }

  const bool temporaryCopy = isReadOnly() && !keepCompatibilityCopy;
  QString compatFilename;
  if (temporaryCopy) {
    QFile sourceFile(filename);
    QTemporaryFile temporaryFile(
      QDir(QDir::tempPath()).filePath("xtalopt-state-XXXXXX.compat"));
    if (!sourceFile.open(QIODevice::ReadOnly) || !temporaryFile.open()) {
      Common::error(QString("%1: could not prepare a temporary compatibility state copy.")
                      .arg(__func__));
      return false;
    }

    const QByteArray contents = sourceFile.readAll();
    if (temporaryFile.write(contents) != contents.size() ||
        !temporaryFile.flush()) {
      Common::error(QString("%1: could not write temporary compatibility state copy.")
                      .arg(__func__));
      return false;
    }
    temporaryFile.setAutoRemove(false);
    compatFilename = temporaryFile.fileName();
    temporaryFile.close();
  } else {
    compatFilename = filename + ".compat";
    if (QFile::exists(compatFilename) && !QFile::remove(compatFilename)) {
      Common::error(QString("%1: could not replace compatibility state copy %2.")
                      .arg(__func__)
                      .arg(compatFilename));
      return false;
    }

    if (!QFile::copy(filename, compatFilename)) {
      Common::error(QString("%1: could not write compatibility state copy %2.")
                      .arg(__func__)
                      .arg(compatFilename));
      return false;
    }
  }

  QStringList notes;
  QString error;
  bool converted = false;
  {
    QSettings compatSettings(compatFilename, QSettings::IniFormat);
    if (!rewriteVersion4XtalOptStateFile(compatSettings, fullState, loadedVersion,
          x_loadedStateConstraintObjectiveIndices, notes, error)) {
      Common::error(QString("%1: %2").arg(__func__).arg(error));
    } else {
      compatSettings.sync();
      if (compatSettings.status() != QSettings::NoError) {
        Common::error(QString("%1: failed to sync compatibility state copy %2.")
                        .arg(__func__)
                        .arg(compatFilename));
      } else {
        converted = true;
      }
    }
  }

  if (!converted) {
    if (temporaryCopy)
      QFile::remove(compatFilename);
    return false;
  }

  if (!temporaryCopy && !appendCompatibilityNotes(compatFilename, notes)) {
    Common::warning(QString("%1: could not append compatibility notes to %2.")
                      .arg(__func__)
                      .arg(compatFilename));
  }

  x_loadedVersion4State = true;
  readFilename = compatFilename;
  return true;
}

bool XtalOpt::normalizeLoadedStructureObjectives(Structure* structure, const QString& stateFilename) const
{
  if (!structure)
    return false;

  const int loadedVersion = structureStateVersion(stateFilename);
  if (loadedVersion != FloorStateSchemaVersion)
    return true;

  if (!x_loadedVersion4State) {
    Common::error(QString("The old structure state file %1 cannot be read with the current "
                          "main state file. Restore its matching version 4 xtalopt.state file.")
                          .arg(stateFilename));
    return false;
  }

  QList<double> legacyValues;
  int legacyConstraintRedoCount = 0;
  {
    QSettings settings(stateFilename, QSettings::IniFormat);
    settings.beginGroup("structure");
    legacyConstraintRedoCount = settings.value("objectivesFailCount", 0).toInt();

    int size = settings.beginReadArray("objectives");
    for (int i = 0; i < size; ++i) {
      settings.setArrayIndex(i);
      legacyValues.append(settings.value("value").toDouble());
    }
    settings.endArray();
    settings.endGroup();
  }

  QWriteLocker locker(&structure->lock());
  structure->setStrucConstraintRedoCount(legacyConstraintRedoCount);
  QList<double> values = legacyValues.isEmpty() ? structure->getStrucObjValuesVec() : legacyValues;
  if (!legacyValues.isEmpty())
    structure->setStrucObjValuesVec(values);

  const int currentFullCount = getObjectivesNum();
  const int currentUserCount = getUserObjectivesNum();

  if (!x_loadedStateConstraintObjectiveIndices.isEmpty() && !legacyValues.isEmpty()) {
    QList<double> normalized;
    QList<double> constraintValues;
    if (!splitVersion4ObjectiveValues(values, currentFullCount, currentUserCount,
                                      x_loadedStateConstraintObjectiveIndices,
                                      normalized, &constraintValues)) {
      Common::error(QString("The objective information in old structure state file %1 does "
                            "not match the main state file.").arg(stateFilename));
      return false;
    }

    structure->setStrucObjValuesVec(normalized);
    structure->setStrucConstraintValuesVec(constraintValues);
    values = normalized;
    if (structure->getStrucConstraintState() == Structure::Cs_NotCalculated) {
      if (structure->getStrucObjState() == Structure::Os_Dismiss)
        structure->setStrucConstraintState(Structure::Cs_Dismiss);
      else if (structure->getStrucObjState() == Structure::Os_Fail)
        structure->setStrucConstraintState(Structure::Cs_Fail);
      else
        structure->setStrucConstraintState(Structure::Cs_Retain);
      normalizeStatusAfterLegacyConstraintMove(structure);
    }
  }

  if (!legacyValues.isEmpty() && values.size() != currentFullCount &&
      values.size() != currentUserCount) {
    Common::error(QString("The objective information in old structure state file %1 does "
                          "not match the main state file.").arg(stateFilename));
    return false;
  }

  // Add the built-in objective value.
  if (currentFullCount > currentUserCount && values.size() == currentUserCount) {
    QList<double> expanded;
    expanded.reserve(currentFullCount);
    for (int i = 0; i < currentFullCount; ++i)
      expanded.append(std::numeric_limits<double>::quiet_NaN());
    for (int i = 0; i < currentUserCount; ++i)
      expanded[getUserObjectiveIndex(i)] = values.at(i);
    structure->setStrucObjValuesVec(expanded);
  } else if (values.size() == currentFullCount && currentFullCount > currentUserCount) {
    values[getBuiltinObjectiveIndex()] = std::numeric_limits<double>::quiet_NaN();
    structure->setStrucObjValuesVec(values);
  }

  return true;
}

bool XtalOpt::convertFileToCurrent(const QString& filename)
{
  if (filename.isEmpty()) {
    Common::error("Cannot convert without an explicit filename.");
    return false;
  }
  if (!Common::isReadableFile(filename)) {
    Common::error(QString("Cannot read file to convert: %1").arg(filename));
    return false;
  }

  const QString compat = filename + ".compat";
  QStringList groups;
  {
    QSettings probe(filename, QSettings::IniFormat);
    groups = probe.childGroups();
  }

  if (groups.contains("xtalopt")) {
    QString readFilename;
    if (!prepareXtalOptStateFileForRead(filename, true, readFilename, true))
      return false;
    if (readFilename == filename) {
      Common::message(QString("%1 is already current; nothing written.").arg(filename));
      return true;
    }
    Common::message(QString("Converted session file to current format: %1").arg(readFilename));
    return true;
  }

  if (groups.contains("structure")) {
    Common::error("structure.state files are converted while loading their "
                  "XtalOpt session and cannot be converted separately.");
    return false;
  }

  QString inputText, outputText, error;
  if (!Common::readFileToQString(filename, &inputText)) {
    Common::error(QString("Cannot read file to convert: %1").arg(filename));
    return false;
  }
  if (!Legacy::rewriteLegacyXtalOptInputText(inputText, outputText, &error)) {
    Common::error(error);
    return false;
  }
  QFile compatFile(compat);
  if (!compatFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
    Common::error(QString("Could not write converted input copy: %1").arg(compat));
    return false;
  }
  compatFile.write(outputText.toLocal8Bit());
  compatFile.close();
  Common::message(QString("Converted xtalopt.in to current format: %1").arg(compat));
  return true;
}

} // end namespace XtalOpt

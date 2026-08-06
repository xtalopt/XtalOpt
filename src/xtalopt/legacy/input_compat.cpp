/**********************************************************************
  input_compat - Old xtalopt.in input compatibility.

  Copyright (C) 2017 by Patrick S. Avery
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/legacy/input_compat.h>
#include <xtalopt/legacy/legacy_helpers.h>

#include <common/compatibility/qt_compat.h>
#include <common/output.h>
#include <atoms/eleminfo.h>

#include <QFile>
#include <QMap>
#include <QRegularExpression>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QVector>

// Convert old input files.

namespace XtalOpt {
namespace Legacy {
namespace {

struct ParsedLine
{
  QString original;
  QString key;
  QString lowerKey;
  QString value;
  bool active;
};

ParsedLine parseLine(const QString& line)
{
  ParsedLine parsed;
  parsed.original = line;
  parsed.active = false;

  QString work = line.trimmed();
  work.replace(QRegularExpression(" *#.*"), "");
  work = work.simplified();
  if (work.isEmpty())
    return parsed;

  const QString key = work.section('=', 0, 0).trimmed();
  const QString value = work.section('=', 1).trimmed();
  if (key.isEmpty() || value.isEmpty())
    return parsed;

  parsed.key = key;
  parsed.lowerKey = key.toLower();
  parsed.value = value;
  parsed.active = true;
  return parsed;
}

bool isIndexedKeyword(const QString& lowerKey, const QString& lowerKeyword)
{
  return lowerKey == lowerKeyword || lowerKey.startsWith(lowerKeyword + " ");
}

bool optionToBool(const QString& s)
{
  const QString value = s.trimmed();
  return value.startsWith("t", Qt::CaseInsensitive) || value.toInt() != 0;
}

QString constraintKeyFromFields(const QString& exe, const QString& out)
{
  return exe + "\n" + out;
}

QString commentedLine(const QString& line)
{
  return "#" + line;
}

void appendNote(QStringList& notes, const QString& note)
{
  if (!notes.contains(note))
    notes.append(note);
}

QString boolToOption(bool value)
{
  return value ? "true" : "false";
}

// Check for an old "numbered" keyword (repeated keywords).
bool hasIntegerSuffix(const QString& key)
{
  const QString suffix = key.section(' ', 1).trimmed();
  if (suffix.isEmpty())
    return false;
  bool ok = false;
  suffix.toInt(&ok);
  return ok;
}

// Check an old input asset file id (left of "=").
bool isLegacyAssetId(const QString& id, bool allowSystem)
{
  if (allowSystem && id.compare("system", Qt::CaseInsensitive) == 0)
    return true;
  return Atoms::ElementInfo::getAtomicNum(id.trimmed().toStdString()) != 0;
}

bool legacyFiltrationConstraint(const QString& value, QString& constraintLine,
                                QString& constraintKey, QString& error)
{
  const QStringList fields = value.split(" ", QtCompat::SkipEmptyParts);
  if (fields.isEmpty() || fields.at(0).toLower().left(3) != "fil")
    return false;

  if (fields.size() < 4) {
    error = QString("legacy filtration objective expects: " "fil <script> <output> <value>");
    return false;
  }

  constraintLine = QString("constraint = %1 %2")
                     .arg(fields.at(1))
                     .arg(fields.at(2));
  constraintKey = constraintKeyFromFields(fields.at(1), fields.at(2));
  return true;
}

bool parseLegacyMolUnitText(const QString& text, LegacyMolUnitFields& fields, QString& error)
{
  const QStringList splitLine = text.split(",", QtCompat::SkipEmptyParts);
  if (splitLine.size() != 6) {
    error = QString("legacy molecularUnits entry must have 6 "
                    "comma-delimited fields: %1").arg(text);
    return false;
  }

  bool ok = false;
  fields.centerSymbol = splitLine.at(0).trimmed();
  fields.numCenters = splitLine.at(1).trimmed().toInt(&ok);
  if (!ok || fields.numCenters <= 0) {
    error = QString("legacy molecularUnits numCenters must be positive: %1")
              .arg(text);
    return false;
  }

  fields.neighborSymbol = splitLine.at(2).trimmed();
  fields.numNeighbors = splitLine.at(3).trimmed().toInt(&ok);
  if (!ok || fields.numNeighbors <= 0) {
    error = QString("legacy molecularUnits numNeighbors must be positive: %1")
              .arg(text);
    return false;
  }

  fields.geometry = splitLine.at(4).trimmed();
  return true;
}

bool convertLegacyMolUnit(const LegacyMolUnitFields& fields, QStringList& molUnitEntries,
                          QString& error)
{
  molUnitEntries.clear();

  const bool noCenter = isNoCenterSymbol(fields.centerSymbol);
  unsigned int centerAtomicNum = 0;
  QString centerSymbol;
  if (!noCenter) {
    centerAtomicNum = Atoms::ElementInfo::getAtomicNum(fields.centerSymbol.trimmed().toStdString());
    if (centerAtomicNum == 0) {
      error = QString("invalid legacy molecularUnits center symbol: %1")
                .arg(fields.centerSymbol);
      return false;
    }
    centerSymbol = Atoms::ElementInfo::getAtomicSymbol(centerAtomicNum).c_str();
  }

  const unsigned int neighborAtomicNum = Atoms::ElementInfo::getAtomicNum(
      fields.neighborSymbol.trimmed().toStdString());
  if (neighborAtomicNum == 0) {
    error = QString("invalid legacy molecularUnits neighbor symbol: %1")
              .arg(fields.neighborSymbol);
    return false;
  }
  const QString neighborSymbol = Atoms::ElementInfo::getAtomicSymbol(neighborAtomicNum).c_str();

  const LegacyMolUnitGeometry geometry = parseGeometry(fields.geometry);
  if (!geometryFitsNeighborCount(fields.numNeighbors, geometry)) {
    error = QString("invalid legacy molecularUnits geometry '%1' for " "numNeighbors=%2")
              .arg(fields.geometry)
              .arg(fields.numNeighbors);
    return false;
  }

  QString formula;
  QString templateName;
  if (noCenter) {
    if (fields.numNeighbors == 1 && geometry == LegacyGeomLinear)
      return true;

    formula = formulaEntry(neighborSymbol, fields.numNeighbors);
    templateName = shellOnlyTemplate(fields.numNeighbors, geometry);
  } else if (centerAtomicNum == neighborAtomicNum) {
    const int atomCount = fields.numNeighbors + 1;
    formula = formulaEntry(neighborSymbol, atomCount);
    templateName = centeredHomonuclearTemplate(fields.numNeighbors, geometry);
  } else {
    formula = formulaEntry(centerSymbol, 1) + formulaEntry(neighborSymbol, fields.numNeighbors);
    templateName = centeredHeteroTemplate(fields.numNeighbors, geometry);
  }

  if (templateName.isEmpty()) {
    error = QString("unsupported legacy molecularUnits geometry '%1' for " "numNeighbors=%2")
              .arg(fields.geometry)
              .arg(fields.numNeighbors);
    return false;
  }

  const QString entry = QString("%1 %2").arg(formula).arg(templateName);
  for (int i = 0; i < fields.numCenters; ++i)
    molUnitEntries.append(entry);
  return true;
}

bool convertLegacyInputText(const QString& inputText, QString& outputText,
                                    QString* errorMessage, bool& compatibilityApplied)
{
  outputText.clear();
  if (errorMessage)
    errorMessage->clear();
  compatibilityApplied = false;

  QVector<ParsedLine> lines;
  QString textCopy = inputText;
  QTextStream inputStream(&textCopy);
  while (!inputStream.atEnd())
    lines.append(parseLine(inputStream.readLine()));

  bool hasCurrentMolUnit = false;
  bool hasLegacyMolecularUnit = false;
  bool usingLegacyMolUnits = false;
  bool hasRemoteQueue = false;
  bool hasActiveLocalQueue = false;
  QString effectiveQueueInterface;
  QSet<QString> currentConstraints;
  const bool mentionsLegacyLocalQueue =
             inputText.contains(QRegularExpression("(^|\\n)\\s*#?\\s*localQueue\\s*=", QRegularExpression::CaseInsensitiveOption));

  for (int i = 0; i < lines.size(); ++i) {
    const ParsedLine& line = lines.at(i);
    if (!line.active)
      continue;

    if (isIndexedKeyword(line.lowerKey, "molunit"))
      hasCurrentMolUnit = true;
    else if (isIndexedKeyword(line.lowerKey, "molecularunits"))
      hasLegacyMolecularUnit = true;
    else if (line.lowerKey == "usingmolecularunits")
      usingLegacyMolUnits = optionToBool(line.value);
    else if (line.lowerKey == "queueinterface") {
      effectiveQueueInterface = line.value.trimmed().toLower();
      if (effectiveQueueInterface == "local")
        effectiveQueueInterface = "none";
    } else if (line.lowerKey == "remotequeue") {
      hasRemoteQueue = true;
    } else if (line.lowerKey == "localqueue") {
      hasActiveLocalQueue = true;
    } else if (line.lowerKey == "constraint") {
      const QStringList fields = line.value.split(" ", QtCompat::SkipEmptyParts);
      if (fields.size() >= 2)
        currentConstraints.insert(constraintKeyFromFields(fields.at(0), fields.at(1)));
    }
  }

  QStringList outputLines;
  QStringList notes;

  for (int i = 0; i < lines.size(); ++i) {
    const ParsedLine& line = lines.at(i);
    if (!line.active) {
      outputLines.append(line.original);
      continue;
    }

    if (line.lowerKey == "queueinterface" &&
        line.value.trimmed().compare("local", Qt::CaseInsensitive) == 0) {
      outputLines.append(commentedLine(line.original));
      outputLines.append("queueInterface = none");
      appendNote(notes, "converted legacy queueInterface = local to queueInterface = none");
    } else if (line.lowerKey == "localqueue") {
      outputLines.append(commentedLine(line.original));
      const bool remoteQueue = effectiveQueueInterface != "none" && !optionToBool(line.value);
      outputLines.append("remoteQueue = " + boolToOption(remoteQueue));
      appendNote(notes, "converted legacy localQueue to remoteQueue");
    } else if (line.lowerKey == "exelocation") {
      outputLines.append(commentedLine(line.original));
      outputLines.append("directRunCommand = " + line.value);
      appendNote(notes, "converted legacy exeLocation to directRunCommand");
    } else if (line.lowerKey == "usingradiiinteratomicdistancelimit" ||
               line.lowerKey == "usingradiisinteratomicdistancelimit") {
      outputLines.append(commentedLine(line.original));
      outputLines.append("usingScaledIADs = " + line.value);
      appendNote(notes, "converted legacy radii IAD option to usingScaledIADs");
    } else if (line.lowerKey == "objectivesredo") {
      outputLines.append(commentedLine(line.original));
      outputLines.append("constraintsReDo = " + line.value);
      appendNote(notes, "converted legacy objectivesReDo to constraintsReDo");
    } else if (line.lowerKey == "forcedspgswithrandspg") {
      outputLines.append(commentedLine(line.original));
      QStringList groups = line.value.split(",", QtCompat::SkipEmptyParts);

      for (int j = 0; j < groups.size(); ++j)
        groups[j] = groups.at(j).trimmed();
      outputLines.append("forcedSpgs = " + groups.join(","));
      appendNote(notes, "converted legacy forcedSpgsWithRandSpg to forcedSpgs");
    } else if (line.lowerKey.startsWith("molunit ") &&
               hasIntegerSuffix(line.key)) {
      // Convert an old molUnit key.
      outputLines.append(commentedLine(line.original));
      outputLines.append("molUnit = " + line.value);
      appendNote(notes, "converted legacy numbered molUnit to bare molUnit entries");
    } else if (line.lowerKey.startsWith("customiad ") &&
               hasIntegerSuffix(line.key)) {
      // Convert an old customIAD key.
      outputLines.append(commentedLine(line.original));
      outputLines.append("customIAD = " + line.value);
      appendNote(notes, "converted legacy numbered customIAD to bare customIAD entries");
    } else if (line.lowerKey.startsWith("potcarfile ") &&
               isLegacyAssetId(line.key.section(' ', 1), true)) {
      // Convert an old POTCAR file key.
      const QString id = line.key.section(' ', 1).trimmed();
      outputLines.append(commentedLine(line.original));
      outputLines.append("potcarFile = " + id + " " + line.value);
      appendNote(notes, "converted legacy potcarFile <id> = <file> to potcarFile = <id> <file>");
    } else if (line.lowerKey.startsWith("psffile ") &&
               isLegacyAssetId(line.key.section(' ', 1), false)) {
      // Convert an old PSF file key.
      const QString id = line.key.section(' ', 1).trimmed();
      outputLines.append(commentedLine(line.original));
      outputLines.append("psfFile = " + id + " " + line.value);
      appendNote(notes, "converted legacy psfFile <id> = <file> to psfFile = <id> <file>");
    } else if (line.lowerKey == "objective") {
      QString constraintLine;
      QString constraintKey;
      QString error;
      if (legacyFiltrationConstraint(line.value, constraintLine, constraintKey, error)) {
        outputLines.append(commentedLine(line.original));
        if (!currentConstraints.contains(constraintKey)) {
          outputLines.append(constraintLine);
          currentConstraints.insert(constraintKey);
        }
        appendNote(notes, "converted legacy filtration objective to constraint");
      } else if (!error.isEmpty()) {
        if (errorMessage)
          *errorMessage = error + ": " + line.value;
        return false;
      } else {
        outputLines.append(line.original);
      }
    } else if (line.lowerKey == "usingmolecularunits") {
      outputLines.append(commentedLine(line.original));
      if (hasCurrentMolUnit)
        appendNote(notes, "ignored legacy molecularUnits because current molUnit entries are present");
      else if (usingLegacyMolUnits && hasLegacyMolecularUnit)
        appendNote(notes, "converted enabled legacy molecularUnits to current molUnit entries");
      else if (usingLegacyMolUnits)
        appendNote(notes, "removed enabled legacy molecularUnits flag without entries");
      else
        appendNote(notes, "ignored disabled legacy molecularUnits");
    } else if (isIndexedKeyword(line.lowerKey, "molecularunits")) {
      outputLines.append(commentedLine(line.original));
      if (hasCurrentMolUnit) {
        appendNote(notes, "ignored legacy molecularUnits because current molUnit entries are present");
      } else if (usingLegacyMolUnits) {
        LegacyMolUnitFields fields;
        QString error;
        if (!parseLegacyMolUnitText(line.value, fields, error)) {
          if (errorMessage)
            *errorMessage = error;
          return false;
        }

        QStringList convertedEntries;
        if (!convertLegacyMolUnit(fields, convertedEntries, error)) {
          if (errorMessage)
            *errorMessage = error;
          return false;
        }

        for (const auto& entry : convertedEntries)
          outputLines.append("molUnit = " + entry);
        if (convertedEntries.isEmpty())
          appendNote(notes, "dropped legacy molecularUnits entry with a single "
                            "neighbor and no center (a lone atom needs no molUnit)");
        else
          appendNote(notes, "converted enabled legacy molecularUnits to current molUnit entries");
        if (!convertedEntries.isEmpty()) {
          appendNote(notes, "legacy molecularUnits bond distances are not used by current molecule templates");
        }
      } else {
        appendNote(notes, "ignored disabled legacy molecularUnits");
      }
    } else {
      outputLines.append(line.original);
    }
  }

  if (!hasRemoteQueue && !hasActiveLocalQueue &&
      mentionsLegacyLocalQueue && !effectiveQueueInterface.isEmpty() &&
      effectiveQueueInterface != "none") {
    outputLines.append("remoteQueue = true");
    appendNote(notes, "preserved legacy remote submission for the batch queue");
  }

  if (!notes.isEmpty()) {
    outputLines.append("");
    outputLines.append("# XtalOpt input compatibility notes:");
    for (const auto& note : notes)
      outputLines.append("# " + note);
  }

  outputText = outputLines.join("\n");
  if (!outputText.endsWith("\n"))
    outputText += "\n";
  compatibilityApplied = !notes.isEmpty();
  return true;
}

} // end anonymous namespace

bool convertInputText(const QString& filename, const QString& inputText,
                      QString& outputText, bool keepCompatibilityCopy,
                      QString* compatibilityFilename, QString* errorMessage)
{
  if (compatibilityFilename)
    compatibilityFilename->clear();
  bool applied = false;
  if (!convertLegacyInputText(inputText, outputText, errorMessage, applied))
    return false;

  if (!applied)
    return true;

  if (!keepCompatibilityCopy)
    return true;

  QFile compatFile(filename + ".compat");
  if (!compatFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
    if (errorMessage)
      *errorMessage = QString("Could not write compatibility input copy '%1'.")
                        .arg(compatFile.fileName());
    return false;
  }

  const QByteArray output = outputText.toLocal8Bit();
  if (compatFile.write(output) != output.size() || !compatFile.flush() ||
      compatFile.error() != QFileDevice::NoError) {
    compatFile.close();
    QFile::remove(compatFile.fileName());
    if (errorMessage)
      *errorMessage = QString("Could not write compatibility input copy '%1'.")
                        .arg(compatFile.fileName());
    return false;
  }
  if (compatibilityFilename)
    *compatibilityFilename = compatFile.fileName();
  return true;
}

} // namespace Legacy
} // end namespace XtalOpt

/**********************************************************************
  io_text - Reading and applying xtalopt.in, runtime options, and structured inputs.

  Copyright (C) 2009-2011 by David C. Lonie
  Copyright (C) 2017 by Patrick S. Avery
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <vector>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>
#include <QPair>
#include <QSet>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QWriteLocker>

#include <common/compatibility/qt_compat.h>
#include <common/constants.h>
#include <common/fileutils.h>
#include <common/stringutils.h>
#include <atoms/eleminfo.h>
#include <atoms/molecule.h>
#include <search/optimizer.h>
#include <common/output.h>
#include <search/queueinterface.h>

#include <xtalopt/legacy/input_compat.h>
#include <xtalopt/xtalopt.h>

#include <xtalopt/settings.h>

using namespace Search;

namespace XtalOpt {

namespace {

// Functions for reading and writing input files

bool inputOptimizerTemplateKeywords(const Optimizer* optimizer, QStringList& keywords,
                                     QString* err = nullptr)
{
  keywords.clear();
  if (!optimizer)
    return true;

  const QStringList filenames = optimizer->getOptimizerTemplateFileNames();
  for (const auto& filename : filenames) {
    const QString keyword = Settings::optimizerTemplateFilenameToKeyword(filename);
    if (keyword.isEmpty()) {
      if (err) {
        *err = QString("No XtalOpt input keyword is defined for optimizer "
                       "template '%1'.").arg(filename);
      }
      return false;
    }
    keywords.append(keyword);
  }
  return true;
}

bool inputQueueTemplateKeywords(const QueueInterface* queue, QStringList& keywords,
                                 QString* err = nullptr)
{
  keywords.clear();
  if (!queue)
    return true;

  const QStringList filenames = queue->getQueueInterfaceTemplateFileNames();
  if (filenames.isEmpty())
    return true;

  if (filenames.size() > 1) {
    if (err) {
      *err = QString("XtalOpt input keyword '%1' can map only one "
                     "queue template, but queue interface '%2' defines %3.")
                     .arg(Settings::queueTemplateKeyword())
                     .arg(queue->getIDString()).arg(filenames.join(", "));
    }
    return false;
  }

  keywords.append(Settings::queueTemplateKeyword());
  return true;
}

QString queueTemplateKeywordToFilename(const QueueInterface* queue, const QString& keyword,
                                        QString* err = nullptr)
{
  if (err)
    err->clear();

  if (keyword.compare(Settings::queueTemplateKeyword(), Qt::CaseInsensitive) != 0) {
    if (err)
      *err = "Unknown queue template keyword: " + keyword;
    return QString();
  }
  if (!queue) {
    if (err)
      *err = "No queue interface is selected for jobTemplates.";
    return QString();
  }

  const QStringList filenames = queue->getQueueInterfaceTemplateFileNames();
  if (filenames.size() != 1) {
    if (err) {
      *err = QString("Queue interface '%1' defines %2 queue templates for " "jobTemplates.")
                     .arg(queue->getIDString())
                     .arg(filenames.size());
    }
    return QString();
  }

  return filenames.first();
}

// Write one "  keyword = value" line (or "  keyword =" when value is empty).
void writeInputLine(QTextStream& stream, const QString& k, const QString& v)
{
  if (v.isEmpty())
    stream << "  " << k << " =\n";
  else
    stream << "  " << k << " = " << v << "\n";
}

bool parseInputAssetLine(const QString& line, QString& id, QString& file)
{
  const QString trimmed = line.trimmed();
  const int space = trimmed.indexOf(' ');
  if (space <= 0)
    return false;

  id = trimmed.left(space).trimmed();
  file = trimmed.mid(space + 1).trimmed();
  if (file.startsWith("%fileContents:", Qt::CaseInsensitive) && file.endsWith("%")) {
    file = file.mid(QString("%fileContents:").size());
    file.chop(1);
    file = file.trimmed();
  }
  return !id.isEmpty() && !file.isEmpty() && !file.contains(';') && !file.contains('#');
}

QStringList inputAssetLines(const OptimizerInputAssetMap& assets)
{
  QStringList lines;
  for (const auto& asset : assets)
    lines.append(QString::fromStdString(asset.first) + " " +
                 QString::fromStdString(asset.second));
  return lines;
}

// Write XtalOpt settings in xtalopt.in format.
void writeInputText(QTextStream& stream, XtalOpt* x)
{
  const Search::Optimizer* opt = x->optimizer(0);
  const Search::QueueInterface* queue = x->queueInterface(0);

  QStringList optimizerTemplateKws;
  QString keywordError;
  if (!inputOptimizerTemplateKeywords(opt, optimizerTemplateKws, &keywordError))
    Common::warning(keywordError);

  QStringList queueTemplateKws;
  if (!inputQueueTemplateKeywords(queue, queueTemplateKws, &keywordError))
    Common::warning(keywordError);

  QHash<QString, QString> assetKws;
  if (opt) {
    for (const auto& assetName : opt->getOptimizerInputAssetNames()) {
      const QString assetKeyword = Settings::optimizerInputAssetToKeyword(assetName);
      if (!assetKeyword.isEmpty())
        assetKws.insert(assetKeyword, assetName);
    }
  }

  stream << "\n### XtalOpt Run Options ###\n";

  for (const auto& keyword : Settings::allSettingKeywords()) {
    // Write settings with one value.
    if (Settings::hasScalarSettingBinding(keyword)) {
      const QString value = Settings::scalarSettingValue(*x, keyword);
      if (keyword == "seedStructures" && value.isEmpty())
        continue;
      writeInputLine(stream, keyword, value);
      continue;
    }

    // Write settings with more than one line.
    if (Settings::hasRepeatedSettingBinding(keyword)) {
      for (const QString& entry : Settings::repeatedSettingEntries(*x, keyword))
        writeInputLine(stream, keyword, entry);
      continue;
    }

    // Write optimizer and queue settings.
    if (keyword == "queueInterface") {
      writeInputLine(stream, keyword, queue ? queue->getIDString().toLower() : "none");
    } else if (keyword == "optimizer") {
      writeInputLine(stream, keyword, opt ? opt->getIDString().toLower() : "none");
    } else if (keyword == "numOptimizationSteps") {
      writeInputLine(stream, keyword, QString::number(x->getNumOptSteps()));
    } else if (keyword == "templatesDirectory") {
      writeInputLine(stream, keyword, "");
    } else if (keyword == "directRunCommand") {
      writeInputLine(stream, keyword, opt ? opt->getDirectRunCommand() : QString());
    } else if (keyword == "submitCommand" || keyword == "cancelCommand" || keyword == "statusCommand") {
      if (queue && queue->isBatchQueue()) {
        if (keyword == "submitCommand")
          writeInputLine(stream, keyword, queue->submitCommand());
        else if (keyword == "cancelCommand")
          writeInputLine(stream, keyword, queue->cancelCommand());
        else
          writeInputLine(stream, keyword, queue->statusCommand());
      }
    } else if (queueTemplateKws.contains(keyword)) {
      writeInputLine(stream, keyword, "");
    } else if (optimizerTemplateKws.contains(keyword)) {
      writeInputLine(stream, keyword, "");
    } else if (assetKws.contains(keyword)) {
      QStringList assetLines = inputAssetLines(
        x->getOptimizerInputAssets(0, assetKws.value(keyword).toStdString()));
      if (assetLines.isEmpty())
        assetLines.append("");
      for (const QString& line : assetLines)
        writeInputLine(stream, keyword, line);
    } else if (Settings::isInputJobFileKeyword(keyword)) {
      // Template/asset keyword for an optimizer/queue that is not selected.
    } else {
      // A settings-table row this writer does not know how to handle yet.
      Common::warning("writeInputText: keyword not exported: " + keyword);
    }
  }
}

// Read one input line as "keyword = value".
// Multi-entry keywords are stored to relevant lists; while
//   scalar keywords are stored in options.
void parseSettingLine(const QString& tmpLine, QHash<QString, QString>& options,
                       QHash<QString, QStringList>& multiInput,
                       const QString& sourceDescription = QString())
{
  QString line = tmpLine.trimmed();
  const QString displayLine = line;

  // Remove everything to the right of '#' (including '#') since it is a comment
  line.replace(QRegularExpression(" *#.*"), "");

  // Simplify 'space' characters (e.g., prevent issues in reading 'potcar element')
  line = line.simplified();

  if (line.isEmpty())
    return;

  // We might have additional "=" signs in the value (e.g., arguments
  //   of the direct run command). So, we split the input based on the
  //   "leftmost '=' sign", to obtain the key and value.

  // Get the key and the value
  QString key = line.section('=', 0, 0).trimmed().toLower();
  QString value = line.section('=', 1).trimmed();

  // Make sure the line has a key and an '='.
  if (key.isEmpty() || !line.contains('=')) {
    Common::warning(QString("%1 Invalid line '%2' is ignored")
                            .arg(sourceDescription).arg(displayLine));
    return;
  }

  // Case insensitive key
  const QString csKey = Settings::findKeywordName(key);
  if (csKey.isEmpty()) {
    Common::warning(QString("%1 Unrecognized option '%2' is ignored")
                            .arg(sourceDescription).arg(key));
    return;
  }

  // Multi-objective related entries are treated separately. The reason is that
  //   there might be multiple of these entries and each have multiple fields.
  // So, we won't assign actual variables here. Rather, add them all to a list
  //   to process them later on.
  if (Settings::isRepeatableInputKeyword(csKey)) {
    if (!value.isEmpty())
      multiInput[csKey].append(value);
  } else {
    options[csKey] = value.isEmpty() ? Settings::defaultValue(csKey) : value;
  }
}

bool inputTextHasRequiredKeyword(const QString& parserText)
{
  const QStringList requiredKeywords = Settings::requiredInputKeywords();
  QString textCopy = parserText;
  QTextStream stream(&textCopy);
  while (!stream.atEnd()) {
    QString line = stream.readLine();
    line.replace(QRegularExpression(" *#.*"), "");
    if (!line.contains('='))
      continue;
    const QString key = line.section('=', 0, 0).trimmed().toLower();
    if (requiredKeywords.contains(Settings::findKeywordName(key)))
      return true;
  }
  return false;
}

bool readInputFile(const QString& filename, QHash<QString, QString>& options,
                           QHash<QString, QStringList>& multiInput, bool bestEffort,
                           bool keepCompatibilityCopy)
{
  QString inputText;
  if (!Common::readFileToQString(filename, &inputText)) {
    Common::error(QString("Could not open file '%1'.").arg(filename));
    return false;
  }

  QString parserText;
  QString compatError;
  if (!Legacy::convertInputText(filename, inputText, parserText,
                                keepCompatibilityCopy, nullptr, &compatError)) {
    Common::error(compatError);
    return false;
  }

  if (!bestEffort && !inputTextHasRequiredKeyword(parserText)) {
    Common::error(QString("The input file '%1' does not look like an XtalOpt input file")
                    .arg(filename));
    return false;
  }

  QString parserTextCopy = parserText;
  QTextStream stream(&parserTextCopy);
  while (!stream.atEnd()) {
    const QString line = stream.readLine();
    parseSettingLine(line, options, multiInput, "Settings file:");
  }
  return true;
}

bool hasRequiredInputValues(const QHash<QString, QString>& options)
{
  const QStringList requiredKeywords = Settings::requiredInputKeywords();

  for (const auto& keyword : requiredKeywords) {
    if (options.value(keyword).isEmpty()) {
      Common::error(QString("Required option '%1' was not set in the "
                            "options file.\nRequired options for every run "
                            "are: %2")
                            .arg(keyword)
                            .arg(requiredKeywords.join(", ")));
      return false;
    }
  }

  const QString queueInterfaceStr = options.value("queueInterface").toLower();
  const QString optimizerStr = options.value("optimizer").toLower();
  QStringList validQueueInterfaces = QueueInterface::registeredQueueInterfaces();
  QStringList validOptimizers = Optimizer::registeredOptimizers();
  for (QString& name : validQueueInterfaces)
    name = name.toLower();
  for (QString& name : validOptimizers)
    name = name.toLower();

  // Make sure that the queue interface is valid
  if (!validQueueInterfaces.contains(queueInterfaceStr)) {
    Common::error(QString("Unrecognized queue interface '%1' was "
                          "entered.\nValid queue interfaces are: %2")
                          .arg(options.value("queueInterface"))
                          .arg(validQueueInterfaces.join(", ")));
    return false;
  }

  // Make sure that the optimizer is valid
  if (!validOptimizers.contains(optimizerStr)) {
    Common::error(QString("Unrecognized optimizer '%1' was entered.\n"
                          "Valid optimizers are: %2")
                          .arg(options.value("optimizer"))
                          .arg(validOptimizers.join(", ")));
    return false;
  }

  bool remoteQueueRequested = false;

  const QString remoteQueueText = options.value("remoteQueue", Settings::defaultValue("remoteQueue"));

  if (!Common::textToValue(remoteQueueText, remoteQueueRequested)) {
    Common::error("Invalid value for option 'remoteQueue': " + options.value("remoteQueue"));
    return false;
  }
  const bool batchQueueInterface = (queueInterfaceStr != "none");
  const QString sshMethod = options.value("sshMethod", Settings::defaultValue("sshMethod"));

  if (!SearchBase::isValidSshMethod(sshMethod)) {
    Common::error(QString("Unrecognized sshMethod '%1'. Valid values are: "
                          "system, libssh, auto.")
                  .arg(sshMethod));
    return false;
  }

  if (remoteQueueRequested && !batchQueueInterface) {
    Common::error("'remoteQueue = true' cannot be used with " "'queueInterface = none'.");
    return false;
  }

  if (remoteQueueRequested && batchQueueInterface) {
    if (!SearchBase::isSshMethodAvailable(sshMethod)) {
      Common::error(QString("sshMethod '%1' is not available in this build.")
                    .arg(sshMethod));
      return false;
    }

    QStringList requiredRemoteKeywords;
    requiredRemoteKeywords << "host" << "user" << "remoteWorkingDirectory";
    for (const auto& requiredKeyword : requiredRemoteKeywords) {
      if (options.value(requiredKeyword).isEmpty()) {
        Common::error(QString("Required option for remote queue "
                              "submission, '%1', was not set in the options "
                              "file.\nRequired options for remote queue "
                              "submission are: %2")
                              .arg(requiredKeyword)
                              .arg(requiredRemoteKeywords.join(", ")));
        return false;
      }
    }
  }

  // Everything that was required was set!
  return true;
}

bool reportInputSettings(const QHash<QString, QString>& options, XtalOpt& xtalopt)
{
  QStringList keys = options.keys();
  std::sort(keys.begin(), keys.end());

  QString output;
  QTextStream stream(&output);

  // Options the user set explicitly in the input file.
  stream << "\n=== Manually Set Options\n\n";
  for (const auto& key : keys)
    stream << key << " = " << options[key] << "\n";

  // Every run option, in settings-table order.
  stream << "\n=== All Run Options\n";
  writeInputText(stream, &xtalopt);

  // We need to convert to c string to properly print newlines
  Common::message(output);
  return true;
}

// Add the verbose summary of the main search inputs to the output.
void addInputSummaryReport(XtalOpt& xtalopt, QString* verboseReport)
{
  if (!xtalopt.isVerbose() || !verboseReport)
    return;

  QString outstr = "\n\n   -----\n\n";
  outstr += "   Final list of input compositions:\n";
  for (int i = 0; i < xtalopt.compList().size(); i++) {
    outstr += QString("%1").arg(xtalopt.compList()[i].getFormula(), 20);
    if ((i + 1) % 3 == 0)
      outstr += "\n";
  }
  outstr += "\n\n";
  outstr += "   Chemical System: " + xtalopt.getChemicalSystem().join(" ");
  outstr += "\n\n";
  outstr += "   Initial atomic min radii: \n";
  for (const auto& el : xtalopt.eleScaledRadii().getRadiiAtomicNumbers())
    outstr += QString("      %1 : %2\n")
                      .arg(el)
                      .arg(xtalopt.eleScaledRadii().getMinRadius(el));

  outstr += "\n";
  for (int i = 0; i < xtalopt.refEnergies().size(); i++) {
    outstr += QString("   Reference energy %1 : %2\n")
                     .arg(xtalopt.refEnergies()[i].cell.getFormula(), 10)
                     .arg(xtalopt.refEnergies()[i].energy, 12, 'f', 6);
  }

  outstr += "\n";
  for (const auto& atomcn : xtalopt.eleVolumes().getVolumeAtomicNumbers()) {
    outstr += QString("   Elemental volume %1 : %2 %3\n")
                      .arg(Atoms::ElementInfo::getAtomicSymbol(atomcn).c_str(), 10)
                      .arg(xtalopt.eleVolumes().getMinVolume(atomcn), 12, 'f', 6)
                      .arg(xtalopt.eleVolumes().getMaxVolume(atomcn), 12, 'f', 6);
  }
  outstr += "   -----\n\n"; // Just for style!

  *verboseReport += outstr;
}

// Appy default values for keywords that don't have any.
void applyInputDefaults(QHash<QString, QString>& options)
{
  for (const auto& keyword : Settings::allSettingKeywords()) {
    const QString value = Settings::defaultValue(keyword);
    if (!options.contains(keyword) && !value.isEmpty())
      options[keyword] = value;
  }
}

// Make a path relative to sourceDir absolute (unchanged if already absolute).
QString inputAssetPath(const QString& sourceDir, const QString& path)
{
  const QString trimmed = path.trimmed();
  if (trimmed.isEmpty())
    return trimmed;
  return QFileInfo(Common::localPath(sourceDir, trimmed)).absoluteFilePath();
}

bool resolveReadableInputFile(QString& path, const QString& description, const QString& sourceDir)
{
  path = inputAssetPath(sourceDir, path);
  if (!Common::isReadableFile(path)) {
    Common::error(QString("%1 was not found or is not readable: %2").arg(description).arg(path));
    return false;
  }
  path = QFileInfo(path).absoluteFilePath();
  return true;
}

bool resolveReadableInputDirectory(QString& path, const QString& description, const QString& sourceDir)
{
  path = inputAssetPath(sourceDir, path);
  if (!Common::isReadableDirectory(path)) {
    Common::error(QString("%1 was not found or is not readable: %2").arg(description).arg(path));
    return false;
  }
  path = QFileInfo(path).absoluteFilePath();
  return true;
}

bool resolveInputTemplateFileKeyword(QString& str, const QString& keyword, const QString& sourceDir)
{
  if (!str.startsWith(keyword + ":", Qt::CaseInsensitive))
    return true;

  const int colon = str.indexOf(':');
  QString filename = str.mid(colon + 1).trimmed();
  if (filename.isEmpty()) {
    Common::error(QString("Template keyword %1 has an empty filename.").arg(keyword));
    return false;
  }

  if (!resolveReadableInputFile(filename,
                                QString("Template keyword %1 file").arg(keyword),
                                sourceDir)) {
    return false;
  }

  str = str.left(colon + 1) + filename;
  return true;
}

bool resolveInputTemplateFileKeywords(QString& text, const QString& sourceDir)
{
  QStringList parts = text.split('%');
  bool changed = false;
  for (int i = 0; i < parts.size(); ++i) {
    const QString original = parts[i];
    if (!resolveInputTemplateFileKeyword(parts[i], "fileContents", sourceDir))
      return false;
    if (!resolveInputTemplateFileKeyword(parts[i], "copyFile", sourceDir))
      return false;
    changed = changed || parts[i] != original;
  }

  if (changed)
    text = parts.join("%");
  return true;
}

QList<uint> sortedCompositionAtomicNumbers(const XtalOpt& xtalopt)
{
  QList<uint> atomicNums = xtalopt.compList()[0].getCompositionAtomicNumbers();

  std::sort(atomicNums.begin(), atomicNums.end());
  return atomicNums;
}

QStringList sortedCompositionSymbols(const QList<uint>& atomicNums)
{
  QStringList symbols;
  for (const auto& atomicNum : atomicNums) {
    if (atomicNum != 0)
      symbols.append(Atoms::ElementInfo::getAtomicSymbol(atomicNum).c_str());
  }
  std::sort(symbols.begin(), symbols.end());
  return symbols;
}

bool applyInputOptimizerAssets(XtalOpt& xtalopt, size_t optStep,
                                      const QHash<QString, QStringList>& multiInput,
                                      const QString& sourceDir)
{
  Optimizer* optimizer = xtalopt.optimizer(optStep);
  if (!optimizer)
    return true;

  const QStringList assetNames = optimizer->getOptimizerInputAssetNames();
  if (assetNames.isEmpty())
    return true;

  // The per-species asset files are usually matched to the elements; so composition is needed
  // This is a defensive guard mostly for GUI (CLI already checks it in order).
  if (xtalopt.compList().isEmpty()) {
    Common::error("The composition must be set before reading the optimizer "
                  "input asset files.");
    return false;
  }

  const QList<uint> atomicNums = sortedCompositionAtomicNumbers(xtalopt);
  const QStringList symbols = sortedCompositionSymbols(atomicNums);

  for (const auto& assetName : assetNames) {
    const QString optionKeyword = Settings::optimizerInputAssetToKeyword(assetName);
    if (optionKeyword.isEmpty())
      continue;

    // Each repeated entry is "<id> <file>": <id> is an element symbol or, for
    //   POTCAR, the literal "system"; <file> is a file path.
    QList<QPair<QString, QString>> assetEntries;
    for (const QString& entry : multiInput.value(optionKeyword)) {
      QString id, file;
      if (!parseInputAssetLine(entry, id, file)) {
        Common::error("The " + assetName + " entry must be '<id> <file>' with no "
                      "';' or '#' in the file path: " + entry);
        return false;
      }
      assetEntries.append(qMakePair(id, file));
    }

    const bool allowSystemFile = assetName.compare("POTCAR", Qt::CaseInsensitive) == 0;
    int systemEntry = -1;
    for (int i = assetEntries.size() - 1; i >= 0; --i) {
      if (assetEntries.at(i).first.compare("system", Qt::CaseInsensitive) == 0) {
        systemEntry = i;
        break;
      }
    }
    if (allowSystemFile && systemEntry >= 0) {
      QString file = assetEntries.at(systemEntry).second;
      if (!resolveReadableInputFile(file, "The " + assetName + " file for the system", sourceDir))
        return false;
      OptimizerInputAssetMap systemFile;
      systemFile[assetEntries.at(systemEntry).first.toStdString()] = file.toStdString();
      xtalopt.setOptimizerInputAssets(optStep, assetName.toStdString(), systemFile);
      continue;
    }

    OptimizerInputAssetMap speciesFiles;
    for (const auto& symbol : symbols) {
      int speciesEntry = -1;
      for (int i = assetEntries.size() - 1; i >= 0; --i) {
        if (assetEntries.at(i).first.compare(symbol, Qt::CaseInsensitive) == 0) {
          speciesEntry = i;
          break;
        }
      }
      if (speciesEntry < 0) {
        QString example = optionKeyword + " = " + symbol + " /path/to/";
        if (assetName.compare("POTCAR", Qt::CaseInsensitive) == 0)
          example += "vasp_potcars/symbol/POTCAR";
        else if (assetName.compare("PSF", Qt::CaseInsensitive) == 0)
          example += "siesta_psfs/symbol.psf";
        else
          example += assetName.toLower() + "/" + symbol;
        Common::error("No " + assetName + " file found for atom type " +
                      symbol + ". You must set the " + assetName +
                      " file in the options like so: " + example);
        return false;
      }

      QString file = assetEntries.at(speciesEntry).second;
      if (!resolveReadableInputFile(file, "The " + assetName + " file for atom type " + symbol,
                                    sourceDir))
        return false;
      speciesFiles[assetEntries.at(speciesEntry).first.toStdString()] = file.toStdString();
    }

    xtalopt.setOptimizerInputAssets(optStep, assetName.toStdString(), speciesFiles);
  }

  return true;
}

bool hasRequiredInputTemplates(const QStringList& templateKeywords, const QString& optQueueName,
                                      const QHash<QString, QString>& options)
{
  for (const auto& templateKeyword : templateKeywords) {
    if (options[templateKeyword].isEmpty()) {
      Common::error(QString("Required option for %1, '%2', was not set "
                            "in the options file.\nRequired options for %1 "
                            "are: %3")
                            .arg(optQueueName)
                            .arg(templateKeyword)
                            .arg(templateKeywords.join(", ")));
      return false;
    }
  }
  return true;
}

bool readInputOptimizerTemplate(const QString& templateKeyword, size_t optStep,
                                const QHash<QString, QString>& options, const QString& sourceDir,
                                QString& text)
{
  QString optStepStr = QString::number(optStep + 1);
  QStringList fileList = options[templateKeyword].split(",");

  if (fileList.size() <= static_cast<int>(optStep)) {
    Common::error(QString("%1: %2 does not contain a template for "
                          "opt step %3").arg(__func__, templateKeyword, optStepStr));
    return false;
  }

  QString filename = fileList[static_cast<int>(optStep)].trimmed();
  if (filename.isEmpty()) {
    Common::error(QString("%1: %2 is missing!").arg(__func__, templateKeyword));
    return false;
  }

  const QString templatePath =
    QFileInfo(Common::localPath(options.value("templatesDirectory"), filename))
      .absoluteFilePath();

  const QFileInfo templateInfo(templatePath);
  if (!templateInfo.exists() || !templateInfo.isFile() || !templateInfo.isReadable()) {
    Common::error("Could not read file '" + filename + "' in the templates directory: " +
                  options.value("templatesDirectory"));
    return false;
  }

  if (!Common::readFileToQString(templatePath, &text)) {
    Common::error("Could not open file '" + filename + "' in the templates directory: " +
                  options.value("templatesDirectory"));
    return false;
  }
  if (!resolveInputTemplateFileKeywords(text, sourceDir))
    return false;

  return true;
}

bool applyInputOptimizerTemplate(XtalOpt& xtalopt, const QString& templateKeyword, size_t optStep,
                                 const QHash<QString, QString>& options, const QString& sourceDir)
{
  QString text;
  if (!readInputOptimizerTemplate(templateKeyword, optStep, options, sourceDir, text)) {
    return false;
  }

  const QString engineTemplateName = Settings::optimizerTemplateKeywordToFilename(templateKeyword);

  if (engineTemplateName.isEmpty()) {
    Common::error("Unknown optimizer template keyword: " + templateKeyword);
    return false;
  }

  xtalopt.setOptimizerTemplate(optStep, engineTemplateName.toStdString(), text.toStdString());
  return true;
}

bool applyInputQueueTemplate(XtalOpt& xtalopt, const QueueInterface* queue,
                             const QString& templateKeyword, size_t optStep,
                             const QHash<QString, QString>& options, const QString& sourceDir)
{
  QString text;
  if (!readInputOptimizerTemplate(templateKeyword, optStep, options, sourceDir, text)) {
    return false;
  }

  QString filenameError;
  const QString engineTemplateName =
    queueTemplateKeywordToFilename(queue, templateKeyword, &filenameError);
  if (engineTemplateName.isEmpty()) {
    Common::error(filenameError);
    return false;
  }

  xtalopt.setQueueInterfaceTemplate(optStep, engineTemplateName.toStdString(), text.toStdString());
  return true;
}

bool applyInputOptimizer(XtalOpt& xtalopt, size_t optStep,
                                         const QHash<QString, QString>& options,
                                         const QHash<QString, QStringList>& multiInput,
                                         const QString& sourceDir)
{
  Optimizer* optimizer = xtalopt.optimizer(optStep);

  QStringList templateKeywords;
  QString templateKeywordError;
  if (!inputOptimizerTemplateKeywords(optimizer, templateKeywords, &templateKeywordError)) {
    Common::error(templateKeywordError);
    return false;
  }

  if (!hasRequiredInputTemplates(templateKeywords, options["optimizer"], options))
    return false;

  for (const auto& templateKeyword : templateKeywords) {
    if (!applyInputOptimizerTemplate(xtalopt, templateKeyword, optStep, options, sourceDir)) {
      return false;
    }
  }

  return applyInputOptimizerAssets(xtalopt, optStep, multiInput, sourceDir);
}

bool applyInputOptimizerAndQueue(QHash<QString, QString>& options,
                                         const QHash<QString, QStringList>& multiInput, XtalOpt& xtalopt,
                                         const QString& sourceDir, bool loadAndVerifyAssets, bool bestEffort)
{
  size_t numOptSteps = options.value("numOptimizationSteps").toUInt();
  // Zero (or an unreadable value) would leave the run with no
  //   optimizer/queue steps at all; use one step, as the state loader does.
  if (numOptSteps == 0) {
    Common::warning("numOptimizationSteps must be at least 1; using 1.");
    numOptSteps = 1;
  }

  xtalopt.clearOptSteps();

  const QString queueInterfaceStr = options["queueInterface"].toLower();
  const QString optimizerStr = options["optimizer"].toLower();

  // Set the queue and optimizer.
  for (size_t i = 0; i < numOptSteps; ++i) {
    xtalopt.appendOptStep();

    if (!xtalopt.setQueueInterface(i, queueInterfaceStr.toStdString()))
      return false;

    QueueInterface* queue = xtalopt.queueInterface(i);
    if (queue && queue->isBatchQueue()) {
      if (!options["submitCommand"].isEmpty())
        queue->setSubmitCommand(options["submitCommand"]);
      if (!options["cancelCommand"].isEmpty())
        queue->setCancelCommand(options["cancelCommand"]);
      if (!options["statusCommand"].isEmpty())
        queue->setStatusCommand(options["statusCommand"]);
    }

    if (!xtalopt.setOptimizer(i, optimizerStr.toStdString()))
      return false;

    Optimizer* optimizer = xtalopt.optimizer(i);
    if (!options["directRunCommand"].isEmpty())
      optimizer->setDirectRunCommand(options["directRunCommand"]);
  }

  // A simple read sets the optimizers/queues but does not load template contents.
  if (!loadAndVerifyAssets)
    return true;

  // Load the template and asset input files.
  QString templatesDirectory = options.value("templatesDirectory");

  if (!resolveReadableInputDirectory(templatesDirectory, "templatesDirectory", sourceDir)) {
    if (!bestEffort)
      return false;
    Common::warning("Templates directory was not found; template contents were "
                    "left empty. Set them before starting a search.");
    return true;
  }
  options["templatesDirectory"] = templatesDirectory;

  for (size_t i = 0; i < numOptSteps; ++i) {
    QueueInterface* queue = xtalopt.queueInterface(i);
    if (queue && queue->isBatchQueue()) {
      QStringList queueTemplateKeywords;
      QString queueTemplateKeywordError;

      bool queueTemplatesOk =
        inputQueueTemplateKeywords(queue, queueTemplateKeywords, &queueTemplateKeywordError) &&
        hasRequiredInputTemplates(queueTemplateKeywords, options["queueInterface"], options);

      for (const auto& templateKeyword : queueTemplateKeywords) {
        if (!queueTemplatesOk)
          break;
        if (!applyInputQueueTemplate(xtalopt, queue, templateKeyword, i, options, sourceDir))
          queueTemplatesOk = false;
      }
      if (!queueTemplatesOk) {
        if (!bestEffort) {
          if (!queueTemplateKeywordError.isEmpty())
            Common::error(queueTemplateKeywordError);
          return false;
        }
        Common::warning(QString("Queue templates for opt step %1 were left empty.").arg(i + 1));
        continue;
      }
    }

    if (!applyInputOptimizer(xtalopt, i, options, multiInput, sourceDir)) {
      if (!bestEffort)
        return false;
      Common::warning(QString("Optimizer templates or input assets for opt step %1 "
                              "could not be fully loaded.").arg(i + 1));
    }
  }

  return true;
}

bool applyInputRepeatedEntries(const QHash<QString, QStringList>& multiInput, XtalOpt& xtalopt)
{
  // Apply multi-entry settings.
  for (const QString& keyword : Settings::allSettingKeywords()) {
    if (!Settings::hasRepeatedSettingBinding(keyword))
      continue;
    Settings::clearRepeatedSetting(xtalopt, keyword);

    for (const QString& entry : multiInput.value(keyword)) {
      if (entry.trimmed().isEmpty())
        continue;

      if (!Settings::addRepeatedSettingEntry(xtalopt, keyword, entry)) {
        Common::error("Invalid " + keyword + " entry: " + entry);
        return false;
      }
    }
  }
  return true;
}

// Set the local working directory. Imports keep a relative path; command-line
// input uses the process directory.
bool applyInputWorkDir(const QHash<QString, QString>& options,
                       XtalOpt& xtalopt, bool bestEffort)
{
  const QString locWorkDir = options.value("localWorkingDirectory");

  if (!bestEffort)
    xtalopt.setLocWorkDir(QDir(locWorkDir).absolutePath());
  else
    xtalopt.setLocWorkDir(locWorkDir);

  return true;
}

// Apply input settings in order: structure values, search values, then optimizers/queues.
bool applyInputSettings(QHash<QString, QString> options, const QHash<QString,
                        QStringList>& multiInput, XtalOpt& xtalopt,
                        const QString& sourceDir, bool loadAndVerifyAssets,
                        bool bestEffort, QString* verboseReport)
{
  applyInputDefaults(options);

  // Scalars apply through the settings table. localWorkingDirectory is
  //   excluded here since it may need to be made absolute below.
  for (auto it = options.constBegin(); it != options.constEnd(); ++it) {
    const QString canon = Settings::findKeywordName(it.key());

    if (canon.isEmpty() || canon == "localWorkingDirectory" || !Settings::hasScalarSettingBinding(canon))
      continue;
    if (!Settings::applyScalarSetting(xtalopt, canon, it.value())) {
      Common::error(QString("Invalid value for option '%1': %2").arg(canon).arg(it.value()));
      return false;
    }
  }
  // Seed paths (a scalar now) are resolved relative to the input file when this
  //   read loads assets; the scalar set already populated seedList().
  if (loadAndVerifyAssets) {
    QStringList resolvedSeeds;
    for (const QString& seed : xtalopt.seedList())
      resolvedSeeds.append(inputAssetPath(sourceDir, seed));
    xtalopt.seedList() = resolvedSeeds;
  }

  // Process the single-line inputs (eg, compositions and radii).
  if (!xtalopt.rebuildDerivedSettings()) {
    Common::error("Input compositions were not read in successfully.");
    return false;
  }
  addInputSummaryReport(xtalopt, verboseReport);

  if (!applyInputRepeatedEntries(multiInput, xtalopt))
    return false;

  xtalopt.refreshBuiltinObjectiveWeight();

  if (!applyInputOptimizerAndQueue(options, multiInput, xtalopt, sourceDir,
                                           loadAndVerifyAssets, bestEffort))
    return false;

  if (!applyInputWorkDir(options, xtalopt, bestEffort))
    return false;

  // One final validation: a fresh input file with bad values is rejected.
  if (!Settings::validateSettings(xtalopt, Settings::InvalidSettingAction::Reject))
    return false;

  xtalopt.warnVolumeLimitConflicts();
  return true;
}

QString stripOptionalQuotes(QString value)
{
  value = value.trimmed();
  if (value.size() >= 2 && value.at(0) == '"' && value.at(value.size() - 1) == '"')
    return value.mid(1, value.size() - 2);
  return value;
}

bool compositionFitsInAnyInputFormula(const CellComp& requested,
                                      const QList<CellComp>& inputFormulas)
{
  if (inputFormulas.isEmpty())
    return true;

  const QList<unsigned int> atomicNums = requested.getCompositionAtomicNumbers();
  for (int formulaIndex = 0; formulaIndex < inputFormulas.size(); ++formulaIndex) {
    const CellComp& inputFormula = inputFormulas.at(formulaIndex);
    if (requested.getNumAtoms() > inputFormula.getNumAtoms())
      continue;

    bool fits = true;
    for (int i = 0; i < atomicNums.size(); ++i) {
      const unsigned int atomicNum = atomicNums.at(i);
      if (requested.getCount(atomicNum) > inputFormula.getCount(atomicNum)) {
        fits = false;
        break;
      }
    }
    if (fits)
      return true;
  }

  return false;
}

bool parseFormulaWithTrailingDoubles(const QString& entry, int valueCount,
                                     QString& formula, QList<double>& values, QString* error)
{
  formula.clear();
  values.clear();

  QStringList fields = entry.split(" ", QtCompat::SkipEmptyParts);
  if (fields.size() < valueCount + 1) {
    if (error) {
      *error = QString("Entry must contain a formula and %1 numeric value(s): %2")
                       .arg(valueCount).arg(entry);
    }
    return false;
  }

  values.reserve(valueCount);
  QList<double> reversedValues;
  reversedValues.reserve(valueCount);
  for (int i = 0; i < valueCount; ++i) {
    bool ok = false;
    const QString valueText = fields.takeLast();
    const double value = valueText.toDouble(&ok);
    if (!ok || !GS_ISFINITE(value)) {
      if (error) {
        *error = QString("Invalid numeric value '%1' in entry: %2").arg(valueText).arg(entry);
      }
      return false;
    }
    reversedValues.prepend(value);
  }

  formula = fields.join(' ');
  values = reversedValues;
  return true;
}

// Apply runtime-changeable options from the parsed options map to xtalopt.
void applyRuntimeSettings(const QHash<QString, QString>& options,
                          const QHash<QString, QStringList>& multiInput, XtalOpt& xtalopt)
{
  bool settingsChanged = false;
  bool spacegroupSettingsChanged = false;
  bool similaritySettingsChanged = false;
  bool selectionSettingsChanged = false;
  {
    QWriteLocker runtimeLocker(xtalopt.runtimeSettingsLock());

    // Store the current settings: invalid edits restore to it, and changes are
    //   reported against it afterwards.
    const Settings::ScalarSnapshot before = Settings::captureScalarSettings(xtalopt);

    for (auto it = multiInput.constBegin(); it != multiInput.constEnd(); ++it) {
      for (const QString& entry : it.value()) {
        Common::warning("Runtime file: Ignored unsupported repeated option: " + it.key() + " = " + entry);
      }
    }

    for (auto it = options.constBegin(); it != options.constEnd(); ++it) {
      const QString canon = Settings::findKeywordName(it.key());
      const QString& value = it.value();

      if (canon.isEmpty() || !Settings::isRuntimeKeyword(canon)) {
        Common::warning("Runtime file: Ignored unknown or fixed option: " + it.key());
        continue;
      }

      // Everything (elementalVolumes included) applies through the settings
      //   table; we keep the raw text as entered and re-parse it below.
      if (!Settings::applyScalarSetting(xtalopt, canon, value)) {
        Common::warning("Runtime file: Ignored invalid value for " + canon + ": " + value);
        continue;
      }
    }

    // elementalVolumes is parsed text: keep the previous value if a new one is
    //   invalid (the setter already stored the raw string).
    if (xtalopt.getInputEleVolmString() != before.value("elementalVolumes") &&
        !xtalopt.compList().isEmpty() &&
        !xtalopt.processInputElementalVolumes(xtalopt.getInputEleVolmString())) {
      Common::warning("Runtime file: Ignored invalid elemental volume limits.");
      xtalopt.setInputEleVolmString(before.value("elementalVolumes"));
    }

    // Validate the settings (KeepPrevious): an edit the makes settings invalid
    //   is restored to the previous value.
    Settings::validateSettings(xtalopt, Settings::InvalidSettingAction::KeepPrevious, &before);

    // Recompute compositions, volumes, radii, etc after restoring bad values.
    xtalopt.rebuildDerivedSettings();

    const bool iadModesChanged =
      Settings::scalarSettingValue(xtalopt, "usingScaledIADs") != before.value("usingScaledIADs") ||
      Settings::scalarSettingValue(xtalopt, "usingCustomIADs") != before.value("usingCustomIADs");
    if (iadModesChanged && xtalopt.getUsingCustomIAD() &&
        !xtalopt.verifyCustomIADValues(false)) {
      Settings::applyScalarSetting(xtalopt, "usingScaledIADs", before.value("usingScaledIADs"));
      Settings::applyScalarSetting(xtalopt, "usingCustomIADs", before.value("usingCustomIADs"));
      Common::warning("Runtime file: Custom IAD mode requires a complete customIAD table. "
                      "Keeping the previous IAD settings.");
    }

    // Report every runtime-changeable value that actually changed.
    for (const auto& keyword : Settings::allSettingKeywords()) {
      if (!Settings::hasScalarSettingBinding(keyword) || !Settings::isRuntimeKeyword(keyword))
        continue;
      const QString now = Settings::scalarSettingValue(xtalopt, keyword);
      if (now != before.value(keyword)) {
        settingsChanged = true;
        Common::warning("Runtime file: Updated option " + keyword + " = " + now);
        if (xtalopt.spacegroupKeywordInUse(keyword))
          spacegroupSettingsChanged = true;
        if (xtalopt.similarityKeywordInUse(keyword))
          similaritySettingsChanged = true;
        // Rebuild parent-selection values when they are next needed.
        if (xtalopt.selectionKeywordInUse(keyword))
          selectionSettingsChanged = true;
      }
    }
  }

  if (spacegroupSettingsChanged)
    xtalopt.resetSpacegroups();
  if (similaritySettingsChanged)
    xtalopt.resetSimilarities();
  if (selectionSettingsChanged && xtalopt.applyParentSelectionFronts()) {
    emit xtalopt.structureViewDataChanged();
    xtalopt.requestResultsFileSave();
  }
  if (settingsChanged) {
    xtalopt.warnVolumeLimitConflicts();
    xtalopt.requestStateFileSave();
  }
}

} // namespace

// Write the runtime file with the current value of every
//   runtime-changeable setting, one "keyword = value" per line.
void XtalOpt::saveRuntimeFile()
{
  if (!QDir().mkpath(getLocWorkDir()))
    return;

  QFile file(runtimeFilePath());
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    return;

  QString t = "# XtalOpt Run-Time File\n"
              "# Edit these options while the CLI run is active to update "
              "the search.\n\n";

  for (const auto& keyword : Settings::runtimeKeywords()) {
    t += keyword + " = " + Settings::scalarSettingValue(*this, keyword) + "\n";
  }

  const QByteArray bytes = t.toLocal8Bit();
  if (file.write(bytes) != bytes.size() || !file.flush() || file.error() != QFileDevice::NoError) {
    Common::error("Could not write the runtime file " + file.fileName());
  }
}

void XtalOpt::applyRuntimeText(const QString& runtimeText)
{
  QHash<QString, QString> options;
  QHash<QString, QStringList> multiInput;

  QString runtimeTextCopy = runtimeText;
  QTextStream stream(&runtimeTextCopy);
  while (!stream.atEnd()) {
    const QString line = stream.readLine();
    parseSettingLine(line, options, multiInput, "Runtime file:");
  }

  applyRuntimeSettings(options, multiInput, *this);
}

void XtalOpt::loadRuntimeFile()
{
  QString runtimeText;
  if (!Common::readFileToQString(runtimeFilePath(), &runtimeText))
    return;

  applyRuntimeText(runtimeText);
}

void XtalOpt::checkRuntimeFile()
{
  // Only refresh the runtime file in an active CLI session.
  if (!isSessionActive() ||
      (x_runMode != RunModeCliStart && x_runMode != RunModeCliResume))
    return;

  const QString filename = runtimeFilePath();
  if (filename.isEmpty() || !QFileInfo(filename).exists())
    return;

  QString runtimeText;
  if (!Common::readFileToQString(filename, &runtimeText))
    return;
  if (runtimeText == x_lastRuntimeText)
    return;

  // Do not read an unchanged runtime file. Warnings belong to one file update.
  x_lastRuntimeText = runtimeText;
  applyRuntimeText(runtimeText);
}

bool XtalOpt::saveInputFile(const QString& filename)
{
  if (filename.isEmpty())
    return false;

  QString output;
  QTextStream stream(&output);

  stream << "# NOTE: This file is generated from XtalOpt settings as a best-effort.\n";
  stream << "# Review template paths, job templates, and any optimizer or "
         << "queue settings before runnint it.\n";
  stream << "\n";

  writeInputText(stream, this);

  QFile file(filename);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    return false;

  QTextStream out(&file);
  out << output;
  out.flush();
  const bool success = out.status() == QTextStream::Ok && file.flush() && file.error() == QFileDevice::NoError;
  if (!success)
    Common::error("Could not write input file " + filename);
  return success;
}

// Main reader of the input file. Imports keep paths relative to the input file; a normal
// command-line read checks the templates and input files.
bool XtalOpt::loadInputFile(const QString& filename, bool bestEffort,
                            bool loadAndVerifyAssets)
{
  QString verboseReport;
  // Find the input file directory for relative input paths.
  const QString sourceDir =
    bestEffort ? QFileInfo(filename).absoluteDir().absolutePath() : QDir::currentPath();

  // Parse the raw keyword/value lines of the file.
  QHash<QString, QString> options;
  QHash<QString, QStringList> multiInput;
  if (!readInputFile(filename, options, multiInput, bestEffort, !isReadOnly()))
    return false;

  // A strict CLI start needs every required keyword before anything is applied.
  const bool requiredOptionsOk = hasRequiredInputValues(options);
  if (!requiredOptionsOk && !bestEffort)
    return false;

  // Apply all parsed options to this XtalOpt instance.
  if (!applyInputSettings(options, multiInput, *this,
                      sourceDir, loadAndVerifyAssets, bestEffort, &verboseReport))
    return false;

  // A GUI import tolerates missing required keywords while applying the rest,
  //   but still reports the gap to the caller.
  if (!requiredOptionsOk)
    return false;

  if (bestEffort) {
    // softExit only makes sense for a CLI start; an imported session should
    //   not auto-quit.
    setSoftExit(false);
  } else {
    // CLI startup echo.
    if (!reportInputSettings(options, *this))
      return false;
    if (!verboseReport.isEmpty())
      Common::message(verboseReport);
  }

  return true;
}

bool XtalOpt::processInputChemicalFormulas(QString s)
{
  // This function, one of the first things to be called, processes
  //   the input chemical formulas, and sets the list of compositions.
  // Input formula should all be of the same chemical system, and
  //   "full" chemical formula, i.e., proper combination of symbols
  //   and quantities, e.g., "Ti2O4" and not "TiO2".
  // An entry can also be a hyphen-separated list of "supercells",
  //   e.g., "Ti1O4 - Ti3O12".
  // If anything goes wrong, we will return false which quits the run.
  //   Examples of input formulae issues:
  //   - input formulae don't have correct format,
  //   - list of symbols of input formulae don't match the chemical system.
  //
  // At the end of this function, where we know the chemical system,
  //   we initiate atomic radii.
  //
  // If it returns true, the composition list is overwritten;
  //   otherwise it will have its previous value (if any).

  // Working output variable
  QList<CellComp> out;

  // Input list of formulas.
  const QStringList formulalist = s.split(',');

  // Process the input formula list and produce composition object.
  // At the end, we will check to see if we have any valid compositions,
  //   and if so, they belong to the same chemical system.
  for (const auto& tmpform : formulalist) {
    QString formula = tmpform.simplified();
    formula.replace(" ", "");

    // First, is this a "single" formula entry?
    if (!formula.contains("-")) {
      CellComp tmpcomp = formulaToComposition(formula);
      if (tmpcomp.getNumTypes() > 0) {
        out.append(tmpcomp);
        continue;
      } else {
        Common::error(QString("Incorrect chemical formula entry '%1'.").arg(formula));
        return false;
      }
    }

    // Then, we have a formula range ("-" entry) to deal with.
    QStringList expcomp = formula.split("-");
    if (expcomp.size() != 2) {
      Common::error(QString("Incorrect chemical formula entry '%1'.").arg(formula));
      return false;
    }

    // Convert limits to compositions; this makes it easier to verify and analyse them
    CellComp comp1 = formulaToComposition(expcomp[0]);
    CellComp comp2 = formulaToComposition(expcomp[1]);

    // Proceed only if they are legit compositions.
    if (comp1.getNumTypes() == 0 || comp2.getNumTypes() == 0) {
      Common::error(QString("Failed to process chemical formula entry '%1'.").arg(formula));
      return false;
    }

    // Formulae at the both end must be equivalent, proper supercells,
    //   and the second one being larger than the first.
    double ratio = compareCompositions(comp1, comp2);
    if (ratio == 0 || ratio != std::floor(ratio) || ratio < 1) {
      Common::error(QString("Incorrect chemical formula entry '%1'.").arg(formula));
      return false;
    }

    // Now: comp2 is a "proper supercell" of (or equal to) comp1.
    // Find the supercell ratio (largest expansion factor).
    uint quantratio = static_cast<unsigned int>(ratio);

    // Process all supercell formula and add them to composition list
    for (uint i = 1; i <= quantratio; i++) {
      QString frm = "";
      for (const auto& symb : comp1.getCompositionSymbols()) {
        frm += symb + QString::number(comp1.getCount(symb) * i);
      }
      out.append(formulaToComposition(frm));
    }
  }

  // A few final sanity checks.

  // Are we left with any valid formula?
  if (out.isEmpty()) {
    Common::error("No valid chemical formula was present in the list.");
    return false;
  }

  // All compositions must be non-empty; have the same number of types of elements
  if (out[0].getNumTypes() == 0) {
    Common::error("Empty formula is not accepted.");
    return false;
  }
  for (int i = 1; i < out.size(); i++) {
    if (out[i].getNumTypes() != out[0].getNumTypes()) {
      Common::error("Number of elements in formulas must be the same.");
      return false;
    }
    for (int j = 0; j < out[i].getNumTypes(); j++) {
      if (out[i].getCompositionAtomicNumbers()[j] != out[0].getCompositionAtomicNumbers()[j]) {
        Common::error("Element types in all formulas must be the same.");
        return false;
      }
    }
  }

  // Set the composition list
  compList() = out;

  // Finally, at this point we have the final list of elements in the search;
  //   time to set the initial elemental minimum radii!
  refreshElementMinRadii();

  return true;
}

bool XtalOpt::processInputReferenceEnergies(QString s)
{
  // This function processes the input reference energies (if any),
  //   and given a proper input, it sets the set of "lists" each
  //   for a reference entry and containing:
  //     "normalized composition plus the energy per atom".
  // In general, ref energy input entries include "formula energy"
  //   where the formula can be a subsystem of our chemical system.
  // We will ignore "empty" entries, and will return false:
  //  (1) if failed to read any "non-empty" entry,
  //  (2) if failed to read values for "all elements" in chemical system.
  //
  // This function sets the global variable "m_reference_energies".
  // If it returns true, the relevant global variable is overwritten;
  //   otherwise it will have its previous value (if any).

  // Basic sanity checks
  if (compList().isEmpty()) {
    Common::error(QString("%1: composition is not set.").arg(__func__));
    return false;
  }

  // List of chemical elements in the system
  QStringList chemSystem = getChemicalSystem();

  // To keep track of elemental references (if user provides ref energies)
  std::vector<int> eleRefs(chemSystem.size(), 0);

  // Final list of reference energies to return
  QList<RefEnergy> output_list;

  // Start processing the entries.
  int nonEmptyEntries = 0;
  QStringList entries = s.split(',');

  for (int i = 0; i < entries.size(); i++) {
    QString form;
    QList<double> parsedValues;
    QString parseError;

    // Ignore empty entries.
    if (entries[i].trimmed().isEmpty())
      continue;

    nonEmptyEntries += 1;

    if (!parseFormulaWithTrailingDoubles(entries[i], 1, form, parsedValues, &parseError)) {
      Common::error(parseError);
      return false;
    }
    const double ener = parsedValues.at(0);

    // Convert formula to composition: this makes sure we have a valid
    //   formula and simplifies obtaining needed information.
    CellComp comp = formulaToComposition(form);

    // If formula is not a "valid" composition, return false.
    if (comp.getNumTypes() == 0 || comp.getNumAtoms() == 0) {
      Common::error(QString("Could not read entry '%1' in reference energy list.").arg(entries[i]));
      return false;
    }

    // Also, all symbols in the entry should be in our chemical system.
    QList<QString> names = comp.getCompositionSymbols();
    for (int j = 0; j < names.size(); j++) {
      if (!chemSystem.contains(names[j])) {
        Common::error(QString("Unknown element '%1' in reference energy list.").arg(names[j]));
        return false;
      }
    }

    // So, the composition is good! See if it's an elemental reference
    if (comp.getNumTypes() == 1) {
      eleRefs[chemSystem.indexOf(names[0])]++;
    }
    // Finally, add the data to output list.
    output_list.append({comp, ener});
  }

  // If no non-empty entries are given, just construct the
  //   list with default "zero" value for elements.
  if (nonEmptyEntries == 0) {
    output_list.clear();
    for (int i = 0; i < chemSystem.size(); i++) {
      QString frm = chemSystem[i] + "1";
      CellComp comp = formulaToComposition(frm);
      output_list.append({comp, 0.0});
    }
    refEnergies() = output_list;
    return true;
  }

  // If user gives ref energies, at least one of each elements must be given.
  for (int i = 0; i < chemSystem.size(); i++) {
    if (eleRefs[i] == 0) {
      Common::error("Reference energies must include all elements.");
      return false;
    }
  }

  // We're good! Initiate the main reference energy "global" variable.
  refEnergies() = output_list;

  return true;
}

bool XtalOpt::processInputElementalVolumes(QString s)
{
  // This function processes the input elemental volumes (if any),
  //  and sets the elemental volume global variable if valid volume
  //  limits for all elements in chemical system are given.
  // We will ignore "empty" entries, and will return false:
  //  (1) if failed to read any "non-empty" entry,
  //  (2) if failed to read values for "all elements" in chemical system.
  //
  // This function sets the global variable "elemental_volumes".
  // If it returns true, the relevant global variable is overwritten;
  //   otherwise it will have its previous value (if any).

  // Basic sanity checks
  if (compList().isEmpty()) {
    Common::error(QString("%1: composition is not set.").arg(__func__));
    return false;
  }

  // List of chemical elements in the system
  QStringList chemSystem = getChemicalSystem();

  // To keep track of elemental references (if user provides ref energies)
  std::vector<int> eleVols(chemSystem.size(), 0);

  // Final processed elemental volumes.
  EleVolume out;

  // Start processing the entries.
  int nonEmptyEntries = 0;
  QStringList entries = s.split(',');

  for (int i = 0; i < entries.size(); i++) {
    QString form;
    QList<double> parsedValues;
    QString parseError;

    // Ignore empty entries.
    if (entries[i].trimmed().isEmpty())
      continue;

    nonEmptyEntries += 1;

    if (!parseFormulaWithTrailingDoubles(entries[i], 2, form, parsedValues, &parseError)) {
      Common::error(parseError);
      return false;
    }
    const double vmin = parsedValues.at(0);
    const double vmax = parsedValues.at(1);

    // If not a valid set of limits, just ignore the entry.
    if (vmin < ZERO06 || vmax < vmin) {
      Common::error(QString("Incorrect volume limits for elemental volume entry '%1'.").arg(entries[i]));
      return false;
    }

    // Convert formula to composition: this makes sure we have a valid
    //   formula and simplifies obtaining needed information.
    CellComp comp = formulaToComposition(form);

    // If entry is not proper (e.g., formula with more than one element) ignore it.
    if (comp.getNumTypes() != 1 || comp.getNumAtoms() == 0) {
      Common::error(QString("Could not process elemental volume entry '%1'.").arg(entries[i]));
      return false;
    }

    // Symbol must be on the list. Otherwise, ignore it.
    QString symbol = comp.getCompositionSymbols()[0];
    uint atomcn = comp.getCompositionAtomicNumbers()[0];
    uint totaln = comp.getNumAtoms();
    int ind = chemSystem.indexOf(symbol);
    if (ind == -1) {
      Common::error(QString("Elemental volume for unknown symbol '%1'.").arg(symbol));
      return false;
    } else {
      // Which element is this?!
      eleVols[chemSystem.indexOf(symbol)]++;
    }

    // Add volume data to composition object (we save "per atom" values)
    out.setElementVolumeRange(atomcn, vmin / totaln, vmax / totaln);
  }

  // If no non-empty entries are given, just return.
  if (nonEmptyEntries == 0) {
    eleVolumes().clearElementVolumes();
    return true;
  }

  // Otherwise, make sure every non-empty entry was read in successfully.
  if (nonEmptyEntries != out.getVolumeAtomicNumbers().size()) {
    Common::error("Failed to process some elemental volume entries.");
    return false;
  }

  // If elemental volumes are given; we should have one per chemical element.
  for (int i = 0; i < chemSystem.size(); i++) {
    if (eleVols[i] != 1) {
      Common::error("Elemental volumes must include all elements.");
      return false;
    }
  }

  // We're good! Assign the main elemental volume object
  eleVolumes().clearElementVolumes();
  eleVolumes() = out;

  return true;
}

bool XtalOpt::processInputCustomIAD(QString s)
{
  QStringList splitLine = s.split(",", QtCompat::SkipEmptyParts);
  if (splitLine.size() != 3) {
    Common::error("customIAD line must have 3 comma-delimited items on "
                  "the right-hand side of the equals sign.\nFaulty option "
                  "is as follows: customIAD = " + s +
                  ". Proper format is as follows: <firstSymbol>, " "<secondSymbol>, <minDistance>");
    return false;
  }
  QString firstSymbol = splitLine[0].trimmed(),
          secondSymbol = splitLine[1].trimmed();

  bool ok = false;
  double minDist = splitLine[2].toDouble(&ok);
  if (!ok || !GS_ISFINITE(minDist) || minDist <= 0.0) {
    Common::error("Invalid minDistance in customIAD line: " "distance: " + splitLine[2].trimmed() +
                  ". Proper format is as follows: <firstSymbol>, " "<secondSymbol>, <minDistance>");
    return false;
  }

  // Make sure the data is valid
  unsigned short firstAtomicNum =
    Atoms::ElementInfo::getAtomicNum(firstSymbol.toStdString());

  // If the atomic number is 0, the symbol is invalid
  if (firstAtomicNum == 0) {
    Common::error("Invalid atomic symbol in customIAD line: " "symbol: " + firstSymbol +
                  ". Proper format is as follows: <firstSymbol>, " "<secondSymbol>, <minDistance>");
    return false;
  }

  unsigned short secondAtomicNum =
    Atoms::ElementInfo::getAtomicNum(secondSymbol.toStdString());

  // If the atomic number is 0, the symbol is invalid
  if (secondAtomicNum == 0) {
    Common::error("Invalid atomic symbol in customIAD line: " "symbol: " + secondSymbol +
                  ". Proper format is as follows: <firstSymbol>, " "<secondSymbol>, <minDistance>");
    return false;
  }

  pairCustomDistances().setPairDistance(firstAtomicNum, secondAtomicNum, minDist);

  return true;
}

bool XtalOpt::processInputMoleculeUnit(QString s)
{
  s = s.simplified();
  if (s.isEmpty()) {
    Common::error("molUnit entry cannot be empty.");
    return false;
  }

  const QStringList fields = s.split(' ', QtCompat::SkipEmptyParts);
  if (fields.size() < 2) {
    Common::error("molUnit entry expects: <formula> <template>.");
    return false;
  }
  const QString selectedTemplate = stripOptionalQuotes(fields.last());
  QStringList formulaFields = fields;
  formulaFields.removeLast();
  s = formulaFields.join(" ");

  const CellComp requested = formulaToComposition(s);
  if (requested.getNumTypes() == 0 || requested.getNumAtoms() == 0) {
    Common::error(QString("Invalid molUnit composition: %1").arg(s));
    return false;
  }
  const QString normalizedFormula = requested.getFormula();

  Atoms::Geometry molecule;
  QString error;
  // Store molUnit templates at normal scale (a molUnit entry is a 0D Geometry).
  //   Random generation expands a selected unit later if the current IAD
  //   settings need it.
  if (!Atoms::buildMoleculeFromFormula(normalizedFormula.toStdString(),
                                       selectedTemplate.toStdString(),
                                       molecule, error)) {
    Common::error(error);
    return false;
  }

  if (!compositionFitsInAnyInputFormula(requested, compList())) {
    Common::error(QString("molUnit composition %1 does not fit in any input formula.").arg(s));
    return false;
  }

  moleculeUnitInputs().append(QString("%1 %2").arg(normalizedFormula).arg(selectedTemplate));

  moleculeUnits().push_back(molecule);
  return true;
}

bool XtalOpt::processInputObjectives(QString s)
{
  // This function processes the objective entries from a string
  //   input that includes all relevant fields:
  //   type, executable, output filename, and weight.

  ensureBuiltinObjective();

  // Total weight of objectives
  double totalweight = 0.0;

  for (int userObjective = 0; userObjective < getUserObjectivesNum(); ++userObjective)
    totalweight += getObjectivesWgt(getUserObjectiveIndex(userObjective));

  // Process entries

  QStringList sline = s.split(" ", QtCompat::SkipEmptyParts);
  // There should be four entries in the input: (1) objective
  //   type (min/max), (2) executable script, (3) output
  //   filename, and (4) weight.
  if (sline.size() != 4) {
    Common::error("objective is not properly initiated");
    return false;
  }

  // 1st item is always objective's type: min/max
  QString tmps = sline.at(0).toLower().mid(0, 3);
  ObjType objtyp;

  if (tmps == "min")
    objtyp = ObjType::Ot_Min;
  else if (tmps == "max")
    objtyp = ObjType::Ot_Max;
  else {
    Common::error(tr("unknown objective type: '%1' in '%2'").arg(tmps).arg(s));
    return false;
  }

  // 2nd item is always the script path
  QString objexe = sline.at(1);

  // 3rd item is the output filename
  QString objout = sline.at(2);

  // 4th item is the weight
  bool isNumber;
  double objwgt = sline.at(3).toDouble(&isNumber);
  if (!isNumber) {
    Common::error("objective weight should be a digit in [0,1]");
    return false;
  }

  QString definitionError;
  if (!validateUserObjectiveDefinition(objtyp, objexe, objout, objwgt, &definitionError)) {
    Common::error(definitionError);
    return false;
  }

  // Sanity check: total weights should be less than or equal to 1
  if (totalweight + objwgt > 1.0) {
    Common::error("total weight of objectives can't exceed 1.0");
    return false;
  }

  // We're good! Add the objective
  addObjective(objtyp, objexe, objout, objwgt);

  return true;
}

bool XtalOpt::processInputConstraint(QString s)
{
  QStringList sline = s.split(" ", QtCompat::SkipEmptyParts);
  if (sline.size() != 2) {
    Common::error("constraint is not properly initiated");
    return false;
  }

  const QString exe = sline.at(0).trimmed();
  const QString out = sline.at(1).trimmed();
  QString err;
  if (!validateConstraintDefinition(exe, out, &err)) {
    Common::error(err);
    return false;
  }

  addConstraint(exe, out);
  return true;
}

bool XtalOpt::rebuildDerivedSettings()
{
  // Reconstruct composition, volume, radius, and objective data from the input
  //   settings; safe to call repeatedly.
  bool ok = true;

  // 1a) Compositions from chemicalFormulas. An empty input clears them; a
  //     non-empty but malformed value fails (callers treat this like the old
  //     reader return).
  if (getInputFormulasString().trimmed().isEmpty())
    compList().clear();
  else if (!processInputChemicalFormulas(getInputFormulasString()))
    ok = false;

  // 1b) Reference energies depend on a composition (with one but no input,
  //     processInputReferenceEnergies builds the default zero references). With
  //     no composition there are no references.
  if (compList().isEmpty())
    refEnergies().clear();
  else if (ok && !processInputReferenceEnergies(getInputEneRefsString()))
    ok = false;

  // 1c) Elemental volumes depend on a composition (lenient: a bad value is
  //     ignored, not fatal). With no composition there are no volumes.
  if (compList().isEmpty())
    eleVolumes().clearElementVolumes();
  else if (!processInputElementalVolumes(getInputEleVolmString()))
    Common::warning("Ignoring elemental volume input.");

  // 1d) Per-element minimum radii (also refreshed inside the composition parse;
  //    doing it here too lets rebuildDerivedSettings() recompute everything on its own).
  refreshElementMinRadii();

  // 2) Built-in above-hull objective weight.
  refreshBuiltinObjectiveWeight();

  return ok;
}

// Process one user objective as the "min|max exe out wgt".
//   The same format is parsed back by processInputObjectives().
QString XtalOpt::objectiveEntryToText(int objectiveIndex) const
{
  const QString objType = getObjectivesTyp(objectiveIndex) == SearchBase::Ot_Min ? "min" : "max";

  return objType + " " + getObjectivesExe(objectiveIndex) + " " +
         getObjectivesOut(objectiveIndex) + " " + QString::number(getObjectivesWgt(objectiveIndex));
}

// Return the repeated keywords' values.
QStringList XtalOpt::objectiveLines() const
{
  QStringList out;
  for (int i = 0; i < getUserObjectivesNum(); ++i)
    out << objectiveEntryToText(getUserObjectiveIndex(i));
  return out;
}

// Process one constraint as the "exe out".
//   The same format is parsed back by processInputConstraint().
QString XtalOpt::constraintEntryToText(int constraintIndex) const
{
  return getConstraintExe(constraintIndex) + " " + getConstraintOut(constraintIndex);
}

QStringList XtalOpt::constraintLines() const
{
  QStringList out;
  for (int i = 0; i < getConstraintsNum(); ++i)
    out << constraintEntryToText(i);
  return out;
}

// Process one custom-IAD pair as the "sym1, sym2, minIAD".
QString XtalOpt::customIADEntryToText(int atomicNumber1, int atomicNumber2, double minIAD)
{
  return QString("%1, %2, %3")
    .arg(Atoms::ElementInfo::getAtomicSymbol(atomicNumber1).c_str())
    .arg(Atoms::ElementInfo::getAtomicSymbol(atomicNumber2).c_str())
    .arg(minIAD, 0, 'g', std::numeric_limits<double>::max_digits10);
}

// Return the custom IAD values.
QStringList XtalOpt::customIADLines() const
{
  QStringList out;
  for (const auto& pair : pairCustomDistances().getPairs())
    out << customIADEntryToText(pair.first, pair.second,
                               pairCustomDistances().getPairDistance(pair.first, pair.second));
  out.sort();
  return out;
}

QStringList XtalOpt::molUnitLines() const
{
  return moleculeUnitInputs();
}} // namespace XtalOpt

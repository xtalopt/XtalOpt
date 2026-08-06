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

#ifndef XTALOPT_SETTINGS_H
#define XTALOPT_SETTINGS_H

#include <QHash>
#include <QString>
#include <QStringList>

namespace XtalOpt {

class XtalOpt;

namespace Settings {

// Return the "official" keyword name, or an empty string if it is unknown.
QString findKeywordName(const QString& raw);

QString defaultValue(const QString& keyword);

bool isRequiredInputKeyword(const QString& keyword);

bool isRuntimeKeyword(const QString& keyword);

bool isRepeatableInputKeyword(const QString& keyword);

QStringList allSettingKeywords();

QStringList requiredInputKeywords();

QStringList runtimeKeywords();

bool hasScalarSettingBinding(const QString& keyword);

// Set a scalar value from text. Return false when it has no scalar setter.
bool applyScalarSetting(XtalOpt& opt, const QString& keyword, const QString& value);

// Return the value of a scalar setting as text.
QString scalarSettingValue(const XtalOpt& opt, const QString& keyword);

// Repeated entries
bool hasRepeatedSettingBinding(const QString& keyword);
QStringList repeatedSettingEntries(const XtalOpt& opt, const QString& keyword);
void clearRepeatedSetting(XtalOpt& opt, const QString& keyword);
bool addRepeatedSettingEntry(XtalOpt& opt, const QString& keyword, const QString& entry);

void applyDefaultSettings(XtalOpt& opt);

// A copy of all keywords/values, used for checking and restoring values.
typedef QHash<QString, QString> ScalarSnapshot;
ScalarSnapshot captureScalarSettings(const XtalOpt& opt);

// How validateSettings() handles an unacceptable value:
//  - Reject:         fail (return false); for fresh start, user must fix input.
//  - ResetToDefault: reset the value to default and warn the user, then proceed.
//  - KeepPrevious:   restore value from last good known and warn the user, then proceed.
enum class InvalidSettingAction { Reject, ResetToDefault, KeepPrevious };

// Check all settings in one place.
bool validateSettings(XtalOpt& opt, InvalidSettingAction invalidAction, const ScalarSnapshot* base = nullptr);

// Return the list of all keywords (used by --xtalopt-flags).
QString keywordSummaryText();

// Convert between optimizer template files and input keywords.
QString optimizerTemplateFilenameToKeyword(const QString& filename);
QString optimizerTemplateKeywordToFilename(const QString& keyword);

// Return the input asset keyword for an optimizer file.
QString optimizerInputAssetToKeyword(const QString& assetName);

// Return the input keyword for a queue template.
const char* queueTemplateKeyword();

// Check whether keyword names an input file (optimizer template/asset or queue job template).
bool isInputJobFileKeyword(const QString& keyword);

} // namespace Settings

} // namespace XtalOpt

#endif // XTALOPT_SETTINGS_H

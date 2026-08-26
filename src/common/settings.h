/**********************************************************************
  settings - Help macro for QSettings

  Copyright (C) 2010-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef COMMON_SETTINGS_H
#define COMMON_SETTINGS_H

#include <QSettings>

#include <memory>

// Create a pointer of type QSettings *settings that writes to "f".
// An explicit file path is required ("f" must be non-empty): the default
//   application QSettings would write to the platform-global preferences.
#define QSETTINGS_FILE(f)                                                      \
  const QString qsFilename = QString(f);                                       \
  Q_ASSERT_X(!qsFilename.isEmpty(), "QSETTINGS_FILE",                          \
             "Explicit settings filename required");                           \
  QSettings qsFile(qsFilename, QSettings::IniFormat);                          \
  QSettings* settings = &qsFile;

#endif // COMMON_SETTINGS_H

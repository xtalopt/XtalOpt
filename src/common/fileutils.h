/**********************************************************************
  fileutils - File and path utilities

  Copyright (C) 2015 XtalOpt Developers
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef COMMON_FILEUTILS_H
#define COMMON_FILEUTILS_H

#include <QList>
#include <QString>

#include <string>


namespace Common {

QString localPath(const QString& base, const QString& child);
QString remotePath(const QString& base, const QString& child);
QString quoteRemotePath(const QString& path);
bool readFileToString(const QString& filename, std::string* contents);

// Produces a local 8-bit; returns false (contents untouched) if open fails.
bool readFileToQString(const QString& filename, QString* contents);

bool isReadableFile(const QString& path);
bool isReadableDirectory(const QString& path);

// Recursively copy sourceDir into destDir.
bool copyDir(const QString& sourceDir, const QString& destDir);

// Recursively delete dirName and all its contents.
bool removeDir(const QString& dirName);

} // namespace Common

#endif // COMMON_FILEUTILS_H

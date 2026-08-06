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

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QString>

#include "fileutils.h"

namespace Common {

QString localPath(const QString& base, const QString& child)
{
  if (child.isEmpty())
    return base;
  if (base.isEmpty() || QFileInfo(child).isAbsolute())
    return child;
  return QDir(base).filePath(child);
}

QString remotePath(const QString& base, const QString& child)
{
  if (child.isEmpty())
    return base;
  if (base.isEmpty() || child.startsWith('/') || child == "~" || child.startsWith("~/"))
    return child;

  QString left = base;
  while (left.size() > 1 && left.endsWith('/'))
    left.chop(1);

  QString right = child;
  while (right.startsWith('/'))
    right.remove(0, 1);

  if (left == "/")
    return left + right;
  return left + "/" + right;
}

QString quoteRemotePath(const QString& path)
{
  if (path == "~")
    return path;

  QString quoted = path;
  if (quoted.startsWith("~/"))
    quoted.remove(0, 2);
  quoted.replace("'", "'\\''");
  quoted = "'" + quoted + "'";
  return path.startsWith("~/") ? "~/" + quoted : quoted;
}

bool readFileToString(const QString& filename, std::string* contents)
{
  if (contents == nullptr)
    return false;

  QFile file(filename);
  // Text mode converts "\r\n" line endings to "\n", so readers always
  //   get the same text no matter where the file was written.
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return false;

  const QByteArray data = file.readAll();
  contents->assign(data.constData(), static_cast<size_t>(data.size()));
  return true;
}

bool readFileToQString(const QString& filename, QString* contents)
{
  if (contents == nullptr)
    return false;

  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return false;

  *contents = QString::fromLocal8Bit(file.readAll());
  return true;
}

bool isReadableFile(const QString& path)
{
  const QFileInfo info(path);
  return info.exists() && info.isFile() && info.isReadable();
}

bool isReadableDirectory(const QString& path)
{
  const QFileInfo info(path);
  return info.exists() && info.isDir() && info.isReadable();
}

bool copyDir(const QString& sourceDir, const QString& destDir)
{
  QDir source(sourceDir);
  if (!source.exists())
    return false;

  if (!QDir().mkpath(destDir))
    return false;

  const QFileInfoList entries = source.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries |
                         QDir::Hidden | QDir::System);
  for (const QFileInfo& entry : entries) {
    const QString srcPath = entry.absoluteFilePath();
    const QString dstPath = localPath(destDir, entry.fileName());

    // Do not follow a directory link: it can make a loop!
    if (entry.isSymLink() && entry.isDir())
      continue;

    if (entry.isDir()) {
      if (!copyDir(srcPath, dstPath))
        return false;
      continue;
    }

    QFile::remove(dstPath);
    if (!QFile::copy(srcPath, dstPath))
      return false;
  }

  return true;
}

bool removeDir(const QString& dirName)
{
// Adapted from john.nachtimwald.com/2010/06/08/qt-remove-directory-and-its-contents/
  bool result = true;
  QDir dir(dirName);

  if (dir.exists(dirName)) {
    const QFileInfoList entries = dir.entryInfoList(
      QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::AllDirs | QDir::Files,
      QDir::DirsFirst);
    for (const QFileInfo& info : entries) {
      // Do not follow a directory link: remove the link, not its contents.
      if (info.isSymLink() && info.isDir()) {
        result = QFile::remove(info.absoluteFilePath()) || dir.rmdir(info.absoluteFilePath());
      } else if (info.isDir()) {
        result = removeDir(info.absoluteFilePath());
      } else {
        result = QFile::remove(info.absoluteFilePath());
      }

      if (!result) {
        return result;
      }
    }
    result = dir.rmdir(dirName);
  }

  return result;
}

} // namespace Common

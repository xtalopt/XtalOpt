#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QString>
#include <QTextStream>

#include "fileutils.h"

// Obtained from:
// http://john.nachtimwald.com/2010/06/08/qt-remove-directory-and-its-contents/
// on 04/14/2015

/*!
   Delete a directory along with all of its contents.

   \param dirName Path of directory to remove.
   \return true on success; false on error.
*/
bool FileUtils::removeDir(const QString& dirName)
{
  bool result = true;
  QDir dir(dirName);

  if (dir.exists(dirName)) {
    Q_FOREACH (QFileInfo info,
               dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System |
                                   QDir::Hidden | QDir::AllDirs | QDir::Files,
                                 QDir::DirsFirst)) {
      if (info.isDir()) {
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

QList<uint> FileUtils::parseUIntString(const QString& s, QString& result)
{
  QList<bool> series;
  result = "";
  QTextStream string(&result);

  QStringList sList = s.split(",", QString::SkipEmptyParts);
  QStringList sList2;

  // Fix to correct crashing when there is a hyphen at the beginning
  if (!sList.isEmpty()) {
    sList[0].prepend(" ");
  }

  // Check for values that begin, are between, or end hyphens
  int j = 0;
  bool isNumeric;
  for (int i = 0; i < sList.size(); i++) {
    sList2 = sList.at(i).split("-", QString::SkipEmptyParts);
    if (sList2.at(0) != sList.at(i)) {
      sList2.at(0).toUInt(&isNumeric);
      if (isNumeric == true) {
        sList2.at(1).toUInt(&isNumeric);
        if (isNumeric == true) {
          uint smaller = sList2.at(0).toUInt();
          uint larger = sList2.at(1).toUInt();
          if (larger < smaller) {
            smaller = sList2.at(1).toUInt();
            larger = sList2.at(0).toUInt();
          }
          for (j = smaller; j <= larger; j++) {
            sList.append(QString::number(j));
          }
        }
      }
    }
  }

  // Remove leading zeros
  for (int i = 0; i < sList.size(); i++) {
    while (sList.at(i).trimmed().startsWith("0")) {
      sList[i].remove(0, 1);
    }
  }

  // Check that each QString may be converted to an unsigned int. Discard it if
  // it cannot.
  for (int i = 0; i < sList.size(); i++) {
    sList.at(i).toUInt(&isNumeric);
    if (isNumeric == false) {
      sList.removeAt(i);
      i--;
    }
  }

  // Remove all numbers greater than 100
  for (int i = 0; i < sList.size(); i++) {
    if (sList.at(i).toUInt() > 100) {
      sList.removeAt(i);
    }
  }

  // If nothing valid was entered, return 1
  if (sList.size() == 0) {
    string << "1";
    return QList<uint>();
  }

  // Remove duplicates from the sList
  for (int i = 0; i < sList.size() - 1; i++) {
    for (int j = i + 1; j < sList.size(); j++) {
      if (sList.at(i) == sList.at(j)) {
        sList.removeAt(j);
        j--;
      }
    }
  }

  // Sort from smallest value to greatest value
  for (int i = 0; i < sList.size() - 1; i++) {
    for (int j = i + 1; j < sList.size(); j++) {
      if (sList.at(i).toUInt() > sList.at(j).toUInt()) {
        sList.swap(i, j);
      }
    }
  }

  // Populate series with false
  series.clear();
  for (int i = 0; i < sList.size(); i++) {
    series.append(false);
  }

  // Check for series to hyphenate
  for (int i = 0; i < sList.size() - 2; i++) {
    if ((sList.at(i).toUInt() + 1 == sList.at(i + 1).toUInt()) &&
        (sList.at(i + 1).toUInt() + 1 == sList.at(i + 2).toUInt())) {
      series.replace(i, true);
      series.replace(i + 1, true);
      series.replace(i + 2, true);
    }
  }

  // Create the text stream to put back into the UI
  for (int i = 0; i < sList.size(); i++) {
    if (series.at(i) == false) {
      if (i + 1 == sList.size()) {
        string << sList.at(i).trimmed();
      } else if (i + 1 != sList.size()) {
        string << sList.at(i).trimmed() << ", ";
      }
    } else if (series.at(i) == true) {
      uint seriesLength = 1;
      int j = i + 1;
      while ((j != sList.size()) && (series.at(j) == true) &&
             (sList.at(j - 1).toUInt() + 1 == sList.at(j).toUInt())) {
        seriesLength += 1;
        j++;
      }
      if (i + seriesLength == sList.size()) {
        string << sList.at(i).trimmed() << " - " << sList.at(j - 1).trimmed();
      } else if (i + seriesLength != sList.size()) {
        string << sList.at(i).trimmed() << " - " << sList.at(j - 1).trimmed()
               << ", ";
      }
      i = i + seriesLength - 1;
    }
  }

  // Create the uintList
  QList<uint> uintList;
  uintList.clear();
  for (int i = 0; i < sList.size(); i++) {
    uintList.append(sList.at(i).toUInt());
  }
  return uintList;
}

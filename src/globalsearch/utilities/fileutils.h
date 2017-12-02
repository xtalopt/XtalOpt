#ifndef FILEUTILS_H
#define FILEUTILS_H

class QString;

// Obtained from:
// http://john.nachtimwald.com/2010/06/08/qt-remove-directory-and-its-contents/
// on 04/14/2015

class FileUtils
{
public:
  static bool removeDir(const QString& dirName);
  // This function, written by Patrick Avery, will parse a string s
  // and return a list of the unsigned integers found in it.
  // If anything invalid is found in it (like a character), it will
  // return nothing.
  // It will also add to 'result' an organized version of s that is
  // arranged in order of increasing number separated by commas.
  // If three or more numbers are in series, they will be hyphenated
  static QList<uint> parseUIntString(const QString& s, QString& result);
};

#endif // FILEUTILS_H

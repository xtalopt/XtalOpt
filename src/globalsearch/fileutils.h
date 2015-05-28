#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <QString>

// Obtained from:
// http://john.nachtimwald.com/2010/06/08/qt-remove-directory-and-its-contents/
// on 04/14/2015

class FileUtils
{
public:
    static bool removeDir(const QString &dirName);
};

#endif // FILEUTILS_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QRegExp>

#define _HAVE_EXECINFO_H_ 1

#ifdef _HAVE_EXECINFO_H_
#include <execinfo.h>
#include <cxxabi.h>
#endif

QStringList getBackTrace();

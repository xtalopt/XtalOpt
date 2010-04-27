#include <QObject>
#include <QString>
#include <QStringList>
#include <QRegExp>

#define _HAVE_EXECINFO_H_ 1

#ifdef _HAVE_EXECINFO_H_
#include <execinfo.h>
#include <cxxabi.h>
#endif

QRegExp regexp("([^(]+)\\(([^)^+]+)(\\+[^)]+)\\)\\s(\\[[^]]+\\])");

QStringList getBackTrace();

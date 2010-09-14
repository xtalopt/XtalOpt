#include <QtCore/QStringList>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QRegExp>

#ifdef _HAVE_EXECINFO_H_
#include <execinfo.h>
#include <cxxabi.h>
#endif

QStringList getBackTrace();

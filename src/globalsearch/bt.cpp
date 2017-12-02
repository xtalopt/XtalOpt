//
// Function to get a backtrace
//
// Fixed so that it correctly demangles the function names!
//
// See glibc documentation for backtrace
// (http://www.gnu.org/software/libc/manual/html_node/Backtraces.html#Backtraces)
//
// This code comes from a mailing list discussion at:
// http://lists.trolltech.com/qt-interest/2006-07/thread00444-0.html
// I owe the author my eternal gratitude.

#include <QDebug>
#include <QObject>
#include <QRegExp>
#include <QString>
#include <QStringList>

#ifdef _HAVE_EXECINFO_H_
#include <cxxabi.h>
#include <execinfo.h>
#endif

// need to extract the symbol from the output of 'backtrace_symbols'

// a typical output from backtrace_symbols will look like;
// a.out(_ZNK1A12getBackTraceEv+0x12) [0x804ad36]

// This needs to be split into;
//  (1) The program or library containing the symbol (a.out)
//  (2) The symbol itself (_ZNK1A12getBackTraceEv)
//  (3) The offset? +0x12
//  (4) The symbol address ([0x804ad36])

// This is achieved by the regexp below

//              (unit )  (symbol)   (offset)          (address)
QRegExp btregexp("([^(]+)\\(([^)^+]+)(\\+[^)]+)\\)\\s(\\[[^]]+\\])");

/** Obtain a backtrace and return as a QStringList.
    This is not well-optimised, requires compilation with "-rdynamic" on linux
    and doesn't do a great job of demangling the symbol names. It is sufficient
    though to work out call trace. */
QStringList getBackTrace()
{
// now get the backtrace of the code at this point
//(we can only do this if we have 'execinfo.h'
#ifdef _HAVE_EXECINFO_H_

  // create a void* array to hold the function addresses. We will only go at
  // most 25 deep
  const int maxdepth(25);

  void* func_addresses[maxdepth];
  int nfuncs = backtrace(func_addresses, maxdepth);

  // now get the function names associated with these symbols. This should work
  // for elf
  // binaries, though additional linker options may need to have been called
  //(e.g. -rdynamic for GNU ld. See the glibc documentation for 'backtrace')
  char** symbols = backtrace_symbols(func_addresses, nfuncs);

  // save all of the function names onto the QStringList....
  //(note that none of this will work if we have run out of memory)
  QStringList ret;

  for (int i = 0; i < nfuncs; i++) {
    if (btregexp.indexIn(symbols[i]) != -1) {
      // get the library or app that contains this symbol
      QString unit = btregexp.cap(1);
      // get the symbol
      QString symbol = btregexp.cap(2);
      // get the offset
      QString offset = btregexp.cap(3);
      // get the address
      QString address = btregexp.cap(4);

      // now try and demangle the symbol
      int stat;
      char* demangled = abi::__cxa_demangle(qPrintable(symbol), 0, 0, &stat);

      if (demangled) {
        symbol = demangled;
        delete demangled;
      }

      // put this all together
      ret.append(QString("%1: %2  %3").arg(unit, symbol, address));
    } else {
      // I don't recognise this string - just add the raw
      // string to the backtrace
      ret.append(symbols[i]);
    }
  }

  // we now need to release the memory of the symbols array. Since it was
  // allocated using
  // malloc, we must release it using 'free'
  delete symbols;

  return ret;

#else
  return QStringList(
    QObject::tr("Backtrace is not available without execinfo.h"));
#endif
}

void printBackTrace()
{
  QStringList bt = getBackTrace();
  for (const auto& item : bt)
    qDebug() << item;
}

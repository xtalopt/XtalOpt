#ifndef EXCEPTIONHANDLER_H
#define EXCEPTIONHANDLER_H

#include <QString>

class ExceptionHandler
{
 public:
  // This function will catch all exceptions and attempt to print it out
  // into the terminal (if it catches a QString or std::string or
  // std::exception). It is currently being used for destructors because
  // destructors should never throw. The function name should be passed in
  // Usage: call this in a catch(){} block. It rethrows the exception and will
  // catch it and attempt to read it.
  static void handleAllExceptions(const QString& funcName = "");
};

#endif // EXCEPTIONHANDLER_H

#include <QDebug>
#include <QString>

#include "exceptionhandler.h"

void ExceptionHandler::handleAllExceptions(const QString& msg)
{
  // This will re-throw the exception that was thrown
  try {
    throw;
  }
  // catch ANY exception that is thrown. If a std::exception, Qstring, or
  // std::string was thrown, print it out. Otherwise, it is an unknown
  // exception.
  catch(const std::exception& ex) {
    qWarning() << "Error." << msg << "has caught a std::exception:" <<ex.what();
  }
  catch(const QString& ex) {
    qWarning() << "Error." << msg << "has caught a Qstring:" << ex;
  }
  catch(const std::string& ex) {
    qWarning() << "Error." << msg
             << "has caught a std::string:" << QString::fromStdString(ex);
  }
  catch(...) {
    qWarning() << "Error." << msg << "has caught an unknown exception";
  }
}

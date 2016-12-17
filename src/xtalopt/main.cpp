#include <QApplication>

#include <xtalopt/ui/dialog.h>

int main(int argc, char* argv[])
{
 // Set up groups for QSettings
  QCoreApplication::setOrganizationName("XtalOpt");
  QCoreApplication::setOrganizationDomain("xtalopt.github.io");
  QCoreApplication::setApplicationName("XtalOpt");

  QApplication app(argc, argv);
  XtalOpt::XtalOptDialog d;
  d.show();
  return app.exec();
}

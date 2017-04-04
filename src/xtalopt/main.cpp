#include <QApplication>

#include <xtalopt/cliOptions.h>
#include <xtalopt/ui/dialog.h>
#include <xtalopt/xtalopt.h>

int main(int argc, char* argv[])
{
  // Loop through command line arguments
  bool noGUI = false;
  QString inputfile = "xtalopt.in";

  // The names of the argument options go here
  static const QString noGUIArgumentStr = "--no-gui",
                       inputfileArgumentStr = "--input-file=";

  for (size_t i = 1; i < argc; ++i) {
    QString arg(argv[i]);
    if (arg == noGUIArgumentStr) {
      noGUI = true;
    }
    else if (arg.startsWith(inputfileArgumentStr)) {
      arg.remove(0, inputfileArgumentStr.size());
      inputfile = arg;
    }
    else {
      qDebug() << "Warning: ignoring unrecognized option" << arg;
    }
  }

  // Set up groups for QSettings
  QCoreApplication::setOrganizationName("XtalOpt");
  QCoreApplication::setOrganizationDomain("xtalopt.github.io");
  QCoreApplication::setApplicationName("XtalOpt");

  QApplication app(argc, argv);

  // It would be nice if sometime in the future we didn't have to create
  // all the dialogs for a run that doesn't use the GUI. However, it is
  // deeply integrated and hard to do now, so we are going to create
  // the dialogs anyways
  XtalOpt::XtalOptDialog d;

  if (noGUI) {
    XtalOpt::XtalOpt& xtalopt =
      *qobject_cast<XtalOpt::XtalOpt*>(d.getOptBase());
    xtalopt.setUsingGUI(false);

    if (!XtalOpt::XtalOptCLIOptions::readOptions(inputfile, xtalopt))
      return 1;
    if (!xtalopt.startSearch())
      return 1;
  }
  // If we are using the GUI, show the dialog...
  else {
    d.show();
  }

  return app.exec();
}

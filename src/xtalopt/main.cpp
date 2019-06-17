/**********************************************************************
  main.cpp - The main() function to be used by XtalOpt 11.0 and beyond.

  Copyright (C) 2016-2017 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#include <QApplication>
#include <QCommandLineParser>

#include <globalsearch/utilities/makeunique.h>

#include <xtalopt/cliOptions.h>
#include <xtalopt/ui/dialog.h>
#include <xtalopt/xtalopt.h>

int main(int argc, char* argv[])
{
  // Unfortunately, it is becoming more and more difficult to run
  // QCommandLineParser without having a QApplication first. For us,
  // it is ideal to run QCommandLineParser first because we want to
  // determine whether we are using CLI mode or not (which will
  // determine whether we instantiate a QApplication or
  // QCoreApplication)

  // Because we run into great difficulties, let's examine the arguments
  // manually and determine whether or not we are in CLI mode first, and
  // then perform the rest of the QCommandLineParser actions
  const char* cliModeStr = "--cli";
  const char* cliResumeStr = "--resume";

  bool cliMode = false;
  bool cliResume = false;
  for (int i = 0; i < argc; ++i) {
    const QString& curArg(argv[i]);
    if (curArg == cliModeStr) {
      cliMode = true;
    }
    else if (curArg == cliResumeStr) {
      cliResume = true;
    }
  }

  // If we are running in CLI mode, we want a QCoreApplication
  // If we are running in GUI mode, we want a QApplication
  std::unique_ptr<QCoreApplication> app =
    (cliMode || cliResume) ? make_unique<QCoreApplication>(argc, argv)
                           : make_unique<QApplication>(argc, argv);

  // Now that we have the QApplication, we can proceed with the rest
  // of the command line options.

  // Set up groups for QSettings
  QCoreApplication::setOrganizationName("XtalOpt");
  QCoreApplication::setOrganizationDomain("xtalopt.github.io");
  QCoreApplication::setApplicationName("XtalOpt");
  QCoreApplication::setApplicationVersion("12.1");

  QCommandLineParser parser;
  parser.setApplicationDescription("XtalOpt: an open-source evolutionary "
                                   "algorithm for crystal structure "
                                   "prediction");
  parser.addHelpOption();
  parser.addVersionOption();

  QCommandLineOption cliModeOption(
    QStringList() << QString(cliModeStr).remove(0, 2), // Remove "--"
    QCoreApplication::translate("main",
                                "Use the command-line interface (CLI) mode."));
  parser.addOption(cliModeOption);

  QCommandLineOption cliResumeOption(
    QStringList() << QString(cliResumeStr).remove(0, 2), // Remove "--"
    QCoreApplication::translate("main", "Resume an XtalOpt run in CLI mode."));
  parser.addOption(cliResumeOption);

  QCommandLineOption inputFileOption(
    QStringList() << "input-file",
    QCoreApplication::translate("main", "Specify the input file for CLI mode."),
    QCoreApplication::translate("main", "file"));
  inputFileOption.setDefaultValue("xtalopt.in");
  parser.addOption(inputFileOption);

  QCommandLineOption plotModeOption(
    QStringList() << "plot",
    QCoreApplication::translate(
      "main", "Show a plot of a specified XtalOpt directory."));
  parser.addOption(plotModeOption);

  QCommandLineOption dataDirOption(
    QStringList() << "dir",
    QCoreApplication::translate(
      "main", "Specify the XtalOpt results directory to be used for a CLI "
              "resume or a plot."),
    QCoreApplication::translate("main", "directory"));
  parser.addOption(dataDirOption);

  // Make a QStringList of the arguments
  QStringList args;
  for (int i = 0; i < argc; ++i)
    args << argv[i];

  // Process the arguments
  parser.process(args);

  bool plotMode = parser.isSet(plotModeOption);

  QString inputfile = parser.value(inputFileOption);
  QString dataDir = parser.value(dataDirOption);

  // Make sure we have valid options set...
  if (plotMode && !parser.isSet(dataDirOption)) {
    qDebug() << "To use plot mode, you must specify an XtalOpt results"
             << "directory with --dir";
    return 1;
  }

  if (cliResume && !parser.isSet(dataDirOption)) {
    qDebug() << "To resume an XtalOpt run in CLI mode, you must specify an"
             << "XtalOpt results directory with --dir";
    return 1;
  }

  if (plotMode && cliMode) {
    qDebug() << "Error: you cannot use CLI mode and plot mode"
             << "at the same time!";
    return 1;
  }

  if (plotMode && cliResume) {
    qDebug() << "Error: you cannot resume in CLI mode and use plot mode"
             << "at the same time!";
    return 1;
  }

  // XtalOptDialog needs to be destroyed before XtalOpt gets destroyed. So
  // the ordering here matters.
  XtalOpt::XtalOpt xtalopt;
  std::unique_ptr<XtalOpt::XtalOptDialog> d;

  if (cliMode) {
    xtalopt.setUsingGUI(false);

    if (!XtalOpt::XtalOptCLIOptions::readOptions(inputfile, xtalopt))
      return 1;
    if (!xtalopt.startSearch())
      return 1;
  }
  // We just want to generate a plot tab and display it...
  else if (plotMode) {
    d = std::move(
      make_unique<XtalOpt::XtalOptDialog>(nullptr, Qt::Window, true, &xtalopt));
    xtalopt.setDialog(d.get());
    if (!xtalopt.plotDir(dataDir))
      return 1;
    d->beginPlotOnlyMode();
  } else if (cliResume) {
    xtalopt.setUsingGUI(false);

    // Make sure the state file exists
    if (!QDir(dataDir).exists("xtalopt.state")) {
      qDebug() << "Error: no xtalopt.state file found in" << dataDir;
      qDebug() << "Please check your --dir option and try again";
      return 1;
    }

    // Try to load the state file
    if (!xtalopt.load(QDir(dataDir).filePath("xtalopt.state")))
      return 1;

    // Warn the user if they need to change something to get XtalOpt to run
    if (xtalopt.limitRunningJobs && xtalopt.runningJobLimit == 0) {
      qDebug() << "Warning: the running job limit is set to zero. You can"
               << "change this in the runtime options file in the local"
               << "working directory";
    }
    if (xtalopt.contStructs == 0) {
      qDebug() << "Warning: the continuous structure limit is set to zero. You"
               << "can change this in the runtime options file in the local"
               << "working directory";
    }

    // If the runtime file doesn't exist, write one
    if (!QFile(xtalopt.CLIRuntimeFile()).exists())
      XtalOpt::XtalOptCLIOptions::writeInitialRuntimeFile(xtalopt);

    // Emit that we are starting a session
    emit xtalopt.sessionStarted();
  }
  // If we are using the GUI, show the dialog...
  else {
    d = std::move(
      make_unique<XtalOpt::XtalOptDialog>(nullptr, Qt::Window, true, &xtalopt));
    xtalopt.setDialog(d.get());
    d->show();
  }

  return app->exec();
}

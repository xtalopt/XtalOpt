/**********************************************************************
  main - The main() function to be used by XtalOpt

  Copyright (C) 2016-2017 by Patrick S. Avery
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include <search/cliinterface.h>
#include <common/compatibility/platform_defs.h>
#include <common/output.h>
#include <common/makeunique.h>
#include <xtalopt/structuretool.h>
#include <xtalopt/xtalopt.h>

extern "C" {
#include <args.h>
}

#ifdef BUILD_XTALOPT_GUI
#include <QApplication>
#include <QMessageBox>
#include <xtalopt/gui/dialog.h>
#endif

#include <memory>

namespace XtalOpt {

namespace {

// We will process and parse the "launch options" first to avoid complications
//   with Qt construction.
// The followings control how the executable starts (help/version, CLI vs GUI,
//   target directory, logging).
enum LaunchMode
{
  LaunchHelp,
  LaunchVersion,
  LaunchKeywords,
  LaunchInputKeywords,
  LaunchConvert,
  LaunchStructureTool,
  LaunchCliStart,
  LaunchCliResume,
  LaunchGui,
  LaunchPlot
};

struct LaunchOptions
{
  LaunchMode mode = LaunchGui;
  bool saveLog = false;
  QString inputFile = "xtalopt.in";
  QString dataDir;
  QString error;
};

bool isHelpInvocation(int argc, char* argv[])
{
  for (int i = 1; i < argc; ++i) {
    const QString arg = QString::fromLocal8Bit(argv[i]);
    if (arg == "--help" || arg.startsWith("--help="))
      return true;
    if (arg.startsWith("-") && !arg.startsWith("--") && arg.contains('h'))
      return true;
  }
  return false;
}

bool parseLaunchOptions(int argc, char* argv[], LaunchOptions& options)
{
  if (isHelpInvocation(argc, argv)) {
    options.mode = LaunchHelp;
    return true;
  }

  if (StructureTool::isInvocation(argc, argv)) {
    options.mode = LaunchStructureTool;
    return true;
  }

  std::unique_ptr<ArgParser, void (*)(ArgParser*)> parser(ap_new_parser(), &ap_free);
  if (!parser) {
    options.error = "Failed to initialize command-line parser.";
    return false;
  }

  ap_set_exit_on_error(parser.get(), false);

  // The search start and input file flags were changed as of XtalOpt v15.
  // For now, we keep "legacy aliases" for backward compatibility.
  ap_add_flag(parser.get(), "start s cli");
  ap_add_flag(parser.get(), "resume r");
  ap_add_flag(parser.get(), "plot p");
  ap_add_flag(parser.get(), "keywords k");
  ap_add_flag(parser.get(), "xtalopt-flags x");
  ap_add_flag(parser.get(), "convert c");
  ap_add_flag(parser.get(), "log l");
  ap_add_flag(parser.get(), "version v");
  ap_add_str_opt(parser.get(), "input i input-file", "xtalopt.in");
  ap_add_str_opt(parser.get(), "dir d", "");

  if (!ap_parse(parser.get(), argc, argv)) {
    const char* error = ap_get_parse_error(parser.get());
    options.error = error && error[0] != '\0'
                      ? QString::fromLocal8Bit(error)
                      : "Failed to parse command-line options.";
    return false;
  }

  if (ap_has_args(parser.get())) {
    options.error = QString("Unknown option: %1")
                      .arg(QString::fromLocal8Bit(ap_get_arg_at_index(parser.get(), 0)));
    return false;
  }

  const bool cliStart = ap_found(parser.get(), "start");
  const bool cliResume = ap_found(parser.get(), "resume");
  const bool plotMode = ap_found(parser.get(), "plot");
  const bool showKeywords = ap_found(parser.get(), "keywords");
  const bool showInputKeywords = ap_found(parser.get(), "xtalopt-flags");
  const bool convertMode = ap_found(parser.get(), "convert");
  const bool showVersion = ap_found(parser.get(), "version");
  options.saveLog = ap_found(parser.get(), "log");
  options.inputFile = QString::fromLocal8Bit(ap_get_str_value(parser.get(), "input"));
  options.dataDir = QString::fromLocal8Bit(ap_get_str_value(parser.get(), "dir"));

  if (showVersion) {
    options.mode = LaunchVersion;
    return true;
  }
  if (showKeywords) {
    options.mode = LaunchKeywords;
    return true;
  }
  if (showInputKeywords) {
    options.mode = LaunchInputKeywords;
    return true;
  }
  if (convertMode) {
    if (cliStart || cliResume || plotMode) {
      options.error = "You cannot combine --convert with --start, --resume, "
                      "or --plot.";
      return false;
    }
    // Convert needs an explicit file; no fallback to the xtalopt.in default.
    if (!ap_found(parser.get(), "input")) {
      options.error = "--convert requires an input file: -i, --input <file>.";
      return false;
    }
    options.mode = LaunchConvert;
    return true;
  }

  if (cliStart && cliResume) {
    options.error = "You cannot start and resume a CLI run at the same time.";
    return false;
  }

  if (plotMode && cliStart) {
    options.error = "You cannot use CLI mode and plot mode at the same time.";
    return false;
  }

  if (plotMode && cliResume) {
    options.error = "You cannot resume in CLI mode and use plot mode at "
                    "the same time.";
    return false;
  }

  if (plotMode && options.dataDir.isEmpty()) {
    options.error = "To use plot mode, you must specify an XtalOpt results "
                    "directory with --dir";
    return false;
  }

  if (cliResume && options.dataDir.isEmpty()) {
    options.error = "To resume an XtalOpt run in CLI mode, you must specify an "
                    "XtalOpt results directory with --dir";
    return false;
  }

  // A results directory is only meaningful with a flag that uses it.
  if (!options.dataDir.isEmpty() && !cliResume && !plotMode && !options.saveLog) {
    options.error = "The --dir option requires --resume, --plot, or --log.";
    return false;
  }

#ifndef BUILD_XTALOPT_GUI
  if (plotMode) {
    options.error = "Plot mode is unavailable in this BUILD_XTALOPT_GUI=OFF build.";
    return false;
  }
#endif

  if (cliStart)
    options.mode = LaunchCliStart;
  else if (cliResume)
    options.mode = LaunchCliResume;
  else if (plotMode)
    options.mode = LaunchPlot;
  else if (ap_found(parser.get(), "input"))
    options.mode = LaunchCliStart;
#ifdef BUILD_XTALOPT_GUI
  else
    options.mode = LaunchGui;
#else
  else
    options.mode = LaunchCliStart;
#endif

  return true;
}

bool installLogHandler(const LaunchOptions& options, QString& error)
{
  error.clear();
  if (!options.saveLog)
    return true;

  if (options.dataDir.isEmpty()) {
    error = "--log requires an existing XtalOpt results directory with --dir.";
    return false;
  }

  QDir logDir(options.dataDir);
  if (!logDir.exists()) {
    error = QString("--log directory does not exist: %1")
              .arg(options.dataDir);
    return false;
  }

  const QString logFilename = logDir.absoluteFilePath("outlog_xtalopt.txt");
  QFile initialLogFile(logFilename);
  if (!initialLogFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
    error = QString("Cannot write log file: %1").arg(logFilename);
    return false;
  }
  initialLogFile.close();

  Common::addOutputHandler([logFilename](Common::OutputLevel level, const QString& text) {
    QFile file(logFilename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
      return;
    QTextStream stream(&file);
    Common::writeFormattedOutput(stream, level, text);
  });
  return true;
}

QString headerString()
{
  return QString("\n================================================\n")
       + QString("                      XtalOpt")
       + QString("\n Evolutionary Algorithm for Ground State Search\n")
       + QString("\n Version %1").arg(XTALOPT_VER)
       + QString("\n Zurek Group, University at Buffalo")
       + QString("\n================================================\n")
       + QString("\n");
}

QString buildSummary()
{
  QString buildType = "CLI binary";
  QString summary = QString("Qt-%1").arg(QT_VER);
#ifdef BUILD_XTALOPT_GUI
  summary += QString(", Qwt-%1").arg(QWT_VER);
  buildType = "CLI+GUI binary";
#endif
  summary += QString(", SSH-%1").arg(SSH_VER);
  summary += QString(" (%1)").arg(buildType);
  return summary;
}

void printHelp(const QString& argv0)
{
  const QString appName = QFileInfo(argv0).fileName();
  QString text;
  QTextStream out(&text);
  out << "Build:   " << buildSummary() << "\n\n";
  out << "Usage:   " << appName << " [options]\n\n";
  out << "Options:\n";
  out << "  -h, --help                          Print this help\n";
  out << "  -v, --version                       Print version information\n";
  out << "  -k, --keywords                      Print all available XtalOpt template keywords\n";
  out << "  -x, --xtalopt-flags                 Print all xtalopt.in input keywords with defaults\n";
  out << "  -c, --convert                       Convert an old CLI input or state <file> to latest format as <file>.compat\n";
  out << "  -s, --start                         Start an XtalOpt search in CLI\n";
  out << "  -r, --resume                        Resume an XtalOpt search from <directory> in CLI\n";
  out << "  -p, --plot                          Show a plot of a XtalOpt search saved at <directory> (requires CLI+GUI binary)\n";
  out << "  -l, --log                           Save output to <directory>/outlog_xtalopt.txt\n";
  out << "  -d, --dir <directory>               Directory for resuming search, plotting data, or saving log\n";
  out << "  -i, --input <file>                  Input file for XtalOpt search in CLI (Default: xtalopt.in)\n";
  out << "\n";
  StructureTool::printHelp(out);
  Common::message(text);
}

void printVersion()
{
  Common::message(QString("XtalOpt %1").arg(XTALOPT_VER));
}

bool startCliRun(XtalOpt& xtalopt, const QString& inputfile)
{
  // Set the run mode before any session work begins.
  xtalopt.setRunMode(XtalOpt::RunModeCliStart);

  if (!xtalopt.loadInputFile(inputfile, false))
    return false;

  // Start the search.
  return xtalopt.startSearch();
}

QString findStateFileInDataDir(const QDir& dataDir)
{
  const QString stateFile = dataDir.filePath("xtalopt.state");
  if (QFile::exists(stateFile))
    return stateFile;

  const QString backupStateFile = stateFile + ".old";
  if (QFile::exists(backupStateFile))
    return backupStateFile;

  return QString();
}

bool resumeCliRun(XtalOpt& xtalopt, const QString& dataDir)
{
  xtalopt.setRunMode(XtalOpt::RunModeCliResume);
  const QDir resultsDir(dataDir);
  const QString stateFile = findStateFileInDataDir(resultsDir);
  if (stateFile.isEmpty()) {
    Common::error(QString("No xtalopt.state file found in %1.").arg(dataDir));
    Common::message("Please check your --dir option and try again.");
    return false;
  }

  // resumeSearch() also writes the CLI runtime file on success.
  return xtalopt.resumeSearch(stateFile);
}

#ifdef BUILD_XTALOPT_GUI
void initializeGuiDialog(XtalOpt& xtalopt, std::unique_ptr<XtalOptDialog>& dialog)
{
  dialog = make_unique<XtalOptDialog>(nullptr, Qt::Window, &xtalopt);
#if GS_WINDOWS
  dialog->setStyleSheet("QWidget{font-size: 10pt;}");
#else
  dialog->setStyleSheet("QWidget{font-size: 12pt;}");
#endif
}

bool showGui(XtalOpt& xtalopt, std::unique_ptr<XtalOptDialog>& dialog)
{
  // GUI mode: start or resume is decided later inside the dialog.
  xtalopt.setRunMode(XtalOpt::RunModeGui);
  initializeGuiDialog(xtalopt, dialog);
  dialog->show();
  return true;
}

bool startPlotRun(XtalOpt& xtalopt, std::unique_ptr<XtalOptDialog>& dialog, const QString& dataDir)
{
  xtalopt.setRunMode(XtalOpt::RunModeReadOnly);
  initializeGuiDialog(xtalopt, dialog);

  const QString stateFile = findStateFileInDataDir(QDir(dataDir));
  if (stateFile.isEmpty()) {
    QMessageBox::critical(dialog.get(), "XtalOpt Plot",
                          QString("No xtalopt.state file was found in:\n%1").arg(dataDir));
    return false;
  }

  QString startupError;
  const int outHandlerId = Common::addOutputHandler(
    [&startupError](Common::OutputLevel level,
                    const QString& text) {
      if (level == Common::OutputLevel::Error)
        startupError = text;
    });
  Common::message("Loading xtals for plotting...");
  if (!xtalopt.resumeSearch(stateFile)) {
    Common::removeOutputHandler(outHandlerId);
    QMessageBox::critical(dialog.get(), "XtalOpt Plot", startupError.isEmpty()
                            ? QString("Failed to load XtalOpt plot data from:\n%1").arg(dataDir)
                            : startupError);
    return false;
  }
  Common::removeOutputHandler(outHandlerId);

  dialog->beginPlotOnlyMode();
  return true;
}
#endif

int runApplication(int argc, char* argv[], const LaunchOptions& options)
{
  // Set the Qt application and XtalOpt object; then start the requested mode:
  //   CLI start/resume, GUI run, or GUI read-only plot.
  std::unique_ptr<QCoreApplication> app;
#ifdef BUILD_XTALOPT_GUI
  const bool needsGuiApp = options.mode == LaunchGui || options.mode == LaunchPlot;
  if (needsGuiApp) {
    app.reset(make_unique<QApplication>(argc, argv).release());
  } else
#endif
  {
    app = make_unique<QCoreApplication>(argc, argv);
  }

  QCoreApplication::setOrganizationName("XtalOpt");
  QCoreApplication::setOrganizationDomain("xtalopt.github.io");
  QCoreApplication::setApplicationName("XtalOpt");
  QCoreApplication::setApplicationVersion(XTALOPT_VER);

  if (options.mode == LaunchKeywords) {
    XtalOpt xtalopt;
    Common::message(xtalopt.getTemplateKeywordHelp());
    return 0;
  }

  if (options.mode == LaunchInputKeywords) {
    Common::message(Settings::keywordSummaryText());
    return 0;
  }

  if (options.mode == LaunchConvert) {
    XtalOpt xtalopt;
    return xtalopt.convertLegacyFileToCurrent(options.inputFile) ? 0 : 1;
  }

  XtalOpt xtalopt;
  // Install terminal prompts first; GUI construction replaces them later if needed.
  Search::installTerminalInterface(xtalopt);
#ifdef BUILD_XTALOPT_GUI
  std::unique_ptr<XtalOptDialog> dialog;
#endif

  switch (options.mode) {
  case LaunchCliStart:
    if (!startCliRun(xtalopt, options.inputFile))
      return 1;
    break;
  case LaunchCliResume:
    if (!resumeCliRun(xtalopt, options.dataDir))
      return 1;
    break;
  case LaunchPlot:
#ifdef BUILD_XTALOPT_GUI
    if (!startPlotRun(xtalopt, dialog, options.dataDir))
      return 1;
#else
    Common::error("GUI support is unavailable in this build.");
    return 1;
#endif
    break;
  case LaunchGui:
#ifdef BUILD_XTALOPT_GUI
    if (!showGui(xtalopt, dialog))
      return 1;
#else
    Common::error("GUI support is unavailable in this build.");
    return 1;
#endif
    break;
  case LaunchHelp:
  case LaunchVersion:
  case LaunchKeywords:
  case LaunchInputKeywords:
  case LaunchConvert:
  case LaunchStructureTool:
    return 0;
  }

  return app->exec();
}

} // namespace

} // namespace XtalOpt

int main(int argc, char* argv[])
{
  using namespace XtalOpt;

  LaunchOptions options;
  QString startupError;
  bool showHelpHint = false;
  QString logError;

  if (!parseLaunchOptions(argc, argv, options)) {
    startupError = options.error;
    showHelpHint = true;
  }

  if (startupError.isEmpty() && options.mode == LaunchStructureTool)
    return StructureTool::run(argc, argv);

  if (startupError.isEmpty() && options.mode != LaunchHelp && !installLogHandler(options, logError)) {
    startupError = logError;
  }

  Common::message(headerString());

  if (options.mode == LaunchHelp) {
    printHelp(QString::fromLocal8Bit(argv[0]));
    return 0;
  }

  if (!startupError.isEmpty()) {
    Common::error(startupError);
    if (showHelpHint)
      Common::message("Use --help to see available options.");
    return 1;
  }

  if (options.mode == LaunchVersion) {
    printVersion();
    return 0;
  }

  return runApplication(argc, argv, options);
}

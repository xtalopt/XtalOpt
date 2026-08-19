/**********************************************************************
  LegacyCompatTest - Unit testing for XtalOpt legacy-format compatibility

  Copyright (C) 2010 David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/xtalopt.h>

#include <xtalopt/structures/xtal.h>
#include <xtalopt/legacy/structure_state_compat.h>
#include <xtalopt/settings.h>

#include <common/compatibility/qt_compat.h>
#include <common/compatibility/platform_compat.h>
#include <common/fileutils.h>
#include <search/queueinterface.h>
#include <search/queueinterfaces/batch.h>
#include <search/optimizer.h>
#include <search/queuemanager.h>
#include <common/random.h>
#include <search/structure.h>
#include <search/tracker.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QSettings>
#include <QString>
#include <QTemporaryDir>
#include <QtTest>

using namespace Search;

namespace XtalOpt {
namespace {

const int OriginalStateVersion = 4;

QString tempPath(const QTemporaryDir& dir, const QString& child)
{
  return Common::localPath(dir.path(), child);
}

// Compare a file with its expected copy. Line endings are normalized so
//   the comparison works the same on all platforms.
// Note: the expected copies can be re-written with XTALOPT_UPDATE_EXPECTED=1
void compareWithExpectedFile(const QString& producedPath, const QString& expectedPath)
{
  QFile producedFile(producedPath);
  QVERIFY(producedFile.open(QIODevice::ReadOnly));
  QByteArray produced = producedFile.readAll();
  QVERIFY(!produced.isEmpty());
  produced.replace("\r\n", "\n");

  if (qgetenv("XTALOPT_UPDATE_EXPECTED") == "1") {
    QVERIFY(QDir().mkpath(QFileInfo(expectedPath).absolutePath()));
    QFile expectedFile(expectedPath);
    QVERIFY(expectedFile.open(QIODevice::WriteOnly));
    QCOMPARE(expectedFile.write(produced), qint64(produced.size()));
    return;
  }

  QFile expectedFile(expectedPath);
  QVERIFY2(expectedFile.open(QIODevice::ReadOnly),
           "Expected file is missing. Run this test once with "
           "XTALOPT_UPDATE_EXPECTED=1 to create it.");
  QByteArray expected = expectedFile.readAll();
  expected.replace("\r\n", "\n");
  QCOMPARE(produced, expected);
}

void markXtalOptStateVersion(QSettings& settings, int version)
{
  settings.setValue("xtalopt/version", version);
  settings.setValue("xtalopt/init/version", version);
  settings.setValue("xtalopt/edit/version", version);
}

// Raw structure.state status values used by v4 files.
enum Version14State
{
  Version14Optimized = 0,
  Version14StepOptimized = 1,
  Version14WaitingForOptimization = 2,
  Version14InProcess = 3,
  Version14Empty = 4,
  Version14Updating = 5,
  Version14Error = 6,
  Version14Submitted = 7,
  Version14Killed = 8,
  Version14Removed = 9,
  Version14Similar = 10,
  Version14Restart = 11,
  Version14ObjectiveDismiss = 12,
  Version14ObjectiveFail = 13,
  Version14ObjectiveRetain = 14,
  Version14ObjectiveCalculation = 15
};

void writeRawWorkflowState(const QString& filename, int version, int status)
{
  Xtal xtal;
  writeStructureStateFile(xtal, filename);

  QSettings settings(filename, QSettings::IniFormat);
  settings.beginGroup("structure");
  settings.setValue("version", version);
  settings.setValue("status", status);
  settings.endGroup();
  settings.sync();
}

} // namespace

class LegacyCompatTest : public QObject
{
  Q_OBJECT

private slots:
  // Called before the first test function is executed.
  void initTestCase()
  {
    Common::seedMt19937Generator(0);
  }
  // Called after the last test function is executed.
  void cleanupTestCase()
  {
  }
  // Called before each test function is executed.
  void init(){};
  // Called after every test function.
  void cleanup(){};

  // Test old state files and output files.
  void v4SessionRestoreIsStable();
  void v4ConvertedSettingsAreStable();
  void schemeIsVersionedAndRoutesThroughLegacyLayer();
  void convertProducesCurrentFormatPerType();
  void convertMechanicalReadIgnoresMissingAssets();

  void objectiveSettingsLoadLegacyFiltrationAsConstraint();
  void legacyFiltrationStructureObjectivesNormalizeOnPlotLoad();
  void legacyV4StructureUserObjectivesExpandOnPlotLoad();
  void loadInputFileNormalizesLegacyLocalQueue();
  void loadInputFileNormalizesLegacyExeLocation();
  void loadInputFileNormalizesLegacyRadiiIADOption();
  void loadInputFileLoadsLegacyMolecularUnits();
  void loadInputFileConvertsLegacyMolecularUnitsWithoutAggregateCheck();
  void loadInputFileIgnoresLegacyMolecularUnitsWhenDisabled();
  void loadInputFileNormalizesLegacyObjectivesReDoOption();
  void loadInputFileConvertsLegacyFiltrationObjectiveOnce();
  void loadInputFileConvertsLegacyNumberedAndAssetForms();
  void loadInputFileParsesForcedRandSpgWithoutUsingRandSpg();
  void stateSettingsLoadLegacyRadiiIADOption();
  void stateSettingsLoadLegacyBatchQueueCommands();
  void stateSettingsLoadLegacyMolecularUnits();
  void stateSaveRewritesLegacyResumeToFreshV5State();
  void stateSettingsNormalizeSingleLegacyVaspPotcarEntry();
  void stateSettingsNormalizeLegacyVaspPotcarEntries();
  void stateSettingsRejectMismatchedLegacyVaspPotcarEntries();
  void legacyWorkflowStatusesNormalizeOnRead();
  void unversionedTextStateIsRejected();
  void unsupportedStructureVersionsAreRejected();
};

void LegacyCompatTest::v4SessionRestoreIsStable()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());
  const QString srcData = Common::localPath(QString(TESTDATADIR), "legacy/xo-duplicateXtals-v4");
  const QString dstData = tempPath(tempDir, "xo-duplicateXtals");

  QVERIFY(QDir().mkpath(dstData));
  QVERIFY(QFile::copy(Common::localPath(srcData, "xtalopt.state"),
                      Common::localPath(dstData, "xtalopt.state")));

  const QString structureDir = Common::localPath(dstData, "00001x00001");
  QVERIFY(QDir().mkpath(structureDir));

  Xtal xtal(5.0, 5.0, 5.0, 90.0, 90.0, 90.0);
  Atoms::Atom& atom = xtal.addAtom();
  atom.setAtomicNumber(22);
  atom.setPos(Common::Vector3(0.0, 0.0, 0.0));
  xtal.setGeneration(1);
  xtal.setIDNumber(1);
  xtal.setIndex(0);
  xtal.setParents("Randomly generated");
  xtal.setStatus(Xtal::Optimized);
  xtal.setLocpath(structureDir);
  xtal.setCurrentOptStep(0);

  const QString structureState = Common::localPath(structureDir, "structure.state");
  writeStructureStateFile(xtal, structureState);
  {
    QSettings settings(structureState, QSettings::IniFormat);
    settings.setValue("structure/version", OriginalStateVersion);
  }

  XtalOpt opt;
  opt.setRunMode(XtalOpt::RunModeReadOnly);
  opt.tracker()->blockSignals(true);

  QVERIFY(opt.resumeSearch(Common::localPath(dstData, "xtalopt.state")));
  opt.tracker()->blockSignals(false);
  QVERIFY(!QFile::exists(Common::localPath(dstData, "xtalopt.state.compat")));

  // Session-level values from the v4 state file.
  QCOMPARE(opt.tracker()->size(), 1);
  QVERIFY(opt.optimizer(0));
  QCOMPARE(opt.optimizer(0)->getDirectRunCommand(), QString("gulp"));
  QCOMPARE(opt.getChemicalSystem(), QList<QString>() << "O" << "Ti");
  QCOMPARE(opt.getAMin(), 1.0);
  QCOMPARE(opt.getAMax(), 20.0);

  // One v14 structure loaded with the session.
  Structure* first = nullptr;
  for (int i = 0; i < opt.tracker()->size(); ++i) {
    Structure* s = opt.tracker()->at(i);
    if (s && s->getGeneration() == 1 && s->getIDNumber() == 1) {
      first = s;
      break;
    }
  }
  QVERIFY(first);
  QReadLocker locker(&first->lock());
  QCOMPARE(first->getParents(), QString("Randomly generated"));
  QCOMPARE(first->getStatus(), Structure::Optimized);
  QCOMPARE(static_cast<int>(first->getCurrentOptStep()), 0);
}

void LegacyCompatTest::v4ConvertedSettingsAreStable()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());
  const QString srcData = Common::localPath(QString(TESTDATADIR), "legacy/xo-duplicateXtals-v4");
  const QString dstData = tempPath(tempDir, "xo-duplicateXtals");

  QVERIFY(QDir().mkpath(dstData));
  QVERIFY(QFile::copy(Common::localPath(srcData, "xtalopt.state"),
                      Common::localPath(dstData, "xtalopt.state")));

  XtalOpt opt;
  opt.setRunMode(XtalOpt::RunModeGui);
  QVERIFY(opt.readStateFile(Common::localPath(dstData, "xtalopt.state"), true));

  // Check the converted settings.
  const QString compatPath = Common::localPath(dstData, "xtalopt.state.compat");
  QSettings compat(compatPath, QSettings::IniFormat);
  QStringList lines;
  for (const QString& key : compat.allKeys()) {
    if (key.startsWith("xtalopt/input/")) {
      QString value = compat.value(key).toString();
      value.replace(dstData, "<STATEDIR>");
      value.replace(tempDir.path(), "<TMP>");
      lines << (key + "=" + value);
    }
  }
  lines.sort();
  const QString produced = lines.join("\n") + "\n";

  const QString producedPath = tempPath(tempDir, "v4_converted_settings.txt");
  QFile producedFile(producedPath);
  QVERIFY(producedFile.open(QIODevice::WriteOnly | QIODevice::Text));
  producedFile.write(produced.toLocal8Bit());
  producedFile.close();

  const QString expectedDir = Common::localPath(QString(TESTDATADIR), "outputs");
  compareWithExpectedFile(producedPath,
                        Common::localPath(expectedDir, "v4_converted_settings.txt"));
}

void LegacyCompatTest::schemeIsVersionedAndRoutesThroughLegacyLayer()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());
  const QString srcData = Common::localPath(QString(TESTDATADIR), "legacy/xo-duplicateXtals-v4");
  const QString dstData = tempPath(tempDir, "xo-duplicateXtals");
  QVERIFY(QDir().mkpath(dstData));
  QVERIFY(QFile::copy(Common::localPath(srcData, "xtalopt.state"),
                      Common::localPath(dstData, "xtalopt.state")));

  // Read an old scheme file.
  XtalOpt opt;
  opt.setRunMode(XtalOpt::RunModeReadOnly);
  QVERIFY(opt.loadSchemeFile(Common::localPath(dstData, "xtalopt.state"), false));
  QVERIFY(!QFile::exists(Common::localPath(dstData, "xtalopt.state.compat")));
  QCOMPARE(static_cast<int>(opt.getNumOptSteps()), 1);
  QVERIFY(opt.optimizer(0));
  QCOMPARE(opt.optimizer(0)->getDirectRunCommand(), QString("gulp"));
  QCOMPARE(opt.queueInterface(0)->getIDString().toLower(), QString("none"));

  // Check the current scheme version.
  const QString schemePath = tempPath(tempDir, "test.scheme");
  QVERIFY(opt.saveSchemeFile(schemePath));
  QSettings schemeFile(schemePath, QSettings::IniFormat);
  QCOMPARE(schemeFile.value("xtalopt/version").toInt(),
           static_cast<int>(CurrentStateSchemaVersion));

  // Read the current scheme file.
  XtalOpt opt2;
  opt2.setRunMode(XtalOpt::RunModeReadOnly);
  QVERIFY(opt2.loadSchemeFile(schemePath, false));
  QCOMPARE(opt2.getNumOptSteps(), opt.getNumOptSteps());
  QVERIFY(opt2.optimizer(0));
  QCOMPARE(opt2.optimizer(0)->getDirectRunCommand(), QString("gulp"));
  QCOMPARE(opt2.queueInterface(0)->getIDString().toLower(), QString("none"));
}

void LegacyCompatTest::convertProducesCurrentFormatPerType()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());
  const QString srcData = Common::localPath(QString(TESTDATADIR), "legacy/xo-duplicateXtals-v4");
  const QString dstData = tempPath(tempDir, "xo-duplicateXtals");
  QVERIFY(QDir().mkpath(dstData));
  QVERIFY(QFile::copy(Common::localPath(srcData, "xtalopt.state"),
                      Common::localPath(dstData, "xtalopt.state")));
  const QString structureDir = Common::localPath(dstData, "00001x00001");
  QVERIFY(QDir().mkpath(structureDir));

  const QString structPath = Common::localPath(structureDir, "structure.state");
  Xtal structure;
  writeStructureStateFile(structure, structPath);

  XtalOpt opt;
  opt.setRunMode(XtalOpt::RunModeReadOnly);

  // Convert an old state file.
  const QString statePath = Common::localPath(dstData, "xtalopt.state");
  QVERIFY(opt.convertLegacyFileToCurrent(statePath));
  const QString stateCompat = statePath + ".compat";
  QVERIFY(QFile::exists(stateCompat));
  {
    QSettings s(stateCompat, QSettings::IniFormat);
    QCOMPARE(s.value("xtalopt/version").toInt(), static_cast<int>(CurrentStateSchemaVersion));
    // Check that old GUI settings were removed.
    s.beginGroup("xtalopt");
    const QStringList groups = s.childGroups();
    s.endGroup();
    QVERIFY(!groups.contains("plot"));
    QVERIFY(!groups.contains("progress"));
    for (const QString& group : groups)
      QVERIFY2(group == "input" || group == "optscheme",
               qPrintable("unexpected group in compat file: " + group));
  }

  XtalOpt loaded;
  loaded.setRunMode(XtalOpt::RunModeReadOnly);
  QVERIFY(loaded.loadSchemeFile(stateCompat, false));
  QVERIFY(loaded.optimizer(0));
  QCOMPARE(loaded.optimizer(0)->getDirectRunCommand(), QString("gulp"));

  // Read a current state file.
  QVERIFY(opt.convertLegacyFileToCurrent(stateCompat));
  QVERIFY(!QFile::exists(stateCompat + ".compat"));

  // Individual structure states are converted only while loading a session.
  QVERIFY(!opt.convertLegacyFileToCurrent(structPath));
  const QString structCompat = structPath + ".compat";
  QVERIFY(!QFile::exists(structCompat));

  // Read a current text input file.
  const QString currentIn = tempPath(tempDir, "xtalopt.in");
  QVERIFY(QFile::copy(Common::localPath(QString(TESTDATADIR), "inputs/xtalopt.in"), currentIn));
  QVERIFY(opt.convertLegacyFileToCurrent(currentIn));
  QVERIFY(!QFile::exists(currentIn + ".compat"));
}

void LegacyCompatTest::convertMechanicalReadIgnoresMissingAssets()
{
  // Convert an old text input without its template files.
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());
  const QString dstIn = tempPath(tempDir, "xtalopt.in");
  QFile inputFile(dstIn);
  QVERIFY(inputFile.open(QIODevice::WriteOnly | QIODevice::Text));
  QTextStream input(&inputFile);

  input << "chemicalFormulas = O1\n";
  input << "queueInterface = local\n";
  input << "localQueue = true\n";
  input << "exeLocation = gulp\n";
  input << "templatesDirectory = ./missing-templates\n";
  input << "ginTemplates = missing.gin\n";

  inputFile.close();

  XtalOpt opt;
  opt.setRunMode(XtalOpt::RunModeReadOnly);
  QVERIFY(opt.convertLegacyFileToCurrent(dstIn));

  const QString compat = dstIn + ".compat";
  QVERIFY(QFile::exists(compat));
  QFile compatFile(compat);
  QVERIFY(compatFile.open(QIODevice::ReadOnly | QIODevice::Text));

  const QString text = QString::fromLocal8Bit(compatFile.readAll());
  QVERIFY(text.contains("queueInterface = none"));
  QVERIFY(text.contains("remoteQueue = false"));
  QVERIFY(text.contains("directRunCommand = gulp"));
  QVERIFY(text.contains("templatesDirectory = ./missing-templates"));
  QVERIFY(text.contains("ginTemplates = missing.gin"));
}

void LegacyCompatTest::objectiveSettingsLoadLegacyFiltrationAsConstraint()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  const QString stateFile = tempPath(tempDir, "legacy-filtration-constraint.state");
  QSettings settings(stateFile, QSettings::IniFormat);
  settings.beginGroup("xtalopt/init");
  settings.setValue("version", OriginalStateVersion);
  settings.endGroup();
  settings.beginGroup("xtalopt/edit");
  settings.setValue("version", OriginalStateVersion);
  settings.endGroup();
  settings.beginGroup("xtalopt/obj");
  settings.setValue("objectivesReDo", false);
  settings.beginWriteArray("objectives");
  settings.setArrayIndex(0);
  settings.setValue("typ", 2);
  settings.setValue("exe", "/tmp/objective-filter");
  settings.setValue("out", "filter.out");
  settings.setValue("wgt", 0.1);
  settings.endArray();
  settings.endGroup();

  settings.sync();

  XtalOpt loaded;
  QVERIFY(loaded.readStateFile(stateFile, true));
  QCOMPARE(loaded.getObjectivesNum(), 1);
  QCOMPARE(loaded.getConstraintsNum(), 1);
  QCOMPARE(loaded.getConstraintExe(0), QString("/tmp/objective-filter"));
  QCOMPARE(loaded.getConstraintOut(0), QString("filter.out"));
}

void LegacyCompatTest::legacyFiltrationStructureObjectivesNormalizeOnPlotLoad()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  const QString dataDir = tempPath(tempDir, "legacy-filtration-run");
  QVERIFY(QDir().mkpath(dataDir));

  const QString stateFile = Common::localPath(dataDir, "xtalopt.state");
  {
    QSettings settings(stateFile, QSettings::IniFormat);
    settings.beginGroup("xtalopt");
    settings.setValue("version", OriginalStateVersion);
    settings.setValue("saveSuccessful", true);
    settings.endGroup();

    settings.beginGroup("xtalopt/init");
    settings.setValue("version", OriginalStateVersion);
    settings.setValue("chemical_formulas", "H1");
    settings.setValue("referenceEnergies", "H1 0.0");
    settings.endGroup();

    settings.beginGroup("xtalopt/edit");
    settings.setValue("version", OriginalStateVersion);
    settings.setValue("numOptSteps", 1);
    settings.setValue("optimizer/0", "gulp");
    settings.setValue("queueInterface/0", "none");
    settings.endGroup();

    settings.beginGroup("xtalopt/obj");
    settings.setValue("objectivesReDo", false);
    settings.beginWriteArray("objectives");
    settings.setArrayIndex(0);
    settings.setValue("typ", SearchBase::Ot_Min);
    settings.setValue("exe", "min-objective");
    settings.setValue("out", "min.out");
    settings.setValue("wgt", 0.2);
    settings.setArrayIndex(1);
    settings.setValue("typ", 2);
    settings.setValue("exe", "legacy-filter");
    settings.setValue("out", "filter.out");
    settings.setValue("wgt", 0.0);
    settings.setArrayIndex(2);
    settings.setValue("typ", SearchBase::Ot_Max);
    settings.setValue("exe", "max-objective");
    settings.setValue("out", "max.out");
    settings.setValue("wgt", 0.3);
    settings.endArray();
    settings.endGroup();
    settings.sync();
  }

  const QString structureDir = Common::localPath(dataDir, "00001x00001");
  QVERIFY(QDir().mkpath(structureDir));
  Xtal xtal(5.0, 5.0, 5.0, 90.0, 90.0, 90.0);
  Atoms::Atom& atom = xtal.addAtom();
  atom.setAtomicNumber(1);
  atom.setPos(Common::Vector3(0.0, 0.0, 0.0));
  xtal.setGeneration(1);
  xtal.setIDNumber(1);
  xtal.setIndex(0);
  xtal.setStatus(Xtal::Optimized);
  xtal.setLocpath(structureDir);
  xtal.setEnthalpy(-1.0);
  xtal.setStrucObjValuesVec(QList<double>() << 0.0 << 4.0 << 1.0 << 8.0);
  xtal.setStrucObjState(Structure::Os_Dismiss);

  const QString structureStateFile = Common::localPath(structureDir, "structure.state");
  writeStructureStateFile(xtal, structureStateFile);
  {
    QSettings settings(structureStateFile, QSettings::IniFormat);
    settings.beginGroup("structure");
    settings.setValue("version", OriginalStateVersion);
    settings.setValue("status", Version14ObjectiveDismiss);
    settings.remove("constraintRedoCount");
    settings.setValue("objectivesFailCount", 1);
    settings.remove("userObjectives");
    settings.beginWriteArray("objectives");
    const QList<double> objectives = { 0.0, 4.0, 1.0, 8.0 };

    for (int i = 0; i < objectives.size(); ++i) {
      settings.setArrayIndex(i);
      settings.setValue("value", objectives.at(i));
    }

    settings.endArray();

    settings.beginGroup("history");
    settings.beginWriteArray("objectives");
    settings.setArrayIndex(0);
    settings.beginWriteArray("value");
    const QList<double> historyObjectives = { 0.0, 3.0, 2.0, 7.0 };
    for (int i = 0; i < historyObjectives.size(); ++i) {
      settings.setArrayIndex(i);
      settings.setValue("value", historyObjectives.at(i));
    }
    settings.endArray();
    settings.setValue("state", Structure::Os_Retain);
    settings.setValue("failcount", 0);
    settings.endArray();
    settings.endGroup();
    settings.endGroup();
    settings.sync();
  }

  XtalOpt loaded;
  loaded.setRunMode(XtalOpt::RunModeReadOnly);
  QVERIFY(loaded.resumeSearch(Common::localPath(dataDir, "xtalopt.state")));
  QCOMPARE(loaded.getObjectivesNum(), 3);
  QCOMPARE(loaded.getConstraintsNum(), 1);
  QCOMPARE(loaded.tracker()->size(), 1);

  Structure* loadedStructure = loaded.tracker()->at(0);
  QCOMPARE(loadedStructure->getStatus(), Structure::Dismissed);
  QCOMPARE(loadedStructure->getStrucObjNumber(), 3);
  QCOMPARE(loadedStructure->getStrucObjValues(1), 4.0);
  QCOMPARE(loadedStructure->getStrucObjValues(2), 8.0);
  QCOMPARE(loadedStructure->getStrucObjState(), Structure::Os_Retain);
  QCOMPARE(loadedStructure->getStrucConstraintNumber(), 1);
  QCOMPARE(loadedStructure->getStrucConstraintValues(0), 1.0);
  QCOMPARE(loadedStructure->getStrucConstraintState(), Structure::Cs_Dismiss);
  QCOMPARE(loadedStructure->getStrucConstraintRedoCount(), 1);
}

void LegacyCompatTest::legacyV4StructureUserObjectivesExpandOnPlotLoad()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  const QString dataDir = tempPath(tempDir, "legacy-v4-objective-run");
  QVERIFY(QDir().mkpath(dataDir));

  const QString stateFile = Common::localPath(dataDir, "xtalopt.state");
  {
    QSettings settings(stateFile, QSettings::IniFormat);
    settings.beginGroup("xtalopt");
    settings.setValue("version", OriginalStateVersion);
    settings.setValue("saveSuccessful", true);
    settings.endGroup();

    settings.beginGroup("xtalopt/init");
    settings.setValue("version", OriginalStateVersion);
    settings.setValue("chemical_formulas", "H1");
    settings.setValue("referenceEnergies", "H1 0.0");
    settings.endGroup();

    settings.beginGroup("xtalopt/edit");
    settings.setValue("version", OriginalStateVersion);
    settings.setValue("numOptSteps", 1);
    settings.setValue("optimizer/0", "gulp");
    settings.setValue("queueInterface/0", "none");
    settings.endGroup();

    settings.beginGroup("xtalopt/obj");
    settings.setValue("objectivesReDo", false);
    settings.beginWriteArray("objectives");
    settings.setArrayIndex(0);
    settings.setValue("typ", SearchBase::Ot_Min);
    settings.setValue("exe", "min-objective");
    settings.setValue("out", "min.out");
    settings.setValue("wgt", 0.2);
    settings.endArray();
    settings.endGroup();
    settings.sync();
  }

  const QString structureDir = Common::localPath(dataDir, "00001x00001");
  QVERIFY(QDir().mkpath(structureDir));
  const QString structureState = Common::localPath(structureDir, "structure.state");

  Xtal xtal(5.0, 5.0, 5.0, 90.0, 90.0, 90.0);
  Atoms::Atom& atom = xtal.addAtom();
  atom.setAtomicNumber(1);
  atom.setPos(Common::Vector3(0.0, 0.0, 0.0));
  xtal.setGeneration(1);
  xtal.setIDNumber(1);
  xtal.setIndex(0);
  xtal.setStatus(Xtal::Optimized);
  xtal.setLocpath(structureDir);
  xtal.setEnthalpy(-1.0);
  xtal.setStrucObjValuesVec(QList<double>() << 4.0);
  xtal.setStrucObjState(Structure::Os_Retain);
  writeStructureStateFile(xtal, structureState);

  {
    QSettings settings(structureState, QSettings::IniFormat);
    settings.beginGroup("structure");
    settings.setValue("version", OriginalStateVersion);
    settings.remove("userObjectives");
    settings.beginWriteArray("objectives");
    settings.setArrayIndex(0);
    settings.setValue("value", 4.0);
    settings.endArray();
    settings.endGroup();
    settings.sync();
  }

  XtalOpt loaded;
  loaded.setRunMode(XtalOpt::RunModeReadOnly);
  QVERIFY(loaded.resumeSearch(Common::localPath(dataDir, "xtalopt.state")));
  QCOMPARE(loaded.getObjectivesNum(), 2);
  QCOMPARE(loaded.getUserObjectivesNum(), 1);
  QCOMPARE(loaded.tracker()->size(), 1);

  Structure* loadedStructure = loaded.tracker()->at(0);
  QCOMPARE(loadedStructure->getStrucObjNumber(), 2);
  QCOMPARE(loadedStructure->getStrucObjValues(1), 4.0);
}

void LegacyCompatTest::loadInputFileNormalizesLegacyLocalQueue()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  QFile templateFile(tempPath(tempDir, "xtal.gin"));
  QVERIFY(templateFile.open(QIODevice::WriteOnly | QIODevice::Text));
  templateFile.write("# gulp template\n");
  templateFile.close();

  QFile inputFile(tempPath(tempDir, "xtalopt.in"));
  QVERIFY(inputFile.open(QIODevice::WriteOnly | QIODevice::Text));
  QTextStream input(&inputFile);
  input << "chemicalFormulas = O1\n";
  input << "queueInterface = local\n";
  input << "localQueue = true\n";
  input << "optimizer = gulp\n";
  input << "templatesDirectory = " << tempDir.path() << "\n";
  input << "ginTemplates = xtal.gin\n";
  input << "localWorkingDirectory = " << tempPath(tempDir, "run") << "\n";
  inputFile.close();

  XtalOpt opt;
  QVERIFY(opt.loadInputFile(inputFile.fileName(), true));
  QCOMPARE(opt.queueInterface(0)->getIDString(), QString("none"));
  QVERIFY(!opt.isRemoteQueue());
}

void LegacyCompatTest::loadInputFileNormalizesLegacyExeLocation()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  QFile templateFile(tempPath(tempDir, "xtal.gin"));
  QVERIFY(templateFile.open(QIODevice::WriteOnly | QIODevice::Text));
  templateFile.write("# gulp template\n");
  templateFile.close();

  QFile inputFile(tempPath(tempDir, "xtalopt.in"));
  QVERIFY(inputFile.open(QIODevice::WriteOnly | QIODevice::Text));
  QTextStream input(&inputFile);
  input << "chemicalFormulas = O1\n";
  input << "queueInterface = none\n";
  input << "optimizer = gulp\n";
  input << "templatesDirectory = " << tempDir.path() << "\n";
  input << "ginTemplates = xtal.gin\n";
  input << "exeLocation = mpirun -np 4 gulp\n";
  input << "localWorkingDirectory = " << tempPath(tempDir, "run") << "\n";
  inputFile.close();

  XtalOpt opt;
  QVERIFY(opt.loadInputFile(inputFile.fileName(), true));
  QVERIFY(opt.optimizer(0));
  QCOMPARE(opt.optimizer(0)->getDirectRunCommand(), QString("mpirun -np 4 gulp"));
}

void LegacyCompatTest::loadInputFileNormalizesLegacyRadiiIADOption()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  QFile templateFile(tempPath(tempDir, "xtal.gin"));
  QVERIFY(templateFile.open(QIODevice::WriteOnly | QIODevice::Text));
  templateFile.write("# gulp template\n");
  templateFile.close();

  QFile inputFile(tempPath(tempDir, "xtalopt.in"));
  QVERIFY(inputFile.open(QIODevice::WriteOnly | QIODevice::Text));
  QTextStream input(&inputFile);
  input << "chemicalFormulas = O1\n";
  input << "queueInterface = none\n";
  input << "optimizer = gulp\n";
  input << "templatesDirectory = " << tempDir.path() << "\n";
  input << "ginTemplates = xtal.gin\n";
  input << "usingRadiiInteratomicDistanceLimit = false\n";
  input << "usingCustomIADs = true\n";
  input << "customIAD 1 = O, O, 1.2\n";
  input << "localWorkingDirectory = " << tempPath(tempDir, "run") << "\n";
  inputFile.close();

  XtalOpt opt;
  QVERIFY(opt.loadInputFile(inputFile.fileName(), true));
  QVERIFY(!opt.getUsingScaledIAD());
  QVERIFY(opt.getUsingCustomIAD());
}

void LegacyCompatTest::loadInputFileParsesForcedRandSpgWithoutUsingRandSpg()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  QFile templateFile(tempPath(tempDir, "xtal.gin"));
  QVERIFY(templateFile.open(QIODevice::WriteOnly | QIODevice::Text));
  templateFile.write("# gulp template\n");
  templateFile.close();

  const QString runDir = tempPath(tempDir, "run");
  QFile inputFile(tempPath(tempDir, "xtalopt.in"));
  QVERIFY(inputFile.open(QIODevice::WriteOnly | QIODevice::Text));
  QTextStream input(&inputFile);
  input << "chemicalFormulas = H1\n";
  input << "usingRandSpg = false\n";
  input << "forcedSpgsWithRandSpg = 1,1\n";
  input << "queueInterface = none\n";
  input << "optimizer = gulp\n";
  input << "templatesDirectory = " << tempDir.path() << "\n";
  input << "ginTemplates = xtal.gin\n";
  input << "localWorkingDirectory = " << runDir << "\n";
  inputFile.close();

  XtalOpt opt;
  QVERIFY(opt.loadInputFile(inputFile.fileName(), true));
  QVERIFY(!opt.getUsingRandSpg());
  QVERIFY(opt.minXtalsOfSpg().size() >= 1);
  QCOMPARE(opt.minXtalsOfSpg().at(0), 2);
}

void LegacyCompatTest::loadInputFileConvertsLegacyNumberedAndAssetForms()
{
  // Read old input syntax (multi-entries, etc).
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  QFile incarFile(tempPath(tempDir, "INCAR"));
  QVERIFY(incarFile.open(QIODevice::WriteOnly | QIODevice::Text));
  incarFile.write("# incar\n");
  incarFile.close();

  QFile kpointsFile(tempPath(tempDir, "KPOINTS"));
  QVERIFY(kpointsFile.open(QIODevice::WriteOnly | QIODevice::Text));
  kpointsFile.write("# kpoints\n");
  kpointsFile.close();

  QFile potcarTi(tempPath(tempDir, "POTCAR_Ti"));
  QVERIFY(potcarTi.open(QIODevice::WriteOnly | QIODevice::Text));
  potcarTi.write("potcar ti\n");
  potcarTi.close();

  QFile potcarO(tempPath(tempDir, "POTCAR_O"));
  QVERIFY(potcarO.open(QIODevice::WriteOnly | QIODevice::Text));
  potcarO.write("potcar o\n");
  potcarO.close();

  QFile inputFile(tempPath(tempDir, "xtalopt.in"));
  QVERIFY(inputFile.open(QIODevice::WriteOnly | QIODevice::Text));
  QTextStream input(&inputFile);
  input << "chemicalFormulas = Ti2O18\n";
  input << "queueInterface = none\n";
  input << "optimizer = vasp\n";
  input << "templatesDirectory = " << tempDir.path() << "\n";
  input << "incarTemplates = INCAR\n";
  input << "kpointsTemplates = KPOINTS\n";
  input << "potcarFile Ti = POTCAR_Ti\n";
  input << "potcarFile O = POTCAR_O\n";
  input << "molUnit 1 = O2 linear_2_pair\n";
  input << "molUnit 2 = Ti1O4 tetrahedral_td_center_shell\n";
  input << "localWorkingDirectory = " << tempPath(tempDir, "run") << "\n";
  inputFile.close();

  XtalOpt opt;
  QVERIFY2(opt.loadInputFile(inputFile.fileName(), true),
           "legacy numbered/asset input should convert to current syntax");

  // Numbered molUnit keys converted to repeated bare molUnit entries.
  QStringList expected;
  expected << "O2 linear_2_pair" << "O4Ti1 tetrahedral_td_center_shell";
  QCOMPARE(opt.moleculeUnitInputs(), expected);

  // Per-species POTCAR ids converted from the left of '=' into the value.
  const Search::OptimizerInputAssetMap assets = opt.getOptimizerInputAssets(0, "POTCAR");
  QCOMPARE(QString::fromStdString(assets.at("Ti")),
           QFileInfo(potcarTi.fileName()).absoluteFilePath());
  QCOMPARE(QString::fromStdString(assets.at("O")),
           QFileInfo(potcarO.fileName()).absoluteFilePath());
}

void LegacyCompatTest::loadInputFileLoadsLegacyMolecularUnits()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  QFile templateFile(tempPath(tempDir, "xtal.gin"));
  QVERIFY(templateFile.open(QIODevice::WriteOnly | QIODevice::Text));
  templateFile.write("# gulp template\n");
  templateFile.close();

  QFile inputFile(tempPath(tempDir, "xtalopt.in"));
  QVERIFY(inputFile.open(QIODevice::WriteOnly | QIODevice::Text));
  QTextStream input(&inputFile);
  input << "chemicalFormulas = Ti2O18\n";
  input << "queueInterface = none\n";
  input << "optimizer = gulp\n";
  input << "templatesDirectory = " << tempDir.path() << "\n";
  input << "ginTemplates = xtal.gin\n";
  input << "usingMolecularUnits = true\n";
  input << "molecularUnits 1 = Ti, 2, O, 4, tetrahedral, 1.320\n";
  input << "molecularUnits 2 = None, 1, O, 4, see-saw, 1.0\n";
  input << "molecularUnits 3 = O, 1, O, 5, square pyramidal, 1.0\n";
  input << "molecularUnits 4 = None, 7, O, 1, linear, 1.0\n";
  input << "localWorkingDirectory = " << tempPath(tempDir, "run") << "\n";
  inputFile.close();

  XtalOpt opt;
  QVERIFY2(opt.loadInputFile(inputFile.fileName(), true),
           "legacy molecularUnits input should convert to current molUnit entries");

  QStringList expected;
  expected << "O4Ti1 tetrahedral_td_center_shell"
           << "O4Ti1 tetrahedral_td_center_shell"
           << "O4 see_saw_c2v_homonuclear_shell"
           << "O6 square_pyramidal_c4v_homonuclear";
  QCOMPARE(opt.moleculeUnitInputs(), expected);
  QCOMPARE(opt.moleculeUnits().size(), static_cast<size_t>(expected.size()));
}

void LegacyCompatTest::loadInputFileConvertsLegacyMolecularUnitsWithoutAggregateCheck()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  QFile templateFile(tempPath(tempDir, "xtal.gin"));
  QVERIFY(templateFile.open(QIODevice::WriteOnly | QIODevice::Text));
  templateFile.write("# gulp template\n");
  templateFile.close();

  QFile inputFile(tempPath(tempDir, "xtalopt.in"));
  QVERIFY(inputFile.open(QIODevice::WriteOnly | QIODevice::Text));
  QTextStream input(&inputFile);
  input << "chemicalFormulas = Ti1O4\n";
  input << "queueInterface = none\n";
  input << "optimizer = gulp\n";
  input << "templatesDirectory = " << tempDir.path() << "\n";
  input << "ginTemplates = xtal.gin\n";
  input << "usingMolecularUnits = true\n";
  input << "molecularUnits 1 = Ti, 2, O, 4, tetrahedral, 1.320\n";
  input << "localWorkingDirectory = " << tempPath(tempDir, "run") << "\n";
  inputFile.close();

  XtalOpt opt;
  QVERIFY(opt.loadInputFile(inputFile.fileName(), true));

  QStringList expected;
  expected << "O4Ti1 tetrahedral_td_center_shell"
           << "O4Ti1 tetrahedral_td_center_shell";
  QCOMPARE(opt.moleculeUnitInputs(), expected);
}

void LegacyCompatTest::loadInputFileIgnoresLegacyMolecularUnitsWhenDisabled()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  QFile templateFile(tempPath(tempDir, "xtal.gin"));
  QVERIFY(templateFile.open(QIODevice::WriteOnly | QIODevice::Text));
  templateFile.write("# gulp template\n");
  templateFile.close();

  QFile inputFile(tempPath(tempDir, "xtalopt.in"));
  QVERIFY(inputFile.open(QIODevice::WriteOnly | QIODevice::Text));
  QTextStream input(&inputFile);
  input << "chemicalFormulas = O1\n";
  input << "queueInterface = none\n";
  input << "optimizer = gulp\n";
  input << "templatesDirectory = " << tempDir.path() << "\n";
  input << "ginTemplates = xtal.gin\n";
  input << "usingMolecularUnits = false\n";
  input << "molecularUnits 1 = Xx, 9, O, 4, tetrahedral, 1.320\n";
  input << "localWorkingDirectory = " << tempPath(tempDir, "run") << "\n";
  inputFile.close();

  XtalOpt opt;
  QVERIFY(opt.loadInputFile(inputFile.fileName(), true));
  QVERIFY(opt.moleculeUnitInputs().isEmpty());
  QVERIFY(opt.moleculeUnits().empty());
}

void LegacyCompatTest::loadInputFileNormalizesLegacyObjectivesReDoOption()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  QFile templateFile(tempPath(tempDir, "xtal.gin"));
  QVERIFY(templateFile.open(QIODevice::WriteOnly | QIODevice::Text));
  templateFile.write("# gulp template\n");
  templateFile.close();

  QFile inputFile(tempPath(tempDir, "xtalopt.in"));
  QVERIFY(inputFile.open(QIODevice::WriteOnly | QIODevice::Text));
  QTextStream input(&inputFile);
  input << "chemicalFormulas = O1\n";
  input << "queueInterface = none\n";
  input << "optimizer = gulp\n";
  input << "templatesDirectory = " << tempDir.path() << "\n";
  input << "ginTemplates = xtal.gin\n";
  input << "objectivesReDo = true\n";
  input << "localWorkingDirectory = " << tempPath(tempDir, "run") << "\n";
  inputFile.close();

  XtalOpt opt;
  QVERIFY(opt.loadInputFile(inputFile.fileName(), true));
  QVERIFY(opt.isConstraintsReDo());
}

void LegacyCompatTest::loadInputFileConvertsLegacyFiltrationObjectiveOnce()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  QFile templateFile(tempPath(tempDir, "xtal.gin"));
  QVERIFY(templateFile.open(QIODevice::WriteOnly | QIODevice::Text));
  templateFile.write("# gulp template\n");
  templateFile.close();

  QFile inputFile(tempPath(tempDir, "xtalopt.in"));
  QVERIFY(inputFile.open(QIODevice::WriteOnly | QIODevice::Text));
  QTextStream input(&inputFile);
  input << "chemicalFormulas = O1\n";
  input << "queueInterface = none\n";
  input << "optimizer = gulp\n";
  input << "templatesDirectory = " << tempDir.path() << "\n";
  input << "ginTemplates = xtal.gin\n";
  input << "constraint = filter filter.out\n";
  input << "objective = fil filter filter.out 0\n";
  input << "objective = fil filter2 filter2.out 0\n";
  input << "localWorkingDirectory = " << tempPath(tempDir, "run") << "\n";
  inputFile.close();

  XtalOpt opt;
  QVERIFY(opt.loadInputFile(inputFile.fileName(), true));
  QCOMPARE(opt.getUserObjectivesNum(), 0);
  QCOMPARE(opt.getConstraintsNum(), 2);
  QCOMPARE(opt.getConstraintExe(0), QString("filter"));
  QCOMPARE(opt.getConstraintOut(0), QString("filter.out"));
  QCOMPARE(opt.getConstraintExe(1), QString("filter2"));
  QCOMPARE(opt.getConstraintOut(1), QString("filter2.out"));
}

void LegacyCompatTest::stateSettingsLoadLegacyRadiiIADOption()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  const QString stateFile = tempPath(tempDir, "legacy-radii-iad.state");

  XtalOpt saved;
  saved.setLocWorkDir(tempDir.path());
  saved.setInputFormulasString("O1");
  QVERIFY(saved.processInputChemicalFormulas(saved.getInputFormulasString()));
  saved.setUsingScaledIAD(true);
  if (saved.getNumOptSteps() == 0)
    saved.appendOptStep();
  saved.setQueueInterface(0, "none");
  saved.setOptimizer(0, "gulp");
  QVERIFY(saved.saveSessionState(stateFile, false));

  {
    QSettings settings(stateFile, QSettings::IniFormat);
    markXtalOptStateVersion(settings, OriginalStateVersion);
    settings.setValue("xtalopt/init/chemical_formulas", "O1");
    settings.setValue("xtalopt/edit/numOptSteps", 1);
    settings.setValue("xtalopt/edit/queueInterface/0", "none");
    settings.setValue("xtalopt/edit/optimizer/0", "gulp");
    settings.beginGroup("xtalopt/init");
    settings.remove("using/radiiIADs");
    settings.setValue("using/interatomicDistanceLimit", false);
    settings.endGroup();
  }

  XtalOpt loaded;
  QVERIFY(loaded.readStateFile(stateFile, true));
  QVERIFY(QFile::exists(stateFile + ".compat"));
  QVERIFY(!loaded.getUsingScaledIAD());
}

void LegacyCompatTest::stateSettingsLoadLegacyBatchQueueCommands()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  const QString stateFile = tempPath(tempDir, "legacy-batch-queue.state");

  XtalOpt saved;
  saved.setLocWorkDir(tempDir.path());
  saved.setInputFormulasString("O1");
  QVERIFY(saved.processInputChemicalFormulas(saved.getInputFormulasString()));
  saved.setQueueInterface(0, "pbs");
  saved.setOptimizer(0, "gulp");
  QVERIFY(saved.saveSessionState(stateFile, false));

  {
    QSettings settings(stateFile, QSettings::IniFormat);
    markXtalOptStateVersion(settings, OriginalStateVersion);
    settings.setValue("xtalopt/init/chemical_formulas", "O1");
    settings.setValue("xtalopt/edit/numOptSteps", 1);
    settings.setValue("xtalopt/edit/queueInterface", "local");
    settings.setValue("xtalopt/edit/queueInterface/0", "pbs");
    settings.setValue("xtalopt/edit/optimizer/0", "gulp");
    settings.remove("xtalopt/optscheme/queue/0/commands/pbs");
    settings.remove("xtalopt/queueinterface");
    settings.setValue("xtalopt/sys/queue/qsub", "/old/bin/qsub");
    settings.setValue("xtalopt/sys/queue/qstat", "/old/bin/qstat");
    settings.setValue("xtalopt/sys/queue/qdel", "/old/bin/qdel");
  }

  XtalOpt loaded;
  QVERIFY(loaded.readStateFile(stateFile, true));
  QVERIFY(QFile::exists(stateFile + ".compat"));

  QSettings compatSettings(stateFile + ".compat", QSettings::IniFormat);
  QCOMPARE(compatSettings.value("xtalopt/optscheme/queue/0/commands/pbs/submit")
             .toString(),
           QString("/old/bin/qsub"));
  QVERIFY(!compatSettings.contains("xtalopt/sys/queue/qsub"));

  BatchQueueInterface* batch = qobject_cast<BatchQueueInterface*>(loaded.queueInterface(0));
  QVERIFY(batch != nullptr);
  QCOMPARE(batch->submitCommand(), QString("/old/bin/qsub"));
  QCOMPARE(batch->statusCommand(), QString("/old/bin/qstat"));
  QCOMPARE(batch->cancelCommand(), QString("/old/bin/qdel"));

}

void LegacyCompatTest::stateSettingsLoadLegacyMolecularUnits()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  const QString stateFile = tempPath(tempDir, "legacy-molunit.state");

  XtalOpt saved;
  saved.setLocWorkDir(tempDir.path());
  saved.setInputFormulasString("Ti1O6");
  QVERIFY(saved.processInputChemicalFormulas(saved.getInputFormulasString()));
  if (saved.getNumOptSteps() == 0)
    saved.appendOptStep();
  saved.setQueueInterface(0, "none");
  saved.setOptimizer(0, "gulp");
  QVERIFY(saved.saveSessionState(stateFile, false));

  {
    QSettings settings(stateFile, QSettings::IniFormat);
    markXtalOptStateVersion(settings, OriginalStateVersion);
    settings.setValue("xtalopt/init/chemical_formulas", "Ti1O6");
    settings.setValue("xtalopt/edit/numOptSteps", 1);
    settings.setValue("xtalopt/edit/queueInterface/0", "none");
    settings.setValue("xtalopt/edit/optimizer/0", "gulp");
    settings.beginGroup("xtalopt/init");
    settings.setValue("version", OriginalStateVersion);
    settings.remove("molUnit");
    settings.remove("compMolUnit");
    settings.setValue("using/molUnit", true);
    settings.beginWriteArray("compMolUnit");
    settings.setArrayIndex(0);
    settings.setValue("center", 0);
    settings.setValue("number_of_centers", 1);
    settings.setValue("neighbor", "O");
    settings.setValue("number_of_neighbors", 3);
    settings.setValue("geometry", "t-shaped");
    settings.setValue("distance", 1.0);
    settings.setArrayIndex(1);
    settings.setValue("center", "Ti");
    settings.setValue("number_of_centers", 1);
    settings.setValue("neighbor", "O");
    settings.setValue("number_of_neighbors", 3);
    settings.setValue("geometry", "t-shaped");
    settings.setValue("distance", 1.0);
    settings.endArray();
    settings.endGroup();
  }

  XtalOpt loaded;
  QVERIFY2(loaded.readStateFile(stateFile, true),
           "legacy compMolUnit state entries should convert to current molUnits");

  QStringList expected;
  expected << "O3 t_shaped_c2v_homonuclear_shell"
           << "O3Ti1 t_shaped_c2v_shared_neighbors";
  QCOMPARE(loaded.moleculeUnitInputs(), expected);
  QCOMPARE(loaded.moleculeUnits().size(), static_cast<size_t>(expected.size()));
}

void LegacyCompatTest::stateSaveRewritesLegacyResumeToFreshV5State()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  const QString stateFile = tempPath(tempDir, "legacy-resume.state");

  XtalOpt saved;
  saved.setLocWorkDir(tempDir.path());
  saved.setInputFormulasString("O1");
  QVERIFY(saved.processInputChemicalFormulas(saved.getInputFormulasString()));
  saved.setQueueInterface(0, "none");
  saved.setOptimizer(0, "gulp");
  QVERIFY(saved.saveSessionState(stateFile, false));

  {
    QSettings settings(stateFile, QSettings::IniFormat);
    markXtalOptStateVersion(settings, OriginalStateVersion);
    settings.setValue("xtalopt/init/chemical_formulas", "O1");
    settings.remove("xtalopt/init/using/radiiIADs");
    settings.setValue("xtalopt/init/using/interatomicDistanceLimit", false);
    settings.setValue("xtalopt/init/using/molUnit", false);
    settings.setValue("xtalopt/edit/numOptimizationSteps", 1);
    settings.setValue("xtalopt/edit/queueInterface/0", "local");
    settings.remove("xtalopt/edit/optimizer/0/directRunCommand");
    settings.setValue("xtalopt/edit/optimizer/0/exeLocation", "legacy-gulp-command");
    settings.remove("xtalopt/opt/opt/remoteQueue");
    settings.setValue("xtalopt/opt/opt/localQueue", true);
    settings.remove("xtalopt/obj/constraintsReDo");
    settings.setValue("xtalopt/obj/objectivesReDo", true);
    settings.setValue("xtalopt/optimizer/VASP/0/data/POTCAR info", "legacy optimizer state");
  }

  XtalOpt loaded;
  QVERIFY(loaded.readStateFile(stateFile, true));
  QVERIFY(QFile::exists(stateFile + ".compat"));
  QSettings compatSettings(stateFile + ".compat", QSettings::IniFormat);
  QVERIFY(!compatSettings.contains("xtalopt/optimizer/VASP/0/data/POTCAR info"));
  QCOMPARE(loaded.queueInterface(0)->getIDString(), QString("none"));
  QCOMPARE(loaded.optimizer(0)->getDirectRunCommand(), QString("legacy-gulp-command"));
  QVERIFY(!loaded.isRemoteQueue());
  QVERIFY(loaded.isConstraintsReDo());

  QVERIFY(loaded.saveSessionState(stateFile, false));
  QVERIFY(QFile::exists(stateFile + ".old"));

  QSettings finalSettings(stateFile, QSettings::IniFormat);
  QCOMPARE(finalSettings.value("xtalopt/version").toInt(),
           static_cast<int>(CurrentStateSchemaVersion));
  QVERIFY(!finalSettings.contains("xtalopt/init/version"));
  QVERIFY(!finalSettings.contains("xtalopt/edit/version"));
  QVERIFY(finalSettings.value("xtalopt/saveSuccessful", false).toBool());
  QVERIFY(finalSettings.contains("xtalopt/input/usingScaledIADs"));
  QVERIFY(finalSettings.contains("xtalopt/optscheme/optimizer/0/directRunCommand"));
  QVERIFY(finalSettings.contains("xtalopt/input/remoteQueue"));
  QVERIFY(finalSettings.contains("xtalopt/input/constraintsReDo"));
  QVERIFY(!finalSettings.contains("xtalopt/init"));
  QVERIFY(!finalSettings.contains("xtalopt/edit"));
  QVERIFY(!finalSettings.contains("xtalopt/opt"));
  QVERIFY(!finalSettings.contains("xtalopt/obj"));
  QVERIFY(!finalSettings.contains("xtalopt/optimizer/VASP/0/data/POTCAR info"));

  QSettings oldSettings(stateFile + ".old", QSettings::IniFormat);
  QCOMPARE(oldSettings.value("xtalopt/version").toInt(), static_cast<int>(OriginalStateVersion));
  QVERIFY(oldSettings.contains("xtalopt/init/using/interatomicDistanceLimit"));
  QVERIFY(oldSettings.contains("xtalopt/edit/optimizer/0/exeLocation"));
  QVERIFY(oldSettings.contains("xtalopt/opt/opt/localQueue"));
  QVERIFY(oldSettings.contains("xtalopt/obj/objectivesReDo"));
}

void LegacyCompatTest::stateSettingsNormalizeSingleLegacyVaspPotcarEntry()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  const QString stateFile = tempPath(tempDir, "legacy-vasp-potcar-single.state");

  XtalOpt saved;
  saved.setLocWorkDir(tempDir.path());
  saved.setInputFormulasString("Ti2O4");
  QVERIFY(saved.processInputChemicalFormulas(saved.getInputFormulasString()));
  saved.setQueueInterface(0, "none");
  saved.setOptimizer(0, "vasp");
  QVERIFY(saved.saveSessionState(stateFile, false));
  {
    QSettings settings(stateFile, QSettings::IniFormat);
    markXtalOptStateVersion(settings, OriginalStateVersion);
    settings.setValue("xtalopt/init/chemical_formulas", "Ti2O4");
    settings.setValue("xtalopt/edit/numOptSteps", 1);
    settings.setValue("xtalopt/edit/queueInterface/0", "none");
    settings.setValue("xtalopt/edit/optimizer/0", "vasp");
    settings.setValue("xtalopt/edit/optimizer/0/vasp/POTCAR",
                      "%fileContents:/potentials/system/POTCAR%\n");
  }

  XtalOpt loaded;
  QVERIFY(loaded.readStateFile(stateFile, true));

  const Search::OptimizerInputAssetMap assets = loaded.getOptimizerInputAssets(0, "POTCAR");
  QCOMPARE(QString::fromStdString(assets.at("system")), QString("/potentials/system/POTCAR"));
}

void LegacyCompatTest::stateSettingsNormalizeLegacyVaspPotcarEntries()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  const QString stateFile = tempPath(tempDir, "legacy-vasp-potcar.state");

  XtalOpt saved;
  saved.setLocWorkDir(tempDir.path());
  saved.setInputFormulasString("Ti2O4");
  QVERIFY(saved.processInputChemicalFormulas(saved.getInputFormulasString()));
  saved.setQueueInterface(0, "none");
  saved.setOptimizer(0, "vasp");
  QVERIFY(saved.saveSessionState(stateFile, false));
  {
    QSettings settings(stateFile, QSettings::IniFormat);
    markXtalOptStateVersion(settings, OriginalStateVersion);
    settings.setValue("xtalopt/init/chemical_formulas", "Ti2O4");
    settings.setValue("xtalopt/edit/numOptSteps", 1);
    settings.setValue("xtalopt/edit/queueInterface/0", "none");
    settings.setValue("xtalopt/edit/optimizer/0", "vasp");
    settings.setValue("xtalopt/edit/optimizer/0/vasp/POTCAR",
                      "%fileContents:/potentials/O/POTCAR%\n"
                      "%fileContents:/potentials/Ti/POTCAR%\n");
  }

  XtalOpt loaded;
  QVERIFY(loaded.readStateFile(stateFile, true));

  const Search::OptimizerInputAssetMap assets = loaded.getOptimizerInputAssets(0, "POTCAR");
  QCOMPARE(QString::fromStdString(assets.at("O")), QString("/potentials/O/POTCAR"));
  QCOMPARE(QString::fromStdString(assets.at("Ti")), QString("/potentials/Ti/POTCAR"));
}

void LegacyCompatTest::stateSettingsRejectMismatchedLegacyVaspPotcarEntries()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  const QString stateFile = tempPath(tempDir, "legacy-vasp-potcar-mismatch.state");

  XtalOpt saved;
  saved.setLocWorkDir(tempDir.path());
  saved.setInputFormulasString("Ti2O4");
  QVERIFY(saved.processInputChemicalFormulas(saved.getInputFormulasString()));
  saved.setQueueInterface(0, "none");
  saved.setOptimizer(0, "vasp");
  QVERIFY(saved.saveSessionState(stateFile, false));
  {
    QSettings settings(stateFile, QSettings::IniFormat);
    markXtalOptStateVersion(settings, OriginalStateVersion);
    settings.setValue("xtalopt/init/chemical_formulas", "Ti2O4");
    settings.setValue("xtalopt/edit/numOptSteps", 1);
    settings.setValue("xtalopt/edit/queueInterface/0", "none");
    settings.setValue("xtalopt/edit/optimizer/0", "vasp");
    settings.setValue("xtalopt/edit/optimizer/0/vasp/POTCAR",
                      "%fileContents:/potentials/system/POTCAR%\n"
                      "%fileContents:/potentials/O/POTCAR%\n"
                      "%fileContents:/potentials/Ti/POTCAR%\n");
  }

  XtalOpt loaded;
  QVERIFY(!loaded.readStateFile(stateFile, true));
  QVERIFY(!QFile::exists(stateFile + ".compat"));
}

void LegacyCompatTest::legacyWorkflowStatusesNormalizeOnRead()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  // Old structure files are converted along with their old main state file.
  const QString mainStateFile = tempPath(tempDir, "xtalopt.state");
  {
    QSettings settings(mainStateFile, QSettings::IniFormat);
    markXtalOptStateVersion(settings, OriginalStateVersion);
  }

  struct StatusConversion
  {
    int version14State;
    Structure::State currentState;
  };

  const StatusConversion conversions[] = {
    { Version14Optimized, Structure::Optimized },
    { Version14StepOptimized, Structure::StepOptimized },
    { Version14WaitingForOptimization, Structure::WaitingForOptimization },
    { Version14InProcess, Structure::InProcess },
    { Version14Empty, Structure::Empty },
    { Version14Updating, Structure::Updating },
    { Version14Error, Structure::Error },
    { Version14Submitted, Structure::Submitted },
    { Version14Killed, Structure::Killed },
    { Version14Removed, Structure::Removed },
    { Version14Similar, Structure::Optimized },
    { Version14Restart, Structure::Restart },
    { Version14ObjectiveDismiss, Structure::Dismissed },
    { Version14ObjectiveFail, Structure::ObjcFailed },
    { Version14ObjectiveRetain, Structure::Optimized },
    { Version14ObjectiveCalculation, Structure::ObjectiveCalculation }
  };

  for (size_t i = 0; i < sizeof(conversions) / sizeof(conversions[0]); ++i) {
    const QString stateFile = tempPath(tempDir, QString("status-%1.state").arg(i));
    writeRawWorkflowState(stateFile, OriginalStateVersion, conversions[i].version14State);

    // The file is read before it is converted, so the structure starts out with
    //   the old status value; the conversion replaces it with the current one.
    Xtal loaded;
    loaded.setStatus(static_cast<Structure::State>(conversions[i].version14State));
    bool currentInfoRead = false;
    QVERIFY(Legacy::convertAndReadStructureState(loaded, stateFile, mainStateFile,
                                                 currentInfoRead));
    QCOMPARE(loaded.getStatus(), conversions[i].currentState);
  }

  // Check an unknown old status.
  const QString unknownStateFile = tempPath(tempDir, "unknown-status.state");
  writeRawWorkflowState(unknownStateFile, OriginalStateVersion, 999);
  Xtal unknown;
  unknown.setStatus(static_cast<Structure::State>(999));
  bool currentInfoRead = false;
  QVERIFY(!Legacy::convertAndReadStructureState(unknown, unknownStateFile, mainStateFile,
                                                currentInfoRead));
}

void LegacyCompatTest::unversionedTextStateIsRejected()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  const QString stateFile = tempPath(tempDir, "legacy-v0.state");

  QFile file(stateFile);
  QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
  file.write("Generation: 4\n");
  file.write("Status: 12\n");
  file.close();

  // Check an unsupported old structure state.
  Xtal loaded;
  bool currentInfoRead = false;
  QVERIFY(!Legacy::convertAndReadStructureState(loaded, stateFile, QString(), currentInfoRead));
}

void LegacyCompatTest::unsupportedStructureVersionsAreRejected()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  bool currentInfoRead = false;
  for (int version = 1; version <= 3; ++version) {
    const QString stateFile = tempPath(tempDir,
      QString("structure-v%1.state").arg(version));
    Xtal saved;
    writeStructureStateFile(saved, stateFile);
    {
      QSettings settings(stateFile, QSettings::IniFormat);
      settings.setValue("structure/version", version);
    }
    Xtal loaded;
    QVERIFY(!Legacy::convertAndReadStructureState(loaded, stateFile, QString(), currentInfoRead));
  }

  const QString version13State = tempPath(tempDir, "structure-v13.state");
  Xtal saved;
  writeStructureStateFile(saved, version13State);
  {
    QSettings settings(version13State, QSettings::IniFormat);
    settings.setValue("structure/version", OriginalStateVersion);
    settings.remove("structure/hasValidComposition");
    settings.setValue("structure/supercellGenerationChecked", false);
    settings.setValue("structure/objectivesState", Structure::Os_NotCalculated);
  }
  Xtal version13Loaded;
  QVERIFY(!Legacy::convertAndReadStructureState(version13Loaded, version13State, QString(), currentInfoRead));

  const QString version12State = tempPath(tempDir, "structure-v12.state");
  writeStructureStateFile(saved, version12State);
  {
    QSettings settings(version12State, QSettings::IniFormat);
    settings.setValue("structure/version", OriginalStateVersion);
    settings.remove("structure/hasValidComposition");
    settings.remove("structure/objectivesState");
    settings.setValue("structure/fileName", "old-structure");
    settings.setValue("structure/supercellGenerationChecked", false);
  }
  Xtal version12Loaded;
  QVERIFY(!Legacy::convertAndReadStructureState(version12Loaded, version12State, QString(), currentInfoRead));
}

} // namespace XtalOpt

QTEST_MAIN(XtalOpt::LegacyCompatTest)

#include "legacycompattest.moc"

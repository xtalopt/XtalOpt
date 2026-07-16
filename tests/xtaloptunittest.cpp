/**********************************************************************
  XtalOptUnitTest - Unit testing for XtalOpt functions

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

#include <cmath>

using namespace Search;

namespace XtalOpt {
namespace {

QString tempPath(const QTemporaryDir& dir, const QString& child)
{
  return Common::localPath(dir.path(), child);
}

// Compare a produced file with its expected copy. Set the
// XTALOPT_UPDATE_EXPECTED=1 to re-write/update the expected copy.
void compareWithExpectedFile(const QString& producedPath, const QString& expectedPath)
{
  QFile producedFile(producedPath);
  QVERIFY(producedFile.open(QIODevice::ReadOnly));
  const QByteArray produced = producedFile.readAll();
  QVERIFY(!produced.isEmpty());

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
  QCOMPARE(produced, expectedFile.readAll());
}

Xtal* makeTestXtal(uint generation, uint id, int index, int atomicNumber,
                   const Common::Vector3& position, double distAboveHull)
{
  Xtal* xtal = new Xtal(5.0, 5.0, 5.0, 90.0, 90.0, 90.0);
  Atoms::Atom& atom = xtal->addAtom();
  atom.setAtomicNumber(atomicNumber);
  atom.setPos(position);
  xtal->setGeneration(generation);
  xtal->setIDNumber(id);
  xtal->setIndex(index);
  xtal->setStatus(Xtal::Optimized);
  xtal->setChangedSinceSimChecked(true);
  xtal->setEnthalpy(-1.0);
  xtal->setDistAboveHull(distAboveHull);
  xtal->setStrucObjValuesVec(QList<double>() << distAboveHull);
  return xtal;
}

QString exportedOptionsText(XtalOpt& opt)
{
  QTemporaryDir tempDir;
  if (!tempDir.isValid())
    return QString();

  const QString filename = tempPath(tempDir, "xtalopt.in");
  if (!opt.writeInputFile(filename))
    return QString();

  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return QString();

  return QString::fromLocal8Bit(file.readAll());
}

void insertCustomIAD(XtalOpt& opt, int atomicNum1, int atomicNum2, double value)
{
  IAD iad;
  iad.minIAD = value;
  opt.interComp().insert(qMakePair(atomicNum1, atomicNum2), iad);
  opt.interComp().insert(qMakePair(atomicNum2, atomicNum1), iad);
}

// Write a current state file with the given objective entries.
void writeObjectiveStateFixture(const QString& stateFile, const QStringList& entries)
{
  QSettings settings(stateFile, QSettings::IniFormat);
  settings.setValue("xtalopt/version", CurrentStateSchemaVersion);
  settings.beginGroup("xtalopt/input");
  settings.beginWriteArray("objective");
  for (int i = 0; i < entries.size(); ++i) {
    settings.setArrayIndex(i);
    settings.setValue("entry", entries.at(i));
  }
  settings.endArray();
  settings.endGroup();
  settings.sync();
}

// Build a fixed Ti-O session with several structure states, a parent link,
// and an objective value. Each structure has a directory under workdir.
int buildTestSearch(XtalOpt& opt, const QString& workdir)
{
  opt.setLocWorkDir(workdir);

  // Set a direct-run GULP queue and optimizer (save function needs both).
  opt.appendOptStep();
  opt.setQueueInterface(0, "none");
  opt.setOptimizer(0, "gulp");

  CellComp comp;
  comp.setCompositionEntry("Ti", 22, 1);
  comp.setCompositionEntry("O", 8, 2);
  opt.compList().append(comp);
  // Set the composition text used when the session is read again.
  opt.setInputFormulasString("Ti2O4 - Ti4O8");

  struct Spec
  {
    uint generation;
    uint id;
    int index;
    int nTi;
    int nO;
    double enthalpy;
    Xtal::State status;
    const char* parentTag;     // nullptr when the structure has no parent
    QList<double> objectives;  // empty unless this is a multi-objective entry
  };
  // Use terminal states for all structures so the load part keeps them unchanged.
  const QList<Spec> specs{
    { 1, 1, 0, 1, 0, -1.0, Xtal::Optimized, nullptr, {} },
    { 1, 2, 1, 0, 1, -0.5, Xtal::Optimized, nullptr, {} },
    { 1, 3, 2, 1, 2, -6.0, Xtal::Optimized, nullptr, {} },
    { 2, 1, 3, 1, 1, -3.0, Xtal::Optimized, "1x1", {} },
    { 2, 2, 4, 2, 1, -2.5, Xtal::Killed, nullptr, {} },
    { 2, 3, 5, 1, 2, -4.0, Xtal::Removed, nullptr, { 0.1, 2.5 } },
  };

  QList<Structure*> structures;
  QHash<QString, Xtal*> byTag;
  for (const Spec& spec : specs) {
    Xtal* xtal = new Xtal(5.0, 5.0, 5.0, 90.0, 90.0, 90.0);
    for (int i = 0; i < spec.nTi; ++i) {
      Atoms::Atom& atom = xtal->addAtom();
      atom.setAtomicNumber(22);
      atom.setPos(Common::Vector3(0.5 * i, 0.0, 0.0));
    }
    for (int i = 0; i < spec.nO; ++i) {
      Atoms::Atom& atom = xtal->addAtom();
      atom.setAtomicNumber(8);
      atom.setPos(Common::Vector3(0.0, 0.5 * (i + 1), 0.0));
    }
    xtal->setGeneration(spec.generation);
    xtal->setIDNumber(spec.id);
    xtal->setIndex(spec.index);
    xtal->setStatus(spec.status);
    xtal->setEnthalpy(spec.enthalpy);
    xtal->setChangedSinceSimChecked(false);
    if (!spec.objectives.isEmpty()) {
      xtal->setStrucObjValuesVec(spec.objectives);
      xtal->setStrucObjState(Xtal::Os_Retain);
    }

    const QString locpath = Common::localPath(
      workdir, QString("%1x%2")
                 .arg(spec.generation, 5, 10, QChar('0'))
                 .arg(spec.id, 5, 10, QChar('0')));
    if (!QDir().mkpath(locpath))
      return -1;
    xtal->setLocpath(locpath);

    byTag.insert(xtal->getTag(), xtal);
    structures.append(xtal);
  }

  // Add parent links after all structures have been made.
  for (int i = 0; i < specs.size(); ++i) {
    if (specs.at(i).parentTag)
      structures.at(i)->setParentStructure(byTag.value(specs.at(i).parentTag));
  }

  {
    QWriteLocker trackerLocker(opt.tracker()->rwLock());
    for (auto* structure : structures)
      opt.tracker()->append(structure);
  }

  return structures.size();
}

class CurrentPathGuard
{
public:
  explicit CurrentPathGuard(const QString& path)
    : m_oldPath(QDir::currentPath()),
      m_changed(QDir::setCurrent(path))
  {
  }

  ~CurrentPathGuard()
  {
    if (m_changed)
      QDir::setCurrent(m_oldPath);
  }

  bool changed() const
  {
    return m_changed;
  }

private:
  QString m_oldPath;
  bool m_changed;
};

class SessionTestXtalOpt : public XtalOpt
{
public:
  using XtalOpt::setSessionStarting;
  using XtalOpt::setSessionActive;
};

} // namespace

class XtalOptUnitTest : public QObject
{
  Q_OBJECT

private slots:
  // Called before the first test function is executed.
  void initTestCase()
  {
    Common::seedMt19937Generator(0);
  }
  // Called after the last test function is executed.
  void cleanupTestCase(){};
  // Called before each test function is executed.
  void init(){};
  // Called after every test function.
  void cleanup(){};

  // Tests
  void constructorUsesBuiltInDefaults();
  void initialGenerationCountBehavior();
  void initialGenerationPlanForcedRandSpg();

  void loadTest();
  void readOnlyResumeLoadsState();

  // Invalid settings values are rejected, reset, or kept; per invalid action.
  void invalidSettingsRejectedResetOrReverted();

  // Output files must match the saved expected files.
  void inputReadAndVerifyLoadsAssets();
  void structureStateFileIsStable();
  void saveWritesOnlyChangedStructureStates();
  void resultsOutputsAreStable();

  // Settings
  void settingsDefaultsMatchBuiltInDefaults();
  void settingsRegistryAppliesAndReadsScalars();
  void readOnlySearchDoesNotSaveHullSnapshot();
  void objectiveSettingsValidation();
  void constraintInputSyntax();
  void removeUserObjectivePreservesBuiltinObjective();
  void runtimeOptionsApplyRuntimeChangeableKeys();
  void processInputDataClearsCachesWhenInputsEmpty();
  void optimizerAssetSyntaxReadAndWrite();
  void writeOptionsFileUsesCentralTemplateMap();
  void writeOptionsFileCoversEveryScalarKeyword();
  void readOptionsAppliesFailActionPolicy();
  void readOptionsAppliesScalarSettingRegistry();
  void readOptionsAppliesCentralTemplateMap();
  void importReadResolvesInputsWithoutRunArtifacts();
  void readOptionsResolvesFromProcessCwdWithoutRunArtifacts();
  void optimizerInputAssetsAreLiteral();
  void startSearchPrechecksInputFiles();
  void readOptionsLocalSchedulerConfiguration();
  void readOptionsRemoteQueueConfiguration();
  void initialRuntimeFileContainsOnlyRuntimeSubset();
  void runtimeOptionsDoNotChangeFixedKeys();
  void genericSettingsDoNotEnableRemoteQueueByDefault();
  void loadPathsGovernLocalWorkDir();
  void stateSettingsReadAndWritePreservesMainFields();
  void customIADCheckLimitsValidatesTable();
  void generateRandomXtalHonorsConstraints();
  void generateRandomXtalWithMoleculeUnits();
  void resultsFileSortsByAboveHull();
  void checkForDuplicatesTest();
  void stepwiseCheckForDuplicatesTest();
};

void XtalOptUnitTest::constructorUsesBuiltInDefaults()
{
  XtalOpt opt;
  QCOMPARE(opt.getNumInitial(), static_cast<uint>(20));
  QCOMPARE(opt.getMaxNumStructures(), 100);
  QCOMPARE(static_cast<int>(opt.getNumOptSteps()), 1);
  QVERIFY(opt.optimizer(0) != nullptr);
  QVERIFY(opt.queueInterface(0) != nullptr);
  QCOMPARE(opt.optimizer(0)->getIDString().toLower(), QString("gulp"));
  QCOMPARE(opt.queueInterface(0)->getIDString().toLower(), QString("none"));
  QVERIFY(!opt.isParetoFilterZeroWeights());
}

void XtalOptUnitTest::initialGenerationCountBehavior()
{
  // numInitial is the random initial structure count.
  {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    SessionTestXtalOpt opt;
    opt.setLocWorkDir(tempDir.path());
    opt.setUsingScaledIAD(false);
    opt.setNumInitial(3);
    opt.setMaxNumStructures(100);

    CellComp comp;
    comp.setCompositionEntry("H", 1, 1);
    opt.compList().append(comp);

    opt.setSessionStarting(true);
    QVERIFY(opt.generateInitialStructures());
    opt.setSessionStarting(false);

    QCOMPARE(opt.tracker()->size(), 3);
  }

  // numInitial is used even when it exceeds maxNumStructures.
  {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    SessionTestXtalOpt opt;
    opt.setLocWorkDir(tempDir.path());
    opt.setUsingScaledIAD(false);
    opt.setNumInitial(10);
    opt.setMaxNumStructures(4);

    CellComp comp;
    comp.setCompositionEntry("H", 1, 1);
    opt.compList().append(comp);

    opt.setSessionStarting(true);
    QVERIFY(opt.generateInitialStructures());
    opt.setSessionStarting(false);

    QCOMPARE(opt.tracker()->size(), 10);
  }

  // Do not add structures only to cover every formula.
  {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    SessionTestXtalOpt opt;
    opt.setLocWorkDir(tempDir.path());
    opt.setUsingScaledIAD(false);
    opt.setNumInitial(3);
    opt.setMaxNumStructures(100);

    CellComp comp;
    comp.setCompositionEntry("H", 1, 1);
    for (int i = 0; i < 20; ++i)
      opt.compList().append(comp);

    opt.setSessionStarting(true);
    QVERIFY(opt.generateInitialStructures());
    opt.setSessionStarting(false);

    QCOMPARE(opt.tracker()->size(), 3);
  }
}

void XtalOptUnitTest::initialGenerationPlanForcedRandSpg()
{
  // Forced space groups add to the random initial structures.
  {
    XtalOpt opt;
    opt.setUsingRandSpg(true);
    opt.setNumInitial(3);
    opt.setMaxNumStructures(100);

    CellComp comp;
    comp.setCompositionEntry("H", 1, 1);
    opt.compList().append(comp);

    QList<int> forced;
    for (int i = 0; i < 230; ++i)
      forced.append(0);
    forced[0] = 2;
    opt.minXtalsOfSpg() = forced;

    XtalOpt::InitialGenerationPlan plan;
    QString error;
    QVERIFY(opt.buildInitialGenerationPlan(plan, &error));
    QCOMPARE(plan.seedCount, static_cast<uint>(0));
    QCOMPARE(plan.forcedRandSpgCount, static_cast<uint>(2));
    QCOMPARE(plan.randomCount, static_cast<uint>(3));
    QCOMPARE(plan.totalTarget, static_cast<uint>(5));
  }

  // Ignore a forced space group with no matching formula.
  {
    XtalOpt opt;
    opt.setUsingRandSpg(false);
    opt.setNumInitial(3);

    CellComp comp;
    comp.setCompositionEntry("H", 1, 1);
    opt.compList().append(comp);

    QList<int> forced;
    for (int i = 0; i < 230; ++i)
      forced.append(0);
    int impossibleIndex = -1;
    for (uint spg = 1; spg <= 230; ++spg) {
      if (opt.randSpgCompatibleFormulaStrings(spg).isEmpty()) {
        impossibleIndex = static_cast<int>(spg) - 1;
        break;
      }
    }
    QVERIFY(impossibleIndex >= 0);

    forced[impossibleIndex] = 1;
    opt.minXtalsOfSpg() = forced;

    XtalOpt::InitialGenerationPlan plan;
    QString error;
    QVERIFY(opt.buildInitialGenerationPlan(plan, &error));
    QVERIFY(error.isEmpty());
    QCOMPARE(plan.randSpgCounts.at(impossibleIndex), -1);
    QCOMPARE(plan.forcedRandSpgCount, static_cast<uint>(0));
    QCOMPARE(plan.randomCount, static_cast<uint>(3));
    QCOMPARE(plan.totalTarget, static_cast<uint>(3));
  }
}

void XtalOptUnitTest::loadTest()
{
  // Save and load a session.
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  // Keep the saved session available.
  XtalOpt producer;
  const int count = buildTestSearch(producer, tempDir.path());
  QVERIFY(count > 0);
  QVERIFY(producer.refreshStructureEvaluationData());
  QVERIFY(producer.save(Common::localPath(tempDir.path(), "xtalopt.state")));

  XtalOpt reloaded;
  reloaded.tracker()->blockSignals(true);
  reloaded.setRunMode(XtalOpt::RunModeReadOnly);
  QVERIFY(reloaded.resumeSearch(Common::localPath(tempDir.path(), "xtalopt.state")));
  reloaded.tracker()->blockSignals(false);

  QCOMPARE(reloaded.tracker()->size(), count);

  // The direct-run gulp optimizer full test through save/load.
  QVERIFY(reloaded.optimizer(0));
  QCOMPARE(reloaded.optimizer(0)->getIDString().toLower(), QString("gulp"));

  // Look a reloaded structure up by its <generation, id> tag.
  auto findByTag = [&](uint generation, uint id) -> Structure* {
    for (Structure* structure : reloaded.queue()->getAllStructures()) {
      if (structure->getGeneration() == generation && structure->getIDNumber() == id)
        return structure;
    }
    return nullptr;
  };

  // Check one structure's identity and energy.
  Structure* tio2 = findByTag(1, 3);
  QVERIFY(tio2);
  QCOMPARE(tio2->getEnthalpy(), -6.0);

  // Check "terminal" structure states.
  QVERIFY(findByTag(1, 1));
  QCOMPARE(findByTag(1, 1)->getStatus(), Structure::Optimized);
  QVERIFY(findByTag(2, 1));
  QCOMPARE(findByTag(2, 1)->getStatus(), Structure::Optimized);
  QVERIFY(findByTag(2, 2));
  QCOMPARE(findByTag(2, 2)->getStatus(), Structure::Killed);
  QVERIFY(findByTag(2, 3));
  QCOMPARE(findByTag(2, 3)->getStatus(), Structure::Removed);

  // Check that the saved parent tag becomes a live parent pointer.
  Structure* child = findByTag(2, 1);
  QVERIFY(child->getParentStructure());
  QCOMPARE(child->getParentStructure()->getGeneration(), 1u);
  QCOMPARE(child->getParentStructure()->getIDNumber(), 1u);

  // Check the objective values.
  Structure* removed = findByTag(2, 3);
  QVERIFY(removed);
  QCOMPARE(removed->getStrucObjNumber(), 2);
  QVERIFY(std::isnan(removed->getStrucObjValues(0)));
  QCOMPARE(removed->getStrucObjValues(1), 2.5);
  QCOMPARE(removed->getStrucObjState(), Structure::Os_Retain);
}

void XtalOptUnitTest::readOnlyResumeLoadsState()
{
  // Read a saved session in read-only (eg, plotting mode).
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  XtalOpt producer;
  const int count = buildTestSearch(producer, tempDir.path());
  QVERIFY(count > 0);
  QVERIFY(producer.refreshStructureEvaluationData());
  QVERIFY(producer.save(Common::localPath(tempDir.path(), "xtalopt.state")));

  XtalOpt opt;
  opt.setRunMode(XtalOpt::RunModeReadOnly);
  QVERIFY(opt.resumeSearch(Common::localPath(tempDir.path(), "xtalopt.state")));
  QVERIFY(opt.isReadOnly());
  QCOMPARE(opt.tracker()->size(), count);

  // Check saved structures and states (parent links are checked by the full load).
  Structure* killed = nullptr;
  for (Structure* structure : opt.queue()->getAllStructures()) {
    if (structure->getGeneration() == 2 && structure->getIDNumber() == 2) {
      killed = structure;
      break;
    }
  }
  QVERIFY(killed);
  QCOMPARE(killed->getStatus(), Structure::Killed);
}

void XtalOptUnitTest::invalidSettingsRejectedResetOrReverted()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  // Reject an input file with maximum volume below minimum volume.
  {
    const QString src = Common::localPath(QString(TESTDATADIR), "inputs/xtalopt.in");
    const QString dst = tempPath(tempDir, "xtalopt.in");
    QVERIFY(QFile::copy(src, dst));
    {
      // Add bad volume limits.
      QFile f(dst);
      QVERIFY(f.open(QIODevice::Append | QIODevice::Text));
      f.write("\nminVolume = 100.0\nmaxVolume = 1.0\n");
      f.close();
    }

    XtalOpt good;
    good.setRunMode(XtalOpt::RunModeReadOnly);
    QVERIFY(good.readInputFile(src, /*bestEffort*/ true,
                               /*loadAndVerifyAssets*/ false)); // control: valid

    XtalOpt bad;
    bad.setRunMode(XtalOpt::RunModeReadOnly);
    QVERIFY(!bad.readInputFile(dst, /*bestEffort*/ true,
                               /*loadAndVerifyAssets*/ false)); // bad volume
  }

  // Reset bad saved volume limits to their defaults.
  {
    const QString file = tempPath(tempDir, "settings.state");

    XtalOpt producer;
    producer.setVolMin(100.0); // deliberately inconsistent
    producer.setVolMax(1.0);
    QVERIFY(producer.saveSettingsState(file));

    XtalOpt loaded;
    QVERIFY(loaded.loadSettingsState(file));
    QCOMPARE(loaded.getVolMin(), 1.0);   // reset to the default
    QCOMPARE(loaded.getVolMax(), 100.0);
  }

  // Reject bad runtime volume limits and keep the current values.
  {
    XtalOpt opt; // defaults seed volMin = 1.0, volMax = 100.0
    QCOMPARE(opt.getVolMin(), 1.0);
    QCOMPARE(opt.getVolMax(), 100.0);
    opt.readRuntimeOptions("minVolume = 100.0\nmaxVolume = 1.0\n");
    QCOMPARE(opt.getVolMin(), 1.0); // unchanged
    QCOMPARE(opt.getVolMax(), 100.0);
  }

  // Reject a runtime crossover min contribution outside its allowed range.
  {
    XtalOpt opt;
    opt.readRuntimeOptions("crossoverMinContribution = 60\n");
    QCOMPARE(opt.getCrossMinimumContribution(), 25u);
  }

  // Reject invalid search limits and Stripple ranges at runtime.
  {
    XtalOpt opt;
    opt.readRuntimeOptions("parentsPoolSize = 0\n"
                           "maxNumStructures = 0\n"
                           "strippleAmplitudeMin = 0.9\n"
                           "strippleAmplitudeMax = 0.2\n");
    QCOMPARE(opt.getParentsPoolSize(), 20u);
    QCOMPARE(opt.getMaxNumStructures(), 100);
    QCOMPARE(opt.getStripAmpMin(), 0.5);
    QCOMPARE(opt.getStripAmpMax(), 1.0);
  }
}

void XtalOptUnitTest::inputReadAndVerifyLoadsAssets()
{
  // Read and load the input templates from a complete input file.
  const QString fixture = Common::localPath(QString(TESTDATADIR), "inputs/xtalopt.in");
  XtalOpt opt;
  opt.setRunMode(XtalOpt::RunModeReadOnly);
  // Resolve relative template paths beside the input file.
  QVERIFY(opt.readInputFile(fixture, /*bestEffort*/ true));
  QCOMPARE(static_cast<int>(opt.getNumOptSteps()), 1);
  QVERIFY(opt.optimizer(0));
  QCOMPARE(opt.optimizer(0)->getIDString().toLower(), QString("gulp"));
  QCOMPARE(opt.queueInterface(0)->getIDString().toLower(), QString("none"));
  // Check the GULP input file.
  QVERIFY(!QString::fromStdString(opt.getOptimizerTemplate(0, "xtal.gin"))
             .isEmpty());
}

void XtalOptUnitTest::settingsDefaultsMatchBuiltInDefaults()
{
  XtalOpt opt;
  Settings::applyAllDefaults(opt);

  // Every scalar setting must return its table default.
  for (const auto& keyword : Settings::allKeywords()) {
    if (!Settings::hasScalarBinding(keyword))
      continue;

    const QString expected = Settings::defaultValue(keyword);
    const QString actual = Settings::scalarValue(opt, keyword);

    bool expectedIsNumber = false;
    const double expectedNumber = expected.toDouble(&expectedIsNumber);
    if (expectedIsNumber) {
      bool actualIsNumber = false;
      const double actualNumber = actual.toDouble(&actualIsNumber);
      QVERIFY2(actualIsNumber, qPrintable(keyword));
      QVERIFY2(qFuzzyCompare(1.0 + expectedNumber, 1.0 + actualNumber),
               qPrintable(keyword + ": " + expected + " vs " + actual));
    } else {
      QVERIFY2(actual.compare(expected, Qt::CaseInsensitive) == 0,
               qPrintable(keyword + ": " + expected + " vs " + actual));
    }
  }

  // Spot checks across a range of scalar keywords.
  QCOMPARE(opt.getMaxAtoms(), 20);
  QCOMPARE(opt.getAMin(), 3.0);
  QCOMPARE(opt.getPCross(), 35u);
  QCOMPARE(opt.getTolRdfNbins(), 3000);
  QCOMPARE(opt.getContStructs(), 15u);
  QCOMPARE(opt.getFailAction(), Search::SearchBase::FA_KillIt);
}

void XtalOptUnitTest::settingsRegistryAppliesAndReadsScalars()
{
  XtalOpt opt;
  Settings::applyAllDefaults(opt);

  // Check a normal scalar setting.
  QVERIFY(Settings::applyScalar(opt, "maxAtoms", "33"));
  QCOMPARE(opt.getMaxAtoms(), 33);
  QCOMPARE(Settings::scalarValue(opt, "maxAtoms"), QString("33"));

  // Case-insensitive keywords.
  QVERIFY(Settings::applyScalar(opt, "AMIN", "2.5"));
  QCOMPARE(opt.getAMin(), 2.5);

  // Boolean settings accept true/false and yes/no.
  QVERIFY(Settings::applyScalar(opt, "softExit", "yes"));
  QVERIFY(opt.isSoftExit());
  QVERIFY(Settings::applyScalar(opt, "softExit", "NO"));
  QVERIFY(!opt.isSoftExit());
  QVERIFY(!Settings::applyScalar(opt, "softExit", "1"));
  QVERIFY(!Settings::applyScalar(opt, "softExit", "0"));
  QVERIFY(!Settings::applyScalar(opt, "softExit", "maybe"));
  QVERIFY(!opt.isSoftExit());

  // Check a setting with special input handling.
  QVERIFY(Settings::applyScalar(opt, "jobFailAction", "kill"));
  QCOMPARE(opt.getFailAction(), Search::SearchBase::FA_KillIt);
  QCOMPARE(Settings::scalarValue(opt, "jobFailAction"), QString("kill"));
  QVERIFY(!Settings::applyScalar(opt, "jobFailAction", "nonsense"));

  // Bad numeric values change nothing and report failure.
  QVERIFY(!Settings::applyScalar(opt, "maxAtoms", "abc"));
  QCOMPARE(opt.getMaxAtoms(), 33);

  // Queue settings are not scalar settings.
  QVERIFY(!Settings::hasScalarBinding("queueInterface"));
  QVERIFY(!Settings::applyScalar(opt, "queueInterface", "none"));

  // Flags come from the table.
  QVERIFY(Settings::isRequired("optimizer"));
  QVERIFY(Settings::isRuntimeChangeable("aMin"));
  QVERIFY(!Settings::isRuntimeChangeable("usingRandSpg"));
  // Check all multi-line (repeatable) input values.
  QVERIFY(Settings::isRepeatableInput("molUnit"));
  QVERIFY(Settings::isRepeatableInput("customIAD"));
  QVERIFY(Settings::isRepeatableInput("objective"));
  QVERIFY(Settings::isRepeatableInput("constraint"));
  QVERIFY(Settings::isRepeatableInput("potcarFile"));
  QVERIFY(Settings::isRepeatableInput("psfFile"));
  QCOMPARE(Settings::findKeywordName("amin"), QString("aMin"));
  QCOMPARE(Settings::findKeywordName("molunit"), QString("molUnit"));
  QVERIFY(Settings::findKeywordName("molunit 2").isEmpty());
  QVERIFY(Settings::findKeywordName("noSuchKeyword").isEmpty());
}

void XtalOptUnitTest::structureStateFileIsStable()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  // Check the structure state file.
  std::unique_ptr<Xtal> xtal(makeTestXtal(3, 7, 4, 22, Common::Vector3(0.1, 0.2, 0.3), 0.0));
  Atoms::Atom& atom2 = xtal->addAtom();
  atom2.setAtomicNumber(8);
  atom2.setPos(Common::Vector3(0.5, 0.5, 0.5));

  xtal->setJobID(12345);
  xtal->setCurrentOptStep(1);
  xtal->setParents("parents string");
  xtal->setRempath("remote/path");
  xtal->setLocpath("local/path");
  xtal->setStatus(Structure::Optimized);
  xtal->setFailCount(1);
  xtal->setOptTimerStart(QDateTime(QDate(2026, 1, 2), QTime(3, 4, 5)));
  xtal->setOptTimerEnd(QDateTime(QDate(2026, 1, 2), QTime(6, 7, 8)));
  xtal->appendCopyFile("extra-file-one");
  xtal->appendCopyFile("extra-file-two");
  xtal->setEnergy(-12.5);
  xtal->setEnthalpy(-12.25);

  xtal->setStrucObjValuesVec(QList<double>() << 1.5 << 2.5);
  xtal->setStrucObjState(Structure::Os_Retain);
  xtal->setStrucConstraintValuesVec(QList<double>() << 0.25);
  xtal->setStrucConstraintState(Structure::Cs_Retain);

  // One history entry exercises the history sections deterministically.
  QList<unsigned int> histNums;
  histNums << 22 << 8;
  QList<Common::Vector3> histCoords;
  histCoords << Common::Vector3(0.0, 0.0, 0.0)
             << Common::Vector3(0.25, 0.25, 0.25);
  Common::Matrix3 histCell;
  histCell << 5.0, 0.0, 0.0, 0.0, 5.0, 0.0, 0.0, 0.0, 5.0;
  xtal->updateAndAddToHistory(histNums, histCoords, -10.0, -9.75, histCell);

  const QString statePath = tempPath(tempDir, "structure.state");
  writeStructureState(*xtal, statePath);

  const QString expectedDir = Common::localPath(QString(TESTDATADIR), "outputs");
  compareWithExpectedFile(statePath, Common::localPath(expectedDir, "structure.state"));
}

void XtalOptUnitTest::saveWritesOnlyChangedStructureStates()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  SessionTestXtalOpt opt;
  const int count = buildTestSearch(opt, tempDir.path());
  QVERIFY(count > 1);

  const QString stateFile = tempPath(tempDir, "xtalopt.state");
  QVERIFY(opt.save(stateFile));

  const QList<Structure*> structures = opt.queue()->getAllStructures();
  Structure* changed = structures.first();
  const QString changedState = Common::localPath(changed->getLocpath(), "structure.state");
  QVERIFY(QFile::exists(changedState));
  QVERIFY(!QFile::exists(changedState + ".old"));

  opt.setSessionActive(true);
  QVERIFY(QMetaObject::invokeMethod(opt.queue(), "structureUpdated",
                                    Qt::DirectConnection,
                                    Q_ARG(Search::Structure*, changed)));
  QTRY_VERIFY(QFile::exists(changedState + ".old"));
  opt.setSessionActive(false);

  for (int i = 1; i < structures.size(); ++i) {
    const QString unchangedState = Common::localPath(structures.at(i)->getLocpath(), "structure.state");
    QVERIFY(!QFile::exists(unchangedState + ".old"));
  }
}

void XtalOptUnitTest::resultsOutputsAreStable()
{
  // Check the results and hull files.
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  XtalOpt opt;
  opt.setLocWorkDir(tempDir.path());

  CellComp comp;
  comp.setCompositionEntry("Ti", 22, 1);
  comp.setCompositionEntry("O", 8, 2);
  opt.compList().append(comp);

  auto buildXtal = [](uint generation, uint id, int index, int nTi, int nO,
                      double enthalpy) -> Xtal* {
    Xtal* xtal = new Xtal(5.0, 5.0, 5.0, 90.0, 90.0, 90.0);
    for (int i = 0; i < nTi; ++i) {
      Atoms::Atom& atom = xtal->addAtom();
      atom.setAtomicNumber(22);
      atom.setPos(Common::Vector3(0.5 * i, 0.0, 0.0));
    }
    for (int i = 0; i < nO; ++i) {
      Atoms::Atom& atom = xtal->addAtom();
      atom.setAtomicNumber(8);
      atom.setPos(Common::Vector3(0.0, 0.5 * (i + 1), 0.0));
    }
    xtal->setGeneration(generation);
    xtal->setIDNumber(id);
    xtal->setIndex(index);
    xtal->setStatus(Xtal::Optimized);
    xtal->setEnthalpy(enthalpy);
    xtal->setChangedSinceSimChecked(false);
    return xtal;
  };

  // Add several compositions for a proper hull.
  QList<Structure*> structures;
  structures << buildXtal(1, 1, 0, 1, 0, -1.0)    // pure Ti
             << buildXtal(1, 2, 1, 0, 1, -0.5)    // pure O
             << buildXtal(1, 3, 2, 1, 2, -6.0)    // TiO2, on the hull
             << buildXtal(2, 1, 3, 1, 1, -3.0)    // TiO
             << buildXtal(2, 2, 4, 2, 1, -2.5)    // Ti2O
             << buildXtal(2, 3, 5, 1, 2, -4.0);   // TiO2, above the hull

  {
    // Block tracker signals.
    const bool wasBlocked = opt.tracker()->blockSignals(true);
    QWriteLocker trackerLocker(opt.tracker()->rwLock());
    for (auto* structure : structures)
      opt.tracker()->append(structure);
    opt.tracker()->blockSignals(wasBlocked);
  }

  QVERIFY(opt.refreshStructureEvaluationData());
  opt.refreshParentSelectionFronts(opt.queue()->getAllParentPoolStructures());
  QVERIFY(opt.writeResultsFile(opt.queue()->getAllStructures(), false));
  QVERIFY(opt.writeHullFile(opt.queue()->getAllStructures(), tempPath(tempDir, "hull.txt")));

  // Check the output files against the saved copies.
  const QString expectedDir = Common::localPath(QString(TESTDATADIR), "outputs");
  compareWithExpectedFile(tempPath(tempDir, "results.txt"),
                        Common::localPath(expectedDir, "results.txt"));
  compareWithExpectedFile(tempPath(tempDir, "hull.txt"),
                        Common::localPath(expectedDir, "hull.txt"));
}

void XtalOptUnitTest::readOnlySearchDoesNotSaveHullSnapshot()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  XtalOpt opt;
  opt.setLocWorkDir(tempDir.path());
  opt.setRunMode(XtalOpt::RunModeReadOnly);

  opt.queueHullSnapshot();
  QVERIFY(!QDir(Common::localPath(tempDir.path(), "movie")).exists());
}

void XtalOptUnitTest::objectiveSettingsValidation()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  // Read an objective value.
  {
    const QString stateFile = tempPath(tempDir, "objective-readwrite.state");
    writeObjectiveStateFixture(stateFile,
      QStringList() << "max /tmp/fake-objective objective.out 0.25");

    XtalOpt loaded;
    QVERIFY(loaded.readSettings(stateFile, true));

    QCOMPARE(loaded.getObjectivesNum(), 2);
    QCOMPARE(loaded.getObjectivesTyp(0), SearchBase::Ot_Min);
    QCOMPARE(loaded.getObjectivesWgt(0), 0.75);
    QCOMPARE(loaded.getObjectivesExe(0), QString());
    QCOMPARE(loaded.getObjectivesOut(0), QString());

    QCOMPARE(loaded.getObjectivesTyp(1), SearchBase::Ot_Max);
    QCOMPARE(loaded.getObjectivesWgt(1), 0.25);
    QCOMPARE(loaded.getObjectivesExe(1), QString("/tmp/fake-objective"));
    QCOMPARE(loaded.getObjectivesOut(1), QString("objective.out"));
  }

  // Reject bad objective weights.
  {
    const QString stateFile = tempPath(tempDir, "objective-invalid-weights.state");
    writeObjectiveStateFixture(stateFile,
      QStringList() << "min /tmp/objective-a a.out 0.75"
                    << "max /tmp/objective-b b.out 0.5");

    XtalOpt loaded;
    QVERIFY(!loaded.readSettings(stateFile, true));
  }

  // Reject an incomplete/failed objective.
  {
    const QString stateFile = tempPath(tempDir, "objective-missing-script.state");
    writeObjectiveStateFixture(stateFile,
      QStringList() << "min  objective.out 0.2");

    XtalOpt loaded;
    QVERIFY(!loaded.readSettings(stateFile, true));
  }
}

void XtalOptUnitTest::constraintInputSyntax()
{
  XtalOpt opt;
  QVERIFY(opt.processInputConstraint("filter filter.out"));

  QCOMPARE(opt.getConstraintsNum(), 1);
  QCOMPARE(opt.getConstraintExe(0), QString("filter"));
  QCOMPARE(opt.getConstraintOut(0), QString("filter.out"));
  QVERIFY(opt.needsObjectiveOrConstraintCalculations());

  QVERIFY(opt.removeConstraint(0));
  QVERIFY(!opt.needsObjectiveOrConstraintCalculations());
}

void XtalOptUnitTest::removeUserObjectivePreservesBuiltinObjective()
{
  XtalOpt opt;
  QVERIFY(opt.processInputObjectives("min /tmp/objective-a a.out 0.25"));
  QVERIFY(opt.processInputObjectives("max /tmp/objective-b b.out 0.25"));
  opt.refreshBuiltinObjectiveWeight();

  QCOMPARE(opt.getObjectivesNum(), 3);
  QCOMPARE(opt.getUserObjectivesNum(), 2);
  QCOMPARE(opt.getUserObjectiveIndex(0), 1);
  QCOMPARE(opt.getObjectivesWgt(0), 0.5);
  QVERIFY(opt.needsObjectiveOrConstraintCalculations());

  QVERIFY(!opt.removeUserObjective(0));
  QCOMPARE(opt.getObjectivesNum(), 3);

  QVERIFY(opt.removeUserObjective(1));
  QCOMPARE(opt.getObjectivesNum(), 2);
  QCOMPARE(opt.getUserObjectivesNum(), 1);
  QCOMPARE(opt.getObjectivesTyp(0), SearchBase::Ot_Min);
  QCOMPARE(opt.getObjectivesExe(0), QString());
  QCOMPARE(opt.getObjectivesOut(0), QString());
  QCOMPARE(opt.getObjectivesWgt(0), 0.75);
  QCOMPARE(opt.getObjectivesExe(1), QString("/tmp/objective-b"));

  QVERIFY(opt.removeUserObjective(1));
  QCOMPARE(opt.getObjectivesNum(), 1);
  QCOMPARE(opt.getUserObjectivesNum(), 0);
  QCOMPARE(opt.getObjectivesWgt(0), 1.0);
  QVERIFY(!opt.needsObjectiveOrConstraintCalculations());
}

void XtalOptUnitTest::runtimeOptionsApplyRuntimeChangeableKeys()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  XtalOpt opt;
  opt.setLocWorkDir(tempDir.path());

  QFile runtimeFile(opt.CLIRuntimeFile());

  // Read hardExit.
  QVERIFY(!opt.isHardExit());
  QVERIFY(runtimeFile.open(QIODevice::WriteOnly | QIODevice::Text));
  runtimeFile.write("hardExit = true\n");
  runtimeFile.close();

  opt.readRuntimeOptions();
  QVERIFY(opt.isHardExit());

  // Ignore invalid boolean text.
  QVERIFY(runtimeFile.open(QIODevice::WriteOnly | QIODevice::Text));
  runtimeFile.write("hardExit = maybe\n");
  runtimeFile.close();

  opt.readRuntimeOptions();
  QVERIFY(opt.isHardExit());

  // Read paretoFilterZeroWeights.
  QVERIFY(!opt.isParetoFilterZeroWeights());
  QVERIFY(runtimeFile.open(QIODevice::WriteOnly | QIODevice::Text));
  runtimeFile.write("paretoFilterZeroWeights = true\n");
  runtimeFile.close();

  opt.readRuntimeOptions();
  QVERIFY(opt.isParetoFilterZeroWeights());

  // Read constraintsReDo.
  QVERIFY(!opt.isConstraintsReDo());
  QVERIFY(runtimeFile.open(QIODevice::WriteOnly | QIODevice::Text));
  runtimeFile.write("constraintsReDo = true\n");
  runtimeFile.close();

  opt.readRuntimeOptions();
  QVERIFY(opt.isConstraintsReDo());

  // Read jobFailAction.
  opt.setFailAction(SearchBase::FA_Randomize);
  QVERIFY(runtimeFile.open(QIODevice::WriteOnly | QIODevice::Text));
  runtimeFile.write("jobFailAction = replaceWithOffspring\n");
  runtimeFile.close();

  opt.readRuntimeOptions();
  QCOMPARE(opt.getFailAction(), SearchBase::FA_NewOffspring);

  QVERIFY(runtimeFile.open(QIODevice::WriteOnly | QIODevice::Text));
  runtimeFile.write("jobFailAction = notARealAction\n");
  runtimeFile.close();

  opt.readRuntimeOptions();
  QCOMPARE(opt.getFailAction(), SearchBase::FA_NewOffspring);

  // Reset a setting value to default (with empty value).
  // maxNumStructures is a runtime-changeable scalar with default 100.
  opt.setMaxNumStructures(44);
  QCOMPARE(opt.getMaxNumStructures(), 44);

  // Clear the maximum count.
  opt.readRuntimeOptions("maxNumStructures =\n");
  QCOMPARE(opt.getMaxNumStructures(), 100);

  // Clear an empty value for an empty-default parameter.
  opt.readRuntimeOptions("elementalVolumes =\n");
  QVERIFY(opt.getInputEleVolmString().isEmpty());
}

void XtalOptUnitTest::processInputDataClearsCachesWhenInputsEmpty()
{
  XtalOpt opt;

  // Keep a valid forced-spacegroup list when a later value is invalid.
  QVERIFY(opt.setInputForcedSpgsString("1, 3-4, 4"));
  QCOMPARE(opt.minXtalsOfSpg().size(), 230);
  QCOMPARE(opt.minXtalsOfSpg().at(3), 2);
  QVERIFY(!opt.setInputForcedSpgsString("1, 5-3"));
  QCOMPARE(opt.getInputForcedSpgsString(), QString("1, 3-4, 4"));
  QCOMPARE(opt.minXtalsOfSpg().at(3), 2);

  // A composition (and its default reference energies) is built from raw text.
  opt.setInputFormulasString("Ti1O2");
  QVERIFY(opt.processInputData());
  QVERIFY(!opt.compList().isEmpty());

  // Clear the raw input text composition.
  opt.setInputFormulasString("");
  opt.processInputData();
  QVERIFY(opt.compList().isEmpty());
  QVERIFY(opt.refEnergies().isEmpty());
  QVERIFY(opt.eleVolumes().getVolumeAtomicNumbers().isEmpty());
}

void XtalOptUnitTest::optimizerAssetSyntaxReadAndWrite()
{
  // Check the optimizer input-file text used by the command line and GUI.

  // Read one "id file" line and reject an incomplete line.
  QString id, file;
  QVERIFY(Search::Optimizer::parseAssetIdFileLine("Ti /p/Ti/POTCAR", id, file));
  QCOMPARE(id, QString("Ti"));
  QCOMPARE(file, QString("/p/Ti/POTCAR"));
  QVERIFY(!Search::Optimizer::parseAssetIdFileLine("Ti", id, file));
  QVERIFY(!Search::Optimizer::parseAssetIdFileLine("", id, file));

  // Wrap a plain file name (%fileContents:%) keeping an input keyword.
  QCOMPARE(Search::Optimizer::inputAssetValueForSave("/p/POTCAR"), QString("%fileContents:/p/POTCAR%"));
  QCOMPARE(Search::Optimizer::inputAssetValueForSave("%copyFile:/p/POTCAR%"),
           QString("%copyFile:/p/POTCAR%"));

  // Save input files as a map and read editable lines back.
  QHash<QString, QString> assets;
  assets.insert("O", Search::Optimizer::inputAssetValueForSave("/p/O/POTCAR"));
  assets.insert("Ti", Search::Optimizer::inputAssetValueForSave("/p/Ti/POTCAR"));
  const QString parsedStr = Search::Optimizer::inputAssetFilesToText(assets);
  QCOMPARE(parsedStr, QString("O=%fileContents:/p/O/POTCAR%; " "Ti=%fileContents:/p/Ti/POTCAR%"));
  QCOMPARE(Search::Optimizer::inputAssetTextToFiles(parsedStr), QString("O /p/O/POTCAR\nTi /p/Ti/POTCAR"));

  // Keep plain input text.
  QCOMPARE(Search::Optimizer::inputAssetTextToFiles("just some literal text"),
           QString("just some literal text"));
}

void XtalOptUnitTest::writeOptionsFileCoversEveryScalarKeyword()
{
  XtalOpt opt;
  const QString output = exportedOptionsText(opt);
  QVERIFY(!output.isEmpty());

  // Export every scalar setting (except an empty seedStructures value).
  for (const auto& keyword : Settings::allKeywords()) {
    if (!Settings::hasScalarBinding(keyword) || keyword == "seedStructures")
      continue;
    QVERIFY2(output.contains("  " + keyword + " ="), qPrintable("not exported: " + keyword));
  }

  // Check the input values.
  const QStringList structured = { "chemicalFormulas", "queueInterface",
                                   "optimizer",        "referenceEnergies",
                                   "elementalVolumes", "numOptimizationSteps",
                                   "templatesDirectory", "directRunCommand" };
  for (const auto& keyword : structured) {
    QVERIFY2(output.contains("  " + keyword + " ="), qPrintable("not exported: " + keyword));
  }

  // Check the accepted input keywords.
  XtalOpt renamed;
  if (renamed.getNumOptSteps() == 0)
    renamed.appendOptStep();
  renamed.setOptimizer(0, "mtp");
  renamed.setUsername("remote-user");

  const QString renamedOutput = exportedOptionsText(renamed);
  QVERIFY(!renamedOutput.isEmpty());

  QVERIFY(renamedOutput.contains("  user = remote-user\n"));
  QVERIFY(renamedOutput.contains("  directRunCommand = "));
  QVERIFY(renamedOutput.contains("  constraintsReDo = "));
  QVERIFY(renamedOutput.contains("  paretoFilterZeroWeights = "));
  QVERIFY(renamedOutput.contains("  usingScaledIADs = "));
  QVERIFY(renamedOutput.contains("  mtpRelaxTemplates =\n"));
}

void XtalOptUnitTest::writeOptionsFileUsesCentralTemplateMap()
{
  const QHash<QString, QString> templateKeywordByFilename = {
    { "xtal.gin",   "ginTemplates"         },
    { "INCAR",      "incarTemplates"       },
    { "KPOINTS",    "kpointsTemplates"     },
    { "xtal.in",    "pwscfTemplates"       },
    { "xtal.cell",  "castepCellTemplates"  },
    { "xtal.param", "castepParamTemplates" },
    { "xtal.fdf",   "fdfTemplates"         },
    { "mtp.cfg",    "mtpCellTemplates"     },
    { "mtp.relax",  "mtpRelaxTemplates"    },
    { "mtp.pot",    "mtpPotTemplates"      }
  };

  const QHash<QString, QString> assetKeywordByName = {
    { "POTCAR", "potcarFile" },
    { "PSF",    "psfFile"    }
  };

  for (const auto& optimizerName : Optimizer::availableBuiltInOptimizers()) {
    XtalOpt opt;
    opt.appendOptStep();
    QVERIFY2(opt.setOptimizer(0, optimizerName.toStdString()),
             qPrintable(QString("Could not create optimizer ") + optimizerName));
    const Optimizer* optimizer = opt.optimizer(0);
    QVERIFY(optimizer != nullptr);

    const QString output = exportedOptionsText(opt);
    QVERIFY(!output.isEmpty());

    for (const auto& filename : optimizer->getOptimizerTemplateFileNames()) {
      QVERIFY2(templateKeywordByFilename.contains(filename), qPrintable(optimizerName +
                          " template has no option keyword: " + filename));
      const QString keyword = templateKeywordByFilename.value(filename);
      QVERIFY2(output.contains("  " + keyword + " =\n"),
               qPrintable(optimizerName + " export omitted " + keyword));
    }

    for (const auto& assetName : optimizer->getOptimizerInputAssetNames()) {
      QVERIFY2(assetKeywordByName.contains(assetName), qPrintable(optimizerName +
                          " asset has no option keyword: " + assetName));
      const QString keyword = assetKeywordByName.value(assetName);
      QVERIFY2(output.contains("  " + keyword + " =\n"),
               qPrintable(optimizerName + " export omitted " + keyword));
    }
  }

  for (const auto& queueName : QueueInterface::availableBuiltInQueueInterfaces()) {
    XtalOpt opt;
    QVERIFY2(opt.setQueueInterface(0, queueName.toStdString()),
             qPrintable(QString("Could not create queue interface ") + queueName));
    const QueueInterface* queue = opt.queueInterface(0);
    QVERIFY(queue != nullptr);

    const QString output = exportedOptionsText(opt);
    QVERIFY(!output.isEmpty());

    const QStringList filenames = queue->getQueueInterfaceTemplateFileNames();
    if (filenames.isEmpty()) {
      QVERIFY2(!output.contains("  jobTemplates ="), qPrintable(queueName +
                          " export included unexpected jobTemplates"));
    } else {
      QVERIFY2(filenames.size() == 1, qPrintable(queueName + " defines multiple queue templates"));
      QVERIFY2(output.contains("  jobTemplates =\n"),
               qPrintable(queueName + " export omitted jobTemplates"));
    }
  }
}

void XtalOptUnitTest::readOptionsAppliesFailActionPolicy()
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
  input << "jobFailAction = keepTrying\n";
  input << "templatesDirectory = " << tempDir.path() << "\n";
  input << "ginTemplates = xtal.gin\n";
  input << "localWorkingDirectory = " << tempPath(tempDir, "run") << "\n";
  inputFile.close();

  XtalOpt opt;
  QVERIFY(opt.readInputFile(inputFile.fileName(), true));
  QCOMPARE(opt.getFailAction(), SearchBase::FA_DoNothing);
}

void XtalOptUnitTest::readOptionsAppliesScalarSettingRegistry()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  QFile optimizerTemplate(tempPath(tempDir, "xtal.gin"));
  QVERIFY(optimizerTemplate.open(QIODevice::WriteOnly | QIODevice::Text));
  optimizerTemplate.write("# gulp template\n");
  optimizerTemplate.close();

  QFile queueTemplate(tempPath(tempDir, "job.pbs.in"));
  QVERIFY(queueTemplate.open(QIODevice::WriteOnly | QIODevice::Text));
  queueTemplate.write("# pbs template\n");
  queueTemplate.close();

  QFile inputFile(tempPath(tempDir, "xtalopt.in"));
  QVERIFY(inputFile.open(QIODevice::WriteOnly | QIODevice::Text));
  QTextStream input(&inputFile);
  input << "chemicalFormulas = O1\n";
  input << "queueInterface = pbs\n";
  input << "remoteQueue = false\n";
  input << "optimizer = gulp\n";
  input << "templatesDirectory = " << tempDir.path() << "\n";
  input << "ginTemplates = xtal.gin\n";
  input << "jobTemplates = job.pbs.in\n";
  input << "localWorkingDirectory = " << tempPath(tempDir, "run") << "\n";
  input << "verboseOutput = true\n";
  input << "saveHullSnapshots = true\n";
  input << "parentsPoolSize = 33\n";
  input << "limitRunningJobs = false\n";
  input << "runningJobLimit = 7\n";
  input << "continuousStructures = 9\n";
  input << "maxNumStructures = 44\n";
  input << "softExit = true\n";
  input << "constraintsReDo = true\n";
  input << "optimizationType = pareto\n";
  input << "tournamentSelection = false\n";
  input << "restrictedPool = true\n";
  input << "crowdingDistance = false\n";
  input << "paretoFilterZeroWeights = true\n";
  input << "objectivePrecision = 5\n";
  input << "weightCrossover = 31\n";
  input << "weightStripple = 17\n";
  input << "weightPermustrain = 19\n";
  input << "weightPermutomic = 23\n";
  input << "weightPermucomp = 29\n";
  input << "randomSuperCell = 3\n";
  input << "strippleAmplitudeMin = 0.2\n";
  input << "strippleAmplitudeMax = 0.8\n";
  input << "strippleNumWavesAxis1 = 2\n";
  input << "strippleNumWavesAxis2 = 4\n";
  input << "strippleStrainStdevMin = 0.3\n";
  input << "strippleStrainStdevMax = 0.9\n";
  input << "permustrainNumExchanges = 6\n";
  input << "permustrainStrainStdevMax = 0.7\n";
  input << "crossoverCuts = 2\n";
  input << "crossoverMinContribution = 35\n";
  input << "rdfTolerance = 0.4\n";
  input << "rdfCutoff = 7.5\n";
  input << "rdfNumBins = 1024\n";
  input << "rdfSigma = 0.02\n";
  input << "xtalcompToleranceLength = 0.12\n";
  input << "xtalcompToleranceAngle = 1.5\n";
  input << "spglibTolerance = 0.03\n";
  input << "user1 = alpha\n";
  input << "user2 = beta\n";
  input << "user3 = gamma\n";
  input << "user4 = delta\n";
  input << "logErrorDirectories = true\n";
  input << "autoCancelJobAfterTime = true\n";
  input << "hoursForAutoCancelJob = 12.5\n";
  input << "queueRefreshInterval = 11\n";
  inputFile.close();

  XtalOpt opt;
  QVERIFY(opt.readInputFile(inputFile.fileName(), true));
  QVERIFY(opt.isVerbose());
  QVERIFY(opt.getSaveHullSnapshots());
  QCOMPARE(opt.getParentsPoolSize(), static_cast<uint>(33));
  QVERIFY(!opt.isLimitRunningJobs());
  QCOMPARE(opt.getRunningJobLimit(), static_cast<uint>(7));
  QCOMPARE(opt.getContStructs(), static_cast<uint>(9));
  QCOMPARE(opt.getMaxNumStructures(), 44);
  QVERIFY(!opt.isSoftExit());
  QVERIFY(opt.isConstraintsReDo());
  QCOMPARE(opt.getOptimizationType(), QString("pareto"));
  QVERIFY(!opt.isTournamentSelection());
  QVERIFY(opt.isRestrictedPool());
  QVERIFY(!opt.isCrowdingDistance());
  QVERIFY(opt.isParetoFilterZeroWeights());
  QCOMPARE(opt.getObjectivePrecision(), 5);
  QCOMPARE(opt.getPCross(), static_cast<uint>(31));
  QCOMPARE(opt.getPStrip(), static_cast<uint>(17));
  QCOMPARE(opt.getPPerm(), static_cast<uint>(19));
  QCOMPARE(opt.getPAtomic(), static_cast<uint>(23));
  QCOMPARE(opt.getPComp(), static_cast<uint>(29));
  QCOMPARE(opt.getPSupercell(), 3.0);
  QVERIFY(fabs(opt.getStripAmpMin() - 0.2) < 1e-6);
  QVERIFY(fabs(opt.getStripAmpMax() - 0.8) < 1e-6);
  QCOMPARE(opt.getStripPer1(), static_cast<uint>(2));
  QCOMPARE(opt.getStripPer2(), static_cast<uint>(4));
  QVERIFY(fabs(opt.getStripStrainStdevMin() - 0.3) < 1e-6);
  QVERIFY(fabs(opt.getStripStrainStdevMax() - 0.9) < 1e-6);
  QCOMPARE(opt.getPermEx(), static_cast<uint>(6));
  QVERIFY(fabs(opt.getPermStrainStdevMax() - 0.7) < 1e-6);
  QCOMPARE(opt.getCrossNcuts(), static_cast<uint>(2));
  QCOMPARE(opt.getCrossMinimumContribution(), static_cast<uint>(35));
  QCOMPARE(opt.getTolRdf(), 0.4);
  QCOMPARE(opt.getTolRdfCutoff(), 7.5);
  QCOMPARE(opt.getTolRdfNbins(), 1024);
  QCOMPARE(opt.getTolRdfSigma(), 0.02);
  QVERIFY(fabs(opt.getTolXcLength() - 0.12) < 1e-6);
  QVERIFY(fabs(opt.getTolXcAngle() - 1.5) < 1e-6);
  QVERIFY(fabs(opt.getTolSpg() - 0.03) < 1e-6);
  QCOMPARE(opt.getUser1(), QString("alpha"));
  QCOMPARE(opt.getUser2(), QString("beta"));
  QCOMPARE(opt.getUser3(), QString("gamma"));
  QCOMPARE(opt.getUser4(), QString("delta"));
  QVERIFY(opt.logErrorDirs());
  QVERIFY(opt.cancelJobAfterTime());
  QCOMPARE(opt.hoursForCancelJobAfterTime(), 12.5);
  QCOMPARE(opt.queueRefreshInterval(), 11);
}

void XtalOptUnitTest::readOptionsAppliesCentralTemplateMap()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  QFile cellTemplate(tempPath(tempDir, "cell.in"));
  QVERIFY(cellTemplate.open(QIODevice::WriteOnly | QIODevice::Text));
  cellTemplate.write("cell template\n");
  cellTemplate.close();

  QFile relaxTemplate(tempPath(tempDir, "relax.in"));
  QVERIFY(relaxTemplate.open(QIODevice::WriteOnly | QIODevice::Text));
  relaxTemplate.write("relax template\n");
  relaxTemplate.close();

  QFile potTemplate(tempPath(tempDir, "pot.in"));
  QVERIFY(potTemplate.open(QIODevice::WriteOnly | QIODevice::Text));
  potTemplate.write("pot template\n");
  potTemplate.close();

  QFile inputFile(tempPath(tempDir, "xtalopt.in"));
  QVERIFY(inputFile.open(QIODevice::WriteOnly | QIODevice::Text));
  QTextStream input(&inputFile);
  input << "chemicalFormulas = O1\n";
  input << "queueInterface = none\n";
  input << "optimizer = mtp\n";
  input << "templatesDirectory = " << tempDir.path() << "\n";
  input << "mtpCellTemplates = cell.in\n";
  input << "mtpRelaxTemplates = relax.in\n";
  input << "mtpPotTemplates = pot.in\n";
  input << "localWorkingDirectory = " << tempPath(tempDir, "run") << "\n";
  inputFile.close();

  XtalOpt opt;
  QVERIFY(opt.readInputFile(inputFile.fileName(), true));
  QCOMPARE(QString::fromStdString(opt.getOptimizerTemplate(0, "mtp.cfg")),
           QString("cell template\n"));
  QCOMPARE(QString::fromStdString(opt.getOptimizerTemplate(0, "mtp.relax")),
           QString("relax template\n"));
  QCOMPARE(QString::fromStdString(opt.getOptimizerTemplate(0, "mtp.pot")),
           QString("pot template\n"));
}

void XtalOptUnitTest::importReadResolvesInputsWithoutRunArtifacts()
{
  // Read a "best-effort" imported input file.
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
    input << "chemicalFormulas = O1\n";
    input << "queueInterface = none\n";
    input << "optimizer = gulp\n";
    input << "templatesDirectory = " << tempDir.path() << "\n";
    input << "ginTemplates = xtal.gin\n";
    input << "localWorkingDirectory = " << runDir << "\n";
    inputFile.close();

    XtalOpt opt;
    QVERIFY(opt.readInputFile(inputFile.fileName(), true));
    QCOMPARE(opt.getLocWorkDir(), QDir(runDir).absolutePath());
    QVERIFY(!QFile::exists(runDir));
    QVERIFY(!QFile::exists(opt.CLIRuntimeFile()));
  }

  // Keep a relative local work directory.
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
    input << "localWorkingDirectory = ./local\n";
    inputFile.close();

    XtalOpt opt;
    QVERIFY(opt.readInputFile(inputFile.fileName(), true));
    QCOMPARE(opt.getLocWorkDir(), QString("./local"));
  }

  // Read template files (they should be resolved agains file's directory).
  {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QFile fragmentFile(tempPath(tempDir, "fragment.txt"));
    QVERIFY(fragmentFile.open(QIODevice::WriteOnly | QIODevice::Text));
    fragmentFile.write("included text\n");
    fragmentFile.close();

    QFile copyFile(tempPath(tempDir, "copy.dat"));
    QVERIFY(copyFile.open(QIODevice::WriteOnly | QIODevice::Text));
    copyFile.write("copy me\n");
    copyFile.close();

    QFile templateFile(tempPath(tempDir, "xtal.gin"));
    QVERIFY(templateFile.open(QIODevice::WriteOnly | QIODevice::Text));
    templateFile.write("%fileContents:fragment.txt%\n%copyFile:copy.dat%\n");
    templateFile.close();

    QFile inputFile(tempPath(tempDir, "xtalopt.in"));
    QVERIFY(inputFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream input(&inputFile);
    input << "chemicalFormulas = O1\n";
    input << "queueInterface = none\n";
    input << "optimizer = gulp\n";
    input << "templatesDirectory = .\n";
    input << "ginTemplates = xtal.gin\n";
    input << "localWorkingDirectory = ./local\n";
    inputFile.close();

    XtalOpt opt;
    QVERIFY(opt.readInputFile(inputFile.fileName(), true));

    const QString text = QString::fromStdString(opt.getOptimizerTemplate(0, "xtal.gin"));
    QVERIFY(text.contains(QFileInfo(fragmentFile.fileName()).absoluteFilePath()));
    QVERIFY(text.contains(QFileInfo(copyFile.fileName()).absoluteFilePath()));
  }

  // Read support files (eg, potcar): resolved to the input file's directory.
  {
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

    QFile potcarFile(tempPath(tempDir, "POTCAR_O"));
    QVERIFY(potcarFile.open(QIODevice::WriteOnly | QIODevice::Text));
    potcarFile.write("potcar\n");
    potcarFile.close();

    QFile inputFile(tempPath(tempDir, "xtalopt.in"));
    QVERIFY(inputFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream input(&inputFile);
    input << "chemicalFormulas = O1\n";
    input << "queueInterface = none\n";
    input << "optimizer = vasp\n";
    input << "templatesDirectory = .\n";
    input << "incarTemplates = INCAR\n";
    input << "kpointsTemplates = KPOINTS\n";
    input << "potcarFile = O POTCAR_O\n";
    input << "localWorkingDirectory = ./local\n";
    inputFile.close();

    XtalOpt opt;
    QVERIFY(opt.readInputFile(inputFile.fileName(), true));

    const QString text = QString::fromStdString(opt.getOptimizerInputAsset(0, "POTCAR"));
    QVERIFY(text.contains(QFileInfo(potcarFile.fileName()).absoluteFilePath()));
    QVERIFY(!text.contains("%fileContents:POTCAR_O%"));
  }
}

void XtalOptUnitTest::readOptionsResolvesFromProcessCwdWithoutRunArtifacts()
{
  // Read the working directory: a relative path should be resolved against process work directory.
  {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString processCwd = tempPath(tempDir, "process-cwd");
    const QString inputDir = tempPath(tempDir, "input-dir");
    QVERIFY(QDir().mkpath(processCwd));
    QVERIFY(QDir().mkpath(inputDir));

    QFile templateFile(Common::localPath(processCwd, "xtal.gin"));
    QVERIFY(templateFile.open(QIODevice::WriteOnly | QIODevice::Text));
    templateFile.write("# gulp template from process cwd\n");
    templateFile.close();

    QFile inputFile(Common::localPath(inputDir, "xtalopt.in"));
    QVERIFY(inputFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream input(&inputFile);
    input << "chemicalFormulas = O1\n";
    input << "queueInterface = none\n";
    input << "optimizer = gulp\n";
    input << "templatesDirectory = .\n";
    input << "ginTemplates = xtal.gin\n";
    input << "localWorkingDirectory = ./run\n";
    inputFile.close();

    CurrentPathGuard currentPath(processCwd);
    QVERIFY(currentPath.changed());

    XtalOpt opt;
    QVERIFY(opt.readInputFile(inputFile.fileName(), false));
    QCOMPARE(opt.getLocWorkDir(), QDir(processCwd).absoluteFilePath("run"));
  }

  // Read input files: assets and templates are resolved against process work directory.
  {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString processCwd = tempPath(tempDir, "process-cwd");
    const QString inputDir = tempPath(tempDir, "input-dir");
    QVERIFY(QDir().mkpath(processCwd));
    QVERIFY(QDir().mkpath(inputDir));

    QFile fragmentFile(Common::localPath(processCwd, "fragment.txt"));
    QVERIFY(fragmentFile.open(QIODevice::WriteOnly | QIODevice::Text));
    fragmentFile.write("included text\n");
    fragmentFile.close();

    QFile copyFile(Common::localPath(processCwd, "copy.dat"));
    QVERIFY(copyFile.open(QIODevice::WriteOnly | QIODevice::Text));
    copyFile.write("copy me\n");
    copyFile.close();

    QFile templateFile(Common::localPath(processCwd, "xtal.gin"));
    QVERIFY(templateFile.open(QIODevice::WriteOnly | QIODevice::Text));
    templateFile.write("%fileContents:fragment.txt%\n%copyFile:copy.dat%\n");
    templateFile.close();

    QFile inputFile(Common::localPath(inputDir, "xtalopt.in"));
    QVERIFY(inputFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream input(&inputFile);
    input << "chemicalFormulas = O1\n";
    input << "queueInterface = none\n";
    input << "optimizer = gulp\n";
    input << "templatesDirectory = .\n";
    input << "ginTemplates = xtal.gin\n";
    input << "localWorkingDirectory = ./run\n";
    inputFile.close();

    CurrentPathGuard currentPath(processCwd);
    QVERIFY(currentPath.changed());

    XtalOpt opt;
    QVERIFY(opt.readInputFile(inputFile.fileName(), false));

    const QString text = QString::fromStdString(opt.getOptimizerTemplate(0, "xtal.gin"));
    QVERIFY(text.contains(QFileInfo(fragmentFile.fileName()).absoluteFilePath()));
    QVERIFY(text.contains(QFileInfo(copyFile.fileName()).absoluteFilePath()));
  }

  // A full read doen't start the search.
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
    input << "chemicalFormulas = O1\n";
    input << "queueInterface = none\n";
    input << "optimizer = gulp\n";
    input << "templatesDirectory = " << tempDir.path() << "\n";
    input << "ginTemplates = xtal.gin\n";
    input << "localWorkingDirectory = " << runDir << "\n";
    inputFile.close();

    XtalOpt opt;
    QVERIFY(opt.readInputFile(inputFile.fileName(), false));
    QCOMPARE(opt.getLocWorkDir(), QDir(runDir).absolutePath());
    QVERIFY(!QFile::exists(runDir));
    QVERIFY(!QFile::exists(opt.CLIRuntimeFile()));
  }
}

void XtalOptUnitTest::optimizerInputAssetsAreLiteral()
{
  // Write a POTCAR file (should not be interpreted).
  {
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

    QFile potcarFile(tempPath(tempDir, "POTCAR"));
    QVERIFY(potcarFile.open(QIODevice::WriteOnly | QIODevice::Text));
    potcarFile.write("literal %description% content\n");
    potcarFile.close();

    QFile inputFile(tempPath(tempDir, "xtalopt.in"));
    QVERIFY(inputFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream input(&inputFile);
    input << "chemicalFormulas = O1\n";
    input << "description = replaced-description\n";
    input << "queueInterface = none\n";
    input << "optimizer = vasp\n";
    input << "templatesDirectory = .\n";
    input << "incarTemplates = INCAR\n";
    input << "kpointsTemplates = KPOINTS\n";
    input << "potcarFile = system " << potcarFile.fileName() << "\n";
    input << "localWorkingDirectory = ./local\n";
    inputFile.close();

    XtalOpt opt;
    QVERIFY(opt.readInputFile(inputFile.fileName(), true));
    QVERIFY(!opt.optimizer(0)->getOptimizerTemplateFileNames().contains("POTCAR"));
    QVERIFY(opt.optimizer(0)->getOptimizerInputAssetNames().contains("POTCAR"));

    Xtal* xtal = makeTestXtal(1, 1, 0, 8, Common::Vector3(0.0, 0.0, 0.0), 0.0);
    xtal->setCurrentOptStep(0);
    xtal->setLocpath(tempDir.path());
    QHash<QString, QString> files = opt.optimizer(0)->getInputFiles(xtal);
    QCOMPARE(files.value("POTCAR"), QString("literal %description% content"));
    delete xtal;
  }

  // Write a PSF file (should not be interpreted).
  {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QFile fdfFile(tempPath(tempDir, "xtal.fdf"));
    QVERIFY(fdfFile.open(QIODevice::WriteOnly | QIODevice::Text));
    fdfFile.write("# fdf\n");
    fdfFile.close();

    QFile psfFile(tempPath(tempDir, "O.psf"));
    QVERIFY(psfFile.open(QIODevice::WriteOnly | QIODevice::Text));
    psfFile.write("psf %description% content\n");
    psfFile.close();

    QFile inputFile(tempPath(tempDir, "xtalopt.in"));
    QVERIFY(inputFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream input(&inputFile);
    input << "chemicalFormulas = O1\n";
    input << "description = replaced-description\n";
    input << "queueInterface = none\n";
    input << "optimizer = siesta\n";
    input << "templatesDirectory = .\n";
    input << "fdfTemplates = xtal.fdf\n";
    input << "psfFile = O O.psf\n";
    input << "localWorkingDirectory = ./local\n";
    inputFile.close();

    XtalOpt opt;
    QVERIFY(opt.readInputFile(inputFile.fileName(), true));
    QVERIFY(opt.optimizer(0)->getOptimizerInputAssetNames().contains("PSF"));

    Xtal* xtal = makeTestXtal(1, 1, 0, 8, Common::Vector3(0.0, 0.0, 0.0), 0.0);
    xtal->setCurrentOptStep(0);
    xtal->setLocpath(tempDir.path());
    QHash<QString, QString> files = opt.optimizer(0)->getInputFiles(xtal);
    QCOMPARE(files.value("O.psf"), QString("psf %description% content"));
    delete xtal;
  }
}

void XtalOptUnitTest::startSearchPrechecksInputFiles()
{
  // Check a missing seed structure.
  {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QFile templateFile(tempPath(tempDir, "xtal.gin"));
    QVERIFY(templateFile.open(QIODevice::WriteOnly | QIODevice::Text));
    templateFile.write("# gulp template\n");
    templateFile.close();

    const QString runDir = tempPath(tempDir, "run");
    const QString missingSeed = tempPath(tempDir, "missing.cif");
    QFile inputFile(tempPath(tempDir, "xtalopt.in"));
    QVERIFY(inputFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream input(&inputFile);
    input << "chemicalFormulas = O1\n";
    input << "queueInterface = none\n";
    input << "optimizer = gulp\n";
    input << "templatesDirectory = " << tempDir.path() << "\n";
    input << "ginTemplates = xtal.gin\n";
    input << "seedStructures = " << missingSeed << "\n";
    input << "localWorkingDirectory = " << runDir << "\n";
    inputFile.close();

    XtalOpt opt;
    QVERIFY(opt.readInputFile(inputFile.fileName(), false));

    QSignalSpy errorDialogSpy(&opt, SIGNAL(errorDialogRequested(QString)));
    QVERIFY(!opt.startSearch());
    QCOMPARE(errorDialogSpy.count(), 1);
    const QString error = errorDialogSpy.takeFirst().at(0).toString();
    QVERIFY(error.contains("Seed structure"));
    QVERIFY(error.contains(missingSeed));
    QVERIFY(!QFile::exists(runDir));
  }

  // Check a missing input asset/template file.
  {
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

    QFile potcarFile(tempPath(tempDir, "POTCAR"));
    QVERIFY(potcarFile.open(QIODevice::WriteOnly | QIODevice::Text));
    potcarFile.write("potcar\n");
    potcarFile.close();

    const QString runDir = tempPath(tempDir, "run");
    const QString missingPotcar = tempPath(tempDir, "missing-POTCAR");
    QFile inputFile(tempPath(tempDir, "xtalopt.in"));
    QVERIFY(inputFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream input(&inputFile);
    input << "chemicalFormulas = O1\n";
    input << "queueInterface = none\n";
    input << "optimizer = vasp\n";
    input << "templatesDirectory = " << tempDir.path() << "\n";
    input << "incarTemplates = INCAR\n";
    input << "kpointsTemplates = KPOINTS\n";
    input << "potcarFile = system " << potcarFile.fileName() << "\n";
    input << "localWorkingDirectory = " << runDir << "\n";
    inputFile.close();

    XtalOpt opt;
    QVERIFY(opt.readInputFile(inputFile.fileName(), false));
    opt.setOptimizerInputAsset(0, "POTCAR",
      ("system=%fileContents:" + missingPotcar + "%").toStdString());

    QSignalSpy errorDialogSpy(&opt, SIGNAL(errorDialogRequested(QString)));
    QVERIFY(!opt.startSearch());
    QCOMPARE(errorDialogSpy.count(), 1);
    const QString error = errorDialogSpy.takeFirst().at(0).toString();
    QVERIFY(error.contains("optimizer input asset POTCAR"));
    QVERIFY(error.contains(missingPotcar));
    QVERIFY(!QFile::exists(runDir));
  }
}

void XtalOptUnitTest::readOptionsLocalSchedulerConfiguration()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  // Reject a remote direct run for a "direct run" (queue = none).
  {
    QFile templateFile(tempPath(tempDir, "xtal.gin"));
    QVERIFY(templateFile.open(QIODevice::WriteOnly | QIODevice::Text));
    templateFile.write("# gulp template\n");
    templateFile.close();

    QFile inputFile(tempPath(tempDir, "xtalopt.in"));
    QVERIFY(inputFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream input(&inputFile);
    input << "chemicalFormulas = O1\n";
    input << "queueInterface = none\n";
    input << "remoteQueue = true\n";
    input << "optimizer = gulp\n";
    input << "templatesDirectory = " << tempDir.path() << "\n";
    input << "ginTemplates = xtal.gin\n";
    input << "localWorkingDirectory = " << tempPath(tempDir, "run") << "\n";
    inputFile.close();

    XtalOpt opt;
    QVERIFY(!opt.readInputFile(inputFile.fileName(), false));
  }

  // Read local queue settings.
  QFile optimizerTemplate(tempPath(tempDir, "xtal.gin"));
  QVERIFY(optimizerTemplate.open(QIODevice::WriteOnly | QIODevice::Text));
  optimizerTemplate.write("# gulp template\n");
  optimizerTemplate.close();

  for (const auto& queueName : QueueInterface::availableBuiltInQueueInterfaces()) {
    XtalOpt reference;
    QVERIFY(reference.setQueueInterface(0, queueName.toStdString()));
    const QueueInterface* queue = reference.queueInterface(0);
    QVERIFY(queue != nullptr);
    const QStringList engineTemplateNames = queue->getQueueInterfaceTemplateFileNames();
    if (engineTemplateNames.isEmpty())
      continue;
    QCOMPARE(engineTemplateNames.size(), 1);

    const QString templateInputName = "job-" + queueName + ".in";
    const QString templateText = "# " + queueName + " job template\n";
    QFile queueTemplate(tempPath(tempDir, templateInputName));
    QVERIFY(queueTemplate.open(QIODevice::WriteOnly | QIODevice::Text));
    queueTemplate.write(templateText.toLocal8Bit());
    queueTemplate.close();

    QFile inputFile(tempPath(tempDir, "xtalopt-" + queueName + ".in"));
    QVERIFY(inputFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream input(&inputFile);
    input << "chemicalFormulas = O1\n";
    input << "queueInterface = " << queueName << "\n";
    input << "remoteQueue = false\n";
    input << "optimizer = gulp\n";
    input << "templatesDirectory = " << tempDir.path() << "\n";
    input << "ginTemplates = xtal.gin\n";
    input << "jobTemplates = " << templateInputName << "\n";
    input << "localWorkingDirectory = " << tempPath(tempDir, "run") << "\n";
    inputFile.close();

    XtalOpt opt;
    QVERIFY2(opt.readInputFile(inputFile.fileName(), true),
             qPrintable("Failed to read queue interface " + queueName));
    QCOMPARE(opt.queueInterface(0)->getIDString().toLower(), queueName.toLower());
    QVERIFY(!opt.isRemoteQueue());
    QCOMPARE(QString::fromStdString(opt.getQueueInterfaceTemplate(
                 0, engineTemplateNames.first().toStdString())), templateText);
  }
}

void XtalOptUnitTest::readOptionsRemoteQueueConfiguration()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  QFile optimizerTemplate(tempPath(tempDir, "xtal.gin"));
  QVERIFY(optimizerTemplate.open(QIODevice::WriteOnly | QIODevice::Text));
  optimizerTemplate.write("# gulp template\n");
  optimizerTemplate.close();

  // Read remote queue settings.
  {
    QFile queueTemplate(tempPath(tempDir, "job.pbs.in"));
    QVERIFY(queueTemplate.open(QIODevice::WriteOnly | QIODevice::Text));
    queueTemplate.write("# remote pbs job template\n");
    queueTemplate.close();

    QFile inputFile(tempPath(tempDir, "xtalopt.in"));
    QVERIFY(inputFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream input(&inputFile);
    input << "chemicalFormulas = O1\n";
    input << "queueInterface = pbs\n";
    input << "remoteQueue = true\n";
    input << "optimizer = gulp\n";
    input << "templatesDirectory = " << tempDir.path() << "\n";
    input << "ginTemplates = xtal.gin\n";
    input << "jobTemplates = job.pbs.in\n";
    input << "localWorkingDirectory = " << tempPath(tempDir, "run") << "\n";
    input << "remoteWorkingDirectory = /remote/xtalopt\n";
    input << "host = scheduler.example.invalid\n";
    input << "user = remote-user\n";
    inputFile.close();

    XtalOpt opt;
    QVERIFY(opt.readInputFile(inputFile.fileName(), true));
    QCOMPARE(opt.queueInterface(0)->getIDString().toLower(), QString("pbs"));
    QVERIFY(opt.isRemoteQueue());
    QCOMPARE(QString::fromStdString(opt.getQueueInterfaceTemplate(0, "job.pbs")),
             QString("# remote pbs job template\n"));
  }

  // Read the SSH setting.
  {
    QFile inputFile(tempPath(tempDir, "xtalopt.in"));
    QVERIFY(inputFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream input(&inputFile);
    input << "chemicalFormulas = O1\n";
    input << "queueInterface = none\n";
    input << "remoteQueue = false\n";
    input << "sshMethod = auto\n";
    input << "optimizer = gulp\n";
    input << "templatesDirectory = " << tempDir.path() << "\n";
    input << "ginTemplates = xtal.gin\n";
    input << "localWorkingDirectory = " << tempPath(tempDir, "run") << "\n";
    inputFile.close();

    XtalOpt opt;
    QVERIFY(opt.readInputFile(inputFile.fileName(), true));
    QCOMPARE(opt.sshMethod(), QString("auto"));
  }
}

void XtalOptUnitTest::initialRuntimeFileContainsOnlyRuntimeSubset()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  XtalOpt opt;
  opt.setLocWorkDir(tempDir.path());
  opt.writeInitialRuntimeFile();

  QFile runtimeFile(opt.CLIRuntimeFile());
  QVERIFY(runtimeFile.open(QIODevice::ReadOnly | QIODevice::Text));
  const QString runtimeText = QString::fromUtf8(runtimeFile.readAll());

  QVERIFY(runtimeText.contains("queueRefreshInterval = "));
  QVERIFY(runtimeText.contains("softExit = "));
  QVERIFY(runtimeText.contains("hardExit = "));
  QVERIFY(runtimeText.contains("usingScaledIADs = "));
  QVERIFY(runtimeText.contains("paretoFilterZeroWeights = "));
  QVERIFY(!runtimeText.contains("queueInterface = "));
  QVERIFY(!runtimeText.contains("remoteQueue = "));
  QVERIFY(!runtimeText.contains("optimizer = "));
  QVERIFY(!runtimeText.contains("templatesDirectory = "));
  QVERIFY(!runtimeText.contains("localWorkingDirectory = "));
}

void XtalOptUnitTest::runtimeOptionsDoNotChangeFixedKeys()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  // Keep queue settings (not runtime adjustable).
  {
    XtalOpt opt;
    opt.setLocWorkDir(tempDir.path());
    opt.setRemoteQueue(false);

    QFile runtimeFile(opt.CLIRuntimeFile());
    QVERIFY(runtimeFile.open(QIODevice::WriteOnly | QIODevice::Text));
    runtimeFile.write("remoteQueue = true\n");
    runtimeFile.write("queueInterface = pbs\n");
    runtimeFile.close();

    opt.readRuntimeOptions();
    QVERIFY(!opt.isRemoteQueue());
  }

  // Keep the working directory (not runtime adjustable).
  {
    const QString originalRunDir = tempPath(tempDir, "original-run");
    const QString attemptedRunDir = tempPath(tempDir, "attempted-run");

    XtalOpt opt;
    opt.setLocWorkDir(originalRunDir);

    QFile runtimeFile(opt.CLIRuntimeFile());
    QVERIFY(QDir().mkpath(originalRunDir));
    QVERIFY(runtimeFile.open(QIODevice::WriteOnly | QIODevice::Text));
    runtimeFile.write("localWorkingDirectory = ");
    runtimeFile.write(attemptedRunDir.toLocal8Bit());
    runtimeFile.write("\n");
    runtimeFile.write("hardExit = true\n");
    runtimeFile.close();

    opt.readRuntimeOptions();
    QCOMPARE(opt.getLocWorkDir(), originalRunDir);
    QVERIFY(opt.isHardExit());
  }
}

void XtalOptUnitTest::genericSettingsDoNotEnableRemoteQueueByDefault()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  // Reject incomplete queue settings.
  const QString settingsFile = tempPath(tempDir, "generic-settings.ini");
  {
    QSettings settings(settingsFile, QSettings::IniFormat);
    settings.setValue("xtalopt/version", CurrentStateSchemaVersion);
    settings.setValue("xtalopt/input/remoteQueue", true);
    settings.sync();
  }

  XtalOpt loaded;
  QVERIFY(loaded.readSettings(settingsFile, false));
  QVERIFY(!loaded.isRemoteQueue());
}

void XtalOptUnitTest::loadPathsGovernLocalWorkDir()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  // Read the state file directory in load (becomes local work dir).
  {
    const QString savedRunDir = tempPath(tempDir, "saved-run");
    const QString stateDir = tempPath(tempDir, "state-dir");
    QVERIFY(QDir().mkpath(stateDir));

    const QString stateFile = Common::localPath(stateDir, "xtalopt.state");

    XtalOpt saved;
    saved.setLocWorkDir(savedRunDir);
    saved.setInputFormulasString("O1");
    QVERIFY(saved.processInputChemicalFormulas(saved.getInputFormulasString()));
    saved.setQueueInterface(0, "none");
    saved.setOptimizer(0, "gulp");
    const QString structureDir = Common::localPath(stateDir, "00001x00001");
    QVERIFY(QDir().mkpath(structureDir));
    Xtal* xtal = makeTestXtal(1, 1, 0, 8, Common::Vector3(0.0, 0.0, 0.0), 0.0);
    xtal->setLocpath(structureDir);
    QVERIFY(saved.tracker()->append(xtal));
    QVERIFY(saved.save(stateFile, false));

    XtalOpt loaded;
    loaded.setRunMode(XtalOpt::RunModeReadOnly);
    QVERIFY(loaded.resumeSearch(stateFile));
    QCOMPARE(loaded.getLocWorkDir(), QFileInfo(stateFile).absoluteDir().absolutePath());
  }

  // Read the settings directory (becomes local work dir).
  {
    const QString savedRunDir = tempPath(tempDir, "saved-run");
    const QString stateDir = tempPath(tempDir, "settings-state-dir");
    QVERIFY(QDir().mkpath(stateDir));

    const QString stateFile = Common::localPath(stateDir, "settings-only.state");

    XtalOpt saved;
    saved.setLocWorkDir(savedRunDir);
    saved.setInputFormulasString("O1");
    QVERIFY(saved.processInputChemicalFormulas(saved.getInputFormulasString()));
    saved.setQueueInterface(0, "none");
    saved.setOptimizer(0, "gulp");
    QVERIFY(saved.saveSettingsState(stateFile));

    XtalOpt loaded;
    QVERIFY(loaded.loadSettingsState(stateFile));
    QCOMPARE(loaded.getLocWorkDir(), QFileInfo(stateFile).absoluteDir().absolutePath());
  }

  // Keep the local work dir when loading scheme file.
  {
    const QString settingsFile = tempPath(tempDir, "job-settings.ini");

    XtalOpt saved;
    saved.setLocWorkDir("./saved-local");
    saved.setQueueInterface(0, "none");
    saved.setOptimizer(0, "gulp");
    QVERIFY(saved.writeOptScheme(settingsFile));

    XtalOpt loaded;
    loaded.setLocWorkDir("/before/read-job-settings");
    QVERIFY(loaded.readOptScheme(settingsFile));
    // Keep the work directory.
    QCOMPARE(loaded.getLocWorkDir(), QString("/before/read-job-settings"));
  }
}

void XtalOptUnitTest::stateSettingsReadAndWritePreservesMainFields()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  const QString stateFile = tempPath(tempDir, "state-readwrite.state");

  XtalOpt saved;
  saved.setLocWorkDir(tempDir.path());
  saved.setInputFormulasString("O1");
  QVERIFY(saved.processInputChemicalFormulas(saved.getInputFormulasString()));
  saved.setVerbose(true);
  saved.setAMin(4.5);
  saved.setAMax(8.5);
  saved.setVolMin(10.0);
  saved.setVolMax(200.0);
  saved.setNumInitial(7);
  saved.setParentsPoolSize(11);
  saved.setContStructs(13);
  saved.setRunningJobLimit(3);
  saved.setLimitRunningJobs(true);
  saved.setFailLimit(2);
  saved.setFailAction(SearchBase::FA_NewOffspring);
  saved.setMaxNumStructures(41);
  saved.setTolSpg(0.02);
  saved.setTolRdf(0.75);
  saved.setPSupercell(4.0);
  saved.setPCross(31);
  saved.setPStrip(17);
  saved.setPPerm(19);
  saved.setPAtomic(23);
  saved.setPComp(29);
  saved.setOptimizationType("pareto");
  saved.setTournamentSelection(false);
  saved.setRestrictedPool(true);
  saved.setCrowdingDistance(false);
  saved.setParetoFilterZeroWeights(true);
  saved.setObjectivePrecision(4);
  saved.setConstraintsReDo(true);
  saved.setUser1("alpha");
  saved.setUser2("beta");
  saved.setUser3("gamma");
  saved.setUser4("delta");
  saved.setRemoteQueue(false);
  QVERIFY(saved.setSshMethod("auto"));
  saved.setSoftExit(true);
  if (saved.getNumOptSteps() == 0)
    saved.appendOptStep();
  saved.setQueueInterface(0, "none");
  saved.setOptimizer(0, "gulp");
  QVERIFY(saved.optimizer(0));
  saved.optimizer(0)->setDirectRunCommand("mpirun -np 3 gulp");

  QVERIFY(saved.save(stateFile, false));

  XtalOpt loaded;
  QVERIFY(loaded.readSettings(stateFile, true));

  QCOMPARE(loaded.getInputFormulasString(), QString("O1"));
  QVERIFY(loaded.isVerbose());
  QCOMPARE(loaded.getAMin(), 4.5);
  QCOMPARE(loaded.getAMax(), 8.5);
  QCOMPARE(loaded.getVolMin(), 10.0);
  QCOMPARE(loaded.getVolMax(), 200.0);
  QCOMPARE(loaded.getNumInitial(), static_cast<uint>(7));
  QCOMPARE(loaded.getParentsPoolSize(), static_cast<uint>(11));
  QCOMPARE(loaded.getContStructs(), static_cast<uint>(13));
  QCOMPARE(loaded.getRunningJobLimit(), static_cast<uint>(3));
  QVERIFY(loaded.isLimitRunningJobs());
  QCOMPARE(loaded.getFailLimit(), static_cast<uint>(2));
  QCOMPARE(loaded.getFailAction(), SearchBase::FA_NewOffspring);
  QCOMPARE(loaded.getMaxNumStructures(), 41);
  QCOMPARE(loaded.getTolSpg(), 0.02);
  QCOMPARE(loaded.getTolRdf(), 0.75);
  QCOMPARE(loaded.getPSupercell(), 4.0);
  QCOMPARE(loaded.getPCross(), static_cast<uint>(31));
  QCOMPARE(loaded.getPStrip(), static_cast<uint>(17));
  QCOMPARE(loaded.getPPerm(), static_cast<uint>(19));
  QCOMPARE(loaded.getPAtomic(), static_cast<uint>(23));
  QCOMPARE(loaded.getPComp(), static_cast<uint>(29));
  QCOMPARE(loaded.getOptimizationType(), QString("pareto"));
  QVERIFY(!loaded.isTournamentSelection());
  QVERIFY(loaded.isRestrictedPool());
  QVERIFY(!loaded.isCrowdingDistance());
  QVERIFY(loaded.isParetoFilterZeroWeights());
  QCOMPARE(loaded.getObjectivePrecision(), 4);
  QVERIFY(loaded.isConstraintsReDo());
  QCOMPARE(loaded.getUser1(), QString("alpha"));
  QCOMPARE(loaded.getUser2(), QString("beta"));
  QCOMPARE(loaded.getUser3(), QString("gamma"));
  QCOMPARE(loaded.getUser4(), QString("delta"));
  QVERIFY(!loaded.isRemoteQueue());
  QCOMPARE(loaded.sshMethod(), QString("auto"));
  QVERIFY(!loaded.isSoftExit());
  QCOMPARE(loaded.queueInterface(0)->getIDString(), QString("none"));
  QCOMPARE(loaded.optimizer(0)->getIDString().toLower(), QString("gulp"));
  QCOMPARE(loaded.optimizer(0)->getDirectRunCommand(), QString("mpirun -np 3 gulp"));
}

void XtalOptUnitTest::customIADCheckLimitsValidatesTable()
{
  // Reject an incomplete IAD table (missing element pairs).
  {
    XtalOpt opt;
    QVERIFY(opt.processInputChemicalFormulas("H1O1"));
    opt.setUsingScaledIAD(false);
    opt.setUsingCustomIAD(true);

    insertCustomIAD(opt, 1, 8, 1.0);

    QVERIFY(!opt.checkLimits());
  }

  // Read a complete IAD table.
  {
    XtalOpt opt;
    QVERIFY(opt.processInputChemicalFormulas("H1O1"));
    opt.setUsingScaledIAD(false);
    opt.setUsingCustomIAD(true);

    insertCustomIAD(opt, 1, 1, 0.5);
    insertCustomIAD(opt, 1, 8, 1.0);
    insertCustomIAD(opt, 8, 8, 0.5);

    QVERIFY(opt.checkLimits());
  }

  // Read molecule distance limits.
  {
    XtalOpt opt;
    QVERIFY(opt.processInputChemicalFormulas("C1K3"));
    opt.setUsingScaledIAD(false);
    opt.setUsingCustomIAD(true);
    QVERIFY(opt.processInputMoleculeUnit("C1K3 trigonal_pyramidal_c3v_center_shell"));

    insertCustomIAD(opt, 6, 6, 0.5);
    insertCustomIAD(opt, 6, 19, 100.0);
    insertCustomIAD(opt, 19, 19, 0.5);

    QVERIFY(opt.checkLimits());
  }
}

void XtalOptUnitTest::generateRandomXtalHonorsConstraints()
{
  // Check IAD limits for a randomly generated xtal.
  {
    XtalOpt opt;
    QVERIFY(opt.processInputChemicalFormulas("H1O1"));
    opt.setAMin(8.0);
    opt.setAMax(8.0);
    opt.setBMin(8.0);
    opt.setBMax(8.0);
    opt.setCMin(8.0);
    opt.setCMax(8.0);
    opt.setAlphaMin(90.0);
    opt.setAlphaMax(90.0);
    opt.setBetaMin(90.0);
    opt.setBetaMax(90.0);
    opt.setGammaMin(90.0);
    opt.setGammaMax(90.0);
    opt.setVolMin(100.0);
    opt.setVolMax(600.0);
    opt.setUsingScaledIAD(false);
    opt.setUsingCustomIAD(true);

    insertCustomIAD(opt, 1, 1, 0.5);
    insertCustomIAD(opt, 1, 8, 2.0);
    insertCustomIAD(opt, 8, 8, 0.5);

    Xtal* xtal = opt.generateRandomXtal(1, 1, opt.compList().first());
    QVERIFY(xtal != nullptr);
    QCOMPARE(xtal->numAtoms(), static_cast<size_t>(2));
    QVERIFY(xtal->distance(0, 1) >= 2.0);
    delete xtal;
  }

  // Check fixed cell limits for a randomly generated xtal.
  {
    XtalOpt opt;
    QVERIFY(opt.processInputChemicalFormulas("O1"));
    opt.setAMin(4.0);
    opt.setAMax(4.0);
    opt.setBMin(6.0);
    opt.setBMax(6.0);
    opt.setCMin(8.0);
    opt.setCMax(8.0);
    opt.setAlphaMin(90.0);
    opt.setAlphaMax(90.0);
    opt.setBetaMin(90.0);
    opt.setBetaMax(90.0);
    opt.setGammaMin(90.0);
    opt.setGammaMax(90.0);
    opt.setVolMin(192.0);
    opt.setVolMax(192.0);

    Xtal* xtal = opt.generateRandomXtal(1, 1, opt.compList().first());
    QVERIFY(xtal != nullptr);
    QCOMPARE(xtal->numAtoms(), static_cast<size_t>(1));
    QVERIFY(fabs(xtal->getA() - 4.0) < 1e-6);
    QVERIFY(fabs(xtal->getB() - 6.0) < 1e-6);
    QVERIFY(fabs(xtal->getC() - 8.0) < 1e-6);
    QVERIFY(fabs(xtal->getAlpha() - 90.0) < 1e-6);
    QVERIFY(fabs(xtal->getBeta() - 90.0) < 1e-6);
    QVERIFY(fabs(xtal->getGamma() - 90.0) < 1e-6);
    delete xtal;
  }
}

void XtalOptUnitTest::generateRandomXtalWithMoleculeUnits()
{
  // Read a molecule unit (current syntax should be valid, old numeric syntax should be rejected).
  {
    XtalOpt opt;
    QVERIFY(opt.processInputChemicalFormulas("C1K1, C2K2"));
    opt.setUsingScaledIAD(true);
    opt.setScaleFactor(0.4);
    opt.setMinRadius(0.0);
    opt.refreshElementMinRadii();
    QVERIFY(!opt.processInputMoleculeUnit("H2"));
    if (!opt.processInputMoleculeUnit("K 1 c 1 linear_2_hetero"))
      QSKIP("libmsym molecule generation is not available.");

    QCOMPARE(opt.moleculeUnitInputs().size(), 1);
    QCOMPARE(opt.moleculeUnitInputs().at(0), QString("C1K1 linear_2_hetero"));
    QCOMPARE(opt.moleculeUnits().size(), static_cast<size_t>(1));
    QCOMPARE(opt.moleculeUnits().front().atoms().size(), static_cast<size_t>(2));

    const std::vector<Atoms::Atom>& atoms = opt.moleculeUnits().front().atoms();
    const double distance = (atoms[0].pos() - atoms[1].pos()).norm();
    const double minDistance = opt.eleMinRadii().getMinRadius(atoms[0].atomicNumber()) +
      opt.eleMinRadii().getMinRadius(atoms[1].atomicNumber());
    QVERIFY(distance > 2.0 * minDistance);
  }

  // Random xtal generation should prefer a proper molunit over a pure random atomic xtal.
  {
    XtalOpt opt;
    QVERIFY(opt.processInputChemicalFormulas("C1K1"));
    opt.setUsingRandSpg(false);
    opt.setUsingScaledIAD(false);
    opt.setVolMin(500.0);
    opt.setVolMax(500.0);

    if (!opt.processInputMoleculeUnit("C1K1 linear_2_hetero"))
      QSKIP("libmsym molecule generation is not available.");

    Xtal* xtal = opt.generateRandomXtal(1, 1, opt.compList().first());
    QVERIFY(xtal != nullptr);
    QVERIFY(xtal->getParents().contains("random molUnit"));
    delete xtal;
  }
}

void XtalOptUnitTest::resultsFileSortsByAboveHull()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  XtalOpt opt;
  opt.setLocWorkDir(tempDir.path());

  CellComp comp;
  comp.setCompositionEntry("O", 8, 1);
  opt.compList().append(comp);

  Xtal* lowAboveHull = makeTestXtal(1, 1, 20, 8, Common::Vector3(0.0, 0.0, 0.0), 0.10);
  Xtal* tieLowIndex = makeTestXtal(1, 2, 3, 8, Common::Vector3(0.0, 0.0, 0.0), 0.20);
  Xtal* tieHighIndex = makeTestXtal(1, 3, 9, 8, Common::Vector3(0.0, 0.0, 0.0), 0.20);
  Xtal* inProgress = makeTestXtal(1, 4, 1, 8, Common::Vector3(0.0, 0.0, 0.0), 0.00);
  Xtal* removed = makeTestXtal(1, 5, 2, 8, Common::Vector3(0.0, 0.0, 0.0), 0.05);

  inProgress->setStatus(Structure::InProcess);
  removed->setStatus(Structure::Removed);
  opt.addConstraint("filter", "filter.out");
  lowAboveHull->setStrucConstraintValues(1.0);
  tieLowIndex->setStrucConstraintValues(0.0);
  tieHighIndex->setStrucConstraintValues(1.0);

  QList<Structure*> structures;
  structures << tieHighIndex << removed << lowAboveHull << inProgress
             << tieLowIndex;

  QVERIFY(opt.writeResultsFile(structures, false));

  QFile results(tempPath(tempDir, "results.txt"));
  QVERIFY(results.open(QIODevice::ReadOnly));
  const QStringList lines =
    QString::fromUtf8(results.readAll()).split('\n', QtCompat::SkipEmptyParts);

  QCOMPARE(lines.size(), 6);
  QVERIFY(lines.first().contains("Cons1"));

  QStringList orderedTags;
  QList<int> orderedRanks;
  for (int i = 1; i < lines.size(); ++i) {
    const QStringList fields = lines.at(i).simplified().split(' ');
    QVERIFY(fields.size() > 1);
    orderedRanks.append(fields.at(0).toInt());
    orderedTags.append(fields.at(1));
  }

  QCOMPARE(orderedRanks, QList<int>() << 1 << 2 << 3 << 4 << 5);
  QCOMPARE(orderedTags, QStringList() << "1x1" << "1x2" << "1x3" << "1x4" << "1x5");

  qDeleteAll(structures);
}

void XtalOptUnitTest::checkForDuplicatesTest()
{
  XtalOpt opt;
  opt.setTolXcLength(0.1);
  opt.setTolXcAngle(0.1);
  opt.setTolRdf(0.0);

  Xtal* keep = makeTestXtal(1, 1, 0, 8, Common::Vector3(0.0, 0.0, 0.0), 0.0);
  Xtal* similar = makeTestXtal(1, 2, 1, 8, Common::Vector3(0.0, 0.0, 0.0), 0.0);
  Xtal* unique = makeTestXtal(1, 3, 2, 14, Common::Vector3(0.0, 0.0, 0.0), 0.0);

  QVERIFY(opt.tracker()->append(keep));
  QVERIFY(opt.tracker()->append(similar));
  QVERIFY(opt.tracker()->append(unique));

  opt.checkForSimilarities_();

  const QList<Structure*> similarStructures = opt.queue()->getAllSimilarStructures();
  QCOMPARE(similarStructures.size(), 1);
  QCOMPARE(keep->getStatus(), Xtal::Optimized);
  QCOMPARE(similar->getStatus(), Xtal::Optimized);
  QVERIFY(similar->isSimilar());
  QCOMPARE(unique->getStatus(), Xtal::Optimized);
  QCOMPARE(similar->getSimilarityString(), QString("1x1"));

  opt.reset();
}

void XtalOptUnitTest::stepwiseCheckForDuplicatesTest()
{
  XtalOpt opt;
  opt.setTolXcLength(0.1);
  opt.setTolXcAngle(0.1);
  opt.setTolRdf(0.0);

  Xtal* keep = makeTestXtal(2, 1, 0, 8, Common::Vector3(0.0, 0.0, 0.0), 0.0);
  Xtal* similar = makeTestXtal(2, 2, 1, 8, Common::Vector3(0.0, 0.0, 0.0), 0.0);
  Xtal* unique = makeTestXtal(2, 3, 2, 8, Common::Vector3(0.5, 0.5, 0.5), 0.5);

  QVERIFY(opt.tracker()->append(keep));
  opt.checkForSimilarities_();
  QCOMPARE(opt.queue()->getAllSimilarStructures().size(), 0);

  QVERIFY(opt.tracker()->append(similar));
  opt.checkForSimilarities_();
  QCOMPARE(opt.queue()->getAllSimilarStructures().size(), 1);
  QCOMPARE(similar->getStatus(), Xtal::Optimized);
  QVERIFY(similar->isSimilar());
  QCOMPARE(similar->getSimilarityString(), QString("2x1"));

  QVERIFY(opt.tracker()->append(unique));
  opt.checkForSimilarities_();
  QCOMPARE(opt.queue()->getAllSimilarStructures().size(), 1);
  QCOMPARE(unique->getStatus(), Xtal::Optimized);

  opt.reset();
}
}

QTEST_MAIN(XtalOpt::XtalOptUnitTest)

#include "xtaloptunittest.moc"

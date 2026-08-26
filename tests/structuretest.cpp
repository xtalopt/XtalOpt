/**********************************************************************
  StructureTest - StructureTest class provides unit testing for Structure

  Copyright (C) 2010 David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/structure.h>
#include <search/tracker.h>
#include <xtalopt/structures/xtal.h>
#include <xtalopt/xtalopt.h>
#include <common/fileutils.h>
#include <common/output.h>

#include <common/random.h>
#include <atoms/formats/cmlformat.h>
#include <atoms/generators.h>
#include <atoms/geometry.h>

#include <QDateTime>
#include <QDir>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

#include <fstream>
#include <memory>
#include <type_traits>
#include <utility>

#define APPROX_EQ(a, b) (fabs((a) - (b)) < 1e-6)

using namespace Search;

// Check that Structure can be derived from Geometry.
static_assert(std::is_base_of<Atoms::Geometry, Structure>::value,
              "Structure must expose the reusable structure API.");

namespace {

typedef bool (*StateCheck)(Structure::State);
typedef bool (Structure::*MemberStateCheck)() const;

QList<Structure::State> allStructureStates()
{
  QList<Structure::State> states;
  states << Structure::Optimized
         << Structure::StepOptimized
         << Structure::WaitingForOptimization
         << Structure::InProcess
         << Structure::Empty
         << Structure::Updating
         << Structure::Error
         << Structure::Submitted
         << Structure::Failed
         << Structure::Removed
         << Structure::Restart
         << Structure::ScriptCalculation
         << Structure::ObjectiveCalculation
         << Structure::ConstraintCalculation
         << Structure::Dismissed
         << Structure::ObjcFailed
         << Structure::ConsFailed;
  return states;
}

void checkState(StateCheck check, MemberStateCheck memberCheck,
                         const QList<Structure::State>& expectedStates)
{
  for (Structure::State state : allStructureStates()) {
    Structure structure;
    structure.setStatus(state);

    const bool expected = expectedStates.contains(state);
    QCOMPARE(check(state), expected);
    QCOMPARE((structure.*memberCheck)(), expected);
  }
}

void checkCopiedState(Structure& structure)
{
  QCOMPARE(structure.getFixCount(), 3);
  QCOMPARE(structure.getRestartOptStep(), 2);
  QVERIFY(structure.hasEnthalpy());
  QCOMPARE(structure.getEnergy(), -4.5);
  QCOMPARE(structure.getEnthalpy(), -4.0);
  QCOMPARE(structure.getPV(), 0.5);

  QCOMPARE(structure.sizeOfHistory(), 1u);

  QList<unsigned int> atomicNums;
  QList<Common::Vector3> coords;
  double energy = 0.0;
  double enthalpy = 0.0;
  Common::Matrix3 cell;

  structure.retrieveHistoryEntry(0, &atomicNums, &coords, &energy, &enthalpy,&cell);
  QCOMPARE(atomicNums, QList<unsigned int>() << 8u << 14u);
  QCOMPARE(coords.size(), 2);
  QVERIFY(coords.at(0) == Common::Vector3(0.0, 0.0, 0.0));
  QVERIFY(coords.at(1) == Common::Vector3(1.0, 2.0, 3.0));
  QCOMPARE(energy, -3.5);
  QCOMPARE(enthalpy, -3.0);

  Common::Matrix3 expectedCell;
  expectedCell << 4.0, 0.0, 0.0,
                  0.0, 5.0, 0.0,
                  0.0, 0.0, 6.0;
  QVERIFY(cell == expectedCell);

  const std::vector<Atoms::Bond>& bonds = structure.getPreoptBonding();
  QCOMPARE(static_cast<int>(bonds.size()), 1);
  QCOMPARE(bonds.at(0).first(), static_cast<size_t>(0));
  QCOMPARE(bonds.at(0).second(), static_cast<size_t>(1));
  QCOMPARE(bonds.at(0).bondOrder(), static_cast<unsigned short>(2));
}
}

class StructureTest : public QObject
{
  Q_OBJECT

private:
  Structure* m_structure;

private slots:
  /**
   * Called before the first test function is executed.
   */
  void initTestCase();

  /**
   * Called after the last test function is executed.
   */
  void cleanupTestCase();

  /**
   * Called before each test function is executed.
   */
  void init();

  /**
   * Called after every test function.
   */
  void cleanup();

  // Tests
  void enthalpyFallBack();
  void generateRandomStructure();
  void generateRandomStructureUsesDirectedPairDistances();
  void generateRandomStructureSortsAtomsByRadius();
  void generateRandomStructureAllowsAnisotropicFixedCells();
  void workflowStateCycle();
  void stateHelpersClassifyStates();
  void workflowStateStatusText();
  void copyAndMovePreserveState();
  void malformedCurrentGeometryIsRejectedOnSessionLoad();
  void generateIADHistogramSkipsDuplicateZeroTranslation();
  void perceiveBonds();
  void updateAndSkipHistoryUpdatesAtoms();
};

void StructureTest::initTestCase()
{
  Common::seedMt19937Generator(0);
}

void StructureTest::cleanupTestCase()
{
}

void StructureTest::init()
{
  m_structure = new Structure;
}

void StructureTest::cleanup()
{
  if (m_structure) {
    delete m_structure;
  }
  m_structure = 0;
}

void StructureTest::enthalpyFallBack()
{
  Structure s;
  s.addAtom();

  s.setEnergy(1.0);
  Common::message(QString("Enthalpy is: %1").arg(s.getEnthalpy()));
  QVERIFY(APPROX_EQ(s.getEnthalpy(), 1.0));

  s.setEnergy(-1.0);
  QVERIFY(APPROX_EQ(s.getEnthalpy(), -1.0));

  s.setEnthalpy(3.0);
  QVERIFY(APPROX_EQ(s.getEnthalpy(), 3.0));

  s.setEnthalpy(-3.0);
  QVERIFY(APPROX_EQ(s.getEnthalpy(), -3.0));

  QList<unsigned int> anums;
  anums << 1;
  QList<Common::Vector3> coords;
  coords << Common::Vector3(0, 0, 0);

  s.updateAndSkipHistory(anums, coords,
                         1.0,  // energy
                         0.0); // enthalpy
  QVERIFY(APPROX_EQ(s.getEnthalpy(), 1.0));

  s.updateAndSkipHistory(anums, coords,
                         -1.0, // energy
                         0.0); // enthalpy
  QVERIFY(APPROX_EQ(s.getEnthalpy(), -1.0));

  s.updateAndAddToHistory(anums, coords,
                          1.0,  // energy
                          0.0); // enthalpy
  QVERIFY(APPROX_EQ(s.getEnthalpy(), 1.0));

  s.updateAndAddToHistory(anums, coords,
                          -1.0, // energy
                          0.0); // enthalpy
  QVERIFY(APPROX_EQ(s.getEnthalpy(), -1.0));

  s.updateAndSkipHistory(anums, coords,
                         0.0,  // energy
                         1.0); // enthalpy
  QVERIFY(APPROX_EQ(s.getEnthalpy(), 1.0));

  s.updateAndSkipHistory(anums, coords,
                         0.0,   // energy
                         -1.0); // enthalpy
  QVERIFY(APPROX_EQ(s.getEnthalpy(), -1.0));

  s.updateAndAddToHistory(anums, coords,
                          0.0,  // energy
                          1.0); // enthalpy
  QVERIFY(APPROX_EQ(s.getEnthalpy(), 1.0));

  s.updateAndAddToHistory(anums, coords,
                          0.0,   // energy
                          -1.0); // enthalpy
  QVERIFY(APPROX_EQ(s.getEnthalpy(), -1.0));
}

void StructureTest::generateRandomStructure()
{
  Atoms::Generators::CrystalGenerationOptions options;
  options.atomicNumbers.push_back(8);
  options.atomicNumbers.push_back(1);
  options.atomicNumbers.push_back(1);
  options.aMin = 6.0;
  options.bMin = 6.0;
  options.cMin = 6.0;
  options.alphaMin = 70.0;
  options.betaMin = 70.0;
  options.gammaMin = 70.0;
  options.aMax = 8.0;
  options.bMax = 8.0;
  options.cMax = 8.0;
  options.alphaMax = 110.0;
  options.betaMax = 110.0;
  options.gammaMax = 110.0;
  options.minVolume = 180.0;
  options.maxVolume = 220.0;
  options.atomicRadii[1] = 0.3;
  options.atomicRadii[8] = 0.6;

  std::unique_ptr<Atoms::Geometry> structure = Atoms::Generators::generateRandom(options);

  QVERIFY(structure != nullptr);
  QCOMPARE(structure->numAtoms(), static_cast<size_t>(3));
  QVERIFY(structure->getVolume() >= options.minVolume);
  QVERIFY(structure->getVolume() <= options.maxVolume);

  for (size_t i = 0; i < structure->numAtoms(); ++i) {
    for (size_t j = i + 1; j < structure->numAtoms(); ++j) {
      const double minDistance = options.atomicRadii[structure->atom(i).atomicNumber()] +
        options.atomicRadii[structure->atom(j).atomicNumber()];
      QVERIFY(structure->distance(i, j) >= minDistance);
    }
  }
}

void StructureTest::generateRandomStructureUsesDirectedPairDistances()
{
  Atoms::Generators::CrystalGenerationOptions options;
  options.atomicNumbers.push_back(8);
  options.atomicNumbers.push_back(1);
  options.aMin = options.aMax = 10.0;
  options.bMin = options.bMax = 10.0;
  options.cMin = options.cMax = 10.0;
  options.alphaMin = options.alphaMax = 90.0;
  options.betaMin = options.betaMax = 90.0;
  options.gammaMin = options.gammaMax = 90.0;
  options.maxAtomPlacementAttempts = 10;
  options.reduceCell = false;

  Atoms::Generators::CrystalGenerationOptions reverseOnly = options;
  reverseOnly.pairMinDistances[std::make_pair(8u, 1u)] = 100.0;
  std::unique_ptr<Atoms::Geometry> structure = Atoms::Generators::generateRandom(reverseOnly);
  QVERIFY(structure != nullptr);
  QCOMPARE(structure->numAtoms(), static_cast<size_t>(2));

  Atoms::Generators::CrystalGenerationOptions directed = options;
  directed.pairMinDistances[std::make_pair(1u, 8u)] = 100.0;
  structure = Atoms::Generators::generateRandom(directed);
  QVERIFY(structure == nullptr);
}

void StructureTest::generateRandomStructureSortsAtomsByRadius()
{
  Atoms::Generators::CrystalGenerationOptions options;
  options.atomicNumbers.push_back(1);
  options.atomicNumbers.push_back(8);
  options.atomicNumbers.push_back(1);
  options.aMin = options.aMax = 10.0;
  options.bMin = options.bMax = 10.0;
  options.cMin = options.cMax = 10.0;
  options.alphaMin = options.alphaMax = 90.0;
  options.betaMin = options.betaMax = 90.0;
  options.gammaMin = options.gammaMax = 90.0;
  options.atomicRadii[1] = 0.3;
  options.atomicRadii[8] = 0.6;
  options.reduceCell = false;

  std::unique_ptr<Atoms::Geometry> structure = Atoms::Generators::generateRandom(options);
  QVERIFY(structure != nullptr);
  QCOMPARE(structure->numAtoms(), static_cast<size_t>(3));
  QCOMPARE(structure->atom(0).atomicNumber(), static_cast<unsigned short>(8));
  QCOMPARE(structure->atom(1).atomicNumber(), static_cast<unsigned short>(1));
  QCOMPARE(structure->atom(2).atomicNumber(), static_cast<unsigned short>(1));
}

void StructureTest::generateRandomStructureAllowsAnisotropicFixedCells()
{
  Atoms::Generators::CrystalGenerationOptions options;
  options.atomicNumbers.push_back(8);
  options.aMin = options.aMax = 1.0;
  options.bMin = options.bMax = 100.0;
  options.cMin = options.cMax = 100.0;
  options.alphaMin = options.alphaMax = 90.0;
  options.betaMin = options.betaMax = 90.0;
  options.gammaMin = options.gammaMax = 90.0;
  options.reduceCell = false;

  std::unique_ptr<Atoms::Geometry> structure = Atoms::Generators::generateRandom(options);
  QVERIFY(structure != nullptr);
  QCOMPARE(structure->numAtoms(), static_cast<size_t>(1));
  QVERIFY(APPROX_EQ(structure->getA(), 1.0));
  QVERIFY(APPROX_EQ(structure->getB(), 100.0));
  QVERIFY(APPROX_EQ(structure->getC(), 100.0));
}

void StructureTest::workflowStateCycle()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  const QString stateFile = Common::localPath(tempDir.path(), "xtalopt.state");
  const QString structureDir = Common::localPath(tempDir.path(), "00003x00007");
  QVERIFY(QDir().mkpath(structureDir));

  XtalOpt::XtalOpt producer;
  producer.setLocWorkDir(tempDir.path());
  producer.setInputFormulasString("O1");
  QVERIFY(producer.processInputChemicalFormulas(producer.getInputFormulasString()));
  producer.clearOptSteps();
  for (int i = 0; i < 3; ++i) {
    producer.appendOptStep();
    QVERIFY(producer.setQueueInterface(i, "none"));
    QVERIFY(producer.setOptimizer(i, "gulp"));
  }

  XtalOpt::Xtal* saved = new XtalOpt::Xtal;
  saved->setGeneration(3);
  saved->setIDNumber(7);
  saved->setJobID(1234);
  saved->setCurrentOptStep(2);
  saved->setParents("parent-a parent-b");
  saved->setLocpath(structureDir);
  saved->setRempath("/remote/structure");
  saved->setStatus(Structure::ObjectiveCalculation);
  saved->setFailCount(4);
  saved->setOptTimerStart(QDateTime::fromString("2026-01-02T03:04:05", Qt::ISODate));
  saved->setOptTimerEnd(QDateTime::fromString("2026-01-02T04:04:05", Qt::ISODate));
  saved->appendCopyFile("extra.dat");

  const double objectiveValue = 2.500000123456789;
  const double constraintValue = 0.12345678901234566;
  const Common::Vector3 position(1.2345678901234567, -2.3456789012345678, 3.4567890123456789);
  saved->addAtom(8, position);
  saved->setStrucObjValues(1.25);
  saved->setStrucObjValues(objectiveValue);
  saved->setStrucObjState(Structure::Os_Retain);
  saved->setStrucConstraintValues(constraintValue);
  saved->setStrucConstraintState(Structure::Cs_Dismiss);
  saved->setStrucConstraintRedoCount(1);

  QVERIFY(producer.tracker()->append(saved));
  QVERIFY(producer.saveSessionState(stateFile, false));

  XtalOpt::XtalOpt restored;
  restored.setRunMode(XtalOpt::XtalOpt::RunModeReadOnly);
  QVERIFY(restored.resumeSearch(stateFile));
  QCOMPARE(restored.tracker()->size(), 1);

  XtalOpt::Xtal* loaded = static_cast<XtalOpt::Xtal*>(restored.tracker()->at(0));
  QVERIFY(loaded);

  QCOMPARE(loaded->getGeneration(), static_cast<uint>(3));
  QCOMPARE(loaded->getIDNumber(), static_cast<uint>(7));
  QCOMPARE(loaded->getIndex(), 0);
  QCOMPARE(loaded->getJobID(), static_cast<uint>(1234));
  QCOMPARE(loaded->getCurrentOptStep(), static_cast<uint>(2));
  QCOMPARE(loaded->getParents(), QString("parent-a parent-b"));
  QCOMPARE(loaded->getLocpath(), structureDir);
  QCOMPARE(loaded->getRempath(), QString("/remote/structure"));
  QCOMPARE(loaded->getStatus(), Structure::ObjectiveCalculation);
  QCOMPARE(loaded->getFailCount(), static_cast<uint>(4));
  QCOMPARE(loaded->copyFiles().size(), static_cast<size_t>(1));
  QCOMPARE(QString::fromStdString(loaded->copyFiles().front()), QString("extra.dat"));
  QCOMPARE(loaded->getStrucObjNumber(), 2);
  QVERIFY(std::isnan(loaded->getStrucObjValues(0)));
  QCOMPARE(loaded->getStrucObjValues(1), objectiveValue);
  QCOMPARE(loaded->getStrucObjState(), Structure::Os_Retain);
  QCOMPARE(loaded->getStrucConstraintNumber(), 1);
  QCOMPARE(loaded->getStrucConstraintValues(0), constraintValue);
  QCOMPARE(loaded->getStrucConstraintState(), Structure::Cs_Dismiss);
  QCOMPARE(loaded->getStrucConstraintRedoCount(), 1);
  QCOMPARE(loaded->numAtoms(), static_cast<size_t>(1));
  QCOMPARE(loaded->atom(0).atomicNumber(), static_cast<unsigned short>(8));
  QCOMPARE(loaded->atom(0).pos().x(), position.x());
  QCOMPARE(loaded->atom(0).pos().y(), position.y());
  QCOMPARE(loaded->atom(0).pos().z(), position.z());
}

void StructureTest::stateHelpersClassifyStates()
{
  QList<Structure::State> expected;

  expected << Structure::Optimized
           << Structure::Failed
           << Structure::Removed
           << Structure::Dismissed
           << Structure::ObjcFailed
           << Structure::ConsFailed;
  checkState(static_cast<StateCheck>(&Structure::isQueueTerminalState),
    static_cast<MemberStateCheck>(&Structure::isQueueTerminalState), expected);

  expected.clear();
  expected << Structure::Submitted
           << Structure::InProcess
           << Structure::ObjectiveCalculation
           << Structure::ConstraintCalculation;
  checkState(static_cast<StateCheck>(&Structure::isQueueInProgressState),
    static_cast<MemberStateCheck>(&Structure::isQueueInProgressState), expected);

  expected.clear();
  expected << Structure::ObjectiveCalculation
           << Structure::ConstraintCalculation;
  checkState(static_cast<StateCheck>(&Structure::isScriptCalculationState),
    static_cast<MemberStateCheck>(&Structure::isScriptCalculationState), expected);

  expected.clear();
  expected << Structure::Optimized;
  checkState(static_cast<StateCheck>(&Structure::isOptimizedState),
    static_cast<MemberStateCheck>(&Structure::isOptimizedState), expected);

  expected.clear();
  expected << Structure::ObjcFailed
           << Structure::ConsFailed;
  checkState(static_cast<StateCheck>(&Structure::isScriptFailedState),
    static_cast<MemberStateCheck>(&Structure::isScriptFailedState), expected);

  expected.clear();
  expected << Structure::Dismissed;
  checkState(static_cast<StateCheck>(&Structure::isScriptDismissedState),
    static_cast<MemberStateCheck>(&Structure::isScriptDismissedState), expected);

  expected.clear();
  expected << Structure::Failed
           << Structure::Removed;
  checkState(static_cast<StateCheck>(&Structure::isFailedOrRemovedState),
    static_cast<MemberStateCheck>(&Structure::isFailedOrRemovedState), expected);

  expected.clear();
  expected << Structure::Failed
           << Structure::Removed
           << Structure::Dismissed
           << Structure::ObjcFailed
           << Structure::ConsFailed;
  checkState(static_cast<StateCheck>(&Structure::isStoppedFinalState),
    static_cast<MemberStateCheck>(&Structure::isStoppedFinalState), expected);

  expected.clear();
  expected << Structure::Error
           << Structure::Restart;
  checkState(static_cast<StateCheck>(&Structure::isQueueErrorRecoveryState),
    static_cast<MemberStateCheck>(&Structure::isQueueErrorRecoveryState), expected);
}

void StructureTest::workflowStateStatusText()
{
  struct StatusTextExpectation
  {
    Structure::State state;
    const char* shortText;
    const char* longText;
  };

  const StatusTextExpectation expectations[] = {
    { Structure::Optimized, "Optimized", "Optimized" },
    { Structure::StepOptimized, "Checking", "Checking status" },
    { Structure::WaitingForOptimization, "Waiting", "Waiting for optimization" },
    { Structure::InProcess, "InProcess", "In process" },
    { Structure::Empty, "Empty", "Structure empty" },
    { Structure::Updating, "Updating", "Updating structure" },
    { Structure::Error, "Error", "Job error" },
    { Structure::Submitted, "Submitted", "Job submitted" },
    { Structure::Failed, "Failed", "Failed" },
    { Structure::Removed, "Removed", "Removed" },
    { Structure::Restart, "Restart", "Restarting job" },
    { Structure::ScriptCalculation, "ScriptCalcs", "Script Calculation" },
    { Structure::ObjectiveCalculation, "ObjcCalcs", "Calculating objectives" },
    { Structure::ConstraintCalculation, "ConsCalcs", "Calculating constraints" },
    { Structure::Dismissed, "Dismissed", "Dismissed" },
    { Structure::ObjcFailed, "ObjcFailed", "Objective failed" },
    { Structure::ConsFailed, "ConsFailed", "Constraint failed" }
  };

  for (size_t i = 0; i < sizeof(expectations) / sizeof(expectations[0]); ++i) {
    const StatusTextExpectation& expectation = expectations[i];
    QCOMPARE(Structure::statusText(expectation.state, false), QString(expectation.shortText));
    QCOMPARE(Structure::statusText(expectation.state, true), QString(expectation.longText));

    Structure structure;
    structure.setStatus(expectation.state);
    QCOMPARE(structure.statusText(false), QString(expectation.shortText));
    QCOMPARE(structure.statusText(true), QString(expectation.longText));
  }

  Structure similar;
  similar.setStatus(Structure::Optimized);
  similar.setSimilarityString("1x2");
  QVERIFY(similar.isSimilar());
  QCOMPARE(similar.statusText(false), QString("Sim:1x2"));
  QCOMPARE(similar.statusText(true), QString("Similar to 1x2"));
  similar.setStatus(Structure::Restart);
  QVERIFY(!similar.isSimilar());

  Structure invalid;
  invalid.setStatus(static_cast<Structure::State>(999));
  QCOMPARE(invalid.statusText(false), QString("Unknown"));
  QCOMPARE(invalid.statusText(true), QString("Unknown"));
}

void StructureTest::copyAndMovePreserveState()
{
  Structure record;
  QCOMPARE(record.getFixCount(), 0);

  record.setFixCount(3);
  record.setRestartOptStep(2);
  record.setStatus(Structure::Restart);
  record.setEnergy(-4.5);
  record.setEnthalpy(-4.0);
  record.setPV(0.5);

  QList<unsigned int> atomicNums;
  atomicNums << 8u << 14u;
  QList<Common::Vector3> coords;
  coords << Common::Vector3(0.0, 0.0, 0.0)
         << Common::Vector3(1.0, 2.0, 3.0);
  Common::Matrix3 cell;
  cell << 4.0, 0.0, 0.0,
          0.0, 5.0, 0.0,
          0.0, 0.0, 6.0;
  record.appendHistoryEntry(atomicNums, coords, -3.5, -3.0, cell);

  std::vector<Atoms::Bond> bonds;
  bonds.push_back(Atoms::Bond(0, 1, 2));
  record.setPreoptBonding(bonds);

  Structure copied(record);
  checkCopiedState(copied);

  Structure assigned;
  assigned.setEnergy(10.0);
  assigned.appendHistoryEntry(QList<unsigned int>() << 1u,
                              QList<Common::Vector3>() << Common::Vector3(9.0, 9.0, 9.0),
                              10.0, 11.0, Common::Matrix3::Identity());
  assigned.setPreoptBonding(std::vector<Atoms::Bond>(1, Atoms::Bond(2, 3, 1)));
  assigned = record;
  checkCopiedState(assigned);

  Structure moved(std::move(copied));
  checkCopiedState(moved);

  Structure moveAssigned;
  moveAssigned.setEnergy(20.0);
  moveAssigned.appendHistoryEntry(QList<unsigned int>() << 2u,
                                  QList<Common::Vector3>() << Common::Vector3(8.0, 8.0, 8.0),
                                  20.0, 21.0, Common::Matrix3::Identity());
  moveAssigned.setPreoptBonding(std::vector<Atoms::Bond>(1, Atoms::Bond(3, 4, 1)));
  moveAssigned = std::move(assigned);
  checkCopiedState(moveAssigned);
}

void StructureTest::malformedCurrentGeometryIsRejectedOnSessionLoad()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  const QString stateFile = Common::localPath(tempDir.path(), "xtalopt.state");
  const QString structureDir = Common::localPath(tempDir.path(), "00001x00001");
  QVERIFY(QDir().mkpath(structureDir));

  XtalOpt::XtalOpt producer;
  producer.setLocWorkDir(tempDir.path());
  producer.setInputFormulasString("O1");
  QVERIFY(producer.processInputChemicalFormulas(producer.getInputFormulasString()));
  QVERIFY(producer.setQueueInterface(0, "none"));
  QVERIFY(producer.setOptimizer(0, "gulp"));

  XtalOpt::Xtal* saved = new XtalOpt::Xtal;
  saved->setGeneration(1);
  saved->setIDNumber(1);
  saved->setLocpath(structureDir);
  saved->setStatus(Structure::Optimized);
  saved->addAtom(1, Common::Vector3(0.0, 0.0, 0.0));
  saved->addAtom(8, Common::Vector3(1.0, 0.0, 0.0));
  QVERIFY(producer.tracker()->append(saved));
  QVERIFY(producer.saveSessionState(stateFile, false));

  {
    QSettings settings(Common::localPath(structureDir, "structure.state"), QSettings::IniFormat);
    settings.beginGroup("structure/current");
    settings.remove("coords");
    settings.beginWriteArray("coords");
    settings.setArrayIndex(0);
    settings.setValue("x", 0.0);
    settings.setValue("y", 0.0);
    settings.setValue("z", 0.0);
    settings.endArray();
    settings.endGroup();
    settings.sync();
  }

  XtalOpt::XtalOpt restored;
  restored.setRunMode(XtalOpt::XtalOpt::RunModeReadOnly);
  QVERIFY(!restored.resumeSearch(stateFile));
  QCOMPARE(restored.tracker()->size(), 0);
}

void StructureTest::generateIADHistogramSkipsDuplicateZeroTranslation()
{
  Structure s;
  s.setCellInfo(10.0, 10.0, 10.0, 90.0, 90.0, 90.0);
  s.addAtom(1, Common::Vector3(0.0, 0.0, 0.0));
  s.addAtom(1, Common::Vector3(1.0, 0.0, 0.0));

  std::vector<double> distances;
  std::vector<double> frequencies;
  QVERIFY(s.generateIADHistogram(&distances, &frequencies, 0.0, 2.0, 1.0));
  QCOMPARE(distances.size(), static_cast<size_t>(2));
  QCOMPARE(frequencies.size(), static_cast<size_t>(2));

  QCOMPARE(frequencies.at(1), 1.0);

  frequencies.clear();
  QVERIFY(s.generateIADHistogram(&distances, &frequencies, 0.0, 2.0, 1.0, s.atom(0)));
  QCOMPARE(frequencies.at(0), 0.0);
  QCOMPARE(frequencies.at(1), 1.0);
}

void StructureTest::perceiveBonds()
{
  QString ethaneFileName = Common::localPath(Common::localPath(QString(TESTDATADIR), "formats"),
                      "ethane.cml");
  std::ifstream in(ethaneFileName.toStdString().c_str());
  QVERIFY(in.is_open());

  Search::Structure ethane;
  QVERIFY(Atoms::CmlFormat::read(ethane, in));

  QVERIFY(ethane.numBonds() == 7);

  ethane.clearBonds();

  QVERIFY(ethane.numBonds() == 0);

  ethane.perceiveBonds();

  QVERIFY(ethane.numBonds() == 7);
}

void StructureTest::updateAndSkipHistoryUpdatesAtoms()
{
  Structure s;
  s.addAtom(1, Common::Vector3(0, 0, 0));

  QList<unsigned int> atomicNums;
  atomicNums << 8;
  QList<Common::Vector3> coords;
  coords << Common::Vector3(1, 2, 3);

  s.updateAndSkipHistory(atomicNums, coords, 0.0, 0.0);

  QCOMPARE(s.atom(0).atomicNumber(), static_cast<unsigned short>(8));
  QVERIFY(APPROX_EQ(s.atom(0).pos().x(), 1.0));
  QVERIFY(APPROX_EQ(s.atom(0).pos().y(), 2.0));
  QVERIFY(APPROX_EQ(s.atom(0).pos().z(), 3.0));
}

QTEST_MAIN(StructureTest)

#include "structuretest.moc"

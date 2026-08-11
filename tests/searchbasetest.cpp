/**********************************************************************
  SearchBaseTest - Unit tests for SearchBase class

  Copyright (C) 2011 David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/search.h>
#include <common/compatibility/qt_compat.h>
#include <common/fileutils.h>
#include <common/output.h>

#include <search/optimizer.h>
#include <common/output.h>
#include <search/queueinterface.h>
#include <search/queuemanager.h>
#include <search/structure.h>
#include <common/makeunique.h>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using namespace Search;

const QString DUMMYNAME = "Dummy";
const QString DESCRIPTION = "Description";

// Dummy optimizer for user value keyword checking
class DummyOptimizer : public Optimizer
{
  Q_OBJECT
public:
  DummyOptimizer(SearchBase* p)
    : Optimizer(p){};

  QString getIDString() const override { return "Generic"; }

  bool update(Structure* structure) override
  {
    structure->setEnergy(-1.0);
    structure->setEnthalpy(-1.0);
    return true;
  }
};

class DummyQueueInterface : public QueueInterface
{
  Q_OBJECT
public:
  explicit DummyQueueInterface(SearchBase* p)
    : QueueInterface(p)
  {
  }
  QString getIDString() const override { return "dummyqueue"; }

  bool writeFiles(Structure*, const QHash<QString, QString>&) const override
  {
    return true;
  }
  bool writeInputFiles(Structure*) const override { return true; }
  bool startJob(Structure*) override { return true; }
  bool stopJob(Structure*) override
  {
    ++stopCalls;
    return true;
  }
  QueueStatus getStatus(Structure*) const override
  {
    if (statuses.isEmpty())
      return Success;
    return statuses.takeFirst();
  }
  bool prepareForStructureUpdate(Structure*) const override { return true; }
  CommandResult runACommand(const QString&, const QString&, int = -1) const override
  {
    CommandResult result;
    result.launched = true;
    result.exitCode = 0;
    return result;
  }

  mutable QList<QueueStatus> statuses;
  int stopCalls = 0;
};

class LocalDirCreatingQueueInterface : public DummyQueueInterface
{
  Q_OBJECT
public:
  explicit LocalDirCreatingQueueInterface(SearchBase* p)
    : DummyQueueInterface(p)
  {
  }
  QString getIDString() const override { return "dirqueue"; }

  bool isReadyToSearch(QString* err) override
  {
    Q_UNUSED(err);
    return QDir().mkpath(m_search->getLocWorkDir());
  }
};

class LocalFileQueueInterface : public DummyQueueInterface
{
  Q_OBJECT
public:
  explicit LocalFileQueueInterface(SearchBase* p)
    : DummyQueueInterface(p)
  {
  }
  QString getIDString() const override { return "localfiles"; }

  bool checkIfFileExists(Structure* s, const QString& filename, bool* exists) override
  {
    *exists = QFile::exists(Common::localPath(s->getLocpath(), filename));
    return true;
  }
};

class FailingWriteQueueInterface : public DummyQueueInterface
{
  Q_OBJECT
public:
  explicit FailingWriteQueueInterface(SearchBase* p)
    : DummyQueueInterface(p)
  {
  }
  QString getIDString() const override { return "failingwritequeue"; }

  bool writeInputFiles(Structure*) const override
  {
    ++writeCalls;
    return false;
  }

  bool startJob(Structure*) override
  {
    ++startCalls;
    return true;
  }

  mutable int writeCalls = 0;
  int startCalls = 0;
};

class TestQueueManager : public QueueManager
{
  Q_OBJECT
public:
  explicit TestQueueManager(SearchBase* search)
    : QueueManager(QThread::currentThread(), search)
  {
    // Run the check loop quickly (default in search itself is 1s,
    //   here we use a shorter period to finish the test quickly).
    setCheckInterval(25);
  }

  using QueueManager::addStructureToSubmissionQueue;
  using QueueManager::handleRestartStructure;

  // Slots for this test.
  static const int kKilledSlot = QueueManager::KilledHandler;
  static const int kErrorSlot = QueueManager::ErrorHandler;

  bool tryEnterForTest(Structure* s, int slot)
  {
    return m_runningHandlers.tryStart(s, slot);
  }
  void leaveForTest(Structure* s, int slot) { m_runningHandlers.finish(s, slot); }
  bool hasAnyForTest(Structure* s) { return m_runningHandlers.hasHandlerFor(s); }
};

class ScopedContStructs
{
public:
  ScopedContStructs(SearchBase* search, uint value)
    : m_search(search), m_oldValue(search ? search->getContStructs() : 0)
  {
    if (m_search)
      m_search->setContStructs(value);
  }

  ~ScopedContStructs()
  {
    if (m_search)
      m_search->setContStructs(m_oldValue);
  }

private:
  SearchBase* m_search;
  uint m_oldValue;
};

// Since this is a pure virtual class, create a dummy derived class
class DummySearchBase : public SearchBase
{
  Q_OBJECT
public:
  DummySearchBase()
    : SearchBase(0)
  {
    setSearchIDString(DUMMYNAME);
  };

  void refreshFrontsForTest(const QList<Structure*>& structures)
  {
    for (auto* structure : structures)
      tracker()->append(structure);
    refreshParentSelectionFronts(structures);
    tracker()->reset();
  }

  bool runStartWorkflowForTest()
  {
    if (!beginSession())
      return false;
    ++initializeCalls;
    launchSession();
    return true;
  }

  // Start a test session.
  void markSessionActiveForTest() { setSessionActive(true); }

  // Set read-only mode for this test.
  void setReadOnlyForTest(bool v) { setReadOnly(v); }

  bool runAbortedWorkflowForTest()
  {
    if (!beginSession())
      return false;
    abortSession();
    return true;
  }

  bool runResumeWorkflowForTest()
  {
    if (!beginSession())
      return false;
    ++restoreCalls;
    launchSession();
    return true;
  }

  bool createSSHConnections() override
  {
    ++sshConnectionCalls;
    return !failSSHConnections;
  }

  // Override this function to allow the creation of the dummy optimizer
  std::unique_ptr<Optimizer> createOptimizer(
    const std::string& optName) override
  {
    if (optName == "dummy")
      return make_unique<DummyOptimizer>(this);

    Common::message(QString("Unknown optName: %1").arg(optName.c_str()));

    return nullptr;
  }

  std::unique_ptr<QueueInterface> createQueueInterface(const std::string& queueName) override
  {
    if (queueName == "dummyqueue")
      return make_unique<DummyQueueInterface>(this);
    if (queueName == "dirqueue")
      return make_unique<LocalDirCreatingQueueInterface>(this);
    if (queueName == "localfiles")
      return make_unique<LocalFileQueueInterface>(this);
    if (queueName == "failingwritequeue")
      return make_unique<FailingWriteQueueInterface>(this);

    Common::message(QString("Unknown queueName: %1").arg(queueName.c_str()));

    return nullptr;
  }

public slots:
  bool startSearch() override { return true; }
  void generateNewStructure() override
  {
    ++newStructureRequests;
    // Clear the pending structure request.
    queue()->structureGenerationFailed();
  }

protected:
  void setOptimizer_string(const QString&, const QString&) {}

public:
  bool failSSHConnections = false;
  int sshConnectionCalls = 0;
  std::atomic<int> newStructureRequests{0};
  int initializeCalls = 0;
  int restoreCalls = 0;
};

class TestPromptHandler : public QObject
{
  Q_OBJECT
public:
  explicit TestPromptHandler(QObject* parent = nullptr)
    : QObject(parent), booleanResponse(false),
      passwordAccepted(false),
      progressStartCalls(0),
      progressStopCalls(0),
      reportDebugCalls(0),
      reportWarningCalls(0),
      reportErrorCalls(0),
      reportMessageCalls(0)
  {
  }

  bool booleanResponse;
  bool passwordAccepted;
  QString passwordResponse;
  QString lastBooleanPrompt;
  QString lastPasswordPrompt;
  QString lastProgressLabel;
  QString lastDebug;
  QString lastWarning;
  QString lastError;
  QString lastMessage;
  int progressStartCalls;
  int progressStopCalls;
  int reportDebugCalls;
  int reportWarningCalls;
  int reportErrorCalls;
  int reportMessageCalls;

public:
  void beginProgress(const QString& label, int min, int max)
  {
    Q_UNUSED(min);
    Q_UNUSED(max);
    lastProgressLabel = label;
    ++progressStartCalls;
  }

  void endProgress() { ++progressStopCalls; }

  void updateProgress(int value, const QString& label, int min, int max)
  {
    Q_UNUSED(value);
    Q_UNUSED(min);
    Q_UNUSED(max);
    if (!label.isNull())
      lastProgressLabel = label;
  }

  bool promptForBoolean(const QString& message, bool defaultValue)
  {
    lastBooleanPrompt = message;
    Q_UNUSED(defaultValue);
    return booleanResponse;
  }

  bool promptForPassword(const QString& message, QString& newPassword)
  {
    lastPasswordPrompt = message;
    newPassword = passwordResponse;
    return passwordAccepted;
  }

  void reportDebug(const QString& text)
  {
    lastDebug = text;
    ++reportDebugCalls;
  }

  void reportWarning(const QString& text)
  {
    lastWarning = text;
    ++reportWarningCalls;
  }

  void reportError(const QString& text)
  {
    lastError = text;
    ++reportErrorCalls;
  }

  void reportMessage(const QString& text)
  {
    lastMessage = text;
    ++reportMessageCalls;
  }
};

class SearchBaseTest : public QObject
{
  Q_OBJECT

private:
  SearchBase* m_opt;

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

  void abortedSessionStaysIdle();

  void dispatchDedupesPerStructureAndSlot();

  // Tests
  void interpretKeyword();
  void interpretCrystalKeywords();
  void paretoFrontsIgnoreConstraints();
  void constraintDismissalPrecedesFailure();
  void constraintOutputAcceptsBooleanString();
  void scriptPrecheckRequiresAbsoluteExistingLocalPaths();
  void paretoFrontsRespectMaxObjectives();
  void paretoFilterZeroWeightsRemovesZeroWeightObjectives();
  void resetObjectivesClearsParetoFront();
  void runtimeInteractionDispatchesPromptsAndReports();
  void startWorkflowDoesNotRunApplicationPrecheck();
  void activeResumeWorkflowClearsSessionStartingAfterRestore();
  void unknownOptimizerQueueNamesDoNotChangeExistingOptStep();
  void queueManagerRunsSubmissionToOptimizedLifecycle();
  void queueManagerDoesNotSubmitWhenInputStagingFails();
  void restartChangesStepOnlyAfterStoppingOldQueue();
};

void SearchBaseTest::initTestCase()
{
  m_opt = new DummySearchBase();
  if (m_opt->getNumOptSteps() == 0)
    m_opt->appendOptStep();
  m_opt->setQueueInterface(0, "dummyqueue");
  m_opt->setOptimizer(0, "dummy");
}

void SearchBaseTest::cleanupTestCase()
{
  delete m_opt;
  m_opt = 0;
}

void SearchBaseTest::init() {}

void SearchBaseTest::cleanup() {}

void SearchBaseTest::interpretKeyword()
{
  const int NUMATOMS = 30;
  const int NUMSPECIES = 5;
  Q_ASSERT(NUMATOMS >= NUMSPECIES);

  const int COPTSTEP = 3;
  const int GENERATION = 4;
  const int IDNUM = 6;
  const QString FILENAME = "/tmp/fake/filename/";
  const QString REMPATH = "/now/its/remote/" + FILENAME;
  const QString USER1 = "user1";
  const QString USER2 = "user2";
  const QString USER3 = "user3";
  const QString USER4 = "user4";

  // Setup
  m_opt->setDescription(DESCRIPTION);
  m_opt->setUser1(USER1);
  m_opt->setUser2(USER2);
  m_opt->setUser3(USER3);
  m_opt->setUser4(USER4);

  Structure* s = new Structure;
  for (int i = 0; i < NUMATOMS; ++i) {
    Atoms::Atom& a = s->addAtom();
    a.setAtomicNumber((i % NUMSPECIES) + 1);
    a.setPos(Common::Vector3(i, i, i));
  }
  s->setLocpath(FILENAME);
  s->setRempath(REMPATH);
  s->setGeneration(GENERATION);
  s->setIDNumber(IDNUM);
  s->setCurrentOptStep(COPTSTEP);

  // Reference outputs
  QString coordsRef;
  QString coordsIdRef;
  QString coordsInternalFlagsRef;
  QString coordsSuffixFlagsRef;

  coordsRef =
    "H 0 0 0\nHe 1 1 1\nLi 2 2 2\nBe 3 3 3\nB 4 4 4\nH 5 5 5\nHe 6 6 6\n"
    "Li 7 7 7\nBe 8 8 8\nB 9 9 9\nH 10 10 10\nHe 11 11 11\nLi 12 12 12\n"
    "Be 13 13 13\nB 14 14 14\nH 15 15 15\nHe 16 16 16\nLi 17 17 17\n"
    "Be 18 18 18\nB 19 19 19\nH 20 20 20\nHe 21 21 21\nLi 22 22 22\n"
    "Be 23 23 23\nB 24 24 24\nH 25 25 25\nHe 26 26 26\nLi 27 27 27\n"
    "Be 28 28 28\nB 29 29 29";

  coordsIdRef =
    "H 1 0 0 0\nHe 2 1 1 1\nLi 3 2 2 2\nBe 4 3 3 3\nB 5 4 4 4\nH 1 5 5 5\n"
    "He 2 6 6 6\nLi 3 7 7 7\nBe 4 8 8 8\nB 5 9 9 9\nH 1 10 10 10\n"
    "He 2 11 11 11\nLi 3 12 12 12\nBe 4 13 13 13\nB 5 14 14 14\n"
    "H 1 15 15 15\nHe 2 16 16 16\nLi 3 17 17 17\nBe 4 18 18 18\n"
    "B 5 19 19 19\nH 1 20 20 20\nHe 2 21 21 21\nLi 3 22 22 22\n"
    "Be 4 23 23 23\nB 5 24 24 24\nH 1 25 25 25\nHe 2 26 26 26\n"
    "Li 3 27 27 27\nBe 4 28 28 28\nB 5 29 29 29";

  coordsInternalFlagsRef =
    "H 0 1 0 1 0 1\nHe 1 1 1 1 1 1\nLi 2 1 2 1 2 1\nBe 3 1 3 1 3 1\n"
    "B 4 1 4 1 4 1\nH 5 1 5 1 5 1\nHe 6 1 6 1 6 1\nLi 7 1 7 1 7 1\n"
    "Be 8 1 8 1 8 1\nB 9 1 9 1 9 1\nH 10 1 10 1 10 1\nHe 11 1 11 1 11 1\n"
    "Li 12 1 12 1 12 1\nBe 13 1 13 1 13 1\nB 14 1 14 1 14 1\n"
    "H 15 1 15 1 15 1\nHe 16 1 16 1 16 1\nLi 17 1 17 1 17 1\n"
    "Be 18 1 18 1 18 1\nB 19 1 19 1 19 1\nH 20 1 20 1 20 1\n"
    "He 21 1 21 1 21 1\nLi 22 1 22 1 22 1\nBe 23 1 23 1 23 1\n"
    "B 24 1 24 1 24 1\nH 25 1 25 1 25 1\nHe 26 1 26 1 26 1\n"
    "Li 27 1 27 1 27 1\nBe 28 1 28 1 28 1\nB 29 1 29 1 29 1";

  coordsSuffixFlagsRef =
    "H 0 0 0 1 1 1\nHe 1 1 1 1 1 1\nLi 2 2 2 1 1 1\nBe 3 3 3 1 1 1\n"
    "B 4 4 4 1 1 1\nH 5 5 5 1 1 1\nHe 6 6 6 1 1 1\nLi 7 7 7 1 1 1\n"
    "Be 8 8 8 1 1 1\nB 9 9 9 1 1 1\nH 10 10 10 1 1 1\nHe 11 11 11 1 1 1\n"
    "Li 12 12 12 1 1 1\nBe 13 13 13 1 1 1\nB 14 14 14 1 1 1\n"
    "H 15 15 15 1 1 1\nHe 16 16 16 1 1 1\nLi 17 17 17 1 1 1\n"
    "Be 18 18 18 1 1 1\nB 19 19 19 1 1 1\nH 20 20 20 1 1 1\n"
    "He 21 21 21 1 1 1\nLi 22 22 22 1 1 1\nBe 23 23 23 1 1 1\n"
    "B 24 24 24 1 1 1\nH 25 25 25 1 1 1\nHe 26 26 26 1 1 1\n"
    "Li 27 27 27 1 1 1\nBe 28 28 28 1 1 1\nB 29 29 29 1 1 1";

#define VERIFYKEYWORD(key, value)                                              \
  QVERIFY(m_opt->interpretTemplate(key, s).compare(QString(value) + "\n") == 0)

  VERIFYKEYWORD("%percent%", "%");
  VERIFYKEYWORD("%user1%", USER1);
  VERIFYKEYWORD("%user2%", USER2);
  VERIFYKEYWORD("%user3%", USER3);
  VERIFYKEYWORD("%user4%", USER4);
  VERIFYKEYWORD("%description%", DESCRIPTION);
  VERIFYKEYWORD("%coords%", coordsRef);
  VERIFYKEYWORD("%coordsId%", coordsIdRef);
  VERIFYKEYWORD("%coordsInternalFlags%", coordsInternalFlagsRef);
  VERIFYKEYWORD("%coordsSuffixFlags%", coordsSuffixFlagsRef);
  VERIFYKEYWORD("%numAtoms%", QString::number(NUMATOMS));
  VERIFYKEYWORD("%numSpecies%", QString::number(NUMSPECIES));
  VERIFYKEYWORD("%filename%", FILENAME);
  VERIFYKEYWORD("%rempath%", REMPATH);
  VERIFYKEYWORD("%gen%", QString::number(GENERATION));
  VERIFYKEYWORD("%id%", QString::number(IDNUM));
  VERIFYKEYWORD("%optStep%", QString::number(COPTSTEP));
}

void SearchBaseTest::interpretCrystalKeywords()
{
  Structure s;
  s.setCellInfo(2.0, 4.0, 8.0, 90.0, 90.0, 90.0);
  s.setLocpath("structure-1");

  Atoms::Atom& h = s.addAtom();
  h.setAtomicNumber(1);
  h.setPos(Common::Vector3(0.0, 0.0, 0.0));

  Atoms::Atom& o = s.addAtom();
  o.setAtomicNumber(8);
  o.setPos(Common::Vector3(1.0, 2.0, 4.0));

  QCOMPARE(m_opt->interpretTemplate("%a% %b% %c%", &s), QString("2 4 8\n"));
  QCOMPARE(m_opt->interpretTemplate("%alphaDeg% %betaDeg% %gammaDeg%", &s), QString("90 90 90\n"));
  QCOMPARE(m_opt->interpretTemplate("%coordsFrac%", &s), QString("H 0 0 0\nO 0.5 0.5 0.5\n"));
  QCOMPARE(m_opt->interpretTemplate("%chemicalSpeciesLabel%", &s), QString(" 1 1 H\n 2 8 O\n"));
  QCOMPARE(m_opt->interpretTemplate("%block% lattice\n%endblock%", &s),
           QString("%block lattice\n%endblock\n"));
  QCOMPARE(m_opt->interpretTemplate("literal % text and %% escape", &s),
           QString("literal % text and % escape\n"));
  QCOMPARE(m_opt->interpretTemplate("%unknown% %a% %j", &s),
           QString("%unknown% 2 %j\n"));

  const QString poscar = m_opt->interpretTemplate("%POSCAR%", &s);
  QVERIFY(poscar.contains("structure-1"));
  QVERIFY(poscar.contains("H O"));
  QVERIFY(poscar.contains("Direct"));
}

void SearchBaseTest::paretoFrontsIgnoreConstraints()
{
  DummySearchBase* dummy = static_cast<DummySearchBase*>(m_opt);
  const QList<double> minValues = {1.0, 2.0, 3.0};

  QList<Structure*> baselineStructures;
  m_opt->setOptimizationType(Search::SearchBase::OT_Pareto);
  m_opt->resetObjectives();
  m_opt->addObjective(SearchBase::Ot_Min, QString(), QString(), 1.0);
  for (double minValue : minValues) {
    Structure* s = new Structure;
    s->addAtom(1);
    s->setStrucObjValues(minValue);
    baselineStructures.append(s);
  }

  dummy->refreshFrontsForTest(baselineStructures);

  QList<int> baselineFronts;
  for (auto* structure : baselineStructures)
    baselineFronts.append(structure->getParetoFront());

  QList<Structure*> filteredStructures;
  m_opt->resetObjectives();
  m_opt->resetConstraints();
  m_opt->addObjective(SearchBase::Ot_Min, QString(), QString(), 1.0);
  m_opt->addConstraint("/tmp/filter", "filter.out");

  for (int i = 0; i < minValues.size(); ++i) {
    Structure* s = new Structure;
    s->addAtom(1);
    s->setStrucObjValues(minValues[i]);
    filteredStructures.append(s);
  }

  dummy->refreshFrontsForTest(filteredStructures);

  for (int i = 0; i < filteredStructures.size(); ++i)
    QCOMPARE(filteredStructures[i]->getParetoFront(), baselineFronts[i]);

  qDeleteAll(baselineStructures);
  qDeleteAll(filteredStructures);
  m_opt->resetObjectives();
  m_opt->resetConstraints();
  m_opt->setOptimizationType(Search::SearchBase::OT_Basic);
}

void SearchBaseTest::constraintDismissalPrecedesFailure()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  Structure structure;
  structure.setLocpath(tempDir.path());
  structure.setCurrentOptStep(0);

  QFile dismissFile(Common::localPath(tempDir.path(), "dismiss.out"));
  QVERIFY(dismissFile.open(QIODevice::WriteOnly | QIODevice::Text));
  dismissFile.write("0.0\n");
  dismissFile.close();

  QFile invalidFile(Common::localPath(tempDir.path(), "invalid.out"));
  QVERIFY(invalidFile.open(QIODevice::WriteOnly | QIODevice::Text));
  invalidFile.write("not-a-number\n");
  invalidFile.close();

  m_opt->setQueueInterface(0, "localfiles");
  m_opt->resetConstraints();
  m_opt->addConstraint(QString(), "dismiss.out");
  m_opt->addConstraint(QString(), "invalid.out");

  m_opt->finishConstraintCalculations(&structure);

  QCOMPARE(structure.getStrucConstraintState(), Structure::Cs_Dismiss);

  m_opt->resetConstraints();
  m_opt->setQueueInterface(0, "dummyqueue");
}

void SearchBaseTest::constraintOutputAcceptsBooleanString()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  QFile passFile(Common::localPath(tempDir.path(), "pass.out"));
  QVERIFY(passFile.open(QIODevice::WriteOnly | QIODevice::Text));
  passFile.write("TrUe ignored-fields\n");
  passFile.close();

  QFile dismissFile(Common::localPath(tempDir.path(), "dismiss.out"));
  QVERIFY(dismissFile.open(QIODevice::WriteOnly | QIODevice::Text));
  dismissFile.write("FALSE\n");
  dismissFile.close();

  Structure structure;
  structure.setLocpath(tempDir.path());
  structure.setCurrentOptStep(0);

  m_opt->setQueueInterface(0, "localfiles");
  m_opt->resetConstraints();
  m_opt->addConstraint(QString(), "pass.out");
  m_opt->finishConstraintCalculations(&structure);
  QCOMPARE(structure.getStrucConstraintNumber(), 1);
  QCOMPARE(structure.getStrucConstraintValues(0), 1.0);
  QCOMPARE(structure.getStrucConstraintState(), Structure::Cs_Retain);

  structure.resetStrucConstraint();
  m_opt->resetConstraints();
  m_opt->addConstraint(QString(), "dismiss.out");
  m_opt->finishConstraintCalculations(&structure);
  QCOMPARE(structure.getStrucConstraintNumber(), 1);
  QCOMPARE(structure.getStrucConstraintValues(0), 0.0);
  QCOMPARE(structure.getStrucConstraintState(), Structure::Cs_Dismiss);

  m_opt->resetConstraints();
  m_opt->setQueueInterface(0, "dummyqueue");
}

void SearchBaseTest::scriptPrecheckRequiresAbsoluteExistingLocalPaths()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  DummySearchBase opt;
  opt.appendOptStep();
  opt.setQueueInterface(0, "dummyqueue");
  opt.setOptimizer(0, "dummy");
  opt.setLocWorkDir(tempDir.path());

  QString err;
  opt.addObjective(SearchBase::Ot_Min, "relative-objective", "objective.out", 1.0);
  QVERIFY(!opt.isReadyToSearch(&err));
  QVERIFY(err.contains("absolute local path"));

  opt.resetObjectives();
  opt.addConstraint(Common::localPath(tempDir.path(), "missing-constraint"), "constraint.out");
  QVERIFY(!opt.isReadyToSearch(&err));
  QVERIFY(err.contains("was not found on the local machine"));

  QFile script(Common::localPath(tempDir.path(), "constraint-script"));
  QVERIFY(script.open(QIODevice::WriteOnly | QIODevice::Text));
  script.write("# test script\n");
  script.close();

  opt.resetConstraints();
  opt.addConstraint(script.fileName(), "constraint.out");
  QVERIFY2(opt.isReadyToSearch(&err), qPrintable(err));
}

void SearchBaseTest::runtimeInteractionDispatchesPromptsAndReports()
{
  QVERIFY(m_opt != nullptr);

  TestPromptHandler interaction;
  m_opt->setDecisionPromptHandler(
    [&interaction](const QString& message, bool defaultValue) {
      return interaction.promptForBoolean(message, defaultValue);
    });
  m_opt->setPasswordPromptHandler(
    [&interaction](const QString& message, QString& password) {
      return interaction.promptForPassword(message, password);
    });
  const int outHandlerId = Common::addOutputHandler(
    [&interaction](Common::OutputLevel level, const QString& text) {
      switch (level) {
        case Common::OutputLevel::Debug:
          interaction.reportDebug(text);
          break;
        case Common::OutputLevel::Warning:
          interaction.reportWarning(text);
          break;
        case Common::OutputLevel::Error:
          interaction.reportError(text);
          break;
        case Common::OutputLevel::Message:
          interaction.reportMessage(text);
          break;
      }
    });
  // Enable debug output.
  struct InteractionCleanup
  {
    SearchBase* opt;
    int handlerId;
    ~InteractionCleanup()
    {
      Common::removeOutputHandler(handlerId);
      opt->clearPromptHandlers();
      Common::setDebugOutputEnabled(false);
    }
  } interactionCleanup = { m_opt, outHandlerId };
  Common::setDebugOutputEnabled(true);
  QObject::connect(m_opt, &SearchBase::progressRangeChanged,
                   &interaction, &TestPromptHandler::beginProgress);
  QObject::connect(m_opt, &SearchBase::progressValueChanged,
                   &interaction, &TestPromptHandler::updateProgress);
  QObject::connect(m_opt, &SearchBase::progressEnded,
                   &interaction, &TestPromptHandler::endProgress);

  interaction.booleanResponse = true;
  const bool booleanOk = m_opt->requestBooleanDecision("bool prompt");
  QVERIFY(booleanOk);
  QCOMPARE(interaction.lastBooleanPrompt, QString("bool prompt"));

  interaction.passwordAccepted = true;
  interaction.passwordResponse = "secret";
  QString password;
  const bool passwordOk = m_opt->requestPassword("password prompt", password);
  QVERIFY(passwordOk);
  QCOMPARE(password, QString("secret"));
  QCOMPARE(interaction.lastPasswordPrompt, QString("password prompt"));

  Common::debug("debug text");
  Common::warning("warning text");
  Common::error("error text");
  Common::message("message text");
  m_opt->beginProgressUpdate("progress label", 0, 10);
  m_opt->updateProgressValue(5, "updated label", 0, 10);
  m_opt->endProgressUpdate();

  QCOMPARE(interaction.reportDebugCalls, 1);
  QCOMPARE(interaction.reportWarningCalls, 1);
  QCOMPARE(interaction.reportErrorCalls, 1);
  QCOMPARE(interaction.reportMessageCalls, 1);
  QCOMPARE(interaction.lastDebug, QString("debug text"));
  QCOMPARE(interaction.lastWarning, QString("warning text"));
  QCOMPARE(interaction.lastError, QString("error text"));
  QCOMPARE(interaction.lastMessage, QString("message text"));
  QCOMPARE(interaction.progressStartCalls, 1);
  QCOMPARE(interaction.progressStopCalls, 1);
  QCOMPARE(interaction.lastProgressLabel, QString("updated label"));
}

void SearchBaseTest::startWorkflowDoesNotRunApplicationPrecheck()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  const QString runDir = Common::localPath(tempDir.path(), "run");

  DummySearchBase opt;
  opt.appendOptStep();
  opt.setQueueInterface(0, "dirqueue");
  opt.setOptimizer(0, "dummy");
  opt.setLocWorkDir(runDir);
  opt.setRemoteQueue(true);
  opt.failSSHConnections = true;

  QVERIFY(!QFile::exists(runDir));
  QVERIFY(opt.runStartWorkflowForTest());
  QCOMPARE(opt.sshConnectionCalls, 0);
  QCOMPARE(opt.initializeCalls, 1);
  QVERIFY(!QFile::exists(runDir));
}

void SearchBaseTest::activeResumeWorkflowClearsSessionStartingAfterRestore()
{
  DummySearchBase opt;
  opt.appendOptStep();
  opt.setQueueInterface(0, "dummyqueue");
  opt.setOptimizer(0, "dummy");
  opt.setLocWorkDir(QString());
  opt.setReadOnlyForTest(false);

  QVERIFY(opt.runResumeWorkflowForTest());
  QCOMPARE(opt.restoreCalls, 1);
  QVERIFY(!opt.isSessionStarting());
}

void SearchBaseTest::paretoFrontsRespectMaxObjectives()
{
  DummySearchBase* dummy = static_cast<DummySearchBase*>(m_opt);
  m_opt->setOptimizationType(Search::SearchBase::OT_Pareto);
  m_opt->resetObjectives();
  m_opt->addObjective(SearchBase::Ot_Max, QString(), QString(), 1.0);

  Structure* lowMax = new Structure;
  lowMax->addAtom(1);
  lowMax->setStrucObjValues(1.0); // maximization objective

  Structure* highMax = new Structure;
  highMax->addAtom(1);
  highMax->setStrucObjValues(9.0); // better maximization objective

  QList<Structure*> structures;
  structures << lowMax << highMax;

  dummy->refreshFrontsForTest(structures);

  QCOMPARE(highMax->getParetoFront(), 0);
  QCOMPARE(lowMax->getParetoFront(), 1);

  qDeleteAll(structures);
  m_opt->resetObjectives();
  m_opt->setOptimizationType(Search::SearchBase::OT_Basic);
}

void SearchBaseTest::paretoFilterZeroWeightsRemovesZeroWeightObjectives()
{
  DummySearchBase* dummy = static_cast<DummySearchBase*>(m_opt);
  m_opt->resetObjectives();
  m_opt->setOptimizationType(Search::SearchBase::OT_Pareto);
  m_opt->addObjective(SearchBase::Ot_Min, QString(), QString(), 0.0);
  m_opt->addObjective(SearchBase::Ot_Min, QString(), QString(), 1.0);

  Structure* lowZeroWeightObjective = new Structure;
  lowZeroWeightObjective->addAtom(1);
  lowZeroWeightObjective->setStrucObjValues(0.0);
  lowZeroWeightObjective->setStrucObjValues(10.0);

  Structure* lowActiveObjective = new Structure;
  lowActiveObjective->addAtom(1);
  lowActiveObjective->setStrucObjValues(10.0);
  lowActiveObjective->setStrucObjValues(0.0);

  QList<Structure*> structures;
  structures << lowZeroWeightObjective << lowActiveObjective;

  m_opt->setParetoFilterZeroWeights(false);
  dummy->refreshFrontsForTest(structures);
  QCOMPARE(lowZeroWeightObjective->getParetoFront(), 0);
  QCOMPARE(lowActiveObjective->getParetoFront(), 0);

  m_opt->setParetoFilterZeroWeights(true);
  dummy->refreshFrontsForTest(structures);
  QCOMPARE(lowActiveObjective->getParetoFront(), 0);
  QCOMPARE(lowZeroWeightObjective->getParetoFront(), 1);

  qDeleteAll(structures);
  m_opt->resetObjectives();
  m_opt->setOptimizationType(Search::SearchBase::OT_Basic);
  m_opt->setParetoFilterZeroWeights(false);
}

void SearchBaseTest::resetObjectivesClearsParetoFront()
{
  Structure structure;
  structure.setParetoFront(3);
  structure.setStrucObjValues(1.0);
  structure.setStrucObjValues(2.0);

  structure.resetStrucObj();

  QCOMPARE(structure.getStrucObjNumber(), 0);
  QCOMPARE(structure.getParetoFront(), -1);
}

void SearchBaseTest::unknownOptimizerQueueNamesDoNotChangeExistingOptStep()
{
  QVERIFY(m_opt->queueInterface(0) != nullptr);
  QVERIFY(m_opt->optimizer(0) != nullptr);
  QCOMPARE(m_opt->queueInterface(0)->getIDString(), QString("dummyqueue"));
  QCOMPARE(m_opt->optimizer(0)->getIDString(), QString("Generic"));

  QVERIFY(!m_opt->setQueueInterface(0, "missingqueue"));
  QVERIFY(!m_opt->setOptimizer(0, "missingoptimizer"));

  QVERIFY(m_opt->queueInterface(0) != nullptr);
  QVERIFY(m_opt->optimizer(0) != nullptr);
  QCOMPARE(m_opt->queueInterface(0)->getIDString(), QString("dummyqueue"));
  QCOMPARE(m_opt->optimizer(0)->getIDString(), QString("Generic"));
}

void SearchBaseTest::queueManagerRunsSubmissionToOptimizedLifecycle()
{
  ScopedContStructs noPopulationRequests(m_opt, 0);

  QVERIFY(m_opt->queueInterface(0) != nullptr);
  QVERIFY(m_opt->optimizer(0) != nullptr);

  DummyQueueInterface* queue = qobject_cast<DummyQueueInterface*>(m_opt->queueInterface(0));
  QVERIFY(queue != nullptr);
  queue->statuses.clear();
  queue->statuses << QueueInterface::Running << QueueInterface::Success;

  static_cast<DummySearchBase*>(m_opt)->markSessionActiveForTest();
  TestQueueManager queueManager(m_opt);
  QList<Structure::State> observedStates;
  QMutex observedStatesMutex;
  QObject::connect(&queueManager, &QueueManager::structureUpdated,
                   &queueManager,
                   [&observedStates, &observedStatesMutex](Structure* s) {
                     QtCompat::MutexLocker locker(&observedStatesMutex);
                     observedStates.append(s->getStatus());
                   },
                   Qt::DirectConnection);
  QObject::connect(&queueManager, &QueueManager::structureSubmitted,
                   &queueManager,
                   [&observedStates, &observedStatesMutex](Structure*) {
                     QtCompat::MutexLocker locker(&observedStatesMutex);
                     observedStates.append(Structure::Submitted);
                   },
                   Qt::DirectConnection);
  QObject::connect(&queueManager, &QueueManager::structureFinished,
                   &queueManager,
                   [&observedStates, &observedStatesMutex](Structure*) {
                     QtCompat::MutexLocker locker(&observedStatesMutex);
                     observedStates.append(Structure::Optimized);
                   },
                   Qt::DirectConnection);

  Structure structure;
  structure.setGeneration(1);
  structure.setIDNumber(1);
  structure.setCurrentOptStep(0);
  structure.setStatus(Structure::Empty);

  queueManager.addStructureToSubmissionQueue(&structure);
  QTRY_COMPARE(structure.getStatus(), Structure::Optimized);
  QTRY_COMPARE(queueManager.getAllRunningStructures().size(), 0);

  QList<Structure::State> states;
  {
    QtCompat::MutexLocker locker(&observedStatesMutex);
    states = observedStates;
  }

  const QList<Structure::State> expectedStates = QList<Structure::State>()
      << Structure::Submitted
      << Structure::InProcess
      << Structure::StepOptimized
      << Structure::Optimized;

  int searchFrom = 0;
  for (Structure::State expected : expectedStates) {
    const int index = states.indexOf(expected, searchFrom);
    QVERIFY2(index >= 0,
             qPrintable(QString("Queue lifecycle did not emit state %1")
                           .arg(static_cast<int>(expected))));
    searchFrom = index + 1;
  }
}

void SearchBaseTest::queueManagerDoesNotSubmitWhenInputStagingFails()
{
  ScopedContStructs noPopulationRequests(m_opt, 0);

  QVERIFY(m_opt->setQueueInterface(0, "failingwritequeue"));
  FailingWriteQueueInterface* queue =
    qobject_cast<FailingWriteQueueInterface*>(m_opt->queueInterface(0));
  QVERIFY(queue != nullptr);

  static_cast<DummySearchBase*>(m_opt)->markSessionActiveForTest();
  TestQueueManager queueManager(m_opt);
  Structure structure;
  structure.setGeneration(1);
  structure.setIDNumber(1);
  structure.setCurrentOptStep(0);
  structure.setStatus(Structure::Empty);

  queueManager.addStructureToSubmissionQueue(&structure);

  QTRY_COMPARE(queue->writeCalls, 1);
  QTRY_COMPARE(structure.getStatus(), Structure::Error);
  QCOMPARE(queue->startCalls, 0);
  QCOMPARE(queueManager.getAllRunningStructures().size(), 0);

  QVERIFY(m_opt->setQueueInterface(0, "dummyqueue"));
}

void SearchBaseTest::restartChangesStepOnlyAfterStoppingOldQueue()
{
  DummySearchBase opt;
  if (opt.getNumOptSteps() == 0)
    opt.appendOptStep();
  opt.appendOptStep();
  QVERIFY(opt.setQueueInterface(0, "dummyqueue"));
  QVERIFY(opt.setOptimizer(0, "dummy"));
  QVERIFY(opt.setQueueInterface(1, "dummyqueue"));
  QVERIFY(opt.setOptimizer(1, "dummy"));

  DummyQueueInterface* oldQueue =
    qobject_cast<DummyQueueInterface*>(opt.queueInterface(0));
  DummyQueueInterface* newQueue =
    qobject_cast<DummyQueueInterface*>(opt.queueInterface(1));
  QVERIFY(oldQueue != nullptr);
  QVERIFY(newQueue != nullptr);

  Structure structure;
  TestQueueManager queueManager(&opt);
  structure.setGeneration(1);
  structure.setIDNumber(1);
  structure.setCurrentOptStep(0);
  structure.setRestartOptStep(1);
  structure.setStatus(Structure::Restart);

  queueManager.handleRestartStructure(&structure);

  QCOMPARE(oldQueue->stopCalls, 1);
  QCOMPARE(newQueue->stopCalls, 0);
  QCOMPARE(structure.getCurrentOptStep(), 1u);
  QCOMPARE(structure.getRestartOptStep(), -1);
  QTRY_COMPARE(structure.getStatus(), Structure::WaitingForOptimization);
}

void SearchBaseTest::dispatchDedupesPerStructureAndSlot()
{
  TestQueueManager queueManager(m_opt);
  Structure a;
  Structure b;

  // Do not run the same work twice.
  QVERIFY(queueManager.tryEnterForTest(&a, TestQueueManager::kKilledSlot));
  QVERIFY(!queueManager.tryEnterForTest(&a, TestQueueManager::kKilledSlot));

  // Different work may run at the same time.
  QVERIFY(queueManager.tryEnterForTest(&a, TestQueueManager::kErrorSlot));
  QVERIFY(queueManager.hasAnyForTest(&a));
  QVERIFY(!queueManager.hasAnyForTest(&b));

  queueManager.leaveForTest(&a, TestQueueManager::kKilledSlot);
  QVERIFY(queueManager.hasAnyForTest(&a)); // ErrorSlot still in flight
  queueManager.leaveForTest(&a, TestQueueManager::kErrorSlot);
  QVERIFY(!queueManager.hasAnyForTest(&a));

  // Run the work again.
  QVERIFY(queueManager.tryEnterForTest(&a, TestQueueManager::kKilledSlot));
  queueManager.leaveForTest(&a, TestQueueManager::kKilledSlot);
}

void SearchBaseTest::abortedSessionStaysIdle()
{
  DummySearchBase opt;
  // Run the loop quickly.
  opt.queue()->setCheckInterval(25);

  // Abort the session.
  QVERIFY(opt.runAbortedWorkflowForTest());
  QTest::qWait(200);
  QCOMPARE(opt.newStructureRequests.load(), 0);

  // Conclude the session.
  QVERIFY(opt.runStartWorkflowForTest());
  QTRY_VERIFY(opt.newStructureRequests.load() > 0);
}

QTEST_MAIN(SearchBaseTest)

#include "searchbasetest.moc"

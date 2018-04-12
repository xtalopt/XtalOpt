/**********************************************************************
  OptBaseTest - OptBaseTest class provides unit testing for OptBase

  Copyright (C) 2011 David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 **********************************************************************/

#include <globalsearch/optbase.h>

#include <globalsearch/optimizer.h>
#include <globalsearch/structure.h>
#include <globalsearch/utilities/makeunique.h>

#include <QtTest>

using namespace GlobalSearch;

const QString DUMMYNAME = "Dummy";
const QString DESCRIPTION = "Description";

// Dummy optimizer for user value keyword checking
class DummyOptimizer : public Optimizer
{
  Q_OBJECT
public:
  DummyOptimizer(OptBase* p)
    : Optimizer(p){};
};

// Since this is a pure virtual class, create a dummy derived class
class DummyOptBase : public OptBase
{
  Q_OBJECT
public:
  DummyOptBase()
    : OptBase(0)
  {
    m_idString = DUMMYNAME;
  };

  // Override this function to allow the creation of the dummy optimizer
  std::unique_ptr<Optimizer> createOptimizer(
    const std::string& optName) override
  {
    if (optName == "dummy")
      return make_unique<DummyOptimizer>(this);

    qDebug() << "Error in" << __FUNCTION__
             << ": unknown optName:" << optName.c_str();
    return nullptr;
  }

public slots:
  bool startSearch() override { return true; }
  bool checkLimits() override { return true; }
  void readRuntimeOptions() override {}

protected:
  void setOptimizer_string(const QString&, const QString&) {}
};

class OptBaseTest : public QObject
{
  Q_OBJECT

private:
  OptBase* m_opt;

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
  void setIsStartingTrue();
  void setIsStartingFalse();
  void getIDString();
  void getProbabilityList();
  void interpretKeyword();
};

void OptBaseTest::initTestCase()
{
  m_opt = new DummyOptBase();
  if (m_opt->getNumOptSteps() == 0)
    m_opt->appendOptStep();
  m_opt->setOptimizer(0, "dummy");
}

void OptBaseTest::cleanupTestCase()
{
  delete m_opt;
  m_opt = 0;
}

void OptBaseTest::init() {}

void OptBaseTest::cleanup() {}

void OptBaseTest::setIsStartingTrue()
{
  m_opt->isStarting = false;
  m_opt->setIsStartingTrue();
  QVERIFY(m_opt->isStarting);
}

void OptBaseTest::setIsStartingFalse()
{
  m_opt->isStarting = true;
  m_opt->setIsStartingFalse();
  QVERIFY(!m_opt->isStarting);
}

void OptBaseTest::getIDString()
{
  QVERIFY(m_opt->getIDString().compare(DUMMYNAME) == 0);
}

void OptBaseTest::getProbabilityList()
{
  const double minE = 1.0;
  const double maxE = 100.0;
  const double hardnessWeight = 0.0;

  // Empty / short lists
  double enthalpy = minE - 1.0; // subtract one since we use ++enthalpy first.
  QList<Structure*> structures;
  for (int listSize = 0; listSize <= 2; ++listSize) {
    while (structures.size() < listSize) {
      Structure* s = new Structure;
      // We need at least one atom for the probability list calculation
      s->addAtom(1);
      s->setEnthalpy(++enthalpy);
      structures.append(s);
    }
    QVERIFY(m_opt->getProbabilityList(structures, 0, hardnessWeight).isEmpty());
  }

  // Fill to 100 structures
  while (structures.size() < static_cast<int>(maxE)) {
    Structure* s = new Structure;
    // We need at least one atom for the probability list calculation
    s->addAtom(1);
    s->setEnthalpy(++enthalpy);
    structures.append(s);
  }

  // Check probabilities
  auto probs =
    m_opt->getProbabilityList(structures, structures.size(), hardnessWeight);
  // reference probabilities
  QList<double> refProbs;
  refProbs << 0 << 0.00020202 << 0.000606061 << 0.00121212 << 0.0020202
           << 0.0030303 << 0.00424242 << 0.00565657 << 0.00727273 << 0.00909091
           << 0.0111111 << 0.0133333 << 0.0157576 << 0.0183838 << 0.0212121
           << 0.0242424 << 0.0274747 << 0.0309091 << 0.0345455 << 0.0383838
           << 0.0424242 << 0.0466667 << 0.0511111 << 0.0557576 << 0.0606061
           << 0.0656566 << 0.0709091 << 0.0763636 << 0.0820202 << 0.0878788
           << 0.0939394 << 0.100202 << 0.106667 << 0.113333 << 0.120202
           << 0.127273 << 0.134545 << 0.14202 << 0.149697 << 0.157576
           << 0.165657 << 0.173939 << 0.182424 << 0.191111 << 0.2 << 0.209091
           << 0.218384 << 0.227879 << 0.237576 << 0.247475 << 0.257576
           << 0.267879 << 0.278384 << 0.289091 << 0.3 << 0.311111 << 0.322424
           << 0.333939 << 0.345657 << 0.357576 << 0.369697 << 0.38202
           << 0.394545 << 0.407273 << 0.420202 << 0.433333 << 0.446667
           << 0.460202 << 0.473939 << 0.487879 << 0.50202 << 0.516364
           << 0.530909 << 0.545657 << 0.560606 << 0.575758 << 0.591111
           << 0.606667 << 0.622424 << 0.638384 << 0.654545 << 0.670909
           << 0.687475 << 0.704242 << 0.721212 << 0.738384 << 0.755758
           << 0.773333 << 0.791111 << 0.809091 << 0.827273 << 0.845657
           << 0.864242 << 0.88303 << 0.90202 << 0.921212 << 0.940606 << 0.960202
           << 0.98 << 1;

  QVERIFY(probs.size() == structures.size());
  QVERIFY(probs.size() == refProbs.size());

  for (int i = 0; i < probs.size(); ++i)
    QVERIFY(fabs(probs[i].second - refProbs[i]) < 1e-5);


  // All equal
  for (QList<Structure*>::iterator it = structures.begin(),
                                   it_end = structures.end();
       it != it_end;
       ++it) {
    (*it)->setEnthalpy(0.0);
  }

  probs =
    m_opt->getProbabilityList(structures, structures.size(), hardnessWeight);
  double dref = 1.0 / static_cast<double>(structures.size());
  double ref = 0.0;

  QVERIFY(probs.size() == structures.size());

  for (auto it = probs.begin(), it_end = probs.end(); it != it_end; ++it) {
    QVERIFY(fabs((*it).second - ref) < 1e-5);
    ref += dref;
  }

  // cleanup
  qDeleteAll(structures);
}

void OptBaseTest::interpretKeyword()
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
  m_opt->description = DESCRIPTION;
  m_opt->setUser1(USER1.toStdString());
  m_opt->setUser2(USER2.toStdString());
  m_opt->setUser3(USER3.toStdString());
  m_opt->setUser4(USER4.toStdString());

  Structure* s = new Structure;
  for (int i = 0; i < NUMATOMS; ++i) {
    Atom& a = s->addAtom();
    a.setAtomicNumber((i % NUMSPECIES) + 1);
    a.setPos(Eigen::Vector3d(i, i, i));
  }
  s->setFileName(FILENAME);
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

QTEST_MAIN(OptBaseTest)

#include "optbasetest.moc"

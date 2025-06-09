/**********************************************************************
  SearchBaseTest - SearchBaseTest class provides unit testing for SearchBase

  Copyright (C) 2011 David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 **********************************************************************/

#include <globalsearch/searchbase.h>

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
  DummyOptimizer(SearchBase* p)
    : Optimizer(p){};
};

// Since this is a pure virtual class, create a dummy derived class
class DummySearchBase : public SearchBase
{
  Q_OBJECT
public:
  DummySearchBase()
    : SearchBase(0)
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

  std::vector<double> getReferenceEnergiesVector() override
  {
    // Just return an empty vector for the dummy case
    return {};
  }

  QList<QString> getChemicalSystem() const override
  {
    // Just return an empty list for the dummy case
    return {};
  }

public slots:
  bool startSearch() override { return true; }
  bool checkLimits() override { return true; }
  void readRuntimeOptions() override {}

protected:
  void setOptimizer_string(const QString&, const QString&) {}
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

  // Tests
  void setIsStartingTrue();
  void setIsStartingFalse();
  void getIDString();
  void getProbabilityList();
  void interpretKeyword();
};

void SearchBaseTest::initTestCase()
{
  m_opt = new DummySearchBase();
  if (m_opt->getNumOptSteps() == 0)
    m_opt->appendOptStep();
  m_opt->setOptimizer(0, "dummy");
}

void SearchBaseTest::cleanupTestCase()
{
  delete m_opt;
  m_opt = 0;
}

void SearchBaseTest::init() {}

void SearchBaseTest::cleanup() {}

void SearchBaseTest::setIsStartingTrue()
{
  m_opt->isStarting = false;
  m_opt->setIsStartingTrue();
  QVERIFY(m_opt->isStarting);
}

void SearchBaseTest::setIsStartingFalse()
{
  m_opt->isStarting = true;
  m_opt->setIsStartingFalse();
  QVERIFY(!m_opt->isStarting);
}

void SearchBaseTest::getIDString()
{
  QVERIFY(m_opt->getIDString().compare(DUMMYNAME) == 0);
}

void SearchBaseTest::getProbabilityList()
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
      s->setDistAboveHull(++enthalpy);
      structures.append(s);
    }
    QVERIFY(m_opt->getProbabilityList(structures, 0).isEmpty());
  }

  // Fill to 100 structures
  while (structures.size() < static_cast<int>(maxE)) {
    Structure* s = new Structure;
    // We need at least one atom for the probability list calculation
    s->addAtom(1);
    s->setDistAboveHull(++enthalpy);
    structures.append(s);
  }

  // Check probabilities
  auto probs =
    m_opt->getProbabilityList(structures, structures.size());

  // reference probabilities
  QList<double> refProbs;
  refProbs <<   0.00000000  <<   0.00020202  <<   0.00060606  <<   0.00121212  <<   0.00202020  <<
                0.00303030  <<   0.00424242  <<   0.00565656  <<   0.00727272  <<   0.00909090  <<
                0.01111110  <<   0.01333332  <<   0.01575756  <<   0.01838382  <<   0.02121210  <<
                0.02424240  <<   0.02747472  <<   0.03090906  <<   0.03454542  <<   0.03838380  <<
                0.04242420  <<   0.04666662  <<   0.05111106  <<   0.05575752  <<   0.06060600  <<
                0.06565650  <<   0.07090902  <<   0.07636356  <<   0.08202012  <<   0.08787870  <<
                0.09393930  <<   0.10020192  <<   0.10666656  <<   0.11333322  <<   0.12020190  <<
                0.12727260  <<   0.13454532  <<   0.14202006  <<   0.14969682  <<   0.15757560  <<
                0.16565640  <<   0.17393922  <<   0.18242406  <<   0.19111092  <<   0.19999980  <<
                0.20909070  <<   0.21838362  <<   0.22787856  <<   0.23757552  <<   0.24747450  <<
                0.25757552  <<   0.26787856  <<   0.27838362  <<   0.28909070  <<   0.29999980  <<
                0.31111092  <<   0.32242406  <<   0.33393922  <<   0.34565640  <<   0.35757560  <<
                0.36969682  <<   0.38202006  <<   0.39454532  <<   0.40727260  <<   0.42020190  <<
                0.43333322  <<   0.44666656  <<   0.46020192  <<   0.47393930  <<   0.48787870  <<
                0.50202012  <<   0.51636356  <<   0.53090902  <<   0.54565650  <<   0.56060600  <<
                0.57575752  <<   0.59111106  <<   0.60666662  <<   0.62242420  <<   0.63838380  <<
                0.65454542  <<   0.67090906  <<   0.68747472  <<   0.70424240  <<   0.72121210  <<
                0.73838382  <<   0.75575756  <<   0.77333332  <<   0.79111110  <<   0.80909090  <<
                0.82727272  <<   0.84565656  <<   0.86424242  <<   0.88303030  <<   0.90202020  <<
                0.92121212  <<   0.94060606  <<   0.96020202  <<   0.98000000  <<   1.00000000;

  QVERIFY(probs.size() == structures.size());
  QVERIFY(probs.size() == refProbs.size());

  for (int i = 0; i < probs.size(); ++i)
    QVERIFY(fabs(probs[i].second - refProbs[i]) < 1e-5);


  // All equal
  for (QList<Structure*>::iterator it = structures.begin(),
                                   it_end = structures.end();
       it != it_end;
       ++it) {
    (*it)->setDistAboveHull(0.0);
  }

  probs =
    m_opt->getProbabilityList(structures, structures.size());
  double dref = 1.0 / static_cast<double>(structures.size());
  double ref = 0.01;

  QVERIFY(probs.size() == structures.size());

  for (auto it = probs.begin(), it_end = probs.end(); it != it_end; ++it) {
    QVERIFY(fabs((*it).second - ref) < 1e-5);
    ref += dref;
  }

  // cleanup
  qDeleteAll(structures);
}

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

QTEST_MAIN(SearchBaseTest)

#include "searchbasetest.moc"

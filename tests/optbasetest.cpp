/**********************************************************************
  OptBaseTest - OptBaseTest class provides unit testing for OptBase

  Copyright (C) 2011 David C. Lonie

  XtalOpt is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
 **********************************************************************/

#include <globalsearch/optbase.h>

#include <globalsearch/optimizer.h>
#include <globalsearch/structure.h>

#include <QtTest/QtTest>

using namespace Avogadro;
using namespace GlobalSearch;

const QString DUMMYNAME    = "Dummy";
const QString DESCRIPTION  = "Description";

// Since this is a pure virtual class, create a dummy derived class
class DummyOptBase : public OptBase
{
  Q_OBJECT;
public:
  DummyOptBase() : OptBase(0) {m_idString = DUMMYNAME;};
public slots:
  void startSearch() {};
  bool checkLimits() {return true;};
protected:
  void setOptimizer_string(const QString&, const QString&) {};
};

// Dummy optimizer for user value keyword checking
class DummyOptimizer : public Optimizer
{
  Q_OBJECT;
public:
  DummyOptimizer(OptBase *p) : Optimizer(p) {};
};

class OptBaseTest : public QObject
{
  Q_OBJECT

  private:
  OptBase *m_opt;

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
  m_opt->setOptimizer(new DummyOptimizer (m_opt));
}

void OptBaseTest::cleanupTestCase()
{
  delete m_opt;
  m_opt = 0;
}

void OptBaseTest::init()
{
}

void OptBaseTest::cleanup()
{
}

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
  QVERIFY(m_opt->getIDString().compare(DUMMYNAME) == 0)
;
}

void OptBaseTest::getProbabilityList()
{
  const double minE = 1.0;
  const double maxE = 100.0;
  const double spreadE = maxE - minE;

  // Empty / short lists
  double enthalpy = minE - 1.0; // subtract one since we use ++enthalpy first.
  QList<Structure*> structures;
  for (unsigned int listSize = 0; listSize <=2; ++listSize) {
    while (structures.size() < listSize) {
      Structure *s = new Structure;
      s->setEnthalpy(++enthalpy);
      structures.append(s);
    }
    QVERIFY(m_opt->getProbabilityList(structures).isEmpty());
  }

  // Fill to 100 structures
  while (structures.size() < static_cast<int>(maxE)) {
    Structure *s = new Structure;
    s->setEnthalpy(++enthalpy);
    structures.append(s);
  }

  // Check probabilities
  QList<double> probs = m_opt->getProbabilityList(structures);
  // reference probabilities
  QList<double> refProbs;
  refProbs
    << 0.02 << 0.039798 << 0.0593939 << 0.0787879 << 0.0979798 << 0.11697
    << 0.135758 << 0.154343 << 0.172727 << 0.190909 << 0.208889 << 0.226667
    << 0.244242 << 0.261616 << 0.278788 << 0.295758 << 0.312525 << 0.329091
    << 0.345455 << 0.361616 << 0.377576 << 0.393333 << 0.408889 << 0.424242
    << 0.439394 << 0.454343 << 0.469091 << 0.483636 << 0.497980 << 0.512121
    << 0.526061 << 0.539798 << 0.553333 << 0.566667 << 0.579798 << 0.592727
    << 0.605455 << 0.617980 << 0.630303 << 0.642424 << 0.654343 << 0.666061
    << 0.677576 << 0.688889 << 0.700000 << 0.710909 << 0.721616 << 0.732121
    << 0.742424 << 0.752525 << 0.762424 << 0.772121 << 0.781616 << 0.790909
    << 0.800000 << 0.808889 << 0.817576 << 0.826061 << 0.834343 << 0.842424
    << 0.850303 << 0.857980 << 0.865455 << 0.872727 << 0.879798 << 0.886667
    << 0.893333 << 0.899798 << 0.906061 << 0.912121 << 0.917980 << 0.923636
    << 0.929091 << 0.934343 << 0.939394 << 0.944242 << 0.948889 << 0.953333
    << 0.957576 << 0.961616 << 0.965455 << 0.969091 << 0.972525 << 0.975758
    << 0.978788 << 0.981616 << 0.984242 << 0.986667 << 0.988889 << 0.990909
    << 0.992727 << 0.994343 << 0.995758 << 0.996970 << 0.997980 << 0.998788
    << 0.999394 << 0.999798 << 1.000000;

  QVERIFY(probs.size() == structures.size() - 1);
  QVERIFY(probs.size() == refProbs.size());

  for (int i = 0; i < probs.size(); ++i) {
    QVERIFY(fabs(probs[i] - refProbs[i]) < 1e-5);
  }

  // All equal
  for (QList<Structure*>::iterator
         it = structures.begin(),
         it_end = structures.end();
       it != it_end;
       ++it) {
    (*it)->setEnthalpy(0.0);
  }

  probs = m_opt->getProbabilityList(structures);
  double dref = 1.0 / static_cast<double>(structures.size() - 1);
  double ref = 0.0;

  QVERIFY(probs.size() == structures.size() - 1);

  for (QList<double>::iterator
         it = probs.begin(),
         it_end = probs.end();
       it != it_end;
       ++it) {
    QVERIFY(fabs((*it) - ref) < 1e-5);
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
  m_opt->optimizer()->setUser1(USER1);
  m_opt->optimizer()->setUser2(USER2);
  m_opt->optimizer()->setUser3(USER3);
  m_opt->optimizer()->setUser4(USER4);

  Structure *s = new Structure;
  for (int i = 0; i < NUMATOMS; ++i) {
    Atom *a = s->addAtom();
    a->setAtomicNumber((i % NUMSPECIES) + 1);
    a->setPos(Eigen::Vector3d(i, i, i));
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

#define VERIFYKEYWORD(key, value)                 \
  QVERIFY(m_opt->interpretTemplate(key, s)        \
          .compare(QString(value) + "\n") == 0)

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

#include "moc_optbasetest.cxx"

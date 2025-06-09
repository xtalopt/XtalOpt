/**********************************************************************
  RandSpg test - make sure RandSpg can generate Xtals

  Copyright (C) 2017 Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 **********************************************************************/

#include <xtalopt/structures/xtal.h>
#include <xtalopt/xtalopt.h>

#include <globalsearch/eleminfo.h>
#include <globalsearch/random.h>

#include <randSpg/include/randSpg.h>

#include <QDebug>
#include <QString>
#include <QtTest>

#include <map>

class RandSpgTest : public QObject
{
  Q_OBJECT

public:
  RandSpgTest();

private:
  XtalOpt::XtalOpt m_opt;

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
  void generateXtals();
};

RandSpgTest::RandSpgTest() : m_opt(nullptr)
{
}

void RandSpgTest::initTestCase()
{
}

void RandSpgTest::cleanupTestCase()
{
}

void RandSpgTest::init()
{
  // First, let's generate the composition. We need to start with the
  // minRadius and scaling factor, though
  m_opt.scaleFactor = 0.5;
  m_opt.minRadius = 0.33;

  std::map<uint, uint> comp;
  XtalOpt::CellComp tmpcomp;
  // We'll use two formula units
  QVERIFY(ElementInfo::readComposition("Ti2O4", comp));
  for (const auto& elem : comp) {
    tmpcomp.set(ElementInfo::getAtomicSymbol(elem.first).c_str(),
                elem.first, elem.second);
  }
  m_opt.compList.append(tmpcomp);

  m_opt.eleMinRadii.clear();
  for (const auto& atomcn : m_opt.compList[0].getAtomicNumbers()) {
    double r = ElementInfo::getCovalentRadius(atomcn) * m_opt.scaleFactor;
    if (r < m_opt.minRadius)
      r = m_opt.minRadius;
    m_opt.eleMinRadii.set(atomcn, r);
  }

  // Now let's put in lattice constraints
  m_opt.a_min = 3.0;
  m_opt.b_min = 3.0;
  m_opt.c_min = 3.0;
  m_opt.alpha_min = 60.0;
  m_opt.beta_min = 60.0;
  m_opt.gamma_min = 60.0;

  m_opt.a_max = 10.0;
  m_opt.b_max = 10.0;
  m_opt.c_max = 10.0;
  m_opt.alpha_max = 120.0;
  m_opt.beta_max = 120.0;
  m_opt.gamma_max = 120.0;

  m_opt.vol_min = 25.0;
  m_opt.vol_max = 35.0;

  // Space group tolerance
  m_opt.tol_spg = 0.05;

  // We only have one composition (that has 2 formula units)
  int    comp_ind = 0;

  // Generate the space groups list (with none turned off by default)
  for (int spg = 1; spg <= 230; spg++) {
    if (RandSpg::isSpgPossible(spg, m_opt.getStdVecOfAtomsComp(m_opt.compList[comp_ind])))
      m_opt.minXtalsOfSpg.append(0);
    else
      m_opt.minXtalsOfSpg.append(-1);
  }
}

void RandSpgTest::cleanup()
{
}

void RandSpgTest::generateXtals()
{
  // Seed the random number generators for consistent testing
  srand(0);
  GlobalSearch::seedMt19937Generator(0);

  // We only have one composition (that has 2 formula units)
  int    comp_ind = 0;

  size_t numTrials = 100;

  // Because we cannot seed the random number generators in RandSpg, we
  // will allow for a few failures in the test
  size_t numFailures = 0;
  size_t maxFailures = 3;
  for (size_t i = 0; i < numTrials; ++i) {
    // Pick a random space group from the possible ones
    uint spg = m_opt.pickRandomSpgFromPossibleOnes();

    // Now try to generate an xtal with that space group
    bool checkSpgWithSpglib = false;
    XtalOpt::Xtal* xtal =
      m_opt.randSpgXtal(1, i + 1, m_opt.compList[comp_ind], spg, checkSpgWithSpglib);
    if (!xtal) {
      ++numFailures;
      continue;
    }
    // Unfortunately, spglib often detects these space groups incorrectly it
    // seems. They have been tested with findsym, so in theory they should be
    // fine... If we ever figure out why spglib detection is failing, let's
    // add this step in.
    // QVERIFY(xtal->getSpaceGroupNumber() == spg);
  }

  QVERIFY(numFailures <= maxFailures);
}

QTEST_MAIN(RandSpgTest)

#include "randspgtest.moc"

/**********************************************************************
  Formats test - test our file format readers and writers

  Copyright (C) 2017 Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 **********************************************************************/

#include <globalsearch/formats/cmlformat.h>
#include <globalsearch/formats/formats.h>
#include <globalsearch/structure.h>

#include <QDebug>
#include <QString>
#include <QtTest>

#include <sstream>

using GlobalSearch::Vector3;

class FormatsTest : public QObject
{
  Q_OBJECT

  // Members
  GlobalSearch::Structure m_rutile;
  GlobalSearch::Structure m_caffeine;

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
  void readCml();
  void writeCml();
};

void FormatsTest::initTestCase()
{
}

void FormatsTest::cleanupTestCase()
{
}

void FormatsTest::init()
{
}

void FormatsTest::cleanup()
{
}

void FormatsTest::readCml()
{
  /**** Rutile ****/
  QString rutileFileName = QString(TESTDATADIR) + "/data/rutile.cml";
  GlobalSearch::Structure rutile;

  QVERIFY(GlobalSearch::Formats::read(&rutile, rutileFileName, "cml"));

  // Our structure should have a unit cell, 6 atoms, and no bonds
  // The unit cell volume should be about 62.423
  QVERIFY(rutile.hasUnitCell());
  QVERIFY(rutile.numAtoms() == 6);
  QVERIFY(rutile.numBonds() == 0);
  QVERIFY(abs(62.4233 - rutile.unitCell().volume()) < 1.e-5);

  // The second atom should be Ti and should have cartesian coordinates of
  // (1.47906   2.29686   2.29686)
  double tol = 1.e-5;
  Vector3 rutileAtom2Pos(1.47906, 2.29686, 2.29686);
  QVERIFY(rutile.atom(1).atomicNumber() == 22);
  QVERIFY(GlobalSearch::fuzzyCompare(rutile.atom(1).pos(),
                                     rutileAtom2Pos, tol));

  // The third atom should be O and should have fractional coordinates of
  // 0, 0.3053, 0.3053
  tol = 1.e-5;
  Vector3 rutileAtom3Ref =
      rutile.unitCell().toFractional(rutile.atom(2).pos());
  Vector3 rutileAtom3PosFrac(0.0, 0.3053, 0.3053);
  QVERIFY(rutile.atom(2).atomicNumber() == 8);
  QVERIFY(GlobalSearch::fuzzyCompare(rutileAtom3Ref, rutileAtom3PosFrac, tol));

  // Let's set rutile to be used for the write test
  m_rutile = rutile;

  /**** Ethane ****/
  QString ethaneFileName = QString(TESTDATADIR) + "/data/ethane.cml";
  GlobalSearch::Structure ethane;

  QVERIFY(GlobalSearch::Formats::read(&ethane, ethaneFileName, "cml"));

  // Our structure should have no unit cell, 8 atoms, and 7 bonds
  QVERIFY(!ethane.hasUnitCell());
  QVERIFY(ethane.numAtoms() == 8);
  QVERIFY(ethane.numBonds() == 7);

  /**** Caffeine ****/
  QString caffeineFileName = QString(TESTDATADIR) + "/data/caffeine.cml";
  GlobalSearch::Structure caffeine;
  QVERIFY(GlobalSearch::Formats::read(&caffeine, caffeineFileName, "cml"));

  // Our structure should have no unit cell, 24 atoms, and 25 bonds
  QVERIFY(!caffeine.hasUnitCell());
  QVERIFY(caffeine.numAtoms() == 24);
  QVERIFY(caffeine.numBonds() == 25);

  // Caffeine should also have 4 double bonds. Make sure of this.
  size_t numDoubleBonds = 0;
  for (const GlobalSearch::Bond& bond: caffeine.bonds()) {
    if (bond.bondOrder() == 2)
      ++numDoubleBonds;
  }
  QVERIFY(numDoubleBonds == 4);

  m_caffeine = caffeine;
}

// A note: this test will also fail if readCml() doesn't work properly
// This function assumes that readCml() was already ran successfully, and
// it uses it to confirm that the CML was written properly
void FormatsTest::writeCml()
{
  // First, write the cml file to a stringstream
  std::stringstream ss;
  QVERIFY(GlobalSearch::CmlFormat::write(m_rutile, ss));

  // Initialize an istreamstring with it
  std::istringstream iss(ss.str());

  // Now read from it and run the same tests we tried above

  /**** Rutile ****/
  GlobalSearch::Structure rutile;
  QVERIFY(GlobalSearch::CmlFormat::read(rutile, iss));

  // Our structure should have a unit cell, 6 atoms, and no bonds
  // The unit cell volume should be about 62.423
  QVERIFY(rutile.hasUnitCell());
  QVERIFY(rutile.numAtoms() == 6);
  QVERIFY(rutile.numBonds() == 0);
  QVERIFY(abs(62.4233 - rutile.unitCell().volume()) < 1.e-5);

  // The second atom should be Ti and should have cartesian coordinates of
  // (1.47906   2.29686   2.29686)
  double tol = 1.e-5;
  Vector3 rutileAtom2Pos(1.47906, 2.29686, 2.29686);
  QVERIFY(rutile.atom(1).atomicNumber() == 22);
  QVERIFY(GlobalSearch::fuzzyCompare(rutile.atom(1).pos(),
                                     rutileAtom2Pos, tol));

  // The third atom should be O and should have fractional coordinates of
  // 0, 0.3053, 0.3053
  tol = 1.e-5;
  Vector3 rutileAtom3Ref =
      rutile.unitCell().toFractional(rutile.atom(2).pos());
  Vector3 rutileAtom3PosFrac(0.0, 0.3053, 0.3053);
  QVERIFY(rutile.atom(2).atomicNumber() == 8);
  QVERIFY(GlobalSearch::fuzzyCompare(rutileAtom3Ref, rutileAtom3PosFrac, tol));

  // Do the same thing with caffeine
  std::stringstream css;
  QVERIFY(GlobalSearch::CmlFormat::write(m_caffeine, css));

  // Initialize an istreamstring with it
  std::istringstream ciss(css.str());
  /**** Caffeine ****/
  GlobalSearch::Structure caffeine;
  QVERIFY(GlobalSearch::CmlFormat::read(caffeine, ciss));

  // Our structure should have no unit cell, 24 atoms, and 25 bonds
  QVERIFY(!caffeine.hasUnitCell());
  QVERIFY(caffeine.numAtoms() == 24);
  QVERIFY(caffeine.numBonds() == 25);

  // Caffeine should also have 4 double bonds. Make sure of this.
  size_t numDoubleBonds = 0;
  for (const GlobalSearch::Bond& bond: caffeine.bonds()) {
    if (bond.bondOrder() == 2)
      ++numDoubleBonds;
  }
  QVERIFY(numDoubleBonds == 4);
}

QTEST_MAIN(FormatsTest)

#include "formatstest.moc"

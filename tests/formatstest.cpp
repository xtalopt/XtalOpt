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

#include <globalsearch/formats/obconvert.h>
#include <globalsearch/formats/cmlformat.h>
#include <globalsearch/formats/poscarformat.h>
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
  void readPoscar();
  void writePoscar();
  void readCml();
  void writeCml();
  void OBConvert();
  void readGulp();
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

void FormatsTest::readPoscar()
{
  /**** Rutile ****/
  QString rutileFileName = QString(TESTDATADIR) + "/data/rutile.POSCAR";
  GlobalSearch::Structure rutile;

  QVERIFY(GlobalSearch::Formats::read(&rutile, rutileFileName, "POSCAR"));

  // Our structure should have a unit cell, 6 atoms, and no bonds
  // The unit cell volume should be about 62.423
  QVERIFY(rutile.hasUnitCell());
  QVERIFY(rutile.numAtoms() == 6);
  QVERIFY(rutile.numBonds() == 0);
  QVERIFY(abs(62.4233 - rutile.unitCell().volume()) < 1.e-5);

  // The sixth atom should be Ti and should have cartesian coordinates of
  // (1.47906   2.29686   2.29686)
  double tol = 1.e-5;
  Vector3 rutileAtom2Pos(1.47906, 2.29686, 2.29686);
  QVERIFY(rutile.atom(5).atomicNumber() == 22);
  QVERIFY(GlobalSearch::fuzzyCompare(rutile.atom(5).pos(),
                                     rutileAtom2Pos, tol));

  // The first atom should be O and should have fractional coordinates of
  // 0, 0.3053, 0.3053
  tol = 1.e-5;
  Vector3 rutileAtom3Ref =
      rutile.unitCell().toFractional(rutile.atom(0).pos());
  Vector3 rutileAtom3PosFrac(0.0, 0.3053, 0.3053);
  QVERIFY(rutile.atom(0).atomicNumber() == 8);
  QVERIFY(GlobalSearch::fuzzyCompare(rutileAtom3Ref, rutileAtom3PosFrac, tol));

  // Let's set rutile to be used for the write test
  m_rutile = rutile;
}

void FormatsTest::writePoscar()
{
  // First, write the POSCAR file to a stringstream
  std::stringstream ss;
  QVERIFY(GlobalSearch::PoscarFormat::write(m_rutile, ss));

  // Now read from it and run the same tests we tried above
  /**** Rutile ****/
  GlobalSearch::Structure rutile;
  QVERIFY(GlobalSearch::PoscarFormat::read(rutile, ss));

  // Our structure should have a unit cell, 6 atoms, and no bonds
  // The unit cell volume should be about 62.423
  QVERIFY(rutile.hasUnitCell());
  QVERIFY(rutile.numAtoms() == 6);
  QVERIFY(rutile.numBonds() == 0);
  QVERIFY(abs(62.4233 - rutile.unitCell().volume()) < 1.e-5);

  // The sixth atom should be Ti and should have cartesian coordinates of
  // (1.47906   2.29686   2.29686)
  double tol = 1.e-5;
  Vector3 rutileAtom2Pos(1.47906, 2.29686, 2.29686);
  QVERIFY(rutile.atom(5).atomicNumber() == 22);
  QVERIFY(GlobalSearch::fuzzyCompare(rutile.atom(5).pos(),
                                     rutileAtom2Pos, tol));

  // The first atom should be O and should have fractional coordinates of
  // 0, 0.3053, 0.3053
  tol = 1.e-5;
  Vector3 rutileAtom3Ref =
      rutile.unitCell().toFractional(rutile.atom(0).pos());
  Vector3 rutileAtom3PosFrac(0.0, 0.3053, 0.3053);
  QVERIFY(rutile.atom(0).atomicNumber() == 8);
  QVERIFY(GlobalSearch::fuzzyCompare(rutileAtom3Ref, rutileAtom3PosFrac, tol));
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

  // Now read from it and run the same tests we tried above

  /**** Rutile ****/
  GlobalSearch::Structure rutile;
  QVERIFY(GlobalSearch::CmlFormat::read(rutile, ss));

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

  /**** Caffeine ****/
  GlobalSearch::Structure caffeine;
  QVERIFY(GlobalSearch::CmlFormat::read(caffeine, css));

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

void FormatsTest::OBConvert()
{
  /**** Caffeine PDB ****/
  QString caffeineFileName = QString(TESTDATADIR) + "/data/caffeine.pdb";
  QFile file(caffeineFileName);
  QVERIFY(file.open(QIODevice::ReadOnly));
  QByteArray caffeinePDBData(file.readAll());

  // First, use OBConvert to convert it to cml
  QByteArray caffeineCMLData;
  QVERIFY(GlobalSearch::OBConvert::convertFormat("pdb", "cml",
                                                 caffeinePDBData,
                                                 caffeineCMLData));

  std::stringstream css(caffeineCMLData.data());

  // Now read it
  GlobalSearch::Structure caffeine;
  QVERIFY(GlobalSearch::CmlFormat::read(caffeine, css));

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

  // We can make sure the energy and enthalpy can be read correctly here
  /**** Caffeine SDF ****/
  caffeineFileName = QString(TESTDATADIR) + "/data/caffeine-mmff94.sdf";
  QFile fileSDF(caffeineFileName);
  QVERIFY(fileSDF.open(QIODevice::ReadOnly));
  QByteArray caffeineSDFData(fileSDF.readAll());

  // First, use OBConvert to convert it to cml
  QByteArray caffeineCMLDataWithEnergy;

  // If we want obabel to print the energy and enthalpy in the
  // cml output, we HAVE to pass "-xp" to it.
  QStringList options;
  options << "-xp";

  QVERIFY(GlobalSearch::OBConvert::convertFormat("sdf", "cml",
                                                 caffeineSDFData,
                                                 caffeineCMLDataWithEnergy,
                                                 options));

  std::stringstream csdfss(caffeineCMLDataWithEnergy.data());

  // Now read it
  GlobalSearch::Structure caffeineSDF;
  QVERIFY(GlobalSearch::CmlFormat::read(caffeineSDF, csdfss));

  // Our structure should have no unit cell, 24 atoms, and 25 bonds
  QVERIFY(!caffeineSDF.hasUnitCell());
  QVERIFY(caffeineSDF.numAtoms() == 24);
  QVERIFY(caffeineSDF.numBonds() == 25);

  // Caffeine should also have 4 double bonds. Make sure of this.
  numDoubleBonds = 0;
  for (const GlobalSearch::Bond& bond: caffeineSDF.bonds()) {
    if (bond.bondOrder() == 2)
      ++numDoubleBonds;
  }
  QVERIFY(numDoubleBonds == 4);

  // We should have enthalpy here. It should be -122.350, and energy should
  // be -122.351.
  QVERIFY(caffeineSDF.hasEnthalpy());
  QVERIFY(std::fabs(caffeineSDF.getEnthalpy() - -122.350) < 1.e-5);
  QVERIFY(std::fabs(caffeineSDF.getEnergy() - -122.351) < 1.e-5);
}

void FormatsTest::readGulp()
{
  /**** Some random GULP output ****/
  QString gulpFileName =
    QString(TESTDATADIR) + "/data/optimizerSamples/gulp/xtal.got";

  GlobalSearch::Structure s;
  GlobalSearch::Formats::read(&s, gulpFileName, "gulp");

  double tol = 1.e-5;

  // b should be 4.3098, gamma should be 102.5730, and the volume should
  // be 67.7333397.
  QVERIFY(fabs(s.unitCell().b() - 3.398685) < tol);
  QVERIFY(fabs(s.unitCell().gamma() - 120.000878) < tol);
  QVERIFY(fabs(s.unitCell().volume() - 55.508520) < tol);

  // Atom #2 should be Ti and have a fractional position of
  // 0.499957, 0.999999, 0.500003
  QVERIFY(s.atom(1).atomicNumber() == 22);
  QVERIFY(GlobalSearch::fuzzyCompare(
      s.unitCell().toFractional(s.atom(1).pos()),
      GlobalSearch::Vector3(0.499957, 0.999999, 0.500003),
      tol
    )
  );

  // Atom #3 should be O and have a fractional position of
  // 0.624991, 0.250020, 0.874996
  QVERIFY(s.atom(2).atomicNumber() == 8);
  QVERIFY(GlobalSearch::fuzzyCompare(
      s.unitCell().toFractional(s.atom(2).pos()),
      GlobalSearch::Vector3(0.624991, 0.250020, 0.874996),
      tol
    )
  );

  // Energy should be -78.44239332
  QVERIFY(fabs(s.getEnergy() - -78.44239332) < tol);
}

QTEST_MAIN(FormatsTest)

#include "formatstest.moc"

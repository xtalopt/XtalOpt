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
#include <globalsearch/formats/obconvert.h>
#include <globalsearch/formats/poscarformat.h>
#include <globalsearch/formats/zmatrixformat.h>
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
  void ZMatrixEntryGenerator();
  void writeSiestaZMatrix();

  // Different optimizer formats
  void readCastep();
  void readGulp();
  void readPwscf();
  void readSiesta();
  void readVasp();
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
  QVERIFY(
    GlobalSearch::fuzzyCompare(rutile.atom(5).pos(), rutileAtom2Pos, tol));

  // The first atom should be O and should have fractional coordinates of
  // 0, 0.3053, 0.3053
  tol = 1.e-5;
  Vector3 rutileAtom3Ref = rutile.unitCell().toFractional(rutile.atom(0).pos());
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
  QVERIFY(
    GlobalSearch::fuzzyCompare(rutile.atom(5).pos(), rutileAtom2Pos, tol));

  // The first atom should be O and should have fractional coordinates of
  // 0, 0.3053, 0.3053
  tol = 1.e-5;
  Vector3 rutileAtom3Ref = rutile.unitCell().toFractional(rutile.atom(0).pos());
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
  QVERIFY(
    GlobalSearch::fuzzyCompare(rutile.atom(1).pos(), rutileAtom2Pos, tol));

  // The third atom should be O and should have fractional coordinates of
  // 0, 0.3053, 0.3053
  tol = 1.e-5;
  Vector3 rutileAtom3Ref = rutile.unitCell().toFractional(rutile.atom(2).pos());
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
  for (const GlobalSearch::Bond& bond : caffeine.bonds()) {
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
  QVERIFY(
    GlobalSearch::fuzzyCompare(rutile.atom(1).pos(), rutileAtom2Pos, tol));

  // The third atom should be O and should have fractional coordinates of
  // 0, 0.3053, 0.3053
  tol = 1.e-5;
  Vector3 rutileAtom3Ref = rutile.unitCell().toFractional(rutile.atom(2).pos());
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
  for (const GlobalSearch::Bond& bond : caffeine.bonds()) {
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
  QVERIFY(GlobalSearch::OBConvert::convertFormat("pdb", "cml", caffeinePDBData,
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
  for (const GlobalSearch::Bond& bond : caffeine.bonds()) {
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

  QVERIFY(GlobalSearch::OBConvert::convertFormat(
    "sdf", "cml", caffeineSDFData, caffeineCMLDataWithEnergy, options));

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
  for (const GlobalSearch::Bond& bond : caffeineSDF.bonds()) {
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

void FormatsTest::ZMatrixEntryGenerator()
{
  // We will test it on caffeine
  QString caffeineFileName = QString(TESTDATADIR) + "/data/caffeine.cml";
  GlobalSearch::Structure caffeine;
  QVERIFY(GlobalSearch::Formats::read(&caffeine, caffeineFileName, "cml"));

  std::vector<GlobalSearch::ZMatrixEntry> entries =
    GlobalSearch::ZMatrixFormat::generateZMatrixEntries(&caffeine);

  /*
    qDebug() << "Entries are:";
    for (const auto& entry: entries)
      qDebug() << entry.ind << entry.rInd << entry.angleInd <<
    entry.dihedralInd;
  */

  // There should be 24 entries. We can check more info in the future
  QVERIFY(entries.size() == 24);

  // Write it to the z-matrix format
  std::stringstream ss;
  GlobalSearch::ZMatrixFormat::write(caffeine, ss);
  // qDebug() << ("output is:\n" + ss.str()).c_str();

  // Now read it back in to make sure our writer worked
  GlobalSearch::Structure newCaffeine;
  GlobalSearch::ZMatrixFormat::read(&newCaffeine, ss);

  // Make sure we have 24 atoms
  QVERIFY(newCaffeine.numAtoms() == 24);

  // We can potentially run more tests in the future
}

void FormatsTest::writeSiestaZMatrix()
{
  // We will test it on caffeine
  QString caffeineFileName = QString(TESTDATADIR) + "/data/caffeine.cml";
  GlobalSearch::Structure caffeine;
  QVERIFY(GlobalSearch::Formats::read(&caffeine, caffeineFileName, "cml"));

  // Now make a unit cell
  GlobalSearch::UnitCell uc(5.0, 5.0, 5.0, 90.0, 90.0, 90.0);
  caffeine.setUnitCell(uc);

  std::stringstream ss;
  QVERIFY(GlobalSearch::ZMatrixFormat::writeSiestaZMatrix(caffeine, ss, true,
                                                          true, true));

  // For now, just make sure the string isn't empty. We should probably
  // include more tests in the future.
  QVERIFY(!ss.str().empty());

  // std::cout << "ss is:\n" << ss.str() << "\n";
}

void FormatsTest::readCastep()
{
  /**** Some random CASTEP output ****/
  QString fileName =
    QString(TESTDATADIR) + "/data/optimizerSamples/castep/xtal.castep";

  GlobalSearch::Structure s;
  GlobalSearch::Formats::read(&s, fileName, "castep");

  double tol = 1.e-5;

  // b should be 2.504900, gamma should be 103.045287, and the volume should
  // be 8.358635.
  QVERIFY(fabs(s.unitCell().b() - 2.504900) < tol);
  QVERIFY(fabs(s.unitCell().gamma() - 103.045287) < tol);
  QVERIFY(fabs(s.unitCell().volume() - 8.358635) < tol);

  // We should have two atoms
  QVERIFY(s.numAtoms() == 2);

  // Atom #1 should be H and have a fractional position of
  // -0.037144, 0.000195, 0.088768
  QVERIFY(s.atom(0).atomicNumber() == 1);
  QVERIFY(GlobalSearch::fuzzyCompare(
    s.unitCell().toFractional(s.atom(0).pos()),
    GlobalSearch::Vector3(-0.037144, 0.000195, 0.088768), tol));

  // Atom #2 should be H and have a fractional position of
  // 0.474430, 0.498679, 0.850082
  QVERIFY(s.atom(1).atomicNumber() == 1);
  QVERIFY(GlobalSearch::fuzzyCompare(
    s.unitCell().toFractional(s.atom(1).pos()),
    GlobalSearch::Vector3(0.474430, 0.498679, 0.850082), tol));

  // Energy should be -29.55686228188
  QVERIFY(fabs(s.getEnergy() - -29.55686228188) < tol);

  // Enthalpy should be -29.5566434
  QVERIFY(fabs(s.getEnthalpy() - -29.5566434) < tol);
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

  // NumAtoms should be 6
  QVERIFY(s.numAtoms() == 6);

  // Atom #2 should be Ti and have a fractional position of
  // 0.499957, 0.999999, 0.500003
  QVERIFY(s.atom(1).atomicNumber() == 22);
  QVERIFY(GlobalSearch::fuzzyCompare(
    s.unitCell().toFractional(s.atom(1).pos()),
    GlobalSearch::Vector3(0.499957, 0.999999, 0.500003), tol));

  // Atom #3 should be O and have a fractional position of
  // 0.624991, 0.250020, 0.874996
  QVERIFY(s.atom(2).atomicNumber() == 8);
  QVERIFY(GlobalSearch::fuzzyCompare(
    s.unitCell().toFractional(s.atom(2).pos()),
    GlobalSearch::Vector3(0.624991, 0.250020, 0.874996), tol));

  // Energy should be -78.44239332
  QVERIFY(fabs(s.getEnergy() - -78.44239332) < tol);
}

void FormatsTest::readPwscf()
{
  /**** Some random PWSCF output ****/
  QString fileName =
    QString(TESTDATADIR) + "/data/optimizerSamples/pwscf/xtal.out";

  GlobalSearch::Structure s;
  GlobalSearch::Formats::read(&s, fileName, "pwscf");

  double tol = 1.e-5;

  // b should be 4.13754, gamma should be 75.31465, and the volume should
  // be 70.0484.
  QVERIFY(fabs(s.unitCell().b() - 4.13754) < tol);
  QVERIFY(fabs(s.unitCell().gamma() - 75.31465) < tol);
  QVERIFY(fabs(s.unitCell().volume() - 70.0484) < tol);

  // We should have two atoms
  QVERIFY(s.numAtoms() == 2);

  // Atom #1 should be O and have a fractional position of
  // 0.040806225, 0.100970667, 0.003304159
  QVERIFY(s.atom(0).atomicNumber() == 8);
  QVERIFY(GlobalSearch::fuzzyCompare(
    s.unitCell().toFractional(s.atom(0).pos()),
    GlobalSearch::Vector3(0.040806225, 0.100970667, 0.003304159), tol));

  // Atom #2 should be O and have a fractional position of
  // 0.577212775, 0.316072333, 0.629713841
  QVERIFY(s.atom(1).atomicNumber() == 8);
  QVERIFY(GlobalSearch::fuzzyCompare(
    s.unitCell().toFractional(s.atom(1).pos()),
    GlobalSearch::Vector3(0.577212775, 0.316072333, 0.629713841), tol));

  static const double RYDBERG_TO_EV = 13.605698066;

  // We need to reduce the tolerance a little bit for these.
  tol = 1.e-3;

  // Energy should be -63.35870913 Rydbergs.
  QVERIFY(fabs(s.getEnergy() - (-63.35870913 * RYDBERG_TO_EV)) < tol);

  // Enthalpy should be -62.9446011092 Rydbergs
  QVERIFY(fabs(s.getEnthalpy() - (-62.9446011092 * RYDBERG_TO_EV)) < tol);
}

void FormatsTest::readSiesta()
{
  /**** Some random SIESTA output ****/
  QString fileName =
    QString(TESTDATADIR) + "/data/optimizerSamples/siesta/xtal.out";

  GlobalSearch::Structure s;
  GlobalSearch::Formats::read(&s, fileName, "siesta");

  double tol = 1.e-4;

  // b should be 3.874763, gamma should be 74.2726, and the volume should
  // be 75.408578.
  QVERIFY(fabs(s.unitCell().b() - 3.874763) < tol);
  QVERIFY(fabs(s.unitCell().gamma() - 74.2726) < tol);
  QVERIFY(fabs(s.unitCell().volume() - 75.408578) < tol);

  // We should have six atoms
  QVERIFY(s.numAtoms() == 6);

  // Atom #2 should be Ti and have a fractional position of
  // 0.40338285, 0.38896410, 0.75921162
  QVERIFY(s.atom(1).atomicNumber() == 22);
  QVERIFY(GlobalSearch::fuzzyCompare(
    s.unitCell().toFractional(s.atom(1).pos()),
    GlobalSearch::Vector3(0.40338285, 0.38896410, 0.75921162), tol));

  // Atom #3 should be O and have a fractional position of
  // 0.38568921, 0.74679127, 0.21473350
  QVERIFY(s.atom(2).atomicNumber() == 8);
  QVERIFY(GlobalSearch::fuzzyCompare(
    s.unitCell().toFractional(s.atom(2).pos()),
    GlobalSearch::Vector3(0.38568921, 0.74679127, 0.21473350), tol));

  // Energy should be -2005.342641 eV
  QVERIFY(fabs(s.getEnergy() - -2005.342641) < tol);
}

void FormatsTest::readVasp()
{
  /**** Some random VASP output ****/
  QString fileName =
    QString(TESTDATADIR) + "/data/optimizerSamples/vasp/CONTCAR";

  GlobalSearch::Structure s;
  GlobalSearch::Formats::read(&s, fileName, "vasp");

  double tol = 1.e-5;

  // b should be 2.52221, gamma should be 86.32327, and the volume should
  // be 6.01496.
  QVERIFY(fabs(s.unitCell().b() - 2.52221) < tol);
  QVERIFY(fabs(s.unitCell().gamma() - 86.32327) < tol);
  QVERIFY(fabs(s.unitCell().volume() - 6.01496) < tol);

  // We should have three atoms
  QVERIFY(s.numAtoms() == 3);

  // Atom #2 should be H and have a fractional position of
  // 0.9317195263978, 0.20419595775, 0.4923304223199
  QVERIFY(s.atom(1).atomicNumber() == 1);
  QVERIFY(GlobalSearch::fuzzyCompare(
    s.unitCell().toFractional(s.atom(1).pos()),
    GlobalSearch::Vector3(0.9317195263978, 0.20419595775, 0.4923304223199),
    tol));

  // Atom #3 should be O and have a fractional position of
  // 0.087068957523, -0.1465596214494, 0.1524183445695
  QVERIFY(s.atom(2).atomicNumber() == 8);
  QVERIFY(GlobalSearch::fuzzyCompare(
    s.unitCell().toFractional(s.atom(2).pos()),
    GlobalSearch::Vector3(0.087068957523, -0.1465596214494, 0.1524183445695),
    tol));

  // Energy should be 5.56502673 eV.
  QVERIFY(fabs(s.getEnergy() - 5.56502673) < tol);

  // Enthalpy should be 43.10746559 eV
  QVERIFY(fabs(s.getEnthalpy() - 43.10746559) < tol);
}

QTEST_MAIN(FormatsTest)

#include "formatstest.moc"

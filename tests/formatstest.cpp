/**********************************************************************
  FormatsTest - Testing file format readers and writers

  Copyright (C) 2017 Patrick Avery
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/optimizers/castepoptimizer.h>
#include <search/optimizers/gulpoptimizer.h>
#include <search/optimizers/pwscfoptimizer.h>
#include <search/optimizers/siestaoptimizer.h>
#include <search/optimizers/vaspoptimizer.h>
#include <search/structure.h>
#include <atoms/formats/castepformat.h>
#include <atoms/formats/cifformat.h>
#include <atoms/formats/cmlformat.h>
#include <atoms/formats/formats.h>
#include <atoms/formats/mtpformat.h>
#include <atoms/formats/poscarformat.h>
#include <atoms/formats/pwscfformat.h>
#include <atoms/formats/siestaformat.h>
#include <atoms/formats/xyzformat.h>
#include <atoms/geometry.h>

#include <common/constants.h>
#include <common/fileutils.h>

#include <QString>
#include <QFile>
#include <QtTest>

#include <cmath>
#include <fstream>
#include <sstream>

namespace {

QString testDataPath(const QString& filename)
{
  return Common::localPath(Common::localPath(QString(TESTDATADIR), "formats"), filename);
}

QString optimizerSamplePath(const QString& optimizer, const QString& filename)
{
  return Common::localPath(Common::localPath(testDataPath("optimizerSamples"), optimizer),
                           filename);
}

} // namespace

class FormatsTest : public QObject
{
  Q_OBJECT

  // Members
  Search::Structure m_rutile;
  Search::Structure m_caffeine;

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
  void readCifWithAtoms();
  void writeCifWithAtoms();
  void writeXyzWithAtoms();
  void readWithAtomsFormats();
  void readCml();
  void writeCml();
  void readWriteMtpInputWithAtoms();
  void readOptimizerInputsWithAtoms();

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
  QString rutileFileName = testDataPath("rutile.POSCAR");
  Search::Structure rutile;
  std::ifstream in(rutileFileName.toStdString().c_str());
  QVERIFY(in.is_open());

  QVERIFY(Atoms::PoscarFormat::read(rutile, in));

  // Our structure should have a unit cell, 6 atoms, and no bonds
  // The unit cell volume should be about 62.423
  QVERIFY(rutile.is3D());
  QVERIFY(rutile.numAtoms() == 6);
  QVERIFY(rutile.numBonds() == 0);
  QVERIFY(std::fabs(62.4233 - rutile.unitCell().volume()) < 1.e-5);

  // The sixth atom should be Ti and should have cartesian coordinates of
  // (1.47906   2.29686   2.29686)
  double tol = 1.e-5;
  Common::Vector3 rutileAtom2Pos(1.47906, 2.29686, 2.29686);
  QVERIFY(rutile.atom(5).atomicNumber() == 22);
  QVERIFY(
    Common::fuzzyCompare(rutile.atom(5).pos(), rutileAtom2Pos, tol));

  // The first atom should be O and should have fractional coordinates of
  // 0, 0.3053, 0.3053
  tol = 1.e-5;
  Common::Vector3 rutileAtom3Ref = rutile.unitCell().toFractional(rutile.atom(0).pos());
  Common::Vector3 rutileAtom3PosFrac(0.0, 0.3053, 0.3053);
  QVERIFY(rutile.atom(0).atomicNumber() == 8);
  QVERIFY(Common::fuzzyCompare(rutileAtom3Ref, rutileAtom3PosFrac, tol));

  // Let's set rutile to be used for the write test
  m_rutile = rutile;
}

void FormatsTest::writePoscar()
{
  // First, write the POSCAR file to a stringstream
  std::stringstream ss;
  QVERIFY(Atoms::PoscarFormat::write(m_rutile, ss, m_rutile.getLocpath()));

  // Now read from it and run the same tests we tried above
  /**** Rutile ****/
  Search::Structure rutile;
  QVERIFY(Atoms::PoscarFormat::read(rutile, ss));

  // Our structure should have a unit cell, 6 atoms, and no bonds
  // The unit cell volume should be about 62.423
  QVERIFY(rutile.is3D());
  QVERIFY(rutile.numAtoms() == 6);
  QVERIFY(rutile.numBonds() == 0);
  QVERIFY(std::fabs(62.4233 - rutile.unitCell().volume()) < 1.e-5);

  // The sixth atom should be Ti and should have cartesian coordinates of
  // (1.47906   2.29686   2.29686)
  double tol = 1.e-5;
  Common::Vector3 rutileAtom2Pos(1.47906, 2.29686, 2.29686);
  QVERIFY(rutile.atom(5).atomicNumber() == 22);
  QVERIFY(
    Common::fuzzyCompare(rutile.atom(5).pos(), rutileAtom2Pos, tol));

  // The first atom should be O and should have fractional coordinates of
  // 0, 0.3053, 0.3053
  tol = 1.e-5;
  Common::Vector3 rutileAtom3Ref = rutile.unitCell().toFractional(rutile.atom(0).pos());
  Common::Vector3 rutileAtom3PosFrac(0.0, 0.3053, 0.3053);
  QVERIFY(rutile.atom(0).atomicNumber() == 8);
  QVERIFY(Common::fuzzyCompare(rutileAtom3Ref, rutileAtom3PosFrac, tol));
}

void FormatsTest::readCifWithAtoms()
{
  const QString filename = testDataPath("diamond-primitive.cif");
  Atoms::Geometry diamond;
  QVERIFY(Atoms::CifFormat::read(&diamond, filename));

  QVERIFY(diamond.is3D());
  QCOMPARE(diamond.numAtoms(), static_cast<size_t>(2));
  QCOMPARE(diamond.atom(0).atomicNumber(), static_cast<unsigned short>(6));
  QCOMPARE(diamond.atom(1).atomicNumber(), static_cast<unsigned short>(6));

  const Common::Vector3 frac = diamond.cartToFrac(diamond.atom(0).pos());
  QVERIFY(std::fabs(frac.x() - 0.75) < 1.e-5);
  QVERIFY(std::fabs(frac.y() - 0.75) < 1.e-5);
  QVERIFY(std::fabs(frac.z() - 0.75) < 1.e-5);
}

void FormatsTest::writeCifWithAtoms()
{
  const QString filename = testDataPath("diamond-primitive.cif");
  Atoms::Geometry diamond;
  QVERIFY(Atoms::CifFormat::read(&diamond, filename));

  std::stringstream out;
  QVERIFY(Atoms::CifFormat::write(diamond, out));

  const std::string cif = out.str();
  QVERIFY(cif.find("_chemical_formula_sum") != std::string::npos);
  QVERIFY(cif.find("_symmetry_Int_Tables_number") != std::string::npos);
  QVERIFY(cif.find("_atom_site_fract_x") != std::string::npos);
}

void FormatsTest::writeXyzWithAtoms()
{
  Atoms::Geometry structure;
  structure.setUnitCell(Atoms::UnitCell(3.0, 4.0, 5.0, 90.0, 90.0, 90.0));
  structure.addAtom(6, Common::Vector3(0.0, 0.0, 0.0));
  structure.addAtom(1, Common::Vector3(0.0, 0.0, 1.0));

  std::stringstream out;
  QVERIFY(Atoms::XyzFormat::write(structure, out));

  const std::string xyz = out.str();
  std::istringstream in(xyz);
  std::string line;
  QVERIFY(static_cast<bool>(std::getline(in, line)));
  QCOMPARE(line, std::string("2"));
  QVERIFY(static_cast<bool>(std::getline(in, line)));
  QVERIFY(static_cast<bool>(std::getline(in, line)));
  QVERIFY(line.find("C") == 0);
  QVERIFY(line.find("0.00000000") != std::string::npos);
  QVERIFY(static_cast<bool>(std::getline(in, line)));
  QVERIFY(line.find("H") == 0);
  QVERIFY(line.find("1.00000000") != std::string::npos);
}

void FormatsTest::readWithAtomsFormats()
{
  const QString rutileFileName = testDataPath("rutile.POSCAR");

  Atoms::Geometry rutile;
  QVERIFY(Atoms::Formats::read(&rutile, rutileFileName));
  QCOMPARE(rutile.numAtoms(), static_cast<size_t>(6));
  QVERIFY(rutile.is3D());
  QVERIFY(std::fabs(62.4233 - rutile.unitCell().volume()) < 1.e-5);
}

void FormatsTest::readCml()
{
  /**** Rutile ****/
  QString rutileFileName = testDataPath("rutile.cml");
  Search::Structure rutile;
  std::ifstream rutileIn(rutileFileName.toStdString().c_str());
  QVERIFY(rutileIn.is_open());

  QVERIFY(Atoms::CmlFormat::read(rutile, rutileIn));

  // Our structure should have a unit cell, 6 atoms, and no bonds
  // The unit cell volume should be about 62.423
  QVERIFY(rutile.is3D());
  QVERIFY(rutile.numAtoms() == 6);
  QVERIFY(rutile.numBonds() == 0);
  QVERIFY(std::fabs(62.4233 - rutile.unitCell().volume()) < 1.e-5);

  // The second atom should be Ti and should have cartesian coordinates of
  // (1.47906   2.29686   2.29686)
  double tol = 1.e-5;
  Common::Vector3 rutileAtom2Pos(1.47906, 2.29686, 2.29686);
  QVERIFY(rutile.atom(1).atomicNumber() == 22);
  QVERIFY(
    Common::fuzzyCompare(rutile.atom(1).pos(), rutileAtom2Pos, tol));

  // The third atom should be O and should have fractional coordinates of
  // 0, 0.3053, 0.3053
  tol = 1.e-5;
  Common::Vector3 rutileAtom3Ref = rutile.unitCell().toFractional(rutile.atom(2).pos());
  Common::Vector3 rutileAtom3PosFrac(0.0, 0.3053, 0.3053);
  QVERIFY(rutile.atom(2).atomicNumber() == 8);
  QVERIFY(Common::fuzzyCompare(rutileAtom3Ref, rutileAtom3PosFrac, tol));

  // Let's set rutile to be used for the write test
  m_rutile = rutile;

  /**** Ethane ****/
  QString ethaneFileName = testDataPath("ethane.cml");
  Search::Structure ethane;
  std::ifstream ethaneIn(ethaneFileName.toStdString().c_str());
  QVERIFY(ethaneIn.is_open());

  QVERIFY(Atoms::CmlFormat::read(ethane, ethaneIn));

  // Our structure should have no unit cell, 8 atoms, and 7 bonds
  QVERIFY(!ethane.is3D());
  QVERIFY(ethane.numAtoms() == 8);
  QVERIFY(ethane.numBonds() == 7);

  /**** Caffeine ****/
  QString caffeineFileName = testDataPath("caffeine.cml");
  Search::Structure caffeine;
  std::ifstream caffeineIn(caffeineFileName.toStdString().c_str());
  QVERIFY(caffeineIn.is_open());
  QVERIFY(Atoms::CmlFormat::read(caffeine, caffeineIn));

  // Our structure should have no unit cell, 24 atoms, and 25 bonds
  QVERIFY(!caffeine.is3D());
  QVERIFY(caffeine.numAtoms() == 24);
  QVERIFY(caffeine.numBonds() == 25);

  // Caffeine should also have 4 double bonds. Make sure of this.
  size_t numDoubleBonds = 0;
  for (const Atoms::Bond& bond : caffeine.bonds()) {
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
  QVERIFY(Atoms::CmlFormat::write(m_rutile, ss));

  // Now read from it and run the same tests we tried above

  /**** Rutile ****/
  Search::Structure rutile;
  QVERIFY(Atoms::CmlFormat::read(rutile, ss));

  // Our structure should have a unit cell, 6 atoms, and no bonds
  // The unit cell volume should be about 62.423
  QVERIFY(rutile.is3D());
  QVERIFY(rutile.numAtoms() == 6);
  QVERIFY(rutile.numBonds() == 0);
  QVERIFY(std::fabs(62.4233 - rutile.unitCell().volume()) < 1.e-5);

  // The second atom should be Ti and should have cartesian coordinates of
  // (1.47906   2.29686   2.29686)
  double tol = 1.e-5;
  Common::Vector3 rutileAtom2Pos(1.47906, 2.29686, 2.29686);
  QVERIFY(rutile.atom(1).atomicNumber() == 22);
  QVERIFY(
    Common::fuzzyCompare(rutile.atom(1).pos(), rutileAtom2Pos, tol));

  // The third atom should be O and should have fractional coordinates of
  // 0, 0.3053, 0.3053
  tol = 1.e-5;
  Common::Vector3 rutileAtom3Ref = rutile.unitCell().toFractional(rutile.atom(2).pos());
  Common::Vector3 rutileAtom3PosFrac(0.0, 0.3053, 0.3053);
  QVERIFY(rutile.atom(2).atomicNumber() == 8);
  QVERIFY(Common::fuzzyCompare(rutileAtom3Ref, rutileAtom3PosFrac, tol));

  // Do the same thing with caffeine
  std::stringstream css;
  QVERIFY(Atoms::CmlFormat::write(m_caffeine, css));

  /**** Caffeine ****/
  Search::Structure caffeine;
  QVERIFY(Atoms::CmlFormat::read(caffeine, css));

  // Our structure should have no unit cell, 24 atoms, and 25 bonds
  QVERIFY(!caffeine.is3D());
  QVERIFY(caffeine.numAtoms() == 24);
  QVERIFY(caffeine.numBonds() == 25);

  // Caffeine should also have 4 double bonds. Make sure of this.
  size_t numDoubleBonds = 0;
  for (const Atoms::Bond& bond : caffeine.bonds()) {
    if (bond.bondOrder() == 2)
      ++numDoubleBonds;
  }
  QVERIFY(numDoubleBonds == 4);
}

void FormatsTest::readWriteMtpInputWithAtoms()
{
  Atoms::Geometry structure;
  structure.setCellInfo(3.0, 4.0, 5.0, 90.0, 90.0, 90.0);
  structure.addAtom(6, Common::Vector3(0.0, 0.0, 0.0));
  structure.addAtom(1, Common::Vector3(0.0, 0.0, 1.0));

  std::stringstream out;
  QVERIFY(Atoms::MtpFormat::write(structure, out));
  const std::string text = out.str();
  QVERIFY(text.find("BEGIN_CFG") != std::string::npos);
  QVERIFY(text.find("Feature chemical_system C H") != std::string::npos);

  const QString filename = "formatstest-atoms-mtp.cfg";
  {
    std::ofstream file(filename.toStdString().c_str(), std::ios::out | std::ios::trunc);
    QVERIFY(file.is_open());
    file << text;
  }

  Atoms::Geometry tstgeom;
  QVERIFY(Atoms::MtpFormat::read(&tstgeom, filename));
  QFile::remove(filename);

  QVERIFY(tstgeom.is3D());
  QCOMPARE(tstgeom.numAtoms(), static_cast<size_t>(2));
  QVERIFY(std::fabs(60.0 - tstgeom.unitCell().volume()) < 1.e-5);
  QCOMPARE(tstgeom.atom(0).atomicNumber(), static_cast<unsigned short>(6));
  QCOMPARE(tstgeom.atom(1).atomicNumber(), static_cast<unsigned short>(1));
}

void FormatsTest::readOptimizerInputsWithAtoms()
{
  const QString castepFile = "formatstest-atoms-castep.cell";
  {
    std::ofstream file(castepFile.toStdString().c_str(), std::ios::out | std::ios::trunc);
    QVERIFY(file.is_open());
    file << "%BLOCK LATTICE_CART\n"
         << "ang\n"
         << "3.0 0.0 0.0\n"
         << "0.0 4.0 0.0\n"
         << "0.0 0.0 5.0\n"
         << "%ENDBLOCK LATTICE_CART\n"
         << "%BLOCK POSITIONS_FRAC\n"
         << "C 0.0 0.0 0.0\n"
         << "H 0.0 0.0 0.2\n"
         << "%ENDBLOCK POSITIONS_FRAC\n";
  }
  Atoms::Geometry castep;
  QVERIFY(Atoms::CastepFormat::read(&castep, castepFile));
  QFile::remove(castepFile);
  QVERIFY(castep.is3D());
  QCOMPARE(castep.numAtoms(), static_cast<size_t>(2));
  QVERIFY(std::fabs(60.0 - castep.unitCell().volume()) < 1.e-5);
  QCOMPARE(castep.atom(0).atomicNumber(), static_cast<unsigned short>(6));
  QVERIFY(std::fabs(1.0 - castep.atom(1).pos().z()) < 1.e-5);

  const QString pwscfFile = "formatstest-atoms-pwscf.in";
  {
    std::ofstream file(pwscfFile.toStdString().c_str(), std::ios::out | std::ios::trunc);
    QVERIFY(file.is_open());
    file << "CELL_PARAMETERS angstrom\n"
         << "3.0 0.0 0.0\n"
         << "0.0 4.0 0.0\n"
         << "0.0 0.0 5.0\n"
         << "ATOMIC_POSITIONS crystal\n"
         << "C 0.0 0.0 0.0\n"
         << "H 0.0 0.0 0.2\n"
         << "K_POINTS gamma\n";
  }
  Atoms::Geometry pwscf;
  QVERIFY(Atoms::PwscfFormat::read(&pwscf, pwscfFile));
  QFile::remove(pwscfFile);
  QVERIFY(pwscf.is3D());
  QCOMPARE(pwscf.numAtoms(), static_cast<size_t>(2));
  QVERIFY(std::fabs(60.0 - pwscf.unitCell().volume()) < 1.e-5);
  QCOMPARE(pwscf.atom(0).atomicNumber(), static_cast<unsigned short>(6));
  QVERIFY(std::fabs(1.0 - pwscf.atom(1).pos().z()) < 1.e-5);

  const QString siestaFile = "formatstest-atoms-siesta.fdf";
  {
    std::ofstream file(siestaFile.toStdString().c_str(), std::ios::out | std::ios::trunc);
    QVERIFY(file.is_open());
    file << "LatticeConstant 1.0 Ang\n"
         << "%block ChemicalSpeciesLabel\n"
         << "1 6 C\n"
         << "2 1 H\n"
         << "%endblock ChemicalSpeciesLabel\n"
         << "%block LatticeVectors\n"
         << "3.0 0.0 0.0\n"
         << "0.0 4.0 0.0\n"
         << "0.0 0.0 5.0\n"
         << "%endblock LatticeVectors\n"
         << "AtomicCoordinatesFormat Fractional\n"
         << "%block AtomicCoordinatesAndAtomicSpecies\n"
         << "0.0 0.0 0.0 1\n"
         << "0.0 0.0 0.2 2\n"
         << "%endblock AtomicCoordinatesAndAtomicSpecies\n";
  }
  Atoms::Geometry siesta;
  QVERIFY(Atoms::SiestaFormat::read(&siesta, siestaFile));
  QFile::remove(siestaFile);
  QVERIFY(siesta.is3D());
  QCOMPARE(siesta.numAtoms(), static_cast<size_t>(2));
  QVERIFY(std::fabs(60.0 - siesta.unitCell().volume()) < 1.e-5);
  QCOMPARE(siesta.atom(0).atomicNumber(), static_cast<unsigned short>(6));
  QVERIFY(std::fabs(1.0 - siesta.atom(1).pos().z()) < 1.e-5);
}

void FormatsTest::readCastep()
{
  /**** Some random CASTEP output ****/
  QString fileName = optimizerSamplePath("castep", "xtal.castep");

  Search::Structure s;
  Search::CASTEPOptimizer(nullptr).read(&s, fileName);

  double tol = 1.e-5;

  // b should be 2.504900, gamma should be 103.045287, and the volume should
  // be 8.358635.
  QVERIFY(std::fabs(s.unitCell().b() - 2.504900) < tol);
  QVERIFY(std::fabs(s.unitCell().gamma() - 103.045287) < tol);
  QVERIFY(std::fabs(s.unitCell().volume() - 8.358635) < tol);

  // We should have two atoms
  QVERIFY(s.numAtoms() == 2);

  // Atoms::Atom #1 should be H and have a fractional position of
  // -0.037144, 0.000195, 0.088768
  QVERIFY(s.atom(0).atomicNumber() == 1);
  QVERIFY(Common::fuzzyCompare(
    s.unitCell().toFractional(s.atom(0).pos()),
    Common::Vector3(-0.037144, 0.000195, 0.088768), tol));

  // Atoms::Atom #2 should be H and have a fractional position of
  // 0.474430, 0.498679, 0.850082
  QVERIFY(s.atom(1).atomicNumber() == 1);
  QVERIFY(Common::fuzzyCompare(
    s.unitCell().toFractional(s.atom(1).pos()),
    Common::Vector3(0.474430, 0.498679, 0.850082), tol));

  // Energy should be -29.55686228188
  QVERIFY(std::fabs(s.getEnergy() - -29.55686228188) < tol);

  // Enthalpy should be -29.5566434
  QVERIFY(std::fabs(s.getEnthalpy() - -29.5566434) < tol);
}

void FormatsTest::readGulp()
{
  /**** Some random GULP output ****/
  QString gulpFileName = optimizerSamplePath("gulp", "xtal.got");

  Search::Structure s;
  Search::GULPOptimizer(nullptr).read(&s, gulpFileName);

  double tol = 1.e-5;

  // b should be 3.398685, gamma should be 120.000878, and the volume
  // should be 55.508520.
  QVERIFY(std::fabs(s.unitCell().b() - 3.398685) < tol);
  QVERIFY(std::fabs(s.unitCell().gamma() - 120.000878) < tol);
  QVERIFY(std::fabs(s.unitCell().volume() - 55.508520) < tol);

  // NumAtoms should be 6
  QVERIFY(s.numAtoms() == 6);

  // Atoms::Atom #2 should be Ti and have a fractional position of
  // 0.499957, 0.999999, 0.500003
  QVERIFY(s.atom(1).atomicNumber() == 22);
  QVERIFY(Common::fuzzyCompare(
    s.unitCell().toFractional(s.atom(1).pos()),
    Common::Vector3(0.499957, 0.999999, 0.500003), tol));

  // Atoms::Atom #3 should be O and have a fractional position of
  // 0.624991, 0.250020, 0.874996
  QVERIFY(s.atom(2).atomicNumber() == 8);
  QVERIFY(Common::fuzzyCompare(
    s.unitCell().toFractional(s.atom(2).pos()),
    Common::Vector3(0.624991, 0.250020, 0.874996), tol));

  // Energy should be -78.44239332
  QVERIFY(std::fabs(s.getEnergy() - -78.44239332) < tol);
}

void FormatsTest::readPwscf()
{
  /**** Some random PWSCF output ****/
  QString fileName = optimizerSamplePath("pwscf", "xtal.out");

  Search::Structure s;
  Search::PWSCFOptimizer(nullptr).read(&s, fileName);

  double tol = 1.e-5;

  // Check the PWSCF cell values.
  // b should be 2.189494, gamma should be 75.31465, and volume should be 10.380105.
  QVERIFY(std::fabs(s.unitCell().b() - 2.189494) < tol);
  QVERIFY(std::fabs(s.unitCell().gamma() - 75.31465) < tol);
  QVERIFY(std::fabs(s.unitCell().volume() - 10.380105) < tol);

  // We should have two atoms
  QVERIFY(s.numAtoms() == 2);

  // Atoms::Atom #1 should be O and have a fractional position of
  // 0.040806225, 0.100970667, 0.003304159
  QVERIFY(s.atom(0).atomicNumber() == 8);
  QVERIFY(Common::fuzzyCompare(
    s.unitCell().toFractional(s.atom(0).pos()),
    Common::Vector3(0.040806225, 0.100970667, 0.003304159), tol));

  // Atoms::Atom #2 should be O and have a fractional position of
  // 0.577212775, 0.316072333, 0.629713841
  QVERIFY(s.atom(1).atomicNumber() == 8);
  QVERIFY(Common::fuzzyCompare(
    s.unitCell().toFractional(s.atom(1).pos()),
    Common::Vector3(0.577212775, 0.316072333, 0.629713841), tol));

  // We need to reduce the tolerance a little bit for these.
  tol = 1.e-3;

  // Energy should be -63.35870913 Rydbergs.
  QVERIFY(std::fabs(s.getEnergy() - (-63.35870913 * RY2EV)) < tol);

  // Enthalpy should be -62.9446011092 Rydbergs
  QVERIFY(std::fabs(s.getEnthalpy() - (-62.9446011092 * RY2EV)) < tol);
}

void FormatsTest::readSiesta()
{
  /**** Some random SIESTA output ****/
  QString fileName = optimizerSamplePath("siesta", "xtal.out");

  Search::Structure s;
  Search::SIESTAOptimizer(nullptr).read(&s, fileName);

  double tol = 1.e-4;

  // b should be 3.874763, gamma should be 74.2726, and the volume should
  // be 75.408578.
  QVERIFY(std::fabs(s.unitCell().b() - 3.874763) < tol);
  QVERIFY(std::fabs(s.unitCell().gamma() - 74.2726) < tol);
  QVERIFY(std::fabs(s.unitCell().volume() - 75.408578) < tol);

  // We should have six atoms
  QVERIFY(s.numAtoms() == 6);

  // Atoms::Atom #2 should be Ti and have a fractional position of
  // 0.40338285, 0.38896410, 0.75921162
  QVERIFY(s.atom(1).atomicNumber() == 22);
  QVERIFY(Common::fuzzyCompare(
    s.unitCell().toFractional(s.atom(1).pos()),
    Common::Vector3(0.40338285, 0.38896410, 0.75921162), tol));

  // Atoms::Atom #3 should be O and have a fractional position of
  // 0.38568921, 0.74679127, 0.21473350
  QVERIFY(s.atom(2).atomicNumber() == 8);
  QVERIFY(Common::fuzzyCompare(
    s.unitCell().toFractional(s.atom(2).pos()),
    Common::Vector3(0.38568921, 0.74679127, 0.21473350), tol));

  // Energy should be -2005.342641 eV
  QVERIFY(std::fabs(s.getEnergy() - -2005.342641) < tol);
}

void FormatsTest::readVasp()
{
  /**** Some random VASP output ****/
  QString fileName = optimizerSamplePath("vasp", "CONTCAR");

  Search::Structure s;
  Search::VASPOptimizer(nullptr).read(&s, fileName);

  double tol = 1.e-5;

  // b should be 2.52221, gamma should be 86.32327, and the volume should
  // be 6.01496.
  QVERIFY(std::fabs(s.unitCell().b() - 2.52221) < tol);
  QVERIFY(std::fabs(s.unitCell().gamma() - 86.32327) < tol);
  QVERIFY(std::fabs(s.unitCell().volume() - 6.01496) < tol);

  // We should have three atoms
  QVERIFY(s.numAtoms() == 3);

  // Atoms::Atom #2 should be H and have a fractional position of
  // 0.9317195263978, 0.20419595775, 0.4923304223199
  QVERIFY(s.atom(1).atomicNumber() == 1);
  QVERIFY(Common::fuzzyCompare(
    s.unitCell().toFractional(s.atom(1).pos()),
    Common::Vector3(0.9317195263978, 0.20419595775, 0.4923304223199),
    tol));

  // Atoms::Atom #3 should be O and have a fractional position of
  // 0.087068957523, -0.1465596214494, 0.1524183445695
  QVERIFY(s.atom(2).atomicNumber() == 8);
  QVERIFY(Common::fuzzyCompare(
    s.unitCell().toFractional(s.atom(2).pos()),
    Common::Vector3(0.087068957523, -0.1465596214494, 0.1524183445695),
    tol));

  // Energy should be 5.56502673 eV.
  QVERIFY(std::fabs(s.getEnergy() - 5.56502673) < tol);

  // Enthalpy should be 43.10746559 eV
  QVERIFY(std::fabs(s.getEnthalpy() - 43.10746559) < tol);
}

QTEST_MAIN(FormatsTest)

#include "formatstest.moc"

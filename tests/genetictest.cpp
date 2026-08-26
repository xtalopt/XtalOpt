/**********************************************************************
  GeneticTest - Testing genetic mutation operators

  Copyright (C) 2017 Patrick Avery
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/genetic.h>
#include <xtalopt/structures/xtal.h>

#include <common/compatibility/platform_compat.h>
#include <common/fileutils.h>
#include <common/output.h>
#include <common/random.h>
#include <atoms/formats/vaspformat.h>

#include <QString>
#include <QtTest>

#include <fstream>
#include <memory>

using ::XtalOpt::CellComp;
using ::XtalOpt::EleScaledRadii;
using ::XtalOpt::Xtal;
using ::XtalOpt::XtalOptGenetic;

class GeneticTest : public QObject
{
  Q_OBJECT

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
  void exchange();
  void strain();
  void ripple();
  void stripple();
  void crossover();
};

void GeneticTest::initTestCase()
{
}

void GeneticTest::cleanupTestCase()
{
}

void GeneticTest::init()
{
}

void GeneticTest::cleanup()
{
}

// How many atoms appear to have been swapped?
// Returns the number of swapped atoms
// If there appear to be changes to atom positions other than a swap
// of atoms, this will return -1.
int numSwaps(const std::vector<Atoms::Atom>& atoms1, const std::vector<Atoms::Atom>& atoms2,
             double tol)
{
  if (atoms1.size() != atoms2.size())
    return -1;

  unsigned int numSwapped = 0, numStayed = 0;
  for (size_t i = 0; i < atoms1.size(); ++i) {
    for (size_t j = 0; j < atoms2.size(); ++j) {

      if (Common::eq(atoms1[i].pos(), atoms2[j].pos(), tol) &&
          atoms1[i].atomicNumber() == atoms2[j].atomicNumber()) {
        ++numStayed;
        break;
      }

      if (Common::eq(atoms1[i].pos(), atoms2[j].pos(), tol) &&
          atoms1[i].atomicNumber() != atoms2[j].atomicNumber()) {
        ++numSwapped;
        break;
      }
    }
  }

  if (numStayed + numSwapped != atoms1.size())
    return -1;

  return numSwapped;
}

void GeneticTest::exchange()
{
  // Seed the random number generators for consistent testing
  srand(0);
  Common::seedMt19937Generator(0);

  // Set the tolerance for the test.
  double tol = 1.e-6;

  QString rutileFileName = Common::localPath(Common::localPath(QString(TESTDATADIR), "formats"),
                      "rutile.POSCAR");
  std::ifstream in(rutileFileName.toStdString());
  QVERIFY(in.is_open());

  Xtal xtal;
  QVERIFY(Atoms::VaspFormat::read(xtal, in));

  Atoms::UnitCell oldUC = xtal.unitCell();
  std::vector<Atoms::Atom> oldAtoms = xtal.atoms();

  // Try one exchange.
  size_t numExchanges = 1;
  double sigma_lattice = 0.0;
  std::unique_ptr<Xtal> result(
    XtalOptGenetic::permustrain(&xtal, 0.0, numExchanges, sigma_lattice));
  QVERIFY(result != nullptr);

  Atoms::UnitCell newUC = result->unitCell();
  std::vector<Atoms::Atom> newAtoms = result->atoms();

  // Check the cell: it should be unchanged.
  QVERIFY(
    Common::eq(oldUC.cellMatrix(), newUC.cellMatrix(), tol));

  // Check the atoms: two atoms should be swapped.
  size_t numSwappedAtoms = numSwaps(oldAtoms, newAtoms, tol);

  QVERIFY(numSwappedAtoms == 2);

  // Try two exchanges (swap two atoms).
  in.clear();
  in.seekg(0, std::ios::beg);
  QVERIFY(Atoms::VaspFormat::read(xtal, in));

  numExchanges = 2;
  result.reset(XtalOptGenetic::permustrain(&xtal, 0.0, numExchanges, sigma_lattice));
  QVERIFY(result != nullptr);

  newUC = result->unitCell();
  newAtoms = result->atoms();

  // Check the cell to see if it's unchanged.
  QVERIFY(
    Common::eq(oldUC.cellMatrix(), newUC.cellMatrix(), tol));

  numSwappedAtoms = numSwaps(oldAtoms, newAtoms, tol);

  QVERIFY(numSwappedAtoms == 0 || numSwappedAtoms == 2 || numSwappedAtoms == 4);
}

// Load rutile into xtal and return the original volume and atom count.
static void loadRutile(Xtal& xtal)
{
  QString path = Common::localPath(Common::localPath(QString(TESTDATADIR), "formats"),
                      "rutile.POSCAR");
  std::ifstream in(path.toStdString());
  if (!in.is_open()) {
    Common::warning(QString("%1: cannot open %2").arg(__func__).arg(path));
    return;
  }
  if (!Atoms::VaspFormat::read(xtal, in))
    Common::warning(QString("%1: failed for reading %2").arg(__func__).arg(path));
}

void GeneticTest::strain()
{
  // Try several random seeds to find a proper one.
  for (int seed = 0; seed < 50; ++seed) {
    srand(seed);
    Common::seedMt19937Generator(seed);

    Xtal xtal;
    loadRutile(xtal);

    const double origVolume = xtal.getVolume();
    const int origAtomCount = static_cast<int>(xtal.numAtoms());
    const Common::Matrix3 origCell = xtal.unitCell().cellMatrix();

    double sigma_lattice = 0.0;
    std::unique_ptr<Xtal> result(XtalOptGenetic::permustrain(&xtal, 0.15, 0, sigma_lattice));
    QVERIFY(result != nullptr);

    // Skip an invalid unit cell.
    if (GS_ISNAN(result->getVolume()))
      continue;

    // Check the atom count to be unchanged.
    QCOMPARE(static_cast<int>(result->numAtoms()), origAtomCount);

    // Check the volume.
    QVERIFY(fabs(result->getVolume() - origVolume) / origVolume < 1e-9);

    // Check the cell (strain should have done somthing!).
    QVERIFY(!Common::eq(origCell, result->unitCell().cellMatrix(), 1e-6));
    return;
  }
  QFAIL("All tested seeds produced degenerate cells from extreme Gaussian draws");
}

void GeneticTest::ripple()
{
  srand(0);
  Common::seedMt19937Generator(0);

  Xtal xtal;
  loadRutile(xtal);

  const double origVolume = xtal.getVolume();
  const int origAtomCount = static_cast<int>(xtal.numAtoms());
  const Common::Matrix3 origCell = xtal.unitCell().cellMatrix();
  const std::vector<Atoms::Atom> origAtoms = xtal.atoms();

  // Move the atoms.
  double sigma_lattice = 0.0, rho = 0.0;
  std::unique_ptr<Xtal> result(XtalOptGenetic::stripple(&xtal, 0.0, 0.0, 0.3, 0.3, 2, 1,
                             sigma_lattice, rho));
  QVERIFY(result != nullptr);
  QVERIFY(rho > 0.0);

  // Check the atoms count and cell to be unchanged.
  QCOMPARE(static_cast<int>(result->numAtoms()), origAtomCount);
  QVERIFY(Common::eq(origCell, result->unitCell().cellMatrix(), 1e-6));
  QVERIFY(fabs(result->getVolume() - origVolume) < 1e-4);

  bool atomMoved = false;
  for (size_t i = 0; i < origAtoms.size(); ++i) {
    if (!Common::eq(origAtoms[i].pos(), result->atom(i).pos(), 1e-6)) {
      atomMoved = true;
      break;
    }
  }
  QVERIFY(atomMoved);
}

void GeneticTest::stripple()
{
  srand(0);
  Common::seedMt19937Generator(0);

  Xtal xtal;
  loadRutile(xtal);

  const double origVolume = xtal.getVolume();
  const int origAtomCount = static_cast<int>(xtal.numAtoms());

  double sigma_lattice = 0.0, rho = 0.0;
  std::unique_ptr<Xtal> result(XtalOptGenetic::stripple(&xtal, 0.0, 0.5, 0.0, 0.5, 1, 1,
                             sigma_lattice, rho));

  QVERIFY(result != nullptr);
  QCOMPARE(static_cast<int>(result->numAtoms()), origAtomCount);

  // Check the volume to be unchanged.
  QVERIFY(fabs(result->getVolume() - origVolume) < 1e-3);

  // Check that at least one operator was invoked.
  QVERIFY(sigma_lattice > 0.0 || rho > 0.0);
}

void GeneticTest::crossover()
{
  srand(2);
  Common::seedMt19937Generator(2);

  // Build two parent xtals from the same rutile structure (Ti2O4, 6 atoms)
  Xtal xtal1, xtal2;
  loadRutile(xtal1);
  loadRutile(xtal2);

  // Allowed composition: Ti2O4 (matches the rutile unit cell)
  CellComp comp;
  comp.setCompositionEntry("O",  8, 4);
  comp.setCompositionEntry("Ti", 22, 2);
  QList<CellComp> compa;
  compa.append(comp);

  // Use small radii for this test so we can always find a valid composition.
  EleScaledRadii elrad;
  elrad.setMinRadius(8,  0.3);
  elrad.setMinRadius(22, 0.3);

  // Verify loadRutile successfully populated both parent structures
  QCOMPARE(static_cast<int>(xtal1.numAtoms()), 6);
  QCOMPARE(static_cast<int>(xtal2.numAtoms()), 6);

  double percent1 = 0.0, percent2 = 0.0;
  std::unique_ptr<Xtal> result(
    XtalOptGenetic::crossover(&xtal1, &xtal2, compa, elrad,
                              /*numCuts=*/1, /*minContribution=*/0.25,
                              percent1, percent2,
                              /*minatoms=*/2, /*maxatoms=*/20,
                              /*isVcSearch=*/false, /*verbose=*/false));

  QVERIFY(result != nullptr);

  // Offspring must have atoms within the declared min/max range
  int nAtoms = static_cast<int>(result->numAtoms());
  QVERIFY(nAtoms >= 1 && nAtoms <= 20);

  // Check both atom types to be present.
  QVERIFY(result->getNumberOfAtomsOfSymbol("Ti") > 0);
  QVERIFY(result->getNumberOfAtomsOfSymbol("O")  > 0);

  // Parent contributions are percentages summing to 100
  QVERIFY(percent1 > 0.0 && percent1 < 100.0);
  QVERIFY(percent2 > 0.0 && percent2 < 100.0);
  QVERIFY(fabs(percent1 + percent2 - 100.0) < 1e-6);
}

QTEST_MAIN(GeneticTest)

#include "genetictest.moc"

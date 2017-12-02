/**********************************************************************
  Genetic test - make sure our mutators are working

  Copyright (C) 2017 Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 **********************************************************************/

#include <xtalopt/genetic.h>
#include <xtalopt/structures/xtal.h>

#include <globalsearch/formats/poscarformat.h>
#include <globalsearch/random.h>

#include <QDebug>
#include <QString>
#include <QtTest>

#include <fstream>
#include <memory>

using GlobalSearch::Atom;
using XtalOpt::Xtal;
using XtalOpt::XtalOptGenetic;

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
int numSwaps(const std::vector<Atom>& atoms1, const std::vector<Atom>& atoms2,
             double tol)
{
  if (atoms1.size() != atoms2.size())
    return -1;

  unsigned int numSwapped = 0, numStayed = 0;
  for (size_t i = 0; i < atoms1.size(); ++i) {
    for (size_t j = 0; j < atoms2.size(); ++j) {

      if (GlobalSearch::fuzzyCompare(atoms1[i].pos(), atoms2[j].pos(), tol) &&
          atoms1[i].atomicNumber() == atoms2[j].atomicNumber()) {
        ++numStayed;
        break;
      }

      if (GlobalSearch::fuzzyCompare(atoms1[i].pos(), atoms2[j].pos(), tol) &&
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
  GlobalSearch::seedMt19937Generator(0);

  // The tolerance for our tests
  double tol = 1.e-6;

  QString rutileFileName = QString(TESTDATADIR) + "/data/rutile.POSCAR";
  std::ifstream in(rutileFileName.toStdString());
  QVERIFY(in.is_open());

  Xtal xtal;
  QVERIFY(GlobalSearch::PoscarFormat::read(xtal, in));

  GlobalSearch::UnitCell oldUC = xtal.unitCell();
  std::vector<Atom> oldAtoms = xtal.atoms();

  // Try out one exchange
  size_t numExchanges = 1;
  XtalOptGenetic::exchange(&xtal, numExchanges);

  GlobalSearch::UnitCell newUC = xtal.unitCell();
  std::vector<Atom> newAtoms = xtal.atoms();

  // The unit cell should be unchanged
  QVERIFY(
    GlobalSearch::fuzzyCompare(oldUC.cellMatrix(), newUC.cellMatrix(), tol));

  // There should be two atoms that were swapped
  size_t numSwappedAtoms = numSwaps(oldAtoms, newAtoms, tol);

  QVERIFY(numSwappedAtoms == 2);

  // Now let's swap two atoms. Because we can possibly swap the same atoms
  // twice, we have the possibility of ending up with 0 swaps detected. So
  // the answer must be 0, 2, or 4.
  in.clear();
  in.seekg(0, std::ios::beg);
  QVERIFY(GlobalSearch::PoscarFormat::read(xtal, in));

  // Try out two exchanges
  numExchanges = 2;
  XtalOptGenetic::exchange(&xtal, numExchanges);

  newUC = xtal.unitCell();
  newAtoms = xtal.atoms();

  // The unit cell should be unchanged
  QVERIFY(
    GlobalSearch::fuzzyCompare(oldUC.cellMatrix(), newUC.cellMatrix(), tol));

  numSwappedAtoms = numSwaps(oldAtoms, newAtoms, tol);

  QVERIFY(numSwappedAtoms == 0 || numSwappedAtoms == 2 || numSwappedAtoms == 4);
}

QTEST_MAIN(GeneticTest)

#include "genetictest.moc"

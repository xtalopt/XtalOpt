/**********************************************************************
  SubMoleculeRankerTest

  Copyright (C) 2012 David C. Lonie

  XtalOpt is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
 **********************************************************************/

#include <xtalopt/submoleculeranker.h>

#include <xtalopt/structures/molecularxtal.h>
#include <xtalopt/structures/submolecule.h>
#include <xtalopt/structures/submoleculesource.h>

#include <avogadro/moleculefile.h>

#include <QtCore/QPair>
#include <QtCore/QVector>

#include <QtTest/QtTest>

#define USE_MATH_DEFINES
#include <cmath>

using namespace Avogadro;
using namespace XtalOpt;

bool fuzzyComp(double d1, double d2, double eps = 1e-2)
{
  if (fabs(d1-d2) < eps) {
    return true;
  }
  qDebug() << QString("fuzzy comp failed: fabs(%L1 - %L2) = %L3 > %L4")
              .arg(d1).arg(d2).arg(fabs(d1-d2)).arg(eps);
  return false;
}

class SubMoleculeRankerTest : public QObject
{
  Q_OBJECT

private:
  SubMoleculeSource *m_ureaSource;
  MolecularXtal *m_mxtal;
  SubMoleculeRanker *m_ranker;

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
  void benchmark_constructor();
  void benchmark_setMXtal();
  void benchmark_evaluate();
  void benchmark_evaluate_each();
  void benchmark_evaluateInter();
  void benchmark_evaluateInter_each();
  void benchmark_evaluateIntra();
  void benchmark_evaluateIntra_each();
  void benchmark_evaluateTotalEnergy();
  void benchmark_evaluateTotalEnergyInter();
  void benchmark_evaluateTotalEnergyIntra();

  void scan();

};

void SubMoleculeRankerTest::initTestCase()
{
  Molecule *urea = MoleculeFile::readMolecule(TESTDATADIR "/cmls/urea.cml");
  m_ureaSource = new SubMoleculeSource (urea);
  delete urea;
  urea = NULL;

  // Create a mxtal with three urea molecules
  m_mxtal = new MolecularXtal (5, 5, 5, 90, 90, 90);
  SubMolecule *sub = NULL;

  sub = m_ureaSource->getSubMolecule();
  m_mxtal->addSubMolecule(sub);
  sub->setCenter(Eigen::Vector3d(0.0, 0.0, 0.0));

  sub = m_ureaSource->getSubMolecule();
  m_mxtal->addSubMolecule(sub);
  sub->setCenter(Eigen::Vector3d(0.0, 0.0, 0.0));
  sub->translateFrac(0.25, 0.25, 0.25);
  sub->rotate(M_PI_2, sub->normal());

  sub = m_ureaSource->getSubMolecule();
  m_mxtal->addSubMolecule(sub);
  sub->setCenter(Eigen::Vector3d(0.0, 0.0, 0.0));
  sub->translateFrac(0.5, 0.75, 0.75);
  sub->rotate(M_PI_2, sub->normal().unitOrthogonal());

  MoleculeFile::writeMolecule(m_mxtal, "/tmp/threeUrea.cml");

  m_ranker = NULL;
}

void SubMoleculeRankerTest::cleanupTestCase()
{
  delete m_ureaSource;
  m_ureaSource = NULL;

  MoleculeFile::writeMolecule(m_mxtal, "/tmp/threeUrea-best.cml");

  delete m_mxtal;
  m_mxtal = NULL;
}

void SubMoleculeRankerTest::init()
{
}

void SubMoleculeRankerTest::cleanup()
{
}

void SubMoleculeRankerTest::benchmark_constructor()
{
  QBENCHMARK {
    delete m_ranker;
    m_ranker = new SubMoleculeRanker ();
  }
//  m_ranker->setDebug(true);
}

void SubMoleculeRankerTest::benchmark_setMXtal()
{
  QBENCHMARK {
    m_ranker->setMXtal(m_mxtal);
  }
}

void SubMoleculeRankerTest::benchmark_evaluate()
{
  QVector<double> energies;
  QBENCHMARK {
    energies = m_ranker->evaluate();
  }
//  qDebug() << energies;
  QVERIFY(fuzzyComp(energies[0], 5577.41));
  QVERIFY(fuzzyComp(energies[1], 1216.11));
  QVERIFY(fuzzyComp(energies[2], 4423.21));
}

void SubMoleculeRankerTest::benchmark_evaluate_each()
{
  int i = 0;
  foreach (const SubMolecule *submol, m_mxtal->subMolecules()) {
    double energy;
    QBENCHMARK {
      energy = m_ranker->evaluate(submol);
    }
//    qDebug() << energy;
    switch (i++) {
    case 0:
      QVERIFY(fuzzyComp(energy, 5577.41));
      break;
    case 1:
      QVERIFY(fuzzyComp(energy, 1216.11));
      break;
    case 2:
      QVERIFY(fuzzyComp(energy, 4423.21));
      break;
    }
  }
}

void SubMoleculeRankerTest::benchmark_evaluateInter()
{
  QVector<double> energies;
  QBENCHMARK {
    energies = m_ranker->evaluateInter();
  }
//  qDebug() << energies;
  QVERIFY(fuzzyComp(energies[0], 5567.77));
  QVERIFY(fuzzyComp(energies[1], 1206.47));
  QVERIFY(fuzzyComp(energies[2], 4413.58));
}

void SubMoleculeRankerTest::benchmark_evaluateInter_each()
{
  int i = 0;
  foreach (const SubMolecule *submol, m_mxtal->subMolecules()) {
    double energy;
    QBENCHMARK {
      energy = m_ranker->evaluateInter(submol);
    }
//    qDebug() << energy;
    switch (i++) {
    case 0:
      QVERIFY(fuzzyComp(energy, 5567.77));
      break;
    case 1:
      QVERIFY(fuzzyComp(energy, 1206.47));
      break;
    case 2:
      QVERIFY(fuzzyComp(energy, 4413.58));
      break;
    }
  }
}

void SubMoleculeRankerTest::benchmark_evaluateIntra()
{
  QVector<double> energies;
  QBENCHMARK {
    energies = m_ranker->evaluateIntra();
  }
//  qDebug() << energies;
  QVERIFY(fuzzyComp(energies[0], 9.63));
  QVERIFY(fuzzyComp(energies[1], 9.63));
  QVERIFY(fuzzyComp(energies[2], 9.63));
}

void SubMoleculeRankerTest::benchmark_evaluateIntra_each()
{
  int i = 0;
  foreach (const SubMolecule *submol, m_mxtal->subMolecules()) {
    double energy;
    QBENCHMARK {
      energy = m_ranker->evaluateIntra(submol);
    }
//    qDebug() << energy;
    switch (i++) {
    case 0:
      QVERIFY(fuzzyComp(energy, 9.63));
      break;
    case 1:
      QVERIFY(fuzzyComp(energy, 9.63));
      break;
    case 2:
      QVERIFY(fuzzyComp(energy, 9.63));
      break;
    }
  }
}

void SubMoleculeRankerTest::benchmark_evaluateTotalEnergy()
{
  double energy;
  QBENCHMARK {
    energy = m_ranker->evaluateTotalEnergy();
  }
//  qDebug() << energy;
  QVERIFY(fuzzyComp(energy, 9998.33));
}

void SubMoleculeRankerTest::benchmark_evaluateTotalEnergyInter()
{
  double energy;
  QBENCHMARK {
    energy = m_ranker->evaluateTotalEnergyInter();
  }
//    qDebug() << energy;
  QVERIFY(fuzzyComp(energy, 9969.43));
}

void SubMoleculeRankerTest::benchmark_evaluateTotalEnergyIntra()
{
  double energy;
  QBENCHMARK {
    energy = m_ranker->evaluateTotalEnergyIntra();
  }
//    qDebug() << energy;
  QVERIFY(fuzzyComp(energy, 28.90));
}

void SubMoleculeRankerTest::scan()
{
  // Rotate the third urea around its normal
  const double resolution = 20.0; // degree
  QHash<int, double> scanData; // degree angle, energy
  scanData.reserve(360 / resolution);

  SubMolecule *sub = m_mxtal->subMolecule(2);
  const Eigen::Vector3d axis = sub->normal();

  qDebug() << "Scanning" << 360/resolution << "orientations.";

  QBENCHMARK {
    for (double angle = 0; angle <= 360.001; angle += resolution) {

      if (fabs(angle) > 1e-3)
        sub->rotate(resolution * DEG_TO_RAD, axis);

      m_ranker->updateGeometry(sub);

      double energy = m_ranker->evaluate(sub);

      scanData.insert(angle, energy);
    }
  }

  QList<int> keys = scanData.keys();
  qSort(keys);
  double minEnergy = DBL_MAX;
  int minRot = INT_MAX;
  foreach (int key, keys) {
    double energy = scanData.value(key, -1);
    qDebug() << key << energy;

    if (energy < minEnergy) {
      minEnergy = energy;
      minRot = key;
    }
  }

  QVERIFY(fuzzyComp(minRot, 60));

  qDebug() << "Rotating by" << minRot << "for minimum energy configuration...";
  sub->rotate(minRot * DEG_TO_RAD, axis);
  m_ranker->updateGeometry(sub);

  qDebug() << "New total energy:" << m_ranker->evaluateTotalEnergy();
  qDebug() << "New submol energies:" << m_ranker->evaluate();

  // reset for when this function inevitably loops due to QTest's benchmarking
  sub->rotate(-minRot * DEG_TO_RAD, axis);
  m_ranker->updateGeometry();
}

QTEST_MAIN(SubMoleculeRankerTest)

#include "moc_submoleculerankertest.cxx"

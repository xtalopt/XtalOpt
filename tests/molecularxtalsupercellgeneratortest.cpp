/**********************************************************************
  MolecularXtalSuperCellGeneratorTest

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

#include <xtalopt/molecularxtalsupercellgenerator.h>

#include <xtalopt/structures/molecularxtal.h>
#include <xtalopt/structures/submolecule.h>
#include <xtalopt/structures/submoleculesource.h>
#include <xtalopt/submoleculeranker.h>

#include <avogadro/moleculefile.h>

#include <QtTest/QtTest>

#define _USE_MATH_DEFINES
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

class MolecularXtalSuperCellGeneratorTest : public QObject
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
  void test();
};

void MolecularXtalSuperCellGeneratorTest::initTestCase()
{
  Molecule *urea = MoleculeFile::readMolecule(TESTDATADIR "/cmls/urea.cml");
  m_ureaSource = new SubMoleculeSource (urea);
  m_ureaSource->setSourceId(0);
  delete urea;
  urea = NULL;

  // Create a mxtal with four urea molecules
  m_mxtal = new MolecularXtal (7, 7, 7, 90, 90, 90);
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

  sub = m_ureaSource->getSubMolecule();
  m_mxtal->addSubMolecule(sub);
  sub->setCenter(Eigen::Vector3d(0.0, 0.0, 0.0));
  sub->translateFrac(0.0, 0.75, 0.5);
  sub->rotate(M_PI_4, sub->normal().unitOrthogonal());

  MoleculeFile::writeMolecule(m_mxtal, "/tmp/fourUrea.cml");

  m_ranker = new SubMoleculeRanker ();
}

void MolecularXtalSuperCellGeneratorTest::cleanupTestCase()
{
  delete m_ureaSource;
  m_ureaSource = NULL;

  delete m_mxtal;
  m_mxtal = NULL;
}

void MolecularXtalSuperCellGeneratorTest::init()
{
}

void MolecularXtalSuperCellGeneratorTest::cleanup()
{
}

void MolecularXtalSuperCellGeneratorTest::test()
{
  m_ranker->setMXtal(m_mxtal);

  QVector<double> energies;
  energies = m_ranker->evaluate();

  MolecularXtalSuperCellGenerator gen;
  gen.setMXtal(*m_mxtal);
  gen.setSubMolecularEnergies(energies);
  gen.setDebug(true);

  QVector<MolecularXtal *> supercells = gen.getSuperCells();
  QVector<int> unassigned = gen.getUnassignedSubMolecules();

  QCOMPARE(unassigned.size(), 0);
  int i = 0;
  qDebug() << "Supercell:" << "P" << "parent"
           << m_ranker->evaluateTotalEnergy();
  foreach (MolecularXtal *supercell, supercells) {
    const QString cellName = supercell->property("supercellType").toString();
    *m_mxtal = *supercell;
    m_ranker->updateGeometry();
    double energy = m_ranker->evaluateTotalEnergy();
    MoleculeFile::writeMolecule(supercell, QString("/tmp/fourUreaSC%1.cml")
                                .arg(i++), "cml", "p");
    qDebug() << "Supercell:" << i << cellName << energy;
  }
}

QTEST_MAIN(MolecularXtalSuperCellGeneratorTest)

#include "moc_molecularxtalsupercellgeneratortest.cxx"

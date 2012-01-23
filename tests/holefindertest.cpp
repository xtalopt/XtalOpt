/**********************************************************************
  HoleFinderTest

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

#include <xtalopt/holefinder.h>

#include <xtalopt/structures/molecularxtal.h>
#include <xtalopt/structures/submolecule.h>
#include <xtalopt/structures/submoleculesource.h>

#include <globalsearch/obeigenconv.h>

#include <avogadro/moleculefile.h>

#include <QtTest/QtTest>

using namespace Avogadro;
using namespace XtalOpt;

class HoleFinderTest : public QObject
{
  Q_OBJECT

private:
  HoleFinder *m_finder;
  SubMoleculeSource *m_ureaSource;
  MolecularXtal *m_mxtal;

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
  void benchmark();
  void test();

};

void HoleFinderTest::initTestCase()
{
  Molecule *urea = MoleculeFile::readMolecule(TESTDATADIR "/cmls/urea.cml");
  m_ureaSource = new SubMoleculeSource (urea);
  delete urea;
  urea = NULL;

  // Create a mxtal with three urea molecules
  m_mxtal = new MolecularXtal (7.0, 7.0, 7.0, 90, 90, 90);
  SubMolecule *sub = NULL;

  sub = m_ureaSource->getSubMolecule();
  m_mxtal->addSubMolecule(sub);
  sub->setCenter(Eigen::Vector3d(0.0, 0.0, 0.0));

  sub = m_ureaSource->getSubMolecule();
  m_mxtal->addSubMolecule(sub);
  sub->setCenter(Eigen::Vector3d(0.0, 0.0, 0.0));
  sub->translateFrac(0.5, 0.5, 0.5);
  sub->rotate(M_PI_2, sub->normal());
  sub->rotate(M_PI_4, sub->normal().unitOrthogonal());

  sub = m_ureaSource->getSubMolecule();
  m_mxtal->addSubMolecule(sub);
  sub->setCenter(Eigen::Vector3d(0.0, 0.0, 0.0));
  sub->translateFrac(0.0, 0.0, 0.25);
  sub->rotate(M_PI_2, sub->normal().unitOrthogonal());

  sub = m_ureaSource->getSubMolecule();
  m_mxtal->addSubMolecule(sub);
  sub->setCenter(Eigen::Vector3d(0.0, 0.0, 0.0));
  sub->translateFrac(0.5, 0.75, 0.75);
  sub->rotate(M_PI_4, sub->normal().unitOrthogonal());
  sub->rotate(M_PI_4, sub->normal());

  m_finder = NULL;
}

void HoleFinderTest::cleanupTestCase()
{
  delete m_ureaSource;
  m_ureaSource = NULL;

  delete m_mxtal;
  m_mxtal = NULL;

  delete m_finder;
  m_finder = NULL;
}

void HoleFinderTest::init()
{
}

void HoleFinderTest::cleanup()
{
}

void HoleFinderTest::benchmark()
{
  delete m_finder;
  m_finder = new HoleFinder (m_mxtal->numAtoms());

  m_finder->setDebug(true);
  m_finder->setGridResolution(0.5);
  m_finder->setMinimumHoleRadius(2.0);
  m_finder->setMinimumDistance(1.5);
  m_finder->setTranslations(
        OB2Eigen(m_mxtal->OBUnitCell()->GetCellMatrix().GetRow(0)),
        OB2Eigen(m_mxtal->OBUnitCell()->GetCellMatrix().GetRow(1)),
        OB2Eigen(m_mxtal->OBUnitCell()->GetCellMatrix().GetRow(2)));

  m_mxtal->niggliReduce();
  m_mxtal->wrapAtomsToCell();

  foreach (const Atom *atom, m_mxtal->atoms())
    m_finder->addPoint(*atom->pos());

  QBENCHMARK {
    m_finder->run();
  }
}

#include <globalsearch/macros.h>

void HoleFinderTest::test()
{
  // Add tiny florine atoms to randomly selected hole points
  for (int i = 0; i < 5000; ++i)
    m_mxtal->addAtom(9, m_finder->getRandomPoint())->setCustomRadius(0.1);

  // Remove all bonds, they get in the way of visualizing the structure
  while (m_mxtal->numBonds() != 0)
    m_mxtal->removeBond(m_mxtal->bond(0));
  m_mxtal->wrapAtomsToCell();

  MoleculeFile::writeMolecule(m_mxtal, "/tmp/holefinder.cml");
}

QTEST_MAIN(HoleFinderTest)

#include "moc_holefindertest.cxx"

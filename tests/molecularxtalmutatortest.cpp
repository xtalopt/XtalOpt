/**********************************************************************
  MolecularXtalMutatorTest

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

#include <xtalopt/molecularxtalmutator.h>

#include <xtalopt/structures/molecularxtal.h>
#include <xtalopt/structures/submolecule.h>
#include <xtalopt/structures/submoleculesource.h>

#include <avogadro/moleculefile.h>

#include <QtTest/QtTest>

using namespace Avogadro;
using namespace XtalOpt;

class MolecularXtalMutatorTest : public QObject
{
  Q_OBJECT

private:
  SubMoleculeSource *m_ureaSource;
  MolecularXtal *m_parentMMFF94;
  MolecularXtal *m_offspringMMFF94;
  MolecularXtal *m_parentUFF;
  MolecularXtal *m_offspringUFF;

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
  void testMMFF94();
  void testUFF();

  void testMMFF94CreateBest();
  void testUFFCreateBest();

  void testMMFF94StartWithSuperCell();
  void testUFFStartWithSuperCell();
};

void MolecularXtalMutatorTest::initTestCase()
{
  Molecule *urea = MoleculeFile::readMolecule(TESTDATADIR "/cmls/urea.cml");
  m_ureaSource = new SubMoleculeSource (urea);
  m_ureaSource->setSourceId(0);
  delete urea;
  urea = NULL;

  // Create a mxtal with three urea molecules
  m_parentMMFF94 = new MolecularXtal (11.13, 5.565, 4.684, 90, 90, 90);
  SubMolecule *sub = NULL;

  sub = m_ureaSource->getSubMolecule();
  m_parentMMFF94->addSubMolecule(sub);
  sub->setCenter(Eigen::Vector3d(0.0, 0.0, 0.0));

  sub = m_ureaSource->getSubMolecule();
  m_parentMMFF94->addSubMolecule(sub);
  sub->setCenter(Eigen::Vector3d(0.0, 0.0, 0.0));
  sub->translateFrac(0.25, 0.25, 0.25);
  sub->rotate(M_PI_2, sub->normal());

  sub = m_ureaSource->getSubMolecule();
  m_parentMMFF94->addSubMolecule(sub);
  sub->setCenter(Eigen::Vector3d(0.0, 0.0, 0.0));
  sub->translateFrac(0.5, 0.75, 0.75);
  sub->rotate(M_PI_2, sub->normal().unitOrthogonal());

  sub = m_ureaSource->getSubMolecule();
  m_parentMMFF94->addSubMolecule(sub);
  sub->setCenter(Eigen::Vector3d(0.0, 0.0, 0.0));
  sub->translateFrac(0.75, 0.25, 0.75);
  sub->rotate(M_PI_4, sub->normal().unitOrthogonal());
  sub->rotate(M_PI_4, sub->normal());

  // Something weird, like BF2, will cause the mutator to reset to UFF
  Molecule bf2;
  Atom *b  = bf2.addAtom(5, Eigen::Vector3d( 0.0, 0.0, 0.0));
  Atom *f1 = bf2.addAtom(9, Eigen::Vector3d( 1.0, 0.0, 0.0));
  Atom *f2 = bf2.addAtom(9, Eigen::Vector3d(-1.0, 0.0, 0.0));

  bf2.addBond(b, f1);
  bf2.addBond(b, f2);

  SubMoleculeSource bf2Source (&bf2);
  bf2Source.setSourceId(0);
  m_parentUFF = new MolecularXtal (10.0, 10.0, 10.0, 90.0, 90.0, 90.0);

  sub = bf2Source.getSubMolecule();
  m_parentUFF->addSubMolecule(sub);

  sub = bf2Source.getSubMolecule();
  m_parentUFF->addSubMolecule(sub);
  sub->translateFrac(0.5, 0.5, 0.5);

  m_offspringMMFF94 = NULL;
  m_offspringUFF = NULL;
}

void MolecularXtalMutatorTest::cleanupTestCase()
{
  delete m_ureaSource;
  m_ureaSource = NULL;

//  MoleculeFile::writeMolecule(m_parentMMFF94, "/tmp/mxtalmutator-parent.cml");
//  MoleculeFile::writeMolecule(m_offspring, "/tmp/mxtalmutator-offspring.cml");

  delete m_parentMMFF94;
  m_parentMMFF94 = NULL;

  delete m_offspringMMFF94;
  m_offspringMMFF94 = NULL;

  delete m_parentUFF;
  m_parentUFF = NULL;

  delete m_offspringUFF;
  m_offspringUFF = NULL;
}


void MolecularXtalMutatorTest::init()
{
}

void MolecularXtalMutatorTest::cleanup()
{
}

void MolecularXtalMutatorTest::testMMFF94()
{
  MolecularXtalMutator mutator (m_parentMMFF94);

  mutator.setDebug(true);
  mutator.setNumberOfStrains(3);
  mutator.setStrainSigmaRange(0.0, 0.5);
  mutator.setNumMovers(1);
  mutator.setNumDisplacements(2);
  mutator.setRotationResolution(90.0 * DEG_TO_RAD);

  mutator.mutate();

  m_offspringMMFF94 = mutator.getOffspring();
}

void MolecularXtalMutatorTest::testUFF()
{
  MolecularXtalMutator mutator (m_parentUFF);

  mutator.setDebug(true);
  mutator.setNumberOfStrains(3);
  mutator.setStrainSigmaRange(0.0, 0.5);
  mutator.setNumMovers(1);
  mutator.setNumDisplacements(2);
  mutator.setRotationResolution(90.0 * DEG_TO_RAD);

  mutator.mutate();

  m_offspringUFF = mutator.getOffspring();
}

void MolecularXtalMutatorTest::testMMFF94CreateBest()
{
  MolecularXtalMutator mutator (m_parentMMFF94);

  mutator.setDebug(true);
  mutator.setCreateBest(true);
  mutator.setNumberOfStrains(3);
  mutator.setStrainSigmaRange(0.0, 0.5);
  mutator.setNumMovers(1);
  mutator.setNumDisplacements(2);
  mutator.setRotationResolution(90.0 * DEG_TO_RAD);

  mutator.mutate();
}

void MolecularXtalMutatorTest::testUFFCreateBest()
{
  MolecularXtalMutator mutator (m_parentUFF);

  mutator.setDebug(true);
  mutator.setCreateBest(true);
  mutator.setNumberOfStrains(3);
  mutator.setStrainSigmaRange(0.0, 0.5);
  mutator.setNumMovers(1);
  mutator.setNumDisplacements(2);
  mutator.setRotationResolution(90.0 * DEG_TO_RAD);

  mutator.mutate();
}

void MolecularXtalMutatorTest::testMMFF94StartWithSuperCell()
{
  MolecularXtalMutator mutator (m_parentMMFF94);

  mutator.setDebug(true);
  mutator.setStartWithSuperCell(true);
  mutator.setNumberOfStrains(3);
  mutator.setStrainSigmaRange(0.0, 0.5);
  mutator.setNumMovers(1);
  mutator.setNumDisplacements(2);
  mutator.setRotationResolution(90.0 * DEG_TO_RAD);

  mutator.mutate();

//  MolecularXtal *offspring = mutator.getOffspring();
//  Avogadro::MoleculeFile::writeMolecule(offspring,
//                                        "/tmp/supercellMMFF94.cml");
}

void MolecularXtalMutatorTest::testUFFStartWithSuperCell()
{
  MolecularXtalMutator mutator (m_parentUFF);

  mutator.setDebug(true);
  mutator.setStartWithSuperCell(true);
  mutator.setNumberOfStrains(3);
  mutator.setStrainSigmaRange(0.0, 0.5);
  mutator.setNumMovers(1);
  mutator.setNumDisplacements(2);
  mutator.setRotationResolution(90.0 * DEG_TO_RAD);

  mutator.mutate();

//  MolecularXtal *offspring = mutator.getOffspring();
//  Avogadro::MoleculeFile::writeMolecule(offspring,
//                                    "/tmp/supercellUFF.cml");
}

QTEST_MAIN(MolecularXtalMutatorTest)

#include "moc_molecularxtalmutatortest.cxx"

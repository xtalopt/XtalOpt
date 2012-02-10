/**********************************************************************
  MolecularXtalTest

  Copyright (C) 2011 David C. Lonie

  XtalOpt is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
 **********************************************************************/

#include <xtalopt/structures/molecularxtal.h>

#include <xtalopt/structures/submolecule.h>
#include <xtalopt/structures/submoleculesource.h>

#include <avogadro/atom.h>
#include <avogadro/bond.h>

#include <QtTest/QtTest>

using namespace Avogadro;
using namespace XtalOpt;

class MolecularXtalTest : public QObject
{
  Q_OBJECT

  private:
  SubMoleculeSource m_source;

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
  void subMoleculeHandling();
  void equalityTests();
};

void MolecularXtalTest::initTestCase()
{
  // Construct source
  Atom *o1 = m_source.addAtom();
  o1->setAtomicNumber(8);
  o1->setPos(Eigen::Vector3d(0,0,0));

  Atom *h1 = m_source.addAtom();
  h1->setAtomicNumber(1);
  h1->setPos(Eigen::Vector3d(.75,.75,0));

  Atom *h2 = m_source.addAtom();
  h2->setAtomicNumber(1);
  h2->setPos(Eigen::Vector3d(.75,-.75,0));

  Bond *oh1 = m_source.addBond();
  oh1->setBegin(o1);
  oh1->setEnd(h1);
  oh1->setOrder(1);

  Bond *oh2 = m_source.addBond();
  oh2->setBegin(o1);
  oh2->setEnd(h2);
  oh2->setOrder(1);
}

void MolecularXtalTest::cleanupTestCase()
{
}

void MolecularXtalTest::init()
{
}

void MolecularXtalTest::cleanup()
{
}

void MolecularXtalTest::subMoleculeHandling()
{
  MolecularXtal mxtal (10,10,10,90,90,90);

  // Fetch submolecule
  SubMolecule *sub = m_source.getRandomSubMolecule();

  /////////////////
  // Begin tests //
  /////////////////

  // Verify that the atom counts start off at what we expect
  QVERIFY(mxtal.numAtoms() == 0);
  QVERIFY(sub->numAtoms() == 3);

  // Add SubMolecule to mxtal
  mxtal.addSubMolecule(sub);

  // Verify atom counts
  QVERIFY(mxtal.numAtoms() == 3);
  QVERIFY(sub->numAtoms() == 3);

  // Check that the atom pointers are the same:
  QVERIFY(sub->atom(0) == mxtal.atom(0));
  QVERIFY(sub->atom(1) == mxtal.atom(1));
  QVERIFY(sub->atom(2) == mxtal.atom(2));

  // Translate sub to center of cell, and verify that the new positions are
  // the same
  sub->translateFrac(0.5, 0.5, 0.5);
  QVERIFY(mxtal.center().isApprox(sub->center(), 1e-6));

  /* This no longer works since Avogadro::Molecule::center() returns the
    center of the unit cell, not the center of the atoms.

  // Translate the SubMolecule, check that mxtal reflects change
  sub->translate(2.0, -3.0, 1.0);
  Eigen::Vector3d mxcent = mxtal.center();
  Eigen::Vector3d subcent = sub->center();

  QVERIFY(mxcent.isApprox(subcent, 1e-6));
  */

  // Verify that one submolecule exists
  QVERIFY(mxtal.numSubMolecules() == 1);

  // Remove sub from mxtal
  mxtal.removeSubMolecule(sub);

  // Verify counts
  QVERIFY(mxtal.numSubMolecules() == 0);
  QVERIFY(mxtal.numAtoms() == 0);
  QVERIFY(sub->numAtoms() == 3);
}

void MolecularXtalTest::equalityTests()
{
  // Test that identical mxtals match
  MolecularXtal mxtal1 (5.0, 5.0, 5.0, 90.0, 90.0, 90.0);
  MolecularXtal mxtal2 (5.0, 5.0, 5.0, 90.0, 90.0, 90.0);

  SubMolecule *sub = m_source.getSubMolecule();
  mxtal1.addSubMolecule(sub);

  sub = m_source.getSubMolecule();
  mxtal2.addSubMolecule(sub);

  sub = m_source.getSubMolecule();
  mxtal1.addSubMolecule(sub);
  sub->translateFrac(0.5, 0.5, 0.5);

  sub = m_source.getSubMolecule();
  mxtal2.addSubMolecule(sub);
  sub->translateFrac(0.5, 0.5, 0.5);

  QVERIFY(mxtal1 == mxtal2);

  // Verify that moving a submolecule makes the test fail
  sub->translate(1.0, 1.0, 1.0);
  QVERIFY(mxtal1 != mxtal2);

  // Move submolecule back to original position
  sub->translate(-1.0, -1.0, -1.0);

  // Check that they match again
  QVERIFY(mxtal1 == mxtal2);

  // Displace some hydrogen atoms
  const Eigen::Vector3d disp (1.0, 1.0, 1.0);
  foreach (Atom *atom, sub->atoms()) {
    if (atom->isHydrogen()) {
      atom->setPos(*atom->pos() + disp);
    }
  }

  // They should still match
  QVERIFY(mxtal1 == mxtal2);
}

QTEST_MAIN(MolecularXtalTest)

#include "moc_molecularxtaltest.cxx"

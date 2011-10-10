/**********************************************************************
  SubMoleculeTest

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

#include <xtalopt/structures/submolecule.h>

#include <xtalopt/structures/molecularxtal.h>
#include <xtalopt/structures/submoleculesource.h>

#include <globalsearch/macros.h>
#include <globalsearch/obeigenconv.h>

#include <avogadro/atom.h>
#include <avogadro/bond.h>
#include <avogadro/moleculefile.h>

#include <Eigen/Core>

#include <QtTest/QtTest>

using namespace Avogadro;
using namespace XtalOpt;

class SubMoleculeTest : public QObject
{
  Q_OBJECT

private:
  SubMoleculeSource *m_source_h2o;
  SubMoleculeSource *m_source_urea;

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
  void center();
  void farthestAtom();
  void radius();

  void makeCoherent();
  void getCoherentClone();

  void takeOwnership();
  void releaseOwnership();

  void rotate();
  void translate();
  void translateFrac();

  void wrapAtomsIntoUnitCell();
};

void SubMoleculeTest::initTestCase()
{
  // Cleaned up when test object is deleted:
  m_source_h2o = new SubMoleculeSource (this);
  Atom *o1, *h1, *h2;
  Bond *oh1, *oh2;

  o1 = m_source_h2o->addAtom();
  o1->setPos(Eigen::Vector3d(0,0,0));
  o1->setAtomicNumber(8);

  h1 = m_source_h2o->addAtom();
  h1->setPos(Eigen::Vector3d(0.75,0.75,0));
  h1->setAtomicNumber(1);

  h2 = m_source_h2o->addAtom();
  h2->setPos(Eigen::Vector3d(0.75,-0.75,0));
  h2->setAtomicNumber(1);

  oh1 = m_source_h2o->addBond();
  oh1->setOrder(1);
  oh1->setBegin(o1);
  oh1->setEnd(h1);

  oh2 = m_source_h2o->addBond();
  oh2->setOrder(1);
  oh2->setBegin(o1);
  oh2->setEnd(h2);

  Molecule *urea = MoleculeFile::readMolecule(TESTDATADIR "/cmls/urea.cml");
  m_source_urea = new SubMoleculeSource (urea);
  delete urea;
  urea = NULL;
}

void SubMoleculeTest::cleanupTestCase()
{
}

void SubMoleculeTest::init()
{
}

void SubMoleculeTest::cleanup()
{
}

// Actual tests begin:
void SubMoleculeTest::center()
{
  SubMolecule *sub = m_source_h2o->getRandomSubMolecule();

  // The submolecule should initially be centered:
  QVERIFY(sub->center().isZero(1e-5));

  delete sub;
}

void SubMoleculeTest::farthestAtom()
{
  SubMolecule *sub = m_source_h2o->getRandomSubMolecule();

  // The farthest atom from the center should be the oxygen
  QVERIFY(sub->farthestAtom()->atomicNumber() == 8);
  delete sub;
}

void SubMoleculeTest::radius()
{
  SubMolecule *sub = m_source_h2o->getRandomSubMolecule();

  // The farthest atom (oxygen) should be 0.5 angstrom from center.
  QVERIFY(sub->radius());
  delete sub;
}

void SubMoleculeTest::makeCoherent()
{
  // Get submolecule from source, store a copy for later
  SubMolecule *sub = m_source_urea->getSubMolecule(0);
  SubMolecule *sub_cached = m_source_urea->getSubMolecule(0);

  // Create mxtal with large unit cell
  MolecularXtal mxtal (10.0, 10.0, 10.0, 90.0, 90.0, 90.0);

  // Add submolecule to mxtal and center on the origin
  mxtal.addSubMolecule(sub);
  sub->translate(-sub->center());

  // Wrapping atoms to the cell will split the molecule across coherent images
  mxtal.wrapAtomsToCell();

  // Make the submolecule coherent
  sub->makeCoherent();

  // Center each submolecule
  sub->translate(-sub->center());
  sub_cached->translate(-sub_cached->center());

  // Check that the molecules are still the same
  QVERIFY(sub->numAtoms() == sub_cached->numAtoms());
  QVERIFY(sub->numBonds() == sub_cached->numBonds());
  for (int i = 0; i < sub->numAtoms(); ++i) {
    sub->atom(i)->pos()->isApprox(*sub_cached->atom(i)->pos(), 1e-6);
    QVERIFY(sub->atom(i)->neighbors() == sub_cached->atom(i)->neighbors());
  }

  // Make some random translations and test if structure is preserved.
  sub->rotate(M_PI * 0.25, sub->normal().unitOrthogonal());
  for (int i = 0; i < 1000; ++i) {
    Eigen::Vector3d trans (RANDDOUBLE(), RANDDOUBLE(), RANDDOUBLE());
    sub->translateFrac(trans);
    sub->wrapAtomsIntoUnitCell();
    sub->makeCoherent();
    sub->assertCoherentBondLengths(
          OB2Eigen(mxtal.OBUnitCell()->GetCellMatrix()), sub_cached);
  }

  // mxtal will clean up sub.
  delete sub_cached;
}

void SubMoleculeTest::getCoherentClone()
{
  // Get submolecule from source, store a copy for later
  SubMolecule *sub = m_source_h2o->getSubMolecule(0);
  SubMolecule *sub_cached = m_source_h2o->getSubMolecule(0);

  // Create mxtal with large unit cell
  MolecularXtal mxtal (10.0, 10.0, 10.0, 90.0, 90.0, 90.0);

  // Add submolecule to mxtal and center on the origin
  mxtal.addSubMolecule(sub);
  sub->translate(-sub->center());

  // Wrapping atoms to the cell will split the molecule across coherent images
  mxtal.wrapAtomsToCell();

  // Make the submolecule coherent
  SubMolecule *clone = sub->getCoherentClone();

  // This is a new object...
  QVERIFY(clone != sub);

  // Center each submolecule
  clone->translate(-clone->center());
  sub_cached->translate(-sub_cached->center());

  // Check that the molecules are still the same
  QVERIFY(clone->numAtoms() == sub_cached->numAtoms());
  QVERIFY(clone->numBonds() == sub_cached->numBonds());
  for (int i = 0; i < clone->numAtoms(); ++i) {
    clone->atom(i)->pos()->isApprox(*sub_cached->atom(i)->pos(), 1e-6);
    QVERIFY(clone->atom(i)->neighbors() == sub_cached->atom(i)->neighbors());
  }

  // mxtal will clean up sub.
  delete clone;
  delete sub_cached;
}

void SubMoleculeTest::takeOwnership()
{
  QEXPECT_FAIL("", "Test not implemented.", Continue);
  QCOMPARE(true, false);
}

void SubMoleculeTest::releaseOwnership()
{
  QEXPECT_FAIL("", "Test not implemented.", Continue);
  QCOMPARE(true, false);
}

void SubMoleculeTest::rotate()
{
  SubMolecule *sub = m_source_h2o->getRandomSubMolecule();

  // Rotate into xy-plane: Align the cross product of the bond vectors
  // with the z-axis
  Q_ASSERT(sub->numBonds() == 2);
  const Eigen::Vector3d b1= *sub->bond(0)->beginPos()-*sub->bond(0)->endPos();
  const Eigen::Vector3d b2= *sub->bond(1)->beginPos()-*sub->bond(1)->endPos();
  const Eigen::Vector3d cross = b1.cross(b2).normalized();

  // Axis is the cross-product of cross with zhat:
  const Eigen::Vector3d axis = cross.cross(Eigen::Vector3d::UnitZ()).normalized();

  // Angle is the angle between cross and jhat:
  const double angle = acos(cross.dot(Eigen::Vector3d::UnitZ()));

  // Rotate the submolecule
  sub->rotate(angle, axis);

  // Verify that the molecule is in the xy-plane
  QVERIFY(fabs(sub->atom(0)->pos()->z()) < 1e-2);
  QVERIFY(fabs(sub->atom(1)->pos()->z()) < 1e-2);
  QVERIFY(fabs(sub->atom(2)->pos()->z()) < 1e-2);
  delete sub;
}

void SubMoleculeTest::translate()
{
  SubMolecule *sub = m_source_h2o->getRandomSubMolecule();

  // Place the first atom at the origin
  sub->translate(-*sub->atom(0)->pos());
  QVERIFY(sub->atom(0)->pos()->isZero(1e-6));
  delete sub;
}

void SubMoleculeTest::translateFrac()
{
  QEXPECT_FAIL("", "Test not implemented.", Continue);
  QCOMPARE(true, false);
}

void SubMoleculeTest::wrapAtomsIntoUnitCell()
{
  SubMolecule *sub = m_source_h2o->getSubMolecule(0);
  const SubMolecule *ref = m_source_h2o->getSubMolecule(0);

  Eigen::Matrix3d rowVectors;
  rowVectors << 5.0, 0.0, 0.0,  0.0, 5.0, 0.0,  0.0, 0.0, 5.0;

  sub->translate(25.0, 25.0, 25.0);
  sub->wrapAtomsIntoUnitCell(rowVectors);

  // Verify that all atoms are in the unit cell:
  foreach (const Atom *atom, sub->atoms()) {
    const Eigen::Vector3d *pos = atom->pos();
    qDebug() << pos->x() << pos->y() << pos->z();
    QVERIFY(pos->x() >= 0.0);
    QVERIFY(pos->x() <= 5.0);
    QVERIFY(pos->y() >= 0.0);
    QVERIFY(pos->y() <= 5.0);
    QVERIFY(pos->z() >= 0.0);
    QVERIFY(pos->z() <= 5.0);
  }

  // Make sub coherent and check that all bond lengths are the same
  sub->makeCoherent(NULL, rowVectors);

  for (int bondInd = 0; bondInd < sub->numBonds(); ++bondInd) {
    const Bond *subBond = sub->bond(bondInd);
    const Bond *refBond = ref->bond(bondInd);

    QVERIFY(fabs(subBond->length() - refBond->length()) < 1e-5);
  }
}

QTEST_MAIN(SubMoleculeTest)

#include "moc_submoleculetest.cxx"

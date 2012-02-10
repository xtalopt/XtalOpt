/**********************************************************************
  SubMoleculeSourceTest

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

#include <xtalopt/structures/submoleculesource.h>

#include <xtalopt/structures/submolecule.h>

#include <avogadro/atom.h>
#include <avogadro/bond.h>
#include <avogadro/molecule.h>

#include <openbabel/atom.h>
#include <openbabel/bond.h>
#include <openbabel/mol.h>
#include <openbabel/forcefield.h>

#include <QtTest/QtTest>

using namespace Avogadro;
using namespace XtalOpt;

class SubMoleculeSourceTest : public QObject
{
  Q_OBJECT

  private:
  SubMoleculeSource *m_randConfSource;
  SubMoleculeSource *m_sysConfSource;

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
  void setOB();
  void setAvo();

  void findAndSetConformers_rand();
  void findAndSetConformers_sys();

  void uffFallback();

  void getRandomSubMolecule();
};

#define ADD_ATOM(token,num,x,y,z,target) \
  Atom *token = target->addAtom(); \
  token->setPos(Eigen::Vector3d(x,y,z)); \
  token->setAtomicNumber(num)

#define ADD_BOND(token,order,atm1,atm2,target) \
  Bond *token = target->addBond(); \
  token->setOrder(order); \
  token->setBegin(atm1); \
  token->setEnd(atm2)

void SubMoleculeSourceTest::initTestCase()
{
  m_randConfSource = new SubMoleculeSource (this);
  { // Limit scope
    ADD_ATOM(a01, 35,  2.7496150637,  0.5529165646, -0.9212755061, m_randConfSource); //Br
    ADD_ATOM(a02, 35,  2.4668400485,  1.3831541868,  2.5018663900, m_randConfSource); //Br
    ADD_ATOM(a03, 35, -0.9637117405,  1.6874348278,  2.1938329282, m_randConfSource); //Br
    ADD_ATOM(a04, 17,  2.2352309504, -1.6034097347,  2.0735200693, m_randConfSource); //Cl
    ADD_ATOM(a05, 17, -1.1897838133, -1.2935345170,  2.1593501079, m_randConfSource); //Cl
    ADD_ATOM(a06,  9,  0.3865973947,  1.3209619409, -1.9245361993, m_randConfSource); //F1
    ADD_ATOM(a07,  9,  0.1789174426, -1.2796531621, -1.9930159488, m_randConfSource); //F2
    ADD_ATOM(a08,  9, -1.3077835616,  0.3417664000, -0.3533936085, m_randConfSource); //F3
    ADD_ATOM(a09,  6,  0.1952602909, -1.3855476163, -0.6348555098, m_randConfSource); //C1
    ADD_ATOM(a10,  6,  0.8791334701,  1.0612033545, -0.6799136697, m_randConfSource); //C2
    ADD_ATOM(a11,  6,  0.0000000000,  0.0000000000,  0.0000000000, m_randConfSource); //C3
    ADD_ATOM(a12,  6,  1.2656498305, -0.1577122662,  2.4350904828, m_randConfSource); //C4
    ADD_ATOM(a13,  6,  0.0000000000,  0.0000000000,  1.5658587598, m_randConfSource); //C5
    ADD_ATOM(a14,  1,  0.8795277479,  2.0130071080, -0.1458137231, m_randConfSource); //H1
    ADD_ATOM(a15,  1, -0.6251159940, -2.0635927806, -0.3860184702, m_randConfSource); //H2
    ADD_ATOM(a16,  1,  1.1328161512, -1.8690292178, -0.3672323990, m_randConfSource); //H3
    ADD_ATOM(a17,  1,  0.9790110257, -0.2549204749,  3.4888524258, m_randConfSource); //H4

    ADD_BOND(b01, 1, a02, a12, m_randConfSource);
    ADD_BOND(b02, 1, a17, a12, m_randConfSource);
    ADD_BOND(b03, 1, a04, a12, m_randConfSource);
    ADD_BOND(b04, 1, a13, a12, m_randConfSource);
    ADD_BOND(b05, 1, a13, a03, m_randConfSource);
    ADD_BOND(b06, 1, a13, a05, m_randConfSource);
    ADD_BOND(b07, 1, a13, a11, m_randConfSource);
    ADD_BOND(b08, 1, a08, a11, m_randConfSource);
    ADD_BOND(b09, 1, a10, a11, m_randConfSource);
    ADD_BOND(b10, 1, a09, a11, m_randConfSource);
    ADD_BOND(b11, 1, a09, a07, m_randConfSource);
    ADD_BOND(b12, 1, a09, a16, m_randConfSource);
    ADD_BOND(b13, 1, a09, a15, m_randConfSource);
    ADD_BOND(b14, 1, a10, a01, m_randConfSource);
    ADD_BOND(b15, 1, a10, a06, m_randConfSource);
    ADD_BOND(b16, 1, a10, a14, m_randConfSource);
  }

  m_sysConfSource = new SubMoleculeSource (this);
  { // Limit scope
    ADD_ATOM(a01, 35,  2.4668400485,  1.3831541868,  2.5018663900, m_sysConfSource); //Br
    ADD_ATOM(a02, 35, -0.9637117405,  1.6874348278,  2.1938329282, m_sysConfSource); //Br
    ADD_ATOM(a03, 17,  2.2352309504, -1.6034097347,  2.0735200693, m_sysConfSource); //Cl
    ADD_ATOM(a04, 17, -1.1897838133, -1.2935345170,  2.1593501079, m_sysConfSource); //Cl
    ADD_ATOM(a05,  9, -1.3077835616,  0.3417664000, -0.3533936085, m_sysConfSource); //F
    ADD_ATOM(a06,  6,  0.1952602909, -1.3855476163, -0.6348555098, m_sysConfSource); //C
    ADD_ATOM(a07,  6,  0.0000000000,  0.0000000000,  0.0000000000, m_sysConfSource); //C
    ADD_ATOM(a08,  6,  1.2656498305, -0.1577122662,  2.4350904828, m_sysConfSource); //C
    ADD_ATOM(a09,  6,  0.0000000000,  0.0000000000,  1.5658587598, m_sysConfSource); //C
    ADD_ATOM(a10,  1,  0.1733853079, -1.2957254771, -1.7008547862, m_sysConfSource); //H
    ADD_ATOM(a11,  1,  0.7909300117,  0.6424703339, -0.3264377223, m_sysConfSource); //H
    ADD_ATOM(a12,  1, -0.6251159940, -2.0635927806, -0.3860184702, m_sysConfSource); //H
    ADD_ATOM(a13,  1,  1.1328161512, -1.8690292178, -0.3672323990, m_sysConfSource); //H
    ADD_ATOM(a14,  1,  0.9790110257, -0.2549204749,  3.4888524258, m_sysConfSource); //H

    ADD_BOND(b01, 1, a01, a08, m_sysConfSource);
    ADD_BOND(b02, 1, a14, a08, m_sysConfSource);
    ADD_BOND(b03, 1, a03, a08, m_sysConfSource);
    ADD_BOND(b04, 1, a09, a08, m_sysConfSource);
    ADD_BOND(b05, 1, a09, a02, m_sysConfSource);
    ADD_BOND(b06, 1, a09, a04, m_sysConfSource);
    ADD_BOND(b07, 1, a09, a07, m_sysConfSource);
    ADD_BOND(b08, 1, a05, a07, m_sysConfSource);
    ADD_BOND(b09, 1, a11, a07, m_sysConfSource);
    ADD_BOND(b10, 1, a06, a07, m_sysConfSource);
    ADD_BOND(b11, 1, a06, a13, m_sysConfSource);
    ADD_BOND(b12, 1, a06, a12, m_sysConfSource);
    ADD_BOND(b13, 1, a06, a10, m_sysConfSource);
  }
}

void SubMoleculeSourceTest::cleanupTestCase()
{
}

void SubMoleculeSourceTest::init()
{
}

void SubMoleculeSourceTest::cleanup()
{
}

void SubMoleculeSourceTest::setOB()
{
  OpenBabel::OBMol mol;

  OpenBabel::OBAtom *h1, *h2, *o1;
  OpenBabel::OBBond *oh1, *oh2;

  o1 = mol.NewAtom();
  o1->SetVector(0, 0, 0);
  o1->SetAtomicNum(8);

  h1 = mol.NewAtom();
  h1->SetVector(0.75,0.75,0);
  h1->SetAtomicNum(1);

  h2 = mol.NewAtom();
  h2->SetVector(0.75,-0.75,0);
  h2->SetAtomicNum(1);

  oh1 = mol.NewBond();
  oh1->SetBondOrder(1);
  oh1->SetBegin(o1);
  oh1->SetEnd(h1);

  oh2 = mol.NewBond();
  oh2->SetBondOrder(1);
  oh2->SetBegin(o1);
  oh2->SetEnd(h2);

  SubMoleculeSource source;
  source.set(&mol);

  QVERIFY(source.numAtoms() == 3);
  QVERIFY(source.numBonds() == 2);
  QVERIFY(source.atom(0)->atomicNumber() == 8);
  QVERIFY(source.atom(1)->atomicNumber() == 1);
  QVERIFY(source.atom(2)->atomicNumber() == 1);
  QVERIFY(source.bond(0)->order() == 1);
  QVERIFY(source.bond(1)->order() == 1);
  QVERIFY(fabs(source.bond(0)->length() - 1.06066) < 1e-4);
  QVERIFY(fabs(source.bond(1)->length() - 1.06066) < 1e-4);
}

void SubMoleculeSourceTest::setAvo()
{
  Avogadro::Molecule mol;

  Atom *o1, *h1, *h2;
  Bond *oh1, *oh2;

  o1 = mol.addAtom();
  o1->setPos(Eigen::Vector3d(0,0,0));
  o1->setAtomicNumber(8);

  h1 = mol.addAtom();
  h1->setPos(Eigen::Vector3d(0.75,0.75,0));
  h1->setAtomicNumber(1);

  h2 = mol.addAtom();
  h2->setPos(Eigen::Vector3d(0.75,-0.75,0));
  h2->setAtomicNumber(1);

  oh1 = mol.addBond();
  oh1->setOrder(1);
  oh1->setBegin(o1);
  oh1->setEnd(h1);

  oh2 = mol.addBond();
  oh2->setOrder(1);
  oh2->setBegin(o1);
  oh2->setEnd(h2);

  SubMoleculeSource source;
  source.set(&mol);

  QVERIFY(source.numAtoms() == 3);
  QVERIFY(source.numBonds() == 2);
  QVERIFY(source.atom(0)->atomicNumber() == 8);
  QVERIFY(source.atom(1)->atomicNumber() == 1);
  QVERIFY(source.atom(2)->atomicNumber() == 1);
  QVERIFY(source.bond(0)->order() == 1);
  QVERIFY(source.bond(1)->order() == 1);
  QVERIFY(fabs(source.bond(0)->length() - 1.06066) < 1e-4);
  QVERIFY(fabs(source.bond(1)->length() - 1.06066) < 1e-4);
}

void SubMoleculeSourceTest::findAndSetConformers_rand()
{
  m_randConfSource->setMaxConformers(20); // The default, but just in case...
  m_randConfSource->setNumGeoSteps(500); // Save some time...
  m_randConfSource->findAndSetConformers();

  // Some of the filtering code in findAndSetConformers may kick a few out.
  //QVERIFY(m_randConfSource->numConformers() == 20);

  // Verify that all are sorted:
  for (unsigned int i = 0; i < m_randConfSource->numConformers() - 1; ++i) {
    QVERIFY(m_randConfSource->energy(i) <= m_randConfSource->energy(i+1));
  }
}

void SubMoleculeSourceTest::findAndSetConformers_sys()
{
  m_sysConfSource->setMaxConformers(20); // The default, but just in case...
  m_sysConfSource->setNumGeoSteps(500); // Save some time...
  m_sysConfSource->findAndSetConformers();

  QVERIFY(m_sysConfSource->numConformers() == 8);
  QVERIFY(fabs(m_sysConfSource->energy(0) - 34.9393) < 1.0);
  QVERIFY(fabs(m_sysConfSource->energy(7) - 40.2857) < 1.0);

  // Verify that all are sorted:
  for (unsigned int i = 0; i < m_sysConfSource->numConformers() - 1; ++i) {
    QVERIFY(m_sysConfSource->energy(i) <= m_sysConfSource->energy(i+1));
  }
}

void SubMoleculeSourceTest::uffFallback()
{
  SubMoleculeSource source;
  ADD_ATOM(N1,  7,  1.2192210000,  1.2192210000, -1.2192210000, (&source));
  ADD_ATOM(N2,  7, -1.2192210000, -1.2192210000, -1.2192210000, (&source));
  ADD_ATOM(N3,  7,  1.2192210000, -1.2192210000,  1.2192210000, (&source));
  ADD_ATOM(N4,  7, -1.2192210000,  1.2192210000,  1.2192210000, (&source));
  ADD_ATOM(Li,  3,  0.0000000000,  0.0000000000,  0.0000000000, (&source));
  ADD_ATOM(H1,  1,  1.8386850000,  0.6815750000, -1.8386850000, (&source));
  ADD_ATOM(H2,  1,  1.8386850000,  1.8386850000, -0.6815750000, (&source));
  ADD_ATOM(H3,  1,  0.6815750000,  1.8386850000, -1.8386850000, (&source));
  ADD_ATOM(H4,  1, -0.6815750000, -1.8386850000, -1.8386850000, (&source));
  ADD_ATOM(H5,  1, -1.8386850000, -0.6815750000, -1.8386850000, (&source));
  ADD_ATOM(H6,  1, -1.8386850000, -1.8386850000, -0.6815750000, (&source));
  ADD_ATOM(H7,  1,  1.8386850000, -1.8386850000,  0.6815750000, (&source));
  ADD_ATOM(H8,  1,  0.6815750000, -1.8386850000,  1.8386850000, (&source));
  ADD_ATOM(H9,  1,  1.8386850000, -0.6815750000,  1.8386850000, (&source));
  ADD_ATOM(H10, 1, -1.8386850000,  1.8386850000,  0.6815750000, (&source));
  ADD_ATOM(H11, 1, -0.6815750000,  1.8386850000,  1.8386850000, (&source));
  ADD_ATOM(H12, 1, -1.8386850000,  0.6815750000,  1.8386850000, (&source));

  ADD_BOND(B1,  1, Li, N1,  (&source));
  ADD_BOND(B2,  1, Li, N2,  (&source));
  ADD_BOND(B3,  1, Li, N3,  (&source));
  ADD_BOND(B4,  1, Li, N4,  (&source));
  ADD_BOND(B5,  1, N1, H1,  (&source));
  ADD_BOND(B6,  1, N1, H2,  (&source));
  ADD_BOND(B7,  1, N1, H3,  (&source));
  ADD_BOND(B8,  1, N2, H4,  (&source));
  ADD_BOND(B9,  1, N2, H5,  (&source));
  ADD_BOND(B10, 1, N2, H6,  (&source));
  ADD_BOND(B11, 1, N3, H7,  (&source));
  ADD_BOND(B12, 1, N3, H8,  (&source));
  ADD_BOND(B13, 1, N3, H9,  (&source));
  ADD_BOND(B14, 1, N4, H10, (&source));
  ADD_BOND(B15, 1, N4, H11, (&source));
  ADD_BOND(B16, 1, N4, H12, (&source));

  source.findAndSetConformers();

  QVERIFY(QString(source.m_ff->GetID()) == QString("UFF"));
}

void SubMoleculeSourceTest::getRandomSubMolecule()
{
  SubMoleculeSource source;

  Atom *o1, *h1, *h2;
  Bond *oh1, *oh2;

  o1 = source.addAtom();
  o1->setPos(Eigen::Vector3d(0,0,0));
  o1->setAtomicNumber(8);

  h1 = source.addAtom();
  h1->setPos(Eigen::Vector3d(0.75,0.75,0));
  h1->setAtomicNumber(1);

  h2 = source.addAtom();
  h2->setPos(Eigen::Vector3d(0.75,-0.75,0));
  h2->setAtomicNumber(1);

  oh1 = source.addBond();
  oh1->setOrder(1);
  oh1->setBegin(o1);
  oh1->setEnd(h1);

  oh2 = source.addBond();
  oh2->setOrder(1);
  oh2->setBegin(o1);
  oh2->setEnd(h2);

  SubMolecule *sub = source.getRandomSubMolecule();

  QVERIFY(sub->numAtoms() == 3);
  QVERIFY(sub->numBonds() == 2);
  QVERIFY(sub->atom(0)->atomicNumber() == 8);
  QVERIFY(sub->atom(1)->atomicNumber() == 1);
  QVERIFY(sub->atom(2)->atomicNumber() == 1);
  QVERIFY(sub->bond(0)->order() == 1);
  QVERIFY(sub->bond(1)->order() == 1);
  QVERIFY(fabs(sub->bond(0)->length() - 1.06066) < 1e-4);
  QVERIFY(fabs(sub->bond(1)->length() - 1.06066) < 1e-4);
}

QTEST_MAIN(SubMoleculeSourceTest)

#include "moc_submoleculesourcetest.cxx"

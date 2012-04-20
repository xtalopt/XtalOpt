/**********************************************************************
  MXtalOptGeneticTest

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

#include <xtalopt/mxtaloptgenetic.h>

#include <xtalopt/structures/molecularxtal.h>
#include <xtalopt/structures/submolecule.h>
#include <xtalopt/structures/submoleculesource.h>

#include <globalsearch/obeigenconv.h>

#include <avogadro/atom.h>
#include <avogadro/bond.h>
#include <avogadro/molecule.h>
#include <avogadro/moleculefile.h>

#include <QtTest/QtTest>

using namespace Avogadro;
using namespace GlobalSearch;
using namespace XtalOpt;

class MXtalOptGeneticTest : public QObject
{
  Q_OBJECT

  private:
  SubMoleculeSource *m_source1;
  SubMoleculeSource *m_source2;

  MolecularXtal *m_parent1;
  MolecularXtal *m_parent2;

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
  void crossover();
  void crossoverCutAxisTest();
  void reconf();
  void swirl();
  void strain();
};

inline bool compareVectors(const Eigen::Vector3d &v1,
                           const Eigen::Vector3d &v2)
{
  static const double tolerance = 1e-5;
  if (fabs(v1.x() - v2.x()) > tolerance) return false;
  if (fabs(v1.y() - v2.y()) > tolerance) return false;
  if (fabs(v1.z() - v2.z()) > tolerance) return false;
  return true;
}

inline Atom *addAtomToMol(int atomicNum, const Eigen::Vector3d &pos,
                          Molecule *mol)
{
  Atom *atom = mol->addAtom();
  atom->setAtomicNumber(atomicNum);
  atom->setPos(pos);
  return atom;
}

inline Atom *addAtomToMol(int atomicNum, double x, double y, double z,
                          Molecule *mol)
{
  return addAtomToMol(atomicNum, Eigen::Vector3d(x,y,z), mol);
}

inline Bond *addBondToMol(Atom *beg, Atom *end, int order, Molecule *mol)
{
  Bond *bond = mol->addBond();
  bond->setBegin(beg);
  bond->setEnd(end);
  bond->setOrder(order);
  return bond;
}

void wrapFractionalCoordinate(Eigen::Vector3d *vec)
{
  if (((*vec)[0] = fmod((*vec)[0], 1.0)) < 0) ++(*vec)[0];
  if (((*vec)[1] = fmod((*vec)[1], 1.0)) < 0) ++(*vec)[1];
  if (((*vec)[2] = fmod((*vec)[2], 1.0)) < 0) ++(*vec)[2];
}

inline void dumpMXtal(const MolecularXtal *mxtal, const QString &name = "unnamed")
{
  QString out = QString(
        "\n"
        "MXtal %1:\n"
        "  Num submols: %2\n"
        )
      .arg(name).arg(mxtal->numSubMolecules());

  for (int i = 0; i < mxtal->numSubMolecules(); ++i) {
    const SubMolecule *submol = mxtal->subMolecule(i);
    Eigen::Vector3d center (submol->center());
    Eigen::Vector3d fracCenter (mxtal->cartToFrac(center));
    Eigen::Vector3d normal (submol->normal());
    Eigen::Vector3d farvec (submol->farthestAtomVector());
    Eigen::Vector3d farpos (*submol->farthestAtomPos());
    out += QString ("  SubMol %1:\n").arg(i);
    out += QString ("    Source: %1\n").arg(submol->sourceId());
    out += QString ("    Center     @ %1 %2 %3:\n")
        .arg(center.x(), 9, 'f', 5)
        .arg(center.y(), 9, 'f', 5)
        .arg(center.z(), 9, 'f', 5);
    out += QString ("    FracCenter @ %1 %2 %3:\n")
        .arg(fracCenter.x(), 9, 'f', 5)
        .arg(fracCenter.y(), 9, 'f', 5)
        .arg(fracCenter.z(), 9, 'f', 5);
    out += QString ("    Normal     @ %1 %2 %3:\n")
        .arg(normal.x(), 9, 'f', 5)
        .arg(normal.y(), 9, 'f', 5)
        .arg(normal.z(), 9, 'f', 5);
    out += QString ("    FarVec     @ %1 %2 %3:\n")
        .arg(farvec.x(), 9, 'f', 5)
        .arg(farvec.y(), 9, 'f', 5)
        .arg(farvec.z(), 9, 'f', 5);
    out += QString ("    FarPos     @ %1 %2 %3:\n")
        .arg(farpos.x(), 9, 'f', 5)
        .arg(farpos.y(), 9, 'f', 5)
        .arg(farpos.z(), 9, 'f', 5);
  }

  QString err;
  bool ok = MoleculeFile::writeMolecule(mxtal, QString("%1.cml").arg(name),
                                        "cml", "p", &err);

  if (ok)
    out += QString("  File written: %1.cml\n").arg(name);
  else
    out += "  Could not write file\n";

  std::cout << out.toStdString().c_str();

}

void MXtalOptGeneticTest::initTestCase()
{
  m_source1 = new SubMoleculeSource ();

  Atom *a0 = addAtomToMol(8,-0.4298560000,-0.7288350000,-2.1966190000, m_source1);
  Atom *a1 = addAtomToMol(6, 0.0000000000, 0.0000000000, 0.0000000000, m_source1);
  Atom *a2 = addAtomToMol(6, 0.0000000000, 0.0000000000, 1.3397690000, m_source1);
  Atom *a3 = addAtomToMol(6,-0.6284610000,-1.0649930000,-0.8321560000, m_source1);
  Atom *a4 = addAtomToMol(1,-0.8461370000,-1.4341930000,-2.7193030000, m_source1);
  Atom *a5 = addAtomToMol(1,-0.1598640000,-2.0358150000,-0.6435940000, m_source1);
  Atom *a6 = addAtomToMol(1,-1.7048860000,-1.1238990000,-0.6438150000, m_source1);
  Atom *a7 = addAtomToMol(1, 0.4837560000, 0.8197700000,-0.5275260000, m_source1);
  Atom *a8 = addAtomToMol(1,-0.4694030000,-0.7954380000, 1.9106440000, m_source1);
  Atom *a9 = addAtomToMol(1, 0.4745970000, 0.8042530000, 1.8926780000, m_source1);

  Bond *b0 = addBondToMol(a4, a0, 1, m_source1);
  Bond *b1 = addBondToMol(a0, a3, 1, m_source1);
  Bond *b2 = addBondToMol(a3, a5, 1, m_source1);
  Bond *b3 = addBondToMol(a3, a6, 1, m_source1);
  Bond *b4 = addBondToMol(a3, a1, 1, m_source1);
  Bond *b5 = addBondToMol(a1, a7, 1, m_source1);
  Bond *b6 = addBondToMol(a1, a2, 2, m_source1);
  Bond *b7 = addBondToMol(a2, a8, 1, m_source1);
  Bond *b8 = addBondToMol(a2, a9, 1, m_source1);

  m_source1->translate(-m_source1->center());

  m_source2 = new SubMoleculeSource ();

  Atom *a00 = addAtomToMol(1, 1.2194260000,-0.1651620000, 2.1599560000, m_source2);
  Atom *a01 = addAtomToMol(6, 0.6824840000,-0.0923930000, 1.2087510000, m_source2);
  Atom *a02 = addAtomToMol(6,-0.7074390000,-0.0351890000, 1.1973160000, m_source2);
  Atom *a03 = addAtomToMol(1,-1.2643640000,-0.0629720000, 2.1393530000, m_source2);
  Atom *a04 = addAtomToMol(6,-1.3898230000, 0.0572170000,-0.0114320000, m_source2);
  Atom *a05 = addAtomToMol(1,-2.4835640000, 0.1021430000,-0.0204440000, m_source2);
  Atom *a06 = addAtomToMol(6,-0.6824190000, 0.0924780000,-1.2087550000, m_source2);
  Atom *a07 = addAtomToMol(1,-1.2194020000, 0.1651950000,-2.1599000000, m_source2);
  Atom *a08 = addAtomToMol(6, 0.7074790000, 0.0352270000,-1.1972980000, m_source2);
  Atom *a09 = addAtomToMol(1, 1.2641300000, 0.0628480000,-2.1394580000, m_source2);
  Atom *a10 = addAtomToMol(6, 1.3898940000,-0.0571740000, 0.0114260000, m_source2);
  Atom *a11 = addAtomToMol(1, 2.4835970000,-0.1022170000, 0.0204890000, m_source2);

  Bond *b00 = addBondToMol(a00, a01, 1, m_source2);
  Bond *b01 = addBondToMol(a01, a02, 2, m_source2);
  Bond *b02 = addBondToMol(a01, a10, 1, m_source2);
  Bond *b03 = addBondToMol(a02, a03, 1, m_source2);
  Bond *b04 = addBondToMol(a02, a04, 1, m_source2);
  Bond *b05 = addBondToMol(a04, a05, 1, m_source2);
  Bond *b06 = addBondToMol(a04, a06, 2, m_source2);
  Bond *b07 = addBondToMol(a06, a07, 1, m_source2);
  Bond *b08 = addBondToMol(a06, a08, 1, m_source2);
  Bond *b09 = addBondToMol(a08, a09, 1, m_source2);
  Bond *b10 = addBondToMol(a08, a10, 2, m_source2);
  Bond *b11 = addBondToMol(a10, a11, 1, m_source2);

  m_source2->translate(-m_source2->center());

  m_source1->setSourceId(0);
  m_source2->setSourceId(1);

  // parent1:
  m_parent1 = new MolecularXtal (6, 6, 6, 90, 90, 90);
  SubMolecule *sub = m_source1->getSubMolecule(0);
  m_parent1->addSubMolecule(sub);
  sub->translateFrac(0, 0, 0);

  sub = m_source1->getSubMolecule(0);
  m_parent1->addSubMolecule(sub);
  sub->translateFrac(0, 0, 0.5);

  sub = m_source2->getSubMolecule(0);
  m_parent1->addSubMolecule(sub);
  sub->translateFrac(0, 0.5, 0);

  sub = m_source2->getSubMolecule(0);
  m_parent1->addSubMolecule(sub);
  sub->translateFrac(0.5, 0, 0);

  // parent2:
  m_parent2 = new MolecularXtal (6, 6, 6, 90, 90, 90);
  sub = m_source1->getSubMolecule(0);
  m_parent2->addSubMolecule(sub);
  sub->translateFrac(0.25, 0.25, 0.25);

  sub = m_source1->getSubMolecule(0);
  m_parent2->addSubMolecule(sub);
  sub->translateFrac(0.25, 0.25, 0.75);

  sub = m_source2->getSubMolecule(0);
  m_parent2->addSubMolecule(sub);
  sub->translateFrac(0.25, 0.75, 0.25);

  sub = m_source2->getSubMolecule(0);
  m_parent2->addSubMolecule(sub);
  sub->translateFrac(0.75, 0.25, 0.25);

//  dumpMXtal(m_parent1, "parent1");
//  dumpMXtal(m_parent2, "parent2");
}

void MXtalOptGeneticTest::cleanupTestCase()
{
  delete m_source1;
  delete m_source2;
  delete m_parent1;
  delete m_parent2;
}

void MXtalOptGeneticTest::init()
{
}

void MXtalOptGeneticTest::cleanup()
{
}

void MXtalOptGeneticTest::crossover()
{
  QVector<unsigned int> quantities;
  quantities << 2 << 2;

  MolecularXtal *mxtal = XtalOpt::MXtalOptGenetic::crossover(
        m_parent1, m_parent2,
        MXtalOptGenetic::C_AXIS, MXtalOptGenetic::C_AXIS,
        Eigen::Vector3d::Zero(), Eigen::Vector3d::Zero(),
        0.5, quantities);

  //dumpMXtal(mxtal, "crossover");

  QList<SubMolecule*> mxtalSubmols = mxtal->subMolecules();

  // Verify that the composition is correct
  QVector<unsigned int> mxtalQuantities;
  mxtalQuantities.insert(0, quantities.size(), 0);

  foreach (const SubMolecule *sub, mxtalSubmols) {
    unsigned long sourceId = sub->sourceId();
    QVERIFY(sourceId < mxtalQuantities.size());
    ++mxtalQuantities[sourceId];
  }

  QVERIFY(mxtalQuantities == quantities);

  delete mxtal;
}

inline bool atomIsOnAxis(const Atom *atom, const MolecularXtal *parent,
                         MXtalOptGenetic::Axis axis)
{
  static const Eigen::Vector3d fixNoise = Eigen::Vector3d(1.0,1.0,1.0) * 1e-6;
  Eigen::Vector3d fracCoord = parent->cartToFrac(*atom->pos() + fixNoise);
  wrapFractionalCoordinate(&fracCoord);
  switch (axis)
  {
  case XtalOpt::MXtalOptGenetic::A_AXIS:
    if (fracCoord[1] > 0.05 || fracCoord[2] > 0.05)
      return false;
    break;
  case XtalOpt::MXtalOptGenetic::B_AXIS:
    if (fracCoord[0] > 0.05 || fracCoord[2] > 0.05)
      return false;
    break;
  case XtalOpt::MXtalOptGenetic::C_AXIS:
    if (fracCoord[0] > 0.05 || fracCoord[1] > 0.05)
      return false;
    break;
  }
  return true;
}

void MXtalOptGeneticTest::crossoverCutAxisTest()
{
  // Three sources: single hydrogen, helium, lithium
  SubMoleculeSource source1, source2, source3;
  addAtomToMol(1, 0.0, 0.0, 0.0, &source1);
  source1.setSourceId(0);
  addAtomToMol(2, 0.0, 0.0, 0.0, &source2);
  source2.setSourceId(1);
  addAtomToMol(3, 0.0, 0.0, 0.0, &source3);
  source3.setSourceId(2);

  // Two parents with atoms along the a,b,c axes
  MolecularXtal parent1 (1, 2, 3, 70, 80, 90);
  MolecularXtal parent2 (4, 5, 6, 60, 85, 72);
  QVector<unsigned int> quantities;
  quantities << 10 << 10 << 10;
  for (double d = 0.0; d < 0.99; d += 0.1) {
    SubMolecule *sub11 = source1.getSubMolecule(0);
    SubMolecule *sub21 = source1.getSubMolecule(0);
    SubMolecule *sub12 = source2.getSubMolecule(0);
    SubMolecule *sub22 = source2.getSubMolecule(0);
    SubMolecule *sub13 = source3.getSubMolecule(0);
    SubMolecule *sub23 = source3.getSubMolecule(0);
    parent1.addSubMolecule(sub11);
    parent2.addSubMolecule(sub21);
    parent1.addSubMolecule(sub12);
    parent2.addSubMolecule(sub22);
    parent1.addSubMolecule(sub13);
    parent2.addSubMolecule(sub23);
    sub11->translateFrac(d, 0.0, 0.0);
    sub21->translateFrac(d, 0.0, 0.0);
    sub12->translateFrac(0.0, d, 0.0);
    sub22->translateFrac(0.0, d, 0.0);
    sub13->translateFrac(0.0, 0.0, d);
    sub23->translateFrac(0.0, 0.0, d);
  }

//  dumpMXtal(&parent1, "parent1");
//  dumpMXtal(&parent2, "parent2");

  MolecularXtal *mxtal_aa = MXtalOptGenetic::crossover(
        &parent1, &parent2,
        MXtalOptGenetic::A_AXIS, MXtalOptGenetic::A_AXIS,
        Eigen::Vector3d::Zero(), Eigen::Vector3d::Zero(),
        0.5, quantities);
  MolecularXtal *mxtal_ab = MXtalOptGenetic::crossover(
        &parent1, &parent2,
        MXtalOptGenetic::A_AXIS, MXtalOptGenetic::B_AXIS,
        Eigen::Vector3d::Zero(), Eigen::Vector3d::Zero(),
        0.5, quantities);
  MolecularXtal *mxtal_ac = MXtalOptGenetic::crossover(
        &parent1, &parent2,
        MXtalOptGenetic::A_AXIS, MXtalOptGenetic::C_AXIS,
        Eigen::Vector3d::Zero(), Eigen::Vector3d::Zero(),
        0.5, quantities);
  MolecularXtal *mxtal_ba = MXtalOptGenetic::crossover(
        &parent1, &parent2,
        MXtalOptGenetic::B_AXIS, MXtalOptGenetic::A_AXIS,
        Eigen::Vector3d::Zero(), Eigen::Vector3d::Zero(),
        0.5, quantities);
  MolecularXtal *mxtal_bb = MXtalOptGenetic::crossover(
        &parent1, &parent2,
        MXtalOptGenetic::B_AXIS, MXtalOptGenetic::B_AXIS,
        Eigen::Vector3d::Zero(), Eigen::Vector3d::Zero(),
        0.5, quantities);
  MolecularXtal *mxtal_bc = MXtalOptGenetic::crossover(
        &parent1, &parent2,
        MXtalOptGenetic::B_AXIS, MXtalOptGenetic::C_AXIS,
        Eigen::Vector3d::Zero(), Eigen::Vector3d::Zero(),
        0.5, quantities);
  MolecularXtal *mxtal_ca = MXtalOptGenetic::crossover(
        &parent1, &parent2,
        MXtalOptGenetic::C_AXIS, MXtalOptGenetic::A_AXIS,
        Eigen::Vector3d::Zero(), Eigen::Vector3d::Zero(),
        0.5, quantities);
  MolecularXtal *mxtal_cb = MXtalOptGenetic::crossover(
        &parent1, &parent2,
        MXtalOptGenetic::C_AXIS, MXtalOptGenetic::B_AXIS,
        Eigen::Vector3d::Zero(), Eigen::Vector3d::Zero(),
        0.5, quantities);
  MolecularXtal *mxtal_cc = MXtalOptGenetic::crossover(
        &parent1, &parent2,
        MXtalOptGenetic::C_AXIS, MXtalOptGenetic::C_AXIS,
        Eigen::Vector3d::Zero(), Eigen::Vector3d::Zero(),
        0.5, quantities);

//  dumpMXtal(mxtal_aa, "mxtal_aa");
  foreach (const Atom *atom, mxtal_aa->atoms()) {
    switch(atom->atomicNumber())
    {
    case 1:
      QVERIFY(atomIsOnAxis(atom, mxtal_aa, MXtalOptGenetic::A_AXIS));
      break;
    case 2:
      QVERIFY(atomIsOnAxis(atom, mxtal_aa, MXtalOptGenetic::B_AXIS));
     break;
    case 3:
      QVERIFY(atomIsOnAxis(atom, mxtal_aa, MXtalOptGenetic::C_AXIS));
      break;
    }
  }

  //  dumpMXtal(mxtal_ab, "mxtal_ab");
  foreach (const Atom *atom, mxtal_ab->atoms()) {
    switch(atom->atomicNumber())
    {
    case 1:
      QVERIFY(atomIsOnAxis(atom, mxtal_ab, MXtalOptGenetic::A_AXIS) ||
              atomIsOnAxis(atom, mxtal_ab, MXtalOptGenetic::B_AXIS) );
      break;
    case 2:
      QVERIFY(atomIsOnAxis(atom, mxtal_ab, MXtalOptGenetic::A_AXIS) ||
              atomIsOnAxis(atom, mxtal_ab, MXtalOptGenetic::B_AXIS) );
      break;
    case 3:
      QVERIFY(atomIsOnAxis(atom, mxtal_ab, MXtalOptGenetic::C_AXIS));
      break;
    }
  }

  //    dumpMXtal(mxtal_ac, "mxtal_ac");
  foreach (const Atom *atom, mxtal_ac->atoms()) {
    switch(atom->atomicNumber())
    {
    case 1:
      QVERIFY(atomIsOnAxis(atom, mxtal_ac, MXtalOptGenetic::A_AXIS) ||
              atomIsOnAxis(atom, mxtal_ac, MXtalOptGenetic::C_AXIS) );
      break;
    case 2:
      QVERIFY(atomIsOnAxis(atom, mxtal_ac, MXtalOptGenetic::A_AXIS) ||
              atomIsOnAxis(atom, mxtal_ac, MXtalOptGenetic::B_AXIS) );
      break;
    case 3:
      QVERIFY(atomIsOnAxis(atom, mxtal_ac, MXtalOptGenetic::B_AXIS) ||
              atomIsOnAxis(atom, mxtal_ac, MXtalOptGenetic::C_AXIS) );
      break;
    }
  }

  //    dumpMXtal(mxtal_ba, "mxtal_ba");
  foreach (const Atom *atom, mxtal_ba->atoms()) {
    switch(atom->atomicNumber())
    {
    case 1:
      QVERIFY(atomIsOnAxis(atom, mxtal_ba, MXtalOptGenetic::A_AXIS) ||
              atomIsOnAxis(atom, mxtal_ba, MXtalOptGenetic::B_AXIS) );
      break;
    case 2:
      QVERIFY(atomIsOnAxis(atom, mxtal_ba, MXtalOptGenetic::A_AXIS) ||
              atomIsOnAxis(atom, mxtal_ba, MXtalOptGenetic::B_AXIS) );
      break;
    case 3:
      QVERIFY(atomIsOnAxis(atom, mxtal_ba, MXtalOptGenetic::C_AXIS));
      break;
    }
  }

  //    dumpMXtal(mxtal_bb, "mxtal_bb");
  foreach (const Atom *atom, mxtal_bb->atoms()) {
    switch(atom->atomicNumber())
    {
    case 1:
      QVERIFY(atomIsOnAxis(atom, mxtal_bb, MXtalOptGenetic::B_AXIS));
      break;
    case 2:
      QVERIFY(atomIsOnAxis(atom, mxtal_bb, MXtalOptGenetic::A_AXIS));
      break;
    case 3:
      QVERIFY(atomIsOnAxis(atom, mxtal_bb, MXtalOptGenetic::C_AXIS));
      break;
    }
  }

  //    dumpMXtal(mxtal_bc, "mxtal_bc");
  foreach (const Atom *atom, mxtal_bc->atoms()) {
    switch(atom->atomicNumber())
    {
    case 1:
      QVERIFY(atomIsOnAxis(atom, mxtal_bc, MXtalOptGenetic::B_AXIS) ||
              atomIsOnAxis(atom, mxtal_bc, MXtalOptGenetic::C_AXIS) );
      break;
    case 2:
      QVERIFY(atomIsOnAxis(atom, mxtal_bc, MXtalOptGenetic::A_AXIS));
      break;
    case 3:
      QVERIFY(atomIsOnAxis(atom, mxtal_bc, MXtalOptGenetic::B_AXIS) ||
              atomIsOnAxis(atom, mxtal_bc, MXtalOptGenetic::C_AXIS) );
      break;
    }
  }

  //    dumpMXtal(mxtal_ca, "mxtal_ca");
  foreach (const Atom *atom, mxtal_ca->atoms()) {
    switch(atom->atomicNumber())
    {
    case 1:
      QVERIFY(atomIsOnAxis(atom, mxtal_ca, MXtalOptGenetic::A_AXIS) ||
              atomIsOnAxis(atom, mxtal_ca, MXtalOptGenetic::C_AXIS) );
      break;
    case 2:
      QVERIFY(atomIsOnAxis(atom, mxtal_ca, MXtalOptGenetic::A_AXIS) ||
              atomIsOnAxis(atom, mxtal_ca, MXtalOptGenetic::B_AXIS) );
      break;
    case 3:
      QVERIFY(atomIsOnAxis(atom, mxtal_ca, MXtalOptGenetic::B_AXIS) ||
              atomIsOnAxis(atom, mxtal_ca, MXtalOptGenetic::C_AXIS) );
      break;
    }
  }

  //    dumpMXtal(mxtal_cb, "mxtal_cb");
  foreach (const Atom *atom, mxtal_cb->atoms()) {
    switch(atom->atomicNumber())
    {
    case 1:
      QVERIFY(atomIsOnAxis(atom, mxtal_cb, MXtalOptGenetic::B_AXIS) ||
              atomIsOnAxis(atom, mxtal_cb, MXtalOptGenetic::C_AXIS) );
      break;
    case 2:
      QVERIFY(atomIsOnAxis(atom, mxtal_cb, MXtalOptGenetic::A_AXIS));
      break;
    case 3:
      QVERIFY(atomIsOnAxis(atom, mxtal_cb, MXtalOptGenetic::B_AXIS) ||
              atomIsOnAxis(atom, mxtal_cb, MXtalOptGenetic::C_AXIS) );
      break;
    }
  }

  //    dumpMXtal(mxtal_cc, "mxtal_cc");
  foreach (const Atom *atom, mxtal_cc->atoms()) {
    switch(atom->atomicNumber())
    {
    case 1:
      QVERIFY(atomIsOnAxis(atom, mxtal_cc, MXtalOptGenetic::C_AXIS));
      break;
    case 2:
      QVERIFY(atomIsOnAxis(atom, mxtal_cc, MXtalOptGenetic::A_AXIS));
      break;
    case 3:
      QVERIFY(atomIsOnAxis(atom, mxtal_cc, MXtalOptGenetic::B_AXIS));
      break;
    }
  }

  // Clean up
  delete mxtal_aa;
  delete mxtal_ab;
  delete mxtal_ac;
  delete mxtal_ba;
  delete mxtal_bb;
  delete mxtal_bc;
  delete mxtal_ca;
  delete mxtal_cb;
  delete mxtal_cc;
}

void MXtalOptGeneticTest::reconf()
{
  QVector<SubMoleculeSource*> sources;
  sources << m_source1 << m_source2;
  QVector<int> confIndices;
  confIndices << 0 << 0 << 0 << 0;
  MolecularXtal *mxtal = new MolecularXtal ();
  *mxtal = *m_parent1;
  XtalOpt::MXtalOptGenetic::reconf(mxtal, sources, confIndices);

  //dumpMXtal(mxtal, "reconf");

  // Verify that the centers, normals, and farvecs all match. Also check that
  // the submols are in the same order
  QVERIFY(mxtal->numSubMolecules() == m_parent1->numSubMolecules());

  const double tolerance = 1e-5;
  for (int i = 0; i < mxtal->numSubMolecules(); ++i) {
    SubMolecule *mxtalSub = mxtal->subMolecule(i);
    SubMolecule *parentSub = m_parent1->subMolecule(i);

    QVERIFY(mxtalSub->sourceId() == parentSub->sourceId());
    QVERIFY(compareVectors(mxtalSub->center(), parentSub->center()));
    QVERIFY(compareVectors(mxtalSub->normal(), parentSub->normal()));
    QVERIFY(compareVectors(mxtalSub->farthestAtomVector(),
              parentSub->farthestAtomVector()));
  }

  delete mxtal;
}

void MXtalOptGeneticTest::swirl()
{
  MolecularXtal *mxtal = new MolecularXtal ();
  *mxtal = *m_parent1;

  QVector<Eigen::Vector3d> axes;
  axes << mxtal->subMolecule(0)->normal()
       << mxtal->subMolecule(1)->normal()
       << Eigen::Vector3d(1, 0, 0)
       << Eigen::Vector3d(0, 0, 1);
  QVector<double> angles;
  angles << 0.0 << M_PI/2.0 << 0.0 << 3.0 * M_PI / 2.0;

  XtalOpt::MXtalOptGenetic::swirl(mxtal, axes, angles);

  //dumpMXtal(mxtal, "swirl");

  // Build transforms
  QVector<Eigen::Transform3d> xforms;
  for (int i = 0; i < axes.size(); ++i) {
    xforms.append(Eigen::Transform3d(Eigen::AngleAxisd(angles[i], axes[i])));
  }

  // Check orientation vectors
  const double tolerance = 1e-5;
  for (int i = 0; i < axes.size(); ++i) {
    SubMolecule *mxtalSub = mxtal->subMolecule(i);
    SubMolecule *parentSub = m_parent1->subMolecule(i);

    const Eigen::Vector3d center (mxtalSub->center());
    const Eigen::Vector3d normal (mxtalSub->normal());
    const Eigen::Vector3d farvec (mxtalSub->farthestAtomVector());

    const Eigen::Vector3d oldcenter (parentSub->center());
    const Eigen::Vector3d oldnormal (parentSub->normal());
    const Eigen::Vector3d oldfarvec (parentSub->farthestAtomVector());

    QVERIFY(compareVectors(center, oldcenter));
    QVERIFY(compareVectors(normal, xforms[i] * oldnormal));
    QVERIFY(compareVectors(farvec, xforms[i] * oldfarvec));
  }

  delete mxtal;
}

void MXtalOptGeneticTest::strain()
{
  MolecularXtal *mxtal = new MolecularXtal ();
  *mxtal = *m_parent1;

  Eigen::Matrix3d voight;
  voight << 1.72, 0.35, -0.25, /**/ 0.35, 0.96, 0.14, /**/ -0.25, 0.14, 1.23;

  XtalOpt::MXtalOptGenetic::strain(mxtal, voight);

  // Verify cell matrices
  Eigen::Matrix3d pmat (OB2Eigen(m_parent1->OBUnitCell()->GetCellMatrix()));
  Eigen::Matrix3d omat (OB2Eigen(mxtal->OBUnitCell()->GetCellMatrix()));

  const double tolerance = 1e-5;
  QVERIFY(omat.isApprox(voight * pmat, tolerance));

  // Verify that fractional submolecule centers are preserved
  for (int i = 0; i < mxtal->numSubMolecules(); ++i) {
    SubMolecule *mxtalSub = mxtal->subMolecule(i);
    SubMolecule *parentSub = m_parent1->subMolecule(i);

    const Eigen::Vector3d mxtalCenter (mxtal->cartToFrac(mxtalSub->center()));
    const Eigen::Vector3d parentCenter (
          m_parent1->cartToFrac(parentSub->center()));

    QVERIFY(compareVectors(mxtalCenter, parentCenter));
  }

  delete mxtal;

}


QTEST_MAIN(MXtalOptGeneticTest)

#include "moc_mxtaloptgenetictest.cxx"

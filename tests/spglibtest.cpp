/**********************************************************************
  SPGLibTest - Unit test for SPGLib

  Copyright (C) 2010 David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <atoms/formats/vaspformat.h>

#include <common/fileutils.h>
#include <xtalopt/structures/xtal.h>

#include <fstream>

#include <QString>
#include <QtTest>

using namespace XtalOpt;

class SPGLibTest : public QObject
{
  Q_OBJECT

private:
  Xtal* m_xtal;

private slots:
  /**
   * Called before each test function is executed.
   */
  void init();

  /**
   * Called after every test function.
   */
  void cleanup();

  // Tests
  void emptyStructure();
  void atomsWithoutCell();
  //  Simple test to make sure that nothing is seriously broken
  void idealBCC();
  void idealFCC();
  void diamondPrimitiveFile();
  void rutileFile();
  void toleranceRestoresSlightlyDistortedBCC();
  void strictToleranceRejectsSlightlyDistortedBCC();
};

void SPGLibTest::init()
{
  m_xtal = new Xtal;
}

void SPGLibTest::cleanup()
{
  delete m_xtal;
  m_xtal = nullptr;
}

void SPGLibTest::emptyStructure()
{
  m_xtal->setCellInfo(3.0, 3.0, 3.0, 90.0, 90.0, 90.0);

  m_xtal->findSpaceGroup();
  QCOMPARE(m_xtal->getSpaceGroupNumber(), 0U);
  QCOMPARE(m_xtal->getSpaceGroupSymbol(), QString("Unknown"));
}

void SPGLibTest::atomsWithoutCell()
{
  Atoms::Atom& atom = m_xtal->addAtom();
  atom.setPos(Common::Vector3(0.0, 0.0, 0.0));
  atom.setAtomicNumber(1);

  m_xtal->findSpaceGroup();
  QCOMPARE(m_xtal->getSpaceGroupNumber(), 0U);
  QCOMPARE(m_xtal->getSpaceGroupSymbol(), QString("Unknown"));
}

void SPGLibTest::idealBCC()
{
  m_xtal->setCellInfo(3.0, 3.0, 3.0, 90.0, 90.0, 90.0);

  Atoms::Atom& atom1 = m_xtal->addAtom();
  atom1.setPos(Common::Vector3(0.0, 0.0, 0.0));
  atom1.setAtomicNumber(1);

  Atoms::Atom& atom2 = m_xtal->addAtom();
  atom2.setPos(Common::Vector3(1.5, 1.5, 1.5));
  atom2.setAtomicNumber(1);

  m_xtal->findSpaceGroup();
  QCOMPARE(m_xtal->getSpaceGroupNumber(), 229U);
  QCOMPARE(m_xtal->getSpaceGroupSymbol(), QString("Im-3m"));
  QCOMPARE(
    m_xtal->getHTMLSpaceGroupSymbol(),
    QString(
      "<HTML>Im<span style=\"text-decoration: overline\">3</span>m</HTML>"));
}

void SPGLibTest::idealFCC()
{
  m_xtal->setCellInfo(4.0, 4.0, 4.0, 90.0, 90.0, 90.0);

  const Common::Vector3 positions[] = {
    Common::Vector3(0.0, 0.0, 0.0),
    Common::Vector3(0.0, 2.0, 2.0),
    Common::Vector3(2.0, 0.0, 2.0),
    Common::Vector3(2.0, 2.0, 0.0)
  };

  for (size_t i = 0; i < sizeof(positions) / sizeof(positions[0]); ++i) {
    Atoms::Atom& atom = m_xtal->addAtom();
    atom.setPos(positions[i]);
    atom.setAtomicNumber(1);
  }

  m_xtal->findSpaceGroup();
  QCOMPARE(m_xtal->getSpaceGroupNumber(), 225U);
  QCOMPARE(m_xtal->getSpaceGroupSymbol(), QString("Fm-3m"));
}

void SPGLibTest::diamondPrimitiveFile()
{
  const QString filename = Common::localPath(Common::localPath(QString(TESTDATADIR), "formats"),
                      "diamond-primitive.POSCAR");
  std::ifstream in(filename.toStdString());
  QVERIFY(in.is_open());
  QVERIFY(Atoms::VaspFormat::read(*m_xtal, in));

  m_xtal->findSpaceGroup();
  QCOMPARE(m_xtal->getSpaceGroupNumber(), 227U);
  QCOMPARE(m_xtal->getSpaceGroupSymbol(), QString("Fd-3m"));
}

void SPGLibTest::rutileFile()
{
  const QString filename = Common::localPath(Common::localPath(QString(TESTDATADIR), "formats"),
                      "rutile.POSCAR");
  std::ifstream in(filename.toStdString());
  QVERIFY(in.is_open());
  QVERIFY(Atoms::VaspFormat::read(*m_xtal, in));

  m_xtal->findSpaceGroup();
  QCOMPARE(m_xtal->getSpaceGroupNumber(), 136U);
  QCOMPARE(m_xtal->getSpaceGroupSymbol(), QString("P4_2/mnm"));
}

void SPGLibTest::toleranceRestoresSlightlyDistortedBCC()
{
  m_xtal->setCellInfo(3.0, 3.0, 3.0, 90.0, 90.0, 90.0);

  Atoms::Atom& atom1 = m_xtal->addAtom();
  atom1.setPos(Common::Vector3(0.0, 0.0, 0.0));
  atom1.setAtomicNumber(1);

  Atoms::Atom& atom2 = m_xtal->addAtom();
  atom2.setPos(Common::Vector3(1.5003, 1.4998, 1.5002));
  atom2.setAtomicNumber(1);

  m_xtal->findSpaceGroup(1.0e-2);
  QCOMPARE(m_xtal->getSpaceGroupNumber(), 229U);
  QCOMPARE(m_xtal->getSpaceGroupSymbol(), QString("Im-3m"));
}

void SPGLibTest::strictToleranceRejectsSlightlyDistortedBCC()
{
  m_xtal->setCellInfo(3.0, 3.0, 3.0, 90.0, 90.0, 90.0);

  Atoms::Atom& atom1 = m_xtal->addAtom();
  atom1.setPos(Common::Vector3(0.0, 0.0, 0.0));
  atom1.setAtomicNumber(1);

  Atoms::Atom& atom2 = m_xtal->addAtom();
  atom2.setPos(Common::Vector3(1.5003, 1.4998, 1.5002));
  atom2.setAtomicNumber(1);

  m_xtal->findSpaceGroup(1.0e-5);
  QVERIFY(m_xtal->getSpaceGroupNumber() != 229U ||
          m_xtal->getSpaceGroupSymbol() != QString("Im-3m"));
}

QTEST_MAIN(SPGLibTest)

#include "spglibtest.moc"

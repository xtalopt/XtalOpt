/**********************************************************************
  MolTransformations test - make sure our molecular transformation functions
                            work

  Copyright (C) 2017 Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 **********************************************************************/

#include <globalsearch/molecular/moltransformations.h>
#include <globalsearch/structures/molecule.h>

#include <QDebug>
#include <QString>
#include <QtTest>

#include <iostream>

using namespace GlobalSearch;

class MolTransformationsTest : public QObject
{
  Q_OBJECT

public:
  MolTransformationsTest();

private:
  double m_tol;

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
  void getMeanPosition();
  void setMeanPosition();
  void centerMolecule();
  void rotateMolecule();
  void translateMolecule();
};

MolTransformationsTest::MolTransformationsTest() : m_tol(1.e-5)
{
}

void MolTransformationsTest::initTestCase()
{
}

void MolTransformationsTest::cleanupTestCase()
{
}

void MolTransformationsTest::init()
{
}

void MolTransformationsTest::cleanup()
{
}

void MolTransformationsTest::getMeanPosition()
{
  Molecule mol;
  mol.addAtom(1, Vector3(1, 0, 0));
  mol.addAtom(1, Vector3(0, 1, 0));
  mol.addAtom(1, Vector3(0, 0, 1));

  QVERIFY(fuzzyCompare(MolTransformations::getMeanPosition(mol),
                       Vector3(0.33333, 0.33333, 0.33333), m_tol));
}

void MolTransformationsTest::setMeanPosition()
{
  Molecule mol;
  mol.addAtom(1, Vector3(1, 0, 0));
  mol.addAtom(1, Vector3(0, 1, 0));
  mol.addAtom(1, Vector3(0, 0, 1));

  Vector3 newMeanPos(0.50000, 1.26492, -1.75123);

  MolTransformations::setMeanPosition(mol, newMeanPos);

  QVERIFY(
    fuzzyCompare(MolTransformations::getMeanPosition(mol), newMeanPos, m_tol));
}

void MolTransformationsTest::centerMolecule()
{
  Molecule mol;
  mol.addAtom(1, Vector3(1, 0, 0));
  mol.addAtom(1, Vector3(0, 1, 0));
  mol.addAtom(1, Vector3(0, 0, 1));

  QVERIFY(fuzzyCompare(MolTransformations::getMeanPosition(mol),
                       Vector3(0.33333, 0.33333, 0.33333), m_tol));

  MolTransformations::centerMolecule(mol);

  QVERIFY(fuzzyCompare(MolTransformations::getMeanPosition(mol),
                       Vector3(0.00000, 0.00000, 0.00000), m_tol));

  QVERIFY(fuzzyCompare(mol.atom(0).pos(), Vector3(0.66667, -0.33333, -0.33333),
                       m_tol));
}

void MolTransformationsTest::rotateMolecule()
{
  Molecule mol;
  mol.addAtom(1, Vector3(1, 1, 0));
  mol.addAtom(1, Vector3(-1, -1, 0));
  mol.addAtom(1, Vector3(0, 0, 1));

  MolTransformations::rotateMolecule(mol, 2, 60.0 * DEG2RAD);

  // New position of the first atom should be as follows
  QVERIFY(fuzzyCompare(mol.atom(0).pos(), Vector3(-0.366025, 1.366025, 0.00000),
                       m_tol));

  // The atom on the z-axis should be in the same position
  QVERIFY(
    fuzzyCompare(mol.atom(2).pos(), Vector3(0.00000, 0.00000, 1.00000), m_tol));

  // Restore it to its original state
  MolTransformations::rotateMolecule(mol, 2, -60.0 * DEG2RAD);

  // Now let's test multiple rotations at once
  MolTransformations::rotateMolecule(mol, 0.0, 90.0 * DEG2RAD, 90.0 * DEG2RAD);

  QVERIFY(fuzzyCompare(mol.atom(2).pos(), Vector3(0.00000, -1.00000, 0.00000),
                       m_tol));
}

void MolTransformationsTest::translateMolecule()
{
  Molecule mol;
  mol.addAtom(1, Vector3(1, 0, 0));
  mol.addAtom(1, Vector3(0, 1, 0));
  mol.addAtom(1, Vector3(0, 0, 1));

  MolTransformations::translateMolecule(mol, 1.0, 0.0, -0.5);

  QVERIFY(fuzzyCompare(mol.atom(0).pos(), Vector3(2.00000, 0.00000, -0.50000),
                       m_tol));
  QVERIFY(fuzzyCompare(mol.atom(1).pos(), Vector3(1.00000, 1.00000, -0.50000),
                       m_tol));
  QVERIFY(
    fuzzyCompare(mol.atom(2).pos(), Vector3(1.00000, 0.00000, 0.50000), m_tol));
}

QTEST_MAIN(MolTransformationsTest)

#include "moltransformationstest.moc"

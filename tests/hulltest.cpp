/**********************************************************************
  HullTest - Unit tests for convex-hull calculations

  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <common/chull.h>

#include <QtTest>

class HullTest : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase()  {}
  void cleanupTestCase() {}
  void init()    {}
  void cleanup() {}

  // Tests
  void elemental();
  void binaryOnHull();
  void binaryAboveHull();
  void ternaryAboveHull();
  void missingReference();
};

// Elemental system (input_dim = 2).
void HullTest::elemental()
{
  // Format per point: [ x_A, E/atom ]
  // Three structures of the same element; ground state at E = -2.0 eV/atom.
  const int num = 3, dim = 2;
  std::vector<double> data = {
    1.0, -2.0,   // ground state: distance above hull = 0.0
    1.0, -1.5,   // 0.5 eV above: distance = 0.5
    1.0, -1.0,   // 1.0 eV above: distance = 1.0
  };

  std::vector<double> above(num, 0.0);
  QVERIFY(Common::distAboveHull(data, num, dim, above));

  QVERIFY(fabs(above[0])       < 1e-9);
  QVERIFY(fabs(above[1] - 0.5) < 1e-9);
  QVERIFY(fabs(above[2] - 1.0) < 1e-9);
}

// Binary system (with one point above the hull).
void HullTest::binaryOnHull()
{
  const int num = 4, dim = 3;
  std::vector<double> data = {
    1.00, 0.00, -1.00,   // pure A (reference),  E_form = 0   (on hull)
    0.00, 1.00, -2.00,   // pure B (reference),  E_form = 0   (on hull)
    0.50, 0.50, -1.50,   // 50/50,               E_form = 0   (on hull)
    0.50, 0.50, -1.00,   // 50/50, above hull by 0.5 eV/atom
  };

  std::vector<double> above(num, -1.0);
  QVERIFY(Common::distAboveHull(data, num, dim, above));

  // First three points are on the hull
  for (int i = 0; i < 3; ++i)
    QVERIFY2(fabs(above[i]) < 1e-9,
             qPrintable(QString("Point %1: expected 0, got %2").arg(i).arg(above[i])));

  // Fourth point is 0.5 above the hull
  QVERIFY(fabs(above[3] - 0.5) < 1e-9);
}

// Binary system (with one point above the hull).
void HullTest::binaryAboveHull()
{
  const int num = 3, dim = 3;
  std::vector<double> data = {
    1.00, 0.00, -1.00,   // pure A (reference)
    0.00, 1.00, -2.00,   // pure B (reference)
    0.50, 0.50, -1.00,   // 50/50, E_form = -1.0 - (-1.5) = +0.5 eV/atom above hull
  };

  std::vector<double> above(num, -1.0);
  QVERIFY(Common::distAboveHull(data, num, dim, above));

  // References are on the hull
  QVERIFY(fabs(above[0]) < 1e-9);
  QVERIFY(fabs(above[1]) < 1e-9);
  // Compound is 0.5 eV/atom above the tie-line
  QVERIFY(fabs(above[2] - 0.5) < 1e-9);
}

// Ternary system (with one point above the hull).
void HullTest::ternaryAboveHull()
{
  const double third = 1.0 / 3.0;
  const int num = 5, dim = 4;
  std::vector<double> data = {
    1.0,   0.0,   0.0,   -1.0,   // pure A
    0.0,   1.0,   0.0,   -2.0,   // pure B
    0.0,   0.0,   1.0,   -3.0,   // pure C
    third, third, third, -2.0,   // on the hull, E_form = 0
    third, third, third, -1.5,   // above hull by 0.5 eV/atom
  };

  std::vector<double> above(num, -1.0);
  QVERIFY(Common::distAboveHull(data, num, dim, above));

  // Pure elements on hull
  QVERIFY(fabs(above[0]) < 1e-9);
  QVERIFY(fabs(above[1]) < 1e-9);
  QVERIFY(fabs(above[2]) < 1e-9);
  // Equal-thirds compound on hull
  QVERIFY(fabs(above[3]) < 1e-9);
  // Equal-thirds compound 0.5 above hull
  QVERIFY(fabs(above[4] - 0.5) < 1e-9);
}

// Missing elemental reference: hull must return false.
void HullTest::missingReference()
{
  // Binary data with no pure-element entry for component A
  const int num = 2, dim = 3;
  std::vector<double> data = {
    0.00, 1.00, -2.00,   // only pure B present
    0.50, 0.50, -1.50,   // compound: no pure A reference
  };

  std::vector<double> above(num, 0.0);
  QVERIFY(!Common::distAboveHull(data, num, dim, above));
}

QTEST_MAIN(HullTest)

#include "hulltest.moc"

/**********************************************************************
  XtalTest -- Unit testing for XtalOpt::Xtal class

  Copyright (C) 2010 David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 **********************************************************************/

#include <xtalopt/structures/xtal.h>

#include <xtalopt/debug.h>

#include <globalsearch/eleminfo.h>
#include <globalsearch/formats/cmlformat.h>
#include <globalsearch/formats/formats.h>
#include <globalsearch/formats/poscarformat.h>
#include <globalsearch/macros.h>
#include <globalsearch/random.h>

#include <Eigen/Geometry>

#include <QDebug>
#include <QString>
#include <QtTest>

#include <fstream>

#define ASSIGN_PARAMS(a, b, c, alpha, beta, gamma)                             \
  if (!xtal)                                                                   \
    xtal = new Xtal(a, b, c, alpha, beta, gamma);                              \
  else                                                                         \
    xtal->setCellInfo(a, b, c, alpha, beta, gamma);

#define ROUGH_EQ(v1, v2) (fabs((v1) - (v2)) < 1e-3)
#define VERIFY_PARAMS(a_, b_, c_, alpha_, beta_, gamma_)                       \
  QVERIFY(ROUGH_EQ(a_, xtal->getA()));                                         \
  QVERIFY(ROUGH_EQ(b_, xtal->getB()));                                         \
  QVERIFY(ROUGH_EQ(c_, xtal->getC()));                                         \
  QVERIFY(ROUGH_EQ(alpha_, xtal->getAlpha()));                                 \
  QVERIFY(ROUGH_EQ(beta_, xtal->getBeta()));                                   \
  QVERIFY(ROUGH_EQ(gamma_, xtal->getGamma()));

#define DEBUG_ATOM(t, c)                                                       \
  printf("%2d %9.5f %9.5f %9.5f\n", t, (c).x(), (c).y(), (c).z())
#define DEBUG_VECTOR(v)                                                        \
  printf("| %9.5f %9.5f %9.5f |\n", (v).x(), (v).y(), (v).z())
#define DEBUG_MATRIX(m)                                                        \
  printf("| %9.5f %9.5f %9.5f |\n"                                             \
         "| %9.5f %9.5f %9.5f |\n"                                             \
         "| %9.5f %9.5f %9.5f |\n",                                            \
         (m)(0, 0), (m)(0, 1), (m)(0, 2), (m)(1, 0), (m)(1, 1), (m)(1, 2),     \
         (m)(2, 0), (m)(2, 1), (m)(2, 2))

using namespace XtalOpt;
using namespace GlobalSearch;

class XtalTest : public QObject
{
  Q_OBJECT

private:
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

  // tests
  void rotateToStdOrientationTest();
  void compareCoordinatesTest_simple();
  void compareCoordinatesTest_shifted();
  void compareCoordinatesTest_huge();
  void equalityOperatorTest_simple();
  void equalityOperatorTest_shifted();
  void equalityOperatorTest_huge();
  void niggliReduceTest();
  void fixAnglesTest();
  void getRandomRepresentationTest();
  void equalityVsFingerprintTest();
};

void XtalTest::initTestCase()
{
}

void XtalTest::cleanupTestCase()
{
}

void XtalTest::init()
{
}

void XtalTest::cleanup()
{
}

void XtalTest::rotateToStdOrientationTest()
{
  Xtal xtal;
  Vector3 v1, v2, v3;
  double origVolume, newVolume;
  double origA, newA;
  double origB, newB;
  double origC, newC;
  double origAlpha, newAlpha;
  double origBeta, newBeta;
  double origGamma, newGamma;

#define ROTTEST_GET_ORIG_INFO                                                  \
  origVolume = xtal.getVolume();                                               \
  origA = xtal.getA();                                                         \
  origB = xtal.getB();                                                         \
  origC = xtal.getC();                                                         \
  origAlpha = xtal.getAlpha();                                                 \
  origBeta = xtal.getBeta();                                                   \
  origGamma = xtal.getGamma()
#define ROTTEST_GET_NEW_INFO                                                   \
  newVolume = xtal.getVolume();                                                \
  newA = xtal.getA();                                                          \
  newB = xtal.getB();                                                          \
  newC = xtal.getC();                                                          \
  newAlpha = xtal.getAlpha();                                                  \
  newBeta = xtal.getBeta();                                                    \
  newGamma = xtal.getGamma()
#define ROTTEST_VERIFY_INFO                                                    \
  QCOMPARE(newVolume, origVolume);                                             \
  QCOMPARE(newA, origA);                                                       \
  QCOMPARE(newB, origB);                                                       \
  QCOMPARE(newC, origC);                                                       \
  QCOMPARE(newAlpha, origAlpha);                                               \
  QCOMPARE(newBeta, origBeta);                                                 \
  QCOMPARE(newGamma, origGamma)
#define ROTTEST_ROT_AND_TEST                                                   \
  ROTTEST_GET_ORIG_INFO;                                                       \
  QVERIFY(xtal.rotateCellToStandardOrientation());                             \
  ROTTEST_GET_NEW_INFO;                                                        \
  /*ROTTEST_DEBUG_INFO;*/                                                      \
  ROTTEST_VERIFY_INFO
#define ROTTEST_DEBUG_INFO                                                     \
  qDebug() << newVolume << origVolume << endl                                  \
           << newA << origA << endl                                            \
           << newB << origB << endl                                            \
           << newC << origC << endl                                            \
           << newAlpha << origAlpha << endl                                    \
           << newBeta << origBeta << endl                                      \
           << newGamma << origGamma;                                           \
  std::cout << xtal.OBUnitCell()->GetCellMatrix()

  // test cells that are already in std orientation:
  xtal.setCellInfo(3, 3, 3, 90, 90, 90);
  ROTTEST_ROT_AND_TEST;

  xtal.setCellInfo(3, 3, 3, 70, 90, 80);
  ROTTEST_ROT_AND_TEST;

  xtal.setCellInfo(3, 3, 3, 120, 123, 100);
  ROTTEST_ROT_AND_TEST;

  xtal.setCellInfo(4, 3, 1, 75.44444, 68.33333, 123.15682);
  ROTTEST_ROT_AND_TEST;

  // These cell will need rotation
  v1 = Vector3(1, -4, 3);
  v2 = Vector3(0, 5, -8);
  v3 = Vector3(0, 0, -3);
  xtal.setCellInfo(v1, v2, v3);
  ROTTEST_ROT_AND_TEST;

  v1 = Vector3(1, 3, 6);
  v2 = Vector3(-4, 5, 1);
  v3 = Vector3(3, -8, -3);
  xtal.setCellInfo(v1, v2, v3);
  ROTTEST_ROT_AND_TEST;
}

void XtalTest::compareCoordinatesTest_simple()
{
  Xtal xtal1, xtal2;

  xtal1 = Xtal(2, 2, 2, 90, 90, 90);
  Atom& atm = xtal1.addAtom();
  atm.setPos(Vector3(0, 0, 0));
  xtal2 = xtal1;
  QVERIFY(xtal1.compareCoordinates(xtal2));
}

void XtalTest::compareCoordinatesTest_shifted()
{
  Xtal xtal1, xtal2;

  xtal1 = Xtal(2, 2, 2, 90, 90, 90);
  Atom& atm = xtal1.addAtom();
  atm.setPos(Vector3(1, 1, 1));
  xtal2 = xtal1;
  QVERIFY(xtal1.compareCoordinates(xtal2));
}

void XtalTest::compareCoordinatesTest_huge()
{
  Xtal xtal1, xtal2;

  xtal1.setCellInfo(20, 30, 30, 60, 75.5225, 70.5288);

  for (double x = 0.0; x < .999; x += 0.333333333333) {
    for (double y = 0.0; y < .999; y += 0.333333333333) {
      for (double z = 0.0; z < .999; z += 0.333333333333) {
        Atom& atm = xtal1.addAtom();
        atm.setPos(xtal1.fracToCart(Vector3(x, y, z)));
        atm.setAtomicNumber(static_cast<int>(10 * (x + y + z)) % 3);
      }
    }
  }

  // Test for equality
  xtal2 = xtal1;
  QVERIFY(xtal1.compareCoordinates(xtal2));

  // Delete an atom and ensure that the comparison fails
  xtal2.removeAtom(xtal2.atom(0));
  QVERIFY(!xtal1.compareCoordinates(xtal2));
}

void XtalTest::equalityOperatorTest_simple()
{
  Xtal xtal1, xtal2;

  xtal1 = Xtal(2, 2, 2, 90, 90, 90);
  Atom& atm = xtal1.addAtom();
  atm.setPos(Vector3(0, 0, 0));
  xtal2 = xtal1;
  QVERIFY(xtal1 == xtal2);
  // Change cell size and retest
  xtal2.setCellInfo(4, 4, 4, 90, 90, 90);
  QVERIFY(xtal1 != xtal2);
}

void XtalTest::equalityOperatorTest_shifted()
{
  Xtal xtal1, xtal2;

  xtal1 = Xtal(2, 2, 2, 90, 90, 90);
  Atom& atm = xtal1.addAtom();
  atm.setPos(Vector3(1, 1, 1));
  xtal2 = xtal1;
  QVERIFY(xtal1 == xtal2);
  // Change cell size and retest
  xtal2.setCellInfo(4, 4, 4, 90, 90, 90);
  QVERIFY(xtal1 != xtal2);
}

void XtalTest::equalityOperatorTest_huge()
{
  Xtal xtal1, xtal2;

  xtal1.setCellInfo(20, 30, 30, 60, 75.5225, 70.5288);

  for (double x = 0.0; x < 0.999; x += 0.333333333333) {
    for (double y = 0.0; y < 0.999; y += 0.333333333333) {
      for (double z = 0.0; z < 0.999; z += 0.333333333333) {
        Atom& atm = xtal1.addAtom();
        atm.setPos(xtal1.fracToCart(Vector3(x, y, z)));
        atm.setAtomicNumber(static_cast<int>(10 * (x + y + z)) % 3);
      }
    }
  }

  // Test for equality
  xtal2 = xtal1;
  QVERIFY(xtal1 == xtal2);

  // Change cell size and retest
  xtal2.setCellInfo(4, 4, 4, 90, 90, 90);
  QVERIFY(xtal1 != xtal2);

  // Delete an atom and ensure that the comparison fails
  xtal2 = xtal1;
  xtal2.removeAtom(xtal2.atom(0));
  QVERIFY(xtal1 != xtal2);
}

void XtalTest::niggliReduceTest()
{
  // Seed the random number generator to ensure similar results each run
  seedMt19937Generator(0);
  srand(0);

  Xtal* xtal = 0;

  // Test from Gruber-Krivy 1976
  ASSIGN_PARAMS(3.000, 5.19615242271, 2.00, 103.919748556, 109.471220635,
                134.882107117);
  QVERIFY(xtal->niggliReduce());
  VERIFY_PARAMS(2.0, 3.0, 3.0, 60.0, 75.5225, 70.5288);

  // Test from Gruber 1973
  ASSIGN_PARAMS(2.000, 11.6619037897, 8.71779788708, 139.667309857,
                152.746099475, 019.396625679);
  QVERIFY(xtal->niggliReduce());
  VERIFY_PARAMS(2.0, 4.0, 4.0, 60.0, 79.1931, 75.5225);

  // These have failed in the past:
  ASSIGN_PARAMS(5.33246, 7.54122, 7.64391, 75.7212, 110.414, 44.9999);
  QVERIFY(xtal->niggliReduce());
  //
  ASSIGN_PARAMS(11.4674, 15.8504, 17.3282, 87.6738, 90, 80.141);
  QVERIFY(xtal->niggliReduce());

  // Random test
  const double minLength = 10.0;
  const double maxLength = 30.0;
  const double minAngle = 45.0;
  const double maxAngle = 135.0;
  for (unsigned int i = 0; i < 1000; i++) {
    const double a = getRandDouble() * (maxLength - minLength) + minLength;
    const double b = getRandDouble() * (maxLength - minLength) + minLength;
    const double c = getRandDouble() * (maxLength - minLength) + minLength;
    const double alpha = getRandDouble() * (maxAngle - minAngle) + minAngle;
    const double beta = getRandDouble() * (maxAngle - minAngle) + minAngle;
    const double gamma = getRandDouble() * (maxAngle - minAngle) + minAngle;
    // is the cell valid?
    UnitCell tmp(alpha, beta, gamma, a, b, c);
    if (tmp.cellMatrix().determinant() <= 0 ||
        GS_IS_NAN_OR_INF(tmp.cellMatrix().determinant())) {
      i--;
      continue;
    }
    ASSIGN_PARAMS(a, b, c, alpha, beta, gamma);
    QVERIFY2(xtal->niggliReduce(),
             QString("Unable to reduce cell. Params: %1 %2 %3 %4 %5 %6")
               .arg(a)
               .arg(b)
               .arg(c)
               .arg(alpha)
               .arg(beta)
               .arg(gamma)
               .toStdString()
               .c_str());
    QVERIFY2(
      xtal->isNiggliReduced(),
      QString(
        "Cell did not reduced to niggli cell. Final params: %1 %2 %3 %4 %5 %6")
        .arg(xtal->getA())
        .arg(xtal->getB())
        .arg(xtal->getC())
        .arg(xtal->getAlpha())
        .arg(xtal->getBeta())
        .arg(xtal->getGamma())
        .toStdString()
        .c_str());
  }
}

struct CellParam
{
  CellParam(const double& a_, const double& b_, const double& c_,
            const double& alpha_, const double& beta_, const double& gamma_)
  {
    a = a_;
    b = b_;
    c = c_;
    alpha = alpha_;
    beta = beta_;
    gamma = gamma_;
  };
  double a, b, c, alpha, beta, gamma;
};

void XtalTest::fixAnglesTest()
{
  // Seed the random number generator to ensure similar results each run
  seedMt19937Generator(0);

  Xtal xtal;
  const double minLength = 10.0;
  const double maxLength = 30.0;
  const double minAngle = 45.0;
  const double maxAngle = 135.0;
  QList<CellParam> badParams;
  Vector3 axis;
  Eigen::Matrix3d mat;
  for (unsigned int iter = 0; iter < 100; iter++) {
    const double a = getRandDouble() * (maxLength - minLength) + minLength;
    const double b = getRandDouble() * (maxLength - minLength) + minLength;
    const double c = getRandDouble() * (maxLength - minLength) + minLength;
    const double alpha = getRandDouble() * (maxAngle - minAngle) + minAngle;
    const double beta = getRandDouble() * (maxAngle - minAngle) + minAngle;
    const double gamma = getRandDouble() * (maxAngle - minAngle) + minAngle;
    // is the cell valid?
    UnitCell tmp(alpha, beta, gamma, a, b, c);
    if (tmp.cellMatrix().determinant() <= 0 ||
        GS_IS_NAN_OR_INF(tmp.cellMatrix().determinant())) {
      --iter;
      continue;
    }

    // Create random rotation matrix
    Eigen::AngleAxis<double> t(getRandDouble() * 360.0, axis.setRandom());

    // Rotate cell
    tmp.setCellMatrix(t * tmp.cellMatrix());

    // Update cell
    xtal.setUnitCell(tmp);

    // Add some atoms
    for (unsigned int i = 0; i < getRandUInt() % 100; i++) {
      Atom& atm = xtal.addAtom();
      atm.setPos(xtal.fracToCart(
        Vector3(getRandDouble(), getRandDouble(), getRandDouble())));
      atm.setAtomicNumber(getRandUInt() % 5);
    }
    if (!xtal.fixAngles()) {
      badParams.push_back(CellParam(a, b, c, alpha, beta, gamma));
      iter--;
      continue;
    }
    QVERIFY(xtal.isNiggliReduced());
  }

  if (badParams.size() != 0) {
    printf("%5s %10s %10s %10s %10s %10s %10s\n", "num", "a", "b", "c", "alpha",
           "beta", "gamma");
    for (int i = 0; i < badParams.size(); i++) {
      printf("%5d %10f %10f %10f %10f %10f %10f\n", i + 1, badParams.at(i).a,
             badParams.at(i).b, badParams.at(i).c, badParams.at(i).alpha,
             badParams.at(i).beta, badParams.at(i).gamma);
    }
  }
  QVERIFY2(badParams.size() == 0, "The above cells did not reduce cleanly.");
}

void XtalTest::getRandomRepresentationTest()
{
  // Seed the random number generator to ensure similar results between tests
  seedMt19937Generator(0);

  // Parameters:
  const int iterations = 250;
  const int numAtoms = 50;

  Xtal* nxtal = 0;

  QTime start, end;
  unsigned long long int success_msecs = 0;
  unsigned long long int failure_msecs = 0;

  for (int i = 0; i < iterations; ++i) {
    //
    // Build initial xtal
    //
    //  - First select lattice lengths. Generate a list of three
    //    random doubles between 0 and 1, the second two have a finite
    //    probability of being the same as the previous one. This
    //    gives the following probabiltities P(N), where N is the
    //    number of unique lengths:
    //
    //    P(1) = (1/3)(1/3)              = 1/9
    //    P(2) = (1/3)(2/3) + (2/3)(1/3) = 4/9
    //    P(3) = (2/3)(2/3)              = 4/9
    //
    std::vector<double> lengths(3);
    lengths[0] = getRandDouble();
    if (getRandDouble() < 0.3333333)
      lengths[1] = lengths[0];
    else
      lengths[1] = getRandDouble();
    if (getRandDouble() < 0.3333333)
      lengths[2] = lengths[1];
    else
      lengths[2] = getRandDouble();
    //
    // Adjust each length to be between 5->25 angstrom
    //
    lengths[0] = lengths[0] * 20 + 5;
    lengths[1] = lengths[1] * 20 + 5;
    lengths[2] = lengths[2] * 20 + 5;
    //
    // Randomize the order
    //
    std::random_shuffle(lengths.begin(), lengths.end());
    //
    //  - Now for the angles. Similarly, each may be the same as the
    //    previous, but there is also a 1/3 chance that the angle will
    //    be 90 degrees.
    //
    double rand;
    std::vector<double> angles(3);
    angles[0] = getRandDouble();
    rand = getRandDouble();
    if (rand < 0.33333333)
      angles[1] = angles[0];
    else if (rand < 0.6666666)
      angles[1] = 0.5; // will convert to 90
    else
      angles[1] = getRandDouble(); // degrees later
    rand = getRandDouble();
    if (rand < 0.33333333)
      angles[2] = angles[1];
    else if (rand < 0.6666666)
      angles[2] = 0.5; // will convert to 90
    else
      angles[2] = getRandDouble(); // degrees later
    //
    // Adjust each angle to lie between 60->120 degrees
    //
    angles[0] = angles[0] * 60 + 60;
    angles[1] = angles[1] * 60 + 60;
    angles[2] = angles[2] * 60 + 60;
    //
    // Randomize the order
    //
    std::random_shuffle(angles.begin(), angles.end());
    //
    // Construct xtal
    //
    Xtal xtal(lengths[0], lengths[1], lengths[2], angles[0], angles[1],
              angles[2]);

    //
    // Randomly add between 50 atoms to the cell. No more than 5
    // atomic species will be present.
    //
    unsigned int failedAtomAdds = 0;
    unsigned int failedAtomAddsMax = 10;
    for (int j = 0; j < numAtoms; ++j) {
      unsigned short atomicNum = getRandUInt() % 5 + 1;
      if (!xtal.addAtomRandomly(atomicNum, 0.5)) {
        --j;
        ++failedAtomAdds;
        if (failedAtomAdds > failedAtomAddsMax) {
          break;
        }
        continue;
      }
    }
    if (failedAtomAdds > failedAtomAddsMax) {
      --i;
      continue;
    }

    nxtal = xtal.getRandomRepresentation();

    // Check that comparison algorithm detects that these are identical
    start = QTime::currentTime();
    bool match = (xtal == *nxtal);
    end = QTime::currentTime();
    success_msecs += start.msecsTo(end);

    if (!match) {
      qDebug() << "Failure on comparison" << i + 1 << "(false negative)";
    }
    QVERIFY(match);

    // Signficantly displace an atom of nxtal and ensure that the
    // comparison fails. Displacement is ~1-4 angstrom
    Q_ASSERT(nxtal->numAtoms() > 3);
    Vector3 displacement;
    displacement.x() = getRandDouble() + 1.0;
    displacement.y() = getRandDouble() + 1.0;
    displacement.z() = getRandDouble() + 1.0;
    nxtal->atom(0).setPos(nxtal->atom(0).pos() + displacement);

    start = QTime::currentTime();
    match = (xtal == *nxtal);
    end = QTime::currentTime();
    failure_msecs += start.msecsTo(end);

    if (match) {
      // Move atom back
      nxtal->atom(0).setPos(nxtal->atom(0).pos() - displacement);
      qDebug() << "Failure on comparison" << i + 1 << "(false positive)";
    }
    QVERIFY(!match);

    nxtal->deleteLater();
  }

  qDebug() << QString(
                "Made 2 * %1 comparisons of %2 atom unit cells, one positive "
                "control and one negative control for each pair of structures."
                "\n\tSuccess time: %3 ms total (%4 ms average), Failure time: "
                "%5 ms total (%6 ms average).")
                .arg(iterations)
                .arg(numAtoms)
                .arg(success_msecs)
                .arg(success_msecs / static_cast<double>(iterations))
                .arg(failure_msecs)
                .arg(failure_msecs / static_cast<double>(iterations));
}

void XtalTest::equalityVsFingerprintTest()
{
  // Load rutile POSCAR: (16xTiO2)
  std::stringstream rutilePOSCAR("\
Ti16 O32 \n\
1\n\
    6.01724500   0.00000000   0.00000000\n\
    0.00000193   6.35400106   0.00000000\n\
   -0.00000303  -0.00000000  12.70800514\n\
Ti O  \n\
16 32 \n\
Direct\n\
    0.31620100   0.59409200   0.00000000\n\
    0.31620100   0.59409300   0.50000000\n\
    0.06620100   0.09409200   0.50000000\n\
    0.06620100   0.59409200   0.75000000\n\
    0.56620100   0.09409200   0.50000000\n\
    0.81620000   0.09409200   0.25000000\n\
    0.56620100   0.59409300   0.75000000\n\
    0.31620100   0.09409200   0.75000000\n\
    0.81620100   0.59409200   0.00000100\n\
    0.06620100   0.59409200   0.25000000\n\
    0.81620100   0.09409200   0.75000000\n\
    0.81620100   0.59409200   0.50000000\n\
    0.31620000   0.09409200   0.25000000\n\
    0.56620000   0.59409200   0.25000000\n\
    0.56620000   0.09409300   0.00000000\n\
    0.06620100   0.09409200   0.00000000\n\
    0.06620000   0.59409200   0.59835800\n\
    0.81620100   0.89737700   0.50000000\n\
    0.56620100   0.59409200   0.09835800\n\
    0.06620000   0.09409100   0.15164300\n\
    0.56620100   0.09409100   0.65164300\n\
    0.06620000   0.09409300   0.34835700\n\
    0.31620100   0.29080700   0.50000000\n\
    0.06620200   0.59409200   0.09835700\n\
    0.06620200   0.59409200   0.90164300\n\
    0.81620000   0.29080700   0.00000100\n\
    0.56620000   0.59409400   0.59835700\n\
    0.31620100   0.79080600   0.25000000\n\
    0.31620200   0.29080600   0.00000000\n\
    0.81620000   0.39737700   0.75000000\n\
    0.56620100   0.09409300   0.34835700\n\
    0.31620100   0.39737700   0.25000000\n\
    0.81620200   0.89737700   0.00000000\n\
    0.06620100   0.09409200   0.65164300\n\
    0.31620100   0.79080600   0.75000100\n\
    0.56620200   0.59409100   0.40164300\n\
    0.81620100   0.79080600   0.75000100\n\
    0.56620100   0.09409200   0.84835700\n\
    0.81620100   0.39737700   0.25000000\n\
    0.81620100   0.29080700   0.50000000\n\
    0.31620100   0.39737700   0.75000000\n\
    0.81620100   0.79080600   0.25000000\n\
    0.31620000   0.89737800   0.50000000\n\
    0.31620100   0.89737800   0.00000000\n\
    0.56620200   0.59409200   0.90164300\n\
    0.06620100   0.09409300   0.84835700\n\
    0.06620100   0.59409200   0.40164300\n\
    0.56620000   0.09409200   0.15164300\n");

  // This is the niggli reduced rutile structure
  Xtal rutileSeed;
  QVERIFY(PoscarFormat::read(rutileSeed, rutilePOSCAR));

  // List to store reproductions
  QList<Xtal*> rutiles;

  // Generate a random representation of each structure by applying
  // each mix and transformation in the Xtal static lists
  const Matrix3 oldCell = rutileSeed.unitCell().cellMatrix();
  Matrix3 newCell;
  for (QVector<Eigen::Matrix3d>::const_iterator
         xform = Xtal::m_transformationMatrices.constBegin(),
         xform_end = Xtal::m_transformationMatrices.constEnd();
       xform != xform_end; ++xform) {
    const Eigen::Matrix3d xformTranspose(xform->transpose());
    for (QVector<Eigen::Matrix3d>::const_iterator
           mix = Xtal::m_mixMatrices.constBegin(),
           mix_end = Xtal::m_mixMatrices.constEnd();
         mix != mix_end; ++mix) {
      newCell = (*mix) * oldCell * xformTranspose;
      rutiles << new Xtal(rutileSeed);
      rutiles.last()->setCellInfo(newCell);
      // Transform atoms
      std::vector<Atom>& newAtoms = rutiles.last()->atoms();
      for (auto& atom : newAtoms) {
        atom.setPos((*xform) * atom.pos());
      }
    }
  }

  // Now a uniform translation to each structure. Initialize the
  // random number generator to the same value to ensure consistent
  // results.
  srand(0);
  Vector3 uTranslation(rand(), rand(), rand());
  uTranslation.normalize();
  // Now loop through all structures in rutile seeds, creating new
  // xtals with random noise
  double coordNoiseMax = 0.005; // angstrom
  // double angleNoiseMax = 0.150; // degree
  // double lengthNoiseMax= 0.005; // angstrom

  const unsigned int noiselessDups = rutiles.size();

  Vector3 curUTranslation; // xtal-specific uniform translation
  Vector3 curNTranslation; // xtal-specific noise translation
  for (unsigned int i = 0; i < noiselessDups; ++i) {
    rutiles << new Xtal(*rutiles.at(i));
    curUTranslation = uTranslation * i;
    std::vector<Atom>& currentAtoms = rutiles.last()->atoms();
    for (auto& atom : currentAtoms) {
      curNTranslation << rand(), rand(), rand();
      curNTranslation.normalize();
      curNTranslation *= coordNoiseMax;
      atom.setPos(atom.pos() + curUTranslation + curNTranslation);
    }
  }

  qDebug() << "\n"
           << noiselessDups << "pure mix/rot/ref structures\n"
           << rutiles.size() << "total structures.";

  // Perform niggli reduction on each structure. This will also wrap
  // atoms to the cell. Calculate the spacegroups first, this more
  // closely resembles the fingerprint method.
  rutileSeed.findSpaceGroup();
  QVERIFY(rutileSeed.niggliReduce());
  for (QList<Xtal *>::iterator it = rutiles.begin(), it_end = rutiles.end();
       it != it_end; ++it) {
    (*it)->findSpaceGroup();
    QVERIFY((*it)->niggliReduce());
  }

  // Test equality between each rutile and the seed
  unsigned int count = 0;
  for (QList<Xtal *>::iterator it = rutiles.begin(), it_end = rutiles.end();
       it != it_end; ++it) {
    ++count;
    bool match = (rutileSeed == *(*it));
    if (!match) {
      XtalOptDebug::dumpPseudoPwscfOut(&rutileSeed, "Testing/seed");
      XtalOptDebug::dumpPseudoPwscfOut((*it), "Testing/gen");
      qDebug() << "Failure on comparison" << count;
    }
    QVERIFY(match);
  }

  qDebug() << "\n"
           << "All structures pass the XtalComparison test.";

  // Compare the "old" fingerprint method.
  unsigned int failed = 0;
  count = 0;
  // structure number (>=1) and spg number
  QHash<unsigned int, unsigned int> failures;
  for (QList<Xtal *>::iterator it = rutiles.begin(), it_end = rutiles.end();
       it != it_end; ++it) {
    ++count;
    // Be generous and assume that the enthalpies match. All
    // transformations preserve volume, so don't check that either.
    if (rutileSeed.getSpaceGroupNumber() != (*it)->getSpaceGroupNumber()) {
      ++failed;
      failures.insert(count, (*it)->getSpaceGroupNumber());
    }
  }

  qDebug() << "\n"
           << failed << "structures failed the "
                        "fingerprint comparison (volume + spg):";

  printf("Seeded structure has spacegroup: %d\n",
         rutileSeed.getSpaceGroupNumber());
  QList<unsigned int> failureKeys = failures.keys();
  qSort(failureKeys);
  unsigned int entriesInLine = 0;
  for (QList<unsigned int>::const_iterator it = failureKeys.constBegin(),
                                           it_end = failureKeys.constEnd();
       it != it_end; ++it) {
    printf("%03d: %-3d  ", // width = 10
           (*it), failures[*it]);
    if (++entriesInLine == 7) {
      printf("\n");
      entriesInLine = 0;
    }
  }
}

QTEST_MAIN(XtalTest)

#include "xtaltest.moc"

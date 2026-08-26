/**********************************************************************
  XtalTest - Unit testing for Xtal class in XtalOpt

  Copyright (C) 2010 David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/structures/xtal.h>
#include <xtalopt/xtalopt.h>

#include <search/tracker.h>

#include <common/constants.h>
#include <common/fileutils.h>
#include <common/output.h>
#include <atoms/eleminfo.h>
#include <atoms/formats/vaspformat.h>
#include <common/compatibility/platform_compat.h>
#include <common/random.h>

#include <QString>
#include <QDir>
#include <QTemporaryDir>
#include <QtTest>

#include <algorithm>
#include <random>

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

using namespace XtalOpt;
using namespace Search;

namespace {

Common::Vector3 randomUnitVector()
{
  Common::Vector3 axis;
  do {
    axis = Common::Vector3(Common::getRandDouble(-1.0, 1.0), Common::getRandDouble(-1.0, 1.0),
                   Common::getRandDouble(-1.0, 1.0));
  } while (axis.squaredNorm() < 1.0e-12);
  axis.normalize();
  return axis;
}

Common::Matrix3 rotationFromAxisAngle(const Common::Vector3& axis, double angleDegrees)
{
  const double angleRadians = angleDegrees * DEG2RAD;
  const double c = std::cos(angleRadians);
  const double s = std::sin(angleRadians);
  const double t = 1.0 - c;

  Common::Matrix3 rotation;
  rotation << t * axis.x() * axis.x() + c,
              t * axis.x() * axis.y() - s * axis.z(),
              t * axis.x() * axis.z() + s * axis.y(),
              t * axis.x() * axis.y() + s * axis.z(),
              t * axis.y() * axis.y() + c,
              t * axis.y() * axis.z() - s * axis.x(),
              t * axis.x() * axis.z() - s * axis.y(),
              t * axis.y() * axis.z() + s * axis.x(),
              t * axis.z() * axis.z() + c;
  return rotation;
}

} // namespace

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
  void compareCoordinatesTest_huge();
  void equalityOperatorTest_huge();
  void niggliReduceTest();
  void fixAnglesTest();
  void getRandomRepresentationTest();
  void checkInteratomicDistancesTest();
  void moveAtomRandomlyChecksDistancesAgainstEachAtom();
  void writeReadSettingsPreservesCompositionValidity();
  void resultsEntryUsesComputedSpaceGroup();
  void hullFormattingTest();
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
  Common::Vector3 v1, v2, v3;
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
  QVERIFY(xtal.rotateCellAndCoordsToStandardOrientation());                    \
  ROTTEST_GET_NEW_INFO;                                                        \
  ROTTEST_VERIFY_INFO

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
  v1 = Common::Vector3(1, -4, 3);
  v2 = Common::Vector3(0, 5, -8);
  v3 = Common::Vector3(0, 0, -3);
  xtal.setCellInfo(v1, v2, v3);
  ROTTEST_ROT_AND_TEST;

  v1 = Common::Vector3(1, 3, 6);
  v2 = Common::Vector3(-4, 5, 1);
  v3 = Common::Vector3(3, -8, -3);
  xtal.setCellInfo(v1, v2, v3);
  ROTTEST_ROT_AND_TEST;
}

void XtalTest::compareCoordinatesTest_huge()
{
  Xtal xtal1, xtal2;

  xtal1.setCellInfo(20, 30, 30, 60, 75.5225, 70.5288);

  for (double x = 0.0; x < .999; x += 0.333333333333) {
    for (double y = 0.0; y < .999; y += 0.333333333333) {
      for (double z = 0.0; z < .999; z += 0.333333333333) {
        Atoms::Atom& atm = xtal1.addAtom();
        atm.setPos(xtal1.fracToCart(Common::Vector3(x, y, z)));
        atm.setAtomicNumber(static_cast<int>(10 * (x + y + z)) % 3);
      }
    }
  }

  // Test for equality
  xtal2 = xtal1;
  QVERIFY(xtal1.compareXtalComp(xtal2));

  // Delete an atom and ensure that the comparison fails
  xtal2.removeAtom(xtal2.atom(0));
  QVERIFY(!xtal1.compareXtalComp(xtal2));
}

void XtalTest::equalityOperatorTest_huge()
{
  Xtal xtal1, xtal2;

  xtal1.setCellInfo(20, 30, 30, 60, 75.5225, 70.5288);

  for (double x = 0.0; x < 0.999; x += 0.333333333333) {
    for (double y = 0.0; y < 0.999; y += 0.333333333333) {
      for (double z = 0.0; z < 0.999; z += 0.333333333333) {
        Atoms::Atom& atm = xtal1.addAtom();
        atm.setPos(xtal1.fracToCart(Common::Vector3(x, y, z)));
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
  Common::seedMt19937Generator(0);
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

  // Test randomly generated cells.
  for (unsigned int i = 0; i < 200; i++) {
    const double a = Common::getRandDouble() * (maxLength - minLength) + minLength;
    const double b = Common::getRandDouble() * (maxLength - minLength) + minLength;
    const double c = Common::getRandDouble() * (maxLength - minLength) + minLength;
    const double alpha = Common::getRandDouble() * (maxAngle - minAngle) + minAngle;
    const double beta = Common::getRandDouble() * (maxAngle - minAngle) + minAngle;
    const double gamma = Common::getRandDouble() * (maxAngle - minAngle) + minAngle;

    // is the cell valid?
    Atoms::UnitCell tmp(a, b, c, alpha, beta, gamma);
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
  Common::seedMt19937Generator(0);

  Xtal xtal;
  const double minLength = 10.0;
  const double maxLength = 30.0;
  const double minAngle = 45.0;
  const double maxAngle = 135.0;

  QList<CellParam> badParams;

  // Test random rotated cells.
  for (unsigned int iter = 0; iter < 30; iter++) {
    const double a = Common::getRandDouble() * (maxLength - minLength) + minLength;
    const double b = Common::getRandDouble() * (maxLength - minLength) + minLength;
    const double c = Common::getRandDouble() * (maxLength - minLength) + minLength;
    const double alpha = Common::getRandDouble() * (maxAngle - minAngle) + minAngle;
    const double beta = Common::getRandDouble() * (maxAngle - minAngle) + minAngle;
    const double gamma = Common::getRandDouble() * (maxAngle - minAngle) + minAngle;

    // is the cell valid?
    Atoms::UnitCell tmp(a, b, c, alpha, beta, gamma);
    if (tmp.cellMatrix().determinant() <= 0 ||
        GS_IS_NAN_OR_INF(tmp.cellMatrix().determinant())) {
      --iter;
      continue;
    }

    // Create random rotation matrix
    const Common::Matrix3 rotation = rotationFromAxisAngle(randomUnitVector(),
                                                   Common::getRandDouble() * 360.0);

    // Rotate cell
    tmp.setCellMatrix(rotation * tmp.cellMatrix());

    // Update cell
    xtal.setUnitCell(tmp);

    // Add some atoms
    for (unsigned int i = 0; i < Common::getRandUInt() % 100; i++) {
      Atoms::Atom& atm = xtal.addAtom();
      atm.setPos(xtal.fracToCart(
        Common::Vector3(Common::getRandDouble(), Common::getRandDouble(), Common::getRandDouble())));
      atm.setAtomicNumber(Common::getRandUInt() % 5);
    }
    if (!xtal.fixAngles()) {
      badParams.push_back(CellParam(a, b, c, alpha, beta, gamma));
      iter--;
      continue;
    }
    QVERIFY(xtal.isNiggliReduced());
  }

  if (badParams.size() != 0) {
    Common::message(QString("%1 %2 %3 %4 %5 %6 %7")
                     .arg("num", 5)
                     .arg("a", 10)
                     .arg("b", 10)
                     .arg("c", 10)
                     .arg("alpha", 10)
                     .arg("beta", 10)
                     .arg("gamma", 10));
    for (int i = 0; i < badParams.size(); i++) {
      Common::message(QString("%1 %2 %3 %4 %5 %6 %7")
                       .arg(i + 1, 5)
                       .arg(badParams.at(i).a, 10, 'f', 6)
                       .arg(badParams.at(i).b, 10, 'f', 6)
                       .arg(badParams.at(i).c, 10, 'f', 6)
                       .arg(badParams.at(i).alpha, 10, 'f', 6)
                       .arg(badParams.at(i).beta, 10, 'f', 6)
                       .arg(badParams.at(i).gamma, 10, 'f', 6));
    }
  }
  QVERIFY2(badParams.size() == 0, "The above cells did not reduce cleanly.");
}

void XtalTest::getRandomRepresentationTest()
{
  // Seed the random number generator to ensure similar results between tests
  Common::seedMt19937Generator(0);
  std::mt19937 shuffleGenerator(0);

  // Parameters:
  // Keep this test quick.
  const int iterations = 60;
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
    lengths[0] = Common::getRandDouble();

    if (Common::getRandDouble() < 0.3333333)
      lengths[1] = lengths[0];
    else
      lengths[1] = Common::getRandDouble();
    if (Common::getRandDouble() < 0.3333333)
      lengths[2] = lengths[1];
    else
      lengths[2] = Common::getRandDouble();

    //
    // Adjust each length to be between 5->25 angstrom
    //
    lengths[0] = lengths[0] * 20 + 5;
    lengths[1] = lengths[1] * 20 + 5;
    lengths[2] = lengths[2] * 20 + 5;
    //
    // Randomize the order
    //
    std::shuffle(lengths.begin(), lengths.end(), shuffleGenerator);
    //
    //  - Now for the angles. Similarly, each may be the same as the
    //    previous, but there is also a 1/3 chance that the angle will
    //    be 90 degrees.
    //
    double rand;
    std::vector<double> angles(3);
    angles[0] = Common::getRandDouble();

    rand = Common::getRandDouble();
    if (rand < 0.33333333)
      angles[1] = angles[0];
    else if (rand < 0.6666666)
      angles[1] = 0.5; // will convert to 90
    else
      angles[1] = Common::getRandDouble(); // degrees later

    rand = Common::getRandDouble();
    if (rand < 0.33333333)
      angles[2] = angles[1];
    else if (rand < 0.6666666)
      angles[2] = 0.5; // will convert to 90
    else
      angles[2] = Common::getRandDouble(); // degrees later
    //
    // Adjust each angle to lie between 60->120 degrees
    //
    angles[0] = angles[0] * 60 + 60;
    angles[1] = angles[1] * 60 + 60;
    angles[2] = angles[2] * 60 + 60;
    //
    // Randomize the order
    //
    std::shuffle(angles.begin(), angles.end(), shuffleGenerator);
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
      unsigned short atomicNum = Common::getRandUInt() % 5 + 1;
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
      Common::message(QString("Failure on comparison %1 (false negative)").arg(i+1));
    }
    QVERIFY(match);

    // Signficantly displace an atom of nxtal and ensure that the
    // comparison fails. Displacement is ~1-4 angstrom
    Q_ASSERT(nxtal->numAtoms() > 3);
    Common::Vector3 displacement;
    displacement.x() = Common::getRandDouble() + 1.0;
    displacement.y() = Common::getRandDouble() + 1.0;
    displacement.z() = Common::getRandDouble() + 1.0;
    nxtal->atom(0).setPos(nxtal->atom(0).pos() + displacement);

    start = QTime::currentTime();
    match = (xtal == *nxtal);
    end = QTime::currentTime();
    failure_msecs += start.msecsTo(end);

    if (match) {
      // Move atom back
      nxtal->atom(0).setPos(nxtal->atom(0).pos() - displacement);
      Common::message(QString("Failure on comparison %1 (false positive)").arg(i+1));
    }
    QVERIFY(!match);

    nxtal->deleteLater();
  }

  Common::message(QString(
                  "Made 2 * %1 comparisons of %2 atom unit cells, one positive "
                  "control and one negative control for each pair of structures."
                  "\n\tSuccess time: %3 ms total (%4 ms average), Failure time: "
                  "%5 ms total (%6 ms average).")
                  .arg(iterations)
                  .arg(numAtoms)
                  .arg(success_msecs)
                  .arg(success_msecs / static_cast<double>(iterations))
                  .arg(failure_msecs)
                  .arg(failure_msecs / static_cast<double>(iterations)));
}

void XtalTest::checkInteratomicDistancesTest()
{
  using ::XtalOpt::EleScaledRadii;

  // O-O minimum pair distance = 0.66 + 0.66 = 1.32 A
  EleScaledRadii limits;
  limits.setMinRadius(8, 0.66); // O

  int atom1 = -1, atom2 = -1;
  double iad = 0.0;

  // --- Test 1: two O atoms clearly far enough apart: true ---
  {
    Xtal xtal(10.0, 10.0, 10.0, 90.0, 90.0, 90.0);
    Atoms::Atom& a = xtal.addAtom();
    a.setAtomicNumber(8);
    a.setPos(Common::Vector3(0.0, 0.0, 0.0));
    Atoms::Atom& b = xtal.addAtom();
    b.setAtomicNumber(8);
    b.setPos(Common::Vector3(2.0, 0.0, 0.0)); // 2.0 A > 1.32 A
    QVERIFY(xtal.checkInterAtomicDistancesScaled(limits, &atom1, &atom2, &iad));
    QVERIFY(atom1 == -1 && atom2 == -1);
  }

  // --- Test 2: two O atoms too close: false ---
  {
    Xtal xtal(10.0, 10.0, 10.0, 90.0, 90.0, 90.0);
    Atoms::Atom& a = xtal.addAtom();
    a.setAtomicNumber(8);
    a.setPos(Common::Vector3(0.0, 0.0, 0.0));
    Atoms::Atom& b = xtal.addAtom();
    b.setAtomicNumber(8);
    b.setPos(Common::Vector3(1.0, 0.0, 0.0)); // 1.0 A < 1.32 A
    QVERIFY(!xtal.checkInterAtomicDistancesScaled(limits, &atom1, &atom2, &iad));
    QVERIFY(atom1 >= 0 && atom2 >= 0 && atom1 != atom2);
    QVERIFY(iad < 1.32);
  }

  // --- Test 3: close contact only through periodic image: false ---
  // In a 3 A cubic cell, atoms at x=0.2 and x=2.8 are 2.6 A apart
  // directly but only 0.4 A apart through the periodic boundary.
  {
    Xtal xtal(3.0, 3.0, 3.0, 90.0, 90.0, 90.0);
    Atoms::Atom& a = xtal.addAtom();
    a.setAtomicNumber(8);
    a.setPos(Common::Vector3(0.2, 0.0, 0.0));
    Atoms::Atom& b = xtal.addAtom();
    b.setAtomicNumber(8);
    b.setPos(Common::Vector3(2.8, 0.0, 0.0)); // image distance = 3.0-2.6 = 0.4 A
    QVERIFY(!xtal.checkInterAtomicDistancesScaled(limits, &atom1, &atom2, &iad));
    QVERIFY(iad < 1.32);
  }

  // --- Test 4: single atom: no pairs, always passes ---
  {
    Xtal xtal(10.0, 10.0, 10.0, 90.0, 90.0, 90.0);
    Atoms::Atom& a = xtal.addAtom();
    a.setAtomicNumber(8);
    a.setPos(Common::Vector3(0.0, 0.0, 0.0));
    QVERIFY(xtal.checkInterAtomicDistancesScaled(limits, &atom1, &atom2, &iad));
  }
}

void XtalTest::moveAtomRandomlyChecksDistancesAgainstEachAtom()
{
  using ::XtalOpt::EleScaledRadii;

  EleScaledRadii limits;
  limits.setMinRadius(8, 1.0);

  Xtal xtal(1.0, 1.0, 1.0, 90.0, 90.0, 90.0);
  Atoms::Atom& movingAtom = xtal.addAtom();
  movingAtom.setAtomicNumber(8);
  movingAtom.setPos(Common::Vector3(0.0, 0.0, 0.0));

  Atoms::Atom& fixedAtom = xtal.addAtom();
  fixedAtom.setAtomicNumber(8);
  fixedAtom.setPos(Common::Vector3(0.5, 0.5, 0.5));

  // Check a short periodic cell (moving atom 0 must fail!).
  QVERIFY(!xtal.moveAtomRandomlyScaledIAD(8, limits, 4, &xtal.atom(0)));
}

void XtalTest::writeReadSettingsPreservesCompositionValidity()
{
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  const QString stateFile = Common::localPath(tempDir.path(), "xtalopt.state");
  const QString structureDir = Common::localPath(tempDir.path(), "00001x00001");
  QVERIFY(QDir().mkpath(structureDir));

  ::XtalOpt::XtalOpt producer;
  producer.setLocWorkDir(tempDir.path());
  producer.setInputFormulasString("O1");
  QVERIFY(producer.processInputChemicalFormulas(producer.getInputFormulasString()));
  QVERIFY(producer.setQueueInterface(0, "none"));
  QVERIFY(producer.setOptimizer(0, "gulp"));

  Xtal* source = new Xtal(4.0, 4.0, 4.0, 90.0, 90.0, 90.0);
  source->setGeneration(1);
  source->setIDNumber(1);
  source->setLocpath(structureDir);
  source->setStatus(Structure::Optimized);
  source->addAtom(8, Common::Vector3(0.0, 0.0, 0.0));
  source->setCompositionValidity(false);
  QVERIFY(producer.tracker()->append(source));
  QVERIFY(producer.saveSessionState(stateFile, false));

  ::XtalOpt::XtalOpt loaded;
  loaded.setRunMode(::XtalOpt::XtalOpt::RunModeReadOnly);
  QVERIFY(loaded.resumeSearch(stateFile));
  QCOMPARE(loaded.tracker()->size(), 1);
  Xtal* loadedStructure = static_cast<Xtal*>(loaded.tracker()->at(0));
  QVERIFY(loadedStructure);

  QVERIFY(!loadedStructure->hasValidComposition());
}

void XtalTest::resultsEntryUsesComputedSpaceGroup()
{
  Xtal xtal(5.0, 5.0, 5.0, 90.0, 90.0, 90.0);
  xtal.addAtom(14, Common::Vector3(0.0, 0.0, 0.0));
  xtal.findSpaceGroup();

  const QString entry = xtal.getResultsEntry(0, 0, 0);
  QVERIFY(entry.contains("   221 "));
}

void XtalTest::hullFormattingTest()
{
  Xtal xtal(5.0, 5.0, 5.0, 90.0, 90.0, 90.0);

  Atoms::Atom& ti = xtal.addAtom();
  ti.setAtomicNumber(22);
  ti.setPos(Common::Vector3(0.0, 0.0, 0.0));

  Atoms::Atom& o1 = xtal.addAtom();
  o1.setAtomicNumber(8);
  o1.setPos(Common::Vector3(1.0, 0.0, 0.0));

  Atoms::Atom& o2 = xtal.addAtom();
  o2.setAtomicNumber(8);
  o2.setPos(Common::Vector3(0.0, 1.0, 0.0));

  xtal.setGeneration(2);
  xtal.setIDNumber(7);
  xtal.setIndex(11);
  xtal.setEnthalpy(-12.5);
  xtal.setParetoFront(3);
  xtal.setDistAboveHull(0.125);

  QList<QString> chemSystem;
  chemSystem << "Ti" << "O";

  const QString header = Xtal::getHullHeader(chemSystem);
  const QString entry = xtal.getHullEntry(chemSystem);

  QVERIFY(header.contains("AboveHullAtm"));
  QVERIFY(header.contains("Pareto"));
  QVERIFY(header.contains("Index"));

  const QStringList entryFields = entry.simplified().split(' ');
  QCOMPARE(entryFields.size(), 8);
  QCOMPARE(entryFields[0], QString("1"));
  QCOMPARE(entryFields[1], QString("2"));
  QCOMPARE(entryFields[2], QString("-12.500000"));
  QCOMPARE(entryFields[3], QString("#"));
  QCOMPARE(entryFields[4], QString("0.125000"));
  QCOMPARE(entryFields[5], QString("3"));
  QCOMPARE(entryFields[6], QString("11"));
  QCOMPARE(entryFields[7], QString("2x7"));
}

QTEST_MAIN(XtalTest)

#include "xtaltest.moc"

/**********************************************************************
  XtalTest -- Unit testing for XtalOpt::Xtal class

  Copyright (C) 2010 David C. Lonie

  XtalOpt is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
 **********************************************************************/

#include <xtalopt/structures/xtal.h>

#include <globalsearch/macros.h>

#include <Eigen/Geometry>

#include <QtCore/QDebug>
#include <QtCore/QString>
#include <QtTest/QtTest>

#define ASSIGN_PARAMS(a_,b_,c_,alpha_,beta_,gamma_)     \
  a = a_;                                               \
  b = b_;                                               \
  c = c_;                                               \
  alpha = alpha_;                                       \
  beta  = beta_;                                        \
  gamma = gamma_;

#define ROUGH_EQ(v1, v2) (fabs((v1)-(v2)) < 1e-3)
#define VERIFY_PARAMS(a_,b_,c_,alpha_,beta_,gamma_)     \
  QVERIFY(ROUGH_EQ(a_, a));                             \
  QVERIFY(ROUGH_EQ(b_, b));                             \
  QVERIFY(ROUGH_EQ(c_, c));                             \
  QVERIFY(ROUGH_EQ(alpha_, alpha));                     \
  QVERIFY(ROUGH_EQ(beta_,  beta));                      \
  QVERIFY(ROUGH_EQ(gamma_, gamma));

using namespace XtalOpt;
using namespace GlobalSearch;
using namespace Avogadro;

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
  void compareCoordinatesTest_simple();
  void compareCoordinatesTest_shifted();
  void compareCoordinatesTest_huge();
  void equalityOperatorTest_simple();
  void equalityOperatorTest_shifted();
  void equalityOperatorTest_huge();
  void niggliReduceTest();
  void fixAnglesTest();
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


void XtalTest::compareCoordinatesTest_simple()
{
  Xtal xtal1, xtal2;
  Atom *atm;

  xtal1 = Xtal (2, 2, 2, 90, 90, 90);
  atm = xtal1.addAtom();
  atm->setPos(Eigen::Vector3d(0,0,0));
  xtal2 = xtal1;
  QVERIFY(xtal1.compareCoordinates(xtal2));
}

void XtalTest::compareCoordinatesTest_shifted()
{
  Xtal xtal1, xtal2;
  Atom *atm;

  xtal1 = Xtal (2, 2, 2, 90, 90, 90);
  atm = xtal1.addAtom();
  atm->setPos(Eigen::Vector3d(1,1,1));
  xtal2 = xtal1;
  QVERIFY(xtal1.compareCoordinates(xtal2));
}

void XtalTest::compareCoordinatesTest_huge()
{
  Xtal xtal1, xtal2;
  Atom *atm;

  xtal1.setCellInfo(20,30,30,60,75.5225,70.5288);

  for (double x = 0.0; x < 1.0; x += 0.1) {
    for (double y = 0.0; y < 1.0; y += 0.1) {
      for (double z = 0.0; z < 1.0; z += 0.1) {
        atm = xtal1.addAtom();
        atm->setAtomicNumber(1);
        atm->setPos(xtal1.fracToCart(Eigen::Vector3d(x,y,z)));
        atm->setAtomicNumber(static_cast<int>(10*(x + y + z)) % 3);
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
  Atom *atm;

  xtal1 = Xtal (2, 2, 2, 90, 90, 90);
  atm = xtal1.addAtom();
  atm->setPos(Eigen::Vector3d(0,0,0));
  xtal2 = xtal1;
  QVERIFY(xtal1 == xtal2);
  // Change cell size and retest
  xtal2.setCellInfo(4, 4, 4, 90, 90, 90);
  QVERIFY(xtal1 != xtal2);
}

void XtalTest::equalityOperatorTest_shifted()
{
  Xtal xtal1, xtal2;
  Atom *atm;

  xtal1 = Xtal (2, 2, 2, 90, 90, 90);
  atm = xtal1.addAtom();
  atm->setPos(Eigen::Vector3d(1,1,1));
  xtal2 = xtal1;
  QVERIFY(xtal1 == xtal2);
  // Change cell size and retest
  xtal2.setCellInfo(4, 4, 4, 90, 90, 90);
  QVERIFY(xtal1 != xtal2);
}

void XtalTest::equalityOperatorTest_huge()
{
  Xtal xtal1, xtal2;
  Atom *atm;

  xtal1.setCellInfo(20,30,30,60,75.5225,70.5288);

  for (double x = 0.0; x < 1.0; x += 0.1) {
    for (double y = 0.0; y < 1.0; y += 0.1) {
      for (double z = 0.0; z < 1.0; z += 0.1) {
        atm = xtal1.addAtom();
        atm->setAtomicNumber(1);
        atm->setPos(xtal1.fracToCart(Eigen::Vector3d(x,y,z)));
        atm->setAtomicNumber(static_cast<int>(10*(x + y + z)) % 3);
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
  double a,b,c,alpha,beta,gamma;

  // Test from Gruber-Krivy 1976
  ASSIGN_PARAMS(3.000,
                5.19615242271,
                2.00,
                103.919748556,
                109.471220635,
                134.882107117);
  QVERIFY(Xtal::niggliReduce(&a, &b, &c, &alpha, &beta, &gamma));
  VERIFY_PARAMS(2.0, 3.0, 3.0,
                60.0, 75.5225, 70.5288);

  // Test from Gruber 1973
  ASSIGN_PARAMS(2.000,
                11.6619037897,
                8.71779788708,
                139.667309857,
                152.746099475,
                019.396625679);
  QVERIFY(Xtal::niggliReduce(&a, &b, &c, &alpha, &beta, &gamma));
  VERIFY_PARAMS(2.0, 4.0, 4.0,
                60.0, 79.1931, 75.5225);

  // This currently fails
  ASSIGN_PARAMS(5.33246,
                7.54122,
                7.64391,
                75.7212,
                110.414,
                44.9999);
  QEXPECT_FAIL("", "This fails the Niggli reduction.", Continue);
  QVERIFY(Xtal::niggliReduce(&a, &b, &c, &alpha, &beta, &gamma));

  // Random test
  const double minLength = 10.0;
  const double maxLength = 30.0;
  const double minAngle  = 45.0;
  const double maxAngle  = 135.0;
  OpenBabel::matrix3x3 tmp;
  for (unsigned int i = 0; i < 1000; i++) {
    ASSIGN_PARAMS(RANDDOUBLE() * (maxLength - minLength) + minLength,
                  RANDDOUBLE() * (maxLength - minLength) + minLength,
                  RANDDOUBLE() * (maxLength - minLength) + minLength,
                  RANDDOUBLE() * (maxAngle - minAngle) + minAngle,
                  RANDDOUBLE() * (maxAngle - minAngle) + minAngle,
                  RANDDOUBLE() * (maxAngle - minAngle) + minAngle);
    // is the cell valid?
    tmp.FillOrth(alpha, beta, gamma, a, b, c);
    if (tmp.determinant() <= 0 || GS_IS_NAN_OR_INF(tmp.determinant())) {
      i--;
      continue;
    }
    QVERIFY2(Xtal::niggliReduce(&a, &b, &c, &alpha, &beta, &gamma),
             QString("Unable to reduce cell. Params: %1 %2 %3 %4 %5 %6")
             .arg(a)
             .arg(b)
             .arg(c)
             .arg(alpha)
             .arg(beta)
             .arg(gamma).toStdString().c_str());
    QVERIFY2(Xtal::isNiggliReduced(a, b, c, alpha, beta, gamma),
             QString("Cell did not reduced to niggli cell. Final params: %1 %2 %3 %4 %5 %6")
             .arg(a)
             .arg(b)
             .arg(c)
             .arg(alpha)
             .arg(beta)
             .arg(gamma).toStdString().c_str());
  }
}

struct CellParam
{
  CellParam(const double & a_,
            const double & b_,
            const double & c_,
            const double & alpha_,
            const double & beta_,
            const double & gamma_) {
    a = a_; b = b_; c = c_;
    alpha = alpha_; beta = beta_; gamma = gamma_;
  };
  double a,b,c,alpha,beta,gamma;
};

void XtalTest::fixAnglesTest()
{
  double a,b,c,alpha,beta,gamma;
  Xtal xtal;
  const double minLength = 10.0;
  const double maxLength = 30.0;
  const double minAngle  = 45.0;
  const double maxAngle  = 135.0;
  QList<CellParam> badParams;
  OpenBabel::matrix3x3 tmp;
  Eigen::Vector3d axis;
  Eigen::Matrix3d mat;
  for (unsigned int iter = 0; iter < 100; iter++) {
    ASSIGN_PARAMS(RANDDOUBLE() * (maxLength - minLength) + minLength,
                  RANDDOUBLE() * (maxLength - minLength) + minLength,
                  RANDDOUBLE() * (maxLength - minLength) + minLength,
                  RANDDOUBLE() * (maxAngle - minAngle) + minAngle,
                  RANDDOUBLE() * (maxAngle - minAngle) + minAngle,
                  RANDDOUBLE() * (maxAngle - minAngle) + minAngle);

    // is the cell valid?
    tmp.FillOrth(alpha, beta, gamma, a, b, c);
    if (tmp.determinant() <= 0 || GS_IS_NAN_OR_INF(tmp.determinant())) {
      iter--;
      continue;
    }

    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        mat(i,j) = tmp.Get(i,j);
      }
    }

    // Create random rotation matrix
    Eigen::AngleAxis<double> t (RANDDOUBLE() * 360.0,
                                axis.setRandom());

    // std::cout << "Axis:\n" << t.axis() << "\n angle: " << t.angle() << std::endl;

    // std::cout << "Old matrix:" << std::endl << mat << std::endl;

    // std::cout << "Rot matrix:" << std::endl << t.toRotationMatrix() << std::endl;

    // Rotate cell
    mat = t * mat;
    // std::cout << "New matrix:" << std::endl << mat << std::endl;
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        tmp.Set(i,j,mat(i,j));
      }
    }

    // Update cell
    xtal.setCellInfo(tmp);

    // Add some atoms
    Atom *atm;
    for (int i = 0; i < RANDUINT() % 100; i++) {
      atm = xtal.addAtom();
      atm->setPos(xtal.fracToCart(Eigen::Vector3d(RANDDOUBLE(),
                                                  RANDDOUBLE(),
                                                  RANDDOUBLE())));
      atm->setAtomicNumber(RANDUINT() % 5);
    }
    if (!xtal.fixAngles()) {
      badParams.push_back(CellParam(a,b,c,alpha,beta,gamma));
      iter--;
      continue;
    }
    QVERIFY(xtal.isNiggliReduced());
  }

  if (badParams.size() != 0) {
    printf("%5s %10s %10s %10s %10s %10s %10s\n",
           "num", "a", "b", "c", "alpha", "beta", "gamma");
    for (int i = 0; i < badParams.size(); i++) {
      printf("%5s %10s %10s %10s %10s %10s %10s\n",
             i+1,
             badParams.at(i).a,
             badParams.at(i).b,
             badParams.at(i).c,
             badParams.at(i).alpha,
             badParams.at(i).beta,
             badParams.at(i).gamma);
    }
  }
  QVERIFY2(badParams.size() == 0,
           "The above cells did not reduce cleanly.");

}


QTEST_MAIN(XtalTest)

#include "moc_xtaltest.cxx"

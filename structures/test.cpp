
#include "molecule.h"

#include <iostream>

bool fuzzyCompare(double a, double b)
{
  return std::fabs(a - b) < 1e-4;
}

bool fuzzyCompare(const Vector3& v1, const Vector3& v2)
{
  return (fuzzyCompare(v1[0], v2[0]) &&
          fuzzyCompare(v1[1], v2[1]) &&
          fuzzyCompare(v1[2], v2[2]));
}

bool fuzzyCompare(const Matrix3& m1, const Matrix3& m2)
{
  return (fuzzyCompare(Vector3(m1.col(0)), Vector3(m2.col(0))) &&
          fuzzyCompare(Vector3(m1.col(1)), Vector3(m2.col(1))) &&
          fuzzyCompare(Vector3(m1.col(2)), Vector3(m2.col(2))));
}

using namespace XtalOpt;

int main()
{
  // SET CELL PARAMETERS TEST
  UnitCell uc(1.4, 2.1, 1.45, 75.26, 42.8, 39.06);

  Matrix3 neededMat;
  neededMat.col(0) = Vector3(1.40, 0.0, 0.0);
  neededMat.col(1) = Vector3(1.63062, 1.32328, 0.0);
  neededMat.col(2) = Vector3(1.06391, -0.72553, 0.66649);

  if (!fuzzyCompare(uc.cellMatrix(), neededMat)) {
    std::cout << "SetCellParameters Test: FAILED\n";
    std::cout << "Needed matrix: " << neededMat << "\n";
    std::cout << "Actual matrix: " << uc.cellMatrix() << "\n";
  }
  else {
    std::cout << "SetCellParameters Test: PASSED\n";
  }

  // VOLUME TEST 1
  double neededVolume = 1.23473;
  if (!fuzzyCompare(uc.volume(), neededVolume)) {
    std::cout << "Volume Test1: FAILED\n";
    std::cout << "Needed volume: " << neededVolume << "\n";
    std::cout << "Actual volume: " << uc.volume() << "\n";
  }
  else {
    std::cout << "Volume Test1: PASSED\n";
  }

  // VOLUME TEST 2
  uc.setAVector(Vector3(1.2, 3.4, 2.1));
  uc.setBVector(Vector3(1.4, 2.0, 1.8));
  uc.setCVector(Vector3(3.1, 0.9, 2.2));
  neededVolume = 1.462;

  if (!fuzzyCompare(uc.volume(), neededVolume)) {
    std::cout << "Volume Test2: FAILED\n";
    std::cout << "Needed volume: " << neededVolume << "\n";
    std::cout << "Actual volume: " << uc.volume() << "\n";
  }
  else {
    std::cout << "Volume Test2: PASSED\n";
  }

  // SET VECTOR TEST
  neededMat.col(0) = Vector3(1.2, 3.4, 2.1);
  neededMat.col(1) = Vector3(1.4, 2.0, 1.8);
  neededMat.col(2) = Vector3(3.1, 0.9, 2.2);
  if (!fuzzyCompare(uc.cellMatrix(), neededMat)) {
    std::cout << "Set Vector Test: FAILED\n";
    std::cout << "Needed matrix: " << neededMat << "\n";
    std::cout << "Actual matrix: " << uc.cellMatrix() << "\n";
  }
  else {
    std::cout << "Set Vector Test: PASSED\n";
  }

  // CALCULATE CELL PARAMS TEST
  double a = 4.17253;
  double b = 3.03315;
  double c = 3.90640;
  double alpha = 31.52487;
  double beta = 45.62078;
  double gamma = 14.36900;

  if (!fuzzyCompare(uc.a(), a) || !fuzzyCompare(uc.b(), b) ||
      !fuzzyCompare(uc.c(), c) || !fuzzyCompare(uc.alpha(), alpha) ||
      !fuzzyCompare(uc.beta(), beta) || !fuzzyCompare(uc.gamma(), gamma)) {
    std::cout << "Calculate Cell Params Test: FAILED\n";
    std::cout << "Needed Cell Params:\n"
              << "a: " << a << "\n"
              << "b: " << b << "\n"
              << "c: " << c << "\n"
              << "alpha: " << alpha << "\n"
              << "beta: " << beta << "\n"
              << "gamma: " << gamma << "\n";
    std::cout << "Actual Cell Params:\n"
              << "a: " << uc.a() << "\n"
              << "b: " << uc.b() << "\n"
              << "c: " << uc.c() << "\n"
              << "alpha: " << uc.alpha() << "\n"
              << "beta: " << uc.beta() << "\n"
              << "gamma: " << uc.gamma() << "\n";
  }
  else {
    std::cout << "Calculate Cell Params Test: PASSED\n";
  }

  // TO CARTESIAN TEST
  Vector3 frac(0.1, 0.6, 0.2);
  Vector3 cart(1.58, 1.72, 1.73);
  if (!fuzzyCompare(uc.toCartesian(frac), cart)) {
    std::cout << "To Cartesian Test: FAILED\n";
    std::cout << "Needed Vector: " << cart << "\n";
    std::cout << "Actual Vector: " << uc.toCartesian(frac) << "\n";
  }
  else {
    std::cout << "To Cartesian Test: PASSED\n";
  }

  // TO FRACTIONAL TEST
  if (!fuzzyCompare(uc.toFractional(cart), frac)) {
    std::cout << "To Fractional Test: FAILED\n";
    std::cout << "Needed Vector: " << frac << "\n";
    std::cout << "Actual Vector: " << uc.toFractional(cart) << "\n";
  }
  else {
    std::cout << "To Fractional Test: PASSED\n";
  }

  // WRAP ATOMS TEST
  cart = Vector3(-1.1, 10.11, 1.7);
  Vector3 wrapped(1.8, 2.61, 2.2);
  if (!fuzzyCompare(uc.wrapCartesian(cart), wrapped)) {
    std::cout << "Wrap Atoms Test: FAILED\n";
    std::cout << "Needed Vector: " << wrapped << "\n";
    std::cout << "Actual Vector: " << uc.wrapCartesian(cart) << "\n";
  }
  else {
    std::cout << "Wrap Atoms Test: PASSED\n";
  }

  // DISTANCE TEST
  double dist = 1.78885532517;
  uc.setCellParameters(8.0, 8.0, 8.0, 60.0, 60.0, 60.0);
  Vector3 v1(9.2, 2.54034, 1.30639);
  Vector3 v2(2.8, 2.07846, 1.95959);
  if (!fuzzyCompare(uc.distance(v1, v2), dist)) {
    std::cout << "Distance Test: FAILED\n";
    std::cout << "Needed Distance: " << dist << "\n";
    std::cout << "Actual Distance: " << uc.distance(v1, v2) << "\n";
  }
  else {
    std::cout << "Distance Test: PASSED\n";
  }

}

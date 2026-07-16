/**********************************************************************
  Matrix3 - Simple fixed-size 3x3 matrix utilities.

  Copyright (C) 2016 by Patrick S. Avery
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef COMMON_MATRIX3_H
#define COMMON_MATRIX3_H

#include <common/constants.h>
#include <common/vector.h>
#include <common/random.h>

#include <cmath>
#include <cstddef>
#include <ostream>
#include <vector>

namespace Common {

// A fixed-size 3x3 matrix of doubles with a set of standard arithmetic
//   operations. Used for unit cells and transformations.
// This has no external math library dependency.
class Matrix3
{
public:
  class CommaInitializer
  {
  public:
    CommaInitializer(Matrix3& matrix, double value) : m_matrix(matrix), m_index(1)
    {
      m_matrix(0, 0) = value;
    }

    CommaInitializer& operator,(double value)
    {
      if (m_index < 9) {
        m_matrix(m_index / 3, m_index % 3) = value;
        ++m_index;
      }
      return *this;
    }

  private:
    Matrix3& m_matrix;
    std::size_t m_index;
  };

  Matrix3()
    : m_data{{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}}
  {
  }

  static Matrix3 Zero() { return Matrix3(); }

  static Matrix3 Identity()
  {
    Matrix3 matrix;
    matrix(0, 0) = 1.0;
    matrix(1, 1) = 1.0;
    matrix(2, 2) = 1.0;
    return matrix;
  }

  double& operator()(std::size_t row, std::size_t column)
  {
    return m_data[row][column];
  }

  const double& operator()(std::size_t row, std::size_t column) const
  {
    return m_data[row][column];
  }

  CommaInitializer operator<<(double value) { return CommaInitializer(*this, value); }

  Vector3 row(std::size_t rowIndex) const
  {
    return Vector3(m_data[rowIndex][0], m_data[rowIndex][1], m_data[rowIndex][2]);
  }

  Vector3 col(std::size_t colIndex) const
  {
    return Vector3(m_data[0][colIndex], m_data[1][colIndex], m_data[2][colIndex]);
  }

  void setRow(std::size_t rowIndex, const Vector3& rowVector)
  {
    m_data[rowIndex][0] = rowVector.x();
    m_data[rowIndex][1] = rowVector.y();
    m_data[rowIndex][2] = rowVector.z();
  }

  void setCol(std::size_t colIndex, const Vector3& colVector)
  {
    m_data[0][colIndex] = colVector.x();
    m_data[1][colIndex] = colVector.y();
    m_data[2][colIndex] = colVector.z();
  }

  Matrix3 transpose() const
  {
    Matrix3 matrix;
    for (std::size_t rowIndex = 0; rowIndex < 3; ++rowIndex) {
      for (std::size_t colIndex = 0; colIndex < 3; ++colIndex) {
        matrix(colIndex, rowIndex) = (*this)(rowIndex, colIndex);
      }
    }
    return matrix;
  }

  double determinant() const
  {
    return (*this)(0, 0) * ((*this)(1, 1) * (*this)(2, 2) - (*this)(1, 2) * (*this)(2, 1)) -
           (*this)(0, 1) * ((*this)(1, 0) * (*this)(2, 2) - (*this)(1, 2) * (*this)(2, 0)) +
           (*this)(0, 2) * ((*this)(1, 0) * (*this)(2, 1) - (*this)(1, 1) * (*this)(2, 0));
  }

  Matrix3 inverse() const
  {
    const double det = determinant();
    Matrix3 matrix;
    if (det == 0.0) {
      return matrix;
    }

    matrix(0, 0) =  ((*this)(1, 1) * (*this)(2, 2) - (*this)(1, 2) * (*this)(2, 1)) / det;
    matrix(0, 1) = -((*this)(0, 1) * (*this)(2, 2) - (*this)(0, 2) * (*this)(2, 1)) / det;
    matrix(0, 2) =  ((*this)(0, 1) * (*this)(1, 2) - (*this)(0, 2) * (*this)(1, 1)) / det;
    matrix(1, 0) = -((*this)(1, 0) * (*this)(2, 2) - (*this)(1, 2) * (*this)(2, 0)) / det;
    matrix(1, 1) =  ((*this)(0, 0) * (*this)(2, 2) - (*this)(0, 2) * (*this)(2, 0)) / det;
    matrix(1, 2) = -((*this)(0, 0) * (*this)(1, 2) - (*this)(0, 2) * (*this)(1, 0)) / det;
    matrix(2, 0) =  ((*this)(1, 0) * (*this)(2, 1) - (*this)(1, 1) * (*this)(2, 0)) / det;
    matrix(2, 1) = -((*this)(0, 0) * (*this)(2, 1) - (*this)(0, 1) * (*this)(2, 0)) / det;
    matrix(2, 2) =  ((*this)(0, 0) * (*this)(1, 1) - (*this)(0, 1) * (*this)(1, 0)) / det;
    return matrix;
  }

  bool isZero() const
  {
    for (std::size_t rowIndex = 0; rowIndex < 3; ++rowIndex) {
      for (std::size_t colIndex = 0; colIndex < 3; ++colIndex) {
        if ((*this)(rowIndex, colIndex) != 0.0) {
          return false;
        }
      }
    }
    return true;
  }

  Matrix3& operator+=(const Matrix3& other)
  {
    for (std::size_t rowIndex = 0; rowIndex < 3; ++rowIndex) {
      for (std::size_t colIndex = 0; colIndex < 3; ++colIndex) {
        (*this)(rowIndex, colIndex) += other(rowIndex, colIndex);
      }
    }
    return *this;
  }

  Matrix3& operator-=(const Matrix3& other)
  {
    for (std::size_t rowIndex = 0; rowIndex < 3; ++rowIndex) {
      for (std::size_t colIndex = 0; colIndex < 3; ++colIndex) {
        (*this)(rowIndex, colIndex) -= other(rowIndex, colIndex);
      }
    }
    return *this;
  }

  Matrix3& operator*=(double scalar)
  {
    for (std::size_t rowIndex = 0; rowIndex < 3; ++rowIndex) {
      for (std::size_t colIndex = 0; colIndex < 3; ++colIndex) {
        (*this)(rowIndex, colIndex) *= scalar;
      }
    }
    return *this;
  }

  Matrix3& operator/=(double scalar)
  {
    for (std::size_t rowIndex = 0; rowIndex < 3; ++rowIndex) {
      for (std::size_t colIndex = 0; colIndex < 3; ++colIndex) {
        (*this)(rowIndex, colIndex) /= scalar;
      }
    }
    return *this;
  }

  Matrix3& operator*=(const Matrix3& other)
  {
    Matrix3 result;
    for (std::size_t rowIndex = 0; rowIndex < 3; ++rowIndex) {
      for (std::size_t colIndex = 0; colIndex < 3; ++colIndex) {
        for (std::size_t innerIndex = 0; innerIndex < 3; ++innerIndex) {
          result(rowIndex, colIndex) += (*this)(rowIndex, innerIndex) * other(innerIndex, colIndex);
        }
      }
    }
    *this = result;
    return *this;
  }

  bool operator==(const Matrix3& other) const
  {
    for (std::size_t rowIndex = 0; rowIndex < 3; ++rowIndex) {
      for (std::size_t colIndex = 0; colIndex < 3; ++colIndex) {
        if ((*this)(rowIndex, colIndex) != other(rowIndex, colIndex)) {
          return false;
        }
      }
    }
    return true;
  }

  bool operator!=(const Matrix3& other) const { return !(*this == other); }

private:
  double m_data[3][3];
};

inline Matrix3 operator+(Matrix3 lhs, const Matrix3& rhs)
{
  lhs += rhs;
  return lhs;
}

inline Matrix3 operator-(Matrix3 lhs, const Matrix3& rhs)
{
  lhs -= rhs;
  return lhs;
}

inline Matrix3 operator*(Matrix3 matrix, double scalar)
{
  matrix *= scalar;
  return matrix;
}

inline Matrix3 operator*(double scalar, Matrix3 matrix)
{
  matrix *= scalar;
  return matrix;
}

inline Matrix3 operator/(Matrix3 matrix, double scalar)
{
  matrix /= scalar;
  return matrix;
}

inline Matrix3 operator*(const Matrix3& lhs, const Matrix3& rhs)
{
  Matrix3 matrix;
  for (std::size_t rowIndex = 0; rowIndex < 3; ++rowIndex) {
    for (std::size_t colIndex = 0; colIndex < 3; ++colIndex) {
      for (std::size_t innerIndex = 0; innerIndex < 3; ++innerIndex) {
        matrix(rowIndex, colIndex) += lhs(rowIndex, innerIndex) * rhs(innerIndex, colIndex);
      }
    }
  }
  return matrix;
}

inline Vector3 operator*(const Matrix3& matrix, const Vector3& vector)
{
  return Vector3(matrix.row(0).dot(vector), matrix.row(1).dot(vector), matrix.row(2).dot(vector));
}

inline Matrix3 rotationAroundX(double alpha)
{
  const double c = std::cos(alpha);
  const double s = std::sin(alpha);
  Matrix3 matrix;
  matrix << 1.0, 0.0, 0.0,
            0.0, c, -s,
            0.0, s, c;
  return matrix;
}

inline Matrix3 rotationAroundY(double beta)
{
  const double c = std::cos(beta);
  const double s = std::sin(beta);
  Matrix3 matrix;
  matrix << c, 0.0, s,
            0.0, 1.0, 0.0,
            -s, 0.0, c;
  return matrix;
}

inline Matrix3 rotationAroundZ(double gamma)
{
  const double c = std::cos(gamma);
  const double s = std::sin(gamma);
  Matrix3 matrix;
  matrix << c, -s, 0.0,
            s, c, 0.0,
            0.0, 0.0, 1.0;
  return matrix;
}

inline Matrix3 rotationMatrixEuler(double alpha, double beta, double gamma)
{
  // Make a right-handed XYZ rotation matrix from Euler angles.
  // Conventions: alpha=X, beta=Y, gamma=Z; right-handed with R = Rz*Ry*Rx
  // Other conventions can be obtained by multiplication order below.
  return rotationAroundZ(gamma) * rotationAroundY(beta) * rotationAroundX(alpha);
}

inline Matrix3 rotationMatrixRandom()
{
  // Make a uniformly random rotation matrix (Shoemake).
  const double pi = std::acos(-1.0);
  const double u1 = Common::getRandDouble();
  const double u2 = Common::getRandDouble();
  const double u3 = Common::getRandDouble();

  // Make a random unit quaternion.
  const double x = std::sqrt(1.0 - u1) * std::sin(2.0 * pi * u2);
  const double y = std::sqrt(1.0 - u1) * std::cos(2.0 * pi * u2);
  const double z = std::sqrt(u1) * std::sin(2.0 * pi * u3);
  const double w = std::sqrt(u1) * std::cos(2.0 * pi * u3);

  Matrix3 matrix;
  matrix << 1.0 - 2.0 * (y * y + z * z),
            2.0 * (x * y - w * z),
            2.0 * (x * z + w * y),
            2.0 * (x * y + w * z),
            1.0 - 2.0 * (x * x + z * z),
            2.0 * (y * z - w * x),
            2.0 * (x * z - w * y),
            2.0 * (y * z + w * x),
            1.0 - 2.0 * (x * x + y * y);
  return matrix;
}

inline Vector3 rotatePoint(const Matrix3& rotation, const Vector3& point)
{
  Vector3 rotated = rotation * point;
  return rotated;
}

inline std::ostream& operator<<(std::ostream& out, const Matrix3& matrix)
{
  out << matrix(0, 0) << " " << matrix(0, 1) << " " << matrix(0, 2) << "\n"
      << matrix(1, 0) << " " << matrix(1, 1) << " " << matrix(1, 2) << "\n"
      << matrix(2, 0) << " " << matrix(2, 1) << " " << matrix(2, 2);
  return out;
}

inline bool fuzzyCompare(const Matrix3& v1, const Matrix3& v2, double tol = ZERO08)
{
  return fuzzyCompare(v1.row(0), v2.row(0), tol) && fuzzyCompare(v1.row(1), v2.row(1), tol) &&
         fuzzyCompare(v1.row(2), v2.row(2), tol);
}

} // namespace Common

#endif

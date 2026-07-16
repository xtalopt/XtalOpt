/**********************************************************************
  Vector3 - Simple fixed-size 3D vector utilities.

  Copyright (C) 2016 by Patrick S. Avery
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef COMMON_VECTOR3_H
#define COMMON_VECTOR3_H

#include <cmath>
#include <cstddef>
#include <ostream>

#include <common/constants.h>
#include <common/numericutils.h>

namespace Common {

// A 3-component vector of doubles with a set of standard arithmetic
//   operations. Used for atom positions and cell vectors.
class Vector3
{
public:
  class CommaInitializer
  {
  public:
    CommaInitializer(Vector3& vector, double value) : m_vector(vector), m_index(1)
    {
      m_vector[0] = value;
    }

    CommaInitializer& operator,(double value)
    {
      if (m_index < 3) {
        m_vector[m_index++] = value;
      }
      return *this;
    }

  private:
    Vector3& m_vector;
    std::size_t m_index;
  };

  Vector3() : m_data{0.0, 0.0, 0.0} {}
  Vector3(double x, double y, double z) : m_data{x, y, z} {}
  explicit Vector3(const double* values)
    : m_data{values[0], values[1], values[2]}
  {
  }

  static Vector3 Zero() { return Vector3(0.0, 0.0, 0.0); }

  double& operator[](std::size_t index) { return m_data[index]; }
  const double& operator[](std::size_t index) const { return m_data[index]; }

  double& x() { return m_data[0]; }
  double& y() { return m_data[1]; }
  double& z() { return m_data[2]; }

  double x() const { return m_data[0]; }
  double y() const { return m_data[1]; }
  double z() const { return m_data[2]; }

  CommaInitializer operator<<(double value) { return CommaInitializer(*this, value); }

  Vector3 operator-() const { return Vector3(-x(), -y(), -z()); }

  Vector3& operator+=(const Vector3& other)
  {
    x() += other.x();
    y() += other.y();
    z() += other.z();
    return *this;
  }

  Vector3& operator-=(const Vector3& other)
  {
    x() -= other.x();
    y() -= other.y();
    z() -= other.z();
    return *this;
  }

  Vector3& operator*=(double scalar)
  {
    x() *= scalar;
    y() *= scalar;
    z() *= scalar;
    return *this;
  }

  Vector3& operator/=(double scalar)
  {
    x() /= scalar;
    y() /= scalar;
    z() /= scalar;
    return *this;
  }

  double squaredNorm() const { return dot(*this); }
  double norm() const { return std::sqrt(squaredNorm()); }

  void normalize()
  {
    const double magnitude = norm();
    if (magnitude != 0.0) {
      *this /= magnitude;
    }
  }

  Vector3 normalized() const
  {
    Vector3 copy(*this);
    copy.normalize();
    return copy;
  }

  double dot(const Vector3& other) const
  {
    return x() * other.x() + y() * other.y() + z() * other.z();
  }

  Vector3 cross(const Vector3& other) const
  {
    return Vector3(y() * other.z() - z() * other.y(), z() * other.x() - x() * other.z(),
                   x() * other.y() - y() * other.x());
  }

  bool operator==(const Vector3& other) const
  {
    return x() == other.x() && y() == other.y() && z() == other.z();
  }

  bool operator!=(const Vector3& other) const { return !(*this == other); }

private:
  double m_data[3];
};

inline Vector3 operator+(Vector3 lhs, const Vector3& rhs)
{
  lhs += rhs;
  return lhs;
}

inline Vector3 operator-(Vector3 lhs, const Vector3& rhs)
{
  lhs -= rhs;
  return lhs;
}

inline Vector3 operator*(Vector3 vector, double scalar)
{
  vector *= scalar;
  return vector;
}

inline Vector3 operator*(double scalar, Vector3 vector)
{
  vector *= scalar;
  return vector;
}

inline Vector3 operator/(Vector3 vector, double scalar)
{
  vector /= scalar;
  return vector;
}

inline std::ostream& operator<<(std::ostream& out, const Vector3& vector)
{
  out << vector.x() << " " << vector.y() << " " << vector.z();
  return out;
}

inline bool fuzzyCompare(const Vector3& v1, const Vector3& v2, double tol = ZERO08)
{
  return fuzzyCompare(v1[0], v2[0], tol) && fuzzyCompare(v1[1], v2[1], tol) &&
         fuzzyCompare(v1[2], v2[2], tol);
}

} // namespace Common

#endif

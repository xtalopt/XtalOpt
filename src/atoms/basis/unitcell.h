/**********************************************************************
  Unit Cell - A basic unit cell class.

  Copyright (C) 2016 by Patrick S. Avery
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef ATOMS_UNITCELL_H
#define ATOMS_UNITCELL_H

#include <cmath>
#include <type_traits>
#include <utility>

#include <common/constants.h>
#include <common/matrix.h>
#include <common/vector.h>

namespace Atoms {

/**
 * @class UnitCell unitcell.h
 * @brief A unit cell with per-axis periodicity and a lattice matrix.
 *
 * A default UnitCell is 0D: no periodic axes and a zero matrix. Setters that
 * take cell parameters or a full matrix make all axes periodic. The code uses
 * 0D and 3D cells now; the other dimensions are kept for later use.
 */
class UnitCell
{
public:
  /** Default constructor for a non-periodic cell. */
  UnitCell();

  /** Constructor from a lattice matrix. Makes a periodic cell. */
  UnitCell(const Common::Matrix3& m);

  /**
   * Constructor. This uses cell parameters to create the cell matrix.
   * The A vector will lie along the x-axis. The B vector will lie in
   * the positive y region of the x-y plane. The C vector will then be
   * constrained.
   *
   * @param a The length (Angstroms) of vector A.
   * @param b The length (Angstroms) of vector B.
   * @param c The length (Angstroms) of vector C.
   * @param alpha The angle (degrees) between vectors B and C.
   * @param beta The angle (degrees) between vectors A and C.
   * @param gamma The angle (degrees) between vectors A and B.
   */
  UnitCell(double a, double b, double c, double alpha, double beta, double gamma);

  /* Default destructor */
  ~UnitCell() = default;

  /* Default copy constructor */
  UnitCell(const UnitCell& other) = default;

  /**
   * Move constructor. Unfortunately, we must define it ourselves for it to
   * be noexcept.
   */
  UnitCell(UnitCell&& other) noexcept;

  /* Default assignment operator */
  UnitCell& operator=(const UnitCell& other) = default;

  /**
   * Move assignment operator. Unfortunately, we must define it ourselves
   * for it to be noexcept.
   */
  UnitCell& operator=(UnitCell&& other) noexcept;

  /**
   * Per-axis periodicity query.
   *
   * @param axis 0 = x, 1 = y, 2 = z.
   * @return true if @p axis is periodic.
   */
  bool isPeriodic(int axis) const {
    return axis >= 0 && axis < 3 && u_periodic[axis];
  }

  /**
   * @return Number of periodic axes (0 through 3).
   */
  int dimension() const {
    return (u_periodic[0] ? 1 : 0) + (u_periodic[1] ? 1 : 0) + (u_periodic[2] ? 1 : 0);
  }

  /** @return true if all three axes are periodic (3D crystal). */
  bool is3D() const { return dimension() == 3; }
  /** @return true if exactly two axes are periodic. */
  bool is2D() const { return dimension() == 2; }
  /** @return true if exactly one axis is periodic. */
  bool is1D() const { return dimension() == 1; }
  /** @return true if no axes are periodic. */
  bool is0D() const { return dimension() == 0; }

  /**
   * This uses cell parameters to create the cell matrix.
   * The A vector will lie along the x-axis. The B vector will lie in
   * the positive y region of the x-y plane. The C vector will then be
   * constrained.
   *
   * @param a The length (Angstroms) of vector A.
   * @param b The length (Angstroms) of vector B.
   * @param c The length (Angstroms) of vector C.
   * @param alpha The angle (degrees) between vectors B and C.
   * @param beta The angle (degrees) between vectors A and C.
   * @param gamma The angle (degrees) between vectors A and B.
   */
  void setCellParameters(double a, double b, double c, double alpha,
                         double beta, double gamma);

  /**
   * Set the cell matrix as row vectors. Marks all three axes periodic (3D).
   *
   * @param mat The 3x3 cell matrix to be set in row vector form.
   */
  void setCellMatrix(const Common::Matrix3& mat) {
    u_cellMatrix = mat;
    u_periodic[0] = u_periodic[1] = u_periodic[2] = true;
  };

  // Cell functions return default values for a non-periodic cell.

  /**
   * Get the cell matrix as row vectors.
   *
   * @return The 3x3 cell matrix in row vector form.
   */
  Common::Matrix3 cellMatrix() const {
    return is3D() ? u_cellMatrix : Common::Matrix3::Zero();
  };


  /**
   * Set the A vector. Marks the A axis periodic.
   *
   * @param v The vector with which to set A.
   */
  void setAVector(const Common::Vector3& v) {
    u_cellMatrix.setRow(0, v);
    u_periodic[0] = true;
  };

  /**
   * Set the B vector. Marks the B axis periodic.
   *
   * @param v The vector with which to set B.
   */
  void setBVector(const Common::Vector3& v) {
    u_cellMatrix.setRow(1, v);
    u_periodic[1] = true;
  };

  /**
   * Set the C vector. Marks the C axis periodic.
   *
   * @param v The vector with which to set C.
   */
  void setCVector(const Common::Vector3& v) {
    u_cellMatrix.setRow(2, v);
    u_periodic[2] = true;
  };

  /**
   * Set the cell vectors - a, b, and c.
   *
   * @param a The A vector.
   * @param b The B vector.
   * @param c The C vector.
   */
  void setCellVectors(const Common::Vector3& a, const Common::Vector3& b, const Common::Vector3& c);

  /* Returns the A vector */
  Common::Vector3 aVector() const {
    return is3D() ? u_cellMatrix.row(0) : Common::Vector3();
  };

  /* Returns the B vector */
  Common::Vector3 bVector() const {
    return is3D() ? u_cellMatrix.row(1) : Common::Vector3();
  };

  /* Returns the C vector */
  Common::Vector3 cVector() const {
    return is3D() ? u_cellMatrix.row(2) : Common::Vector3();
  };

  /* Returns the length of A in Angstroms */
  double a() const { return is3D() ? u_cellMatrix.row(0).norm() : 0.0; };

  /* Returns the length of B in Angstroms */
  double b() const { return is3D() ? u_cellMatrix.row(1).norm() : 0.0; };

  /* Returns the length of C in Angstroms */
  double c() const { return is3D() ? u_cellMatrix.row(2).norm() : 0.0; };

  /* Returns the angle (degrees) between B and C */
  double alpha() const { return is3D() ? angleDegrees(bVector(), cVector()) : 0.0; };

  /* Returns the angle (degrees) between A and C */
  double beta() const { return is3D() ? angleDegrees(aVector(), cVector()) : 0.0; };

  /* Returns the angle (degrees) between A and B */
  double gamma() const { return is3D() ? angleDegrees(aVector(), bVector()) : 0.0; };

  /* Returns the volume of the unit cell in Angstroms cubed */
  double volume() const;

  /**
   * Converts the @p frac vector into a Cartesian vector.
   *
   * @param cart The fractional vector to be converted.
   *
   * @return The Cartesian (Angstrom) vector.
   */
  Common::Vector3 toCartesian(const Common::Vector3& frac) const;

  /**
   * Converts the @p Cartesian vector into a fractional vector.
   *
   * @param cart The Cartesian vector (Angstroms) to be converted.
   *
   * @return The vector with fractional units.
   */
  Common::Vector3 toFractional(const Common::Vector3& cart) const;

  /**
   * Wrap Cartesian coordinates to be within the unit cell.
   *
   * @param cart The Cartesian (Angstrom) coordinates to be wrapped.
   *
   * @return The wrapped Cartesian coordinates.
   */
  Common::Vector3 wrapCartesian(const Common::Vector3& cart) const;

  /**
   * Wrap fractional coordinates to be within the unit cell.
   *
   * @param frac The fractional coordinates to be wrapped.
   *
   * @return The wrapped fractional coordinates.
   */
  Common::Vector3 wrapFractional(const Common::Vector3& frac) const;

  /**
   * Find the minimum image of a Cartesian vector @p cart.
   * A minimum image has all fractional coordinates between -0.5 and 0.5
   *
   * @param cart The Cartesian (Angstrom) vector whose minimum image we are
   *             finding.
   *
   * @return The minimum image of the Cartesian vector @p cart.
   */
  Common::Vector3 minimumImage(const Common::Vector3& cart) const;

  /**
   * Find the minimum fractional image of a fractional vector @p frac.
   * A minimum image has all fractional coordinates between -0.5 and 0.5.
   *
   * @param frac The fractional vector whose minimum image we are finding.
   *
   * @return The minimum image of the fractional vector @p frac.
   */
  static Common::Vector3 minimumImageFractional(const Common::Vector3& frac);

  /**
   * Find the shortest distance between vectors @a v1 and @a v2.
   *
   * @param v1 The first vector.
   * @param v2 The second vector.
   *
   * @return The shortest distance between the two vectors.
   */
  double distance(const Common::Vector3& v1, const Common::Vector3& v2) const;

  /**
   * Zeroes the unit cell and resets it to 0D (no axes periodic).
   */
  void clear() {
    u_cellMatrix = Common::Matrix3::Zero();
    u_periodic[0] = u_periodic[1] = u_periodic[2] = false;
  };

private:
  /**
   * Get the unsigned angle in radians between two vectors.
   *
   * @param v1 The first vector.
   * @param v2 The second vector.
   *
   * @return The unsigned angle in radians between @p v1 and @p v2.
   */
  static double angle(const Common::Vector3& v1, const Common::Vector3& v2);

  /**
   * Get the unsigned angle in degrees between two vectors.
   *
   * @param v1 The first vector.
   * @param v2 The second vector.
   *
   * @return The unsigned angle in degrees between @p v1 and @p v2.
   */
  static double angleDegrees(const Common::Vector3& v1, const Common::Vector3& v2);

  Common::Matrix3 u_cellMatrix;
  bool u_periodic[3] = {false, false, false};
};

// Make sure the move constructor is noexcept
static_assert(std::is_nothrow_move_constructible<UnitCell>::value,
              "UnitCell should be noexcept move constructible.");

// Make sure the move assignment operator is noexcept
static_assert(std::is_nothrow_move_assignable<UnitCell>::value,
              "UnitCell should be noexcept move assignable.");

inline UnitCell::UnitCell() : u_cellMatrix(Common::Matrix3::Zero())
{
}

inline UnitCell::UnitCell(const Common::Matrix3& m) : u_cellMatrix(m)
{
  u_periodic[0] = u_periodic[1] = u_periodic[2] = true;
}

inline UnitCell::UnitCell(double a, double b, double c, double alpha,
                          double beta, double gamma)
  : u_cellMatrix(Common::Matrix3())
{
  setCellParameters(a, b, c, alpha, beta, gamma);
}

inline UnitCell::UnitCell(UnitCell&& other) noexcept
  : u_cellMatrix(std::move(other.u_cellMatrix))
{
  u_periodic[0] = other.u_periodic[0];
  u_periodic[1] = other.u_periodic[1];
  u_periodic[2] = other.u_periodic[2];
}

inline UnitCell& UnitCell::operator=(UnitCell&& other) noexcept
{
  if (this != &other) {
    u_cellMatrix = std::move(other.u_cellMatrix);
    u_periodic[0] = other.u_periodic[0];
    u_periodic[1] = other.u_periodic[1];
    u_periodic[2] = other.u_periodic[2];
  }
  return *this;
}

inline void UnitCell::setCellVectors(const Common::Vector3& a, const Common::Vector3& b,
                                     const Common::Vector3& c)
{
  setAVector(a);
  setBVector(b);
  setCVector(c);
}

inline double UnitCell::volume() const
{
  return std::fabs(aVector().cross(bVector()).dot(cVector()));
}

inline Common::Vector3 UnitCell::toCartesian(const Common::Vector3& frac) const
{
  if (!is3D()) return Common::Vector3();
  return u_cellMatrix.transpose() * frac;
}

inline Common::Vector3 UnitCell::toFractional(const Common::Vector3& cart) const
{
  if (!is3D()) return Common::Vector3();
  return u_cellMatrix.transpose().inverse() * cart;
}

inline Common::Vector3 UnitCell::wrapCartesian(const Common::Vector3& cart) const
{
  return toCartesian(wrapFractional(toFractional(cart)));
}

inline Common::Vector3 UnitCell::minimumImage(const Common::Vector3& cart) const
{
  return toCartesian(minimumImageFractional(toFractional(cart)));
}

inline double UnitCell::distance(const Common::Vector3& v1, const Common::Vector3& v2) const
{
  return std::fabs(minimumImage(v1 - v2).norm());
}

inline double UnitCell::angle(const Common::Vector3& v1, const Common::Vector3& v2)
{
  return std::acos(v1.dot(v2) / (v1.norm() * v2.norm()));
}

inline double UnitCell::angleDegrees(const Common::Vector3& v1, const Common::Vector3& v2)
{
  return angle(v1, v2) * RAD2DEG;
}
} // namespace Atoms

#endif

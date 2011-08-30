/**********************************************************************
  XcMatrix - 3x3 matrix of doubles, in-line class

  Copyright (C) 2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef XCMATRIX_H
#define XCMATRIX_H

#include "xcvector.h"

class XcMatrix
{
 public:
  XcMatrix() {};

  XcMatrix(const double s)
  {
    this->arr[0][0] = s;
    this->arr[0][1] = 0.0;
    this->arr[0][2] = 0.0;
    this->arr[1][0] = 0.0;
    this->arr[1][1] = s;
    this->arr[1][2] = 0.0;
    this->arr[2][0] = 0.0;
    this->arr[2][1] = 0.0;
    this->arr[2][2] = s;
  }

  XcMatrix(const double init [3][3])
  {
    this->arr[0][0] = init[0][0];
    this->arr[0][1] = init[0][1];
    this->arr[0][2] = init[0][2];
    this->arr[1][0] = init[1][0];
    this->arr[1][1] = init[1][1];
    this->arr[1][2] = init[1][2];
    this->arr[2][0] = init[2][0];
    this->arr[2][1] = init[2][1];
    this->arr[2][2] = init[2][2];
  }

  XcMatrix(const double d00, const double d01, const double d02,
           const double d10, const double d11, const double d12,
           const double d20, const double d21, const double d22)
  {
    this->arr[0][0] = d00;
    this->arr[0][1] = d01;
    this->arr[0][2] = d02;
    this->arr[1][0] = d10;
    this->arr[1][1] = d11;
    this->arr[1][2] = d12;
    this->arr[2][0] = d20;
    this->arr[2][1] = d21;
    this->arr[2][2] = d22;
  }

  XcMatrix(const XcMatrix &other)
  {
    this->arr[0][0] = other.arr[0][0];
    this->arr[0][1] = other.arr[0][1];
    this->arr[0][2] = other.arr[0][2];
    this->arr[1][0] = other.arr[1][0];
    this->arr[1][1] = other.arr[1][1];
    this->arr[1][2] = other.arr[1][2];
    this->arr[2][0] = other.arr[2][0];
    this->arr[2][1] = other.arr[2][1];
    this->arr[2][2] = other.arr[2][2];
  }

  XcMatrix & operator=(const XcMatrix &other)
  {
    this->arr[0][0] = other.arr[0][0];
    this->arr[0][1] = other.arr[0][1];
    this->arr[0][2] = other.arr[0][2];
    this->arr[1][0] = other.arr[1][0];
    this->arr[1][1] = other.arr[1][1];
    this->arr[1][2] = other.arr[1][2];
    this->arr[2][0] = other.arr[2][0];
    this->arr[2][1] = other.arr[2][1];
    this->arr[2][2] = other.arr[2][2];
    return *this;
  }

  XcMatrix & fill(const double d00, const double d01, const double d02,
                  const double d10, const double d11, const double d12,
                  const double d20, const double d21, const double d22)
  {
    this->arr[0][0] = d00;
    this->arr[0][1] = d01;
    this->arr[0][2] = d02;
    this->arr[1][0] = d10;
    this->arr[1][1] = d11;
    this->arr[1][2] = d12;
    this->arr[2][0] = d20;
    this->arr[2][1] = d21;
    this->arr[2][2] = d22;
    return *this;
  }

  XcMatrix & fillRows(const XcVector &v1, const XcVector &v2, const XcVector &v3)
  {
    this->arr[0][0] = v1(0);
    this->arr[0][1] = v1(1);
    this->arr[0][2] = v1(2);
    this->arr[1][0] = v2(0);
    this->arr[1][1] = v2(1);
    this->arr[1][2] = v2(2);
    this->arr[2][0] = v3(0);
    this->arr[2][1] = v3(1);
    this->arr[2][2] = v3(2);
    return *this;
  }

  XcMatrix & fillCols(const XcVector &v1, const XcVector &v2, const XcVector &v3)
  {
    this->arr[0][0] = v1(0);
    this->arr[1][0] = v1(1);
    this->arr[2][0] = v1(2);
    this->arr[0][1] = v2(0);
    this->arr[1][1] = v2(1);
    this->arr[2][1] = v2(2);
    this->arr[0][2] = v3(0);
    this->arr[1][2] = v3(1);
    this->arr[2][2] = v3(2);
    return *this;
  }

  XcVector col(const unsigned short i) const
  {
    return XcVector(this->arr[0][i], this->arr[1][i], this->arr[2][i]);
  }

  XcVector row(const unsigned short i) const
  {
    return XcVector(this->arr[i][0], this->arr[i][1], this->arr[i][2]);
  }

  XcMatrix & fillFromScalar(const double s)
  {
    this->arr[0][0] = s;
    this->arr[0][1] = 0.0;
    this->arr[0][2] = 0.0;
    this->arr[1][0] = 0.0;
    this->arr[1][1] = s;
    this->arr[1][2] = 0.0;
    this->arr[2][0] = 0.0;
    this->arr[2][1] = 0.0;
    this->arr[2][2] = s;
    return *this;
  }

  double & operator()(const unsigned short row, const unsigned short col)
  {return arr[row][col];}
  const double & operator()(const unsigned short row, const unsigned short col) const
  {return arr[row][col];}

  double * operator[](const unsigned short row)
  {return arr[row];}
  const double * operator[](const unsigned short row) const
  {return arr[row];}

  XcMatrix & operator*=(const double scalar)
  {
    this->arr[0][0] *= scalar;
    this->arr[0][1] *= scalar;
    this->arr[0][2] *= scalar;
    this->arr[1][0] *= scalar;
    this->arr[1][1] *= scalar;
    this->arr[1][2] *= scalar;
    this->arr[2][0] *= scalar;
    this->arr[2][1] *= scalar;
    this->arr[2][2] *= scalar;
    return *this;
  }

  XcMatrix operator*(const double scalar) const
  {
    return XcMatrix
      (this->arr[0][0]*scalar, this->arr[0][1]*scalar, this->arr[0][2]*scalar,
       this->arr[1][0]*scalar, this->arr[1][1]*scalar, this->arr[1][2]*scalar,
       this->arr[2][0]*scalar, this->arr[2][1]*scalar, this->arr[2][2]*scalar);
  }

  XcMatrix & operator/=(const double scalar) {return ((*this) *= (1.0 / scalar));}

  XcMatrix & operator*=(const XcMatrix &other)
  {
    // Probably a smarter way to do this, but be careful overwriting
    // values. Just use a tmp double for swaps.
    return *this = (*this * other);
  }

  XcMatrix operator*(const XcMatrix &other) const
  {
    return XcMatrix
      (this->arr[0][0]*other.arr[0][0] + this->arr[0][1]*other.arr[1][0] + this->arr[0][2]*other.arr[2][0],
       this->arr[0][0]*other.arr[0][1] + this->arr[0][1]*other.arr[1][1] + this->arr[0][2]*other.arr[2][1],
       this->arr[0][0]*other.arr[0][2] + this->arr[0][1]*other.arr[1][2] + this->arr[0][2]*other.arr[2][2],

       this->arr[1][0]*other.arr[0][0] + this->arr[1][1]*other.arr[1][0] + this->arr[1][2]*other.arr[2][0],
       this->arr[1][0]*other.arr[0][1] + this->arr[1][1]*other.arr[1][1] + this->arr[1][2]*other.arr[2][1],
       this->arr[1][0]*other.arr[0][2] + this->arr[1][1]*other.arr[1][2] + this->arr[1][2]*other.arr[2][2],

       this->arr[2][0]*other.arr[0][0] + this->arr[2][1]*other.arr[1][0] + this->arr[2][2]*other.arr[2][0],
       this->arr[2][0]*other.arr[0][1] + this->arr[2][1]*other.arr[1][1] + this->arr[2][2]*other.arr[2][1],
       this->arr[2][0]*other.arr[0][2] + this->arr[2][1]*other.arr[1][2] + this->arr[2][2]*other.arr[2][2]);
  }

  XcVector operator*(const XcVector &other) const
  {
    return XcVector
      (this->arr[0][0]*other(0) + this->arr[0][1]*other(1) + this->arr[0][2]*other(2),
       this->arr[1][0]*other(0) + this->arr[1][1]*other(1) + this->arr[1][2]*other(2),
       this->arr[2][0]*other(0) + this->arr[2][1]*other(1) + this->arr[2][2]*other(2));
  }

  // See http://www.dr-lex.be/random/matrix_inv.html -- determinant must not be near zero.
  XcMatrix inverse() const
  {
    return
      (XcMatrix
       (this->arr[1][1]*this->arr[2][2] - this->arr[1][2]*this->arr[2][1],
        this->arr[2][1]*this->arr[0][2] - this->arr[2][2]*this->arr[0][1],
        this->arr[0][1]*this->arr[1][2] - this->arr[0][2]*this->arr[1][1],
        this->arr[1][2]*this->arr[2][0] - this->arr[1][0]*this->arr[2][2],
        this->arr[2][2]*this->arr[0][0] - this->arr[2][0]*this->arr[0][2],
        this->arr[0][2]*this->arr[1][0] - this->arr[0][0]*this->arr[1][2],
        this->arr[1][0]*this->arr[2][1] - this->arr[1][1]*this->arr[2][0],
        this->arr[2][0]*this->arr[0][1] - this->arr[2][1]*this->arr[0][0],
        this->arr[0][0]*this->arr[1][1] - this->arr[0][1]*this->arr[1][0])
       /= this->determinant());
  }

  XcMatrix transpose() const
  {
    return XcMatrix (this->arr[0][0], this->arr[1][0], this->arr[2][0],
                     this->arr[0][1], this->arr[1][1], this->arr[2][1],
                     this->arr[0][2], this->arr[1][2], this->arr[2][2]);
  }

  double determinant() const
  {
    return
      this->arr[0][0] * (this->arr[1][1]*this->arr[2][2] - this->arr[1][2]*this->arr[2][1]) +
      this->arr[0][1] * (this->arr[1][2]*this->arr[2][0] - this->arr[1][0]*this->arr[2][2]) +
      this->arr[0][2] * (this->arr[1][0]*this->arr[2][1] - this->arr[1][1]*this->arr[2][0]);
  }

 private:
  double arr[3][3];
};

#endif

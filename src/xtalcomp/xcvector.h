/**********************************************************************
  XcVector - vector of three doubles, in-line class

  Copyright (C) 2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef XCVECTOR_H
#define XCVECTOR_H

#include <math.h>

class XcVector
{
 public:
  XcVector() {};

  XcVector(const double s) {arr[0]=s; arr[1]=s; arr[2]=s;}

  XcVector(const double x, const double y, const double z)
  {
    arr[0] = x; arr[1] = y; arr[2] = z;
  }

  XcVector(const XcVector &other)
  {
    this->arr[0] = other.arr[0];
    this->arr[1] = other.arr[1];
    this->arr[2] = other.arr[2];
  }

  XcVector & fill(const double s) {arr[0]=s; arr[1]=s; arr[2]=s; return *this;}

  XcVector & set(const double x, const double y, const double z)
  {
    arr[0] = x; arr[1]=y; arr[2]=z; return *this;
  }

  double & x() {return arr[0];}
  double & y() {return arr[1];}
  double & z() {return arr[2];}
  const double & x() const {return arr[0];}
  const double & y() const {return arr[1];}
  const double & z() const {return arr[2];}

  double & operator()(const unsigned short i)
  {return arr[i];}
  const double & operator()(const unsigned short i) const
  {return arr[i];}

  double & operator[](const unsigned short i)
  {return arr[i];}
  const double & operator[](const unsigned short i) const
  {return arr[i];}

  XcVector & operator*=(const double scalar)
  {
    this->arr[0] *= scalar;
    this->arr[1] *= scalar;
    this->arr[2] *= scalar;
    return *this;
  }

  XcVector & operator/=(const double scalar) {return ((*this) *= (1.0 / scalar));}

  XcVector & operator+=(const XcVector &other)
  {
    this->arr[0] += other.arr[0];
    this->arr[1] += other.arr[1];
    this->arr[2] += other.arr[2];
    return *this;
  }

  XcVector & operator-=(const XcVector &other)
  {
    this->arr[0] -= other.arr[0];
    this->arr[1] -= other.arr[1];
    this->arr[2] -= other.arr[2];
    return *this;
  }

  XcVector operator-() const
  {
    return XcVector(-this->arr[0], -this->arr[1], -this->arr[2]);
  }

  XcVector operator*(const double scalar) const
  {
    return XcVector(this->arr[0] * scalar,
                    this->arr[1] * scalar,
                    this->arr[2] * scalar);
  }

  XcVector operator/(const double scalar) const {return ((*this) * (1.0 / scalar));}

  XcVector operator+(const XcVector &other) const
  {
    return XcVector(this->arr[0] + other.arr[0],
                    this->arr[1] + other.arr[1],
                    this->arr[2] + other.arr[2]);
  }

  XcVector operator-(const XcVector &other) const
  {
    return XcVector(this->arr[0] - other.arr[0],
                    this->arr[1] - other.arr[1],
                    this->arr[2] - other.arr[2]);
  }

  double squaredNorm() const {return this->dot(*this);}

  double norm() const {return sqrt(this->squaredNorm());}

  double dot(const XcVector &other) const
  {
    return
      this->arr[0]*other.arr[0] +
      this->arr[1]*other.arr[1] +
      this->arr[2]*other.arr[2];
  }

 private:
  double arr[3];
};

#endif

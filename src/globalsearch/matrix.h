/**********************************************************************
  Matrix - typedefs for matrices using Eigen.

  Copyright (C) 2016 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#ifndef GLOBALSEARCH_MATRIX_H
#define GLOBALSEARCH_MATRIX_H

#include <Eigen/Dense>

#include "vector.h"

namespace GlobalSearch {
typedef Eigen::Matrix<double, 3, 3> Matrix3;

inline bool fuzzyCompare(const Matrix3& v1, const Matrix3& v2,
                         double tol = 1e-8)
{
  return fuzzyCompare(Vector3(v1.row(0)), Vector3(v2.row(0)), tol) &&
         fuzzyCompare(Vector3(v1.row(1)), Vector3(v2.row(1)), tol) &&
         fuzzyCompare(Vector3(v1.row(2)), Vector3(v2.row(2)), tol);
}
}

#endif

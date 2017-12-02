/**********************************************************************
  Vector - typedefs for vectors using Eigen.

  Copyright (C) 2016 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#ifndef GLOBALSEARCH_VECTOR_H
#define GLOBALSEARCH_VECTOR_H

#include <Eigen/Dense>

namespace GlobalSearch {
typedef Eigen::Matrix<double, 3, 1> Vector3;

inline bool fuzzyCompare(double a1, double a2, double tol = 1e-8)
{
  return std::fabs(a1 - a2) < tol;
}

inline bool fuzzyCompare(const Vector3& v1, const Vector3& v2,
                         double tol = 1e-8)
{
  return fuzzyCompare(v1[0], v2[0], tol) && fuzzyCompare(v1[1], v2[1], tol) &&
         fuzzyCompare(v1[2], v2[2], tol);
}
}

#endif

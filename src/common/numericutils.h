/**********************************************************************
  numericutils - Numeric helpers: rounding, reductions, and tolerant
                 floating-point comparisons.

  Copyright (C) 2011 by David C. Lonie
  Copyright (C) 2015 - 2017 by Patrick S. Avery
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef COMMON_NUMERICUTILS_H
#define COMMON_NUMERICUTILS_H

#include <common/constants.h>

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace Common {

inline int findMinIndex(const std::vector<double>& list)
{
  // A helper function to find the index of the minimum
  //   of a list of double values.
  if (list.empty()) {
    return -1;
  }

  auto min_it = std::min_element(list.begin(), list.end());
  return std::distance(list.begin(), min_it);
}

inline double roundToDecimalPlaces(double d, int n)
{
  // A helper function to set the precision of double values
  // Basically, it returns "d" with "n" decimal digits. If n<0,
  //   "d" is returned as is.
  if (n < 0)
    return d;
  double prec = std::pow(10.0, n);
  return std::round(d * prec) / prec;
}

inline bool lt(double v1, double v2, double prec = STABLE_TOL)
{
  return (v1 < (v2 - prec));
}

inline bool gt(double v1, double v2, double prec = STABLE_TOL)
{
  return (v2 < (v1 - prec));
}

inline bool eq(double v1, double v2, double prec = STABLE_TOL)
{
  return (!(lt(v1, v2, prec) || gt(v1, v2, prec)));
}

inline bool neq(double v1, double v2, double prec = STABLE_TOL)
{
  return (!(eq(v1, v2, prec)));
}

inline bool leq(double v1, double v2, double prec = STABLE_TOL)
{
  return (!gt(v1, v2, prec));
}

inline bool geq(double v1, double v2, double prec = STABLE_TOL)
{
  return (!lt(v1, v2, prec));
}

inline bool fuzzyCompare(double a1, double a2, double tol = ZERO08)
{
  return std::fabs(a1 - a2) < tol;
}

inline double sign(double v)
{
  // consider 0 to be positive
  if (v >= 0)
    return 1.0;
  else
    return -1.0;
}

} // namespace Common

#endif // COMMON_NUMERICUTILS_H

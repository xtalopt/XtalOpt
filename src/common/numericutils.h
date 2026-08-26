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
#include <common/compatibility/platform_compat.h>

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

// About a "not a number" value: it is neither smaller than, equal to, nor
//   larger than anything else. The two tests just below answer "no" for it on
//   their own, because any direct comparison with it is false. But "eq", "leq"
//   and "geq" are written as negations, which would turn that into a "yes",
//   so they check for it first.
// For any "nan" present in arguments; they all return "false" except than
//   "neq" which (correctly!) returns true.
// Overall, it is always assumed that the caller would check for "nan".

inline bool lt(double v1, double v2, double prec = ZERO08)
{
  return (v1 < (v2 - prec));
}

inline bool gt(double v1, double v2, double prec = ZERO08)
{
  return (v2 < (v1 - prec));
}

inline bool eq(double v1, double v2, double prec = ZERO08)
{
  return (!GS_ISNAN(v1) && !GS_ISNAN(v2) &&
          !(lt(v1, v2, prec) || gt(v1, v2, prec)));
}

inline bool neq(double v1, double v2, double prec = ZERO08)
{
  return (!(eq(v1, v2, prec)));
}

inline bool leq(double v1, double v2, double prec = ZERO08)
{
  return (!GS_ISNAN(v1) && !GS_ISNAN(v2) && !gt(v1, v2, prec));
}

inline bool geq(double v1, double v2, double prec = ZERO08)
{
  return (!GS_ISNAN(v1) && !GS_ISNAN(v2) && !lt(v1, v2, prec));
}

inline double sign(double v)
{
  // Consider 0 to be positive.
  // Note: this is not guarded against NaN, etc!
  return (v >= 0) ? 1.0 : -1.0;
}

} // namespace Common

#endif // COMMON_NUMERICUTILS_H

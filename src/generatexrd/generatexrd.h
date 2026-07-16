/**********************************************************************
  generatexrd - Generate simulated Xrd pattern

  Copyright (C) 2018 by Patrick Avery
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef GLOBALXRD_GENERATEXRD_H
#define GLOBALXRD_GENERATEXRD_H

#include <utility>
#include <vector>
#include <cstddef> // for size_t

#include <generatexrd/constants.h>

namespace Atoms {
class Geometry;
}

// Functions for making XRD patterns. There is no GenerateXrd class.
namespace GenerateXrd {

using XrdData = std::vector<std::pair<double, double>>;

// Make an XRD pattern. Results are angle and intensity pairs.
bool generatePattern(const Atoms::Geometry& structure, XrdData& results,
                     double wavelength = DEFAULT_WAVELENGTH, double peakwidth = DEFAULT_PEAKWIDTH,
                     size_t numpoints = DEFAULT_NUMPOINTS, double max2theta = DEFAULT_MAX_2THETA);

} // namespace GenerateXrd

#endif // GLOBALXRD_GENERATEXRD_H

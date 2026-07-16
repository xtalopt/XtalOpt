/**********************************************************************
  constants - Constants used in the Xrd utilities.

  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef GLOBALXRD_CONSTANTS_H
#define GLOBALXRD_CONSTANTS_H

namespace GenerateXrd {

static constexpr double DEFAULT_WAVELENGTH = 1.5056;
static constexpr double DEFAULT_PEAKWIDTH = 0.52958;
static constexpr int    DEFAULT_NUMPOINTS = 1000;
static constexpr double DEFAULT_MAX_2THETA = 162.0;
static constexpr double XRD_MERGE_TOL = 1.0e-3;

} // namespace GenerateXrd

#endif // GLOBALXRD_CONSTANTS_H

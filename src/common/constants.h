/**********************************************************************
  constants - Constants used in Common and all related codes

  Copyright (C) 2024 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef COMMON_CONSTANTS_H
#define COMMON_CONSTANTS_H

static constexpr double PI = 3.14159265358979323846;

static constexpr double DEG2RAD = PI / 180.0;
static constexpr double RAD2DEG = 180.0 / PI;

static constexpr double ANG2BOHR = 1.889725989;

static constexpr double EV2KCALMOL = 23.060538;
static constexpr double KCALMOL2EV = 0.043364122;
static constexpr double EV2KJMOL   = 96.4853365;
static constexpr double KJMOL2EV   = 0.0103642692;
static constexpr double RY2EV      = 13.60569193;

static constexpr double PINF =  1.0e+300;
static constexpr double MINF = -1.0e+300;

static constexpr double ZERO00 = 1.0e-50;
static constexpr double ZERO12 = 1.0e-12;
static constexpr double ZERO08 = 1.0e-8;
static constexpr double ZERO06 = 1.0e-6;
static constexpr double ZERO05 = 1.0e-5;
static constexpr double ZERO04 = 1.0e-4;
static constexpr double ZERO03 = 1.0e-3;
static constexpr double ZERO02 = 1.0e-2;

// Comparison tolerance
static constexpr double STABLE_TOL = 1.0e-5;

// Crystallography tolerances
static constexpr double SPGLIB_TOL = 0.01;
static constexpr double NIGGLI_TOL = 0.01;

#endif // COMMON_CONSTANTS_H

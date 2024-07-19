/**********************************************************************
  constants.h - Constants and conversion factors used in XtalOpt.

  Copyright (C) 2024 by Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#ifndef CONSTANTS_H
#define CONSTANTS_H

// Constant numerical values and conversion factors used in the code
// The goal is to use these constants, wherever they are needed, instead
//   of explicit numerical values. That's, to keep code easier to maintain.

// Conversions
static const double EV2KCALMOL = 23.060538;
static const double KCALMOL2EV =  0.043364122;
static const double EV2KJMOL   = 96.4853365;
static const double KJMOL2EV   =  0.0103642692;
static const double ANG2BOHR   =  1.889725989;

// Pi constant
static const double PI = 3.14159265358979323846;

// Degree to Radian & Radian to Degree conversion factors
static const double DEG2RAD = PI / 180.0;
static const double RAD2DEG = 180.0 / PI;

// Very large positive/negative numbers (pos/neg infinity)
static const double PINF = 1.0e+300;
static const double MINF = 1.0e-300;

// Various "zero"s used in the code as threshold for a small value
static const double ZERO0 = 1.0e-50;
static const double ZERO8 = 1.0e-8;
static const double ZERO6 = 1.0e-6;
static const double ZERO5 = 1.0e-5;
static const double ZERO4 = 1.0e-4;
static const double ZERO2 = 1.0e-2;

#endif // CONSTANTS_H

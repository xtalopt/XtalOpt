/**********************************************************************
  VaspFormat -- A simple reader for VASP output.

  Copyright (C) 2017 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef GLOBALSEARCH_VASP_FORMAT_H
#define GLOBALSEARCH_VASP_FORMAT_H

#include <istream>

// Forward declaration
class QString;

namespace GlobalSearch {

// Forward declaration
class Structure;

/**
 * @class Vienna Ab initio Simulation Package (VASP) format.
 *        https://www.vasp.at/
 */
class VaspFormat
{
public:
  // filename should be a CONTCAR file. This function will search
  // for the OUTCAR file in the same directory.
  static bool read(Structure* s, const QString& filename);

  // Read the OUTCAR given in @p istream and find the energy. If it is
  // not found, return false.
  static bool getOUTCAREnergy(std::istream& in, double& energy);

  // Read the OUTCAR given in @p istream and find the enthalpy. If it is
  // not found, return false.
  static bool getOUTCAREnthalpy(std::istream& in, double& ethalpy);
};
}

#endif // GLOBALSEARCH_VASP_FORMAT_H

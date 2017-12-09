/**********************************************************************
  PoscarFormat -- A simple reader for the POSCAR POSCAR file.

  Copyright (C) 2017 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef GLOBALSEARCH_POSCAR_FORMAT_H
#define GLOBALSEARCH_POSCAR_FORMAT_H

#include <istream>
#include <ostream>

namespace GlobalSearch {

// Forward declaration
class Structure;

/**
 * @class Vienna Ab initio Simulation Package (POSCAR) format.
 *        https://www.vasp.at/
 */
class PoscarFormat
{
public:
  static bool read(Structure& s, std::istream& in);
  static bool write(const Structure& s, std::ostream& out);
  static void reorderAtomsToMatchPoscar(Structure& s);
};
}

#endif // GLOBALSEARCH_POSCAR_FORMAT_H

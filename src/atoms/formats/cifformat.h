/**********************************************************************
  CifFormat - Handlers for CIF format structure files.

  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef ATOMS_CIF_FORMAT_H
#define ATOMS_CIF_FORMAT_H

#include <common/constants.h>

#include <ostream>

class QString;

namespace Atoms {

class Geometry;

/**
 * @class The Crystallographic Information File (CIF) format.
 */
class CifFormat
{
public:
  static bool read(Geometry* s, const QString& filename);

  // symprec is the symmetry tolerance for the space-group information.
  static bool write(const Geometry& s, std::ostream& out, double symprec = SPGLIB_TOL);
};

} // namespace Atoms

#endif // ATOMS_CIF_FORMAT_H

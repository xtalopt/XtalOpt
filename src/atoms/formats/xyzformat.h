/**********************************************************************
  XyzFormat - Handlers for XYZ format structure files.

  Copyright (C) 2017 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef ATOMS_XYZ_FORMAT_H
#define ATOMS_XYZ_FORMAT_H

// Forward declaration
class QString;

#include <ostream>

namespace Atoms {
class Geometry;
}

namespace Atoms {

/**
 * @class The XYZ format. These files will typically look like this:
 *        5
 *        Methane
 *        C      0.00000    0.00000    0.00000
 *        H      0.00000    0.00000    1.08900
 *        H      1.02672    0.00000   -0.36300
 *        H     -0.51336   -0.88916   -0.36300
 *        H     -0.51336    0.88916   -0.36300
 *
 *        Multiple molecules are allowed as well.
 */
class XyzFormat
{
public:
  /**
   * Read an XYZ file into @p s.
   */
  static bool read(Atoms::Geometry& s, const QString& filename);

  /**
   * Write @p s as XYZ.
   */
  static bool write(const Atoms::Geometry& s, std::ostream& out);
};
} // namespace Atoms

#endif // ATOMS_XYZ_FORMAT_H

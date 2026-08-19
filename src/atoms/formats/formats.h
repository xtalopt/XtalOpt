/**********************************************************************
  Formats - Geometry format detection and import utilities.

  Copyright (C) 2016 by Patrick Avery
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef ATOMS_FORMATS_H
#define ATOMS_FORMATS_H

#include <common/constants.h>

#include <iosfwd>

class QString;

namespace Atoms {

class Geometry;

// Read and write geometry in supported formats.
class Formats
{
public:
  static bool read(Atoms::Geometry& s, const QString& filename);
  static bool read(Atoms::Geometry& s, const QString& filename, const QString& format);
  static bool write(const Atoms::Geometry& s, std::ostream& out, const QString& format,
                    double symprec = SPGLIB_TOL);
  // The names a user may type for a format, mapped to the ones used here.
  static QString normalizedFormatName(const QString& format);
};

} // namespace Atoms

#endif // ATOMS_FORMATS_H

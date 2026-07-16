/**********************************************************************
  MtpFormat - Handlers for CFG format structure files (MTP code).

  Copyright (C) 2025 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef ATOMS_MTP_FORMAT_H
#define ATOMS_MTP_FORMAT_H

// Forward declaration
class QString;

#include <ostream>

namespace Atoms {

class Geometry;

/**
 * @class The MTP potential format.
 */
class MtpFormat
{
public:
  /**
   * Read an MTP structure file.
   */
  static bool read(Atoms::Geometry* s, const QString& filename);

  /**
   * Read MTP optimizer output.
   */
  static bool readOutput(Atoms::Geometry* s, const QString& filename);

  /**
   * Write @p s in MTP structure format.
   */
  static bool write(const Atoms::Geometry& s, std::ostream& out);
};
} // namespace Atoms

#endif // ATOMS_MTP_FORMAT_H

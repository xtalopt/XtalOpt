/**********************************************************************
  CastepFormat - A simple reader for CASTEP output structure.

  Copyright (C) 2017 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef ATOMS_CASTEP_FORMAT_H
#define ATOMS_CASTEP_FORMAT_H

// Forward declaration
class QString;

namespace Atoms {

class Geometry;

/**
 * @class The CASTEP (Cambridge Serial Total Energy Package) format.
 *        http://www.castep.org/
 */
class CastepFormat
{
public:
  /**
   * Read a CASTEP structure file.
   */
  static bool read(Atoms::Geometry& s, const QString& filename);

  /**
   * Read CASTEP optimizer output.
   */
  static bool readOutput(Atoms::Geometry& s, const QString& filename);
};
} // namespace Atoms

#endif // ATOMS_CASTEP_FORMAT_H

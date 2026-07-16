/**********************************************************************
  GulpFormat - A simple reader for GULP output structure.

  Copyright (C) 2016 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef ATOMS_GULP_FORMAT_H
#define ATOMS_GULP_FORMAT_H

// Forward declaration
class QString;

namespace Atoms {

class Geometry;

/**
 * @class The General Utility Lattice Program (GULP) format.
 *        http://gulp.curtin.edu.au/gulp/
 */
class GulpFormat
{
public:
  /**
   * Read GULP optimizer output.
   */
  static bool readOutput(Atoms::Geometry* s, const QString& filename);
};
} // namespace Atoms

#endif // ATOMS_GULP_FORMAT_H

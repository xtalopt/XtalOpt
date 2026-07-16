/**********************************************************************
  PwscfFormat - A simple reader for PWSCF output structure.

  Copyright (C) 2017 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef ATOMS_PWSCF_FORMAT_H
#define ATOMS_PWSCF_FORMAT_H

// Forward declaration
class QString;

namespace Atoms {

class Geometry;

/**
 * @class The PWSCF format for Quantum Espresso.
 *        http://www.quantum-espresso.org/
 */
class PwscfFormat
{
public:
  /**
   * Read a PWSCF input/structure file.
   */
  static bool read(Atoms::Geometry* s, const QString& filename);

  /**
   * Read PWSCF optimizer output.
   */
  static bool readOutput(Atoms::Geometry* s, const QString& filename);
};
} // namespace Atoms

#endif // ATOMS_PWSCF_FORMAT_H

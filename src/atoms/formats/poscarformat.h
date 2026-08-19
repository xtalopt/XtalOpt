/**********************************************************************
  PoscarFormat - Handlers for POSCAR format structure files.

  Copyright (C) 2017 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef ATOMS_POSCAR_FORMAT_H
#define ATOMS_POSCAR_FORMAT_H

#include <istream>
#include <ostream>

#include <QString>

namespace Atoms {

class Geometry;

/**
 * @class Vienna Ab initio Simulation Package (POSCAR) format.
 *        https://www.vasp.at/
 */
class PoscarFormat
{
public:
  /**
   * Read POSCAR/CONTCAR data from @p filename.
   */
  static bool read(Atoms::Geometry& s, const QString& filename);

  /**
   * Read POSCAR/CONTCAR data from @p in.
   */
  static bool read(Atoms::Geometry& s, std::istream& in);

  /**
   * Write @p s as POSCAR.
   */
  static bool write(const Atoms::Geometry& s, std::ostream& out,
                    const QString& comment = QString());

  /**
   * Write @p s as POSCAR and return the text as a QString.
   *
   * @return POSCAR text on success, or an empty string on failure.
   */
  static QString writeToString(const Atoms::Geometry& s, const QString& comment = QString());

  /**
   * Reorder atoms into POSCAR species ordering.
   */
  static void reorderAtomsToMatchPoscar(Atoms::Geometry& s);
};
} // namespace Atoms

#endif // ATOMS_POSCAR_FORMAT_H

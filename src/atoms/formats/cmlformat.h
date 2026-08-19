/**********************************************************************
  CmlFormat - Handlers for CML format structure files.

  Copyright (C) 2017 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef ATOMS_CML_FORMAT_H
#define ATOMS_CML_FORMAT_H

#include <istream>
#include <ostream>

class QString;

namespace Atoms {

class Geometry;

/**
 * @class CmlFormat cmlformat.h
 * @brief Implementation of the Chemical Markup Language format.
 * @author Patrick Avery
 */

class CmlFormat
{
public:
  static bool read(Atoms::Geometry& s, const QString& filename);
  static bool read(Atoms::Geometry& s, std::istream& in);
  static bool write(const Atoms::Geometry& s, std::ostream& out);
};
} // namespace Atoms

#endif // ATOMS_CML_FORMAT_H

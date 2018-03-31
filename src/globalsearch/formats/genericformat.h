/**********************************************************************
  GenericFormat -- A simple reader involving Open Babel for generic formats.

  Copyright (C) 2018 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef GLOBALSEARCH_GENERIC_FORMAT_H
#define GLOBALSEARCH_GENERIC_FORMAT_H

#include <istream>

namespace GlobalSearch {
class Structure;

/**
 * @class GenericFormat genericformat.h
 * @brief Generic format reader that uses Open Babel to read the file.
 * @author Patrick Avery
 */

class GenericFormat
{
public:
  // If @p formatName may be empty. If it is non-empty, that will be the
  // format used with Open Babel.
  static bool read(GlobalSearch::Structure& s, std::istream& in,
                   const std::string& formatName);
};
}

#endif // GLOBALSEARCH_GENERIC_FORMAT_H

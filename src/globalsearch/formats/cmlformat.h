/**********************************************************************
  CmlFormat -- A simple reader and writer for the CML format.

  Copyright (C) 2017 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef GLOBALSEARCH_CML_FORMAT_H
#define GLOBALSEARCH_CML_FORMAT_H

#include <istream>
#include <ostream>

namespace GlobalSearch {
class Structure;

/**
 * @class CmlFormat cmlformat.h
 * @brief Implementation of the Chemical Markup Language format - based
 *        upon the cml format implementation in Avogadro2.
 * @author Patrick Avery
 */

class CmlFormat
{
public:
  static bool read(GlobalSearch::Structure& s, std::istream& in);
  static bool write(const GlobalSearch::Structure& s, std::ostream& out);
};
}

#endif // GLOBALSEARCH_CML_FORMAT_H

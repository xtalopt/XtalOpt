/**********************************************************************
  SiestaFormat -- A simple reader for Siesta output.

  Copyright (C) 2016 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef GLOBALSEARCH_SIESTA_FORMAT_H
#define GLOBALSEARCH_SIESTA_FORMAT_H

// Forward declaration
class QString;

namespace GlobalSearch {

// Forward declaration
class Structure;

/**
 * @class The Spanish Initiative for Electronic Simulations with Thousands
 *        of Atoms (SIESTA) format.
 *        https://departments.icmab.es/leem/siesta/
 */
class SiestaFormat
{
public:
  static bool read(Structure* s, const QString& filename);
};
}

#endif // GLOBALSEARCH_SIESTA_FORMAT_H

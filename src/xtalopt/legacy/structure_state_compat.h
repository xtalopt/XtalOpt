/**********************************************************************
  structure_state_compat - Old structure.state compatibility.

  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef XTALOPT_LEGACY_STRUCTURE_STATE_COMPAT_H
#define XTALOPT_LEGACY_STRUCTURE_STATE_COMPAT_H

#include <search/structure.h>

class QString;

namespace XtalOpt {
namespace Legacy {

bool convertAndReadStructureState(Search::Structure& structure,
                                  const QString& structureStateFilename,
                                  const QString& mainStateFilename,
                                  bool& currentInfoRead);

} // namespace Legacy
} // namespace XtalOpt

#endif // XTALOPT_LEGACY_STRUCTURE_STATE_COMPAT_H

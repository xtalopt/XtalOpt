/**********************************************************************
  state_compat - Old XtalOpt state-file compatibility.

  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef XTALOPT_LEGACY_STATE_COMPAT_H
#define XTALOPT_LEGACY_STATE_COMPAT_H

#include <QList>

class QString;

namespace XtalOpt {
namespace Legacy {

const int Version4StateSchemaVersion = 4;

bool convertStateFile(const QString& filename, bool fullState,
                      bool keepCompatibilityCopy, QString& readFilename);

bool readVersion4Objectives(const QString& mainStateFilename,
                            QList<int>& constraintObjectiveIndices,
                            int& objectiveCount,
                            QString* errorMessage = nullptr);

} // namespace Legacy
} // namespace XtalOpt

#endif // XTALOPT_LEGACY_STATE_COMPAT_H

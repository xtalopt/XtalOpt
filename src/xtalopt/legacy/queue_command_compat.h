/**********************************************************************
  queue_command_compat - Old batch-queue command-key compatibility.

  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef XTALOPT_LEGACY_QUEUE_COMMAND_COMPAT_H
#define XTALOPT_LEGACY_QUEUE_COMMAND_COMPAT_H

#include <QString>
#include <QStringList>

class QSettings;

namespace XtalOpt {
namespace Legacy {

void normalizeSearchState(QSettings& settings, const QString& searchId,
                          int loadedVersion, QStringList& notes);

} // namespace Legacy
} // namespace XtalOpt

#endif // XTALOPT_LEGACY_QUEUE_COMMAND_COMPAT_H

/**********************************************************************
  input_compat - Old xtalopt.in input compatibility.

  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef XTALOPT_LEGACY_INPUT_COMPAT_H
#define XTALOPT_LEGACY_INPUT_COMPAT_H

class QString;

namespace XtalOpt {
namespace Legacy {

// Prepare input settings text for the current reader.
// NOTE: compatibilityFilename is set only when a converted copy was kept.
bool prepareXtalOptInputTextForRead(const QString& filename, const QString& inputText,
                                    QString& outputText, bool keepCompatibilityCopy,
                                    QString* compatibilityFilename = nullptr,
                                    QString* errorMessage = nullptr);

} // namespace Legacy
} // end namespace XtalOpt

#endif // XTALOPT_LEGACY_INPUT_COMPAT_H

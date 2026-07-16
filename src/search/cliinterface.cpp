/**********************************************************************
  cliinterface - Install output and prompt handlers for terminal

  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/cliinterface.h>

#include <QTextStream>

#include <common/output.h>
#include <search/search.h>
#include <search/passwordprompt.h>

namespace Search {

// Install blocking prompt handlers for terminal use. Output needs no special
//   setup (Common writes to stdout); prompts go through SearchBase so the
//   same code works for both the CLI and the GUI.
void installTerminalInterface(SearchBase& search)
{
  search.setDecisionPromptHandler(
    [](const QString& message, bool defaultValue) -> bool {
      // Print the prompt through Common so it also ends up in the --log file.
      QTextStream in(stdin);
      Common::message(message + (defaultValue ? "\n[Y/n]" : "\n[y/N]"));

      const QString response = in.readLine().trimmed();
      if (response.isEmpty())
        return defaultValue;
      const QChar first = response.at(0);
      return first == QLatin1Char('y') || first == QLatin1Char('Y');
    });
  search.setPasswordPromptHandler(
    [](const QString& message, QString& password) -> bool {
      // PasswordPrompt handles platform-specific echo suppression.
      password = QString::fromStdString(PasswordPrompt::getPassword(message.toStdString()));
      return true;
    });
}

} // namespace Search

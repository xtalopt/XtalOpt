/**********************************************************************
  passwordprompt - Interactive password prompt

  Copyright (C) 2015 by XtalOpt Developers

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

// Written by Patrick S. Avery -- 2015
// Hides the console input while the user inputs the password
// Returns the password

#include <iostream>
#include <string>

#include <common/compatibility/platform_defs.h>
#include <common/output.h>

// Include Windows header for Windows
#if GS_WINDOWS
#include <windows.h>
#else // Include Unix headers for Unix
#include <termios.h>
#include <unistd.h>
#endif

#include "passwordprompt.h"

namespace Search {

std::string PasswordPrompt::getPassword(const std::string& prompt)
{
  bool echoDisabled = false;

// For Windows
#if GS_WINDOWS
  HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
  DWORD mode = 0;
  if (hStdin != INVALID_HANDLE_VALUE && GetConsoleMode(hStdin, &mode) &&
      SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT))) {
    echoDisabled = true;
  }
#else // For Unix
  termios oldt;
  if (tcgetattr(STDIN_FILENO, &oldt) == 0) {
    termios newt = oldt;
    newt.c_lflag &= ~ECHO;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) == 0)
      echoDisabled = true;
  }
#endif

  std::string s;
  Common::message(QString::fromStdString(prompt));
  getline(std::cin, s);
  if (echoDisabled)
    Common::message(QString());

// Cleanup
#if GS_WINDOWS
  if (echoDisabled)
    SetConsoleMode(hStdin, mode);
#else
  if (echoDisabled)
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif

  return s;
}

} // namespace Search

/**********************************************************************
  passwordprompt - Interactive password prompt

  Copyright (C) 2015 XtalOpt Developers

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

// Written by Patrick S. Avery -- 2015

#ifndef SEARCH_PASSWORD_PROMPT_H
#define SEARCH_PASSWORD_PROMPT_H

#include <string>

namespace Search {

/**
 * Reads a password from the terminal, suppressing the echo of typed
 *   characters where the platform permits.
 */
class PasswordPrompt
{
public:
  /**
   * Prompt on the terminal and return the entered password.
   */
  static std::string getPassword(const std::string& prompt = "Enter password: ");
};

} // namespace Search

#endif // SEARCH_PASSWORD_PROMPT_H

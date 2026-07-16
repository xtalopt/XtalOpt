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

#ifndef SEARCH_CLIINTERFACE_H
#define SEARCH_CLIINTERFACE_H

namespace Search {
class SearchBase;

void installTerminalInterface(SearchBase& search);

} // namespace Search

#endif // SEARCH_CLIINTERFACE_H

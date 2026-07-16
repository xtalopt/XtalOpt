/**********************************************************************
  constants - Constants used in Search engine

  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef SEARCH_CONSTANTS_H
#define SEARCH_CONSTANTS_H

static constexpr int OBJECTIVE_WAIT_CYCLES = 720;

// SSH constants
static constexpr int CLISSH_RUN_TIMEOUT = 60000;
static constexpr int CLISSH_SCP_TIMEOUT = 600000;
static constexpr int CLISSH_PRECHECK_TIMEOUT = 15000;

// Process control timeouts (milliseconds): how long to wait for a process to
//   quit after asking it to terminate (before we force-kill it), and how long
//   to wait for it to be gone after a kill.
static constexpr int PROCESS_TERMINATE_TIMEOUT = 5000;
static constexpr int PROCESS_KILL_TIMEOUT = 1000;

// One wait round for the running structure handlers (milliseconds); the callers
//   keep waiting round after round and print a warning after each one.
static constexpr int STRUCTURE_HANDLER_WAIT_TIMEOUT = 10000;

#endif // SEARCH_CONSTANTS_H

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

// == Engine "timing" constants.
// Period of the QueueManager check loop (the engine "tick") in ms.
static constexpr int QUEUE_CHECK_INTERVAL = 1000;
// Minimum interval between submitting batch jobs to a scheduler in ms.
static constexpr int SUBMISSION_MIN_GAP_MS = 500;
// Wait time delay for a soft exit (performTheExit) in seconds.
static constexpr int SOFT_EXIT_GRACE_S = 3;

// == Job and processes "timing" constants.
// Process control timeouts (milliseconds): how long to wait for a process to
//   start, quit after terminate (before force-kill), and finish after kill.
static constexpr int PROCESS_START_TIMEOUT = 5000;
static constexpr int PROCESS_TERMINATE_TIMEOUT = 5000;
static constexpr int PROCESS_KILL_TIMEOUT = 1000;

// One wait round for the running structure handlers (milliseconds); the callers
//   keep waiting round after round and print a warning after each one.
static constexpr int STRUCTURE_HANDLER_WAIT_TIMEOUT = 10000;

// Time interval to check output file for objc/cons after it appears
static constexpr int OBJECTIVE_CHECK_MS = 500;

// == SSH "timing" constants.
static constexpr int CLISSH_RUN_TIMEOUT = 60000;
static constexpr int CLISSH_SCP_TIMEOUT = 600000;
static constexpr int CLISSH_PRECHECK_TIMEOUT = 15000;

#endif // SEARCH_CONSTANTS_H

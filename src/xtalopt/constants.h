/**********************************************************************
  constants - Constants used in XtalOpt

  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef XTALOPT_CONSTANTS_H
#define XTALOPT_CONSTANTS_H

// Wait times for saving files to disk (in milliseconds):
//   1) for how long we collect results-file save requests before a writing
//   2) for how long we wait before re-try a failed writing
static constexpr int RESULTS_SAVE_DELAY = 250;
static constexpr int SAVE_RETRY_DELAY = 60000;

// "Time interval" factor between two results file writings:
//   time interval between two writes is the greatest of default results save delay
//   and this factor * duration of previous write; so results writing frequency
//   is adjusted on-the-fly with the local optimization speed and population size.
static constexpr int RESULTS_SAVE_SPACING_FACTOR = 25;

// How often to check the run-time file during a CLI run (milliseconds).
static constexpr int RUNTIME_FILE_CHECK_INTERVAL = 1000;

// The time period for waiting for initial structures to be generated (milliseconds)
static constexpr int INIT_WAIT_TIMEOUT = 250;

// How long the progress table waits before trying to refresh again; while a
//   context menu task is still running (in milliseconds).
static constexpr int PROGRESS_REFRESH_DELAY = 1000;

#endif // XTALOPT_CONSTANTS_H

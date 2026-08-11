/**********************************************************************
  timing - Profiling helper module.

  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef COMMON_TIMING_H
#define COMMON_TIMING_H

// Optional timing information, enabled by XTALOPT_TIMING=1.
// A call to "ScopedTimer" adds the wall time and per-thread CPU time of its "scope".
// The total per scopes are outputted with every batch, as well as at the end;
//   while a header line is printed for "whole-process" timing (so, CPU/elapsed
//   will show the average number of busy cores over the measured period).
// A low value for CPU/elapsed total (and cpu_ms/call much smaller than wall_ms/call)
//   means that the code (scoped) were mostly waiting over that period of time.

#include <common/compatibility/platform_compat.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace Common {

namespace timing_detail {

struct Bucket
{
  long long ns = 0;      // wall
  long long cpu_ns = 0;  // per-thread CPU
  long long calls = 0;
};

// Keep the timer data until exit.
inline std::mutex& mutex()
{
  static std::mutex* m = new std::mutex();
  return *m;
}

inline std::map<std::string, Bucket>& buckets()
{
  static std::map<std::string, Bucket>* b = new std::map<std::string, Bucket>();
  return *b;
}

inline std::chrono::steady_clock::time_point startPoint()
{
  static const std::chrono::steady_clock::time_point t = std::chrono::steady_clock::now();
  return t;
}

// Return the process CPU start time.
inline long long startProcessCpu()
{
  static const long long t = processCpuNanos();
  return t;
}

inline double& lastDumpSeconds()
{
  static double t = 0.0;
  return t;
}

} // namespace timing_detail

inline double timingElapsedSeconds()
{
  return std::chrono::duration<double>(
           std::chrono::steady_clock::now() - timing_detail::startPoint()).count();
}

inline double timingProcessCpuSeconds()
{
  return (processCpuNanos() - timing_detail::startProcessCpu()) / 1.0e9;
}

// The timing can be turned off while the program runs; e.g. a read-only
//   session does not report any timing.
inline bool& timingSuspended()
{
  static bool suspended = false;
  return suspended;
}

// Print the timer values (sorts with largest wall time at the top).
inline void dumpTimings()
{
  // Nothing to report if the timing is turned off.
  if (timingSuspended())
    return;

  std::vector<std::pair<std::string, timing_detail::Bucket> > rows;
  {
    std::lock_guard<std::mutex> lock(timing_detail::mutex());
    rows.assign(timing_detail::buckets().begin(), timing_detail::buckets().end());
  }
  std::sort(rows.begin(), rows.end(),
            [](const std::pair<std::string, timing_detail::Bucket>& a,
               const std::pair<std::string, timing_detail::Bucket>& b) {
              return a.second.ns > b.second.ns;
            });

  const double elapsed = timingElapsedSeconds();
  const double cpu = timingProcessCpuSeconds();
  const double occupancy = elapsed > 0.0 ? cpu / elapsed : 0.0;

  // Build the full output table before printing it as a single drop.
  std::string table;
  char line[512];
  std::snprintf(line, sizeof(line),
                "\n=== XtalOpt timing (elapsed %.1f s, CPU %.1f s, %.2fx cores) ===\n",
                elapsed, cpu, occupancy);
  table += line;
  std::snprintf(line, sizeof(line), "  %-55s %12s %12s %10s %13s %13s\n",
                "section", "wall_s", "cpu_s", "calls", "wall_ms/call", "cpu_ms/call");
  table += line;
  for (const auto& row : rows) {
    const double wall = row.second.ns / 1.0e9;
    const double cpus = row.second.cpu_ns / 1.0e9;
    const double wallPer = row.second.calls ? (row.second.ns / 1.0e6 / row.second.calls) : 0.0;
    const double cpuPer = row.second.calls ? (row.second.cpu_ns / 1.0e6 / row.second.calls) : 0.0;
    std::snprintf(line, sizeof(line), "  %-55s %12.3f %12.3f %10lld %13.4f %13.4f\n",
                  row.first.c_str(), wall, cpus, row.second.calls, wallPer, cpuPer);
    table += line;
  }
  std::fprintf(stdout, "%s\n", table.c_str());
  std::fflush(stdout);
}

inline bool timingEnabled()
{
  static const bool enabled = []() -> bool {
    const bool on = (std::getenv("XTALOPT_TIMING") != nullptr);
    if (on) {
      timing_detail::startPoint();      // fix the wall window start now
      timing_detail::startProcessCpu(); // ... and the CPU window start
      std::atexit(+[]() { Common::dumpTimings(); });
    }
    return on;
  }();
  return enabled && !timingSuspended();
}

// Add a timer value.
inline void addTiming(const char* name, long long nanoseconds, long long cpuNanoseconds = 0)
{
  std::lock_guard<std::mutex> lock(timing_detail::mutex());
  timing_detail::Bucket& bucket = timing_detail::buckets()[name];
  bucket.ns += nanoseconds;
  bucket.cpu_ns += cpuNanoseconds;
  ++bucket.calls;
}

// Print the timer values when needed.
inline void maybePeriodicDump()
{
  const double interval = 20.0; // seconds between periodic dumps
  const double now = timingElapsedSeconds();
  {
    std::lock_guard<std::mutex> lock(timing_detail::mutex());
    if (now - timing_detail::lastDumpSeconds() < interval)
      return;
    timing_detail::lastDumpSeconds() = now;
  }
  dumpTimings();
}

// Add a scope time to a named timer.
class ScopedTimer
{
public:
  explicit ScopedTimer(const char* name) : m_name(name), m_enabled(timingEnabled())
  {
    if (m_enabled) {
      m_start = std::chrono::steady_clock::now();
      m_cpuStart = threadCpuNanos();
    }
  }

  // Record the current time.
  void stop()
  {
    if (!m_enabled || m_done)
      return;
    m_done = true;
    const long long wall = std::chrono::duration_cast<std::chrono::nanoseconds>(
                             std::chrono::steady_clock::now() - m_start).count();
    addTiming(m_name, wall, threadCpuNanos() - m_cpuStart);
  }

  ~ScopedTimer()
  {
    if (!m_enabled)
      return;
    stop();
    maybePeriodicDump();
  }

private:
  const char* m_name;
  bool m_enabled;
  bool m_done = false;
  std::chrono::steady_clock::time_point m_start;
  long long m_cpuStart = 0;
};

} // namespace Common

#endif // COMMON_TIMING_H

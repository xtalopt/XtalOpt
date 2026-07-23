/**********************************************************************
  platform_compat - Platform-specific compatibility definitions

  Copyright (C) 2010-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef COMMON_PLATFORM_COMPAT_H
#define COMMON_PLATFORM_COMPAT_H

#include <common/compatibility/platform_defs.h>

#include <cmath>
#include <cstdlib>

#if GS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h> // Sleep
#include <float.h>   // _finite
#else
#include <time.h>   // clock_gettime
#include <unistd.h> // sleep
#endif

#if GS_WINDOWS

// Windows uses underscore-prefixed names for these
#define GS_ISNAN(a) _isnan((a))
#define GS_ISINF(a)                                                            \
  ((!static_cast<bool>(_finite((a)))) && !static_cast<bool>(_isnan((a))))
#define GS_ISFINITE(a) static_cast<bool>(_finite((a)))
#define GS_SLEEP(a)                                                            \
  Sleep(static_cast<unsigned long int>((a)*1000)) // arg in seconds

#else // !GS_WINDOWS

#define GS_ISNAN(a) std::isnan(a)
#define GS_ISINF(a) std::isinf(a)
#define GS_ISFINITE(a) std::isfinite(a)
#define GS_SLEEP(a) sleep(a) // arg in seconds

#endif // GS_WINDOWS

#define GS_IS_NAN_OR_INF(a) (GS_ISNAN(a) || GS_ISINF(a))

namespace Common {

// Process CPU counters (e.g., for the timing report).
// These return 0 when the value is not available.

// CPU time used by the calling thread, in nanoseconds.
inline long long threadCpuNanos()
{
#if GS_WINDOWS
  FILETIME creation, exitt, kernel, user;
  if (!GetThreadTimes(GetCurrentThread(), &creation, &exitt, &kernel, &user))
    return 0;
  ULARGE_INTEGER k, u;
  k.LowPart = kernel.dwLowDateTime; k.HighPart = kernel.dwHighDateTime;
  u.LowPart = user.dwLowDateTime;   u.HighPart = user.dwHighDateTime;
  return static_cast<long long>(k.QuadPart + u.QuadPart) * 100LL; // 100ns units
#elif defined(CLOCK_THREAD_CPUTIME_ID)
  struct timespec ts;
  if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) != 0)
    return 0;
  return static_cast<long long>(ts.tv_sec) * 1000000000LL + ts.tv_nsec;
#else
  return 0;
#endif
}

// CPU time used by the process, over all threads, in nanoseconds.
inline long long processCpuNanos()
{
#if GS_WINDOWS
  FILETIME creation, exitt, kernel, user;
  if (!GetProcessTimes(GetCurrentProcess(), &creation, &exitt, &kernel, &user))
    return 0;
  ULARGE_INTEGER k, u;
  k.LowPart = kernel.dwLowDateTime; k.HighPart = kernel.dwHighDateTime;
  u.LowPart = user.dwLowDateTime;   u.HighPart = user.dwHighDateTime;
  return static_cast<long long>(k.QuadPart + u.QuadPart) * 100LL;
#elif defined(CLOCK_PROCESS_CPUTIME_ID)
  struct timespec ts;
  if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts) != 0)
    return 0;
  return static_cast<long long>(ts.tv_sec) * 1000000000LL + ts.tv_nsec;
#else
  return 0;
#endif
}

} // namespace Common

#endif // COMMON_PLATFORM_COMPAT_H

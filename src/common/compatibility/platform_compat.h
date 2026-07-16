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

#endif // COMMON_PLATFORM_COMPAT_H

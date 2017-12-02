/**********************************************************************
  Macros - Some helpful definitions to simplify code

  Copyright (C) 2010-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef GLOBALSEARCHMACROS_H
#define GLOBALSEARCHMACROS_H

#include <QSettings>

#include <cstdlib>

#ifdef WIN32
// For Sleep
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
// For _finite
#include <float.h>
#else
#include <unistd.h> // For sleep on GCC >= 4.7
#endif

// Create a pointer of type QSettings *settings that points to either:
// 1) The default application QSettings object, or
// 2) A QSettings object that writes to "f"
// If "f" is empty or it is equal to the config file name, it will be a default
// application QSettings object. Any other condition produces a QSettings
// object that writes to "f".
#define SETTINGS(f)                                                            \
  QSettings *settings, pQS, rQS(f, QSettings::IniFormat);                      \
  settings =                                                                   \
    (QString(f).isEmpty() || QString(f) == pQS.fileName()) ? &pQS : &rQS;

// Platform specific defines
#ifdef WIN32

// Legacy windows functions with underscore prefix
#define GS_ISNAN(a) _isnan((a))
#define GS_ISINF(a)                                                            \
  ((!static_cast<bool>(_finite((a)))) && !static_cast<bool>(_isnan((a))))
#define GS_SLEEP(a)                                                            \
  Sleep(static_cast<unsigned long int>((a)*1000)) // arg in seconds
#define GS_MSLEEP(a)                                                           \
  Sleep(static_cast<unsigned long int>(a)) // arg in milliseconds

#else // def WIN32

// Legacy windows functions have underscore prefix
#define GS_ISNAN(a) std::isnan(a)
#define GS_ISINF(a) std::isinf(a)
#define GS_SLEEP(a) sleep(a)          // arg in seconds
#define GS_MSLEEP(a) usleep((a)*1000) // arg in milliseconds

#endif // WIN32

// combine isnan with inf

#define GS_IS_NAN_OR_INF(a) (GS_ISNAN(a) || GS_ISINF(a))

#endif // GLOBALSEARCHMACROS_H

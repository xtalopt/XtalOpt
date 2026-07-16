/**********************************************************************
  platform_defs - Platform detection macros

  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef COMMON_PLATFORM_DEFS_H
#define COMMON_PLATFORM_DEFS_H

#if defined(_WIN32) || defined(WIN32)
#define GS_WINDOWS 1
#else
#define GS_WINDOWS 0
#endif

#if GS_WINDOWS
#define GS_EXECUTABLE_SUFFIX ".exe"
#else
#define GS_EXECUTABLE_SUFFIX ""
#endif

#endif // COMMON_PLATFORM_DEFS_H

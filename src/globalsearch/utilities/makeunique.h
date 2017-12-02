/**********************************************************************
  make_unique - An implementation for make_unique for C++11.
                You should be able to include this header in programs that
                compile with both C++11 and C++14 since it checks the version.

  Copyright (C) 2017 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#ifndef MAKE_UNIQUE_H
#define MAKE_UNIQUE_H

#include <memory>

// Obtained from Item 21 in Scott Meyer's Effective Modern C++

// This function is not needed in C++14, so check for C++14
// MSVC hasn't updated the __cplusplus variable in a while, so do
// a separate check for them
#if (__cplusplus <= 201103L && !defined(_MSC_VER)) ||                          \
  (defined(_MSC_VER) && _MSC_VER < 1900)
template <typename T, typename... Ts>
std::unique_ptr<T> make_unique(Ts&&... params)
{
  return std::unique_ptr<T>(new T(std::forward<Ts>(params)...));
}
#else
using std::make_unique;
#endif

#endif

/**********************************************************************
  timer.h - a basic timer for functions. The user just needs to add the
            macro, START_TIMER, to the beginning of a function they want to
            time. When the function is ending, the timer will print the
            timing in microseconds by default.

            Macros:
            START_TIMER_S  - printed time is in seconds
            START_TIMER_MS - printed time is in milliseconds
            START_TIMER    - printed time is in microseconds
            START_TIMER_NS - printed time is in nanoseconds

  Copyright (C) 2017 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#ifndef TIMER_H
#define TIMER_H

#include <chrono>
#include <iostream>

#define START_TIMER_S Timer<std::chrono::seconds> timer(__FUNCTION__);
#define START_TIMER_MS Timer<std::chrono::milliseconds> timer(__FUNCTION__);
#define START_TIMER Timer<std::chrono::microseconds> timer(__FUNCTION__);
#define START_TIMER_NS Timer<std::chrono::nanoseconds> timer(__FUNCTION__);

// timeType needs to be one of the std::chrono time types such as
// std::chrono:microseconds
template <typename timeType>
class Timer
{
public:
  Timer(const std::string& functionName)
    : funcName(functionName), start(std::chrono::system_clock::now())
  {
    std::cout << funcName << " started!\n";
  };

  virtual ~Timer()
  {
    std::cout << funcName << " ending!\n";
    std::cout << funcName << " total elapsed time: "
              << std::chrono::duration_cast<timeType>(
                   std::chrono::system_clock::now() - start)
                   .count()
              << " μs\n";
  };

private:
  std::string funcName;
  std::chrono::time_point<std::chrono::system_clock> start;
};

#endif

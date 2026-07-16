/**********************************************************************
  random - Provides a function to generate random doubles/ints between
           a min and a max value

  Copyright (C) 2016 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef COMMON_RANDOM_H
#define COMMON_RANDOM_H

#include <climits>
#include <random>

namespace Common {

// Return the random generator for this thread.
inline std::mt19937& getMt19937Generator()
{
  thread_local std::mt19937 generator(std::random_device{}());
  return generator;
}

// Seed the generator
inline void seedMt19937Generator(unsigned int s)
{
  getMt19937Generator().seed(s);
}

// Output is in in [min, max)
inline double getRandDouble(double min = 0.0, double max = 1.0)
{
  std::uniform_real_distribution<double> distribution(min, max);
  return distribution(getMt19937Generator());
}

// Output is in [min, max]
inline int getRandInt(int min = INT_MIN, int max = INT_MAX)
{
  std::uniform_int_distribution<int> distribution(min, max);
  return distribution(getMt19937Generator());
}

// Output is in [min, max]
inline unsigned int getRandUInt(unsigned int min = 0, unsigned int max = UINT_MAX)
{
  std::uniform_int_distribution<unsigned int> distribution(min, max);
  return distribution(getMt19937Generator());
}
} // namespace Common

#endif // COMMON_RANDOM_H

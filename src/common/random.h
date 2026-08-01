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

#include <atomic>
#include <chrono>
#include <climits>
#include <limits>
#include <random>
#include <thread>

namespace Common {

struct RandomGeneratorState
{
  RandomGeneratorState()
    : generation(0), nextThread(0)
  {
    std::random_device device;
    const unsigned int clockValue = static_cast<unsigned int>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::seed_seq sequence{ device(), device(), device(), device(), clockValue };
    unsigned int initialSeed = 0;
    sequence.generate(&initialSeed, &initialSeed + 1);
    seed.store(initialSeed);
  }

  std::atomic<unsigned long> generation;
  std::atomic<unsigned int> nextThread;
  std::atomic<unsigned int> seed;
};

inline RandomGeneratorState& randomGeneratorState()
{
  static RandomGeneratorState state;
  return state;
}

// Return the random generator for this thread.
inline std::mt19937& getMt19937Generator()
{
  struct ThreadGenerator
  {
    ThreadGenerator()
      : generation(std::numeric_limits<unsigned long>::max())
    {}

    std::mt19937 generator;
    unsigned long generation;
  };

  RandomGeneratorState& state = randomGeneratorState();
  thread_local ThreadGenerator threadGenerator;
  const unsigned long generation = state.generation.load();
  if (threadGenerator.generation != generation) {
    const unsigned int threadNumber = state.nextThread.fetch_add(1);
    std::seed_seq sequence{ state.seed.load(), threadNumber };
    threadGenerator.generator.seed(sequence);
    threadGenerator.generation = generation;
  }
  return threadGenerator.generator;
}

// Seed the generator
inline void seedMt19937Generator(unsigned int s)
{
  RandomGeneratorState& state = randomGeneratorState();
  state.seed = s;
  state.nextThread.store(0);
  state.generation.fetch_add(1);
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

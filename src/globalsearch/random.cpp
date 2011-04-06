/**********************************************************************
  GSRandom -- A singleton randomnumber generator

  Copyright (C) 2010-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <globalsearch/random.h>

#include <cstdlib>
#include <ctime>

#ifdef WIN32
#include <process.h>
#define GETPID _getpid
#else // WIN32
#include <unistd.h>
#define GETPID getpid
#endif // WIN32

unsigned long getSeed()
{
    unsigned long a,b,c;
    a = std::clock();
    b = std::time(0);
    c = GETPID();
    a=a-b;  a=a-c;  a=a^(c >> 13);
    b=b-c;  b=b-a;  b=b^(a << 8);
    c=c-a;  c=c-b;  c=c^(b >> 13);
    a=a-b;  a=a-c;  a=a^(c >> 12);
    b=b-c;  b=b-a;  b=b^(a << 16);
    c=c-a;  c=c-b;  c=c^(b >> 5);
    a=a-b;  a=a-c;  a=a^(c >> 3);
    b=b-c;  b=b-a;  b=b^(a << 10);
    c=c-a;  c=c-b;  c=c^(b >> 15);
    return c;
}

namespace GlobalSearch {

  GSRandom* GSRandom::m_instance = 0;

  GSRandom* GSRandom::instance()
  {
    if (!m_instance) {
      m_instance = new GSRandom();
    }
    return m_instance;
  }

  GSRandom::GSRandom() :
    m_seedLock(false)
  {
    while (m_seedLock) {};
    m_seedLock = true;
    std::srand(getSeed());
    m_seedLock = false;
  }

  double GSRandom::getRandomDouble()
  {
    while (m_seedLock) {};
    m_seedLock = true;
    double d = rand() / (double)RAND_MAX;
    m_seedLock = false;
    return d;
  }

  unsigned int GSRandom::getRandomUInt()
  {
    while (m_seedLock) {};
    m_seedLock = true;
    unsigned int i = rand();
    m_seedLock = false;
    return i;
  }

}

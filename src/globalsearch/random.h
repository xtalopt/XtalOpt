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

#ifndef GLOBALSEARCHRANDOM_H
#define GLOBALSEARCHRANDOM_H

namespace GlobalSearch {

  /**
   * @class GSRandom random.h <globalsearch/random.h>
   *
   * @brief This class implements a thread-safe, cross-platform
   * singleton random number generator.
   *
   * Direct use of this class is
   * discouraged.  Rather, use the RANDDOUBLE(), etc macros in
   * globalsearch/macros.h.
   *
   * @author David C. Lonie
   */
  class GSRandom
  {
  public:
    /// @return the global GSRandom instance
    static GSRandom* instance();

    /// @return a random floating point number between zero and one
    double getRandomDouble();
    /// @return a random unsigned integer
    unsigned int getRandomUInt();

  protected:
    /// Constructor
    GSRandom();
    /// Disabled copy constructor
    GSRandom(const GSRandom&) {};
    /// Disabled assignment operator
    GSRandom& operator= (const GSRandom&) {};
  private:
    /// Global instances
    static GSRandom* m_instance;
    /// Internal use only
    bool m_seedLock;
  };
}

#endif

/**********************************************************************
  GSRandom -- A singleton randomnumber generator

  Copyright (C) 2010 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
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

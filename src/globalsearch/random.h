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

  class GSRandom
  {
  public:
    static GSRandom* instance();

    double getRandomDouble();
    unsigned int getRandomUInt();
  protected:
    GSRandom();
    GSRandom(const GSRandom&) {};
    GSRandom& operator= (const GSRandom&) {};
  private:
    static GSRandom* m_instance;
    bool m_seedLock;
  };
}

#endif

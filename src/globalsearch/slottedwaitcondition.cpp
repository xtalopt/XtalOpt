/**********************************************************************
  SlottedWaitCondition - Simple wrapper around QWaitCondition with
  wake slots.

  Copyright (C) 2010 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <globalsearch/slottedwaitcondition.h>

namespace GlobalSearch {

  SlottedWaitCondition::SlottedWaitCondition(QObject *parent)
    : QObject(parent),
      QWaitCondition(),
      m_mutex()
  {
  }

  SlottedWaitCondition::~SlottedWaitCondition()
  {
  }
}

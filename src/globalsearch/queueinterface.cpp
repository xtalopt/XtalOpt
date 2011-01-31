/**********************************************************************
  QueueInterface - Base queue interface class implementation

  Copyright (C) 2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <globalsearch/queueinterface.h>

#include <globalsearch/structure.h>
#include <globalsearch/optimizer.h>
#include <globalsearch/structure.h>

#include <QtCore/QString>
#include <QtCore/QStringList>


namespace GlobalSearch {

  bool QueueInterface::writeInputFiles(Structure *s) const
  {
    return writeFiles(s, m_opt->optimizer()->getInterpretedTemplates(s));
  }

} // end namespace GlobalSearch

/**********************************************************************
  Scene - Contains a docked substrate in a matrix

  Copyright (C) 2009-2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef SCENEMOL_H
#define SCENEMOL_H

#include <globalsearch/structure.h>

#include <QtCore/QDebug>

namespace RandomDock {
  class Scene : public GlobalSearch::Structure
  {
    Q_OBJECT

   public:
    Scene(QObject *parent = 0);
    virtual ~Scene();

   signals:

   public slots:

   private slots:

   private:

  };

} // end namespace RandomDock

#endif

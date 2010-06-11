/**********************************************************************
  RandomDock -- Templates: - Templates for input file for random
  docking problem

  Copyright (C) 2009 by David Lonie

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public icense for more details.
 ***********************************************************************/

#ifndef TEMPLATES_H
#define TEMPLATES_H

#include "scenemol.h"

#include <avogadro/molecule.h>

#include <QString>

namespace Avogadro {
  class RandomDockParams;

  class Templates
  {
  public:
    static QString interpretTemplate(const QString & str, Scene* mol);
    static void showHelp();
  };
}

#endif

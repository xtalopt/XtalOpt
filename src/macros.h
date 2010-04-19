/**********************************************************************
  Macros - Some helpful definitions to simplify code

  Copyright (C) 2010 by David C. Lonie

  This file is part of the Avogadro molecular editor project.
  For more information, see <http://avogadro.openmolecules.net/>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef XTALOPTMACROS_H
#define XTALOPTMACROS_H

#include <QSettings>

#define SETTINGS(f) QSettings *settings, pQS, rQS (f, QSettings::IniFormat); settings = (QString(f).isEmpty()) ? &pQS : &rQS;

#endif

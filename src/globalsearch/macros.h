/**********************************************************************
  Macros - Some helpful definitions to simplify code

  Copyright (C) 2010 by David C. Lonie

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
#include <cstdlib>

// Create a pointer of type QSettings *settings that points to either:
// 1) The default application QSettings object, or
// 2) A QSettings object that writes to "f"
#define SETTINGS(f) QSettings *settings, pQS, rQS (f, QSettings::IniFormat); settings = (QString(f).isEmpty()) ? &pQS : &rQS;

// If string f is non-empty, write the file immediately with sync(),
// otherwise, let the system decide when to write to file
#define DESTROY_SETTINGS(f) if (!QString(f).isEmpty()) settings->sync();

#define RANDDOUBLE() ( rand() / double(RAND_MAX) )

#endif

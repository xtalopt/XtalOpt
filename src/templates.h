/**********************************************************************
  XtalOpt - Tools for advanced crystal optimization

  Copyright (C) 2009 by David Lonie

  This file is part of the Avogadro molecular editor project.
  For more information, see <http://avogadro.openmolecules.net/>

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public icense for more details.
 ***********************************************************************/

#ifndef XTALOPTTEMPLATES_H
#define XTALOPTTEMPLATES_H

#include "xtal.h"
#include "xtalopt.h"

namespace Avogadro {
  class XtalOpt;
  class XtalOptTemplate
  {
  public:
    XtalOptTemplate() {};
    static QString interpretTemplate(const QString & str, Xtal* xtal, XtalOpt *p);
    static void showHelp();

    /////////////////////
    // Input Templates //
    /////////////////////

    static QString input_dump();
    static QString input_launcher();
    static QString input_VASP_POSCAR();
    static QString input_VASP_KPOINTS();
    static QString input_VASP_INCAR();

    //////////////////////
    // Input Generators //
    //////////////////////

    static void input_VASP_POTCAR(XtalOpt *p, int optIndex);
    static void buildVASP_POTCAR(XtalOpt *p);
  };
}

#endif

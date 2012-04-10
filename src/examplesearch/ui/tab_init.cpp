/**********************************************************************
  ExampleSearch -- A tool for analysing a matrix-substrate docking problem

  Copyright (C) 2012 by David C. Lonie

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <examplesearch/ui/tab_init.h>

#include <examplesearch/ui/dialog.h>
#include <examplesearch/examplesearch.h>

#include <globalsearch/macros.h>

#include <avogadro/moleculefile.h>

#include <QtCore/QSettings>

#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

using namespace std;
using namespace Avogadro;

namespace ExampleSearch {

  TabInit::TabInit( ExampleSearchDialog *dialog, ExampleSearch *opt ) :
    AbstractTab(dialog, opt)
  {
    ui.setupUi(m_tab_widget);


    initialize();
  }

  TabInit::~TabInit()
  {
  }

  void TabInit::lockGUI()
  {
  }

  void TabInit::updateParams()
  {
  }

}

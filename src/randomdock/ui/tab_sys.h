/**********************************************************************
  RandomDock -- A tool for analysing a matrix-substrate docking problem

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

#ifndef TAB_SYS_H
#define TAB_SYS_H

#include <globalsearch/ui/abstracttab.h>

#include "ui_tab_sys.h"

namespace RandomDock {
  class RandomDockDialog;
  class RandomDock;

  class TabSys : public GlobalSearch::AbstractTab
  {
    Q_OBJECT

  public:
    explicit TabSys( RandomDockDialog *dialog, RandomDock *opt );
    virtual ~TabSys();

  public slots:
    void lockGUI();
    void readSettings(const QString &filename = "");
    void writeSettings(const QString &filename = "");
    void selectLocalPath();
    void updateSystemInfo();

  signals:

  private:
    Ui::Tab_Sys ui;
  };
}

#endif

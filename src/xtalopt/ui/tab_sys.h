/**********************************************************************
  XtalOpt - Tools for advanced crystal optimization

  Copyright (C) 2009-2010 by David Lonie

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

using namespace GlobalSearch;

namespace XtalOpt {
  class XtalOptDialog;
  class XtalOpt;

  class TabSys : public AbstractTab
  {
    Q_OBJECT

  public:
    explicit TabSys( XtalOptDialog *parent, XtalOpt *p );
    virtual ~TabSys();

  public slots:
    void lockGUI();
    void readSettings(const QString &filename = "");
    void writeSettings(const QString &filename = "");
    void updateGUI();
    void selectLocalPath();
    void selectGULPPath();
    void updateSystemInfo();

  signals:
    void dataChanged();

  private:
    Ui::Tab_Sys ui;
  };
}

#endif

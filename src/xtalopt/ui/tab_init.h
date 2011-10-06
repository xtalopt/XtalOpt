/**********************************************************************
  XtalOpt - Tools for advanced crystal optimization

  Copyright (C) 2009-2011 by David Lonie

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public icense for more details.
 ***********************************************************************/

#ifndef TAB_INIT_H
#define TAB_INIT_H

#include <globalsearch/ui/abstracttab.h>

#include "ui_tab_init.h"

namespace XtalOpt {
  class XtalOptDialog;
  class XtalOpt;

  class TabInit : public GlobalSearch::AbstractTab
  {
    Q_OBJECT

  public:
    explicit TabInit( XtalOptDialog *parent, XtalOpt *p );
    virtual ~TabInit();

    enum CompositionColumns
    {
      CC_SYMBOL = 0,
      CC_ATOMICNUM,
      CC_QUANTITY,
      CC_MASS,
      CC_MINRADIUS
    };

  public slots:
    void lockGUI();
    void readSettings(const QString &filename = "");
    void writeSettings(const QString &filename = "");
    void updateGUI();
    void getComposition(const QString & str);
    void updateComposition();
    void updateCompositionTable();
    void updateDimensions();
    void updateMinRadii();

  signals:

  private:
    Ui::Tab_Init ui;
  };
}

#endif

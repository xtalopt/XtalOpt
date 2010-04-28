/**********************************************************************
  RandomDock -- A tool for analysing a matrix-substrate docking problem

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

#ifndef TAB_SYS_H
#define TAB_SYS_H

#include "ui_tab_sys.h"

namespace Avogadro {
  class RandomDockDialog;
  class RandomDockParams;

  class TabSys : public QObject
  {
    Q_OBJECT

  public:
    explicit TabSys( RandomDockParams *p );
    virtual ~TabSys();

    QWidget *getTabWidget() {return m_tab_widget;};

  public slots:
    void readSettings();
    void writeSettings();
    void updateSystemInfo();

  signals:

  private:
    Ui::Tab_Sys ui;
    QWidget *m_tab_widget;
    RandomDockParams *m_params;
  };
}

#endif

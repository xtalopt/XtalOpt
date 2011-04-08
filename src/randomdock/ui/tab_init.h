/**********************************************************************
  RandomDock -- A tool for analysing a matrix-substrate docking problem

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

namespace RandomDock {
  class RandomDockDialog;
  class RandomDock;
  class Substrate;
  class Matrix;

  class TabInit : public GlobalSearch::AbstractTab
  {
    Q_OBJECT

  public:
    explicit TabInit( RandomDockDialog *dialog, RandomDock *opt );
    virtual ~TabInit();

    enum Columns {
      Num = 0,
      Stoich,
      Filename
    };

  public slots:
    void lockGUI();
    void updateParams();
    void substrateBrowse();
    void substrateCurrent();
    void matrixAdd();
    void matrixRemove();
    void matrixCurrent();

  signals:
    void substrateChanged(Substrate*);
    void matrixAdded(Matrix*);
    void matrixRemoved();

  private:
    Ui::Tab_Init ui;
  };
}

#endif

/**********************************************************************
  TabEdit - Interface to edit optimization templates

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

#ifndef TAB_EDIT_H
#define TAB_EDIT_H

#include "ui_tab_edit.h"

#include <gapc/gapc.h>

#include <globalsearch/ui/abstracttab.h>

#include <QMessageBox>

namespace GlobalSearch {
  class Optimizer;
}

using namespace GlobalSearch;

namespace GAPC {
  class GAPCDialog;

  class TabEdit : public AbstractTab
  {
    Q_OBJECT

  public:
    enum ADF_Templates {
      ADFT_pbs = 0
    };

    explicit TabEdit( GAPCDialog *parent, OptGAPC *p );
    virtual ~TabEdit();

  public slots:
    void lockGUI();
    void readSettings(const QString &filename = "");
    void writeSettings(const QString &filename = "");
    void updateGUI();
    void templateChanged(int ind);
    void showHelp();
    void updateTemplates();
    void populateOptList();
    void appendOptStep();
    void removeCurrentOptStep();
    void optStepChanged();
    void saveScheme();
    void loadScheme();

  signals:
    void optimizerChanged(Optimizer*);

  private slots:
    void updateUserValues();
    void updateOptType();

  private:
    Ui::Tab_Edit ui;
  };
}

#endif

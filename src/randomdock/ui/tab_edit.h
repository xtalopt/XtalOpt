/**********************************************************************
  RandomDock -- A tool for analysing a matrix-substrate docking problem

  Copyright (C) 2009-2010 by David Lonie

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

#ifndef TAB_EDIT_H
#define TAB_EDIT_H

#include "ui_tab_edit.h"

#include <randomdock/randomdock.h>

#include <QMessageBox>

namespace Avogadro {
  class Optimizer;
}  

using namespace Avogadro;

namespace RandomDock {
  class RandomDockDialog;

  class TabEdit : public QObject
  {
    Q_OBJECT

  public:
    explicit TabEdit( RandomDockDialog *parent, RandomDock *p );
    virtual ~TabEdit();

    enum GAMESS_Templates {
      GAMT_pbs = 0,
      GAMT_inp
    };

    QWidget *getTabWidget() {return m_tab_widget;};

    public slots:
    void lockGUI();
    void readSettings(const QString &filename = "");
    void writeSettings(const QString &filename = "");
    void updateGUI();
    void disconnectGUI();
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
    QWidget *m_tab_widget;
    RandomDockDialog *m_dialog;
    RandomDock *m_opt;
  };
}

#endif

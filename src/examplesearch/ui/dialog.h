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
  GNU General Public icense for more details.
 ***********************************************************************/

#ifndef EXAMPLESEARCHDIALOG_H
#define EXAMPLESEARCHDIALOG_H

#include <examplesearch/examplesearch.h>

#include <globalsearch/ui/abstractdialog.h>

#include "ui_dialog.h"

namespace Avogadro {
  class Molecule;
  class GLWidget;
}

namespace ExampleSearch {
  class TabInit;
  class TabEdit;
  class TabProgress;
  class TabPlot;
  class TabLog;
  class ExampleSearch;

  class ExampleSearchDialog : public GlobalSearch::AbstractDialog
  {
    Q_OBJECT

  public:

    explicit ExampleSearchDialog( Avogadro::GLWidget *glWidget = 0,
                               QWidget *parent = 0,
                               Qt::WindowFlags f = 0 );
    virtual ~ExampleSearchDialog();

  public slots:
    void saveSession();

  private slots:
    void startSearch();

  signals:

  private:
    Ui::ExampleSearchDialog ui;

    TabInit *m_tab_init;
    TabEdit *m_tab_edit;
    TabProgress *m_tab_progress;
    TabPlot *m_tab_plot;
    TabLog *m_tab_log;
  };
}

#endif

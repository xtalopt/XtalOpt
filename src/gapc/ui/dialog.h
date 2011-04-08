/**********************************************************************
  GAPC -- A genetic algorithm for protected clusters

  Copyright (C) 2010-2011 by David Lonie

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public icense for more details.
 ***********************************************************************/

#ifndef GAPCDIALOG_H
#define GAPCDIALOG_H

#include <QDialog>
#include <QMutex>
#include <QTimer>

#include <globalsearch/ui/abstractdialog.h>

#include <avogadro/molecule.h>
#include <avogadro/glwidget.h>

#include "ui_dialog.h"

namespace GAPC {
  class ProtectedCluster;
  class GAPC;
  class TabInit;
  class TabEdit;
  class TabOpt;
  class TabSys;
  class TabProgress;
  class TabPlot;
  class TabLog;

  class GAPCDialog : public GlobalSearch::AbstractDialog
  {
    Q_OBJECT

  public:
    explicit GAPCDialog( Avogadro::GLWidget *glWidget = 0,
                         QWidget *parent = 0,
                         Qt::WindowFlags f = 0 );
    virtual ~GAPCDialog();

    void setMolecule(Avogadro::Molecule *molecule);

  public slots:
    void saveSession();

  private slots:
    void startSearch();

  signals:

  private:
    Ui::GAPCDialog ui;

    TabInit *m_tab_init;
    TabEdit *m_tab_edit;
    TabOpt *m_tab_opt;
    TabSys *m_tab_sys;
    TabProgress *m_tab_progress;
    TabPlot *m_tab_plot;
    TabLog *m_tab_log;
  };
}

#endif

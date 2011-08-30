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

#ifndef XTALOPTDIALOG_H
#define XTALOPTDIALOG_H

#include <globalsearch/ui/abstractdialog.h>

#include <avogadro/molecule.h>
#include <avogadro/glwidget.h>
#include <QtCore/QMutex>
#include <QtCore/QTimer>

#include <QtGui/QDialog>

namespace Ui {class XtalOptDialog;};

namespace XtalOpt {
  class Xtal;
  class XtalOpt;
  class TabInit;
  class TabEdit;
  class TabOpt;
  class TabSys;
  class TabProgress;
  class TabPlot;
  class TabLog;
  class XtalOptTest;

  class XtalOptDialog : public GlobalSearch::AbstractDialog
  {
    Q_OBJECT

  public:
    // Setting interactive to false will disable the tutorial popup
    explicit XtalOptDialog( Avogadro::GLWidget *glWidget = 0,
                            QWidget *parent = 0,
                            Qt::WindowFlags f = 0,
                            bool interactive = true);
    virtual ~XtalOptDialog();

    void setMolecule(Avogadro::Molecule *molecule);

  public slots:
    void saveSession();
    void showTutorialDialog() const;

  private slots:
    void startSearch();

  signals:

  private:
    Ui::XtalOptDialog *ui;

    TabInit *m_tab_init;
    TabEdit *m_tab_edit;
    TabOpt *m_tab_opt;
    TabSys *m_tab_sys;
    TabProgress *m_tab_progress;
    TabPlot *m_tab_plot;
    TabLog *m_tab_log;

    XtalOptTest *m_test;
  };
}

#endif

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
  GNU General Public License for more details.
 ***********************************************************************/

#include <examplesearch/ui/dialog.h>

#include <examplesearch/ui/tab_init.h>
#include <examplesearch/ui/tab_edit.h>
#include <examplesearch/ui/tab_progress.h>
#include <examplesearch/ui/tab_plot.h>
#include <examplesearch/ui/tab_log.h>

#include <globalsearch/macros.h>

#include <QtCore/QTimer>
#include <QtCore/QSettings>
#include <QtCore/QtConcurrentRun>

#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QProgressDialog>

using namespace std;
using namespace Avogadro;

namespace ExampleSearch {

  ExampleSearchDialog::ExampleSearchDialog(GLWidget *glWidget,
                                     QWidget *parent,
                                     Qt::WindowFlags f ) :
    AbstractDialog( glWidget, parent, f )
  {
    ui.setupUi(this);
    ui_push_begin   = ui.push_begin;
    ui_push_save    = ui.push_save;
    ui_push_resume  = ui.push_resume;
    ui_label_opt    = ui.label_opt;
    ui_label_run    = ui.label_run;
    ui_label_fail   = ui.label_fail;
    ui_label_prog   = ui.label_prog;
    ui_progbar      = ui.progbar;
    ui_tabs         = ui.tabs;

    // Disable save/resume
    ui.push_save->setVisible(false);
    ui.push_resume->setVisible(false);

    // Initialize vars, connections, etc
    ExampleSearch *examplesearch = new ExampleSearch (this);
    m_opt = examplesearch;

    // Initialize tabs
    m_tab_init		= new TabInit(this, examplesearch);
    m_tab_edit		= new TabEdit(this, examplesearch);
    m_tab_progress	= new TabProgress(this, examplesearch);
    m_tab_plot		= new TabPlot(this, examplesearch);
    m_tab_log		= new TabLog(this, examplesearch);

    // Populate tab widget
    ui.tabs->clear();
    ui.tabs->addTab(m_tab_init->getTabWidget(),		tr("&Structures"));
    ui.tabs->addTab(m_tab_edit->getTabWidget(),         tr("Optimization &Templates"));
    ui.tabs->addTab(m_tab_progress->getTabWidget(),     tr("&Progress"));
    ui.tabs->addTab(m_tab_plot->getTabWidget(),         tr("P&lot"));
    ui.tabs->addTab(m_tab_log->getTabWidget(),          tr("&Log"));

    initialize();
  }

  ExampleSearchDialog::~ExampleSearchDialog()
  {
    this->hide();

    m_opt->tracker()->lockForRead();
    writeSettings();
    saveSession();
    m_opt->tracker()->unlock();
  }

  void ExampleSearchDialog::saveSession()
  {
    // Notify if this was user requested.
    bool notify = false;
    if (sender() == ui.push_save) {
      notify = true;
    }
    if (m_opt->savePending) {
      return;
    }
    m_opt->savePending = true;
    QtConcurrent::run(m_opt,
                      &GlobalSearch::OptBase::save,
                      QString(""),
                      notify);
  }

  void ExampleSearchDialog::startSearch()
  {
    QtConcurrent::run(qobject_cast<ExampleSearch*>(m_opt),
                      &ExampleSearch::startSearch);
  }

}

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
  GNU General Public License for more details.
 ***********************************************************************/

#include <xtalopt/ui/dialog.h>

#include <globalsearch/optimizer.h>
#include <xtalopt/testing/xtalopttest.h>

#include <xtalopt/ui/tab_init.h>
#include <xtalopt/ui/tab_edit.h>
#include <xtalopt/ui/tab_opt.h>
#include <xtalopt/ui/tab_sys.h>
#include <xtalopt/ui/tab_progress.h>
#include <xtalopt/ui/tab_plot.h>
#include <xtalopt/ui/tab_log.h>

#include <openbabel/oberror.h>

#include <QtCore/QSettings>
#include <QtCore/QtConcurrentRun>

#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

using namespace GlobalSearch;
using namespace Avogadro;
using namespace std;

namespace XtalOpt {

  XtalOptDialog::XtalOptDialog( GLWidget *glWidget,
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

    // Initialize vars, connections, etc
    XtalOpt *xtalopt = new XtalOpt (this);

    m_opt = xtalopt;

    // Initialize tabs
    m_tab_init		= new TabInit(this, xtalopt);
    m_tab_edit		= new TabEdit(this, xtalopt);
    m_tab_opt		= new TabOpt(this, xtalopt);
    m_tab_sys		= new TabSys(this, xtalopt);
    m_tab_progress	= new TabProgress(this, xtalopt);
    m_tab_plot		= new TabPlot(this, xtalopt);
    m_tab_log		= new TabLog(this, xtalopt);

    // Populate tab widget
    ui.tabs->clear();
    ui.tabs->addTab(m_tab_init->getTabWidget(),		tr("Cell &Initialization"));
    ui.tabs->addTab(m_tab_edit->getTabWidget(),         tr("Optimization &Templates"));
    ui.tabs->addTab(m_tab_opt->getTabWidget(),          tr("&Optimization Settings"));
    ui.tabs->addTab(m_tab_sys->getTabWidget(),		tr("&System Settings"));
    ui.tabs->addTab(m_tab_progress->getTabWidget(),     tr("&Progress"));
    ui.tabs->addTab(m_tab_plot->getTabWidget(),         tr("&Plot"));
    ui.tabs->addTab(m_tab_log->getTabWidget(),          tr("&Log"));

    initialize();
  }

  XtalOptDialog::~XtalOptDialog()
  {
    if (m_opt->saveOnExit) {
      m_opt->tracker()->lockForRead();
      writeSettings();
      saveSession();
      m_opt->tracker()->unlock();
    }
    // m_opt is deleted by ~AbstractDialog
  }

  void XtalOptDialog::setMolecule(Molecule *molecule)
  {
    if (m_molecule == molecule || !molecule) {
      return;
    }
    m_molecule = molecule;
  }

  void XtalOptDialog::saveSession() {
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
                      &OptBase::save,
                      QString(""),
                      notify);
  }

  void XtalOptDialog::startSearch() {
    if (m_opt->testingMode) {
      m_test = new XtalOptTest(qobject_cast<XtalOpt*>(m_opt));
      QtConcurrent::run(m_test, &XtalOptTest::start);
    }
    else {
      QtConcurrent::run(qobject_cast<XtalOpt*>(m_opt),
                        &XtalOpt::startSearch);
    }
  }

  void XtalOptDialog::startProgressTimer()
  {
    m_tab_progress->startTimer();
  };

}

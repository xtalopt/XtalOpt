/**********************************************************************
  RandomDock -- A tool for analysing a matrix-substrate docking problem

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

#include <randomdock/ui/dialog.h>

#include <randomdock/ui/tab_init.h>
#include <randomdock/ui/tab_conformers.h>
#include <randomdock/ui/tab_params.h>
#include <randomdock/ui/tab_edit.h>
#include <randomdock/ui/tab_sys.h>
#include <randomdock/ui/tab_progress.h>
#include <randomdock/ui/tab_plot.h>
#include <randomdock/ui/tab_log.h>

#include <globalsearch/macros.h>

#include <QTimer>
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QtConcurrentRun>
#include <QProgressDialog>

using namespace std;
using namespace Avogadro;

namespace RandomDock {

  RandomDockDialog::RandomDockDialog(GLWidget *glWidget, QWidget *parent, Qt::WindowFlags f ) :
    QDialog( parent, f ), m_glWidget(glWidget)
  {
    ui.setupUi(this);

    // Initialize vars, connections, etc
    m_opt = new RandomDock (this);
    progMutex = new QMutex;
    progTimer = new QTimer;

    // Initialize tabs
    m_tab_init		= new TabInit(this, m_opt);
    m_tab_conformers	= new TabConformers(this, m_opt);
    m_tab_edit		= new TabEdit(this, m_opt);
    m_tab_params	= new TabParams(this, m_opt);
    m_tab_sys		= new TabSys(this, m_opt);
    m_tab_progress	= new TabProgress(this, m_opt);
    m_tab_plot		= new TabPlot(this, m_opt);
    m_tab_log		= new TabLog(this, m_opt);

    // Populate tab widget
    ui.tabs->clear();
    ui.tabs->addTab(m_tab_init->getTabWidget(),		tr("&Structures"));
    ui.tabs->addTab(m_tab_conformers->getTabWidget(),  	tr("&Conformers"));
    ui.tabs->addTab(m_tab_edit->getTabWidget(), 	tr("Optimization &Templates"));
    ui.tabs->addTab(m_tab_params->getTabWidget(),  	tr("&Search Settings"));
    ui.tabs->addTab(m_tab_sys->getTabWidget(),		tr("S&ystem Settings"));
    ui.tabs->addTab(m_tab_progress->getTabWidget(),  	tr("&Progress"));
    ui.tabs->addTab(m_tab_plot->getTabWidget(),  	tr("P&lot"));
    ui.tabs->addTab(m_tab_log->getTabWidget(),  	tr("&Log"));

    // Select the first tab by default
    ui.tabs->setCurrentIndex(0);

    // Hide the progress bar/label
    ui.label_prog->setVisible(false);
    ui.progbar->setVisible(false);

    // Connections
    connect(ui.push_begin, SIGNAL(clicked()),
            this, SLOT(startSearch()));

    connect(progTimer, SIGNAL(timeout()),
            this, SLOT(repaintProgressBar_()));
    connect(this, SIGNAL(sig_startProgressUpdate(const QString &, int, int)),
            this, SLOT(startProgressUpdate_(const QString &, int, int)));
    connect(this, SIGNAL(sig_stopProgressUpdate()),
            this, SLOT(stopProgressUpdate_()));
    connect(this, SIGNAL(sig_updateProgressMinimum(int)),
            this, SLOT(updateProgressMinimum_(int)));
    connect(this, SIGNAL(sig_updateProgressMaximum(int)),
            this, SLOT(updateProgressMaximum_(int)));
    connect(this, SIGNAL(sig_updateProgressValue(int)),
            this, SLOT(updateProgressValue_(int)));
    connect(this, SIGNAL(sig_updateProgressLabel(const QString &)),
            this, SLOT(updateProgressLabel_(const QString &)));
    connect(this, SIGNAL(sig_repaintProgressBar()),
            this, SLOT(repaintProgressBar_()));

    connect(m_opt, SIGNAL(warningStatement(const QString &)),
            this, SLOT(newWarning(const QString &)));
    connect(m_opt, SIGNAL(debugStatement(const QString &)),
            this, SLOT(newDebug(const QString &)));
    connect(m_opt, SIGNAL(errorStatement(const QString &)),
            this, SLOT(newError(const QString &)));
    connect(this, SIGNAL(sig_errorBox(const QString &)),
            this, SLOT(errorBox_(const QString &)));

    // Cross-tab connections
    // Update structure in conformer tab when sub/mat change
    connect(m_tab_init, SIGNAL(substrateChanged(Substrate*)),
            m_tab_conformers, SLOT(updateStructureList()));
    connect(m_tab_init, SIGNAL(matrixAdded(Matrix*)),
            m_tab_conformers, SLOT(updateStructureList()));
    connect(m_tab_init, SIGNAL(matrixRemoved()),
            m_tab_conformers, SLOT(updateStructureList()));

    readSettings();
  }

  RandomDockDialog::~RandomDockDialog()
  {
    writeSettings();
  }

  void RandomDockDialog::saveSession()
  {
    // TODO more to do here?
    m_opt->save();
  }

  void RandomDockDialog::resumeSession()
  {
    // TODO is this right still?
    QString filename;
    QFileDialog dialog (NULL, QString("Select .state file to resume"), m_opt->filePath, "*.state;;*.*");
    dialog.selectFile(m_opt->filePath + "/" + "randomdock.state");
    dialog.setFileMode(QFileDialog::ExistingFile);
    if (dialog.exec())
      filename = dialog.selectedFiles().first();
    else { return;} // User cancel file selection.
    m_opt->reset();
    m_opt->blockSignals(true);
    m_opt->load(filename);
    m_opt->blockSignals(false);
    // Refresh dialog and settings
    writeSettings();
    readSettings();
  }

  void RandomDockDialog::writeSettings(const QString &filename)
  {
    emit tabsWriteSettings(filename);
  }

  void RandomDockDialog::readSettings(const QString &filename)
  {
    emit tabsReadSettings(filename);
  }

  void RandomDockDialog::disconnectGUI()
  {
    emit tabsDisconnectGUI();
    disconnect(m_opt, SIGNAL(sessionStarted()),
               this, SLOT(updateGUI()));
    disconnect(this, SIGNAL(sig_updateStatus(int,int,int)),
               this, SLOT(updateStatus_(int,int,int)));
  }

  void RandomDockDialog::lockGUI()
  {
    // ui.push_resume->setDisabled(true);
    ui.push_begin->setDisabled(true);
    emit tabsLockGUI();
  }

  void RandomDockDialog::updateGUI()
  {
    setWindowTitle(QString("RandomDock - %1 @ %2")
                   .arg(m_opt->description)
                   .arg(m_opt->host)
                   );
    emit tabsUpdateGUI();
  }

  void RandomDockDialog::newDebug(const QString & s)
  {
    newLog("Debug: " + s);
  }

  void RandomDockDialog::newWarning(const QString & s)
  {
    newLog("Warning: " + s);
  }

  void RandomDockDialog::newError(const QString & s)
  {
    newLog("Error: " + s);
    errorBox(s);
  }

  void RandomDockDialog::updateStatus(int opt, int run, int fail)
  {
    emit sig_updateStatus(opt,run,fail);
  }

  void RandomDockDialog::updateStatus_(int opt, int run, int fail)
  {
    ui.label_opt->setText(QString::number(opt));
    ui.label_run->setText(QString::number(run));
    ui.label_fail->setText(QString::number(fail));
  }

  void RandomDockDialog::startSearch()
  {
    QtConcurrent::run(m_opt, &RandomDock::startSearch);
  }

  void RandomDockDialog::startProgressUpdate(const QString & text, int min, int max)
  {
    emit sig_startProgressUpdate(text,min,max);
  }

  void RandomDockDialog::startProgressUpdate_(const QString & text, int min, int max)
  {
    progMutex->lock();
    ui.progbar->reset();
    ui.progbar->setRange(min, max);
    ui.progbar->setValue(min);
    ui.label_prog->setText(text);
    ui.progbar->setVisible(true);
    ui.label_prog->setVisible(true);
    repaintProgressBar();
    progTimer->start(1000);
  }

  void RandomDockDialog::stopProgressUpdate()
  {
    emit sig_stopProgressUpdate();
  }

  void RandomDockDialog::stopProgressUpdate_()
  {
    ui.progbar->reset();
    ui.label_prog->setText("");
    ui.progbar->setVisible(false);
    ui.label_prog->setVisible(false);
    progTimer->stop();
    progMutex->unlock();
    repaintProgressBar();
  }

  void RandomDockDialog::updateProgressMaximum(int max)
  {
    emit sig_updateProgressMaximum(max);
  }

  void RandomDockDialog::updateProgressMaximum_(int max)
  {
    ui.progbar->setMaximum(max);
    repaintProgressBar();
  }

  void RandomDockDialog::updateProgressMinimum(int min)
  {
    emit sig_updateProgressMinimum(min);
  }

  void RandomDockDialog::updateProgressMinimum_(int min)
  {
    ui.progbar->setMinimum(min);
    repaintProgressBar();
  }

  void RandomDockDialog::updateProgressValue(int val)
  {
    emit sig_updateProgressValue(val);
  }

  void RandomDockDialog::updateProgressValue_(int val)
  {
    ui.progbar->setValue(val);
    repaintProgressBar();
  }

  void RandomDockDialog::updateProgressLabel(const QString & text)
  {
    emit sig_updateProgressLabel(text);
  }

  void RandomDockDialog::updateProgressLabel_(const QString & text)
  {
    ui.label_prog->setText(text);
    repaintProgressBar();
  }

  void RandomDockDialog::repaintProgressBar()
  {
    emit sig_repaintProgressBar();
  }

  void RandomDockDialog::repaintProgressBar_()
  {
    ui.label_prog->repaint();
    ui.progbar->repaint();
  }

  void RandomDockDialog::errorBox(const QString & s)
  {
    emit sig_errorBox(s);
  }

  void RandomDockDialog::errorBox_(const QString & s)
  {
    QMessageBox::critical(this, "Error", s);
  }

}

//#include "randomdockdialog.moc"

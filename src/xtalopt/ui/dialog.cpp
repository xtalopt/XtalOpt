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

#include "dialog.h"

#include "../../generic/optimizer.h"
#include "../testing/xtalopttest.h"

#include "tab_init.h"
#include "tab_edit.h"
#include "tab_opt.h"
#include "tab_sys.h"
#include "tab_progress.h"
#include "tab_plot.h"
#include "tab_log.h"

#include <avogadro/glwidget.h>

#include <openbabel/oberror.h>

#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QtConcurrentRun>
#include <QSemaphore>

using namespace std;

namespace Avogadro {

  XtalOptDialog::XtalOptDialog( GLWidget *glWidget, QWidget *parent, Qt::WindowFlags f ) :
    QDialog( parent, f ), m_opt(0), m_glWidget(glWidget)
  {
    // qDebug()
    //   << "XtalOptDialog::XtalOptDialog( "
    //   << parent << ", "
    //   << f << ") "
    //   << "called.";
    ui.setupUi(this);

    // Turn off OB error logging
    OpenBabel::obErrorLog.StopLogging();

    // Initialize vars, connections, etc
    m_opt = new XtalOpt (this);
    progMutex = new QMutex;
    progTimer = new QTimer;

    // Initialize tabs
    m_tab_init		= new TabInit(this, m_opt);
    m_tab_edit		= new TabEdit(this, m_opt);
    m_tab_opt		= new TabOpt(this, m_opt);
    m_tab_sys		= new TabSys(this, m_opt);
    m_tab_progress	= new TabProgress(this, m_opt);
    m_tab_plot		= new TabPlot(this, m_opt);
    m_tab_log		= new TabLog(this, m_opt);

    // Populate tab widget
    ui.tabs->clear();
    ui.tabs->addTab(m_tab_init->getTabWidget(),		tr("Cell &Initialization"));
    ui.tabs->addTab(m_tab_edit->getTabWidget(),         tr("Optimization &Templates"));
    ui.tabs->addTab(m_tab_opt->getTabWidget(),          tr("&Optimization Settings"));
    ui.tabs->addTab(m_tab_sys->getTabWidget(),		tr("&System Settings"));
    ui.tabs->addTab(m_tab_progress->getTabWidget(),     tr("&Progress"));
    ui.tabs->addTab(m_tab_plot->getTabWidget(),         tr("&Plot"));
    ui.tabs->addTab(m_tab_log->getTabWidget(),          tr("&Log"));

    // Select the first tab by default
    ui.tabs->setCurrentIndex(0);

    // Hide the progress bar/label
    ui.label_prog->setVisible(false);
    ui.progbar->setVisible(false);

    // Connections
    connect(ui.push_begin, SIGNAL(clicked()),
            this, SLOT(startOptimization()));
    connect(ui.push_save, SIGNAL(clicked()),
            this, SLOT(saveSession()));
    connect(ui.push_resume, SIGNAL(clicked()),
            this, SLOT(resumeSession()));
    connect(m_opt->tracker(), SIGNAL(newStructureAdded(Structure*)),
            this, SLOT(saveSession()));
    connect(m_opt, SIGNAL(newInfoUpdate()),
            this, SLOT(saveSession()));
    connect(m_opt, SIGNAL(sessionStarted()),
            this, SLOT(updateGUI()));
    connect(m_opt, SIGNAL(sessionStarted()),
            this, SLOT(lockGUI()));
    connect(m_opt->queue(), SIGNAL(newStatusOverview(int,int,int)),
            this, SLOT(updateStatus(int,int,int)));
    connect(this, SIGNAL(sig_updateStatus(int,int,int)),
            this, SLOT(updateStatus_(int,int,int)));

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

    readSettings();
  }

  XtalOptDialog::~XtalOptDialog()
  {
    //qDebug() << "XtalOptDialog::~XtalOptDialog() called";
    if (m_opt->saveOnExit) {
    m_opt->tracker()->lockForRead();
    writeSettings();
    saveSession();
    m_opt->tracker()->unlock();
    }
    delete m_opt;
  }

  void XtalOptDialog::disconnectGUI() {
    //qDebug() << "XtalOptDialog::disconnectGUI() called";
    emit tabsDisconnectGUI();
    disconnect(m_opt, SIGNAL(sessionStarted()),
               this, SLOT(updateGUI()));
    disconnect(this, SIGNAL(sig_updateStatus(int,int,int)),
               this, SLOT(updateStatus_(int,int,int)));
  }

  void XtalOptDialog::lockGUI() {
    //qDebug() << "XtalOptDialog::lockGUI() called";
    ui.push_resume->setDisabled(true);
    ui.push_begin->setDisabled(true);
    emit tabsLockGUI();
  }

  void XtalOptDialog::setGLWidget(GLWidget *w) {m_glWidget = w;}
  GLWidget* XtalOptDialog::getGLWidget() {return m_glWidget;}

  void XtalOptDialog::setMolecule(Molecule *molecule)
  {
    //qDebug() << "XtalOptDialog::setMolecule( " << molecule << ") " << "called";

    if (m_molecule == molecule || !molecule) {
      return;
    }
    m_molecule = molecule;

    // Populate m_comp with molecule if empty
    if (!m_opt->comp.isEmpty()) {
      QHash<uint, uint> comp;
      uint atomicNum;
      QList<Atom *> atoms = m_molecule->atoms();

      for (int i = 0; i < atoms.size(); i++) {
        atomicNum = atoms.at(i)->atomicNumber();

        if (!comp.contains(atomicNum)) comp[atomicNum] = 0;
        comp[atomicNum]++;
      }
      m_opt->comp = comp;
      emit m_tab_init->updateComposition();
    }
  }

  void XtalOptDialog::saveSession() {
    if (m_opt->savePending) {
      return;
    }
    m_opt->savePending = true;
    QtConcurrent::run(m_opt, &XtalOpt::save, QString(""));
  }

  void XtalOptDialog::resumeSession() {
    QMutexLocker locker (m_opt->stateFileMutex);
    QString filename;
    QFileDialog dialog (NULL, QString("Select .state file to resume"), m_opt->filePath, "*.state;;*.*");
    dialog.selectFile("xtalopt.state");
    dialog.setFileMode(QFileDialog::ExistingFile);
    if (dialog.exec())
      filename = dialog.selectedFiles().first();
    else { // User cancel file selection.
      return;
    }
    m_opt->emitStartingSession();
    startProgressUpdate(tr("Loading structures..."), 0, 0);
    m_opt->tracker()->deleteAllStructures();
    if (!m_opt->load(filename)) {
      stopProgressUpdate();
      m_opt->isStarting = false;
      return;
    }
    // Refresh dialog and settings
    writeSettings();
    stopProgressUpdate();
    m_opt->emitSessionStarted();
  }

  void XtalOptDialog::startOptimization() {
    //qDebug() << "XtalOptDialog::startOptimization() called";
    if (m_opt->testingMode) {
      m_test = new XtalOptTest(m_opt);
      QtConcurrent::run(m_test, &XtalOptTest::start);
    }
    else {
      QtConcurrent::run(m_opt, &XtalOpt::startOptimization);
    }
  }

  void XtalOptDialog::writeSettings(const QString &filename)
  {
    emit tabsWriteSettings(filename);
  }

  void XtalOptDialog::readSettings(const QString &filename)
  {
    emit tabsReadSettings(filename);
  }

  void XtalOptDialog::updateGUI() {
    setWindowTitle(QString("XtalOpt - %1 @ %2")
                   .arg(m_opt->description)
                   .arg(m_opt->host)
                   );
    emit tabsUpdateGUI();
  }

  void XtalOptDialog::updateStatus(int opt, int run, int fail) {
    // qDebug() << "XtalOptDialog::updateStatus( "
    //          << opt << ", "
    //          << run << ", "
    //          << fail << " ) called";
    emit sig_updateStatus(opt,run,fail);
  }

  void XtalOptDialog::updateStatus_(int opt, int run, int fail) {
    // qDebug() << "XtalOptDialog::updateStatus_( "
    //          << opt << ", "
    //          << run << ", "
    //          << fail << " ) called";
    ui.label_opt->setText(QString::number(opt));
    ui.label_run->setText(QString::number(run));
    ui.label_fail->setText(QString::number(fail));
  }

  void XtalOptDialog::startProgressUpdate(const QString & text, int min, int max) {
    //qDebug() << "XtalOptDialog::startProgressUpdate( " << text << ", " << min << ", " << max << ") called";
    emit sig_startProgressUpdate(text,min,max);
  }

  void XtalOptDialog::startProgressUpdate_(const QString & text, int min, int max) {
    //qDebug() << "XtalOptDialog::startProgressUpdate_( " << text << ", " << min << ", " << max << ") called";
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

  void XtalOptDialog::stopProgressUpdate() {
    //qDebug() << "XtalOptDialog::stopProgressUpdate() called";
    emit sig_stopProgressUpdate();
  }

  void XtalOptDialog::stopProgressUpdate_() {
    //qDebug() << "XtalOptDialog::stopProgressUpdate_() called";
    ui.progbar->reset();
    ui.label_prog->setText("");
    ui.progbar->setVisible(false);
    ui.label_prog->setVisible(false);
    progTimer->stop();
    progMutex->unlock();
    repaintProgressBar();
  }

  void XtalOptDialog::updateProgressMaximum(int max) {
    //qDebug() << "XtalOptDialog::updateProgressMaximum( " << max << " ) called";
    emit sig_updateProgressMaximum(max);
  }

  void XtalOptDialog::updateProgressMaximum_(int max) {
    //qDebug() << "XtalOptDialog::updateProgressMaximum_( " << max << " ) called";
    ui.progbar->setMaximum(max);
    repaintProgressBar();
  }

  void XtalOptDialog::updateProgressMinimum(int min) {
    //qDebug() << "XtalOptDialog::updateProgressMinimum( " << min << " ) called";
    emit sig_updateProgressMinimum(min);
  }

  void XtalOptDialog::updateProgressMinimum_(int min) {
    //qDebug() << "XtalOptDialog::updateProgressMinimum_( " << min << " ) called";
    ui.progbar->setMinimum(min);
    repaintProgressBar();
  }

  void XtalOptDialog::updateProgressValue(int val) {
    //qDebug() << "XtalOptDialog::updateProgressValue( " << val << " ) called";
    emit sig_updateProgressValue(val);
  }

  void XtalOptDialog::updateProgressValue_(int val) {
    //qDebug() << "XtalOptDialog::updateProgressValue_( " << val << " ) called";
    ui.progbar->setValue(val);
    repaintProgressBar();
  }

  void XtalOptDialog::updateProgressLabel(const QString & text) {
    //qDebug() << "XtalOptDialog::updateProgressLabel( " << text << " ) called";
    emit sig_updateProgressLabel(text);
  }

  void XtalOptDialog::updateProgressLabel_(const QString & text) {
    //qDebug() << "XtalOptDialog::updateProgressLabel_( " << text << " ) called";
    ui.label_prog->setText(text);
    repaintProgressBar();
  }

  void XtalOptDialog::repaintProgressBar() {
    emit sig_repaintProgressBar();
  }

  void XtalOptDialog::repaintProgressBar_() {
    ui.label_prog->repaint();
    ui.progbar->repaint();
  }

  void XtalOptDialog::newDebug(const QString & s) {
    newLog("Debug: " + s);
  }

  void XtalOptDialog::newWarning(const QString & s) {
    newLog("Warning: " + s);
  }

  void XtalOptDialog::newError(const QString & s) {
    newLog("Error: " + s);
    errorBox(s);
  }

  void XtalOptDialog::errorBox(const QString & s) {
    //qDebug() << "XtalOptDialog::errorBox( " << s << " ) called";
    emit sig_errorBox(s);
  }

  void XtalOptDialog::errorBox_(const QString & s) {
    //qDebug() << "XtalOptDialog::errorBox_( " << s << " ) called";
    QMessageBox::critical(this, "Error", s);
  }

}

//#include "dialog.moc"

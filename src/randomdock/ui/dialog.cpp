/**********************************************************************
  RandomDock -- A tool for analysing a matrix-substrate docking problem

  Copyright (C) 2009 by David Lonie

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include "randomdockdialog.h"

#include "randomdockGAMESS.h"

#include "tab_init.h"
#include "tab_conformers.h"
#include "tab_params.h"
#include "tab_edit.h"
#include "tab_sys.h"
#include "tab_results.h"
#include "tab_plot.h"
#include "tab_log.h"

#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QtConcurrentRun>
#include <QProgressDialog>

using namespace std;

namespace Avogadro {

  RandomDockDialog::RandomDockDialog( QWidget *parent, Qt::WindowFlags f ) :
    QDialog( parent, f ), m_params(0)
  {
    qDebug() 
      << "RandomDockDialog::RandomDockDialog( "
      << parent << ", "
      << f << ") "
      << "called.";
    ui.setupUi(this);

    // Initialize vars, connections, etc
    m_params = new RandomDockParams (this);

    // Initialize tabs
    m_tab_init		= new TabInit(m_params);
    m_tab_conformers	= new TabConformers(m_params);
    m_tab_edit		= new TabEdit(m_params);
    m_tab_params	= new TabParams(m_params);
    m_tab_sys		= new TabSys(m_params);
    m_tab_results	= new TabResults(m_params);
    m_tab_plot		= new TabPlot(m_params);
    m_tab_log		= new TabLog(m_params);

    // Populate tab widget
    ui.tabs->clear();
    ui.tabs->addTab(m_tab_init->getTabWidget(),		tr("&Structures"));
    ui.tabs->addTab(m_tab_conformers->getTabWidget(),  	tr("&Conformers"));
    ui.tabs->addTab(m_tab_edit->getTabWidget(), 	tr("Optimization &Templates"));
    ui.tabs->addTab(m_tab_params->getTabWidget(),  	tr("&Search Settings"));
    ui.tabs->addTab(m_tab_sys->getTabWidget(),		tr("&System Settings"));
    ui.tabs->addTab(m_tab_results->getTabWidget(),  	tr("&Results"));
    ui.tabs->addTab(m_tab_plot->getTabWidget(),  	tr("&Plot"));
    ui.tabs->addTab(m_tab_log->getTabWidget(),  	tr("&Log"));

    // Select the first tab by default
    ui.tabs->setCurrentIndex(0);

    // Connections
    connect(ui.push_begin, SIGNAL(clicked()),
            this, SLOT(startSearch()));

    // Cross-tab connections
    connect(m_tab_results, SIGNAL(cutoffReached()),
            m_tab_params, SLOT(stopSubmission()));

    readSettings();
  }

  RandomDockDialog::~RandomDockDialog()
  {
    qDebug() << "RandomDockDialog::~RandomDockDialog() called";
    writeSettings();
  }

  void RandomDockDialog::saveSession() {
    RandomDock::save(m_params);
  }

  void RandomDockDialog::resumeSession() {
    QString filename;
    QFileDialog dialog (NULL, QString("Select .state file to resume"), m_params->filePath, "*.state;;*.*");
    dialog.selectFile(m_params->filePath + "/" + m_params->fileBase + "randomdock.state");
    dialog.setFileMode(QFileDialog::ExistingFile);
    if (dialog.exec())
      filename = dialog.selectedFiles().first();
    else { return;} // User cancel file selection.
    m_params->reset();
    m_params->blockSignals(true);
    RandomDock::load(m_params, filename);
    m_params->blockSignals(false);
    // Refresh dialog and settings
    writeSettings();
    readSettings();
  }

  void RandomDockDialog::writeSettings() {
    qDebug() << "RandomDockDialog::writeSettings() called";
    QSettings settings; // Already set up in avogadro/src/main.cpp

    emit tabsWriteSettings();
  }

  void RandomDockDialog::readSettings() {
    qDebug() << "RandomDockDialog::readSettings() called";
    QSettings settings; // Already set up in avogadro/src/main.cpp

    emit tabsReadSettings();
  }

  void RandomDockDialog::updateScene(int ind) {
    qDebug() << "RandomDockDialog::updateScene( " << ind << " ) called";
    QtConcurrent::run(Avogadro::RandomDockGAMESS::readOutputFiles,
                      m_params->getSceneAt(ind), m_params);
    QtConcurrent::run(Avogadro::RandomDockGAMESS::stripOutputFile,
                      m_params->getSceneAt(ind), m_params);
  }

  void RandomDockDialog::errorScene(int ind) {
    qDebug() << "RandomDockDialog::errorScene( " << ind << " ) called";
    QtConcurrent::run(this, &RandomDockDialog::errorScene_, ind);
  }

  void RandomDockDialog::errorScene_(int ind) {
    qDebug() << "RandomDockDialog::errorScene_( " << ind << " ) called";

    m_params->rwLock->lockForRead();
    Scene *scene = m_params->getSceneAt(ind);
    m_params->rwLock->unlock();

    if (scene->getJobID()) // e.g. if scene has a job id!
      RandomDockGAMESS::deleteJob(scene, m_params);

    m_params->rwLock->lockForWrite();
    // Don'e restart job, just leave as errored and spawn a new job
    //    RandomDockGAMESS::writeInputFiles(scene, m_params);
    //    RandomDockGAMESS::startJob(scene, m_params);
    scene->setStatus(Scene::Killed);
    m_params->rwLock->unlock();
  }

  void RandomDockDialog::deleteJob(int ind) {
    qDebug() << "RandomDockDialog::deleteJob( " << ind << " ) called";
    QtConcurrent::run(&RandomDockGAMESS::deleteJob, 
                      m_params->getSceneAt(ind),
                      m_params);
  }

  void RandomDockDialog::killScene(int ind) {
    qDebug() << "RandomDockDialog::killScene( " << ind << " ) called";
    QtConcurrent::run(this, &RandomDockDialog::killScene_, ind);
  }

  void RandomDockDialog::killScene_(int ind) {
    qDebug() << "RandomDockDialog::killScene_( " << ind << " ) called";

    m_params->rwLock->lockForWrite();
    m_params->getSceneAt(ind)->setStatus(Scene::Killed);
    m_params->rwLock->unlock();
  }

  void RandomDockDialog::updateRunning(int i) {
    qDebug() << "RandomDockDialog::updateRunning( " << i << " ) called";
    ui.label_running->setText(QString::number(i));
  }

  void RandomDockDialog::updateOptimized(int i) {
    qDebug() << "RandomDockDialog::updateOptimized( " << i << " ) called";
    ui.label_optimized->setText(QString::number(i));
  }

}

//#include "randomdockdialog.moc"

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

  void RandomDockDialog::startSearch() {
    qDebug() << "RandomDockDialog::startSearch() called";

    if (!m_params->substrate || m_params->matrixList->size() == 0)
      return;

    emit newLog(tr("Starting search..."));

    // prepare pointers
    m_params->deleteAllScenes();

    ///////////////////////////////
    // Generate random structure //
    ///////////////////////////////

    // Set up progress dialog
    QProgressDialog prog(tr("Generating structures..."), tr("Abort"), 0, 0);
    prog.setWindowModality(Qt::WindowModal);
    prog.setMinimumDuration(2000);
    prog.setValue(0);

    // Initalize loop variables
    uint progCount = 0;
    QString filename;
    Scene *scene;

    // Prevent livelocks from spawned progress updates
    m_params->blockSignals(true);

    // Generation loop...
    for (uint i = 0; i < m_params->numSearches; i++) {
      // Update progress bar
      prog.setMaximum( (i == 0)
                       ? 0
                       : (progCount / static_cast<double>(i)) * m_params->numSearches );                       
      prog.setValue(progCount);
      progCount++;
      prog.setLabelText(tr("%1 scenes generated of (%2)...").arg(i).arg(m_params->numSearches));
      if (prog.wasCanceled()) {
        m_params->deleteAllScenes();
        emit newLog(tr("Random scene generation canceled"));
        return;
      }

      // Generate/Check molecule
      m_params->generateNewScene();
    }

    prog.setValue(prog.maximum());
    emit newLog(tr("%1 random scenes generated")
                .arg(m_params->numSearches));

    /////////////////
    // Submit jobs //
    /////////////////

    // Setup progress dialog
    QProgressDialog prog1(tr("Writing input files and submitting jobs..."), tr("Abort"), 0, 0); // Indeterminant for now...
    prog1.setWindowModality(Qt::WindowModal);
    prog1.setCancelButton(0);
    prog1.setMinimumDuration(0);
    prog1.setValue(0);

    QString id;

    m_params->rwLock->lockForWrite();
    for (int i = 0; i < m_params->getScenes()->size(); i++) {
      if (i != 0) { // So that something is shown during the first iteration, don't set max until iter 2
        prog1.setValue(i);
        prog1.setMaximum(m_params->getScenes()->size());
      }
      scene = m_params->getSceneAt(i);
      id.sprintf("%05d",scene->getSceneNumber());
      scene->setFileName(m_params->filePath + "/" + m_params->fileBase + "-" + id + "/");
      scene->setRempath(m_params->rempath + "/" + m_params->fileBase + "-" + id + "/");
      bool ok = RandomDockGAMESS::writeInputFiles(scene, m_params);
      if (ok) ok = RandomDockGAMESS::copyToRemote(scene, m_params);
      if (!ok) {
        QMessageBox::critical(this, tr("Something went horribly wrong..."),
                              tr("Cannot write structures to either local or remote disk. Check log for details."));
        newLog("Error during creation of input files. See stderr for details");
        m_params->rwLock->unlock();
        return;
      }

      if (!RandomDockGAMESS::startJob(scene, m_params)) {
        QMessageBox::critical(this, tr("Something went horribly wrong..."),
                              tr("Cannot submit files to queue manager. Check log for details."));
        newLog("Error during submission of input files. See stderr for details");
        m_params->rwLock->unlock();
        return;
      }
    }

    m_params->rwLock->unlock();
    // Re-allow progress updates
    m_params->blockSignals(false);

    m_tab_results->populateResultsTable();
    m_tab_results->startTimer();
    saveSession();
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

#include "randomdockdialog.moc"

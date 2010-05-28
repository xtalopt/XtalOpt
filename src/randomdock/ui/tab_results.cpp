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

#include "tab_results.h"

#include "randomdock.h"
#include "randomdockGAMESS.h"
#include "randomdockdialog.h"

#include <QFile>
#include <QTimer>
#include <QSettings>
#include <QMessageBox>
#include <QtConcurrentRun>

using namespace std;

namespace Avogadro {

  TabResults::TabResults( RandomDockParams *p ) :
    QObject( p->dialog ), m_params(p), m_timer(0), m_update_mutex(0), m_mutex(0)
  {
    qDebug() << "TabResults::TabResult( " << p <<  " ) called.";

    m_tab_widget = new QWidget;
    ui.setupUi(m_tab_widget);

    m_mutex = new QMutex;
    m_update_mutex = new QMutex;
    m_timer = new QTimer(this);

    QHeaderView *horizontal = ui.table_list->horizontalHeader();
    horizontal->setResizeMode(QHeaderView::ResizeToContents);

    // dialog connections
    connect(p->dialog, SIGNAL(tabsReadSettings()),
            this, SLOT(readSettings()));
    connect(p->dialog, SIGNAL(tabsWriteSettings()),
            this, SLOT(writeSettings()));
    connect(this, SIGNAL(newLog(QString)),
            p->dialog, SIGNAL(newLog(QString)));
    connect(this, SIGNAL(moleculeChanged(Molecule*)),
            p->dialog, SIGNAL(moleculeChanged(Molecule*)));
    connect(p->dialog, SIGNAL(moleculeChanged(Molecule*)),
            this, SLOT(highlightScene(Molecule*)));
    connect(this, SIGNAL(sceneFinished(int)),
            p->dialog, SLOT(updateScene(int)));
    connect(this, SIGNAL(sceneErrored(int)),
            p->dialog, SLOT(errorScene(int)));
    connect(this, SIGNAL(deleteJob(int)),
            p->dialog, SLOT(deleteJob(int)));
    connect(this, SIGNAL(killScene(int)),
            p->dialog, SLOT(killScene(int)));
    connect(this, SIGNAL(updateRunning(int)),
            p->dialog, SLOT(updateRunning(int)));
    connect(this, SIGNAL(updateOptimized(int)),
            p->dialog, SLOT(updateOptimized(int)));

    // Results tab connections
    connect(ui.table_list, SIGNAL(currentCellChanged(int,int,int,int)),
            this, SLOT(selectSceneFromResults(int,int,int,int)));
    connect(m_params, SIGNAL(sceneCountChanged()),
            this, SLOT(updateResultsTable()));
    connect(m_timer, SIGNAL(timeout()),
            this, SLOT(updateResultsTable()));
    connect(ui.push_refresh, SIGNAL(clicked()),
            this, SLOT(startTimer()));
    connect(ui.push_refresh, SIGNAL(clicked()),
            this, SLOT(updateResultsTable()));
    connect(this, SIGNAL(refresh()),
            this, SLOT(refreshResultsTable()));
  }

  TabResults::~TabResults()
  {
    qDebug() << "TabResults::~TabResults() called";
    delete m_mutex;
    delete m_update_mutex;
    delete m_timer;

  }

  void TabResults::startTimer() {
    if (m_timer->isActive())
      m_timer->stop();
    m_timer->start(15 * 1000);
  }

  void TabResults::stopTimer() {
    m_timer->stop();
  }

  void TabResults::writeSettings() {
    qDebug() << "TabResults::writeSettings() called";
    QSettings settings; // Already set up in avogadro/src/main.cpp
    // Nothing to do!
  }

  void TabResults::readSettings() {
    qDebug() << "TabResults::readSettings() called";
    QSettings settings; // Already set up in avogadro/src/main.cpp
    // Nothing to do!
  }

  void TabResults::updateResultsTable() {
    qDebug() << "TabResults::updateResultsTable( ) called";

    // Only allow one update at a time
    if (!m_update_mutex->tryLock()) {
      qDebug() << "Killing extra TabProgress::updateProgressTable() call";
      return;
    }

    if (!m_params->rwLock->tryLockForRead()){
      m_update_mutex->unlock();
      qDebug() << "Not updating progress table while Mutex is write locked...";
      return;
    }

    if (ui.table_list->rowCount() != m_params->getScenes()->size())
      populateResultsTable();

    if (m_params->getScenes()->size() == 0) {
      qDebug() << "Not updating table with no scene data!";
      m_params->rwLock->unlock();
      m_update_mutex->unlock();
      return;
    }

    uint running = 0;
    uint optimized = 0;
    // Check the number of running jobs
    for (int i = 0; i < m_params->getScenes()->size(); i++) {
      if ( m_params->getSceneAt(i)->getStatus() != Scene::Optimized &&
           m_params->getSceneAt(i)->getStatus() != Scene::Killed ) {
        running++;
      }
      if ( m_params->getSceneAt(i)->getStatus() == Scene::Optimized )
        optimized++;
    }

    // Update number of running and updated jobs
    emit updateRunning(running);
    emit updateOptimized(optimized);

    // If optimized is greater than the cutoff limit, stop submitting jobs.
    if (optimized >= m_params->cutoff && m_params->cutoff != 0)
      emit cutoffReached();

    m_params->rwLock->unlock();
    Scene* scene;
    bool locked = false;

    if (running < m_params->numSearches) {
      if (!m_params->rwLock->tryLockForWrite()) {
        qDebug() << "Not launching more jobs while rwLock is locked for write...wait for next loop.";
        m_update_mutex->unlock();
        return;
      }
      locked = true;
    }

    while (running < m_params->numSearches) {
      // Add a new structure to replace the one that optimized
      scene = m_params->generateNewScene();
      QString id;
      id.sprintf("%05d",scene->getSceneNumber());
      scene->setFileName(m_params->filePath + "/" + m_params->fileBase + "-" + id + "/");
      scene->setRempath(m_params->rempath + "/" + m_params->fileBase + "-" + id + "/");
      bool ok = RandomDockGAMESS::writeInputFiles(scene, m_params);
      if (ok) ok = RandomDockGAMESS::copyToRemote(scene, m_params);
      if (!ok) {
        QMessageBox::critical(m_params->dialog, tr("Something went horribly wrong..."),
                              tr("Cannot write structures to either local or remote disk. Check log for details."));
        newLog("Error during creation of input files. See stderr for details");
        delete m_params->getScenes()->takeLast();
        m_params->rwLock->unlock();
        m_update_mutex->unlock();
        return;
      }

      if (!RandomDockGAMESS::startJob(scene, m_params)) {
        QMessageBox::critical(m_params->dialog, tr("Something went horribly wrong..."),
                              tr("Cannot submit files to queue manager. Check log for details."));
        newLog("Error during submission of input files. See stderr for details");
        delete m_params->getScenes()->takeLast();
        m_params->rwLock->unlock();
        m_update_mutex->unlock();
        return;
      }
      running++;
    }
    if (locked)
      m_params->rwLock->unlock();

    emit refresh();
    QtConcurrent::run(this, &TabResults::dumpResults);
    m_update_mutex->unlock();
  }

  void TabResults::refreshResultsTable() {
    qDebug() << "TabResults::refreshResultsTable( ) called";

    if (m_params->getScenes()->size() == 0) return;

    if (!m_params->rwLock->tryLockForRead()) {
      qDebug() << "Not refreshing results table while rwLock is locked for write...";
      return;
    }

    int size = m_params->getScenes()->size();

    if (size != ui.table_list->rowCount())
      populateResultsTable();

    Scene *scene;

    // Lock the table
    QMutexLocker locker (m_mutex);

    // Get queue data
    QStringList queueData;
    if (!RandomDockGAMESS::getQueue(m_params, queueData)) {
      newLog("Cannot fetch queue -- check stderr");
      qWarning() << "Cannot fetch queue -- check stderr";
      m_params->rwLock->unlock();
      return;
    }

    for (int i = 0; i < size; i++) {
      scene = m_params->getSceneAt(i);

      int jobid = scene->getJobID();
      double energy = scene->getEnergy();
      if (energy)
        ui.table_list->item(i, Energy)->setText(QString::number(energy));
      else
        ui.table_list->item(i, Energy)->setText("--");

      switch (scene->getStatus()) {

      case Scene::InProcess:

        switch (RandomDockGAMESS::getStatus(scene, m_params, queueData)) {

        case RandomDockGAMESS::Running:
          ui.table_list->item(i, Status)->setText(tr("Running (JobID: %1)")
                                                  .arg(QString::number(jobid)));
          break;

        case RandomDockGAMESS::Queued:
          ui.table_list->item(i, Status)->setText(tr("Queued (JobID: %1)")
                                                  .arg(QString::number(jobid)));
          break;

        case RandomDockGAMESS::Success:
          emit sceneFinished(i);
          ui.table_list->item(i, Status)->setText("Starting update...");
          break;

        case RandomDockGAMESS::Unknown:
          ui.table_list->item(i, Status)->setText("Unknown");
          break;

        case RandomDockGAMESS::Error:
          qDebug() << "--------------- JOB ERROR! ---------------" << endl
                   << "Structure " << scene->getSceneNumber() << endl
                   << "JobID: " << jobid << endl
                   << "------------------------------------------";
          emit sceneErrored(i);
          ui.table_list->item(i, Status)->setText("Error occurred");
          break;
        case RandomDockGAMESS::CommunicationError:
          ui.table_list->item(i, Status)->setText("Comm. Error");
          break;
        }
        break;
      case Scene::Submitted:
        ui.table_list->item(i, Status)->setText("Scene submitted...");
        break;

      case Scene::Optimized:
        if (jobid) {
          emit deleteJob(i);
        }
        // if energy is == 0, something went wrong in the update. Kill scene.
        if (energy == 0.0)
          emit killScene(i);
        ui.table_list->item(i, Status)->setText("Optimized");
        break;

      case Scene::WaitingForOptimization:
        ui.table_list->item(i, Status)->setText("Waiting for Optimization");
        break;

      case Scene::Error:
        ui.table_list->item(i, Status)->setText(tr("Error occurred..."));
        emit sceneErrored(i);
        break;

      case Scene::Updating:
        ui.table_list->item(i, Status)->setText("Updating structure...");
        break;

      case Scene::Empty:
        ui.table_list->item(i, Status)->setText("Structure empty...");
        break;

      case Scene::Killed:
        ui.table_list->item(i, Status)->setText("Killed");
        break;

      }
    }
    m_params->rwLock->unlock();
  }

  void TabResults::populateResultsTable() {
    qDebug() << "TabResults::populateResultsTable( ) called";

    QMutexLocker locker (m_mutex);

    // Rank energies
    RandomDock::rankByEnergy(m_params->getScenes());

    QList<Scene*> *scenes = m_params->getScenes();

    // Clear old table data
    while (ui.table_list->rowCount() != 0)
      ui.table_list->removeRow(0);

    // Initialize vars for filling table
    Scene* scene;
    QTableWidgetItem *rank, *ind, *energy, *status;
    QString ind_s, rank_s;

    for (int i = 0; i < scenes->size(); i++) {
      scene = scenes->at(i);
      ind_s.sprintf("%04d", i);
      rank_s.sprintf("%04d", scene->getEnergyRank());
      ind               = new QTableWidgetItem ( ind_s );
      rank              = new QTableWidgetItem ( rank_s );
      energy		= new QTableWidgetItem ( QString::number(scene->getEnergy()) );
      status		= new QTableWidgetItem ( "TODO..." );

      uint row = ui.table_list->rowCount();

      ui.table_list->insertRow(row);
      ui.table_list->setItem(row, Index,	ind);
      ui.table_list->setItem(row, Rank,		rank);
      ui.table_list->setItem(row, Energy,	energy);
      ui.table_list->setItem(row, Status,	status);
    }
  }

  void TabResults::selectSceneFromResults(int row,int,int oldrow,int) {
    qDebug() << "TabResults::selectSceneFromResults( " << row << " " << oldrow << " ) called";
    if (row == -1) return;
    row = ui.table_list->item(row, Index)->text().toInt();
    emit moleculeChanged(m_params->getSceneAt(row));
  }

 void TabResults::highlightScene(Molecule *mol) {
    qDebug() << "TabResults::highlightScene( " << mol << " ) called.";
    m_params->rwLock->lockForRead();
    int ind = -1;
    for ( int i = 0; i < m_params->getScenes()->size(); i++) {
      if (m_params->getSceneAt(i) == mol) {
        ind = i;
        break;
      }
    }
    m_params->rwLock->unlock();
    ui.table_list->blockSignals(true);
    ui.table_list->setCurrentCell(ind, 0);
    ui.table_list->blockSignals(false);
    return;
  }

  void TabResults::dumpResults() {
    qDebug() << "TabResults::dumpResults() called";

    QReadLocker locker (m_params->rwLock);

    QString filename = m_params->filePath + "/" + m_params->fileBase + "-results.tsv";

    QFile file (filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
      qWarning() << "Cannot open file " << filename << " for writing!";
      return;
    }

    QTextStream out(&file);

    out << "Energy\tPath\n";

    // Iterate over all scenes in order of rank
    Scene* scene;
    // i = current rank
    for (int i = 0; i < m_params->getScenes()->size(); i++) {
      // Find scene with matching rank
      for (int j = 0; j < m_params->getScenes()->size(); j++) {
        scene = m_params->getSceneAt(j);
        if (scene->getEnergyRank() == i &&
            scene->getStatus() == Scene::Optimized) {
          // dump energy
          out << QString::number(scene->getEnergy(), 8, 'f');
          out << "\t";
          // filename
          out << scene->fileName();
          out << "\n";
        }
      }
    }

    file.close();

    return;
  }

}

//#include "tab_results.moc"

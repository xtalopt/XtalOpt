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

#include "tab_progress.h"

#include "dialog.h"
#include "../randomdock.h"
#include "../structures/scene.h"
#include "../../generic/optimizer.h"
#include "../../generic/queuemanager.h"
#include "../../generic/macros.h"

#include <QMenu>
#include <QTimer>
#include <QBrush>
#include <QSettings>
#include <QMutexLocker>
#include <QInputDialog>
#include <QtConcurrentRun>

using namespace std;
using namespace Avogadro;

namespace RandomDock {

  TabProgress::TabProgress( RandomDockDialog *parent, RandomDock *p ) :
    QObject(parent),
    m_dialog(parent),
    m_opt(p),
    m_timer(0),
    m_mutex(0),
    m_update_mutex(0),
    m_update_all_mutex(0),
    m_context_scene(0)
  {
    m_tab_widget = new QWidget;
    ui.setupUi(m_tab_widget);

    m_dialog = parent;

    QHeaderView *horizontal = ui.table_list->horizontalHeader();
    horizontal->setResizeMode(QHeaderView::ResizeToContents);

    m_mutex = new QMutex;
    m_update_mutex = new QMutex;
    m_update_all_mutex = new QMutex;
    m_timer = new QTimer(this);

    rowTracking = true;

    // dialog connections
    connect(m_dialog, SIGNAL(tabsReadSettings(const QString &)),
            this, SLOT(readSettings(const QString &)));
    connect(m_dialog, SIGNAL(tabsWriteSettings(const QString &)),
            this, SLOT(writeSettings(const QString &)));
    connect(m_dialog, SIGNAL(tabsUpdateGUI()),
            this, SLOT(updateGUI()));
    connect(m_dialog, SIGNAL(tabsDisconnectGUI()),
            this, SLOT(disconnectGUI()));
    connect(m_dialog, SIGNAL(tabsLockGUI()),
            this, SLOT(lockGUI()));
    connect(this, SIGNAL(newLog(QString)),
            m_dialog, SIGNAL(newLog(QString)));
    connect(this, SIGNAL(moleculeChanged(Structure*)),
            m_dialog, SIGNAL(moleculeChanged(Structure*)));
    connect(m_dialog, SIGNAL(moleculeChanged(Structure*)),
            this, SLOT(highlightScene(Structure*)));
    connect(this, SIGNAL(refresh()),
            m_opt->queue(), SLOT(checkRunning()));
    connect(this, SIGNAL(refresh()),
            m_opt->queue(), SLOT(checkPopulation()));
    connect(m_opt, SIGNAL(updateAllInfo()),
            this, SLOT(updateAllInfo()));

    // Progress table connections
    connect(m_timer, SIGNAL(timeout()),
            this, SLOT(updateProgressTable()));
    connect(ui.push_refresh, SIGNAL(clicked()),
            this, SLOT(startTimer()));
    connect(ui.push_refresh, SIGNAL(clicked()),
            this, SLOT(updateProgressTable()));
    connect(ui.spin_period, SIGNAL(editingFinished()),
            this, SLOT(updateProgressTable()));
    connect(ui.table_list, SIGNAL(currentCellChanged(int,int,int,int)),
            this, SLOT(selectMoleculeFromProgress(int,int,int,int)));
    connect(m_opt->tracker(), SIGNAL(newStructureAdded(Structure*)),
            this, SLOT(addNewEntry()));
    connect(m_opt->queue(), SIGNAL(structureUpdated(Structure*)),
            this, SLOT(newInfoUpdate(Structure *)));
    connect(this, SIGNAL(infoUpdate()),
            this, SLOT(updateInfo()));
    connect(ui.table_list, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(progressContextMenu(QPoint)));
    connect(ui.push_refreshAll, SIGNAL(clicked()),
            this, SLOT(updateAllInfo()));
    connect(m_opt, SIGNAL(startingSession()),
            this, SLOT(disableRowTracking()));
    connect(m_opt, SIGNAL(sessionStarted()),
            this, SLOT(enableRowTracking()));
  }

  TabProgress::~TabProgress()
  {
    delete m_mutex;
    delete m_update_mutex;
    delete m_update_all_mutex;
    delete m_timer;
  }

  void TabProgress::writeSettings(const QString &filename) {
    SETTINGS(filename);
    settings->beginGroup("randomdock/progress");
    settings->setValue("refreshTime", ui.spin_period->value());
    settings->endGroup();
    DESTROY_SETTINGS(filename);
  }

  void TabProgress::readSettings(const QString &filename) {
    SETTINGS(filename);
    settings->beginGroup("randomdock/progress");
    ui.spin_period->setValue(settings->value("refreshTime", 1).toInt());
    settings->endGroup();      
  }

  void TabProgress::updateGUI()
  {
  }

  void TabProgress::disconnectGUI() {
    m_timer->disconnect();
    ui.push_refresh->disconnect();
    ui.push_refreshAll->disconnect();
    ui.spin_period->disconnect();
    ui.table_list->disconnect();
    disconnect(m_opt->tracker(), 0, this, 0);
    disconnect(m_opt->queue(), 0, this, 0);
    disconnect(m_dialog, 0, this, 0);
    this->disconnect();
  }

  void TabProgress::lockGUI()
  {
  }

  void TabProgress::updateProgressTable() {
    // Only allow one update at a time
    if (!m_update_mutex->tryLock()) {
      return;
    }

    if (!m_opt) {
      m_update_mutex->unlock();
      return;
    }

    if (m_opt->tracker()->size() == 0) {
      m_update_mutex->unlock();
      return;
    }

    QtConcurrent::run(m_opt->queue(), &Avogadro::QueueManager::checkPopulation);

    emit refresh();
    m_update_mutex->unlock();
  }

  void TabProgress::addNewEntry() {
    QMutexLocker locker (m_mutex);

    // The new entry will be at the end of the table, so determine the index:
    int index = ui.table_list->rowCount();
    Scene *scene = qobject_cast<Scene*>(m_opt->tracker()->at(index));

    // Turn off signals
    ui.table_list->blockSignals(true);

    // Store current index for later. If -1, this will be re-set at the end of table
    int currentInd = ui.table_list->currentRow();
    if (currentInd >= ui.table_list->rowCount() - 1) currentInd = -1;

    // Add the new row
    ui.table_list->insertRow(index);
    // Columns: once for each column in ProgressColumns:
    for (int i = 0; i < 6; i++) {
      ui.table_list->setItem(index, i, new QTableWidgetItem());
    }

    m_infoUpdateTracker.append(scene);
    locker.unlock();
    emit infoUpdate();

    ui.table_list->blockSignals(false);

    if (currentInd < 0) currentInd = index;
    if (rowTracking) ui.table_list->setCurrentCell(currentInd, 0);
  }

  void TabProgress::updateAllInfo() {
    if (!m_update_all_mutex->tryLock()) {
      return;
    }
    QList<Structure*> *structures = m_opt->tracker()->list();
    for (int i = 0; i < ui.table_list->rowCount(); i++) {
      newInfoUpdate(structures->at(i));
    }
    m_update_all_mutex->unlock();
  }

  void TabProgress::newInfoUpdate(Structure *s) {
    m_infoUpdateTracker.append(s);
    emit infoUpdate();
  }

  void TabProgress::updateInfo() {
    if (m_infoUpdateTracker.size() == 0) {
      return;
    }

    // Don't update while a context operation is in the works
    if (m_context_scene !=0) {
      qDebug() << "TabProgress::updateInfo: Waiting for context operation to complete (" << m_context_scene << ") Trying again very soon.";
      QTimer::singleShot(1000, this, SLOT(updateInfo()));
      return;
    }

    // Lock the table
    QMutexLocker locker (m_mutex);

    // Prep variables
    Structure *structure;
    if (!m_infoUpdateTracker.popFirst(structure))
      return;
    int i = m_opt->tracker()->list()->indexOf(structure);

    Scene *scene = qobject_cast<Scene*>(structure);

    if (i < 0 || i > ui.table_list->rowCount() - 1) {
      qDebug() << "TabProgress::updateInfo: Trying to update an index that doesn't exist (" << i << ") Waiting...";
      m_infoUpdateTracker.append(scene);
      QTimer::singleShot(100, this, SLOT(updateInfo()));
      return;
    }

    uint totalOptSteps = m_opt->optimizer()->getNumberOfOptSteps();
    QBrush brush (Qt::white);

    // Get queue data
    m_opt->queue()->updateQueue();

    QReadLocker sceneLocker (scene->lock());

    ui.table_list->item(i, C_Rank)->setText(QString::number(scene->getRank()));
    ui.table_list->item(i, C_Index)->setText(QString::number(scene->getIndex()));
    ui.table_list->item(i, C_Elapsed)->setText(scene->getOptElapsed());

    if (scene->getJobID())
      ui.table_list->item(i, C_JobID)->setText(QString::number(scene->getJobID()));
    else
      ui.table_list->item(i, C_JobID)->setText("N/A");

    if (scene->getEnergy() != 0.0)
      ui.table_list->item(i, C_Energy)->setText(QString::number(scene->getEnergy()));
    else
      ui.table_list->item(i, C_Energy)->setText("N/A");

    switch (scene->getStatus()) {
    case Scene::InProcess: {
      sceneLocker.unlock();
      Optimizer::JobState state = m_opt->optimizer()->getStatus(scene);
      sceneLocker.relock();
      switch (state) {
      case Optimizer::Running:
        ui.table_list->item(i, C_Status)->setText(tr("Running (Opt Step %1 of %2, %3 failures)")
                                                .arg(QString::number(scene->getCurrentOptStep()))
                                                .arg(QString::number(totalOptSteps))
                                                .arg(QString::number(scene->getFailCount()))
                                                );
        brush.setColor(Qt::green);
        break;
      case Optimizer::Queued:
        ui.table_list->item(i, C_Status)->setText(tr("Queued (Opt Step %1 of %2, %3 failures)")
                                                .arg(QString::number(scene->getCurrentOptStep()))
                                                .arg(QString::number(totalOptSteps))
                                                .arg(QString::number(scene->getFailCount()))
                                                );
        brush.setColor(Qt::cyan);
        break;
      case Optimizer::Success:
        ui.table_list->item(i, C_Status)->setText("Starting update...");
        break;
      case Optimizer::Unknown:
        ui.table_list->item(i, C_Status)->setText("Unknown");
        break;
      case Optimizer::Error:
        ui.table_list->item(i, C_Status)->setText("Error: Restarting job...");
        brush.setColor(Qt::darkRed);
        break;
      case Optimizer::CommunicationError:
        ui.table_list->item(i, C_Status)->setText("Comm. Error");
        brush.setColor(Qt::darkRed);
        break;
      // Shouldn't happen; started and pending only occur when scene is "Submitted"
      case Optimizer::Started:
      case Optimizer::Pending:
      default:
        break;
      }
      break;
    }
    case Scene::Submitted:
      ui.table_list->item(i, C_Status)->setText(tr("Job submitted (%1 of %2)")
                                              .arg(QString::number(scene->getCurrentOptStep()))
                                              .arg(QString::number(totalOptSteps))
                                              );
      brush.setColor(Qt::cyan);
      break;
    case Scene::Restart:
      ui.table_list->item(i, C_Status)->setText("Restarting job...");
      brush.setColor(Qt::cyan);
      break;
    case Scene::Killed:
    case Scene::Removed:
      brush.setColor(Qt::darkGray);
      ui.table_list->item(i, C_Status)->setText("Killed");
      break;
    case Scene::Duplicate:
      brush.setColor(Qt::darkGreen);
      ui.table_list->item(i, C_Status)->setText(tr("Duplicate of %1")
                                              .arg(scene->getDuplicateString()));
      break;
    case Scene::StepOptimized:
      ui.table_list->item(i, C_Status)->setText("Checking status...");
      brush.setColor(Qt::cyan);
      break;
    case Scene::Optimized:
      ui.table_list->item(i, C_Status)->setText("Optimized");
      brush.setColor(Qt::yellow);
      break;
    case Scene::WaitingForOptimization:
      ui.table_list->item(i, C_Status)->setText(tr("Waiting for Optimization (%1 of %2)")
                                              .arg(QString::number(scene->getCurrentOptStep()))
                                              .arg(QString::number(totalOptSteps))
                                              );
      brush.setColor(Qt::darkCyan);
      break;
    case Scene::Error:
      ui.table_list->item(i, C_Status)->setText(tr("Job failed. Restarting..."));
      brush.setColor(Qt::red);
      break;
    case Scene::Updating:
      ui.table_list->item(i, C_Status)->setText("Updating structure...");
      brush.setColor(Qt::cyan);
      break;
    case Scene::Empty:
      ui.table_list->item(i, C_Status)->setText("Structure empty...");
      break;
    }

    if (scene->getFailCount() != 0) {
      brush.setColor(Qt::red);
    }
    // paint cell:
    ui.table_list->item(i, C_Status)->setBackground(brush);
  }

  void TabProgress::selectMoleculeFromProgress(int row,int,int oldrow,int) {
    Q_UNUSED(oldrow);
    if (m_opt->isStarting) {
      qDebug() << "TabProgress::selectMoleculeFromProgress: Not updating widget while session is starting";
      return;
    }
    if ( row == -1 ) return;
    emit moleculeChanged(m_opt->tracker()->at(row));
  }

  void TabProgress::highlightScene(Structure *structure) {
    structure->lock()->lockForRead();
    int id  = structure->getIDNumber();
    structure->lock()->unlock();
    for (int row = 0; row < ui.table_list->rowCount(); row++) {
      if (ui.table_list->item(row, C_Index)->text().toInt() == id) {
        ui.table_list->blockSignals(true);
        ui.table_list->setCurrentCell(row, 0);
        ui.table_list->blockSignals(false);
        return;
      }
    }
    // If not found, clear selection
    ui.table_list->blockSignals(true);
    ui.table_list->setCurrentCell(-1, -1);
    ui.table_list->blockSignals(false);
  }


  void TabProgress::startTimer() {
    if (m_timer->isActive())
      m_timer->stop();
    m_timer->start(ui.spin_period->value() * 1000);
  }

  void TabProgress::stopTimer() {
    m_timer->stop();
  }

  void TabProgress::progressContextMenu(QPoint p) {
    if (m_context_scene) return;
    QApplication::setOverrideCursor( Qt::WaitCursor );
    QTableWidgetItem *item = ui.table_list->itemAt(p);
    if (!item) {
      QApplication::restoreOverrideCursor();
      return;
    }
    int index = item->row();

    qDebug() << "Context menu at row " << index;

    // Set m_context_scene after locking to avoid threading issues.
    Scene* scene = qobject_cast<Scene*>(m_opt->tracker()->at(index));

    scene->lock()->lockForRead();

    m_context_scene = scene;

    QApplication::restoreOverrideCursor();

    Scene::State state = m_context_scene->getStatus();

    QMenu menu;
    QAction *a_restart  = menu.addAction("&Restart job");
    QAction *a_kill	= menu.addAction("&Kill structure");
    QAction *a_unkill	= menu.addAction("Un&kill structure");
    QAction *a_resetFail= menu.addAction("Reset &failure count");
    menu.addSeparator();
    QAction *a_randomize= menu.addAction("Replace with &new random structure");

    // Connect actions
    connect(a_restart, SIGNAL(triggered()), this, SLOT(restartJobProgress()));
    connect(a_kill, SIGNAL(triggered()), this, SLOT(killSceneProgress()));
    connect(a_unkill, SIGNAL(triggered()), this, SLOT(unkillSceneProgress()));
    connect(a_resetFail, SIGNAL(triggered()), this, SLOT(resetFailureCountProgress()));
    connect(a_randomize, SIGNAL(triggered()), this, SLOT(randomizeStructureProgress()));

    if (state == Scene::Killed || state == Scene::Removed) {
      a_kill->setVisible(false);
      a_restart->setVisible(false);
    }
    else {
      a_unkill->setVisible(false);
    }

    m_context_scene->lock()->unlock();
    QAction *selection = menu.exec(QCursor::pos());

    if (selection == 0) {
      m_context_scene = 0;
      return;
    }
    QtConcurrent::run(this, &TabProgress::updateProgressTable);
    a_restart->disconnect();
    a_kill->disconnect();
    a_unkill->disconnect();
    a_resetFail->disconnect();
    a_randomize->disconnect();
  }

  void TabProgress::restartJobProgress() {
    if (!m_context_scene) return;

    // Get info from scene
    m_context_scene->lock()->lockForRead();
    int id = m_context_scene->getIDNumber();
    int optstep = m_context_scene->getCurrentOptStep();
    m_context_scene->lock()->unlock();

    // Choose which OptStep to use
    bool ok;
    int optStep = QInputDialog::getInt(m_dialog,
                                       tr("Restart Optimization %1")
                                       .arg(id),
                                       "Select optimization step to restart from:",
                                       optstep,
                                       1,
                                       m_opt->optimizer()->getNumberOfOptSteps(),
                                       1,
                                       &ok);
    if (!ok) return;
    QtConcurrent::run(this, &TabProgress::restartJobProgress_, optStep);
  }

  void TabProgress::restartJobProgress_(int optStep) {
    QWriteLocker locker (m_context_scene->lock());
    m_context_scene->setCurrentOptStep(optStep);

    // Restart job if currently running
    if ( m_context_scene->getStatus() == Scene::InProcess ||
         m_context_scene->getStatus() == Scene::Submitted ) {
      locker.unlock();
      m_opt->optimizer()->deleteJob(m_context_scene);
      locker.relock();
    }

    m_context_scene->setStatus(Scene::Restart);
    newInfoUpdate(m_context_scene);

    // Clear context scene pointer
    locker.unlock();
    m_context_scene = 0;
  }

  void TabProgress::killSceneProgress() {
    QtConcurrent::run(this, &TabProgress::killSceneProgress_);
  }

  void TabProgress::killSceneProgress_() {
    if (!m_context_scene) return;
    QWriteLocker locker (m_context_scene->lock());

    // End job if currently running
    if ( m_context_scene->getStatus() != Scene::Optimized ) {
      locker.unlock();
      m_opt->optimizer()->deleteJob(m_context_scene);
      locker.relock();
      m_context_scene->setStatus(Scene::Killed);
    }
    else m_context_scene->setStatus(Scene::Removed);

    // Clear context scene pointer
    locker.unlock();
    newInfoUpdate(m_context_scene);
    m_context_scene = 0;
  }

  void TabProgress::unkillSceneProgress() {
    QtConcurrent::run(this, &TabProgress::unkillSceneProgress_);
  }

  void TabProgress::unkillSceneProgress_() {
    if (!m_context_scene) return;
    QWriteLocker locker (m_context_scene->lock());
    if (m_context_scene->getStatus() != Scene::Killed &&
        m_context_scene->getStatus() != Scene::Removed ) return;

    // Setting status to Scene::Error will restart the job if was killed
    if (m_context_scene->getStatus() == Scene::Killed)
      m_context_scene->setStatus(Scene::Error);

    // Set status to Optimized if scene was previously optimized
    if (m_context_scene->getStatus() == Scene::Removed)
      m_context_scene->setStatus(Scene::Optimized);

    // Clear context scene pointer
    newInfoUpdate(m_context_scene);
    locker.unlock();
    m_context_scene = 0;
  }

  void TabProgress::resetFailureCountProgress() {
    QtConcurrent::run(this, &TabProgress::resetFailureCountProgress_);
  }

  void TabProgress::resetFailureCountProgress_() {
    if (!m_context_scene) return;
    QWriteLocker locker (m_context_scene->lock());

    m_context_scene->resetFailCount();

    // Clear context scene pointer
    newInfoUpdate(m_context_scene);
    locker.unlock();
    m_context_scene = 0;

    emit refresh();
  }

  void TabProgress::randomizeStructureProgress() {
    QtConcurrent::run(this, &TabProgress::randomizeStructureProgress_);
  }

  void TabProgress::randomizeStructureProgress_() {
    if (!m_context_scene) return;

    // End job if currently running
    if (m_context_scene->getJobID()) {
      m_opt->optimizer()->deleteJob(m_context_scene);
    }

    m_opt->replaceWithRandom(m_context_scene, "manual");

    // Restart job:
    newInfoUpdate(m_context_scene);
    restartJobProgress_(1);
  }


}

//#include "tab_progress.moc"

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

#include "tab_progress.h"

#include "dialog.h"
#include "../optimizer.h"
#include "../macros.h"

#include <QMenu>
#include <QTimer>
#include <QBrush>
#include <QSettings>
#include <QMutexLocker>
#include <QInputDialog>
#include <QtConcurrentRun>

using namespace std;

namespace Avogadro {

  TabProgress::TabProgress( XtalOptDialog *parent, XtalOpt *p ) :
    QObject( parent ), m_dialog(parent), m_opt(p), m_timer(0), m_mutex(0), m_update_mutex(0), m_update_all_mutex(0), m_context_xtal(0)
  {
    //qDebug() << "TabProgress::TabProgress( " << parent <<  " ) called.";

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
    connect(this, SIGNAL(moleculeChanged(Xtal*)),
            m_dialog, SIGNAL(moleculeChanged(Xtal*)));
    connect(m_dialog, SIGNAL(moleculeChanged(Xtal*)),
            this, SLOT(highlightXtal(Xtal*)));
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
    //qDebug() << "TabProgress::~TabProgress() called";
    delete m_mutex;
    delete m_update_mutex;
    delete m_update_all_mutex;
    delete m_timer;
  }

  void TabProgress::writeSettings(const QString &filename) {
    SETTINGS(filename);
    settings->beginGroup("xtalopt/progress");
    settings->setValue("refreshTime", ui.spin_period->value());
    settings->endGroup();      
  }

  void TabProgress::readSettings(const QString &filename) {
    SETTINGS(filename);
    settings->beginGroup("xtalopt/progress");
    ui.spin_period->setValue(settings->value("refreshTime", 1).toInt());
    settings->endGroup();      
  }

  void TabProgress::updateGUI() {
    //qDebug() << "TabProgress::updateGUI() called";
    // Nothing to do!
  }

  void TabProgress::disconnectGUI() {
    //qDebug() << "TabProgress::disconnectGUI() called";
    m_timer->disconnect();
    ui.push_refresh->disconnect();
    ui.push_refreshAll->disconnect();
    ui.spin_period->disconnect();
    ui.table_list->disconnect();
    disconnect(m_opt, 0, this, 0);
    disconnect(m_dialog, 0, this, 0);
    this->disconnect();
  }

  void TabProgress::lockGUI() {
    //qDebug() << "TabProgress::lockGUI() called";
    // Nothing to do!
  }

  void TabProgress::updateProgressTable() {
    //qDebug() << "TabProgress::updateProgressTable( ) called";

    // Only allow one update at a time
    if (!m_update_mutex->tryLock()) {
      qDebug() << "Killing extra TabProgress::updateProgressTable() call";
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
    //qDebug() << "TabProgress::addNewEntry() called";

    // Prevent XtalOpt threads from modifying the table
    QMutexLocker locker (m_mutex);

    // The new entry will be at the end of the table, so determine the index:
    int index = ui.table_list->rowCount();
    Xtal *xtal = qobject_cast<Xtal*>(m_opt->tracker()->at(index));
    //qDebug() << "TabProgress::addNewEntry() at index " << index;

    // Turn off signals
    ui.table_list->blockSignals(true);

    // Store current index for later. If -1, this will be re-set at the end of table
    int currentInd = ui.table_list->currentRow();
    if (currentInd >= ui.table_list->rowCount() - 1) currentInd = -1;

    // Add the new row
    ui.table_list->insertRow(index);
    // Columns: once for each column in ProgressColumns:
    for (int i = 0; i < 9; i++) {
      ui.table_list->setItem(index, i, new QTableWidgetItem());
    }

    m_infoUpdateTracker.append(xtal);
    locker.unlock();
    emit infoUpdate();

    ui.table_list->blockSignals(false);

    if (currentInd < 0) currentInd = index;
    if (rowTracking) ui.table_list->setCurrentCell(currentInd, 0);
  }

  void TabProgress::updateAllInfo() {
    //qDebug() << "TabProgress::updateAllInfo() called";
    if (!m_update_all_mutex->tryLock()) {
      qDebug() << "Killing extra TabProgress::updateAllInfo() call";
      return;
    }
    QList<Structure*> *structures = m_opt->tracker()->list();
    for (int i = 0; i < ui.table_list->rowCount(); i++) {
      m_infoUpdateTracker.append(structures->at(i));
      emit infoUpdate();
    }
    m_update_all_mutex->unlock();
  }

  void TabProgress::newInfoUpdate(Structure *s) {
    m_infoUpdateTracker.append(s);
    emit infoUpdate();
  }

  void TabProgress::updateInfo() {
    //qDebug() << "TabProgress::updateInfo( ) called";

    if (m_infoUpdateTracker.size() == 0) {
      return;
    }

    // Don't update while a context operation is in the works
    if (m_context_xtal !=0) {
      qDebug() << "TabProgress::updateInfo: Waiting for context operation to complete (" << m_context_xtal << ") Trying again very soon.";
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

    Xtal *xtal = qobject_cast<Xtal*>(structure);

    if (i < 0 || i > ui.table_list->rowCount() - 1) {
      qDebug() << "TabProgress::updateInfo: Trying to update an index that doesn't exist (" << i << ") Waiting...";
      m_infoUpdateTracker.append(xtal);
      QTimer::singleShot(100, this, SLOT(updateInfo()));
      return;
    }

    QString time;
    uint totalOptSteps = m_opt->optimizer()->getNumberOfOptSteps();
    QBrush brush (Qt::white);

    // Get queue data
    m_opt->queue()->updateQueue();

    QReadLocker xtalLocker (xtal->lock());

    time = xtal->getOptElapsed();

    ui.table_list->item(i, TimeElapsed)->setText(time);

    ui.table_list->item(i, Gen)->setText(QString::number(xtal->getGeneration()));
    ui.table_list->item(i, Mol)->setText(QString::number(xtal->getIDNumber()));
    ui.table_list->item(i, Ancestry)->setText(xtal->getParents());

    if (xtal->getJobID())
      ui.table_list->item(i, JobID)->setText(QString::number(xtal->getJobID()));
    else
      ui.table_list->item(i, JobID)->setText("N/A");

    if (xtal->hasEnthalpy() || xtal->hasEnergy())
      ui.table_list->item(i, Enthalpy)->setText(QString::number(xtal->getEnthalpy()));
    else
      ui.table_list->item(i, Enthalpy)->setText("N/A");
    ui.table_list->item(i, SpaceGroup)->setText( QString::number( xtal->getSpaceGroupNumber()) + ": " + xtal->getSpaceGroupSymbol() );
    ui.table_list->item(i, Volume)->setText( QString::number( xtal->getVolume(), 'f', 2 ));
    switch (xtal->getStatus()) {
    case Xtal::InProcess: {
      xtalLocker.unlock();
      Optimizer::JobState state = m_opt->optimizer()->getStatus(xtal);
      xtalLocker.relock();
      switch (state) {
      case Optimizer::Running:
        ui.table_list->item(i, Status)->setText(tr("Running (Opt Step %1 of %2, %3 failures)")
                                                .arg(QString::number(xtal->getCurrentOptStep()))
                                                .arg(QString::number(totalOptSteps))
                                                .arg(QString::number(xtal->getFailCount()))
                                                );
        brush.setColor(Qt::green);
        break;
      case Optimizer::Queued:
        ui.table_list->item(i, Status)->setText(tr("Queued (Opt Step %1 of %2, %3 failures)")
                                                .arg(QString::number(xtal->getCurrentOptStep()))
                                                .arg(QString::number(totalOptSteps))
                                                .arg(QString::number(xtal->getFailCount()))
                                                );
        brush.setColor(Qt::cyan);
        break;
      case Optimizer::Success:
        ui.table_list->item(i, Status)->setText("Starting update...");
        break;
      case Optimizer::Unknown:
        ui.table_list->item(i, Status)->setText("Unknown");
        break;
      case Optimizer::Error:
        ui.table_list->item(i, Status)->setText("Error: Restarting job...");
        brush.setColor(Qt::darkRed);
        break;
      case Optimizer::CommunicationError:
        ui.table_list->item(i, Status)->setText("Comm. Error");
        brush.setColor(Qt::darkRed);
        break;
      // Shouldn't happen; started and pending only occur when xtal is "Submitted"
      case Optimizer::Started:
      case Optimizer::Pending:
      default:
        break;
      }
      break;
    }
    case Xtal::Submitted:
      ui.table_list->item(i, Status)->setText(tr("Job submitted (%1 of %2)")
                                              .arg(QString::number(xtal->getCurrentOptStep()))
                                              .arg(QString::number(totalOptSteps))
                                              );
      brush.setColor(Qt::cyan);
      break;
    case Xtal::Restart:
      ui.table_list->item(i, Status)->setText("Restarting job...");
      brush.setColor(Qt::cyan);
      break;
    case Xtal::Killed:
    case Xtal::Removed:
      brush.setColor(Qt::darkGray);
      ui.table_list->item(i, Status)->setText("Killed");
      break;
    case Xtal::Duplicate:
      brush.setColor(Qt::darkGreen);
      ui.table_list->item(i, Status)->setText(tr("Duplicate of %1")
                                              .arg(xtal->getDuplicateString()));
      break;
    case Xtal::StepOptimized:
      ui.table_list->item(i, Status)->setText("Checking status...");
      brush.setColor(Qt::cyan);
      break;
    case Xtal::Optimized:
      ui.table_list->item(i, Status)->setText("Optimized");
      brush.setColor(Qt::yellow);
      break;
    case Xtal::WaitingForOptimization:
      ui.table_list->item(i, Status)->setText(tr("Waiting for Optimization (%1 of %2)")
                                              .arg(QString::number(xtal->getCurrentOptStep()))
                                              .arg(QString::number(totalOptSteps))
                                              );
      brush.setColor(Qt::darkCyan);
      break;
    case Xtal::Error:
      ui.table_list->item(i, Status)->setText(tr("Job failed. Restarting..."));
      brush.setColor(Qt::red);
      break;
    case Xtal::Updating:
      ui.table_list->item(i, Status)->setText("Updating structure...");
      brush.setColor(Qt::cyan);
      break;
    case Xtal::Empty:
      ui.table_list->item(i, Status)->setText("Structure empty...");
      break;
    }

    if (xtal->getFailCount() != 0) {
      brush.setColor(Qt::red);
    }
    // paint cell:
    ui.table_list->item(i, Status)->setBackground(brush);
  }

  void TabProgress::selectMoleculeFromProgress(int row,int,int oldrow,int) {
    //qDebug() << "TabProgress::selectMoleculeFromProgress( " << row << " " << oldrow << " ) called";
    Q_UNUSED(oldrow);
    if (m_opt->isStarting) {
      qDebug() << "TabProgress::selectMoleculeFromProgress: Not updating widget while session is starting";
      return;
    }
    if ( row == -1 ) return;
    emit moleculeChanged(qobject_cast<Xtal*>(m_opt->tracker()->at(row)));
  }

  void TabProgress::highlightXtal(Xtal* xtal) {
    //qDebug() << "TabProgress::highlightMolecule( " << xtal << " ) called.";
    xtal->lock()->lockForRead();
    int gen = xtal->getGeneration();
    int id  = xtal->getIDNumber();
    xtal->lock()->unlock();
    for (int row = 0; row < ui.table_list->rowCount(); row++) {
      if (ui.table_list->item(row, Gen)->text().toInt() == gen &&
          ui.table_list->item(row, Mol)->text().toInt() == id) {
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
    if (m_context_xtal) return;
    QApplication::setOverrideCursor( Qt::WaitCursor );
    QTableWidgetItem *item = ui.table_list->itemAt(p);
    if (!item) {
      QApplication::restoreOverrideCursor();
      return;
    }
    int index = item->row();

    qDebug() << "Context menu at row " << index;

    // Set m_context_xtal after locking to avoid threading issues.
    Xtal* xtal = qobject_cast<Xtal*>(m_opt->tracker()->at(index));

    xtal->lock()->lockForRead();

    m_context_xtal = xtal;

    QApplication::restoreOverrideCursor();

    Xtal::State state = m_context_xtal->getStatus();

    QMenu menu;
    QAction *a_restart  = menu.addAction("&Restart job");
    QAction *a_kill	= menu.addAction("&Kill structure");
    QAction *a_unkill	= menu.addAction("Un&kill structure");
    QAction *a_resetFail= menu.addAction("Reset &failure count");
    menu.addSeparator();
    QAction *a_randomize= menu.addAction("Replace with &new random structure");

    // Connect actions
    connect(a_restart, SIGNAL(triggered()), this, SLOT(restartJobProgress()));
    connect(a_kill, SIGNAL(triggered()), this, SLOT(killXtalProgress()));
    connect(a_unkill, SIGNAL(triggered()), this, SLOT(unkillXtalProgress()));
    connect(a_resetFail, SIGNAL(triggered()), this, SLOT(resetFailureCountProgress()));
    connect(a_randomize, SIGNAL(triggered()), this, SLOT(randomizeStructureProgress()));

    if (state == Xtal::Killed || state == Xtal::Removed) {
      a_kill->setVisible(false);
      a_restart->setVisible(false);
    }
    else {
      a_unkill->setVisible(false);
    }

    m_context_xtal->lock()->unlock();
    QAction *selection = menu.exec(QCursor::pos());

    if (selection == 0) {
      m_context_xtal = 0;
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
    //qDebug() << "TabProgress::restartJobProgress() called";
    if (!m_context_xtal) return;

    // Get info from xtal
    m_context_xtal->lock()->lockForRead();
    int gen = m_context_xtal->getGeneration();
    int id = m_context_xtal->getIDNumber();
    int optstep = m_context_xtal->getCurrentOptStep();
    m_context_xtal->lock()->unlock();

    // Choose which OptStep to use
    bool ok;
    int optStep = QInputDialog::getInt(m_dialog,
                                       tr("Restart Optimization %1x%2")
                                       .arg(gen)
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
    //qDebug() << "TabProgress::restartJobProgress_( " << optStep << " ) called";
    QWriteLocker locker (m_context_xtal->lock());
    m_context_xtal->setCurrentOptStep(optStep);

    // Restart job if currently running
    if ( m_context_xtal->getStatus() == Xtal::InProcess ||
         m_context_xtal->getStatus() == Xtal::Submitted ) {
      locker.unlock();
      m_opt->optimizer()->deleteJob(m_context_xtal);
      locker.relock();
    }

    m_context_xtal->setStatus(Xtal::Restart);
    newInfoUpdate(m_context_xtal);

    // Clear context xtal pointer
    locker.unlock();
    m_context_xtal = 0;
  }

  void TabProgress::killXtalProgress() {
    //qDebug() << "TabProgress::killXtalProgress() called";
    QtConcurrent::run(this, &TabProgress::killXtalProgress_);
  }

  void TabProgress::killXtalProgress_() {
    //qDebug() << "TabProgress::killXtalProgress_() called";
    if (!m_context_xtal) return;
    QWriteLocker locker (m_context_xtal->lock());

    // End job if currently running
    if ( m_context_xtal->getStatus() != Xtal::Optimized ) {
      locker.unlock();
      m_opt->optimizer()->deleteJob(m_context_xtal);
      locker.relock();
      m_context_xtal->setStatus(Xtal::Killed);
    }
    else m_context_xtal->setStatus(Xtal::Removed);

    // Clear context xtal pointer
    locker.unlock();
    newInfoUpdate(m_context_xtal);
    m_context_xtal = 0;
  }

  void TabProgress::unkillXtalProgress() {
    //qDebug() << "TabProgress::unkillXtalProgress() called";
    QtConcurrent::run(this, &TabProgress::unkillXtalProgress_);
  }

  void TabProgress::unkillXtalProgress_() {
    //qDebug() << "TabProgress::unkillXtalProgress_() called";
    if (!m_context_xtal) return;
    QWriteLocker locker (m_context_xtal->lock());
    if (m_context_xtal->getStatus() != Xtal::Killed &&
        m_context_xtal->getStatus() != Xtal::Removed ) return;

    // Setting status to Xtal::Error will restart the job if was killed
    if (m_context_xtal->getStatus() == Xtal::Killed)
      m_context_xtal->setStatus(Xtal::Error);

    // Set status to Optimized if xtal was previously optimized
    if (m_context_xtal->getStatus() == Xtal::Removed)
      m_context_xtal->setStatus(Xtal::Optimized);

    // Clear context xtal pointer
    newInfoUpdate(m_context_xtal);
    locker.unlock();
    m_context_xtal = 0;
  }

  void TabProgress::resetFailureCountProgress() {
    //qDebug() << "TabProgress::resetFailureCountProgress() called";
    QtConcurrent::run(this, &TabProgress::resetFailureCountProgress_);
  }

  void TabProgress::resetFailureCountProgress_() {
    //qDebug() << "TabProgress::resetFailureCountProgress_() called";
    if (!m_context_xtal) return;
    QWriteLocker locker (m_context_xtal->lock());

    m_context_xtal->resetFailCount();

    // Clear context xtal pointer
    newInfoUpdate(m_context_xtal);
    locker.unlock();
    m_context_xtal = 0;

    emit refresh();
  }

  void TabProgress::randomizeStructureProgress() {
    //qDebug() << "TabProgress::randomizeStructureProgress() called";
    QtConcurrent::run(this, &TabProgress::randomizeStructureProgress_);
  }

  void TabProgress::randomizeStructureProgress_() {
    //qDebug() << "TabProgress::randomizeStructureProgress_() called";
    if (!m_context_xtal) return;

    // End job if currently running
    if (m_context_xtal->getJobID()) {
      m_opt->optimizer()->deleteJob(m_context_xtal);
    }

    m_opt->replaceWithRandom(m_context_xtal, "manual");

    // Restart job:
    newInfoUpdate(m_context_xtal);
    restartJobProgress_(1);
  }


}

#include "tab_progress.moc"

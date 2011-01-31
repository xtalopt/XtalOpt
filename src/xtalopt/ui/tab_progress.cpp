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

#include <xtalopt/ui/tab_progress.h>

#include <globalsearch/optimizer.h>
#include <globalsearch/ui/abstracttab.h>

#include <xtalopt/xtalopt.h>
#include <xtalopt/ui/dialog.h>

#include <QtCore/QTimer>
#include <QtCore/QSettings>
#include <QtCore/QMutexLocker>
#include <QtCore/QtConcurrentRun>

#include <QtGui/QMenu>
#include <QtGui/QInputDialog>

using namespace GlobalSearch;

namespace XtalOpt {

  TabProgress::TabProgress( XtalOptDialog *parent, XtalOpt *p ) :
    AbstractTab(parent, p),
    m_timer(new QTimer (this)),
    m_mutex(new QMutex),
    m_update_mutex(new QMutex),
    m_update_all_mutex(new QMutex),
    m_context_mutex(new QMutex),
    m_context_xtal(0)
  {
    // Allow queued connections to work with the TableEntry struct
    qRegisterMetaType<XO_Prog_TableEntry>("XO_Prog_TableEntry");

    ui.setupUi(m_tab_widget);

    QHeaderView *horizontal = ui.table_list->horizontalHeader();
    horizontal->setResizeMode(QHeaderView::ResizeToContents);

    rowTracking = true;

    // dialog connections
    connect(m_dialog, SIGNAL(moleculeChanged(GlobalSearch::Structure*)),
            this, SLOT(highlightXtal(GlobalSearch::Structure*)));
    connect(m_opt, SIGNAL(updateAllInfo()),
            this, SLOT(updateAllInfo()));
    connect(m_opt, SIGNAL(sessionStarted()),
            this, SLOT(startTimer()));

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
    connect(m_opt->tracker(), SIGNAL(newStructureAdded(GlobalSearch::Structure*)),
            this, SLOT(addNewEntry()),
            Qt::QueuedConnection);
    connect(m_opt->queue(), SIGNAL(structureUpdated(GlobalSearch::Structure*)),
            this, SLOT(newInfoUpdate(GlobalSearch::Structure *)));
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
    connect(this, SIGNAL(updateTableEntry(int, const XO_Prog_TableEntry&)),
            this, SLOT(setTableEntry(int, const XO_Prog_TableEntry&)));

    initialize();
  }

  TabProgress::~TabProgress()
  {
    delete m_mutex;
    delete m_update_mutex;
    delete m_update_all_mutex;
    delete m_context_mutex;
    delete m_timer;
  }

  void TabProgress::writeSettings(const QString &filename)
  {
    SETTINGS(filename);
    const int VERSION = 1;
    settings->beginGroup("xtalopt/progress");
    settings->setValue("version",     VERSION);
    settings->setValue("refreshTime", ui.spin_period->value());
    settings->endGroup();
    DESTROY_SETTINGS(filename);
  }

  void TabProgress::readSettings(const QString &filename)
  {
    SETTINGS(filename);
    settings->beginGroup("xtalopt/progress");
    int loadedVersion = settings->value("version", 0).toInt();
    ui.spin_period->setValue(settings->value("refreshTime", 1).toInt());
    settings->endGroup();

    // Update config data
    switch (loadedVersion) {
    case 0:
    case 1:
    default:
      break;
    }

  }

  void TabProgress::disconnectGUI()
  {
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

  void TabProgress::updateProgressTable()
  {
    // Only allow one update at a time
    if (!m_update_mutex->tryLock()) {
      qDebug() << "Killing extra TabProgress::updateProgressTable() call";
      return;
    }

    QList<Structure*> running = m_opt->queue()->getAllRunningStructures();

    for (QList<Structure*>::iterator
           it = running.begin(),
           it_end = running.end();
         it != it_end;
         ++it) {
      newInfoUpdate(*it);
    }

    m_update_mutex->unlock();
  }

  void TabProgress::addNewEntry()
  {
    // Prevent XtalOpt threads from modifying the table
    QMutexLocker locker (m_mutex);

    // The new entry will be at the end of the table, so determine the index:
    int index = ui.table_list->rowCount();
    m_opt->tracker()->lockForRead();
    Xtal *xtal = qobject_cast<Xtal*>(m_opt->tracker()->at(index));
    m_opt->tracker()->unlock();
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

    m_infoUpdateTracker.lockForWrite();
    m_infoUpdateTracker.append(xtal);
    m_infoUpdateTracker.unlock();
    locker.unlock();
    XO_Prog_TableEntry e;
    xtal->lock()->lockForRead();
    e.elapsed = xtal->getOptElapsed();
    e.gen     = xtal->getGeneration();
    e.id      = xtal->getIDNumber();
    e.parents = xtal->getParents();
    e.jobID   = xtal->getJobID();
    e.volume  = xtal->getVolume();
    e.status  = "Waiting for data...";
    e.brush   = QBrush (Qt::white);
    e.spg     = QString::number( xtal->getSpaceGroupNumber()) + ": "
      + xtal->getSpaceGroupSymbol();

    if (xtal->hasEnthalpy() || xtal->getEnergy() != 0)
      e.enthalpy = xtal->getEnthalpy();
    else
      e.enthalpy = 0.0;
    xtal->lock()->unlock();

    ui.table_list->blockSignals(false);

    if (currentInd < 0) currentInd = index;
    if (rowTracking) ui.table_list->setCurrentCell(currentInd, 0);

    locker.unlock();

    setTableEntry(index, e);
    emit infoUpdate();
  }

  void TabProgress::updateAllInfo()
  {
    if (!m_update_all_mutex->tryLock()) {
      qDebug() << "Killing extra TabProgress::updateAllInfo() call";
      return;
    }
    m_opt->tracker()->lockForRead();
    m_infoUpdateTracker.lockForWrite();
    QList<Structure*> *structures = m_opt->tracker()->list();
    for (int i = 0; i < ui.table_list->rowCount(); i++) {
      m_infoUpdateTracker.append(structures->at(i));
      emit infoUpdate();
    }
    m_infoUpdateTracker.unlock();
    m_opt->tracker()->unlock();
    m_update_all_mutex->unlock();
  }

  void TabProgress::newInfoUpdate(Structure *s)
  {
    m_infoUpdateTracker.lockForWrite();
    if (m_infoUpdateTracker.append(s)) {
      emit infoUpdate();
    }
    m_infoUpdateTracker.unlock();
  }

  void TabProgress::updateInfo()
  {
    if (m_infoUpdateTracker.size() == 0) {
      return;
    }

    // Don't update while a context operation is in the works
    if (m_context_xtal !=0) {
      qDebug() << "TabProgress::updateInfo: Waiting for context operation to complete (" << m_context_xtal << ") Trying again very soon.";
      QTimer::singleShot(1000, this, SLOT(updateInfo()));
      return;
    }

    QtConcurrent::run(this, &TabProgress::updateInfo_);
    return;
  }

  void TabProgress::updateInfo_()
  {
    // Prep variables
    Structure *structure;
    m_infoUpdateTracker.lockForWrite();
    if (!m_infoUpdateTracker.popFirst(structure)) {
      m_infoUpdateTracker.unlock();
      return;
    }
    m_infoUpdateTracker.unlock();

    m_opt->tracker()->lockForRead();
    int i = m_opt->tracker()->list()->indexOf(structure);
    m_opt->tracker()->unlock();

    Xtal *xtal = qobject_cast<Xtal*>(structure);

    if (i < 0 || i > ui.table_list->rowCount() - 1) {
      qDebug() << "TabProgress::updateInfo: Trying to update an index that doesn't exist...yet: ("
               << i << ") Waiting...";
      m_infoUpdateTracker.lockForWrite();
      m_infoUpdateTracker.append(xtal);
      m_infoUpdateTracker.unlock();
      QTimer::singleShot(100, this, SLOT(updateInfo()));
      return;
    }

    XO_Prog_TableEntry e;
    uint totalOptSteps = m_opt->optimizer()->getNumberOfOptSteps();
    e.brush = QBrush (Qt::white);

    QReadLocker xtalLocker (xtal->lock());
    e.elapsed = xtal->getOptElapsed();
    e.gen     = xtal->getGeneration();
    e.id      = xtal->getIDNumber();
    e.parents = xtal->getParents();
    e.jobID   = xtal->getJobID();
    e.volume  = xtal->getVolume();
    e.spg     = QString::number( xtal->getSpaceGroupNumber()) + ": "
      + xtal->getSpaceGroupSymbol();

    if (xtal->hasEnthalpy() || xtal->getEnergy() != 0)
      e.enthalpy = xtal->getEnthalpy();
    else
      e.enthalpy = 0.0;

    switch (xtal->getStatus()) {
    case Xtal::InProcess: {
      xtalLocker.unlock();
      Optimizer::JobState state = m_opt->optimizer()->getStatus(xtal);
      xtalLocker.relock();
      switch (state) {
      case Optimizer::Running:
        e.status = tr("Running (Opt Step %1 of %2, %3 failures)")
          .arg(QString::number(xtal->getCurrentOptStep()))
          .arg(QString::number(totalOptSteps))
          .arg(QString::number(xtal->getFailCount()));
        e.brush.setColor(Qt::green);
        break;
      case Optimizer::Queued:
        e.status = tr("Queued (Opt Step %1 of %2, %3 failures)")
          .arg(QString::number(xtal->getCurrentOptStep()))
          .arg(QString::number(totalOptSteps))
          .arg(QString::number(xtal->getFailCount()));
        e.brush.setColor(Qt::cyan);
        break;
      case Optimizer::Success:
        e.status = "Starting update...";
        break;
      case Optimizer::Unknown:
        e.status = "Unknown";
        e.brush.setColor(Qt::red);
        break;
      case Optimizer::Error:
        e.status = "Error: Restarting job...";
        e.brush.setColor(Qt::darkRed);
        break;
      case Optimizer::CommunicationError:
        e.status = "Communication Error";
        e.brush.setColor(Qt::darkRed);
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
      e.status = tr("Job submitted (%1 of %2)")
        .arg(QString::number(xtal->getCurrentOptStep()))
        .arg(QString::number(totalOptSteps));
      e.brush.setColor(Qt::cyan);
      break;
    case Xtal::Restart:
      e.status = "Restarting job...";
      e.brush.setColor(Qt::cyan);
      break;
    case Xtal::Killed:
    case Xtal::Removed:
      e.status = "Killed";
      e.brush.setColor(Qt::darkGray);
      break;
    case Xtal::Duplicate:
      e.status = tr("Duplicate of %1")
        .arg(xtal->getDuplicateString());
      e.brush.setColor(Qt::darkGreen);
      break;
    case Xtal::StepOptimized:
      e.status = "Checking status...";
      e.brush.setColor(Qt::cyan);
      break;
    case Xtal::Optimized:
      e.status = "Optimized";
      e.brush.setColor(Qt::yellow);
      break;
    case Xtal::WaitingForOptimization:
      e.status = tr("Waiting for Optimization (%1 of %2)")
        .arg(QString::number(xtal->getCurrentOptStep()))
        .arg(QString::number(totalOptSteps));
      e.brush.setColor(Qt::darkCyan);
      break;
    case Xtal::Error:
      e.status = "Job failed";
      e.brush.setColor(Qt::red);
      break;
    case Xtal::Updating:
      e.status = "Updating structure...";
      e.brush.setColor(Qt::cyan);
      break;
    case Xtal::Empty:
      e.status = "Structure empty...";
      break;
    }

    if (xtal->getFailCount() != 0) {
      e.brush.setColor(Qt::red);
    }
    emit updateTableEntry(i, e);
  }

  void TabProgress::setTableEntry(int row, const XO_Prog_TableEntry & e)
  {
    // Lock the table
    QMutexLocker locker (m_mutex);

    ui.table_list->item(row, TimeElapsed)->setText(e.elapsed);
    ui.table_list->item(row, Gen)->setText(QString::number(e.gen));
    ui.table_list->item(row, Mol)->setText(QString::number(e.id));
    ui.table_list->item(row, Ancestry)->setText(e.parents);
    ui.table_list->item(row, SpaceGroup)->setText(e.spg);
    ui.table_list->item(row, Volume)->setText(QString::number(e.volume,'f',2));
    ui.table_list->item(row, Status)->setText(e.status);
    ui.table_list->item(row, Status)->setBackground(e.brush);

    if (e.jobID)
      ui.table_list->item(row, JobID)->setText(QString::number(e.jobID));
    else
      ui.table_list->item(row, JobID)->setText("N/A");

    if (e.enthalpy != 0)
      ui.table_list->item(row, Enthalpy)->setText(QString::number(e.enthalpy));
    else
      ui.table_list->item(row, Enthalpy)->setText("N/A");
  }

  void TabProgress::selectMoleculeFromProgress(int row,int,int oldrow,int)
  {
    Q_UNUSED(oldrow);
    if (m_opt->isStarting) {
      qDebug() << "TabProgress::selectMoleculeFromProgress: Not updating widget while session is starting";
      return;
    }
    if ( row == -1 ) return;
    emit moleculeChanged(qobject_cast<Xtal*>(m_opt->tracker()->at(row)));
  }

  void TabProgress::highlightXtal(Structure *s)
  {
    Xtal *xtal = qobject_cast<Xtal*>(s);
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


  void TabProgress::startTimer()
  {
    if (m_timer->isActive())
      m_timer->stop();
    m_timer->start(ui.spin_period->value() * 1000);
  }

  void TabProgress::stopTimer()
  {
    m_timer->stop();
  }

  void TabProgress::progressContextMenu(QPoint p)
  {
    if (m_context_xtal) return;
    // m_context_mutex prevents multiple menus from appearing, which
    // ultimately prevents m_context_xtal from being cleared.
    if (!m_context_mutex->tryLock()) {
      return;
    }

    QApplication::setOverrideCursor( Qt::WaitCursor );
    QTableWidgetItem *item = ui.table_list->itemAt(p);
    if (!item) {
      QApplication::restoreOverrideCursor();
      m_context_mutex->unlock();
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
    menu.addSeparator();
    QAction *a_clipPOSCAR= menu.addAction("&Copy POSCAR to clipboard");

    // Connect actions
    connect(a_restart, SIGNAL(triggered()), this, SLOT(restartJobProgress()));
    connect(a_kill, SIGNAL(triggered()), this, SLOT(killXtalProgress()));
    connect(a_unkill, SIGNAL(triggered()), this, SLOT(unkillXtalProgress()));
    connect(a_resetFail, SIGNAL(triggered()), this, SLOT(resetFailureCountProgress()));
    connect(a_randomize, SIGNAL(triggered()), this, SLOT(randomizeStructureProgress()));
    connect(a_clipPOSCAR, SIGNAL(triggered()), this, SLOT(clipPOSCARProgress()));

    if (state == Xtal::Killed || state == Xtal::Removed) {
      a_kill->setVisible(false);
      a_restart->setVisible(false);
    }
    else {
      a_unkill->setVisible(false);
    }

    m_context_xtal->lock()->unlock();
    QAction *selection = menu.exec(QCursor::pos());
    qDebug() << selection;

    if (selection == 0) {
      m_context_xtal = 0;
    }
    else {
      QtConcurrent::run(this, &TabProgress::updateProgressTable);
      a_restart->disconnect();
      a_kill->disconnect();
      a_unkill->disconnect();
      a_resetFail->disconnect();
      a_randomize->disconnect();
    }
    m_context_mutex->unlock();
  }

  void TabProgress::restartJobProgress()
  {
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
    if (!ok) {
      m_context_xtal = 0;
      return;
    }
    QtConcurrent::run(this, &TabProgress::restartJobProgress_, optStep);
  }

  void TabProgress::restartJobProgress_(int optStep)
  {
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

  void TabProgress::killXtalProgress()
  {
    QtConcurrent::run(this, &TabProgress::killXtalProgress_);
  }

  void TabProgress::killXtalProgress_()
  {
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

  void TabProgress::unkillXtalProgress()
  {
    QtConcurrent::run(this, &TabProgress::unkillXtalProgress_);
  }

  void TabProgress::unkillXtalProgress_()
  {
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

  void TabProgress::resetFailureCountProgress()
  {
    QtConcurrent::run(this, &TabProgress::resetFailureCountProgress_);
  }

  void TabProgress::resetFailureCountProgress_()
  {
    if (!m_context_xtal) return;
    QWriteLocker locker (m_context_xtal->lock());

    m_context_xtal->resetFailCount();

    // Clear context xtal pointer
    newInfoUpdate(m_context_xtal);
    locker.unlock();
    m_context_xtal = 0;
  }

  void TabProgress::randomizeStructureProgress()
  {
    QtConcurrent::run(this, &TabProgress::randomizeStructureProgress_);
  }

  void TabProgress::randomizeStructureProgress_()
  {
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

  void TabProgress::clipPOSCARProgress()
  {
    QtConcurrent::run(this, &TabProgress::clipPOSCARProgress_);
  }

  void TabProgress::clipPOSCARProgress_()
  {
    if (!m_context_xtal) return;
    QReadLocker locker (m_context_xtal->lock());

    QString poscar = qobject_cast<XtalOpt*>(m_opt)->
      interpretTemplate("%POSCAR%", m_context_xtal);

    m_opt->setClipboard(poscar);

    // Clear context xtal pointer
    locker.unlock();
    m_context_xtal = 0;
  }
}

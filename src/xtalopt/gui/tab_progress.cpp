/**********************************************************************
  TabProgress - The progress tab: the live table of structures and their current states.

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/gui/tab_progress.h>

#include <common/compatibility/qt_compat.h>
#include <common/output.h>
#include <search/search.h>
#include <search/optimizer.h>
#include <search/queueinterface.h>
#include <search/queuemanager.h>
#include <search/tracker.h>
#include <search/gui/abstracttab.h>
#include <common/fileutils.h>
#include <atoms/formats/vaspformat.h>

#include <xtalopt/constants.h>
#include <xtalopt/structures/xtal.h>
#include <xtalopt/gui/dialog.h>
#include <xtalopt/xtalopt.h>

#include <QAbstractSpinBox>
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QColor>
#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QReadLocker>
#include <QSettings>
#include <QTableWidget>
#include <QTimer>
#include <QtConcurrent>

#include <sstream>

using namespace Search;

namespace XtalOpt {
TabProgress::TabProgress(Search::AbstractDialog* parent, XtalOpt* p)
  : AbstractTab(parent, p),
    m_timer(new QTimer(this)), m_mutex(new QMutex), m_update_mutex(new QMutex),
    m_update_all_mutex(new QMutex), m_context_mutex(new QMutex),
    m_context_xtal(0), m_tableRows(0)
{
  // Allow queued connections to work with the TableEntry struct
  qRegisterMetaType<XO_Prog_TableEntry>("XO_Prog_TableEntry");

  ui.setupUi(m_tab_widget);

  QHeaderView* horizontal = ui.table_list->horizontalHeader();
  horizontal->setSectionResizeMode(QHeaderView::ResizeToContents);

  rowTracking = true;

  // dialog connections
  connect(m_search, &SearchBase::sessionStarted, this, &TabProgress::startTimer);

  // Progress table connections
  connect(m_timer.get(), &QTimer::timeout, this, &TabProgress::updateProgressTable);
  connect(ui.push_refresh, &QPushButton::clicked, this, &TabProgress::startTimer);
  connect(ui.push_refresh, &QPushButton::clicked, this, &TabProgress::updateProgressTable);
  connect(ui.spin_period, &QAbstractSpinBox::editingFinished, this, &TabProgress::startTimer);
  connect(ui.spin_period, &QAbstractSpinBox::editingFinished,
          this, &TabProgress::updateProgressTable);
  connect(m_search->tracker(), &Tracker::newStructureAdded,
          this, [this](Structure*) { addNewEntry(); }, Qt::QueuedConnection);
  connect(m_search->queue(), &QueueManager::structureUpdated, this, &TabProgress::newInfoUpdate);
  connect(this, &TabProgress::infoUpdate,    this, &TabProgress::updateInfo);
  connect(ui.table_list, &QWidget::customContextMenuRequested,
          this, &TabProgress::progressContextMenu);
  connect(ui.push_refreshAll, &QPushButton::clicked, this, &TabProgress::updateAllInfo);
  connect(m_search, &SearchBase::refreshAllStructureInfo, this, &TabProgress::updateAllInfo);
  connect(m_search, &SearchBase::structureViewDataChanged, this, &TabProgress::updateAllInfo);
  connect(m_search, &SearchBase::startingSession, this, &TabProgress::disableRowTracking);
  connect(m_search, &SearchBase::sessionStarted, this, &TabProgress::enableRowTracking);
  connect(m_search, &SearchBase::structuresAboutToBeDeleted, this, &TabProgress::releaseStructureReferences);
  connect(this, &TabProgress::updateTableEntry, this, &TabProgress::setTableEntry);
  connect(ui.push_clear, &QPushButton::clicked, this, &TabProgress::clearFiles);
  // This is a potentially demanding task: refresh info after hull calculations
  connect(m_search->queue(), &QueueManager::hullCalculationFinished,
          this, &TabProgress::refreshHullFrontEntries);
  connect(ui.push_hull, &QPushButton::clicked, this, &TabProgress::refreshHullFrontEntries);

  initialize();
}

TabProgress::~TabProgress()
{
  releaseStructureReferences();
}

void TabProgress::lockGUI()
{
  if (m_search->isReadOnly())
    ui.push_clear->setDisabled(true);
}

void TabProgress::disconnectGUI()
{
  m_timer->disconnect();
  ui.push_refresh->disconnect();
  ui.push_refreshAll->disconnect();
  ui.spin_period->disconnect();
  ui.table_list->disconnect();
  disconnect(m_search->tracker(), 0, this, 0);
  disconnect(m_search->queue(), 0, this, 0);
  disconnect(m_dialog, 0, this, 0);
  this->disconnect();
}

void TabProgress::updateProgressTable()
{
  // Only allow one update at a time
  if (!m_update_mutex->tryLock()) {
    Common::debug(QString("%1: Killing extra call")
                 .arg(__func__));
    return;
  }

  QList<Structure*> running = m_search->queue()->getAllRunningStructures();

  for (QList<Structure *>::iterator it = running.begin(),
                                    it_end = running.end();
       it != it_end; ++it) {
    newInfoUpdate(*it);
  }

  m_update_mutex->unlock();
}

void TabProgress::addNewEntry()
{
  // Prevent XtalOpt threads from modifying the table
  QtCompat::MutexLocker locker(m_mutex.get());

  // The new entry will be at the end of the table, so determine the index:
  int index = ui.table_list->rowCount();
  QReadLocker trackerLocker(m_search->tracker()->rwLock());
  if (index >= m_search->tracker()->size())
    return;
  Xtal* xtal = qobject_cast<Xtal*>(m_search->tracker()->at(index));
  if (!xtal)
    return;

  // Turn off signals
  ui.table_list->blockSignals(true);

  // Store current index for later. If -1, this will be re-set at the end of
  // table
  int currentInd = ui.table_list->currentRow();
  if (currentInd >= ui.table_list->rowCount() - 1)
    currentInd = -1;

  // Add the new row
  ui.table_list->insertRow(index);
  m_tableRows.store(ui.table_list->rowCount());
  // Columns: once for each column in ProgressColumns:
  for (int i = 0; i <= ProgressColumns::Ancestry; i++) {
    ui.table_list->setItem(index, i, new QTableWidgetItem());
  }

  {
    QWriteLocker wl(m_infoUpdateTracker.rwLock());
    m_infoUpdateTracker.append(xtal);
  }

  XO_Prog_TableEntry e;
  {
    QReadLocker xtalLocker(&xtal->lock());
    e.elapsed = xtal->getOptElapsed();
    e.tag = xtal->getTag();
    e.formula = xtal->getChemicalFormula();
    e.parents = xtal->getParents();
    e.jobID = xtal->getJobID();
    e.volume = xtal->getVolumePerAtom();
    e.status = "Waiting for data...";
    e.brush = QBrush(Qt::white);
    e.pen = QBrush(Qt::black);
    e.spg = QString::number(xtal->getSpaceGroupNumber()) + ": " +
            xtal->getSpaceGroupSymbol();

    if (xtal->hasEnthalpy() || xtal->getEnergy() != 0)
      e.enthalpy =
        xtal->getEnthalpyPerAtom();
    else
      e.enthalpy = 0.0;

    e.abovehull = xtal->getDistAboveHull();
    e.front = xtal->getParetoFront();
  }

  ui.table_list->blockSignals(false);

  if (currentInd < 0)
    currentInd = index;
  if (rowTracking)
    ui.table_list->setCurrentCell(currentInd, 0);

  locker.unlock();

  setTableEntry(index, e);
  emit infoUpdate();
}

void TabProgress::updateAllInfo()
{
  if (!m_update_all_mutex->tryLock()) {
    Common::debug(QString("%1: Killing extra call")
                 .arg(__func__));
    return;
  }
  bool appended = false;
  {
    QReadLocker trackerLocker(m_search->tracker()->rwLock());
    QWriteLocker infoUpdateTrackerLocker(m_infoUpdateTracker.rwLock());
    QList<Structure*>* structures = m_search->tracker()->list();
    const int count = qMin(ui.table_list->rowCount(), static_cast<int>(structures->size()));

    for (int i = 0; i < count; ++i) {
      appended = m_infoUpdateTracker.append(structures->at(i)) || appended;
    }
  }

  if (appended)
    emit infoUpdate();
  m_update_all_mutex->unlock();
}

void TabProgress::newInfoUpdate(Structure* s)
{
  bool appended;
  {
    QWriteLocker wl(m_infoUpdateTracker.rwLock());
    appended = m_infoUpdateTracker.append(s);
  }
  if (appended)
    emit infoUpdate();
}

void TabProgress::updateInfo()
{
  {
    QReadLocker locker(m_infoUpdateTracker.rwLock());
    if (m_infoUpdateTracker.size() == 0)
      return;
  }

  // Don't update while a context operation is in the works
  {
    QtCompat::MutexLocker contextLocker(m_context_mutex.get());
    if (m_context_xtal != 0) {
      /*
      Common::debug(QString("%1: Waiting for context operation to complete. "
                           "Trying again very soon.").arg(__func__));
      */
      QTimer::singleShot(PROGRESS_REFRESH_DELAY, this, &TabProgress::updateInfo);
      return;
    }
  }

  QPointer<TabProgress> self(this);
  (void)QtConcurrent::run(&m_workerPool, [self]() {
    if (self)
      self->updateInfo_();
  });
}

void TabProgress::updateInfo_()
{
  // Background color for statuses
  //
  // Active runs (optimization, queued, scripts)
  static const QColor colInProcess    = QColor(  0, 255,   0);
  static const QColor colScriptCalc   = QColor(255, 255,   0);
  // Transitions
  static const QColor colWaiting      = QColor(  0, 139, 139);
  static const QColor colSubmitted    = QColor(120, 145, 145);
  static const QColor colTransition   = QColor(120, 145, 145);
  static const QColor colError        = QColor(120, 145, 145);
  // Failures and errors
  static const QColor colScriptFail   = QColor(220, 150, 150);
  static const QColor colTerminalFail = QColor(139,   0,   0);
  // Optimized (including dismissed, similar)
  static const QColor colDismiss      = QColor(160, 205, 245);
  static const QColor colOptimized    = QColor(  0,   0, 255);
  static const QColor colSimilar      = QColor(  0, 100,   0);

  for (;;) {
    Structure* structure = nullptr;
    {
      QWriteLocker wl(m_infoUpdateTracker.rwLock());
      if (!m_infoUpdateTracker.popFirst(structure))
        return;
    }

    QReadLocker trackerLocker(m_search->tracker()->rwLock());
    int i = m_search->tracker()->list()->indexOf(structure);

    Xtal* xtal = qobject_cast<Xtal*>(structure);
    if (!xtal)
      continue;

    // Check the current row count.
    if (i < 0 || i >= m_tableRows.load()) {
      Common::debug(QString("%1: Trying to update an index that "
                           "doesn't exist yet: (%2) Waiting ...")
                   .arg(__func__).arg(i));
      {
        QWriteLocker wl(m_infoUpdateTracker.rwLock());
        m_infoUpdateTracker.append(xtal);
      }
      // Wait for the new table row.
      return;
    }

    XO_Prog_TableEntry e;
    uint totalOptSteps = m_search->getNumOptSteps();
    e.brush = QBrush(Qt::white);
    e.pen = QBrush(Qt::black);

    QReadLocker xtalLocker(&xtal->lock());
    e.elapsed = xtal->getOptElapsed();
    e.tag = xtal->getTag();
    e.formula = xtal->getChemicalFormula();
    e.parents = xtal->getParents();
    e.jobID = xtal->getJobID();
    e.volume = xtal->getVolumePerAtom();
    e.spg = QString::number(xtal->getSpaceGroupNumber()) + ": " +
            xtal->getSpaceGroupSymbol();

    if (xtal->hasEnthalpy() || xtal->getEnergy() != 0)
      e.enthalpy =
        xtal->getEnthalpyPerAtom();
    else
      e.enthalpy = 0.0;

    e.abovehull = xtal->getDistAboveHull();
    e.front = xtal->getParetoFront();

    const Xtal::State lifecycleState = xtal->getStatus();
    switch (lifecycleState) {
      // A structure with a job in the queue. The engine owns the job status;
      //   we only show the text that it has saved for us.
      case Xtal::InProcess:
        // Wait for the session to start.
        if (m_search->isSessionStarting()) {
          e.status = tr("In process (loading session)...");
          break;
        }
        e.status = tr("%1 (Opt Step %2 of %3, %4 failures)")
                     .arg(xtal->statusText(true))
                     .arg(QString::number(xtal->getCurrentOptStep() + 1))
                     .arg(QString::number(totalOptSteps))
                     .arg(QString::number(xtal->getFailCount()));
        e.brush.setColor(colInProcess);
        break;
      // Structures waiting for or moving between optimization steps.
      case Xtal::Submitted:
        if (xtal->getJobID() == 0) {
          e.status = tr("Submitted, job ID unavailable: inspect scheduler "
                        "(Opt Step %1 of %2)")
                       .arg(QString::number(xtal->getCurrentOptStep() + 1))
                       .arg(QString::number(totalOptSteps));
        } else {
          e.status = tr("%1 (%2 of %3)")
                       .arg(xtal->statusText(true))
                       .arg(QString::number(xtal->getCurrentOptStep() + 1))
                       .arg(QString::number(totalOptSteps));
        }
        e.brush.setColor(colSubmitted);
        break;
      case Xtal::Restart:
      case Xtal::StepOptimized:
      case Xtal::Updating:
        e.status = xtal->statusText(true);
        e.brush.setColor(colTransition);
        break;

      // A structure is waiting for the next optimization job.
      case Xtal::WaitingForOptimization:
        e.status = tr("%1 (%2 of %3)")
                     .arg(xtal->statusText(true))
                     .arg(QString::number(xtal->getCurrentOptStep() + 1))
                     .arg(QString::number(totalOptSteps));
        e.brush.setColor(colWaiting);
        break;

      // Objective and constraint calculations are in progress.
      case Xtal::ScriptCalculation:
      case Xtal::ObjectiveCalculation:
      case Xtal::ConstraintCalculation:
        e.status = xtal->statusText(true);
        e.brush.setColor(colScriptCalc);
        break;

      // Terminal script calculation failures.
      case Xtal::ObjcFailed:
      case Xtal::ConsFailed:
        e.status = xtal->statusText(true);
        e.brush.setColor(colScriptFail);
        break;

      // Terminal failed/removed structures.
      case Xtal::Failed:
      case Xtal::Removed:
        e.status = xtal->statusText(true);
        e.brush.setColor(colTerminalFail);
        e.pen.setColor(Qt::white);
        break;

      // Terminal dismissal of structure by constraints.
      case Xtal::Dismissed:
        e.status = xtal->statusText(true);
        e.brush.setColor(colDismiss);
        break;

      // Finished structures.
      case Xtal::Optimized:
        e.status = xtal->statusText(true);
        if (xtal->isSimilar()) {
          e.brush.setColor(colSimilar);
        } else {
          e.brush.setColor(colOptimized);
        }
        e.pen.setColor(Qt::white);
        break;

      // A temporary error may still be handled by the failure policy.
      case Xtal::Error:
        e.status = xtal->statusText(true);
        e.brush.setColor(colError);
        break;

      case Xtal::Empty:
        e.status = xtal->statusText(true);
        break;
      default:
        break;
    }


    // The below override is commented out! With that, any restored structure
    //   with previous failure would be shown with "darkRed" color (ie, the one
    //   for the failed/removed) until it was successfully optimized. Without this,
    //   the color coding will reflect the actual current status (failure count
    //   is still an indicator of history!)
    /*
    if (xtal->getFailCount() != 0) {
      e.brush.setColor(Qt::darkRed);
    }
    */

    emit updateTableEntry(i, e);
  }
}


// Update hull and front entries in the table.
void TabProgress::refreshHullFrontEntries()
{
  if (!m_update_all_mutex->tryLock()) {
    Common::debug(QString("%1: Killing extra table refresh call)")
                 .arg(__func__));
    return;
  }

  // Don't update while a context operation is in the works
  {
    QtCompat::MutexLocker contextLocker(m_context_mutex.get());
    if (m_context_xtal != 0) {
      Common::debug(QString("%1: Waiting for context operation to complete.").arg(__func__));
      m_update_all_mutex->unlock();
      return;
    }
  }

  // Copy the structure list.
  QList<Structure*> structures;
  {
    QReadLocker trackerLocker(m_search->tracker()->rwLock());
    structures = *m_search->tracker()->list();
  }

  // Lock the table
  QtCompat::MutexLocker locker(m_mutex.get());

  for (int row = 0; row < ui.table_list->rowCount() && row < structures.size(); row++) {
    Xtal* xtal = qobject_cast<Xtal*>(structures.at(row));
    if (!xtal)
      continue;

    QReadLocker xtalLocker(&xtal->lock());

    // Read in the latest values
    double abovehull = xtal->getDistAboveHull();
    int    front     = xtal->getParetoFront();

    // Update the table entries
    if (!GS_ISNAN(abovehull))
      ui.table_list->item(row, AboveHull)->setText(QString("%1").arg(abovehull, 12, 'f', 6));
    else
      ui.table_list->item(row, AboveHull)->setText("N/A");

    if (front >= 0)
      ui.table_list->item(row, Front)->setText(QString::number(front));
    else
      ui.table_list->item(row, Front)->setText("N/A");
  }

  m_update_all_mutex->unlock();
}

void TabProgress::setTableEntry(int row, const XO_Prog_TableEntry& e)
{
  // Lock the table
  QtCompat::MutexLocker locker(m_mutex.get());
  if (row < 0 || row >= ui.table_list->rowCount() || !ui.table_list->item(row, TimeElapsed))
    return;

  ui.table_list->item(row, TimeElapsed)->setText(e.elapsed);
  ui.table_list->item(row, Tag)->setText(e.tag);
  ui.table_list->item(row, Formula)->setText(e.formula);
  ui.table_list->item(row, Ancestry)->setText(e.parents);
  ui.table_list->item(row, SpaceGroup)->setText(e.spg);
  ui.table_list->item(row, Volume)->setText(QString::number(e.volume, 'f', 2));
  ui.table_list->item(row, Status)->setText(e.status);
  ui.table_list->item(row, Status)->setBackground(e.brush);
  ui.table_list->item(row, Status)->setForeground(e.pen);

  if (e.jobID)
    ui.table_list->item(row, JobID)->setText(QString::number(e.jobID));
  else
    ui.table_list->item(row, JobID)->setText("N/A");

  if (e.enthalpy != 0)
    ui.table_list->item(row, Enthalpy)->setText(QString("%1").arg(e.enthalpy, 12, 'f', 6));
  else
    ui.table_list->item(row, Enthalpy)->setText("N/A");

  if (!GS_ISNAN(e.abovehull))
    ui.table_list->item(row, AboveHull)->setText(QString("%1").arg(e.abovehull, 12, 'f', 6));
  else
    ui.table_list->item(row, AboveHull)->setText("N/A");

  if (e.front >= 0)
    ui.table_list->item(row, Front)->setText(QString::number(e.front));
  else
    ui.table_list->item(row, Front)->setText("N/A");
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

void TabProgress::releaseStructureReferences()
{
  m_timer->stop();
  m_workerPool.clear();
  m_workerPool.waitForDone();

  {
    QWriteLocker locker(m_infoUpdateTracker.rwLock());
    m_infoUpdateTracker.reset();
  }
  {
    QtCompat::MutexLocker locker(m_context_mutex.get());
    m_context_xtal = nullptr;
  }
  {
    QtCompat::MutexLocker locker(m_mutex.get());
    ui.table_list->clearContents();
    ui.table_list->setRowCount(0);
    m_tableRows.store(0);
  }
}

void TabProgress::progressContextMenu(QPoint p)
{
  QTableWidgetItem* item = ui.table_list->itemAt(p);
  bool xtalIsSelected = true;
  int index = -1;
  if (item == nullptr) {
    xtalIsSelected = false;
  } else {
    index = item->row();
  }

  Xtal* xtal = nullptr;
  if (index != -1) {
    QReadLocker trackerLocker(m_search->tracker()->rwLock());
    if (index < m_search->tracker()->size())
      xtal = qobject_cast<Xtal*>(m_search->tracker()->at(index));
  }

  // Keep this lock short: menu.exec() runs a nested event loop, and the
  //   update functions also need it.
  if (!m_context_mutex->tryLock(100)) {
    return;
  }
  if (m_context_xtal) {
    m_context_mutex->unlock();
    return;
  }
  m_context_xtal = xtal;
  m_context_mutex->unlock();

  // Used to determine available options:
  bool canGenerateOffspring = (this->m_search->getParentPoolSize() >= 3);
  const bool readOnly = m_search->isReadOnly();

  bool isStopped = false;
  bool canUnfail = false;
  if (xtal != nullptr) {
    QReadLocker xtalLocker(&xtal->lock());
    isStopped = xtal->isStoppedFinalState();
    canUnfail = xtal->isFailedOrRemovedState();
    /*
    Common::debug(QString("%1: Context menu at row %2 for structure %3")
                  .arg(__func__).arg(index).arg(xtal->getTag()));
    */
  }

  QMenu menu;
  QAction* a_restart = menu.addAction("&Restart job");
  QAction* a_fail = menu.addAction("&Fail structure");
  QAction* a_unfail = menu.addAction("&Unfail structure");
  QAction* a_resetFail = menu.addAction("R&eset failure count");
  menu.addSeparator();
  QAction* a_randomize = menu.addAction("Replace with &new random structure");
  QAction* a_offspring = menu.addAction("Replace with new &offspring");
  menu.addSeparator();
  QAction* a_injectSeed = menu.addAction("Inject &seed structure");
  menu.addSeparator();
  QAction* a_clipPOSCAR = menu.addAction("&Copy POSCAR to clipboard");
  menu.addSeparator();
  QAction* a_viewStructure = menu.addAction("&View Structure");
  menu.addSeparator();
  QAction* a_plotXrd = menu.addAction("View Simulated &XRD Pattern");

  // Connect actions
  connect(a_restart,    &QAction::triggered, this, &TabProgress::restartJobProgress);
  connect(a_fail,       &QAction::triggered, this, &TabProgress::failXtalProgress);
  connect(a_unfail,     &QAction::triggered, this, &TabProgress::unfailXtalProgress);
  connect(a_resetFail,  &QAction::triggered, this, &TabProgress::resetFailureCountProgress);
  connect(a_randomize,  &QAction::triggered, this, &TabProgress::randomizeStructureProgress);
  connect(a_offspring,  &QAction::triggered, this, &TabProgress::replaceWithOffspringProgress);
  connect(a_injectSeed, &QAction::triggered, this, &TabProgress::injectStructureProgress);
  connect(a_clipPOSCAR, &QAction::triggered, this, &TabProgress::clipPOSCARProgress);
  connect(a_viewStructure, &QAction::triggered, this, &TabProgress::viewStructureProgress);
  connect(a_plotXrd,    &QAction::triggered, this, &TabProgress::plotXrdProgress);

  // Disable / hide illogical operations
  if (isStopped) {
    a_fail->setVisible(false);
    a_restart->setVisible(false);
  } else {
    a_unfail->setVisible(false);
  }
  if (!canUnfail)
    a_unfail->setVisible(false);

  if (!canGenerateOffspring) {
    a_offspring->setDisabled(true);
  }

  if (readOnly) {
    a_restart->setEnabled(false);
    a_fail->setEnabled(false);
    a_unfail->setEnabled(false);
    a_resetFail->setEnabled(false);
    a_randomize->setEnabled(false);
    a_offspring->setEnabled(false);
    a_injectSeed->setEnabled(false);
  }

  if (!xtalIsSelected) {
    a_restart->setEnabled(false);
    a_fail->setEnabled(false);
    a_unfail->setEnabled(false);
    a_resetFail->setEnabled(false);
    a_randomize->setEnabled(false);
    a_offspring->setEnabled(false);
    a_injectSeed->setEnabled(!readOnly);
    a_clipPOSCAR->setEnabled(false);
    a_viewStructure->setEnabled(false);
    a_plotXrd->setEnabled(false);
  }

  QAction* selection = menu.exec(QCursor::pos());

  if (selection == 0) {
    QtCompat::MutexLocker contextLocker(m_context_mutex.get());
    m_context_xtal = 0;
    return;
  }
}

void TabProgress::restartJobProgress()
{
  QPointer<Xtal> xtal;
  {
    QtCompat::MutexLocker contextLocker(m_context_mutex.get());
    xtal = m_context_xtal;
  }
  if (!xtal)
    return;

  // Get info from xtal
  int optstep;
  QString tag;
  {
    QReadLocker xtalLocker(&xtal->lock());
    optstep = xtal->getCurrentOptStep();
    tag = xtal->getTag();
    if (xtal->getStatus() == Xtal::Submitted && xtal->getJobID() == 0) {
      xtalLocker.unlock();

      const QMessageBox::StandardButton answer = QMessageBox::question(
        m_dialog, tr("Unconfirmed scheduler job"),
        tr("The scheduler accepted this job, but XtalOpt could not read its job ID.\n\n"
           "Inspect the scheduler and cancel the job manually before restarting it. "
           "Continue only after doing that."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

      if (answer != QMessageBox::Yes) {
        QtCompat::MutexLocker contextLocker(m_context_mutex.get());
        m_context_xtal = nullptr;
        return;
      }
    }
  }

  // Choose which OptStep to use
  bool ok;
  int optStep = QInputDialog::getInt(
    m_dialog, tr("Restart Optimization %1").arg(tag),
    "Select optimization step to restart from:", optstep + 1, 1,
    m_search->getNumOptSteps(), 1, &ok);
  --optStep;

  if (!ok) {
    QtCompat::MutexLocker contextLocker(m_context_mutex.get());
    m_context_xtal = 0;
    return;
  }
  emit startingBackgroundProcessing();
  QPointer<TabProgress> self(this);
  (void)QtConcurrent::run(&m_workerPool, [self, optStep]() {
    if (self)
      self->restartJobProgress_(optStep);
  });
}

void TabProgress::restartJobProgress_(int optStep)
{
  QPointer<Xtal> xtal;
  {
    QtCompat::MutexLocker contextLocker(m_context_mutex.get());
    xtal = m_context_xtal;
  }
  if (!xtal) {
    emit finishedBackgroundProcessing();
    return;
  }

  bool wasOptimized = false;
  {
    QWriteLocker locker(&xtal->lock());
    wasOptimized = xtal->getStatus() == Xtal::Optimized;
    xtal->setRestartOptStep(optStep);
    xtal->setStatus(Xtal::Restart);
  }
  m_search->reportStructureStateChanged(xtal);
  // Restarting an optimized structure takes its point out of the hull.
  if (wasOptimized) {
    XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);
    xtalopt->handleOptimizedDeparture(xtal);
  }
  newInfoUpdate(xtal);

  // Clear context xtal pointer
  emit finishedBackgroundProcessing();
  QtCompat::MutexLocker contextLocker(m_context_mutex.get());
  m_context_xtal = 0;
}

void TabProgress::failXtalProgress()
{
  QPointer<Xtal> xtal;
  {
    QtCompat::MutexLocker contextLocker(m_context_mutex.get());
    xtal = m_context_xtal;
  }
  if (!xtal)
    return;

  {
    QReadLocker locker(&xtal->lock());
    if (xtal->getStatus() == Xtal::Submitted && xtal->getJobID() == 0) {
      locker.unlock();

      const QMessageBox::StandardButton answer = QMessageBox::question(
        m_dialog, tr("Unconfirmed scheduler job"),
        tr("The scheduler accepted this job, but XtalOpt could not read its job ID.\n\n"
           "Inspect the scheduler and cancel the job manually before stopping the "
           "structure. Continue only after doing that."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

      if (answer != QMessageBox::Yes) {
        QtCompat::MutexLocker contextLocker(m_context_mutex.get());
        m_context_xtal = nullptr;
        return;
      }
    }
  }

  emit startingBackgroundProcessing();
  QPointer<TabProgress> self(this);
  (void)QtConcurrent::run(&m_workerPool, [self]() {
    if (self)
      self->failXtalProgress_();
  });
}

void TabProgress::failXtalProgress_()
{
  QPointer<Xtal> xtal;
  {
    QtCompat::MutexLocker contextLocker(m_context_mutex.get());
    xtal = m_context_xtal;
  }
  if (!xtal) {
    emit finishedBackgroundProcessing();
    return;
  }

  // QueueManager will handle mutex locking
  m_search->queue()->failStructure(xtal);

  // Clear context xtal pointer
  emit finishedBackgroundProcessing();
  newInfoUpdate(xtal);
  QtCompat::MutexLocker contextLocker(m_context_mutex.get());
  m_context_xtal = 0;
}

void TabProgress::unfailXtalProgress()
{
  emit startingBackgroundProcessing();
  QPointer<TabProgress> self(this);
  (void)QtConcurrent::run(&m_workerPool, [self]() {
    if (self)
      self->unfailXtalProgress_();
  });
}

void TabProgress::unfailXtalProgress_()
{
  QPointer<Xtal> xtal;
  {
    QtCompat::MutexLocker contextLocker(m_context_mutex.get());
    xtal = m_context_xtal;
  }
  bool nowOptimized = false;
  bool changed = false;
  if (xtal) {
    QWriteLocker locker(&xtal->lock());
    if (xtal->isFailedOrRemovedState()) {
      // Restart a failed structure from its current optimization step.
      if (xtal->getStatus() == Xtal::Failed)
        xtal->setStatus(Xtal::Restart);
      else {
        xtal->setStatus(Xtal::Optimized);
        nowOptimized = true;
      }
      changed = true;
    }
  }

  if (changed)
    m_search->reportStructureStateChanged(xtal);
  if (changed && nowOptimized) {
    XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);
    xtalopt->requestStructureEvaluation(xtal);
  }

  emit finishedBackgroundProcessing();
  if (changed)
    newInfoUpdate(xtal);
  QtCompat::MutexLocker contextLocker(m_context_mutex.get());
  m_context_xtal = 0;
}

void TabProgress::resetFailureCountProgress()
{
  emit startingBackgroundProcessing();
  QPointer<TabProgress> self(this);
  (void)QtConcurrent::run(&m_workerPool, [self]() {
    if (self)
      self->resetFailureCountProgress_();
  });
}

void TabProgress::resetFailureCountProgress_()
{
  QPointer<Xtal> xtal;
  {
    QtCompat::MutexLocker contextLocker(m_context_mutex.get());
    xtal = m_context_xtal;
  }
  if (!xtal) {
    emit finishedBackgroundProcessing();
    return;
  }

  QWriteLocker locker(&xtal->lock());

  xtal->resetFailCount();

  locker.unlock();
  m_search->reportStructureStateChanged(xtal);

  // Clear context xtal pointer
  emit finishedBackgroundProcessing();
  newInfoUpdate(xtal);
  QtCompat::MutexLocker contextLocker(m_context_mutex.get());
  m_context_xtal = 0;
}

void TabProgress::randomizeStructureProgress()
{
  emit startingBackgroundProcessing();
  QPointer<TabProgress> self(this);
  (void)QtConcurrent::run(&m_workerPool, [self]() {
    if (self)
      self->randomizeStructureProgress_();
  });
}

void TabProgress::randomizeStructureProgress_()
{
  QPointer<Xtal> xtal;
  {
    QtCompat::MutexLocker contextLocker(m_context_mutex.get());
    xtal = m_context_xtal;
  }
  if (!xtal) {
    emit finishedBackgroundProcessing();
    return;
  }

  int jobID;
  int optStep;
  {
    QReadLocker locker(&xtal->lock());
    jobID = xtal->getJobID();
    optStep = xtal->getCurrentOptStep();
  }

  // End job if currently running
  if (jobID) {
    QueueInterface* queue = m_search->queueInterface(optStep);
    if (queue)
      queue->stopJob(xtal);
  }

  if (!m_search->replaceWithRandom(xtal, "manual")) {
    QString tag;
    {
      QReadLocker locker(&xtal->lock());
      tag = xtal->getTag();
      optStep = xtal->getCurrentOptStep();
    }
    Common::warning(tr("Manual random replacement failed for structure %1 in opt step %2.")
                       .arg(tag).arg(optStep + 1));
    emit finishedBackgroundProcessing();
    QtCompat::MutexLocker contextLocker(m_context_mutex.get());
    m_context_xtal = 0;
    return;
  }

  // Restart job:
  newInfoUpdate(xtal);
  restartJobProgress_(0);
  // above function handles background processing signal
}

void TabProgress::replaceWithOffspringProgress()
{
  emit startingBackgroundProcessing();
  QPointer<TabProgress> self(this);
  (void)QtConcurrent::run(&m_workerPool, [self]() {
    if (self)
      self->replaceWithOffspringProgress_();
  });
}

void TabProgress::replaceWithOffspringProgress_()
{
  QPointer<Xtal> xtal;
  {
    QtCompat::MutexLocker contextLocker(m_context_mutex.get());
    xtal = m_context_xtal;
  }
  if (!xtal) {
    emit finishedBackgroundProcessing();
    return;
  }

  int jobID;
  int optStep;
  {
    QReadLocker locker(&xtal->lock());
    jobID = xtal->getJobID();
    optStep = xtal->getCurrentOptStep();
  }

  // End job if currently running
  if (jobID) {
    QueueInterface* queue = m_search->queueInterface(optStep);
    if (queue)
      queue->stopJob(xtal);
  }

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);
  Q_ASSERT_X(xtalopt != nullptr, Q_FUNC_INFO, "m_search is not an instance of "
                                              "XtalOpt.");

  if (!xtalopt->replaceWithOffspring(xtal, "manual")) {
    QString tag;
    {
      QReadLocker locker(&xtal->lock());
      tag = xtal->getTag();
    }
    Common::warning(tr("Manual offspring replacement failed for structure %1.").arg(tag));
    emit finishedBackgroundProcessing();
    QtCompat::MutexLocker contextLocker(m_context_mutex.get());
    m_context_xtal = 0;
    return;
  }

  // Restart job:
  newInfoUpdate(xtal);
  restartJobProgress_(0);
  // above function handles background processing signal
}

void TabProgress::injectStructureProgress()
{
  // It doesn't matter what xtal was selected
  {
    QtCompat::MutexLocker contextLocker(m_context_mutex.get());
    m_context_xtal = nullptr;
  }

  // Prompt for filename
  QString filename = m_search->getLocWorkDir();

  // Launch file dialog
  QString newFilename = QFileDialog::getOpenFileName(
    m_dialog, QString("Select structure file to use as seed"), filename,
    "VASP input (*POSCAR *CONTCAR *.vasp);;" "PWSCF input/output (*.pwscf);;"
    "CASTEP input/output (*.cell *.castep);;" "SIESTA input/output (*.fdf *.siesta);;"
    "GULP output (*.got *.gout);;" "MTP input/output (*.cfg *.mot);;" "CIF (*.cif);;"
    "XYZ (*.xyz);;" "CML (*.cml);;" "All Files (*)", 0, QFileDialog::DontUseNativeDialog);

  // User canceled selection
  if (newFilename.isEmpty())
    return;

  // Load in background
  emit startingBackgroundProcessing();
  QPointer<TabProgress> self(this);
  (void)QtConcurrent::run(&m_workerPool, [self, newFilename]() {
    if (self)
      self->injectStructureProgress_(newFilename);
  });
}

void TabProgress::injectStructureProgress_(const QString& filename)
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);
  if (!xtalopt->addSeed(filename)) {
    Common::warning(tr("Seed injection failed for %1.").arg(filename));
  }
  emit finishedBackgroundProcessing();
}

void TabProgress::clipPOSCARProgress()
{
  emit startingBackgroundProcessing();
  clipPOSCARProgress_();
}

void TabProgress::clipPOSCARProgress_()
{
  QPointer<Xtal> xtal;
  {
    QtCompat::MutexLocker contextLocker(m_context_mutex.get());
    xtal = m_context_xtal;
  }
  if (!xtal) {
    emit finishedBackgroundProcessing();
    return;
  }

  std::stringstream poscarStream;
  {
    QReadLocker locker(&xtal->lock());
    Atoms::VaspFormat::write(*xtal, poscarStream, xtal->getLocpath());
  }
  const QString poscar = QString::fromStdString(poscarStream.str());
  if (!poscar.isEmpty()) {
    QApplication::clipboard()->setText(poscar, QClipboard::Clipboard);
    if (QApplication::clipboard()->supportsSelection())
      QApplication::clipboard()->setText(poscar, QClipboard::Selection);
  }

  // Clear context xtal pointer
  emit finishedBackgroundProcessing();
  QtCompat::MutexLocker contextLocker(m_context_mutex.get());
  m_context_xtal = 0;
}

void TabProgress::viewStructureProgress()
{
  QPointer<Xtal> xtal;
  {
    QtCompat::MutexLocker contextLocker(m_context_mutex.get());
    xtal = m_context_xtal;
  }
  if (!xtal)
    return;

  auto* dialog = qobject_cast<XtalOptDialog*>(m_dialog);
  if (dialog)
    dialog->showStructureViewer(xtal);

  QtCompat::MutexLocker contextLocker(m_context_mutex.get());
  m_context_xtal = nullptr;
}

void TabProgress::plotXrdProgress()
{
  QPointer<Xtal> xtal;
  {
    QtCompat::MutexLocker contextLocker(m_context_mutex.get());
    xtal = m_context_xtal;
  }
  if (!xtal)
    return;

  auto* dialog = qobject_cast<XtalOptDialog*>(m_dialog);
  if (dialog)
    dialog->showXrdViewer(xtal);

  QtCompat::MutexLocker contextLocker(m_context_mutex.get());
  m_context_xtal = nullptr;
}

void TabProgress::clearFiles()
{
  for (size_t i = 0; i < m_search->getNumOptSteps(); ++i) {
    const Optimizer* optimizer = m_search->optimizer(static_cast<int>(i));
    if (!optimizer ||
        optimizer->getIDString().compare("VASP", Qt::CaseInsensitive) != 0) {
      Common::warning(tr("Clear Extra Files is available only when every "
                         "optimization step uses VASP."));
      return;
    }
  }

  const QString runPath = m_search->getLocWorkDir();
  if (runPath.isEmpty())
    return;

  // The file removal can be slow (many structures, network filesystems),
  //   so it runs on the worker pool like the other heavy operations.
  emit startingBackgroundProcessing();
  QPointer<TabProgress> self(this);
  (void)QtConcurrent::run(&m_workerPool, [self]() {
    if (self)
      self->clearFiles_();
  });
}

void TabProgress::clearFiles_()
{
  QStringList optimizedStructurePaths;
  {
    QReadLocker trackerLocker(m_search->tracker()->rwLock());
    QList<Structure*>* structures = m_search->tracker()->list();
    for (Structure* structure : *structures) {
      if (!structure)
        continue;
      QReadLocker structureLocker(&structure->lock());
      if (structure->getStatus() == Structure::Optimized)
        optimizedStructurePaths.append(structure->getLocpath());
    }
  }

  for (const QString& structurePath : optimizedStructurePaths) {
    QDir dir(structurePath);
    if (!dir.exists())
      continue;

    const QFileInfoList entries = dir.entryInfoList(
      QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::AllDirs | QDir::Files,
      QDir::DirsFirst);
    for (const QFileInfo& info : entries) {
      if (info.fileName() != "CONTCAR" && info.fileName() != "structure.state" &&
          info.fileName() != "OUTCAR"  && info.fileName() != "structure.state.old") {
        dir.remove(info.fileName());
      }
    }
  }

  emit finishedBackgroundProcessing();
}
}

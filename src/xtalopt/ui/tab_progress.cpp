/**********************************************************************
  XtalOpt - Tools for advanced crystal optimization

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/ui/tab_progress.h>

#include <globalsearch/optbase.h>
#include <globalsearch/optimizer.h>
#include <globalsearch/queueinterface.h>
#include <globalsearch/queuemanager.h>
#include <globalsearch/tracker.h>
#include <globalsearch/ui/abstracttab.h>
#include <globalsearch/utilities/fileutils.h>
#include <globalsearch/xrd/generatexrd.h>

#include <xtalopt/structures/xtal.h>
#include <xtalopt/ui/dialog.h>
#include <xtalopt/ui/xrd_plot.h>
#include <xtalopt/xtalopt.h>

#include <QDesktopWidget>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QMutexLocker>
#include <QSettings>
#include <QTimer>
#include <QtConcurrent>

#include <QInputDialog>
#include <QMenu>

#include "ui_xrdOptionsDialog.h"

using namespace GlobalSearch;

namespace XtalOpt {

TabProgress::TabProgress(GlobalSearch::AbstractDialog* parent, XtalOpt* p)
  : AbstractTab(parent, p), m_ui_xrdOptionsDialog(new Ui::XrdOptionsDialog),
    m_xrdOptionsDialog(new QDialog(parent)),
    m_xrdPlotDialog(new QDialog(parent)),
    m_xrdPlot(new XrdPlot(m_xrdPlotDialog)),
    m_timer(new QTimer(this)), m_mutex(new QMutex), m_update_mutex(new QMutex),
    m_update_all_mutex(new QMutex), m_context_mutex(new QMutex),
    m_context_xtal(0)
{
  m_ui_xrdOptionsDialog->setupUi(m_xrdOptionsDialog);

  // Allow queued connections to work with the TableEntry struct
  qRegisterMetaType<XO_Prog_TableEntry>("XO_Prog_TableEntry");

  ui.setupUi(m_tab_widget);

  QHeaderView* horizontal = ui.table_list->horizontalHeader();
  horizontal->setSectionResizeMode(QHeaderView::ResizeToContents);

  rowTracking = true;

  // dialog connections
  connect(m_dialog, SIGNAL(moleculeChanged(GlobalSearch::Structure*)), this,
          SLOT(highlightXtal(GlobalSearch::Structure*)));
  connect(m_opt, SIGNAL(sessionStarted()), this, SLOT(startTimer()));

  // Progress table connections
  connect(m_timer, SIGNAL(timeout()), this, SLOT(updateProgressTable()));
  connect(ui.push_refresh, SIGNAL(clicked()), this, SLOT(startTimer()));
  connect(ui.push_refresh, SIGNAL(clicked()), this,
          SLOT(updateProgressTable()));
  connect(ui.spin_period, SIGNAL(editingFinished()), this,
          SLOT(updateProgressTable()));
  connect(ui.table_list, SIGNAL(currentCellChanged(int, int, int, int)), this,
          SLOT(selectMoleculeFromProgress(int, int, int, int)));
  connect(m_opt->tracker(), SIGNAL(newStructureAdded(GlobalSearch::Structure*)),
          this, SLOT(addNewEntry()), Qt::QueuedConnection);
  connect(m_opt->queue(), SIGNAL(structureUpdated(GlobalSearch::Structure*)),
          this, SLOT(newInfoUpdate(GlobalSearch::Structure*)));
  connect(this, SIGNAL(infoUpdate()), this, SLOT(updateInfo()));
  connect(ui.table_list, SIGNAL(customContextMenuRequested(QPoint)), this,
          SLOT(progressContextMenu(QPoint)));
  connect(ui.push_refreshAll, SIGNAL(clicked()), this, SLOT(updateAllInfo()));
  connect(m_opt, SIGNAL(refreshAllStructureInfo()), this,
          SLOT(updateAllInfo()));
  connect(m_opt, SIGNAL(startingSession()), this, SLOT(disableRowTracking()));
  connect(m_opt, SIGNAL(sessionStarted()), this, SLOT(enableRowTracking()));
  connect(this, SIGNAL(updateTableEntry(int, const XO_Prog_TableEntry&)), this,
          SLOT(setTableEntry(int, const XO_Prog_TableEntry&)));
  connect(ui.push_rank, SIGNAL(clicked()), this, SLOT(updateRank()));
  connect(ui.push_print, SIGNAL(clicked()), this, SLOT(printFile()));
  connect(ui.push_clear, SIGNAL(clicked()), this, SLOT(clearFiles()));

  initialize();
}

TabProgress::~TabProgress()
{
  delete m_ui_xrdOptionsDialog;
  delete m_xrdOptionsDialog;
  delete m_xrdPlot;
  delete m_xrdPlotDialog; // This needs to go after m_xrdPlot
  delete m_mutex;
  delete m_update_mutex;
  delete m_update_all_mutex;
  delete m_context_mutex;
  delete m_timer;
}

void TabProgress::writeSettings(const QString& filename)
{
  SETTINGS(filename);
  const int version = 1;
  settings->beginGroup("xtalopt/progress");
  settings->setValue("version", version);
  settings->setValue("refreshTime", ui.spin_period->value());
  settings->endGroup();
}

void TabProgress::readSettings(const QString& filename)
{
  SETTINGS(filename);
  settings->beginGroup("xtalopt/progress");
  ui.spin_period->setValue(settings->value("refreshTime", 1).toInt());
  settings->endGroup();
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
  QMutexLocker locker(m_mutex);

  // The new entry will be at the end of the table, so determine the index:
  int index = ui.table_list->rowCount();
  QReadLocker trackerLocker(m_opt->tracker()->rwLock());
  Xtal* xtal = qobject_cast<Xtal*>(m_opt->tracker()->at(index));
  // qDebug() << "TabProgress::addNewEntry() at index " << index;

  // Turn off signals
  ui.table_list->blockSignals(true);

  // Store current index for later. If -1, this will be re-set at the end of
  // table
  int currentInd = ui.table_list->currentRow();
  if (currentInd >= ui.table_list->rowCount() - 1)
    currentInd = -1;

  // Add the new row
  ui.table_list->insertRow(index);
  // Columns: once for each column in ProgressColumns:
  for (int i = 0; i < 10; i++) {
    ui.table_list->setItem(index, i, new QTableWidgetItem());
  }

  m_infoUpdateTracker.lockForWrite();
  m_infoUpdateTracker.append(xtal);
  m_infoUpdateTracker.unlock();

  XO_Prog_TableEntry e;
  xtal->lock().lockForRead();
  e.elapsed = xtal->getOptElapsed();
  e.gen = xtal->getGeneration();
  e.id = xtal->getIDNumber();
  e.parents = xtal->getParents();
  e.jobID = xtal->getJobID();
  e.volume = xtal->getVolume();
  e.status = "Waiting for data...";
  e.brush = QBrush(Qt::white);
  e.pen = QBrush(Qt::black);
  e.spg = QString::number(xtal->getSpaceGroupNumber()) + ": " +
          xtal->getSpaceGroupSymbol();
  e.FU = xtal->getFormulaUnits();

  if (xtal->hasEnthalpy() || xtal->getEnergy() != 0)
    e.enthalpy =
      xtal->getEnthalpy() /
      static_cast<double>(xtal->getFormulaUnits()); // PSA Enthalpy per atom
  else
    e.enthalpy = 0.0;

  xtal->lock().unlock();

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
    qDebug() << "Killing extra TabProgress::updateAllInfo() call";
    return;
  }
  QReadLocker trackerLocker(m_opt->tracker()->rwLock());
  QWriteLocker infoUpdateTrackerLocker(m_infoUpdateTracker.rwLock());
  QList<Structure*>* structures = m_opt->tracker()->list();
  for (int i = 0; i < ui.table_list->rowCount(); i++) {
    m_infoUpdateTracker.append(structures->at(i));
    emit infoUpdate();
  }
  m_update_all_mutex->unlock();
}

void TabProgress::newInfoUpdate(Structure* s)
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
  if (m_context_xtal != 0) {
    qDebug()
      << "TabProgress::updateInfo: Waiting for context operation to complete ("
      << m_context_xtal << ") Trying again very soon.";
    QTimer::singleShot(1000, this, SLOT(updateInfo()));
    return;
  }

  QtConcurrent::run(this, &TabProgress::updateInfo_);
  return;
}

void TabProgress::updateInfo_()
{
  // Prep variables
  Structure* structure;
  m_infoUpdateTracker.lockForWrite();
  if (!m_infoUpdateTracker.popFirst(structure)) {
    m_infoUpdateTracker.unlock();
    return;
  }
  m_infoUpdateTracker.unlock();

  QReadLocker trackerLocker(m_opt->tracker()->rwLock());
  int i = m_opt->tracker()->list()->indexOf(structure);

  Xtal* xtal = qobject_cast<Xtal*>(structure);

  if (i < 0 || i > ui.table_list->rowCount() - 1) {
    qDebug() << "TabProgress::updateInfo: Trying to update an index that "
                "doesn't exist...yet: ("
             << i << ") Waiting...";
    m_infoUpdateTracker.lockForWrite();
    m_infoUpdateTracker.append(xtal);
    m_infoUpdateTracker.unlock();
    QTimer::singleShot(100, this, SLOT(updateInfo()));
    return;
  }

  XO_Prog_TableEntry e;
  uint totalOptSteps = m_opt->getNumOptSteps();
  e.brush = QBrush(Qt::white);
  e.pen = QBrush(Qt::black);

  QReadLocker xtalLocker(&xtal->lock());
  e.elapsed = xtal->getOptElapsed();
  e.gen = xtal->getGeneration();
  e.id = xtal->getIDNumber();
  e.parents = xtal->getParents();
  e.jobID = xtal->getJobID();
  e.volume = xtal->getVolume();
  e.spg = QString::number(xtal->getSpaceGroupNumber()) + ": " +
          xtal->getSpaceGroupSymbol();
  e.FU = xtal->getFormulaUnits();

  if (xtal->hasEnthalpy() || xtal->getEnergy() != 0)
    e.enthalpy =
      xtal->getEnthalpy() /
      static_cast<double>(xtal->getFormulaUnits()); // PSA Enthalpy per atom
  else
    e.enthalpy = 0.0;

  switch (xtal->getStatus()) {
    case Xtal::InProcess: {
      xtalLocker.unlock();
      QueueInterface::QueueStatus state =
        m_opt->queueInterface(xtal->getCurrentOptStep())->getStatus(xtal);
      xtalLocker.relock();
      switch (state) {
        case QueueInterface::Running:
          e.status = tr("Running (Opt Step %1 of %2, %3 failures)")
                       .arg(QString::number(xtal->getCurrentOptStep() + 1))
                       .arg(QString::number(totalOptSteps))
                       .arg(QString::number(xtal->getFailCount()));
          e.brush.setColor(Qt::green);
          break;
        case QueueInterface::Queued:
          e.status = tr("Queued (Opt Step %1 of %2, %3 failures)")
                       .arg(QString::number(xtal->getCurrentOptStep() + 1))
                       .arg(QString::number(totalOptSteps))
                       .arg(QString::number(xtal->getFailCount()));
          e.brush.setColor(Qt::cyan);
          break;
        case QueueInterface::Success:
          e.status = "Starting update...";
          break;
        case QueueInterface::Unknown:
          e.status = "Unknown";
          e.brush.setColor(Qt::red);
          break;
        case QueueInterface::Error:
          e.status = "Error: Restarting job...";
          e.brush.setColor(Qt::darkRed);
          break;
        case QueueInterface::CommunicationError:
          e.status = "Communication Error";
          e.brush.setColor(Qt::darkRed);
          break;
        // Shouldn't happen; started and pending only occur when xtal is
        // "Submitted"
        case QueueInterface::Started:
        case QueueInterface::Pending:
        default:
          break;
      }
      break;
    }
    case Xtal::Submitted:
      e.status = tr("Job submitted (%1 of %2)")
                   .arg(QString::number(xtal->getCurrentOptStep() + 1))
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
    case Xtal::ObjectiveDismiss:
      e.status = "ObjectiveDismiss";
      e.brush.setColor(Qt::darkGray);
      break;
    case Xtal::ObjectiveFail:
      e.status = "ObjectiveFail";
      e.brush.setColor(Qt::darkGray);
      break;
    case Xtal::ObjectiveRetain:
    case Xtal::ObjectiveCalculation:
      e.status = "Calculating objectives...";
      e.brush.setColor(Qt::yellow);
      break;
    case Xtal::Duplicate:
      e.status = tr("Duplicate of %1").arg(xtal->getDuplicateString());
      e.brush.setColor(Qt::darkGreen);
      break;
    case Xtal::Supercell:
      e.status = tr("Supercell of %1").arg(xtal->getSupercellString());
      e.brush.setColor(QColor(204, 102, 0, 255));
      break;
    case Xtal::StepOptimized:
      e.status = "Checking status...";
      e.brush.setColor(Qt::cyan);
      break;
    case Xtal::Optimized:
      if (xtal->skippedOptimization()) {
        e.status = "Skipped Optimization";
        e.brush.setColor(QColor(138, 43, 226, 255));
        e.pen.setColor(Qt::white);
      } else {
        e.status = "Optimized";
        e.brush.setColor(Qt::blue);
        e.pen.setColor(Qt::white);
      }
      break;
    case Xtal::WaitingForOptimization:
      e.status = tr("Waiting for Optimization (%1 of %2)")
                   .arg(QString::number(xtal->getCurrentOptStep() + 1))
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

void TabProgress::setTableEntry(int row, const XO_Prog_TableEntry& e)
{
  // Lock the table
  QMutexLocker locker(m_mutex);

  ui.table_list->item(row, TimeElapsed)->setText(e.elapsed);
  ui.table_list->item(row, Gen)->setText(QString::number(e.gen));
  ui.table_list->item(row, Mol)->setText(QString::number(e.id));
  ui.table_list->item(row, Ancestry)->setText(e.parents);
  ui.table_list->item(row, SpaceGroup)->setText(e.spg);
  ui.table_list->item(row, Volume)->setText(QString::number(e.volume, 'f', 2));
  ui.table_list->item(row, Status)->setText(e.status);
  ui.table_list->item(row, Status)->setBackground(e.brush);
  ui.table_list->item(row, Status)->setForeground(e.pen);
  ui.table_list->item(row, FU)->setText(QString::number(e.FU));

  if (e.jobID)
    ui.table_list->item(row, JobID)->setText(QString::number(e.jobID));
  else
    ui.table_list->item(row, JobID)->setText("N/A");

  if (e.enthalpy != 0)
    ui.table_list->item(row, Enthalpy)->setText(QString::number(e.enthalpy));
  else
    ui.table_list->item(row, Enthalpy)->setText("N/A");
}

void TabProgress::selectMoleculeFromProgress(int row, int, int oldrow, int)
{
  Q_UNUSED(oldrow);
  if (m_opt->isStarting) {
    // qDebug() << "TabProgress::selectMoleculeFromProgress: Not updating widget
    // while session is starting";
    return;
  }
  if (row == -1)
    return;
  emit moleculeChanged(qobject_cast<Xtal*>(m_opt->tracker()->at(row)));
}

void TabProgress::highlightXtal(Structure* s)
{
  Xtal* xtal = qobject_cast<Xtal*>(s);
  xtal->lock().lockForRead();
  int gen = xtal->getGeneration();
  int id = xtal->getIDNumber();
  xtal->lock().unlock();
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
  if (m_context_xtal)
    return;
  // m_context_mutex prevents multiple menus from appearing, which
  // ultimately prevents m_context_xtal from being cleared.
  if (!m_context_mutex->tryLock(100)) {
    return;
  }

  QTableWidgetItem* item = ui.table_list->itemAt(p);
  bool xtalIsSelected = true;
  int index = -1;
  if (item == nullptr) {
    xtalIsSelected = false;
  } else {
    index = item->row();
  }

  // Used to determine available options:
  bool canGenerateOffspring =
    (this->m_opt->queue()->getAllOptimizedStructures().size() >= 3);

  qDebug() << "Context menu at row " << index;

  // Set m_context_xtal after locking to avoid threading issues.
  Xtal* xtal = nullptr;
  if (index != -1) {
    xtal = qobject_cast<Xtal*>(m_opt->tracker()->at(index));
  }

  bool isKilled = false;
  if (xtal != nullptr) {
    xtal->lock().lockForRead();
    m_context_xtal = xtal;

    Xtal::State state = m_context_xtal->getStatus();
    isKilled = (state == Xtal::Killed || state == Xtal::Removed ||
                state == Xtal::ObjectiveFail || state == Xtal::ObjectiveDismiss);

    xtal->lock().unlock();
  }

  QMenu menu;
  QAction* a_restart = menu.addAction("&Restart job");
  QAction* a_kill = menu.addAction("&Kill structure");
  QAction* a_unkill = menu.addAction("Un&kill structure");
  QAction* a_resetFail = menu.addAction("Reset &failure count");
  menu.addSeparator();
  QAction* a_randomize = menu.addAction("Replace with &new random structure");
  QAction* a_offspring = menu.addAction("Replace with new &offspring");
  menu.addSeparator();
  QAction* a_injectSeed = menu.addAction("Inject &seed structure");
  menu.addSeparator();
  QAction* a_clipPOSCAR = menu.addAction("&Copy POSCAR to clipboard");
  menu.addSeparator();
  QAction* a_plotXrd = menu.addAction("Plot Simulated XRD Pattern");

  // Connect actions
  connect(a_restart, SIGNAL(triggered()), this, SLOT(restartJobProgress()));
  connect(a_kill, SIGNAL(triggered()), this, SLOT(killXtalProgress()));
  connect(a_unkill, SIGNAL(triggered()), this, SLOT(unkillXtalProgress()));
  connect(a_resetFail, SIGNAL(triggered()), this,
          SLOT(resetFailureCountProgress()));
  connect(a_randomize, SIGNAL(triggered()), this,
          SLOT(randomizeStructureProgress()));
  connect(a_offspring, SIGNAL(triggered()), this,
          SLOT(replaceWithOffspringProgress()));
  connect(a_injectSeed, SIGNAL(triggered()), this,
          SLOT(injectStructureProgress()));
  connect(a_clipPOSCAR, SIGNAL(triggered()), this, SLOT(clipPOSCARProgress()));
  connect(a_plotXrd, SIGNAL(triggered()), this, SLOT(plotXrdProgress()));

  // Disable / hide illogical operations
  if (isKilled) {
    a_kill->setVisible(false);
    a_restart->setVisible(false);
  } else {
    a_unkill->setVisible(false);
  }

  if (!canGenerateOffspring) {
    a_offspring->setDisabled(true);
  }

  if (!xtalIsSelected) {
    a_restart->setEnabled(false);
    a_kill->setEnabled(false);
    a_unkill->setEnabled(false);
    a_resetFail->setEnabled(false);
    a_randomize->setEnabled(false);
    a_offspring->setEnabled(false);
    a_injectSeed->setEnabled(true);
    a_clipPOSCAR->setEnabled(false);
    a_plotXrd->setEnabled(false);
  }

  QAction* selection = menu.exec(QCursor::pos());

  if (selection == 0) {
    m_context_xtal = 0;
    m_context_mutex->unlock();
    return;
  }

  a_restart->disconnect();
  a_kill->disconnect();
  a_unkill->disconnect();
  a_resetFail->disconnect();
  if (canGenerateOffspring) {
    a_offspring->disconnect();
  }
  a_injectSeed->disconnect();
  a_randomize->disconnect();

  m_context_mutex->unlock();
}

void TabProgress::restartJobProgress()
{
  if (!m_context_xtal)
    return;

  // Get info from xtal
  m_context_xtal->lock().lockForRead();
  int optstep = m_context_xtal->getCurrentOptStep();
  m_context_xtal->lock().unlock();

  // Choose which OptStep to use
  bool ok;
  int optStep = QInputDialog::getInt(
    m_dialog, tr("Restart Optimization %1").arg(m_context_xtal->getTag()),
    "Select optimization step to restart from:", optstep, 1,
    m_opt->getNumOptSteps(), 1, &ok);
  --optStep;

  if (!ok) {
    m_context_xtal = 0;
    return;
  }
  emit startingBackgroundProcessing();
  QtConcurrent::run(this, &TabProgress::restartJobProgress_, optStep);
}

void TabProgress::restartJobProgress_(int optStep)
{
  QWriteLocker locker(&m_context_xtal->lock());
  m_context_xtal->setCurrentOptStep(optStep);

  m_context_xtal->setStatus(Xtal::Restart);
  newInfoUpdate(m_context_xtal);

  // Clear context xtal pointer
  emit finishedBackgroundProcessing();
  locker.unlock();
  m_context_xtal = 0;
}

void TabProgress::killXtalProgress()
{
  emit startingBackgroundProcessing();
  QtConcurrent::run(this, &TabProgress::killXtalProgress_);
}

void TabProgress::killXtalProgress_()
{
  if (!m_context_xtal) {
    emit finishedBackgroundProcessing();
    return;
  }

  // QueueManager will handle mutex locking
  m_opt->queue()->killStructure(m_context_xtal);

  // Clear context xtal pointer
  emit finishedBackgroundProcessing();
  newInfoUpdate(m_context_xtal);
  m_context_xtal = 0;
}

void TabProgress::unkillXtalProgress()
{
  emit startingBackgroundProcessing();
  QtConcurrent::run(this, &TabProgress::unkillXtalProgress_);
}

void TabProgress::unkillXtalProgress_()
{
  if (!m_context_xtal) {
    emit finishedBackgroundProcessing();
    return;
  }

  QWriteLocker locker(&m_context_xtal->lock());
  if (m_context_xtal->getStatus() != Xtal::Killed &&
      m_context_xtal->getStatus() != Xtal::Removed &&
      m_context_xtal->getStatus() != Xtal::ObjectiveFail &&
      m_context_xtal->getStatus() != Xtal::ObjectiveDismiss) {
    emit finishedBackgroundProcessing();
    return;
  }

  // Setting status to Xtal::Error will restart the job if was killed
  if (m_context_xtal->getStatus() == Xtal::Killed)
    m_context_xtal->setStatus(Xtal::Error);
  // Set status to Optimized if xtal was previously optimized
  else if (m_context_xtal->getStatus() == Xtal::Removed ||
           m_context_xtal->getStatus() == Xtal::ObjectiveFail ||
           m_context_xtal->getStatus() == Xtal::ObjectiveDismiss) {
    m_context_xtal->setStatus(Xtal::Optimized);
  }

  // Clear context xtal pointer
  emit finishedBackgroundProcessing();
  newInfoUpdate(m_context_xtal);
  locker.unlock();
  m_context_xtal = 0;
}

void TabProgress::resetFailureCountProgress()
{
  emit startingBackgroundProcessing();
  QtConcurrent::run(this, &TabProgress::resetFailureCountProgress_);
}

void TabProgress::resetFailureCountProgress_()
{
  if (!m_context_xtal) {
    emit finishedBackgroundProcessing();
    return;
  }

  QWriteLocker locker(&m_context_xtal->lock());

  m_context_xtal->resetFailCount();

  // Clear context xtal pointer
  emit finishedBackgroundProcessing();
  newInfoUpdate(m_context_xtal);
  locker.unlock();
  m_context_xtal = 0;
}

void TabProgress::randomizeStructureProgress()
{
  emit startingBackgroundProcessing();
  QtConcurrent::run(this, &TabProgress::randomizeStructureProgress_);
}

void TabProgress::randomizeStructureProgress_()
{
  if (!m_context_xtal) {
    emit finishedBackgroundProcessing();
    return;
  }

  // End job if currently running
  if (m_context_xtal->getJobID()) {
    m_opt->queueInterface(m_context_xtal->getCurrentOptStep())
      ->stopJob(m_context_xtal);
  }

  m_opt->replaceWithRandom(m_context_xtal, "manual");

  // Restart job:
  newInfoUpdate(m_context_xtal);
  restartJobProgress_(0);
  // above function handles background processing signal
}

void TabProgress::replaceWithOffspringProgress()
{
  emit startingBackgroundProcessing();
  QtConcurrent::run(this, &TabProgress::replaceWithOffspringProgress_);
}

void TabProgress::replaceWithOffspringProgress_()
{
  if (!m_context_xtal) {
    emit finishedBackgroundProcessing();
    return;
  }

  // End job if currently running
  if (m_context_xtal->getJobID()) {
    m_opt->queueInterface(m_context_xtal->getCurrentOptStep())
      ->stopJob(m_context_xtal);
  }

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_opt);
  Q_ASSERT_X(xtalopt != nullptr, Q_FUNC_INFO, "m_opt is not an instance of "
                                              "XtalOpt.");

  xtalopt->replaceWithOffspring(m_context_xtal, "manual");

  // Restart job:
  newInfoUpdate(m_context_xtal);
  restartJobProgress_(1);
  // above function handles background processing signal
}

void TabProgress::injectStructureProgress()
{
  // It doesn't matter what xtal was selected
  m_context_xtal = nullptr;

  // Prompt for filename
  QSettings settings;
  QString filename =
    settings.value("xtalopt/opt/seedPath", m_opt->locWorkDir).toString();

  // Launch file dialog
  QString newFilename = QFileDialog::getOpenFileName(
    m_dialog, QString("Select structure file to use as seed"), filename,
    "Common formats (*POSCAR *CONTCAR *.got *.cml *cif"
    " *.out);;All Files (*)",
    0, QFileDialog::DontUseNativeDialog);

  // User canceled selection
  if (newFilename.isEmpty())
    return;

  settings.setValue("xtalopt/opt/seedPath", newFilename);

  // Load in background
  QtConcurrent::run(this, &TabProgress::injectStructureProgress_, newFilename);
}

void TabProgress::injectStructureProgress_(const QString& filename)
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_opt);
  xtalopt->addSeed(filename);
}

void TabProgress::clipPOSCARProgress()
{
  emit startingBackgroundProcessing();
  QtConcurrent::run(this, &TabProgress::clipPOSCARProgress_);
}

void TabProgress::clipPOSCARProgress_()
{
  if (!m_context_xtal) {
    emit finishedBackgroundProcessing();
    return;
  }
  QReadLocker locker(&m_context_xtal->lock());

  QString poscar = m_context_xtal->toPOSCAR();

  m_opt->setClipboard(poscar);

  // Clear context xtal pointer
  emit finishedBackgroundProcessing();
  locker.unlock();
  m_context_xtal = 0;
}

void TabProgress::plotXrdProgress()
{
  if (!m_context_xtal)
    return;

  QReadLocker locker(&m_context_xtal->lock());

  // Save these options for use in the current run
  static double wavelength = 1.5056;
  static double peakwidth = 0.52958;
  static size_t numpoints = 1000;
  static double max2theta = 162.0;

  m_ui_xrdOptionsDialog->spin_wavelength->setValue(wavelength);
  m_ui_xrdOptionsDialog->spin_peakwidth->setValue(peakwidth);
  m_ui_xrdOptionsDialog->spin_numpoints->setValue(numpoints);
  m_ui_xrdOptionsDialog->spin_max2theta->setValue(max2theta);

  if (m_xrdOptionsDialog->exec() != QDialog::Accepted) {
    m_context_xtal = 0;
    return;
  }

  wavelength = m_ui_xrdOptionsDialog->spin_wavelength->value();
  peakwidth = m_ui_xrdOptionsDialog->spin_peakwidth->value();
  numpoints = m_ui_xrdOptionsDialog->spin_numpoints->value();
  max2theta = m_ui_xrdOptionsDialog->spin_max2theta->value();

  GlobalSearch::XrdData results;
  if (!GlobalSearch::GenerateXrd::generateXrdPattern(*m_context_xtal, results,
                                                     wavelength, peakwidth,
                                                     numpoints, max2theta)) {
    qDebug() << "GenerateXrd failed for xtal " << m_context_xtal->getTag();
    m_context_xtal = 0;
    return;
  }

  if (m_xrdPlotDialog->isVisible())
    m_xrdPlotDialog->hide();

  m_xrdPlot->clearPlotCurves();
  m_xrdPlot->addXrdData(results);
  m_xrdPlotDialog->resize(600, 600);
  m_xrdPlot->resize(600, 600);
  m_xrdPlot->setAxisAutoScale(QwtPlot::yLeft);
  m_xrdPlot->setAxisAutoScale(QwtPlot::xBottom);
  m_xrdPlot->replot();
  m_xrdPlotDialog->show();

  // Clear context xtal pointer
  m_context_xtal = 0;
}

void TabProgress::updateRank()
{
  /*
       Optimizer* opti = m_opt->optimizer();
       QString runpath = m_opt->locWorkDir;
        QDir dir(runpath+"/ranked");
        QDir cifDir(runpath+"/ranked/CIF");
        QDir contDir(runpath+"/ranked/CONTCAR");
        QDir gotDir(runpath+"/ranked/GOT");

     if(dir.exists()) FileUtils::removeDir(runpath+"/ranked");
     dir.mkpath(".");
     cifDir.mkpath(".");
     if (opti->getIDString() == "VASP") contDir.mkpath(".");
     else if (opti->getIDString() == "GULP") gotDir.mkpath(".");

     int gen, id;
     QString space, stat, pathName, rank, gen_s, id_s, enthalpy;
     QFile results (runpath+"/results.txt");
        if(!results.open(QIODevice::ReadOnly)) {
            return;
        }
      size_t rankCounter = 0;
      // Skip over the first line in the results.txt file
      QString line = results.readLine();
      while (!results.atEnd()) {
          line  = results.readLine();
          gen_s = line.split(QRegExp("\\s"), QString::SkipEmptyParts)[1];
          id_s  = line.split(QRegExp("\\s"), QString::SkipEmptyParts)[2];
          stat  = line.split(QRegExp("\\s"), QString::SkipEmptyParts)[6];
          if (stat != "Optimized") continue;
          rankCounter++;
          rank = QString::number(rankCounter);
          gen=gen_s.toInt();
          id=id_s.toInt();
          gen_s.sprintf("%05d", gen);
          id_s.sprintf("%05d", id);

          if (opti->getIDString() == "VASP") {
              QFile file (runpath+"/" +gen_s+ "x" +id_s+ "/CONTCAR");
              QFile potFile (runpath+"/" +gen_s+ "x" +id_s+ "/POTCAR");
              QFile newFile (runpath+"/ranked/CONTCAR/" + rank +
     "-"+gen_s+"x"+id_s);
              if(file.exists()) {
                  if(potFile.exists()) {
                      file.copy(newFile.fileName());
                      file.close();
                      newFile.close();
                      QString command = "obabel -iVASP
     \""+runpath+"\"/\""+gen_s+"\"x\""+id_s+"\"/CONTCAR -ocif -O
     \""+runpath+"\"/ranked/CIF/\""+rank+"\"-\""+gen_s+"\"x\""+id_s+"\".cif";
                      system(qPrintable(command));
                  } else {
                      QFile tempFile (runpath+"/CONTCAR");
                      file.copy(tempFile.fileName());
                      file.close();
                      newFile.close();
                      tempFile.close();
                      QString command = "obabel -iVASP \""+runpath+"\"/CONTCAR
     -ocif -O
     \""+runpath+"\"/ranked/CIF/\""+rank+"\"-\""+gen_s+"\"x\""+id_s+"\".cif";
                      system(qPrintable(command));
                      QFile::remove(runpath+"/CONTCAR");
                  }
              }
          } else if (opti->getIDString() == "GULP") {
              QFile file (runpath+"/" +gen_s+ "x" +id_s+ "/xtal.got");
              QFile newFile (runpath+"/ranked/GOT/" + rank +
     "-"+gen_s+"x"+id_s+".got");
              if(file.exists()) {
                      file.copy(newFile.fileName());
                      file.close();
                      newFile.close();
                      QString command = "obabel -igot
     \""+runpath+"\"/\""+gen_s+"\"x\""+id_s+"\"/xtal.got -ocif -O
     \""+runpath+"\"/ranked/CIF/\""+rank+"\"-\""+gen_s+"\"x\""+id_s+"\".cif";
                      system(qPrintable(command));
              }
          }
      }
  */
}

void TabProgress::printFile()
{
  QFile file;
  file.setFileName(m_opt->locWorkDir + "/run-results.txt");
  if (!file.open(QIODevice::WriteOnly)) {
    m_opt->error("TabProgress::printFile(): Error opening file " +
                 file.fileName() + " for writing...");
  }
  QTextStream out;
  out.setDevice(&file);
  m_opt->tracker()->lockForRead();
  QList<Structure*>* structures = m_opt->tracker()->list();
  Xtal* xtal;

  // Print the data to the file:
  out << "Index\tGen\tID\tEnthalpy/FU\tFU\tSpaceGroup\t\tStatus\t\tParentage\n";
  for (int i = 0; i < structures->size(); i++) {
    xtal = qobject_cast<Xtal*>(structures->at(i));
    if (!xtal)
      continue; // In case there was a problem copying.
    xtal->lock().lockForRead();
    QString gen_s, id_s, enthalpy, formulaUnits, space;
    int gen = xtal->getGeneration();
    int id = xtal->getIDNumber();
    double en =
      xtal->getEnthalpy() / static_cast<double>(xtal->getFormulaUnits());
    int FU = xtal->getFormulaUnits();
    space = xtal->getSpaceGroupSymbol();
    space = space.leftJustified(10, ' ');
    out << i << "\t" << gen_s.sprintf("%u", gen) << "\t"
        << id_s.sprintf("%u", id) << "\t" << enthalpy.sprintf("%.4f", en)
        << "\t" << formulaUnits.sprintf("%u", FU) << "\t"
        << xtal->getSpaceGroupNumber() << ": " << space << "\t\t";

    // Status:
    switch (xtal->getStatus()) {
      case Xtal::Optimized:
        out << "Optimized";
        break;
      case Xtal::Killed:
      case Xtal::Removed:
        out << "Killed";
        break;
      case Xtal::Duplicate:
        out << "Duplicate";
        break;
      case Xtal::Supercell:
        out << "Supercell";
        break;
      case Xtal::Error:
        out << "Error";
        break;
      case Xtal::ObjectiveDismiss:
        out << "ObjectiveDismiss";
        break;
      case Xtal::ObjectiveFail:
        out << "ObjectiveFail";
        break;
      case Xtal::ObjectiveRetain:
      case Xtal::ObjectiveCalculation:
        out << "ObjectiveCalculation";
        break;
      case Xtal::StepOptimized:
      case Xtal::WaitingForOptimization:
      case Xtal::Submitted:
      case Xtal::InProcess:
      case Xtal::Empty:
      case Xtal::Updating:
        out << "Opt Step " << xtal->getCurrentOptStep() + 1;
        break;
      default:
        out << "In progress";
        break;
    }

    // Parentage:
    out << "\t" << xtal->getParents();
    xtal->lock().unlock();
    out << endl;
  }
  m_opt->tracker()->unlock();
}

void TabProgress::clearFiles()
{
  int gen, id;
  QString stat, gen_s, id_s;
  QString runath = m_opt->locWorkDir;
  QFile results(runath + "/results.txt");
  if (!results.open(QIODevice::ReadOnly))
    return;

  // Skip over the first line in the results.txt file
  QString line = results.readLine();
  while (!results.atEnd()) {
    line = results.readLine();
    gen_s = line.split(QRegExp("\\s"), QString::SkipEmptyParts)[1];
    id_s = line.split(QRegExp("\\s"), QString::SkipEmptyParts)[2];
    stat = line.split(QRegExp("\\s"), QString::SkipEmptyParts)[6];
    gen = gen_s.toInt();
    id = id_s.toInt();
    gen_s.sprintf("%05d", gen);
    id_s.sprintf("%05d", id);
    if (stat == "Optimized") {
      QDir dir(runath + "/" + gen_s + "x" + id_s);
      if (dir.exists()) {
        Q_FOREACH (QFileInfo info,
                   dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System |
                                       QDir::Hidden | QDir::AllDirs |
                                       QDir::Files,
                                     QDir::DirsFirst)) {
          if (info.fileName() == "POTCAR") {
            QFile file(info.filePath());
            QFile newFile(runath + "/POTCAR");
            if (!newFile.exists()) {
              file.copy(newFile.fileName());
              newFile.link(runath + "/POTCAR",
                           runath + "/" + gen_s + "x" + id_s + "/POTCAR");
              file.close();
              newFile.close();
              dir.remove(info.fileName());
            } else {
              dir.remove(info.fileName());
            }
            if (info.fileName() != "CONTCAR" &&
                info.fileName() != "structure.state" &&
                info.fileName() != "OUTCAR" && info.fileName() != "POTCAR") {
              dir.remove(info.fileName());
            }
          }
        }
      }
    }
  }
}
}

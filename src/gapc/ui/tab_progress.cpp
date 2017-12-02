/**********************************************************************
  TabProgress - Table showing the progress of the search

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <gapc/ui/tab_progress.h>

#include <gapc/gapc.h>
#include <gapc/structures/protectedcluster.h>
#include <gapc/ui/dialog.h>

#include <globalsearch/macros.h>
#include <globalsearch/optimizer.h>
#include <globalsearch/queueinterface.h>
#include <globalsearch/queuemanager.h>

#include <QBrush>
#include <QInputDialog>
#include <QMenu>
#include <QMutexLocker>
#include <QSettings>
#include <QTimer>
#include <QtConcurrentRun>

using namespace GlobalSearch;

namespace GAPC {

TabProgress::TabProgress(GAPCDialog* parent, OptGAPC* p)
  : AbstractTab(parent, p), m_timer(new QTimer(this)), m_mutex(new QMutex),
    m_update_mutex(new QMutex), m_update_all_mutex(new QMutex), m_context_pc(0)
{
  ui.setupUi(m_tab_widget);

  QHeaderView* horizontal = ui.table_list->horizontalHeader();
  horizontal->setResizeMode(QHeaderView::ResizeToContents);

  rowTracking = true;

  // dialog connections
  connect(m_dialog, SIGNAL(moleculeChanged(GlobalSearch::Structure*)), this,
          SLOT(highlightPC(GlobalSearch::Structure*)));

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
          this, SLOT(addNewEntry()));
  connect(m_opt->queue(), SIGNAL(structureUpdated(GlobalSearch::Structure*)),
          this, SLOT(newInfoUpdate(GlobalSearch::Structure*)));
  connect(this, SIGNAL(infoUpdate()), this, SLOT(updateInfo()),
          Qt::QueuedConnection);
  connect(ui.table_list, SIGNAL(customContextMenuRequested(QPoint)), this,
          SLOT(progressContextMenu(QPoint)));
  connect(ui.push_refreshAll, SIGNAL(clicked()), this, SLOT(updateAllInfo()));
  connect(m_opt, SIGNAL(refreshAllStructureInfo()), this,
          SLOT(updateAllInfo()));
  connect(m_opt, SIGNAL(startingSession()), this, SLOT(disableRowTracking()));
  connect(m_opt, SIGNAL(sessionStarted()), this, SLOT(enableRowTracking()));

  initialize();
}

TabProgress::~TabProgress()
{
  delete m_mutex;
  delete m_update_mutex;
  delete m_update_all_mutex;
  delete m_timer;
}

void TabProgress::writeSettings(const QString& filename)
{
  SETTINGS(filename);
  const int version = 1;
  settings->beginGroup("gapc/progress");
  settings->setValue("version", version);
  settings->setValue("refreshTime", ui.spin_period->value());
  settings->endGroup();
  DESTROY_SETTINGS(filename);
}

void TabProgress::readSettings(const QString& filename)
{
  SETTINGS(filename);
  settings->beginGroup("gapc/progress");
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

  if (!m_opt) {
    m_update_mutex->unlock();
    return;
  }

  if (m_opt->tracker()->size() == 0) {
    m_update_mutex->unlock();
    return;
  }

  m_update_mutex->unlock();
}

void TabProgress::addNewEntry()
{
  // Prevent GPAC threads from modifying the table
  QMutexLocker locker(m_mutex);

  // The new entry will be at the end of the table, so determine the index:
  int index = ui.table_list->rowCount();
  ProtectedCluster* pc =
    qobject_cast<ProtectedCluster*>(m_opt->tracker()->at(index));
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
  for (int i = 0; i < 9; i++) {
    ui.table_list->setItem(index, i, new QTableWidgetItem());
  }

  m_infoUpdateTracker.lockForWrite();
  m_infoUpdateTracker.append(pc);
  m_infoUpdateTracker.unlock();
  locker.unlock();
  emit infoUpdate();

  ui.table_list->blockSignals(false);

  if (currentInd < 0)
    currentInd = index;
  if (rowTracking)
    ui.table_list->setCurrentCell(currentInd, 0);
}

void TabProgress::updateAllInfo()
{
  if (!m_update_all_mutex->tryLock()) {
    qDebug() << "Killing extra TabProgress::updateAllInfo() call";
    return;
  }

  m_opt->tracker()->lockForRead();
  m_infoUpdateTracker.lockForWrite();
  QList<Structure*>* structures = m_opt->tracker()->list();
  for (int i = 0; i < ui.table_list->rowCount(); i++) {
    m_infoUpdateTracker.append(structures->at(i));
    emit infoUpdate();
  }
  m_infoUpdateTracker.unlock();
  m_opt->tracker()->unlock();
  m_update_all_mutex->unlock();
}

void TabProgress::newInfoUpdate(Structure* s)
{
  m_infoUpdateTracker.lockForWrite();
  m_infoUpdateTracker.append(s);
  m_infoUpdateTracker.unlock();
  emit infoUpdate();
}

void TabProgress::updateInfo()
{
  if (m_infoUpdateTracker.size() == 0) {
    return;
  }

  // Don't update while a context operation is in the works
  if (m_context_pc != 0) {
    qDebug()
      << "TabProgress::updateInfo: Waiting for context operation to complete ("
      << m_context_pc << ") Trying again very soon.";
    QTimer::singleShot(1000, this, SLOT(updateInfo()));
    return;
  }

  // Lock the table
  QMutexLocker locker(m_mutex);

  // Prep variables
  Structure* structure;
  m_infoUpdateTracker.lockForWrite();
  if (!m_infoUpdateTracker.popFirst(structure)) {
    m_infoUpdateTracker.unlock();
    return;
  }
  m_infoUpdateTracker.unlock();

  int i = m_opt->tracker()->list()->indexOf(structure);

  ProtectedCluster* pc = qobject_cast<ProtectedCluster*>(structure);

  if (i < 0 || i > ui.table_list->rowCount() - 1) {
    qDebug() << "TabProgress::updateInfo: Trying to update an index that "
                "doesn't exist ("
             << i << ") Waiting...";
    m_infoUpdateTracker.lockForWrite();
    m_infoUpdateTracker.append(pc);
    m_infoUpdateTracker.unlock();
    QTimer::singleShot(100, this, SLOT(updateInfo()));
    return;
  }

  QString time;
  uint totalOptSteps = m_opt->optimizer()->getNumberOfOptSteps();
  QBrush brush(Qt::white);

  QReadLocker pcLocker(pc->lock());

  time = pc->getOptElapsed();

  ui.table_list->item(i, TimeElapsed)->setText(time);

  ui.table_list->item(i, Gen)->setText(QString::number(pc->getGeneration()));
  ui.table_list->item(i, Mol)->setText(QString::number(pc->getIDNumber()));
  ui.table_list->item(i, Ancestry)->setText(pc->getParents());

  if (pc->getJobID())
    ui.table_list->item(i, JobID)->setText(QString::number(pc->getJobID()));
  else
    ui.table_list->item(i, JobID)->setText("N/A");

  if (pc->hasEnthalpy() || pc->getEnergy() != 0)
    ui.table_list->item(i, Enthalpy)
      ->setText(QString::number(pc->getEnthalpy()));
  else
    ui.table_list->item(i, Enthalpy)->setText("N/A");

  switch (pc->getStatus()) {
    case ProtectedCluster::InProcess: {
      pcLocker.unlock();
      QueueInterface::QueueStatus state =
        m_opt->queueInterface()->getStatus(pc);
      pcLocker.relock();
      switch (state) {
        case QueueInterface::Running:
          ui.table_list->item(i, Status)->setText(
            tr("Running (Opt Step %1 of %2, %3 failures)")
              .arg(QString::number(pc->getCurrentOptStep()))
              .arg(QString::number(totalOptSteps))
              .arg(QString::number(pc->getFailCount())));
          brush.setColor(Qt::green);
          break;
        case QueueInterface::Queued:
          ui.table_list->item(i, Status)->setText(
            tr("Queued (Opt Step %1 of %2, %3 failures)")
              .arg(QString::number(pc->getCurrentOptStep()))
              .arg(QString::number(totalOptSteps))
              .arg(QString::number(pc->getFailCount())));
          brush.setColor(Qt::cyan);
          break;
        case QueueInterface::Success:
          ui.table_list->item(i, Status)->setText("Starting update...");
          break;
        case QueueInterface::Unknown:
          ui.table_list->item(i, Status)->setText("Unknown");
          break;
        case QueueInterface::Error:
          ui.table_list->item(i, Status)->setText("Error: Restarting job...");
          brush.setColor(Qt::darkRed);
          break;
        case QueueInterface::CommunicationError:
          ui.table_list->item(i, Status)->setText("Comm. Error");
          brush.setColor(Qt::darkRed);
          break;
        // Shouldn't happen; started and pending only occur when pc is
        // "Submitted"
        case QueueInterface::Started:
        case QueueInterface::Pending:
        default:
          break;
      }
      break;
    }
    case ProtectedCluster::Submitted:
      ui.table_list->item(i, Status)->setText(
        tr("Job submitted (%1 of %2)")
          .arg(QString::number(pc->getCurrentOptStep()))
          .arg(QString::number(totalOptSteps)));
      brush.setColor(Qt::cyan);
      break;
    case ProtectedCluster::Restart:
      ui.table_list->item(i, Status)->setText("Restarting job...");
      brush.setColor(Qt::cyan);
      break;
    case ProtectedCluster::Killed:
    case ProtectedCluster::Removed:
      brush.setColor(Qt::darkGray);
      ui.table_list->item(i, Status)->setText("Killed");
      break;
    case ProtectedCluster::Duplicate:
      brush.setColor(Qt::darkGreen);
      ui.table_list->item(i, Status)->setText(
        tr("Duplicate of %1").arg(pc->getDuplicateString()));
      break;
    case ProtectedCluster::StepOptimized:
      ui.table_list->item(i, Status)->setText("Checking status...");
      brush.setColor(Qt::cyan);
      break;
    case ProtectedCluster::Optimized:
      ui.table_list->item(i, Status)->setText("Optimized");
      brush.setColor(Qt::yellow);
      break;
    case ProtectedCluster::WaitingForOptimization:
      ui.table_list->item(i, Status)->setText(
        tr("Waiting for Optimization (%1 of %2)")
          .arg(QString::number(pc->getCurrentOptStep()))
          .arg(QString::number(totalOptSteps)));
      brush.setColor(Qt::darkCyan);
      break;
    case ProtectedCluster::Error:
      ui.table_list->item(i, Status)->setText(tr("Job failed"));
      brush.setColor(Qt::red);
      break;
    case ProtectedCluster::Updating:
      ui.table_list->item(i, Status)->setText("Updating structure...");
      brush.setColor(Qt::cyan);
      break;
    case ProtectedCluster::Empty:
      ui.table_list->item(i, Status)->setText("Structure empty...");
      break;
  }

  if (pc->getFailCount() != 0) {
    brush.setColor(Qt::red);
  }
  // paint cell:
  ui.table_list->item(i, Status)->setBackground(brush);
}

void TabProgress::selectMoleculeFromProgress(int row, int, int oldrow, int)
{
  Q_UNUSED(oldrow);
  if (m_opt->isStarting) {
    qDebug() << "TabProgress::selectMoleculeFromProgress: Not updating widget "
                "while session is starting";
    return;
  }
  if (row == -1)
    return;
  emit moleculeChanged(
    qobject_cast<ProtectedCluster*>(m_opt->tracker()->at(row)));
}

void TabProgress::highlightPC(Structure* s)
{
  ProtectedCluster* pc = qobject_cast<ProtectedCluster*>(s);
  pc->lock()->lockForRead();
  int gen = pc->getGeneration();
  int id = pc->getIDNumber();
  pc->lock()->unlock();
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
  if (m_context_pc)
    return;
  QApplication::setOverrideCursor(Qt::WaitCursor);
  QTableWidgetItem* item = ui.table_list->itemAt(p);
  if (!item) {
    QApplication::restoreOverrideCursor();
    return;
  }
  int index = item->row();

  qDebug() << "Context menu at row " << index;

  // Set m_context_pc after locking to avoid threading issues.
  ProtectedCluster* pc =
    qobject_cast<ProtectedCluster*>(m_opt->tracker()->at(index));

  pc->lock()->lockForRead();

  m_context_pc = pc;

  QApplication::restoreOverrideCursor();

  ProtectedCluster::State state = m_context_pc->getStatus();

  QMenu menu;
  QAction* a_restart = menu.addAction("&Restart job");
  QAction* a_kill = menu.addAction("&Kill structure");
  QAction* a_unkill = menu.addAction("Un&kill structure");
  QAction* a_resetFail = menu.addAction("Reset &failure count");
  menu.addSeparator();
  QAction* a_randomize = menu.addAction("Replace with &new random structure");

  // Connect actions
  connect(a_restart, SIGNAL(triggered()), this, SLOT(restartJobProgress()));
  connect(a_kill, SIGNAL(triggered()), this, SLOT(killPCProgress()));
  connect(a_unkill, SIGNAL(triggered()), this, SLOT(unkillPCProgress()));
  connect(a_resetFail, SIGNAL(triggered()), this,
          SLOT(resetFailureCountProgress()));
  connect(a_randomize, SIGNAL(triggered()), this,
          SLOT(randomizeStructureProgress()));

  if (state == ProtectedCluster::Killed || state == ProtectedCluster::Removed) {
    a_kill->setVisible(false);
    a_restart->setVisible(false);
  } else {
    a_unkill->setVisible(false);
  }

  m_context_pc->lock()->unlock();
  QAction* selection = menu.exec(QCursor::pos());

  if (selection == 0) {
    m_context_pc = 0;
    return;
  }
  QtConcurrent::run(this, &TabProgress::updateProgressTable);
  a_restart->disconnect();
  a_kill->disconnect();
  a_unkill->disconnect();
  a_resetFail->disconnect();
  a_randomize->disconnect();
}

void TabProgress::restartJobProgress()
{
  if (!m_context_pc)
    return;

  // Get info from cluster
  m_context_pc->lock()->lockForRead();
  int gen = m_context_pc->getGeneration();
  int id = m_context_pc->getIDNumber();
  int optstep = m_context_pc->getCurrentOptStep();
  m_context_pc->lock()->unlock();

  // Choose which OptStep to use
  bool ok;
  int optStep = QInputDialog::getInt(
    m_dialog, tr("Restart Optimization %1x%2").arg(gen).arg(id),
    "Select optimization step to restart from:", optstep, 1,
    m_opt->optimizer()->getNumberOfOptSteps(), 1, &ok);
  if (!ok)
    return;
  QtConcurrent::run(this, &TabProgress::restartJobProgress_, optStep);
}

void TabProgress::restartJobProgress_(int optStep)
{
  QWriteLocker locker(m_context_pc->lock());
  m_context_pc->setCurrentOptStep(optStep);

  m_context_pc->setStatus(ProtectedCluster::Restart);
  newInfoUpdate(m_context_pc);

  // Clear context pointer
  locker.unlock();
  m_context_pc = 0;
}

void TabProgress::killPCProgress()
{
  QtConcurrent::run(this, &TabProgress::killPCProgress_);
}

void TabProgress::killPCProgress_()
{
  if (!m_context_pc)
    return;

  // QueueManager takes care of mutex locking
  m_opt->queue()->killStructure(m_context_pc);

  // Clear context pointer
  newInfoUpdate(m_context_pc);
  m_context_pc = 0;
}

void TabProgress::unkillPCProgress()
{
  QtConcurrent::run(this, &TabProgress::unkillPCProgress_);
}

void TabProgress::unkillPCProgress_()
{
  if (!m_context_pc)
    return;
  QWriteLocker locker(m_context_pc->lock());
  if (m_context_pc->getStatus() != ProtectedCluster::Killed &&
      m_context_pc->getStatus() != ProtectedCluster::Removed)
    return;

  // Setting status to Error will restart the job if was killed
  if (m_context_pc->getStatus() == ProtectedCluster::Killed)
    m_context_pc->setStatus(ProtectedCluster::Error);

  // Set status to Optimized if pc was previously optimized
  if (m_context_pc->getStatus() == ProtectedCluster::Removed)
    m_context_pc->setStatus(ProtectedCluster::Optimized);

  // Clear context pc pointer
  newInfoUpdate(m_context_pc);
  locker.unlock();
  m_context_pc = 0;
}

void TabProgress::resetFailureCountProgress()
{
  QtConcurrent::run(this, &TabProgress::resetFailureCountProgress_);
}

void TabProgress::resetFailureCountProgress_()
{
  if (!m_context_pc)
    return;
  QWriteLocker locker(m_context_pc->lock());

  m_context_pc->resetFailCount();

  // Clear context pointer
  newInfoUpdate(m_context_pc);
  locker.unlock();
  m_context_pc = 0;
}

void TabProgress::randomizeStructureProgress()
{
  QtConcurrent::run(this, &TabProgress::randomizeStructureProgress_);
}

void TabProgress::randomizeStructureProgress_()
{
  if (!m_context_pc)
    return;

  // End job if currently running
  if (m_context_pc->getJobID()) {
    m_opt->queueInterface()->stopJob(m_context_pc);
  }

  m_opt->replaceWithRandom(m_context_pc, "manual");

  // Restart job:
  newInfoUpdate(m_context_pc);
  restartJobProgress_(1);
}
}

/**********************************************************************
  RandomDock -- A tool for analysing a matrix-substrate docking problem

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <randomdock/ui/tab_progress.h>

#include <randomdock/randomdock.h>
#include <randomdock/structures/scene.h>
#include <randomdock/ui/dialog.h>

#include <globalsearch/macros.h>
#include <globalsearch/optimizer.h>
#include <globalsearch/queuemanager.h>

#include <QMutexLocker>
#include <QSettings>
#include <QTimer>
#include <QtConcurrentRun>

#include <QInputDialog>
#include <QMenu>

using namespace std;
using namespace Avogadro;
using namespace GlobalSearch;

namespace RandomDock {

TabProgress::TabProgress(RandomDockDialog* parent, RandomDock* p)
  : AbstractTab(parent, p), m_timer(new QTimer(this)), m_mutex(new QMutex),
    m_update_mutex(new QMutex), m_update_all_mutex(new QMutex),
    m_context_scene(0)
{
  // Allow queued connections to work with the TableEntry struct
  qRegisterMetaType<RD_Prog_TableEntry>("RD_Prog_TableEntry");

  ui.setupUi(m_tab_widget);

  QHeaderView* horizontal = ui.table_list->horizontalHeader();
  horizontal->setResizeMode(QHeaderView::ResizeToContents);

  rowTracking = true;

  // dialog connections
  connect(m_dialog, SIGNAL(moleculeChanged(GlobalSearch::Structure*)), this,
          SLOT(highlightScene(GlobalSearch::Structure*)));
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
          this, SLOT(addNewEntry()));
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
  connect(this, SIGNAL(updateTableEntry(int, const RD_Prog_TableEntry&)), this,
          SLOT(setTableEntry(int, const RD_Prog_TableEntry&)));

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
  settings->beginGroup("randomdock/progress");
  const int version = 1;
  settings->setValue("version", version);

  settings->setValue("refreshTime", ui.spin_period->value());
  settings->endGroup();
  DESTROY_SETTINGS(filename);
}

void TabProgress::readSettings(const QString& filename)
{
  SETTINGS(filename);
  settings->beginGroup("randomdock/progress");
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

  for (QList<Structure *>::iterator it = running.begin(),
                                    it_end = running.end();
       it != it_end; ++it) {
    newInfoUpdate(*it);
  }

  m_update_mutex->unlock();
}

void TabProgress::addNewEntry()
{
  QMutexLocker locker(m_mutex);

  // The new entry will be at the end of the table, so determine the index:
  int index = ui.table_list->rowCount();
  m_opt->tracker()->lockForRead();
  Scene* scene = qobject_cast<Scene*>(m_opt->tracker()->at(index));
  m_opt->tracker()->unlock();

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
  for (int i = 0; i < 6; i++) {
    ui.table_list->setItem(index, i, new QTableWidgetItem());
  }

  m_infoUpdateTracker.lockForWrite();
  m_infoUpdateTracker.append(scene);
  m_infoUpdateTracker.unlock();
  locker.unlock();
  RD_Prog_TableEntry e;
  scene->lock()->lockForRead();
  e.rank = scene->getRank();
  e.elapsed = scene->getOptElapsed();
  e.id = scene->getIDNumber();
  e.jobID = scene->getJobID();
  e.status = "Waiting for data...";
  e.brush = QBrush(Qt::white);

  if (scene->hasEnthalpy() || scene->getEnergy() != 0)
    e.energy = scene->getEnthalpy();
  else
    e.energy = 0.0;
  scene->lock()->unlock();

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
  if (m_context_scene != 0) {
    qDebug()
      << "TabProgress::updateInfo: Waiting for context operation to complete ("
      << m_context_scene << ") Trying again very soon.";
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

  int i = m_opt->tracker()->list()->indexOf(structure);

  Scene* scene = qobject_cast<Scene*>(structure);

  if (i < 0 || i > ui.table_list->rowCount() - 1) {
    qDebug() << "TabProgress::updateInfo: Trying to update an index that "
                "doesn't exist...yet: ("
             << i << ") Waiting...";
    m_infoUpdateTracker.lockForWrite();
    m_infoUpdateTracker.append(scene);
    m_infoUpdateTracker.unlock();
    QTimer::singleShot(100, this, SLOT(updateInfo()));
    return;
  }

  RD_Prog_TableEntry e;
  uint totalOptSteps = m_opt->optimizer()->getNumberOfOptSteps();

  e.brush = QBrush(Qt::white);

  QReadLocker sceneLocker(scene->lock());

  e.rank = scene->getRank();
  e.id = scene->getIndex();
  e.elapsed = scene->getOptElapsed();
  e.jobID = scene->getJobID();
  e.energy = scene->getEnergy();

  switch (scene->getStatus()) {
    case Scene::InProcess: {
      sceneLocker.unlock();
      QueueInterface::QueueStatus state =
        m_opt->queueInterface()->getStatus(scene);
      sceneLocker.relock();
      switch (state) {
        case QueueInterface::Running:
          e.status = tr("Running (Opt Step %1 of %2, %3 failures)")
                       .arg(QString::number(scene->getCurrentOptStep()))
                       .arg(QString::number(totalOptSteps))
                       .arg(QString::number(scene->getFailCount()));
          e.brush.setColor(Qt::green);
          break;
        case QueueInterface::Queued:
          e.status = tr("Queued (Opt Step %1 of %2, %3 failures)")
                       .arg(QString::number(scene->getCurrentOptStep()))
                       .arg(QString::number(totalOptSteps))
                       .arg(QString::number(scene->getFailCount()));
          e.brush.setColor(Qt::cyan);
          break;
        case QueueInterface::Success:
          e.status = "Starting update...";
          break;
        case QueueInterface::Unknown:
          e.status = "Unknown";
          break;
        case QueueInterface::Error:
          e.status = "Error: Restarting job...";
          e.brush.setColor(Qt::darkRed);
          break;
        case QueueInterface::CommunicationError:
          e.status = "Communication Error";
          e.brush.setColor(Qt::darkRed);
          break;
        // Shouldn't happen; started and pending only occur when scene is
        // "Submitted"
        case QueueInterface::Started:
        case QueueInterface::Pending:
        default:
          break;
      }
      break;
    }
    case Scene::Submitted:
      e.status = tr("Job submitted (%1 of %2)")
                   .arg(QString::number(scene->getCurrentOptStep()))
                   .arg(QString::number(totalOptSteps));
      e.brush.setColor(Qt::cyan);
      break;
    case Scene::Restart:
      e.status = "Restarting job...";
      e.brush.setColor(Qt::cyan);
      break;
    case Scene::Killed:
    case Scene::Removed:
      e.status = "Killed";
      e.brush.setColor(Qt::darkGray);
      break;
    case Scene::Duplicate:
      e.status = tr("Duplicate of %1").arg(scene->getDuplicateString());
      e.brush.setColor(Qt::darkGreen);
      break;
    case Scene::StepOptimized:
      e.status = "Checking status...";
      e.brush.setColor(Qt::cyan);
      break;
    case Scene::Optimized:
      e.status = "Optimized";
      e.brush.setColor(Qt::yellow);
      break;
    case Scene::WaitingForOptimization:
      e.status = tr("Waiting for Optimization (%1 of %2)")
                   .arg(QString::number(scene->getCurrentOptStep()))
                   .arg(QString::number(totalOptSteps));
      e.brush.setColor(Qt::darkCyan);
      break;
    case Scene::Error:
      e.status = tr("Job failed. Restarting...");
      e.brush.setColor(Qt::red);
      break;
    case Scene::Updating:
      e.status = "Updating structure...";
      e.brush.setColor(Qt::cyan);
      break;
    case Scene::Empty:
      e.status = "Structure empty...";
      break;
  }

  if (scene->getFailCount() != 0) {
    e.brush.setColor(Qt::red);
  }

  emit updateTableEntry(i, e);
}

void TabProgress::setTableEntry(int row, const RD_Prog_TableEntry& e)
{
  QMutexLocker locker(m_mutex);

  ui.table_list->item(row, C_Rank)->setText(QString::number(e.rank));
  ui.table_list->item(row, C_Index)->setText(QString::number(e.id));
  ui.table_list->item(row, C_Elapsed)->setText(e.elapsed);
  ui.table_list->item(row, C_Status)->setText(e.status);
  ui.table_list->item(row, C_Status)->setBackground(e.brush);

  if (e.jobID)
    ui.table_list->item(row, C_JobID)->setText(QString::number(e.jobID));
  else
    ui.table_list->item(row, C_JobID)->setText("N/A");

  if (e.energy != 0.0)
    ui.table_list->item(row, C_Energy)->setText(QString::number(e.energy));
  else
    ui.table_list->item(row, C_Energy)->setText("N/A");
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
  emit moleculeChanged(m_opt->tracker()->at(row));
}

void TabProgress::highlightScene(Structure* structure)
{
  structure->lock()->lockForRead();
  int id = structure->getIndex();
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
  if (m_context_scene)
    return;
  QApplication::setOverrideCursor(Qt::WaitCursor);
  QTableWidgetItem* item = ui.table_list->itemAt(p);
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
  QAction* a_restart = menu.addAction("&Restart job");
  QAction* a_kill = menu.addAction("&Kill structure");
  QAction* a_unkill = menu.addAction("Un&kill structure");
  QAction* a_resetFail = menu.addAction("Reset &failure count");
  menu.addSeparator();
  QAction* a_randomize = menu.addAction("Replace with &new random structure");

  // Connect actions
  connect(a_restart, SIGNAL(triggered()), this, SLOT(restartJobProgress()));
  connect(a_kill, SIGNAL(triggered()), this, SLOT(killSceneProgress()));
  connect(a_unkill, SIGNAL(triggered()), this, SLOT(unkillSceneProgress()));
  connect(a_resetFail, SIGNAL(triggered()), this,
          SLOT(resetFailureCountProgress()));
  connect(a_randomize, SIGNAL(triggered()), this,
          SLOT(randomizeStructureProgress()));

  if (state == Scene::Killed || state == Scene::Removed) {
    a_kill->setVisible(false);
    a_restart->setVisible(false);
  } else {
    a_unkill->setVisible(false);
  }

  m_context_scene->lock()->unlock();
  QAction* selection = menu.exec(QCursor::pos());

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

void TabProgress::restartJobProgress()
{
  if (!m_context_scene)
    return;

  // Get info from scene
  m_context_scene->lock()->lockForRead();
  int id = m_context_scene->getIDNumber();
  int optstep = m_context_scene->getCurrentOptStep();
  m_context_scene->lock()->unlock();

  // Choose which OptStep to use
  bool ok;
  int optStep =
    QInputDialog::getInt(m_dialog, tr("Restart Optimization %1").arg(id),
                         "Select optimization step to restart from:", optstep,
                         1, m_opt->optimizer()->getNumberOfOptSteps(), 1, &ok);
  if (!ok)
    return;
  QtConcurrent::run(this, &TabProgress::restartJobProgress_, optStep);
}

void TabProgress::restartJobProgress_(int optStep)
{
  QWriteLocker locker(m_context_scene->lock());
  m_context_scene->setCurrentOptStep(optStep);

  m_context_scene->setStatus(Scene::Restart);
  newInfoUpdate(m_context_scene);

  // Clear context scene pointer
  locker.unlock();
  m_context_scene = 0;
}

void TabProgress::killSceneProgress()
{
  QtConcurrent::run(this, &TabProgress::killSceneProgress_);
}

void TabProgress::killSceneProgress_()
{
  if (!m_context_scene)
    return;

  // QueueManager will take care of mutex locking
  m_opt->queueInterface()->stopJob(m_context_scene);

  // Clear context scene pointer
  newInfoUpdate(m_context_scene);
  m_context_scene = 0;
}

void TabProgress::unkillSceneProgress()
{
  QtConcurrent::run(this, &TabProgress::unkillSceneProgress_);
}

void TabProgress::unkillSceneProgress_()
{
  if (!m_context_scene)
    return;
  QWriteLocker locker(m_context_scene->lock());
  if (m_context_scene->getStatus() != Scene::Killed &&
      m_context_scene->getStatus() != Scene::Removed)
    return;

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

void TabProgress::resetFailureCountProgress()
{
  QtConcurrent::run(this, &TabProgress::resetFailureCountProgress_);
}

void TabProgress::resetFailureCountProgress_()
{
  if (!m_context_scene)
    return;
  QWriteLocker locker(m_context_scene->lock());

  m_context_scene->resetFailCount();

  // Clear context scene pointer
  newInfoUpdate(m_context_scene);
  locker.unlock();
  m_context_scene = 0;
}

void TabProgress::randomizeStructureProgress()
{
  QtConcurrent::run(this, &TabProgress::randomizeStructureProgress_);
}

void TabProgress::randomizeStructureProgress_()
{
  if (!m_context_scene)
    return;

  // End job if currently running
  if (m_context_scene->getJobID()) {
    m_opt->queueInterface()->stopJob(m_context_scene);
  }

  m_opt->replaceWithRandom(m_context_scene, "manual");

  // Restart job:
  newInfoUpdate(m_context_scene);
  restartJobProgress_(1);
}
}

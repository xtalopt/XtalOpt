/**********************************************************************
  ExampleSearch

  Copyright (C) 2012 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <examplesearch/examplesearch.h>

#include <examplesearch/ui/dialog.h>

#include <globalsearch/optbase.h>
#include <globalsearch/queueinterfaces/remote.h>
#include <globalsearch/queuemanager.h>
#include <globalsearch/slottedwaitcondition.h>
#include <globalsearch/tracker.h>
#ifdef ENABLE_SSH
#include <globalsearch/sshmanager.h>
#endif // ENABLE_SSH
#include <globalsearch/macros.h>

#include <avogadro/atom.h>
#include <avogadro/bond.h>

#include <openbabel/mol.h>

#include <QDir>
#include <QFile>
#include <QStringList>
#include <QThread>

using namespace std;
using namespace Avogadro;
using namespace GlobalSearch;

namespace ExampleSearch {

ExampleSearch::ExampleSearch(ExampleSearchDialog* parent)
  : OptBase(parent), m_initWC(new SlottedWaitCondition(this)),
    m_structureInitMutex(new QMutex)
{
  m_idString = "ExampleSearch";
  m_schemaVersion = 1;
  m_structureInitMutex = new QMutex;
  limitRunningJobs = true;
  // By default, just replace with random when an optimization fails.
  failLimit = 1;
  failAction = FA_Randomize;
  contStructs = 10;
  runningJobLimit = 1;
}

ExampleSearch::~ExampleSearch()
{
  // Stop queuemanager thread
  if (m_queueThread->isRunning()) {
    m_queueThread->disconnect();
    m_queueThread->quit();
    m_queueThread->wait();
  }

  // Delete queuemanager
  delete m_queue;
  m_queue = 0;

#ifdef ENABLE_SSH
  // Stop SSHManager
  delete m_ssh;
  m_ssh = 0;
#endif // ENABLE_SSH

  // Wait for save to finish
  while (savePending) {
    qDebug() << "Spinning on save before destroying ExampleSearch...";
    GS_SLEEP(1);
  };
  savePending = true;

  // Clean up various members
  m_initWC->deleteLater();
  m_initWC = 0;
}

void ExampleSearch::startSearch()
{
  // Add search-specific checks here
  // If something is amiss, call error("Error message") and return.

  // Are the selected queueinterface and optimizer happy?
  QString err;
  if (!m_optimizer->isReadyToSearch(&err)) {
    error(tr("Optimizer is not fully initialized:") + "\n\n" + err);
    return;
  }

  if (!m_queueInterface->isReadyToSearch(&err)) {
    error(tr("QueueInterface is not fully initialized:") + "\n\n" + err);
    return;
  }

  // Warn user if runningJobLimit is 0
  if (limitRunningJobs && runningJobLimit == 0) {
    error(tr("Warning: the number of running jobs is currently set to 0."
             "\n\nYou will need to increase this value before the search "
             "can begin (The option is on the 'Optimization Settings' tab)."));
  };

// Initialize ssh connections
#ifdef ENABLE_SSH
  // Create the SSHManager if running remotely
  if (qobject_cast<RemoteQueueInterface*>(m_queueInterface) != 0) {
    if (!this->createSSHConnections()) {
      error(tr("Could not create ssh connections."));
      return;
    }
  }
#endif // ENABLE_SSH

  // Here we go!
  debug("Starting optimization.");

  // prepare pointers
  m_tracker->lockForWrite();
  m_tracker->deleteAllStructures();
  m_tracker->unlock();

  // Throw signal
  emit startingSession();

  ////////////////////////////////
  // Generate random structures //
  ////////////////////////////////

  // Set up progress bar
  m_dialog->startProgressUpdate(tr("Generating structures..."), 0, 0);

  // Initalize loop variables
  int progCount = 0;

  // Generation loop...
  for (uint i = 0; i < runningJobLimit; i++) {
    m_dialog->updateProgressMaximum(
      (i == 0) ? 0 : int(progCount / static_cast<double>(i)) * runningJobLimit);
    m_dialog->updateProgressValue(progCount++);
    m_dialog->updateProgressLabel(
      tr("%1 structures generated of (%2)...").arg(i).arg(runningJobLimit));

    generateNewStructure();
  }

  // Wait for all structures to appear in tracker
  m_dialog->updateProgressLabel(tr("Waiting for structures to initialize..."));
  m_dialog->updateProgressMinimum(0);
  m_dialog->updateProgressMinimum(runningJobLimit);

  connect(m_tracker, SIGNAL(newStructureAdded(GlobalSearch::Structure*)),
          m_initWC, SLOT(wakeAllSlot()));

  m_initWC->prewaitLock();
  do {
    m_dialog->updateProgressValue(m_tracker->size());
    m_dialog->updateProgressLabel(
      tr("Waiting for structures to initialize (%1 of %2)...")
        .arg(m_tracker->size())
        .arg(runningJobLimit));
    // Don't block here forever -- there is a race condition where
    // the final newStructureAdded signal may be emitted while the
    // WC is not waiting. Since this is just trivial GUI updating
    // and we check the condition in the do-while loop, this is
    // acceptable. The following call will timeout in 250 ms.
    m_initWC->wait(250);
  } while (m_tracker->size() < runningJobLimit);
  m_initWC->postwaitUnlock();

  // We're done with m_initWC.
  m_initWC->disconnect();

  m_dialog->stopProgressUpdate();

  emit sessionStarted();
  m_dialog->saveSession();
}

Structure* ExampleSearch::replaceWithRandom(Structure* s, const QString& reason)
{
  QWriteLocker locker1(s->lock());

  // Generate/Check new structure
  Structure* structure = generateRandomStructure();

  // Copy info over
  QWriteLocker locker2(structure->lock());
  s->copyStructure(*structure);
  s->resetEnergy();
  s->resetEnthalpy();
  s->setPV(0);
  s->setCurrentOptStep(1);
  QString parents = "Randomly generated";
  if (!reason.isEmpty())
    parents += " (" + reason + ")";
  s->setParents(parents);
  s->resetFailCount();

  // Delete random structure
  structure->deleteLater();
  return s;
}

Structure* ExampleSearch::generateRandomStructure()
{
  Structure* s = new Structure;

  Atom* O1 = s->addAtom(8, Eigen::Vector3d(0.0, 0.0, 0.0));
  Atom* O2 = s->addAtom(8, Eigen::Vector3d(1.0, 0.0, 0.0));

  s->addBond(O1, O2, 2);

  return s;
}

bool ExampleSearch::checkLimits()
{
  // Validate input parameters here, see XtalOpt class for example.
}

void ExampleSearch::generateNewStructure()
{
  initializeAndAddStructure(generateRandomStructure());
}

void ExampleSearch::initializeAndAddStructure(Structure* structure)
{
  // Initialize vars
  QString id_s;
  QString locpath_s;
  QString rempath_s;

  // So as to not assign duplicate ids, ensure only one assignment
  // is made at a time
  m_structureInitMutex->lock();

  // lockForNaming returns a list of all structures, both accepted
  // and pending, so it's size+1 is the id of the new structure.
  int id = m_queue->lockForNaming().size() + 1;

  // Generate locations using id number
  id_s.sprintf("%05d", id);
  locpath_s = filePath + "/" + id_s + "/";
  rempath_s = rempath + "/" + id_s + "/";

  // Create path
  QDir dir(locpath_s);
  if (!dir.exists()) {
    if (!dir.mkpath(locpath_s)) {
      error(tr("ExampleSearch::initializeAndAddStructure: Cannot write to "
               "path: %1 (path creation failure)",
               "1 is a file path.")
              .arg(locpath_s));
    }
  }

  // Assign data to structure
  structure->lock()->lockForWrite();
  structure->moveToThread(m_queueThread);
  structure->setIDNumber(id);
  structure->setIndex(id - 1);
  structure->setFileName(locpath_s);
  structure->setRempath(rempath_s);
  structure->setCurrentOptStep(1);
  structure->setStatus(Structure::WaitingForOptimization);
  structure->lock()->unlock();

  // unlockForNaming will append the structure
  m_queue->unlockForNaming(structure);

  // Done!
  m_structureInitMutex->unlock();
}

} // end namespace ExampleSearch

/**********************************************************************
  SearchBase - Base class for global search extensions

  Copyright (C) 2010-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/search.h>

#include <common/constants.h>
#include <common/timing.h>
#include <atoms/eleminfo.h>
#include <atoms/formats/poscarformat.h>
#include <atoms/formats/siestaformat.h>
#include <search/optimizer.h>
#include <common/output.h>
#include <search/queueinterface.h>
#include <search/queuemanager.h>
#include <search/selection.h>

#include <cfloat>
#include <common/random.h>
#include <search/ssh/sshconnection.h>
#include <search/ssh/sshmanager.h>
#include <search/ssh/sshmanager_cli.h>
#ifdef ENABLE_LIBSSH
#include <search/ssh/sshmanager_libssh.h>
#endif // ENABLE_LIBSSH
#include <search/structure.h>
#include <common/numericutils.h>

#include <common/compatibility/qt_compat.h>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QMetaObject>
#include <QThread>
#include <QThreadPool>
#include <QWriteLocker>

#include <QtConcurrent>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <mutex>

// Uncomment for yet more debug info about probabilities
//#define SEARCHBASE_PROBS_DEBUG

namespace {

QString sshMethodFromText(const QString& method)
{
  const QString value = method.trimmed().toLower();
  if (value == "system" || value == "cli" || value == "commandline" ||
      value == "command-line" || value == "ssh" || value == "scp")
    return "system";
  if (value == "libssh" || value == "lib")
    return "libssh";
  if (value == "auto" || value == "automatic")
    return "auto";
  return QString();
}

QString shellSingleQuote(const QString& text)
{
  QString quoted = text;
  quoted.replace("'", "'\\''");
  return "'" + quoted + "'";
}

bool isRemoteAbsolutePath(const QString& path)
{
  return path.startsWith("/");
}

}

namespace Search {

SearchBase::SearchBase(QObject* parent)
  : QObject(parent),
    m_idString("Generic"),
    m_ssh(nullptr),
    m_tracker(std::unique_ptr<Tracker>(new Tracker())),
    m_queueThread(std::unique_ptr<QThread>(new QThread())),
    m_queue(std::unique_ptr<QueueManager>(new QueueManager(m_queueThread.get(), this))),
    m_runtimeSettingsLock(QReadWriteLock::Recursive),
    m_optSteps([this](const std::string& n) { return createQueueInterface(n); },
               [this](const std::string& n) { return createOptimizer(n); }),
    // Engine defaults: these are not really in effect - at runtime the
    //   application overwrites them with its own read of the settings.
    //   For now they just mirror the app's values.
    m_verbose(false),
    m_debugOutput(false),
    m_limitRunningJobs(true),
    m_runningJobLimit(1),
    m_contStructs(15),
    m_maxNumStructures(100),
    m_failLimit(1),
    m_failAction(FA_KillIt),
    m_objectivePrecision(-1),
    m_softExit(false),
    m_hardExit(false),
    m_remoteQueue(false),
    m_constraintsReDo(false),
    m_optimizationType(OT_Basic),
    m_tournamentSelection(true),
    m_restrictedPool(false),
    m_crowdingDistance(true),
    m_paretoFilterZeroWeights(false),
    m_queueRefreshInterval(10),
    m_cleanRemoteOnStop(false),
    m_port(22),
    m_isStarting(false),
    m_sessionActive(false),
    m_readOnly(false),
    m_shuttingDown(false),
    m_logErrorDirs(false),
    m_evaluationUpdateJob([this]() { emit structureEvaluationUpdateRequested(); })
{
  m_sshMethod = defaultSshMethod();
  clearPromptHandlers();

  // Connections
  connect(tracker(), &Tracker::newStructureAdded,
          this, &SearchBase::reportStructureStateChanged);
  connect(queue(), &QueueManager::structureUpdated,
          this, &SearchBase::reportStructureStateChanged);
  connect(queue(), &QueueManager::structureUpdated,
          this, &SearchBase::refreshParentPoolMembership, Qt::DirectConnection);

  // Register base template keywords
  registerKeyword("user1", [this](Structure*) -> QString { return getUser1(); },
                  "%user1% -- User specified value 1");
  registerKeyword("user2", [this](Structure*) -> QString { return getUser2(); },
                  "%user2% -- User specified value 2");
  registerKeyword("user3", [this](Structure*) -> QString { return getUser3(); },
                  "%user3% -- User specified value 3");
  registerKeyword("user4", [this](Structure*) -> QString { return getUser4(); },
                  "%user4% -- User specified value 4");
  registerKeyword("description", [this](Structure*) -> QString { return getDescription(); },
                  "%description% -- Optimization description");
  registerKeyword("percent", [](Structure*) -> QString { return "%"; },
                  "%percent% -- Literal percent sign (needed for CASTEP!)");
  registerKeyword("coords", [](Structure* s) -> QString {
    QString rep;
    const std::vector<Atoms::Atom>& atoms = s->atoms();
    for (auto it = atoms.begin(); it != atoms.end(); ++it) {
      rep += QString(Atoms::ElementInfo::getAtomicSymbol((*it).atomicNumber()).c_str()) + " ";
      const Common::Vector3& vec = (*it).pos();
      rep += QString::number(vec.x()) + " ";
      rep += QString::number(vec.y()) + " ";
      rep += QString::number(vec.z()) + "\n";
    }
    return rep;
  }, "%coords% -- Cartesian coordinates: [symbol] [x] [y] [z]");
  registerKeyword("coordsInternalFlags", [](Structure* s) -> QString {
    QString rep;
    const std::vector<Atoms::Atom>& atoms = s->atoms();
    for (auto it = atoms.begin(); it != atoms.end(); ++it) {
      rep += QString(Atoms::ElementInfo::getAtomicSymbol((*it).atomicNumber()).c_str()) + " ";
      const Common::Vector3& vec = (*it).pos();
      rep += QString::number(vec.x()) + " 1 ";
      rep += QString::number(vec.y()) + " 1 ";
      rep += QString::number(vec.z()) + " 1\n";
    }
    return rep;
  }, "%coordsInternalFlags% -- Cartesian coordinates, flag after each coordinate: [symbol] [x] 1 [y] 1 [z] 1");
  registerKeyword("coordsSuffixFlags", [](Structure* s) -> QString {
    QString rep;
    const std::vector<Atoms::Atom>& atoms = s->atoms();
    for (auto it = atoms.begin(); it != atoms.end(); ++it) {
      rep += QString(Atoms::ElementInfo::getAtomicSymbol((*it).atomicNumber()).c_str()) + " ";
      const Common::Vector3& vec = (*it).pos();
      rep += QString::number(vec.x()) + " ";
      rep += QString::number(vec.y()) + " ";
      rep += QString::number(vec.z()) + " 1 1 1\n";
    }
    return rep;
  }, "%coordsSuffixFlags% -- Cartesian coordinates, flags after all coordinates: [symbol] [x] [y] [z] 1 1 1");
  registerKeyword("coordsId", [](Structure* s) -> QString {
    QString rep;
    const std::vector<Atoms::Atom>& atoms = s->atoms();
    for (auto it = atoms.begin(); it != atoms.end(); ++it) {
      rep += QString(Atoms::ElementInfo::getAtomicSymbol((*it).atomicNumber()).c_str()) + " ";
      rep += QString::number((*it).atomicNumber()) + " ";
      const Common::Vector3& vec = (*it).pos();
      rep += QString::number(vec.x()) + " ";
      rep += QString::number(vec.y()) + " ";
      rep += QString::number(vec.z()) + "\n";
    }
    return rep;
  }, "%coordsId% -- Cartesian coordinates with atomic number: [symbol] [atomic number] [x] [y] [z]");
  registerKeyword("numAtoms",   [](Structure* s) -> QString { return QString::number(s->numAtoms()); },
                  "%numAtoms% -- Number of atoms in unit cell");
  registerKeyword("numSpecies", [](Structure* s) -> QString { return QString::number(s->getSymbols().size()); },
                  "%numSpecies% -- Number of unique atomic species in unit cell");
  registerKeyword("filename",   [](Structure* s) -> QString { return s->getLocpath(); },
                  "%filename% -- Local output filename");
  registerKeyword("rempath",    [](Structure* s) -> QString { return s->getRempath(); },
                  "%rempath% -- Path to structure's remote directory");
  registerKeyword("gen",        [](Structure* s) -> QString { return QString::number(s->getGeneration()); },
                  "%gen% -- Structure generation number");
  registerKeyword("id",         [](Structure* s) -> QString { return QString::number(s->getIDNumber()); },
                  "%id% -- Structure id number");
  registerKeyword("optStep",    [](Structure* s) -> QString { return QString::number(s->getCurrentOptStep()); },
                  "%optStep% -- Current optimization step");
  registerKeyword("incar",      [](Structure* s) -> QString { return QString::number(s->getCurrentOptStep()); },
                  "%incar% -- Legacy alias for %optStep%");
  // filecontents: and copyfile: are handled as prefix keywords in interpretKeyword_base()
  registerKeyword("filecontents:", [](Structure*) -> QString { return ""; },
                  "%fileContents:/path/to/local/file% -- Replaced with the contents of the specified file");
  registerKeyword("copyfile:", [](Structure*) -> QString { return ""; },
                  "%copyFile:/path/to/local/file% -- Copy the specified file to the structure's working directory");

  // Register generic crystal-structure template keywords
  registerKeyword("a", [](Structure* s) -> QString {
    return QString::number(s->getA());
  }, "%a% -- Lattice parameter A");
  registerKeyword("b", [](Structure* s) -> QString {
    return QString::number(s->getB());
  }, "%b% -- Lattice parameter B");
  registerKeyword("c", [](Structure* s) -> QString {
    return QString::number(s->getC());
  }, "%c% -- Lattice parameter C");
  registerKeyword("alphaRad", [](Structure* s) -> QString {
    return QString::number(s->getAlpha() * DEG2RAD);
  }, "%alphaRad% -- Lattice parameter Alpha in rad");
  registerKeyword("betaRad", [](Structure* s) -> QString {
    return QString::number(s->getBeta() * DEG2RAD);
  }, "%betaRad% -- Lattice parameter Beta in rad");
  registerKeyword("gammaRad", [](Structure* s) -> QString {
    return QString::number(s->getGamma() * DEG2RAD);
  }, "%gammaRad% -- Lattice parameter Gamma in rad");
  registerKeyword("alphaDeg", [](Structure* s) -> QString {
    return QString::number(s->getAlpha());
  }, "%alphaDeg% -- Lattice parameter Alpha in degrees");
  registerKeyword("betaDeg", [](Structure* s) -> QString {
    return QString::number(s->getBeta());
  }, "%betaDeg% -- Lattice parameter Beta in degrees");
  registerKeyword("gammaDeg", [](Structure* s) -> QString {
    return QString::number(s->getGamma());
  }, "%gammaDeg% -- Lattice parameter Gamma in degrees");
  registerKeyword("volume", [](Structure* s) -> QString {
    return QString::number(s->getVolume());
  }, "%volume% -- Unit cell volume");
  registerKeyword("block",    [](Structure*) -> QString { return "%block"; },    "");
  registerKeyword("endblock", [](Structure*) -> QString { return "%endblock"; }, "");
  registerKeyword("coordsFrac", [](Structure* s) -> QString {
    QString rep;
    const std::vector<Atoms::Atom>& atoms = s->atoms();
    for (auto it = atoms.begin(); it != atoms.end(); ++it) {
      const Common::Vector3 coords = s->cartToFrac((*it).pos());
      rep += QString(Atoms::ElementInfo::getAtomicSymbol((*it).atomicNumber()).c_str()) + " ";
      rep += QString::number(coords.x()) + " ";
      rep += QString::number(coords.y()) + " ";
      rep += QString::number(coords.z()) + "\n";
    }
    return rep;
  }, "%coordsFrac% -- Fractional coordinate: [symbol] [x] [y] [z]");
  registerKeyword("chemicalSpeciesLabel", [](Structure* s) -> QString {
    QString rep;
    QList<QString> symbols = s->getSymbols();
    for (int i = 0; i < symbols.size(); i++) {
      rep += " " + QString::number(i + 1) + " ";
      rep += QString::number(Atoms::ElementInfo::getAtomicNum(symbols[i].toStdString())) + " ";
      rep += symbols[i] + "\n";
    }
    return rep;
  }, "%chemicalSpeciesLabel% -- Chemical species labels for SIESTA");
  registerKeyword("atomicCoordsAndAtomicSpecies", [](Structure* s) -> QString {
    const std::vector<Atoms::Atom>& atoms = s->atoms();
    QList<QString> symbol = s->getSymbols();
    QString rep;
    for (auto it = atoms.begin(); it != atoms.end(); ++it) {
      const Common::Vector3 coords = s->cartToFrac((*it).pos());
      QString currAtom = Atoms::ElementInfo::getAtomicSymbol((*it).atomicNumber()).c_str();
      int i = symbol.indexOf(currAtom) + 1;
      rep += " ";
      rep += QString("%1").arg(coords.x(), 14, 'f', 8) + "  ";
      rep += QString("%1").arg(coords.y(), 14, 'f', 8) + "  ";
      rep += QString("%1").arg(coords.z(), 14, 'f', 8) + "  ";
      rep += QString::number(i) + "\n";
    }
    return rep;
  }, "%atomicCoordsAndAtomicSpecies% -- Atomic coordinates with species index for SIESTA");
  registerKeyword("coordsFracId", [](Structure* s) -> QString {
    QString rep;
    const std::vector<Atoms::Atom>& atoms = s->atoms();
    for (auto it = atoms.begin(); it != atoms.end(); ++it) {
      const Common::Vector3 coords = s->cartToFrac((*it).pos());
      rep += QString(Atoms::ElementInfo::getAtomicSymbol((*it).atomicNumber()).c_str()) + " ";
      rep += QString::number((*it).atomicNumber()) + " ";
      rep += QString::number(coords.x()) + " ";
      rep += QString::number(coords.y()) + " ";
      rep += QString::number(coords.z()) + "\n";
    }
    return rep;
  }, "%coordsFracId% -- Fractional coordinate with atomic number: [symbol] [atomic number] [x] [y] [z]");
  registerKeyword("coordsFracIndex", [](Structure* s) -> QString {
    QString rep;
    const std::vector<Atoms::Atom>& atoms = s->atoms();
    int tag = 0;
    for (auto it = atoms.begin(); it != atoms.end(); ++it) {
      const Common::Vector3 coords = s->cartToFrac((*it).pos());
      rep += QString::number(tag++) + " ";
      rep += QString::number(coords.x()) + " ";
      rep += QString::number(coords.y()) + " ";
      rep += QString::number(coords.z()) + "\n";
    }
    return rep;
  }, "%coordsFracIndex% -- Fractional coordinate with order index: [index: 0..number of atoms] [x] [y] [z]");
  registerKeyword("gulpFracShell", [](Structure* s) -> QString {
    const std::vector<Atoms::Atom>& atoms = s->atoms();
    QString rep;
    for (auto it = atoms.begin(); it != atoms.end(); ++it) {
      const Common::Vector3 coords = s->cartToFrac((*it).pos());
      const std::string symbol = Atoms::ElementInfo::getAtomicSymbol((*it).atomicNumber());
      rep += QString("%1 core %2 %3 %4\n").arg(symbol.c_str()).arg(coords.x()).arg(coords.y()).arg(coords.z());
      rep += QString("%1 shel %2 %3 %4\n").arg(symbol.c_str()).arg(coords.x()).arg(coords.y()).arg(coords.z());
    }
    return rep;
  }, "%gulpFracShell% -- Fractional coordinates for GULP core/shell calculations");
  registerKeyword("cellMatrixAngstrom", [](Structure* s) -> QString {
    Common::Matrix3 m = s->unitCell().cellMatrix();
    QString rep;
    for (int i = 0; i < 3; i++) {
      rep += " ";
      for (int j = 0; j < 3; j++)
        rep += QString("%1").arg(m(i, j), 14, 'f', 8) + "  ";
      rep += "\n";
    }
    return rep;
  }, "%cellMatrixAngstrom% -- Cell matrix in Angstrom");
  registerKeyword("cellVector1Angstrom", [](Structure* s) -> QString {
    Common::Vector3 v = s->unitCell().aVector();
    QString rep;
    for (int i = 0; i < 3; i++) rep += QString::number(v[i]) + "   ";
    return rep;
  }, "%cellVector1Angstrom% -- First cell vector in Angstrom");
  registerKeyword("cellVector2Angstrom", [](Structure* s) -> QString {
    Common::Vector3 v = s->unitCell().bVector();
    QString rep;
    for (int i = 0; i < 3; i++) rep += QString::number(v[i]) + "   ";
    return rep;
  }, "%cellVector2Angstrom% -- Second cell vector in Angstrom");
  registerKeyword("cellVector3Angstrom", [](Structure* s) -> QString {
    Common::Vector3 v = s->unitCell().cVector();
    QString rep;
    for (int i = 0; i < 3; i++) rep += QString::number(v[i]) + "   ";
    return rep;
  }, "%cellVector3Angstrom% -- Third cell vector in Angstrom");
  registerKeyword("cellMatrixBohr", [](Structure* s) -> QString {
    Common::Matrix3 m = s->unitCell().cellMatrix();
    QString rep;
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++)
        rep += QString::number(m(i, j) * ANG2BOHR) + "   ";
      rep += "\n";
    }
    return rep;
  }, "%cellMatrixBohr% -- Cell matrix in Bohr");
  registerKeyword("cellVector1Bohr", [](Structure* s) -> QString {
    Common::Vector3 v = s->unitCell().aVector();
    QString rep;
    for (int i = 0; i < 3; i++) rep += QString::number(v[i] * ANG2BOHR) + "   ";
    return rep;
  }, "%cellVector1Bohr% -- First cell vector in Bohr");
  registerKeyword("cellVector2Bohr", [](Structure* s) -> QString {
    Common::Vector3 v = s->unitCell().bVector();
    QString rep;
    for (int i = 0; i < 3; i++) rep += QString::number(v[i] * ANG2BOHR) + "   ";
    return rep;
  }, "%cellVector2Bohr% -- Second cell vector in Bohr");
  registerKeyword("cellVector3Bohr", [](Structure* s) -> QString {
    Common::Vector3 v = s->unitCell().cVector();
    QString rep;
    for (int i = 0; i < 3; i++) rep += QString::number(v[i] * ANG2BOHR) + "   ";
    return rep;
  }, "%cellVector3Bohr% -- Third cell vector in Bohr");
  registerKeyword("POSCAR", [](Structure* s) -> QString {
    QWriteLocker locker(&s->lock());

    const QString poscar = Atoms::PoscarFormat::writeToString(*s, s->getLocpath());

    if (s->hasBonds() && s->reusePreoptBonding()) {
      Atoms::PoscarFormat::reorderAtomsToMatchPoscar(*s);
      s->setPreoptBonding(s->bonds());
    }

    return poscar;
  }, "%POSCAR% -- VASP POSCAR generator");
  registerKeyword("siestaZMatrix", [](Structure* s) -> QString {
    QWriteLocker locker(&s->lock());

    const QString zMatrix = Atoms::SiestaFormat::writeSiestaZMatrixToString(
      *s, true, true, true, s->reusePreoptBonding());
    if (zMatrix.isEmpty()) {
      Common::error("Writing the SIESTA z-matrix failed.");
      return QString();
    }

    if (s->reusePreoptBonding()) {
      s->setPreoptBonding(s->bonds());
    }

    return zMatrix;
  }, "%siestaZMatrix% -- SIESTA Z-matrix coordinates");
}

SearchBase::BackgroundJob::BackgroundJob(std::function<void()> job)
  : m_job(std::move(job)), m_pending(false), m_running(false),
    m_shutDown(false)
{
}

void SearchBase::BackgroundJob::request()
{
  if (m_shutDown.load())
    return;

  bool launchWorker = false;
  {
    std::lock_guard<std::mutex> guard(m_mutex);
    m_pending = true;
    if (!m_running) {
      m_running = true;
      launchWorker = true;
    }
  }

  if (!launchWorker)
    return;

  (void)QtConcurrent::run([this]() { this->runLoop(); });
}

void SearchBase::BackgroundJob::shutdown()
{
  m_shutDown.store(true);
}

void SearchBase::BackgroundJob::runLoop()
{
  for (;;) {
    {
      std::lock_guard<std::mutex> guard(m_mutex);
      if (m_shutDown.load() || !m_pending) {
        m_pending = false;
        m_running = false;
        return;
      }
      m_pending = false;
    }

    m_job();
  }
}

void SearchBase::requestStructureEvaluationUpdate()
{
  if (m_shuttingDown.load() || isSessionStarting() || isReadOnly())
    return;

  m_evaluationUpdateJob.request();
}

void SearchBase::reportStructureStateChanged(Structure* structure)
{
  if (!structure || m_shuttingDown.load() || isReadOnly())
    return;

  emit structureStateChanged(structure);
}

SearchBase::~SearchBase()
{
  m_shuttingDown.store(true);
  m_evaluationUpdateJob.shutdown();

  // Make sure no new async work gets queued up while we shut down.
  disconnect(tracker(), nullptr, this, nullptr);
  disconnect(queue(), nullptr, this, nullptr);

  // Wait for the queue thread to finish before it is destroyed.
  stopQueueThread();

  // Wait for objective work before destroying the structures.
  queue()->waitForScriptCalculations(-1);

  // Wait for save work before destroying the search.
  QThreadPool::globalInstance()->waitForDone(-1);

  // All workers have stopped and their queued work is finished.
  queue()->reset();
  tracker()->deleteAllStructures();
  // m_queueThread and m_tracker are destroyed automatically.
}

void SearchBase::stopQueueThread()
{
  if (!m_queueThread)
    return;

  queue()->prepareForThreadStop();
  if (!m_queueThread->isRunning())
    return;

  for (size_t i = 0; i < getNumOptSteps(); ++i) {
    QueueInterface* queue = queueInterface(static_cast<int>(i));
    if (queue)
      queue->prepareForThreadStop();
  }

  m_queueThread->disconnect();
  m_queueThread->quit();
  if (QThread::currentThread() != m_queueThread.get())
    m_queueThread->wait();
}

void SearchBase::reset()
{
  queue()->reset();
  // Wait for structure work before deleting the structures.
  QThreadPool::globalInstance()->waitForDone(-1);
  tracker()->deleteAllStructures();

  // Clear selection table with all structures gone
  QWriteLocker tableLocker(&m_parentSelectionDataLock);
  m_parentPool.clear();
  m_parentSelectionData = ParentSelectionData();
  ++m_selectionDataStamp;
}

QThread* SearchBase::restoredStructureThread() const
{
  return m_tracker->thread();
}

void SearchBase::addRestoredStructure(Structure* structure, bool queueWaitingForOptimization)
{
  {
    QWriteLocker locker(m_tracker->rwLock());
    m_tracker->append(structure);
  }

  m_queue->trackRestoredStructure(structure, queueWaitingForOptimization);
}

QString SearchBase::defaultSshMethod()
{
#ifdef ENABLE_LIBSSH
  return "libssh";
#else
  return "system";
#endif
}

bool SearchBase::isValidSshMethod(const QString& method)
{
  return !sshMethodFromText(method).isEmpty();
}

bool SearchBase::isSshMethodAvailable(const QString& method)
{
  const QString value = sshMethodFromText(method);
  if (value == "system" || value == "auto")
    return true;
#ifdef ENABLE_LIBSSH
  if (value == "libssh")
    return true;
#endif
  return false;
}

bool SearchBase::setSshMethod(const QString& method)
{
  const QString value = sshMethodFromText(method);
  if (value.isEmpty())
    return false;
  m_sshMethod = value;
  return true;
}

bool SearchBase::beginSession()
{
  // Refuse a second session while one is active or being set up.
  if (isSessionActive() || m_isStarting.exchange(true))
    return false;

  // Clear the hard exit. Keep a soft exit read from the input file.
  setHardExit(false);

  emit startingSession();

  queue()->reset();
  tracker()->deleteAllStructures();
  return true;
}

void SearchBase::launchSession()
{
  Q_ASSERT_X(isSessionStarting(), "SearchBase::launchSession",
             "launchSession() called without a matching beginSession()");

  // Start the queue only after the structures and settings are ready.
  if (!isReadOnly() && !m_queueThread->isRunning())
    m_queueThread->start();

  // Start the session only after leaving the starting state.
  setSessionStarting(false);
  setSessionActive(true);
  emit sessionStarted();
}

void SearchBase::abortSession()
{
  setSessionActive(false);
  setSessionStarting(false);
  queue()->reset();
  tracker()->deleteAllStructures();
}

bool SearchBase::createSSHConnections()
{
  m_ssh.reset();

  beginProgressUpdate(tr("Preparing remote connection..."), 0, 0);

  const auto trySystem = [this](QString* error) {
    const QString label = tr("Trying System SSH method...");
    Common::message(label);
    updateProgressValue(-1, label);
    m_ssh.reset(SSHManagerCLI::createConnections(this, getHost(), getUsername(), getPort(), error));
    return m_ssh != nullptr;
  };

  const auto reportFailure = [this](const QString& error) {
    Common::error(error);
    emit errorDialogRequested(error);
  };

#ifdef ENABLE_LIBSSH
  const auto tryLibssh = [this](QString* error) {
    const QString label = tr("Trying libssh method...");
    Common::message(label);
    updateProgressValue(-1, label);
    m_ssh.reset(SSHManagerLibSSH::createConnections(
      this, getHost(), getUsername(), getPort(),
      [this](const QString& question) {
        return requestBooleanDecision(question);
      },
      [this](const QString& question, QString& password) {
        return m_passwordPromptHandler(question, password);
      },
      error));
    return m_ssh != nullptr;
  };
#endif

  if (m_sshMethod == "system") {
    QString error;
    const bool ok = trySystem(&error);
    endProgressUpdate();
    if (!ok)
      reportFailure(error);
    return ok;
  }

#ifdef ENABLE_LIBSSH
  if (m_sshMethod == "libssh") {
    QString error;
    const bool ok = tryLibssh(&error);
    endProgressUpdate();
    if (!ok)
      reportFailure(error);
    return ok;
  }

  if (m_sshMethod == "auto") {
    QString systemError;
    if (trySystem(&systemError)) {
      endProgressUpdate();
      Common::message(tr("Using System SSH method."));
      return true;
    }

    Common::warning(tr("System SSH precheck failed: %1").arg(systemError));
    emit errorDialogRequested(tr("System SSH precheck failed:\n%1")
                                .arg(systemError));

    QString libsshError;
    const bool ok = tryLibssh(&libsshError);
    endProgressUpdate();
    if (!ok)
      reportFailure(libsshError);
    return ok;
  }
#else
  if (m_sshMethod == "auto") {
    QString systemError;
    if (trySystem(&systemError)) {
      endProgressUpdate();
      Common::message(tr("Using System SSH method."));
      return true;
    }
    endProgressUpdate();
    const QString error = tr("System SSH precheck failed and libssh is not "
                             "available in this build: %1")
                            .arg(systemError);
    Common::error(error);
    emit errorDialogRequested(error);
    return false;
  }
#endif

  endProgressUpdate();
  Common::error(tr("SSH method '%1' is not available in this build.")
                  .arg(m_sshMethod));
  return false;
}

void SearchBase::performTheExit(int delay)
{
  // Stop queue checks before ending the session.
  setSessionActive(false);

  // This functions performs the exit, i.e., terminates the run.
  // The input parameter "delay" has a default of 0. If a non-zero
  //   delay is specified, the function waits for that amount, and
  //   will try to do some clean up before quitting.

  bool runDidNotStart = queue()->getAllStructures().isEmpty();

  // A hard exit (no delay) leaves right now: no waits, no final save, nothing!
  if (delay <= 0) {
    if (!runDidNotStart && !isReadOnly()) {
      QString formattedTime = QDateTime::currentDateTime().toString("MMMM dd, yyyy   hh:mm:ss");
      Common::message("\n=== Optimization aborted ... " + formattedTime.toLocal8Bit() + "\n");
    }
    std::fflush(nullptr);
    std::_Exit(0);
  }

  if (delay > 0) {
    // Give running script launches the extra time to finish naturally.
    queue()->waitForScriptCalculations(delay * 1000);

    // Wait for pending work.
    QThreadPool::globalInstance()->waitForDone(delay * 1000);

    // Stop pending work before deleting structures it may still use.
    m_shuttingDown.store(true);
    m_evaluationUpdateJob.shutdown();
    queue()->waitForScriptCalculations(-1);
    QThreadPool::globalInstance()->waitForDone(-1);

    // Let the application refresh and save while structures are still loaded.
    emit activeSessionFinalizing();

    // Stop the queue thread.
    stopQueueThread();

    // Destroy queuemanager
    queue()->reset();
    tracker()->deleteAllStructures();

    // m_ssh destroyed automatically by unique_ptr.
    m_ssh.reset();
  }

  if (!runDidNotStart && !isReadOnly()) {
    QString formattedTime = QDateTime::currentDateTime().toString("MMMM dd, yyyy   hh:mm:ss");
    QByteArray formattedTimeMsg = formattedTime.toLocal8Bit();
    Common::message("\n=== Optimization finished ... " + formattedTimeMsg + "\n");
  }

  // The engine is done. The application decides what happens to the
  // process (normally: quit).
  emit sessionEnded();
}

int SearchBase::getOptimizableObjectivesNum() const
{
  int objNumb = 0;
  for (int i = 0; i < getObjectivesNum(); ++i) {
    if (objectiveParticipatesInOptimization(i))
      ++objNumb;
  }
  return objNumb;
}

bool SearchBase::objectiveParticipatesInOptimization(int i) const
{
  const ObjType type = getObjectivesTyp(i);
  if (type != SearchBase::Ot_Min && type != SearchBase::Ot_Max)
    return false;

  if (m_paretoFilterZeroWeights && m_optimizationType == OT_Pareto &&
      std::fabs(getObjectivesWgt(i)) <= ZERO08)
    return false;

  return true;
}

void SearchBase::buildObjWeights(int& objNumb, std::vector<double>& objWght) const
{
  objNumb = getOptimizableObjectivesNum();
  objWght.clear();
  for (int i = 0; i < getObjectivesNum(); i++) {
    if (objectiveParticipatesInOptimization(i))
      objWght.push_back(getObjectivesWgt(i));
  }
}

bool SearchBase::hasCompleteObjectiveValues(const Structure& structure) const
{
  if (structure.getStrucObjNumber() != getObjectivesNum())
    return false;

  for (int i = 0; i < getObjectivesNum(); ++i) {
    const double value = structure.getStrucObjValues(i);
    if (GS_ISNAN(value) || GS_ISINF(value))
      return false;
  }
  return true;
}

bool SearchBase::buildObjDataFromPool(const QList<Structure*>& structures, int objNumb,
                                      std::vector<std::vector<double>>& objData,
                                      QList<QString>& strTags) const
{
  int strNumb = structures.size();
  objData.clear();
  objData.reserve(strNumb);
  strTags.clear();

  for (int i = 0; i < strNumb; i++) {
    Structure* s = structures[i];
    QReadLocker lock(&s->lock());

    strTags.push_back(s->getTag());

    if (!hasCompleteObjectiveValues(*s)) {
      Common::error(QString("%1: structure %2 does not have a complete objective table")
              .arg(__func__)
              .arg(s->getTag()));
      objData.clear();
      strTags.clear();
      return false;
    }

    std::vector<double> obj_vec;
    obj_vec.reserve(objNumb);
    for (int j = 0; j < getObjectivesNum(); ++j) {
      if (!objectiveParticipatesInOptimization(j))
        continue;
      if (getObjectivesTyp(j) == SearchBase::Ot_Min) {
        obj_vec.push_back(s->getStrucObjValues(j));
      } else if (getObjectivesTyp(j) == SearchBase::Ot_Max) {
        obj_vec.push_back(-s->getStrucObjValues(j));
      }
    }
    objData.push_back(obj_vec);
  }
  return true;
}

void SearchBase::normalizeObjData(std::vector<std::vector<double>>& objData) const
{
  if (objData.empty())
    return;

  const int strNumb = static_cast<int>(objData.size());
  const int objNumb = static_cast<int>(objData.front().size());

  std::vector<double> objMins(objNumb, DBL_MAX);
  std::vector<double> objMaxs(objNumb, -DBL_MAX);
  std::vector<double> objSprd(objNumb, 0.0);
  for (int i = 0; i < objNumb; ++i) {
    for (int j = 0; j < strNumb; ++j) {
      const double objval = objData[j][i];
      if (objval < objMins[i])
        objMins[i] = objval;
      if (objval > objMaxs[i])
        objMaxs[i] = objval;
    }
    objSprd[i] = objMaxs[i] - objMins[i];
  }

  for (int i = 0; i < objNumb; ++i) {
    for (int j = 0; j < strNumb; ++j) {
      objData[j][i] -= objMins[i];
      if (objSprd[i] > ZERO08)
        objData[j][i] /= objSprd[i];
      else
        objData[j][i] = 0.0;
      objData[j][i] = Common::roundToDecimalPlaces(objData[j][i], m_objectivePrecision);
    }
  }
}

bool SearchBase::parentPoolEligible(Structure* s) const
{
  QReadLocker structureLocker(&s->lock());
  return s->getStatus() == Structure::Optimized && !s->isSimilar() &&
         hasCompleteObjectiveValues(*s);
}

void SearchBase::refreshParentPoolMembership(Structure* s)
{
  if (!s)
    return;

  const bool eligible = parentPoolEligible(s);

  QWriteLocker tableLocker(&m_parentSelectionDataLock);
  if (eligible == m_parentPool.contains(s))
    return;
  if (eligible)
    m_parentPool.insert(s);
  else
    m_parentPool.remove(s);
  // The pool changed; the selection table is rebuilt at the next selection.
  ++m_selectionDataStamp;
}

void SearchBase::rebuildParentPoolMembership()
{
  QSet<Structure*> pool;
  {
    QReadLocker trackerLocker(m_tracker->rwLock());
    for (Structure* s : *m_tracker->list()) {
      if (s && parentPoolEligible(s))
        pool.insert(s);
    }
  }

  QWriteLocker tableLocker(&m_parentSelectionDataLock);
  m_parentPool = pool;
  ++m_selectionDataStamp;
}

int SearchBase::getParentPoolSize() const
{
  QReadLocker tableLocker(&m_parentSelectionDataLock);
  return m_parentPool.size();
}

QList<Structure*> SearchBase::getAllParentPoolStructures() const
{
  QReadLocker tableLocker(&m_parentSelectionDataLock);
  return m_parentPool.values();
}

void SearchBase::applyParentSelectionFronts()
{
  bool needsUpdate = false;
  {
    QReadLocker tableLocker(&m_parentSelectionDataLock);
    needsUpdate = !m_parentSelectionData.built ||
                  m_parentSelectionData.stamp != m_selectionDataStamp.load();
  }

  if (needsUpdate) {
    QReadLocker runtimeLocker(runtimeSettingsLock());
    bool updated = false;
    while (!updated) {
      const long long poolStamp = m_selectionDataStamp.load();
      const QList<Structure*> currentPool = getAllParentPoolStructures();
      if (poolStamp != m_selectionDataStamp.load())
        continue;

      QWriteLocker tableLocker(&m_parentSelectionDataLock);
      if (m_parentSelectionData.built &&
          m_parentSelectionData.stamp == m_selectionDataStamp.load()) {
        updated = true;
      } else if (poolStamp == m_selectionDataStamp.load()) {
        rebuildParentSelectionData(currentPool);
        updated = true;
      }
    }
  }

  QList<Structure*> pool;
  std::vector<int> fronts;
  {
    QReadLocker tableLocker(&m_parentSelectionDataLock);
    if (m_parentSelectionData.built && m_parentSelectionData.valid) {
      pool = m_parentSelectionData.pool;
      fronts = m_parentSelectionData.fronts;
    }
  }
  if (pool.size() != static_cast<int>(fronts.size()))
    return;

  QList<Structure*> structures;
  {
    QReadLocker trackerLocker(tracker()->rwLock());
    structures.reserve(tracker()->list()->size());
    for (Structure* structure : *tracker()->list())
      structures.append(structure);
  }

  QHash<Structure*, int> frontOfPoolMember;
  for (int i = 0; i < pool.size(); ++i)
    frontOfPoolMember.insert(pool.at(i), fronts[i]);

  QHash<QString, Structure*> structureByTag;
  QHash<Structure*, QString> similarityTag;
  for (Structure* structure : structures) {
    QReadLocker structureLocker(&structure->lock());
    structureByTag.insert(structure->getTag(), structure);
    similarityTag.insert(structure, structure->getSimilarityString());
  }

  // Set Pareto front of every structure (uses pool values). For similar
  //   structures, we use the front for the "similar to" value!
  for (Structure* structure : structures) {
    int front = frontOfPoolMember.value(structure, -1);
    if (front < 0 && !similarityTag.value(structure).isEmpty()) {
      Structure* keep = structure;
      int hops = structures.size();
      while (keep && !similarityTag.value(keep).isEmpty() && hops-- > 0)
        keep = structureByTag.value(similarityTag.value(keep), nullptr);
      front = frontOfPoolMember.value(keep, -1);
    }
    QWriteLocker structureLocker(&structure->lock());
    structure->setParetoFront(front);
  }
}

void SearchBase::refreshParentSelectionFronts(const QList<Structure*>& pool)
{
  QReadLocker runtimeLocker(runtimeSettingsLock());
  {
    QWriteLocker tableLocker(&m_parentSelectionDataLock);
    rebuildParentSelectionData(pool);
  }
  runtimeLocker.unlock();
  applyParentSelectionFronts();
}

int SearchBase::selectTournamentParent(const QList<Structure*>& structures,
                                       const std::vector<int>& strFrnt,
                                       const std::vector<double>& strDist,
                                       int str_a, int str_b, int total) const
{
  int parent;
  if      (strFrnt[str_a] < strFrnt[str_b])
    parent = str_a;
  else if (strFrnt[str_b] < strFrnt[str_a])
    parent = str_b;
  else if (strDist[str_a] > strDist[str_b])
    parent = str_a;
  else if (strDist[str_b] > strDist[str_a])
    parent = str_b;
  else
    parent = (Common::getRandDouble() < 0.5) ? str_a : str_b;

  if (m_verbose) {
    QString outs = QString("\n   Selected (tournament) %1 from structures with rank-dist (%2)")
      .arg(structures[parent]->getTag(),7).arg(total);
    outs += QString("\n   %1   %2   %3").arg(structures[str_a]->getTag(),7)
                .arg(strFrnt[str_a], 4).arg(strDist[str_a],10,'f',6);
    outs += QString("\n   %1   %2   %3").arg(structures[str_b]->getTag(),7)
                .arg(strFrnt[str_b], 4).arg(strDist[str_b],10,'f',6);
    outs += QString("\n\n");
    Common::message(outs);
  }

  return parent;
}

void SearchBase::rebuildParentSelectionData(const QList<Structure*>& structures)
{
  Common::ScopedTimer _timer("SearchBase::rebuildParentSelectionData");

  // Build the selection data once for this parent list.
  ParentSelectionData table;
  table.pool = structures;
  table.stamp = m_selectionDataStamp.load();
  table.built = true;

  int strNumb = structures.size();
  int objNumb = 0;
  std::vector<double> objWght;
  buildObjWeights(objNumb, objWght);

  // No optimizable objectives: parent selection falls back to a random pick.
  if (objNumb == 0) {
    table.noObjectives = true;
    m_parentSelectionData = std::move(table);
    return;
  }

  bool usePareto = (m_optimizationType == OT_Pareto) ? true : false;
  table.usePareto = usePareto;

  std::vector<std::vector<double>> objData; // final optimization 2D input matrix
  QList<QString> strTags;                   // structures tags
  if (!buildObjDataFromPool(structures, objNumb, objData, strTags)) {
    table.valid = false;
    m_parentSelectionData = std::move(table);
    return;
  }

  // Scale objective values and apply the objective precision to the values and weights.
  normalizeObjData(objData);
  for (size_t i = 0; i < objData.size(); i++)
    for (int j = 0; j < objNumb; j++)
      objData[i][j] = Common::roundToDecimalPlaces(objData[i][j], m_objectivePrecision);
  for (int j = 0; j < objNumb; j++)
    objWght[j] = Common::roundToDecimalPlaces(objWght[j], m_objectivePrecision);

  // Calculate the basic and Pareto probabilities.
  std::vector<double> strProb; // final raw probability of structures
  std::vector<int>    strFrnt(strNumb, -1);
  std::vector<double> strDist(strNumb, -1.0);
  std::vector<double> rawprob(strNumb, -1.0);
  std::vector<double> rawdist(strNumb, -1.0);

  if (usePareto) {
    strProb = paretoProbs(objData, m_crowdingDistance, strFrnt, strDist,
                          rawprob, rawdist);
  } else {
    strProb = scalarProbs(objData, objWght);
    // Although basic selection doesn't use fronts; we always produce and save them
    const std::vector<std::vector<int>> fronts = nonDominatedSorting(objData);
    for (int i = 0; i < static_cast<int>(fronts.size()); i++)
      for (int j = 0; j < static_cast<int>(fronts[i].size()); j++)
        strFrnt[fronts[i][j]] = i;
  }

  // extra "debug-like" output (if verbose output is set)
  if (m_verbose && strNumb > 0) {
    QString debOuts = "\n   STARTOBJECTIVESDATA=============\n";
    debOuts += QString("   Total structures: %1 - ").arg(strNumb,5);
    debOuts += QString("Precision: %1 - ").arg(m_objectivePrecision);
    if (usePareto) {
      debOuts += QString("Optimization type: Pareto - ");
      if (m_crowdingDistance)
        debOuts += QString("Crowding: Yes\n");
      else
        debOuts += QString("Crowding: No\n");
    } else {
      debOuts += QString("Optimization type: Basic\n");
    }

    QString enet = "Primary";
    debOuts += QString("   Objs [%1] - Wgt\n").arg(enet,4);
    for (int i = 0; i < objNumb; i++)
      debOuts += QString("  %1\n").arg(objWght[i],16,'f',10);

    debOuts += QString("   Tag and Normalized minimization matrix # Probs (finProb");
    if (usePareto) {
      debOuts += QString(", rawProb, Frnt, rawDist, sclDist");
    }
    debOuts += QString(")\n");
    for (int i = 0; i < strNumb; i++) {
      debOuts += QString("    %1 ").arg(strTags[i],7);
      for (int j = 0; j < objNumb; j++)
        debOuts += QString(" %1 ").arg(objData[i][j],10,'f',6);
      debOuts += QString(" # %1 ").arg(strProb[i],10,'f',6);
      if (usePareto) {
        debOuts += QString(" %1 ").arg(rawprob[i],10,'f',6);
        debOuts += QString(" %1 ").arg(strFrnt[i],4);
        debOuts += QString(" %1 ").arg(rawdist[i],10,'f',6);
        debOuts += QString(" %1 ").arg(strDist[i],10,'f',6);
      }
      debOuts += QString("\n");
    }
    debOuts += "   ENDOBJECTIVESDATA===============\n";
    Common::message(debOuts);
  }

  table.objData = std::move(objData);
  table.fronts = std::move(strFrnt);
  table.distances = std::move(strDist);
  table.probs = std::move(strProb);
  m_parentSelectionData = std::move(table);
}

Structure* SearchBase::selectParentStructure(size_t poolSize)
{
  // Select a parent structure from the in-memory parent pool.
  //
  // We compute scalar probabilities from both the basic generalized fitness
  //   function and the Pareto-based fitness. Which one actually drives parent
  //   selection is the user's choice:
  //   - Basic  optimization (the scalar generalized fitness function)
  //   - Pareto optimization (tournament selection, with/without a restricted pool)
  //   - Pareto optimization (ranks/distances turned into scalar fitness)
  //
  // For Pareto we may ignore the crowding distances if the user asks, and we
  //   apply the user's objective precision to the values throughout.

  QReadLocker runtimeLocker(runtimeSettingsLock());

  if (poolSize == 0)
    return nullptr;

  //============================ MAKE SURE THE SELECTION TABLE IS CURRENT
  // Update the selection data when the parent pool or selection settings change.
  {
    QReadLocker tableLocker(&m_parentSelectionDataLock);
    if (!m_parentSelectionData.built ||
        m_parentSelectionData.stamp != m_selectionDataStamp.load()) {
      tableLocker.unlock();
      QWriteLocker tableWriteLocker(&m_parentSelectionDataLock);
      if (!m_parentSelectionData.built ||
          m_parentSelectionData.stamp != m_selectionDataStamp.load())
        rebuildParentSelectionData(m_parentPool.values());
    }
  }

  QReadLocker tableLocker(&m_parentSelectionDataLock);
  const ParentSelectionData& table = m_parentSelectionData;
  const QList<Structure*>& structures = table.pool;

  if (structures.isEmpty())
    return nullptr;
  if (structures.size() == 1)
    return structures.first();

  if (!table.valid)
    return nullptr;
  if (table.noObjectives)
    return structures[Common::getRandUInt(0, structures.size() - 1)];

  // Read the selection data.
  int strNumb = structures.size();
  const bool usePareto = table.usePareto;
  const std::vector<std::vector<double>>& objData = table.objData;
  const std::vector<double>& strProb = table.probs;
  const std::vector<int>& strFrnt = table.fronts;
  const std::vector<double>& strDist = table.distances;

  // Sanity check
  if (static_cast<int>(strProb.size()) != strNumb) {
    Common::error("Failed to calculate fitness values!");
    return nullptr;
  }

  // Select from the full list for an unrestricted tournament selection.
  if (usePareto && m_tournamentSelection && !m_restrictedPool) {
    const int total = strNumb;
    const int str_a = Common::getRandUInt(0, total-1);
    int str_b;
    do {str_b = Common::getRandUInt(0, total-1);}
    while (str_b == str_a);
    return structures[selectTournamentParent(structures, strFrnt, strDist, str_a, str_b, total)];
  }

  // Construct <index in structures list, prob> variable for further processing
  //   the probabilities and converting them to normalized fitness
  //   values for "poolSize" number of structures.
  QList<QPair<int, double>> probs;
  for (int i = 0; i < strNumb; i++)
    probs.append(QPair<int, double>(i, strProb[i]));

  //============================ FIND BEST "POOLSIZE" STRUCTURES
  // Another introductory step! Finding top structures is always needed for basic
  //   optimization. For Pareto optimization, however, this is only needed if
  //   restricted pool with tournament selection is chosen.
  // So, we do this step here, and use it depending on the parent selection setting.

  // Contribution of any objective with zero spread is already set to zero.
  // So, we won't have all probs "nan" anymore. Still, they might be all "zero"!
  // Just to be sure, here we check if they are all zero (or nan). If so, we set them
  //   all to a fixed value and proceed as usual (i.e., normalizing the probs etc.)
  bool allNan = true;
  bool allZer = true;
  for (const auto& prob: probs) {
    if (!GS_ISNAN(prob.second))
      allNan = false;
    if (prob.second > ZERO08)
      allZer = false;
  }

  if (allNan || allZer) {
    for (auto& prob: probs)
      prob.second = 1.0;
  }

  // Sort by probability
  std::sort(probs.begin(), probs.end(),
            [](const QPair<int, double>& a,
               const QPair<int, double>& b)
            {
              return a.second < b.second;
            });

  // Remove the lowest probability structures until we have the parent pool size
  size_t effectivePoolSize = poolSize;
  if (usePareto && m_tournamentSelection && m_restrictedPool &&
      effectivePoolSize < 2 && probs.size() >= 2) {
    effectivePoolSize = 2;
  }
  while (static_cast<size_t>(probs.size()) > effectivePoolSize)
    probs.pop_front();

#ifdef SEARCHBASE_PROBS_DEBUG
  QString outs1 = QString("\n   Unnormalized (but sorted and trimmed) probs list is:\n"
                         "    structure : energetics : probs\n");
  for (const auto& elem: probs) {
    outs1 += QString("      %1 : %3 : %4\n").arg(structures[elem.first]->getTag(),7)
      .arg(objData[elem.first][0],0,'f',6).arg(elem.second,0,'f',6);
  }
  Common::debug(outs1);
#endif

  //============================ PARENT SELECTION 1: PARETO (TOURNAMENT SELECTION)
  // If performing Pareto optimization with restricted-pool tournament selection,
  // make a binary selection from the best subset. Unrestricted tournament already
  // returned above before constructing the probability subset.
  if (usePareto && m_tournamentSelection) {
    const int total = probs.size();
    if (total < 2) {
      Common::error("Tournament parent selection requires at least two structures.");
      return nullptr;
    }

    const int str_a = probs[Common::getRandUInt(0, total-1)].first;
    int str_b;
    do {str_b = probs[Common::getRandUInt(0, total-1)].first;}
    while (str_b == str_a);
    return structures[selectTournamentParent(structures, strFrnt, strDist, str_a, str_b, total)];
  }

  // Sum the resulting probs
  double sum = 0.0;
  for (const auto& elem: probs)
    sum += elem.second;

  // Normalize the list so that the sum is 1
  for (auto& elem: probs)
    elem.second /= sum;

#ifdef SEARCHBASE_PROBS_DEBUG
  outs1 = QString("   Normalized, sorted, and trimmed probs list is:\n"
                  "    structure : energetics : probs\n");
  for (const auto& elem: probs) {
    outs1 += QString("      %1 : %3 : %4\n").arg(structures[elem.first]->getTag(),7)
      .arg(objData[elem.first][0],0,'f',6).arg(elem.second,0,'f',6);
  }
  Common::debug(outs1);
#endif

  // Now replace each entry with a cumulative total
  sum = 0.0;
  for (auto& elem: probs) {
    sum += elem.second;
    elem.second = sum;
  }

#ifdef SEARCHBASE_PROBS_DEBUG
  outs1 = QString("   Cumulative (final) probs list is:\n" "    structure : energetics : probs\n");
  for (const auto& elem: probs) {
    outs1 += QString("      %1 : %3 : %4\n").arg(structures[elem.first]->getTag(),7)
      .arg(objData[elem.first][0],0,'f',6).arg(elem.second,0,'f',6);
  }
  Common::debug(outs1);
#endif

  //============================ PARENT SELECTION 2: BASIC AND PARETO (SCALAR FITNESS-BASED)
  // Pick a parent using scalar fitness values from the "top" structures
  //   for either basic/Pareto optimization.
  // We use a random threshold and select the structure
  //   with fitness above the chosen random threshold
  int parent = probs.back().first;
  double r = Common::getRandDouble();
  for (const auto& elem : probs) {
    if (r < elem.second) {
      parent = elem.first;
      break;
    }
  }
  if (m_verbose) {
    QString scaltype = "scalar-basic";
    if (usePareto)
      scaltype = "scalar-pareto";

    QString outs = QString("\n   Selected (%1) %2 ( r = %3 ) from structures with probs (%5)")
      .arg(scaltype).arg(structures[parent]->getTag(),7).arg(r,8,'f',6).arg(probs.size());
    outs += QString("\n      structure : energetics :    probs   : cumulative probs\n");
    double previousProbs = 0.0;
    for (const auto& elem: probs) {
      outs += QString("        %1 :     %2 : %3 : %4\n")
              .arg(structures[elem.first]->getTag(),7)
              .arg(objData[elem.first][0],12,'f',6)
              .arg(elem.second - previousProbs,10,'f',6).arg(elem.second,10,'f',6);
      previousProbs = elem.second;
    }
    outs += QString("\n");
    Common::message(outs);
  }

  return structures[parent];
}


bool SearchBase::isReadyToSearch(QString* err) const
{
  if (err)
    err->clear();

  // The application already checks the working directory before starting;
  //   this line only covers direct use of the engine.
  if (getLocWorkDir().isEmpty()) {
    if (err)
      *err += "Local working directory is not set.";
    return false;
  }

  for (size_t i = 0; i < getNumOptSteps(); ++i) {
    if (!queueInterface(i)) {
      if (err) {
        *err += "Queue interface at opt step " + QString::number(i + 1) +
                " is not set!";
      }
      return false;
    }

    if (!optimizer(i)) {
      if (err)
        *err += "Optimizer at opt step " + QString::number(i + 1) + " is not set!";
      return false;
    }

    if (!queueInterface(i)->isReadyToSearch(err))
      return false;
  }

  if (!checkObjectiveAndConstraintScripts(err))
    return false;

  return true;
}

bool SearchBase::checkObjectiveAndConstraintScripts(QString* err) const
{
  if (err)
    err->clear();

  for (size_t optStep = 0; optStep < getNumOptSteps(); ++optStep) {
    QueueInterface* queue = queueInterface(static_cast<int>(optStep));
    if (!queue)
      continue;

    for (int i = 0; i < getObjectivesNum(); ++i) {
      if (!objectiveNeedsExternalCalculation(i))
        continue;
      if (!checkScriptPath(getObjectivesExe(i), "objective", i, queue, err))
        return false;
    }

    for (int i = 0; i < getConstraintsNum(); ++i) {
      if (!checkScriptPath(getConstraintExe(i), "constraint", i, queue, err))
        return false;
    }
  }

  return true;
}

bool SearchBase::checkScriptPath(const QString& path, const QString& label, int index,
                                 QueueInterface* queue, QString* err) const
{
  const QString trimmed = path.trimmed();
  const bool remoteScript = isRemoteQueue() && queue && queue->getIDString().toLower() != "none";

  if (remoteScript) {
    if (!isRemoteAbsolutePath(trimmed)) {
      if (err) {
        *err = tr("%1 script %2 must be an absolute remote path: %3")
                 .arg(label)
                 .arg(index + 1)
                 .arg(trimmed);
      }
      return false;
    }

    const QString command = "test -f " + shellSingleQuote(trimmed);
    const QueueInterface::CommandResult result = queue->runACommand("", command);
    if (!result.succeeded()) {
      if (err) {
        *err = tr("%1 script %2 was not found on the remote host: %3")
                 .arg(label)
                 .arg(index + 1)
                 .arg(trimmed);
      }
      return false;
    }
    Common::message(QString("Checked --- remote %1 script %2: %3")
                      .arg(label)
                      .arg(index + 1)
                      .arg(trimmed));
    return true;
  }

  const QFileInfo info(trimmed);
  if (!info.isAbsolute()) {
    if (err) {
      *err = tr("%1 script %2 must be an absolute local path: %3")
               .arg(label)
               .arg(index + 1)
               .arg(trimmed);
    }
    return false;
  }

  if (!info.exists() || !info.isFile()) {
    if (err) {
      *err = tr("%1 script %2 was not found on the local machine: %3")
               .arg(label)
               .arg(index + 1)
               .arg(trimmed);
    }
    return false;
  }

  const QString readablePath = info.canonicalFilePath().isEmpty()
                                 ? info.absoluteFilePath()
                                 : info.canonicalFilePath();
  Common::message(QString("Checked --- %1 script %2: %3")
                    .arg(label)
                    .arg(index + 1)
                    .arg(readablePath));
  return true;
}

bool SearchBase::anyBatchQueueInterfaces() const
{
  for (size_t i = 0; i < getNumOptSteps(); ++i) {
    if (queueInterface(i)->getIDString().toLower() != "none")
      return true;
  }
  return false;
}

bool SearchBase::requestBooleanDecision(const QString& message, bool defaultValue)
{
  // The call waits since the answer is needed before we can continue.
  return m_decisionPromptHandler(message, defaultValue);
}

bool SearchBase::requestPassword(const QString& message, QString& newPassword)
{
  // The CLI turns off terminal echo; the GUI asks in its own thread.
  QString password;
  const bool accepted = m_passwordPromptHandler(message, password);
  newPassword = password;
  return accepted;
}

void SearchBase::beginProgressUpdate(const QString& label, int min, int max)
{
  // Progress updates go out as signals rather than Common messages, so
  //   they can drive progress bars without adding permanent log lines.
  emit progressRangeChanged(label, min, max);
}

void SearchBase::endProgressUpdate()
{
  emit progressEnded();
}

void SearchBase::updateProgressValue(int value, const QString& label, int min, int max)
{
  emit progressValueChanged(value, label, min, max);
}

} // end namespace Search

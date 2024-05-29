/**********************************************************************
  XtalOpt - "Engine" for the optimization process

  Copyright (C) 2009-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/xtalopt.h>

#include <xtalopt/cliOptions.h>
#include <xtalopt/genetic.h>
#include <xtalopt/optimizers/optimizers.h>
#include <xtalopt/rpc/xtaloptrpc.h>
#include <xtalopt/structures/xtal.h>
#include <xtalopt/ui/dialog.h>
#include <xtalopt/ui/randSpgDialog.h>

#include <randSpg/include/xtaloptWrapper.h>

#include <globalsearch/bt.h>
#include <globalsearch/eleminfo.h>
#include <globalsearch/formats/cmlformat.h>
#include <globalsearch/formats/obconvert.h>
#include <globalsearch/optbase.h>
#include <globalsearch/optimizer.h>
#include <globalsearch/queueinterface.h>
#include <globalsearch/queueinterfaces/queueinterfaces.h>
#include <globalsearch/queuemanager.h>
#include <globalsearch/random.h>
#include <globalsearch/slottedwaitcondition.h>
#include <globalsearch/utilities/fileutils.h>
#include <globalsearch/utilities/makeunique.h>
#include <globalsearch/utilities/utilityfunctions.h>

#ifdef ENABLE_SSH
#include <globalsearch/queueinterfaces/remote.h>
#include <globalsearch/sshmanager.h>
#endif // ENABLE_SSH

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QReadWriteLock>
#include <QTimer>
#include <QtConcurrent>

#include <randSpg/include/randSpg.h>

#include <fstream>
#include <iostream>
#include <mutex>

using namespace GlobalSearch;

namespace XtalOpt {

XtalOpt::XtalOpt(GlobalSearch::AbstractDialog* parent)
  : OptBase(parent), formulaUnitsList({ 1 }), lowestEnthalpyFUList({ 0, 0 }),
    using_randSpg(false), minXtalsOfSpgPerFU(QList<int>()),
    m_rpcClient(make_unique<XtalOptRpc>()),
    m_initWC(new SlottedWaitCondition(this))
{
  xtalInitMutex = new QMutex;
  m_idString = "XtalOpt";
  m_schemaVersion = 3;

  // Read the general settings
  readSettings();

  // Connections
  connect(m_tracker, SIGNAL(newStructureAdded(GlobalSearch::Structure*)), this,
          SLOT(checkForDuplicates()));
  connect(this, SIGNAL(sessionStarted()), this, SLOT(resetDuplicates()));
  connect(m_queue, SIGNAL(structureFinished(GlobalSearch::Structure*)), this,
          SLOT(updateLowestEnthalpyFUList(GlobalSearch::Structure*)));

  connect(this, &OptBase::dialogSet, this, &XtalOpt::setupRpcConnections);
}

XtalOpt::~XtalOpt()
{
  // The dialog should be gone, so make sure everywhere else knows
  m_dialog = nullptr;

  // Save one last time
  qDebug() << "\nSaving XtalOpt settings...";

  // First save the state file (only if we have structures)
  if (!m_queue->getAllStructures().isEmpty())
    save("", false);

  // Then save the config settings
  QString configFileName = QSettings().fileName();
  save(configFileName, false);

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

  // Clean up various members
  m_initWC->deleteLater();
  m_initWC = 0;

  QString formattedTime = QDateTime::currentDateTime().toString("MMMM dd, yyyy   hh:mm:ss");
  QByteArray formattedTimeMsg = formattedTime.toLocal8Bit();
  qDebug().noquote() << "\n=== Optimization finished ... " + formattedTimeMsg + "\n";
}

bool XtalOpt::startSearch()
{
  // Let's make sure it doesn't glitch if the user presses "Begin"
  // too many times in a row.
  static std::mutex startMutex;
  std::unique_lock<std::mutex> startLock(startMutex, std::defer_lock);
  if (!startLock.try_lock())
    return false;

#ifdef XTALOPT_DEBUG
  // Setup the message handler for the run log file
  saveLogFileOfRun(locWorkDir);
#endif

  // Settings checks
  // Check lattice parameters, volume, etc
  if (!XtalOpt::checkLimits()) {
    error("Cannot create structures. Check log for details.");
    return false;
  }

  // Do we have a composition?
  if (comp.isEmpty()) {
    error("Cannot create structures. Composition is not set.");
    return false;
  }

  // Check if xtalopt data is already saved at the locWorkDir
  // If we are in cli mode, we check it elsewhere
  while (QFile::exists(locWorkDir + QDir::separator() + "xtalopt.state") &&
      !testingMode && m_usingGUI) {
    bool proceed;
    needBoolean(tr("Error: XtalOpt data is already saved at:"
                   "\n%1"
                   "\n\nEmpty the directory to proceed or "
                   "select a new 'Local working directory'!"
                   "\n\nDo you wish to proceed?"
                   "\n[Yes] directory is cleaned up"
                   "\n[No] return")
                   .arg(locWorkDir),
                   &proceed);
    if (!proceed) {
      return false;
    }
  }

  QString err;
  if (!isReadyToSearch(err)) {
    error(tr("Error: search is not ready to start: ") + err);
    return false;
  }

  // Warn user if runningJobLimit is 0
  if (limitRunningJobs && runningJobLimit == 0) {
    if (m_usingGUI) {
      error(tr("Warning: the number of running jobs is currently set to 0."
               "\n\nYou will need to increase this value before the search "
               "can begin (The option is on the 'Search Settings' tab)."));
    } else {
      qDebug() << "\nWarning: the number of running jobs is currently set to"
               << "0."
               << "\nYou will need to increase this value before the"
               << "search can begin \n(You can change this in the"
               << "xtalopt-runtime-options.txt file in the local working"
               << "directory).\n";
    }
  };

  // Warn user if contStructs is 0
  if (contStructs == 0) {
    if (m_usingGUI) {
      error(tr("Warning: the number of continuous structures is "
               "currently set to 0."
               "\n\nYou will need to increase this value before the search "
               "can move past the first generation (The option is on the "
               "'Search Settings' tab)."));
    } else {
      qDebug() << "\nWarning: the number of continuous structures is"
               << "currently set to 0."
               << "\nYou will need to increase this value before the"
               << "search can move past the first generation \n(You can"
               << "change this in the xtalopt-runtime-options.txt file in"
               << "the local working directory).\n";
    }
  };

#ifdef ENABLE_SSH
  // Create the SSHManager if running remotely
  if (anyRemoteQueueInterfaces()) {
    qDebug() << "Creating SSH connections...";
    if (!this->createSSHConnections()) {
      error(tr("Could not create ssh connections."));
      return false;
    }
  }
#endif // ENABLE_SSH

  // Here we go!
  QString formattedTime = QDateTime::currentDateTime().toString("MMMM dd, yyyy   hh:mm:ss");
  QByteArray formattedTimeMsg = formattedTime.toLocal8Bit();

  qDebug().noquote() << "\n=== Optimization started ... " + formattedTimeMsg + "\n";
  emit startingSession();

  // prepare pointers
  QWriteLocker trackerLocker(m_tracker->rwLock());
  m_tracker->deleteAllStructures();
  trackerLocker.unlock();

  ///////////////////////////////////////////////
  // Generate random structures and load seeds //
  ///////////////////////////////////////////////

  // Set up progress bar
  if (m_usingGUI && m_dialog)
    m_dialog->startProgressUpdate(tr("Generating structures..."), 0, 0);

  // Initalize loop variables
  int failed = 0;
  QString filename;
  Xtal* xtal = 0;
  // Use new xtal count in case "addXtal" falls behind so that we
  // don't duplicate structures when switching from seeds -> random.
  uint newXtalCount = 0;

  // Load seeds...
  for (int i = 0; i < seedList.size(); i++) {
    filename = seedList.at(i);
    if (this->addSeed(filename)) {
      if (m_usingGUI && m_dialog) {
        m_dialog->updateProgressLabel(
          tr("%1 structures generated (%2 kept, %3 rejected)...")
            .arg(i + failed)
            .arg(i)
            .arg(failed));
      }
      newXtalCount++;
    }
  }

  // Perform a regular random generation
  if (!using_randSpg) {
    // Generation loop...
    while (newXtalCount < numInitial) {
      updateProgressBar(numInitial, newXtalCount + failed, newXtalCount);

      // Generate/Check xtal
      xtal = generateRandomXtal(1, newXtalCount + 1);
      if (!checkXtal(xtal)) {
        delete xtal;
        failed++;
      } else {
        xtal->findSpaceGroup(tol_spg);
        initializeAndAddXtal(xtal, 1, xtal->getParents());
        newXtalCount++;
      }
    }
  }
  // Perform a spacegroup generation generation
  else {
    // If minXtalsOfSpgPerFU was never correctly generated, just generate one
    // now
    if (minXtalsOfSpgPerFU.size() == 0) {
      for (size_t spg = 1; spg <= 230; spg++)
        minXtalsOfSpgPerFU.append(0);
    }

    QList<int> spgStillNeeded = minXtalsOfSpgPerFU;

    // Find the total number of xtals that will be generated
    // If minXtalsOfSpgPerFU specifies more xtals to be generated than
    // numInitial, then the total needed will be the sum from
    // minXtalsOfSpgPerFU

    size_t numXtalsToBeGenerated = 0;
    for (size_t i = 0; i < minXtalsOfSpgPerFU.size(); i++) {
      // The value in the vector is -1 if that spg is to not be used
      if (minXtalsOfSpgPerFU.at(i) != -1)
        numXtalsToBeGenerated +=
          (minXtalsOfSpgPerFU.at(i) * formulaUnitsList.size());
    }

    // Now that minXtalsOfSpgPerFU is set up, proceed!
    newXtalCount = failed = 0;
    // Find spacegroups for which we have required a certain number of xtals
    // per formula unit
    for (size_t i = 0; i < spgStillNeeded.size(); i++) {
      while (spgStillNeeded.at(i) > 0) {
        for (size_t FU_ind = 0; FU_ind < formulaUnitsList.size(); FU_ind++) {
          // Update the progresss bar
          updateProgressBar((numXtalsToBeGenerated - failed > numInitial
                               ? numXtalsToBeGenerated - failed
                               : numInitial),
                            newXtalCount + failed, newXtalCount);

          uint spg = i + 1;
          uint FU = formulaUnitsList.at(FU_ind);

          // If the spacegroup isn't possible for this FU, just continue
          if (!RandSpg::isSpgPossible(spg, getStdVecOfAtoms(FU))) {
            numXtalsToBeGenerated--;
            continue;
          }

          // Generate/Check xtal
          xtal = randSpgXtal(1, newXtalCount + 1, FU, spg);
          if (!checkXtal(xtal)) {
            delete xtal;
            qWarning() << "Failed to generate an xtal with spacegroup of"
                       << QString::number(spg) << "and FU of"
                       << QString::number(FU);
            failed++;
          } else {
            xtal->findSpaceGroup(tol_spg);
            initializeAndAddXtal(xtal, 1, xtal->getParents());
            newXtalCount++;
          }
        }
        spgStillNeeded[i]--;
      }
    }
    // If we still haven't generated enough xtals, pick a random FU and spg
    // to be generated
    while (newXtalCount < numInitial) {
      // Let's keep the progress bar updated
      updateProgressBar(numInitial, newXtalCount + failed, newXtalCount);

      // Randomly select a formula unit
      uint randomFU =
        formulaUnitsList.at(rand() % int(formulaUnitsList.size()));
      // Randomly select a possible spg
      uint randomSpg =
        pickRandomSpgFromPossibleOnes(); // pickRandomSpgFromPossibleOnes();
      // If it isn't possible, try again
      if (!RandSpg::isSpgPossible(randomSpg, getStdVecOfAtoms(randomFU)))
        continue;
      // Try it out
      xtal = randSpgXtal(1, newXtalCount + 1, randomFU, randomSpg);
      if (!checkXtal(xtal)) {
        delete xtal;
        qWarning() << "Failed to generate an xtal with spacegroup of"
                   << QString::number(randomSpg) << "and FU of"
                   << QString::number(randomFU);
        failed++;
      } else {
        xtal->findSpaceGroup(tol_spg);
        initializeAndAddXtal(xtal, 1, xtal->getParents());
        newXtalCount++;
      }
      // If we failed, shake it off and try again
    }
  }

  // Wait for all structures to appear in tracker
  if (m_usingGUI && m_dialog) {
    m_dialog->updateProgressLabel(
      tr("Waiting for structures to initialize..."));
    m_dialog->updateProgressMinimum(0);
    m_dialog->updateProgressMinimum(newXtalCount);
  }

  connect(m_tracker, SIGNAL(newStructureAdded(GlobalSearch::Structure*)),
          m_initWC, SLOT(wakeAllSlot()));

  m_initWC->prewaitLock();
  do {
    if (m_usingGUI && m_dialog) {
      m_dialog->updateProgressValue(m_tracker->size());
      m_dialog->updateProgressLabel(
        tr("Waiting for structures to initialize (%1 of %2)...")
          .arg(m_tracker->size())
          .arg(newXtalCount));
    }
    // Don't block here forever -- there is a race condition where
    // the final newStructureAdded signal may be emitted while the
    // WC is not waiting. Since this is just trivial GUI updating
    // and we check the condition in the do-while loop, this is
    // acceptable. The following call will timeout in 250 ms.
    m_initWC->wait(250);
  } while (m_tracker->size() < newXtalCount);
  m_initWC->postwaitUnlock();

  // We're done with m_initWC.
  m_initWC->disconnect();

  if (m_usingGUI && m_dialog)
    m_dialog->stopProgressUpdate();

  if (m_dialog)
    m_dialog->saveSession();
  emit sessionStarted();
  return true;
}

bool XtalOpt::save(QString filename, bool notify)
{
  // We will only save once at a time
  std::unique_lock<std::mutex> saveLock(saveMutex, std::defer_lock);
  if (!saveLock.try_lock())
    return false;

  if (filename.isEmpty() && !locWorkDir.isEmpty())
    filename = locWorkDir + "/" + m_idString.toLower() + ".state";

  bool isStateFile = filename.endsWith(".state");

  // If we have a state file, call the parent save
  if (isStateFile)
    OptBase::save(filename, notify);

  SETTINGS(filename);
  settings->beginGroup("xtalopt/init/");

  settings->setValue("version", m_schemaVersion);

  settings->setValue("limits/a/min", a_min);
  settings->setValue("limits/b/min", b_min);
  settings->setValue("limits/c/min", c_min);
  settings->setValue("limits/a/max", a_max);
  settings->setValue("limits/b/max", b_max);
  settings->setValue("limits/c/max", c_max);
  settings->setValue("limits/alpha/min", alpha_min);
  settings->setValue("limits/beta/min", beta_min);
  settings->setValue("limits/gamma/min", gamma_min);
  settings->setValue("limits/alpha/max", alpha_max);
  settings->setValue("limits/beta/max", beta_max);
  settings->setValue("limits/gamma/max", gamma_max);
  settings->setValue("limits/volume/min", vol_min);
  settings->setValue("limits/volume/max", vol_max);
  settings->setValue("limits/volume/fixed", vol_fixed);
  settings->setValue("limits/scaleFactor", scaleFactor);
  settings->setValue("limits/minRadius", minRadius);
  settings->setValue("using/fixedVolume", using_fixed_volume);
  settings->setValue("limits/volume/scale_min", vol_scale_min);
  settings->setValue("limits/volume/scale_max", vol_scale_max);
  settings->setValue("using/mitosis", using_mitosis);
  settings->setValue("using/subcellPrint", using_subcellPrint);
  settings->setValue("limits/divisions", divisions);
  settings->setValue("limits/ax", ax);
  settings->setValue("limits/bx", bx);
  settings->setValue("limits/cx", cx);
  settings->setValue("using/interatomicDistanceLimit",
                     using_interatomicDistanceLimit);
  settings->setValue("using/molUnit", using_molUnit);
  settings->setValue("using/customIAD", using_customIAD);
  settings->setValue("using/randSpg", using_randSpg);
  settings->setValue("using/checkStepOpt", using_checkStepOpt);

  // Composition
  settings->beginWriteArray("composition");
  QList<uint> keys = comp.keys();
  for (int i = 0; i < keys.size(); i++) {
    if (keys.at(i) == 0)
      continue;
    settings->setArrayIndex(i);
    settings->setValue("atomicNumber", keys.at(i));
    settings->setValue("quantity", comp.value(keys.at(i)).quantity);
    settings->setValue("minRadius", comp.value(keys.at(i)).minRadius);
  }
  settings->endArray();

  // Formula Units List
  settings->beginWriteArray("Formula_Units");
  for (int i = 0; i < formulaUnitsList.size(); i++) {
    settings->setArrayIndex(i);
    settings->setValue("FU", formulaUnitsList.at(i));
  }
  settings->endArray();

  // Mol Unit stuff
  if (using_molUnit) {
    settings->beginWriteArray("compMolUnit");
    size_t ind = 0;
    for (const auto& pair : compMolUnit.keys()) {
      settings->setArrayIndex(ind);
      settings->setValue("center",
                         ElemInfo::getAtomicSymbol(pair.first).c_str());
      settings->setValue("number_of_centers", compMolUnit[pair].numCenters);
      settings->setValue("neighbor",
                         ElemInfo::getAtomicSymbol(pair.second).c_str());
      settings->setValue("number_of_neighbors", compMolUnit[pair].numNeighbors);
      settings->setValue("geometry", getGeom(compMolUnit[pair].numNeighbors,
                                             compMolUnit[pair].geom));
      settings->setValue("distance", compMolUnit[pair].dist);
      ++ind;
    }
    settings->endArray();
  }

  // Custom IAD stuff
  if (using_customIAD) {
    settings->beginWriteArray("customIAD");
    size_t ind = 0;
    for (const auto& pair : interComp.keys()) {
      settings->setArrayIndex(ind);
      settings->setValue("atomicNumber1", pair.first);
      settings->setValue("atomicNumber2", pair.second);
      settings->setValue("minInteratomicDist", interComp[pair].minIAD);
      ++ind;
    }
    settings->endArray();
  }

  settings->endGroup();

  writeEditSettings(filename);

  settings->beginGroup("xtalopt/opt/");

  // Initial generation
  settings->setValue("opt/numInitial", numInitial);

  // Search parameters
  settings->setValue("opt/popSize", popSize);
  settings->setValue("opt/contStructs", contStructs);
  settings->setValue("opt/limitRunningJobs", limitRunningJobs);
  settings->setValue("opt/runningJobLimit", runningJobLimit);
  settings->setValue("opt/failLimit", failLimit);
  settings->setValue("opt/failAction", failAction);
  settings->setValue("opt/cutoff", cutoff);
  settings->setValue("opt/using_mitotic_growth", using_mitotic_growth);
  settings->setValue("opt/using_FU_crossovers", using_FU_crossovers);
  settings->setValue("opt/FU_crossovers_generation", FU_crossovers_generation);
  settings->setValue("opt/using_one_pool", using_one_pool);
  settings->setValue("opt/chance_of_mitosis", chance_of_mitosis);

  // Duplicates
  settings->setValue("tol/xtalcomp/length", tol_xcLength);
  settings->setValue("tol/xtalcomp/angle", tol_xcAngle);
  settings->setValue("tol/spg", tol_spg);

  // Crossover
  settings->setValue("opt/p_cross", p_cross);
  settings->setValue("opt/cross_minimumContribution",
                     cross_minimumContribution);

  // Stripple
  settings->setValue("opt/p_strip", p_strip);
  settings->setValue("opt/strip_strainStdev_min", strip_strainStdev_min);
  settings->setValue("opt/strip_strainStdev_max", strip_strainStdev_max);
  settings->setValue("opt/strip_amp_min", strip_amp_min);
  settings->setValue("opt/strip_amp_max", strip_amp_max);
  settings->setValue("opt/strip_per1", strip_per1);
  settings->setValue("opt/strip_per2", strip_per2);

  // Permustrain
  settings->setValue("opt/p_perm", p_perm);
  settings->setValue("opt/perm_strainStdev_max", perm_strainStdev_max);
  settings->setValue("opt/perm_ex", perm_ex);

  // Multi-objective, aflow-hardness, softExit, localQueue, ...
  settings->setValue("opt/softExit", m_softExit);
  settings->setValue("opt/localQueue", m_localQueue);
  settings->setValue("obj/calculateHardness", m_calculateHardness.load());
  settings->setValue("obj/hardnessFitnessWeight", m_hardnessFitnessWeight.load());
  settings->setValue("obj/objectivesReDo", m_objectivesReDo);
  settings->setValue("obj/objectivesNum", getObjectivesNum());
  settings->beginWriteArray("obj/objectives");
  for (size_t i = 0; i < getObjectivesNum(); i++) {
    settings->setArrayIndex(i);
    settings->setValue("exe",getObjectivesExe(i));
    settings->setValue("typ",getObjectivesTyp(i));
    settings->setValue("wgt",getObjectivesWgt(i));
    settings->setValue("out",getObjectivesOut(i));
  }
  settings->endArray();

  return true;
}

bool XtalOpt::writeEditSettings(const QString& filename)
{
  SETTINGS(filename);
  settings->beginGroup("xtalopt/edit");
  settings->setValue("version", m_schemaVersion);

  settings->setValue("description", description);
  settings->setValue("locWorkDir", locWorkDir);
  settings->setValue("remote/host", host);
  settings->setValue("remote/port", port);
  settings->setValue("remote/username", username);
  settings->setValue("remote/queueRefreshInterval", queueRefreshInterval());
  settings->setValue("remote/cleanRemoteOnStop", cleanRemoteOnStop());
  settings->setValue("remote/remWorkDir", remWorkDir);
  settings->setValue("remote/cancelJobAfterTime", cancelJobAfterTime());
  settings->setValue("remote/hoursForCancelJobAfterTime",
                     hoursForCancelJobAfterTime());

  // This will also write the number of opt steps
  writeAllTemplatesToSettings(filename.toStdString());

  for (size_t i = 0; i < getNumOptSteps(); ++i) {
    settings->setValue("optimizer/" + QString::number(i),
                       optimizer(i)->getIDString().toLower());
    settings->setValue("queueInterface/" + QString::number(i),
                       queueInterface(i)->getIDString().toLower());
  }
  settings->setValue("logErrorDirs", m_logErrorDirs);
  settings->endGroup();

  writeUserValuesToSettings(filename.toStdString());

  for (size_t i = 0; i < getNumOptSteps(); ++i) {
    optimizer(i)->writeSettings(filename);
    queueInterface(i)->writeSettings(filename);
  }

  return true;
}

bool XtalOpt::readEditSettings(const QString& filename)
{
  SETTINGS(filename);

  // Edit tab
  settings->beginGroup("xtalopt/edit");
  port = settings->value("remote/port", 22).toInt();
  m_logErrorDirs = settings->value("logErrorDirs", false).toBool();

  int loadedVersion = settings->value("version", 0).toInt();

  // Only scheme version 3 or less will work currently
  if (!filename.isEmpty() && loadedVersion > 3) {
    error("XtalOpt::readSettings(): Settings in file " + filename +
          " cannot be opened by this version of XtalOpt. Please "
          "visit https://xtalopt.github.io to obtain a "
          "newer version.");
    return false;
  }

  // Temporary variables to test settings. This prevents empty
  // scheme values from overwriting defaults.
  QString tmpstr;

  tmpstr = settings->value("description", "").toString();
  if (!tmpstr.isEmpty()) {
    description = tmpstr;
  }

  tmpstr = settings->value("remote/remWorkDir", "").toString();
  if (!tmpstr.isEmpty()) {
    remWorkDir = tmpstr;
  }

  tmpstr = settings->value("locWorkDir", "").toString();
  if (!tmpstr.isEmpty()) {
    locWorkDir = tmpstr;
  }

  tmpstr = settings->value("remote/host", "").toString();
  if (!tmpstr.isEmpty()) {
    host = tmpstr;
  }

  tmpstr = settings->value("remote/username", "").toString();
  if (!tmpstr.isEmpty()) {
    username = tmpstr;
  }

  setQueueRefreshInterval(
    settings->value("remote/queueRefreshInterval", "10").toInt());

  setCleanRemoteOnStop(
    settings->value("remote/cleanRemoteOnStop", "false").toBool());

  m_cancelJobAfterTime =
    settings->value("remote/cancelJobAfterTime", "false").toBool();
  m_hoursForCancelJobAfterTime =
    settings->value("remote/hoursForCancelJobAfterTime", "100.0").toDouble();

  // Old load instructions
  if (loadedVersion < 3) {
    settings->endGroup();
    settings->beginGroup("xtalopt");

    QString optimizer = settings->value("edit/optimizer", "").toString();
    if (optimizer.isEmpty()) {
      if (!filename.isEmpty()) {
        qDebug() << "Error: loading settings for older XtalOpt version and"
                 << "the optimizer is empty!";
        return false;
      }
      optimizer = "gulp";
    }

    QString qi = settings->value("edit/queueInterface", "").toString();
    if (qi.isEmpty()) {
      if (!filename.isEmpty()) {
        qDebug() << "Error: loading settings for older XtalOpt version and"
                 << "the queue interface is empty!";
        return false;
      }
      qi = "local";
      ;
    }

    clearOptSteps();

    // Append the first step and grab the template names.
    appendOptStep();
    setOptimizer(0, optimizer.toStdString());
    setQueueInterface(0, qi.toStdString());

    // Get the POTCAR stuff if VASP
    QString potcarTemplate;
    if (optimizer.toLower() == "vasp") {
      this->optimizer(0)->readSettings(filename);
      QVariantList potcarInfo =
        this->optimizer(0)->getData("POTCAR info").toList();
      if (!potcarInfo.isEmpty()) {
        QVariantHash hash = potcarInfo.at(0).toHash();

        QList<uint> atomicNums = comp.keys();
        QStringList atomicSymbols;
        for (const auto& atomicNum : atomicNums)
          atomicSymbols.append(ElemInfo::getAtomicSymbol(atomicNum).c_str());
        qSort(atomicSymbols);

        // Now get the potcar info and store it
        for (const auto& atomicSymbol : atomicSymbols) {
          potcarTemplate +=
            "%filecontents:" + hash[atomicSymbol].toString() + "%\n";
        }
      }
    }

    QStringList optTemplateNames = this->optimizer(0)->getTemplateFileNames();

    QStringList queueTemplateNames =
      this->queueInterface(0)->getTemplateFileNames();

    for (int i = 0; i < optTemplateNames.size(); i++) {
      QStringList temp = settings
                           ->value("optimizer/" + optimizer.toUpper() + "/" +
                                     optTemplateNames.at(i) + "_list",
                                   "")
                           .toStringList();
      temp.removeAll("");
      if (!temp.empty()) {
        for (size_t j = 0; j < temp.size(); ++j) {
          while (j + 1 > getNumOptSteps())
            appendOptStep();

          setOptimizerTemplate(j, optTemplateNames[i].toStdString(),
                               temp[j].toStdString());
        }
        continue;
      }

      // If "temp" is empty, perhaps we have some template filenames to open
      QStringList templateFileNames =
        settings
          ->value("optimizer/" + optimizer.toUpper() + "/" +
                    optTemplateNames.at(i) + "_templates",
                  "")
          .toStringList();

      templateFileNames.removeAll("");
      // Loop through the files and see if they exist. If they do, store
      // the contents
      for (size_t j = 0; j < templateFileNames.size(); ++j) {
        while (j + 1 > getNumOptSteps())
          appendOptStep();

        const QString& templateFile = templateFileNames[j];

        QFile file(templateFile);
        if (!file.exists()) {
          qWarning() << "Warning in " << __FUNCTION__ << ": " << templateFile
                     << "does not exist!";
          continue;
        }
        if (!file.open(QIODevice::ReadOnly)) {
          qWarning() << "Warning in " << __FUNCTION__ << ": " << templateFile
                     << "could not be opened!";
          continue;
        }
        setOptimizerTemplate(j, templateFile.toStdString(),
                             QString(file.readAll()).toStdString());
        file.close();
      }
    }

    // If we are using VASP, update the potcar info
    if (optimizer.toLower() == "vasp") {
      for (size_t i = 0; i < getNumOptSteps(); ++i)
        setOptimizerTemplate(i, "POTCAR", potcarTemplate.toStdString());
    }

    // Repeat for queue interfaces
    for (int i = 0; i < queueTemplateNames.size(); i++) {
      QStringList temp =
        settings
          ->value("optimizer/" + optimizer.toUpper() + "/QI/" + qi.toUpper() +
                    "/" + queueTemplateNames.at(i) + "_list",
                  "")
          .toStringList();
      temp.removeAll("");
      if (!temp.empty()) {
        for (size_t j = 0; j < temp.size(); ++j) {
          while (j + 1 > getNumOptSteps())
            appendOptStep();

          setQueueInterfaceTemplate(j, queueTemplateNames[i].toStdString(),
                                    temp[j].toStdString());
        }
        continue;
      }

      // If "temp" is empty, perhaps we have some template filenames to open
      QStringList templateFileNames =
        settings
          ->value("optimizer/" + optimizer.toUpper() + "/QI/" + qi.toUpper() +
                    "/" + queueTemplateNames.at(i) + "_templates",
                  "")
          .toStringList();

      templateFileNames.removeAll("");
      // Loop through the files and see if they exist. If they do, store
      // the contents
      for (size_t j = 0; j < templateFileNames.size(); ++j) {
        while (j + 1 > getNumOptSteps())
          appendOptStep();

        const QString& templateFile = templateFileNames[j];

        QFile file(templateFile);
        if (!file.exists()) {
          qWarning() << "Warning in " << __FUNCTION__ << ": " << templateFile
                     << "does not exist!";
          continue;
        }
        if (!file.open(QIODevice::ReadOnly)) {
          qWarning() << "Warning in " << __FUNCTION__ << ": " << templateFile
                     << "could not be opened!";
          continue;
        }
        setQueueInterfaceTemplate(j, templateFile.toStdString(),
                                  QString(file.readAll()).toStdString());
        file.close();
      }
    }

    // Finally, read the user values
    setUser1(settings->value("optimizer/" + optimizer.toUpper() + "/user1", "")
               .toString()
               .toStdString());
    setUser2(settings->value("optimizer/" + optimizer.toUpper() + "/user2", "")
               .toString()
               .toStdString());
    setUser3(settings->value("optimizer/" + optimizer.toUpper() + "/user3", "")
               .toString()
               .toStdString());
    setUser4(settings->value("optimizer/" + optimizer.toUpper() + "/user4", "")
               .toString()
               .toStdString());

  }
  // New load instructions
  else {
    size_t numOptSteps = settings->value("numOptSteps", "1").toUInt();

    // Let's make sure this is at least 1, or we may have some issues
    if (numOptSteps == 0)
      numOptSteps = 1;

    clearOptSteps();
    for (size_t i = 0; i < numOptSteps; ++i) {
      appendOptStep();

      QString queueInterface =
        settings->value("queueInterface/" + QString::number(i), "local")
          .toString()
          .toLower();

      setQueueInterface(i, queueInterface.toStdString());

      readQueueInterfaceTemplatesFromSettings(i, filename.toStdString());

      this->queueInterface(i)->readSettings(filename);

      QString optimizerName =
        settings->value("optimizer/" + QString::number(i), "gulp")
          .toString()
          .toLower();

      setOptimizer(i, optimizerName.toStdString());

      readOptimizerTemplatesFromSettings(i, filename.toStdString());

      this->optimizer(i)->readSettings(filename);
    }

    readUserValuesFromSettings(filename.toStdString());
  }

  settings->endGroup();
  return true;
}

bool XtalOpt::readSettings(const QString& filename)
{
  // Some sections, we only want to load if we are loading a state file
  bool isStateFile = filename.endsWith(".state") ||
                     filename.endsWith(".state.old");

  SETTINGS(filename);

  settings->beginGroup("xtalopt/init/");
  int loadedVersion = settings->value("version", 0).toInt();

  // Only scheme version 3 will work currently
  if (!filename.isEmpty() && loadedVersion > 3) {
    error("XtalOpt::readSettings(): Settings in file " + filename +
          " cannot be opened by this version of XtalOpt. Please "
          "visit https://xtalopt.github.io to obtain a "
          "newer version.");
    return false;
  }

  a_min = settings->value("limits/a/min", 3).toDouble();
  b_min = settings->value("limits/b/min", 3).toDouble();
  c_min = settings->value("limits/c/min", 3).toDouble();
  a_max = settings->value("limits/a/max", 10).toDouble();
  b_max = settings->value("limits/b/max", 10).toDouble();
  c_max = settings->value("limits/c/max", 10).toDouble();
  alpha_min = settings->value("limits/alpha/min", 60).toDouble();
  beta_min = settings->value("limits/beta/min", 60).toDouble();
  gamma_min = settings->value("limits/gamma/min", 60).toDouble();
  alpha_max = settings->value("limits/alpha/max", 120).toDouble();
  beta_max = settings->value("limits/beta/max", 120).toDouble();
  gamma_max = settings->value("limits/gamma/max", 120).toDouble();
  vol_min = settings->value("limits/volume/min", 1).toDouble();
  vol_max = settings->value("limits/volume/max", 100000).toDouble();
  vol_fixed = settings->value("limits/volume/fixed", 500).toDouble();
  scaleFactor = settings->value("limits/scaleFactor", 0.5).toDouble();
  minRadius = settings->value("limits/minRadius", 0.25).toDouble();
  using_fixed_volume = settings->value("using/fixedVolume", false).toBool();
  vol_scale_max = settings->value("limits/volume/scale_max", 0.0).toDouble();
  vol_scale_min = settings->value("limits/volume/scale_min", 0.0).toDouble();
  using_interatomicDistanceLimit =
    settings->value("using/interatomicDistanceLimit", false).toBool();
  using_customIAD = settings->value("using/customIAD").toBool();
  using_randSpg = settings->value("using/randSpg").toBool();
  using_checkStepOpt = settings->value("using/checkStepOpt").toBool();

  using_mitosis = settings->value("using/mitosis", false).toBool();
  using_subcellPrint = settings->value("using/subcellPrint", false).toBool();

  if (isStateFile) {
    divisions = settings->value("limits/divisions").toInt();
    ax = settings->value("limits/ax").toInt();
    bx = settings->value("limits/bx").toInt();
    cx = settings->value("limits/cx").toInt();
  }

  using_molUnit = settings->value("using/molUnit", false).toBool();

  if (isStateFile) {
    // Composition
    int size = settings->beginReadArray("composition");
    comp = QHash<uint, XtalCompositionStruct>();
    for (int i = 0; i < size; i++) {
      settings->setArrayIndex(i);
      uint atomicNum, quantity;
      XtalCompositionStruct entry;
      atomicNum = settings->value("atomicNumber").toUInt();
      if (atomicNum == 0)
        continue;
      quantity = settings->value("quantity").toUInt();
      entry.quantity = quantity;
      comp.insert(atomicNum, entry);
    }
    settings->endArray();
  }

  if (using_molUnit && isStateFile) {
    int size = settings->beginReadArray("compMolUnit");
    compMolUnit = QHash<QPair<int, int>, MolUnit>();
    for (int i = 0; i < size; i++) {
      settings->setArrayIndex(i);
      int centerNum, numCenters, neighborNum, numNeighbors;
      unsigned int geom;
      double dist;
      MolUnit entry;

      QString center = settings->value("center").toString();
      centerNum = ElemInfo::getAtomicNum(center.trimmed().toStdString());
      QString strNumCenters = settings->value("number_of_centers").toString();
      numCenters = strNumCenters.toInt();
      QString neighbor = settings->value("neighbor").toString();
      neighborNum = ElemInfo::getAtomicNum(neighbor.trimmed().toStdString());
      QString strNumNeighbors =
        settings->value("number_of_neighbors").toString();
      numNeighbors = strNumNeighbors.toInt();
      QString strGeom = settings->value("geometry").toString();
      setGeom(geom, strGeom);
      dist = settings->value("distance").toDouble();
      QString strDist = QString::number(dist, 'f', 3);
      entry.numCenters = numCenters;
      entry.numNeighbors = numNeighbors;
      entry.geom = geom;
      entry.dist = dist;

      compMolUnit.insert(qMakePair<int, int>(centerNum, neighborNum), entry);
    }
    settings->endArray();
  }

  // Custom IAD
  if (using_customIAD && isStateFile) {
    int size = settings->beginReadArray("customIAD");
    interComp = QHash<QPair<int, int>, IAD>();
    for (int i = 0; i < size; i++) {
      settings->setArrayIndex(i);
      int atomicNum1, atomicNum2;
      IAD entry;
      atomicNum1 = settings->value("atomicNumber1").toInt();
      atomicNum2 = settings->value("atomicNumber2").toInt();
      double minInteratomicDist =
        settings->value("minInteratomicDist").toDouble();
      entry.minIAD = minInteratomicDist;
      interComp[qMakePair<int, int>(atomicNum1, atomicNum2)] = entry;
    }
    settings->endArray();
  }

  // Formula Units List
  formulaUnitsList.clear();
  if (isStateFile) {
    int size = settings->beginReadArray("Formula_Units");
    for (int i = 0; i < size; i++) {
      settings->setArrayIndex(i);
      formulaUnitsList.append(settings->value("FU").toUInt());
    }
    settings->endArray();
  }

  if (formulaUnitsList.isEmpty())
    formulaUnitsList = { 1 };

  settings->endGroup();

  // We have a separate function for reading the edit settings because
  // the edit tab may need to call it
  if (!readEditSettings(filename))
    return false;

  // Optimization settings tab
  settings->beginGroup("xtalopt/opt/");

  // Initial generation
  numInitial = settings->value("opt/numInitial", 20).toInt();

  // Search parameters
  popSize = settings->value("opt/popSize", 20).toUInt();
  contStructs = settings->value("opt/contStructs", 10).toUInt();
  limitRunningJobs = settings->value("opt/limitRunningJobs", false).toBool();
  runningJobLimit = settings->value("opt/runningJobLimit", 1).toUInt();
  failLimit = settings->value("opt/failLimit", 2).toUInt();

  failAction = static_cast<OptBase::FailActions>(
    settings->value("opt/failAction", XtalOpt::FA_Randomize).toUInt());
  cutoff = settings->value("opt/cutoff", 100).toInt();
  using_mitotic_growth =
    settings->value("opt/using_mitotic_growth", false).toBool();
  using_FU_crossovers =
    settings->value("opt/using_FU_crossovers", false).toBool();
  FU_crossovers_generation =
    settings->value("opt/FU_crossovers_generation", 4).toUInt();
  using_one_pool = settings->value("opt/using_one_pool", false).toBool();
  chance_of_mitosis = settings->value("opt/chance_of_mitosis", 50).toUInt();

  // Duplicates
  tol_xcLength = settings->value("tol/xtalcomp/length", 0.1).toDouble();
  tol_xcAngle = settings->value("tol/xtalcomp/angle", 2.0).toDouble();
  tol_spg = settings->value("tol/spg", 0.01).toDouble();

  // Crossover
  p_cross = settings->value("opt/p_cross", 15).toUInt();
  cross_minimumContribution =
    settings->value("opt/cross_minimumContribution", 25).toUInt();

  // Stripple
  p_strip = settings->value("opt/p_strip", 50).toUInt();
  strip_strainStdev_min =
    settings->value("opt/strip_strainStdev_min", 0.5).toDouble();
  strip_strainStdev_max =
    settings->value("opt/strip_strainStdev_max", 0.5).toDouble();
  strip_amp_min = settings->value("opt/strip_amp_min", 0.5).toDouble();
  strip_amp_max = settings->value("opt/strip_amp_max", 1.0).toDouble();
  strip_per1 = settings->value("opt/strip_per1", 1).toUInt();
  strip_per2 = settings->value("opt/strip_per2", 1).toUInt();

  // Permustrain
  p_perm = settings->value("opt/p_perm", 35).toUInt();
  perm_strainStdev_max =
    settings->value("opt/perm_strainStdev_max", 0.5).toDouble();
  perm_ex = settings->value("opt/perm_ex", 4).toUInt();

  if (isStateFile)
  {
    // Multi-objective, aflow-hardness, softExit, localQueue, ...
    //
    // softExit shouldn't be read in resuming; it causes the code to quit immediately!
    // Instead; always set it to false in resuming for which settings are being read here.
    m_softExit = false;
    //
    m_localQueue = settings->value("opt/localQueue", false).toBool();
    m_calculateHardness = settings->value("obj/calculateHardness", false).toBool();
    m_hardnessFitnessWeight = settings->value("obj/hardnessFitnessWeight", -1.0).toDouble();
    m_objectivesReDo = settings->value("obj/objectivesReDo", false).toBool();
    resetObjectives();
    int objnum = settings->value("obj/objectivesNum", 0).toInt();
    m_calculateObjectives = (objnum > 0) ? true : false;
    setObjectivesNum(objnum);
    int size = settings->beginReadArray("obj/objectives");
    for (size_t i = 0; i < size; i++) {
      settings->setArrayIndex(i);
      setObjectivesTyp(ObjectivesType(settings->value("typ","").toInt()));
      setObjectivesExe(settings->value("exe","").toString());
      setObjectivesWgt(settings->value("wgt",0.0).toDouble());
      setObjectivesOut(settings->value("out","").toString());
    }
    settings->endArray();
    // Double-check weights (and re-construct objectives list for gui)
    if (!processObjectivesWeights())
      return false;
  }

  settings->endGroup();

  return true;
}

bool XtalOpt::addSeed(const QString& filename)
{
  QString err;
  Xtal* xtal = new Xtal;
  xtal->setLocpath(filename);
  xtal->setStatus(Xtal::WaitingForOptimization);

  // We will only display the warning once, so use a static bool for this
  // Use an atomic bool for thread safety
  static std::atomic_bool warningAlreadyDisplayed(false);
  if (!warningAlreadyDisplayed.load()) {
    warning("XtalOpt no longer check to make sure seed "
            "xtals obey user-defined constraints (minimum "
            "interatomic distances, min/max volume, etc.). Be sure "
            "your seed structures are reasonable.");
    warningAlreadyDisplayed = true;
  }

  xtal->moveToThread(m_queue->thread());
  if (!optimizer(0)->read(xtal, filename) ||
      !this->checkComposition(xtal, &err)) {
    error(tr("Error loading seed %1\n\n%2").arg(filename).arg(err));
    xtal->deleteLater();
    return false;
  }
  QString parents = tr("Seeded: %1", "1 is a filename").arg(filename);
  this->m_queue->addManualStructureRequest(1);
  initializeAndAddXtal(xtal, 1, parents);
  debug(
    tr("XtalOpt::addSeed: Loaded seed: %1", "1 is a filename").arg(filename));
  return true;
}

Structure* XtalOpt::replaceWithRandom(Structure* s, const QString& reason)
{

  Xtal* oldXtal = qobject_cast<Xtal*>(s);
  QWriteLocker locker1(&oldXtal->lock());

  // Randomly generated xtals do not have parent structures
  oldXtal->setParentStructure(nullptr);

  uint FU = s->getFormulaUnits();

  uint generation, id;
  generation = s->getGeneration();
  id = s->getIDNumber();
  // Generate/Check new xtal
  Xtal* xtal = 0;
  uint spg = 0;
  int maxAttempts = 10000;
  int attemptCount = 0;
  while (!checkXtal(xtal)) {
    if (xtal) {
      delete xtal;
      xtal = 0;
    }
    if (attemptCount >= maxAttempts) {
      qDebug() << "Failed too many times in replaceWithRandom. Giving up";
      return nullptr;
    }
    ++attemptCount;

    if (using_randSpg) {
      do {
        // Randomly select a possible spg
        spg = pickRandomSpgFromPossibleOnes();
      }
      while (!RandSpg::isSpgPossible(spg, getStdVecOfAtoms(FU)));

      xtal = randSpgXtal(generation, id, FU, spg);
    } else {
      xtal = generateRandomXtal(generation, id, FU);
    }
  }

  // Copy info over
  QWriteLocker locker2(&xtal->lock());
  oldXtal->clear();
  oldXtal->setCellInfo(xtal->unitCell().cellMatrix());
  oldXtal->resetEnergy();
  oldXtal->resetEnthalpy();
  oldXtal->setPV(0);
  oldXtal->setCurrentOptStep(0);
  QString parents;
  if (using_randSpg) {
    QString HM_spg = Xtal::getHMName(spg);
    parents = tr("RandSpg Init: %1 (%2)").arg(spg).arg(HM_spg);
  }
  else {
    parents = "Randomly generated";
  }
  if (!reason.isEmpty())
    parents += " (" + reason + ")";
  oldXtal->setParents(parents);

  for (uint i = 0; i < xtal->numAtoms(); i++) {
    Atom& atom1 = oldXtal->addAtom();
    Atom& atom2 = xtal->atom(i);
    atom1.setPos(atom2.pos());
    atom1.setAtomicNumber(atom2.atomicNumber());
  }
  oldXtal->findSpaceGroup(tol_spg);
  oldXtal->resetFailCount();

  // Delete random xtal
  xtal->deleteLater();
  return qobject_cast<Structure*>(oldXtal);
}

Structure* XtalOpt::replaceWithOffspring(Structure* s, const QString& reason)
{
  Xtal* oldXtal = qobject_cast<Xtal*>(s);

  uint FU = s->getFormulaUnits();
  // Generate/Check new xtal
  Xtal* xtal = 0;
  int maxAttempts = 10000;
  int attemptCount = 0;
  while (!checkXtal(xtal)) {
    if (xtal) {
      xtal->deleteLater();
      xtal = nullptr;
    }
    if (attemptCount >= maxAttempts) {
      qDebug() << "Failed too many times in replaceWithOffspring. Giving up";
      return nullptr;
    }
    ++attemptCount;
    xtal = generateNewXtal(FU);
  }

  // Just return xtal if the formula units are not equivalent.
  // This should theoretically not occur since generateNewXtal(FU) forces
  // xtal to have the correct FU.
  if (xtal->getFormulaUnits() != s->getFormulaUnits()) {
    return xtal;
  }

  // Copy info over
  QWriteLocker locker1(&oldXtal->lock());
  QWriteLocker locker2(&xtal->lock());
  oldXtal->setCellInfo(xtal->unitCell().cellMatrix());
  oldXtal->resetEnergy();
  oldXtal->resetEnthalpy();
  oldXtal->resetFailCount();
  oldXtal->setPV(0);
  oldXtal->setCurrentOptStep(0);
  if (!reason.isEmpty()) {
    QString parents = xtal->getParents();
    parents += " (" + reason + ")";
    oldXtal->setParents(parents);
  }

  oldXtal->setParentStructure(xtal->getParentStructure());

  Q_ASSERT_X(xtal->numAtoms() == oldXtal->numAtoms(), Q_FUNC_INFO,
             "Number of atoms don't match. Cannot copy.");

  for (uint i = 0; i < xtal->numAtoms(); ++i) {
    oldXtal->atom(i) = xtal->atom(i);
  }
  oldXtal->findSpaceGroup(tol_spg);

  // Delete random xtal
  xtal->deleteLater();
  return static_cast<Structure*>(oldXtal);
}

Xtal* XtalOpt::randSpgXtal(uint generation, uint id, uint FU, uint spg,
                           bool checkSpgWithSpglib)
{
  Xtal* xtal = nullptr;

  // Let's make the spg input
  latticeStruct latticeMins, latticeMaxes;
  setLatticeMinsAndMaxes(latticeMins, latticeMaxes);

  // Create the input
  randSpgInput input(spg, getStdVecOfAtoms(FU), latticeMins, latticeMaxes);

  // Add various other input options
  input.IADScalingFactor = scaleFactor;
  input.minRadius = minRadius;
  input.minVolume = vol_min * static_cast<double>(FU);
  input.maxVolume = vol_max * static_cast<double>(FU);

  input.maxAttempts = 10;
  input.verbosity = 'n';
  // This removes the guarantee that we will generate the right space group,
  // but we will just check it with spglib
  input.forceMostGeneralWyckPos = false;

  // Let's try this 3 times
  size_t numAttempts = 0;
  do {
    numAttempts++;
    xtal = RandSpgXtalOptWrapper::randSpgXtal(input);
    // So that we don't crash the program, make sure the xtal exists
    // before attempting to get its spacegroup number
    if (xtal) {
      xtal->findSpaceGroup(tol_spg);
      // If we succeed, we're done!
      if (!checkSpgWithSpglib || xtal->getSpaceGroupNumber() == spg)
        break;
    }
  } while (numAttempts < 3);

  // Make sure we don't call xtal->getSpaceGroupNumber() until we know that
  // we have an xtal
  if (xtal) {
    if (checkSpgWithSpglib && xtal->getSpaceGroupNumber() != spg) {
      delete xtal;
      xtal = 0;
    }
  }

  // We need to set these things before checkXtal() is called
  if (xtal) {
    xtal->setStatus(Xtal::WaitingForOptimization);
    if (using_fixed_volume)
      xtal->setVolume(vol_fixed * FU);
  } else {
    qDebug() << "After" << QString::number(input.maxAttempts)
             << "attempts, failed to generate an xtal with spg of"
             << QString::number(spg);
    return nullptr;
  }

  QString HM_spg = Xtal::getHMName(spg);

  // Set up xtal data
  xtal->setGeneration(generation);
  xtal->setIDNumber(id);
  xtal->setParents(tr("RandSpg Init: %1 (%2)").arg(spg).arg(HM_spg));
  return xtal;
}

Xtal* XtalOpt::generateEmptyXtalWithLattice(uint FU)
{
  Xtal* xtal = nullptr;
  int maxAttempts = 10000;
  int attemptCount = 0;
  do {
    if (attemptCount >= maxAttempts) {
      qDebug() << "Failed too many times in generateEmptyXtalWithLattice. Giving up";
      return nullptr;
    }
    ++attemptCount;
    delete xtal;
    xtal = nullptr;
    double a = getRandDouble() * (a_max - a_min) + a_min;
    double b = getRandDouble() * (b_max - b_min) + b_min;
    double c = getRandDouble() * (c_max - c_min) + c_min;
    double alpha = getRandDouble() * (alpha_max - alpha_min) + alpha_min;
    double beta = getRandDouble() * (beta_max - beta_min) + beta_min;
    double gamma = getRandDouble() * (gamma_max - gamma_min) + gamma_min;
    xtal = new Xtal(a, b, c, alpha, beta, gamma);
  } while (!checkLattice(xtal, FU));

  if (using_fixed_volume)
    xtal->setVolume(vol_fixed * FU);

  return xtal;
}

Xtal* XtalOpt::generateRandomXtal(uint generation, uint id, uint FU)
{
  // Create a valid lattice first
  Xtal* xtal = generateEmptyXtalWithLattice(FU);

  // Cache these for later use
  double a = xtal->getA();
  double b = xtal->getB();
  double c = xtal->getC();

  QWriteLocker locker(&xtal->lock());

  xtal->setStatus(Xtal::Empty);

  // Populate crystal
  QList<uint> atomicNums = comp.keys();
  // Sort atomic number by decreasing minimum radius. Adding the "larger"
  // atoms first encourages a more even (and ordered) distribution
  for (int i = 0; i < atomicNums.size() - 1; ++i) {
    for (int j = i + 1; j < atomicNums.size(); ++j) {
      if (this->comp.value(atomicNums[i]).minRadius <
          this->comp.value(atomicNums[j]).minRadius) {
        atomicNums.swap(i, j);
      }
    }
  }

  unsigned int atomicNum;
  int qRand;
  int qTotal;
  int qRandPre;
  int qRandPost;

  // Mitosis = True
  if (using_mitosis) {
    //  Unit Cell Vectors
    int A = ax;
    int B = bx;
    int C = cx;

    a = a / A;
    b = b / B;
    c = c / C;

    xtal->setCellInfo(a, b, c, xtal->getAlpha(), xtal->getBeta(),
                      xtal->getGamma());

    // First check for "no center" MolUnits
    for (QHash<QPair<int, int>, MolUnit>::const_iterator
           it = this->compMolUnit.constBegin(),
           it_end = this->compMolUnit.constEnd();
         it != it_end; it++) {
      QPair<int, int> key = const_cast<QPair<int, int>&>(it.key());
      if (key.first == 0) {
        for (int i = 0; i < it->numCenters / divisions; i++) {
          if (!xtal->addAtomRandomly(key.first, key.second, this->comp,
                                     this->compMolUnit, true)) {
            xtal->deleteLater();
            debug("XtalOpt::generateRandomXtal: Failed to add atoms with "
                  "specified interatomic distance.");
            return 0;
          }
        }
      }
    }

    for (int num_idx = 0; num_idx < atomicNums.size(); num_idx++) {
      atomicNum = atomicNums.at(num_idx);
      if (atomicNum == 0)
        continue;
      qTotal = comp.value(atomicNum).quantity * FU;

      // Do we use the MolUnit builder?
      bool addAtom = true;
      bool useMolUnit = false;
      for (QHash<QPair<int, int>, MolUnit>::const_iterator
             it = this->compMolUnit.constBegin(),
             it_end = this->compMolUnit.constEnd();
           it != it_end; it++) {
        QPair<int, int> key = const_cast<QPair<int, int>&>(it.key());
        int first = key.first;
        if (atomicNum == first) {
          useMolUnit = true;
          break;
        }
      }

      // Do we add Atom or has it already been placed by MolUnit builder
      unsigned int qCenter = 0;
      unsigned int qNeighbor = 0;
      for (QHash<QPair<int, int>, MolUnit>::const_iterator
             it = this->compMolUnit.constBegin(),
             it_end = this->compMolUnit.constEnd();
           it != it_end; it++) {
        QPair<int, int> key = const_cast<QPair<int, int>&>(it.key());
        if (atomicNum == key.first) {
          qCenter += it->numCenters;
        }
        if (atomicNum == key.second) {
          qNeighbor += it->numCenters * it->numNeighbors;
        }
      }

      if (qCenter / divisions == 0)
        useMolUnit = false;

      if (qTotal == qCenter + qNeighbor) {
        addAtom = false;
        qRandPre = 0;
      } else {
        qRandPre = (qTotal - (qCenter + qNeighbor)) / divisions;
      }

      // Initial atom placement
      for (uint i = 0; i < qRandPre; i++) {
        if (addAtom == true) {
          if (!xtal->addAtomRandomly(atomicNum, this->comp)) {
            xtal->deleteLater();
            debug("XtalOpt::generateRandomXtal: Failed to add atoms with "
                  "specified interatomic distance.");
            return 0;
          }
        }
      }

      // Add atom with MolUnit builder or Randomly
      if (useMolUnit == true) {
        for (QHash<QPair<int, int>, MolUnit>::const_iterator
               it = this->compMolUnit.constBegin(),
               it_end = this->compMolUnit.constEnd();
             it != it_end; it++) {
          QPair<int, int> key = const_cast<QPair<int, int>&>(it.key());
          if (atomicNum == key.first) {
            for (int i = 0; i < it->numCenters / divisions; i++) {
              if (!xtal->addAtomRandomly(atomicNum, key.second, this->comp,
                                         this->compMolUnit, useMolUnit)) {
                xtal->deleteLater();
                debug("XtalOpt::generateRandomXtal: Failed to add atoms with "
                      "specified interatomic distance.");
                return 0;
              }
            }
          }
        }
      }
    }

    // Print subcell if checked
    if (using_subcellPrint)
      printSubXtal(xtal, generation, id);

    // Fill supercell by copying subcell according to parameters
    if (!xtal->fillSuperCell(A, B, C, xtal)) {
      xtal->deleteLater();
      debug("XtalOpt::generateRandomXtal: Failed to add atoms.");
      return 0;
    }

    // Randomly place the left over atoms
    for (int num_idx = 0; num_idx < atomicNums.size(); num_idx++) {
      atomicNum = atomicNums.at(num_idx);
      qTotal = (comp.value(atomicNum).quantity * FU);

      // Do we use the MolUnit builder?
      bool addAtom = true;
      bool useMolUnit = false;
      for (QHash<QPair<int, int>, MolUnit>::const_iterator
             it = this->compMolUnit.constBegin(),
             it_end = this->compMolUnit.constEnd();
           it != it_end; it++) {
        QPair<int, int> key = const_cast<QPair<int, int>&>(it.key());
        int first = key.first;
        if (atomicNum == first) {
          useMolUnit = true;
          break;
        }
      }

      // Do we add Atom or has it already been placed by MolUnit builder
      unsigned int qCenter = 0;
      unsigned int qNeighbor = 0;
      for (QHash<QPair<int, int>, MolUnit>::const_iterator
             it = this->compMolUnit.constBegin(),
             it_end = this->compMolUnit.constEnd();
           it != it_end; it++) {
        QPair<int, int> key = const_cast<QPair<int, int>&>(it.key());
        if (atomicNum == key.first) {
          qCenter += it->numCenters;
        }
        if (atomicNum == key.second) {
          qNeighbor += it->numCenters * it->numNeighbors;
        }
      }

      if (qTotal == qCenter + qNeighbor) {
        addAtom = false;
        qRandPost = 0;
      } else {
        qRandPre = (qTotal - (qCenter + qNeighbor)) / divisions;
        qRandPost = qTotal - (qCenter + qNeighbor) - (qRandPre * divisions);
      }

      if (qCenter / divisions > 0)
        qCenter -= (qCenter / divisions) * divisions;

      // Initial atom placement
      for (uint i = 0; i < qRandPost; i++) {
        if (addAtom == true) {
          if (!xtal->addAtomRandomly(atomicNum, this->comp)) {
            xtal->deleteLater();
            debug("XtalOpt::generateRandomXtal: Failed to add atoms with "
                  "specified interatomic distance.");
            return 0;
          }
        }
      }

      // Add atom with MolUnit builder or Randomly
      if (useMolUnit == true) {
        for (QHash<QPair<int, int>, MolUnit>::const_iterator
               it = this->compMolUnit.constBegin(),
               it_end = this->compMolUnit.constEnd();
             it != it_end; it++) {
          QPair<int, int> key = const_cast<QPair<int, int>&>(it.key());
          if (atomicNum == key.first) {
            for (int i = 0; i < it->numCenters % divisions; i++) {
              if (!xtal->addAtomRandomly(atomicNum, key.second, this->comp,
                                         this->compMolUnit, useMolUnit)) {
                xtal->deleteLater();
                debug("XtalOpt::generateRandomXtal: Failed to add atoms with "
                      "specified interatomic distance.");
                return 0;
              }
            }
          }
        }
      }
    }

    // Mitosis = False
  } else {
    if (using_customIAD) {
      for (int num_idx = 0; num_idx < atomicNums.size(); num_idx++) {
        atomicNum = atomicNums.at(num_idx);
        int q = comp.value(atomicNum).quantity * FU;
        for (uint i = 0; i < q; i++) {
          if (!xtal->addAtomRandomlyIAD(atomicNum, this->comp, this->interComp,
                                        1000)) {
            xtal->deleteLater();
            debug("XtalOpt::generateRandomXtal: Failed to add atoms with "
                  "specified custom interatomic distance.");
            return 0;
          }
        }
      }
    } else {
      // First check for "no center" MolUnits
      for (QHash<QPair<int, int>, MolUnit>::const_iterator
             it = this->compMolUnit.constBegin(),
             it_end = this->compMolUnit.constEnd();
           it != it_end; it++) {
        QPair<int, int> key = const_cast<QPair<int, int>&>(it.key());
        if (key.first == 0) {
          for (int i = 0; i < it->numCenters; i++) {
            if (!xtal->addAtomRandomly(key.first, key.second, this->comp,
                                       this->compMolUnit, true)) {
              xtal->deleteLater();
              debug("XtalOpt::generateRandomXtal: Failed to add atoms with "
                    "specified interatomic distance.");
              return 0;
            }
          }
        }
      }

      for (int num_idx = 0; num_idx < atomicNums.size(); num_idx++) {
        // To avoid messing up the stoichiometry with the MolUnit builder
        atomicNum = atomicNums.at(num_idx);
        qRand = comp.value(atomicNum).quantity * FU;

        if (atomicNum == 0)
          continue;

        bool addAtom = true;
        bool useMolUnit = false;

        for (QHash<QPair<int, int>, MolUnit>::const_iterator
               it = this->compMolUnit.constBegin(),
               it_end = this->compMolUnit.constEnd();
             it != it_end; it++) {
          QPair<int, int> key = const_cast<QPair<int, int>&>(it.key());
          if (atomicNum == key.first) {
            useMolUnit = true;
            break;
          }
        }

        unsigned int qCenter = 0;
        unsigned int qNeighbor = 0;
        for (QHash<QPair<int, int>, MolUnit>::const_iterator
               it = this->compMolUnit.constBegin(),
               it_end = this->compMolUnit.constEnd();
             it != it_end; it++) {
          QPair<int, int> key = const_cast<QPair<int, int>&>(it.key());
          if (atomicNum == key.first) {
            qCenter += it->numCenters;
          }
          if (atomicNum == key.second) {
            qNeighbor += it->numCenters * it->numNeighbors;
          }
        }

        if (qRand == qCenter + qNeighbor) {
          addAtom = false;
          qRand -= qCenter + qNeighbor;
        } else {
          qRand -= qCenter + qNeighbor;
        }

        // Initial atom placement
        for (uint i = 0; i < qRand; i++) {
          if (addAtom == true) {
            if (!xtal->addAtomRandomly(atomicNum, this->comp)) {
              xtal->deleteLater();
              debug("XtalOpt::generateRandomXtal: Failed to add atoms with "
                    "specified interatomic distance.");
              return 0;
            }
          }
        }

        if (useMolUnit == true) {
          for (QHash<QPair<int, int>, MolUnit>::const_iterator
                 it = this->compMolUnit.constBegin(),
                 it_end = this->compMolUnit.constEnd();
               it != it_end; it++) {
            QPair<int, int> key = const_cast<QPair<int, int>&>(it.key());
            if (atomicNum == key.first) {
              for (int i = 0; i < it->numCenters; i++) {
                if (!xtal->addAtomRandomly(atomicNum, key.second, this->comp,
                                           this->compMolUnit, useMolUnit)) {
                  xtal->deleteLater();
                  debug("XtalOpt::generateRandomXtal: Failed to add atoms with "
                        "specified interatomic distance.");
                  return 0;
                }
              }
            }
          }
        }
      }
    }
  }

  // Set up geneology info
  xtal->setGeneration(generation);
  xtal->setIDNumber(id);
  xtal->setParents("Randomly generated");
  xtal->setStatus(Xtal::WaitingForOptimization);

  // Set up xtal data
  return xtal;
}

void XtalOpt::printSubXtal(Xtal* xtal, uint generation, uint id)
{
  QMutexLocker xtalInitMutexLocker(xtalInitMutex);

  QString id_s, gen_s, locpath_s;
  id_s.sprintf("%05d", id);
  gen_s.sprintf("%05d", generation);
  locpath_s = locWorkDir + "/subcells";
  QDir dir(locpath_s);
  if (!dir.exists()) {
    if (!dir.mkpath(locpath_s)) {
      error(tr("XtalOpt::initializeSubXtal: Cannot write to path: %1 "
               "(path creation failure)",
               "1 is a file path.")
              .arg(locpath_s));
    }
  }
  QFile loc_subcell;
  loc_subcell.setFileName(locpath_s + "/" + gen_s + "x" + id_s + ".cml");

  if (!loc_subcell.open(QIODevice::WriteOnly)) {
    error("XtalOpt::initializeSubXtal(): Error opening file " +
          loc_subcell.fileName() + " for writing...");
  }

  QTextStream out;
  out.setDevice(&loc_subcell);

  // Print the subcells as .cml files
  QStringList symbols = xtal->getSymbols();
  QList<unsigned int> atomCounts = xtal->getNumberOfAtomsAlpha();
  out << "<molecule>\n";
  out << "\t<crystal>\n";

  // Unit Cell Vectors
  out << QString(
           "\t\t<scalar title=\"a\" units=\"units:angstrom\">%1</scalar>\n")
           .arg(xtal->unitCell().aVector().x(), 12);
  out << QString(
           "\t\t<scalar title=\"b\" units=\"units:angstrom\">%1</scalar>\n")
           .arg(xtal->unitCell().bVector().y(), 12, 'f', 8);
  out << QString(
           "\t\t<scalar title=\"c\" units=\"units:angstrom\">%1</scalar>\n")
           .arg(xtal->unitCell().cVector().z(), 12, 'f', 8);

  // Unit Cell Angles
  out << QString(
           "\t\t<scalar title=\"alpha\" units=\"units:degree\">%1</scalar>\n")
           .arg(xtal->getAlpha(), 12, 'f', 8);
  out << QString(
           "\t\t<scalar title=\"beta\" units=\"units:degree\">%1</scalar>\n")
           .arg(xtal->getBeta(), 12, 'f', 8);
  out << QString(
           "\t\t<scalar title=\"gamma\" units=\"units:degree\">%1</scalar>\n")
           .arg(xtal->getGamma(), 12, 'f', 8);

  out << "\t</crystal>\n";
  out << "\t<atomArray>\n";

  int symbolCount = 0;
  int j = 1;
  // Coordinates of each atom (sorted alphabetically by symbol)
  QList<Vector3> coords = xtal->getAtomCoordsFrac();
  for (int i = 0; i < coords.size(); i++) {
    if (j > atomCounts[symbolCount]) {
      symbolCount++;
      j = 0;
    }
    j++;
    out << QString("\t\t<atom id=\"a%1\" elementType=\"%2\" xFract=\"%3\" "
                   "yFract=\"%4\" zFract=\"%5\"/>\n")
             .arg(i + 1)
             .arg(symbols[symbolCount])
             .arg(coords[i].x(), 12, 'f', 8)
             .arg(coords[i].y(), 12, 'f', 8)
             .arg(coords[i].z(), 12, 'f', 8);
  }

  out << "\t</atomArray>\n";
  out << "</molecule>\n";
  out << endl;
}

// Overloaded version of generateRandomXtal(uint generation, uint id, uint FU)
// without FU specified
Xtal* XtalOpt::generateRandomXtal(uint generation, uint id)
{
  QList<uint> tempFormulaUnitsList = formulaUnitsList;
  if (using_mitotic_growth && !using_one_pool) {
    // Remove formula units on the list for which there is a smaller multiple
    // that may be used to create a super cell.
    for (int i = 0; i < tempFormulaUnitsList.size(); i++) {
      for (int j = i + 1; j < tempFormulaUnitsList.size(); j++) {
        if (tempFormulaUnitsList.at(j) % tempFormulaUnitsList.at(i) == 0) {
          tempFormulaUnitsList.removeAt(j);
          j--;
        }
      }
    }
  }

  // We will assume modulo bias will be small since formula unit ranges are
  // typically small. Pick random formula units.
  uint randomListIndex = rand() % int(tempFormulaUnitsList.size());

  uint FU = tempFormulaUnitsList.at(randomListIndex);

  return generateRandomXtal(generation, id, FU);
}

void XtalOpt::initializeAndAddXtal(Xtal* xtal, uint generation,
                                   const QString& parents)
{
  QMutexLocker xtalInitMutexLocker(xtalInitMutex);
  QList<Structure*> allStructures = m_queue->lockForNaming();
  Structure* structure;
  uint id = 1;
  for (int j = 0; j < allStructures.size(); j++) {
    structure = allStructures.at(j);
    QReadLocker structureLocker(&structure->lock());
    if (structure->getGeneration() == generation &&
        structure->getIDNumber() >= id) {
      id = structure->getIDNumber() + 1;
    }
  }

  QWriteLocker xtalLocker(&xtal->lock());
  xtal->moveToThread(m_queueThread);
  xtal->setIDNumber(id);
  xtal->setGeneration(generation);
  xtal->setParents(parents);

  QString id_s, gen_s, locpath_s, rempath_s;
  id_s.sprintf("%05d", xtal->getIDNumber());
  gen_s.sprintf("%05d", xtal->getGeneration());
  locpath_s = locWorkDir + "/" + gen_s + "x" + id_s + "/";
  rempath_s = remWorkDir + "/" + gen_s + "x" + id_s + "/";
  QDir dir(locpath_s);
  if (!dir.exists()) {
    if (!dir.mkpath(locpath_s)) {
      error(tr("XtalOpt::initializeAndAddXtal: Cannot write to path: %1 "
               "(path creation failure)",
               "1 is a file path.")
              .arg(locpath_s));
    }
  }
  // xtal->moveToThread(m_tracker->thread());
  xtal->setupConnections();
  xtal->setLocpath(locpath_s);
  xtal->setRempath(rempath_s);
  xtal->setCurrentOptStep(0);
  // If none of the cell parameters are fixed, perform a normalization on
  // the lattice (currently a Niggli reduction)
  if (fabs(a_min - a_max) > 0.01 && fabs(b_min - b_max) > 0.01 &&
      fabs(c_min - c_max) > 0.01 && fabs(alpha_min - alpha_max) > 0.01 &&
      fabs(beta_min - beta_max) > 0.01 && fabs(gamma_min - gamma_max) > 0.01) {
    xtal->fixAngles();
  }
  xtal->findSpaceGroup(tol_spg);
  xtalLocker.unlock();
  m_queue->unlockForNaming(xtal);
}

void XtalOpt::generateNewStructure()
{
  // Generate in background thread:
  QtConcurrent::run(this, &XtalOpt::generateNewStructure_);
}

void XtalOpt::generateNewStructure_()
{
  Xtal* newXtal = generateNewXtal();
  initializeAndAddXtal(newXtal, newXtal->getGeneration(),
                       newXtal->getParents());
}

// Identical to the previous generateNewXtal() function except the number
// formula units to use have been specified
Xtal* XtalOpt::generateNewXtal(uint FU)
{
  QList<Structure*> structures;

  QReadLocker trackerLocker(m_tracker->rwLock());

  // If we are NOT using one pool. FU == 0 implies that we are using one pool
  if (!using_one_pool) {

    // We want to include supercell structures so that each individual formula
    // unit can build off of supercells in their own gene pool. One supercell
    // may be a duplicate of another supercell, though, and we don't want to
    // include duplicate supercells. This function fixes that for us.
    structures =
      m_queue->getAllOptimizedStructuresAndOneSupercellCopyForEachFormulaUnit();

    // Remove all structures that do not have formula units of FU
    for (size_t i = 0; i < structures.size(); i++) {
      if (structures.at(i)->getFormulaUnits() != FU) {
        structures.removeAt(i);
        i--;
      }
    }
  }

  // Just remove non-optimized structures if using_one_pool
  else if (using_one_pool) {
    structures = m_queue->getAllOptimizedStructures();
  }

  // Remove all structures that are not on the formula units list...
  for (size_t i = 0; i < structures.size(); i++) {
    if (!onTheFormulaUnitsList(structures.at(i)->getFormulaUnits())) {
      structures.removeAt(i);
      i--;
    }
  }

  // Check to see if there are enough optimized structure to perform
  // genetic operations
  if (structures.size() < 3) {
    Xtal* xtal = 0;

    // Check to see if a supercell should be formed by mitosis
    if (using_mitotic_growth && FU != 0) {
      QList<Structure*> tempStructures =
        m_queue
          ->getAllOptimizedStructuresAndOneSupercellCopyForEachFormulaUnit();
      QList<uint> numberOfEachFormulaUnit =
        Structure::countStructuresOfEachFormulaUnit(&tempStructures, maxFU());

      // The number of formula units to use to make the super cell must be a
      // multiple of the larger formula unit, and there must be as many at
      // least five optimized structures. If there aren't, then generate more.
      for (int i = FU - 1; 0 < i; i--) {
        if (FU % i == 0 && numberOfEachFormulaUnit.at(i) >= 5 &&
            onTheFormulaUnitsList(i) == true) {
          int maxAttempts = 10000;
          int attemptCount = 0;
          while (!checkXtal(xtal)) {
            if (xtal) {
              delete xtal;
              xtal = 0;
            }
            if (attemptCount >= maxAttempts) {
              qDebug() << "Failed too many times in generateNewXtal1. Giving up";
              return nullptr;
            }
            ++attemptCount;
            xtal = generateSuperCell(i, FU, nullptr, true);
          }
          return xtal;
        }

        // Generates more of a particular formula unit if everything checks
        // except that there aren't enough parent structures. Must be on the
        // formulaUnitsList.
        if (FU % i == 0 && numberOfEachFormulaUnit.at(i) < 5 &&
            onTheFormulaUnitsList(i) == true) {
          xtal = generateNewXtal(i);
          QString parents = xtal->getParents();
          parents =
            parents +
            tr(" for mitosis to make %1 FU xtals").arg(QString::number(FU));
          xtal->setParents(parents);
          return xtal;
        }
      }
    }

    // If a supercell cannot be formed or if using_mitotic_growth == false
    int maxAttempts = 10000;
    int attemptCount = 0;
    while (!checkXtal(xtal)) {
      if (xtal)
        xtal->deleteLater();
      if (attemptCount >= maxAttempts) {
        qDebug() << "Failed too many times in generateNewXtal2. Giving up";
        return nullptr;
      }
      ++attemptCount;
      if (!using_one_pool)
        xtal = generateRandomXtal(1, 0, FU);
      else if (using_one_pool)
        xtal = generateRandomXtal(1, 0);
    }
    xtal->setParents(xtal->getParents() + " (too few optimized structures "
                                          "to generate offspring)");
    return xtal;
  }

  Xtal* xtal = H_getMutatedXtal(structures, FU);
  return xtal;
}

// Overloaded function of generateNewXtal(uint FU)
Xtal* XtalOpt::generateNewXtal()
{
  // Check to see if there are any structures that need to be primitive
  // reduced or if there are supercells that needs to be generated. If
  // there are, then generate and return one.
  QReadLocker trackerLocker(m_tracker->rwLock());
  QList<Structure*> optimizedStructures = m_queue->getAllOptimizedStructures();
  if (supercellCheckLock.try_lock()) {
    for (size_t i = 0; i < optimizedStructures.size(); i++) {
      Xtal* testXtal = qobject_cast<Xtal*>(optimizedStructures.at(i));
      QReadLocker testXtalLocker(&testXtal->lock());
      // If the structure has been primitive checked, we don't need to check
      // it again. If it hasn't been duplicate checked, yet, let it be
      // duplicate checked first (so we don't check unnecessary structures).
      if (!testXtal->wasPrimitiveChecked() &&
          !testXtal->hasChangedSinceDupChecked()) {
        // If testXtal is found to not be primitive, make a new xtal that is
        // the primitive of testXtal.
        if (!testXtal->isPrimitive(tol_spg)) {
          testXtal->setPrimitiveChecked(true);
          Xtal* nxtal = generatePrimitiveXtal(testXtal);
          // This will continue the structure generation while simultaneously
          // unlocking supercellCheckLock in 0.1 second
          // This allows time for the structure to be finished before
          // checking to see if another supercell could be generated
          waitThenUnlockSupercellCheckLock();
          return nxtal;
        }
        testXtal->setPrimitiveChecked(true);
      }

      // Now let's check to see if a supercell should be generated from
      // the optimized structure
      if (!testXtal->wasSupercellGenerationChecked()) {
        // If the optimized structure's enthalpy is not the lowest enthalpy
        // of it's formula unit set, set the supercell generation to be
        // true and just continue to the next structure in the loop.
        // For some reason, even though the datatypes are all doubles, it
        // does not appear that we can do a direct comparison between the
        // lowestEnthalpyFUList and the structure's enthalpy. So, we will
        // just do a basic percent diff comparison instead. If the difference
        // is less than 0.001%, then we will assume they are the same
        uint FU = testXtal->getFormulaUnits();
        double percentDiff =
          fabs((testXtal->getEnthalpy() - lowestEnthalpyFUList.at(FU)) /
               lowestEnthalpyFUList.at(FU) * 100.00000);
        if (percentDiff > 0.001) {
          testXtal->setSupercellGenerationChecked(true);
          continue;
        }
        double enthalpyPerAtom1 =
          testXtal->getEnthalpy() / static_cast<double>(testXtal->numAtoms());
        uint numAtomsPerFU = testXtal->numAtoms() / testXtal->getFormulaUnits();
        for (size_t j = 1; j <= maxFU(); j++) {
          if (!onTheFormulaUnitsList(j))
            continue;
          // j represents a formula unit that is being checked.
          // If optimizedStructures.at(i) can create a supercell with formula
          // units of j and optimizedStructures.at(i)'s enthalpy/atom is
          // smaller than the smallest so-far discovered enthalpy/atom at that
          // FU, and if there is a difference greater than 3 meV/atom between
          // the two, build a supercell and add it to the gene pool
          double enthalpyPerAtom2 =
            (lowestEnthalpyFUList.at(j) / static_cast<double>(j)) /
            static_cast<double>(numAtomsPerFU);
          if (j != testXtal->getFormulaUnits() &&
              j % testXtal->getFormulaUnits() == 0 &&
              (enthalpyPerAtom1 < enthalpyPerAtom2 || enthalpyPerAtom2 == 0)) {
            // enthalpyDiff is in meV
            double enthalpyDiff =
              fabs(enthalpyPerAtom1 - enthalpyPerAtom2) * 1000.0000000;
            if (enthalpyDiff <= 3.000000)
              continue;
            else {
              // We may need to create more than one supercell from a given
              // xtal, so only update this if it is generating an xtal with
              // the maxFU
              if (j == maxFU())
                testXtal->setSupercellGenerationChecked(true);
              Xtal* nxtal = generateSuperCell(testXtal->getFormulaUnits(), j,
                                              testXtal, false);
              nxtal->setParents(tr("Supercell generated from %1x%2")
                                  .arg(testXtal->getGeneration())
                                  .arg(testXtal->getIDNumber()));
              // We only want to perform offspring tracking for mutated
              // offspring.
              nxtal->setParentStructure(nullptr);
              nxtal->setEnthalpy(testXtal->getEnthalpy() *
                                 nxtal->getFormulaUnits() /
                                 testXtal->getFormulaUnits());
              nxtal->setEnergy(testXtal->getEnergy() *
                               nxtal->getFormulaUnits() /
                               testXtal->getFormulaUnits());
              nxtal->setPrimitiveChecked(true);
              nxtal->setSkippedOptimization(true);
              nxtal->setStatus(Xtal::Optimized);
              // This will continue the structure generation while
              // simultaneously unlocking supercellCheckLock in 0.1 second
              // This allows time for the structure to be finished before
              // checking to see if another supercell could be generated
              waitThenUnlockSupercellCheckLock();
              return nxtal;
            }
          }
        }
        testXtal->setSupercellGenerationChecked(true);
      }
    }
    supercellCheckLock.unlock();
  }
  // Having finished doing primitive reduction and supercell generation
  // checks, we can now continue!

  // Inputing a formula unit of 0 implies using all formula units when
  // generating the probability list.
  if (using_one_pool) {
    return generateNewXtal(0);
  }

  // Get all structures to count numbers of each formula unit
  QList<Structure*> allStructures = m_queue->getAllStructures();

  // Count the number of structures of each formula unit
  QList<uint> numberOfEachFormulaUnit =
    Structure::countStructuresOfEachFormulaUnit(&allStructures, maxFU());

  // No need to keep the tracker locked now
  trackerLocker.unlock();

  // If there are not yet at least 5 of any one FU, make more of that FU
  // Will generate smaller FU's first
  for (uint i = minFU(); i <= maxFU(); ++i) {
    if ((numberOfEachFormulaUnit.at(i) < 5) && (onTheFormulaUnitsList(i))) {
      uint FU = i;
      return generateNewXtal(FU);
    }
  }

  // Find the formula unit with the smallest number of total structures.
  uint smallest = numberOfEachFormulaUnit.at(minFU());
  for (uint i = minFU(); i <= maxFU(); ++i) {
    if ((numberOfEachFormulaUnit.at(i) < smallest) &&
        (onTheFormulaUnitsList(i))) {
      smallest = numberOfEachFormulaUnit.at(i);
    }
  }

  // Pick the formula unit with the smallest number of optimized structures. If
  // there are two or more formula units that have the smallest number of
  // optimized structures, pick the smallest
  uint FU;
  for (uint i = minFU(); i <= maxFU(); i++) {
    if ((numberOfEachFormulaUnit.at(i) == smallest) &&
        (onTheFormulaUnitsList(i))) {
      FU = i;
      break;
    }
  }

  return generateNewXtal(FU);
}

// preselectedXtal is nullptr by default
// includeCrossover is true by default
// includeMitosis is also true by default
Xtal* XtalOpt::H_getMutatedXtal(QList<Structure*>& structures, int FU,
                                Xtal* preselectedXtal, bool includeCrossover,
                                bool includeMitosis, bool mitosisMutation)
{
  // Initialize loop vars
  double r;
  unsigned int gen;
  QString parents;
  Xtal *xtal = nullptr, *selectedXtal = nullptr;

  // Perform operation until xtal is valid:
  int maxAttempts = 10000;
  int attemptCount = 0;
  while (!checkXtal(xtal)) {
    // First delete any previous failed structure in xtal
    if (xtal) {
      xtal->deleteLater();
      xtal = 0;
    }

    if (attemptCount >= maxAttempts) {
      qDebug() << "Failed too many times in H_getMutatedXtal1. Giving up";
      return nullptr;
    }
    ++attemptCount;

    // If an xtal hasn't been preselected, select one
    if (!preselectedXtal)
      selectedXtal = selectXtalFromProbabilityList(structures, FU);
    else
      selectedXtal = preselectedXtal;

    // Decide operator:
    r = getRandDouble();

    // We will perform mitosis if:
    // 1: using_one_pool is enabled and
    // 2: the xtal selected can produce an FU on the list through mitosis and
    // 3: the probability succeeds
    if (includeMitosis && using_one_pool) {
      // Find candidate formula units to be created through mitosis
      QList<uint> possibleMitosisFU_index;
      for (int i = 0; i < formulaUnitsList.size(); i++) {
        if (formulaUnitsList.at(i) % selectedXtal->getFormulaUnits() == 0 &&
            formulaUnitsList.at(i) != selectedXtal->getFormulaUnits()) {
          possibleMitosisFU_index.append(i);
        }
      }

      // If no FU's may be created by mitosis, just continue
      if (!possibleMitosisFU_index.isEmpty()) {
        // If the probability fails, delete xtal and continue
        if (r <= chance_of_mitosis / 100.0) {
          // Select an index randomly from the possibleMitosisFU_index
          uint randomListIndex = rand() % int(possibleMitosisFU_index.size());
          uint selectedIndex = possibleMitosisFU_index.at(randomListIndex);
          // Use that selected index to choose the formula units
          uint formulaUnits = formulaUnitsList.at(selectedIndex);
          // Perform mitosis
          Xtal* nxtal = nullptr;
          maxAttempts = 10000;
          attemptCount = 0;
          while (!checkXtal(nxtal)) {
            if (nxtal) {
              delete nxtal;
              nxtal = 0;
            }

            if (attemptCount >= maxAttempts) {
              qDebug() << "Failed too many times in H_getMutatedXtal2. Giving up";
              return nullptr;
            }
            ++attemptCount;

            nxtal = generateSuperCell(selectedXtal->getFormulaUnits(),
                                      formulaUnits, selectedXtal, true);
          }
          return nxtal;
        }
      }
    }

    Operators op;
    // Include the crossover in the selection
    if (includeCrossover) {
      if (r < p_cross / 100.0)
        op = OP_Crossover;
      else if (r < (p_cross + p_strip) / 100.0)
        op = OP_Stripple;
      else
        op = OP_Permustrain;
    }
    // In some cases (like in generateSuperCell()), we may
    // not want to perform crossover. Instead, renormalize
    // the percentages for stripple and permustrain and choose one
    else {
      double temp_p_strip = 100.0 * p_strip / (100.0 - p_cross);
      if (r < temp_p_strip / 100.0)
        op = OP_Stripple;
      else
        op = OP_Permustrain;
    }

    // Try 1000 times to get a good structure from the selected
    // operation. If not possible, send a warning to the log and
    // start anew.
    attemptCount = 0;
    maxAttempts = 10000;
    while (attemptCount < 1000 && !checkXtal(xtal)) {
      if (xtal) {
        delete xtal;
        xtal = 0;
      }

      if (attemptCount >= maxAttempts) {
        qDebug() << "Failed too many times in H_getMutatedXtal3. Giving up";
        return nullptr;
      }
      ++attemptCount;

      // Operation specific set up:
      switch (op) {
        case OP_Crossover: {
          Xtal *xtal1 = 0, *xtal2 = 0;
          // Select structures
          double percent1;
          double percent2;

          // If FU crossovers have been enabled, generate a new breeding pool
          // that includes multiple different formula units then run the
          // alternative crossover function
          // Only perform FU crossover if there is more than one formula unit
          bool enoughStructures = true;
          if (using_FU_crossovers && formulaUnitsList.size() > 1) {
            // Get all optimized structures
            QList<Structure*> tempStructures =
              m_queue->getAllOptimizedStructures();

            // Trim all the structures that aren't of the allowed generation or
            // greater
            for (int i = 0; i < tempStructures.size(); i++) {
              if (tempStructures.at(i)->getGeneration() <
                  FU_crossovers_generation) {
                tempStructures.removeAt(i);
                i--;
              }
            }

            // Only continue if there are at least 3 of the allowed generation
            // or greater
            if (tempStructures.size() < 3)
              enoughStructures = false;
            if (enoughStructures) {

              xtal1 = selectXtalFromProbabilityList(tempStructures);
              xtal2 = selectXtalFromProbabilityList(tempStructures);

              // Perform operation
              xtal = XtalOptGenetic::FUcrossover(
                xtal1, xtal2, cross_minimumContribution, percent1, percent2,
                formulaUnitsList, this->comp);

              if (!xtal)
                continue;
            }
          }

          // Perform a regular crossover instead!
          if (!using_FU_crossovers || !enoughStructures ||
              formulaUnitsList.size() <= 1) {
            xtal1 = selectedXtal;
            xtal2 = selectXtalFromProbabilityList(structures,
                                                  xtal1->getFormulaUnits());

            // Perform operation
            xtal = XtalOptGenetic::crossover(
              xtal1, xtal2, cross_minimumContribution, percent1);
          }

          // Lock parents and get info from them
          xtal1->lock().lockForRead();
          xtal2->lock().lockForRead();
          uint gen1 = xtal1->getGeneration();
          uint gen2 = xtal2->getGeneration();
          uint id1 = xtal1->getIDNumber();
          uint id2 = xtal2->getIDNumber();
          xtal2->lock().unlock();
          xtal1->lock().unlock();

          // We will set the parent xtal of this xtal to be
          // the parent that contributed the most
          if (xtal) {
            if (percent1 >= 50.0)
              xtal->setParentStructure(xtal1);
            else
              xtal->setParentStructure(xtal2);
          }

          // Determine generation number
          gen = (gen1 >= gen2) ? gen1 + 1 : gen2 + 1;

          // A regular crossover was performed. So percent2 will
          // simply be 100.0 - percent1. This may not be the case
          // for an FU Crossover
          if (!using_FU_crossovers || !enoughStructures)
            percent2 = 100.0 - percent1;

          parents = tr("Crossover: %1x%2 (%3%) + %4x%5 (%6%)")
                      .arg(gen1)
                      .arg(id1)
                      .arg(percent1, 0, 'f', 0)
                      .arg(gen2)
                      .arg(id2)
                      .arg(percent2, 0, 'f', 0);
          // To identify that a formula unit crossover was performed...
          if (using_FU_crossovers && enoughStructures)
            parents.prepend("FU ");
          continue;
        }
        case OP_Stripple: {

          // Perform stripple
          double amplitude = 0, stdev = 0;
          xtal = XtalOptGenetic::stripple(selectedXtal, strip_strainStdev_min,
                                          strip_strainStdev_max, strip_amp_min,
                                          strip_amp_max, strip_per1, strip_per2,
                                          stdev, amplitude);

          // Lock parent and extract info
          selectedXtal->lock().lockForRead();
          uint gen1 = selectedXtal->getGeneration();
          uint id1 = selectedXtal->getIDNumber();

          // If it's a mitosis mutation, the parent xtal is already set
          if (!mitosisMutation && xtal)
            xtal->setParentStructure(selectedXtal);
          selectedXtal->lock().unlock();

          // Determine generation number
          gen = gen1 + 1;
          // A regular mutation is being performed
          if (!mitosisMutation) {
            parents = tr("Stripple: %1x%2 stdev=%3 amp=%4 waves=%5,%6")
                        .arg(gen1)
                        .arg(id1)
                        .arg(stdev, 0, 'f', 5)
                        .arg(amplitude, 0, 'f', 5)
                        .arg(strip_per1)
                        .arg(strip_per2);
          }
          // Modified version of setting the parents for mitosis mutation.
          else {
            parents = selectedXtal->getParents() +
                      tr(" followed by Stripple: stdev=%1 amp=%2 waves=%3,%4")
                        .arg(stdev, 0, 'f', 5)
                        .arg(amplitude, 0, 'f', 5)
                        .arg(strip_per1)
                        .arg(strip_per2);
          }
          continue;
        }
        case OP_Permustrain: {

          double stdev = 0;

          xtal = XtalOptGenetic::permustrain(selectedXtal, perm_strainStdev_max,
                                             perm_ex, stdev);

          // Lock parent and extract info
          selectedXtal->lock().lockForRead();
          uint gen1 = selectedXtal->getGeneration();
          uint id1 = selectedXtal->getIDNumber();

          // If it's a mitosis mutation, the parent xtal is already set
          if (!mitosisMutation && xtal)
            xtal->setParentStructure(selectedXtal);
          selectedXtal->lock().unlock();

          // Determine generation number
          gen = gen1 + 1;
          // Set the ancestry like normal...
          if (!mitosisMutation) {
            parents = tr("Permustrain: %1x%2 stdev=%3 exch=%4")
                        .arg(gen1)
                        .arg(id1)
                        .arg(stdev, 0, 'f', 5)
                        .arg(perm_ex);
          }
          // Modified settings if it is a mitosis mutation
          else {
            parents = selectedXtal->getParents() +
                      tr(" followed by Permustrain: stdev=%1 exch=%2")
                        .arg(stdev, 0, 'f', 5)
                        .arg(perm_ex);
          }
          continue;
        }
        default:
          warning("XtalOpt::generateSingleOffspring: Attempt to use an "
                  "invalid operator.");
      }
    }
    if (attemptCount >= 1000) {
      QString opStr;
      switch (op) {
        case OP_Crossover:
          opStr = "crossover";
          break;
        case OP_Stripple:
          opStr = "stripple";
          break;
        case OP_Permustrain:
          opStr = "permustrain";
          break;
        default:
          opStr = "(unknown)";
          break;
      }
      warning(tr("Unable to perform operation %1 after 1000 tries. "
                 "Reselecting operator...")
                .arg(opStr));
    }
  }

  xtal->setGeneration(gen);
  xtal->setParents(parents);
  Xtal* parentXtal;
  if (!mitosisMutation)
    parentXtal = qobject_cast<Xtal*>(xtal->getParentStructure());
  else {
    parentXtal = qobject_cast<Xtal*>(selectedXtal->getParentStructure());
    xtal->setParentStructure(parentXtal);
  }

  return xtal;
}

Xtal* XtalOpt::generatePrimitiveXtal(Xtal* xtal)
{
  Xtal* nxtal = new Xtal();
  // Copy cell over from xtal to nxtal
  QReadLocker xtalLocker(&xtal->lock());
  nxtal->setCellInfo(xtal->unitCell().cellMatrix());
  // Add the atoms in...
  for (const auto& atom : xtal->atoms())
    nxtal->addAtom(atom.atomicNumber(), atom.pos());

  // Reduce it to primitive...
  nxtal->reduceToPrimitive(tol_spg);
  uint gen = xtal->getGeneration() + 1;
  QString parents = tr("Primitive of %1x%2")
                      .arg(xtal->getGeneration())
                      .arg(xtal->getIDNumber());
  nxtal->setGeneration(gen);
  nxtal->setParents(parents);
  nxtal->setEnthalpy(xtal->getEnthalpy() * nxtal->getFormulaUnits() /
                     xtal->getFormulaUnits());
  nxtal->setEnergy(xtal->getEnergy() * nxtal->getFormulaUnits() /
                   xtal->getFormulaUnits());
  nxtal->setPrimitiveChecked(true);
  nxtal->setSkippedOptimization(true);

  nxtal->resetStrucObj();
  nxtal->setStrucObjState(xtal->getStrucObjState());
  nxtal->setStrucObjValuesVec(xtal->getStrucObjValuesVec());

  nxtal->setStatus(Xtal::Optimized);
  return nxtal;
}

// This always returns a dynamically allocated xtal
// Callers take ownership of the pointer
Xtal* XtalOpt::generateSuperCell(uint initialFU, uint finalFU, Xtal* parentXtal,
                                 bool mutate)
{
  // First perform a sanity check
  if (finalFU % initialFU != 0) {
    qDebug() << "Warning:" << __FUNCTION__ << "was called with an impossible"
             << "ratio of finalFU to initialFU! initialFU is" << initialFU
             << "and finalFU is" << finalFU << "\nReturning nullptr";
    return nullptr;
  }

  // This is the return xtal
  Xtal* xtal = new Xtal;

  // Lock the tracker so the parentXtal won't get erased while we are reading
  // from it
  QReadLocker trackerLocker(m_tracker->rwLock());
  if (!parentXtal) {
    QList<Structure*> structures =
      m_queue->getAllOptimizedStructuresAndOneSupercellCopyForEachFormulaUnit();
    parentXtal = selectXtalFromProbabilityList(structures, initialFU);
  }

  // Lock the parent xtal for reading
  QReadLocker parentXtalLocker(&parentXtal->lock());

  // Copy info over from parent to new xtal
  xtal->setCellInfo(parentXtal->unitCell().cellMatrix());
  const std::vector<Atom>& atoms = parentXtal->atoms();
  for (const auto& atom : atoms)
    xtal->addAtom(atom.atomicNumber(), atom.pos());

  uint gen = parentXtal->getGeneration();
  QString parents = tr("%1x%2 mitosis").arg(gen).arg(parentXtal->getIDNumber());
  xtal->setParentStructure(parentXtal);
  xtal->setParents(parents);
  xtal->setGeneration(gen + 1);

  xtal->resetStrucObj();
  xtal->setStrucObjState(parentXtal->getStrucObjState());
  xtal->setStrucObjValuesVec(parentXtal->getStrucObjValuesVec());

  parentXtalLocker.unlock();
  trackerLocker.unlock();

  // Done copying over parent xtal stuff

  // Keep performing the supercell generator until we are at the correct FU
  // Because of the check at the beginning of this function, we should always
  // end up at finalFU
  while (xtal->getFormulaUnits() != finalFU) {
    // Find the largest prime number multiple. We will expand
    // upon the shortest length with this number.
    uint numberOfDuplicates = finalFU / xtal->getFormulaUnits();
    for (int i = 2; i < numberOfDuplicates; ++i) {
      if (numberOfDuplicates % i == 0) {
        numberOfDuplicates = numberOfDuplicates / i;
        i = 2;
      }
    }

    // a, b, and c are the number of duplicates in the A, B, and C
    // directions, respectively.
    uint a = 1;
    uint b = 1;
    uint c = 1;

    // Find the shortest length. We will expand upon this length.
    double A = xtal->getA();
    double B = xtal->getB();
    double C = xtal->getC();

    if (A <= B && A <= C)
      a = numberOfDuplicates;
    else if (B <= A && B <= C)
      b = numberOfDuplicates;
    else if (C <= A && C <= B)
      c = numberOfDuplicates;

    // Extract the old vectors
    const Vector3& oldA = xtal->unitCell().aVector();
    const Vector3& oldB = xtal->unitCell().bVector();
    const Vector3& oldC = xtal->unitCell().cVector();

    const std::vector<Atom> oldAtoms = xtal->atoms();

    // Add the extra atoms in
    for (int ind_a = 0; ind_a < a; ++ind_a) {
      for (int ind_b = 0; ind_b < b; ++ind_b) {
        for (int ind_c = 0; ind_c < c; ++ind_c) {
          if (ind_a == 0 && ind_b == 0 && ind_c == 0)
            continue;

          Vector3 displacement = ind_a * oldA + ind_b * oldB + ind_c * oldC;
          for (const auto& atom : oldAtoms)
            xtal->addAtom(atom.atomicNumber(), atom.pos() + displacement);
        }
      }
    }

    // Scale cell
    xtal->setCellInfo(a * A, b * B, c * C, xtal->getAlpha(), xtal->getBeta(),
                      xtal->getGamma());
  }

  // If we are to do so, mutate the xtal
  if (mutate) {
    // If xtal is already selected, no structure list is needed for
    // parameter 1. So we will use an empty list.
    // Technically, parameter 2 is not needed either.
    // Parameter 3 is the selected xtal to mutate.
    // Parameter 4 is includeCrossover and parameter 5 is includeMitosis.
    // Parameter 6 is mitosisMutation (changes the way the parents are set)
    QList<Structure*> temp;
    Xtal* ret =
      H_getMutatedXtal(temp, xtal->getFormulaUnits(), xtal, false, false, true);

    // We don't need xtal anymore, so delete it
    delete xtal;
    xtal = ret;
  }

  return xtal;
}

Xtal* XtalOpt::selectXtalFromProbabilityList(QList<Structure*> structures,
                                             uint FU)
{
  // Remove all structures that have an FU that ISN'T on the list
  for (size_t i = 0; i < structures.size(); i++) {
    if (!onTheFormulaUnitsList(structures.at(i)->getFormulaUnits())) {
      structures.removeAt(i);
      i--;
    }
  }

  if (FU != 0) {
    // Remove all structures that do not have formula units of FU
    for (int i = 0; i < structures.size(); i++) {
      if (structures.at(i)->getFormulaUnits() != FU) {
        structures.removeAt(i);
        i--;
      }
    }
  }

  int sizeBeforeObjectivesPruning = structures.size();

  // Create auxiliary variables and double-check objectives for submitting to parents pool.
  // This check, basically, is not needed! In the final implementation those whose objectives
  //    are not successfully calculated or dismissed by marking them as Fail or Dismiss, are already removed.
  int                objnum = getObjectivesNum();
  QList<double>      objwgt = {};
  QList<ObjectivesType> objtyp = {};
  if (m_calculateObjectives)
  {
    // First, create list of wgt and out of objectives for later use in get_probability... function
    for (int i = 0; i < objnum; i++) {
      objwgt.push_back(getObjectivesWgt(i));
      objtyp.push_back(getObjectivesTyp(i));
    }

    for (size_t i = 0; i < structures.size(); i++) {
      if (structures[i]->getStrucObjState() != Structure::Os_Retain) {
        structures.removeAt(i);
        --i;
#ifdef MOES_DEBUG
qDebug().noquote() << QString("NOTE: structure %1 excluded from pool "
                              "with objectives state %2 ")
  .arg(structures[i]->getTag(),7).arg(structures[i]->getStrucObjState(),2);
#endif
      }
    }
#ifdef MOES_DEBUG
qDebug().noquote() << QString("NOTE: a total of %1 (from %2) structures "
                              "left for pool after objective analysis\n")
  .arg(structures.size(),5).arg(sizeBeforeObjectivesPruning,5);
#endif

    if (structures.size() == 1 && sizeBeforeObjectivesPruning > 1) {
      warning(tr("A nonzero objective weight is being used for the fitness "
            "function, but very few (%1 from %2) structures have their "
            "objectives calculated. This current probability selection will "
            "not be good.\n").arg(structures.size()).arg(sizeBeforeObjectivesPruning));
    }
  }

  int sizeBeforeHardnessPruning = structures.size();

  // Check aflow-hardness before submitting to parents pool
  if (m_calculateHardness) {
    // If we are using aflow-hardness, remove all structures with a hardness less than 0
    for (size_t i = 0; i < structures.size(); i++) {
      if (structures[i]->vickersHardness() < 0.0 && structures.size() > 1) {
        structures.removeAt(i);
        --i;
      }
    }
    if (structures.size() == 1 && sizeBeforeHardnessPruning > 1) {
      warning(tr("A nonzero hardness weight is being used for the fitness "
            "function, but very few (%1 from %2) structures have their "
            "hardnesses calculated. This current probability selection will "
            "not be good.\n").arg(structures.size()).arg(sizeBeforeHardnessPruning));
    }
  }

  QList<QPair<GlobalSearch::Structure*, double>> probs =
    getProbabilityList(structures, popSize, m_hardnessFitnessWeight, objnum, objwgt, objtyp);

  // Initialize loop vars
  Xtal* xtal = nullptr;

  // Pick a parent
  double r = getRandDouble();
  for (const auto& elem: probs) {
    if (r < elem.second) {
      xtal = qobject_cast<Xtal*>(elem.first);
      break;
    }
  }

#ifdef MOES_DEBUG
QString outs = QString("\nNOTE: Selected %1 ( r = %2 ) from structures with probs:\n"
                       "      structure : enthalpy (eV/FU) :    probs   : cumulative probs\n")
                       .arg(xtal->getTag(),7).arg(r,8,'f',6);
  double previousProbs = 0.0;
  for (const auto& elem: probs) {
    outs += QString("        %1 :     %2 : %3 : %4\n")
      .arg(elem.first->getTag(),7).arg(elem.first->getEnthalpyPerFU(),12,'f',6)
      .arg(elem.second - previousProbs,10,'f',6).arg(elem.second,10,'f',6);
    previousProbs = elem.second;
  }
qDebug().noquote() << outs;
#endif
  return xtal;
}

bool XtalOpt::checkLimits()
{
  if (a_min > a_max) {
    warning("XtalOptRand::checkLimits error: Illogical A limits.");
    return false;
  }
  if (b_min > b_max) {
    warning("XtalOptRand::checkLimits error: Illogical B limits.");
    return false;
  }
  if (c_min > c_max) {
    warning("XtalOptRand::checkLimits error: Illogical C limits.");
    return false;
  }
  if (alpha_min > alpha_max) {
    warning("XtalOptRand::checkLimits error: Illogical Alpha limits.");
    return false;
  }
  if (beta_min > beta_max) {
    warning("XtalOptRand::checkLimits error: Illogical Beta limits.");
    return false;
  }
  if (gamma_min > gamma_max) {
    warning("XtalOptRand::checkLimits error: Illogical Gamma limits.");
    return false;
  }

  // Check to make sure at least one formula unit can be made with
  // the specified lengths and volume constraints
  bool anyFails = false;
  for (int i = 0; i < formulaUnitsList.size(); i++) {
    if ((using_fixed_volume &&
         ((a_min * b_min * c_min) >
            (vol_fixed * static_cast<double>(formulaUnitsList.at(i))) ||
          (a_max * b_max * c_max) <
            (vol_fixed * static_cast<double>(formulaUnitsList.at(i))))) ||
        (!using_fixed_volume &&
         ((a_min * b_min * c_min) >
            (vol_max * static_cast<double>(formulaUnitsList.at(i))) ||
          (a_max * b_max * c_max) <
            (vol_min * static_cast<double>(formulaUnitsList.at(i))) ||
          vol_min > vol_max))) {
      warning(
        tr("XtalOptRand::checkLimits error: Illogical Volume limits for %1 FU. "
           "(Also check min/max volumes based on cell lengths)")
          .arg(QString::number(formulaUnitsList.at(i))));
      anyFails = true;
    }
  }

  if (anyFails == true)
    return false;

  return true;
}

bool XtalOpt::checkComposition(Xtal* xtal, QString* err)
{
  // Check composition
  QList<unsigned int> atomTypes = comp.keys();
  QList<unsigned int> atomCounts;
#if QT_VERSION >= 0x040700
  atomCounts.reserve(atomTypes.size());
#endif // QT_VERSION
  for (size_t i = 0; i < atomTypes.size(); ++i) {
    atomCounts.append(0);
  }

  // Count atoms of each type
  for (size_t i = 0; i < xtal->numAtoms(); ++i) {
    int typeIndex = atomTypes.indexOf(
      static_cast<unsigned int>(xtal->atom(i).atomicNumber()));
    // Type not found:
    if (typeIndex == -1) {
      qDebug() << "XtalOpt::checkXtal: Composition incorrect.";
      if (err != nullptr) {
        *err = "Bad composition.";
      }
      return false;
    }
    ++atomCounts[typeIndex];
  }

  // Check counts. Adjust for formula units.
  for (size_t i = 0; i < atomTypes.size(); ++i) {
    if (atomCounts[i] !=
        comp[atomTypes[i]].quantity * xtal->getFormulaUnits()) { // PSA
      qDebug() << "atomCounts for atomic num " << QString::number(atomTypes[i])
               << "is" << QString::number(atomCounts[i]) << ". It should be "
               << QString::number(comp[atomTypes[i]].quantity *
                                  xtal->getFormulaUnits())
               << "instead.";
      qDebug() << "FU is " << QString::number(xtal->getFormulaUnits())
               << " and comp[atomTypes[i]].quantity is "
               << QString::number(comp[atomTypes[i]].quantity);
      // Incorrect count:
      qDebug() << "XtalOpt::checkXtal: Composition incorrect.";
      if (err != nullptr) {
        *err = "Bad composition.";
      }
      return false;
    }
  }
  return true;
}

// Xtal should be write-locked before calling this function
bool XtalOpt::checkLattice(Xtal* xtal, uint formulaUnits, QString* err)
{
  // Adjust max and min constraints depending on the formula unit
  new_vol_max = static_cast<double>(formulaUnits) * vol_max;
  new_vol_min = static_cast<double>(formulaUnits) * vol_min;

  // Check volume
  if (using_fixed_volume) {
    xtal->setVolume(vol_fixed * static_cast<double>(formulaUnits));
  } else if (xtal->getVolume() < new_vol_min || // PSA
             xtal->getVolume() > new_vol_max) { // PSA
    // I don't want to initialize a random number generator here, so
    // just use the modulus of the current volume as a random float.
    double newvol =
      fabs(fmod(xtal->getVolume(), 1)) * (new_vol_max - new_vol_min) +
      new_vol_min;
    // If the user has set vol_min to 0, we can end up with a null
    // volume. Fix this here. This is just to keep things stable
    // numerically during the rescaling -- it's unlikely that other
    // cells with small, nonzero volumes will pass the other checks
    // so long as other limits are reasonable.
    if (fabs(newvol) < 1.0) {
      newvol = (new_vol_max - new_vol_min) * 0.5 + new_vol_min; // PSA;
    }
    // qDebug() << "XtalOpt::checkXtal: Rescaling volume from "
    //         << xtal->getVolume() << " to " << newvol;
    xtal->setVolume(newvol);
  }

  // Scale to any fixed parameters
  double a, b, c, alpha, beta, gamma;
  a = b = c = alpha = beta = gamma = 0;
  if (fabs(a_min - a_max) < 0.01)
    a = a_min;
  if (fabs(b_min - b_max) < 0.01)
    b = b_min;
  if (fabs(c_min - c_max) < 0.01)
    c = c_min;
  if (fabs(alpha_min - alpha_max) < 0.01)
    alpha = alpha_min;
  if (fabs(beta_min - beta_max) < 0.01)
    beta = beta_min;
  if (fabs(gamma_min - gamma_max) < 0.01)
    gamma = gamma_min;
  xtal->rescaleCell(a, b, c, alpha, beta, gamma);

  // Reject the structure if using VASP and the determinant of the
  // cell matrix is negative (otherwise VASP complains about a
  // "negative triple product")
  if (xtal->unitCell().cellMatrix().determinant() <= 0.0) {
    qDebug() << "Rejecting structure" << xtal->getTag()
             << ": determinant of unit cell is negative or zero.";
    if (err != nullptr) {
      *err = "Unit cell matrix cannot have a negative or zero determinant.";
    }
    return false;
  }

  // Before fixing angles, make sure that the current cell
  // parameters are realistic
  if (GS_IS_NAN_OR_INF(xtal->getA()) || fabs(xtal->getA()) < ZERO8 ||
      GS_IS_NAN_OR_INF(xtal->getB()) || fabs(xtal->getB()) < ZERO8 ||
      GS_IS_NAN_OR_INF(xtal->getC()) || fabs(xtal->getC()) < ZERO8 ||
      GS_IS_NAN_OR_INF(xtal->getAlpha()) || fabs(xtal->getAlpha()) < ZERO8 ||
      GS_IS_NAN_OR_INF(xtal->getBeta()) || fabs(xtal->getBeta()) < ZERO8 ||
      GS_IS_NAN_OR_INF(xtal->getGamma()) || fabs(xtal->getGamma()) < ZERO8) {
    qDebug() << "XtalOpt::checkXtal: A cell parameter is either 0, nan, or "
                "inf. Discarding.";
    if (err != nullptr) {
      *err = "A cell parameter is too small (<10^-8) or not a number.";
    }
    return false;
  }

  // If no cell parameters are fixed, normalize lattice
  if (fabs(a + b + c + alpha + beta + gamma) < ZERO8) {
    // If one length is 25x shorter than another, it can sometimes
    // cause the spglib to crash in this function
    // If one is 25x shorter than another, discard it
    double cutoff = 25.0;
    if (xtal->getA() * cutoff < xtal->getB() ||
        xtal->getA() * cutoff < xtal->getC() ||
        xtal->getB() * cutoff < xtal->getA() ||
        xtal->getB() * cutoff < xtal->getC() ||
        xtal->getC() * cutoff < xtal->getA() ||
        xtal->getC() * cutoff < xtal->getB()) {
      qDebug() << "Error: one of the lengths is more than 25x shorter"
               << "than another length.\nCrystals like these can sometimes"
               << "cause spglib to crash the program. Discarding the xtal:";
      xtal->printXtalInfo();
      return false;
    }
    // Check that the angles aren't 25x different than the others as well
    if (xtal->getAlpha() * cutoff < xtal->getBeta() ||
        xtal->getAlpha() * cutoff < xtal->getGamma() ||
        xtal->getBeta() * cutoff < xtal->getAlpha() ||
        xtal->getBeta() * cutoff < xtal->getGamma() ||
        xtal->getGamma() * cutoff < xtal->getAlpha() ||
        xtal->getGamma() * cutoff < xtal->getBeta()) {
      qDebug() << "Error: one of the angles is more than 25x smaller"
               << "than another angle.\nCrystals like these can sometimes"
               << "cause spglib to crash the program. Discarding the xtal:";
      xtal->printXtalInfo();
      return false;
    }

    xtal->fixAngles();
  }

  // Check lattice
  if ((!a && (xtal->getA() < a_min || xtal->getA() > a_max)) ||
      (!b && (xtal->getB() < b_min || xtal->getB() > b_max)) ||
      (!c && (xtal->getC() < c_min || xtal->getC() > c_max)) ||
      (!alpha &&
       (xtal->getAlpha() < alpha_min || xtal->getAlpha() > alpha_max)) ||
      (!beta && (xtal->getBeta() < beta_min || xtal->getBeta() > beta_max)) ||
      (!gamma &&
       (xtal->getGamma() < gamma_min || xtal->getGamma() > gamma_max))) {
    qDebug() << "Discarding structure -- Bad lattice:" << endl
             << "A:     " << a_min << " " << xtal->getA() << " " << a_max
             << endl
             << "B:     " << b_min << " " << xtal->getB() << " " << b_max
             << endl
             << "C:     " << c_min << " " << xtal->getC() << " " << c_max
             << endl
             << "Alpha: " << alpha_min << " " << xtal->getAlpha() << " "
             << alpha_max << endl
             << "Beta:  " << beta_min << " " << xtal->getBeta() << " "
             << beta_max << endl
             << "Gamma: " << gamma_min << " " << xtal->getGamma() << " "
             << gamma_max;
    if (err != nullptr) {
      *err = "The unit cell parameters do not fall within the specified "
             "limits.";
    }
    return false;
  }

  // We made it!
  return true;
}

bool XtalOpt::checkXtal(Xtal* xtal, QString* err)
{
  if (!xtal) {
    if (err != nullptr) {
      *err = "Xtal pointer is nullptr.";
    }
    return false;
  }

  // Lock xtal
  QWriteLocker locker(&xtal->lock());

  if (xtal->getStatus() == Xtal::Empty) {
    if (err != nullptr) {
      *err = "Xtal status is empty.";
    }
    return false;
  }

  if (!checkComposition(xtal, err))
    return false;

  if (!checkLattice(xtal, xtal->getFormulaUnits(), err))
    return false;

  // Sometimes, all the atom positions are set to 'nan' for an unknown reason
  // Make sure that the position of the first atom is not nan
  if (GS_IS_NAN_OR_INF(xtal->atoms().at(0).pos().x())) {
    qDebug() << "Discarding structure -- contains 'nan' atom positions";
    return false;
  }

  // Never accept the structure if two atoms are basically on top of one
  // another
  for (size_t i = 0; i < xtal->numAtoms(); ++i) {
    for (size_t j = i + 1; j < xtal->numAtoms(); ++j) {
      if (fuzzyCompare(xtal->atom(i).pos(), xtal->atom(j).pos())) {
        qDebug() << "Discarding structure -- two atoms are basically on "
                 << "top of one another. This can confuse some "
                 << "optimizers.";
        return false;
      }
    }
  }

  // Check interatomic distances
  if (using_interatomicDistanceLimit) {
    int atom1, atom2;
    double IAD;
    if (!xtal->checkInteratomicDistances(this->comp, &atom1, &atom2, &IAD)) {
      Atom& a1 = xtal->atom(atom1);
      Atom& a2 = xtal->atom(atom2);
      const double minIAD = this->comp.value(a1.atomicNumber()).minRadius +
                            this->comp.value(a2.atomicNumber()).minRadius;

      qDebug() << "Discarding structure -- Bad IAD (" << IAD << " < " << minIAD
               << ")";
      if (err != nullptr) {
        *err = "Two atoms are too close together.";
      }
      return false;
    }
  }

  if (using_customIAD) {
    int atom1, atom2;
    double IAD;
    if (!xtal->checkMinIAD(this->interComp, &atom1, &atom2, &IAD)) {
      Atom& a1 = xtal->atom(atom1);
      Atom& a2 = xtal->atom(atom2);
      const double minIAD =
        this->interComp
          .value(qMakePair<int, int>(a1.atomicNumber(), a2.atomicNumber()))
          .minIAD;
      xtal->setStatus(Xtal::Killed);
      qDebug() << "Discarding structure -- Bad IAD (" << IAD << " < " << minIAD
               << ")";
      if (err != NULL) {
        *err = "Two atoms are too close together (post-optimization).";
      }
      return false;
    }
  }

  // Xtal is OK!
  if (err != nullptr) {
    *err = "";
  }
  return true;
}

bool XtalOpt::checkIntramolecularIADs(const GlobalSearch::Molecule& mol,
                                      const minIADs& iads,
                                      bool ignoreBondedAtoms)

{
  for (size_t i = 0; i < mol.numAtoms(); ++i) {
    for (size_t j = i + 1; j < mol.numAtoms(); ++j) {
      if (ignoreBondedAtoms && mol.areBonded(i, j))
        continue;

      double iad = iads(mol.atom(i).atomicNumber(), mol.atom(j).atomicNumber());
      if (mol.distance(mol.atom(i).pos(), mol.atom(j).pos()) < iad)
        return false;
    }
  }
  return true;
}

// These two molecules under comparison should have the same unit cell
bool XtalOpt::checkIntermolecularIADs(const GlobalSearch::Molecule& mol1,
                                      const GlobalSearch::Molecule& mol2,
                                      const minIADs& iads)
{
  for (const auto& atom1 : mol1.atoms()) {
    for (const auto& atom2 : mol2.atoms()) {
      double iad = iads(atom1.atomicNumber(), atom2.atomicNumber());
      if (mol1.distance(atom1.pos(), atom2.pos()) < iad)
        return false;
    }
  }
  return true;
}

bool XtalOpt::checkStepOptimizedStructure(Structure* s, QString* err)
{

  Xtal* xtal = qobject_cast<Xtal*>(s);
  uint fixCount = xtal->getFixCount();

  if (xtal == NULL) {
    return true;
  }

  // Check post-opt
  if (using_checkStepOpt) {
    if (using_customIAD) {
      int atom1, atom2;
      double IAD;
      for (int i = 0; i < 100; ++i) {
        if (!xtal->checkMinIAD(this->interComp, &atom1, &atom2, &IAD)) {
          Atom& a1 = xtal->atom(atom1);
          Atom& a2 = xtal->atom(atom2);

          if (fixCount < 10) {
            int atomicNumber = a2.atomicNumber();
            Atom* atom = &a2;
            if (xtal->moveAtomRandomlyIAD(atomicNumber, this->comp,
                                          this->interComp, 1000, atom)) {
              continue;
            } else {
              const double minIAD =
                this->interComp
                  .value(qMakePair<int, int>(a1.atomicNumber(), atomicNumber))
                  .minIAD;
              s->setStatus(Xtal::Killed);

              qDebug() << "Custom IAD: Discarding structure"
                       << "-- Bad IAD (" << IAD << " < " << minIAD
                       << ") \n Could not fix the IAD issue.";
              return false;
            }
          } else {
            const double minIAD = this->interComp
                                    .value(qMakePair<int, int>(
                                      a1.atomicNumber(), a2.atomicNumber()))
                                    .minIAD;
            s->setStatus(Xtal::Killed);

            qDebug() << "Custom IAD: Discarding structure"
                     << "-- Bad IAD (" << IAD << " < " << minIAD
                     << ") \n Exceeded the number of fixes.";
            return false;
          }
        } else {
          if (i > 0) {
            s->setFixCount(fixCount + 1);
            s->setCurrentOptStep(0);
            break;
          } else {
            break;
          }
        }
      }
      return true;
    }

    if (using_interatomicDistanceLimit) {
      int atom1, atom2;
      double IAD;
      if (!xtal->checkInteratomicDistances(this->comp, &atom1, &atom2, &IAD)) {
        Atom& a1 = xtal->atom(atom1);
        Atom& a2 = xtal->atom(atom2);
        const double minIAD = this->comp.value(a1.atomicNumber()).minRadius +
                              this->comp.value(a2.atomicNumber()).minRadius;

        qDebug() << "Discarding structure -- Bad IAD (" << IAD << " < "
                 << minIAD << ")";
        if (err != NULL) {
          *err = "Two atoms are too close together.";
        }
        return false;
      }
      return true;
    }
  }

  // If early, don't check structure
  return true;
}

QString XtalOpt::interpretTemplate(const QString& templateString,
                                   Structure* structure)
{
  QStringList list = templateString.split("%");
  QString line;
  QString origLine;
  for (int line_ind = 0; line_ind < list.size(); line_ind++) {
    origLine = line = list.at(line_ind);
    interpretKeyword_base(line, structure);
    interpretKeyword(line, structure);
    if (line != origLine) { // Line was a keyword
      list.replace(line_ind, line);
    }
  }
  // Rejoin string
  QString ret = list.join("");
  ret += "\n";
  return ret;
}

void XtalOpt::interpretKeyword(QString& line, Structure* structure)
{
  QString rep = "";
  Xtal* xtal = qobject_cast<Xtal*>(structure);

  // Xtal specific keywords
  if (line == "a")
    rep += QString::number(xtal->getA());
  else if (line == "b")
    rep += QString::number(xtal->getB());
  else if (line == "c")
    rep += QString::number(xtal->getC());
  else if (line == "alphaRad")
    rep += QString::number(xtal->getAlpha() * DEG2RAD);
  else if (line == "betaRad")
    rep += QString::number(xtal->getBeta() * DEG2RAD);
  else if (line == "gammaRad")
    rep += QString::number(xtal->getGamma() * DEG2RAD);
  else if (line == "alphaDeg")
    rep += QString::number(xtal->getAlpha());
  else if (line == "betaDeg")
    rep += QString::number(xtal->getBeta());
  else if (line == "gammaDeg")
    rep += QString::number(xtal->getGamma());
  else if (line == "volume")
    rep += QString::number(xtal->getVolume());
  else if (line == "block")
    rep += QString("%block");
  else if (line == "endblock")
    rep += QString("%endblock");
  else if (line == "coordsFrac") {
    const std::vector<Atom>& atoms = structure->atoms();
    std::vector<Atom>::const_iterator it;
    for (it = atoms.begin(); it != atoms.end(); it++) {
      const Vector3 coords = xtal->cartToFrac((*it).pos());
      rep += (QString(ElemInfo::getAtomicSymbol((*it).atomicNumber()).c_str()) +
              " ");
      rep += QString::number(coords.x()) + " ";
      rep += QString::number(coords.y()) + " ";
      rep += QString::number(coords.z()) + "\n";
    }
  } else if (line == "chemicalSpeciesLabel") {
    QList<QString> symbols = xtal->getSymbols();
    for (int i = 0; i < symbols.size(); i++) {
      rep += " ";
      rep += QString::number(i + 1) + " ";
      rep +=
        QString::number(ElemInfo::getAtomicNum(symbols[i].toStdString())) + " ";
      rep += symbols[i] + "\n";
    }
  } else if (line == "atomicCoordsAndAtomicSpecies") {
    const std::vector<Atom>& atoms = xtal->atoms();
    std::vector<Atom>::const_iterator it;
    QList<QString> symbol = xtal->getSymbols();
    for (it = atoms.begin(); it != atoms.end(); it++) {
      const Vector3 coords = xtal->cartToFrac((*it).pos());
      QString currAtom =
        ElemInfo::getAtomicSymbol((*it).atomicNumber()).c_str();
      int i = symbol.indexOf(currAtom) + 1;
      rep += " ";
      QString inp;
      inp.sprintf("%4.8f", coords.x());
      rep += inp + "\t";
      inp.sprintf("%4.8f", coords.y());
      rep += inp + "\t";
      inp.sprintf("%4.8f", coords.z());
      rep += inp + "\t";
      rep += QString::number(i) + "\n";
    }
  } else if (line == "coordsFracId") {
    const std::vector<Atom>& atoms = structure->atoms();
    std::vector<Atom>::const_iterator it;
    for (it = atoms.begin(); it != atoms.end(); it++) {
      const Vector3 coords = xtal->cartToFrac((*it).pos());
      rep += (QString(ElemInfo::getAtomicSymbol((*it).atomicNumber()).c_str()) +
              " ");
      rep += QString::number((*it).atomicNumber()) + " ";
      rep += QString::number(coords.x()) + " ";
      rep += QString::number(coords.y()) + " ";
      rep += QString::number(coords.z()) + "\n";
    }
  } else if (line == "coordsFracIndex") {
    const std::vector<Atom>& atoms = structure->atoms();
    std::vector<Atom>::const_iterator it;
    int tag = 0;
    for (it = atoms.begin(); it != atoms.end(); it++) {
      const Vector3 coords = xtal->cartToFrac((*it).pos());
      rep += QString::number(tag++) + " ";
      rep += QString::number(coords.x()) + " ";
      rep += QString::number(coords.y()) + " ";
      rep += QString::number(coords.z()) + "\n";
    }
  } else if (line == "gulpFracShell") {
    const std::vector<Atom>& atoms = structure->atoms();
    std::vector<Atom>::const_iterator it;
    for (it = atoms.begin(); it != atoms.end(); it++) {
      const Vector3 coords = xtal->cartToFrac((*it).pos());
      const char* symbol =
        ElemInfo::getAtomicSymbol((*it).atomicNumber()).c_str();
      rep += QString("%1 core %2 %3 %4\n")
               .arg(symbol)
               .arg(coords.x())
               .arg(coords.y())
               .arg(coords.z());
      rep += QString("%1 shel %2 %3 %4\n")
               .arg(symbol)
               .arg(coords.x())
               .arg(coords.y())
               .arg(coords.z());
    }
  } else if (line == "cellMatrixAngstrom") {
    Matrix3 m = xtal->unitCell().cellMatrix();
    for (int i = 0; i < 3; i++) {
      rep += " ";
      for (int j = 0; j < 3; j++) {
        QString inp;
        inp.sprintf("%4.8f", m(i, j));
        rep += inp + "  ";
      }
      rep += "\n";
    }
  } else if (line == "cellVector1Angstrom") {
    Vector3 v = xtal->unitCell().aVector();
    for (int i = 0; i < 3; i++) {
      rep += QString::number(v[i]) + "\t";
    }
  } else if (line == "cellVector2Angstrom") {
    Vector3 v = xtal->unitCell().bVector();
    for (int i = 0; i < 3; i++) {
      rep += QString::number(v[i]) + "\t";
    }
  } else if (line == "cellVector3Angstrom") {
    Vector3 v = xtal->unitCell().cVector();
    for (int i = 0; i < 3; i++) {
      rep += QString::number(v[i]) + "\t";
    }
  } else if (line == "cellMatrixBohr") {
    Matrix3 m = xtal->unitCell().cellMatrix();
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        rep += QString::number(m(i, j) * ANG2BOHR) + "\t";
      }
      rep += "\n";
    }
  } else if (line == "cellVector1Bohr") {
    Vector3 v = xtal->unitCell().aVector();
    for (int i = 0; i < 3; i++) {
      rep += QString::number(v[i] * ANG2BOHR) + "\t";
    }
  } else if (line == "cellVector2Bohr") {
    Vector3 v = xtal->unitCell().bVector();
    for (int i = 0; i < 3; i++) {
      rep += QString::number(v[i] * ANG2BOHR) + "\t";
    }
  } else if (line == "cellVector3Bohr") {
    Vector3 v = xtal->unitCell().cVector();
    for (int i = 0; i < 3; i++) {
      rep += QString::number(v[i] * ANG2BOHR) + "\t";
    }
  } else if (line == "POSCAR") {
    rep += xtal->toPOSCAR();
  } // End %POSCAR%
  else if (line == "siestaZMatrix") {
    rep += xtal->toSiestaZMatrix().c_str();
  }

  if (!rep.isEmpty()) {
    // Remove any trailing newlines
    rep = rep.replace(QRegExp("\n$"), "");
    line = rep;
  }
}

QString XtalOpt::getTemplateKeywordHelp()
{
  QString help = "";
  help.append(getTemplateKeywordHelp_base());
  help.append("\n");
  help.append(getTemplateKeywordHelp_xtalopt());
  return help;
}

QString XtalOpt::getTemplateKeywordHelp_xtalopt()
{
  QString str;
  QTextStream out(&str);
  out << "Crystal specific information:\n"
      << "%POSCAR% -- VASP poscar generator\n"
      << "%coordsFrac% -- fractional coordinate data\n\t[symbol] [x] [y] [z]\n"
      << "%coordsFracIndex% -- fractional coordinate data with order index\n"
         "\t[index: 0..number of atoms] [x] [y] [z]\n"
      << "%coordsFracId% -- fractional coordinate data with atomic "
         "number\n\t[symbol] [atomic number] [x] [y] [z]\n"
      << "%gulpFracShell% -- fractional coordinates for use in GULP core/shell "
         "calculations:\n"
         "\tBoth of the following are printed for each atom:\n"
         "\t[symbol] core [x] [y] [z]\n"
         "\t[symbol] shell [x] [y] [z]\n"
      << "%cellMatrixAngstrom% -- Cell matrix in Angstrom\n"
      << "%cellVector1Angstrom% -- First cell vector in Angstrom\n"
      << "%cellVector2Angstrom% -- Second cell vector in Angstrom\n"
      << "%cellVector3Angstrom% -- Third cell vector in Angstrom\n"
      << "%cellMatrixBohr% -- Cell matrix in Bohr\n"
      << "%cellVector1Bohr% -- First cell vector in Bohr\n"
      << "%cellVector2Bohr% -- Second cell vector in Bohr\n"
      << "%cellVector3Bohr% -- Third cell vector in Bohr\n"
      << "%a% -- Lattice parameter A\n"
      << "%b% -- Lattice parameter B\n"
      << "%c% -- Lattice parameter C\n"
      << "%alphaRad% -- Lattice parameter Alpha in rad\n"
      << "%betaRad% -- Lattice parameter Beta in rad\n"
      << "%gammaRad% -- Lattice parameter Gamma in rad\n"
      << "%alphaDeg% -- Lattice parameter Alpha in degrees\n"
      << "%betaDeg% -- Lattice parameter Beta in degrees\n"
      << "%gammaDeg% -- Lattice parameter Gamma in degrees\n"
      << "%volume% -- Unit cell volume\n"
      << "%gen% -- xtal generation number\n"
      << "%id% -- xtal id number\n";

  return str;
}

std::unique_ptr<GlobalSearch::QueueInterface> XtalOpt::createQueueInterface(
  const std::string& queueName)
{

  if (caseInsensitiveCompare(queueName, "local"))
    return make_unique<LocalQueueInterface>(this);

#ifdef ENABLE_SSH
  if (caseInsensitiveCompare(queueName, "loadleveler"))
    return make_unique<LoadLevelerQueueInterface>(this);

  if (caseInsensitiveCompare(queueName, "lsf"))
    return make_unique<LsfQueueInterface>(this);

  if (caseInsensitiveCompare(queueName, "pbs"))
    return make_unique<PbsQueueInterface>(this);

  if (caseInsensitiveCompare(queueName, "sge"))
    return make_unique<SgeQueueInterface>(this);

  if (caseInsensitiveCompare(queueName, "slurm"))
    return make_unique<SlurmQueueInterface>(this);
#endif

  qDebug() << "Error in" << __FUNCTION__
           << ": Unknown queue interface:" << queueName.c_str();
  return nullptr;
}

std::unique_ptr<GlobalSearch::Optimizer> XtalOpt::createOptimizer(
  const std::string& optName)
{
  if (caseInsensitiveCompare(optName, "castep"))
    return make_unique<CASTEPOptimizer>(this);

  if (caseInsensitiveCompare(optName, "generic"))
    return make_unique<GenericOptimizer>(this);

  if (caseInsensitiveCompare(optName, "gulp"))
    return make_unique<GULPOptimizer>(this);

  if (caseInsensitiveCompare(optName, "pwscf"))
    return make_unique<PWscfOptimizer>(this);

  if (caseInsensitiveCompare(optName, "siesta"))
    return make_unique<SIESTAOptimizer>(this);

  if (caseInsensitiveCompare(optName, "vasp"))
    return make_unique<VASPOptimizer>(this);

  qDebug() << "Error in" << __FUNCTION__
           << ": Unknown optimizer:" << optName.c_str();
  return nullptr;
}

bool XtalOpt::load(const QString& filename, const bool forceReadOnly)
{
  if (forceReadOnly) {
    readOnly = true;
  } else
    readOnly = false;
  loaded = true;

  // Attempt to open state file
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly)) {
    error("XtalOpt::load(): Error opening file " + file.fileName() +
          " for reading...");
    return false;
  }

  SETTINGS(filename);
  int loadedVersion = settings->value("xtalopt/version", 0).toInt();

  // Update config data. Be sure to bump m_schemaVersion in ctor if
  // adding updates.
  switch (loadedVersion) {
    case 0:
    case 1:
    case 2:
    case 3:
      break;
    default:
      error("XtalOpt::load(): Settings in file " + file.fileName() +
            " cannot be opened by this version of XtalOpt. Please "
            "visit https://xtalopt.github.io to obtain a "
            "newer version.");
      return false;
  }

  bool stateFileIsValid =
    settings->value("xtalopt/saveSuccessful", false).toBool();
  if (!stateFileIsValid && !file.fileName().endsWith(".old")) {
    bool readFromOldState;
    needBoolean(tr("XtalOpt::load(): File:\n\n'%1'\n\nis incomplete, corrupt, "
                   "or invalid. Would you like to try loading:\n\n'%1"
                   ".old'\n\ninstead?").arg(file.fileName()),
                &readFromOldState);

    if (!readFromOldState)
      return false;

    return load(file.fileName() + ".old", false);

  } else if (!stateFileIsValid && file.fileName().endsWith(".old")) {
    error("XtalOpt::load(): File " + file.fileName() +
          " is incomplete, corrupt, or invalid. Cannot begin run. Please "
          "check your state file.");
    readOnly = true;
    return false;
  }

  // Get path and other info for later:
  QFileInfo stateInfo(file);
  // path to resume file
  QDir dataDir = stateInfo.absoluteDir();
  QString dataPath = dataDir.absolutePath() + "/";

#ifdef XTALOPT_DEBUG
  // Setup the message handler for the run log file
  saveLogFileOfRun(dataPath);
#endif

  // list of xtal dirs
  QStringList xtalDirs =
    dataDir.entryList(QStringList(), QDir::AllDirs, QDir::Size);
  xtalDirs.removeAll(".");
  xtalDirs.removeAll("..");

  for (int i = 0; i < xtalDirs.size(); i++) {
    // old versions of xtalopt used xtal.state, so still check for it.
    if (!QFile::exists(dataPath + "/" + xtalDirs.at(i) + "/structure.state") &&
        !QFile::exists(dataPath + "/" + xtalDirs.at(i) + "/xtal.state")) {
      xtalDirs.removeAt(i);
      i--;
    }
  }

  // Set locWorkDir:
  QString newFilePath = dataPath;
  QString newFileBase = filename;
  newFileBase.remove(newFilePath);
  newFileBase.remove("xtalopt.state.old");
  newFileBase.remove("xtalopt.state.tmp");
  newFileBase.remove("xtalopt.state");

  readSettings(filename);

  // If we have a dialog, read the settings into the dialog as well
  if (m_dialog)
    m_dialog->readSettings(filename);

#ifdef ENABLE_SSH
  // Create the SSHManager if running remotely
  if (anyRemoteQueueInterfaces()) {
    qDebug() << "Creating SSH connections...";
    if (!this->createSSHConnections()) {
      error(tr("Could not create ssh connections."));
      return false;
    }
  }
#endif // ENABLE_SSH

  debug(tr("Resuming XtalOpt session in '%1' readOnly = %2")
          .arg(filename)
          .arg((readOnly) ? "true" : "false"));

  // Xtals
  // Initialize progress bar:
  if (m_dialog)
    m_dialog->updateProgressMaximum(xtalDirs.size());
  // If a local queue interface was used, all InProcess structures must be
  // Restarted.
  bool restartInProcessStructures = false;
  bool clearJobIDs = false;
  if (!anyRemoteQueueInterfaces()) {
    restartInProcessStructures = true;
    clearJobIDs = true;
  }
  // Load xtals
  Xtal* xtal;
  QList<uint> keys = comp.keys();
  QList<Structure*> loadedStructures;
  QString xtalStateFileName;
  bool errorMsgAlreadyGiven = false;

  for (int i = 0; i < xtalDirs.size(); i++) {
    if (m_dialog) {
      m_dialog->updateProgressLabel(
        tr("Loading structures(%1 of %2)...").arg(i + 1).arg(xtalDirs.size()));
      m_dialog->updateProgressValue(i);
    }

    xtalStateFileName = dataPath + "/" + xtalDirs.at(i) + "/structure.state";
    debug(tr("Loading structure %1").arg(xtalStateFileName));
    // Check if this is an older session that used xtal.state instead.
    if (!QFile::exists(xtalStateFileName) &&
        QFile::exists(dataPath + "/" + xtalDirs.at(i) + "/xtal.state")) {
      xtalStateFileName = dataPath + "/" + xtalDirs.at(i) + "/xtal.state";
    }

    xtal = new Xtal();
    QWriteLocker locker(&xtal->lock());
    xtal->moveToThread(m_tracker->thread());
    xtal->setupConnections();

    xtal->setLocpath(dataPath + "/" + xtalDirs.at(i) + "/");
    // The "true" in the second parameter tells it to read current structure
    // info. This sets current cell info, atom info, enthalpy, energy, & PV
    xtal->readSettings(xtalStateFileName, true);

    // Reset it's space group
    xtal->findSpaceGroup(tol_spg);

    // Store current state -- updateXtal will overwrite it.
    Xtal::State state = xtal->getStatus();
    // Set state from InProcess -> Restart if needed
    if (restartInProcessStructures && state == Structure::InProcess) {
      state = Structure::Restart;
    }
    QDateTime endtime = xtal->getOptTimerEnd();

    locker.unlock();

    // If the current settings were saved successfully, then the current
    // enthalpy,energy, atom types, atom positions, and cell info must be
    // set already
    SETTINGS(xtalStateFileName);

    // Store the memory address for a parent structure if one exists. Since
    // this resume loads state files in numerical order, the parent of a
    // given structure SHOULD have already been loaded.
    QString parentStructureString =
      settings->value("structure/parentStructure", "").toString();
    if (!parentStructureString.isEmpty()) {
      for (size_t i = 0; i < loadedStructures.size(); i++) {
        // If the xtal skipped optimization, we don't want to count it
        // We also only want to count finished structures...
        if (parentStructureString == loadedStructures.at(i)->getTag() && 
             !xtal->skippedOptimization() &&
             (xtal->getStatus() == Xtal::Duplicate ||
             xtal->getStatus() == Xtal::Supercell ||
             xtal->getStatus() == Xtal::Optimized)) {
          xtal->setParentStructure(loadedStructures.at(i));
          break;
        }
      }
    }

    int version = settings->value("structure/version", -1).toInt();
    bool saveSuccessful =
      settings->value("structure/saveSuccessful", false).toBool();

    // The error message is given by a pop-up
    QString errorMsg = tr("Some structures were not loaded successfully. "
                          "These structures will be over-written if the "
                          "search is resumed."
                          "\n\nPlease check the log for details. ");

    // The warning message is given in the log
    QString warningMsg = tr("structure.state file was not saved "
                            "successfully for %1. This structure will be "
                            "excluded.")
                           .arg(xtal->getLocpath());

    // version == -1 implies that the save failed.
    if (version == -1) {
      if (!errorMsgAlreadyGiven) {
        error(errorMsg);
        errorMsgAlreadyGiven = true;
      }
      warning(warningMsg);
      continue;
    }

    if (version >= 2) {
      // saveSuccessful wasn't introduced until version 3
      if (version >= 3 && !saveSuccessful) {
        // Check the structure.state.old file if this was not saved
        // successfully.
        SETTINGS(xtalStateFileName + ".old");
        if (!settings->value("structure/saveSuccessful", false).toBool()) {
          if (!errorMsgAlreadyGiven) {
            error(errorMsg);
            errorMsgAlreadyGiven = true;
          }
          warning(warningMsg);
          continue;
        }
        // Otherwise, just continue with the new settings in place
      }

      // Reset state
      locker.relock();
      xtal->setStatus(state);
      xtal->setOptTimerEnd(endtime);
      if (clearJobIDs) {
        xtal->setJobID(0);
      }
      // For some strange reason, setEnergy() does not appear to be
      // working in readSettings() in structure.cpp (even though all the
      // others including setEnthalpy() seem to work fine). So we will set it
      // here.
      double energy = settings->value("structure/current/energy", 0).toDouble();
      xtal->setEnergy(energy);

      locker.unlock();
      updateLowestEnthalpyFUList_(qobject_cast<Structure*>(xtal));
      loadedStructures.append(qobject_cast<Structure*>(xtal));
      continue;
    }

    // Reset state
    locker.relock();
    xtal->setStatus(state);
    xtal->setOptTimerEnd(endtime);
    if (clearJobIDs) {
      xtal->setJobID(0);
    }
    locker.unlock();
    updateLowestEnthalpyFUList_(qobject_cast<Structure*>(xtal));
    loadedStructures.append(qobject_cast<Structure*>(xtal));
    // Update the formula unit list. This is for loading older versions
    // of xtalopt
    if (i == 0) {
      formulaUnitsList.clear();
      formulaUnitsList.append(xtal->getFormulaUnits());
      emit updateFormulaUnitsListUIText();
      emit updateVolumesToBePerFU(xtal->getFormulaUnits());
      error("Warning: an XtalOpt run from an older version is being "
            "loaded.\n\nIf you choose to resume the run, you will not "
            "be able to load it with the older version any longer.\n\n"
            "Upon resuming, the formula units will be set to what they were "
            "in the prior run, and the volumes will be adjusted to be per FU.");
    }
  }

  if (m_dialog) {
    m_dialog->updateProgressMinimum(0);
    m_dialog->updateProgressValue(0);
    m_dialog->updateProgressMaximum(loadedStructures.size());
    m_dialog->updateProgressLabel("Sorting and checking structures...");
  }

  // Sort Xtals by index values
  int curpos = 0;
  // dialog->stopProgressUpdate();
  // dialog->startProgressUpdate("Sorting xtals...", 0,
  // loadedStructures.size()-1);
  for (int i = 0; i < loadedStructures.size(); i++) {
    if (m_dialog)
      m_dialog->updateProgressValue(i);
    for (int j = 0; j < loadedStructures.size(); j++) {
      // dialog->updateProgressValue(curpos);
      if (loadedStructures.at(j)->getIndex() == i) {
        loadedStructures.swap(j, curpos);
        curpos++;
      }
    }
  }

  // Locate and assign memory addresses for parent structures

  if (m_dialog) {
    m_dialog->updateProgressMinimum(0);
    m_dialog->updateProgressValue(0);
    m_dialog->updateProgressMaximum(loadedStructures.size());
    m_dialog->updateProgressLabel("Updating structure indices...");
  }

  // Reassign indices (shouldn't always be necessary, but just in case...)
  for (int i = 0; i < loadedStructures.size(); i++) {
    if (m_dialog)
      m_dialog->updateProgressValue(i);
    loadedStructures.at(i)->setIndex(i);
  }

  if (m_dialog) {
    m_dialog->updateProgressMinimum(0);
    m_dialog->updateProgressValue(0);
    m_dialog->updateProgressMaximum(loadedStructures.size());
    m_dialog->updateProgressLabel("Preparing GUI and tracker...");
  }

  // Reset the local file path information in case the files have moved
  locWorkDir = newFilePath;

  Structure* s = 0;
  emit disablePlotUpdate();
  for (int i = 0; i < loadedStructures.size(); i++) {
    s = loadedStructures.at(i);
    if (m_dialog)
      m_dialog->updateProgressValue(i);
    m_tracker->lockForWrite();
    m_tracker->append(s);
    m_tracker->unlock();
    if (s->getStatus() == Structure::WaitingForOptimization)
      m_queue->appendToJobStartTracker(s);
  }

  emit enablePlotUpdate();
  emit updatePlot();

  if (m_dialog)
    m_dialog->updateProgressLabel("Done!");

  // If no structures were loaded successfully, enter read-only mode.
  if (loadedStructures.size() == 0) {
    error(tr("Critical error! No structures were loaded successfully."
             "\nEntering read-only mode."));
    readOnly = true;
    return false;
  }

  // Check if user wants to resume the search
  // If we are using CLI mode, no prompt is needed...
  if (!readOnly && m_usingGUI) {
    bool resume;
    needBoolean(tr("Session '%1' (%2) loaded. Would you like to start "
                   "submitting jobs and resume the search? (Answering "
                   "\"No\" will enter read-only mode.)")
                  .arg(description)
                  .arg(locWorkDir),
                &resume);

    readOnly = !resume;
    qDebug() << "Read only? " << readOnly;
  }

  // Set this up to prevent a bug if "replace with random" is the failure
  // action and "Initialize with RandSpg" is checked.
  if (minXtalsOfSpgPerFU.empty()) {
    for (size_t spg = 1; spg <= 230; spg++)
      minXtalsOfSpgPerFU.append(0);
  }

  return true;
}

bool XtalOpt::plotDir(const QDir& dataDir)
{
  readOnly = true;
  qDebug() << "Loading xtals for plotting...";

  QStringList xtalDirs =
    dataDir.entryList(QStringList(), QDir::AllDirs, QDir::Size);
  xtalDirs.removeAll(".");
  xtalDirs.removeAll("..");
  for (int i = 0; i < xtalDirs.size(); ++i) {
    if (!dataDir.exists(xtalDirs[i] + "/structure.state")) {
      xtalDirs.removeAt(i);
      --i;
    }
  }

  qDebug() << xtalDirs.size() << "xtals were found!";

  if (xtalDirs.isEmpty()) {
    qDebug() << "Error: no xtals were found in" << dataDir.absolutePath()
             << "! Please check your data directory and try again.";
    return false;
  }

  // Load xtals
  QList<Structure*> loadedStructures;

  for (int i = 0; i < xtalDirs.size(); i++) {
    qDebug() << "Loading xtal" << i + 1 << "...";
    QString xtalStateFileName =
      dataDir.absolutePath() + "/" + xtalDirs.at(i) + "/structure.state";

    Xtal* xtal = new Xtal();

    xtal->setLocpath(dataDir.absolutePath() + "/" + xtalDirs.at(i) + "/");
    // The "true" in the second parameter tells it to read current structure
    // info. This sets current cell info, atom info, enthalpy, energy, & PV
    xtal->readSettings(xtalStateFileName, true);

    // Reset it's space group
    xtal->findSpaceGroup(tol_spg);

    // Store current state -- updateXtal will overwrite it.
    Xtal::State state = xtal->getStatus();
    QDateTime endtime = xtal->getOptTimerEnd();

    // If the current settings were saved successfully, then the current
    // enthalpy,energy, atom types, atom positions, and cell info must be
    // set already
    SETTINGS(xtalStateFileName);

    int version = settings->value("structure/version", -1).toInt();
    bool saveSuccessful =
      settings->value("structure/saveSuccessful", false).toBool();
    // version == -1 was the old way to indicate that the save failed...
    if (version == -1)
      saveSuccessful = false;

    bool errorMsgAlreadyGiven = false;

    // The error message is given by a pop-up
    QString errorMsg = tr("Some structures were not loaded successfully. "
                          "These will be ignored. Check the console for "
                          "details");

    // The warning message is given in the log
    QString warningMsg = tr("structure.state file was not saved "
                            "successfully for %1. This structure will be "
                            "excluded.")
                           .arg(xtal->getLocpath());

    if (!saveSuccessful) {
      // Check the structure.state.old file if this was not saved
      // successfully.
      SETTINGS(xtalStateFileName + ".old");
      if (!settings->value("structure/saveSuccessful", false).toBool()) {
        if (!errorMsgAlreadyGiven) {
          error(errorMsg);
          errorMsgAlreadyGiven = true;
        }
        warning(warningMsg);
        continue;
      }
      // Otherwise, just continue with the new settings in place
    }

    // Reset state
    xtal->setStatus(state);
    xtal->setOptTimerEnd(endtime);
    // For some strange reason, setEnergy() does not appear to be
    // working in readSettings() in structure.cpp (even though all the
    // others including setEnthalpy() seem to work fine). So we will set it
    // here.
    double energy = settings->value("structure/current/energy", 0).toDouble();
    xtal->setEnergy(energy);

    updateLowestEnthalpyFUList_(qobject_cast<Structure*>(xtal));
    loadedStructures.append(qobject_cast<Structure*>(xtal));
  }

  // Sort Xtals by index values
  int curpos = 0;
  for (int i = 0; i < loadedStructures.size(); i++) {
    for (int j = 0; j < loadedStructures.size(); j++) {
      if (loadedStructures.at(j)->getIndex() == i) {
        loadedStructures.swap(j, curpos);
        curpos++;
      }
    }
  }

  // Reassign indices (shouldn't always be necessary, but just in case...)
  for (int i = 0; i < loadedStructures.size(); i++)
    loadedStructures.at(i)->setIndex(i);

  // Append to tracker for the plot
  qDebug() << "Preparing GUI...";
  for (int i = 0; i < loadedStructures.size(); i++) {
    qDebug() << "Loading xtal" << i + 1 << "into the GUI...";
    Structure* s = loadedStructures.at(i);
    m_tracker->append(s);
    if (s->getStatus() == Structure::WaitingForOptimization)
      m_queue->appendToJobStartTracker(s);
  }

  emit updatePlot();

  // If no structures were loaded successfully, warn the user.
  if (loadedStructures.isEmpty()) {
    qDebug() << "Error: no structures were loaded successfully!";
    return false;
  }

  return true;
}

bool XtalOpt::onTheFormulaUnitsList(uint FU)
{
  for (int i = 0; i < formulaUnitsList.size(); i++) {
    if (FU == formulaUnitsList.at(i))
      return true;
  }
  return false;
}

void XtalOpt::resetSpacegroups()
{
  if (isStarting) {
    return;
  }
  QtConcurrent::run(this, &XtalOpt::resetSpacegroups_);
}

void XtalOpt::resetSpacegroups_()
{
  const QList<Structure*> structures = *(m_tracker->list());
  for (QList<Structure *>::const_iterator it = structures.constBegin(),
                                          it_end = structures.constEnd();
       it != it_end; ++it) {
    (*it)->lock().lockForWrite();
    qobject_cast<Xtal*>(*it)->findSpaceGroup(tol_spg);
    (*it)->lock().unlock();
  }
}

void XtalOpt::resetDuplicates()
{
  if (isStarting) {
    return;
  }
  QtConcurrent::run(this, &XtalOpt::resetDuplicates_);
}

void XtalOpt::resetDuplicates_()
{
  const QList<Structure*>* structures = m_tracker->list();
  Xtal* xtal = 0;
  for (int i = 0; i < structures->size(); i++) {
    xtal = qobject_cast<Xtal*>(structures->at(i));
    QWriteLocker xtalLocker(&xtal->lock());
    // Let's reset supercells here too
    if (xtal->getStatus() == Xtal::Duplicate ||
        xtal->getStatus() == Xtal::Supercell) {
      xtal->setStatus(Xtal::Optimized);
    }
    xtal->structureChanged(); // Reset cached comparisons
  }
  checkForDuplicates();
}

void XtalOpt::getScaledVolumePerFU(double& scl_min, double& scl_max,
                                   double& vol_min, double& vol_max)
{
  // This function returns the scaled factors (if any adjustment is needed),
  //   and the min/max of volume per FU as the total vdW spheres' volumes
  //   times the scaling factors.
  // It takes effect only if the composition is set!
  QList<uint> atomicNums = this->comp.keys();
  if (atomicNums.size() == 0)
    return;

  // First, find the equivalent vdW volume per FU
  double vdw_vol = 0.0;
  for (int i = 0; i < atomicNums.size(); i++)
    if (atomicNums[i] != 0) {
      double q = this->comp.value(atomicNums[i]).quantity;
      double r = ElemInfoDatabase::_vdwRadii[atomicNums[i]];
      vdw_vol += q * 4.0 / 3.0 * PI * pow(r, 3.0);
    }
  if (vdw_vol < ZERO5)
    return;

  // Now, sort out the situation with scaling factors
  if (scl_min < ZERO5 && scl_max > ZERO5) {
    vol_min = 1.0;
    vol_max = vdw_vol * scl_max;
  } else if (scl_min > ZERO5 && scl_max < scl_min) {
    scl_max = scl_min;
    vol_min = vdw_vol * scl_min;
    vol_max = vdw_vol * scl_max;
  } else if (scl_min > ZERO5 && scl_max > ZERO5) {
    vol_min = vdw_vol * scl_min;
    vol_max = vdw_vol * scl_max;
  }
}

bool XtalOpt::processObjectivesInfo()
{
  // This function processes the objective/aflow-hardness entries from the CLI
  //   or GUI input. It reads the input info, initiates weights and output files
  //   (which are optional in CLI mode), sets flags and variables for
  //    multi-objective run; then, calls the "processObjectivesWeights" function.
  //
  // From now on, aflow-hardness is being treated just as one of the objectives:
  //      opt_typ = hardness
  //      weight  = input (default value of -1.0)
  //      no script path or output file name is needed
  // Internally, though, the same old flags are set since the nature of the
  //      aflow-hardness calculations is not similar to that of user-defined objectives.
  //      As a result, m_objectives_num ONLY includes non-aflow-hardness objectives.

  // Initialize the temp value for *legacy* aflow-hardness flags,
  //      the weight will be adjusted in processObjectivesWeight
  m_calculateHardness     = false;  // no aflow-hardness calculations
  m_hardnessFitnessWeight = -1.0;   // no weight is given

  // This needs to be done: this function might be
  //   called more than once in gui mode
  resetObjectives();

  int objnum = 0; // Counter for non-aflow-hardness objectives
  for (int i = 0; i < objectiveListSize(); i++)
  {
    QString     line  = objectiveListGet(i);
    QStringList sline = line.split(" ", QString::SkipEmptyParts);
    int nparam = sline.size();
    // There should be at least one entry in the input!
    if (nparam < 1)
    { qDebug() << "Error: objectives are not properly initiated: " << line; return false; }
    // Initializing some temporary variables;
    //   first parameter in the input entry is always optimization type: min/max/fil/har
    QString tmps = sline.at(0).toLower().mid(0,3);
    ObjectivesType objtyp = (tmps == "min") ? ObjectivesType::Ot_Min :
      ((tmps == "max") ? ObjectivesType::Ot_Max : ((tmps == "fil" ) ?
      ObjectivesType::Ot_Fil : ObjectivesType::Ot_Har ));
    QString objout = "objective" + QString::number(objnum+1) + ".out";
    QString objexe = "none";
    double  objwgt = -1.0;
    //
    bool   isNumber;
    double vline;
    // aflow-hardness might have only 1 entry (opt type), the rest should have at least 2
    //   (opt type and executable paht; output file and weight optional in CLI).
    if (objtyp != ObjectivesType::Ot_Har && nparam < 2)
    { qDebug() << "Error: objectives are not properly initiated: " << line; return false; }
    // Start by handling aflow-hardness entry (if any!)
    if (objtyp == ObjectivesType::Ot_Har)
    {
      m_calculateHardness = true;
      // Check if aflow-hardness weight is given
      for (int j = 1; j < nparam; j++)
      {
        vline = sline.at(j).toDouble(&isNumber);
        if (isNumber && vline >= 0.0)
          m_hardnessFitnessWeight = vline;
      }
    }
    // Now, the rest of optimization types (min/max/fil)
    else
    {
      objexe = sline.at(1); // The second field is always the script path. Now, read
                            //   the optional fields of output file and weight if they are
                            //   given; otherwise, they will remain with their default values.
      switch(nparam) {
        case 3: // 3rd item might be weight or output filename
          vline = sline.at(2).toDouble(&isNumber);
          if (isNumber && vline >= 0.0)
            objwgt = vline;
          else
            objout = sline.at(2);
          break;
        case 4: // one of them is weight and one is output filename
          vline = sline.at(2).toDouble(&isNumber);
          if (isNumber && vline >= 0.0) {
            objwgt = vline;
            objout = sline.at(3);
          }
          else {
            objwgt = (sline.at(3).toDouble() >= 0.0) ? sline.at(3).toDouble() : -1.0;
            objout = sline.at(2);
          }
          break;
        default:
          break;
      }

      // We have collected all the info! Add to the list of objectives
      setObjectivesTyp(objtyp);
      setObjectivesExe(objexe);
      setObjectivesOut(objout);
      setObjectivesWgt(objwgt);

      // So, we have a non-aflow-hardness objective; add to the counter
      objnum++;
    }
  }

  setObjectivesNum(objnum);

  // Process weights and initialize unspecified weights
  return processObjectivesWeights();
}

bool XtalOpt::processObjectivesWeights()
{
  // This function adjusts the weights and performs some sanity checks on the
  //   objectives' data.
  //
  // Essentially, objectives/aflow-hardness can have weights of 0.0 <= w <= 1.0.
  //     The 0.0 is to allow the objective being calculated while not enter
  //     the optimization. Here, we want to treat unspecified weights, and
  //     make sure the total optimization weight is fine (0.0<=total_w<=1.0).
  //
  // Arriving here from the processObjectivesInfo(), every objective
  //     (including aflow-hardness) that has a weight of -1.0, means that
  //     its weight is unspecified by the user, and we take care of it here!
  //
  // To not have a chance of affecting the optimization,
  //     weight for filtering objectives will be set to 0.0
  //
  // For aflow-hardness; the weight will be set to "-1.0" if
  //     no aflow-hardness calculation
  //
  // Otherwise, initialization of unspecified weights depends on aflow-hardness,
  //     and on the number of objectives with weight of -1.0. Basically, we divide
  //     the (1.0-total_non_zero_weight) between optimizable objectives, i.e.,
  //     enthalpy and objectives (and possibly aflow-hardness) without weight.
  //
  // We always have enthalpy as a objective with a weight which is not predetermined.
  //     So, the number of optimizable objectives with unspecified weight,
  //     cparam, starts from 1. If we have aflow-hardness calculations and
  //     its weight is still -1 (i.e., not specified), cparam = 2.
  //     Next, we examine non-filter objectives for those without given weight.

  int    cparam      = 1; // # of objectives of unspecified weights; always count enthalpy!
  double totalweight = 0.0;

  // Initiate some temporary variables
  int                   objnum = getObjectivesNum();
  QList<ObjectivesType> objtyp = {};
  QStringList           objout = {};
  QStringList           objexe = {};
  QList<double>         objwgt = {};
  for (int i = 0; i < objnum; i++) {
    objtyp.push_back(getObjectivesTyp(i));
    objout.push_back(getObjectivesOut(i));
    objexe.push_back(getObjectivesExe(i));
    objwgt.push_back(getObjectivesWgt(i));
  }

  // If aflow-hardness, analyse its weight
  if (m_calculateHardness) {
    if (m_hardnessFitnessWeight >= 0.0)
      totalweight += m_hardnessFitnessWeight;
    else // We have aflow-hardness with unspecified weight
      cparam += 1;
  }

  // Find objectives with unspecified weight
  //   (and set the weight for the filtration objectives to 0.0)
  for (int i = 0; i < objnum; i++)
    if (objtyp[i] == ObjectivesType::Ot_Fil)
      objwgt[i] = 0.0;
    else {
    if (objwgt[i] >= 0.0)
      totalweight += objwgt[i];
    else // We have an objective with unspecified weight
      cparam += 1;
    }

  // Initialize hardness weight if it's unspecified (=-1.0)
  //   and if hardness calculation
  if (m_calculateHardness && m_hardnessFitnessWeight < 0.0)
    m_hardnessFitnessWeight = (1.0 - totalweight) / (double)(cparam);

  // Initialize objectives with unspecified weight
  //   (weight=-1.0 as for filtering it's already set to 0.0)
  for (int i = 0; i < objnum; i++)
    if (objwgt[i] < 0.0)
      objwgt[i] = (1.0 - totalweight) / (double)(cparam);

  // Here, everything should be sorted out! Weights must be either 0.0 or
  //     some value less than 1.0, while the total optimization weight is <=1.0.
  //     Now, perform a sanity check to make sure that this is the case.
  totalweight = 0.0;
  if (m_calculateHardness) {
    if (m_hardnessFitnessWeight < 0.0) {
      error(tr("Weight for aflow-hardness can't be negative ( %1 )")
          .arg(m_hardnessFitnessWeight));
      return false;
    } else {
      totalweight += m_hardnessFitnessWeight;
    }
  }
  for (int i = 0; i < objnum; i++)
  {
    if (objwgt[i] < 0.0) {
      error(tr("Weights of objectives can't be negative ( %1 : %2 )")
          .arg(i+1).arg(objwgt[i]));
      return false;
    } else {
      totalweight += objwgt[i];
    }
  }
  if ((totalweight < 0.0) || (totalweight > 1.0))
  {
    error(tr("Total weight for objectives can't be negative or exceed 1.0 ( %1 )")
       .arg(totalweight));
    return false;
  }

  // Finally, re-construct objectives list; this is to make gui table look nice!
  objectiveListClear();
  if (m_calculateHardness)
    objectiveListAdd("hardness  N/A  N/A  " + QString::number(m_hardnessFitnessWeight));

  resetObjectives();
  m_calculateObjectives = (objnum > 0) ? true : false;
  setObjectivesNum(objnum);
  for (int i = 0; i < objnum; i++)
  {
    QString opttype = (objtyp[i] == ObjectivesType::Ot_Min) ? "minimization" :
      ((objtyp[i] == ObjectivesType::Ot_Max) ? "maximization" : "filtration");
    setObjectivesTyp(objtyp[i]);
    setObjectivesExe(objexe[i]);
    setObjectivesOut(objout[i]);
    setObjectivesWgt(objwgt[i]);
    objectiveListAdd(opttype + " " + objexe[i] + " " + objout[i] + " " + QString::number(objwgt[i]));
  }

  return true;
}

// Helper struct for the map below
struct dupCheckStruct
{
  Xtal *i, *j;
  double tol_len, tol_ang;
};

// Helper Supercell Check Struct is defined in the header

void checkIfDups(dupCheckStruct& st)
{
  if (st.i == st.j)
    return;
  Xtal *kickXtal, *keepXtal;
  QReadLocker iLocker(&st.i->lock());
  QReadLocker jLocker(&st.j->lock());
  // if they are already both duplicates, just return.
  if (st.i->getStatus() == Xtal::Duplicate &&
      st.j->getStatus() == Xtal::Duplicate) {
    return;
  }
  if (st.i->compareCoordinates(*st.j, st.tol_len, st.tol_ang)) {
    // Mark the newest xtal as a duplicate of the oldest. This keeps the
    // lowest-energy plot trace accurate.
    // For some reason, primitive structures do not always update their
    // indices immediately, and they remain the default "-1". So, if one
    // of the indices is -1, set that to be the kickXtal
    if (st.i->getIndex() == -1) {
      kickXtal = st.i;
      keepXtal = st.j;
    } else if (st.j->getIndex() == -1) {
      kickXtal = st.j;
      keepXtal = st.i;
    } else if (st.i->getIndex() > st.j->getIndex()) {
      kickXtal = st.i;
      keepXtal = st.j;
    } else {
      kickXtal = st.j;
      keepXtal = st.i;
    }
    // If the kickXtal is already a duplicate, just return
    if (kickXtal->getStatus() == Xtal::Duplicate ||
        kickXtal->getStatus() == Xtal::Supercell) {
      return;
    }
    // Unlock the kickXtal and lock it for writing
    kickXtal == st.i ? iLocker.unlock() : jLocker.unlock();
    QWriteLocker kickXtalLocker(&kickXtal->lock());
    kickXtal->setStatus(Xtal::Duplicate);
    kickXtal->setDuplicateString(QString("%1x%2")
                                   .arg(keepXtal->getGeneration())
                                   .arg(keepXtal->getIDNumber()));
  }
}

void XtalOpt::checkIfSups(supCheckStruct& st)
{
  if (st.i == st.j)
    return;
  Xtal *smallerFormulaUnitXtal, *largerFormulaUnitXtal;
  QReadLocker iLocker(&st.i->lock());
  QReadLocker jLocker(&st.j->lock());

  // Determine the larger formula unit structure and the smaller formula unit
  // structure.
  if (st.i->getFormulaUnits() > st.j->getFormulaUnits()) {
    largerFormulaUnitXtal = st.i;
    smallerFormulaUnitXtal = st.j;
  } else {
    largerFormulaUnitXtal = st.j;
    smallerFormulaUnitXtal = st.i;
  }

  // if the larger formula unit xtal is already a supercell or duplicate,
  // skip over it.
  if (largerFormulaUnitXtal->getStatus() == Xtal::Supercell ||
      largerFormulaUnitXtal->getStatus() == Xtal::Duplicate) {
    return;
  }

  // This temporary xtal will need to be deleted
  Xtal* tempXtal = generateSuperCell(smallerFormulaUnitXtal->getFormulaUnits(),
                                     largerFormulaUnitXtal->getFormulaUnits(),
                                     smallerFormulaUnitXtal, false);

  if (tempXtal->compareCoordinates(*largerFormulaUnitXtal, st.tol_len,
                                   st.tol_ang)) {
    // Unlock the larger formula unit xtal and lock it for writing
    largerFormulaUnitXtal == st.i ? iLocker.unlock() : jLocker.unlock();
    QWriteLocker largerFUXtalLocker(&largerFormulaUnitXtal->lock());

    // We're going to label the larger formula unit structure a supercell
    // of the smaller. The smaller structure is more fundamental and should
    // remain in the gene pool.
    largerFormulaUnitXtal->setStatus(Xtal::Supercell);
    // If the smaller formula unit xtal is already a duplicate, make the
    // supercell a supercell the structure that the smaller formula unit
    // duplicate points to.
    if (smallerFormulaUnitXtal->getStatus() == Xtal::Duplicate)
      largerFormulaUnitXtal->setSupercellString(
        smallerFormulaUnitXtal->getDuplicateString());
    else if (smallerFormulaUnitXtal->getStatus() == Xtal::Supercell)
      largerFormulaUnitXtal->setSupercellString(
        smallerFormulaUnitXtal->getSupercellString());
    // Otherwise, just make it a supercell of the smaller formula unit xtal
    else
      largerFormulaUnitXtal->setSupercellString(
        QString("%1x%2")
          .arg(smallerFormulaUnitXtal->getGeneration())
          .arg(smallerFormulaUnitXtal->getIDNumber()));
  }
  tempXtal->deleteLater();
}

void XtalOpt::checkForDuplicates()
{
  if (isStarting)
    return;

  QtConcurrent::run(this, &XtalOpt::checkForDuplicates_);
}

void XtalOpt::checkForDuplicates_()
{
  // Only run this function with one thread at a time.
  static std::mutex dupMutex;
  std::unique_lock<std::mutex> dupLock(dupMutex, std::defer_lock);
  if (!dupLock.try_lock()) {
    // If a thread is already running this function, we can wait. But
    // we should only have one waiter at any time.
    static std::mutex waitMutex;
    std::unique_lock<std::mutex> waitLock(waitMutex, std::defer_lock);
    if (!waitLock.try_lock())
      return;
    else
      dupLock.lock();
  }

  QReadLocker trackerLocker(m_tracker->rwLock());
  const QList<Structure*>* structures = m_tracker->list();
  QList<Xtal*> xtals;
  xtals.reserve(structures->size());
  std::for_each(structures->begin(), structures->end(), [&xtals](Structure* s) {
    xtals.append(qobject_cast<Xtal*>(s));
  });

  // Build helper structs
  QList<dupCheckStruct> dupSts;
  dupCheckStruct dupSt;
  dupSt.tol_len = this->tol_xcLength;
  dupSt.tol_ang = this->tol_xcAngle;

  QList<supCheckStruct> supSts;
  supCheckStruct supSt;
  supSt.tol_len = this->tol_xcLength;
  supSt.tol_ang = this->tol_xcAngle;

  for (QList<Xtal*>::iterator xi = xtals.begin(); xi != xtals.end(); ++xi) {
    QReadLocker xiLocker(&(*xi)->lock());
    if ((*xi)->getStatus() != Xtal::Optimized)
      continue;

    for (QList<Xtal*>::iterator xj = xi + 1; xj != xtals.end(); ++xj) {
      QReadLocker xjLocker(&(*xj)->lock());
      if ((*xj)->getStatus() != Xtal::Optimized)
        continue;

      if (((*xi)->hasChangedSinceDupChecked() ||
           (*xj)->hasChangedSinceDupChecked()) &&
          // Perform a course enthalpy screening to cut down on number of
          // comparisons
          fabs(((*xi)->getEnthalpyPerAtom()) -
               ((*xj)->getEnthalpyPerAtom())) < 0.1 &&
          // Screen out options that CANNOT be supercells
          (((*xi)->getFormulaUnits() % (*xj)->getFormulaUnits() == 0) ||
           ((*xj)->getFormulaUnits() % (*xi)->getFormulaUnits() == 0))) {
        // Append the duplicate structs list
        if ((*xi)->getFormulaUnits() == (*xj)->getFormulaUnits()) {
          dupSt.i = (*xi);
          dupSt.j = (*xj);
          dupSts.append(dupSt);
        }
        // Append the supercell structs list. One has to be a formula unit
        // multiple of the other to be a candidate supercell. In addition,
        // their formula units cannot equal.
        else if (((*xi)->getFormulaUnits() % (*xj)->getFormulaUnits() == 0) ||
                 ((*xj)->getFormulaUnits() % (*xi)->getFormulaUnits() == 0)) {
          supSt.i = (*xi);
          supSt.j = (*xj);
          supSts.append(supSt);
        }
      }
    }
    // Nothing else should be setting this, so just update under a
    // read lock
    (*xi)->setChangedSinceDupChecked(false);
  }

  for (auto& dupSt : dupSts)
    checkIfDups(dupSt);

  // Tried to run this concurrently. Would freeze upon resuming for some
  // reason, though...
  // QtConcurrent::blockingMap(supSts, checkIfSups);
  for (auto& supSt : supSts)
    checkIfSups(supSt);

  // Label supercells that primitive xtals came from as such
  for (size_t i = 0; i < xtals.size(); ++i) {
    QReadLocker ixtalLocker(&xtals[i]->lock());
    if (xtals[i]->skippedOptimization()) {
      // We should only check the crystals that are ordered before this one,
      // since a primitive reduced crystal will always have an index greater
      // than its parent
      for (size_t j = 0; j < i; ++j) {
        // If the Xtal is not optimized, just continue
        if (xtals[j]->getStatus() != Xtal::Optimized)
          continue;
        QWriteLocker jxtalLocker(&xtals[j]->lock());
        // Check and see if xtals[j] matches xtals[i]
        if (xtals[i]->getParents() == tr("Primitive of %1x%2")
            .arg((xtals[j])->getGeneration())
            .arg(xtals[j]->getIDNumber())) {
          xtals[j]->setStatus(Xtal::Supercell);
          // If xtals[i] is a duplicate, set xtals[j] to be a super cell
          // of the xtal that xtals[i] is a duplicate of
          if (xtals[i]->getStatus() == Xtal::Duplicate)
            xtals[j]->setSupercellString(xtals[i]->getDuplicateString());
          else if (xtals[i]->getStatus() == Xtal::Supercell)
            xtals[j]->setSupercellString(xtals[i]->getSupercellString());
          else {
            xtals[j]->setSupercellString(QString("%1x%2")
                                         .arg(xtals.at(i)->getGeneration())
                                         .arg(xtals.at(i)->getIDNumber()));
          }
        }
      }
    }
  }

  emit refreshAllStructureInfo();
}

void XtalOpt::updateLowestEnthalpyFUList(GlobalSearch::Structure* s)
{
  // Perform this in a background thread
  QtConcurrent::run(this, &XtalOpt::updateLowestEnthalpyFUList_, s);
}

void XtalOpt::updateLowestEnthalpyFUList_(GlobalSearch::Structure* s)
{
  // Thankfully, the enthalpy appears to get updated before it reaches this
  // point
  QReadLocker sLocker(&s->lock());
  // This is to prevent segmentation faults...
  while (lowestEnthalpyFUList.size() < s->getFormulaUnits() + 1) {
    lowestEnthalpyFUList.append(0);
  }
  if (lowestEnthalpyFUList.at(s->getFormulaUnits()) == 0 ||
      lowestEnthalpyFUList.at(s->getFormulaUnits()) > s->getEnthalpy()) {
    lowestEnthalpyFUList[s->getFormulaUnits()] = s->getEnthalpy();
  }
}

void XtalOpt::setLatticeMinsAndMaxes(latticeStruct& latticeMins,
                                     latticeStruct& latticeMaxes)
{
  latticeMins.a = a_min;
  latticeMins.b = b_min;
  latticeMins.c = c_min;
  latticeMins.alpha = alpha_min;
  latticeMins.beta = beta_min;
  latticeMins.gamma = gamma_min;

  latticeMaxes.a = a_max;
  latticeMaxes.b = b_max;
  latticeMaxes.c = c_max;
  latticeMaxes.alpha = alpha_max;
  latticeMaxes.beta = beta_max;
  latticeMaxes.gamma = gamma_max;
}

QList<uint> XtalOpt::getListOfAtoms(uint FU)
{
  // Populate crystal
  QList<uint> atomicNums = comp.keys();
  // Sort atomic number by decreasing minimum radius. Adding the "larger"
  // atoms first encourages a more even (and ordered) distribution
  for (int i = 0; i < atomicNums.size() - 1; ++i) {
    for (int j = i + 1; j < atomicNums.size(); ++j) {
      if (this->comp.value(atomicNums[i]).minRadius <
          this->comp.value(atomicNums[j]).minRadius) {
        atomicNums.swap(i, j);
      }
    }
  }

  QList<uint> atoms;
  for (size_t i = 0; i < atomicNums.size(); i++) {
    for (size_t j = 0; j < comp.value(atomicNums[i]).quantity * FU; j++) {
      atoms.push_back(atomicNums[i]);
    }
  }
  return atoms;
}

std::vector<uint> XtalOpt::getStdVecOfAtoms(uint FU)
{
  return getListOfAtoms(FU).toVector().toStdVector();
}

// minXtalsOfSpgPerFU should already be set up by now
uint XtalOpt::pickRandomSpgFromPossibleOnes()
{
  if (minXtalsOfSpgPerFU.size() == 0) {
    qDebug() << "Error! pickRandomSpgFromPossibleOnes() was called before"
             << "minXtalsOfSpgPerFU was set up!";
    return 1;
  }

  QList<uint> possibleSpgs;
  for (size_t i = 0; i < minXtalsOfSpgPerFU.size(); i++) {
    uint spg = i + 1;
    if (minXtalsOfSpgPerFU.at(i) != -1)
      possibleSpgs.append(spg);
  }

  // If they are all impossible, print an error and return 1
  if (possibleSpgs.size() == 0) {
    qDebug() << "Error! In pickRandomSpgFromPossibleOnes(), no spacegroups"
             << "were selected to be allowed! We will be generating a"
             << "structure with a spacegroup of 1";
    return 1;
  }

  // Pick a random index from the list
  size_t idx = rand() % int(possibleSpgs.size());

  // Return the spacegroup at this index
  return possibleSpgs.at(idx);
}

void XtalOpt::updateProgressBar(size_t goal, size_t attempted, size_t succeeded)
{
  // Update progress bar
  if (m_dialog) {
    m_dialog->updateProgressMaximum(goal);
    m_dialog->updateProgressValue(succeeded);
    m_dialog->updateProgressLabel(
      tr("%1 structures generated (%2 kept, %3 rejected)...")
        .arg(attempted)
        .arg(succeeded)
        .arg(attempted - succeeded));
  }
}

uint XtalOpt::minFU()
{
  if (formulaUnitsList.empty())
    formulaUnitsList.append(1);
  else
    qSort(formulaUnitsList);
  return formulaUnitsList[0];
}

uint XtalOpt::maxFU()
{
  if (formulaUnitsList.empty())
    formulaUnitsList.append(1);
  else
    qSort(formulaUnitsList);
  return formulaUnitsList[formulaUnitsList.size() - 1];
}

QString toString(bool b)
{
  return b ? "true" : "false";
}

void XtalOpt::setGeom(unsigned int& geom, QString strGeom)
{
  // Two neighbors
  if (strGeom.contains("Linear")) {
    geom = 1;
  } else if (strGeom.contains("Bent")) {
    geom = 2;
    // Three neighbors
  } else if (strGeom.contains("Trigonal Planar")) {
    geom = 2;
  } else if (strGeom.contains("Trigonal Pyramidal")) {
    geom = 3;
  } else if (strGeom.contains("T-Shaped")) {
    geom = 4;
    // Four neighbors
  } else if (strGeom.contains("Tetrahedral")) {
    geom = 3;
  } else if (strGeom.contains("See-Saw")) {
    geom = 5;
  } else if (strGeom.contains("Square Planar")) {
    geom = 4;
    // Five neighbors
  } else if (strGeom.contains("Trigonal Bipyramidal")) {
    geom = 5;
  } else if (strGeom.contains("Square Pyramidal")) {
    geom = 6;
    // Six neighbors
  } else if (strGeom.contains("Octahedral")) {
    geom = 6;
    // Default
  } else {
    geom = 0;
  }
}

// With the number of neighbors and the geom number, returns the geom string
// I think this would be simpler if we had 1 geom number per name. But
// unfortunately, that is not the case...
QString XtalOpt::getGeom(int numNeighbors, int geom)
{
  if (numNeighbors == 1) {
    if (geom == 1)
      return "linear";
    else
      return "unknown";
  }
  if (numNeighbors == 2) {
    if (geom == 1)
      return "linear";
    else if (geom == 2)
      return "bent";
    else
      return "unknown";
  }
  if (numNeighbors == 3) {
    if (geom == 2)
      return "trigonal planar";
    else if (geom == 3)
      return "trigonal pyramidal";
    else if (geom == 4)
      return "t-shaped";
    else
      return "unknown";
  }
  if (numNeighbors == 4) {
    if (geom == 3)
      return "tetrahedral";
    else if (geom == 5)
      return "see-saw";
    else if (geom == 4)
      return "square planar";
    else
      return "unknown";
  }
  if (numNeighbors == 5) {
    if (geom == 5)
      return "trigonal bipyramidal";
    else if (geom == 6)
      return "square pyramidal";
    else
      return "unknown";
  }
  if (numNeighbors == 6) {
    if (geom == 6)
      return "octahedral";
    else
      return "unknown";
  }
  return "unknown";
}

void XtalOpt::printOptionSettings(QTextStream& stream) const
{
  stream << "\n=== Initialization Settings";

  stream << "\n  Composition: \n";
  for (const auto& key : comp.keys()) {
    stream << "    " << ElemInfo::getAtomicSymbol(key).c_str()
           << comp[key].quantity << "\n";
  }

  if (using_interatomicDistanceLimit) {
    stream << "\n  Atomic radii (Angstroms): \n";
    for (const auto& key : comp.keys()) {
      stream << "    " << ElemInfo::getAtomicSymbol(key).c_str() << ": "
             << comp[key].minRadius << "\n";
    }
  }

  stream << "\n  Formula Units: \n";
  for (const auto& elem : formulaUnitsList)
    stream << "    " << elem << "\n";
  stream << "\n  aMin: " << a_min << "\n";
  stream << "  bMin: " << b_min << "\n";
  stream << "  cMin: " << c_min << "\n";
  stream << "  aMax: " << a_max << "\n";
  stream << "  bMax: " << b_max << "\n";
  stream << "  cMax: " << c_max << "\n";

  stream << "\n  alphaMin: " << alpha_min << "\n";
  stream << "  betaMin: " << beta_min << "\n";
  stream << "  gammaMin: " << gamma_min << "\n";
  stream << "  alphaMax: " << alpha_max << "\n";
  stream << "  betaMax: " << beta_max << "\n";
  stream << "  gammaMax: " << gamma_max << "\n";

  stream << "\n  volumeMin: " << vol_min << "\n";
  stream << "  volumeMax: " << vol_max << "\n";

  stream << "\n  volumeScaleMin: " << vol_scale_min << "\n";
  stream << "  volumeScaleMax: " << vol_scale_max << "\n";

  stream << "\n  usingRadiiInteratomicDistanceLimit: "
         << toString(using_interatomicDistanceLimit) << "\n";
  if (using_interatomicDistanceLimit) {
    stream << "  radiiScalingFactor: " << scaleFactor << "\n";
    stream << "  minRadius: " << minRadius << "\n";
  }

  stream << "\n  usingCustomIADs: " << toString(using_customIAD) << "\n";
  if (using_customIAD) {
    stream << "  current customIADs:\n";
    stream << "  <firstSymbol>, <secondSymbol>, <minIAD>\n";
    for (const auto& pair : interComp.keys()) {
      stream << "  " << ElemInfo::getAtomicSymbol(pair.first).c_str() << ", "
             << ElemInfo::getAtomicSymbol(pair.second).c_str() << ", "
             << interComp[pair].minIAD << "\n";
    }
  }

  stream << "\n  checkIADPostOptimization: " << toString(using_checkStepOpt)
         << "\n";

  stream << "\n  usingSubcellMitosis: " << toString(using_mitosis) << "\n";
  if (using_mitosis) {
    stream << "  printSubcell: " << toString(using_subcellPrint) << "\n";
    stream << "  mitosisDivisions: " << divisions << "\n";
    stream << "  mitosisA: " << ax << "\n";
    stream << "  mitosisB: " << bx << "\n";
    stream << "  mitosisC: " << cx << "\n";
  }

  stream << "\n  usingMolecularUnits: " << toString(using_molUnit) << "\n";
  if (using_molUnit) {
    stream << "  molUnits:\n";
    stream << "  <center>, <numCenters>, <neighbor>, <numNeighbors>, "
              "<geometry>, <distance>\n";
    for (const auto& pair : compMolUnit.keys()) {
      stream << "    " << ElemInfo::getAtomicSymbol(pair.first).c_str() << ", "
             << compMolUnit[pair].numCenters << ", "
             << ElemInfo::getAtomicSymbol(pair.second).c_str() << ", "
             << compMolUnit[pair].numNeighbors << ", "
             << getGeom(compMolUnit[pair].numNeighbors, compMolUnit[pair].geom)
             << ", " << compMolUnit[pair].dist << "\n";
    }
  }

  stream << "\n  usingRandSpg: " << toString(using_randSpg) << "\n";
  if (using_randSpg) {
    stream << "  Space group settings:\n";
    for (int i = 0; i < minXtalsOfSpgPerFU.size(); ++i) {
      int num = minXtalsOfSpgPerFU[i];
      if (num == -1)
        stream << "    spg " << i + 1 << ": do not generate\n";
      else if (num > 0)
        stream << "    spg " << i + 1 << ": generate " << num << "\n";
    }
  }

  stream << "\n=== Search Settings\n";
  stream << "  numInitial: " << numInitial << "\n";
  stream << "  popSize: " << popSize << "\n";
  stream << "  limitRunningJobs: " << toString(limitRunningJobs) << "\n";
  if (limitRunningJobs) {
    stream << "  runningJobLimit: " << runningJobLimit << "\n";
  }
  stream << "  continuousStructures: " << contStructs << "\n";
  stream << "  jobFailLimit: " << failLimit << "\n";
  stream << "  jobFailAction: ";
  if (failAction == FA_DoNothing)
    stream << "Keep Trying\n";
  else if (failAction == FA_KillIt)
    stream << "Kill it\n";
  else if (failAction == FA_Randomize)
    stream << "replaceWithRandom\n";
  else if (failAction == FA_NewOffspring)
    stream << "replaceWithOffspring\n";
  else
    stream << "Unknown fail action\n";

  stream << "  maxNumStructures: " << cutoff << "\n";

  stream << "\n  usingMitoticGrowth: " << toString(using_mitotic_growth)
         << "\n";
  stream << "  usingFormulUnitCrossovers: " << toString(using_FU_crossovers)
         << "\n";
  if (using_FU_crossovers) {
    stream << "  formulaUnitCrossoversGen: " << FU_crossovers_generation
           << "\n";
  }
  stream << "  usingOneGenePool: " << toString(using_one_pool) << "\n";
  if (using_one_pool)
    stream << "  chanceOfFutureMitosis: " << chance_of_mitosis << "\n";

  stream << "\n  percentChanceStripple: " << p_strip << "\n";
  stream << "  percentChancePermutation: " << p_perm << "\n";
  stream << "  percentChanceCrossover: " << p_cross << "\n";

  stream << "\n  strippleAmplitudeMin: " << strip_amp_min << "\n";
  stream << "  strippleAmplitudeMax: " << strip_amp_max << "\n";
  stream << "  strippleNumWavesAxis1: " << strip_per1 << "\n";
  stream << "  strippleNumWavesAxis2: " << strip_per2 << "\n";
  stream << "  strippleStrainStdevMin: " << strip_strainStdev_min << "\n";
  stream << "  strippleStrainStdevMax: " << strip_strainStdev_max << "\n";

  stream << "\n  permustrainNumExchanges: " << perm_ex << "\n";
  stream << "  permustrainStrainStdevMax: " << perm_strainStdev_max << "\n";

  stream << "\n  crossoverMinContribution: " << cross_minimumContribution
         << "\n";

  stream << "\n  xtalcompToleranceLength: " << tol_xcLength << "\n";
  stream << "  xtalcompToleranceAngle: " << tol_xcAngle << "\n";

  stream << "\n  spglibTolerance: " << tol_spg << "\n";

  stream << "\n=== Queue Interface Settings\n";
  stream << "  localQueue: " << toString(m_localQueue) << "\n";

  bool anyRemote = false;
  for (size_t i = 0; i < getNumOptSteps(); ++i) {
    stream << " OptStep " << i + 1 << ":\n";
    const GlobalSearch::QueueInterface* queue = queueInterface(i);
    if (!queue) {
      stream << "  queueInterface: NONE\n";
    } else {
      stream << "  queueInterface: " << queue->getIDString() << "\n";
#ifdef ENABLE_SSH
      if (queue->getIDString().toLower() != "local") {
        anyRemote = true;

        const GlobalSearch::RemoteQueueInterface* remoteQueue =
          qobject_cast<const GlobalSearch::RemoteQueueInterface*>(queue);

        stream << "    submitCommand: " << remoteQueue->submitCommand() << "\n";
        stream << "    cancelCommand: " << remoteQueue->cancelCommand() << "\n";
        stream << "    statusCommand: " << remoteQueue->statusCommand() << "\n";
      }
#endif
    }
  }

  if (anyRemote) {
    stream << "\n=== Remote Queue Settings\n";
    stream << "    host: " << host << "\n";
    stream << "    port: " << port << "\n";
    stream << "    username: " << username << "\n";
    stream << "    remoteWorkingDirectory: " << remWorkDir << "\n";
    stream << "    queueRefreshInterval: " << queueRefreshInterval() << "\n";
    stream << "    cleanRemoteDirs: " << toString(cleanRemoteOnStop()) << "\n";
  }

  stream << "\n=== Local Queue Settings\n";
  stream << "  localWorkingDirectory: " << locWorkDir << "\n";
  stream << "  logErrorDirectories: " << toString(m_logErrorDirs) << "\n";

  stream << "  autoCancelJobAfterTime: " << toString(m_cancelJobAfterTime)
         << "\n";
  if (m_cancelJobAfterTime) {
    stream << "  hoursForAutoCancelJob: " << m_hoursForCancelJobAfterTime
           << "\n";
  }

  stream << "\n=== Optimizer Settings\n";

  for (size_t i = 0; i < getNumOptSteps(); ++i) {
    stream << " OptStep " << i + 1 << "\n";
    const GlobalSearch::Optimizer* opt = optimizer(i);
    if (!opt) {
      stream << "  optimizer: NONE\n";
    } else {
      stream << "  optimizer: " << opt->getIDString() << "\n";
    }
  }

  // Write multi-objective (and exit) stuff
  stream << "\n=== Run Termination Settings\n";
  stream << "  softExit: " << toString(m_softExit) << "\n";
  stream << "\n=== Multi-Objective Settings\n";
  stream << "  objectivesNum: " << QString::number(getObjectivesNum()) << "\n";
  stream << "  objectivesReDo: " << toString(m_objectivesReDo) << "\n";
  for (int i = 0; i < getObjectivesNum(); i++)
    stream << "  objective" << i+1 << ": " << QString::number(getObjectivesTyp(i)) << "  "
      << getObjectivesExe(i) << "  " << getObjectivesOut(i) << "  " << getObjectivesWgt(i) << "\n";
  stream << "  calculateHardness: " << toString(m_calculateHardness) << "\n";
  if (m_calculateHardness) {
    stream << "  hardnessFitnessWeight: " << m_hardnessFitnessWeight << "\n";
  }

  stream << "\n====================================================\n";
}

void XtalOpt::setupRpcConnections()
{
  if (m_usingGUI && m_dialog) {
    connect(m_dialog, SIGNAL(moleculeChanged(GlobalSearch::Structure*)), this,
            SLOT(sendRpcUpdate(GlobalSearch::Structure*)));
  }
}

void XtalOpt::sendRpcUpdate(GlobalSearch::Structure* s)
{
  if (!m_rpcClient)
    return;

  Xtal* xtal = qobject_cast<Xtal*>(s);
  if (xtal)
    m_rpcClient->updateDisplayedXtal(*xtal);
}

void XtalOpt::readRuntimeOptions()
{
  XtalOptCLIOptions::readRuntimeOptions(*this);
}
} // end namespace XtalOpt

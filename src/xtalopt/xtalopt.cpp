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
#include <globalsearch/searchbase.h>
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
  : SearchBase(parent),
    using_randSpg(false), minXtalsOfSpg(QList<int>()),
    m_rpcClient(make_unique<XtalOptRpc>()),
    m_initWC(new SlottedWaitCondition(this))
{
  xtalInitMutex = new QMutex;
  m_idString = "XtalOpt";
  m_schemaVersion = 4;

  // Read the general settings
  readSettings();

  // Connections
  connect(m_queue, SIGNAL(structureFinished(GlobalSearch::Structure*)), this,
          SLOT(checkForSimilarities()));
  connect(this, SIGNAL(sessionStarted()), this, SLOT(resetSimilarities()));
  connect(this, &SearchBase::dialogSet, this, &XtalOpt::setupRpcConnections);
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
  if (compList.isEmpty()) {
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
               << "cli-runtime-options.txt file in the local working"
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
               << "change this in the cli-runtime-options.txt file in"
               << "the local working directory).\n";
    }
  };

#ifdef ENABLE_SSH
  // Create the SSHManager if running remotely
  if (!m_localQueue && anyRemoteQueueInterfaces()) {
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

  // Initialize loop variables
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
      for (int compi = 0; compi < compList.size(); compi++) {
        if (m_verbose) {
          qDebug() << "   startSearch : new xtal composition "
            << compList[compi].getFormula() << " from list.";
        }
        updateProgressBar(numInitial, newXtalCount + failed, newXtalCount);
        // Generate/Check xtal
        xtal = generateRandomXtal(1, newXtalCount + 1, compList[compi]);
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
  }
  // Perform a spacegroup generation generation
  else {
    // If minXtalsOfSpg was never correctly generated, just generate one
    // now
    if (minXtalsOfSpg.size() == 0) {
      for (size_t spg = 1; spg <= 230; spg++)
        minXtalsOfSpg.append(0);
    }

    QList<int> spgStillNeeded = minXtalsOfSpg;

    // Find the total number of xtals that will be generated
    // If minXtalsOfSpg specifies more xtals to be generated than
    // numInitial, then the total needed will be the sum from
    // minXtalsOfSpg

    size_t numXtalsToBeGenerated = 0;
    for (size_t i = 0; i < minXtalsOfSpg.size(); i++) {
      // The value in the vector is -1 if that spg is to not be used
      if (minXtalsOfSpg.at(i) != -1)
        numXtalsToBeGenerated +=
          (minXtalsOfSpg.at(i) * compList.size());
    }

    // Now that minXtalsOfSpg is set up, proceed!
    newXtalCount = failed = 0;
    // Find spacegroups for which we have required a certain number of xtals
    for (size_t i = 0; i < spgStillNeeded.size(); i++) {
      while (spgStillNeeded.at(i) > 0) {
        for (int compi = 0; compi < compList.size(); compi++) {
          // Update the progress bar
          updateProgressBar((numXtalsToBeGenerated - failed > numInitial
                               ? numXtalsToBeGenerated - failed
                               : numInitial),
                            newXtalCount + failed, newXtalCount);

          uint spg = i + 1;

          // If the spacegroup isn't possible, just continue
          if (!RandSpg::isSpgPossible(spg, getStdVecOfAtomsComp(compList[compi]))) {
            numXtalsToBeGenerated--;
            continue;
          }

          // Generate/Check xtal
          xtal = randSpgXtal(1, newXtalCount + 1, compList[compi], spg);
          if (!checkXtal(xtal)) {
            delete xtal;
            failed++;
          } else {
            if (m_verbose) {
              qDebug() << "   startSearch : new xtal composition "
                       << compList[compi].getFormula() << " randspg " << spg << " from list.";
            }
            xtal->findSpaceGroup(tol_spg);
            initializeAndAddXtal(xtal, 1, xtal->getParents());
            newXtalCount++;
          }
        }
        spgStillNeeded[i]--;
      }
    }
    // If we still haven't generated enough xtals, pick a random spg
    // and a random composition to be generated
    while (newXtalCount < numInitial) {
      // Let's keep the progress bar updated
      updateProgressBar(numInitial, newXtalCount + failed, newXtalCount);

      // Randomly select a composition from the list
      CellComp randomComp = pickRandomCompositionFromPossibleOnes();
      // Randomly select a possible spg
      uint randomSpg = pickRandomSpgFromPossibleOnes();
      // If it isn't possible, try again
      if (!RandSpg::isSpgPossible(randomSpg, getStdVecOfAtomsComp(randomComp)))
        continue;
      // Try it out
      xtal = randSpgXtal(1, newXtalCount + 1, randomComp, randomSpg);
      if (!checkXtal(xtal)) {
        delete xtal;
        failed++;
      } else {
        if (m_verbose) {
          qDebug() << "   startSearch : new xtal composition " << randomComp.getFormula()
                   << " randspg " << randomSpg << " chosen randomly.";
        }
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
    SearchBase::save(filename, notify);

  SETTINGS(filename);
  settings->beginGroup("xtalopt/init/");

  settings->setValue("version", m_schemaVersion);

  settings->setValue("verboseOutput", m_verbose);

  settings->setValue("maxAtoms", maxAtoms);
  settings->setValue("minAtoms", minAtoms);
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
  settings->setValue("limits/scaleFactor", scaleFactor);
  settings->setValue("limits/minRadius", minRadius);
  settings->setValue("limits/volume/min", vol_min);
  settings->setValue("limits/volume/max", vol_max);
  settings->setValue("limits/volume/scale_min", vol_scale_min);
  settings->setValue("limits/volume/scale_max", vol_scale_max);
  settings->setValue("limits/volume/elemental", input_ele_volm_string);
  settings->setValue("using/interatomicDistanceLimit",
                     using_interatomicDistanceLimit);
  settings->setValue("using/molUnit", using_molUnit);
  settings->setValue("using/customIAD", using_customIAD);
  settings->setValue("using/randSpg", using_randSpg);
  settings->setValue("using/checkStepOpt", using_checkStepOpt);

  // Composition, search type, and reference energies
  settings->setValue("saveHullSnapshots", m_saveHullSnapshots);
  settings->setValue("vcSearch", vcSearch);
  settings->setValue("referenceEnergies", input_ene_refs_string);
  settings->setValue("chemical_formulas", input_formulas_string);

  // Mol Unit stuff
  if (using_molUnit) {
    settings->beginWriteArray("compMolUnit");
    size_t ind = 0;
    for (const auto& pair : compMolUnit.keys()) {
      settings->setArrayIndex(ind);
      settings->setValue("center",
                         ElementInfo::getAtomicSymbol(pair.first).c_str());
      settings->setValue("number_of_centers", compMolUnit[pair].numCenters);
      settings->setValue("neighbor",
                         ElementInfo::getAtomicSymbol(pair.second).c_str());
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

  // Search settings tab
  settings->beginGroup("xtalopt/opt/");

  // Initial generation
  settings->setValue("opt/numInitial", numInitial);

  // Search parameters
  settings->setValue("opt/parentsPoolSize", parentsPoolSize);
  settings->setValue("opt/contStructs", contStructs);
  settings->setValue("opt/limitRunningJobs", limitRunningJobs);
  settings->setValue("opt/runningJobLimit", runningJobLimit);
  settings->setValue("opt/failLimit", failLimit);
  settings->setValue("opt/failAction", failAction);
  settings->setValue("opt/maxNumStructures", maxNumStructures);

  // Similarities
  settings->setValue("tol/xtalcomp/length", tol_xcLength);
  settings->setValue("tol/xtalcomp/angle", tol_xcAngle);
  settings->setValue("tol/spg", tol_spg);
  settings->setValue("tol/rdf/tolerance", tol_rdf);
  settings->setValue("tol/rdf/cutoff", tol_rdf_cutoff);
  settings->setValue("tol/rdf/nbins", tol_rdf_nbins);
  settings->setValue("tol/rdf/sigma", tol_rdf_sigma);

  // Crossover
  settings->setValue("opt/p_cross", p_cross);
  settings->setValue("opt/cross_ncuts", cross_ncuts);
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

  // Permutomic
  settings->setValue("opt/p_atomic", p_atomic);

  // Permucomp
  settings->setValue("opt/p_comp", p_comp);

  // Random Supercell
  settings->setValue("opt/p_supercell", p_supercell);

  settings->setValue("opt/softExit", m_softExit);
  settings->setValue("opt/localQueue", m_localQueue);

  settings->endGroup();

  settings->beginGroup("xtalopt/obj/");
  // Optimization Type
  settings->setValue("optimizationType", m_optimizationType);
  settings->setValue("tournamentSelection", m_tournamentSelection);
  settings->setValue("restrictedPool", m_restrictedPool);
  settings->setValue("crowdingDistance", m_crowdingDistance);
  settings->setValue("objectivePrecision", m_objectivePrecision);

  // Multi-objective
  settings->setValue("objectivesReDo", m_objectivesReDo);
  settings->beginWriteArray("objectives");
  for (size_t i = 0; i < getObjectivesNum(); i++) {
    settings->setArrayIndex(i);
    settings->setValue("exe",getObjectivesExe(i));
    settings->setValue("typ",getObjectivesTyp(i));
    settings->setValue("wgt",getObjectivesWgt(i));
    settings->setValue("out",getObjectivesOut(i));
  }
  settings->endArray();

  settings->endGroup();

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

  // As of xtalopt version 14, state files older than v4 will not work
  if (!filename.isEmpty() && loadedVersion < 4) {
    error("XtalOpt::readEditSettings(): Settings in file " + filename +
          " appears to be a run with an older version of XtalOpt. "
          "Please visit https://xtalopt.github.io for latest updates "
          "on XtalOpt and its input flag descriptions.");
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

  settings->endGroup();
  return true;
}

bool XtalOpt::readSettings(const QString& filename)
{
  // Some stuff, we only want to load if we are loading a state file
  //   and not a generic xtalopt saved settings
  bool isStateFile = filename.endsWith(".state") ||
                     filename.endsWith(".state.old");

  SETTINGS(filename);

  // Init (structure limit) tab
  settings->beginGroup("xtalopt/init/");
  int loadedVersion = settings->value("version", 0).toInt();

  // As of xtalopt version 14, state files older than v4 will not work
  if (!filename.isEmpty() && loadedVersion < 4) {
    error("XtalOpt::readSettings(): Settings in file " + filename +
          " appears to be a run with an older version of XtalOpt. "
          "Please visit https://xtalopt.github.io for latest updates "
          "on XtalOpt and its input flag descriptions.");
    return false;
  }

  m_verbose = settings->value("verboseOutput", false).toBool();
  maxAtoms = settings->value("maxAtoms", 20).toInt();
  minAtoms = settings->value("minAtoms", 1).toInt();
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
  vol_max = settings->value("limits/volume/max", 100).toDouble();
  vol_scale_max = settings->value("limits/volume/scale_max", 0.0).toDouble();
  vol_scale_min = settings->value("limits/volume/scale_min", 0.0).toDouble();
  scaleFactor = settings->value("limits/scaleFactor", 0.5).toDouble();
  minRadius = settings->value("limits/minRadius", 0.25).toDouble();
  using_interatomicDistanceLimit =
    settings->value("using/interatomicDistanceLimit", true).toBool();
  using_randSpg = settings->value("using/randSpg").toBool();
  using_checkStepOpt = settings->value("using/checkStepOpt").toBool();

  using_customIAD = settings->value("using/customIAD", false).toBool();
  using_molUnit = settings->value("using/molUnit", false).toBool();

  // Init setup
  if (isStateFile) {
    // Always read the composition stuff first right after radii entries are read!
    // Since these are saved while the run was proceeding, we assume they all have
    //   "valid" outputs; so won't check if they return true or false.
    input_formulas_string = settings->value("chemical_formulas").toString();
    processInputChemicalFormulas(input_formulas_string);
    input_ene_refs_string = settings->value("referenceEnergies").toString();
    processInputReferenceEnergies(input_ene_refs_string);
    input_ele_volm_string = settings->value("limits/volume/elemental").toString();
    processInputElementalVolumes(input_ele_volm_string);
    vcSearch = settings->value("vcSearch", false).toBool();
    m_saveHullSnapshots = settings->value("saveHullSnapshots", false).toBool();
  }

  // Molecular units
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
      centerNum = ElementInfo::getAtomicNum(center.trimmed().toStdString());
      QString strNumCenters = settings->value("number_of_centers").toString();
      numCenters = strNumCenters.toInt();
      QString neighbor = settings->value("neighbor").toString();
      neighborNum = ElementInfo::getAtomicNum(neighbor.trimmed().toStdString());
      QString strNumNeighbors =
          settings->value("number_of_neighbors").toString();
      numNeighbors = strNumNeighbors.toInt();
      QString strGeom = settings->value("geometry").toString();
      setGeom(geom, strGeom);
      dist = settings->value("distance").toDouble();
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

  settings->endGroup();

  // We have a separate function for reading the edit settings because
  //   the edit tab may need to call it
  if (!readEditSettings(filename))
    return false;

  // Optimization settings tab
  settings->beginGroup("xtalopt/opt/");

  // Initial generation
  numInitial = settings->value("opt/numInitial", 0).toInt();

  // Search parameters
  parentsPoolSize = settings->value("opt/parentsPoolSize", 20).toUInt();
  contStructs = settings->value("opt/contStructs", 15).toUInt();
  limitRunningJobs = settings->value("opt/limitRunningJobs", false).toBool();
  runningJobLimit = settings->value("opt/runningJobLimit", 1).toUInt();

  failLimit = settings->value("opt/failLimit", 1).toUInt();
  if (failLimit < 1)
    failLimit = 1;
  failAction = static_cast<SearchBase::FailActions>(
    settings->value("opt/failAction", XtalOpt::FA_Randomize).toUInt());
  maxNumStructures = settings->value("opt/maxNumStructures", 100).toInt();

  // Similarities
  tol_xcLength = settings->value("tol/xtalcomp/length", 0.1).toDouble();
  tol_xcAngle = settings->value("tol/xtalcomp/angle", 2.0).toDouble();
  tol_spg = settings->value("tol/spg", 0.01).toDouble();
  tol_rdf = settings->value("tol/rdf/tolerance", 0.0).toDouble();
  tol_rdf_cutoff = settings->value("tol/rdf/cutoff", 6.0).toDouble(); 
  tol_rdf_nbins = settings->value("tol/rdf/nbins", 3000).toInt();
  tol_rdf_sigma = settings->value("tol/rdf/sigma", 0.008).toDouble();

  // Random supercell
  p_supercell = settings->value("opt/p_supercell", 0).toUInt();

  // Permutomic
  p_atomic = settings->value("opt/p_atomic", 15).toUInt();

  // Permutomic
  p_comp = settings->value("opt/p_comp", 5).toUInt();

  // Crossover
  p_cross = settings->value("opt/p_cross", 35).toUInt();
  cross_ncuts  = settings->value("opt/cross_ncuts", 1).toUInt();
  cross_minimumContribution =
    settings->value("opt/cross_minimumContribution", 25).toUInt();

  // Stripple
  p_strip = settings->value("opt/p_strip", 25).toUInt();
  strip_strainStdev_min =
    settings->value("opt/strip_strainStdev_min", 0.5).toDouble();
  strip_strainStdev_max =
    settings->value("opt/strip_strainStdev_max", 0.5).toDouble();
  strip_amp_min = settings->value("opt/strip_amp_min", 0.5).toDouble();
  strip_amp_max = settings->value("opt/strip_amp_max", 1.0).toDouble();
  strip_per1 = settings->value("opt/strip_per1", 1).toUInt();
  strip_per2 = settings->value("opt/strip_per2", 1).toUInt();

  // Permustrain
  p_perm = settings->value("opt/p_perm", 25).toUInt();
  perm_strainStdev_max =
    settings->value("opt/perm_strainStdev_max", 0.5).toDouble();
  perm_ex = settings->value("opt/perm_ex", 4).toUInt();

  // softExit shouldn't be read in resuming; it causes the code to quit
  //   because the max number of structures is already met! Instead; always
  //   set it to false in resuming for which settings are being read here.
  m_softExit = false;
  //
  m_localQueue = settings->value("opt/localQueue", false).toBool();

  settings->endGroup();

  // Multi-objective tab
  settings->beginGroup("xtalopt/obj/");

  // Optimization Type
  m_optimizationType = settings->value("optimizationType", "basic").toString();
  m_tournamentSelection = settings->value("tournamentSelection", true).toBool();
  m_restrictedPool = settings->value("restrictedPool", false).toBool();
  m_crowdingDistance = settings->value("crowdingDistance", true).toBool();
  m_objectivePrecision = settings->value("objectivePrecision", -1).toInt();

  if (isStateFile) {
    // Multi-objective: we assume weights are already saved correctly
    m_objectivesReDo = settings->value("objectivesReDo", false).toBool();
    resetObjectives();
    int size = settings->beginReadArray("objectives");
    for (size_t i = 0; i < size; i++) {
      settings->setArrayIndex(i);
      setObjectivesTyp(ObjType(settings->value("typ","").toInt()));
      setObjectivesExe(settings->value("exe","").toString());
      setObjectivesWgt(settings->value("wgt",0.0).toDouble());
      setObjectivesOut(settings->value("out","").toString());
    }
    m_calculateObjectives = (size > 0) ? true : false;
    settings->endArray();
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
    warning("XtalOpt no longer checks seed xtals for user-defined "
            "geometrical constraints.");
    warningAlreadyDisplayed = true;
  }

  // For seed structures, we call check composition with "isSeed = true"
  //   where we perform a basic check and increase max atoms if needed.
  if (!optimizer(0)->read(xtal, filename) ||
      !this->checkComposition(xtal, true)) {
    error(tr("Error loading seed %1\n\n%2").arg(filename).arg(err));
    xtal->deleteLater();
    return false;
  }

  QString parents = QString("Seeded: %1").arg(filename);
  initializeAndAddXtal(xtal, 1, parents);
  debug(QString("XtalOpt::addSeed: loaded seed: %1").arg(filename));
  return true;
}

Structure* XtalOpt::replaceWithRandom(Structure* s, const QString& reason)
{
  Xtal* oldXtal = qobject_cast<Xtal*>(s);
  QWriteLocker locker1(&oldXtal->lock());

  // Randomly generated xtals do not have parent structures
  oldXtal->setParentStructure(nullptr);

  // Retrieve the composition of the original cell
  CellComp origComp = getXtalComposition(s);

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
      while (!RandSpg::isSpgPossible(spg, getStdVecOfAtomsComp(origComp)));

      xtal = randSpgXtal(generation, id, origComp, spg);
    } else {
      xtal = generateRandomXtal(generation, id, origComp);
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
    parents = QString("RandSpg: %1 [comp=%2] (%3)")
                  .arg(spg).arg(origComp.getFormula()).arg(HM_spg);
  }
  else {
    parents = QString("Randomly generated replacement (comp=%1)")
                  .arg(origComp.getFormula());
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

  // Retrieve the composition of the original cell
  CellComp origComp = getXtalComposition(s);

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
    xtal = generateNewXtal(origComp);
  }

  // Just return xtal if the formulas are not equivalent.
  // This should theoretically not occur since generateNewXtal(origComp) forces
  // xtal to have the correct composition.
  if (xtal->getChemicalFormula() != s->getChemicalFormula()) {
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

Xtal* XtalOpt::randSpgXtal(uint generation, uint id, CellComp incomp,
                           uint spg, bool checkSpgWithSpglib)
{
  Xtal* xtal = nullptr;

  // Let's make the spg input
  latticeStruct latticeMins, latticeMaxes;
  setLatticeMinsAndMaxes(latticeMins, latticeMaxes);

  // Create the input
  randSpgInput input(spg, getStdVecOfAtomsComp(incomp), latticeMins, latticeMaxes);

  // Add various other input options
  input.minRadius = minRadius;

  // FIXME: this is a mark!
  // At least on mac os, the ElemInfo/ElemInfoDatabase defined in randSpg where not
  //   resolved from those in XtalOpt. So, the following line would change the base
  //   radii, hence, minimum radii of atoms right after the first call to randSpg.
  //   As of XtalOpt14, these class/namespace are renamed to
  //   ElementInfo/ElementInfoDatabase in XtalOpt to avoid any such issues.
  input.IADScalingFactor = scaleFactor;

  getCompositionVolumeLimits(incomp, input.minVolume, input.maxVolume);

  input.maxAttempts = 10;
  input.verbosity = 'n';
  // This removes the guarantee that we will generate the right
  //   space group, but we will just check it with spglib
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

  // Make sure we don't call xtal->getSpaceGroupNumber() until we
  //   know that we have an xtal
  if (xtal) {
    if (checkSpgWithSpglib && xtal->getSpaceGroupNumber() != spg) {
      delete xtal;
      xtal = 0;
    }
  }

  // We need to set these things before checkXtal() is called
  if (xtal) {
    xtal->setStatus(Xtal::WaitingForOptimization);
  } else {
    qDebug() << "After" << QString::number(input.maxAttempts)
             << "attempts, failed to generate an xtal with spg of"
             << QString::number(spg) << "and composition of"
             << incomp.getFormula();

    return nullptr;
  }

  QString HM_spg = Xtal::getHMName(spg);

  // Set up xtal data
  xtal->setGeneration(generation);
  xtal->setIDNumber(id);
  xtal->setParents(QString("RandSpg Init: %1 [comp=%2] (%3)")
                       .arg(spg).arg(incomp.getFormula()).arg(HM_spg));
  return xtal;
}

Xtal* XtalOpt::generateEmptyXtalWithLattice(CellComp incomp)
{
  Xtal* xtal = nullptr;
  int attemptCount = 1;
  do {
    // This is just to let the user know in case we are stuck here
    //   without flooding the output!
    if ((attemptCount % 100000) == 0) {
      qDebug() << "Attempts in generateEmptyXtalWithLattice: "
               << QString::number(attemptCount);
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
  } while (!checkLattice(xtal));

  // Set the volume
  double minvol, maxvol;
  getCompositionVolumeLimits(incomp, minvol, maxvol);
  xtal->setVolume(getRandDouble(minvol, maxvol));

  return xtal;
}

Xtal* XtalOpt::generateRandomXtal(uint generation, uint id, CellComp incomp)
{
  // Create a valid lattice first
  Xtal* xtal = generateEmptyXtalWithLattice(incomp);

  // Cache these for later use
  double a = xtal->getA();
  double b = xtal->getB();
  double c = xtal->getC();

  QWriteLocker locker(&xtal->lock());

  xtal->setStatus(Xtal::Empty);

  // Populate crystal
  QList<uint> atomicNums = incomp.getAtomicNumbers();
  // Sort atomic number by decreasing minimum radius. Adding the "larger"
  // atoms first encourages a more even (and ordered) distribution
  for (int i = 0; i < atomicNums.size() - 1; ++i) {
    for (int j = i + 1; j < atomicNums.size(); ++j) {
      if (this->eleMinRadii.getMinRadius(atomicNums[i]) <
          this->eleMinRadii.getMinRadius(atomicNums[j])) {
        atomicNums.swap(i, j);
      }
    }
  }

  unsigned int atomicNum;
  int qRand;

  if (using_customIAD) {
    for (int num_idx = 0; num_idx < atomicNums.size(); num_idx++) {
      atomicNum = atomicNums.at(num_idx);
      int q = incomp.getCount(atomicNum);
      for (uint i = 0; i < q; i++) {
        if (!xtal->addAtomRandomlyIAD(atomicNum, this->eleMinRadii,
                                      this->interComp, 1000)) {
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
          if (!xtal->addAtomRandomly(key.first, key.second,
                                     this->eleMinRadii,
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
      qRand = incomp.getCount(atomicNum);

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
          if (!xtal->addAtomRandomly(atomicNum, this->eleMinRadii)) {
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
              if (!xtal->addAtomRandomly(atomicNum, key.second,
                                         this->eleMinRadii,
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

  // Set up genealogy info
  xtal->setGeneration(generation);
  xtal->setIDNumber(id);
  xtal->setParents(tr("Randomly generated (comp=%1)").arg(incomp.getFormula()));
  xtal->setStatus(Xtal::WaitingForOptimization);

  // Set up xtal data
  return xtal;
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
      error(QString("XtalOpt::initializeAndAddXtal: Cannot write to path: %1 ")
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
  // This function is being used to generate new structures.
  // We choose a random composition each time it's called.

  CellComp randomComp = pickRandomCompositionFromPossibleOnes();

  Xtal* newXtal = generateNewXtal(randomComp);
  initializeAndAddXtal(newXtal, newXtal->getGeneration(),
                       newXtal->getParents());
}

Xtal* XtalOpt::generateNewXtal(CellComp incomp)
{
  // A sanity check; although this shouldn't happen!
  if (incomp.getNumAtoms() == 0) {
    qDebug() << "Failed in generateNewXtal1 with empty composition";
    return nullptr;
  }

  QList<Structure*> structures;

  QReadLocker trackerLocker(m_tracker->rwLock());

  // Just remove non-optimized structures
  structures = m_queue->getAllParentPoolStructures();

  // Try to get it from the probability list only if we have large
  //   enough parents pool. Otherwise, generate randomly.
  if (structures.size() < 3) {
    Xtal* xtal = 0;

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
      xtal = generateRandomXtal(1, 0, incomp);
    }
    xtal->setParents(xtal->getParents());
    return xtal;
  }

  Xtal* xtal = generateEvolvedXtal_H(structures);
  return xtal;
}

XtalOpt::Operators XtalOpt::selectOperation(bool validComp)
{
  // In this function we start by relative operation weights given
  //   by the user, and will try to find an appropriate genetic operation.
  //
  // Our general considerations are applied in the following order:
  // (1) non-vcSearch: we exclude permutomic and permucomp
  // (2) non-valid parent composition: we exclude stripple and permustrain
  //
  // Here, we:
  //   - decide about the fallback option,
  //   - create helper lists of operators, input weights (and set any negative
  //     weights to zero) and masks for the above conditions,
  //   - combine masks to figure out what operations are allowed,
  //   - normalize the weights to 1.0 for allowed operations while zeroing the rest,
  //   - and finally choose a random number and select/return operator accordingly.

  // In case of any failure, we return crossover since it's always applicable.
  Operators fallback = OP_Crossover;

  // IMPORTANT: IN BUILDING THE FOLLOWING 4 VECTORS; MAKE SURE THAT ALWAYS:
  //   THE LIST OF WEIGHTS (ops_weight) AND LISTS OF APPLIED CONDITIONS
  //   (allow_search and allow_valid) ARE CREATED IN THE SAME ORDER AS
  //   THE LIST OF OPERATORS (ops_list).

  // Build list of operators
  std::vector<Operators> ops_list = {OP_Stripple, OP_Permustrain,
                                     OP_Permutomic, OP_Permucomp,
                                     OP_Crossover};
  // Build list of user weights (in the order of operators list)
  std::vector<double> ops_weight = {
      static_cast<double>(p_strip  >= 0 ? p_strip  : 0),
      static_cast<double>(p_perm   >= 0 ? p_perm   : 0),
      static_cast<double>(p_atomic >= 0 ? p_atomic : 0),
      static_cast<double>(p_comp   >= 0 ? p_comp   : 0),
      static_cast<double>(p_cross  >= 0 ? p_cross  : 0)
  };
  // Build list of allowed‐by‐searchtype conditions (in the order of operators list)
  std::vector<bool> allow_search = { true, true, vcSearch, vcSearch, true };
  // Build list of allowed‐by‐validcomp conditions (in the order of operators list)
  std::vector<bool> allow_valid  = { validComp, validComp, true, true, true };

  int ops_num = ops_weight.size();

  // Now combine masks, find allowed ops, and zero out weights for the rest
  std::vector<bool> allowed(ops_num, false);
  int num_allowed = 0;
  for (int i = 0; i < ops_num; ++i) {
    allowed[i] = allow_search[i] && allow_valid[i];
    if (allowed[i])
      num_allowed++;
    else
      ops_weight[i] = 0.0;
  }

  // Sanity check: are we left with any allowed operations?
  if (num_allowed == 0) {
    qDebug() << "\n*************************************************************";
    qDebug() << "*** Warning: unexpected op weights!!! Selecting crossover ***";
    qDebug() << "*************************************************************";
    return fallback;
  }

  // Find total weight of allowed ops: since we have set the weight for
  //   the rest to zero, a simple sum is fine.
  double total = std::accumulate(ops_weight.begin(), ops_weight.end(), 0.0);

  // Normalize the weights for allowed ops, while leaving the rest zero.
  // If total weight of allowed ops is zero, make them normalized-equal;
  //   otherwise normalize their weight using the total.
  for (int i = 0; i < ops_num; i++) {
    if (allowed[i])
      ops_weight[i] = (total > 0.0) ? ops_weight[i]/total : 1.0/num_allowed;
  }

  if (m_verbose) {
    qDebug().noquote() <<
    QString("   Operation chances: stri %1 perm %2 atom %3 comp %4 cros %5")
            .arg(ops_weight[0], 5, 'f', 2).arg(ops_weight[1], 5, 'f', 2).arg(ops_weight[2], 5, 'f', 2)
            .arg(ops_weight[3], 5, 'f', 2).arg(ops_weight[4], 5, 'f', 2);
  }

  // Perform the selection
  Operators op = fallback;

  double r = getRandDouble();

  double cumr = 0.0;
  for (int i = 0; i < ops_num; i++) {
    cumr += ops_weight[i];
    if (r < cumr) {
      op = ops_list[i];
      break;
    }
  }

  // Return the selected operation
  return op;
}

Xtal* XtalOpt::generateEvolvedXtal_H(QList<Structure*>& structures, Xtal* preselectedXtal)
{
  // preselectedXtal is nullptr by default

  // Initialize loop vars
  unsigned int gen;
  QString parents;
  Xtal *xtal = nullptr, *selectedXtal = nullptr;

  // Shouldn't happen; but just in case ...
  if (!preselectedXtal && structures.size() == 0) {
    qDebug() << "Warning: empty pool and no preselected xtal in generateEvolvedXtal_H (0)!";
    return nullptr;
  }

  // Also, here we determine the chances of generating a random supercell.
  double wSupr;
  if (p_supercell > 100) {
    wSupr = 1.0;
  } else {
    wSupr = static_cast<double>(p_supercell) / 100.0;
  }

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
      qDebug() << "Failed too many times in generateEvolvedXtal_H1. Giving up";
      return nullptr;
    }
    ++attemptCount;

    // If an xtal hasn't been preselected, select one
    if (!preselectedXtal)
      selectedXtal = selectXtalFromProbabilityList(structures);
    else
      selectedXtal = preselectedXtal;

    // Specially, the probability selection might fail and we get null pointer
    if (!selectedXtal) {
      qDebug() << "Warning: selecting xtal failed in generateEvolvedXtal_H (1)!";
      return nullptr;
    }

    // Decide operator
    // As of XtalOpt v14, we read "operation weights" instead of their percentages,
    //   and decide the chance of applying operators at the time of selection; based
    //   on the run condition (if variable-comp or if parent has invalid composition).
    // The following takes into account these conditions, and returns a randomly
    //   selected operator based on the user-specified relative weights.
    Operators op = selectOperation(selectedXtal->hasValidComposition());

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
      case OP_Permutomic:
        opStr = "permutomic";
        break;
      case OP_Permucomp:
        opStr = "permucomp";
        break;
      default:
        opStr = "(unknown)";
        break;
    }
    if (m_verbose)
      qDebug() << "   Operator selected " << opStr
               << " for parent " << selectedXtal->getTag();

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
        qDebug() << "Failed too many times in generateEvolvedXtal_H2. Giving up";
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

          xtal1 = selectedXtal;
          xtal2 = selectXtalFromProbabilityList(structures);

          // The probability selection might fail and we get null pointer
          if (!xtal2) {
            qDebug() << "Warning: selecting xtal failed in generateEvolvedXtal_H (2)!";
            return nullptr;
          }

          // Perform operation
          xtal = XtalOptGenetic::crossover(xtal1, xtal2, this->compList, this->eleMinRadii,
                                           cross_ncuts, cross_minimumContribution,
                                           percent1, percent2,
                                           minAtoms, maxAtoms, vcSearch, m_verbose);

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

          parents = QString("Crossover: %1x%2 (%3%) + %4x%5 (%6%)")
                      .arg(gen1)
                      .arg(id1)
                      .arg(percent1, 0, 'f', 0)
                      .arg(gen2)
                      .arg(id2)
                      .arg(percent2, 0, 'f', 0);
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

          if (xtal)
            xtal->setParentStructure(selectedXtal);
          selectedXtal->lock().unlock();

          // Determine generation number
          gen = gen1 + 1;
          // A regular mutation is being performed
          parents = QString("Stripple: %1x%2 stdev=%3 amp=%4 waves=%5,%6")
            .arg(gen1)
            .arg(id1)
            .arg(stdev, 0, 'f', 5)
            .arg(amplitude, 0, 'f', 5)
            .arg(strip_per1)
            .arg(strip_per2);
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

          if (xtal)
            xtal->setParentStructure(selectedXtal);
          selectedXtal->lock().unlock();

          // Determine generation number
          gen = gen1 + 1;
          // Set the ancestry like normal...
          parents = QString("Permustrain: %1x%2 stdev=%3 exch=%4")
            .arg(gen1)
            .arg(id1)
            .arg(stdev, 0, 'f', 5)
            .arg(perm_ex);
          continue;
        }
        case OP_Permutomic: {
          xtal = XtalOptGenetic::permutomic(selectedXtal, this->compList[0], this->eleMinRadii,
                                            minAtoms, maxAtoms, m_verbose);

          // Lock parent and extract info
          selectedXtal->lock().lockForRead();
          uint gen1 = selectedXtal->getGeneration();
          uint id1 = selectedXtal->getIDNumber();

          if (xtal)
            xtal->setParentStructure(selectedXtal);
          selectedXtal->lock().unlock();

          // Determine generation number
          gen = gen1 + 1;
          // Set the ancestry like normal...
          parents = QString("Permutomic: %1x%2 (%3-%4)")
            .arg(gen1)
            .arg(id1)
            .arg(selectedXtal->getCompositionString(false))
            .arg(xtal->getCompositionString(false));
          continue;
        }
        case OP_Permucomp: {
          xtal = XtalOptGenetic::permucomp(selectedXtal, this->compList[0], this->eleMinRadii,
                                           minAtoms, maxAtoms, m_verbose);

          // Lock parent and extract info
          selectedXtal->lock().lockForRead();
          uint gen1 = selectedXtal->getGeneration();
          uint id1 = selectedXtal->getIDNumber();

          if (xtal)
            xtal->setParentStructure(selectedXtal);
          selectedXtal->lock().unlock();

          // Determine generation number
          gen = gen1 + 1;
          // Set the ancestry like normal...
          parents = QString("Permucomp: %1x%2 (%3-%4)")
            .arg(gen1)
            .arg(id1)
            .arg(selectedXtal->getCompositionString(false))
            .arg(xtal->getCompositionString(false));
          continue;
        }
        default:
          warning("XtalOpt::generateEvolvedXtal_H: Attempt to use an "
                  "invalid operator.");
      }
    }
    if (attemptCount >= 1000) {
      warning(tr("Unable to perform operation %1 after 1000 tries. "
                 "Reselecting operator...")
                .arg(opStr));
    }
  }

  xtal->setGeneration(gen);
  xtal->setParents(parents);
  Xtal* parentXtal = qobject_cast<Xtal*>(xtal->getParentStructure());
  xtal->setParentStructure(parentXtal);

  // This is not a genetic operation, per se. By a user-defined chance (0-100)
  //   of "p_supercell", we try to generate a supercell with a randomly chosen
  //   expansion factor, with up to maximum number of atoms.
  // If this worked, we return the supercell. Otherwise, we just return the
  //   original generated cell.
  // We have already converted user input to a chance in [0,1] range in "wSupr".
  double s = getRandDouble();
  if (s < wSupr) {
    Xtal* supercellXtal = generateSuperCell(xtal, 0, true);
    if (supercellXtal)
      return supercellXtal;
  }

  return xtal;
}

// This always returns a dynamically allocated xtal
// Callers take ownership of the pointer
Xtal* XtalOpt::generateSuperCell(Xtal* inXtal, uint expansion, bool distort)
{
  // This function will generate a supercell out of the input cell.
  // It can be called in two modes:
  //   (1) expansion > 0: make unit cell with that many times the atoms,
  //   (2) expansion = 0: randomly choose a factor, and generate a supercell.
  //
  // If "distort" is true; randomly distorts an atom in the final supercell.
  //
  // If expansion is given, we check to see if it complies with max atoms.
  // If we pick this factor, we make sure it is compatible with max atoms.

  // A basic sanity check
  if (!inXtal)
    return nullptr;

  uint initNumAtoms = inXtal->numAtoms();
  uint finalExpansion = expansion;

  if (finalExpansion > 0) {
    // If pre-defined factor, make sure it's good.
    if (static_cast<double>(finalExpansion) * initNumAtoms > maxAtoms)
      return nullptr;
  } else {
    // If not, try to find a proper one randomly.
    int maxPossibleExpansion = std::floor(maxAtoms / initNumAtoms);
    if (maxPossibleExpansion < 2)
      return nullptr;
    finalExpansion = getRandInt(2, maxPossibleExpansion);
  }

  // This is the return xtal
  Xtal* xtal = new Xtal;

  // Lock the parent xtal for reading
  QReadLocker parentXtalLocker(&inXtal->lock());

  // Copy info over from input to new xtal
  xtal->setCellInfo(inXtal->unitCell().cellMatrix());
  const std::vector<Atom>& atoms = inXtal->atoms();
  for (const auto& atom : atoms)
    xtal->addAtom(atom.atomicNumber(), atom.pos());
  uint gen = inXtal->getGeneration();
  QString parents = inXtal->getParents();
  Xtal* parentXtal = qobject_cast<Xtal*>(inXtal->getParentStructure());
  parentXtalLocker.unlock();

  // Keep performing the supercell generator until we are at the correct size.
  // We have already checked that the target cell will comply with the max atoms.

  // The current expansion factor of the generated supercell.
  uint factor = 1;
  while (factor != finalExpansion) {
    // This never happens; just in case!
    if (xtal->numAtoms() / initNumAtoms != std::floor(xtal->numAtoms() / initNumAtoms))
      return nullptr;

    factor = xtal->numAtoms() / initNumAtoms;

    // Find the largest prime number multiple. We will expand
    // upon the shortest length with this number.
    uint numberOfDuplicates = finalExpansion / factor;
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


  // Distort an atom?
  if (distort) {
    // pick a random atom
    uint ratom = getRandUInt(0, xtal->numAtoms() - 1);
    Atom atom = xtal->atoms().at(ratom);
    int atomicNumber = atom.atomicNumber();
    // try to distort it's position
    xtal->moveAtomRandomlyIAD(atomicNumber, this->eleMinRadii, this->interComp, 1000, &atom);
  }

  // Set the new xtal stuff
  xtal->setGeneration(gen);
  parents=QString("Supercell[%1]-").arg(factor)+parents;
  xtal->setParents(parents);
  xtal->setParentStructure(parentXtal);

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
  QString parents = QString("Primitive of %1x%2")
                      .arg(xtal->getGeneration())
                      .arg(xtal->getIDNumber());
  nxtal->setGeneration(gen);
  nxtal->setParents(parents);
  nxtal->setEnthalpy(xtal->getEnthalpy());
  nxtal->setEnergy(xtal->getEnergy());
  nxtal->setPrimitiveChecked(true);
  nxtal->setSkippedOptimization(true);

  nxtal->resetStrucObj();
  nxtal->setStrucObjState(xtal->getStrucObjState());
  nxtal->setStrucObjValuesVec(xtal->getStrucObjValuesVec());

  nxtal->setStatus(Xtal::Optimized);
  return nxtal;
}

Xtal* XtalOpt::selectXtalFromProbabilityList(QList<Structure*> structures)
{
  // Basically, this function is called only from generateEvolvedXtal_H
  //   (twice!), where the input "structures" is a proper parent pool.
  //
  // That's, "structures" is a set of at least 3 structures that are
  //   optimized, their hull and objectives (if needed) are calculated,
  //   and contain subsystem seeds only if user has allowed it.
  //
  // Still, we will put some safeguards in place (e.g., if list is empty
  //   or empty probs are returned), which result in returning a null pointer.

  if (parentsPoolSize == 0) {
    error("Error: parents pool size is zero for probability selection!");
    return nullptr;
  }

  if (structures.isEmpty()) {
    error("Error: no structure provided for probability selection!");
    return nullptr;
  }

  if (structures.size() == 1)
    warning("Warning: probability selection has only one structure!");

  // Select the parent xtal: this will return the index of the chosen parent
  //   in the list of structures (-1 if anything goes wrong).
  int ind = selectParentFromPool(structures, parentsPoolSize);

  // This shouldn't happen; but just in case ...
  if (ind == -1 ) {
    error("Error: probability selection didn't return any results!");
    return nullptr;
  }

  Xtal* xtal = qobject_cast<Xtal*>(structures[ind]);

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

  return true;
}

bool XtalOpt::checkComposition(Xtal* xtal, bool isSeed)
{
  // This function checks if the composition (atom types and counts)
  //   of a given xtal is "valid".
  // Valid means that it doesn't have unknown element types, and
  //   depending on the search type it has a proper composition, i.e.,
  //   for FC/MC searches, the composition is listed.
  // Also, except than seed structures, it makes sure that the xtal
  //   does not have any zero atom counts, and the total counts does
  //   not exceed the max atom parameter.
  // For seed structures, we allow for zero atom counts, and raise the
  //   max atoms if needed.

  // Basic sanity checks
  if (!xtal || xtal->numAtoms() <= 0) {
    qDebug() << "Error checkComposition: empty xtal.";
    return false;
  }
  if (compList.isEmpty()) {
    qDebug() << "Error checkComposition: composition is not set.";
    return false;
  }

  QStringList chemSystem = getChemicalSystem();

  // Input xtal's info (composition, symbol list, atom counts, etc)
  CellComp comp = getXtalComposition(xtal);
  QList<QString> symbols = comp.getSymbols();
  uint numAtoms = comp.getNumAtoms();

  // Perform a series of checks
  bool hasExtraAtomCount = false;
  bool hasLowAtomCount = false;
  bool hasExtraTypes = false;
  bool hasMissingTypes = false;
  bool compositionIsNew = true;

  // Is there any species not defined in chemical system?
  for(int i = 0; i < symbols.size(); i++)
    if (!chemSystem.contains(symbols[i]))
      hasExtraTypes = true;

  // Does the total atoms count exceed the maximum number of atoms?
  if (numAtoms > maxAtoms)
    hasExtraAtomCount = true;

  // Does the total atoms count exceed the maximum number of atoms?
  if (numAtoms < minAtoms)
    hasLowAtomCount = true;

  // Is there any species of chemical system that is not present?
  for(int i = 0; i < chemSystem.size(); i++)
    if (!symbols.contains(chemSystem[i]))
      hasMissingTypes = true;

  // Is "chemical composition" equivalent to any on the initial list?
  // (We don't force it to be "exact" match with the list because
  //    random supercells generated in the run are accepted too).
  for(int i = 0; i < compList.size(); i++) {
    if (compareCompositions(compList[i], comp) != 0.0) {
      compositionIsNew = false;
      break;
    }
  }

  // Now, according to run type and the above tests, see if xtal is ok.

  // Having extra types is always a no.
  if (hasExtraTypes) {
      qDebug() << "Error checkComposition: unknown types in the xtal.";
      return false;
    }

  // If a seed structure; we're done except than:
  // (1) if seed is a sub-system or it's composition is not on the list, mark it
  //     so we can manage for a proper genetic operation selection.
  // (2) if seed is acceptable, reset the min/max atom counts if needed
  if (isSeed) {
    if (hasMissingTypes || compositionIsNew)
      xtal->setCompositionValidity(false);
    if (hasExtraAtomCount) {
      maxAtoms = numAtoms;
      qDebug() << "Warning checkComposition: increased maxAtoms to " << maxAtoms;
    }
    if (hasLowAtomCount) {
      minAtoms = numAtoms;
      qDebug() << "Warning checkComposition: decreased minAtoms to " << minAtoms;
    }
    return true;
  }

  // Except than seeds, for now, we won't handle "sub-system" structures.
  if (hasMissingTypes) {
    qDebug() << "Error checkComposition: some atomic types are missing.";
    return false;
  }

  // No structure can have more atoms than maxAtoms.
  if (hasExtraAtomCount) {
    qDebug() << "Error checkComposition: number of atoms exceeds the maxAtoms.";
    return false;
  }

  // No structure can have fewer atoms than minAtoms.
  if (hasLowAtomCount) {
    qDebug() << "Error checkComposition: number of atoms lower than minAtoms.";
    return false;
  }

  // For a variable-composition search, we're good.
  if (vcSearch) {
    // FIXME: this is a "mark"! In general, here we can add "new" compositions
    //   to the list of compositions and/or formulae if we wanted! E.g.,
    //formulas_input += "," + form;
    //compList.append(incomp);
    return true;
  }

  // So the search is either fixed-/multi-composition;
  //   that's, composition should be on the list.
  if (compositionIsNew) {
    qDebug() << "Error checkComposition: composition doesn't match any of the list.";
    return false;
  }

  // If we make it here, then xtal has an acceptable composition.
  return true;
}

// Xtal should be write-locked before calling this function
bool XtalOpt::checkLattice(Xtal* xtal)
{
  if (xtal->numAtoms() > 0) {
    // Start with volume check, which is done only for non-empty xtals.
    //   Empty lattices have their volume adjusted separately.
    CellComp xtalComp = getXtalComposition(xtal);
    // The current (original) volume
    double org_vol = xtal->getVolume();

    // First, find volume limits for the structure
    double minvol, maxvol;
    getCompositionVolumeLimits(xtalComp, minvol, maxvol);

    // Check volume
    if (xtal->getVolume() < minvol || // PSA
        xtal->getVolume() > maxvol) { // PSA
      double newvol = getRandDouble(minvol, maxvol);
      // If the user has set vol_min to 0, we can end up with a null
      // volume. Fix this here. This is just to keep things stable
      // numerically during the rescaling -- it's unlikely that other
      // cells with small, nonzero volumes will pass the other checks
      // so long as other limits are reasonable.
      if (fabs(newvol) < 1.0) {
        newvol = (maxvol - minvol) * 0.5 + minvol; // PSA;
      }
      xtal->setVolume(newvol);
    }

    if (m_verbose) {
      double new_vol = xtal->getVolume();
      if (fabs(org_vol - new_vol) > ZERO2)
        qDebug().noquote() <<
            QString("   volume fixed - ori %1   new %2   (%3 - %4) %5")
                                  .arg(org_vol,9,'f',2).arg(new_vol,9,'f',2)
                                  .arg(minvol,9,'f',2).arg(maxvol,9,'f',2)
                                  .arg(xtal->getCompositionString());
    }
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
    QString out0 =
       QString("Discarding structure %1: determinant of unit cell negative or zero")
       .arg(xtal->getTag());
    if (m_verbose) {
      out0 += QString("\n");
      for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++)
          out0 += QString("   %1 ")
                      .arg(xtal->unitCell().cellMatrix()(i,j),12,'f',6);
        out0 += QString("\n");
      }
    }
    qDebug().noquote() << out0;

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
    qDebug() << QString("Discarding structure %1: a cell parameter is either 0, nan, or inf.")
                .arg(xtal->getTag());

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
      qDebug() << "Discarding structure " << xtal->getTag()
               << ": ratio of two lengths is 25x or larger"
               << xtal->getA() << xtal->getB() << xtal->getC();

      return false;
    }
    // Check that the angles aren't 25x different than the others as well
    if (xtal->getAlpha() * cutoff < xtal->getBeta() ||
        xtal->getAlpha() * cutoff < xtal->getGamma() ||
        xtal->getBeta() * cutoff < xtal->getAlpha() ||
        xtal->getBeta() * cutoff < xtal->getGamma() ||
        xtal->getGamma() * cutoff < xtal->getAlpha() ||
        xtal->getGamma() * cutoff < xtal->getBeta()) {
      qDebug() << "Discarding structure " << xtal->getTag()
               << ": ratio of two angles is 25x or larger"
               << xtal->getAlpha() << xtal->getBeta() << xtal->getGamma();

      return false;
    }

    xtal->fixAngles();
  }

  // Check lattice
  if ((!a && (xtal->getA() < a_min || xtal->getA() > a_max)) ||
      (!b && (xtal->getB() < b_min || xtal->getB() > b_max)) ||
      (!c && (xtal->getC() < c_min || xtal->getC() > c_max)) ||
      (!alpha && (xtal->getAlpha() < alpha_min || xtal->getAlpha() > alpha_max)) ||
      (!beta  && (xtal->getBeta() < beta_min || xtal->getBeta() > beta_max)) ||
      (!gamma && (xtal->getGamma() < gamma_min || xtal->getGamma() > gamma_max))) {
    QString out0 = QString("Discarding structure %1: bad lattice").arg(xtal->getTag());
    if (m_verbose) {
      out0 += QString("\n       A:    %1  %2  %3\n       B:    %4  %5  %6"
                      "\n       C:    %7  %8  %9\n   Alpha:    %10  %11  %12"
                      "\n    Beta:    %13  %14  %15\n   Gamma:    %16  %17  %18\n")
              .arg(a_min,12,'f').arg(xtal->getA(),12,'f').arg(a_max,12,'f')
              .arg(b_min,12,'f').arg(xtal->getB(),12,'f').arg(b_max,12,'f')
              .arg(c_min,12,'f').arg(xtal->getC(),12,'f').arg(c_max,12,'f')
              .arg(alpha_min,12,'f').arg(xtal->getAlpha(),12,'f').arg(alpha_max,12,'f')
              .arg(beta_min,12,'f').arg(xtal->getBeta(),12,'f').arg(beta_max,12,'f')
              .arg(gamma_min,12,'f').arg(xtal->getGamma(),12,'f').arg(gamma_max,12,'f');
    }
    qDebug().noquote() << out0;

    return false;
  }

  // We made it!
  return true;
}

bool XtalOpt::checkXtal(Xtal* xtal)
{
  // In this function, we always assume that we have a valid input xtal
  QString err;
  if (!xtal) {
    err = "Xtal pointer is nullptr.";
    return false;
  }

  // Lock xtal
  QWriteLocker locker(&xtal->lock());

  if (xtal->getStatus() == Xtal::Empty) {
    err = "Xtal status is empty.";
    return false;
  }

  if (!checkLattice(xtal))
    return false;

  if (!checkComposition(xtal))
    return false;

  // Sometimes, all the atom positions are set to 'nan' for an unknown reason
  // Make sure that the position of the first atom is not nan
  if (GS_IS_NAN_OR_INF(xtal->atoms().at(0).pos().x())) {
    qDebug() << QString("Discarding structure %1: contains 'nan' atom positions")
                .arg(xtal->getTag());
    return false;
  }

  // Never accept the structure if two atoms are basically on top of one
  // another
  for (size_t i = 0; i < xtal->numAtoms(); ++i) {
    for (size_t j = i + 1; j < xtal->numAtoms(); ++j) {
      if (fuzzyCompare(xtal->atom(i).pos(), xtal->atom(j).pos())) {
        qDebug() << QString("Discarding structure %1: two atoms are basically on "
                            "top of one another. This can confuse some "
                            "optimizers.").arg(xtal->getTag());
        return false;
      }
    }
  }

  // Check interatomic distances
  if (using_interatomicDistanceLimit) {
    int atom1, atom2;
    double IAD;
    if (!xtal->checkInteratomicDistances(this->eleMinRadii,
                                         &atom1, &atom2, &IAD)) {
      Atom& a1 = xtal->atom(atom1);
      Atom& a2 = xtal->atom(atom2);
      const double minIAD = this->eleMinRadii.getMinRadius(a1.atomicNumber()) +
                            this->eleMinRadii.getMinRadius(a2.atomicNumber());

      qDebug() << "Discarding structure " << xtal->getTag() << ": bad IAD ("
               << IAD << " < " << minIAD << ")";
      err = "Two atoms are too close together.";
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
      qDebug() << "Discarding structure " << xtal->getTag() << ": bad post-opt IAD ("
               << IAD << " < " << minIAD << ")";
      err = "Two atoms are too close together (post-optimization).";
      return false;
    }
  }

  // Xtal is OK!
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
            if (xtal->moveAtomRandomlyIAD(atomicNumber, this->eleMinRadii,
                                          this->interComp, 1000, atom)) {
              continue;
            } else {
              const double minIAD =
                this->interComp
                  .value(qMakePair<int, int>(a1.atomicNumber(), atomicNumber))
                  .minIAD;
              s->setStatus(Xtal::Killed);

              qDebug() << "Discarding structure " << xtal->getTag()
                       << ": bad Custom IAD (" << IAD << " < " << minIAD
                       << ") - couldn't fix";
              return false;
            }
          } else {
            const double minIAD = this->interComp
                                    .value(qMakePair<int, int>(
                                      a1.atomicNumber(), a2.atomicNumber()))
                                    .minIAD;
            s->setStatus(Xtal::Killed);

              qDebug() << "Discarding structure " << xtal->getTag()
                       << ": bad Custom IAD (" << IAD << " < " << minIAD
                       << ") - exceeded the number of fixes";
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
      if (!xtal->checkInteratomicDistances(this->eleMinRadii, &atom1, &atom2, &IAD)) {
        Atom& a1 = xtal->atom(atom1);
        Atom& a2 = xtal->atom(atom2);
        const double minIAD = this->eleMinRadii.getMinRadius(a1.atomicNumber()) +
                              this->eleMinRadii.getMinRadius(a2.atomicNumber());

        qDebug() << "Discarding structure " << xtal->getTag()
                 << ": bad IAD (" << IAD << " < " << minIAD << ")";
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
      rep += (QString(ElementInfo::getAtomicSymbol((*it).atomicNumber()).c_str()) +
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
        QString::number(ElementInfo::getAtomicNum(symbols[i].toStdString())) + " ";
      rep += symbols[i] + "\n";
    }
  } else if (line == "atomicCoordsAndAtomicSpecies") {
    const std::vector<Atom>& atoms = xtal->atoms();
    std::vector<Atom>::const_iterator it;
    QList<QString> symbol = xtal->getSymbols();
    for (it = atoms.begin(); it != atoms.end(); it++) {
      const Vector3 coords = xtal->cartToFrac((*it).pos());
      QString currAtom =
        ElementInfo::getAtomicSymbol((*it).atomicNumber()).c_str();
      int i = symbol.indexOf(currAtom) + 1;
      rep += " ";
      QString inp;
      inp.sprintf("%14.8f", coords.x());
      rep += inp + "  ";
      inp.sprintf("%14.8f", coords.y());
      rep += inp + "  ";
      inp.sprintf("%14.8f", coords.z());
      rep += inp + "  ";
      rep += QString::number(i) + "\n";
    }
  } else if (line == "coordsFracId") {
    const std::vector<Atom>& atoms = structure->atoms();
    std::vector<Atom>::const_iterator it;
    for (it = atoms.begin(); it != atoms.end(); it++) {
      const Vector3 coords = xtal->cartToFrac((*it).pos());
      rep += (QString(ElementInfo::getAtomicSymbol((*it).atomicNumber()).c_str()) +
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
  } else if (line == "mtpAtomsInfo") {
    const std::vector<Atom>& atoms = structure->atoms();
    std::vector<Atom>::const_iterator it;
    int tag = 1;
    for (it = atoms.begin(); it != atoms.end(); it++) {
      const Vector3 coords = xtal->cartToFrac((*it).pos());
      QString sym = QString(ElementInfo::getAtomicSymbol((*it).atomicNumber()).c_str());
      int typ = getChemicalSystem().indexOf(sym);
      rep += QString("    %1  %2  %3  %4  %5\n").arg(tag++, 5, 10, QChar(' '))
                    .arg(typ, 5, 10, QChar(' ')).arg(coords.x(), 14, 'f', 8, QChar(' '))
                    .arg(coords.y(), 14, 'f', 8, QChar(' ')).arg(coords.z(), 14, 'f', 8, QChar(' '));
    }
    rep += "Feature chemical_system ";
    for (int i = 0; i < getChemicalSystem().size(); i++) {
      rep += getChemicalSystem()[i] + " ";
    }
    rep += "\n";
  } else if (line == "chemicalSystem") {
    for (int i = 0; i < getChemicalSystem().size(); i++) {
      rep += getChemicalSystem()[i] + " ";
    }
    rep += "\n";
  } else if (line == "gulpFracShell") {
    const std::vector<Atom>& atoms = structure->atoms();
    std::vector<Atom>::const_iterator it;
    for (it = atoms.begin(); it != atoms.end(); it++) {
      const Vector3 coords = xtal->cartToFrac((*it).pos());
      const char* symbol =
        ElementInfo::getAtomicSymbol((*it).atomicNumber()).c_str();
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
        inp.sprintf("%14.8f", m(i, j));
        rep += inp + "  ";
      }
      rep += "\n";
    }
  } else if (line == "cellVector1Angstrom") {
    Vector3 v = xtal->unitCell().aVector();
    for (int i = 0; i < 3; i++) {
      rep += QString::number(v[i]) + "   ";
    }
  } else if (line == "cellVector2Angstrom") {
    Vector3 v = xtal->unitCell().bVector();
    for (int i = 0; i < 3; i++) {
      rep += QString::number(v[i]) + "   ";
    }
  } else if (line == "cellVector3Angstrom") {
    Vector3 v = xtal->unitCell().cVector();
    for (int i = 0; i < 3; i++) {
      rep += QString::number(v[i]) + "   ";
    }
  } else if (line == "cellMatrixBohr") {
    Matrix3 m = xtal->unitCell().cellMatrix();
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        rep += QString::number(m(i, j) * ANG2BOHR) + "   ";
      }
      rep += "\n";
    }
  } else if (line == "cellVector1Bohr") {
    Vector3 v = xtal->unitCell().aVector();
    for (int i = 0; i < 3; i++) {
      rep += QString::number(v[i] * ANG2BOHR) + "   ";
    }
  } else if (line == "cellVector2Bohr") {
    Vector3 v = xtal->unitCell().bVector();
    for (int i = 0; i < 3; i++) {
      rep += QString::number(v[i] * ANG2BOHR) + "   ";
    }
  } else if (line == "cellVector3Bohr") {
    Vector3 v = xtal->unitCell().cVector();
    for (int i = 0; i < 3; i++) {
      rep += QString::number(v[i] * ANG2BOHR) + "   ";
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
      << "%mtpAtomsInfo% -- MTP atomic coordinates and full chemical system info\n"
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
      << "%id% -- xtal id number\n"
      << "%chemicalSystem% -- list of element symbols in alpha. order\n";

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

  if (caseInsensitiveCompare(optName, "mtp"))
    return make_unique<MTPOptimizer>(this);

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
  // As of xtalopt version 14, state files older than v4 will not work
  if (loadedVersion < 4) {
    error("XtalOpt::load(): Settings in file " + file.fileName() +
          " appears to be a run with an older version of XtalOpt. "
          "Please visit https://xtalopt.github.io for latest updates "
          "on XtalOpt and its input flag descriptions.");
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
    // No longer check for xtal.state as they are from old versions of xtalopt.
    if (!QFile::exists(dataPath + "/" + xtalDirs.at(i) + "/structure.state")) {
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
  if (!m_localQueue && anyRemoteQueueInterfaces()) {
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
    // Set state from unfinished objective calcs. -> step optimized; so redo objectives
    if (restartInProcessStructures && state == Structure::ObjectiveCalculation) {
      state = Structure::StepOptimized;
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
             (xtal->getStatus() == Xtal::Similar ||
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
    loadedStructures.append(qobject_cast<Structure*>(xtal));
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

  // As of XtalOpt v14, we use convex hull for fitness. So, the very last
  //   step in loading an older run would be to re-calculate distance
  //   above hull for loaded structures.
  // Since this is a "dynamic" value, it is not saved to structure.state files.
  updateHullAndFrontInfo();

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
  if (minXtalsOfSpg.empty()) {
    for (size_t spg = 1; spg <= 230; spg++)
      minXtalsOfSpg.append(0);
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

  emit refreshAllStructureInfo();
}

void XtalOpt::resetSimilarities()
{
  if (isStarting) {
    return;
  }
  QtConcurrent::run(this, &XtalOpt::resetSimilarities_);
}

void XtalOpt::resetSimilarities_()
{
  const QList<Structure*>* structures = m_tracker->list();
  Xtal* xtal = 0;
  for (int i = 0; i < structures->size(); i++) {
    xtal = qobject_cast<Xtal*>(structures->at(i));
    QWriteLocker xtalLocker(&xtal->lock());
    if (xtal->getStatus() == Xtal::Similar) {
      xtal->setStatus(Xtal::Optimized);
    }
    xtal->structureChanged(); // Reset cached comparisons
  }
  checkForSimilarities();
}

std::vector<double> XtalOpt::getReferenceEnergiesVector()
{
  // This is an override "convenience function" to give searchbase
  //   access to reference energies for hull calculations.
  // It returns a 1D vector of atom counts (sorted with symbols)
  //   and energies; ready to be added to the hull data input.
  // Reference structure might be a subsystem of chemical space;
  //   so we should return a vector with atom counts of "all symbols"
  //   including those that are -possibly- zero.
  std::vector<double> out;

  QList<QString> chem_sys = getChemicalSystem();

  for (int i = 0; i < refEnergies.size(); i++) {
    QList<QString> symbols = refEnergies[i].cell.getSymbols();
    double         energy  = refEnergies[i].energy;
    for (int j = 0; j < chem_sys.size(); j++) {
      double natoms = 0.0;
      int ind = symbols.indexOf(chem_sys[j]);
      if (ind != -1) {
        natoms = static_cast<double>(refEnergies[i].cell.getCount(symbols[ind]));
      }
      out.push_back(natoms);
    }
    out.push_back(energy);
  }

  return out;
}

CellComp XtalOpt::getXtalComposition(GlobalSearch::Structure *s)
{
  // Returns the "actual" composition, e.g., for a "sub-system" structure
  //   the output comp will have fewer atom types compared to the reference
  //   chemical system.
  CellComp out;

  if (s == nullptr || s->numAtoms() == 0)
    return out;

  QList<QString> symbs = s->getSymbols();
  QList<uint>    count = s->getNumberOfAtomsAlpha();

  for (int i = 0; i < symbs.size(); i++) {
    uint atmcn = ElementInfo::getAtomicNum(symbs[i].toStdString());
    out.set(symbs[i], atmcn, count[i]);
  }

  return out;
}

CellComp XtalOpt::formulaToComposition(QString form)
{
  // Given a single input chemical formula (as a string); return
  //   the corresponding composition object.
  // This is done by adding symbol, atomic number, and atom count
  //   of each element to the composition object.

  CellComp compout;

  std::map<uint, uint> cmp;
  if (!ElementInfo::readComposition(form.toStdString(), cmp)) {
    return compout;
  }

  for (const auto& elem : cmp) {
    compout.set(ElementInfo::getAtomicSymbol(elem.first).c_str(),
                elem.first, elem.second);
  }

  return compout;
}

double XtalOpt::compareCompositions(CellComp comp1, CellComp comp2)
{
  // This function determines if species of two comps match and if atom
  //   count of all species in comp2 are a fixed multiple "r > 0"
  //   of those in comp1.
  // So, the return value tells us that:
  // r > 0: comp2 has the same composition as comp1; with "r * num atoms",
  // r = 0: species don't match, or atom counts are not a fixed multiple, etc.

  // Some basic checks first
  if (comp1.getNumTypes() != comp2.getNumTypes())
    return 0;

  // Do species match?
  for (int i = 0; i < comp2.getNumTypes(); i++) {
    if (!comp1.getSymbols().contains(comp2.getSymbols()[i]))
      return 0;
  }

  // Are counts equivalent: start by finding reference ratio.
  QString symb = comp1.getSymbols()[0];
  double ref1 = static_cast<double>(comp1.getCount(symb));
  double ref2 = static_cast<double>(comp2.getCount(symb));
  double refRatio = ref2 / ref1;

  for (int i = 0; i < comp2.getNumTypes(); i++) {
    symb = comp2.getSymbols()[i];
    double ele1 = static_cast<double>(comp1.getCount(symb));
    double ele2 = static_cast<double>(comp2.getCount(symb));
    double checkRatio = ele2 / ele1;
    if (fabs( checkRatio - refRatio ) > ZERO6)
      return 0;
  }

  // They are equivalent! Return the ratio.
  return refRatio;
}

void XtalOpt::getCompositionVolumeLimits(CellComp incomp, double& minvol, double& maxvol)
{
  // This function makes an estimate for min/max limits of volume corresponding to a composition.
  // From the composition, species types and their atom counts are obtained; and these
  //   info are then used to estimate volume limits according to one of the schemes:
  //   (1) elemental volumes, (2) scaled volume, or (3) absolute volume limits.

  // This shouldn't ever happen. Just in case ...
  if (incomp.getNumAtoms() == 0) {
    qDebug() << "Warning: strange things happening with composition volume!!!";
    minvol = 1;
    maxvol = 10000;
    return;
  }

  // Now: which scheme should be used calculate the volume limits?
  //   (1) If elemental volumes are given, use them,
  //   (2) If scaled volume is given, use that,
  //   (3) If none of the above, use absolute limits.
  bool useEleVol = (eleVolumes.getAtomicNumbers().size() == 0) ? false : true;
  bool useScaVol = (vol_scale_min > ZERO6 && vol_scale_max > vol_scale_min) ? true : false;

  minvol = 0.0;
  maxvol = 0.0;

  QList<uint> atomicNums = incomp.getAtomicNumbers();

  for (int i = 0; i < atomicNums.size(); i++) {
    uint atomCount = incomp.getCount(atomicNums[i]);
    double atomVolum = ElementInfo::getCovalentVolume(atomicNums[i]);
    if (useEleVol) {
      minvol += atomCount * eleVolumes.getMinVolume(atomicNums[i]);
      maxvol += atomCount * eleVolumes.getMaxVolume(atomicNums[i]);
    } else if (useScaVol) {
      minvol += atomCount * atomVolum * vol_scale_min;
      maxvol += atomCount * atomVolum * vol_scale_max;
    } else {
      minvol += atomCount * vol_min;
      maxvol += atomCount * vol_max;
    }
  } 

  return;
}

CellComp XtalOpt::getMinimalComposition()
{
  // Return the "minimum quantity composition": the one which
  //   has the smallest number of atoms of each symbol among
  //   all the compositions in the input list.
  // This is actually a small helper function which is being
  //   used whenever we want to setup the "molecular unit".
  // NOTE: it is always assumed that the chemical elements
  //   in all compositions are the same; even if some have
  //   zero atom count!

  CellComp compMins;

  if (compList.isEmpty())
    return compMins;

  QList<QString> symbols = compList[0].getSymbols();
  QList<uint> counts;
  for (int i = 0; i < symbols.size(); i++)
    counts.append(compList[0].getCount(symbols[i]));

  for (int i = 1; i < compList.size(); i++) {
    for (int j = 0; j < symbols.size(); j++) {
      if (!compList[i].getSymbols().contains(symbols[j]))
        return compMins;
      uint c = compList[i].getCount(symbols[j]);
      if (c < counts[j])
        counts[j] = c;
    }
  }

  for (int i = 0; i < symbols.size(); i++)
    compMins.set(symbols[i],
                 ElementInfo::getAtomicNum(symbols[i].toStdString()), counts[i]);

  return compMins;
}

CellComp XtalOpt::getMaximalComposition()
{
  // Return the "maximum quantity composition": the one which
  //   has the largest number of atoms of each symbol among
  //   all the compositions in the input list.
  // NOTE: it is always assumed that the chemical elements
  //   in all compositions are the same; even if some have
  //   zero atom count!

  CellComp compMaxs;

  if (compList.isEmpty())
    return compMaxs;

  QList<QString> symbols = compList[0].getSymbols();
  QList<uint> counts;
  for (int i = 0; i < symbols.size(); i++)
    counts.append(compList[0].getCount(symbols[i]));

  for (int i = 1; i < compList.size(); i++) {
    for (int j = 0; j < symbols.size(); j++) {
      if (!compList[i].getSymbols().contains(symbols[j]))
        return compMaxs;
      uint c = compList[i].getCount(symbols[j]);
      if (c > counts[j])
        counts[j] = c;
    }
  }

  for (int i = 0; i < symbols.size(); i++)
    compMaxs.set(symbols[i],
                 ElementInfo::getAtomicNum(symbols[i].toStdString()), counts[i]);

  return compMaxs;
}

QList<QString> XtalOpt::getChemicalSystem() const
{
  // A small helper function: return sorted list of symbols in ref chemical system

  QList<QString> out = QList<QString>();

  if (compList.isEmpty())
    return out;

  out = compList[0].getSymbols();

  return out;
}

bool XtalOpt::processInputChemicalFormulas(QString s)
{
  // This function, one of the first things to be called, processes
  //   the input chemical formulas, and sets the list of compositions.
  // Input formula should all be of the same chemical system, and
  //   "full" chemical formula, i.e., proper combination of symbols
  //   and quantities, e.g., "Ti2O4" and not "TiO2".
  // An entry can also be a hyphen-separated list of "supercells",
  //   e.g., "Ti1O4 - Ti3O12".
  // If anything goes wrong, we will return false which quits the run.
  //   Examples of input formulae issues:
  //   - input formulae don't have correct format,
  //   - list of symbols of input formulae don't match the chemical system.
  //
  // This function sets the global variable "compList", and at the end of
  //    it where we now the chemical system, we initiate atomic radii.
  //
  // If it returns true, the above global variables are overwritten;
  //   otherwise they will have their previous values (if any).

  // Working output variable
  QList<CellComp> out;

  // Input list of formulas.
  QStringList formulalist = s.split(',');

  // Process the input formula list and produce composition object.
  // At the end, we will check to see if we have any valid compositions,
  //   and if so, they belong to the same chemical system.
  for (const auto &tmpform : qAsConst(formulalist)) {
    QString formula = tmpform.simplified();
    formula.replace(" ", "");
    // First, is this a "single" formula entry?
    if (!formula.contains("-")) {
      CellComp tmpcomp = formulaToComposition(formula);
      if (tmpcomp.getNumTypes() > 0) {
        out.append(tmpcomp);
        continue;
      } else {
        qDebug() << "Error: incorrect chemical formula entry '" << formula << "'";
        return false;
      }
    }
    // Then, we have a formula range ("-" entry) to deal with.
    QStringList expcomp = formula.split("-");
    if (expcomp.size() != 2) {
      qDebug() << "Error: incorrect chemical formula entry '" << formula << "'";
      return false;
    }

    // Convert limits to compositions; this makes it easier to verify and analyse them
    CellComp comp1 = formulaToComposition(expcomp[0]);
    CellComp comp2 = formulaToComposition(expcomp[1]);

    // Proceed only if they are legit compositions.
    if (comp1.getNumTypes() == 0 || comp2.getNumTypes() == 0) {
      qDebug() << "Error: failed to process chemical formula entry '" << formula << "'";
      return false;
    }

    // Formulae at the both end must be equivalent, proper supercells,
    //   and the second one being larger than the first.
    double ratio = compareCompositions(comp1, comp2);
    if (ratio == 0 || ratio != std::floor(ratio) || ratio < 1) {
      qDebug() << "Error: incorrect chemical formula entry '" << formula << "'";
      return false;
    }

    // Now: comp2 is a "proper supercell" of (or equal to) comp1.
    // Find the supercell ratio (largest expansion factor).
    uint quantratio = static_cast<unsigned int>(ratio);

    // Process all supercell formula and add them to composition list
    for (uint i = 1; i <= quantratio; i++) {
      QString frm = "";
      for (const auto& symb : comp1.getSymbols()) {
        frm += symb + QString::number(comp1.getCount(symb) * i);
      }
      out.append(formulaToComposition(frm));
    }
  }

  // A few final sanity checks.

  // Are we left with any valid formula?
  if (out.isEmpty()) {
    qDebug() << "Error: no valid chemical formula was present in the list!";
    return false;
  }

  // All compositions must be non-empty; have the same number of types of elements
  if (out[0].getNumTypes() == 0) {
    qDebug() << "Error: empty formula is not accepted!";
    return false;
  }
  for (int i = 1; i < out.size(); i++) {
    if (out[i].getNumTypes() != out[0].getNumTypes()) {
      qDebug() << "Error: number of elements in formulas must be the same!";
      return false;
    }
    for (int j = 0; j < out[i].getNumTypes(); j++) {
      if (out[i].getAtomicNumbers()[j] != out[0].getAtomicNumbers()[j]) {
        qDebug() << "Error: element types in all formulas must be the same!";
        return false;
      }
    }
  }

  // Set the composition list
  compList = out;

  // Finally, at this point we have the final list of elements in the search;
  //   time to set the initial elemental minimum radii!
  eleMinRadii.clear();
  for (const auto& atomcn : compList[0].getAtomicNumbers()) {
    double r = ElementInfo::getCovalentRadius(atomcn) * scaleFactor;
    if (r < minRadius)
      r = minRadius;
    eleMinRadii.set(atomcn, r);
  }

  return true;
}

bool XtalOpt::processInputReferenceEnergies(QString s)
{
  // This function processes the input reference energies (if any),
  //   and given a proper input, it sets the set of "lists" each
  //   for a reference entry and containing:
  //     "normalized composition plus the energy per atom".
  // In general, ref energy input entries include "formula energy"
  //   where the formula can be a subsystem of our chemical system.
  // We will ignore "empty" entries, and will return false:
  //  (1) if failed to read any "non-empty" entry,
  //  (2) if failed to read values for "all elements" in chemical system.
  //
  // This function sets the global variable "m_reference_energies".
  // If it returns true, the relevant global variable is overwritten;
  //   otherwise it will have its previous value (if any).

  // Basic sanity checks
  if (compList.isEmpty()) {
    qDebug() << "Error processInputReferenceEnergies: composition is not set.";
    return false;
  }

  // List of chemical elements in the system
  QStringList chemSystem = getChemicalSystem();

  // To keep track of elemental references (if user provides ref energies)
  std::vector<int> eleRefs(chemSystem.size(), 0);

  // Final list of reference energies to return
  QList<RefEnergy> output_list;

  // Start processing the entries.
  int nonEmptyEntries = 0;
  QStringList entries = s.split(','); // input entries

  for(int i = 0; i < entries.size(); i++) {
    QStringList ent = entries[i].split(" ", QString::SkipEmptyParts);

    // Ignore empty entries.
    if (ent.isEmpty())
      continue;

    nonEmptyEntries += 1;

    // Each entry must have a formula and an energy.
    if (ent.size() < 2) {
      qDebug() << "Error: reference energy entries must "
                  "include chemical formula and energy.";
      return false;
    }

    // Extract energy
    double ener = ent.takeLast().toDouble();
    // Extract chemical formula
    QString form = ent.join(' ');

    // Convert formula to composition: this makes sure we have a valid
    //   formula and simplifies obtaining needed information.
    CellComp comp = formulaToComposition(form);

    // If formula is not a "valid" composition, return false.
    if (comp.getNumTypes() == 0 || comp.getNumAtoms() == 0) {
      qDebug() << "Error: couldn't read entry '" << entries[i]
               << "' in reference energy list.";
      return false;
    }

    // Also, all symbols in the entry should be in our chemical system.
    QList<QString> names = comp.getSymbols();
    for(int j = 0; j < names.size(); j++) {
      if (!chemSystem.contains(names[j])) {
        qDebug() << "Error: unknown element '" << names[j]
                 << "' in reference energy list.";
        return false;
      }
    }

    // So, the composition is good! See if it's an elemental reference
    if (comp.getNumTypes() == 1) {
      eleRefs[chemSystem.indexOf(names[0])]++;
    }
    // Finally, add the data to output list.
    output_list.append({comp, ener});

  }

  // If no non-empty entries are given, just construct the
  //   list with default "zero" value for elements.
  if (nonEmptyEntries == 0) {
    output_list.clear();
    for (int i = 0; i < chemSystem.size(); i++) {
      QString frm = chemSystem[i]+"1";
      CellComp comp = formulaToComposition(frm);
      output_list.append({comp, 0.0});
    }
    refEnergies = output_list;
    return true;
  }

  // Otherwise, make sure every non-empty entry was read in successfully.
  if (nonEmptyEntries != output_list.size()) {
    qDebug() << "Error: failed to process some reference energy entries.";
    return false;
  }

  // If user gives ref energies, at least one of each elements must be given.
  for (int i = 0; i < chemSystem.size(); i++) {
    if (eleRefs[i] == 0) {
      qDebug() << "Error: reference energies must include all elements.";
      return false;
    }
  }

  // We're good! Initiate the main reference energy "global" variable.
  refEnergies = output_list;

  return true;
}

bool XtalOpt::processInputElementalVolumes(QString s)
{
  // This function processes the input elemental volumes (if any),
  //  and sets the elemental volume global variable if valid volume
  //  limits for all elements in chemical system are given.
  // We will ignore "empty" entries, and will return false:
  //  (1) if failed to read any "non-empty" entry,
  //  (2) if failed to read values for "all elements" in chemical system.
  //
  // This function sets the global variable "elemental_volumes".
  // If it returns true, the relevant global variable is overwritten;
  //   otherwise it will have its previous value (if any).

  // Basic sanity checks
  if (compList.isEmpty()) {
    qDebug() << "Error processInputElementalVolumes: composition is not set.";
    return false;
  }

  // List of chemical elements in the system
  QStringList chemSystem = getChemicalSystem();

  // To keep track of elemental references (if user provides ref energies)
  std::vector<int> eleVols(chemSystem.size(), 0);

  // Final processed elemental volumes.
  EleVolume out;

  // Start processing the entries.
  int nonEmptyEntries = 0;
  QStringList entries = s.split(',');

  for(int i = 0; i < entries.size(); i++) {
    QStringList ent = entries[i].split(" ", QString::SkipEmptyParts);

    // Ignore empty entries.
    if (ent.isEmpty())
      continue;

    nonEmptyEntries += 1;

    // Each entry must have a formula and two volume limits.
    if (ent.size() < 3)
      return false;

    // Extract volume limits
    double vmax = ent.takeLast().toDouble();
    double vmin = ent.takeLast().toDouble();
    // Extract the -elemental- chemical formula
    QString form = ent.join(' ');

    // If not a valid set of limits, just ignore the entry.
    if (vmin < ZERO6 || vmax < vmin) {
      qDebug() << "Error: incorrect volume limits for elemental volume entry '"
               << entries[i] << "'";
      return false;
    }

    // Convert formula to composition: this makes sure we have a valid
    //   formula and simplifies obtaining needed information.
    CellComp comp = formulaToComposition(form);

    // If entry is not proper (e.g., formula with more than one element) ignore it.
    if (comp.getNumTypes() != 1 || comp.getNumAtoms() == 0) {
      qDebug() << "Error: couldn't process elemental volume entry '"
               << entries[i] << "'";
      return false;
    }

    // Symbol must be on the list. Otherwise, ignore it.
    QString symbol = comp.getSymbols()[0];
    uint atomcn = comp.getAtomicNumbers()[0];
    uint totaln = comp.getNumAtoms();
    int ind = chemSystem.indexOf(symbol);
    if (ind == -1) {
      qDebug() << "Error: elemental volume for unknown symbol '"
               << symbol << "'";
      return false;
    } else {
      // Which element is this?!
      eleVols[chemSystem.indexOf(symbol)]++;
    }

    // Add volume data to composition object (we save "per atom" values)
    out.set(atomcn, vmin / totaln, vmax / totaln);
  }

  // If no non-empty entries are given, just return.
  if (nonEmptyEntries == 0) {
    eleVolumes.clear();
    return true;
  }

  // Otherwise, make sure every non-empty entry was read in successfully.
  if (nonEmptyEntries != out.getAtomicNumbers().size()) {
    qDebug() << "Error: failed to process some elemental volume entries.";
    return false;
  }

  // If elemental volumes are given; we should have one per chemical element.
  for (int i = 0; i < chemSystem.size(); i++) {
    if (eleVols[i] != 1) {
      qDebug() << "Error: elemental volumes must include all elements.";
      return false;
    }
  }

  // We're good! Assign the main elemental volume object
  eleVolumes.clear();
  eleVolumes = out;

  return true;
}

bool XtalOpt::processInputObjectives(QString s)
{
  // This function processes the objective entries from a string
  //   input that includes all relevant fields:
  //   type, executable, output filename, and weight.

  // Number of already added objectives
  int objnum = getObjectivesNum();
  // Total weight of objectives
  double totalweight = 0.0;
  for (int i = 0; i < objnum; i++)
    totalweight += getObjectivesWgt(i);
  // We call this function multiple times: just in case!
  m_calculateObjectives = (objnum > 0) ? true : false;

  // Process entries

  QStringList sline = s.split(" ", QString::SkipEmptyParts);
  // There should be four entries in the input: (1) objective
  //   type (min/max/fil), (2) executable script, (3) output
  //   filename, and (4) weight.
  if (sline.size() < 4) {
    error("objective is not properly initiated");
    return false;
  }

  // 1st item is always objective's type: min/max/fil
  QString tmps = sline.at(0).toLower().mid(0,3);
  ObjType objtyp;
  if (tmps == "min")
    objtyp = ObjType::Ot_Min;
  else if (tmps == "max")
    objtyp = ObjType::Ot_Max;
  else if (tmps == "fil")
    objtyp = ObjType::Ot_Fil;
  else {
    error(tr("unknown objective type: '%1' in '%2'")
              .arg(tmps).arg(s));
    return false;
  }
  // 2nd item is always the script path
  QString objexe = sline.at(1);
  // 3rd item is the output filename
  QString objout = sline.at(2);
  // 4th item is the weight
  bool isNumber;
  double objwgt = sline.at(3).toDouble(&isNumber);
  if (!isNumber || objwgt < 0.0 || objwgt > 1.0) {
    error("objective weight should be a digit in [0,1]");
    return false;
  }
  // Filtration objective should have a weight of zero
  if (objtyp == ObjType::Ot_Fil && objwgt != 0) {
    error("filtration objectives should have zero weight");
    return false;
  }

  // Sanity check: total weights should be less than or equal to 1
  if (totalweight + objwgt > 1.0) {
    error("total weight of objectives can't exceed 1.0");
    return false;
  }

  // We're good! Add the objective
  setObjectivesTyp(objtyp);
  setObjectivesExe(objexe);
  setObjectivesOut(objout);
  setObjectivesWgt(objwgt);

  // We have at least one objective added!
  m_calculateObjectives = true;

  return true;
}

void XtalOpt::checkIfSimilar(simCheckStruct& st)
{
  if (st.i == st.j)
    return;
  Xtal *kickXtal, *keepXtal;
  QReadLocker iLocker(&st.i->lock());
  QReadLocker jLocker(&st.j->lock());
  // If they are already both marked as similar, just return.
  if (st.i->getStatus() == Xtal::Similar &&
      st.j->getStatus() == Xtal::Similar) {
    return;
  }

  // With the variable-composition search, we have the possibilities of having:
  //   (1) xtals of different composition (even those which are sub-system seeds)
  //   (2) one xtal being a supercell of the other one without explicitly marked as such.
  //
  // As of XtalOpt v14, we have two options for similarity check:
  //   (1) XtalComp, (2) RDF dot product.
  // By default, we use XtalComp; RDF is used only if it has a non-zero tolerance.
  //   For RDF check, we just pass xtals as is for similarity check.
  //   For XtalComp, we first convert them to primitive cells, and compare them.

  // If the composition of the xtals are different, we won't check for similarity!
  CellComp xc1 = getXtalComposition(st.i);
  CellComp xc2 = getXtalComposition(st.j);
  if (compareCompositions(xc1, xc2) == 0) {
    return;
  }

  bool theyAreSimilar = false;

  if (tol_rdf > 0.0 && tol_rdf <= 1.0) {
    theyAreSimilar = st.i->compareRDFs(st.j, tol_rdf, tol_rdf_nbins,
                                       tol_rdf_cutoff, tol_rdf_sigma, m_verbose);
  } else {
    Xtal* xtali = generatePrimitiveXtal(st.i);
    Xtal* xtalj = generatePrimitiveXtal(st.j);
    theyAreSimilar = xtali->compareCoordinates(*xtalj, st.tol_len, st.tol_ang);
  }

  if (!theyAreSimilar)
    return;

  // Mark the newest xtal as a similarity to the oldest. This keeps the
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
  // If the kickXtal is already a similar, just return
  if (kickXtal->getStatus() == Xtal::Similar) {
    return;
  }
  // Unlock the kickXtal and lock it for writing
  kickXtal == st.i ? iLocker.unlock() : jLocker.unlock();
  QWriteLocker kickXtalLocker(&kickXtal->lock());
  kickXtal->setStatus(Xtal::Similar);
  kickXtal->setSimilarityString(QString("%1x%2")
                   .arg(keepXtal->getGeneration())
                   .arg(keepXtal->getIDNumber()));
}

void XtalOpt::checkForSimilarities()
{
  if (isStarting)
    return;

  QtConcurrent::run(this, &XtalOpt::checkForSimilarities_);
}

void XtalOpt::checkForSimilarities_()
{
  // Only run this function with one thread at a time.
  static std::mutex simMutex;
  std::unique_lock<std::mutex> simLock(simMutex, std::defer_lock);
  if (!simLock.try_lock()) {
    // If a thread is already running this function, we can wait. But
    // we should only have one waiter at any time.
    static std::mutex waitMutex;
    std::unique_lock<std::mutex> waitLock(waitMutex, std::defer_lock);
    if (!waitLock.try_lock())
      return;
    else
      simLock.lock();
  }

  QReadLocker trackerLocker(m_tracker->rwLock());
  const QList<Structure*>* structures = m_tracker->list();
  QList<Xtal*> xtals;
  xtals.reserve(structures->size());
  std::for_each(structures->begin(), structures->end(), [&xtals](Structure* s) {
    xtals.append(qobject_cast<Xtal*>(s));
  });

  // Build helper structs
  QList<simCheckStruct> simSts;
  simCheckStruct simSt;
  simSt.tol_len = this->tol_xcLength;
  simSt.tol_ang = this->tol_xcAngle;

  for (QList<Xtal*>::iterator xi = xtals.begin(); xi != xtals.end(); ++xi) {
    QReadLocker xiLocker(&(*xi)->lock());
    double abvhulli = (*xi)->getDistAboveHull();
    if ((*xi)->getStatus() != Xtal::Optimized || std::isnan(abvhulli))
      continue;

    for (QList<Xtal*>::iterator xj = xi + 1; xj != xtals.end(); ++xj) {
      QReadLocker xjLocker(&(*xj)->lock());
      double abvhullj = (*xj)->getDistAboveHull();
      if ((*xj)->getStatus() != Xtal::Optimized || std::isnan(abvhullj))
        continue;

      if (((*xi)->hasChangedSinceSimChecked() || (*xj)->hasChangedSinceSimChecked()) &&
          // Perform a coarse energy screening to cut down on number of comparisons
          fabs(abvhulli - abvhullj) < 0.1) {
        // Append the similar structs list
        simSt.i = (*xi);
        simSt.j = (*xj);
        simSts.append(simSt);
      }
    }
    // Nothing else should be setting this, so just update under a read lock
    (*xi)->setChangedSinceSimChecked(false);
  }

  for (auto& simSt : simSts)
    checkIfSimilar(simSt);

  emit refreshAllStructureInfo();
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

QList<uint> XtalOpt::getListOfAtomsComp(CellComp incomp)
{
  if (incomp.getNumTypes() == 0) {
    qDebug() << "Warning: getListOfAtomsComp: empty composition!"
             << " Using the first composition!";
    incomp = compList[0];
  }

  // Populate crystal
  QList<uint> atomicNums = incomp.getAtomicNumbers();
  // Sort atomic number by decreasing minimum radius. Adding the "larger"
  // atoms first encourages a more even (and ordered) distribution
  for (int i = 0; i < atomicNums.size() - 1; ++i) {
    for (int j = i + 1; j < atomicNums.size(); ++j) {
      if (this->eleMinRadii.getMinRadius(atomicNums[i]) <
          this->eleMinRadii.getMinRadius(atomicNums[j])) {
        atomicNums.swap(i, j);
      }
    }
  }

  QList<uint> atoms;
  for (size_t i = 0; i < atomicNums.size(); i++) {
    for (size_t j = 0; j < incomp.getCount(atomicNums[i]); j++) {
      atoms.push_back(atomicNums[i]);
    }
  }
  return atoms;
}

std::vector<uint> XtalOpt::getStdVecOfAtomsComp(CellComp incomp)
{
  return getListOfAtomsComp(incomp).toVector().toStdVector();
}

CellComp XtalOpt::pickRandomCompositionFromPossibleOnes()
{
  if (compList.size() < 2)
    return compList[0];

  return compList[getRandInt(0, compList.size() - 1)];
}

// minXtalsOfSpg should already be set up by now
uint XtalOpt::pickRandomSpgFromPossibleOnes()
{
  if (minXtalsOfSpg.size() == 0) {
    qDebug() << "Error! pickRandomSpgFromPossibleOnes() was called before"
             << "minXtalsOfSpg was set up!";
    return 1;
  }

  QList<uint> possibleSpgs;
  for (size_t i = 0; i < minXtalsOfSpg.size(); i++) {
    uint spg = i + 1;
    if (minXtalsOfSpg.at(i) != -1)
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

bool XtalOpt::importSettings_(QString filename, XtalOpt& x)
{
  // This is the "working" extension to the import function
  //   in ui/dialog.cpp which is tasked with reading in the
  //   GUI entries from a CLI input file (as much as possible!).
  // NOTE: the import is not complete!
  //   Especially the optimizer/queue interface settings.
  if (filename.isEmpty())
    return false;

  return XtalOptCLIOptions::readOptions_(filename, x);
}

bool XtalOpt::exportSettings_(QString filename, XtalOpt* x)
{
  // This is the "working" extension to the export function
  //   in ui/dialog.cpp which is tasked with writing the GUI
  //   entries to a file for CLI input format.
  // NOTE: the writing is not complete! template files are not
  //   written, as well as some other fields.
  //   Especially the optimizer/queue interface settings.
  QString output;
  QTextStream stream(&output);

  printOptionSettings(stream, x);

  // Try to write to the CLI input file
  QFile file(filename);
  if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    file.write(output.toStdString().c_str());
  else
    return false;

  return true;
}

void XtalOpt::printOptionSettings(QTextStream& stream, XtalOpt* x)
{
  // This is used to print "all" options, both to produce initial
  //   "settings.log" file, and also is used in exporting GUI entries
  //   to a CLI input file.
  // To be used in GUI->CLI exporting; some fields (e.g., template file names
  //   and their content, job templates, or templates directory) are just printed
  //   as a flag without value, as a placeholder. In that case,
  //   the user must check all entries and make sure they are correct!

  stream << "\n### Search Parameters ###\n";
  stream << "  chemicalFormulas = " << x->input_formulas_string << "\n";
  stream << "  referenceEnergies = " << x->input_ene_refs_string << "\n";
  stream << "  vcSearch = " << toString(x->vcSearch) << "\n";
  stream << "  minAtoms = " << x->minAtoms << "\n";
  stream << "  maxAtoms = " << x->maxAtoms << "\n";
  stream << "  numInitial = " << x->numInitial << "\n";
  stream << "  parentsPoolSize = " << x->parentsPoolSize << "\n";
  stream << "  limitRunningJobs = " << toString(x->limitRunningJobs) << "\n";
  stream << "  maxNumStructures = " << x->maxNumStructures << "\n";
  stream << "  runningJobLimit = " << x->runningJobLimit << "\n";
  stream << "  continuousStructures = " << x->contStructs << "\n";

  stream << "\n";
  stream << "  optimizationType = " << x->m_optimizationType << "\n";
  stream << "  tournamentSelection = " << toString(x->m_tournamentSelection) << "\n";
  stream << "  restrictedPool = " << toString(x->m_restrictedPool) << "\n";
  stream << "  crowdingDistance = " << toString(x->m_crowdingDistance) << "\n";
  stream << "  objectivePrecision = " << QString::number(x->m_objectivePrecision) << "\n";

  stream << "\n";
  stream << "  objectivesReDo = " << toString(x->m_objectivesReDo) << "\n";
  for (int i = 0; i < x->getObjectivesNum(); i++) {
    stream << "  objective = ";
    if (x->getObjectivesTyp(i) == Ot_Min)
      stream << " min ";
    else  if (x->getObjectivesTyp(i) == Ot_Max)
      stream << " max ";
    else if (x->getObjectivesTyp(i) == Ot_Fil)
      stream << " fil ";
    else
      stream << " unkown ";
    stream << "  " << x->getObjectivesExe(i) << "  " << x->getObjectivesOut(i) << "  "
      << x->getObjectivesWgt(i) << "\n";
  }

  if (x->seedList.size() > 0) {
    stream << "\n";
    stream << "  seedStructures = ";
    for (auto& s : x->seedList)
      stream << s << ",";
    stream << "\n";
  }

  stream << "\n";
  stream << "  softExit = " << toString(x->m_softExit) << "\n";
  stream << "  saveHullSnapshots = " << toString(x->m_saveHullSnapshots) << "\n";
  stream << "  verboseOutput = " << toString(x->m_verbose) << "\n";

  stream << "\n### Cell Limits and Initialization ###\n";
  stream << "  minVolume = " << x->vol_min << "\n";
  stream << "  maxVolume = " << x->vol_max << "\n";
  stream << "  minVolumeScale = " << x->vol_scale_min << "\n";
  stream << "  maxVolumeScale = " << x->vol_scale_max << "\n";
  stream << "  elementalVolumes = " << x->input_ele_volm_string << "\n";

  stream << "\n";
  stream << "  aMin = " << x->a_min << "\n";
  stream << "  bMin = " << x->b_min << "\n";
  stream << "  cMin = " << x->c_min << "\n";
  stream << "  aMax = " << x->a_max << "\n";
  stream << "  bMax = " << x->b_max << "\n";
  stream << "  cMax = " << x->c_max << "\n";

  stream << "  alphaMin = " << x->alpha_min << "\n";
  stream << "  betaMin  = " << x->beta_min << "\n";
  stream << "  gammaMin = " << x->gamma_min << "\n";
  stream << "  alphaMax = " << x->alpha_max << "\n";
  stream << "  betaMax  = " << x->beta_max << "\n";
  stream << "  gammaMax = " << x->gamma_max << "\n";

  stream << "\n";
  stream << "  usingRadiiInteratomicDistanceLimit = "
         << toString(x->using_interatomicDistanceLimit) << "\n";
  stream << "  radiiScalingFactor =  " << x->scaleFactor << "\n";
  stream << "  minRadius = " << x->minRadius << "\n";
  stream << "  usingCustomIADs = " << toString(x->using_customIAD) << "\n";
  uint i = 1;
  for (auto& pair : x->interComp.keys()) {
    stream << "  customIAD " << i++ << " = "
      << ElementInfo::getAtomicSymbol(pair.first).c_str()  << ", "
      << ElementInfo::getAtomicSymbol(pair.second).c_str() << ", "
      << x->interComp[pair].minIAD << "\n";
  }
  stream << "  checkIADPostOptimization = " << toString(x->using_checkStepOpt) << "\n";

  stream << "\n";
  stream << "  usingRandSpg = " << toString(x->using_randSpg) << "\n";
  if (x->using_randSpg) {
    stream << "  forcedSpgsWithRandSpg = ";
    for (int i = 0; i < x->minXtalsOfSpg.size(); ++i) {
      int num = x->minXtalsOfSpg[i];
      if (num > 0)
        stream << i + 1 << ",";
    }
    stream << "\n";
  }

  stream << "\n";
  stream << "  usingMolecularUnits = " << toString(x->using_molUnit) << "\n";
  if (x->using_molUnit) {
    uint i = 1;
    for (auto& pair : x->compMolUnit.keys()) {
      stream << "  molecularUnits " << i++ << " = ";
      stream << ElementInfo::getAtomicSymbol(pair.first).c_str() << ", "
             << x->compMolUnit[pair].numCenters << ", "
             << ElementInfo::getAtomicSymbol(pair.second).c_str() << ", "
             << x->compMolUnit[pair].numNeighbors << ", "
             << getGeom(x->compMolUnit[pair].numNeighbors, x->compMolUnit[pair].geom)
             << ", " << x->compMolUnit[pair].dist << "\n";
    }
  }

  stream << "\n### Genetic Operations ###\n";
  stream << "  weightStripple    = " << x->p_strip << "\n";
  stream << "  weightPermustrain = " << x->p_perm << "\n";
  stream << "  weightCrossover   = " << x->p_cross << "\n";
  stream << "  weightPermutomic  = " << x->p_atomic << "\n";
  stream << "  weightPermucomp   = " << x->p_comp << "\n";
  stream << "  randomSuperCell   = " << x->p_supercell << "\n";

  stream << "\n";
  stream << "  crossoverMinContribution = " << x->cross_minimumContribution << "\n";
  stream << "  crossoverCuts            = " << x->cross_ncuts << "\n";
  stream << "  strippleAmplitudeMin     = " << x->strip_amp_min << "\n";
  stream << "  strippleAmplitudeMax     = " << x->strip_amp_max << "\n";
  stream << "  strippleNumWavesAxis1    = " << x->strip_per1 << "\n";
  stream << "  strippleNumWavesAxis2    = " << x->strip_per2 << "\n";
  stream << "  strippleStrainStdevMin   = " << x->strip_strainStdev_min << "\n";
  stream << "  strippleStrainStdevMax   = " << x->strip_strainStdev_max << "\n";
  stream << "  permustrainNumExchanges  = " << x->perm_ex << "\n";
  stream << "  permustrainStrainStdevMax= " << x->perm_strainStdev_max << "\n";

  stream << "\n### Tolerances ###\n";
  stream << "  spglibTolerance        = " << x->tol_spg << "\n";
  stream << "  xtalcompToleranceLength= " << x->tol_xcLength << "\n";
  stream << "  xtalcompToleranceAngle = " << x->tol_xcAngle << "\n";
  stream << "  rdfTolerance           = " << x->tol_rdf << "\n";
  stream << "  rdfCutoff              = " << x->tol_rdf_cutoff << "\n";
  stream << "  rdfSigma               = " << x->tol_rdf_sigma << "\n";
  stream << "  rdfNumBins             = " << x->tol_rdf_nbins << "\n";

  stream << "\n### Job Handling ###\n";
  stream << "  jobFailLimit = " << x->failLimit << "\n";
  stream << "  jobFailAction = ";
  if (x->failAction == FA_DoNothing)
    stream << "keepTrying\n";
  else if (x->failAction == FA_KillIt)
    stream << "kill\n";
  else if (x->failAction == FA_Randomize)
    stream << "replaceWithRandom\n";
  else if (x->failAction == FA_NewOffspring)
    stream << "replaceWithOffspring\n";
  else
    stream << "\n";

  stream << "\n### Optimizer ###\n";
  stream << "  numOptimizationSteps = " << x->getNumOptSteps() << "\n";
  const GlobalSearch::Optimizer* opt = x->optimizer(0);
  QString optname = opt->getIDString().toLower();
  stream << "  optimizer = " << optname << "\n";
  stream << "  templatesDirectory = " << "\n";
  if (optname == "vasp") {
    stream << "  incarTemplates =\n  kpointsTemplates =\n  potcarFile =\n";
  } else if (optname == "mtp") {
    stream << "  mtpCellTemplates =\n  mtpPotTemplates =\n  mtpRelaxTemplates\n";
  } else if (optname == "pwscf") {
    stream << "  pwscfTemplates =\n";
  } else if (optname == "gulp") {
    stream << "  ginTemplates =\n";
  } else if (optname == "castep") {
    stream << "  castepCellTemplates =\n  castepParamTemplates =\n";
  } else if (optname == "siesta") {
    stream << "  fdfTemplates =\n";
  }

  stream << "\n";
  stream << "  user1 = " << QString::fromStdString(x->getUser1()) << "\n";
  stream << "  user2 = " << QString::fromStdString(x->getUser2()) << "\n";
  stream << "  user3 = " << QString::fromStdString(x->getUser3()) << "\n";
  stream << "  user4 = " << QString::fromStdString(x->getUser4()) << "\n";

  stream << "\n### Queue Interface ###\n";
  const GlobalSearch::QueueInterface* queue = x->queueInterface(0);
  if (!queue) {
    stream << "  queueInterface = NONE\n";
  } else {
    stream << "  queueInterface = " << queue->getIDString().toLower() << "\n";
  }
  stream << "  jobTemplates = " << "\n";
  stream << "  exeLocation = " << opt->getLocalRunCommand() << "\n";
  stream << "  localWorkingDirectory = " << x->locWorkDir << "\n";
  stream << "  remoteWorkingDirectory = " << x->remWorkDir << "\n";
  stream << "  localQueue = " << toString(x->m_localQueue) << "\n";
  stream << "\n";
  stream << "  logErrorDirectories   = " << toString(x->m_logErrorDirs) << "\n";
  stream << "  queueRefreshInterval = " << x->queueRefreshInterval() << "\n";
  stream << "  cleanRemoteDirs = " << toString(x->cleanRemoteOnStop()) << "\n";
  stream << "  autoCancelJobAfterTime = " << toString(x->m_cancelJobAfterTime) << "\n";
  stream << "  hoursForAutoCancelJob = " << x->m_hoursForCancelJobAfterTime << "\n";
  stream << "  host = " << x->host << "\n";
  stream << "  port = " << x->port << "\n";
  stream << "  username = " << x->username << "\n";
#ifdef ENABLE_SSH
    if (queue->getIDString().toLower() != "local") {
      const GlobalSearch::RemoteQueueInterface* remoteQueue =
          qobject_cast<const GlobalSearch::RemoteQueueInterface*>(queue);
      stream << "  submitCommand = " << remoteQueue->submitCommand() << "\n";
      stream << "  cancelCommand = " << remoteQueue->cancelCommand() << "\n";
      stream << "  statusCommand = " << remoteQueue->statusCommand() << "\n";
    }
#endif

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

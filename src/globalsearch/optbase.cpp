/**********************************************************************
  OptBase - Base class for global search extensions

  Copyright (C) 2010-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <globalsearch/optbase.h>

#include <globalsearch/constants.h>
#include <globalsearch/bt.h>
#include <globalsearch/eleminfo.h>
#include <globalsearch/formats/poscarformat.h>
#include <globalsearch/http/aflowml.h>
#include <globalsearch/macros.h>
#include <globalsearch/optimizer.h>
#include <globalsearch/queueinterface.h>
#include <globalsearch/queuemanager.h>
#ifdef ENABLE_SSH
#include <globalsearch/sshconnection.h>
#include <globalsearch/sshmanager.h>
#ifdef USE_CLI_SSH
#include <globalsearch/sshmanager_cli.h>
#else // USE_CLI_SSH
#include <globalsearch/sshmanager_libssh.h>
#endif // USE_CLI_SSH
#endif // ENABLE_SSH
#include <globalsearch/structure.h>
#include <globalsearch/ui/abstractdialog.h>
#include <globalsearch/utilities/makeunique.h>
#include <globalsearch/utilities/passwordprompt.h>
#include <globalsearch/utilities/utilityfunctions.h>

#include <QDebug>
#include <QFile>
#include <QThread>

#include <QApplication>
#include <QClipboard>
#include <QInputDialog>
#include <QMessageBox>
#include <QtConcurrent>

#include <cfloat>
#include <cmath>
#include <fstream>
#include <iostream>
#include <mutex>
#include <chrono>
#include <thread>

// Uncomment for yet more debug info about probabilities
//#define OPTBASE_PROBS_DEBUG

namespace GlobalSearch {

OptBase::OptBase(AbstractDialog* parent)
  : QObject(parent),
    cutoff(-1), testingMode(false), test_nRunsStart(1), test_nRunsEnd(100),
    test_nStructs(600), stateFileMutex(new QMutex), readOnly(false),
    m_idString("Generic"),
#ifdef ENABLE_SSH
    m_ssh(nullptr),
#endif // ENABLE_SSH
    m_dialog(parent), m_tracker(new Tracker(this)), m_queueThread(new QThread),
    m_queue(new QueueManager(m_queueThread, this)), m_numOptSteps(0),
    m_schemaVersion(3), m_usingGUI(true),
    m_logErrorDirs(false), m_calculateHardness(false),
    m_hardnessFitnessWeight(-1.0),
    m_networkAccessManager(std::make_shared<QNetworkAccessManager>()),
    m_aflowML(make_unique<AflowML>(m_networkAccessManager, this)),
    m_calculateObjectives(false), m_objectives_num(0), m_objectivesReDo(false),
    m_softExit(false), m_hardExit(false), m_localQueue(false)
{
  // Connections
  connect(this, SIGNAL(sessionStarted()), m_queueThread, SLOT(start()),
          Qt::DirectConnection);
  connect(this, SIGNAL(startingSession()), m_queueThread, SLOT(start()),
          Qt::DirectConnection);
  connect(this, SIGNAL(startingSession()), this, SLOT(setIsStartingTrue()),
          Qt::DirectConnection);
  connect(this, SIGNAL(sessionStarted()), this, SLOT(setIsStartingFalse()),
          Qt::DirectConnection);
  connect(this, SIGNAL(readOnlySessionStarted()), this,
          SLOT(setIsStartingFalse()), Qt::DirectConnection);
  connect(this, SIGNAL(needBoolean(const QString&, bool*)), this,
          SLOT(promptForBoolean(const QString&, bool*)),
          Qt::BlockingQueuedConnection); // Wait until slot returns
  connect(this, SIGNAL(needPassword(const QString&, QString*, bool*)), this,
          SLOT(promptForPassword(const QString&, QString*, bool*)),
          Qt::BlockingQueuedConnection); // Wait until slot returns
  connect(this, SIGNAL(sig_setClipboard(const QString&)), this,
          SLOT(setClipboard_(const QString&)), Qt::QueuedConnection);
  connect(m_tracker, &Tracker::newStructureAdded,
          [this]() { QtConcurrent::run([this]() { this->save("", false); }); });
  connect(m_queue, &QueueManager::structureUpdated,
          [this]() { QtConcurrent::run([this]() { this->save("", false); }); });
  connect(m_queue, &QueueManager::readyForObjectiveCalculations,
          this,    &OptBase::calculateObjectives, Qt::QueuedConnection);
  connect(m_queue, &QueueManager::readyForObjectiveCalculations,
          this,    &OptBase::calculateHardness, Qt::QueuedConnection);
  connect(m_aflowML.get(), &AflowML::received, this,
          &OptBase::finishHardnessCalculation);
}

OptBase::~OptBase()
{
  delete m_queue;
  m_queue = 0;

  if (m_queueThread && m_queueThread->isRunning()) {
    m_queueThread->wait();
  }
  delete m_queueThread;
  m_queueThread = 0;

  delete m_tracker;
  m_tracker = 0;
}

void OptBase::reset()
{
  m_tracker->lockForWrite();
  m_tracker->deleteAllStructures();
  m_tracker->reset();
  m_tracker->unlock();
  m_queue->reset();
}

#ifdef ENABLE_SSH
bool OptBase::createSSHConnections()
{
#ifdef USE_CLI_SSH
  return this->createSSHConnections_cli();
#else  // USE_CLI_SSH
  return this->createSSHConnections_libssh();
#endif // USE_CLI_SSH
}
#endif // ENABLE_SSH

void OptBase::performTheExit(int delay)
{
  // This functions performs the exit, i.e., terminates the run.
  // The input parameter "delay" has a default of 0. If a non-zero
  //   delay is specified, the function waits for that amount, and
  //   will attempt to do some clean up before quitting.

  if (delay > 0) {
    // Impose a delay if needed
    QThread::msleep(delay * 1000);

    m_dialog = nullptr;

    // Save one last time
    warning("Saving XtalOpt settings...");

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
  }

  QString formattedTime = QDateTime::currentDateTime().toString("MMMM dd, yyyy   hh:mm:ss");
  QByteArray formattedTimeMsg = formattedTime.toLocal8Bit();
  qDebug().noquote() << "\n=== Optimization finished ... " + formattedTimeMsg + "\n";
  exit(1);
}

static inline double calculateProb(QString strucTag,
                                   int    objectives_num,
                                   QList<OptBase::ObjectivesType> objectives_typ,
                                   QList<double> objectives_wgt,
                                   QList<double> objectives_val,
                                   QList<double> objectives_min,
                                   QList<double> objectives_max,
                                   double currentEnthalpy,
                                   double lowestEnthalpy,
                                   double highestEnthalpy,
                                   double hardnessWeight,
                                   double currentHardness,
                                   double lowestHardness,
                                   double highestHardness)
{
  // General note: if the spread for any objective/property is zero;
  //   we take its contribution to be zero so it does not suppress the effect of the other
  //   objectives. Otherwise, this single objective will make the whole prob to be "nan" while
  //   there might be other contributing objectives with meaningful values.
  // So, we calculate "partialFitns" for each objective as its raw fitness, and depending
  //   on the spread correct it to get the "correctFitns". Then multiply it into corresponding
  //   weight to obtain the actual contribution of that objective to the total fitness.

  double enthalpySpread = highestEnthalpy - lowestEnthalpy;
  double hardnessSpread = highestHardness - lowestHardness;
  double objectivesSpread;
  double weightsTotal   = 0.0;    // total weight of all objectives/properties
  double fitnessTotal   = 0.0;    // total fitness (to be returned)
  double partialFitns;            // (raw) contribution of a single objective to total fitness
  double correctFitns;            // corrected contrib. of a single objective (if zero spread?)
  QString outs = "";              // auxiliary variable for debug output

  // ===== multi-objective search objectives
  // If no objectives calculation; objectives_num is 0 at this point.
  // Any Filtration objective should have a value of 1 here and zero spread.
  for(int i = 0; i < objectives_num; i++) {
      objectivesSpread = objectives_max[i] - objectives_min[i];
      if (objectives_typ[i] == OptBase::Ot_Min)
        partialFitns = (objectives_max[i] - objectives_val[i]) / objectivesSpread;
      else if (objectives_typ[i] == OptBase::Ot_Max)
        partialFitns = (objectives_val[i] - objectives_min[i]) / objectivesSpread;
      else // Fil objectives
        partialFitns = 0.0;
      // Correction: if spread is zero then contribution is zero
      correctFitns = (objectivesSpread < ZERO8) ? 0.0 : partialFitns;
      //
      weightsTotal += objectives_wgt[i];
      fitnessTotal += objectives_wgt[i] * correctFitns;

#ifdef MOES_DEBUG
outs += QString("NOTE: objc %1 typ %2 val %3 min %4 max %5 ftn %6 wgt %7 - ctr %8\n")
 .arg(i+1,2).arg(objectives_typ[i],2).arg(objectives_val[i],10,'f',5)
 .arg(objectives_min[i],10,'f',5).arg(objectives_max[i],10,'f',5).arg(partialFitns,7,'f',5)
 .arg(objectives_wgt[i],5,'f',3).arg(objectives_wgt[i] * correctFitns,7,'f',5);
#endif
  }

  // ===== aflow-hardness
  // If no aflow-hardness calculation, it's weight is -1.0 at this point.
  if (hardnessWeight >= 0.0) {
    partialFitns = (currentHardness - lowestHardness) / hardnessSpread;
    // Correction: if spread is zero then contribution is zero
    correctFitns = (hardnessSpread < ZERO8) ? 0.0 : partialFitns;
    //
    weightsTotal += hardnessWeight;
    fitnessTotal += hardnessWeight * correctFitns;

#ifdef MOES_DEBUG
outs += QString("NOTE: hard %1 typ %2 val %3 min %4 max %5 ftn %6 wgt %7 - ctr %8\n")
  .arg(-1,2).arg(1,2).arg(currentHardness,10,'f',5).arg(lowestHardness,10,'f',5)
  .arg(highestHardness,10,'f',5).arg(partialFitns,7,'f',5).arg(hardnessWeight,5,'f',3)
  .arg(hardnessWeight * correctFitns,7,'f',5);
#endif
  }

  // ===== enthalpy
  partialFitns = (highestEnthalpy - currentEnthalpy) / enthalpySpread;
  // Correction: if spread is zero then contribution is zero
  correctFitns = (enthalpySpread < ZERO8) ? 0.0 : partialFitns;
  //
  fitnessTotal += (1.0 - weightsTotal) * correctFitns;

#ifdef MOES_DEBUG
outs += QString("NOTE: enth %1 typ %2 val %3 min %4 max %5 ftn %6 wgt %7 - ctr %8\n")
  .arg(0,2).arg(0,2).arg(currentEnthalpy,10,'f',5).arg(lowestEnthalpy,10,'f',5)
  .arg(highestEnthalpy,10,'f',5).arg(partialFitns,7,'f',5).arg(1.0-weightsTotal,5,'f',3)
  .arg((1.0 - weightsTotal) * correctFitns,7,'f',5);
outs += QString("NOTE: struc %1   oldFitness %2   newFitness %3")
  .arg(strucTag,8).arg(correctFitns,8,'f',6).arg(fitnessTotal,8,'f',6);
qDebug().noquote() << outs;
#endif

  // Finally, return the calculated total fitness
  return fitnessTotal;
}

QList<QPair<Structure*, double>>
OptBase::getProbabilityList(const QList<Structure*>& structures,
                     size_t popSize,
                     double hardnessWeight,
                     int    objectives_num,
                     QList<double> objectives_wgt,
                     QList<OptBase::ObjectivesType> objectives_typ)
{
  // This function is modified for multi-objective case;
  //   it has default values for some input parameters in optbase.h
  QList<QPair<Structure*, double>> probs;
  if (structures.isEmpty() || popSize == 0)
    return probs;

  if (structures.size() == 1) {
    probs.append(QPair<Structure*, double>(structures[0], 1.0));
    return probs;
  }

  // Since enthalpy can be negative, we will make -DBL_MAX the starting value
  // for highestEnthalpy. But since hardness can't be negative, we will use
  // DBL_MIN as the starting value for highestHardness.
  double lowestEnthalpy  =  DBL_MAX;
  double highestEnthalpy = -DBL_MAX;
  double lowestHardness  =  DBL_MAX;
  double highestHardness =  DBL_MIN;
  // For multi-objective case
  QList<double> objectives_min = {};
  QList<double> objectives_max = {};
  for (int i = 0; i< objectives_num; i++) {
    objectives_min.push_back(DBL_MAX);
    objectives_max.push_back(-DBL_MAX);
  }

  // Find the lowest and highest of each
  for (const auto& s: structures) {
    QReadLocker lock(&s->lock());
    const auto& enthalpy = s->getEnthalpyPerFU();
    if (enthalpy < lowestEnthalpy)
      lowestEnthalpy = enthalpy;
    if (enthalpy > highestEnthalpy)
      highestEnthalpy = enthalpy;

    const auto& hardness = s->vickersHardness();
    if (hardness < lowestHardness)
      lowestHardness = hardness;
    if (hardness > highestHardness)
      highestHardness = hardness;

    for (int i = 0; i< objectives_num; i++) {
        if(s->getStrucObjValues(i) < objectives_min[i])
          objectives_min[i] = s->getStrucObjValues(i);
        if(s->getStrucObjValues(i) > objectives_max[i])
          objectives_max[i] = s->getStrucObjValues(i);
    }
  }

  // Now calculate the probability of each structure
  for (const auto& s: structures) {
    QReadLocker lock(&s->lock());

    double prob = calculateProb(s->getTag(),
                                objectives_num,
                                objectives_typ,
                                objectives_wgt,
                                s->getStrucObjValuesVec(),
                                objectives_min,
                                objectives_max,
                                s->getEnthalpyPerFU(),
                                lowestEnthalpy,
                                highestEnthalpy,
                                hardnessWeight,
                                s->vickersHardness(),
                                lowestHardness,
                                highestHardness);

    probs.append(QPair<Structure*, double>(s, prob));
  }

  // =======================================================================
  // The probs are set to zero if the spread is zero.
  // So, all probs shouldn't be "nan" anymore; unless there is an unfortunate
  //   case in which the ratio still diverges because of small denaminator, etc.
  //                           -----------------
  // In any case; if all the probs are equal (including "nan"), just return a uniform list
  bool allNan = true;
  bool allEqual = true;
  double refProb = probs[0].second;
  for (const auto& prob: probs) {
    if (!std::isnan(prob.second)) {
      allNan = false;
      if (fabs(prob.second - refProb) > ZERO8)
        allEqual = false;
    }
  }

  if (allNan || allEqual) {
    double dref = 1.0 / probs.size();
    double sum = 0.0;

    for (auto& prob: probs) {
      prob.second = sum;
      sum += dref;
    }
    return probs;
  }
  // =======================================================================

  // Sort by probability
  std::sort(probs.begin(), probs.end(),
            [](const QPair<Structure*, double>& a,
               const QPair<Structure*, double>& b)
            {
              return a.second < b.second;
            });

  // Remove the lowest probability structures until we have the pop size
  while (probs.size() > popSize)
    probs.pop_front();


#ifdef OPTBASE_PROBS_DEBUG
  QString outs1 = QString("\nNOTE: Unnormalized (but sorted and trimmed) probs list is:\n"
                         "    structure :  enthalpy  : probs\n");
  for (const auto& elem: probs) {
    QReadLocker lock(&elem.first->lock());
    outs1 += QString("      %1 : %3 : %4\n").arg(elem.first->getTag(),7)
      .arg(elem.first->getEnthalpyPerFU(),0,'f',6).arg(elem.second,0,'f',6);
  }
  qDebug().noquote() << outs1;
#endif

  // Sum the resulting probs
  double sum = 0.0;
  for (const auto& elem: probs)
    sum += elem.second;

  // Normalize the list so that the sum is 1
  for (auto& elem: probs)
    elem.second /= sum;

#ifdef OPTBASE_PROBS_DEBUG
  outs1 = QString("NOTE: Normalized, sorted, and trimmed probs list is:\n"
                  "    structure :  enthalpy  : probs\n");
  for (const auto& elem: probs) {
    QReadLocker lock(&elem.first->lock());
    outs1 += QString("      %1 : %3 : %4\n").arg(elem.first->getTag(),7)
      .arg(elem.first->getEnthalpyPerFU(),0,'f',6).arg(elem.second,0,'f',6);
  }
  qDebug().noquote() << outs1;
#endif

  // Now replace each entry with a cumulative total
  sum = 0.0;
  for (auto& elem: probs) {
    sum += elem.second;
    elem.second = sum;
  }

#ifdef OPTBASE_PROBS_DEBUG
  outs1 = QString("NOTE: Cumulative (final) probs list is:\n"
                  "    structure :  enthalpy  : probs\n");
  for (const auto& elem: probs) {
    QReadLocker lock(&elem.first->lock());
    outs1 += QString("      %1 : %3 : %4\n").arg(elem.first->getTag(),7)
      .arg(elem.first->getEnthalpyPerFU(),0,'f',6).arg(elem.second,0,'f',6);
  }
  qDebug().noquote() << outs1;
#endif

  return probs;
}

void OptBase::calculateObjectives(Structure* s)
{
  // This is the wrapper for objective calculation for the structure.
  //
  // The objective calculations for each structure is handled in two steps:
  //   (1) startObjectiveCalculations
  //     (1-a) generate output.POSCAR file,
  //     (1-b) copy to remote (if needed),
  //     (1-c) run the commands, i.e., user-defined script.
  //   (2) finishObjectiveCalculations
  //     (2-a) wait for objective output files to appear (if needed),
  //     (2-b) copy back them from remote (if needed),
  //     (2-c) process the output files,
  //     (2-d) update structure object with the results,
  //     (2-e) signal the finish.
  //
  // Note:
  //   (1) Failing in the followings are ***fatal errors****
  //       - writing the output.POSCAR file
  //       - copying the output.POSCAR file to remote (in a remote run)
  //       - running user script
  //       - copying back objective output files (in a remote run)
  //   However, since error handling in local/local-remote runs is not
  //   so reliable (due to error channel pollution, etc.), we won't force
  //   quitting the objective calculcations and just print an error message;
  //   except than error in writing the output.POSCAR which will results
  //   in marking the structure as Fail, signalling the finish, and return.

  // We might be here just because aflow-hardness is requested; so first check
  //   if objective calculations are requested too or not! If not, just do nothing.
  if (!m_calculateObjectives)
    return;

  QtConcurrent::run(this, &OptBase::startObjectiveCalculations, s);

  return;
}

void OptBase::startObjectiveCalculations(Structure* s)
{
  // Set up some variables
  QueueInterface* qi = queueInterface(s->getCurrentOptStep());
  // Structure files prepared for the user script
  QString flname = "output.POSCAR";
  // The local run directory (where files are generated)
  QString locdir = s->getLocpath() + QDir::separator();
  // Where we run the user-provided script
  //   It's assumed that the remote is a "unix"-based system
  QString wrkdir =
    (qi->getIDString().toLower() == "local") ? locdir : s->getRempath() + "/";

  qDebug() << "Objective calculations for " << s->getTag() << " started!";

  // We set the default objective calc. status to Fail. In case the run is
  //   interrupted with a major failure, this will remain as the overall status.
  QWriteLocker structureLocker(&s->lock());
  s->setStrucObjState(Structure::Os_Fail);
  structureLocker.unlock();

  // Write the output.POSCAR file
  std::stringstream ss;
  QFile file(locdir + flname);
  if ((file.open(QIODevice::WriteOnly | QIODevice::Text)) &&
      (PoscarFormat::write(*s, ss) && file.write(ss.str().c_str()))) {
    file.close();
  } else {
    // This is a major failure! We shouldn't proceed!
    error(tr("Failed writing output.POSCAR file for structure %1")
        .arg(s->getTag()));
    emit doneWithObjectives(s);
    return;
  }

  // Copy it to the remote location. This is needed only for remote runs.
  // Actually, this function doesn't do anything if it is a local run!
  if (!qi->copyAFileLocalToRemote(locdir + flname, wrkdir + flname)) {
    error(tr("Failed to copy the output.POSCAR file for structure %1 to remote!")
          .arg(s->getTag()));
  }

  // Run/Submit the objective scripts
  QString stdout_str, stderr_str;
  int ec;
  for(int i = 0; i < getObjectivesNum(); i++) {
    if (!qi->runACommand(wrkdir, getObjectivesExe(i), &stdout_str, &stderr_str, &ec)) {
      error(tr("Failed to run the user script for objective %1 for structure %2")
            .arg(i+1).arg(s->getTag()));
    }
  }

  // Now, on a separate thread, wait for all objective output files
  //   to appear (and copy them back if it's a remote run), and
  //   process the results of objective calculations.
  QtConcurrent::run(this, &OptBase::finishObjectiveCalculations, s);

  return;
}

void OptBase::finishObjectiveCalculations(Structure* s)
{
  // Set up some variables
  QueueInterface* qi = queueInterface(s->getCurrentOptStep());
  // Structure files prepared for the user script
  QString flname = "output.POSCAR";
  // The local run directory (where files are generated)
  QString locdir = s->getLocpath() + QDir::separator();
  // Where we run the user-provided script
  //   It's assumed that the remote is a "unix"-based system
  QString wrkdir =
    (qi->getIDString().toLower() == "local") ? locdir : s->getRempath() + "/";

  // First, check if all output files exist and wait if not. This step is basically
  //   important for objectives that submit their calculation to a queue; otherwise
  //   the qprocess run returns when process finishes and files are already generated.
  // This function runs on a separate thread; so sleep won't freeze gui.
  bool ok = false, exists;
  while (!ok) {
    ok = true;
    for (int i = 0; i < getObjectivesNum(); i++)
      if (!qi->checkIfFileExists(s, getObjectivesOut(i), &exists) || !exists)
        ok = false;
    if (!ok)
      QThread::currentThread()->usleep(queueRefreshInterval() * 1000 * 1000);
  }

  // Copy objective output files back to structure's local working directory.
  // Actually, this function doesn't do anything if it is a local run!
  for (int i = 0; i < getObjectivesNum(); i++) {
    if (!qi->copyAFileRemoteToLocal(wrkdir + getObjectivesOut(i), locdir + getObjectivesOut(i))) {
      error(tr("Failed to copy output for objective %1 for structure %2 from remote!")
            .arg(i+1).arg(s->getTag()));
    }
  }

  // Process the objective output files.

  QList<double> tmp_values;
  int failed_count = 0;
  int dismis_count = 0;
  for (int i = 0; i < getObjectivesNum(); i++) {
    double flagv = 0.0;
    bool   flags = false;

    QFile file(locdir + getObjectivesOut(i));

    if (file.open(QIODevice::ReadOnly)) {
      QTextStream in(&file);
      QString fline = in.readLine();
      QStringList flist = fline.split(" ", QString::SkipEmptyParts);
      if (flist.size() >= 1) {
        // The below line, aims at reading the first entry of the first line;
        //   while ignoring everything else. The default "false" value of the
        //   "flags" variable changes to true if a legit value is read successfully.
        flagv = flist.at(0).toDouble(&flags);
      }
      file.close();
    } else {
      // This is when failed to read the file for any reason, or it's empty.
      //   With default values of flagv/flags, the objective will be automatically marked
      // as failed in the following. We just produce an error message here.
      error(tr("Failed to read any results from output file for objective %1 for structure %2")
            .arg(i+1).arg(s->getTag()));
    }

    // Apparently c++ considers "nan" and "inf" valid numerical entries. To avoid issues
    //   with these type of values, we exclude them and mark the objective calc as failed.
    //   Otherwise, the probability calculation might end up with a seg fault.
    if (std::isnan(flagv) || std::isinf(flagv)) {
      flagv = 0.0;
      flags = false;
    }

    // Save the whatever value we end up with for the objective.
    tmp_values.push_back(flagv);

    if (!flags)
      // Calculations went wrong (e.g. output file is empty or has an incorrect format)
      failed_count += 1;
    else if (getObjectivesTyp(i) == OptBase::Ot_Fil && flagv == 0.0)
      // Structure marked for discarding by a filtration objective
      dismis_count += 1;
  }

  // Update objective calculation status for the structure to either: Retain, Dismiss, Fail.
  // If none of objectives Fail or Dismiss, we mark the structure as Retain.
  // If at least one Dismiss, we don't care about fails and mark it as Dismiss
  //   since it will be removed from the pull anyways, but gives a chance of redoing.
  // If there are at least Fail objectives (and no Dismiss), then we mark it as Fail.

  QWriteLocker structureLocker(&s->lock());
  s->setStrucObjValuesVec(tmp_values);
  if (failed_count == 0 && dismis_count == 0)
    s->setStrucObjState(Structure::Os_Retain);
  else if (dismis_count > 0)
    s->setStrucObjState(Structure::Os_Dismiss);
  else
    s->setStrucObjState(Structure::Os_Fail);
  structureLocker.unlock();

  qDebug() << "Objective calculations for " << s->getTag()
           << " finished ( status = " << s->getStrucObjState() << " )";

  // We are done here.
  emit doneWithObjectives(s);

  return;
}

// Start up a resubmission thread that will attempt resubmissions every
// 10 minutes
void OptBase::startHardnessResubmissionThread()
{
  if (!m_calculateHardness)
    return;

  // Run in a separate thread
  // This is our first usage of std::thread in the program. If we run into
  // linking or packaging problems, we can go back to QtConcurrent
  std::thread(&OptBase::_startHardnessResubmissionThread, this).detach();
}

void OptBase::_startHardnessResubmissionThread()
{
  if (!m_calculateHardness)
    return;

  // Make sure we only ever have one of these going at a time
  static std::mutex resubmissionMutex;
  std::unique_lock<std::mutex> lock(resubmissionMutex, std::defer_lock);
  if (!lock.try_lock())
    return;

  while (m_calculateHardness) {
    // Wait 10 minutes before the resubmission
    std::this_thread::sleep_for(std::chrono::minutes(10));
    resubmitUnfinishedHardnessCalcs();
  }

  lock.unlock();

  // Run this function again if m_calculateHardness became true in
  // between the "while" check and the unlocking of the mutex
  if (m_calculateHardness)
    _startHardnessResubmissionThread();
}

void OptBase::calculateHardness(Structure* s)
{
  // If we are not to calculate hardness, do nothing
  if (!m_calculateHardness)
    return;

  // Convert the structure to a POSCAR file
  std::stringstream ss;
  PoscarFormat::write(*s, ss);

  qDebug() << "Submitting structure" << s->getTag() << "for Aflow ML calculation...";

  size_t ind = m_aflowML->submitPoscar(ss.str().c_str());
  m_pendingHardnessCalculations[ind] = s;
}

void OptBase::resubmitUnfinishedHardnessCalcs()
{
  if (!m_calculateHardness)
    return;

  QReadLocker trackerLocker(m_tracker->rwLock());
  QList<Structure*> structures = m_queue->getAllOptimizedStructures();
  structures.append(m_queue->getAllDuplicateStructures());
  structures.append(m_queue->getAllSupercellStructures());
  for (auto& s : structures) {
    if (s->vickersHardness() < 0.0)
      calculateHardness(s);
  }
}

void OptBase::_finishHardnessCalculation(size_t ind)
{
  // Let's use a mutex so this function can't be run in multiple threads at
  // once
  static std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);

  // First, make sure we have this index
  auto it = m_pendingHardnessCalculations.find(ind);

  if (it == m_pendingHardnessCalculations.end()) {
    qDebug() << "Error in" << __FUNCTION__
             << ": Received hardness data for index" << ind << ", but could"
             << "not find the structure for this index!";
    return;
  }

  Structure* s = it->second;
  m_pendingHardnessCalculations.erase(ind);

  qDebug() << "Received Aflow ML data for structure" << s->getTag();

  // Make sure AflowML actually has the data
  if (!m_aflowML->containsData(ind)) {
    qDebug() << "Error in" << __FUNCTION__
             << ": Received hardness data for index" << ind << ", but could"
             << "not find the AflowMLData for this index!";
    return;
  }

  AflowMLData data = m_aflowML->data(ind);
  m_aflowML->eraseData(ind);

  // Also make sure the structure is still in the tracker
  // Just skip over it if it isn't
  QReadLocker trackerLocker(m_tracker->rwLock());
  if (!m_tracker->contains(s))
    return;

  double bulkModulus = atof(data["ml_ael_bulk_modulus_vrh"].c_str());
  double shearModulus = atof(data["ml_ael_shear_modulus_vrh"].c_str());

  //double k = shearModulus / bulkModulus;

  // The Chen model: 2.0 * (k^2 * shear)^0.585 - 3.0
  //double hardness = 2.0 * pow((pow(k, 2.0) * shearModulus), 0.585) - 3.0;

  // The Teter model: 0.151 * shear
  double hardness = 0.151 * shearModulus;

  QWriteLocker structureLocker(&s->lock());
  s->setBulkModulus(bulkModulus);
  s->setShearModulus(shearModulus);
  s->setVickersHardness(hardness);

  emit doneWithHardness(s);
}

void OptBase::finishHardnessCalculation(size_t ind)
{
  // Run in a separate thread
  QtConcurrent::run(this, &OptBase::_finishHardnessCalculation, ind);
}

bool OptBase::save(QString stateFilename, bool notify)
{
  if (isStarting || readOnly)
    return false;

  QReadLocker trackerLocker(m_tracker->rwLock());
  QMutexLocker locker(stateFileMutex);
  QString filename;
  if (stateFilename.isEmpty()) {
    filename = locWorkDir + "/" + m_idString.toLower() + ".state";
  } else {
    filename = stateFilename;
  }
  QString oldfilename = filename + ".old";

  if (notify && m_dialog) {
    m_dialog->startProgressUpdate(tr("Saving: Writing %1...").arg(filename), 0,
                                  0);
  }

  SETTINGS(filename);

  // Copy .state -> .state.old
  if (QFile::exists(filename)) {
    // Only copy over if the current state is valid
    const bool saveSuccessful =
      settings->value(m_idString.toLower().append("/saveSuccessful"), false)
        .toBool();
    if (saveSuccessful) {
      if (QFile::exists(oldfilename)) {
        QFile::remove(oldfilename);
      }
      QFile::copy(filename, oldfilename);
    }
  }

  const int version = m_schemaVersion;
  settings->beginGroup(m_idString.toLower());
  settings->setValue("version", version);
  settings->setValue("saveSuccessful", false);
  settings->endGroup();

  // Write/update .state
  if (m_dialog)
    m_dialog->writeSettings(filename);

  // Loop over structures and save them
  QList<Structure*>* structures = m_tracker->list();

  QString structureStateFileName, oldStructureStateFileName;

  Structure* structure;
  for (int i = 0; i < structures->size(); i++) {
    structure = structures->at(i);
    QReadLocker structureLocker(&structure->lock());
    // Set index here -- this is the only time these are written, so
    // this is "ok" under a read lock because of the savePending logic
    structure->setIndex(i);
    structureStateFileName = structure->getLocpath() + "/structure.state";
    oldStructureStateFileName = structureStateFileName + ".old";

    // We are going to write to structure.state.old if one already exists
    // and is a valid state file. This is done in response to
    // structure.state files being mysteriously empty on rare occasions...
    if (QFile::exists(structureStateFileName)) {

      // Attempt to open state file. We will make sure it is valid
      QFile file(structureStateFileName);
      if (!file.open(QIODevice::ReadOnly)) {
        error("OptBase::save(): Error opening file " + structureStateFileName +
              " for reading...");
        return false;
      }

      // If the state file is empty or if saveSuccessful is false,
      // stateFileIsValid will be false. This will hopefully not interfere
      // with the previous SETTINGS() declared by hiding it with scoping.
      SETTINGS(structureStateFileName);
      bool stateFileIsValid =
        settings->value("structure/saveSuccessful", false).toBool();

      // Copy it over if it's a valid state file...
      if (stateFileIsValid) {
        if (QFile::exists(oldStructureStateFileName)) {
          QFile::remove(oldStructureStateFileName);
        }
        QFile::copy(structureStateFileName, oldStructureStateFileName);
      }
    }

    if (notify && m_dialog) {
      m_dialog->updateProgressLabel(
        tr("Saving: Writing %1...").arg(structureStateFileName));
    }
    structure->writeSettings(structureStateFileName);

    // Special request from Eva: if we are using VASP and we encounter
    // a structure that skipped optimization (primitive reduction, for
    // instance), still write the CONTCAR in the structure directory.
    if (structure->skippedOptimization() &&
        optimizer(getNumOptSteps() - 1)->getIDString() == "VASP") {
      QFile file(structure->getLocpath() + "/CONTCAR");
      file.open(QIODevice::WriteOnly);

      std::stringstream ss;
      PoscarFormat::write(*structure, ss);
      file.write(ss.str().c_str());
    }
  }

  /////////////////////////
  // Print results files //
  /////////////////////////

  // Only print the results file if we have a file path
  if (!locWorkDir.isEmpty()) {
    QFile file(locWorkDir + "/results.txt");
    QFile oldfile(locWorkDir + "/results_old.txt");
    if (notify && m_dialog) {
      m_dialog->updateProgressLabel(
        tr("Saving: Writing %1...").arg(file.fileName()));
    }
    if (oldfile.open(QIODevice::ReadOnly))
      oldfile.remove();
    if (file.open(QIODevice::ReadOnly))
      file.copy(oldfile.fileName());
    file.close();
    if (!file.open(QIODevice::WriteOnly)) {
      error("OptBase::save(): Error opening file " + file.fileName() +
            " for writing...");
      return false;
    }
    QTextStream out(&file);

    QList<Structure*> sortedStructures;

    for (int i = 0; i < structures->size(); i++)
      sortedStructures.append(structures->at(i));
    if (sortedStructures.size() != 0) {
      Structure::sortAndRankByEnthalpy(&sortedStructures);
      out << sortedStructures.first()->getResultsHeader(m_calculateHardness, getObjectivesNum())
          << endl;
    }

    for (int i = 0; i < sortedStructures.size(); i++) {
      structure = sortedStructures.at(i);
      if (!structure)
        continue; // In case there was a problem copying.
      QReadLocker structureLocker(&structure->lock());
      out << structure->getResultsEntry(m_calculateHardness, getObjectivesNum(),
                                        structure->getCurrentOptStep()) << endl;
      structureLocker.unlock();
      if (notify && m_dialog) {
        m_dialog->stopProgressUpdate();
      }
    }
  }

  // Write the user values to the output
  writeUserValuesToSettings(structureStateFileName.toStdString());

  // Write the template settings to the output file
  writeAllTemplatesToSettings(structureStateFileName.toStdString());

  // Mark operation successful
  settings->setValue(m_idString.toLower() + "/saveSuccessful", true);

  return true;
}

QString OptBase::interpretTemplate(const QString& str, Structure* structure)
{
  QStringList list = str.split("%");
  QString line;
  QString origLine;
  for (int line_ind = 0; line_ind < list.size(); line_ind++) {
    origLine = line = list.at(line_ind);
    interpretKeyword_base(line, structure);
    // Add other interpret keyword sections here if needed when subclassing
    if (line != origLine) { // Line was a keyword
      list.replace(line_ind, line);
    }
  }
  // Rejoin string
  QString ret = list.join("");
  ret += "\n";
  return ret;
}

void OptBase::interpretKeyword_base(QString& line, Structure* structure)
{
  QString rep = "";
  // User data
  if (line == "user1")
    rep += getUser1().c_str();
  else if (line == "user2")
    rep += getUser2().c_str();
  else if (line == "user3")
    rep += getUser3().c_str();
  else if (line == "user4")
    rep += getUser4().c_str();
  else if (line == "description")
    rep += description;
  else if (line == "percent")
    rep += "%";

  // Structure specific data
  if (line == "coords") {
    std::vector<GlobalSearch::Atom>& atoms = structure->atoms();
    std::vector<GlobalSearch::Atom>::const_iterator it;
    for (it = atoms.begin(); it != atoms.end(); it++) {
      rep += (QString(ElemInfo::getAtomicSymbol((*it).atomicNumber()).c_str()) +
              " ");
      const Vector3& vec = (*it).pos();
      rep += QString::number(vec.x()) + " ";
      rep += QString::number(vec.y()) + " ";
      rep += QString::number(vec.z()) + "\n";
    }
  } else if (line == "coordsInternalFlags") {
    std::vector<GlobalSearch::Atom>& atoms = structure->atoms();
    std::vector<GlobalSearch::Atom>::const_iterator it;
    for (it = atoms.begin(); it != atoms.end(); it++) {
      rep += (QString(ElemInfo::getAtomicSymbol((*it).atomicNumber()).c_str()) +
              " ");
      const Vector3& vec = (*it).pos();
      rep += QString::number(vec.x()) + " 1 ";
      rep += QString::number(vec.y()) + " 1 ";
      rep += QString::number(vec.z()) + " 1\n";
    }
  } else if (line == "coordsSuffixFlags") {
    std::vector<GlobalSearch::Atom>& atoms = structure->atoms();
    std::vector<GlobalSearch::Atom>::const_iterator it;
    for (it = atoms.begin(); it != atoms.end(); it++) {
      rep += (QString(ElemInfo::getAtomicSymbol((*it).atomicNumber()).c_str()) +
              " ");
      const Vector3& vec = (*it).pos();
      rep += QString::number(vec.x()) + " ";
      rep += QString::number(vec.y()) + " ";
      rep += QString::number(vec.z()) + " 1 1 1\n";
    }
  } else if (line == "coordsId") {
    std::vector<GlobalSearch::Atom>& atoms = structure->atoms();
    std::vector<GlobalSearch::Atom>::const_iterator it;
    for (it = atoms.begin(); it != atoms.end(); it++) {
      rep += (QString(ElemInfo::getAtomicSymbol((*it).atomicNumber()).c_str()) +
              " ");
      rep += QString::number((*it).atomicNumber()) + " ";
      const Vector3& vec = (*it).pos();
      rep += QString::number(vec.x()) + " ";
      rep += QString::number(vec.y()) + " ";
      rep += QString::number(vec.z()) + "\n";
    }
  } else if (line == "numAtoms")
    rep += QString::number(structure->numAtoms());
  else if (line == "numSpecies")
    rep += QString::number(structure->getSymbols().size());
  else if (line == "filename")
    rep += structure->getLocpath();
  else if (line == "rempath")
    rep += structure->getRempath();
  else if (line == "gen")
    rep += QString::number(structure->getGeneration());
  else if (line == "id")
    rep += QString::number(structure->getIDNumber());
  else if (line == "incar")
    rep += QString::number(structure->getCurrentOptStep());
  else if (line == "optStep")
    rep += QString::number(structure->getCurrentOptStep());
  else if (line.startsWith("filecontents:", Qt::CaseInsensitive)) {
    QString filename = line;
    filename.remove(0, QString("filecontents:").size());
    filename = filename.trimmed();
    // Attempt to open the file
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
      qDebug() << "Error in" << __FUNCTION__ << ": could not open" << filename;
    }
    rep += file.readAll();
  }
  // Append a file to be copied to the working dir
  else if (line.startsWith("copyfile:", Qt::CaseInsensitive)) {
    QString filename = line;
    filename.remove(0, QString("copyfile:").size());
    filename = filename.trimmed();
    structure->appendCopyFile(filename.toStdString());
    line = "";
  }

  if (!rep.isEmpty()) {
    // Remove any trailing newlines
    rep = rep.replace(QRegExp("\n$"), "");
    line = rep;
  }
}

QString OptBase::getTemplateKeywordHelp_base()
{
  QString str;
  QTextStream out(&str);
  out << "The following keywords should be used instead of the indicated "
         "variable data:\n"
      << "\n"
      << "Misc:\n"
      << "%fileContents:/path/to/local/file% -- Replaced with the contents of "
      << "the specified file\n"
      << "%copyFile:/path/to/local/file% -- Copy the specified file to the "
      << "structures's working directory\n"
      << "%percent% -- Literal percent sign (needed for CASTEP!)\n"
      << "\n"
      << "User data:\n"
      << "%userX% -- User specified value, where X = 1, 2, 3, or 4\n"
      << "%description% -- Optimization description\n"
      << "\n"
      << "Atomic coordinate formats for isolated structures:\n"
      << "%coords% -- cartesian coordinates\n\t[symbol] [x] [y] [z]\n"
      << "%coordsInternalFlags% -- cartesian coordinates; flag after each "
         "coordinate\n\t[symbol] [x] 1 [y] 1 [z] 1\n"
      << "%coordsSuffixFlags% -- cartesian coordinates; flags after all "
         "coordinates\n\t[symbol] [x] [y] [z] 1 1 1\n"
      << "%coordsId% -- cartesian coordinates with atomic number\n\t[symbol] "
         "[atomic number] [x] [y] [z]\n"
      << "\n"
      << "Generic structure data:\n"
      << "%numAtoms% -- Number of atoms in unit cell\n"
      << "%numSpecies% -- Number of unique atomic species in unit cell\n"
      << "%filename% -- local output filename\n"
      << "%rempath% -- path to structure's remote directory\n"
      << "%gen% -- structure generation number (if relevant)\n"
      << "%id% -- structure id number\n"
      << "%optStep% -- current optimization step\n";
  return str;
}

std::unique_ptr<QueueInterface> OptBase::createQueueInterface(
  const std::string& queueName)
{
  qDebug() << "Error:" << __FUNCTION__ << "not implemented. It needs to"
           << "be overridden in a derived class.";
  return nullptr;
}

std::unique_ptr<Optimizer> OptBase::createOptimizer(const std::string& optName)
{
  qDebug() << "Error:" << __FUNCTION__ << "not implemented. It needs to"
           << "be overridden in a derived class.";
  return nullptr;
}

QueueInterface* OptBase::queueInterface(int optStep) const
{
  if (optStep >= getNumOptSteps()) {
    qDebug() << "Error in" << __FUNCTION__ << ": optStep," << optStep
             << ", is out of bounds! The number of optimization steps is:"
             << getNumOptSteps();
    return nullptr;
  }
  return m_queueInterfaceAtOptStep[optStep].get();
}

int OptBase::queueInterfaceIndex(const QueueInterface* qi) const
{
  for (size_t i = 0; i < m_queueInterfaceAtOptStep.size(); ++i) {
    if (qi == m_queueInterfaceAtOptStep[i].get())
      return i;
  }
  return -1;
}

Optimizer* OptBase::optimizer(int optStep) const
{
  if (optStep >= getNumOptSteps()) {
    qDebug() << "Error in" << __FUNCTION__ << ": optStep," << optStep
             << ", is out of bounds! The number of optimization steps is:"
             << getNumOptSteps();
    return nullptr;
  }
  return m_optimizerAtOptStep[optStep].get();
}

int OptBase::optimizerIndex(const Optimizer* optimizer) const
{
  for (size_t i = 0; i < m_optimizerAtOptStep.size(); ++i) {
    if (optimizer == m_optimizerAtOptStep[i].get())
      return i;
  }
  return -1;
}

void OptBase::clearOptSteps()
{
  m_queueInterfaceAtOptStep.clear();
  m_optimizerAtOptStep.clear();
  m_queueInterfaceTemplates.clear();
  m_optimizerTemplates.clear();
  m_numOptSteps = 0;
}

void OptBase::appendOptStep()
{
  // If there are no opt steps, we can't copy previous ones
  if (m_numOptSteps == 0) {
    typedef std::map<std::string, std::string> templateMap;
    m_queueInterfaceAtOptStep.push_back(nullptr);
    m_optimizerAtOptStep.push_back(nullptr);
    m_queueInterfaceTemplates.push_back(templateMap());
    m_optimizerTemplates.push_back(templateMap());
  }
  // We will duplicate the most recent opt step otherwise
  else {
    m_queueInterfaceAtOptStep.push_back(createQueueInterface(
      m_queueInterfaceAtOptStep.back()->getIDString().toStdString()));
    m_optimizerAtOptStep.push_back(createOptimizer(
      m_optimizerAtOptStep.back()->getIDString().toStdString()));
    m_queueInterfaceTemplates.push_back(m_queueInterfaceTemplates.back());
    m_optimizerTemplates.push_back(m_optimizerTemplates.back());
  }

  ++m_numOptSteps;
}

void OptBase::insertOptStep(size_t optStep)
{
  // If we are adding an opt step to the end, just use the append function
  if (optStep == m_numOptSteps) {
    appendOptStep();
    return;
  }

  if (optStep > m_numOptSteps) {
    qDebug() << "Error in" << __FUNCTION__ << ": attempting to insert"
             << "an opt step," << optStep << ", that is greater than"
             << "the number of opt steps," << m_numOptSteps;
    return;
  }

  // We will copy another step. Figure out the index of the one we will copy.
  // We will copy the one immediately prior to optStep in most cases, but
  // if optStep is 0, we will copy the first item already present
  size_t copyInd = (optStep == 0 ? 0 : optStep - 1);
  m_queueInterfaceAtOptStep.insert(
    m_queueInterfaceAtOptStep.begin() + optStep,
    createQueueInterface(
      m_queueInterfaceAtOptStep[copyInd]->getIDString().toStdString()));
  m_optimizerAtOptStep.insert(
    m_optimizerAtOptStep.begin() + optStep,
    createOptimizer(
      m_optimizerAtOptStep[copyInd]->getIDString().toStdString()));

  m_queueInterfaceTemplates.insert(m_queueInterfaceTemplates.begin() + optStep,
                                   m_queueInterfaceTemplates[copyInd]);

  m_optimizerTemplates.insert(m_optimizerTemplates.begin() + optStep,
                              m_optimizerTemplates[copyInd]);

  ++m_numOptSteps;
}

void OptBase::removeOptStep(size_t optStep)
{
  if (optStep >= m_numOptSteps) {
    qDebug() << "Error in" << __FUNCTION__ << ": attempting to remove"
             << "an opt step," << optStep << ", that is out of bounds.\n"
             << "The number of opt steps is" << m_numOptSteps;
    return;
  }

  m_queueInterfaceAtOptStep.erase(m_queueInterfaceAtOptStep.begin() + optStep);
  m_optimizerAtOptStep.erase(m_optimizerAtOptStep.begin() + optStep);

  m_queueInterfaceTemplates.erase(m_queueInterfaceTemplates.begin() + optStep);
  m_optimizerTemplates.erase(m_optimizerTemplates.begin() + optStep);

  --m_numOptSteps;
}

void OptBase::setQueueInterface(size_t optStep, const std::string& qiName)
{
  if (optStep >= m_numOptSteps) {
    qDebug() << "Error in" << __FUNCTION__ << ": optStep," << optStep
             << ", is out of bounds. Number of opt steps is" << m_numOptSteps;
    return;
  }
  m_queueInterfaceAtOptStep[optStep] = createQueueInterface(qiName);

  // We need to populate the templates list with empty templates
  for (const auto& templateName :
       m_queueInterfaceAtOptStep[optStep]->getTemplateFileNames()) {
    setQueueInterfaceTemplate(optStep, templateName.toStdString(), "");
  }
}

std::string OptBase::getQueueInterfaceTemplate(size_t optStep,
                                               const std::string& name) const
{
  if (optStep >= m_numOptSteps) {
    qDebug() << "Error in" << __FUNCTION__ << ": optStep," << optStep
             << ", is out of bounds. Number of opt steps is" << m_numOptSteps;
    return "";
  }
  if (m_queueInterfaceTemplates[optStep].count(name) == 0) {
    qDebug() << "Error in" << __FUNCTION__ << ": invalid key entry"
             << "Name:" << name.c_str() << ", for opt step:" << optStep;
    return "";
  }
  return m_queueInterfaceTemplates[optStep].at(name);
}

void OptBase::setQueueInterfaceTemplate(size_t optStep, const std::string& name,
                                        const std::string& temp)
{
  if (optStep >= m_numOptSteps) {
    qDebug() << "Error in" << __FUNCTION__ << ": optStep," << optStep
             << ", is out of bounds. Number of opt steps is" << m_numOptSteps;
    return;
  }
  m_queueInterfaceTemplates[optStep][name] = temp;
}

void OptBase::setOptimizer(size_t optStep, const std::string& optName)
{
  if (optStep >= m_numOptSteps) {
    qDebug() << "Error in" << __FUNCTION__ << ": optStep," << optStep
             << ", is out of bounds. Number of opt steps is" << m_numOptSteps;
    return;
  }
  m_optimizerAtOptStep[optStep] = createOptimizer(optName);

  // We need to populate the templates list with empty templates
  for (const auto& templateName :
       m_optimizerAtOptStep[optStep]->getTemplateFileNames()) {
    setOptimizerTemplate(optStep, templateName.toStdString(), "");
  }
}

std::string OptBase::getOptimizerTemplate(size_t optStep,
                                          const std::string& name) const
{
  if (optStep >= m_numOptSteps) {
    qDebug() << "Error in" << __FUNCTION__ << ": optStep," << optStep
             << ", is out of bounds. Number of opt steps is" << m_numOptSteps;
    return "";
  }
  if (m_optimizerTemplates[optStep].count(name) == 0) {
    qDebug() << "Error in" << __FUNCTION__ << ": invalid key entry"
             << "Name:" << name.c_str() << ", for opt step:" << optStep;
    return "";
  }
  return m_optimizerTemplates[optStep].at(name);
}

void OptBase::setOptimizerTemplate(size_t optStep, const std::string& name,
                                   const std::string& temp)
{
  if (optStep >= m_numOptSteps) {
    qDebug() << "Error in" << __FUNCTION__ << ": optStep," << optStep
             << ", is out of bounds. Number of opt steps is" << m_numOptSteps;
    return;
  }
  m_optimizerTemplates[optStep][name] = temp;
}

OptBase::TemplateType OptBase::getTemplateType(size_t optStep,
                                               const std::string& name) const
{
  TemplateType ret = TT_Unknown;

  if (optStep >= m_numOptSteps) {
    qDebug() << "Error in" << __FUNCTION__ << ": optStep," << optStep
             << ", is out of range! Num opt steps is: " << m_numOptSteps;
    return ret;
  }

  if (queueInterface(optStep) &&
      queueInterface(optStep)->isTemplateFileName(name.c_str())) {
    if (ret != TT_Unknown) {
      qDebug() << "Error: in" << __FUNCTION__ << ": template name,"
               << name.c_str() << ", in multiple template types!";
      ret = TT_Unknown;
      return ret;
    }
    ret = TT_QueueInterface;
  }

  if (optimizer(optStep) &&
      optimizer(optStep)->isTemplateFileName(name.c_str())) {
    if (ret != TT_Unknown) {
      qDebug() << "Error: in" << __FUNCTION__ << ": template name,"
               << name.c_str() << ", in multiple template types!";
      ret = TT_Unknown;
      return ret;
    }
    ret = TT_Optimizer;
  }

  if (ret == TT_Unknown) {
    qDebug() << "Error in" << __FUNCTION__
             << ": unknown template type: " << name.c_str();
  }

  return ret;
}

std::string OptBase::getTemplate(size_t optStep, const std::string& name) const
{
  TemplateType type = getTemplateType(optStep, name);

  if (type == TT_Unknown)
    return "";

  if (type == TT_QueueInterface)
    return getQueueInterfaceTemplate(optStep, name);

  if (type == TT_Optimizer)
    return getOptimizerTemplate(optStep, name);

  // We should never make it here
  return "";
}

void OptBase::setTemplate(size_t optStep, const std::string& name,
                          const std::string& temp)
{
  TemplateType type = getTemplateType(optStep, name);

  if (type == TT_Unknown)
    return;

  if (type == TT_QueueInterface)
    setQueueInterfaceTemplate(optStep, name, temp);

  if (type == TT_Optimizer)
    setOptimizerTemplate(optStep, name, temp);

  // We should never make it here
}

void OptBase::readQueueInterfaceTemplatesFromSettings(
  size_t optStep, const std::string& settingsFile)
{
  QueueInterface* queue = queueInterface(optStep);
  if (!queue) {
    qDebug() << "Error in " << __FUNCTION__ << ": queue interface at"
             << "opt step" << optStep << "does not exist!";
    return;
  }

  SETTINGS(settingsFile.c_str());

  settings->beginGroup(getIDString().toLower() + "/edit/queueInterface/" +
                       QString::number(optStep) + "/" +
                       queue->getIDString().toLower());
  QStringList filenames = queue->getTemplateFileNames();
  for (const auto& filename : filenames) {
    QString temp = settings->value(filename).toString();
    setQueueInterfaceTemplate(optStep, filename.toStdString(),
                              temp.toStdString());
  }
  settings->endGroup();
}

void OptBase::readOptimizerTemplatesFromSettings(
  size_t optStep, const std::string& settingsFile)
{
  Optimizer* optim = optimizer(optStep);
  if (!optim) {
    qDebug() << "Error in " << __FUNCTION__ << ": optimizer at"
             << "opt step" << optStep << "does not exist!";
    return;
  }

  SETTINGS(settingsFile.c_str());

  optim->setLocalRunCommand(settings->value(getIDString().toLower() + "/edit/optimizer/" +
                            QString::number(optStep) + "/exeLocation",
                            optim->getIDString().toLower()).toString());
  settings->beginGroup(getIDString().toLower() + "/edit/optimizer/" +
                       QString::number(optStep) + "/" +
                       optim->getIDString().toLower());
  QStringList filenames = optim->getTemplateFileNames();
  for (const auto& filename : filenames) {
    QString temp = settings->value(filename).toString();

    setOptimizerTemplate(optStep, filename.toStdString(), temp.toStdString());

    if (!temp.isEmpty())
      continue;

    // If "temp" is empty, perhaps we have some template filenames to open
    QString templateFile = settings->value(filename + "_templates").toString();

    if (templateFile.isEmpty())
      continue;

    QFile file(templateFile);

    // If the file exists, store it in the templates
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

    setOptimizerTemplate(optStep, filename.toStdString(),
                         QString(file.readAll()).toStdString());
    file.close();
  }
  settings->endGroup();
}

void OptBase::readTemplatesFromSettings(size_t optStep,
                                        const std::string& filename)
{
  readQueueInterfaceTemplatesFromSettings(optStep, filename);
  readOptimizerTemplatesFromSettings(optStep, filename);
}

void OptBase::readAllTemplatesFromSettings(const std::string& filename)
{
  SETTINGS(filename.c_str());
  settings->beginGroup(getIDString().toLower() + "/edit");
  size_t numOptSteps = settings->value("numOptSteps").toUInt();
  while (getNumOptSteps() < numOptSteps)
    appendOptStep();

  for (size_t i = 0; i < getNumOptSteps(); ++i)
    readTemplatesFromSettings(i, filename);
  settings->endGroup();
}

void OptBase::writeQueueInterfaceTemplatesToSettings(
  size_t optStep, const std::string& settingsFilename)
{
  QueueInterface* queue = queueInterface(optStep);
  if (!queue) {
    qDebug() << "Error in " << __FUNCTION__ << ": queue interface at"
             << "opt step" << optStep << "does not exist!";
    return;
  }

  SETTINGS(settingsFilename.c_str());
  // QueueInterface templates
  settings->beginGroup(getIDString().toLower() + "/edit/queueInterface/" +
                       QString::number(optStep) + "/" +
                       queue->getIDString().toLower());

  QStringList filenames = queue->getTemplateFileNames();
  for (const auto& filename : filenames) {
    settings->setValue(
      filename,
      getQueueInterfaceTemplate(optStep, filename.toStdString()).c_str());
  }
  settings->endGroup();
}

void OptBase::writeOptimizerTemplatesToSettings(
  size_t optStep, const std::string& settingsFilename)
{
  Optimizer* optim = optimizer(optStep);
  if (!optim) {
    qDebug() << "Error in " << __FUNCTION__ << ": optimizer at"
             << "opt step" << optStep << "does not exist!";
    return;
  }

  SETTINGS(settingsFilename.c_str());
  // Optimizer templates
  settings->setValue(getIDString().toLower() + "/edit/optimizer/" +
                     QString::number(optStep) + "/exeLocation",
                     optim->localRunCommand());
  settings->beginGroup(getIDString().toLower() + "/edit/optimizer/" +
                       QString::number(optStep) + "/" +
                       optim->getIDString().toLower());

  QStringList filenames = optim->getTemplateFileNames();
  for (const auto& filename : filenames) {
    settings->setValue(
      filename, getOptimizerTemplate(optStep, filename.toStdString()).c_str());
  }
  settings->endGroup();
}

void OptBase::writeTemplatesToSettings(size_t optStep,
                                       const std::string& filename)
{
  writeQueueInterfaceTemplatesToSettings(optStep, filename);
  writeOptimizerTemplatesToSettings(optStep, filename);
}

void OptBase::writeAllTemplatesToSettings(const std::string& filename)
{
  SETTINGS(filename.c_str());
  settings->beginGroup(getIDString().toLower() + "/edit");
  settings->setValue("numOptSteps", QString::number(getNumOptSteps()));
  for (size_t i = 0; i < getNumOptSteps(); ++i)
    writeTemplatesToSettings(i, filename);
  settings->endGroup();
}

void OptBase::readUserValuesFromSettings(const std::string& filename)
{
  SETTINGS(filename.c_str());

  settings->beginGroup(getIDString().toLower() + "/edit");
  m_user1 = settings->value("/user1", "").toString().toStdString();
  m_user2 = settings->value("/user2", "").toString().toStdString();
  m_user3 = settings->value("/user3", "").toString().toStdString();
  m_user4 = settings->value("/user4", "").toString().toStdString();
  settings->endGroup();
}

void OptBase::writeUserValuesToSettings(const std::string& filename)
{
  SETTINGS(filename.c_str());

  settings->beginGroup(getIDString().toLower() + "/edit");

  settings->setValue("/user1", m_user1.c_str());
  settings->setValue("/user2", m_user2.c_str());
  settings->setValue("/user3", m_user3.c_str());
  settings->setValue("/user4", m_user4.c_str());

  settings->endGroup();
}

bool OptBase::isReadyToSearch(QString& err) const
{
  err.clear();
  if (locWorkDir.isEmpty()) {
    err += "Local working directory is not set.";
    return false;
  }

  for (size_t i = 0; i < getNumOptSteps(); ++i) {
    if (!queueInterface(i)) {
      err += "Queue interface at opt step " + QString::number(i + 1) +
             " is not set!";
      return false;
    }

    if (!optimizer(i)) {
      err += "Optimizer at opt step " + QString::number(i + 1) + " is not set!";
      return false;
    }

    if (!queueInterface(i)->isReadyToSearch(&err))
      return false;

    if (!optimizer(i)->isReadyToSearch(&err))
      return false;
  }

  return true;
}

bool OptBase::anyRemoteQueueInterfaces() const
{
  for (size_t i = 0; i < getNumOptSteps(); ++i) {
    if (queueInterface(i)->getIDString().toLower() != "local")
      return true;
  }
  return false;
}

void OptBase::promptForPassword(const QString& message, QString* newPassword,
                                bool* ok)
{
  if (m_usingGUI) {
    (*newPassword) = QInputDialog::getText(dialog(), "Need password:", message,
                                           QLineEdit::Password, QString(), ok);
  } else {
    (*newPassword) = PasswordPrompt::getPassword().c_str();
  }
}

void OptBase::promptForBoolean(const QString& message, bool* ok)
{
  if (m_usingGUI) {
    if (QMessageBox::question(dialog(), m_idString, message,
                              QMessageBox::Yes | QMessageBox::No) ==
        QMessageBox::Yes) {
      *ok = true;
    } else {
      *ok = false;
    }
  } else {
    std::cout << message.toStdString() << "\n[y/N]\n";
    std::string in;
    std::getline(std::cin, in);
    in = trim(in);
    if (in.size() != 0 && (in[0] == 'y' || in[0] == 'Y'))
      *ok = true;
    else
      *ok = false;
  }
}

void OptBase::setClipboard(const QString& text) const
{
  emit sig_setClipboard(text);
}

// No need to document this
/// @cond
void OptBase::setClipboard_(const QString& text) const
{
  // Set to system clipboard
  QApplication::clipboard()->setText(text, QClipboard::Clipboard);
  // For middle-click on X11
  if (QApplication::clipboard()->supportsSelection()) {
    QApplication::clipboard()->setText(text, QClipboard::Selection);
  }
}
/// @endcond

#ifdef ENABLE_SSH
#ifndef USE_CLI_SSH

bool OptBase::createSSHConnections_libssh()
{
  delete m_ssh;
  SSHManagerLibSSH* libsshManager = new SSHManagerLibSSH(5, this);
  m_ssh = libsshManager;
  QString pw = "";
  for (;;) {
    try {
      libsshManager->makeConnections(host, username, pw, port);
    } catch (SSHConnection::SSHConnectionException e) {
      QString err;
      switch (e) {
        case SSHConnection::SSH_CONNECTION_ERROR:
        case SSHConnection::SSH_UNKNOWN_ERROR:
        default:
          err = "There was a problem connection to the ssh server at " +
                username + "@" + host + ":" + QString::number(port) +
                ". "
                "Please check that all provided information is correct, and "
                "attempt to log in outside of Avogadro before trying again.";
          error(err);
          return false;
        case SSHConnection::SSH_UNKNOWN_HOST_ERROR: {
          // The host is not known, or has changed its key.
          // Ask user if this is ok.
          err = "The host " + host + ":" + QString::number(port) +
                " either has an unknown key, or has changed it's key:\n" +
                libsshManager->getServerKeyHash() + "\n" +
                "Would you like to trust the specified host?";
          bool ok;
          // This is a BlockingQueuedConnection, which blocks until
          // the slot returns.
          emit needBoolean(err, &ok);
          if (!ok) { // user cancels
            return false;
          }
          libsshManager->validateServerKey();
          continue;
        } // end case
        case SSHConnection::SSH_BAD_PASSWORD_ERROR: {
          // Chances are that the pubkey auth was attempted but failed,
          // so just prompt user for password.
          err = "Please enter a password for " + username + "@" + host + ":" +
                QString::number(port) + ": ";
          bool ok;
          QString newPassword;

          if (m_usingGUI)
            // This is a BlockingQueuedConnection, which blocks until
            // the slot returns.
            emit needPassword(err, &newPassword, &ok);
          else {
            newPassword =
              PasswordPrompt::getPassword(err.toStdString()).c_str();
            ok = true;
          }

          if (!ok) { // user cancels
            return false;
          }
          pw = newPassword;
          continue;
        } // end case
      }   // end switch
    }     // end catch
    break;
  } // end forever
  return true;
}

#else // not USE_CLI_SSH

bool OptBase::createSSHConnections_cli()
{
  // Since we rely on public key auth here, it's much easier to set up:
  SSHManagerCLI* cliSSHManager = new SSHManagerCLI(5, this);
  cliSSHManager->makeConnections(host, username, "", port);
  m_ssh = cliSSHManager;
  return true;
}

#endif // not USE_CLI_SSH
#endif // ENABLE_SSH

void OptBase::warning(const QString& s)
{
  qWarning() << "Warning: " << s;
  emit warningStatement(s);
}

void OptBase::debug(const QString& s)
{
  qDebug() << "Debug: " << s;
  emit debugStatement(s);
}

void OptBase::error(const QString& s)
{
  qWarning() << "Error: " << s;
  emit errorStatement(s);
}

void OptBase::message(const QString& s)
{
  qDebug() << "Message: " << s;
  emit messageStatement(s);
}

} // end namespace GlobalSearch

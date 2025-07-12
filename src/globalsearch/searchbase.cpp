/**********************************************************************
  SearchBase - Base class for global search extensions

  Copyright (C) 2010-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <globalsearch/searchbase.h>

#include <globalsearch/constants.h>
#include <globalsearch/bt.h>
#include <globalsearch/eleminfo.h>
#include <globalsearch/formats/poscarformat.h>
#include <globalsearch/macros.h>
#include <globalsearch/optimizer.h>
#include <globalsearch/queueinterface.h>
#include <globalsearch/queuemanager.h>
#include <globalsearch/fitness.h>
#include <globalsearch/chull.h>
#include <globalsearch/random.h>
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

#include <fstream>
#include <iostream>
#include <mutex>
#include <chrono>
#include <thread>

// Uncomment for yet more debug info about probabilities
//#define SEARCHBASE_PROBS_DEBUG

namespace GlobalSearch {

SearchBase::SearchBase(AbstractDialog* parent)
  : QObject(parent),
    maxNumStructures(100), testingMode(false), test_nRunsStart(1), test_nRunsEnd(100), // FIXME
    test_nStructs(600), stateFileMutex(new QMutex), readOnly(false),
    m_idString("Generic"),
#ifdef ENABLE_SSH
    m_ssh(nullptr),
#endif // ENABLE_SSH
    m_dialog(parent), m_tracker(new Tracker(this)), m_queueThread(new QThread),
    m_queue(new QueueManager(m_queueThread, this)), m_numOptSteps(0),
    m_schemaVersion(4), m_usingGUI(true),
    m_logErrorDirs(false),
    m_calculateObjectives(false), m_objectivesReDo(false),
    m_softExit(false), m_hardExit(false), m_localQueue(false),
    m_optimizationType("basic"),
    m_restrictedPool(false), m_tournamentSelection(true),
    m_verbose(false), m_saveHullSnapshots(false)
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
          this,    &SearchBase::calculateObjectives, Qt::QueuedConnection);
  connect(m_queue, &QueueManager::structureFinished,
          [this]() { QtConcurrent::run([this]() { this->updateHullAndFrontInfo(); }); });
}

SearchBase::~SearchBase()
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

void SearchBase::reset()
{
  m_tracker->lockForWrite();
  m_tracker->deleteAllStructures();
  m_tracker->reset();
  m_tracker->unlock();
  m_queue->reset();
}

#ifdef ENABLE_SSH
bool SearchBase::createSSHConnections()
{
#ifdef USE_CLI_SSH
  return this->createSSHConnections_cli();
#else  // USE_CLI_SSH
  return this->createSSHConnections_libssh();
#endif // USE_CLI_SSH
}
#endif // ENABLE_SSH

void SearchBase::performTheExit(int delay)
{
  // This functions performs the exit, i.e., terminates the run.
  // The input parameter "delay" has a default of 0. If a non-zero
  //   delay is specified, the function waits for that amount, and
  //   will try to do some clean up before quitting.

  bool runDidNotStart = m_queue->getAllStructures().isEmpty();

  if (delay > 0) {
    // Impose a delay if needed
    QThread::msleep(delay * 1000);

    // Update the hull and Pareto info one last time, and print them
    updateHullAndFrontInfo();

    m_dialog = nullptr;

    warning("Saving XtalOpt settings...");

    // Save the state file
    save("", false);
    // Save the config settings
    QString configFileName = QSettings().fileName();
    save(configFileName, false);

    // Stop queuemanager thread
    if (m_queueThread->isRunning()) {
      m_queueThread->disconnect();
      m_queueThread->quit();
    }

    // Delete queuemanager
    delete m_queue;

#ifdef ENABLE_SSH
    // Stop SSHManager
    delete m_ssh;
#endif // ENABLE_SSH
  }

  if (!runDidNotStart) {
    QString formattedTime = QDateTime::currentDateTime().toString("MMMM dd, yyyy   hh:mm:ss");
    QByteArray formattedTimeMsg = formattedTime.toLocal8Bit();
    qDebug().noquote() << "\n=== Optimization finished ... " + formattedTimeMsg + "\n";
  }

  exit(0);
}

void SearchBase::updateHullAndFrontInfo()
{
  // This function, called every time a new structure is optimized,
  //   calculates and updates the distance above hull and Pareto front
  //   index for all the structures.
  // The calculated above hull values will be used in the selectParent,
  //   but Pareto front indices (ranks) are used only for output files.
  // The workflow here has a lot in common with selectParent!

  QList<Structure*> structures = m_queue->getAllStructures();

  if (structures.isEmpty())
    return;

  //=== Basic variables
  QList<QString> eleSymb = getChemicalSystem(); // "full" list of symbol of elements
  int eleNumb = eleSymb.size();                 // num of elements
  int chlNdim = eleNumb + 1;                    // hull dimensions (elements + energy)
  std::vector<double> strEnth;                  // enthalpy per atom of structures and refs
  QList<QString> strTags;                       // structure tags
  int strNumb = 0;                              // total number of structures
  int refNumb = 0;                              // number of reference structures
  std::vector<double> refData;                  // reference energy data
  std::vector<double> chlData;                  // input for hull calculations
  int objNumb = 0;                              // number of optimizable objectives
  std::vector<double> objWght;                  // weights for optimizable objectives
  std::vector<std::vector<double>> objData;     // input for Pareto front

  //=== Process objectives and their weights
  objNumb = 1;
  objWght.push_back(1.0);
  for (int i = 0; i < getObjectivesNum(); i++)
    if(getObjectivesTyp(i) == SearchBase::Ot_Min || getObjectivesTyp(i) == SearchBase::Ot_Max) {
      objNumb += 1;
      objWght.push_back(getObjectivesWgt(i));
    }
  for (int j = 1; j < objNumb; j++) {
    objWght[0] -= objWght[j];
  }

  //=== Process input structures; and prepare input for hull and Pareto fronts

  QList<Structure*> opt_structures;

  for (int i = 0; i < structures.size(); i++) {
    QReadLocker lock(&structures[i]->lock());

    Structure* s = structures.at(i);

    Structure::State state = s->getStatus();

    // Check if a valid structure (marked as: optimized or similar); if not ignore it.
    if (!(state == Structure::Optimized || state == Structure::Similar))
      continue;

    // So, we have a valid structure for hull calculation
    opt_structures.append(s);
    strNumb += 1;

    // Save tags (will be used for verification)
    strTags.push_back(s->getTag());

    // Update hull input with reference composition and enthalpy.
    double enth = s->getEnthalpy();
    strEnth.push_back(enth);
    for(int j = 0; j < eleNumb; j++) {
      double comp = (double)s->getNumberOfAtomsOfSymbol(eleSymb[j]);
      chlData.push_back(comp);
    }
    chlData.push_back(enth);

    // Construct objective data matrix: for now we just use enthalpy;
    //   but will replace it with distances above hull later on.
    // Also, we only consider "optimizable" objectives, i.e., min/max;
    //   while converting all of them to minimizable objectives for the
    //   fitness calculation.
    std::vector<double> obj_vec;
    obj_vec.push_back(enth);
    for(int j = 0; j < getObjectivesNum(); j++) {
      if (getObjectivesTyp(j) == SearchBase::Ot_Min)
        obj_vec.push_back(s->getStrucObjValues(j));
      else if (getObjectivesTyp(j) == SearchBase::Ot_Max)
        obj_vec.push_back(-s->getStrucObjValues(j));
    }
    objData.push_back(obj_vec);
  }

  //=== Return if no structure left to process
  if (strNumb == 0)
    return;

  //=== Add reference energies to the end of hull input.
  //   (by default, we always have elemental references;
  //    user might have provided their own set, though).
  refData = getReferenceEnergiesVector();
  refNumb = refData.size() / (eleNumb + 1);
  for(int i = 0; i < refNumb; i++) {
    strTags.append(QString("ref%1").arg(i+1));
    for (int j = 0; j < eleNumb + 1; j++)
      chlData.push_back(refData[i * (eleNumb + 1) + j]);
    strEnth.push_back(refData[i * (eleNumb + 1) + eleNumb]);
  }

  // Total number of structures for convex hull calculation
  int chlNumb = strNumb + refNumb;

  //=== Find the distance above hull
  std::vector<double> strHull(chlNumb);
  if (!distAboveHull(chlData, chlNumb, chlNdim, strHull)) {
    // If it didn't work, just return. User will notice!
    return;
  }

  //=== Find Pareto front indices (for output files)
  std::vector<double> objMins(objNumb,  DBL_MAX);
  std::vector<double> objMaxs(objNumb, -DBL_MAX);
  std::vector<double> objSprd(objNumb,  0.0);
  for (int i = 0; i< objNumb; i++) {
    for (int j = 0; j < strNumb; j++) {
      // Adjust energy objective: enthalpy -> distance above hull
      if (i == 0)
        objData[j][i] = strHull[j];
      // Find the minimum and maximum of objective values
      double objval = objData[j][i];
      if(objval < objMins[i])
        objMins[i] = objval;
      if(objval > objMaxs[i])
        objMaxs[i] = objval;
    }
    objSprd[i] = objMaxs[i] - objMins[i];
  }

  // Scale the objective values to [0,1], and apply precision
  for (int i = 0; i < objNumb; i++) {
    for (int j = 0; j < strNumb; j++) {
      objData[j][i] = (objData[j][i] - objMins[i]);
      if (objSprd[i] > ZERO8)
        objData[j][i] /= objSprd[i];
      // Apply the precision (if needed)
      objData[j][i] = roundToDecimalPlaces(objData[j][i], m_objectivePrecision);
    }
  }

  // Perform non-dominated sorting to obtain Pareto fronts
  std::vector<std::vector<int>> strFrnt = nonDominatedSorting(objData);

  //=== Update structure entries
  for (int i = 0; i < strFrnt.size(); i++) {
    for (int j = 0; j < strFrnt[i].size(); j++) {
      int index = strFrnt[i][j];
      QWriteLocker lock(&opt_structures[index]->lock());
      opt_structures[index]->setParetoFront(i);
      opt_structures[index]->setDistAboveHull(strHull[index]);
    }
  }

  //=== Do we need to save the hull snapshots?
  if (m_saveHullSnapshots)
    saveHullSnapshot();

  //=== This is to properly update GUI progress table
  emit m_queue->hullCalculationFinished();

  return;
}

bool SearchBase::saveHullSnapshot()
{
  // If set by the user, after each successful local optimization this function
  //   saves a snapshot of the hull (i.e., a copy of the hull.txt file) in the
  //   "movie" folder in the local working directory.
  if (locWorkDir.isEmpty())
    return false;

  QString path = locWorkDir + "/movie";

  QDir dir(path);

  if (!dir.exists()) {
    if (!QDir().mkpath(path)) {
      qDebug() << "Error: failed to create 'movie' directory";
      return false;
    }
  }

  QList<Structure*>* structures = m_tracker->list();
  Structure* structure;

  QList<QString> eleSymb = getChemicalSystem(); // "full" list of symbol of elements
  QFile file(path + "/" + uniqueTimestampString());
  if (!file.open(QIODevice::WriteOnly)) {
    error("SearchBase::saveHullSnapshot(): Error opening file " + file.fileName() +
          " for writing...");
    return false;
  }
  QTextStream out(&file);
  // Write structure data for those with calculated hull values
  for (int i = 0; i < eleSymb.size(); i++)
    out << QString(" %1").arg(eleSymb[i], 7);
  out << QString(" %1  # %2 %3 %4  %5\n").arg("Enthalpy", 14).arg("AboveHullAtm", 14)
             .arg("Pareto",7).arg("Index", 7).arg("Tag");
  for (int i = 0; i < structures->size(); i++) {
    structure = structures->at(i);
    QReadLocker structureLocker(&structure->lock());
    if (!std::isnan(structure->getDistAboveHull())) {
      for (const auto& sym : eleSymb) {
        out << QString(" %1")
                   .arg(structure->getNumberOfAtomsOfSymbol(sym), 7);
      }
      out << QString(" %1").arg(structure->getEnthalpy(), 14, 'f', 6);
      out << QString("  # %1").arg(structure->getDistAboveHull(), 14, 'f', 6);
      out << QString(" %1").arg(structure->getParetoFront(), 7);
      out << QString(" %1").arg(structure->getIndex(), 7);
      out << QString("  %1\n").arg(structure->getTag());
    }
    structureLocker.unlock();
  }
  // Write the reference structure/energies
  std::vector<double> refData = getReferenceEnergiesVector();
  int refNumb = refData.size() / (eleSymb.size() + 1);
  for(int i = 0; i < refNumb; i++) {
    for (int j = 0; j < eleSymb.size(); j++)
      out << QString(" %1").arg(refData[i * (eleSymb.size() + 1) + j], 7);
    out << QString(" %1").arg(refData[i * (eleSymb.size() + 1) + eleSymb.size()], 14, 'f', 6);
    out << QString("  # %1 %2 %3  %4\n").arg("ref", 14).arg("ref", 7).arg("ref", 7).arg("ref");
  }

  return true;
}

int SearchBase::selectParentFromPool(const QList<Structure*>& structures, size_t poolSize)
{
  // This function selects a parent from the input structures list, and returns
  //   its order index in the input list. If it fails, the function returns -1.
  //
  // Here, we create a 2D vector "objData" for optimization that includes
  //   only the "optimizable" objectives: energy, then min/max objectives.
  // In preparing this matrix, we will:
  //   (1) convert all "max" objectives to "min" (by multiplying values in -1)
  //       such that we always have a minimization problem for all objectives,
  //   (2) scale objective values to [0,1] range (so, zero is the best candidate),
  //   (3) set all values to zero for objectives with zero spread. So, they won't
  //       affect scalar fitness. This will have no effect in the outcome of
  //       Pareto probabilities.
  // After preparing the objective data matrix, we will calculate scalar probs
  //   from both basic generalized fitness function and the Pareto-based scalar
  //   fitness values. However, at the end, the parent selection will be done
  //   according to the user's choice through:
  //   - Basic  optimization (i.e., scalar generalized fitness function)
  //   - Pareto optimization (tournament selection with/without restricted pool)
  //   - Pareto optimization (ranks/distances converted to scalar fitness)
  //
  // For Pareto optimization, we might ignore the crowding distances if the user
  //   chooses so. Also, generally, we will apply the user-specified precision
  //   on the values of objectives.

  // Before getting here; we have checked that there are enough optimized
  //   structure; and distance above hull (and objectives, if any) are already
  //   calculated for all structures in the input set.

  if (structures.isEmpty() || poolSize == 0)
    return -1;

  if (structures.size() == 1) {
    return 0;
  }

  //============================ INITIATE SOME BASIC VARIABLES
  // Objective and weight related variables
  int strNumb = structures.size(); // total number of structures
  std::vector<double> objWght;     // weight for optimizable objectives
  int objNumb = 0;                 // number of objectives (energy is always included)

  // Initiate objective weights
  //   (at this point, except energy, all objective have their weights
  //    properly determined. Energy's weight is '1 - sum of other weights')
  //
  // Start by assigning a temporary weight of "1.0" to energy objective. Then,
  //   process other objectives (if any); and finally adjust the energy weight.
  objNumb += 1;
  objWght.push_back(1.0);
  double tot_weight = 0.0;
  for (int i = 0; i < getObjectivesNum(); i++) {
    if(getObjectivesTyp(i) == SearchBase::Ot_Min || getObjectivesTyp(i) == SearchBase::Ot_Max) {
      objNumb += 1;
      objWght.push_back(getObjectivesWgt(i));
      tot_weight += getObjectivesWgt(i);
    }
  }
  // Now, adjust the energy weight
  objWght[0] -= tot_weight;

  // Determine the optimization type
  bool usePareto = (m_optimizationType == "pareto") ? true : false;

  // Some basic variables
  std::vector<std::vector<double>> objData; // final optimization 2D input matrix
  std::vector<double> strProb; // final raw probability of structures
  QList<QString> strTags; // structures tags

  Structure* s; // A working variable

  //============================ CREATE DATA STRUCTURE FOR OPTIMIZATION
  // At this point, input structures have their hull calculated.
  for (int i = 0; i < strNumb; i++) {
    s = structures[i];
    QReadLocker lock(&s->lock());

    strTags.push_back(s->getTag());

    std::vector<double> obj_vec;
    obj_vec.push_back(s->getDistAboveHull()); // add energy target value
    for(int j = 0; j < getObjectivesNum(); j++) {
      if (getObjectivesTyp(j) == SearchBase::Ot_Min)
        obj_vec.push_back(s->getStrucObjValues(j)); // add min objs.
      else if (getObjectivesTyp(j) == SearchBase::Ot_Max)
        obj_vec.push_back(-s->getStrucObjValues(j)); // convert and add max objs.
    }
    objData.push_back(obj_vec);
  }

  //============================ SCALE OBJECTIVE VALUES AND WEIGHTS
  // Find the lowest, highest, and spread of all objectives
  std::vector<double> objMins(objNumb,  DBL_MAX);
  std::vector<double> objMaxs(objNumb, -DBL_MAX);
  std::vector<double> objSprd(objNumb,  0.0);
  for (int i = 0; i< objNumb; i++) {
    for (int j = 0; j < strNumb; j++) {
      double objval = objData[j][i];
      if(objval < objMins[i])
        objMins[i] = objval;
      if(objval > objMaxs[i])
        objMaxs[i] = objval;
    }
    objSprd[i] = objMaxs[i] - objMins[i];
  }

  // Scale the objective values to [0,1]. Also, we effectively set the value to
  //   zero if spread is zero, so the objective won't participate in scalar
  //   fitness. This has no effect on the outcome of Pareto optimization.
  for (int i = 0; i < objNumb; i++) {
    for (int j = 0; j < strNumb; j++) {
      objData[j][i] = (objData[j][i] - objMins[i]);
      if (objSprd[i] > ZERO8)
        objData[j][i] /= objSprd[i];
    }
  }

  //============================ APPLY PRECISION
  // If the precision is "-1", this won't change the values
  for (int j = 0; j < objNumb; j++) {
    objWght[j] = roundToDecimalPlaces(objWght[j], m_objectivePrecision);
    objMins[j] = roundToDecimalPlaces(objMins[j], m_objectivePrecision);
    objMaxs[j] = roundToDecimalPlaces(objMaxs[j], m_objectivePrecision);
    for (int i = 0; i < strNumb; i++)
      objData[i][j] = roundToDecimalPlaces(objData[i][j], m_objectivePrecision);
  }

  //============================ CALCULATE SCALAR PROBABILITIES
  // An introductory step: for "Basic" optimization, we will need the scalar
  //   probabilities. But for Pareto optimization with tournament selection, we don't!
  //   However, we still need ranks (and -possibly- distances) which are calculated in
  //   "paretoProbs". Plus, we might need to restrict the pool in this case for which we
  //   use the Pareto-based scalar fitness values to filter the pool.
  // So, regardless of the exact optimization type, we always take this step and
  //   call the relevant "...Probs" function. Later, we do the parent selection
  //   precisely as the user has instructed and using the data we collect here.
  // From the output values produced by paretoProbs, we might actually need the strFrnt
  //   strDist; the other two (rawprob and rawdist) are for debug output, though.
  std::vector<int>    strFrnt(strNumb, -1);
  std::vector<double> strDist(strNumb, -1.0);
  std::vector<double> rawprob(strNumb, -1.0);
  std::vector<double> rawdist(strNumb, -1.0);

  if (usePareto) {
    strProb = paretoProbs(objData, m_crowdingDistance, strFrnt, strDist,
                          rawprob, rawdist);
  } else {
    strProb = scalarProbs(objData, objWght);
  }

  // extra "debug-like" output (if verbose output is set)
  if (m_verbose) {
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

    QString enet = "Hull";
    debOuts += QString("   Objs [%1] - Min   Max  Wgt\n").arg(enet,4);
    for (int i = 0; i < objNumb; i++)
      debOuts += QString("  %1   %2   %3\n").arg(objMins[i],16,'f',10)
        .arg(objMaxs[i],16,'f',10)
        .arg(objWght[i],16,'f',10);

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
    qDebug().noquote() << debOuts;
  }

  // Sanity check
  if (strProb.size() != strNumb) {
    qDebug() << "Error: Failed to calculate fitness values!";
    return -1;
  }

  // Construct <index in structures list, prob> variable for further processing
  //   the probabilities and converting them to normalized fitness
  //   values for "poolSize" number of structures.
  QList<QPair<int, double>> probs;

  for (int i = 0; i < strNumb; i++) {
    s = structures[i];
    // Sanity check: structures should have the same order as input
    if (strTags[i] == s->getTag()) {
      QReadLocker lock(&s->lock());
      probs.append(QPair<int, double>(i, strProb[i]));
    } else {
      qDebug() << "Error: structure " << strTags[i]
               << " does not exist in prob entry list!";
      return -1;
    }
  }

  //============================ FIND BEST "POOLSIZE" STRUCTURES
  // Another introductory step! Finding top candidates is always needed for basic
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
    if (!std::isnan(prob.second))
      allNan = false;
    if (prob.second > ZERO8)
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
  while (probs.size() > poolSize)
    probs.pop_front();

#ifdef SEARCHBASE_PROBS_DEBUG
  QString outs1 = QString("\n   Unnormalized (but sorted and trimmed) probs list is:\n"
                         "    structure :  enthalpy  : probs\n");
  for (const auto& elem: probs) {
    QReadLocker lock(&structures[elem.first]->lock());
    outs1 += QString("      %1 : %3 : %4\n").arg(structures[elem.first]->getTag(),7)
      .arg(structures[elem.first]->getEnthalpyPerAtom(),0,'f',6).arg(elem.second,0,'f',6);
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

#ifdef SEARCHBASE_PROBS_DEBUG
  outs1 = QString("   Normalized, sorted, and trimmed probs list is:\n"
                  "    structure :  enthalpy  : probs\n");
  for (const auto& elem: probs) {
    QReadLocker lock(&structures[elem.first]->lock());
    outs1 += QString("      %1 : %3 : %4\n").arg(structures[elem.first]->getTag(),7)
      .arg(structures[elem.first]->getEnthalpyPerAtom(),0,'f',6).arg(elem.second,0,'f',6);
  }
  qDebug().noquote() << outs1;
#endif

  // Now replace each entry with a cumulative total
  sum = 0.0;
  for (auto& elem: probs) {
    sum += elem.second;
    elem.second = sum;
  }

#ifdef SEARCHBASE_PROBS_DEBUG
  outs1 = QString("   Cumulative (final) probs list is:\n"
                  "    structure :  enthalpy  : probs\n");
  for (const auto& elem: probs) {
    QReadLocker lock(&structures[elem.first]->lock());
    outs1 += QString("      %1 : %3 : %4\n").arg(structures[elem.first]->getTag(),7)
      .arg(structures[elem.first]->getEnthalpyPerAtom(),0,'f',6).arg(elem.second,0,'f',6);
  }
  qDebug().noquote() << outs1;
#endif

  //============================ PARENT SELECTION 1: PARETO (TOURNAMENT SELECTION)
  // If performing Pareto optimization with tournament selection;
  //   we will simply make a binary selection and return the structure.
  // For restricted pool, we choose the pair from the best subset;
  //   otherwise from the entire pool of structures.
  if (usePareto && m_tournamentSelection) {
    int str_a, str_b;
    int total = 0;

    if (m_restrictedPool) {
      // Make selection from the top pool
      total = probs.size();
      str_a = probs[getRandUInt(0, total-1)].first;
      do {str_b = probs[getRandUInt(0, total-1)].first;}
      while (str_b == str_a);
    } else {
      // Make selection from the entire pool
      total = strNumb;
      str_a = getRandUInt(0, total-1);
      do {str_b = getRandUInt(0, total-1);}
      while (str_b == str_a);
    }

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
      parent = (getRandDouble() < 0.5) ? str_a : str_b;

    if (m_verbose) {
      QString outs = QString("   Selected (tournament) %1 from structures with rank-dist (%2)")
                     .arg(structures[parent]->getTag(),7).arg(total);
      outs += QString("\n   %1   %2   %3").arg(structures[str_a]->getTag(),7)
                  .arg(strFrnt[str_a], 4).arg(strDist[str_a],10,'f',6);
      outs += QString("\n   %1   %2   %3").arg(structures[str_b]->getTag(),7)
                  .arg(strFrnt[str_b], 4).arg(strDist[str_b],10,'f',6);
      outs += QString("\n");
      qDebug().noquote() << outs;
    }

    return parent;
  }

  //============================ PARENT SELECTION 2: BASIC AND PARETO (SCALAR FITNESS-BASED)
  // Pick a parent using scalar fitness values from the "top" structures
  //   for either basic/Pareto optimization.
  // We use a random threshold and select the structure
  //   with fitness above the chosen random threshold
  int parent = probs.size() - 1;
  double r = getRandDouble();
  for (const auto& elem : probs) {
    if (r < elem.second) {
      parent = elem.first;
      break;
    }
  }
  if (m_verbose) {
    QString outs =
      QString("   Selected (fitness) %1 ( r = %2 ) from structures with probs (%3)")
      .arg(structures[parent]->getTag(),7).arg(r,8,'f',6).arg(probs.size());
    outs += QString("\n      structure : enthalpy (e/Atom):    probs   : cumulative probs\n");
    double previousProbs = 0.0;
    for (const auto& elem: probs) {
      outs += QString("        %1 :     %2 : %3 : %4\n")
              .arg(structures[elem.first]->getTag(),7)
              .arg(structures[elem.first]->getEnthalpyPerAtom(),12,'f',6)
              .arg(elem.second - previousProbs,10,'f',6).arg(elem.second,10,'f',6);
      previousProbs = elem.second;
    }
    qDebug().noquote() << outs;
  }

  return parent;
}

QList<QPair<Structure*, double>>
SearchBase::getProbabilityList(const QList<Structure*>& structures, size_t poolSize)
{
  // "Legacy" get probability function: return basic scalar fitness values for structures.
  //
  // Starting from XtalOpt 14, we don't use this anymore. However, we keep it
  //   for benchmarking, and also to use in the test suit.

  QList<QPair<Structure*, double>> probs;

  if (structures.isEmpty() || poolSize == 0)
    return probs;

  if (structures.size() == 1) {
    probs.append(QPair<Structure*, double>(structures[0], 1.0));
    return probs;
  }

  //============================ INITIATE SOME BASIC VARIABLES
  // Objective and weight related variables
  int strNumb = structures.size(); // total number of structures
  std::vector<double> objWght;     // weight for optimizable objectives
  int objNumb = 0;                 // number of objectives (energy is always included)

  // Initiate objective weights
  //   (at this point, except energy, all objective have their weights
  //    properly determined. Energy's weight is '1 - sum of other weights')
  //
  // Start by assigning a temporary weight of "1.0" to energy objective. Then,
  //   process other objectives (if any); and finally adjust the energy weight.
  objNumb += 1;
  objWght.push_back(1.0);
  double tot_weight = 0.0;
  for (int i = 0; i < getObjectivesNum(); i++) {
    if(getObjectivesTyp(i) == SearchBase::Ot_Min || getObjectivesTyp(i) == SearchBase::Ot_Max) {
      objNumb += 1;
      objWght.push_back(getObjectivesWgt(i));
      tot_weight += getObjectivesWgt(i);
    }
  }
  // Now, adjust the energy weight
  objWght[0] -= tot_weight;

  // Some basic variables
  std::vector<std::vector<double>> objData; // final optimization 2D input matrix
  std::vector<double> strProb; // final raw probability of structures
  QList<QString> strTags; // structures tags

  Structure* s; // A working variable

  //============================ CREATE DATA STRUCTURE FOR OPTIMIZATION
  // At this point, input structures have their hull calculated.
  for (int i = 0; i < strNumb; i++) {
    s = structures[i];
    QReadLocker lock(&s->lock());

    strTags.push_back(s->getTag());

    std::vector<double> obj_vec;
    obj_vec.push_back(s->getDistAboveHull()); // add energy target value
    for(int j = 0; j < getObjectivesNum(); j++) {
      if (getObjectivesTyp(j) == SearchBase::Ot_Min)
        obj_vec.push_back(s->getStrucObjValues(j)); // add min objs.
      else if (getObjectivesTyp(j) == SearchBase::Ot_Max)
        obj_vec.push_back(-s->getStrucObjValues(j)); // convert and add max objs.
    }
    objData.push_back(obj_vec);
  }

  //============================ SCALE OBJECTIVE VALUES AND WEIGHTS
  // Find the lowest, highest, and spread of all objectives
  std::vector<double> objMins(objNumb,  DBL_MAX);
  std::vector<double> objMaxs(objNumb, -DBL_MAX);
  std::vector<double> objSprd(objNumb,  0.0);
  for (int i = 0; i< objNumb; i++) {
    for (int j = 0; j < strNumb; j++) {
      double objval = objData[j][i];
      if(objval < objMins[i])
        objMins[i] = objval;
      if(objval > objMaxs[i])
        objMaxs[i] = objval;
    }
    objSprd[i] = objMaxs[i] - objMins[i];
  }

  // Scale the objective values to [0,1]. Also, we effectively set the value to
  //   zero if spread is zero, so the objective won't participate in scalar
  //   fitness. This has no effect on the outcome of Pareto fitness.
  for (int i = 0; i < objNumb; i++) {
    for (int j = 0; j < strNumb; j++) {
      objData[j][i] = (objData[j][i] - objMins[i]);
      if (objSprd[i] > ZERO8)
        objData[j][i] /= objSprd[i];
    }
  }

  //============================ Apply precision
  if (m_objectivePrecision > 0) {
    for (int j = 0; j < objNumb; j++) {
      objWght[j] = roundToDecimalPlaces(objWght[j], m_objectivePrecision);
      objMins[j] = roundToDecimalPlaces(objMins[j], m_objectivePrecision);
      objMaxs[j] = roundToDecimalPlaces(objMaxs[j], m_objectivePrecision);
      for (int i = 0; i < strNumb; i++)
        objData[i][j] = roundToDecimalPlaces(objData[i][j], m_objectivePrecision);
    }
  }

  //============================ CALCULATE BASIC SCALAR PROBABILITIES
  strProb = scalarProbs(objData, objWght);

  //============================ CONSTRUCT THE (STRUC, PROB) STRUCTURE
  // Sanity check
  if (strProb.size() != strNumb) {
    qDebug() << "Error: Failed to calculate fitness values!";
    return probs;
  }

  for (int i = 0; i < strNumb; i++) {
    s = structures[i];
    // Sanity check: structures should have the same order as input
    if (strTags[i] == s->getTag()) {
      QReadLocker lock(&s->lock());
      probs.append(QPair<Structure*, double>(s, strProb[i]));
    } else {
      qDebug() << "Error: structure " << strTags[i]
               << " does not exist in prob entry list!";
      return probs;
    }
  }

  // =======================================================================
  // Contribution of any objective with zero spread is already set to zero.
  // So, we won't have all probs "nan" anymore. Still, they might be all "zero"!
  // Just to be sure, here we check if they are all zero (or nan). If so, we set them
  //   all to a fixed value and proceed as usual (i.e., normalizing the probs etc.)

  bool allNan = true;
  bool allZer = true;
  for (const auto& prob: probs) {
    if (!std::isnan(prob.second))
      allNan = false;
    if (prob.second > ZERO8)
      allZer = false;
  }

  if (allNan || allZer) {
    for (auto& prob: probs)
      prob.second = 1.0;
  }

  // Sort by probability
  std::sort(probs.begin(), probs.end(),
            [](const QPair<Structure*, double>& a,
               const QPair<Structure*, double>& b)
            {
              return a.second < b.second;
            });

  // Remove the lowest probability structures until we have the parent pool size
  while (probs.size() > poolSize)
    probs.pop_front();

  // Sum the resulting probs
  double sum = 0.0;
  for (const auto& elem: probs)
    sum += elem.second;

  // Normalize the list so that the sum is 1
  for (auto& elem: probs)
    elem.second /= sum;

  // Now replace each entry with a cumulative total
  sum = 0.0;
  for (auto& elem: probs) {
    sum += elem.second;
    elem.second = sum;
  }

  return probs;
}

void SearchBase::calculateObjectives(Structure* s)
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
  //   quitting the objective calculations and just print an error message;
  //   except than error in writing the output.POSCAR which will result
  //   in marking the structure as Fail, signalling the finish, and return.

  // Just start by checking if objective calculations are requested
  if (!m_calculateObjectives)
    return;

  QtConcurrent::run(this, &SearchBase::startObjectiveCalculations, s);

  return;
}

void SearchBase::startObjectiveCalculations(Structure* s)
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
    (qi->getIDString().toLower() == "local" || m_localQueue) ? locdir : s->getRempath() + "/";

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

  // Make sure the objective output files don't already exist
  for(int i = 0; i < getObjectivesNum(); i++) {
    bool exists;
    if (qi->checkIfFileExists(s, getObjectivesOut(i), &exists) && exists) {
      if(!qi->removeAFile(s, getObjectivesOut(i))) {
        error(tr("Failed to remove file %1!").arg(getObjectivesOut(i)));
      }
    }
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
  QtConcurrent::run(this, &SearchBase::finishObjectiveCalculations, s);

  return;
}

void SearchBase::finishObjectiveCalculations(Structure* s)
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
    (qi->getIDString().toLower() == "local" || m_localQueue) ? locdir : s->getRempath() + "/";

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
    else if (getObjectivesTyp(i) == SearchBase::Ot_Fil && flagv == 0.0)
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

  // Print out "minimization" objectives to run output
  if (s->getStrucObjState() == Structure::Os_Retain) {
    QString outs = "";
    outs += QString(" %1 ").arg(s->getEnthalpyPerAtom(),16,'f',8);
    for (int i = 0; i < tmp_values.size(); i++) {
      if (getObjectivesTyp(i) == SearchBase::Ot_Min)
        outs += QString(" %1 ").arg(tmp_values[i],16,'f',8);
      else if (getObjectivesTyp(i) == SearchBase::Ot_Max)
        outs += QString(" %1 ").arg(-tmp_values[i],16,'f',8);
    }
    outs += QString(" %1   MIN_OBJECTIVES").arg(s->getTag(),8);
    qDebug().noquote() << outs << "\n";
  }

  // We are done here.
  emit doneWithObjectives(s);

  return;
}

bool SearchBase::save(QString stateFilename, bool notify)
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
        error("SearchBase::save(): Error opening file " + structureStateFileName +
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
  // Print hull file     //
  /////////////////////////
  if (!locWorkDir.isEmpty()) {
    QList<QString> eleSymb = getChemicalSystem(); // "full" list of symbol of elements
    QFile file(locWorkDir + "/hull.txt");
    QFile oldfile(locWorkDir + "/hull.txt.old");
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
      error("SearchBase::save(): Error opening file " + file.fileName() +
            " for writing...");
      return false;
    }
    QTextStream out(&file);
    // Write structure data for those with calculated hull values
    for (int i = 0; i < eleSymb.size(); i++)
      out << QString(" %1").arg(eleSymb[i], 7);
    out << QString(" %1  # %2 %3 %4  %5\n").arg("Enthalpy", 14).arg("AboveHullAtm", 14)
               .arg("Pareto",7).arg("Index", 7).arg("Tag");
    for (int i = 0; i < structures->size(); i++) {
      structure = structures->at(i);
      QReadLocker structureLocker(&structure->lock());
      if (!std::isnan(structure->getDistAboveHull())) {
        for (const auto& sym : eleSymb) {
          out << QString(" %1")
                 .arg(structure->getNumberOfAtomsOfSymbol(sym), 7);
        }
        out << QString(" %1").arg(structure->getEnthalpy(), 14, 'f', 6);
        out << QString("  # %1").arg(structure->getDistAboveHull(), 14, 'f', 6);
        out << QString(" %1").arg(structure->getParetoFront(), 7);
        out << QString(" %1").arg(structure->getIndex(), 7);
        out << QString("  %1\n").arg(structure->getTag());
      }
      structureLocker.unlock();
    }
    // Write the reference structure/energies
    std::vector<double> refData = getReferenceEnergiesVector();
    int refNumb = refData.size() / (eleSymb.size() + 1);
    for(int i = 0; i < refNumb; i++) {
      for (int j = 0; j < eleSymb.size(); j++)
        out << QString(" %1").arg(refData[i * (eleSymb.size() + 1) + j], 7);
      out << QString(" %1").arg(refData[i * (eleSymb.size() + 1) + eleSymb.size()], 14, 'f', 6);
      out << QString("  # %1 %2 %3  %4\n").arg("ref", 14).arg("ref", 7).arg("ref", 7).arg("ref");
    }
  }

  /////////////////////////
  // Print results files //
  /////////////////////////

  // Only print the results file if we have a file path
  if (!locWorkDir.isEmpty()) {
    QFile file(locWorkDir + "/results.txt");
    QFile oldfile(locWorkDir + "/results.txt.old");
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
      error("SearchBase::save(): Error opening file " + file.fileName() +
            " for writing...");
      return false;
    }
    QTextStream out(&file);

    QList<Structure*> sortedStructures;

    for (int i = 0; i < structures->size(); i++)
      sortedStructures.append(structures->at(i));
    if (sortedStructures.size() != 0) {
      Structure::sortAndRankStructures(&sortedStructures);
      out << sortedStructures.first()->getResultsHeader(getObjectivesNum())
          << endl;
    }

    for (int i = 0; i < sortedStructures.size(); i++) {
      structure = sortedStructures.at(i);
      if (!structure)
        continue; // In case there was a problem copying.
      QReadLocker structureLocker(&structure->lock());
      out << structure->getResultsEntry(getObjectivesNum(),
                                        structure->getCurrentOptStep(),
                                        getChemicalSystem()) << endl;
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

QString SearchBase::interpretTemplate(const QString& str, Structure* structure)
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

void SearchBase::interpretKeyword_base(QString& line, Structure* structure)
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
      rep += (QString(ElementInfo::getAtomicSymbol((*it).atomicNumber()).c_str()) +
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
      rep += (QString(ElementInfo::getAtomicSymbol((*it).atomicNumber()).c_str()) +
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
      rep += (QString(ElementInfo::getAtomicSymbol((*it).atomicNumber()).c_str()) +
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
      rep += (QString(ElementInfo::getAtomicSymbol((*it).atomicNumber()).c_str()) +
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

QString SearchBase::getTemplateKeywordHelp_base()
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

std::unique_ptr<QueueInterface> SearchBase::createQueueInterface(
  const std::string& queueName)
{
  qDebug() << "Error:" << __FUNCTION__ << "not implemented. It needs to"
           << "be overridden in a derived class.";
  return nullptr;
}

std::unique_ptr<Optimizer> SearchBase::createOptimizer(const std::string& optName)
{
  qDebug() << "Error:" << __FUNCTION__ << "not implemented. It needs to"
           << "be overridden in a derived class.";
  return nullptr;
}

QueueInterface* SearchBase::queueInterface(int optStep) const
{
  if (optStep >= getNumOptSteps()) {
    qDebug() << "Error in" << __FUNCTION__ << ": optStep," << optStep
             << ", is out of bounds! The number of optimization steps is:"
             << getNumOptSteps();
    return nullptr;
  }
  return m_queueInterfaceAtOptStep[optStep].get();
}

int SearchBase::queueInterfaceIndex(const QueueInterface* qi) const
{
  for (size_t i = 0; i < m_queueInterfaceAtOptStep.size(); ++i) {
    if (qi == m_queueInterfaceAtOptStep[i].get())
      return i;
  }
  return -1;
}

Optimizer* SearchBase::optimizer(int optStep) const
{
  if (optStep >= getNumOptSteps()) {
    qDebug() << "Error in" << __FUNCTION__ << ": optStep," << optStep
             << ", is out of bounds! The number of optimization steps is:"
             << getNumOptSteps();
    return nullptr;
  }
  return m_optimizerAtOptStep[optStep].get();
}

int SearchBase::optimizerIndex(const Optimizer* optimizer) const
{
  for (size_t i = 0; i < m_optimizerAtOptStep.size(); ++i) {
    if (optimizer == m_optimizerAtOptStep[i].get())
      return i;
  }
  return -1;
}

void SearchBase::clearOptSteps()
{
  m_queueInterfaceAtOptStep.clear();
  m_optimizerAtOptStep.clear();
  m_queueInterfaceTemplates.clear();
  m_optimizerTemplates.clear();
  m_numOptSteps = 0;
}

void SearchBase::appendOptStep()
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

void SearchBase::insertOptStep(size_t optStep)
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

void SearchBase::removeOptStep(size_t optStep)
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

void SearchBase::setQueueInterface(size_t optStep, const std::string& qiName)
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

std::string SearchBase::getQueueInterfaceTemplate(size_t optStep,
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

void SearchBase::setQueueInterfaceTemplate(size_t optStep, const std::string& name,
                                        const std::string& temp)
{
  if (optStep >= m_numOptSteps) {
    qDebug() << "Error in" << __FUNCTION__ << ": optStep," << optStep
             << ", is out of bounds. Number of opt steps is" << m_numOptSteps;
    return;
  }
  m_queueInterfaceTemplates[optStep][name] = temp;
}

void SearchBase::setOptimizer(size_t optStep, const std::string& optName)
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

std::string SearchBase::getOptimizerTemplate(size_t optStep,
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

void SearchBase::setOptimizerTemplate(size_t optStep, const std::string& name,
                                   const std::string& temp)
{
  if (optStep >= m_numOptSteps) {
    qDebug() << "Error in" << __FUNCTION__ << ": optStep," << optStep
             << ", is out of bounds. Number of opt steps is" << m_numOptSteps;
    return;
  }
  m_optimizerTemplates[optStep][name] = temp;
}

SearchBase::TemplateType SearchBase::getTemplateType(size_t optStep,
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

std::string SearchBase::getTemplate(size_t optStep, const std::string& name) const
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

void SearchBase::setTemplate(size_t optStep, const std::string& name,
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

void SearchBase::readQueueInterfaceTemplatesFromSettings(
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

void SearchBase::readOptimizerTemplatesFromSettings(
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

void SearchBase::readTemplatesFromSettings(size_t optStep,
                                        const std::string& filename)
{
  readQueueInterfaceTemplatesFromSettings(optStep, filename);
  readOptimizerTemplatesFromSettings(optStep, filename);
}

void SearchBase::readAllTemplatesFromSettings(const std::string& filename)
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

void SearchBase::writeQueueInterfaceTemplatesToSettings(
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

void SearchBase::writeOptimizerTemplatesToSettings(
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
                     optim->getLocalRunCommand());
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

void SearchBase::writeTemplatesToSettings(size_t optStep,
                                       const std::string& filename)
{
  writeQueueInterfaceTemplatesToSettings(optStep, filename);
  writeOptimizerTemplatesToSettings(optStep, filename);
}

void SearchBase::writeAllTemplatesToSettings(const std::string& filename)
{
  SETTINGS(filename.c_str());
  settings->beginGroup(getIDString().toLower() + "/edit");
  settings->setValue("numOptSteps", QString::number(getNumOptSteps()));
  for (size_t i = 0; i < getNumOptSteps(); ++i)
    writeTemplatesToSettings(i, filename);
  settings->endGroup();
}

void SearchBase::readUserValuesFromSettings(const std::string& filename)
{
  SETTINGS(filename.c_str());

  settings->beginGroup(getIDString().toLower() + "/edit");
  m_user1 = settings->value("/user1", "").toString().toStdString();
  m_user2 = settings->value("/user2", "").toString().toStdString();
  m_user3 = settings->value("/user3", "").toString().toStdString();
  m_user4 = settings->value("/user4", "").toString().toStdString();
  settings->endGroup();
}

void SearchBase::writeUserValuesToSettings(const std::string& filename)
{
  SETTINGS(filename.c_str());

  settings->beginGroup(getIDString().toLower() + "/edit");

  settings->setValue("/user1", m_user1.c_str());
  settings->setValue("/user2", m_user2.c_str());
  settings->setValue("/user3", m_user3.c_str());
  settings->setValue("/user4", m_user4.c_str());

  settings->endGroup();
}

bool SearchBase::isReadyToSearch(QString& err) const
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

bool SearchBase::anyRemoteQueueInterfaces() const
{
  for (size_t i = 0; i < getNumOptSteps(); ++i) {
    if (queueInterface(i)->getIDString().toLower() != "local")
      return true;
  }
  return false;
}

void SearchBase::promptForPassword(const QString& message, QString* newPassword,
                                bool* ok)
{
  if (m_usingGUI) {
    (*newPassword) = QInputDialog::getText(dialog(), "Need password:", message,
                                           QLineEdit::Password, QString(), ok);
  } else {
    (*newPassword) = PasswordPrompt::getPassword().c_str();
  }
}

void SearchBase::promptForBoolean(const QString& message, bool* ok)
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

void SearchBase::setClipboard(const QString& text) const
{
  emit sig_setClipboard(text);
}

// No need to document this
/// @cond
void SearchBase::setClipboard_(const QString& text) const
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

bool SearchBase::createSSHConnections_libssh()
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

bool SearchBase::createSSHConnections_cli()
{
  // Since we rely on public key auth here, it's much easier to set up:
  SSHManagerCLI* cliSSHManager = new SSHManagerCLI(5, this);
  cliSSHManager->makeConnections(host, username, "", port);
  m_ssh = cliSSHManager;
  return true;
}

#endif // not USE_CLI_SSH
#endif // ENABLE_SSH

void SearchBase::warning(const QString& s)
{
  qWarning() << "Warning: " << s;
  emit warningStatement(s);
}

void SearchBase::debug(const QString& s)
{
  qDebug() << "Debug: " << s;
  emit debugStatement(s);
}

void SearchBase::error(const QString& s)
{
  qWarning() << "Error: " << s;
  emit errorStatement(s);
}

void SearchBase::message(const QString& s)
{
  qDebug() << "Message: " << s;
  emit messageStatement(s);
}

} // end namespace GlobalSearch

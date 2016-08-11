/**********************************************************************
  XtalOpt - "Engine" for the optimization process

  Copyright (C) 2009-2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <xtalopt/xtalopt.h>

#include <xtalopt/structures/xtal.h>
#include <xtalopt/optimizers/vasp.h>
#include <xtalopt/optimizers/gulp.h>
#include <xtalopt/optimizers/pwscf.h>
#include <xtalopt/optimizers/castep.h>
#include <xtalopt/optimizers/siesta.h>
#include <xtalopt/ui/dialog.h>
#include <xtalopt/genetic.h>

#include <globalsearch/optbase.h>
#include <globalsearch/optimizer.h>
#include <globalsearch/queueinterface.h>
#include <globalsearch/queueinterfaces/local.h>
#include <globalsearch/queuemanager.h>
#include <globalsearch/slottedwaitcondition.h>
#include <globalsearch/macros.h>
#include <globalsearch/bt.h>
#include <globalsearch/fileutils.h>

#ifdef ENABLE_SSH
#include <globalsearch/sshmanager.h>
#include <globalsearch/queueinterfaces/remote.h>
#endif // ENABLE_SSH

#include <openbabel/generic.h>

#include <Eigen/LU>

#include <QtCore/QDir>
#include <QtCore/QList>
#include <QtCore/QFile>
#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtCore/QFileInfo>
#include <QtCore/QReadWriteLock>
#include <QtCore/QtConcurrentRun>
#include <QtCore/QtConcurrentMap>

#define ANGSTROM_TO_BOHR 1.889725989

using namespace GlobalSearch;
using namespace OpenBabel;
using namespace Avogadro;

namespace XtalOpt {

  XtalOpt::XtalOpt(XtalOptDialog *parent) :
    OptBase(parent),
    m_initWC(new SlottedWaitCondition (this))
  {
    xtalInitMutex = new QMutex;
    m_idString = "XtalOpt";
    m_schemaVersion = 2;

    // Connections
    connect(m_tracker, SIGNAL(newStructureAdded(GlobalSearch::Structure*)),
            this, SLOT(checkForDuplicates()));
    connect(this, SIGNAL(sessionStarted()),
            this, SLOT(resetDuplicates()));
  }

  XtalOpt::~XtalOpt()
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
    unsigned int timeout = 30;
    while (timeout > 0 && savePending) {
      qDebug() << "Spinning on save before destroying XtalOpt ("
               << timeout << "seconds until timeout).";
      timeout--;
      GS_SLEEP(1);
    };

    savePending = true;

    // Clean up various members
    m_initWC->deleteLater();
    m_initWC = 0;
  }

  void XtalOpt::startSearch()
  {

    // Settings checks
    // Check lattice parameters, volume, etc
    if (!XtalOpt::checkLimits()) {
      error("Cannot create structures. Check log for details.");
      return;
    }

    // Do we have a composition?
    if (comp.isEmpty()) {
      error("Cannot create structures. Composition is not set.");
      return;
    }

    // Check if xtalopt data is already saved at the filePath
    if (QFile::exists(filePath + "/xtalopt.state")) {
      bool proceed;
      needBoolean(tr("Warning: XtalOpt data is already saved at: %1"
                     "\nDo you wish to proceed and overwrite it?"
                     "\n\nIf no, please change the local working directory "
                     "under Queue configure located in the "
                     "'Optimization Settings' tab")
                  .arg(filePath),
                  &proceed);
      if (!proceed) return;
      else {
        bool result = FileUtils::removeDir(filePath);
        if (!result) {
          error(tr("Error removing directory at:\n %1").arg(filePath));
        }
      }
    }

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
               "can begin (The option is on the 'Search Settings' tab)."));
    };

    // Warn user if contStructs is 0
    if (contStructs == 0) {
      error(tr("Warning: the number of continuous structures is "
               "currently set to 0."
               "\n\nYou will need to increase this value before the search "
               "can move past the first generation (The option is on the "
               "'Search Settings' tab)."));
    };

    // VASP checks:
    if (m_optimizer->getIDString() == "VASP") {
      // Is the POTCAR generated? If not, warn user in log and launch
      // generator. Every POTCAR will be identical in this case!
      QList<uint> oldcomp, atomicNums = comp.keys();
      QList<QVariant> oldcomp_ = m_optimizer->getData("Composition").toList();
      for (int i = 0; i < oldcomp_.size(); i++)
        oldcomp.append(oldcomp_.at(i).toUInt());
      qSort(atomicNums);
      if (m_optimizer->getData("POTCAR info").toList().isEmpty() || // No info
          oldcomp != atomicNums // Composition has changed!
          ) {
        error("Using VASP and POTCAR is empty. Please select the "
              "pseudopotentials before continuing.");
        return;
      }

      // Build up the latest and greatest POTCAR compilation
      qobject_cast<VASPOptimizer*>(m_optimizer)->buildPOTCARs();
    }

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
    emit startingSession();

    // prepare pointers
    m_tracker->lockForWrite();
    m_tracker->deleteAllStructures();
    m_tracker->unlock();

    ///////////////////////////////////////////////
    // Generate random structures and load seeds //
    ///////////////////////////////////////////////

    // Set up progress bar
    m_dialog->startProgressUpdate(tr("Generating structures..."), 0, 0);

    // Initalize loop variables
    int failed = 0;
    uint progCount = 0;
    QString filename;
    Xtal *xtal = 0;
    // Use new xtal count in case "addXtal" falls behind so that we
    // don't duplicate structures when switching from seeds -> random.
    uint newXtalCount=0;

    // Load seeds...
    for (int i = 0; i < seedList.size(); i++) {
      filename = seedList.at(i);
      if (this->addSeed(filename)) {
        m_dialog->updateProgressLabel(
              tr("%1 structures generated (%2 kept, %3 rejected)...")
              .arg(i + failed).arg(i).arg(failed));
        newXtalCount++;
      }
    }

    // Generation loop...
    for (uint i = newXtalCount; i < numInitial; i++) {
      // Update progress bar
      m_dialog->updateProgressMaximum(
            (i == 0) ? 0 : int(progCount/static_cast<double>(i))*numInitial);
      m_dialog->updateProgressValue(progCount);
      progCount++;
      m_dialog->updateProgressLabel(
            tr("%1 structures generated (%2 kept, %3 rejected)...")
            .arg(i + failed).arg(i).arg(failed));

      // Generate/Check xtal
      xtal = generateRandomXtal(1, i+1);
      if (!checkXtal(xtal)) {
        delete xtal;
        i--;
        failed++;
      }
      else {
        xtal->findSpaceGroup(tol_spg);
        initializeAndAddXtal(xtal, 1, xtal->getParents());
        newXtalCount++;
      }
    }

    // Wait for all structures to appear in tracker
    m_dialog->updateProgressLabel(
          tr("Waiting for structures to initialize..."));
    m_dialog->updateProgressMinimum(0);
    m_dialog->updateProgressMinimum(newXtalCount);

    connect(m_tracker, SIGNAL(newStructureAdded(GlobalSearch::Structure*)),
            m_initWC, SLOT(wakeAllSlot()));

    m_initWC->prewaitLock();
    do {
      m_dialog->updateProgressValue(m_tracker->size());
      m_dialog->updateProgressLabel(
            tr("Waiting for structures to initialize (%1 of %2)...")
            .arg(m_tracker->size()).arg(newXtalCount));
      // Don't block here forever -- there is a race condition where
      // the final newStructureAdded signal may be emitted while the
      // WC is not waiting. Since this is just trivial GUI updating
      // and we check the condition in the do-while loop, this is
      // acceptable. The following call will timeout in 250 ms.
      m_initWC->wait(250);
    }
    while (m_tracker->size() < newXtalCount);
    m_initWC->postwaitUnlock();

    // We're done with m_initWC.
    m_initWC->disconnect();

    m_dialog->stopProgressUpdate();

    m_dialog->saveSession();
    emit sessionStarted();
  }

  bool XtalOpt::addSeed(const QString &filename)
  {
    QString err;
    Xtal *xtal = new Xtal;
    xtal->setFileName(filename);
    xtal->setStatus(Xtal::WaitingForOptimization);
    // Create atoms
    for (QHash<unsigned int, XtalCompositionStruct>::const_iterator
         it = comp.constBegin(), it_end = comp.constEnd();
         it != it_end; ++it) {
      for (int i = 0; i < it.value().quantity; ++i) {
        xtal->addAtom();
      }
    }
    xtal->moveToThread(m_queue->thread());
    if ( !m_optimizer->read(xtal, filename) || !this->checkXtal(xtal, &err)) {
      error(tr("Error loading seed %1\n\n%2").arg(filename).arg(err));
      xtal->deleteLater();
      return false;
    }
    QString parents =tr("Seeded: %1", "1 is a filename").arg(filename);
    this->m_queue->addManualStructureRequest(1);
    initializeAndAddXtal(xtal, 1, parents);
    debug(tr("XtalOpt::addSeed: Loaded seed: %1",
             "1 is a filename").arg(filename));
    return true;
  }

  Structure* XtalOpt::replaceWithRandom(Structure *s, const QString & reason)
  {
    Xtal *oldXtal = qobject_cast<Xtal*>(s);
    QWriteLocker locker1 (oldXtal->lock());

    uint generation, id;
    generation = s->getGeneration();
    id = s->getIDNumber();
    // Generate/Check new xtal
    Xtal *xtal = 0;
    while (!checkXtal(xtal)) {
      if (xtal) {
        delete xtal;
        xtal = 0;
      }

      xtal = generateRandomXtal(generation, id);
    }

    // Copy info over
    QWriteLocker locker2 (xtal->lock());
    oldXtal->clear();
    oldXtal->setOBUnitCell(new OpenBabel::OBUnitCell);
    oldXtal->setCellInfo(xtal->OBUnitCell()->GetCellMatrix());
    oldXtal->resetEnergy();
    oldXtal->resetEnthalpy();
    oldXtal->setPV(0);
    oldXtal->setCurrentOptStep(1);
    QString parents = "Randomly generated";
    if (!reason.isEmpty())
      parents += " (" + reason + ")";
    oldXtal->setParents(parents);

    Atom *atom1, *atom2;
    for (uint i = 0; i < xtal->numAtoms(); i++) {
      atom1 = oldXtal->addAtom();
      atom2 = xtal->atom(i);
      atom1->setPos(atom2->pos());
      atom1->setAtomicNumber(atom2->atomicNumber());
    }
    oldXtal->findSpaceGroup(tol_spg);
    oldXtal->resetFailCount();

    // Delete random xtal
    xtal->deleteLater();
    return qobject_cast<Structure*>(oldXtal);
  }

  Structure* XtalOpt::replaceWithOffspring(Structure *s,
                                           const QString & reason)
  {
    // Generate/Check new xtal
    Xtal *xtal = 0;
    while (!checkXtal(xtal)) {
      if (xtal) {
        xtal->deleteLater();
        xtal = NULL;
      }
      xtal = generateNewXtal();
    }

    Xtal *oldXtal = qobject_cast<Xtal*>(s);
    // Copy info over
    QWriteLocker locker1 (oldXtal->lock());
    QWriteLocker locker2 (xtal->lock());
    oldXtal->setOBUnitCell(new OpenBabel::OBUnitCell);
    oldXtal->setCellInfo(xtal->OBUnitCell()->GetCellMatrix());
    oldXtal->resetEnergy();
    oldXtal->resetEnthalpy();
    oldXtal->resetFailCount();
    oldXtal->setPV(0);
    oldXtal->setCurrentOptStep(1);
    if (!reason.isEmpty()) {
      QString parents = xtal->getParents();
      parents += " (" + reason + ")";
      oldXtal->setParents(parents);
    }

    Q_ASSERT_X(xtal->numAtoms() == oldXtal->numAtoms(), Q_FUNC_INFO,
               "Number of atoms don't match. Cannot copy.");

    for (uint i = 0; i < xtal->numAtoms(); ++i) {
      (*oldXtal->atom(i)) = (*xtal->atom(i));
    }
    oldXtal->findSpaceGroup(tol_spg);

    // Delete random xtal
    xtal->deleteLater();
    return static_cast<Structure*>(oldXtal);
  }

  Xtal* XtalOpt::generateRandomXtal(uint generation, uint id)
  {
    INIT_RANDOM_GENERATOR();
    // Set cell parameters
    double a            = RANDDOUBLE() * (a_max-a_min) + a_min;
    double b            = RANDDOUBLE() * (b_max-b_min) + b_min;
    double c            = RANDDOUBLE() * (c_max-c_min) + c_min;
    double alpha        = RANDDOUBLE() * (alpha_max - alpha_min) + alpha_min;
    double beta         = RANDDOUBLE() * (beta_max  - beta_min ) + beta_min;
    double gamma        = RANDDOUBLE() * (gamma_max - gamma_min) + gamma_min;

    // Create crystal
    Xtal *xtal	= new Xtal(a, b, c, alpha, beta, gamma);
    QWriteLocker locker (xtal->lock());

    xtal->setStatus(Xtal::Empty);

    if (using_fixed_volume)
      xtal->setVolume(vol_fixed);

    // Populate crystal
    QList<uint> atomicNums = comp.keys();
    // Sort atomic number by decreasing minimum radius. Adding the "larger"
    // atoms first encourages a more even (and ordered) distribution
    for (int i = 0; i < atomicNums.size()-1; ++i) {
      for (int j = i + 1; j < atomicNums.size(); ++j) {
        if (this->comp.value(atomicNums[i]).minRadius <
            this->comp.value(atomicNums[j]).minRadius) {
          atomicNums.swap(i,j);
        }
      }
    }

    unsigned int atomicNum;
    unsigned int q;

    qDebug() << "Xtal has divisions =" << divisions;

    if (using_mitosis){
        //  Unit Cell Vectors
        int A = ax;
        int B = bx;
        int C = cx;

        a = a / A;
        b = b / B;
        c = c / C;

        xtal->setCellInfo(a,
                b,
                c,
                xtal->getAlpha(),
                xtal->getBeta(),
                xtal->getGamma());
        qDebug() << "Xtal cell dimensions are decreasing from a =" << A*a << "b =" << B*b << "c =" << C*c <<
                "to a =" << a << "b =" << b << "c =" << c;

        for (int num_idx = 0; num_idx < atomicNums.size(); num_idx++) {
            atomicNum = atomicNums.at(num_idx);
            q = comp.value(atomicNum).quantity;
            if (using_mitosis){
                q = q / divisions;
                for (uint i = 0; i < q; i++) {
                    if (!xtal->addAtomRandomly(atomicNum, this->comp)) {
                        xtal->deleteLater();
                        debug("XtalOpt::generateRandomXtal: Failed to add atoms with "
                            "specified interatomic distance.");
                        return 0;
                    }
                }
            }
        }

        if (using_subcellPrint) printSubXtal(xtal, generation, id);

        if (!xtal->fillSuperCell(A, B, C, xtal)) {
            xtal->deleteLater();
            debug("XtalOpt::generateRandomXtal: Failed to add atoms.");
            return 0;
        }

        for (int num_idx = 0; num_idx < atomicNums.size(); num_idx++) {
            atomicNum = atomicNums.at(num_idx);
            q = comp.value(atomicNum).quantity;
            if (using_mitosis){
                q = q % divisions;
                for (uint i = 0; i < q; i++) {
                    if (!xtal->addAtomRandomly(atomicNum, this->comp)) {
                        xtal->deleteLater();
                        debug("XtalOpt::generateRandomXtal: Failed to add atoms with "
                            "specified interatomic distance.");
                        return 0;
                    }
                }
            }
        }

    } else {
        for (int num_idx = 0; num_idx < atomicNums.size(); num_idx++) {
            atomicNum = atomicNums.at(num_idx);
            q = comp.value(atomicNum).quantity;
            for (uint i = 0; i < q; i++) {
                if (!xtal->addAtomRandomly(atomicNum, this->comp)) {
                    xtal->deleteLater();
                    debug("XtalOpt::generateRandomXtal: Failed to add atoms with "
                        "specified interatomic distance.");
                    return 0;
                }
            }
        }
    }

    // Set up geneology info
    xtal->setGeneration(generation);
    xtal->setIDNumber(id);
    xtal->setParents("Randomly generated");
    if (using_mitosis) xtal->setParents(xtal->getParents()+" through mitosis");
    xtal->setStatus(Xtal::WaitingForOptimization);

    // Set up xtal data
    return xtal;
  }

  void XtalOpt::printSubXtal(Xtal *xtal, uint generation,
                                     uint id)
    {
    xtalInitMutex->lock();

    QString id_s, gen_s, locpath_s;
    id_s.sprintf("%05d",id);
    gen_s.sprintf("%05d",generation);
    locpath_s = filePath + "/subcells";
    QDir dir (locpath_s);
    if (!dir.exists()) {
      if (!dir.mkpath(locpath_s)) {
        error(tr("XtalOpt::initializeSubXtal: Cannot write to path: %1 "
                 "(path creation failure)", "1 is a file path.")
              .arg(locpath_s));
      }
    }
    QFile loc_subcell;
    loc_subcell.setFileName(locpath_s + "/" + gen_s + "x" + id_s + ".cml");

    if (!loc_subcell.open(QIODevice::WriteOnly)) {
                    error("XtalOpt::initializeSubXtal(): Error opening file "+loc_subcell.fileName()+" for writing...");
    }

    QTextStream out;
    out.setDevice(&loc_subcell);

    /*
    QStringList symbols = xtal->getSymbols();
    QList<unsigned int> atomCounts = xtal->getNumberOfAtomsAlpha();
    Q_ASSERT_X(symbols.size() == atomCounts.size(), Q_FUNC_INFO,
               "xtal->getSymbols is not the same size as xtal->getNumberOfAtomsAlpha.");
    for (unsigned int i = 0; i < symbols.size(); i++) {
      out << QString("%1%2").arg(symbols[i]).arg(atomCounts[i]);
    }
    out << " ";
    out << xtal->fileName();
    out << "\n";
    // Scaling factor. Just 1.0
    out << QString::number(1.0);
    out << "\n";
    // Unit Cell Vectors
    std::vector< vector3 > vecs = xtal->OBUnitCell()->GetCellVectors();
    for (uint i = 0; i < vecs.size(); i++) {
      out << QString("  %1 %2 %3\n")
        .arg(vecs[i].x(), 12, 'f', 8)
        .arg(vecs[i].y(), 12, 'f', 8)
        .arg(vecs[i].z(), 12, 'f', 8);
    }
    // Number of each type of atom (sorted alphabetically by symbol)
    for (int i = 0; i < atomCounts.size(); i++) {
      out << QString::number(atomCounts.at(i)) + " ";
    }
    out << "\n";
    // Use fractional coordinates:
    out << "Direct\n";
    // Coordinates of each atom (sorted alphabetically by symbol)
    QList<Eigen::Vector3d> coords = xtal->getAtomCoordsFrac();
    for (int i = 0; i < coords.size(); i++) {
      out << QString("  %1 %2 %3\n")
        .arg(coords[i].x(), 12, 'f', 8)
        .arg(coords[i].y(), 12, 'f', 8)
        .arg(coords[i].z(), 12, 'f', 8);
    }
    out << endl;
*/

// Print the subcells as .cml files
    QStringList symbols = xtal->getSymbols();
    QList<unsigned int> atomCounts = xtal->getNumberOfAtomsAlpha();
    out << "<molecule>\n";
    out << "\t<crystal>\n";

    // Unit Cell Vectors
    std::vector< vector3 > vecs = xtal->OBUnitCell()->GetCellVectors();
      out << QString("\t\t<scalar title=\"a\" units=\"units:angstrom\">%1</scalar>\n")
        .arg(vecs[0].x(), 12);
      out << QString("\t\t<scalar title=\"b\" units=\"units:angstrom\">%1</scalar>\n")
        .arg(vecs[1].y(), 12, 'f', 8);
      out << QString("\t\t<scalar title=\"c\" units=\"units:angstrom\">%1</scalar>\n")
        .arg(vecs[2].z(), 12, 'f', 8);

    // Unit Cell Angles
    out << QString("\t\t<scalar title=\"alpha\" units=\"units:degree\">%1</scalar>\n")
      .arg(xtal->OBUnitCell()->GetAlpha(), 12, 'f', 8);
    out << QString("\t\t<scalar title=\"beta\" units=\"units:degree\">%1</scalar>\n")
      .arg(xtal->OBUnitCell()->GetBeta(), 12, 'f', 8);
    out << QString("\t\t<scalar title=\"gamma\" units=\"units:degree\">%1</scalar>\n")
      .arg(xtal->OBUnitCell()->GetGamma(), 12, 'f', 8);

    out << "\t</crystal>\n";
    out << "\t<atomArray>\n";

    int symbolCount = 0;
    int j = 1;
    // Coordinates of each atom (sorted alphabetically by symbol)
    QList<Eigen::Vector3d> coords = xtal->getAtomCoordsFrac();
    for (int i = 0; i < coords.size(); i++) {
      if (j > atomCounts[symbolCount]) {
          symbolCount++;
          j = 0;
      }
      j++;
      out << QString("\t\t<atom id=\"a%1\" elementType=\"%2\" xFract=\"%3\" yFract=\"%4\" zFract=\"%5\"/>\n")
        .arg(i+1)
        .arg(symbols[symbolCount])
        .arg(coords[i].x(), 12, 'f', 8)
        .arg(coords[i].y(), 12, 'f', 8)
        .arg(coords[i].z(), 12, 'f', 8);
    }

    out << "\t</atomArray>\n";
    out << "</molecule>\n";
    out << endl;

    xtalInitMutex->unlock();
  }

  void XtalOpt::initializeAndAddXtal(Xtal *xtal, uint generation,
                                     const QString &parents)
    {
    xtalInitMutex->lock();
    QList<Structure*> allStructures = m_queue->lockForNaming();
    Structure *structure;
    uint id = 1;
    for (int j = 0; j < allStructures.size(); j++) {
      structure = allStructures.at(j);
      structure->lock()->lockForRead();
      if (structure->getGeneration() == generation &&
          structure->getIDNumber() >= id) {
        id = structure->getIDNumber() + 1;
      }
      structure->lock()->unlock();
    }

    QWriteLocker xtalLocker (xtal->lock());
    xtal->moveToThread(m_queueThread);
    xtal->setIDNumber(id);
    xtal->setGeneration(generation);
    xtal->setParents(parents);
    QString id_s, gen_s, locpath_s, rempath_s;
    id_s.sprintf("%05d",xtal->getIDNumber());
    gen_s.sprintf("%05d",xtal->getGeneration());
    locpath_s = filePath + "/" + gen_s + "x" + id_s + "/";
    rempath_s = rempath + "/" + gen_s + "x" + id_s + "/";
    QDir dir (locpath_s);
    if (!dir.exists()) {
      if (!dir.mkpath(locpath_s)) {
        error(tr("XtalOpt::initializeAndAddXtal: Cannot write to path: %1 "
                 "(path creation failure)", "1 is a file path.")
              .arg(locpath_s));
      }
    }
//    xtal->moveToThread(m_tracker->thread());
    xtal->setupConnections();
    xtal->setFileName(locpath_s);
    xtal->setRempath(rempath_s);
    xtal->setCurrentOptStep(1);
    // If none of the cell parameters are fixed, perform a normalization on
    // the lattice (currently a Niggli reduction)
    if (fabs(a_min     - a_max)     > 0.01 &&
        fabs(b_min     - b_max)     > 0.01 &&
        fabs(c_min     - c_max)     > 0.01 &&
        fabs(alpha_min - alpha_max) > 0.01 &&
        fabs(beta_min  - beta_max)  > 0.01 &&
        fabs(gamma_min - gamma_max) > 0.01) {
      xtal->fixAngles();
    }
    xtal->findSpaceGroup(tol_spg);
    xtalLocker.unlock();
    xtal->update();
    m_queue->unlockForNaming(xtal);
    xtalInitMutex->unlock();
  }

  void XtalOpt::generateNewStructure()
  {
    // Generate in background thread:
    QtConcurrent::run(this, &XtalOpt::generateNewStructure_);
  }

  void XtalOpt::generateNewStructure_()
  {
    Xtal *newXtal = generateNewXtal();
    initializeAndAddXtal(newXtal, newXtal->getGeneration(),
                         newXtal->getParents());
  }

  Xtal* XtalOpt::generateNewXtal()
  {
    // Get all optimized structures
    QList<Structure*> structures = m_queue->getAllOptimizedStructures();

    // Check to see if there are enough optimized structure to perform
    // genetic operations
    if (structures.size() < 3) {
      Xtal *xtal = 0;
      while (!checkXtal(xtal)) {
        if (xtal) xtal->deleteLater();
        xtal = generateRandomXtal(1, 0);
      }
      xtal->setParents(xtal->getParents() + " (too few optimized structures "
                       "to generate offspring)");
      return xtal;
    }

    // Sort structure list
    Structure::sortByEnthalpy(&structures);

    // Trim list
    // Remove all but (popSize + 1). The "+ 1" will be removed
    // during probability generation.
    while ( static_cast<uint>(structures.size()) > popSize + 1 ) {
      structures.removeLast();
    }

    // Make list of weighted probabilities based on enthalpy values
    QList<double> probs = getProbabilityList(structures);

    // Cast Structures into Xtals
    QList<Xtal*> xtals;
#if QT_VERSION >= 0x040700
    xtals.reserve(structures.size());
#endif // QT_VERSION
    for (int i = 0; i < structures.size(); ++i) {
      xtals.append(qobject_cast<Xtal*>(structures.at(i)));
    }

    // Initialize loop vars
    double r;
    unsigned int gen;
    QString parents;
    Xtal *xtal = NULL;

    // Perform operation until xtal is valid:
    while (!checkXtal(xtal)) {
      // First delete any previous failed structure in xtal
      if (xtal) {
        xtal->deleteLater();
        xtal = 0;
      }

      // Decide operator:
      r = RANDDOUBLE();
      Operators op;
      if (r < p_cross/100.0)
        op = OP_Crossover;
      else if (r < (p_cross + p_strip)/100.0)
        op = OP_Stripple;
      else
        op = OP_Permustrain;

      // Try 1000 times to get a good structure from the selected
      // operation. If not possible, send a warning to the log and
      // start anew.
      int attemptCount = 0;
      while (attemptCount < 1000 && !checkXtal(xtal)) {
        attemptCount++;
        if (xtal) {
          delete xtal;
          xtal = 0;
        }

        // Operation specific set up:
        switch (op) {
        case OP_Crossover: {
          int ind1, ind2;
          Xtal *xtal1=0, *xtal2=0;
          // Select structures
          ind1 = ind2 = 0;
          double r1 = RANDDOUBLE();
          double r2 = RANDDOUBLE();
          for (ind1 = 0; ind1 < probs.size(); ind1++)
            if (r1 < probs.at(ind1)) break;
          for (ind2 = 0; ind2 < probs.size(); ind2++)
            if (r2 < probs.at(ind2)) break;

          xtal1 = xtals.at(ind1);
          xtal2 = xtals.at(ind2);

          // Perform operation
          double percent1;
          xtal = XtalOptGenetic::crossover(
                xtal1, xtal2, cross_minimumContribution, percent1);

          // Lock parents and get info from them
          xtal1->lock()->lockForRead();
          xtal2->lock()->lockForRead();
          uint gen1 = xtal1->getGeneration();
          uint gen2 = xtal2->getGeneration();
          uint id1 = xtal1->getIDNumber();
          uint id2 = xtal2->getIDNumber();
          xtal2->lock()->unlock();
          xtal1->lock()->unlock();

          // Determine generation number
          gen = ( gen1 >= gen2 ) ?
            gen1 + 1 :
            gen2 + 1 ;
          parents = tr("Crossover: %1x%2 (%3%) + %4x%5 (%6%)")
            .arg(gen1)
            .arg(id1)
            .arg(percent1, 0, 'f', 0)
            .arg(gen2)
            .arg(id2)
            .arg(100.0 - percent1, 0, 'f', 0);
          continue;
        }
        case OP_Stripple: {
          // Pick a parent
          int ind;
          double r = RANDDOUBLE();
          for (ind = 0; ind < probs.size(); ind++)
            if (r < probs.at(ind)) break;
          Xtal *xtal1 = xtals.at(ind);

          // Perform stripple
          double amplitude=0, stdev=0;
          xtal = XtalOptGenetic::stripple(xtal1,
                                          strip_strainStdev_min,
                                          strip_strainStdev_max,
                                          strip_amp_min,
                                          strip_amp_max,
                                          strip_per1,
                                          strip_per2,
                                          stdev,
                                          amplitude);

          // Lock parent and extract info
          xtal1->lock()->lockForRead();
          uint gen1 = xtal1->getGeneration();
          uint id1 = xtal1->getIDNumber();
          xtal1->lock()->unlock();

          // Determine generation number
          gen = gen1 + 1;
          parents = tr("Stripple: %1x%2 stdev=%3 amp=%4 waves=%5,%6")
            .arg(gen1)
            .arg(id1)
            .arg(stdev, 0, 'f', 5)
            .arg(amplitude, 0, 'f', 5)
            .arg(strip_per1)
            .arg(strip_per2);
          continue;
        }
        case OP_Permustrain: {
          int ind;
          double r = RANDDOUBLE();
          for (ind = 0; ind < probs.size(); ind++)
            if (r < probs.at(ind)) break;

          Xtal *xtal1 = xtals.at(ind);
          double stdev=0;
          xtal = XtalOptGenetic::permustrain(
                xtals.at(ind), perm_strainStdev_max, perm_ex, stdev);

          // Lock parent and extract info
          xtal1->lock()->lockForRead();
          uint gen1 = xtal1->getGeneration();
          uint id1 = xtal1->getIDNumber();
          xtal1->lock()->unlock();

          // Determine generation number
          gen = gen1 + 1;
          parents = tr("Permustrain: %1x%2 stdev=%3 exch=%4")
            .arg(gen1)
            .arg(id1)
            .arg(stdev, 0, 'f', 5)
            .arg(perm_ex);
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
        case OP_Crossover:   opStr = "crossover"; break;
        case OP_Stripple:    opStr = "stripple"; break;
        case OP_Permustrain: opStr = "permustrain"; break;
        default:             opStr = "(unknown)"; break;
        }
        warning(tr("Unable to perform operation %1 after 1000 tries. "
                   "Reselecting operator...").arg(opStr));
      }
    }
    xtal->setGeneration(gen);
    xtal->setParents(parents);
    return xtal;
  }

  bool XtalOpt::checkLimits() {
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
    if (
        ( using_fixed_volume &&
          ( (a_min * b_min * c_min) > vol_fixed ||
            (a_max * b_max * c_max) < vol_fixed )
          ) ||
        ( !using_fixed_volume &&
          ( (a_min * b_min * c_min) > vol_max ||
            (a_max * b_max * c_max) < vol_min ||
            vol_min > vol_max)
          )) {
      warning("XtalOptRand::checkLimits error: Illogical Volume limits. "
              "(Also check min/max volumes based on cell lengths)");
      return false;
    }
    return true;
  }

  bool XtalOpt::checkXtal(Xtal *xtal, QString * err) {
    if (!xtal) {
      if (err != NULL) {
        *err = "Xtal pointer is NULL.";
      }
      return false;
    }

    // Lock xtal
    QWriteLocker locker (xtal->lock());

    if (xtal->getStatus() == Xtal::Empty) {
      if (err != NULL) {
        *err = "Xtal status is empty.";
      }
      return false;
    }

    // Check composition
    QList<unsigned int> atomTypes = comp.keys();
    QList<unsigned int> atomCounts;
#if QT_VERSION >= 0x040700
    atomCounts.reserve(atomTypes.size());
#endif // QT_VERSION
    for (int i = 0; i < atomTypes.size(); ++i) {
      atomCounts.append(0);
    }
    // Count atoms of each type
    for (int i = 0; i < xtal->numAtoms(); ++i) {
      int typeIndex = atomTypes.indexOf(
            static_cast<unsigned int>(xtal->atom(i)->atomicNumber()));
      // Type not found:
      if (typeIndex == -1) {
        qDebug() << "XtalOpt::checkXtal: Composition incorrect.";
        if (err != NULL) {
          *err = "Bad composition.";
        }
        return false;
      }
      ++atomCounts[typeIndex];
    }
    // Check counts
    for (int i = 0; i < atomTypes.size(); ++i) {
      if (atomCounts[i] != comp[atomTypes[i]].quantity) {
        // Incorrect count:
        qDebug() << "XtalOpt::checkXtal: Composition incorrect.";
        if (err != NULL) {
          *err = "Bad composition.";
        }
        return false;
      }
    }

    // Check volume
    if (using_fixed_volume) {
      locker.unlock();
      xtal->setVolume(vol_fixed);
      locker.relock();
    }
    else if ( xtal->getVolume() < vol_min ||
              xtal->getVolume() > vol_max ) {
      // I don't want to initialize a random number generator here, so
      // just use the modulus of the current volume as a random float.
      double newvol = fabs(fmod(xtal->getVolume(), 1)) *
          (vol_max - vol_min) + vol_min;
      // If the user has set vol_min to 0, we can end up with a null
      // volume. Fix this here. This is just to keep things stable
      // numerically during the rescaling -- it's unlikely that other
      // cells with small, nonzero volumes will pass the other checks
      // so long as other limits are reasonable.
      if (fabs(newvol) < 1.0) {
        newvol = (vol_max - vol_min)*0.5 + vol_min;
      }
      qDebug() << "XtalOpt::checkXtal: Rescaling volume from "
               << xtal->getVolume() << " to " << newvol;
      xtal->setVolume(newvol);
    }

    // Scale to any fixed parameters
    double a, b, c, alpha, beta, gamma;
    a = b = c = alpha = beta = gamma = 0;
    if (fabs(a_min - a_max) < 0.01) a = a_min;
    if (fabs(b_min - b_max) < 0.01) b = b_min;
    if (fabs(c_min - c_max) < 0.01) c = c_min;
    if (fabs(alpha_min - alpha_max) < 0.01) alpha = alpha_min;
    if (fabs(beta_min -  beta_max)  < 0.01)  beta = beta_min;
    if (fabs(gamma_min - gamma_max) < 0.01) gamma = gamma_min;
    xtal->rescaleCell(a, b, c, alpha, beta, gamma);

    // Reject the structure if using VASP and the determinant of the
    // cell matrix is negative (otherwise VASP complains about a
    // "negative triple product")
    if (qobject_cast<VASPOptimizer*>(m_optimizer) != 0 &&
        xtal->OBUnitCell()->GetCellMatrix().determinant() <= 0.0) {
      qDebug() << "Rejecting structure" << xtal->getIDString()
               << ": using VASP negative triple product.";
      if (err != NULL) {
        *err = "Unit cell matrix cannot have a negative triple product "
            "when using VASP.";
      }
      return false;
    }

    // Before fixing angles, make sure that the current cell
    // parameters are realistic
    if (GS_IS_NAN_OR_INF(xtal->getA()) || fabs(xtal->getA()) < 1e-8 ||
        GS_IS_NAN_OR_INF(xtal->getB()) || fabs(xtal->getB()) < 1e-8 ||
        GS_IS_NAN_OR_INF(xtal->getC()) || fabs(xtal->getC()) < 1e-8 ||
        GS_IS_NAN_OR_INF(xtal->getAlpha()) || fabs(xtal->getAlpha()) < 1e-8 ||
        GS_IS_NAN_OR_INF(xtal->getBeta())  || fabs(xtal->getBeta())  < 1e-8 ||
        GS_IS_NAN_OR_INF(xtal->getGamma()) || fabs(xtal->getGamma()) < 1e-8 ) {
      qDebug() << "XtalOpt::checkXtal: A cell parameter is either 0, nan, or "
                  "inf. Discarding.";
      if (err != NULL) {
        *err = "A cell parameter is too small (<10^-8) or not a number.";
      }
      return false;
    }

    // If no cell parameters are fixed, normalize lattice
    if (fabs(a + b + c + alpha + beta + gamma) < 1e-8) {
      xtal->fixAngles();
    }

    // Check lattice
    if ( ( !a     && ( xtal->getA() < a_min         || xtal->getA() > a_max         ) ) ||
         ( !b     && ( xtal->getB() < b_min         || xtal->getB() > b_max         ) ) ||
         ( !c     && ( xtal->getC() < c_min         || xtal->getC() > c_max         ) ) ||
         ( !alpha && ( xtal->getAlpha() < alpha_min || xtal->getAlpha() > alpha_max ) ) ||
         ( !beta  && ( xtal->getBeta()  < beta_min  || xtal->getBeta()  > beta_max  ) ) ||
         ( !gamma && ( xtal->getGamma() < gamma_min || xtal->getGamma() > gamma_max ) ) )  {
      qDebug() << "Discarding structure -- Bad lattice:" <<endl
               << "A:     " << a_min << " " << xtal->getA() << " " << a_max << endl
               << "B:     " << b_min << " " << xtal->getB() << " " << b_max << endl
               << "C:     " << c_min << " " << xtal->getC() << " " << c_max << endl
               << "Alpha: " << alpha_min << " " << xtal->getAlpha() << " " << alpha_max << endl
               << "Beta:  " << beta_min  << " " << xtal->getBeta()  << " " << beta_max << endl
               << "Gamma: " << gamma_min << " " << xtal->getGamma() << " " << gamma_max;
      if (err != NULL) {
        *err = "The unit cell parameters do not fall within the specified "
            "limits.";
      }
      return false;
    }

    // Check interatomic distances
    if (using_interatomicDistanceLimit) {
      int atom1, atom2;
      double IAD;
      if (!xtal->checkInteratomicDistances(this->comp, &atom1, &atom2, &IAD)){
        Atom *a1 = xtal->atom(atom1);
        Atom *a2 = xtal->atom(atom2);
        const double minIAD =
            this->comp.value(a1->atomicNumber()).minRadius +
            this->comp.value(a2->atomicNumber()).minRadius;

        qDebug() << "Discarding structure -- Bad IAD ("
                 << IAD << " < "
                 << minIAD << ")";
        if (err != NULL) {
          *err = "Two atoms are too close together.";
        }
        return false;
      }
    }

    // Xtal is OK!
    if (err != NULL) {
      *err = "";
    }
    return true;
  }

  QString XtalOpt::interpretTemplate(const QString & templateString,
                                     Structure* structure)
  {
    QStringList list = templateString.split("%");
    QString line;
    QString origLine;
    Xtal *xtal = qobject_cast<Xtal*>(structure);
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

  void XtalOpt::interpretKeyword(QString &line, Structure* structure)
  {
    QString rep = "";
    Xtal *xtal = qobject_cast<Xtal*>(structure);

    // Xtal specific keywords
    if (line == "a")                    rep += QString::number(xtal->getA());
    else if (line == "b")               rep += QString::number(xtal->getB());
    else if (line == "c")               rep += QString::number(xtal->getC());
    else if (line == "alphaRad")        rep += QString::number(xtal->getAlpha() * DEG_TO_RAD);
    else if (line == "betaRad")         rep += QString::number(xtal->getBeta() * DEG_TO_RAD);
    else if (line == "gammaRad")        rep += QString::number(xtal->getGamma() * DEG_TO_RAD);
    else if (line == "alphaDeg")        rep += QString::number(xtal->getAlpha());
    else if (line == "betaDeg")         rep += QString::number(xtal->getBeta());
    else if (line == "gammaDeg")        rep += QString::number(xtal->getGamma());
    else if (line == "volume")          rep += QString::number(xtal->getVolume());
    else if (line == "block")           rep += QString("\%block");
    else if (line == "endblock")        rep += QString("\%endblock");
    else if (line == "coordsFrac") {
      QList<Atom*> atoms = structure->atoms();
      QList<Atom*>::const_iterator it;
      for (it  = atoms.begin();
           it != atoms.end();
           it++) {
        const Eigen::Vector3d coords = xtal->cartToFrac(*((*it)->pos()));
        rep += static_cast<QString>(OpenBabel::etab.GetSymbol((*it)->atomicNumber())) + " ";
        rep += QString::number(coords.x()) + " ";
        rep += QString::number(coords.y()) + " ";
        rep += QString::number(coords.z()) + "\n";
      }
    }
    else if (line == "chemicalSpeciesLabel") {
      QList<unsigned int> atomCounts = xtal->getNumberOfAtomsAlpha();
      QList<QString> symbol = xtal->getSymbols();
      for (int i = 0; i < atomCounts.size(); i++) {
        rep += " ";
        rep += QString::number(i+1) + " ";
        rep += QString::number(OpenBabel::etab.GetAtomicNum(symbol.at(i).toStdString().c_str())) + " ";
        rep += symbol.at(i) + "\n";
      }
    }
    else if (line == "atomicCoordsAndAtomicSpecies") {
      QList<Atom*> atoms = xtal->atoms();
      QList<Atom*>::const_iterator it;
      QList<QString> symbol = xtal->getSymbols();
      for (it  = atoms.begin();
           it != atoms.end();
           it++) {
        const Eigen::Vector3d coords = xtal->cartToFrac(*(*it)->pos());
        QString currAtom = static_cast<QString>(OpenBabel::etab.GetSymbol((*it)->atomicNumber()));
        int i = symbol.indexOf(currAtom)+1;
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
    }
    else if (line == "coordsFracId") {
      QList<Atom*> atoms = structure->atoms();
      QList<Atom*>::const_iterator it;
      for (it  = atoms.begin();
           it != atoms.end();
           it++) {
        const Eigen::Vector3d coords = xtal->cartToFrac(*(*it)->pos());
        rep += static_cast<QString>(OpenBabel::etab.GetSymbol((*it)->atomicNumber())) + " ";
        rep += QString::number((*it)->atomicNumber()) + " ";
        rep += QString::number(coords.x()) + " ";
        rep += QString::number(coords.y()) + " ";
        rep += QString::number(coords.z()) + "\n";
      }
    }
    else if (line == "gulpFracShell") {
      QList<Atom*> atoms = structure->atoms();
      QList<Atom*>::const_iterator it;
      for (it  = atoms.begin();
           it != atoms.end();
           it++) {
        const Eigen::Vector3d coords = xtal->cartToFrac(*((*it)->pos()));
        const char *symbol = OpenBabel::etab.GetSymbol((*it)->atomicNumber());
        rep += QString("%1 core %2 %3 %4\n")
            .arg(symbol).arg(coords.x()).arg(coords.y()).arg(coords.z());
        rep += QString("%1 shel %2 %3 %4\n")
            .arg(symbol).arg(coords.x()).arg(coords.y()).arg(coords.z());
        }
    }
    else if (line == "cellMatrixAngstrom") {
      matrix3x3 m = xtal->OBUnitCell()->GetCellMatrix();
      for (int i = 0; i < 3; i++) {
        rep += " ";
        for (int j = 0; j < 3; j++) {
          QString inp;
          inp.sprintf("%4.8f", m.Get(i,j));
          rep += inp + "\t";
        }
        rep += "\n";
      }
    }
    else if (line == "cellVector1Angstrom") {
      vector3 v = xtal->OBUnitCell()->GetCellVectors()[0];
      for (int i = 0; i < 3; i++) {
        rep += QString::number(v[i]) + "\t";
      }
    }
    else if (line == "cellVector2Angstrom") {
      vector3 v = xtal->OBUnitCell()->GetCellVectors()[1];
      for (int i = 0; i < 3; i++) {
        rep += QString::number(v[i]) + "\t";
      }
    }
    else if (line == "cellVector3Angstrom") {
      vector3 v = xtal->OBUnitCell()->GetCellVectors()[2];
      for (int i = 0; i < 3; i++) {
        rep += QString::number(v[i]) + "\t";
      }
    }
    else if (line == "cellMatrixBohr") {
      matrix3x3 m = xtal->OBUnitCell()->GetCellMatrix();
      for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
          rep += QString::number(m.Get(i,j) * ANGSTROM_TO_BOHR) + "\t";
        }
        rep += "\n";
      }
    }
    else if (line == "cellVector1Bohr") {
      vector3 v = xtal->OBUnitCell()->GetCellVectors()[0];
      for (int i = 0; i < 3; i++) {
        rep += QString::number(v[i] * ANGSTROM_TO_BOHR) + "\t";
      }
    }
    else if (line == "cellVector2Bohr") {
      vector3 v = xtal->OBUnitCell()->GetCellVectors()[1];
      for (int i = 0; i < 3; i++) {
        rep += QString::number(v[i] * ANGSTROM_TO_BOHR) + "\t";
      }
    }
    else if (line == "cellVector3Bohr") {
      vector3 v = xtal->OBUnitCell()->GetCellVectors()[2];
      for (int i = 0; i < 3; i++) {
        rep += QString::number(v[i] * ANGSTROM_TO_BOHR) + "\t";
      }
    }
    else if (line == "POSCAR") {
      // Comment line -- set to composition then filename
      // Construct composition
      QStringList symbols = xtal->getSymbols();
      QList<unsigned int> atomCounts = xtal->getNumberOfAtomsAlpha();
      Q_ASSERT_X(symbols.size() == atomCounts.size(), Q_FUNC_INFO,
                 "xtal->getSymbols is not the same size as xtal->getNumberOfAtomsAlpha.");
      for (unsigned int i = 0; i < symbols.size(); i++) {
        rep += QString("%1%2").arg(symbols[i]).arg(atomCounts[i]);
      }
      rep += " ";
      rep += xtal->fileName();
      rep += "\n";
      // Scaling factor. Just 1.0
      rep += QString::number(1.0);
      rep += "\n";
      // Unit Cell Vectors
      std::vector< vector3 > vecs = xtal->OBUnitCell()->GetCellVectors();
      for (uint i = 0; i < vecs.size(); i++) {
        rep += QString("  %1 %2 %3\n")
          .arg(vecs[i].x(), 12, 'f', 8)
          .arg(vecs[i].y(), 12, 'f', 8)
          .arg(vecs[i].z(), 12, 'f', 8);
      }
      // Number of each type of atom (sorted alphabetically by symbol)
      for (int i = 0; i < atomCounts.size(); i++) {
        rep += QString::number(atomCounts.at(i)) + " ";
      }
      rep += "\n";
      // Use fractional coordinates:
      rep += "Direct\n";
      // Coordinates of each atom (sorted alphabetically by symbol)
      QList<Eigen::Vector3d> coords = xtal->getAtomCoordsFrac();
      for (int i = 0; i < coords.size(); i++) {
        rep += QString("  %1 %2 %3\n")
          .arg(coords[i].x(), 12, 'f', 8)
          .arg(coords[i].y(), 12, 'f', 8)
          .arg(coords[i].z(), 12, 'f', 8);
      }
    } // End %POSCAR%

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
    QTextStream out (&str);
    out
      << "Crystal specific information:\n"
      << "%POSCAR% -- VASP poscar generator\n"
      << "%coordsFrac% -- fractional coordinate data\n\t[symbol] [x] [y] [z]\n"
      << "%coordsFracId% -- fractional coordinate data with atomic number\n\t[symbol] [atomic number] [x] [y] [z]\n"
      << "%gulpFracShell% -- fractional coordinates for use in GULP core/shell calculations:\n"
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

  bool XtalOpt::load(const QString &filename, const bool forceReadOnly)
  {
    if (forceReadOnly) {
      readOnly = true;
    }

    loaded = true;

    // Attempt to open state file
    QFile file (filename);
    if (!file.open(QIODevice::ReadOnly)) {
      error("XtalOpt::load(): Error opening file "
            +file.fileName() + " for reading...");
      return false;
    }

    SETTINGS(filename);
    int loadedVersion = settings->value("xtalopt/version", 0).toInt();

    // Update config data. Be sure to bump m_schemaVersion in ctor if
    // adding updates.
    switch (loadedVersion) {
    case 0:
    case 1:
    case 2: // Tab edit bumped to V2. No change here.
      break;
    default:
      error("XtalOpt::load(): Settings in file "+file.fileName()+
            " cannot be opened by this version of XtalOpt. Please "
            "visit http://xtalopt.openmolecules.net to obtain a "
            "newer version.");
      return false;
    }

    bool stateFileIsValid = settings->value("xtalopt/saveSuccessful",
                                            false).toBool();
    if (!stateFileIsValid) {
      error("XtalOpt::load(): File " + file.fileName() +
            " is incomplete, corrupt, or invalid. (Try "
            + file.fileName() + ".old if it exists)");
      return false;
    }

    DESTROY_SETTINGS(filename);

    // Get path and other info for later:
    QFileInfo stateInfo (file);
    // path to resume file
    QDir dataDir  = stateInfo.absoluteDir();
    QString dataPath = dataDir.absolutePath() + "/";
    // list of xtal dirs
    QStringList xtalDirs = dataDir.entryList(QStringList(),
                                             QDir::AllDirs, QDir::Size);
    xtalDirs.removeAll(".");
    xtalDirs.removeAll("..");
    for (int i = 0; i < xtalDirs.size(); i++) {
      // old versions of xtalopt used xtal.state, so still check for it.
      if (!QFile::exists(dataPath + "/" + xtalDirs.at(i)
                         + "/structure.state") &&
          !QFile::exists(dataPath + "/" + xtalDirs.at(i)
                         + "/xtal.state") ) {
          xtalDirs.removeAt(i);
          i--;
      }
    }

    // Set filePath:
    QString newFilePath = dataPath;
    QString newFileBase = filename;
    newFileBase.remove(newFilePath);
    newFileBase.remove("xtalopt.state.old");
    newFileBase.remove("xtalopt.state.tmp");
    newFileBase.remove("xtalopt.state");

    // TODO For some reason, the local view of "this" is not changed
    // when the settings are loaded in the following line. The tabs
    // are loading the settings and setting the variables in their
    // scope, but it isn't changing it here. Caching issue maybe?
    m_dialog->readSettings(filename);

#ifdef ENABLE_SSH
    // Create the SSHManager if running remotely
    if (qobject_cast<RemoteQueueInterface*>(m_queueInterface) != 0) {
      if (!this->createSSHConnections()) {
        error(tr("Could not create ssh connections."));
        return false;
      }
    }
#endif // ENABLE_SSH

    debug(tr("Resuming XtalOpt session in '%1' (%2) readOnly = %3")
          .arg(filename)
          .arg((m_optimizer) ? m_optimizer->getIDString()
                             : "No set optimizer")
          .arg( (readOnly) ? "true" : "false"));

    // Xtals
    // Initialize progress bar:
    m_dialog->updateProgressMaximum(xtalDirs.size());
    // If a local queue interface was used, all InProcess structures must be
    // Restarted.
    bool restartInProcessStructures = false;
    bool clearJobIDs = false;
    if (qobject_cast<LocalQueueInterface*>(m_queueInterface)) {
      restartInProcessStructures = true;
      clearJobIDs = true;
    }
    // Load xtals
    Xtal* xtal;
    QList<uint> keys = comp.keys();
    QList<Structure*> loadedStructures;
    QString xtalStateFileName;
    for (int i = 0; i < xtalDirs.size(); i++) {
      m_dialog->updateProgressLabel(tr("Loading structures(%1 of %2)...")
                                    .arg(i+1).arg(xtalDirs.size()));
      m_dialog->updateProgressValue(i);

      xtalStateFileName = dataPath + "/" + xtalDirs.at(i) + "/structure.state";
      debug(tr("Loading structure %1").arg(xtalStateFileName));
      // Check if this is an older session that used xtal.state instead.
      if ( !QFile::exists(xtalStateFileName) &&
           QFile::exists(dataPath + "/" + xtalDirs.at(i) + "/xtal.state") ) {
        xtalStateFileName = dataPath + "/" + xtalDirs.at(i) + "/xtal.state";
      }

      xtal = new Xtal();
      QWriteLocker locker (xtal->lock());
      xtal->moveToThread(m_tracker->thread());
      xtal->setupConnections();
      xtal->setFileName(dataPath + "/" + xtalDirs.at(i) + "/");
      xtal->readSettings(xtalStateFileName, true);

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
      int version = settings->value("structure/version").toInt();
      bool saveSuccessful = settings->value("structure/saveSuccessful",
                                            false).toBool();
      if (version >= 3) {
        if (!saveSuccessful) {
          error(tr("Error, structure.state file was not saved successfully for "
                   "%1. This structure will be excluded.")
                .arg(xtal->fileName()));
          continue;
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
        double energy = settings->value("structure/current/energy", 0)
                                                                    .toDouble();
        xtal->setEnergy(energy * EV_TO_KJ_PER_MOL);
        DESTROY_SETTINGS(xtalStateFileName);
        locker.unlock();
        loadedStructures.append(qobject_cast<Structure*>(xtal));
        continue;
      }
      DESTROY_SETTINGS(xtalStateFileName);
      if (!m_optimizer->load(xtal)) {
        error(tr("Error, no (or not appropriate for %1) xtal data in "
                 "%2.\n\nThis could be a result of resuming a structure "
                 "that has not yet done any local optimizations. If so, "
                 "safely ignore this message.")
              .arg(m_optimizer->getIDString())
              .arg(xtal->fileName()));
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

    m_dialog->updateProgressMinimum(0);
    m_dialog->updateProgressValue(0);
    m_dialog->updateProgressMaximum(loadedStructures.size());
    m_dialog->updateProgressLabel("Sorting and checking structures...");

    // Sort Xtals by index values
    int curpos = 0;
    //dialog->stopProgressUpdate();
    //dialog->startProgressUpdate("Sorting xtals...", 0, loadedStructures.size()-1);
    for (int i = 0; i < loadedStructures.size(); i++) {
      m_dialog->updateProgressValue(i);
      for (int j = 0; j < loadedStructures.size(); j++) {
        //dialog->updateProgressValue(curpos);
        if (loadedStructures.at(j)->getIndex() == i) {
          loadedStructures.swap(j, curpos);
          curpos++;
        }
      }
    }

    m_dialog->updateProgressMinimum(0);
    m_dialog->updateProgressValue(0);
    m_dialog->updateProgressMaximum(loadedStructures.size());
    m_dialog->updateProgressLabel("Updating structure indices...");

    // Reassign indices (shouldn't always be necessary, but just in case...)
    for (int i = 0; i < loadedStructures.size(); i++) {
      m_dialog->updateProgressValue(i);
      loadedStructures.at(i)->setIndex(i);
    }

    m_dialog->updateProgressMinimum(0);
    m_dialog->updateProgressValue(0);
    m_dialog->updateProgressMaximum(loadedStructures.size());
    m_dialog->updateProgressLabel("Preparing GUI and tracker...");

    // Reset the local file path information in case the files have moved
    filePath = newFilePath;

    Structure *s= 0;
    for (int i = 0; i < loadedStructures.size(); i++) {
      s = loadedStructures.at(i);
      m_dialog->updateProgressValue(i);
      m_tracker->lockForWrite();
      m_tracker->append(s);
      m_tracker->unlock();
      if (s->getStatus() == Structure::WaitingForOptimization)
        m_queue->appendToJobStartTracker(s);
    }

    m_dialog->updateProgressLabel("Done!");

    // Check if user wants to resume the search
    if (!readOnly) {
      bool resume;
      needBoolean(tr("Session '%1' (%2) loaded. Would you like to start "
                     "submitting jobs and resume the search? (Answering "
                     "\"No\" will enter read-only mode.)")
                       .arg(description).arg(filePath),
                       &resume);

      readOnly = !resume;
      qDebug() << "Read only? " << readOnly;
    }

    return true;
  }

  void XtalOpt::resetSpacegroups() {
    if (isStarting) {
      return;
    }
    QtConcurrent::run(this, &XtalOpt::resetSpacegroups_);
  }

  void XtalOpt::resetSpacegroups_() {
    const QList<Structure*> structures = *(m_tracker->list());
    for (QList<Structure*>::const_iterator it = structures.constBegin(),
         it_end = structures.constEnd(); it != it_end; ++it)
    {
      (*it)->lock()->lockForWrite();
      qobject_cast<Xtal*>(*it)->findSpaceGroup(tol_spg);
      (*it)->lock()->unlock();
    }
  }

  void XtalOpt::resetDuplicates() {
    if (isStarting) {
      return;
    }
    QtConcurrent::run(this, &XtalOpt::resetDuplicates_);
  }

  void XtalOpt::resetDuplicates_() {
    const QList<Structure*> *structures = m_tracker->list();
    Xtal *xtal = 0;
    for (int i = 0; i < structures->size(); i++) {
      xtal = qobject_cast<Xtal*>(structures->at(i));
      xtal->lock()->lockForWrite();
      if (xtal->getStatus() == Xtal::Duplicate)
        xtal->setStatus(Xtal::Optimized);
      xtal->structureChanged(); // Reset cached comparisons
      xtal->lock()->unlock();
    }
    checkForDuplicates();
  }

  // Helper struct for the map below
  struct dupCheckStruct
  {
    Xtal *i, *j;
    double tol_len, tol_ang;
  };

  void checkIfDups(dupCheckStruct & st)
  {
    Xtal *kickXtal, *keepXtal;
    st.i->lock()->lockForRead();
    st.j->lock()->lockForRead();
    if (st.i->compareCoordinates(*st.j, st.tol_len, st.tol_ang)) {
      // Mark the newest xtal as a duplicate of the oldest. This keeps the
      // lowest-energy plot trace accurate.
      if (st.i->getIndex() > st.j->getIndex()) {
        kickXtal = st.i;
        keepXtal = st.j;
      }
      else {
        kickXtal = st.j;
        keepXtal = st.i;
      }
      kickXtal->lock()->unlock();
      kickXtal->lock()->lockForWrite();
      kickXtal->setStatus(Xtal::Duplicate);
      kickXtal->setDuplicateString(QString("%1x%2")
                                   .arg(keepXtal->getGeneration())
                                   .arg(keepXtal->getIDNumber()));
    }
    st.i->lock()->unlock();
    st.j->lock()->unlock();
  }

  void XtalOpt::checkForDuplicates() {
    if (isStarting) {
      return;
    }
    QtConcurrent::run(this, &XtalOpt::checkForDuplicates_);
  }

  void XtalOpt::checkForDuplicates_() {
    m_tracker->lockForRead();
    const QList<Structure*> *structures = m_tracker->list();
    QList<Xtal*> xtals;
    Xtal *xtal;
    for (int i = 0; i < structures->size(); i++) {
      xtal = qobject_cast<Xtal*>(structures->at(i));
      xtals.append(xtal);
    }
    m_tracker->unlock();

    // Build helper structs
    QList<dupCheckStruct> sts;
    dupCheckStruct st;
    for (QList<Xtal*>::iterator xi = xtals.begin();
         xi != xtals.end(); xi++) {
      (*xi)->lock()->lockForRead();
      if ((*xi)->getStatus() != Xtal::Optimized) {
        (*xi)->lock()->unlock();
        continue;
      }

      for (QList<Xtal*>::iterator xj = xi + 1;
           xj != xtals.end(); xj++) {
        (*xj)->lock()->lockForRead();
        if ((*xj)->getStatus() != Xtal::Optimized) {
          (*xj)->lock()->unlock();
          continue;
        }
        if (((*xi)->hasChangedSinceDupChecked() ||
             (*xj)->hasChangedSinceDupChecked()) &&
            // Perform a course enthalpy screening to cut down on number of
            // comparisons
            fabs((*xi)->getEnthalpy() - (*xj)->getEnthalpy()) < 1.0)
        {
          st.i = (*xi);
          st.j = (*xj);
          st.tol_len = this->tol_xcLength;
          st.tol_ang = this->tol_xcAngle;
          sts.append(st);
        }
        (*xj)->lock()->unlock();
      }
      // Nothing else should be setting this, so just update under a
      // read lock
      (*xi)->setChangedSinceDupChecked(false);
      (*xi)->lock()->unlock();
    }

    QtConcurrent::blockingMap(sts, checkIfDups);

    emit refreshAllStructureInfo();
  }

} // end namespace XtalOpt

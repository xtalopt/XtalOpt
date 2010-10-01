/**********************************************************************
  OpenBabelOptimizer - Tools to interface with OpenBabel's force fields

  Copyright (C) 2009-2010 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <gapc/optimizers/openbabel.h>
#include <gapc/structures/protectedcluster.h>

#include <avogadro/moleculefile.h>

#include <openbabel/obconversion.h>
#include <openbabel/forcefield.h>
#include <openbabel/mol.h>

#include <QDir>
#include <QString>
#include <QSemaphore>

using namespace Avogadro;
using namespace OpenBabel;

namespace GAPC {

  OpenBabelOptimizer::OpenBabelOptimizer(OptBase *parent, const QString &filename) :
    Optimizer(parent)
  {
    // Since OpenBabel optimization occurs programmatically, this file
    // is quite different from the other optimizers.

    m_idString = "OpenBabel";

    m_templates.clear();
    m_completionFilename = "NA";
    m_completionString   = "Not used!";
    m_outputFilenames.append("structure.cml");

    readSettings(filename);
  }

  bool OpenBabelOptimizer::writeInputFiles(Structure *structure) {
    // Stop any running jobs associated with this structure
    deleteJob(structure);

    // Lock
    QReadLocker locker (structure->lock());

    // No actual files to write.

    // Update info
    locker.unlock();
    structure->lock()->lockForWrite();
    structure->setStatus(Structure::WaitingForOptimization);
    structure->lock()->unlock();
    return true;
  }

  bool OpenBabelOptimizer::startOptimization(Structure *structure) {
    structure->setStatus(Structure::InProcess);
    structure->startOptTimer();

    // Run optimization
    // TODO Don't hard code these values...
    int maxSteps = 10000;
    float econv = 1e-9;
    QString forcefield = "UFF";

    OBMol obmol = structure->OBMol();
    OBForceField *pFF = OBForceField::FindForceField(forcefield.toStdString());
    OBForceField *ff = pFF->MakeNewInstance();
    ff->SetLogFile(&std::cout);
    ff->SetLogLevel(OBFF_LOGLVL_NONE);


    if (!ff || !ff->Setup(obmol)) {
      m_opt->warning("OpenBabelOptimizer::startOptimization: Cannot set up forcefield "
                     + forcefield);
      return false;
    }

    // Perform the actual minimization
    ff->ConjugateGradients(maxSteps, econv);
    ff->GetCoordinates(obmol);
    double energy = ff->Energy();

    // lock xtal
    QWriteLocker wlocker (structure->lock());

    // Update structure
    // Copy info from obmol -> structure.
    // atoms
    while (structure->numAtoms() < obmol.NumAtoms())
      structure->addAtom();
    QList<Atom*> atoms = structure->atoms();

    uint i = 0;
    FOR_ATOMS_OF_MOL(atm, obmol) {
      atoms.at(i)->setPos(Eigen::Vector3d(atm->x(), atm->y(), atm->z()));
      atoms.at(i)->setAtomicNumber(atm->GetAtomicNum());
      i++;
    }

    structure->setEnergy(energy);

    // Write output file:
    if (!Avogadro::MoleculeFile::writeMolecule(structure,
                                               structure->fileName() + "/structure.cml")
        ) {
      m_opt->warning("OpenBabelOptimizer::startOptimization: Error writing to file "
                     + structure->fileName() + "/structure.cml");
      return false;
    }

    structure->stopOptTimer();
    structure->resetFailCount();
    structure->setJobID(0);
    structure->setStatus(Structure::StepOptimized);
    wlocker.unlock();
    return true;
  }

  Optimizer::JobState OpenBabelOptimizer::getStatus(Structure *structure)
  {
    QReadLocker rlocker (structure->lock());
    if (structure->getStatus() == Structure::InProcess) {
      return Optimizer::Running;
    }
    else {
      return Optimizer::Unknown;
    }
  }

  bool OpenBabelOptimizer::getQueueList(QStringList &, QMutex*) {
    return true;
  }

  bool OpenBabelOptimizer::copyRemoteToLocalCache(Structure*)
  {
    return true;
  }

}

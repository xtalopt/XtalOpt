/*****************************************************************************
  OpenBabelQueueInterface - Queue for OpenBabelOptimizer

  Copyright (C) 2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ****************************************************************************/

#include "openbabel.h"


#include <xtalopt/molecularxtaloptimizer.h>
#include <xtalopt/queueinterfaces/openbabeldialog.h>
#include <xtalopt/structures/molecularxtal.h>

#include <globalsearch/macros.h>
#include <globalsearch/optbase.h>
#include <globalsearch/structure.h>

#include <avogadro/moleculefile.h>

#include <QtCore/QDir>
#include <QtCore/QLinkedList>
#include <QtCore/QReadWriteLock>
#include <QtCore/QtConcurrentRun>
#include <QtCore/QVector>

using namespace Avogadro;
using namespace GlobalSearch;

namespace XtalOpt
{

OpenBabelQueueInterface::OpenBabelQueueInterface(OptBase *parent,
                                                 const QString &settingsFile)
  : LocalQueueInterface(parent, settingsFile),
    m_queueMutex(new QReadWriteLock ()),
    m_queue(new QLinkedList<MolecularXtal*> ()),
    m_jobCounter(0),
    m_isDestroying(false)
{
  m_idString = "OpenBabel";
  m_hasDialog = true;
}

OpenBabelQueueInterface::~OpenBabelQueueInterface()
{
  this->prepareForDestroy();

  delete m_queueMutex;
  m_queue = NULL;

  delete m_queue;
  m_queue = NULL;
}

void OpenBabelQueueInterface::prepareForDestroy()
{
  // Prevent new jobs from starting
  m_isDestroying = true;

  // Abort all jobs in the queue:
  m_queueMutex->lockForWrite();
  QLinkedList<MolecularXtal*> toStop = *m_queue;
  m_queueMutex->unlock();
  for (QLinkedList<MolecularXtal*>::const_iterator it = toStop.constBegin(),
       it_end = toStop.constEnd(); it != it_end; ++it) {
    this->stopJob(*it);
  }
}

bool OpenBabelQueueInterface::writeInputFiles(Structure *s) const
{
  QString err;

  s->lock()->lockForRead();
  m_opt->sOBMutex->lock();
  bool ok = MoleculeFile::writeMolecule(s, s->fileName() + "/input.cml",
                                        "cml", "p", &err);
  m_opt->sOBMutex->unlock();
  s->lock()->unlock();

  if (!ok) {
    m_opt->warning(QString("Error writing input geometry: %1").arg(err));
    return false;
  }

  return true;
}

bool OpenBabelQueueInterface::isReadyToSearch(QString *str)
{
  // Is a working directory specified?
  if (m_opt->filePath.isEmpty()) {
    *str = tr("Local working directory is not set. Check your Queue "
              "configuration.");
    return false;
  }

  // Can we write to the working directory?
  QDir workingdir (m_opt->filePath);
  bool writable = true;
  if (!workingdir.exists()) {
    if (!workingdir.mkpath(m_opt->filePath)) {
      writable = false;
    }
  }
  else {
    // If the path exists, attempt to open a small test file for writing
    QString filename = m_opt->filePath + QString("queuetest-")
      + QString::number(RANDUINT());
    QFile file (filename);
    if (!file.open(QFile::ReadWrite)) {
      writable = false;
    }
    file.remove();
  }
  if (!writable) {
    *str = tr("Cannot write to working directory '%1'.\n\nPlease "
              "change the permissions on this directory or use "
              "a different one.").arg(m_opt->filePath);
    return false;
  }

  *str = "";
  return true;
}

bool OpenBabelQueueInterface::startJob(Structure *s)
{
  if (m_isDestroying) {
    qWarning() << "Not starting optimization of" << s->getIDString() <<
                  "-- Queue interface is destroying.";
    return false;
  }

  MolecularXtal *mxtal = qobject_cast<MolecularXtal*>(s);

  if (mxtal == NULL) {
    qWarning() << "OpenBabelQueueInterface may only be used with MolecularXtal"
                  " objects.";
    return false;
  }

  QReadLocker rlocker (s->lock());

  if (s->getJobID() != 0) {
    m_opt->warning(tr("OpenBabelQueueInterface::startJob: Attempting to start "
                      "job for structure %1, but a JobID is already set (%2)")
        .arg(s->getIDString())
        .arg(s->getJobID()));
    return false;
  }

  MolecularXtalOptimizer *mxtalOpt =
      new MolecularXtalOptimizer(m_opt, m_opt->sOBMutex);
  mxtalOpt->setMXtal(mxtal);
  mxtalOpt->setEnergyConvergence(1e-6);
  mxtalOpt->setNumberOfGeometrySteps(1000);
  mxtalOpt->setSuperCellUpdateInterval(5);
  mxtalOpt->setVDWCutoff(10.0);
  mxtalOpt->setElectrostaticCutoff(10.0);
  mxtalOpt->setCutoffUpdateInterval(-1);
  mxtalOpt->setup();
  rlocker.unlock();

  // Make sure that setup() was called before adding mxtal to the queue.
  m_queueMutex->lockForWrite();
  if (m_isDestroying) {
    m_queueMutex->unlock();
    qWarning() << Q_FUNC_INFO << "Refusing structure" << mxtal->getIDString()
               << "-- QueueInterface is destroying.";
    return false;
  }
  int jobId = ++m_jobCounter;
  m_queue->append(mxtal);
  m_queueMutex->unlock();

  // Update mxtal metadata
  mxtal->lock()->lockForWrite();
  mxtal->setJobID(jobId);
  mxtal->startOptTimer();
  mxtal->lock()->unlock();

  QtConcurrent::run(mxtalOpt, &MolecularXtalOptimizer::run);
  return true;
}

bool OpenBabelQueueInterface::stopJob(Structure *s)
{
  MolecularXtal *mxtal = qobject_cast<MolecularXtal*>(s);
  if (mxtal == NULL || mxtal->getJobID() == 0) {
    return true;
  }

  QWriteLocker wlocker (s->lock());
  mxtal->setJobID(0);

  wlocker.unlock();
  m_queueMutex->lockForWrite();
  m_queue->removeOne(mxtal);
  m_queueMutex->unlock();
  wlocker.relock();

  // We'll clean up the mxtalOpt and update geometry here:
  MolecularXtalOptimizer *mxtalOpt = mxtal->getPreoptimizer();
  if (mxtalOpt == NULL) {
    return true;
  }

  // This is safe to call if the mxtalopt has finished
  mxtal->abortPreoptimization();
  mxtal->stopOptTimer();

  // Update if the optimization was complete:
  if (mxtalOpt->isConverged() || mxtalOpt->reachedStepLimit()) {
    mxtalOpt->updateMXtalCoords();
    mxtalOpt->updateMXtalEnergy();
    // Write output file
    QString err;
    m_opt->sOBMutex->lock();
    bool ok = MoleculeFile::writeMolecule(s,s->fileName() + "/optimized.cml",
                                          "cml", "p", &err);
    m_opt->sOBMutex->unlock();
    if (!ok) {
      m_opt->warning(QString("Error writing optimized geometry: %1").arg(err));
    }
  }

  // cleanup
  delete mxtalOpt;
  mxtalOpt = NULL;

  return true;
}

QueueInterface::QueueStatus
OpenBabelQueueInterface::getStatus(Structure *s) const
{
  MolecularXtal *mxtal = qobject_cast<MolecularXtal*>(s);
  if (mxtal == NULL) {
    return Unknown;
  }
  mxtal->lock()->lockForRead();
  unsigned int jobId = mxtal->getJobID();
  Structure::State state = mxtal->getStatus();
  int progress = mxtal->getPreOptProgress();
  mxtal->lock()->unlock();

  if (state == Structure::Submitted) {
    if (jobId == 0) {
      return Queued;
    }
    else {
      return Started;
    }
  }

  if (jobId == 0) {
    return Unknown;
  }

  MolecularXtalOptimizer *mxtalOpt = mxtal->getPreoptimizer();
  if (mxtalOpt == NULL) {
    return Unknown;
  }

  if (mxtalOpt->isRunning()) {
    return Running;
  }
  else {
    if (mxtalOpt->isConverged() || mxtalOpt->reachedStepLimit()) {
      return Success;
    }
    else {
      return Error;
    }
  }

  return Unknown;
}

bool OpenBabelQueueInterface::prepareForStructureUpdate(Structure *s) const
{
  // Nothing to do!
  return true;
}

QDialog * OpenBabelQueueInterface::dialog()
{
  if (!m_dialog) {
    m_dialog = new OpenBabelQueueInterfaceConfigDialog (m_opt->dialog(),
                                                        m_opt,
                                                        this);
  }
  OpenBabelQueueInterfaceConfigDialog *d =
      qobject_cast<OpenBabelQueueInterfaceConfigDialog*>(m_dialog);
  d->updateGUI();

  return d;
}

MolecularXtal *OpenBabelQueueInterface::getMXtal(unsigned long jobId)
{
  QReadLocker rlocker (m_queueMutex);
  for (QLinkedList<MolecularXtal*>::const_iterator it = m_queue->constBegin(),
       it_end = m_queue->constEnd(); it != it_end; ++it) {
    (*it)->lock()->lockForRead();
    if ((*it)->getJobID() == jobId) {
      (*it)->lock()->unlock();
      return (*it);
    }
    (*it)->lock()->unlock();
  }

  return NULL;
}

}


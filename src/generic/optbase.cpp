/**********************************************************************
  OptBase - Base class for global search extensions

  Copyright (C) 2010 by David C. Lonie

  This file is part of the Avogadro molecular editor project.
  For more information, see <http://avogadro.openmolecules.net/>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include "optbase.h"

#include "structure.h"
#include "optimizer.h"
#include "queuemanager.h"
#include "bt.h"

using namespace std;
using namespace OpenBabel;
using namespace Eigen;

namespace Avogadro {

  OptBase::OptBase(QObject *parent) :
    QObject(parent)
  {
    m_tracker   = new Tracker (this);
    m_queue     = new QueueManager(this, m_tracker);
    m_optimizer = 0; // This will be set when the GUI is initialized
    sOBMutex = new QMutex;
    stateFileMutex = new QMutex;
    backTraceMutex = new QMutex;

    savePending = false;

    testingMode = false;
    test_nRunsStart = 1;
    test_nRunsEnd = 100;
    test_nStructs = 600;

    // Connections
    connect(this, SIGNAL(startingSession()),
            this, SLOT(setIsStartingTrue()));
    connect(this, SIGNAL(sessionStarted()),
            this, SLOT(setIsStartingFalse()));
  }

  OptBase::~OptBase() {
    // Wait for save to finish
    while (savePending) {};
    savePending = true;
    delete m_queue;
    delete m_tracker;
  }

  void OptBase::reset() {
    m_tracker->deleteAllStructures();
    m_tracker->reset();
    m_queue->reset();
  }

  void OptBase::printBackTrace() {
    backTraceMutex->lock();
    QStringList l = getBackTrace();
    backTraceMutex->unlock();
    for (int i = 0; i < l.size();i++)
      qDebug() << l.at(i);
  }

  void OptBase::setOptimizer_opt(Optimizer *o) {
    Optimizer *old = m_optimizer;
    if (m_optimizer) {
      // Save settings explicitly. This is called in the destructer, but
      // we may need some settings in the new optimizer.
      old->writeSettings();
      old->deleteLater();
    }
    m_optimizer = o;
    emit optimizerChanged(o);
  }

  void OptBase::warning(const QString & s) {
    qWarning() << "Warning: " << s;
    emit warningStatement(s);
  }

  void OptBase::debug(const QString & s) {
    qDebug() << "Debug: " << s;
    emit debugStatement(s);
  }

  void OptBase::error(const QString & s) {
    qWarning() << "Error: " << s;
    emit errorStatement(s);
  }

} // end namespace Avogadro

#include "optbase.moc"

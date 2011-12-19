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

#ifndef OPENBABELQUEUEINTERFACE_H
#define OPENBABELQUEUEINTERFACE_H

#include <globalsearch/queueinterfaces/local.h>

class QReadWriteLock;
template <typename T> class QLinkedList;

namespace XtalOpt
{
class OpenBabelQueueInterfaceConfigDialog;
class MolecularXtal;

class OpenBabelQueueInterface : public GlobalSearch::LocalQueueInterface
{
  Q_OBJECT
public:
  explicit OpenBabelQueueInterface(GlobalSearch::OptBase *parent,
                                   const QString &settingsFile = "");
  virtual ~OpenBabelQueueInterface();

  void prepareForDestroy(); // stop running new jobs and abort running ones

  virtual bool writeInputFiles(GlobalSearch::Structure *s) const;

  virtual bool isReadyToSearch(QString *err);

  virtual bool startJob(GlobalSearch::Structure *s);

  virtual bool stopJob(GlobalSearch::Structure *s);

  virtual QueueInterface::QueueStatus
  getStatus(GlobalSearch::Structure *s) const;

  virtual bool prepareForStructureUpdate(GlobalSearch::Structure *s) const;

  virtual QDialog* dialog();

  friend class InternalQueueInterfaceConfigDialog;

protected:
  MolecularXtal *getMXtal(unsigned long jobId);

  QReadWriteLock *m_queueMutex;
  QLinkedList<MolecularXtal*> *m_queue;
  unsigned long m_jobCounter;
  bool m_isDestroying;

};

}

#endif // OPENBABELQUEUEINTERFACE_H

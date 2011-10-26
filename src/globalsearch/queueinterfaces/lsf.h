/**********************************************************************
  LsfQueueInterface - Interface for running jobs on a remote LSF
  cluster.

  Copyright (C) 2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef LSFQUEUEINTERFACE_H
#define LSFQUEUEINTERFACE_H

#ifdef ENABLE_SSH

// Tell doxygen to skip this file
/// \cond

#include <globalsearch/queueinterfaces/lsfdialog.h>
#include <globalsearch/queueinterfaces/remote.h>

#include <QtCore/QDateTime>
#include <QtCore/QReadWriteLock>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace GlobalSearch {

  class LsfQueueInterface : public RemoteQueueInterface
  {
    Q_OBJECT;

  public:
    explicit LsfQueueInterface(OptBase *parent,
                               const QString &settingsFile = "");

    virtual ~LsfQueueInterface();

    virtual bool isReadyToSearch(QString *str);

    QDialog* dialog();

    friend class LsfConfigDialog;

  public slots:
    void readSettings(const QString &filename = "");
    void writeSettings(const QString &filename = "");
    bool startJob(Structure *s);
    bool stopJob(Structure *s);
    QueueInterface::QueueStatus getStatus(Structure *s) const;

  protected:
    // Fetches the queue from the server
    QStringList getQueueList() const;
    // Cached queue data
    QStringList m_queueData;
    // Limits queue checks to once per second
    QDateTime m_queueTimeStamp;
    // Locks for m_queueData;
    QReadWriteLock m_queueMutex;
    // Paths:
    QString m_bjobs;
    QString m_bsub;
    QString m_bkill;
    // Clean remote directories when a job is stopped?
    bool m_cleanRemoteOnStop;
  };
}

#endif // ENABLE_SSH
#endif // LSFQUEUEINTERFACE_H

// End doxygen skip:
/// \endcond

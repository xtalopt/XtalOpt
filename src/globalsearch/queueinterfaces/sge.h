/**********************************************************************
  SgeQueueInterface - Interface for running jobs on a remote cluster's
  Sun Grid Engine

  Copyright (C) 2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef SGEQUEUEINTERFACE_H
#define SGEQUEUEINTERFACE_H

#ifdef ENABLE_SSH

// Tell doxygen to skip this file
/// \cond

#include <globalsearch/queueinterfaces/sgedialog.h>
#include <globalsearch/queueinterfaces/remote.h>

#include <QtCore/QDateTime>
#include <QtCore/QReadWriteLock>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace GlobalSearch {

  class SgeQueueInterface : public RemoteQueueInterface
  {
    Q_OBJECT;

  public:
    explicit SgeQueueInterface(OptBase *parent,
                               const QString &settingsFile = "");

    virtual ~SgeQueueInterface();

    virtual bool isReadyToSearch(QString *str);

    QDialog* dialog();

    friend class SgeConfigDialog;

  public slots:
    void readSettings(const QString &filename = "");
    void writeSettings(const QString &filename = "");
    bool startJob(Structure *s);
    bool stopJob(Structure *s);
    QueueInterface::QueueStatus getStatus(Structure *s) const;
    void setInterval(const int sec);

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
    QString m_qstat;
    QString m_qsub;
    QString m_qdel;
    // Refresh interval for queue data
    int m_interval;
    // Clean remote directories when a job is stopped?
    bool m_cleanRemoteOnStop;
  };
}

// End doxygen skip:
/// \endcond

#endif // ENABLE_SSH
#endif // SGEQUEUEINTERFACE

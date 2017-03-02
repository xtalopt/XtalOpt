/**********************************************************************
  PbsQueueInterface - Interface for running jobs on a remote PBS
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

#ifndef PBSQUEUEINTERFACE_H
#define PBSQUEUEINTERFACE_H

#ifdef ENABLE_SSH

// Tell doxygen to skip this file
/// \cond

#include <globalsearch/queueinterfaces/pbsdialog.h>
#include <globalsearch/queueinterfaces/remote.h>

#include <QtCore/QDateTime>
#include <QtCore/QReadWriteLock>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace GlobalSearch {

  class PbsQueueInterface : public RemoteQueueInterface
  {
    Q_OBJECT;

  public:
    explicit PbsQueueInterface(OptBase *parent,
                               const QString &settingsFile = "");

    virtual ~PbsQueueInterface() override;

    virtual bool isReadyToSearch(QString *str) override;

    QDialog* dialog() override;

    friend class PbsConfigDialog;

  public slots:
    void readSettings(const QString &filename = "") override;
    void writeSettings(const QString &filename = "") override;
    bool startJob(Structure *s) override;
    bool stopJob(Structure *s) override;
    QueueInterface::QueueStatus getStatus(Structure *s) const override;
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

#endif // ENABLE_SSH
#endif // PBSQUEUEINTERFACE_H

// End doxygen skip:
/// \endcond

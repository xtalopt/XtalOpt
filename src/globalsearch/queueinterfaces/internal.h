/*****************************************************************************
  InternalQueueInterface - Interface for running jobs locally on an internal
    queue

  Copyright (C) 2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ****************************************************************************/

#ifndef INTERNALQUEUEINTERFACE_H
#define INTERNALQUEUEINTERFACE_H

#include <globalsearch/queueinterfaces/local.h>

#include <QtCore/QProcess>

// Doxygen skip:
/// @cond
namespace GlobalSearch {
class InternalQueueInterfaceConfigDialog;

// Since QProcess doesn't support polling at all (e.g. the only way
// to track its progress is to monitor signals), this class provides
// a basic polling wrapper.
class InternalQueueProcess : public QProcess
{ Q_OBJECT;
public:
  enum Status {NotStarted = 0, Running, Finished, Error};
InternalQueueProcess(QObject *parent) :
  QProcess(parent), m_status(NotStarted)
  {
    connect(this, SIGNAL(started()), this, SLOT(setRunning()));
    connect(this, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(setFinished()));
    connect(this, SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(setFinished()));}
public slots:
  void setRunning() {m_status = Running;};
  void setFinished() {m_status = Finished;};
  Status status() {return m_status;};
private:
  Status m_status;
};

class InternalQueueInterface : public LocalQueueInterface
{
    Q_OBJECT
public:
  friend class InternalQueueInterfaceConfigDialog;
  explicit InternalQueueInterface(OptBase *parent,
                                  const QString &settingsFile = "");
  virtual ~InternalQueueInterface();

  /**
   * Check that all mandatory internal variables are set. Check this
   * before starting a search.
   *
   * @param err String to be overwritten with an error message
   *
   * @return true if all variables are initialized, false
   * otherwise. If false, \a err will be overwritten with a
   * user-friendly error message.
   */
  virtual bool isReadyToSearch(QString *err);

public slots:

  /**
   * Start a job for Structure \a s.
   *
   * @note Ensure that writeFiles is called before attempting to
   * start the job.
   *
   * @return True on success, false otherwise.
   */
  virtual bool startJob(Structure *s);

  /**
   * Stop any currently running jobs for Structure \a s.
   *
   * @return True on success, false otherwise.
   */
  virtual bool stopJob(Structure *s);

  /**
   * @return The queue status of Structure \a s.
   */
  virtual QueueInterface::QueueStatus getStatus(Structure *s) const;

  /**
   * Perform any work needed before calling Optimizer::update. This
   * function mainly exists for RemoteQueue classes to copy files
   * back from the server, but may be used for other purposes. It is
   * guaranteed to be called by Optimizer before updating.
   *
   * @param s The structure that is to be updated.
   *
   * @return True on success, false otherwise.
   */
  virtual bool prepareForStructureUpdate(Structure *s) const;

  /**
   * @return The configuration dialog for this QueueInterface, if it
   * exists, otherwise 0.
   * @sa hasDialog()
   * @ingroup dialog
   */
  virtual QDialog* dialog();

protected:
  /// Look up hash for mapping jobID's to processes.
  /// Key: PID, Value: QProcess handle
  QHash<unsigned long, InternalQueueProcess*> m_processes;

};

}
// End doxygen skip:
/// @endcond

#endif // INTERNALQUEUEINTERFACE_H

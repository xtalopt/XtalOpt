/**********************************************************************
  LocalQueueInterface - Interface for running jobs locally.

  Copyright (C) 2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef LOCALQUEUEINTERFACE_H
#define LOCALQUEUEINTERFACE_H

#include <globalsearch/queueinterface.h>
#include <globalsearch/macros.h>

#include <QtCore/QProcess>

namespace GlobalSearch {
  class LocalQueueInterfaceConfigDialog;

  /// @cond
  // Since QProcess doesn't support polling at all (e.g. the only way
  // to track its progress is to monitor signals), this class provides
  // a basic polling wrapper.
  class LocalQueueProcess : public QProcess
  { Q_OBJECT;
  public:
    enum Status {NotStarted = 0, Running, Finished, Error};
  LocalQueueProcess(QObject *parent) :
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
  /// @endcond

  /**
   * @class LocalQueueInterface local.h <globalsearch/local.h>
   *
   * @brief Interface for running jobs locally.
   *
   * @author David C. Lonie
   */
  class LocalQueueInterface : public QueueInterface
  {
    Q_OBJECT;

  public:
    friend class LocalQueueInterfaceConfigDialog;

    /**
     * Constructor
     *
     * @param parent OptBase parent
     * @param settingFile Filename from which to initialize settings.
     */
    explicit LocalQueueInterface(OptBase *parent,
                                 const QString &settingFile = "");

    /**
     * Destructor
     */
    virtual ~LocalQueueInterface();

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
     * Write the input files in the hash \a files to the appropriate
     * location for Structure \a s.
     *
     * @param s Structure of interest
     * @param files Key: filename, Value: text.
     *
     * @note The filenames in \a files must not be absolute, but
     * relative to the structure's working directory.
     *
     * @return True on success, false otherwise.
     */
    virtual bool writeFiles(Structure *s,
                            const QHash<QString, QString> &files) const;

    /**
     * Saves a copy of the error directory that caused this structure to fail.
     * Saves it in <local_path>/errorDirs
     *
     * @param structure Structure of interest
     *
     * @return True on success, false otherwise
     */
    bool logErrorDirectory(Structure *s) const;

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
     * Check if the file \a filename exists in the working directory
     * of Structure \a s and store the result in \a exists.
     *
     * @note This function uses the argument \a exists to report
     * whether or not the file exists. The return value indicates
     * whether the file check was performed without errors
     * (e.g. network errors).
     *
     * @return True if the test encountered no errors, false otherwise.
     */
    virtual bool checkIfFileExists(Structure *s,
                                   const QString &filename,
                                   bool *exists);

    /**
     * Retrieve the contents of the file \a filename for Structure \a
     * s as a QString \a contents.
     *
     * @return True on success, false otherwise.
     */
    virtual bool fetchFile(Structure *s,
                           const QString &filename,
                           QString *contents) const;

    /**
     * Grep through the file \a filename for Structure \a s's working
     * directory, looking for \a matchText. The list of matches is
     * returned in the QStringList \a matches and the exit status is
     * returned as \a exitcode.
     *
     * Possible exitcodes:
     *   - 0: Matches were found, execution successful
     *   - 1: No matches found, execution successful
     *   - 2: Execution unsuccessful
     *
     * @param s Structure of interest
     * @param matchText Text to match
     * @param filename Name of file to grep
     * @param matches List of matches (return)
     * @param exitcode Exit code of grep (see details) (return)
     * @param caseSensitive If true, match case. Otherwise, perform
     * case-insensitive search (e.g. grep -i) Default is true.
     *
     * @return True on success, false otherwise.
     *
     * @note There are two types of failure possible here: Either the
     * \a exitcode can be 2 or the function can return false. If the
     * \a exitcode is 2, then grep failed to execute. If false, then
     * there was a failure in the interface code, likely a
     * communication error with a remote server.
     *
     * @note On local queue interface, grep is not actually used and
     * the exit code behavior is emulated.
     */
    virtual bool grepFile(Structure *s,
                          const QString &matchText,
                          const QString &filename,
                          QStringList *matches = 0,
                          int *exitcode = 0,
                          const bool caseSensitive = true) const;

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
    QHash<unsigned long, LocalQueueProcess*> m_processes;

  };
}

#endif // LOCALQUEUEINTERFACE_H

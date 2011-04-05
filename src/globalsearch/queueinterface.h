/**********************************************************************
  QueueInterface - Job submission interface

  Copyright (C) 2011 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef QUEUEINTERFACE_H
#define QUEUEINTERFACE_H

#include <globalsearch/optbase.h>

#include <QtCore/QHash>
#include <QtCore/QStringList>

class QDialog;

namespace GlobalSearch {
  class Structure;

  /**
   * @class QueueInterface queueinterface.h <globalsearch/queueinterface.h>
   *
   * @brief Abstract interface for job submission.
   *
   * @author David C. Lonie
   *
   * Do not derive directly from this class. Instead, use LocalQueue
   * or (more likely) RemoteQueue.
   *
   * TODO detailed description.
   */
  class QueueInterface : public QObject
  {
    Q_OBJECT;

  public:
    /**
     * Constructor
     *
     * @param parent OptBase parent
     * @param settingFile Filename from which to initialize settings.
     */
    explicit QueueInterface(OptBase *parent,
                            const QString &settingFile = "")
      : QObject(parent), m_opt(parent), m_hasDialog(false), m_dialog(0) {};

    /**
     * Destructor
     */
    virtual ~QueueInterface() {};

    /**
     * Possible status for running jobs
     * @sa getStatus
     */
    enum QueueStatus {
      /// Something very bizarre has happened
      Unknown = -1,
      /// Job has completed successfully
      Success,
      /// Job finished, but the optimization was unsuccessful
      Error,
      /// Job is queued
      Queued,
      /// Job is current running
      Running,
      /// Communication with a remote server has failed
      CommunicationError,
      /// Job has appeared in queue, but the Structure still returns
      /// Structure::Submitted instead of Structure::InProcess. This
      /// will be corrected in the next iteration of
      /// QueueManager::checkRunning().
      Started,
      /// Job has been submitted, but has not appeared in queue
      Pending
    };

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
    virtual bool isReadyToSearch(QString *err) {*err = ""; return true;}

  public slots:

    /**
     * Read optimizer data from file (.scheme or .state). If called
     * without an argument, this function does nothing, i.e. it will
     * not read optimizer data from the system config file.
     *
     * @param filename Scheme or state file to load data from.
     * @sa writeSettings
     */
    virtual void readSettings(const QString &filename = "") {}

    /**
     * Write optimizer data to file (.scheme or .state). If called
     * without an argument, this function does nothing, i.e. it will
     * not write optimizer data to the system config file.
     *
     * @param filename Scheme or state file to write data to.
     * @sa readSettings
     */
    virtual void writeSettings(const QString &filename = "") {}

    /**
     * Write the input files for Structure \a s to the appropriate
     * location.
     *
     * This function will also construct and write any queue-specific
     * files (e.g. job.pbs for PBS queues) and copy them to the remote
     * host if using a remote queue.
     *
     * @return True on success, false otherwise.
     */
    virtual bool writeInputFiles(Structure *s) const;

    /**
     * Write the provided files in the hash \a files to the local
     * working directory for Structure \a s and (if appropriate) copy
     * them to a remote server.
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
                            const QHash<QString, QString> &files) const =0;

    /**
     * Start a job for Structure \a s.
     *
     * @note Ensure that writeFiles is called before attempting to
     * start the job.
     *
     * @return True on success, false otherwise.
     */
    virtual bool startJob(Structure *s) =0;

    /**
     * Stop any currently running jobs for Structure \a s.
     *
     * @return True on success, false otherwise.
     */
    virtual bool stopJob(Structure *s) =0;

    /**
     * @return The queue status of Structure \a s.
     */
    virtual QueueInterface::QueueStatus getStatus(Structure *s) const =0;

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
    virtual bool prepareForStructureUpdate(Structure *s) const =0;

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
                                   bool *exists) =0;

    /**
     * Retrieve the contents of the file \a filename for Structure \a
     * s as a QString \a contents.
     *
     * @return True on success, false otherwise.
     */
    virtual bool fetchFile(Structure *s,
                           const QString &filename,
                           QString *contents) const =0;

    /**
     * Grep through the file \a filename in Structure \a s's working
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
                          const bool caseSensitive = true) const =0;

    /**
     * @return The name of the queue interface (e.g. "Local", "PBS",
     * etc)
     */
    QString getIDString() const {return m_idString;};

    /**
     * @return The names of all template files associated with this
     * interface.
     */
    QStringList getTemplateFileNames() const {return m_templates;};

    /// \defgroup dialog Dialog access

    /**
     * @return True if this QueueInterface has a configuration dialog.
     * @sa dialog()
     * @ingroup dialog
     */
    bool hasDialog() {return m_hasDialog;};

    /**
     * @return The configuration dialog for this QueueInterface, if it
     * exists, otherwise 0.
     * @sa hasDialog()
     * @ingroup dialog
     */
    virtual QDialog* dialog() {return m_dialog;};

  protected:
    /// Cached pointer to the parent OptBase class
    OptBase *m_opt;

    /// String identifying the type of queue interface
    QString m_idString;

    /// QStringList containing list of all template filenames
    /// @note templates are actually handled in Optimizer
    QStringList m_templates;

    /// Whether this QueueInterface has a configuration dialog.
    bool m_hasDialog;

    /// Pointer to configuration dialog (may be NULL)
    QDialog *m_dialog;

  };
}

#endif

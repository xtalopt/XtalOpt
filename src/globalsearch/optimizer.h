/**********************************************************************
  Optimizer - Generic optimizer interface

  Copyright (C) 2010 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include <globalsearch/optbase.h>

#include <QHash>
#include <QObject>
#include <QVariant>
#include <QStringList>

namespace GlobalSearch {
  class Structure;
  class OptBase;

  /**
   * @class Optimizer optimizer.h <globalsearch/optimizer.h>
   *
   * @brief The Optimizer class provides an interface between an
   * OptBase instance and an external optimization engine.
   *
   * @author David C. Lonie
   *
   * The Optimizer class standardizes communication between an OptBase
   * instance and an external chemical optimization engine, such as
   * GAMESS, ADF, VASP, etc. The optimization can be performed on the
   * local system (see XtalOpt's GULPOptimizer implementation for an
   * example), or on a remote cluster (the default for the Optimizer
   * class).
   *
   * Remote communication is performed by libssh. The default remote
   * job control assumes that the remote system is running a PBS
   * server.
   *
   * To implement a new optimizer that will use a remote PBS server,
   * simply derive Optimizer and implement the constructor:
@verbatim
class MyOptimizer : public GlobalSearch::Optimizer
{
  Q_OBJECT
public:
  MyOptimizer(Optbase *parent, const QString &filename = "") :
    QObject(parent),
    m_opt(parent)
  {
    // Set allowed generic data structure keys, if any, e.g.
    // (uncommon)
    // m_data.insert("Identifier name",QVariant())

    // Set allowed filenames, e.g.
    m_templates.insert("job.pbs",QStringList) // PBS queue script
    m_templates.insert("job.inp",QStringList) // Optimizer input file

    // Setup for completion values
    m_completionFilename = job.out
    m_completionString   = "Optimization completed successfully"

    // Set output filenames to try to read data from, e.g.
    m_outputFilenames.append("job.out");

    // Set the name of the optimizer to be returned by getIDString()
    m_idString = "MyOptimizer";

    readSettings(filename);
   }
}
@endverbatim
   *
   * The simplest usage of an Optimizer is to attach it to an OptBase
   * derived class:
@verbatim
myOptBase->setOptimizer(new MyOptimizer( myOptBase, filename ));
@endverbatim
   *
   * The file contents may be set using the setTemplate,
   * appendTemplate, and removeTemplate functions. These will be
   * passed through the associated OptBase's template interpreter
   * prior to writing.
   *
   * Once the Optimizer is attached to the OptBase instance and the
   * templates are set, the QueueManager handles all job submission,
   * file writing, and Structure updating automatically.
   */
  class Optimizer : public QObject
  {
    Q_OBJECT

  public:
    /**
     * Constructor
     *
     * @param parent OptBase parent
     * @param filename Optional filename to load data from (scheme or
     * resume file)
     */
    explicit Optimizer(OptBase *parent, const QString &filename = "");

    /**
     * Destructor
     */
    virtual ~Optimizer();

    /**
     * Possible status for jobs submitted by the Optimizer
     * @sa getStatus
     */
    enum JobState {
      /// Something very bizarre has happened
      Unknown = -1,
      /// Job has completed successfully
      Success,
      /// Job finished, but the optimization was unsuccessful
      Error,
      /// Job is queued with the PBS server
      Queued,
      /// Optimization is current running
      Running,
      /// Communication with the remote server has failed
      CommunicationError,
      /// Job has been submitted, but has not appeared in queue
      Started,
      /// Job has appeared in queue, but the Structure still returns
      /// Structure::Submitted instead of Structure::InProcess. This
      /// will be corrected by the QueueManager.
      Pending
    };

    /**
     * Read optimizer data from file (.scheme or .state). If called
     * without an argument, this function does nothing, i.e. it will
     * not read optimizer data from the system config file.
     *
     * @param filename Scheme or state file to load data from.
     * @sa writeSettings
     */
    virtual void readSettings(const QString &filename = "");

    /**
     * Write optimizer data to file (.scheme or .state). If called
     * without an argument, this function does nothing, i.e. it will
     * not write optimizer data to the system config file.
     *
     * @param filename Scheme or state file to write data to.
     * @sa readSettings
     */
    virtual void writeSettings(const QString &filename = "");

    /**
     * @return A string identifying the Optimizer
     */
    virtual QString getIDString() {return m_idString;};

    /**
     * @return The total number of optimization steps
     */
    virtual int getNumberOfOptSteps();

    /**
     * Write the input files to the Structure's local path after
     * interpreting the templates.
     *
     * @param structure Structure to generate input files for
     *
     * @return True if write was successful, false otherwise.
     */
    virtual bool writeInputFiles(Structure *structure);

    /**
     * Submit the job to the PBS queue.
     *
     * @param structure Structure to begin optimizing
     *
     * @return True if submission is successful, false otherwise
     */
    virtual bool startOptimization(Structure *structure);

    /**
     * @param filename Name of file to check for on the remote server.
     *
     * @return True if it exists, false otherwise.
     */
    virtual bool checkIfOutputFileExists(const QString & filename);

    /**
     * Retrieve the contents of an output file from the remote server.
     *
     * @param filename Filename to retrieve
     * @param data QStringList to fill with the file's contents, one
     * line per item.
     *
     * @return True if successful, false otherwise.
     */
    virtual bool getOutputFile(const QString & filename, QStringList & data);

    /**
     * Query the remote server for the contents of the PBS queue. The
     * results are filtered to contain only the entries submitted by
     * OptBase::username.
     *
     * @param queueData QStringList to fill with the contents of the
     * queue.
     *
     * @return True if query is successful, false otherwise
     */
    virtual bool getQueueList(QStringList & queueData);

    /**
     * Call OptBase::qdel on the Structure's job ID.
     *
     * @param structure Structure to delete from PBS queue
     *
     * @return True if successful, false otherwise.
     */
    virtual bool deleteJob(Structure *structure);

    /**
     * Find out the status of the currently optimizing structure.
     *
     * @note This function will check the cached queue information
     * in OptBase::queue()->getRemoteQueueData(), which may be
     * slightly outdated. If you need absolutely current status, call
     * OptBase::queue()->updateQueue(0) first.
     *
     * @param structure The Structure whose status is to be determined
     *
     * @return The JobState status of the optimization job.
     * @sa JobState
     */
    virtual Optimizer::JobState getStatus(Structure *structure);

    /**
     * Checks the entries in queueData list for the jobname of the
     * structure, which is extracted from structure->fileName() +
     * "job.pbs". If a job with this name is found in queueData, the
     * referenced boolean is set to true and the job ID is returned.
     *
     * @param structure Structure to check
     * @param queueData List containing the queue data.
     * @param exists Boolean to be set to true if the job exists.
     *
     * @return The job ID of the running job, if found. Otherwise, 0.
     */
    virtual int checkIfJobNameExists(Structure *structure, const QStringList &queueData, bool &exists);

    /**
     * Copy the files from the Structure's remote path to the local
     * path, and then update the Structure based on the optimization
     * results.
     *
     * @param structure Structure to be updated.
     *
     * @return True if successful, false otherwise.
     * @sa load
     * @sa read
     */
    virtual bool update(Structure *structure);

    /**
     * Update the Structure from the files in the Structure's local
     * file path.
     *
     * @param structure Structure to be updated.
     *
     * @return True if successful, false otherwise.
     * @sa update
     * @sa read
     */
    virtual bool load(Structure *structure);

    /**
     * Update the Structure from the specified filename.
     *
     * @param structure Structure to be updated
     * @param filename Filename to read
     *
     * @return True if successful, false otherwise.
     * @sa update
     * @sa load
     */
    virtual bool read(Structure *structure, const QString & filename);

    /**
     * Return a specified template.
     *
     * @param filename Filename of template
     * @param optStep Optimization step of template to retrieve.
     *
     * @return The requested template
     */
    virtual QString getTemplate(const QString &filename, int optStep);

    /**
     * Return a list of all templates for a given filename
     *
     * @param filename Filename of template
     *
     * @return A list of all template matching the given filename, in
     * order of optimization step.
     */
    virtual QStringList getTemplate(const QString &filename);

    /**
     * Get generic data associated with the optimizer.
     *
     * @param identifier QString identifying the data needed.
     *
     * @return A QVariant version of the generic data.
     */
    virtual QVariant getData(const QString &identifier);

    /**
     * @return All filenames that the optimizer can store templates
     * for.
     */
    QStringList getTemplateNames() {return m_templates.keys();};

    /**
     * @return All strings that identify valid generic data sets.
     */
    QStringList getDataIdentifiers() {return m_data.keys();};

    /**
     * @return A user customizable string that is used in template
     * interpretation.
     */
    QString getUser1() {return m_user1;};

    /**
     * @return A user customizable string that is used in template
     * interpretation.
     */
    QString getUser2() {return m_user2;};

    /**
     * @return A user customizable string that is used in template
     * interpretation.
     */
    QString getUser3() {return m_user3;};

    /**
     * @return A user customizable string that is used in template
     * interpretation.
     */
    QString getUser4() {return m_user4;};

  public slots:
    /**
     * Set the template for the specified filename and optimization step.
     *
     * @param filename Filename of template
     * @param templateData Template string
     * @param optStep Optimization step
     *
     * @return True if successful, false otherwise.
     */
    virtual bool setTemplate(const QString &filename,
                             const QString &templateData,
                             int optStep);

    /**
     * Set all templates for the specified filename.
     *
     * @param filename Filename of template
     * @param templateData List of template strings in order of
     * optimization step
     *
     * @return True if successful, false otherwise.
     */
    virtual bool setTemplate(const QString &filename,
                             const QStringList &templateData);

    /**
     * Add a new optimization step for a given filename.
     *
     * @param filename Filename identifying the template
     * @param templateData Template string of the new optimization
     * step.
     *
     * @return True if successful, false otherwise.
     */
    virtual bool appendTemplate(const QString &filename,
                                const QString &templateData);

    /**
     * Remove an optimization step from a filename's templates.
     *
     * @param filename Filename of interest
     * @param optStep Optimization step to remove
     *
     * @return True if successful, false otherwise.
     */
    virtual bool removeTemplate(const QString &filename,
                                int optStep);

    /**
     * Set a generic data entry.
     *
     * @param identifier A valid data identifier
     * @param data QVariant of data
     *
     * @return True if successful, false otherwise.
     * @sa getData
     * @sa getDataIdentifiers
     */
    virtual bool setData(const QString &identifier,
                         const QVariant &data);

    /**
     * @param s A user customizable string that is used in template
     * interpretation.
     */
    void setUser1(const QString &s) {m_user1 = s;};

    /**
     * @param s A user customizable string that is used in template
     * interpretation.
     */
    void setUser2(const QString &s) {m_user2 = s;};

    /**
     * @param s A user customizable string that is used in template
     * interpretation.
     */
    void setUser3(const QString &s) {m_user3 = s;};

    /**
     * @param s A user customizable string that is used in template
     * interpretation.
     */
    void setUser4(const QString &s) {m_user4 = s;};

  protected:
    /**
     * Create the structure's remote working directory.
     *
     * @return True if successful, false otherwise.
     */
    virtual bool createRemoteDirectory(Structure *structure);

    /**
     * Clean all files from the structure's remote working directory.
     *
     * @return True if successful, false otherwise.
     */
    virtual bool cleanRemoteDirectory(Structure *structure);

    /**
     * Interpret and write all templates in m_templates at the
     * Structure's current optimization step.
     *
     * @param s Structure of interest
     *
     * @return True if successful, false otherwise.
     */
    virtual bool writeTemplates(Structure *s);

    /**
     * @param filename Scheme or state file from which to load all
     * templates in m_templates
     */
    virtual void readTemplatesFromSettings(const QString &filename = "");

    /**
     * @param filename Scheme or state file in which to write all
     * templates in m_templates
     */
    virtual void writeTemplatesToSettings(const QString &filename = "");

    /**
     * @param filename Scheme or state file from which to load all
     * user values (m_user[1-4])
     */
    virtual void readUserValuesFromSettings(const QString &filename = "");

    /**
     * @param filename Scheme or state file in which to write all
     * user values (m_user[1-4])
     */
    virtual void writeUserValuesToSettings(const QString &filename = "");

    /**
     * @param filename Scheme or state file from which to load all
     * generic data values in m_data
     */
    virtual void readDataFromSettings(const QString &filename = "");

    /**
     * @param filename Scheme or state file in which to write all
     * generic data values in m_data
     */
    virtual void writeDataToSettings(const QString &filename = "");

    /**
     * Copies all filenames in getTemplateNames() from the Structure's
     * local path to its remote path.
     *
     * @param structure Structure of interest
     *
     * @return True if successful, false otherwise.
     */
    virtual bool copyLocalTemplateFilesToRemote(Structure *structure);

    /**
     * Copies the contents of the Structure's remote path to its local
     * path.
     *
     * @param structure Structure of interest
     *
     * @return True if successful, false otherwise.
     */
    virtual bool copyRemoteToLocalCache(Structure *structure);

    /**
     * Store generic data types. This is not commonly used, see
     * XtalOpt's VASPOptimizer for an example where it is used to
     * store information about pseudopotentials.
     */
    QHash<QString, QVariant> m_data;

    /**
     * Stores all template data. Key is the filename to be written and
     * the value is a list of corresponding templates in order of
     * optimization step.
     */
    QHash<QString, QStringList > m_templates;

    /**
     * File to check if optimization has complete successfully.
     * @sa m_completionString
     */
    QString m_completionFilename;

    /**
     * String to search for in m_completionFilename when checking if
     * optimization has complete successfully.
     * @sa m_completionFilename.
     */
    QString m_completionString;

    /**
     * List of filenames to check when updating structure (will be
     * checked in order of index).
     */
    QStringList m_outputFilenames;

    /**
     * User defined string that is used during template
     * interpretation.
     */
    QString m_user1;

    /**
     * User defined string that is used during template
     * interpretation.
     */
    QString m_user2;

    /**
     * User defined string that is used during template
     * interpretation.
     */
    QString m_user3;

    /**
     * User defined string that is used during template
     * interpretation.
     */
    QString m_user4;

    /**
     * Cached pointer to the associated OptBase instance
     */
    OptBase *m_opt;

    /**
     * Unique identification string for this Optimizer.
     */
    QString m_idString;
  };

} // end namespace Avogadro

#endif

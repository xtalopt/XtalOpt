/**********************************************************************
  Optimizer - Generic optimizer interface

  Copyright (C) 2010-2011 by David C. Lonie

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
#include <globalsearch/queueinterface.h>

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtCore/QStringList>

namespace GlobalSearch {
  class Structure;
  class OptimizerConfigDialog;

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
   * GAMESS, ADF, VASP, etc.
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
    friend class OptimizerConfigDialog;

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

    /**
     * Check if the file m_completionFilename exists in the working
     * directory of Structure \a s and store the result in \a exists.
     *
     * @note This function uses the argument \a exists to report
     * whether or not the file exists. The return value indicates
     * whether the file check was performed without errors
     * (e.g. network errors).
     *
     * @return True if the test encountered no errors, false otherwise.
     */
    virtual bool checkIfOutputFileExists(Structure *s, bool *exists);

    /**
     * Check m_completionFilename for any of the m_completionStrings
     * in the working directory of Structure \a s. If any are found,
     * \a success is set to true.
     *
     * @note This function uses the argument \a success to report
     * whether or not m_completionFileName contains any
     * m_completionStrings. The return value indicates whether the
     * file check was performed without errors (e.g. network errors).
     *
     * @return True if the test encountered no errors, false otherwise.
     */
    virtual bool checkForSuccessfulOutput(Structure *s, bool *success);

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
     * @param optStepIndex  Optimization step index of template to retrieve.
     *
     * @return The requested template
     */
    virtual QString getTemplate(const QString &filename, int optStepIndex);

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
    virtual QStringList getTemplateNames() {return m_templates.keys();};

    /**
     * @return All strings that identify valid generic data sets.
     */
    virtual QStringList getDataIdentifiers() {return m_data.keys();};

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
     * @param optStepIndex Optimization step (index, starts at 0)
     *
     * @return True if successful, false otherwise.
     */
    virtual bool setTemplate(const QString &filename,
                             const QString &templateData,
                             int optStepIndex);

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
     * Removes all template entries for the given optstep.
     *
     * @note This function will remove the template for the current
     * Optimizer and QueueInterface, but will not modify any "data"
     * entries in the Optimizer.
     *
     * @param optStepIndex Optimization step index to remove
     *
     * @return True if successful, false otherwise.
     */
    virtual bool removeAllTemplatesForOptStep(int optStepIndex);

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

    /**
     * Command line used in local execution
     *
     * Details given in m_localRunCommand.
     *
     * @sa localRunArgs
     * @sa stdinFilename
     * @sa stdoutFilename
     * @sa stderrFilename
     * @sa m_localRunCommand
     * @sa m_localRunArgs
     * @sa m_stdinFilename
     * @sa m_stdoutFilename
     * @sa m_stderrFilename
     */
    QString localRunCommand() const {return m_localRunCommand;};

    /**
     * Command line arguments used in local execution
     *
     * Details given in m_localRunCommand.
     *
     * @sa stdinFilename
     * @sa stdoutFilename
     * @sa stderrFilename
     * @sa m_localRunCommand
     * @sa m_stdinFilename
     * @sa m_stdoutFilename
     * @sa m_stderrFilename
     */
    QStringList localRunArgs() const {return m_localRunArgs;};

    /**
     * Filename for standard input
     *
     * Details given in m_localRunCommand.
     *
     * @sa localRunCommand
     * @sa stdoutFilename
     * @sa stderrFilename
     * @sa m_localRunCommand
     * @sa m_stdinFilename
     * @sa m_stdoutFilename
     * @sa m_stderrFilename
     */
    QString stdinFilename() const {return m_stdinFilename;};

    /**
     * Filename for standard output
     *
     * Details given in m_localRunCommand.
     *
     * @sa localRunCommand
     * @sa stdinFilename
     * @sa stderrFilename
     * @sa m_localRunCommand
     * @sa m_stdinFilename
     * @sa m_stdoutFilename
     * @sa m_stderrFilename
     */
    QString stdoutFilename() const {return m_stdoutFilename;};

    /**
     * Filename for standard error
     *
     * Details given in m_localRunCommand.
     *
     * @sa localRunCommand
     * @sa stdinFilename
     * @sa stdoutFilename
     * @sa m_localRunCommand
     * @sa m_stdinFilename
     * @sa m_stdoutFilename
     * @sa m_stderrFilename
     */
    QString stderrFilename() const {return m_stderrFilename;};

    /**
     * Interpret all templates in m_templates at the Structure's
     * current optimization step.
     *
     * @param s Structure of interest
     *
     * @return A hash containing the interpreted templates, key:
     * filename, value: file contents
     */
    virtual QHash<QString, QString> getInterpretedTemplates(Structure *s);

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
    virtual QDialog* dialog();

  protected slots:
    /**
     * Update the m_QITemplates hash.
     *
     * Automatically connected to m_opt's queueInterfaceChanged
     * signal. Should not need to be called directly.
     */
    virtual void updateQueueInterface();

  protected:
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
     * Store generic data types. This is not commonly used, see
     * XtalOpt's VASPOptimizer for an example where it is used to
     * store information about pseudopotentials.
     */
    QHash<QString, QVariant> m_data;

    /**
     * Determine which internal template hash contains \a filename and
     * return a reference to the correct hash.
     */
    QHash<QString, QStringList>& resolveTemplateHash(const QString &filename);

    /**
     * @overload
     * Determine which internal template hash contains \a filename and
     * return a reference to the correct hash.
     */
    const QHash<QString, QStringList>& resolveTemplateHash(const QString &filename) const;

    /**
     * Ensure that all template lists in m_templates and m_QITemplates
     * contain getNumberOfOptSteps() optimization steps.
     *
     * If a template list has too few entries, empty strings are
     * appended.
     */
    virtual void fixTemplateLengths();

    /**
     * Stores all template data for this optimizer. Key is the
     * filename to be written and the value is a list of corresponding
     * templates in order of optimization step.
     */
    QHash<QString, QStringList > m_templates;

    /**
     * Stores all template data for the current QueueInterface. Key is
     * the filename to be written and the value is a list of
     * corresponding templates in order of optimization step.
     */
    QHash<QString, QStringList > m_QITemplates;

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
    QStringList m_completionStrings;

    /**
     * List of filenames to check when updating structure (will be
     * checked in order of index).
     */
    QStringList m_outputFilenames;

    /**
     * Commandline instruction to run this program locally
     *
     * Three common scenarios:
     *
     * VASP-esque: $ vasp
     *  Runs in working directory reading from predefined input
     *  filenames (ie. POSCAR). Set m_localRunCommand="vasp",
     *   m_localRunArgs empty, and
     *  m_stdinFilename=m_stdoutFilename=m_stderrFilename="";
     *
     * GULP-esque: $ gulp < job.gin 1>job.got 2>job.err
     *
     *  Runs in working directory using redirection to specify
     *  input/output. Set m_localRunCommand="gulp",
     *  m_localRunArgs empty,
     *  m_stdinFilename="job.gin", m_stdoutFilename="job.got",
     *  m_stderrFilename="job.err".
     *
     * MOPAC-esque: $ mopac job
     *
     *  Runs in working directory, specifying either an input filename
     *  or a base name. Put the executable name (and path, if needed)
     *  into m_localRunCommand="mopac", and args into m_localRunArgs[0]="job",
     *  m_stdinFilename=m_stdoutFilename=m_stderrFilename=""
     *
     * Stdin/out/err is not used (="") by default.
     *
     * @sa localRunCommand
     * @sa localRunArgs
     * @sa stdinFilename
     * @sa stdoutFilename
     * @sa stderrFilename
     * @sa m_stdinFilename
     * @sa m_stdoutFilename
     * @sa m_stderrFilename
     */
    QString m_localRunCommand;

    /**
     * Filename for standard input
     *
     * Details given in m_localRunCommand.
     *
     * @sa localRunCommand
     * @sa stdinFilename
     * @sa stdoutFilename
     * @sa stderrFilename
     * @sa m_localRunCommand
     * @sa m_stdoutFilename
     * @sa m_stderrFilename
     */
    QStringList m_localRunArgs;

    /**
     * Filename for standard input
     *
     * Details given in m_localRunCommand.
     *
     * @sa localRunCommand
     * @sa stdinFilename
     * @sa stdoutFilename
     * @sa stderrFilename
     * @sa m_localRunCommand
     * @sa m_stdoutFilename
     * @sa m_stderrFilename
     */
    QString m_stdinFilename;

    /**
     * Filename for standard output
     *
     * Details given in m_localRunCommand.
     *
     * @sa localRunCommand
     * @sa stdinFilename
     * @sa stdoutFilename
     * @sa stderrFilename
     * @sa m_localRunCommand
     * @sa m_stdinFilename
     * @sa m_stderrFilename
     */
    QString m_stdoutFilename;

    /**
     * Filename for standard error
     *
     * Details given in m_localRunCommand.
     *
     * @sa localRunCommand
     * @sa stdinFilename
     * @sa stdoutFilename
     * @sa stderrFilename
     * @sa m_localRunCommand
     * @sa m_stdinFilename
     * @sa m_stdoutFilename
     */
    QString m_stderrFilename;

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

    /// @cond
    bool m_hasDialog;
    QDialog *m_dialog;
    /// @endcond
  };
} // end namespace GlobalSearch

#endif

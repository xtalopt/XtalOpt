/**********************************************************************
  Optimizer - Generic optimizer interface

  Copyright (C) 2010-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include <QHash>
#include <QObject>
#include <QStringList>
#include <QVariant>

class QDialog;

namespace GlobalSearch {
class OptBase;
class OptimizerConfigDialog;
class Structure;

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

  QHash<QString, QVariant> m_data;
  /**
   * Constructor
   *
   * @param parent OptBase parent
   * @param filename Optional filename to load data from (scheme or
   * resume file)
   */
  explicit Optimizer(OptBase* parent, const QString& filename = "");

  /**
   * Destructor
   */
  virtual ~Optimizer() override;

  /**
   * Read optimizer data from file (.scheme or .state). If called
   * without an argument, this function does nothing, i.e. it will
   * not read optimizer data from the system config file.
   *
   * @param filename Scheme or state file to load data from.
   * @sa writeSettings
   */
  virtual void readSettings(const QString& filename = "");

  /**
   * Write optimizer data to file (.scheme or .state). If called
   * without an argument, this function does nothing, i.e. it will
   * not write optimizer data to the system config file.
   *
   * @param filename Scheme or state file to write data to.
   * @sa readSettings
   */
  virtual void writeSettings(const QString& filename = "");

  /**
   * @return A string identifying the Optimizer
   */
  virtual QString getIDString() const { return m_idString; };

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
  virtual bool isReadyToSearch(QString* err)
  {
    *err = "";
    return true;
  }

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
  virtual bool checkIfOutputFileExists(Structure* s, bool* exists);

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
  virtual bool checkForSuccessfulOutput(Structure* s, bool* success);

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
  virtual bool update(Structure* structure);

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
  virtual bool load(Structure* structure);

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
  virtual bool read(Structure* structure, const QString& filename);

  /**
   * Get generic data associated with the optimizer.
   *
   * @param identifier QString identifying the data needed.
   *
   * @return A QVariant version of the generic data.
   */
  virtual QVariant getData(const QString& identifier);

  /**
   * @return All filenames that the optimizer can store templates
   * for.
   */
  virtual QStringList getTemplateFileNames() const { return m_templates; }

  /**
   * Check if a name is a template file name.
   *
   * @param name The template file name.
   *
   * @return True if "name" is a template file name. False otherwise.
   */
  bool isTemplateFileName(const char* name) const
  {
    return m_templates.contains(name);
  }

  /**
   * Get a QHash of the interpreted templates.
   *
   * @param s The structure whose templates are to be interpreted.
   *
   * @return A QHash of the template filename to its contents.
   */
  virtual QHash<QString, QString> getInterpretedTemplates(Structure* s);

  /**
   * @return All strings that identify valid generic data sets.
   */
  virtual QStringList getDataIdentifiers() { return m_data.keys(); };

public slots:

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
  virtual bool setData(const QString& identifier, const QVariant& data);

  /**
   * Command line used in local execution
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
  QString localRunCommand() const { return m_localRunCommand; };

  /**
   * Set the local run command.
   *
   * Details given in m_localRunCommand.
   */
  void setLocalRunCommand(const QString& s) { m_localRunCommand = s; }

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
  QString stdinFilename() const { return m_stdinFilename; };

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
  QString stdoutFilename() const { return m_stdoutFilename; };

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
  QString stderrFilename() const { return m_stderrFilename; };

  /// \defgroup dialog Dialog access

  /**
   * @return True if this QueueInterface has a configuration dialog.
   * @sa dialog()
   * @ingroup dialog
   */
  bool hasDialog() { return m_hasDialog; };

  /**
   * @return The configuration dialog for this QueueInterface, if it
   * exists, otherwise 0.
   * @sa hasDialog()
   * @ingroup dialog
   */
  virtual QDialog* dialog();

protected:
  /**
   * @param filename Scheme or state file from which to load all
   * generic data values in m_data
   */
  virtual void readDataFromSettings(const QString& filename = "");

  /**
   * @param filename Scheme or state file in which to write all
   * generic data values in m_data
   */
  virtual void writeDataToSettings(const QString& filename = "");

  /**
   * Store generic data types. This is not commonly used, see
   * XtalOpt's VASPOptimizer for an example where it is used to
   * store information about pseudopotentials.
   */
  // QHash<QString, QVariant> m_data;

  /**
   * The names of templates that this optimizer utilizes.
   */
  QStringList m_templates;

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
   *  filenames (ie. POSCAR). Set m_localRunCommand="vasp", and
   *  m_stdinFilename=m_stdoutFilename=m_stderrFilename="";
   *
   * GULP-esque: $ gulp < job.gin 1>job.got 2>job.err
   *
   *  Runs in working directory using redirection to specify
   *  input/output. Set m_localRunCommand="gulp",
   *  m_stdinFilename="job.gin", m_stdoutFilename="job.got",
   *  m_stderrFilename="job.err".
   *
   * MOPAC-esque: $ mopac job
   *
   *  Runs in working directory, specifying either an input filename
   *  or a base name. In both cases, put the entire command line
   *  into m_localRunCommand, ="mopac job",
   *  m_stdinFilename=m_stdoutFilename=m_stderrFilename=""
   *
   * Stdin/out/err is not used (="") by default.
   *
   * @sa localRunCommand
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
   * Cached pointer to the associated OptBase instance
   */
  OptBase* m_opt;

  /**
   * Unique identification string for this Optimizer.
   */
  QString m_idString;

  /// @cond
  bool m_hasDialog;
  QDialog* m_dialog;
  /// @endcond
};
} // end namespace GlobalSearch

#endif

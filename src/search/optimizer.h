/**********************************************************************
  Optimizer - Local optimizers' interface

  Copyright (C) 2010-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

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
#include <QReadLocker>
#include <QReadWriteLock>
#include <QStringList>
#include <QWriteLocker>

#include <functional>
#include <memory>

namespace Search {
class SearchBase;
class Structure;

/// Default values for an optimizer: its name, files, completion check, and commands.
struct OptimizerDefaults
{
  const char* idString;             // e.g. "VASP"
  const char* const* templateFiles; // user templates, e.g. { "INCAR", ... }
  const char* const* inputAssets;   // engine-written assets, e.g. { "POTCAR" }
  const char* completionFilename;   // file to check for successful completion
  const char* completionString;     // string to find in completionFilename
  const char* const* outputFiles;   // files to read on update (in order)
  const char* directRunCommand;     // default direct-run command
  const char* stdinFilename;        // direct-run stdin ("" if unused)
  const char* stdoutFilename;       // direct-run stdout ("" if unused)
  const char* stderrFilename;       // direct-run stderr ("" if unused)
};

/**
 * @class Optimizer optimizer.h <search/optimizer.h>
 *
 * @brief Interface between SearchBase and an external optimizer.
 *
 * @author David C. Lonie
 *
 * An Optimizer knows its template and output files, how to check a finished
 * job, and how to read the optimized structure and energy. Built-in optimizers
 * live in search/optimizers/ and provide their own defaults(). Do not derive a new
 * built-in optimizer directly from this class.
 *
 * SearchBase keeps one set of templates for each optimization step and
 * interprets them before writing. Optimizer input files are written without
 * interpretation. QueueManager and QueueInterface run the job; Optimizer reads
 * its results.
 */
class Optimizer : public QObject
{
  Q_OBJECT

public:
  /**
   * Register an optimizer under the given name. This is called once per
   *   optimizer type, typically from the application's initialization code.
   * The registration order determines the order returned by
   *   registeredOptimizers(), which, for example, determines the order in a
   *   GUI menu.
   */
  static bool registerOptimizer(const QString& name,
    std::function<std::unique_ptr<Optimizer>(SearchBase*)> creator);

  /** @return Names of all registered optimizers, in registration order. */
  static QStringList registeredOptimizers();

  /** @return Names of all engine-provided optimizers, in built-in order. */
  static QStringList availableBuiltInOptimizers();

  /** Register one external-code optimizer provided by the engine. */
  static bool registerBuiltInOptimizer(const QString& name);

  /**
   * Constructor
   *
   * @param parent The search this optimizer belongs to
   */
  explicit Optimizer(SearchBase* parent);

  /**
   * Destructor
   */
  virtual ~Optimizer() override;

  /**
   * @return A string identifying the Optimizer
   */
  // Built-in optimizers report the id from their default; a custom optimizer (e.g.
  //   a test mock) with no default may override this.
  virtual QString getIDString() const
  {
    return m_optimizerDefaults ? QString::fromLatin1(m_optimizerDefaults->idString) : QString();
  };

  /**
   * Check if the completion file exists in the working directory of Structure
   * \a s and store the result in \a exists.
   *
   * @note This function uses the argument \a exists to report
   * whether or not the file exists. The return value indicates
   * whether the file check was performed without errors
   * (e.g. network errors).
   *
   * @return True if the test encountered no errors, false otherwise.
   */
  bool checkIfOutputFileExists(Structure* s, bool* exists);

  /**
   * Check the completion file for the completion string in the working
   * directory of Structure \a s. If it is found, \a success is set to true.
   *
   * @note This function uses the argument \a success to report
   * whether or not the completion file contains the completion string.
   * The return value indicates whether the file check was performed
   * without errors (e.g. network errors).
   *
   * @return True if the test encountered no errors, false otherwise.
   */
  bool checkForSuccessfulOutput(Structure* s, bool* success);

  /**
   * Copy the files from the Structure's remote path to the local
   * path, and then update the Structure based on the optimization
   * results.
   *
   * @param structure Structure to be updated.
   *
   * @return True if successful, false otherwise.
   * @sa read
   */
  virtual bool update(Structure* structure);

  /**
   * Update the Structure from the specified filename.
   *
   * @param structure Structure to be updated
   * @param filename Filename to read
   *
   * @return True if successful, false otherwise.
   * @sa update
   */
  bool read(Structure* structure, const QString& filename);

  /**
   * @return All filenames that the optimizer can store templates
   * for.
   */
  QStringList getOptimizerTemplateFileNames() const;

  /**
   * @return The optimizer's input asset names; these are not user templates.
   */
  QStringList getOptimizerInputAssetNames() const;

  /**
   * Get the input files to write for a structure.
   *
   * Template files are interpreted through the SearchBase keyword system.
   * Optimizer input assets are added without template interpretation.
   *
   * @param s The structure whose input files are to be generated.
   *
   * @return A QHash of the filename to its contents. An empty hash
   * means failure.
   */
  QHash<QString, QString> getInputFiles(Structure* s);

public slots:

  /**
   * Command line used by the direct-run queue interface.
   *
   * @sa stdinFilename
   * @sa stdoutFilename
   * @sa stderrFilename
   */
  QString getDirectRunCommand() const
  {
    QReadLocker locker(&m_commandLock);
    return !m_directRunOverride.isEmpty()
             ? m_directRunOverride
             : (m_optimizerDefaults ? QString::fromLatin1(m_optimizerDefaults->directRunCommand) : QString());
  };

  /**
   * Set the direct-run command. This overrides the optimizer default until
   * cleared; the standard-IO filenames are part of the optimizer definition.
   */
  void setDirectRunCommand(const QString& command)
  {
    QWriteLocker locker(&m_commandLock);
    m_directRunOverride = command;
  }

  /**
   * Filename for standard input (direct runs).
   */
  QString stdinFilename() const
  {
    return m_optimizerDefaults ? QString::fromLatin1(m_optimizerDefaults->stdinFilename) : QString();
  };

  /**
   * Filename for standard output (direct runs).
   */
  QString stdoutFilename() const
  {
    return m_optimizerDefaults ? QString::fromLatin1(m_optimizerDefaults->stdoutFilename) : QString();
  };

  /**
   * Filename for standard error (direct runs).
   */
  QString stderrFilename() const
  {
    return m_optimizerDefaults ? QString::fromLatin1(m_optimizerDefaults->stderrFilename) : QString();
  };

protected:
  /**
   * Add the optimizer-specific input files (e.g. VASP's POSCAR/POTCAR, SIESTA's
   * per-species PSF files) to @p files for opt step @p optStep. The base
   * optimizer has no optimizer-specific inputs. Return false on failure.
   */
  virtual bool addOptimizerInputFiles(Structure* s, int optStep,
                                      QHash<QString, QString>& files) const;

  /**
   * Read the optimized geometry and energy from @p filename into @p s. Each
   * optimizer subclass parses its own output format. The base optimizer has no
   * reader and returns false.
   */
  virtual bool readOutput(Structure* s, const QString& filename) const;

  // Read one optimizer input asset file. Returns false if the file cannot be read.
  static bool readInputAssetFile(const QString& filename, QString& contents);

  /// Per-optimizer defaults (id, templates, assets, completion, outputs,
  ///   command); each built-in subclass sets this from its own defaults() in its
  ///   ctor. Null until set (e.g. a custom optimizer that overrides getIDString
  ///   and the relevant accessors instead).
  const OptimizerDefaults* m_optimizerDefaults = nullptr;

  /// Cached pointer to the associated SearchBase instance
  SearchBase* m_search;

private:
  friend class SearchBase;

  /**
   * Create an optimizer by its registered name.
   * @return Owning optimizer pointer, or nullptr if name is unknown.
   */
  static std::unique_ptr<Optimizer> createRegisteredOptimizer(
    const QString& name, SearchBase* parent);

  /// User override for the direct-run command. Empty means "use the optimizer
  ///   default" (see getDirectRunCommand()).
  QString m_directRunOverride;
  mutable QReadWriteLock m_commandLock;
};
} // end namespace Search

#endif

/**********************************************************************
  cliOptions.h - Static options class for command-line interface for XtalOpt.

  Copyright (C) 2017 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#ifndef XTALOPT_CLI_OPTIONS_H
#define XTALOPT_CLI_OPTIONS_H

// Forward declarations
class QString;
namespace XtalOpt {
class XtalOpt;
class XtalOptOptimizer;
}
namespace GlobalSearch {
class QueueInterface;
}

namespace XtalOpt {

class XtalOptCLIOptions {
 public:

  /**
   * Reads options from a specified file and sets the options in the
   * XtalOpt object.
   *
   * @param filename The name of the file to be read.
   * @param xtalopt The XtalOpt object whose options are to be set.
   *
   * @return True if all the required options were set. False otherwise.
   */
  static bool readOptions(const QString& filename, XtalOpt& xtalopt);

 private:
  /**
   * Checks to see if s is a valid keyword. If it is, it will then
   * set csString to be the same keyword with the proper cases for each letter
   * (for example, "empiricalformula" will be "empiricalFormula").
   *
   * @param s The keyword to check.
   * @parm csString The string to set to the properly-cased keyword.
   *
   * @return True if it is a valid keyword. False if it is not.
   */
  static bool isKeyword(const QString& s, QString& csString);

  /**
   * Does this string start with 'potcarFile' or 'psfFile'?
   *
   * @param s The string to be check.
   * @return True if it is a potential file. False otherwise.
   */
  static bool isPotFile(const QString& s);

  /**
   * Reads options from a single line and sets it in the options object if
   * and only if the option on that line is a valid keyword.
   *
   * @param line The line to be read.
   * @param options The hash containing the set options.
   */
  static void processLine(const QString& line,
                          QHash<QString, QString>& options);

  /**
   * Checks the options QHash to make sure the required options were set.
   *
   * @param options The options to check.
   *
   * @return True if the required options were set. False otherwise.
   */
  static bool requiredOptionsSet(const QHash<QString, QString>& options);

  /**
   * Takes the options QHash and sets all the options in the XtalOpt object.
   * May fail and return false if the required options were not set.
   *
   * @param options The options QHash.
   * @param xtalopt The XtalOpt object to be set.
   *
   * @return True if the options were set successfully. False otherwise.
   */
  static bool processOptions(const QHash<QString, QString>& options,
                             XtalOpt& xtalopt);

  /**
   * Prints all the options manually set in the options QHash, then
   * prints all the options as they are seen in the XtalOpt object.
   *
   * @param options The manual options to be printed.
   * @param xtalopt The XtalOpt object whose settings are to be printed.
   */
  static void printOptions(const QHash<QString, QString>& options,
                           const XtalOpt& xtalopt);

  /**
   * Convert a Qstring to a boolean
   *
   * @param s The QString to be converted.
   *
   * @return The boolean.
   */
  static bool toBool(const QString& s);

  /**
   * Convert a boolean to a QString
   *
   * @param b The boolean to be converted.
   *
   * @return Returns "true" if b and "false" if not b.
   */
  static QString toString(bool b);

  /**
   * Convert a QString to a QStringList that is split by commas
   * while skipping the empty parts. Each QString will then be trimmed
   * before the function returns.
   *
   * @param s The QString to be converted.
   *
   * @return The QStringList.
   */
  static QStringList toList(const QString& s);

  /**
   * Reads the 'templateName' from the options QHash, splits the
   * list of files into separate files, reads the files and adds the
   * templates to the optimizers.
   *
   * @param templateName The name of the template to add to the optimizer. This
   *                     should be a set option in the options object.
   * @param options The options QHash containing the value for the
   *                template name and the templatesDirectory.
   * @param numOptSteps The number of optimization steps.
   * @param optimizer The optimizer for which to add the template.
   * @param queue The queue (used for 'jobTemplates' to determine queue
   *              template name).
   *
   * @return True on success and false on failure. An error message will
   *         be printed with qDebug() if it fails.
   */
  static bool addOptimizerTemplates(const QString& templateName,
                                    const QHash<QString, QString>& options,
                                    size_t numOptSteps,
                                    XtalOptOptimizer& optimizer,
                                    GlobalSearch::QueueInterface& queue);
};

} // end namespace XtalOpt

#endif

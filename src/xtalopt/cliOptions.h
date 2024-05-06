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

class XtalOptCLIOptions
{
public:

  /**
   * Return a qstring containing a brief header for the code
   */
  static QString xtaloptHeaderString();

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

  /**
   * Read runtime options from the runtime file and write them to the @p
   * xtalopt object. Does nothing if the file does not exist or cannot be
   * opened. The filename is retrieved by calling XtalOpt::CLIRuntimeFile().
   *
   * @param xtalopt The XtalOpt object for which to set the options.
   */
  static void readRuntimeOptions(XtalOpt& xtalopt);

  /**
   * Write the initial runtime options file to @param filename.
   * This file will be read during runtime, and XtalOpt will update
   * its settings based upon the file. The file name is obtained via
   * XtalOpt::CLIRuntimeFile().
   *
   * @param xtalopt The XtalOpt object whose runtime settings wil be
   *                written to the file.
   */
  static void writeInitialRuntimeFile(XtalOpt& xtalopt);

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
   * @param s The string to be checked.
   * @return True if it is a potential file. False otherwise.
   */
  static bool isPotFile(const QString& s);

  /**
   * Does this string start with 'molecularUnits'?
   *
   * @param s The string to be checked.
   * @return True if it is a molecular units line. False otherwise.
   */
  static bool isMolecularUnitsLine(const QString& s);

  /**
   * Does this string start with 'customIAD'?
   *
   * @param s The string to be checked.
   * @return True if it is a custom IAD line. False otherwise.
   */
  static bool isCustomIADLine(const QString& s);

  /**
   * Does this entry involve multiple lines? Two examples are pot file lines
   * and molecular units lines.
   *
   * @param s The string to be checked.
   * @return True if it involves multiple lines. False otherwise.
   */
  static bool isMultiLineEntry(const QString& s);

  /**
   * Reads options from a single line and sets it in the options object if
   * and only if the option on that line is a valid keyword.
   *
   * @param line The line to be read.
   * @param options The hash containing the set options.
   * @param xtalopt The xtalopt object (this is needed for processing objective-related stuff)
   */
  static void processLine(const QString& line,
                          QHash<QString, QString>& options, XtalOpt& xtalopt);

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
   *
   * @return False if a critical error has occured. True otherwise.
   */
  static bool printOptions(const QHash<QString, QString>& options,
                           XtalOpt& xtalopt);

  /**
   * Convert a boolean to a QString
   *
   * @param s The boolean to be converted.
   *
   * @return The QString.
   */
  static QString fromBool(bool b);

  /**
   * Convert a Qstring to a boolean
   *
   * @param s The QString to be converted.
   *
   * @return The boolean.
   */
  static bool toBool(const QString& s);

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
   * Reads the 'templateName' from the options QHash, reads the resulting
   * file name and sets it into XtalOpt.
   *
   * @param xtalopt The XtalOpt object for which to set the template.
   * @param templateName The name of the template to add to the optimizer. This
   *                     should be a set option in the options object.
   * @param queueName The queue name for the current step.
   * @param optSteps The opt step for which to add the template.
   * @param options The options QHash containing the value for the
   *                template name and the templatesDirectory.
   *
   * @return True on success and false on failure. An error message will
   *         be printed with qDebug() if it fails.
   */
  static bool addOptimizerTemplate(XtalOpt& xtalopt,
                                   const QString& templateName,
                                   const QString& queueName, size_t optStep,
                                   const QHash<QString, QString>& options);
  /**
   * Checks the xtalopt settings (@p xtalopt) to see if the mitosis settings
   * are fine. Returns true if they are. Returns false if they are not.
   * The following constitute bad mitosis settings:
   * mitosisA * mitosisB * mitosisC != mitosisDivisions
   * mitosisDivisions > minFU * smallestNumAtomsOfOneType
   *
   * @param xtalopt The XtalOpt object to check.
   *
   * @return True if mitosis settings are okay, and false if they are not.
   */
  static bool isMitosisOk(XtalOpt& xtalopt);

  /**
   * Reads the molecular unit options from @p options and attempts to set them
   * to the xtalopt object. Returns true if it succeeds and false if it fails.
   *
   * @param options The options to be set read.
   * @param xtalopt The xtalopt object for which to set the options.
   *
   * @return True if the mol units were processed successfully. False if
   *         the mol units were not.
   */
  static bool processMolUnits(const QHash<QString, QString>& options,
                              XtalOpt& xtalopt);

  /**
   * Reads the customIAD options from @p options and attempts to set them
   * to the xtalopt object. Returns true if it succeeds and false if it fails.
   *
   * @param options The options to be set read.
   * @param xtalopt The xtalopt object for which to set the options.
   *
   * @return True if the custom IAD options were processed successfully. False
   *         if the custom IAD options were not.
   */
  static bool processCustomIADs(const QHash<QString, QString>& options,
                                XtalOpt& xtalopt);

  /**
   * Reads and sets runtime options from @p options to @p xtalopt.
   * This gets called by readRuntimeOptions().
   *
   * @param options The options to be set.
   * @param xtalopt The XtalOpt object for which to set the options.
   */
  static void processRuntimeOptions(const QHash<QString, QString>& options,
                                    XtalOpt& xtalopt);
};

} // end namespace XtalOpt

#endif

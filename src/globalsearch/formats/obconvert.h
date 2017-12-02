/**********************************************************************
  OBConvert -- Use an Open Babel "obabel" executable to convert file types

  Copyright (C) 2017 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef GLOBALSEARCH_OBCONVERT_H
#define GLOBALSEARCH_OBCONVERT_H

#include <QStringList>

// Forward declarations
class QByteArray;
class QString;

namespace GlobalSearch {

/**
 * @class OBConvert. Uses QProcess and an Open Babel (obabel) executable
 *                   to convert from one format to another.
 */
class OBConvert
{
public:
  /**
   * Convert a file from one format to another. Calls executeOBabel().
   * Read the description for executeOBabel() to see how the obabel
   * executable is found and used.
   *
   * @param inFormat The input format.
   * @param outFormat The output format.
   * @param input The input to be sent to obabel through stdin.
   * @param output The stdout output that obabel returns.
   * @param options Any additional options you wish to pass to obabel.
   *
   * @return Returns true if executeOBabel() ran successfully, and
   *         false if it did not.
   */
  static bool convertFormat(const QString& inFormat, const QString& outFormat,
                            const QByteArray& input, QByteArray& output,
                            const QStringList& options = QStringList());

  /**
   * Execute obabel and get output. Looks for obabel in three places:
   *  1. The OBABEL_EXECUTABLE environment variable. If this is set, it will
   *     be used for the obabel executable.
   *  2. The application directory path. If obabel is found in there, it
   *     will be used.
   *  3. The application directory path + "/../bin/". If obabel is found
   *     there it will be used.
   *
   *  If obabel is not found in any of these places, an error will be printed
   *  and the function will return false.
   *
   * @param args The arguments to be used with the obabel executable.
   * @param input The input to be sent through stdin to obabel.
   * @param output The stdout output that obabel returns.
   *
   * @return True if it ran without any errors. False if the executable
   *         wasn't found or obabel returned an error.
   */
  static bool executeOBabel(const QStringList& args, const QByteArray& input,
                            QByteArray& output);
};
}

#endif // GLOBALSEARCH_OBCONVERT_H

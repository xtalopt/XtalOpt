/**********************************************************************
  Formats -- A wrapper to access the formats.

  Copyright (C) 2016 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <QString>

namespace GlobalSearch {

class Structure;

class Formats
{
public:
  /**
   * Detect the format from the filename extension and return it as a
   * QString. Returns an empty QString if no format was detected.
   *
   * @param filename The filename whose extension is to be examined.
   *
   * @return The format. Returns an empty QString if no format was found.
   */
  static QString detectFormat(const QString& filename);

  /**
   * Try to detect the format and then read the structure. If it fails
   * to detect the format, nothing will be done to @p s, and the return
   * will be false.
   *
   * @param s The structure to be written to. Nothing will be done if
   *          the format is not found correctly.
   * @param filename The filename which is to be read and whose extension
   *                 is to be examined.
   *
   * @return True on success and false on failure.
   */
  static bool read(Structure* s, const QString& filename);

  /**
   * Attempts to read the @p filename with the given @p format, and writes
   * the information to the Structure, @p s. Returns true on succes
   * and false on failure.
   *
   * @param s The structure to be written to. Nothing will be done if
   *          an invalid format is given.
   * @param filename The name of the file to be read.
   * @param format The format to be used. If an invalid format is given,
   *               nothing will be done to @p s and false will be returned.
   *
   * @return True on success and false on failure.
   */
  static bool read(Structure* s, const QString& filename,
                   const QString& format);
};
}

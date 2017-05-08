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

#include <globalsearch/structure.h>
#include <globalsearch/utilities/utilityfunctions.h>

#include <globalsearch/formats/formats.h>
#include <globalsearch/formats/gulpformat.h>
#include <globalsearch/formats/siestaformat.h>
#include <globalsearch/formats/vaspformat.h>

#include <QDebug>
#include <QString>

#include <map>

using std::make_pair;
using std::pair;
using std::string;
using std::vector;

// The list of possible formats
static const vector<string> _formats =
{
  "GULP",
  "VASP",
  "SIESTA"
};

// The map of the formats and their extensions
static const vector<pair<string, string>> _formatExtensions =
{
  make_pair("got", "GULP"),
  make_pair("gout", "GULP")
};

namespace GlobalSearch {

  QString Formats::detectFormat(const QString& filename)
  {
    // First, check for POSCAR or CONTCAR at the end of the filename. If it
    // exists, then the it is VASP format.
    if (filename.endsWith("POSCAR") || filename.endsWith("CONTCAR"))
      return "VASP";
    // Otherwise, find the extension.
    string ext = getFileExt(filename.toStdString());
    for (size_t i = 0; i < _formatExtensions.size(); ++i) {
      if (caseInsensitiveCompare(_formatExtensions[i].first, ext))
        return QString(_formatExtensions[i].second.c_str());
    }
    qDebug() << "Failed to detect format for: " << filename << "!";
    return QString();
  }

  bool Formats::read(Structure* s, const QString& filename)
  {
    QString format = detectFormat(filename);
    if (format.size() == 0)
      return false;
    return read(s, filename, format);
  }

  bool Formats::read(Structure* s, const QString& filename,
                     const QString& format)
  {
    // List the formats here
    if (format == QString("GULP"))
      return GulpFormat::read(s, filename);

    if (format == QString("SIESTA"))
      return SiestaFormat::read(s, filename);

    if (format == QString("VASP"))
      return VaspFormat::read(s, filename);

    qDebug() << "An invalid format, " << format << ", entered into "
             << "Format::read() !";
    return false;
  }
}

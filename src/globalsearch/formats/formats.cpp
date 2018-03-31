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

#include <globalsearch/formats/castepformat.h>
#include <globalsearch/formats/cmlformat.h>
#include <globalsearch/formats/formats.h>
#include <globalsearch/formats/genericformat.h>
#include <globalsearch/formats/gulpformat.h>
#include <globalsearch/formats/poscarformat.h>
#include <globalsearch/formats/pwscfformat.h>
#include <globalsearch/formats/siestaformat.h>
#include <globalsearch/formats/vaspformat.h>
#include <globalsearch/formats/xyzformat.h>
#include <globalsearch/formats/zmatrixformat.h>

#include <QDebug>
#include <QString>

#include <fstream>
#include <map>

using std::make_pair;
using std::pair;
using std::string;
using std::vector;

// The list of possible formats
static const vector<string> _formats = { "CASTEP", "CML",   "GULP",
                                         "POSCAR", "PWSCF", "SIESTA",
                                         "VASP",   "XYZ",   "ZMATRIX" };

// The map of the formats and their extensions
static const vector<pair<string, string>> _formatExtensions = {
  make_pair("castep", "CASTEP"), make_pair("cml", "CML"),
  make_pair("got", "GULP"), make_pair("gout", "GULP"),
  make_pair("xyz", "XYZ")
};

namespace GlobalSearch {

QString Formats::detectFormat(const QString& filename)
{
  // First, check for POSCAR or CONTCAR at the end of the filename. If it
  // exists, then the it is POSCAR format.
  if (filename.endsWith("POSCAR") || filename.endsWith("CONTCAR"))
    return "POSCAR";
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

bool Formats::read(Structure* s, const QString& filename, const QString& format)
{
  // List the formats here
  if (format.toUpper() == QString("CASTEP"))
    return CastepFormat::read(s, filename);

  if (format.toUpper() == QString("CML")) {
    std::ifstream in(filename.toStdString().c_str());
    if (!in.is_open()) {
      qDebug() << "Failed to open CML file: " << filename;
      return false;
    }
    return CmlFormat::read(*s, in);
  }

  if (format.toUpper() == QString("GULP"))
    return GulpFormat::read(s, filename);

  if (format.toUpper() == QString("POSCAR")) {
    std::ifstream in(filename.toStdString().c_str());
    if (!in.is_open()) {
      qDebug() << "Failed to open POSCAR file: " << filename;
      return false;
    }
    return PoscarFormat::read(*s, in);
  }

  if (format.toUpper() == QString("PWSCF"))
    return PwscfFormat::read(s, filename);

  if (format.toUpper() == QString("SIESTA"))
    return SiestaFormat::read(s, filename);

  if (format.toUpper() == QString("VASP"))
    return VaspFormat::read(s, filename);

  if (format.toUpper() == QString("XYZ"))
    return XyzFormat::read(s, filename);

  if (format.toUpper() == QString("ZMATRIX")) {
    std::ifstream in(filename.toStdString().c_str());
    if (!in.is_open()) {
      qDebug() << "Failed to open ZMatrix file: " << filename;
      return false;
    }
    return ZMatrixFormat::read(s, in);
  }

  // If we made it to the end, try the generic format reader
  std::ifstream in(filename.toStdString().c_str());
  if (!in.is_open()) {
    qDebug() << "Failed to open generic file: " << filename;
    return false;
  }
  return GenericFormat::read(*s, in, format.toStdString());
}
}

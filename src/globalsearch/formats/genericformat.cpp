/**********************************************************************
  GenericFormat -- A simple reader involving Open Babel for generic formats.

  Copyright (C) 2018 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include "genericformat.h"

#include <globalsearch/formats/cmlformat.h>
#include <globalsearch/formats/obconvert.h>

#include <QDebug>
#include <QString>

#include <sstream>

namespace GlobalSearch {

bool GenericFormat::read(Structure& s, std::istream& in,
                         const std::string& formatName)
{
  QString format(formatName.c_str());

  // If it is empty or "generic", change it to "out"
  if (format.isEmpty() || format.toLower() == "generic")
    format = "out";

  // Read the whole file
  std::string data(std::istreambuf_iterator<char>(in), {});

  // We need the "-xp" flag to make sure the energy gets written
  QStringList options;
  options << "-xp";

  // Use OBConvert to get it in CML format
  QByteArray output;
  if (!OBConvert::convertFormat(format, "cml", data.c_str(), output, options)) {
    qDebug() << "Failed to convert generic output file to cml with Open Babel";
    return false;
  }

  // Convert to stringstream
  std::stringstream cmlData(output.constData());

  // Read the CML file
  if (!CmlFormat::read(s, cmlData)) {
    qDebug() << "Failed to read generic output file";
    return false;
  }

  // We are done!
  return true;
}
}

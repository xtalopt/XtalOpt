/**********************************************************************
  Formats - Geometry format detection and import utilities.

  Copyright (C) 2016 by Patrick Avery
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <atoms/formats/formats.h>

#include <atoms/geometry.h>
#include <atoms/formats/castepformat.h>
#include <atoms/formats/cifformat.h>
#include <atoms/formats/cmlformat.h>
#include <atoms/formats/gulpformat.h>
#include <atoms/formats/mtpformat.h>
#include <atoms/formats/poscarformat.h>
#include <atoms/formats/pwscfformat.h>
#include <atoms/formats/siestaformat.h>
#include <atoms/formats/xyzformat.h>

#include <common/compatibility/qt_compat.h>
#include <common/fileutils.h>
#include <common/output.h>

#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include <sstream>

namespace {

enum ReadKind
{
  ReadUnknown = 0,
  ReadStructure,
  ReadOptimizerOutput
};

struct DetectedFormat
{
  DetectedFormat() : kind(ReadUnknown) {}
  DetectedFormat(const QString& format_, ReadKind kind_)
    : format(format_), kind(kind_) {}

  bool isValid() const { return !format.isEmpty() && kind != ReadUnknown; }

  QString format;
  ReadKind kind;
};

bool isNumber(const QString& text)
{
  bool ok = false;
  text.toDouble(&ok);
  return ok;
}

bool lineHasNumbers(const QString& line, int count)
{
  const QStringList fields = line.split(" ", QtCompat::SkipEmptyParts);
  if (fields.size() < count)
    return false;
  for (int i = 0; i < count; ++i) {
    if (!isNumber(fields.at(i)))
      return false;
  }
  return true;
}

bool looksLikePoscar(const QStringList& lines)
{
  if (lines.size() < 7)
    return false;
  if (!isNumber(lines.at(1).trimmed()))
    return false;
  return lineHasNumbers(lines.at(2), 3) && lineHasNumbers(lines.at(3), 3) &&
         lineHasNumbers(lines.at(4), 3);
}

bool looksLikeXyz(const QStringList& lines)
{
  if (lines.size() < 3)
    return false;
  bool ok = false;
  const int atoms = lines.first().trimmed().toInt(&ok);
  if (!ok || atoms <= 0 || lines.size() < atoms + 2)
    return false;

  for (int i = 2; i < lines.size() && i < atoms + 2; ++i) {
    const QStringList fields = lines.at(i).split(" ", QtCompat::SkipEmptyParts);
    if (fields.size() < 4 || !fields.at(0).at(0).isLetter())
      return false;
    if (!isNumber(fields.at(1)) || !isNumber(fields.at(2)) || !isNumber(fields.at(3)))
      return false;
  }
  return true;
}

DetectedFormat detectFormatFromContents(const QString& filename)
{
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return DetectedFormat();

  QTextStream in(&file);
  QStringList lines;
  QString text;
  const int maxLines = 300;
  for (int i = 0; i < maxLines && !in.atEnd(); ++i) {
    const QString line = in.readLine();
    if (!line.trimmed().isEmpty())
      lines.append(line.trimmed());
    text += line + "\n";
  }

  if (!in.atEnd()) {
    const qint64 tailBytes = 256 * 1024;
    const qint64 tailStart = qMax<qint64>(0, file.size() - tailBytes);
    if (in.seek(tailStart)) {
      if (tailStart > 0)
        in.readLine();
      while (!in.atEnd())
        text += in.readLine() + "\n";
    }
  }

  const QString lower = text.toLower();

  if (looksLikePoscar(lines))
    return DetectedFormat("POSCAR", ReadStructure);

  if (lower.contains("_cell_length_") && lower.contains("_atom_site") && lower.contains("loop_"))
    return DetectedFormat("CIF", ReadStructure);

  if (lower.contains("<molecule") || lower.contains("<cml") || lower.contains("xml-cml.org"))
    return DetectedFormat("CML", ReadStructure);

  if (looksLikeXyz(lines))
    return DetectedFormat("XYZ", ReadStructure);

  if (text.contains("Final Configuration") && text.contains("Real Lattice(A)") &&
      text.contains("Cell Contents"))
    return DetectedFormat("CASTEP", ReadOptimizerOutput);

  if (text.contains("Begin final coordinates") && text.contains("ATOMIC_POSITIONS") &&
      (text.contains("CELL_PARAMETERS") || text.contains("!    total energy")))
    return DetectedFormat("PWSCF", ReadOptimizerOutput);

  if (text.contains("outcell: Unit cell vectors") && text.contains("outcoor:"))
    return DetectedFormat("SIESTA", ReadOptimizerOutput);

  if (text.contains("GENERAL UTILITY LATTICE PROGRAM") &&
      text.contains("Final fractional coordinates of atoms"))
    return DetectedFormat("GULP", ReadOptimizerOutput);

  if (lower.contains("&control") && lower.contains("&system") && lower.contains("atomic_species") &&
      lower.contains("atomic_positions"))
    return DetectedFormat("PWSCF", ReadStructure);

  if (lower.contains("cell_parameters") && lower.contains("atomic_positions"))
    return DetectedFormat("PWSCF", ReadStructure);

  if ((lower.contains("systemlabel") || lower.contains("chemicalspecieslabel")) &&
      lower.contains("atomiccoordinates"))
    return DetectedFormat("SIESTA", ReadStructure);

  if ((lower.contains("%block lattice_cart") || lower.contains("%block lattice_abc")) &&
      (lower.contains("%block positions_frac") || lower.contains("%block positions_abs")))
    return DetectedFormat("CASTEP", ReadStructure);

  if (text.contains("AtomData") && text.contains("Supercell") && text.contains("Size"))
    return DetectedFormat("MTP", ReadStructure);

  return DetectedFormat();
}

DetectedFormat detectFormatFromFilename(const QString& filename)
{
  const QFileInfo info(filename);
  const QString file = info.fileName().toUpper();
  const QString ext = info.suffix().toLower();

  if (file == "POSCAR" || file == "CONTCAR")
    return DetectedFormat("POSCAR", ReadStructure);

  if (ext == "poscar" || ext == "contcar" || ext == "vasp")
    return DetectedFormat("POSCAR", ReadStructure);
  if (ext == "cif")
    return DetectedFormat("CIF", ReadStructure);
  if (ext == "cml")
    return DetectedFormat("CML", ReadStructure);
  if (ext == "xyz")
    return DetectedFormat("XYZ", ReadStructure);
  if (ext == "cfg" || ext == "mtp")
    return DetectedFormat("MTP", ReadStructure);
  if (ext == "mot")
    return DetectedFormat("MTP", ReadOptimizerOutput);
  if (ext == "cell")
    return DetectedFormat("CASTEP", ReadStructure);
  if (ext == "castep")
    return DetectedFormat("CASTEP", ReadOptimizerOutput);
  if (ext == "fdf")
    return DetectedFormat("SIESTA", ReadStructure);
  if (ext == "got" || ext == "gout")
    return DetectedFormat("GULP", ReadOptimizerOutput);
  if (ext == "pwscf")
    return DetectedFormat("PWSCF", ReadOptimizerOutput);
  if (ext == "siesta")
    return DetectedFormat("SIESTA", ReadOptimizerOutput);

  return DetectedFormat();
}

DetectedFormat detectFormat(const QString& filename)
{
  DetectedFormat detected = detectFormatFromContents(filename);
  if (detected.isValid())
    return detected;
  return detectFormatFromFilename(filename);
}

bool readInputText(const QString& filename, const QString& format, std::string& text)
{
  if (Common::readFileToString(filename, &text))
    return true;
  Common::error(QString("Failed to open %1 file: %2").arg(format).arg(filename));
  return false;
}

} // namespace

namespace Atoms {

bool Formats::read(Atoms::Geometry* s, const QString& filename)
{
  if (!s) {
    Common::error("Cannot read structure into a null Geometry pointer.");
    return false;
  }

  const DetectedFormat detected = detectFormat(filename);
  if (!detected.isValid()) {
    Common::error(QString("Failed to detect structure format for: %1!")
                 .arg(filename));
    return false;
  }

  Atoms::Geometry parsed;
  bool ok = false;

  if (detected.format == "CASTEP") {
    if (detected.kind == ReadOptimizerOutput)
      ok = CastepFormat::readOutput(&parsed, filename);
    else
      ok = CastepFormat::read(&parsed, filename);
  } else if (detected.format == "CIF") {
    ok = CifFormat::read(&parsed, filename);
  } else if (detected.format == "CML") {
    std::string text;
    if (!readInputText(filename, detected.format, text))
      return false;
    std::istringstream in(text);
    ok = CmlFormat::read(parsed, in);
  } else if (detected.format == "GULP") {
    ok = GulpFormat::readOutput(&parsed, filename);
  } else if (detected.format == "MTP") {
    if (detected.kind == ReadOptimizerOutput)
      ok = MtpFormat::readOutput(&parsed, filename);
    else
      ok = MtpFormat::read(&parsed, filename);
  } else if (detected.format == "POSCAR") {
    std::string text;
    if (!readInputText(filename, detected.format, text))
      return false;
    std::istringstream in(text);
    ok = PoscarFormat::read(parsed, in);
  } else if (detected.format == "PWSCF") {
    if (detected.kind == ReadOptimizerOutput)
      ok = PwscfFormat::readOutput(&parsed, filename);
    else
      ok = PwscfFormat::read(&parsed, filename);
  } else if (detected.format == "SIESTA") {
    if (detected.kind == ReadOptimizerOutput)
      ok = SiestaFormat::readOutput(&parsed, filename);
    else
      ok = SiestaFormat::read(&parsed, filename);
  } else if (detected.format == "XYZ") {
    ok = XyzFormat::read(&parsed, filename);
  } else {
    Common::error(QString("Unsupported structure format: %1 for file: %2")
                 .arg(detected.format).arg(filename));
    return false;
  }

  if (!ok)
    return false;
  if (parsed.is3D() && !Atoms::Geometry::isCellMatrixUsable(parsed.unitCell().cellMatrix())) {
    Common::error(QString("The cell read from %1 is invalid.").arg(filename));
    return false;
  }

  *s = parsed;
  return true;
}

} // namespace Atoms

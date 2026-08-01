/**********************************************************************
  MtpFormat - Handlers for CFG format structure files (MTP code).

  Copyright (C) 2025 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <atoms/formats/mtpformat.h>

#include <atoms/eleminfo.h>
#include <common/fileutils.h>
#include <common/output.h>
#include <common/stringutils.h>
#include <atoms/geometry.h>

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <ostream>
#include <sstream>

#include <QString>

namespace Atoms {

namespace {

// Shared CFG reader for MTP input and output files (the format is the
//   same); "kind" only is used for clear messaging. A file may hold a
//   series of configurations; the last complete block is read.
bool readCfgFile(Atoms::Geometry* s, const QString& filename, const QString& kind)
{
  std::string text;
  if (!Common::readFileToString(filename, &text)) {
    Common::error(QString("MTP %1, %2, could not be opened!")
                 .arg(kind)
                 .arg(filename));
    return false;
  }

  const size_t begin = text.rfind("BEGIN_CFG");
  const size_t end =
    (begin != std::string::npos) ? text.find("END_CFG", begin) : std::string::npos;
  if (begin == std::string::npos || end == std::string::npos) {
    Common::error(QString("MTP %1 does not contain a complete CFG block.").arg(kind));
    return false;
  }
  // Parse only the selected block.
  std::istringstream ifs(text.substr(begin, end - begin));

  bool coordsFound = false, cellFound = false;
  bool fractionalCoord = true;

  int numAtoms = 0;
  QList<int> atomicTypes;
  QList<unsigned int> atomicNums;
  QList<QString> chemicalSystem;
  QList<Common::Vector3> coords;
  Common::Matrix3 cellMatrix = Common::Matrix3::Zero();

  std::string line;
  std::vector<std::string> lineSplit;
  while (getline(ifs, line)) {
    if (strstr(line.c_str(), "Size")) {
      // Get the number of atoms
      getline(ifs, line);
      lineSplit = Common::split(line, ' ');
      if (lineSplit.size() != 1 || !Common::parseIntString(lineSplit[0], numAtoms)) {
        Common::error(QString("Could not read the number of atoms in MTP %1! %2")
                     .arg(kind)
                     .arg(line.c_str()));
        return false;
      }
    } else if (strstr(line.c_str(), "Supercell")) {
      // Get the cell matrix
      for (size_t i = 0; i < 3; ++i) {
        getline(ifs, line);
        lineSplit = Common::split(line, ' ');
        if (lineSplit.size() != 3 || !Common::parseDoubleString(lineSplit[0], cellMatrix(i, 0)) ||
            !Common::parseDoubleString(lineSplit[1], cellMatrix(i, 1)) ||
            !Common::parseDoubleString(lineSplit[2], cellMatrix(i, 2))) {
          Common::error(QString("Could not read the cell matrix in MTP %1! %2")
                       .arg(kind)
                       .arg(line.c_str()));
          return false;
        }
      }
      cellFound = true;
    } else if (strstr(line.c_str(), "AtomData")) {
      // Get the atomic coordinates
      if (strstr(line.c_str(), "cartes"))
        fractionalCoord = false;
      for (int i = 0; i < numAtoms; i++) {
        getline(ifs, line);
        lineSplit = Common::split(line, ' ');
        int atomicType;
        double x, y, z;
        if (lineSplit.size() < 5 || !Common::parseIntString(lineSplit[1], atomicType) ||
            !Common::parseDoubleString(lineSplit[2], x) ||
            !Common::parseDoubleString(lineSplit[3], y) ||
            !Common::parseDoubleString(lineSplit[4], z)) {
          Common::error(QString("Incomplete coords line in MTP %1: %2")
                       .arg(kind)
                       .arg(line.c_str()));
          return false;
        }
        atomicTypes.append(atomicType);
        coords.append(Common::Vector3(x, y, z));
      }
      coordsFound = true;
    } else if (strstr(line.c_str(), "chemical_system")) {
      // Get the full chemical system
      // MTP strangely replaces "space" with "tab" in the "Feature"
      //   lines. We should fix this first before reading symbols.
      for (size_t i = 0; i < line.length(); ++i) {
        if (line[i] == '\t') {
          line.replace(i, 1, std::string(1, ' '));
        }
      }
      lineSplit = Common::split(line, ' ');
      if (lineSplit.size() < 3) {
        Common::error(QString("Could not read the chemical system in MTP %1! %2")
                     .arg(kind)
                     .arg(line.c_str()));
        return false;
      }
      for (int i = 2; i < static_cast<int>(lineSplit.size()); i++) {
        chemicalSystem.append(lineSplit[i].c_str());
      }
    }
  }

  if (!cellFound)
    Common::error(QString("Cell info was not found in MTP %1!").arg(kind));
  if (!coordsFound)
    Common::error(QString("Atom coords not found in MTP %1!").arg(kind));
  if (!cellFound || !coordsFound)
    return false;
  if (atomicTypes.isEmpty()) {
    Common::error(QString("No atomic types were found in MTP %1!").arg(kind));
    return false;
  }

  // This is important: we need to properly extract the atomic types.
  int min = *std::min_element(atomicTypes.begin(), atomicTypes.end());
  int max = *std::max_element(atomicTypes.begin(), atomicTypes.end());
  if (min < 0 || max >= chemicalSystem.size()) {
    Common::error(QString("Failed to read atomic types from MTP %1!").arg(kind));
    return false;
  }
  // Produce the list of atomic numbers
  for (int i = 0; i < atomicTypes.size(); i++) {
    QString sym = chemicalSystem[atomicTypes[i]];
    const unsigned int atomicNum = Atoms::ElementInfo::getAtomicNum(sym.toStdString());
    if (atomicNum == 0) {
      Common::error(QString("Unrecognized element symbol in MTP %1: %2")
                   .arg(kind)
                   .arg(sym));
      return false;
    }
    atomicNums.append(atomicNum);
  }

  // Convert coords to Cartesian, if needed
  Atoms::UnitCell uc(cellMatrix);
  if (fractionalCoord) {
    for (int i = 0; i < coords.size(); ++i)
      coords[i] = uc.toCartesian(coords[i]);
  }

  std::vector<Atoms::Atom> atoms;
  atoms.reserve(coords.size());
  for (int i = 0; i < coords.size(); ++i) {
    atoms.push_back(Atoms::Atom(static_cast<unsigned short>(atomicNums.at(i)), coords.at(i)));
  }

  s->clear();
  s->setUnitCell(uc);
  s->setAtoms(atoms);
  return true;
}

} // anonymous namespace

bool MtpFormat::readOutput(Atoms::Geometry* s, const QString& filename)
{
  return readCfgFile(s, filename, "output");
}

bool MtpFormat::read(Atoms::Geometry* s, const QString& filename)
{
  return readCfgFile(s, filename, "input");
}

bool MtpFormat::write(const Atoms::Geometry& s, std::ostream& out)
{
  if (!s.is3D()) {
    Common::error("MTP input writer requires a periodic structure.");
    return false;
  }

  const QList<QString> symbols = s.getSymbols();
  out << "BEGIN_CFG\n";
  out << "Size\n";
  out << "    " << s.numAtoms() << "\n";
  out << "Supercell\n";
  out << std::fixed << std::setprecision(8);
  for (int i = 0; i < 3; ++i) {
    out << "    "
        << std::setw(16) << s.unitCell().cellMatrix()(i, 0)
        << std::setw(16) << s.unitCell().cellMatrix()(i, 1)
        << std::setw(16) << s.unitCell().cellMatrix()(i, 2) << "\n";
  }
  out << "AtomData: id type cartes_x cartes_y cartes_z\n";
  for (size_t i = 0; i < s.numAtoms(); ++i) {
    const QString symbol = QString::fromStdString(
      Atoms::ElementInfo::getAtomicSymbol(s.atom(i).atomicNumber()));
    const int typeIndex = symbols.indexOf(symbol);
    if (typeIndex < 0) {
      Common::error("MTP input writer failed to map atom type.");
      return false;
    }
    out << "    "
        << std::setw(8) << i + 1
        << std::setw(6) << typeIndex
        << std::setw(16) << s.atom(i).pos().x()
        << std::setw(16) << s.atom(i).pos().y()
        << std::setw(16) << s.atom(i).pos().z() << "\n";
  }
  out << "Feature chemical_system";
  for (const QString& symbol : symbols)
    out << " " << symbol.toStdString();
  out << "\n";
  out << "END_CFG\n";

  return true;
}

} // namespace Atoms

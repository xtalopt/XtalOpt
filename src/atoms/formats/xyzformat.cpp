/**********************************************************************
  XyzFormat - Handlers for XYZ format structure files.

  Copyright (C) 2017 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <atoms/formats/xyzformat.h>

#include <atoms/eleminfo.h>
#include <common/fileutils.h>
#include <common/output.h>
#include <common/stringutils.h>
#include <atoms/geometry.h>

#include <iomanip>
#include <ostream>
#include <sstream>

#include <array>
#include <cctype>
#include <string>
#include <vector>
#include <QString>

namespace Atoms {

// Helper structure/functions to read "Lattice" entry from the comment line
struct LatticeInfo
{
  enum Type
  {
    None,
    Vectors,
    Parameters
  };

  Type type;

  std::array<std::array<double, 3>, 3> vectors;
  std::array<double, 6> parameters;

  LatticeInfo()
    : type(None)
  {
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        vectors[i][j] = 0.0;
      }
    }

    for (int i = 0; i < 6; ++i) {
      parameters[i] = 0.0;
    }
  }
};

std::vector<double> parseNumbers(const std::string& text)
{
  std::vector<double> values;
  std::istringstream iss(text);

  double x;
  while (iss >> x) {
    values.push_back(x);
  }

  return values;
}

bool extractQuotedValue(const std::string& line, const std::string& key, std::string& value)
{
  std::string::size_type start = 0;
  while (start < line.size()) {
    const std::string::size_type keyPos = line.find(key, start);
    if (keyPos == std::string::npos)
      return false;

    const bool leftIsClear = keyPos == 0 ||
      (!std::isalnum(static_cast<unsigned char>(line[keyPos - 1])) &&
       line[keyPos - 1] != '_');
    std::string::size_type pos = keyPos + key.size();
    while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos])))
      ++pos;
    if (leftIsClear && pos < line.size() && line[pos] == '=') {
      ++pos;
      while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos])))
        ++pos;
      if (pos < line.size() && line[pos] == '"') {
        const std::string::size_type end = line.find('"', pos + 1);
        if (end == std::string::npos)
          return false;
        value = line.substr(pos + 1, end - pos - 1);
        return true;
      }
    }
    start = keyPos + key.size();
  }

  return false;
}

LatticeInfo parseLatticeFromXYZComment(const std::string& line)
{
  LatticeInfo info;

  std::string latticeText;

  if (!extractQuotedValue(line, "Lattice", latticeText)) {
    return info;
  }

  std::vector<double> nums = parseNumbers(latticeText);

  if (nums.size() == 9) {
    info.type = LatticeInfo::Vectors;

    info.vectors[0][0] = nums[0];
    info.vectors[0][1] = nums[1];
    info.vectors[0][2] = nums[2];

    info.vectors[1][0] = nums[3];
    info.vectors[1][1] = nums[4];
    info.vectors[1][2] = nums[5];

    info.vectors[2][0] = nums[6];
    info.vectors[2][1] = nums[7];
    info.vectors[2][2] = nums[8];
  }
  else if (nums.size() == 6) {
    info.type = LatticeInfo::Parameters;

    for (int i = 0; i < 6; ++i) {
      info.parameters[i] = nums[i];
    }
  }

  return info;
}


bool XyzFormat::read(Atoms::Geometry* s, const QString& filename)
{
  std::string text;
  if (!Common::readFileToString(filename, &text)) {
    Common::error(QString("XYZ file, %1, could not be opened!").arg(filename));
    return false;
  }
  std::istringstream ifs(text);

  std::string line;
  std::vector<std::string> lineSplit;
  std::vector<Atoms::Atom> atoms;
  LatticeInfo lattice;
  bool frameFound = false;

  // An XYZ file may hold a series of frames; the last complete frame is read.
  for (;;) {
    // Find the next frame's atom-count line; the series ends at the file end.
    lineSplit.clear();
    while (getline(ifs, line)) {
      lineSplit = Common::split(Common::reduce(line, " ", " \t\r"), ' ');
      if (!lineSplit.empty())
        break;
    }
    if (lineSplit.empty())
      break;

    if (lineSplit.size() != 1) {
      Common::error("XYZ reader: the line containing the number of "
                   "atoms must only contain one word: the number of atoms!");
      Common::error(QString("Problem line: %1").arg(line.c_str()));
      return false;
    }

    size_t numAtoms = 0;
    if (!Common::parseSizeString(lineSplit[0], numAtoms)) {
      Common::error("XYZ reader: the number of atoms must be a " "non-negative integer.");
      Common::error(QString("Problem line: %1").arg(line.c_str()));
      return false;
    }

    // Next line is a comment line for molecules; but may contain Lattice for extended XYZ formats
    if (!getline(ifs, line)) {
      Common::error("XYZ reader: the end of file was reached before " "the comment line was read.");
      return false;
    }

    lattice = parseLatticeFromXYZComment(line);

    // Now, let's read a number of times equal to numAtoms!
    atoms.clear();
    for (size_t i = 0; i < numAtoms; ++i) {
      if (!getline(ifs, line)) {
        Common::error(QString("XYZ reader: the end of file was reached "
                             "before we finished reading the number of atoms: %1!")
                     .arg(numAtoms));
        return false;
      }

      lineSplit = Common::split(Common::reduce(line, " ", " \t\r"), ' ');
      if (lineSplit.size() < 4) {
        Common::error("XYZ reader: each atom coordinate line should "
                      "contain at least 4 words: <element> <x> <y> <z>");
        Common::error(QString("Problem line: %1").arg(line.c_str()));
        return false;
      }

      // If it is a number, it is the atomic number. Otherwise, a symbol
      int atomicNum = 0;
      if (Common::parseIntString(lineSplit[0], atomicNum)) {
        if (atomicNum < 1 || atomicNum > 117) {
          Common::error("XYZ reader: invalid atomic number entered.");
          Common::error(QString("Problem line: %1")
                       .arg(line.c_str()));
          return false;
        }
      } else {
        atomicNum = Atoms::ElementInfo::getAtomicNum(lineSplit[0]);
        if (atomicNum < 1 || atomicNum > 117) {
          Common::error("XYZ reader: invalid atomic symbol entered.");
          Common::error(QString("Problem line: %1")
                       .arg(line.c_str()));
          return false;
        }
      }
      double x = 0.0;
      double y = 0.0;
      double z = 0.0;
      if (!Common::parseDoubleString(lineSplit[1], x) ||
          !Common::parseDoubleString(lineSplit[2], y) ||
          !Common::parseDoubleString(lineSplit[3], z)) {
        Common::error("XYZ reader: invalid atomic coordinate entered.");
        Common::error(QString("Problem line: %1").arg(line.c_str()));
        return false;
      }
      Atoms::Atom newAtom(atomicNum, Common::Vector3(x, y, z));
      atoms.push_back(newAtom);
    }
    frameFound = true;
  }

  if (!frameFound) {
    Common::error("XYZ reader: file is empty.");
    return false;
  }

  Atoms::Geometry parsed;

  // If we have found a lattice, set it here.
  if (lattice.type == LatticeInfo::Vectors) {
    Common::Matrix3 cellMatrix = Common::Matrix3::Zero();

    for (int i = 0; i < 3; ++i) {
      cellMatrix(i, 0) = lattice.vectors[i][0];
      cellMatrix(i, 1) = lattice.vectors[i][1];
      cellMatrix(i, 2) = lattice.vectors[i][2];
    }
    Atoms::UnitCell uc(cellMatrix);
    parsed.setUnitCell(uc);
  }
  else if (lattice.type == LatticeInfo::Parameters) {
    parsed.setCellInfo(lattice.parameters[0], lattice.parameters[1], lattice.parameters[2],
                       lattice.parameters[3], lattice.parameters[4], lattice.parameters[5]);
  }

  parsed.setAtoms(atoms);
  parsed.perceiveBonds();
  *s = parsed;

  return true;
}

bool XyzFormat::write(const Atoms::Geometry& s, std::ostream& out)
{
  out << s.numAtoms() << "\n";
  if (s.is3D()) {
    Common::Matrix3 cellMatrix = s.unitCell().cellMatrix();
    out << std::fixed << std::setprecision(8)
        << "Lattice=\""
        << cellMatrix(0,0) << " " << cellMatrix(0,1) << " " << cellMatrix(0,2) << " "
        << cellMatrix(1,0) << " " << cellMatrix(1,1) << " " << cellMatrix(1,2) << " "
        << cellMatrix(2,0) << " " << cellMatrix(2,1) << " " << cellMatrix(2,2)
        << "\"";
  } else {
    out << s.getChemicalFormula().toStdString();
  }
  out << "\n";

  out << std::fixed << std::setprecision(8);
  for (const auto& atom : s.atoms()) {
    out << std::left << std::setw(3)
        << Atoms::ElementInfo::getAtomicSymbol(atom.atomicNumber())
        << std::right
        << std::setw(16) << atom.pos().x()
        << std::setw(16) << atom.pos().y()
        << std::setw(16) << atom.pos().z() << "\n";
  }

  return true;
}
}

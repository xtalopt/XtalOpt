/**********************************************************************
  PwscfFormat - A simple reader for PWSCF output structure.

  Copyright (C) 2017 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <atoms/formats/pwscfformat.h>

#include <common/constants.h>
#include <common/compatibility/qt_compat.h>
#include <common/fileutils.h>
#include <atoms/eleminfo.h>
#include <common/output.h>
#include <common/stringutils.h>
#include <atoms/geometry.h>

#include <cstring>
#include <cmath>
#include <sstream>

#include <QFile>
#include <QString>
#include <QStringList>
#include <QTextStream>

namespace Atoms {

/** The output we are looking for should look something like this:
 *
 *      Final enthalpy =     -94.4276660887 Ry
 * Begin final coordinates
 *      new unit-cell volume =    107.95826 a.u.^3 (    15.99776 Ang^3 )
 *      density =      0.31139 g/cm^3
 *
 * CELL_PARAMETERS (alat=  1.00000000)
 *    3.874174633  -0.256713398   0.047571808
 *    0.960054464   5.090350807   0.072713918
 *    0.694196504   1.552402373   5.434153937
 *
 * ATOMIC_POSITIONS (crystal)
 * O       -0.238428760   0.078189145   0.088947201
 * O        0.736946363   0.004722226   0.509718838
 * O        0.249167697   0.541516629   0.799216961
 * End final coordinates
 *
 * ...
 *
 * !    total energy              =     -95.01479203 Ry
 */

bool PwscfFormat::readOutput(Atoms::Geometry* s, const QString& filename)
{
  std::string text;
  if (!Common::readFileToString(filename, &text)) {
    Common::error(QString("PWSCF output, %1, could not be opened!")
                 .arg(filename));
    return false;
  }
  std::istringstream ifs(text);

  bool coordsFound = false, cellFound = false;
  bool coordsAreFractional = true;

  QList<unsigned int> atomicNums;
  QList<Common::Vector3> coords;
  Common::Matrix3 cellMatrix = Common::Matrix3::Zero();

  std::string line;
  std::vector<std::string> lineSplit;
  double outputAlat = 1.0;
  while (getline(ifs, line)) {
    if (strstr(line.c_str(), "lattice parameter (alat)")) {
      lineSplit = Common::split(line, '=');
      if (lineSplit.size() >= 2) {
        const std::string value = Common::trim(lineSplit[1]);
        const std::vector<std::string> fields = Common::split(value, ' ');
        double parsedAlat = 0.0;
        if (!fields.empty() && Common::parseDoubleString(fields[0], parsedAlat) && parsedAlat > 0.0) {
          outputAlat = parsedAlat;
        }
      }
    }
    // Cell parameters
    // This section should contain the final cell and final coordinates
    if (strstr(line.c_str(), "Begin final coordinates")) {
      bool blockCoordsFound = false;
      bool blockCellFound = false;
      bool blockCoordsAreFractional = false;
      double blockScalingFactor = outputAlat;
      QList<unsigned int> blockAtomicNums;
      QList<Common::Vector3> blockCoords;
      Common::Matrix3 blockCellMatrix = Common::Matrix3::Zero();
      getline(ifs, line);
      while (!strstr(line.c_str(), "End final coordinates")) {
        if (strstr(line.c_str(), "CELL_PARAMETERS")) {
          // Read angstrom, bohr, or alat cell units qualifiers.
          double cellScale = blockScalingFactor / ANG2BOHR;
          if (strstr(line.c_str(), "bohr")) {
            cellScale = 1.0 / ANG2BOHR;
          } else if (strstr(line.c_str(), "angstrom")) {
            cellScale = 1.0;
          } else if (strstr(line.c_str(), "alat")) {
            lineSplit = Common::split(line, '=');

            if (lineSplit.size() < 2) {
              Common::warning("In PWSCF output, alat was not found.");
              Common::warning("Assuming alat to be 1.0");
            } else {
              std::string alatLine = Common::trim(lineSplit[1]);
              Common::replaceAll(alatLine, ")", "");
              if (!Common::parseDoubleString(alatLine, blockScalingFactor)) {
                Common::error(QString("Could not read alat in PWSCF output! %1")
                             .arg(line.c_str()));
                return false;
              }
            }
            cellScale = blockScalingFactor / ANG2BOHR;
          }

          // Get the cell matrix
          for (unsigned short i = 0; i < 3; ++i) {
            getline(ifs, line);
            lineSplit = Common::split(Common::reduce(line), ' ');
            if (lineSplit.size() != 3) {
              Common::error(QString("Could not read the cell matrix in PWSCF output! %1")
                           .arg(line.c_str()));
              return false;
            }
            double v0, v1, v2;
            if (!Common::parseDoubleString(lineSplit[0], v0) ||
                !Common::parseDoubleString(lineSplit[1], v1) ||
                !Common::parseDoubleString(lineSplit[2], v2)) {
              Common::error(QString("Could not read the cell matrix in PWSCF output! %1")
                           .arg(line.c_str()));
              return false;
            }
            blockCellMatrix(i, 0) = v0 * cellScale;
            blockCellMatrix(i, 1) = v1 * cellScale;
            blockCellMatrix(i, 2) = v2 * cellScale;
          }

          blockCellFound = true;
        }
        // Atomic coords
        if (strstr(line.c_str(), "ATOMIC_POSITIONS")) {
          // Bare ATOMIC_POSITIONS uses alat.
          blockCoordsAreFractional = false;
          double posScale = blockScalingFactor / ANG2BOHR;
          if (strstr(line.c_str(), "bohr")) {
            posScale = 1.0 / ANG2BOHR;
          } else if (strstr(line.c_str(), "angstrom")) {
            posScale = 1.0;
          } else if (strstr(line.c_str(), "alat")) {
            posScale = blockScalingFactor / ANG2BOHR;
          } else if (strstr(line.c_str(), "crystal")) {
            blockCoordsAreFractional = true;
            posScale = 1.0;
          }

          getline(ifs, line);
          while (!strstr(line.c_str(), "End final coordinates")) {
            lineSplit = Common::split(Common::reduce(line), ' ');
            if (lineSplit.size() < 4) {
              Common::error(QString("Could not read atomic positions in PWSCF output! %1")
                           .arg(line.c_str()));
              return false;
            }
            const unsigned int atomicNum = Atoms::ElementInfo::getAtomicNum(lineSplit[0]);
            if (atomicNum == 0) {
              Common::error(QString("Unrecognized element symbol in PWSCF output: %1")
                           .arg(lineSplit[0].c_str()));
              return false;
            }
            blockAtomicNums.append(atomicNum);
            double x, y, z;
            if (!Common::parseDoubleString(lineSplit[1], x) ||
                !Common::parseDoubleString(lineSplit[2], y) ||
                !Common::parseDoubleString(lineSplit[3], z)) {
              Common::error(QString("Could not read atomic positions in PWSCF output! %1")
                           .arg(line.c_str()));
              return false;
            }
            blockCoords.append(Common::Vector3(x, y, z) * posScale);
            if (!getline(ifs, line))
              break;
          }

          blockCoordsFound = true;

          // If we haven't found CELL_PARAMETERS, assumes that we were
          // not relaxing the unit cell, and that our old unit cell is
          // the same.
          if (!blockCellFound) {
            blockCellMatrix = s->unitCell().cellMatrix();
            blockCellFound = true;
          }

          // After we find ATOMIC_POSITIONS, the loop is done. Break.
          break;
        }

        // Get a new line. If we reached the end of the file, break.
        if (!getline(ifs, line))
          break;
      }
      if (blockCellFound && blockCoordsFound) {
        cellFound = true;
        coordsFound = true;
        coordsAreFractional = blockCoordsAreFractional;
        atomicNums = blockAtomicNums;
        coords = blockCoords;
        cellMatrix = blockCellMatrix;
      }
    }

  }

  if (!cellFound)
    Common::error("Cell info was not found in PWSCF output!");
  if (!coordsFound)
    Common::error("Atom coords not found in PWSCF output!");
  if (!cellFound || !coordsFound)
    return false;

  // Convert coords to Cartesian
  Atoms::UnitCell uc(cellMatrix);
  if (coordsAreFractional) {
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

bool PwscfFormat::read(Atoms::Geometry* s, const QString& filename)
{
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    Common::error(QString("PWSCF input, %1, could not be opened!")
                 .arg(filename));
    return false;
  }

  QTextStream in(&file);
  QStringList lines;
  while (!in.atEnd()) {
    QString line = in.readLine();
    line = line.section('!', 0, 0).trimmed();
    if (!line.isEmpty())
      lines.append(line);
  }

  bool cellFound = false;
  bool coordsFound = false;
  bool hasAlat = false;
  double alat = 1.0;
  Common::Matrix3 cellMatrix = Common::Matrix3::Zero();
  QList<unsigned int> atomicNums;
  QList<Common::Vector3> coords;

  for (int i = 0; i < lines.size(); ++i) {
    const QStringList assignments = lines.at(i).split(",", QtCompat::SkipEmptyParts);
    for (int j = 0; j < assignments.size(); ++j) {
      const QStringList fields = assignments.at(j).split("=", QtCompat::SkipEmptyParts);
      if (fields.size() < 2)
        continue;
      const QString name = fields.at(0).trimmed().toLower();
      bool ok = false;
      const double value = fields.at(1).trimmed().toDouble(&ok);
      if (!ok)
        continue;
      if (name == "celldm(1)" || name == "celldm (1)") {
        alat = value / ANG2BOHR;
        hasAlat = true;
      } else if (name == "a") {
        alat = value;
        hasAlat = true;
      }
    }
  }

  if (alat <= 0.0) {
    Common::error("PWSCF input contains an invalid lattice parameter.");
    return false;
  }

  for (int i = 0; i < lines.size(); ++i) {
    const QString lower = lines.at(i).toLower();
    if (lower.startsWith("cell_parameters")) {
      double scale = alat;
      if (lower.contains("bohr"))
        scale = 1.0 / ANG2BOHR;
      else if (lower.contains("angstrom"))
        scale = 1.0;
      else if (!hasAlat)
        scale = 1.0 / ANG2BOHR;

      if (i + 3 >= lines.size()) {
        Common::error("Incomplete CELL_PARAMETERS block in PWSCF input.");
        return false;
      }
      for (int j = 0; j < 3; ++j) {
        const QStringList fields = lines.at(++i).simplified().split(" ", QtCompat::SkipEmptyParts);
        if (fields.size() < 3) {
          Common::error(QString("Could not read CELL_PARAMETERS line in PWSCF input: %1")
                       .arg(lines.at(i)));
          return false;
        }
        bool ok0 = false, ok1 = false, ok2 = false;
        cellMatrix(j, 0) = fields.at(0).toDouble(&ok0) * scale;
        cellMatrix(j, 1) = fields.at(1).toDouble(&ok1) * scale;
        cellMatrix(j, 2) = fields.at(2).toDouble(&ok2) * scale;
        if (!ok0 || !ok1 || !ok2) {
          Common::error(QString("Could not parse CELL_PARAMETERS line in PWSCF input: %1")
                       .arg(lines.at(i)));
          return false;
        }
      }
      if (!hasAlat) {
        alat = std::sqrt(cellMatrix(0, 0) * cellMatrix(0, 0) +
                         cellMatrix(0, 1) * cellMatrix(0, 1) +
                         cellMatrix(0, 2) * cellMatrix(0, 2));
        if (alat <= 0.0) {
          Common::error("PWSCF input contains an invalid lattice parameter.");
          return false;
        }
      }
      cellFound = true;
    } else if (lower.startsWith("atomic_positions")) {
      if (!cellFound) {
        Common::error("PWSCF input reader requires CELL_PARAMETERS before ATOMIC_POSITIONS.");
        return false;
      }

      const bool fractional = lower.contains("crystal") || lower.contains("crystal_sg");
      double scale = fractional ? 1.0 : alat;
      if (lower.contains("bohr"))
        scale = 1.0 / ANG2BOHR;
      else if (lower.contains("angstrom"))
        scale = 1.0;

      for (++i; i < lines.size(); ++i) {
        const QString l = lines.at(i).trimmed();
        const QString lLower = l.toLower();
        if (l.startsWith("&") || l.startsWith("/") || lLower.startsWith("k_points") ||
            lLower.startsWith("cell_parameters") || lLower.startsWith("atomic_species"))
          break;

        const QStringList fields = l.simplified().split(" ", QtCompat::SkipEmptyParts);
        if (fields.size() < 4)
          break;

        bool ok0 = false, ok1 = false, ok2 = false;
        Common::Vector3 pos(fields.at(1).toDouble(&ok0), fields.at(2).toDouble(&ok1),
                    fields.at(3).toDouble(&ok2));
        if (!ok0 || !ok1 || !ok2)
          break;

        const unsigned int atomicNum = Atoms::ElementInfo::getAtomicNum(fields.at(0).toStdString());
        if (atomicNum == 0) {
          Common::error(QString("Unrecognized element symbol in PWSCF input: %1")
                       .arg(fields.at(0)));
          return false;
        }
        atomicNums.append(atomicNum);
        coords.append(pos * scale);
      }
      --i;
      coordsFound = !coords.isEmpty();

      Atoms::UnitCell uc(cellMatrix);
      if (fractional) {
        for (int j = 0; j < coords.size(); ++j)
          coords[j] = uc.toCartesian(coords[j]);
      }
    }
  }

  if (!cellFound || !coordsFound) {
    if (!cellFound)
      Common::error("Cell info was not found in PWSCF input!");
    if (!coordsFound)
      Common::error("Atom coords not found in PWSCF input!");
    return false;
  }

  std::vector<Atoms::Atom> atoms;
  atoms.reserve(coords.size());
  for (int i = 0; i < coords.size(); ++i)
    atoms.push_back(Atoms::Atom(static_cast<unsigned short>(atomicNums.at(i)), coords.at(i)));

  s->clear();
  s->setUnitCell(Atoms::UnitCell(cellMatrix));
  s->setAtoms(atoms);
  return true;
}

} // namespace Atoms

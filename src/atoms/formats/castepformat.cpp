/**********************************************************************
  CastepFormat - A simple reader for CASTEP output structure.

  Copyright (C) 2017 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <atoms/formats/castepformat.h>

#include <common/compatibility/qt_compat.h>
#include <common/constants.h>
#include <common/fileutils.h>
#include <atoms/eleminfo.h>
#include <common/output.h>
#include <common/stringutils.h>
#include <atoms/geometry.h>

#include <cstring>
#include <sstream>

#include <QFile>
#include <QString>
#include <QStringList>
#include <QTextStream>

namespace Atoms {

namespace {

QString stripCastepComment(const QString& line)
{
  return line.section('#', 0, 0).section('!', 0, 0).trimmed();
}

bool castepIsNumber(const QString& text)
{
  bool ok = false;
  text.toDouble(&ok);
  return ok;
}

double castepUnitScale(const QString& unit)
{
  if (unit.toLower().contains("bohr"))
    return 1.0 / ANG2BOHR;
  return 1.0;
}

bool parseCastepVector(const QString& line, Common::Vector3& vector)
{
  const QStringList fields = line.split(" ", QtCompat::SkipEmptyParts);
  if (fields.size() < 3)
    return false;
  bool ok0 = false, ok1 = false, ok2 = false;
  vector = Common::Vector3(fields.at(0).toDouble(&ok0), fields.at(1).toDouble(&ok1),
                   fields.at(2).toDouble(&ok2));
  return ok0 && ok1 && ok2;
}

} // namespace


bool CastepFormat::read(Atoms::Geometry* s, const QString& filename)
{
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    Common::error(QString("CASTEP input, %1, could not be opened!")
                 .arg(filename));
    return false;
  }

  QTextStream in(&file);
  QStringList lines;
  while (!in.atEnd()) {
    const QString line = stripCastepComment(in.readLine());
    if (!line.isEmpty())
      lines.append(line);
  }

  bool cellFound = false;
  bool coordsFound = false;
  bool fractionalCoords = false;
  Common::Matrix3 cellMatrix = Common::Matrix3::Zero();
  QList<unsigned int> atomicNums;
  QList<Common::Vector3> coords;

  for (int i = 0; i < lines.size(); ++i) {
    const QString line = lines.at(i);
    const QString lower = line.toLower();

    if (lower.startsWith("%block lattice_cart")) {
      double scale = 1.0;
      int firstVectorLine = i + 1;
      if (firstVectorLine < lines.size()) {
        const QStringList fields = lines.at(firstVectorLine).split(" ", QtCompat::SkipEmptyParts);
        if (fields.size() == 1 && !castepIsNumber(fields.first())) {
          scale = castepUnitScale(fields.first());
          ++firstVectorLine;
        }
      }
      if (firstVectorLine + 2 >= lines.size()) {
        Common::error("Incomplete LATTICE_CART block in CASTEP input.");
        return false;
      }
      for (int j = 0; j < 3; ++j) {
        Common::Vector3 vector;
        if (!parseCastepVector(lines.at(firstVectorLine + j), vector)) {
          Common::error(QString("Could not parse LATTICE_CART line in CASTEP input: %1")
                       .arg(lines.at(firstVectorLine + j)));
          return false;
        }
        cellMatrix(j, 0) = vector.x() * scale;
        cellMatrix(j, 1) = vector.y() * scale;
        cellMatrix(j, 2) = vector.z() * scale;
      }
      cellFound = true;
    } else if (lower.startsWith("%block lattice_abc")) {
      double scale = 1.0;
      int firstParamLine = i + 1;
      if (firstParamLine < lines.size()) {
        const QStringList fields = lines.at(firstParamLine).split(" ", QtCompat::SkipEmptyParts);
        if (fields.size() == 1 && !castepIsNumber(fields.first())) {
          scale = castepUnitScale(fields.first());
          ++firstParamLine;
        }
      }
      if (firstParamLine + 1 >= lines.size()) {
        Common::error("Incomplete LATTICE_ABC block in CASTEP input.");
        return false;
      }
      Common::Vector3 lengths, angles;
      if (!parseCastepVector(lines.at(firstParamLine), lengths) ||
          !parseCastepVector(lines.at(firstParamLine + 1), angles)) {
        Common::error("Could not parse LATTICE_ABC block in CASTEP input.");
        return false;
      }
      cellMatrix = Atoms::UnitCell(lengths.x() * scale, lengths.y() * scale,
                            lengths.z() * scale, angles.x(), angles.y(), angles.z()).cellMatrix();
      cellFound = true;
    } else if (lower.startsWith("%block positions_frac") ||
               lower.startsWith("%block positions_abs")) {
      fractionalCoords = lower.startsWith("%block positions_frac");
      double scale = 1.0;
      int firstAtomLine = i + 1;
      if (!fractionalCoords && firstAtomLine < lines.size()) {
        const QStringList fields = lines.at(firstAtomLine).split(" ", QtCompat::SkipEmptyParts);
        if (fields.size() == 1 && !castepIsNumber(fields.first())) {
          scale = castepUnitScale(fields.first());
          ++firstAtomLine;
        }
      }

      for (int j = firstAtomLine; j < lines.size(); ++j) {
        const QString l = lines.at(j);
        if (l.toLower().startsWith("%endblock")) {
          i = j;
          break;
        }
        const QStringList fields = l.split(" ", QtCompat::SkipEmptyParts);
        if (fields.size() < 4)
          continue;
        bool ok0 = false, ok1 = false, ok2 = false;
        Common::Vector3 pos(fields.at(1).toDouble(&ok0), fields.at(2).toDouble(&ok1),
                    fields.at(3).toDouble(&ok2));
        const unsigned int atomicNum = Atoms::ElementInfo::getAtomicNum(fields.at(0).toStdString());
        if (!ok0 || !ok1 || !ok2 || atomicNum == 0) {
          Common::error(QString("Could not parse positions line in CASTEP input: %1")
                       .arg(l));
          return false;
        }
        atomicNums.append(atomicNum);
        coords.append(pos * scale);
      }
      coordsFound = !coords.isEmpty();
    }
  }

  if (!cellFound || !coordsFound) {
    if (!cellFound)
      Common::error("Cell info was not found in CASTEP input!");
    if (!coordsFound)
      Common::error("Atom coords not found in CASTEP input!");
    return false;
  }

  Atoms::UnitCell uc(cellMatrix);
  if (fractionalCoords) {
    for (int i = 0; i < coords.size(); ++i)
      coords[i] = uc.toCartesian(coords[i]);
  }

  std::vector<Atoms::Atom> atoms;
  atoms.reserve(coords.size());
  for (int i = 0; i < coords.size(); ++i)
    atoms.push_back(Atoms::Atom(static_cast<unsigned short>(atomicNums.at(i)), coords.at(i)));

  s->clear();
  s->setUnitCell(uc);
  s->setAtoms(atoms);
  return true;
}

/** The output we are looking for should look something like this:
 *
 * ================================================================================
 *  BFGS: Final Configuration:
 * ================================================================================
 *
 *                            -------------------------------
 *                                       Unit Cell
 *                            -------------------------------
 *         Real Lattice(A)              Reciprocal Lattice(1/A)
 *    1.0352960   0.0311284   0.0112407        6.0183250   1.3284540   0.9861314
 *   -0.4938210   2.2087554   0.0382761       -0.0859956   2.8236982   0.1008568
 *   -0.5538192  -0.1445463   3.5746629       -0.0180041  -0.0344124   1.7535192
 *
 *                        Lattice parameters(A)       Cell Angles
 *                     a =    1.035825          alpha =   89.363425
 *                     b =    2.263609          beta  =   98.243872
 *                     c =    3.620197          gamma =  100.867480
 *
 *                 Current cell volume =    8.248807 A**3
 *
 *                            -------------------------------
 *                                      Cell Contents
 *                            -------------------------------
 *
 *             xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
 *             x  Element    Atom        Fractional coordinates of atoms  x
 *             x            Number           u          v          w      x
 *             x----------------------------------------------------------x
 *             x   H          1         -0.144690  -0.171910  -0.132545   x
 *             x   H          2          0.865522   0.331412   0.367463   x
 *             xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
 *
 *
 *  BFGS: Final Enthalpy     = -3.01566909E+001 eV
 *
 */

bool CastepFormat::readOutput(Atoms::Geometry* s, const QString& filename)
{
  std::string text;
  if (!Common::readFileToString(filename, &text)) {
    Common::error(QString("CASTEP output, %1, could not be opened!")
                 .arg(filename));
    return false;
  }
  std::istringstream ifs(text);

  bool coordsFound = false, cellFound = false;

  QList<unsigned int> atomicNums;
  QList<Common::Vector3> coords;
  Common::Matrix3 cellMatrix = Common::Matrix3::Zero();

  std::string line;
  std::vector<std::string> lineSplit;
  while (getline(ifs, line)) {
    // This section should contain everything we need except energy
    if (strstr(line.c_str(), "Final Configuration")) {
      bool blockCoordsFound = false;
      bool blockCellFound = false;
      QList<unsigned int> blockAtomicNums;
      QList<Common::Vector3> blockCoords;
      Common::Matrix3 blockCellMatrix = Common::Matrix3::Zero();
      // We will break out of this while loop when we finish with it
      while (getline(ifs, line)) {
        // Cell matrix.
        if (strstr(line.c_str(), "Unit Cell")) {
          getline(ifs, line); // Should be: ---------------...
          getline(ifs, line); // Should be: Real Lattice(A)...
          if (!strstr(line.c_str(), "Real Lattice(A)")) {
            Common::error(QString("Could not read the real lattice in CASTEP output! %1")
                         .arg(line.c_str()));
            return false;
          }
          // Get the cell matrix.
          for (unsigned short i = 0; i < 3; ++i) {
            getline(ifs, line);
            lineSplit = Common::split(line, ' ');
            // It has a size of 6 because the reciprocal lattice is here also
            if (lineSplit.size() != 6) {
              Common::error(QString("Could not read the cell matrix in CASTEP output! %1")
                           .arg(line.c_str()));
              return false;
            }
            if (!Common::parseDoubleString(lineSplit[0], blockCellMatrix(i, 0)) ||
                !Common::parseDoubleString(lineSplit[1], blockCellMatrix(i, 1)) ||
                !Common::parseDoubleString(lineSplit[2], blockCellMatrix(i, 2))) {
              Common::error(QString("Could not read the cell matrix in CASTEP output! %1")
                           .arg(line.c_str()));
              return false;
            }
          }

          blockCellFound = true;
        }
        // Atomic coords
        if (strstr(line.c_str(), "Cell Contents")) {
          getline(ifs, line); // Should be: ------------------...
          getline(ifs, line); // Should be:
          getline(ifs, line); // Should be: xxxxxxxxxxxxxxxxxx...
          getline(ifs, line); // Should be: x  Element    Atom ...
          getline(ifs, line); // Should be: x            Number ...
          getline(ifs, line); // Should be: ------------------...

          getline(ifs, line); // Here's where the coordinates actually start!
          while (!strstr(line.c_str(), "xxxxxxxx")) {
            lineSplit = Common::split(line, ' ');
            if (lineSplit.size() != 7) {
              Common::error(QString("Could not read atomic positions in CASTEP output! %1")
                           .arg(line.c_str()));
              return false;
            }
            const unsigned int atomicNum = Atoms::ElementInfo::getAtomicNum(lineSplit[1]);
            if (atomicNum == 0) {
              Common::error(QString("Unrecognized element symbol in CASTEP output: %1")
                           .arg(lineSplit[1].c_str()));
              return false;
            }
            blockAtomicNums.append(atomicNum);
            double x, y, z;
            if (!Common::parseDoubleString(lineSplit[3], x) ||
                !Common::parseDoubleString(lineSplit[4], y) ||
                !Common::parseDoubleString(lineSplit[5], z)) {
              Common::error(QString("Could not read atomic positions in CASTEP output! %1")
                           .arg(line.c_str()));
              return false;
            }
            blockCoords.append(Common::Vector3(x, y, z));
            if (!getline(ifs, line))
              break;
          }

          blockCoordsFound = true;
        }
        if (blockCellFound && blockCoordsFound)
          break;
      }
      if (blockCellFound && blockCoordsFound) {
        cellFound = true;
        coordsFound = true;
        atomicNums = blockAtomicNums;
        coords = blockCoords;
        cellMatrix = blockCellMatrix;
      }
    }
  }

  if (!cellFound)
    Common::error("Cell info was not found in CASTEP output!");
  if (!coordsFound)
    Common::error("Atom coords not found in CASTEP output!");
  if (!cellFound || !coordsFound)
    return false;

  // Convert coords to Cartesian
  Atoms::UnitCell uc(cellMatrix);
  for (int i = 0; i < coords.size(); ++i)
    coords[i] = uc.toCartesian(coords[i]);

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

} // namespace Atoms

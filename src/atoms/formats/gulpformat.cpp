/**********************************************************************
  GulpFormat - A simple reader for GULP output structure.

  Copyright (C) 2016 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <atoms/formats/gulpformat.h>

#include <atoms/eleminfo.h>
#include <common/fileutils.h>
#include <common/output.h>
#include <common/stringutils.h>
#include <atoms/geometry.h>

#include <cstring>
#include <sstream>

#include <QString>

namespace Atoms {

bool GulpFormat::readOutput(Atoms::Geometry* s, const QString& filename)
{
  std::string text;
  if (!Common::readFileToString(filename, &text)) {
    Common::error(QString("GULP output, %1, could not be opened!")
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
    // Cell Matrix
    if (strstr(line.c_str(), "Final Cartesian lattice vectors (Angs")) {
      getline(ifs, line); // Blank

      // Get the cell matrix
      for (size_t i = 0; i < 3; ++i) {
        getline(ifs, line);
        lineSplit = Common::split(line, ' ');
        if (lineSplit.size() != 3) {
          Common::error(QString("Could not read the cell matrix in GULP output! %1")
                       .arg(line.c_str()));
          return false;
        }
        if (!Common::parseDoubleString(lineSplit[0], cellMatrix(i, 0)) ||
            !Common::parseDoubleString(lineSplit[1], cellMatrix(i, 1)) ||
            !Common::parseDoubleString(lineSplit[2], cellMatrix(i, 2))) {
          Common::error(QString("Could not read the cell matrix in GULP output! %1")
                       .arg(line.c_str()));
          return false;
        }
      }
      cellFound = true;
    }
    // Atomic coords
    else if (strstr(line.c_str(), "Final fractional coordinates of atoms")) {
      // Only the last complete coordinates block is kept.
      atomicNums.clear();
      coords.clear();
      // Grab the fractional coordinates of the atoms
      getline(ifs, line); // Blank
      getline(ifs, line); // -----
      getline(ifs, line); // No.
      getline(ifs, line); // Label
      getline(ifs, line); // -----

      // Now let's add in the atoms!
      getline(ifs, line);
      while (!strstr(line.c_str(), "------------------")) {
        lineSplit = Common::split(line, ' ');
        if (lineSplit.size() < 7) {
          Common::error(QString("Incomplete coords line in GULP output: %1")
                       .arg(line.c_str()));
          return false;
        }
        const unsigned int atomicNum = Atoms::ElementInfo::getAtomicNum(lineSplit[1]);
        if (atomicNum == 0) {
          Common::error(QString("Unrecognized element symbol in GULP output: %1")
                       .arg(lineSplit[1].c_str()));
          return false;
        }
        atomicNums.append(atomicNum);
        double x, y, z;
        if (!Common::parseDoubleString(lineSplit[3], x) ||
            !Common::parseDoubleString(lineSplit[4], y) ||
            !Common::parseDoubleString(lineSplit[5], z)) {
          Common::error(QString("Incomplete coords line in GULP output: %1")
                       .arg(line.c_str()));
          return false;
        }
        coords.append(Common::Vector3(x, y, z));
        getline(ifs, line);
      }
      coordsFound = true;
    }
  }

  if (!cellFound)
    Common::error("Cell info was not found in GULP output!");
  if (!coordsFound)
    Common::error("Atom coords not found in GULP output!");
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

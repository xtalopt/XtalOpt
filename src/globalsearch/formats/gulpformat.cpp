/**********************************************************************
  GulpFormat -- A simple reader for GULP output.

  Copyright (C) 2016 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <globalsearch/formats/gulpformat.h>

#include <globalsearch/eleminfo.h>
#include <globalsearch/structure.h>
#include <globalsearch/utilities/utilityfunctions.h>

#include <fstream>

#include <QDebug>
#include <QString>

namespace GlobalSearch {

// We are passing by copy on purpose so we can sort...
/*  static bool sameAtomicNums(std::vector<unsigned int> a,
                             std::vector<unsigned short> b)
  {
    if (a.size() != b.size())
      return false;
    std::sort(a.begin(), a.end());
    std::sort(b.begin(), b.end());
    for (size_t i = 0; i < a.size(); ++i) {
      if (a[i] != b[i])
        return false;
    }
    return true;
  }
*/

bool GulpFormat::read(Structure* s, const QString& filename)
{
  std::ifstream ifs(filename.toStdString());
  if (!ifs) {
    qDebug() << "Error: GULP output, " << filename << ", could not "
             << "be opened!";
    return false;
  }

  bool coordsFound = false, energyFound = false, cellFound = false;

  QList<unsigned int> atomicNums;
  QList<Vector3> coords;
  double energy = 0;
  double enthalpy = 0;
  Matrix3 cellMatrix = Matrix3::Zero();

  std::string line;
  std::vector<std::string> lineSplit;
  while (getline(ifs, line)) {
    // This is an option - but we are currently not using this
    // Cell parameters
    /*
    if (strstr(line.c_str(), "Final cell parameters and derivatives :")) {
      // Grab a, b, c, alpha, beta, gamma
      getline(ifs, line); // Blank
      getline(ifs, line); // -----

      getline(ifs, line); // a
      lineSplit = split(line, ' ');
      double a = atof(lineSplit.at(1).c_str());

      getline(ifs, line); // b
      lineSplit = split(line, ' ');
      removeEmptyStrings(lineSplit);
      double b = atof(lineSplit.at(1).c_str());

      getline(ifs, line); // c
      lineSplit = split(line, ' ');
      removeEmptyStrings(lineSplit);
      double c = atof(lineSplit.at(1).c_str());

      getline(ifs, line); // alpha
      lineSplit = split(line, ' ');
      removeEmptyStrings(lineSplit);
      double alpha = atof(lineSplit.at(1).c_str());

      getline(ifs, line); // beta
      lineSplit = split(line, ' ');
      removeEmptyStrings(lineSplit);
      double beta = atof(lineSplit.at(1).c_str());

      getline(ifs, line); // gamma
      lineSplit = split(line, ' ');
      removeEmptyStrings(lineSplit);
      double gamma = atof(lineSplit.at(1).c_str());

      // Create a temporary unit cell to get the matrix out of
      UnitCell uc;
      uc.setCellParameters(a, b, c, alpha, beta, gamma);
      cellMatrix = uc.cellMatrix();
      cellFound = true;
    }
    */
    // Cell Matrix
    if (strstr(line.c_str(), "Final Cartesian lattice vectors (Angs")) {
      getline(ifs, line); // Blank

      // Get the cell matrix
      for (size_t i = 0; i < 3; ++i) {
        getline(ifs, line);
        lineSplit = split(line, ' ');
        if (lineSplit.size() != 3) {
          qDebug() << "Error reading the cell matrix in GULP output!"
                   << line.c_str();
          return false;
        }
        cellMatrix(i, 0) = atof(lineSplit[0].c_str());
        cellMatrix(i, 1) = atof(lineSplit[1].c_str());
        cellMatrix(i, 2) = atof(lineSplit[2].c_str());
      }
      cellFound = true;
    }
    // Atomic coords
    else if (strstr(line.c_str(), "Final fractional coordinates of atoms")) {
      // Grab the fractional coordinates of the atoms
      getline(ifs, line); // Blank
      getline(ifs, line); // -----
      getline(ifs, line); // No.
      getline(ifs, line); // Label
      getline(ifs, line); // -----

      // Now let's add in the atoms!
      getline(ifs, line);
      while (!strstr(line.c_str(), "------------------")) {
        lineSplit = split(line, ' ');
        if (lineSplit.size() < 7) {
          qDebug() << "Error: incomplete coords line in GULP output: "
                   << line.c_str();
          return false;
        }
        atomicNums.append(ElemInfo::getAtomicNum(lineSplit[1]));
        coords.append(Vector3(atof(lineSplit[3].c_str()),
                              atof(lineSplit[4].c_str()),
                              atof(lineSplit[5].c_str())));
        getline(ifs, line);
      }
      coordsFound = true;
    }

    // Energy in eV - this will be overwritten if a new energy is discovered
    // below it in the file.
    else if (strstr(line.c_str(), "Total lattice energy") &&
             strstr(line.c_str(), "eV")) {
      // Grab the energy in eV
      lineSplit = split(line, ' ');
      if (line.size() < 6) {
        qDebug() << "Error: incomplete energy line in GULP output: "
                 << line.c_str();
        return false;
      }
      energy = atof(lineSplit[4].c_str());
      energyFound = true;
    }
  }

  if (!cellFound)
    qDebug() << "Error: cell info was not found in GULP output!";
  if (!coordsFound)
    qDebug() << "Error: atom coords not found in GULP output!";
  if (!energyFound)
    qDebug() << "Error: energy not found in GULP output!";
  if (!cellFound || !coordsFound || !energyFound)
    return false;

  // Convert coords to Cartesian
  UnitCell uc(cellMatrix);
  for (size_t i = 0; i < coords.size(); ++i)
    coords[i] = uc.toCartesian(coords[i]);

  s->updateAndAddToHistory(atomicNums, coords, energy, enthalpy, cellMatrix);
  return true;
}
}

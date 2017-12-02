/**********************************************************************
  PwscfFormat -- A simple reader for PWSCF output.

  Copyright (C) 2017 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <globalsearch/formats/pwscfformat.h>

#include <globalsearch/eleminfo.h>
#include <globalsearch/structure.h>
#include <globalsearch/utilities/utilityfunctions.h>

#include <fstream>

#include <QDebug>
#include <QString>

// A conversion term we need here...
static const double RY_TO_EV = 13.60569193;

namespace GlobalSearch {

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

bool PwscfFormat::read(Structure* s, const QString& filename)
{
  std::ifstream ifs(filename.toStdString());
  if (!ifs) {
    qDebug() << "Error: PWSCF output, " << filename << ", could not "
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
    // Cell parameters
    // This section should contain the final cell and final coordinates
    if (strstr(line.c_str(), "Begin final coordinates")) {
      getline(ifs, line);
      while (!strstr(line.c_str(), "End final coordinates")) {
        if (strstr(line.c_str(), "CELL_PARAMETERS")) {
          // Get the scaling factor and then the cell matrix
          lineSplit = split(line, '=');
          double scalingFactor = 1.0;

          if (lineSplit.size() < 2) {
            qDebug() << "Warning: in PWSCF output, alat was not found.";
            qDebug() << "Assuming alat to be 1.0";
          } else {
            std::string alatLine = trim(lineSplit[1]);
            replaceAll(alatLine, ")", "");
            scalingFactor = atof(alatLine.c_str());
          }

          // Get the cell matrix
          for (unsigned short i = 0; i < 3; ++i) {
            getline(ifs, line);
            lineSplit = split(line, ' ');
            if (lineSplit.size() != 3) {
              qDebug() << "Error reading the cell matrix in PWSCF output!"
                       << line.c_str();
              return false;
            }
            cellMatrix(i, 0) = atof(lineSplit[0].c_str()) * scalingFactor;
            cellMatrix(i, 1) = atof(lineSplit[1].c_str()) * scalingFactor;
            cellMatrix(i, 2) = atof(lineSplit[2].c_str()) * scalingFactor;
          }

          cellFound = true;
        }
        // Atomic coords
        if (strstr(line.c_str(), "ATOMIC_POSITIONS")) {
          getline(ifs, line);
          while (!strstr(line.c_str(), "End final coordinates")) {
            lineSplit = split(line, ' ');
            if (lineSplit.size() != 4) {
              qDebug() << "Error reading atomic positions in PWSCF output!"
                       << line.c_str();
              return false;
            }
            atomicNums.append(ElemInfo::getAtomicNum(lineSplit[0]));
            coords.append(Vector3(atof(lineSplit[1].c_str()),
                                  atof(lineSplit[2].c_str()),
                                  atof(lineSplit[3].c_str())));
            if (!getline(ifs, line))
              break;
          }

          coordsFound = true;

          // If we haven't found CELL_PARAMETERS, assumes that we were
          // not relaxing the unit cell, and that our old unit cell is
          // the same.
          if (!cellFound) {
            cellMatrix = s->unitCell().cellMatrix();
            cellFound = true;
          }

          // After we find ATOMIC_POSITIONS, the loop is done. Break.
          break;
        }

        // Get a new line. If we reached the end of the file, break.
        if (!getline(ifs, line))
          break;
      }
    }

    // Enthalpy in Ry. Convert to eV.
    else if (strstr(line.c_str(), "Final enthalpy")) {
      lineSplit = split(line, ' ');
      if (line.size() < 5) {
        qDebug() << "Error reading final enthalpy in PWSCF output!"
                 << line.c_str();
        return false;
      }
      enthalpy = atof(lineSplit[3].c_str()) * RY_TO_EV;
      // If the energy hasn't been found, set the energy to be the enthalpy
      if (fabs(energy) < 1e-8)
        energy = enthalpy;
      energyFound = true;
    }
    // Energy in Ry. Convert to eV.
    else if (strstr(line.c_str(), "!    total energy")) {
      lineSplit = split(line, ' ');
      if (line.size() < 6) {
        qDebug() << "Error reading final energy in PWSCF output!"
                 << line.c_str();
        return false;
      }
      energy = atof(lineSplit[4].c_str()) * RY_TO_EV;
      energyFound = true;
    }
  }

  if (!cellFound)
    qDebug() << "Error: cell info was not found in PWSCF output!";
  if (!coordsFound)
    qDebug() << "Error: atom coords not found in PWSCF output!";
  if (!energyFound)
    qDebug() << "Error: energy not found in PWSCF output!";
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

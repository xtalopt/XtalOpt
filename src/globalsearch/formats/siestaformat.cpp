/**********************************************************************
  SiestaFormat -- A simple reader for SIESTA output.

  Copyright (C) 2016 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <globalsearch/formats/siestaformat.h>

#include <globalsearch/eleminfo.h>
#include <globalsearch/structure.h>
#include <globalsearch/utilities/utilityfunctions.h>

#include <fstream>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QString>

namespace GlobalSearch {

bool SiestaFormat::read(Structure* s, const QString& filename)
{
  std::ifstream ifs(filename.toStdString());
  if (!ifs) {
    qDebug() << "Error: SIESTA output, " << filename << ", could not "
             << "be opened!";
    return false;
  }

  bool coordsFound = false, energyFound = false, cellFound = false;
  bool fractionalCoords = false, angstromCoords = true;

  QList<unsigned int> atomicNums;
  QList<Vector3> coords;
  double energy = 0;
  double enthalpy = 0;
  Matrix3 cellMatrix = Matrix3::Zero();

  // See if we can open BASIS_ENTHALPY and read the enthalpy. If not, just
  // leave it at zero.
  QFileInfo info(filename);
  QString basisEnthalpyFile =
    info.absolutePath() + QDir::separator() + "BASIS_ENTHALPY";
  std::ifstream basisEnthalpyIfs(basisEnthalpyFile.toStdString());
  if (basisEnthalpyIfs) {
    std::string line;
    getline(basisEnthalpyIfs, line);
    enthalpy = atof(line.c_str());
  }

  std::string line;
  std::vector<std::string> lineSplit;
  while (getline(ifs, line)) {
    // Cell Matrix
    if (strstr(line.c_str(), "outcell: Unit cell vectors")) {
      if (!strstr(line.c_str(), "(Ang)")) {
        qDebug() << "Error: the output cell matrix is not in Angstroms.";
        qDebug() << "Please contact the developers of XtalOpt.";
        qDebug() << "The faulty line is: " << line.c_str();
        return false;
      }

      // Get the cell matrix
      for (size_t i = 0; i < 3; ++i) {
        getline(ifs, line);
        lineSplit = split(line, ' ');
        if (lineSplit.size() != 3) {
          qDebug() << "Error reading the cell matrix in SIESTA output!"
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
    else if (strstr(line.c_str(), "outcoor:")) {
      atomicNums.clear();
      coords.clear();
      if (strstr(line.c_str(), "(fractional)")) {
        fractionalCoords = true;
        angstromCoords = false;
      } else if (strstr(line.c_str(), "(Ang)")) {
        angstromCoords = true;
      } else if (strstr(line.c_str(), "(Bohr)")) {
        angstromCoords = false;
      } else {
        qDebug() << "Error: the atom coords have unrecognizable units.";
        qDebug() << "Please contact the developers of XtalOpt.";
        qDebug() << "The faulty line is: " << line.c_str();
        return false;
      }

      // Now let's add in the atoms!
      getline(ifs, line);
      line = trim(line);
      // A blank line will be encountered at the end
      while (line.size() > 1) {
        lineSplit = split(line, ' ');
        if (lineSplit.size() < 6) {
          qDebug() << "Error: incomplete coords line in SIESTA output: "
                   << line.c_str();
          return false;
        }
        atomicNums.append(ElemInfo::getAtomicNum(lineSplit[5]));
        coords.append(Vector3(atof(lineSplit[0].c_str()),
                              atof(lineSplit[1].c_str()),
                              atof(lineSplit[2].c_str())));
        getline(ifs, line);
        line = trim(line);
      }
      coordsFound = true;
    }

    // Energy in eV
    else if (strstr(line.c_str(), "siesta: Final energy (eV)")) {
      // Loop through the next several lines
      getline(ifs, line);
      line = trim(line);
      // All of these lines start with "siesta" for some reason...
      // Let's use that to see if we reach the end of the block
      while (strstr(line.c_str(), "siesta")) {
        if (strstr(line.c_str(), "Total =")) {
          lineSplit = split(line, ' ');
          if (lineSplit.size() != 4) {
            qDebug() << "Error reading energy in SIESTA output.";
            qDebug() << "Please contact the XtalOpt developers.";
            qDebug() << "Faulty line is: " << line.c_str();
            return false;
          }
          energy = atof(lineSplit[3].c_str());
          energyFound = true;
          break;
        }
        getline(ifs, line);
        line = trim(line);
      }
    }
  }

  if (!cellFound)
    qDebug() << "Error: cell info was not found in SIESTA output!";
  if (!coordsFound)
    qDebug() << "Error: atom coords not found in SIESTA output!";
  if (!energyFound)
    qDebug() << "Error: energy not found in SIESTA output!";
  if (!cellFound || !coordsFound || !energyFound)
    return false;

  // Convert coords if we need to
  UnitCell uc(cellMatrix);
  if (fractionalCoords) {
    for (size_t i = 0; i < coords.size(); ++i)
      coords[i] = uc.toCartesian(coords[i]);
  }
  // Assume we have Bohr coords
  else if (!fractionalCoords && !angstromCoords) {
    for (size_t i = 0; i < coords.size(); ++i)
      coords[i] *= 0.529177249; // Bohr to Angstrom
  }
  // Nothing to do if !fractionalCoords and angstromCoords

  s->updateAndAddToHistory(atomicNums, coords, energy, enthalpy, cellMatrix);
  return true;
}
}

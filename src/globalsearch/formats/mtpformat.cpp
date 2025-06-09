/**********************************************************************
  MtpFormat -- A simple reader for MTP output.

  Copyright (C) 2025 by Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <globalsearch/formats/mtpformat.h>

#include <globalsearch/eleminfo.h>
#include <globalsearch/structure.h>
#include <globalsearch/utilities/utilityfunctions.h>

#include <fstream>

#include <QDebug>
#include <QString>

namespace GlobalSearch {

bool MtpFormat::read(Structure* s, const QString& filename)
{
  std::ifstream ifs(filename.toStdString());
  if (!ifs) {
    qDebug() << "Error: MTP output, " << filename << ", could not "
             << "be opened!";
    return false;
  }

  bool coordsFound = false, energyFound = false, cellFound = false;
  bool relaxOK = false;
  bool fractionalCoord = true;

  int numAtoms = 0;
  QList<int> atomicTypes;
  QList<unsigned int> atomicNums;
  QList<QString> chemicalSystem;
  QList<Vector3> coords;
  double energy = 0;
  double enthalpy = 0;
  Matrix3 cellMatrix = Matrix3::Zero();

  std::string line;
  std::vector<std::string> lineSplit;
  while (getline(ifs, line)) {
    if (strstr(line.c_str(), "Size")) {
      // Get the number of atoms
      getline(ifs, line);
      lineSplit = split(line, ' ');
      if (lineSplit.size() != 1) {
        qDebug() << "Error reading the number of atoms in MTP output!"
                 << line.c_str();
        return false;
      }
      numAtoms = atoi(lineSplit[0].c_str());
    } else if (strstr(line.c_str(), "Supercell")) {
      // Get the cell matrix
      for (size_t i = 0; i < 3; ++i) {
        getline(ifs, line);
        lineSplit = split(line, ' ');
        if (lineSplit.size() != 3) {
          qDebug() << "Error reading the cell matrix in MTP output!"
                   << line.c_str();
          return false;
        }
        cellMatrix(i, 0) = atof(lineSplit[0].c_str());
        cellMatrix(i, 1) = atof(lineSplit[1].c_str());
        cellMatrix(i, 2) = atof(lineSplit[2].c_str());
      }
      cellFound = true;
    } else if (strstr(line.c_str(), "AtomData")) {
      // Get the atomic coordinates
      if (strstr(line.c_str(), "cartes"))
        fractionalCoord = false;
      for (int i = 0; i < numAtoms; i++) {
        getline(ifs, line);
        lineSplit = split(line, ' ');
        if (lineSplit.size() < 5) {
          qDebug() << "Error: incomplete coords line in MTP output: "
                   << line.c_str();
          return false;
        }
        atomicTypes.append(atoi(lineSplit[1].c_str()));
        coords.append(Vector3(atof(lineSplit[2].c_str()),
                              atof(lineSplit[3].c_str()),
                              atof(lineSplit[4].c_str())));
      }
      coordsFound = true;
    } else if (strstr(line.c_str(), "Energy")) {
      // Get the energy
      getline(ifs, line);
      lineSplit = split(line, ' ');
      if (lineSplit.size() != 1) {
        qDebug() << "Error reading the energy in MTP output!"
                 << line.c_str();
        return false;
      }
      energy = atof(lineSplit[0].c_str());

      energyFound = true;
    } else if (strstr(line.c_str(), "chemical_system")) {
      // Get the full chemical system
      // MTP strangely replaces "space" with "tab" in the "Feature"
      //   lines. We should fix this first before reading symbols.
      for (size_t i = 0; i < line.length(); ++i) {
        if (line[i] == '\t') {
          line.replace(i, 1, std::string(1, ' '));
        }
      }
      lineSplit = split(line, ' ');
      if (lineSplit.size() < 3) {
        qDebug() << "Error reading the chemical system in MTP output!"
                 << line.c_str();
        return false;
      }
      for (int i = 2; i < lineSplit.size(); i++) {
        chemicalSystem.append(lineSplit[i].c_str());
      }
    } else if (strstr(line.c_str(), "relaxation_OK")) {
      // Get the relaxation status
      relaxOK = true;
    }
  }

  // Before everything, let's make sure that the optimization has
  //   converged, withing the force/stress thresholds.
  if (!relaxOK) {
    qDebug() << "Error: un-successfull relaxation in MTP output for" << s->getTag();
    return false;
  }

  if (!cellFound)
    qDebug() << "Error: cell info was not found in MTP output!";
  if (!coordsFound)
    qDebug() << "Error: atom coords not found in MTP output!";
  if (!energyFound)
    qDebug() << "Error: energy not found in MTP output!";
  if (!cellFound || !coordsFound || !energyFound)
    return false;

  // This is important: we need to properly extract the atomic types.
  int min = *std::min_element(atomicTypes.begin(), atomicTypes.end());
  int max = *std::max_element(atomicTypes.begin(), atomicTypes.end());
  if (min < 0 || max > chemicalSystem.size()) {
    qDebug() << "Error: failed to read atomic types from MTP output!";
    return false;
  }
  // Produce the list of atomic numbers
  for (int i = 0; i < atomicTypes.size(); i++) {
    QString sym = chemicalSystem[atomicTypes[i]];
    atomicNums.append(ElementInfo::getAtomicNum(sym.toStdString()));
  }

  // Convert coords to Cartesian, if needed
  if (fractionalCoord) {
  UnitCell uc(cellMatrix);
  for (size_t i = 0; i < coords.size(); ++i)
    coords[i] = uc.toCartesian(coords[i]);
  }

  s->updateAndAddToHistory(atomicNums, coords, energy, enthalpy, cellMatrix);

  return true;
}
}

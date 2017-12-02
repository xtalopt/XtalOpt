/**********************************************************************
  CastepFormat -- A simple reader for CASTEP output.

  Copyright (C) 2017 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <globalsearch/formats/castepformat.h>

#include <globalsearch/eleminfo.h>
#include <globalsearch/structure.h>
#include <globalsearch/utilities/utilityfunctions.h>

#include <fstream>

#include <QDebug>
#include <QString>

namespace GlobalSearch {

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

bool CastepFormat::read(Structure* s, const QString& filename)
{
  std::ifstream ifs(filename.toStdString());
  if (!ifs) {
    qDebug() << "Error: CASTEP output, " << filename << ", could not "
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
    // This section should contain everything we need except energy
    if (strstr(line.c_str(), "Final Configuration")) {
      // We will break out of this while loop when we finish with it
      while (getline(ifs, line)) {
        // Cell matrix.
        if (strstr(line.c_str(), "Unit Cell")) {
          getline(ifs, line); // Should be: ---------------...
          getline(ifs, line); // Should be: Real Lattice(A)...
          if (!strstr(line.c_str(), "Real Lattice(A)")) {
            qDebug() << "Error reading the real lattice in CASTEP output!"
                     << line.c_str();
            return false;
          }
          // Get the cell matrix.
          for (unsigned short i = 0; i < 3; ++i) {
            getline(ifs, line);
            lineSplit = split(line, ' ');
            // It has a size of 6 because the reciprocal lattice is here also
            if (lineSplit.size() != 6) {
              qDebug() << "Error reading the cell matrix in CASTEP output!"
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
        if (strstr(line.c_str(), "Cell Contents")) {
          getline(ifs, line); // Should be: ------------------...
          getline(ifs, line); // Should be:
          getline(ifs, line); // Should be: xxxxxxxxxxxxxxxxxx...
          getline(ifs, line); // Should be: x  Element    Atom ...
          getline(ifs, line); // Should be: x            Number ...
          getline(ifs, line); // Should be: ------------------...

          getline(ifs, line); // Here's where the coordinates actually start!
          while (!strstr(line.c_str(), "xxxxxxxx")) {
            lineSplit = split(line, ' ');
            if (lineSplit.size() != 7) {
              qDebug() << "Error reading atomic positions in CASTEP output!"
                       << line.c_str();
              return false;
            }
            atomicNums.append(ElemInfo::getAtomicNum(lineSplit[1]));
            coords.append(Vector3(atof(lineSplit[3].c_str()),
                                  atof(lineSplit[4].c_str()),
                                  atof(lineSplit[5].c_str())));
            if (!getline(ifs, line))
              break;
          }

          coordsFound = true;
        }
        // Enthalpy
        if (strstr(line.c_str(), "Final Enthalpy")) {
          lineSplit = split(line, ' ');
          if (lineSplit.size() != 6) {
            qDebug() << "Error reading final enthalpy in CASTEP output!"
                     << line.c_str();
            return false;
          }
          enthalpy = atof(lineSplit[4].c_str());

          // If we haven't found the energy yet, set this to be the energy
          if (fabs(energy) < 1e-8)
            energy = enthalpy;

          energyFound = true;

          // This is the last thing that we need in the inner loop. Break.
          break;
        }
      }
    }

    // Energy. This may be encountered several times,
    // so it will be updated with each encounter.
    else if (strstr(line.c_str(), "Final energy, E")) {
      lineSplit = split(line, ' ');
      if (line.size() < 6) {
        qDebug() << "Error reading final energy in CASTEP output!"
                 << line.c_str();
        return false;
      }
      energy = atof(lineSplit[4].c_str());
      energyFound = true;
    }
  }

  if (!cellFound)
    qDebug() << "Error: cell info was not found in CASTEP output!";
  if (!coordsFound)
    qDebug() << "Error: atom coords not found in CASTEP output!";
  if (!energyFound)
    qDebug() << "Error: energy not found in CASTEP output!";
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

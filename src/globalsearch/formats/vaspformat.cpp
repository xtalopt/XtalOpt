/**********************************************************************
  VaspFormat -- A simple reader for VASP output.

  Copyright (C) 2017 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <globalsearch/formats/poscarformat.h>
#include <globalsearch/formats/vaspformat.h>

#include <globalsearch/eleminfo.h>
#include <globalsearch/structure.h>
#include <globalsearch/utilities/utilityfunctions.h>

#include <algorithm> // for std::count()
#include <fstream>
#include <iomanip>
#include <iostream>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QString>

using std::getline;
using std::string;
using std::vector;

namespace GlobalSearch {

// This is for reading a CONTCAR
bool VaspFormat::read(Structure* s, const QString& filename)
{
  std::ifstream ifs(filename.toStdString());
  if (!ifs) {
    qDebug() << "Error: CONTCAR, " << filename << ", could not be opened!";
    return false;
  }

  // First, read the POSCAR file
  if (!PoscarFormat::read(*s, ifs)) {
    qDebug() << "Error in VaspFormat: failed to read POSCAR file!";
    return false;
  }

  // Now find the energy in the OUTCAR.
  QString outcarFile = QFileInfo(filename).dir().absolutePath() + "/OUTCAR";

  // Open it and make sure it exists
  std::ifstream outcar_ifs(outcarFile.toStdString());

  // We don't want to print a warning here if the OUTCAR file doesn't exist
  // because sometimes (like when the user is loading seeds), there is no
  // OUTCAR. We do not want the user to be concerned about it.

  double energy = 0.0, enthalpy = 0.0;

  bool energyFound = getOUTCAREnergy(outcar_ifs, energy);
  bool enthalpyFound = getOUTCAREnthalpy(outcar_ifs, enthalpy);

  if (outcar_ifs && !energyFound) {
    qDebug() << "Warning: the energy could not be found in the"
             << "OUTCAR file!";
  }

  s->setEnergy(energy);
  if (enthalpyFound)
    s->setEnthalpy(enthalpy);

  if (s->reusePreoptBonding()) {
    s->bonds() = s->getPreoptBonding();
    s->clearPreoptBonding();
  }

  return true;
}

bool VaspFormat::getOUTCAREnergy(std::istream& in, double& energy)
{
  if (!in)
    return false;

  string line;
  // We will read backwards and stop as soono as we find the energy
  in.seekg(0, std::ios::end);
  while (in.tellg() >= 0) {
    reverseGetline(in, line);
    if (strstr(line.c_str(), "free  energy   TOTEN")) {
      vector<string> stringSplit = split(line, ' ');
      // Make sure the line is long enough. If not, just keep reading.
      if (stringSplit.size() < 5)
        continue;

      energy = atof(stringSplit[4].c_str());
      return true;
    }
  }
  return false;
}

bool VaspFormat::getOUTCAREnthalpy(std::istream& in, double& enthalpy)
{
  if (!in)
    return false;

  string line;
  // We will read backwards and stop as soono as we find the energy
  in.seekg(0, std::ios::end);
  while (in.tellg() >= 0) {
    reverseGetline(in, line);
    if (strstr(line.c_str(), "enthalpy is")) {
      vector<string> stringSplit = split(line, ' ');
      // Make sure the line is long enough. If not, just keep reading.
      if (stringSplit.size() < 5)
        continue;

      enthalpy = atof(stringSplit[4].c_str());
      return true;
    }
  }
  return false;
}
}

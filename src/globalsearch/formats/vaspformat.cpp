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

#include <globalsearch/formats/vaspformat.h>

#include <globalsearch/eleminfo.h>
#include <globalsearch/utilities/utilityfunctions.h>
#include <globalsearch/structure.h>

#include <algorithm> // for std::count()
#include <fstream>
#include <iomanip>
#include <iostream>

#include <QtCore/QDebug>
#include <QtCore/QString>

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

    size_t numLines = std::count(std::istreambuf_iterator<char>(ifs),
                                 std::istreambuf_iterator<char>(), '\n');

    // There must be at least 7 "\n"'s to have a minimum crystal
    // (including 1 atom)
    if (numLines < 7) {
      qDebug() << "Error: CONTCAR file is invalid: it is 7 or fewer lines";
      return false;
    }

    // We have to go back to the beginning if we are going to read again
    ifs.clear();
    ifs.seekg(0, std::ios::beg);

    // We'll use these throughout
    string line;
    vector<string> stringSplit;

    // First line is comment line
    getline(ifs, line);
    line = trim(line);
    string title = " ";
    if (!line.empty())
      title = line;

    // Next line is scaling factor
    getline(ifs, line);
    double scalingFactor = atof(line.c_str());

    // Next comes the matrix
    Matrix3 cellMat;
    for (size_t i = 0; i < 3; ++i) {
      getline(ifs, line);
      stringSplit = split(line, ' ');
      // If this is not three, then there is some kind of error in the line
      if (stringSplit.size() != 3) {
        qDebug() << "Error reading lattice vectors in CONTCAR";
        return false;
      }
      // UnitCell expects a matrix of this form
      cellMat(i, 0) = atof(stringSplit[0].c_str()) * scalingFactor;
      cellMat(i, 1) = atof(stringSplit[1].c_str()) * scalingFactor;
      cellMat(i, 2) = atof(stringSplit[2].c_str()) * scalingFactor;
    }

    // Sometimes, atomic symbols go here.
    getline(ifs, line);
    stringSplit = split(line, ' ');

    if (stringSplit.empty()) {
      qDebug() << "Error reading numbers of atom types in CONTCAR";
      return false;
    }

    // Check to see if this is an integer.
    // If it is not, assume we have an atomic symbols list
    bool isInt = isInteger(trim(stringSplit.at(0)));
    vector<string> symbolsList;
    QList<unsigned int> atomicNumbers;

    if (!isInt) {
      // Assume atomic symbols are here and store them
      symbolsList = split(line, ' ');
      // Store atomic nums
      for (size_t i = 0; i < symbolsList.size(); ++i)
        atomicNumbers.append(ElemInfo::getAtomicNum(symbolsList[i]));
      // This next one should be atom types
      getline(ifs, line);
    }
    // If the atomic symbols aren't here, try to find them in the title
    // In Vasp 4.x, symbols are in the title like so: " O4H2 <restOfTitle>"
    else {
      stringSplit = split(title, ' ');
      if (stringSplit.size() != 0) {
        string trimFormula = trim(stringSplit.at(0));
        // Let's replace all numbers with spaces
        for (size_t i = 0; i < trimFormula.size(); ++i) {
          if (isdigit(trimFormula.at(i)))
            trimFormula[i] = ' ';
        }
        // Now get the symbols with a simple space split
        symbolsList = split(trimFormula, ' ');
        for (size_t i = 0; i < symbolsList.size(); ++i)
          atomicNumbers.append(ElemInfo::getAtomicNum(symbolsList.at(i)));
      }
    }

    stringSplit = split(line, ' ');
    vector<unsigned int> atomCounts;
    for (size_t i = 0; i < stringSplit.size(); ++i)
      atomCounts.push_back(atoi(stringSplit.at(i).c_str()));

    // Make sure we found the atomic numbers.
    if (atomicNumbers.size() == 0) {
      qDebug() << "Error: atomic numbers not found in CONTCAR!";
      return false;
    }

    // Make sure the numbers match
    if (atomicNumbers.size() != atomCounts.size()) {
      qDebug() << "Error: numSymbols and numTypes are not equal in CONTCAR!";
      return false;
    }

    // Starts with either [Ss]elective dynamics, [KkCc]artesian, or
    // other for fractional coords.
    getline(ifs, line);
    line = trim(line);

    // If selective dynamics, skip over it and get the next line
    if (line.empty() || line.at(0) == 'S' || line.at(0) == 's')
      getline(ifs, line);

    line = trim(line);
    if (line.empty()) {
      qDebug() << "Error determining Direct or Cartesian in CONTCAR";
      return false;
    }

    bool cart;
    // Check if we're using cartesian or fractional coordinates:
    if (line[0] == 'C' || line[0] == 'c' ||
        line[0] == 'K' || line[0] == 'k' ) {
      cart = true;
    }
    // Assume direct if one of these was not found
    else {
      cart = false;
    }

    // Now get the coords and make the expanded atomic nums list
    QList<Vector3> coords;
    QList<unsigned int> expandedAtomicNums;
    for (size_t i = 0; i < atomCounts.size(); ++i) {
      for (size_t j = 0; j < atomCounts[i]; ++j) {
        getline(ifs, line);
        stringSplit = split(line, ' ');
        // This may be greater than 3 with selective dynamics
        if (stringSplit.size() < 3) {
          qDebug() << "Error reading atomic coordinates in CONTCAR.";
          return false;
        }
        Vector3 tmpCoords(atof(stringSplit[0].c_str()),
                          atof(stringSplit[1].c_str()),
                          atof(stringSplit[2].c_str()));
        coords.append(tmpCoords);
        expandedAtomicNums.append(atomicNumbers[i]);
      }
    }

    // Let's make a unit cell
    UnitCell cell = UnitCell(cellMat);

    // If our atomic coordinates are fractional, convert them to Cartesian
    if (!cart) {
      for (size_t i = 0; i < coords.size(); ++i)
        coords[i] = cell.toCartesian(coords.at(i));
    }
    // If they're already cartesian, we just need to apply the scaling factor
    else {
      for (size_t i = 0; i < coords.size(); ++i)
        coords[i] *= scalingFactor;
    }

    // Now find the energy in the OUTCAR.
    QString outcarFile = filename;
    if (filename.endsWith("CONTCAR"))
      outcarFile.chop(7);
    else if (filename.endsWith("POSCAR"))
      outcarFile.chop(6);

    outcarFile.append("OUTCAR");

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

    s->updateAndAddToHistory(expandedAtomicNums, coords,
                             energy, enthalpy, cellMat);

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

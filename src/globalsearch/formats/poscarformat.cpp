/**********************************************************************
  PoscarFormat -- A simple reader for VASP output.

  Copyright (C) 2017 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include "poscarformat.h"

#include <globalsearch/eleminfo.h>
#include <globalsearch/structure.h>
#include <globalsearch/utilities/utilityfunctions.h>

#include <algorithm> // for std::count()
#include <fstream>
#include <iomanip>
#include <iostream>

#include <QStringList>

using std::getline;
using std::string;
using std::vector;

namespace GlobalSearch {

// This is for reading a POSCAR
bool PoscarFormat::read(Structure& s, std::istream& in)
{
  // First, clear the structure
  s.clear();

  size_t numLines = std::count(std::istreambuf_iterator<char>(in),
                               std::istreambuf_iterator<char>(), '\n');

  // There must be at least 7 "\n"'s to have a minimum crystal
  // (including 1 atom)
  if (numLines < 7) {
    std::cerr << "Error: POSCAR file is invalid: it is 7 or fewer lines\n";
    return false;
  }

  // We have to go back to the beginning if we are going to read again
  in.clear();
  in.seekg(0, std::ios::beg);

  // We'll use these throughout
  string line;
  vector<string> stringSplit;

  // First line is comment line
  getline(in, line);
  line = trim(line);
  string title = " ";
  if (!line.empty())
    title = line;

  // Next line is scaling factor
  getline(in, line);
  double scalingFactor = atof(line.c_str());

  // Next comes the matrix
  Matrix3 cellMat;
  for (size_t i = 0; i < 3; ++i) {
    getline(in, line);
    stringSplit = split(line, ' ');
    // If this is not three, then there is some kind of error in the line
    if (stringSplit.size() != 3) {
      std::cerr << "Error reading lattice vectors in POSCAR\n";
      return false;
    }
    // UnitCell expects a matrix of this form
    cellMat(i, 0) = atof(stringSplit[0].c_str()) * scalingFactor;
    cellMat(i, 1) = atof(stringSplit[1].c_str()) * scalingFactor;
    cellMat(i, 2) = atof(stringSplit[2].c_str()) * scalingFactor;
  }

  // Sometimes, atomic symbols go here.
  getline(in, line);
  stringSplit = split(line, ' ');

  if (stringSplit.empty()) {
    std::cerr << "Error reading numbers of atom types in POSCAR\n";
    return false;
  }

  // Check to see if this is an integer.
  // If it is not, assume we have an atomic symbols list
  bool isInt = isInteger(trim(stringSplit.at(0)));
  vector<string> symbolsList;
  vector<unsigned int> atomicNumbers;

  if (!isInt) {
    // Assume atomic symbols are here and store them
    symbolsList = split(line, ' ');
    // Store atomic nums
    for (size_t i = 0; i < symbolsList.size(); ++i)
      atomicNumbers.push_back(ElemInfo::getAtomicNum(symbolsList[i]));
    // This next one should be atom types
    getline(in, line);
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
        atomicNumbers.push_back(ElemInfo::getAtomicNum(symbolsList.at(i)));
    }
  }

  stringSplit = split(line, ' ');
  vector<unsigned int> atomCounts;
  for (size_t i = 0; i < stringSplit.size(); ++i)
    atomCounts.push_back(atoi(stringSplit.at(i).c_str()));

  // Make sure we found the atomic numbers.
  if (atomicNumbers.size() == 0) {
    std::cerr << "Error: atomic numbers not found in POSCAR!\n";
    return false;
  }

  // Make sure the numbers match
  if (atomicNumbers.size() != atomCounts.size()) {
    std::cerr << "Error: numSymbols and numTypes "
              << "are not equal in POSCAR!\n";
    return false;
  }

  // Starts with either [Ss]elective dynamics, [KkCc]artesian, or
  // other for fractional coords.
  getline(in, line);
  line = trim(line);

  // If selective dynamics, skip over it and get the next line
  if (line.empty() || line.at(0) == 'S' || line.at(0) == 's')
    getline(in, line);

  line = trim(line);
  if (line.empty()) {
    std::cerr << "Error determining Direct or Cartesian in POSCAR\n";
    return false;
  }

  bool cart;
  // Check if we're using cartesian or fractional coordinates:
  if (line[0] == 'C' || line[0] == 'c' || line[0] == 'K' || line[0] == 'k') {
    cart = true;
  }
  // Assume direct if one of these was not found
  else {
    cart = false;
  }

  // Now get the coords
  std::vector<Atom> atoms;
  for (size_t i = 0; i < atomCounts.size(); ++i) {
    for (size_t j = 0; j < atomCounts[i]; ++j) {
      getline(in, line);
      stringSplit = split(line, ' ');
      // This may be greater than 3 with selective dynamics
      if (stringSplit.size() < 3) {
        std::cerr << "Error reading atomic coordinates in POSCAR.\n";
        return false;
      }
      Vector3 coord(atof(stringSplit[0].c_str()), atof(stringSplit[1].c_str()),
                    atof(stringSplit[2].c_str()));
      atoms.push_back(Atom(atomicNumbers[i], coord));
    }
  }

  // Let's make a unit cell
  UnitCell cell = UnitCell(cellMat);

  // If our atomic coordinates are fractional, convert them to Cartesian
  if (!cart) {
    for (auto& atom : atoms)
      atom.setPos(cell.toCartesian(atom.pos()));
  }
  // If they're already cartesian, we just need to apply the scaling factor
  else {
    for (auto& atom : atoms)
      atom.setPos(atom.pos() * scalingFactor);
  }

  // Success! Now let's add the unit cell and the atoms!
  s.setUnitCell(cell);
  s.setAtoms(atoms);

  return true;
}

bool PoscarFormat::write(const Structure& s, std::ostream& out)
{
  // Comment line -- set to composition then filename
  // Construct composition
  QStringList symbols = s.getSymbols();
  QList<unsigned int> atomCounts = s.getNumberOfAtomsAlpha();
  Q_ASSERT_X(symbols.size() == atomCounts.size(), Q_FUNC_INFO,
             "getSymbols() is not the same size as getNumberOfAtomsAlpha()");
  for (size_t i = 0; i < symbols.size(); ++i)
    out << symbols[i].toStdString() << atomCounts[i];

  out << " " << s.fileName().toStdString() << "\n";

  // Scaling factor. Just 1.0
  out << 1.0 << "\n";

  // Unit Cell Vectors
  for (uint i = 0; i < 3; i++) {
    out << std::fixed << std::setw(12) << std::setprecision(8)
        << s.unitCell().cellMatrix()(i, 0) << " ";
    out << std::fixed << std::setw(12) << std::setprecision(8)
        << s.unitCell().cellMatrix()(i, 1) << " ";
    out << std::fixed << std::setw(12) << std::setprecision(8)
        << s.unitCell().cellMatrix()(i, 2) << "\n";
  }
  // Atomic symbols
  for (const auto& symbol : symbols)
    out << symbol.toStdString() + " ";
  out << "\n";

  // Number of each type of atom (sorted alphabetically by symbol)
  for (const auto& count : atomCounts)
    out << count << " ";

  out << "\n";
  // Use fractional coordinates:
  out << "Direct\n";
  // Coordinates of each atom (sorted alphabetically by symbol)
  QList<Vector3> coords = s.getAtomCoordsFrac();
  for (const auto& coord : coords) {
    out << std::fixed << std::setw(12) << std::setprecision(8) << coord.x()
        << " ";
    out << std::fixed << std::setw(12) << std::setprecision(8) << coord.y()
        << " ";
    out << std::fixed << std::setw(12) << std::setprecision(8) << coord.z()
        << "\n";
  }

  return true;
}

void PoscarFormat::reorderAtomsToMatchPoscar(Structure& s)
{
  // Sort by symbols
  const auto& symbols = s.getSymbols();
  const auto& atoms = s.atoms();

  std::vector<size_t> newOrder;
  for (const auto& symbol_ref: symbols) {
    for (size_t i = 0; i < atoms.size(); ++i) {
      const auto& symbol_cur =
        ElemInfo::getAtomicSymbol(atoms[i].atomicNumber()).c_str();
      if (symbol_cur == symbol_ref)
        newOrder.push_back(i);
    }
  }

  s.reorderAtoms(newOrder);
}

}

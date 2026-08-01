/**********************************************************************
  PoscarFormat - Handlers for POSCAR format structure files.

  Copyright (C) 2017 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <atoms/formats/poscarformat.h>

#include <atoms/eleminfo.h>
#include <common/output.h>
#include <common/stringutils.h>
#include <atoms/geometry.h>

#include <algorithm> // for std::count()
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <QStringList>

using std::getline;
using std::string;
using std::vector;

namespace Atoms {

// This is for reading a POSCAR
bool PoscarFormat::read(Atoms::Geometry& s, std::istream& in)
{
  // First, clear the structure
  s.clear();

  size_t numLines = std::count(std::istreambuf_iterator<char>(in),
                               std::istreambuf_iterator<char>(), '\n');

  // There must be at least 7 "\n"'s to have a minimum crystal
  // (including 1 atom)
  if (numLines < 7) {
    Common::error("POSCAR file is invalid: it is 7 or fewer lines");
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
  line = Common::trim(line);
  string title = " ";
  if (!line.empty())
    title = line;

  // Next line is scaling factor
  getline(in, line);
  double scalingFactor;
  double scalingFactors[3];
  stringSplit = Common::split(Common::reduce(line), ' ');
  const size_t scalingFactorCount = stringSplit.size();
  if ((stringSplit.size() != 1 && stringSplit.size() != 3) ||
      !Common::parseDoubleString(stringSplit[0], scalingFactor)) {
    Common::error("Could not read scaling factor in POSCAR");
    return false;
  }
  scalingFactors[0] = scalingFactor;
  scalingFactors[1] = scalingFactor;
  scalingFactors[2] = scalingFactor;
  if (stringSplit.size() == 3) {
    for (size_t i = 0; i < 3; ++i) {
      if (!Common::parseDoubleString(stringSplit[i], scalingFactors[i]) ||
          scalingFactors[i] <= 0.0) {
        Common::error("Could not read scaling factors in POSCAR");
        return false;
      }
    }
  }

  // Next comes the matrix
  Common::Matrix3 cellMat;
  for (size_t i = 0; i < 3; ++i) {
    getline(in, line);
    stringSplit = Common::split(Common::reduce(line), ' ');
    // If this is not three, then there is some kind of error in the line
    if (stringSplit.size() != 3) {
      Common::error("Could not read lattice vectors in POSCAR");
      return false;
    }
    // Atoms::UnitCell expects a matrix of this form
    double v0, v1, v2;
    if (!Common::parseDoubleString(stringSplit[0], v0) ||
        !Common::parseDoubleString(stringSplit[1], v1) ||
        !Common::parseDoubleString(stringSplit[2], v2)) {
      Common::error("Could not read lattice vectors in POSCAR");
      return false;
    }
    cellMat(i, 0) = v0;
    cellMat(i, 1) = v1;
    cellMat(i, 2) = v2;
  }

  if (scalingFactor < 0.0) {
    if (scalingFactorCount != 1) {
      Common::error("POSCAR target volume requires one scaling factor");
      return false;
    }
    const double rawVolume = std::fabs(cellMat.determinant());
    if (rawVolume == 0.0) {
      Common::error("Could not apply the target volume in POSCAR");
      return false;
    }
    scalingFactor = std::pow(-scalingFactor / rawVolume, 1.0 / 3.0);
    scalingFactors[0] = scalingFactor;
    scalingFactors[1] = scalingFactor;
    scalingFactors[2] = scalingFactor;
  }
  for (size_t i = 0; i < 3; ++i)
    for (size_t j = 0; j < 3; ++j)
      cellMat(i, j) *= scalingFactors[j];

  // Sometimes, atomic symbols go here.
  getline(in, line);
  stringSplit = Common::split(Common::reduce(line), ' ');

  if (stringSplit.empty()) {
    Common::error("Could not read numbers of atom types in POSCAR");
    return false;
  }

  // Check to see if this is an integer.
  // If it is not, assume we have an atomic symbols list
  bool isInt = Common::isInteger(Common::trim(stringSplit.at(0)));
  vector<string> symbolsList;
  vector<unsigned int> atomicNumbers;

  if (!isInt) {
    // Assume atomic symbols are here and store them
    symbolsList = Common::split(Common::reduce(line), ' ');
    // Store atomic nums
    for (size_t i = 0; i < symbolsList.size(); ++i) {
      // This is to handle VASP compiled with HDF5 where "/..."
      //   might appear after species symbol
      string sptype = symbolsList[i].substr(0, symbolsList[i].find("/"));
      atomicNumbers.push_back(Atoms::ElementInfo::getAtomicNum(sptype));
    }
    // This next one should be atom types
    getline(in, line);
  }
  // If the atomic symbols aren't here, try to find them in the title
  // In Vasp 4.x, symbols are in the title like so: " O4H2 <restOfTitle>"
  else {
    stringSplit = Common::split(Common::reduce(title), ' ');
    if (stringSplit.size() != 0) {
      string trimFormula = Common::trim(stringSplit.at(0));
      // Let's replace all numbers with spaces
      for (size_t i = 0; i < trimFormula.size(); ++i) {
        if (isdigit(trimFormula.at(i)))
          trimFormula[i] = ' ';
      }
      // Now get the symbols with a simple space split
      symbolsList = Common::split(Common::reduce(trimFormula), ' ');
      for (size_t i = 0; i < symbolsList.size(); ++i) {
        // This is to handle VASP compiled with HDF5 where "/..."
        //   might appear after species symbol
        string sptype = symbolsList[i].substr(0, symbolsList[i].find("/"));
        atomicNumbers.push_back(Atoms::ElementInfo::getAtomicNum(sptype));
      }
    }
  }

  stringSplit = Common::split(Common::reduce(line), ' ');
  vector<unsigned int> atomCounts;
  for (size_t i = 0; i < stringSplit.size(); ++i) {
    int count;
    if (!Common::parseIntString(stringSplit.at(i), count) || count < 0) {
      Common::error("Could not read numbers of atom types in POSCAR");
      return false;
    }
    atomCounts.push_back(static_cast<unsigned int>(count));
  }

  // Make sure we found the atomic numbers.
  if (atomicNumbers.size() == 0) {
    Common::error("Atomic numbers not found in POSCAR!");
    return false;
  }

  // Make sure every symbol was recognized.
  for (size_t i = 0; i < atomicNumbers.size(); ++i) {
    if (atomicNumbers[i] == 0) {
      Common::error(QString("Unrecognized element symbol in POSCAR: %1")
                   .arg(QString::fromStdString(symbolsList[i])));
      return false;
    }
  }

  // Make sure the numbers match
  if (atomicNumbers.size() != atomCounts.size()) {
    Common::error("numSymbols and numTypes are not equal in POSCAR!");
    return false;
  }

  // Starts with either [Ss]elective dynamics, [KkCc]artesian, or
  // other for fractional coords.
  getline(in, line);
  line = Common::trim(line);

  // If selective dynamics, skip over it and get the next line
  if (line.empty() || line.at(0) == 'S' || line.at(0) == 's')
    getline(in, line);

  line = Common::trim(line);
  if (line.empty()) {
    Common::error("Could not determine Direct or Cartesian in POSCAR");
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
  std::vector<Atoms::Atom> atoms;
  for (size_t i = 0; i < atomCounts.size(); ++i) {
    for (size_t j = 0; j < atomCounts[i]; ++j) {
      getline(in, line);
      stringSplit = Common::split(Common::reduce(line), ' ');
      // This may be greater than 3 with selective dynamics
      if (stringSplit.size() < 3) {
        Common::error("Could not read atomic coordinates in POSCAR.");
        return false;
      }
      double x, y, z;
      if (!Common::parseDoubleString(stringSplit[0], x) ||
          !Common::parseDoubleString(stringSplit[1], y) ||
          !Common::parseDoubleString(stringSplit[2], z)) {
        Common::error("Could not read atomic coordinates in POSCAR.");
        return false;
      }
      Common::Vector3 coord(x, y, z);
      atoms.push_back(Atoms::Atom(atomicNumbers[i], coord));
    }
  }

  // Let's make a unit cell
  Atoms::UnitCell cell = Atoms::UnitCell(cellMat);

  // If our atomic coordinates are fractional, convert them to Cartesian
  if (!cart) {
    for (auto& atom : atoms)
      atom.setPos(cell.toCartesian(atom.pos()));
  }
  // If they're already cartesian, we just need to apply the scaling factor
  else {
    for (auto& atom : atoms)
      atom.setPos(Common::Vector3(atom.pos()[0] * scalingFactors[0],
                                  atom.pos()[1] * scalingFactors[1],
                                  atom.pos()[2] * scalingFactors[2]));
  }

  // Success! Now let's add the unit cell and the atoms!
  s.setUnitCell(cell);
  s.setAtoms(atoms);

  return true;
}

bool PoscarFormat::write(const Atoms::Geometry& s, std::ostream& out, const QString& comment)
{
  if (!s.is3D() || s.numAtoms() < 1) {
    Common::error("POSCAR writer requires a periodic structure with atoms.");
    return false;
  }

  // Comment line -- set to composition then optional text
  // Construct composition
  QStringList symbols = s.getSymbols();
  std::vector<unsigned int> atomCounts = s.getNumberOfAtomsAlpha();
  Q_ASSERT_X(static_cast<size_t>(symbols.size()) == atomCounts.size(), Q_FUNC_INFO,
             "getSymbols() is not the same size as getNumberOfAtomsAlpha()");
  for (int i = 0; i < symbols.size(); ++i)
    out << symbols[i].toStdString() << atomCounts[i];

  if (!comment.isEmpty())
    out << " " << comment.toStdString();
  out << "\n";

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
  std::vector<Common::Vector3> coords = s.getAtomCoordsFrac();
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

QString PoscarFormat::writeToString(const Atoms::Geometry& s, const QString& comment)
{
  std::ostringstream out;
  if (!write(s, out, comment))
    return QString();
  return QString::fromStdString(out.str());
}

void PoscarFormat::reorderAtomsToMatchPoscar(Atoms::Geometry& s)
{
  // Sort by symbols
  const auto& symbols = s.getSymbols();
  const auto& atoms = s.atoms();

  std::vector<size_t> newOrder;
  for (const auto& symbol_ref: symbols) {
    for (size_t i = 0; i < atoms.size(); ++i) {
      const std::string symbol_cur = Atoms::ElementInfo::getAtomicSymbol(atoms[i].atomicNumber());
      if (symbol_cur == symbol_ref.toStdString())
        newOrder.push_back(i);
    }
  }

  s.reorderAtoms(newOrder);
}

} // namespace Atoms

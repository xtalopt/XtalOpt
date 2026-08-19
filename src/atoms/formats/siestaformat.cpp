/**********************************************************************
  SiestaFormat - A simple reader for SIESTA output structure.

  Copyright (C) 2016 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <atoms/formats/siestaformat.h>

#include <common/compatibility/qt_compat.h>
#include <common/constants.h>
#include <common/fileutils.h>
#include <atoms/eleminfo.h>
#include <common/output.h>
#include <common/stringutils.h>
#include <atoms/geometry.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <vector>

#include <QFile>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include <map>

namespace Atoms {

namespace {

QString stripSiestaComment(const QString& line)
{
  return line.section('#', 0, 0).section('!', 0, 0).trimmed();
}

double siestaUnitScale(const QString& unit)
{
  const QString lower = unit.toLower();
  if (lower.contains("bohr"))
    return 1.0 / ANG2BOHR;
  return 1.0;
}

// For writing SIESTA z-matrices. Indices are zero-based internally; the
// SIESTA Zmatrix block itself is one-based.
struct ZMatrixEntry
{
  long long ind;
  long long rInd;
  long long angleInd;
  long long dihedralInd;

  ZMatrixEntry(long long atomInd = -1, long long rAtomInd = -1, long long angleAtomInd = -1,
               long long dihedralAtomInd = -1)
    : ind(atomInd),
      rInd(rAtomInd),
      angleInd(angleAtomInd),
      dihedralInd(dihedralAtomInd)
  {
  }
};

template<typename T>
bool alreadyInList(const std::vector<T>& v, const T& item)
{
  for (const auto& elem : v) {
    if (elem == item)
      return true;
  }
  return false;
}

long long indInEntries(long long ind, const std::vector<ZMatrixEntry>& entries,
                       size_t numAtomsInPreviousMolecules)
{
  for (size_t i = 0; i < entries.size(); ++i) {
    if (entries[i].ind == ind)
      return i - numAtomsInPreviousMolecules;
  }
  return -1;
}

std::map<unsigned short, size_t> getSiestaSpeciesNumbers(const Atoms::Geometry& s)
{
  std::map<unsigned short, size_t> ret;

  // We are going to store the species numbers in alphabetical order.
  const QStringList atomicSyms = s.getSymbols();

  for (int i = 0; i < atomicSyms.size(); ++i) {
    ret[Atoms::ElementInfo::getAtomicNum(
      atomicSyms.at(i).toStdString())] = static_cast<size_t>(i) + 1;
  }

  return ret;
}

std::vector<ZMatrixEntry> generateZMatrixEntries(const Atoms::Geometry* s)
{
  std::vector<ZMatrixEntry> ret;

  const std::vector<unsigned short>& atomicNums = s->atomicNumbers();
  std::vector<bool> atomAlreadyUsed(atomicNums.size(), false);

  // Let's make a priority list for atom selection
  // Carbon gets 2. Hydrogen gets 0. Everything else gets 1.
  std::vector<int> priorityList(atomicNums.size(), 1);
  for (size_t i = 0; i < atomicNums.size(); ++i) {
    if (atomicNums[i] == 6)
      priorityList[i] = 2;
    else if (atomicNums[i] == 1)
      priorityList[i] = 0;
  }

  // We'll keep doing this until we run out of molecules
  while (true) {
    // Find one that hasn't been used yet. If there aren't any, break
    const std::vector<bool>::iterator it =
      std::find(atomAlreadyUsed.begin(), atomAlreadyUsed.end(), false);

    if (it == atomAlreadyUsed.end())
      break;

    std::vector<ZMatrixEntry> currentMol;
    ZMatrixEntry firstEntry, secondEntry, thirdEntry;

    // If we find a higher priority atom that hasn't been used, start with
    // that one instead
    size_t workingInd = it - atomAlreadyUsed.begin();
    for (size_t i = 0; i < atomicNums.size(); ++i) {
      if (!atomAlreadyUsed[i] && priorityList[i] > priorityList[workingInd])
        workingInd = i;
    }

    atomAlreadyUsed[workingInd] = true;
    firstEntry.ind = workingInd;
    currentMol.push_back(firstEntry);

    // Now find atoms bonded to this one and make the highest priority atom
    // the second entry
    std::vector<size_t> bondedAtoms = s->bondedAtoms(firstEntry.ind);
    // If there are no bonded atoms, just continue
    if (bondedAtoms.empty()) {
      ret.insert(ret.end(), currentMol.begin(), currentMol.end());
      continue;
    }

    // Make sure none of these have been used. If they have been, report
    // an error, because that shouldn't be possible.
    for (const auto& bondedAtom : bondedAtoms) {
      if (atomAlreadyUsed[bondedAtom]) {
        Common::error(QString("%1: an atom bonded to the first atom in "
                              "this molecule has already been used! This "
                              "error should not be possible. Please contact "
                              "a developer.")
                        .arg(__func__));
        return ret;
      }
    }

    // Pick the highest priority atom for the second index
    workingInd = bondedAtoms[0];
    for (const auto& bondedAtom : bondedAtoms) {
      if (priorityList[bondedAtom] > priorityList[workingInd])
        workingInd = bondedAtom;
    }
    atomAlreadyUsed[workingInd] = true;
    secondEntry.ind = workingInd;
    secondEntry.rInd = firstEntry.ind;
    currentMol.push_back(secondEntry);

    // Erase that index so we can use bondedAtoms later
    bondedAtoms.erase(
      std::find(bondedAtoms.begin(), bondedAtoms.end(), workingInd));

    // Now to find the third entry using the same process, but include
    // both the first and second sets of bonded atoms.
    std::vector<size_t> tmpBondedAtoms = s->bondedAtoms(secondEntry.ind);
    for (const auto& e : tmpBondedAtoms) {
      if (!atomAlreadyUsed[e] && !alreadyInList(bondedAtoms, e))
        bondedAtoms.push_back(e);
    }

    // If there are no bonded atoms, just continue
    if (bondedAtoms.empty()) {
      ret.insert(ret.end(), currentMol.begin(), currentMol.end());
      continue;
    }

    // Pick the highest priority atom for the third index
    workingInd = bondedAtoms[0];
    for (const auto& bondedAtom : bondedAtoms) {
      if (priorityList[bondedAtom] > priorityList[workingInd])
        workingInd = bondedAtom;
    }

    atomAlreadyUsed[workingInd] = true;
    thirdEntry.ind = workingInd;

    // Figure out which atom this is bonded to
    if (s->areBonded(thirdEntry.ind, firstEntry.ind)) {
      thirdEntry.rInd = firstEntry.ind;
      thirdEntry.angleInd = secondEntry.ind;
    } else {
      thirdEntry.rInd = secondEntry.ind;
      thirdEntry.angleInd = firstEntry.ind;
    }

    currentMol.push_back(thirdEntry);

    // Erase that index so we can keep using bondedAtoms
    bondedAtoms.erase(
      std::find(bondedAtoms.begin(), bondedAtoms.end(), workingInd));

    tmpBondedAtoms = s->bondedAtoms(thirdEntry.ind);
    for (const auto& e : tmpBondedAtoms) {
      if (!atomAlreadyUsed[e] && !alreadyInList(bondedAtoms, e))
        bondedAtoms.push_back(e);
    }

    // If there are no bonded atoms, just continue
    if (bondedAtoms.empty()) {
      ret.insert(ret.end(), currentMol.begin(), currentMol.end());
      continue;
    }

    // Now loop until we are done
    while (!bondedAtoms.empty()) {
      ZMatrixEntry entry;

      // First, choose an atom via the priority list
      workingInd = bondedAtoms[0];
      for (const auto& bondedAtom : bondedAtoms) {
        if (priorityList[bondedAtom] > priorityList[workingInd])
          workingInd = bondedAtom;
      }
      entry.ind = workingInd;

      // Find an already used atom this one is bonded to
      for (const auto& elem : currentMol) {
        if (s->areBonded(entry.ind, elem.ind)) {
          entry.rInd = elem.ind;
          break;
        }
      }

      if (entry.rInd == -1) {
        Common::error(QString("%1: no atom was found that is bonded to "
                              "atom index %2. This error should not be "
                              "possible. Please contact a developer of this "
                              "program.")
                        .arg(__func__)
                        .arg(entry.ind));
        ret.clear();
        return ret;
      }

      // Now try to find one that rInd is bonded to
      for (const auto& elem : currentMol) {
        if (entry.rInd != elem.ind && s->areBonded(entry.rInd, elem.ind)) {
          entry.angleInd = elem.ind;
          break;
        }
      }

      if (entry.angleInd == -1) {
        Common::error(QString("%1: no atom was found that is bonded "
                              "to atom index %2. This error should not be "
                              "possible. Please contact a developer of this "
                              "program.")
                        .arg(__func__)
                        .arg(entry.rInd));
        ret.clear();
        return ret;
      }

      // Finally, find any others that any indices are connected to one of
      // these atoms.
      for (const auto& e : currentMol) {
        if ((entry.rInd != e.ind && entry.angleInd != e.ind) &&
            (s->areBonded(entry.ind, e.ind) ||
             s->areBonded(entry.rInd, e.ind) ||
             s->areBonded(entry.angleInd, e.ind))) {
          entry.dihedralInd = e.ind;
          break;
        }
      }

      atomAlreadyUsed[entry.ind] = true;
      currentMol.push_back(entry);

      bondedAtoms.erase(
        std::find(bondedAtoms.begin(), bondedAtoms.end(), entry.ind));

      tmpBondedAtoms = s->bondedAtoms(entry.ind);
      for (const auto& e : tmpBondedAtoms) {
        if (!atomAlreadyUsed[e] && !alreadyInList(bondedAtoms, e))
          bondedAtoms.push_back(e);
      }
    }

    // Append these to ret
    for (const auto& e : currentMol)
      ret.push_back(e);
  }

  return ret;
}

/**
 * Reorder the atoms so that they match the ordering for the z-matrix
 * ordering.
 *
 * @param s The Geometry whose atoms are to be reordered.
 */
void reorderAtomsToMatchZMatrix(Atoms::Geometry& s)
{
  const std::vector<ZMatrixEntry> entries = generateZMatrixEntries(&s);

  std::vector<size_t> newOrder;
  newOrder.reserve(entries.size());
  for (const auto& entry : entries)
    newOrder.push_back(entry.ind);

  s.reorderAtoms(newOrder);
}

} // namespace


bool SiestaFormat::read(Atoms::Geometry& s, const QString& filename)
{
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    Common::error(QString("SIESTA input, %1, could not be opened!")
                 .arg(filename));
    return false;
  }

  QTextStream in(&file);
  QStringList lines;
  while (!in.atEnd()) {
    const QString line = stripSiestaComment(in.readLine());
    if (!line.isEmpty())
      lines.append(line);
  }

  bool cellFound = false;
  bool coordsFound = false;
  bool fractionalCoords = false;
  Common::Matrix3 cellMatrix = Common::Matrix3::Zero();
  double latticeScale = 1.0;
  std::map<int, unsigned int> species;
  QList<unsigned int> atomicNums;
  QList<Common::Vector3> coords;

  // FDF entries may appear in any order, so find the lattice constant
  // first: the lattice vectors and scaled coordinates depend on it. Per
  // the FDF rules, the first occurrence of a label is the one that counts.
  for (int i = 0; i < lines.size(); ++i) {
    const QString line = lines.at(i);
    if (!line.toLower().startsWith("latticeconstant"))
      continue;
    const QStringList fields = line.split(" ", QtCompat::SkipEmptyParts);
    if (fields.size() >= 2) {
      bool ok = false;
      const double value = fields.at(1).toDouble(&ok);
      if (ok) {
        const QString unit = fields.size() >= 3 ? fields.at(2) : QString();
        latticeScale = value * siestaUnitScale(unit);
      }
    }
    break;
  }

  for (int i = 0; i < lines.size(); ++i) {
    const QString line = lines.at(i);
    const QString lower = line.toLower();

    if (lower.startsWith("%block chemicalspecieslabel")) {
      for (++i; i < lines.size(); ++i) {
        const QString l = lines.at(i);
        if (l.toLower().startsWith("%endblock"))
          break;
        const QStringList fields = l.split(" ", QtCompat::SkipEmptyParts);
        if (fields.size() < 3)
          continue;
        bool okIndex = false, okAtomic = false;
        const int speciesIndex = fields.at(0).toInt(&okIndex);
        unsigned int atomicNum = fields.at(1).toUInt(&okAtomic);
        if (!okAtomic)
          atomicNum = Atoms::ElementInfo::getAtomicNum(fields.at(2).toStdString());
        if (okIndex && atomicNum > 0)
          species[speciesIndex] = atomicNum;
      }
    } else if (lower.startsWith("%block latticevectors")) {
      if (i + 3 >= lines.size()) {
        Common::error("Incomplete LatticeVectors block in SIESTA input.");
        return false;
      }
      for (int j = 0; j < 3; ++j) {
        const QStringList fields = lines.at(++i).split(" ", QtCompat::SkipEmptyParts);
        if (fields.size() < 3) {
          Common::error(QString("Could not read LatticeVectors line in SIESTA input: %1")
                       .arg(lines.at(i)));
          return false;
        }
        bool ok0 = false, ok1 = false, ok2 = false;
        cellMatrix(j, 0) = fields.at(0).toDouble(&ok0) * latticeScale;
        cellMatrix(j, 1) = fields.at(1).toDouble(&ok1) * latticeScale;
        cellMatrix(j, 2) = fields.at(2).toDouble(&ok2) * latticeScale;
        if (!ok0 || !ok1 || !ok2) {
          Common::error(QString("Could not parse LatticeVectors line in SIESTA input: %1")
                       .arg(lines.at(i)));
          return false;
        }
      }
      cellFound = true;
    } else if (lower.startsWith("atomiccoordinatesformat")) {
      // "ScaledByLatticeVectors" is a synonym of "Fractional" in FDF.
      fractionalCoords = lower.contains("fractional") ||
                         lower.contains("scaledbylatticevectors");
    } else if (lower.startsWith("%block atomiccoordinatesandatomicspecies")) {
      double coordScale = 1.0;
      bool scaledCartesian = false;
      for (int j = 0; j < lines.size(); ++j) {
        const QString fmt = lines.at(j).toLower();
        if (!fmt.startsWith("atomiccoordinatesformat"))
          continue;
        // "ScaledByLatticeVectors" is a synonym of "Fractional" in FDF.
        fractionalCoords = fmt.contains("fractional") ||
                           fmt.contains("scaledbylatticevectors");
        scaledCartesian = fmt.contains("scaledcartesian");
        if (fmt.contains("bohr"))
          coordScale = 1.0 / ANG2BOHR;
        break;
      }
      if (scaledCartesian)
        coordScale = latticeScale;

      for (++i; i < lines.size(); ++i) {
        const QString l = lines.at(i);
        if (l.toLower().startsWith("%endblock"))
          break;
        const QStringList fields = l.split(" ", QtCompat::SkipEmptyParts);
        if (fields.size() < 4)
          continue;
        bool ok0 = false, ok1 = false, ok2 = false, okSpecies = false;
        Common::Vector3 pos(fields.at(0).toDouble(&ok0), fields.at(1).toDouble(&ok1),
                    fields.at(2).toDouble(&ok2));
        const int speciesIndex = fields.at(3).toInt(&okSpecies);
        if (!ok0 || !ok1 || !ok2 || !okSpecies || species.find(speciesIndex) == species.end()) {
          Common::error(QString("Could not parse AtomicCoordinatesAndAtomicSpecies line in SIESTA input: %1")
                       .arg(l));
          return false;
        }
        atomicNums.append(species[speciesIndex]);
        coords.append(pos * coordScale);
      }
      coordsFound = !coords.isEmpty();
    }
  }

  if (!cellFound || !coordsFound) {
    if (!cellFound)
      Common::error("Cell info was not found in SIESTA input!");
    if (!coordsFound)
      Common::error("Atom coords not found in SIESTA input!");
    return false;
  }

  Atoms::UnitCell uc(cellMatrix);
  if (fractionalCoords) {
    for (int i = 0; i < coords.size(); ++i)
      coords[i] = uc.toCartesian(coords[i]);
  }

  std::vector<Atoms::Atom> atoms;
  atoms.reserve(coords.size());
  for (int i = 0; i < coords.size(); ++i)
    atoms.push_back(Atoms::Atom(static_cast<unsigned short>(atomicNums.at(i)), coords.at(i)));

  s.clear();
  s.setUnitCell(uc);
  s.setAtoms(atoms);
  return true;
}

bool SiestaFormat::readOutput(Atoms::Geometry& s, const QString& filename)
{
  std::string text;
  if (!Common::readFileToString(filename, &text)) {
    Common::error(QString("SIESTA output, %1, could not be opened!")
                 .arg(filename));
    return false;
  }
  std::istringstream ifs(text);

  bool coordsFound = false, cellFound = false;
  bool fractionalCoords = false, angstromCoords = true;

  QList<unsigned int> atomicNums;
  QList<Common::Vector3> coords;
  Common::Matrix3 cellMatrix = Common::Matrix3::Zero();

  std::string line;
  std::vector<std::string> lineSplit;
  while (getline(ifs, line)) {
    // Cell Matrix
    if (strstr(line.c_str(), "outcell: Unit cell vectors")) {
      if (!strstr(line.c_str(), "(Ang)")) {
        Common::error("The output cell matrix is not in Angstroms.");
        Common::error("Please contact the developers of XtalOpt.");
        Common::error(QString("The faulty line is: %1").arg(line.c_str()));
        return false;
      }

      // Get the cell matrix
      for (size_t i = 0; i < 3; ++i) {
        getline(ifs, line);
        lineSplit = Common::split(line, ' ');
        if (lineSplit.size() != 3 || !Common::parseDoubleString(lineSplit[0], cellMatrix(i, 0)) ||
            !Common::parseDoubleString(lineSplit[1], cellMatrix(i, 1)) ||
            !Common::parseDoubleString(lineSplit[2], cellMatrix(i, 2))) {
          Common::error(QString("Could not read the cell matrix in SIESTA output! %1")
                       .arg(line.c_str()));
          return false;
        }
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
        fractionalCoords = false;
        angstromCoords = true;
      } else if (strstr(line.c_str(), "(Bohr)")) {
        fractionalCoords = false;
        angstromCoords = false;
      } else {
        Common::error("The atom coords have unrecognizable units.");
        Common::error("Please contact the developers of XtalOpt.");
        Common::error(QString("The faulty line is: %1").arg(line.c_str()));
        return false;
      }

      // Now let's add in the atoms!
      getline(ifs, line);
      line = Common::trim(line);
      // A blank line will be encountered at the end
      while (line.size() > 1) {
        lineSplit = Common::split(line, ' ');
        // A non-numeric first entry means the atom block has ended
        // (some SIESTA versions emit "Kpoints in: ..." without a blank line
        // separator after the last atom).
        if (lineSplit.empty() || lineSplit[0].empty() ||
            (!std::isdigit(static_cast<unsigned char>(lineSplit[0][0])) &&
             lineSplit[0][0] != '-' && lineSplit[0][0] != '+')) {
          break;
        }
        if (lineSplit.size() < 6) {
          Common::error(QString("Incomplete coords line in SIESTA output: %1")
                       .arg(line.c_str()));
          return false;
        }
        const unsigned int atomicNum = Atoms::ElementInfo::getAtomicNum(lineSplit[5]);
        if (atomicNum == 0) {
          Common::error(QString("Unrecognized element symbol in SIESTA output: %1")
                       .arg(lineSplit[5].c_str()));
          return false;
        }
        atomicNums.append(atomicNum);
        double x, y, z;
        if (!Common::parseDoubleString(lineSplit[0], x) ||
            !Common::parseDoubleString(lineSplit[1], y) ||
            !Common::parseDoubleString(lineSplit[2], z)) {
          Common::error(QString("Incomplete coords line in SIESTA output: %1")
                       .arg(line.c_str()));
          return false;
        }
        coords.append(Common::Vector3(x, y, z));
        getline(ifs, line);
        line = Common::trim(line);
      }
      coordsFound = true;
    }
  }

  if (!cellFound)
    Common::error("Cell info was not found in SIESTA output!");
  if (!coordsFound)
    Common::error("Atom coords not found in SIESTA output!");
  if (!cellFound || !coordsFound)
    return false;

  // Convert coords if we need to
  Atoms::UnitCell uc(cellMatrix);
  if (fractionalCoords) {
    for (int i = 0; i < coords.size(); ++i)
      coords[i] = uc.toCartesian(coords[i]);
  }
  // Assume we have Bohr coords
  else if (!fractionalCoords && !angstromCoords) {
    for (int i = 0; i < coords.size(); ++i)
      coords[i] *= 0.529177249; // Bohr to Angstrom
  }
  // Nothing to do if !fractionalCoords and angstromCoords

  std::vector<Atoms::Atom> atoms;
  atoms.reserve(coords.size());
  for (int i = 0; i < coords.size(); ++i) {
    atoms.push_back(Atoms::Atom(static_cast<unsigned short>(atomicNums.at(i)), coords.at(i)));
  }

  s.clear();
  s.setUnitCell(uc);
  s.setAtoms(atoms);
  return true;
}

/**
 * An excerpt from the SIESTA manual with regard to the molecule z-matrix
 * form:
 *
 * @begin_excerpt
 *
 * Nspecies i j k r a t ifr ifa ift
 * Here the values Nspecies, i, j, k, ifr, ifa, and ift are integers and r, a,
 * and t are double precision reals.
 * For most atoms, Nspecies is the species number of the atom, r is distance
 * to atom number i, a is the angle made by the present atom with atoms j and
 * i, while t is the torsional angle made by the present atom with atoms k,
 * j, and i. The values ifr, ifa and ift are integer flags that indicate
 * whether r, a, and t, respectively, should be varied; 0 for fixed, 1 for
 * varying.
 * The first three atoms in a molecule are a special case. Because there are
 * insufficient atoms defined to specify a distance/angle/torsion, the values
 * are set differently. For atom 1, r, a, and t, are the Cartesian
 * coordinates of the atom. For the second atom, r, a, and t are the
 * coordinates in spherical form of the second atom relative to the first:
 * first the radius, then the polar angle (angle between the z-axis and the
 * displacement vector) and then the azimuthal angle (angle between the
 * x-axis and the projection of the displacement vector on the x-y plane).
 * Finally, for the third atom, the numbers take their normal form, but the
 * torsional angle is defined relative to a notional atom 1 unit in the
 * z-direction above the atom j.
 *
 * @end_excerpt
 *
 * An example:
 * %block Zmatrix
 *
 * molecule
 * 1 0 0 0    0.0  0.0  0.0      0 0 0
 * 2 1 0 0    1.0 90.0 37.743919 1 0 0
 * 2 1 2 0    1.0  1.0 90.0      1 1 0
 * %endblock Zmatrix
 */
bool SiestaFormat::writeSiestaZMatrix(Atoms::Geometry& s, std::ostream& out, bool fixR, bool fixA,
                                      bool fixT, bool reorderAtomsToMatch)
{
  // We must perform a molecule wrap first
  s.wrapBondedComponentsToSmallestBonds();

  const std::vector<ZMatrixEntry> entries = generateZMatrixEntries(&s);

  if (entries.empty()) {
    Common::error("ZMatrix writer could not generate any entries. "
                  "This usually means the structure has no usable bond "
                  "connectivity for Z-matrix output.");
    return false;
  }

  const std::map<unsigned short, size_t> speciesNumbers = getSiestaSpeciesNumbers(s);

  // We will for sure use angstroms here
  out << "ZM.UnitsLength = Angstrom\n";
  out << "ZM.UnitsAngle = degrees\n";
  out << "%block Zmatrix\n";

  size_t numAtomsInPreviousMolecules = 0;
  for (size_t i = 0; i < entries.size(); ++i) {
    const ZMatrixEntry& entry = entries[i];
    if (i == 0 && entry.rInd != -1) {
      Common::error(QString("%1: the first entry should have no rInd, "
                            "angleInd, and dihedralInd.")
                      .arg(__func__));
      return false;
    }

    // Put in "molecule" if we do not have an rInd
    if (entry.rInd == -1) {
      numAtomsInPreviousMolecules = i;
      out << "\nmolecule\n";
    }

    // Each line looks like this:
    // Nspecies   i j k   r a t   ifr ifa ift
    // Where "Nspecies" is the species number
    // "i j k" are indices of the z-matrix atoms
    // "r a t" are the distance, angle, and torsion (dihedral) of the
    // z-matrix atoms
    // "ifr ifa ift" are whether or not to fix the distance, angle, and
    // torsion (dihedral) of the z-matrix atoms
    // The first line is a special case - r, a, and t are cartesian
    // coordinates for this atom.

    // First, the species number.
    out << std::setw(4) << speciesNumbers.at(s.atomicNumber(entry.ind))
        << "   ";

    // Next, all the indices of the atoms we are connected to
    out << std::setw(3)
        << (entry.rInd != -1
              ? indInEntries(entry.rInd, entries, numAtomsInPreviousMolecules) +
                  1
              : 0)
        << " ";
    out << std::setw(3)
        << (entry.angleInd != -1 ? indInEntries(entry.angleInd, entries,
                             numAtomsInPreviousMolecules) + 1 : 0)
        << " ";
    out << std::setw(3)
        << (entry.dihedralInd != -1 ? indInEntries(entry.dihedralInd, entries,
                             numAtomsInPreviousMolecules) + 1 : 0)
        << "   ";

    // If we don't have an rInd, fill in the Cartesian coordinates
    if (entry.rInd == -1) {
      out << " " << std::setw(15) << std::fixed << std::setprecision(8)
          << s.atom(entry.ind).pos()[0] << " "
          << std::setw(15) << std::fixed << std::setprecision(8)
          << s.atom(entry.ind).pos()[1] << " "
          << std::setw(15) << std::fixed << std::setprecision(8)
          << s.atom(entry.ind).pos()[2]
          // We will not fix these positions
          << "   1 1 1\n";
      continue;
    }

    // Put in the distance
    out << " " << std::setw(15) << std::fixed << std::setprecision(8)
        << s.distance(entry.ind, entry.rInd);

    // If we don't have an angleInd, put in spherical coordinates relative
    // to the first for r, a, t
    if (entry.angleInd == -1) {
      const Common::Vector3& pos1 = s.atom(entry.rInd).pos();
      const Common::Vector3& pos2 = s.atom(entry.ind).pos();
      // We need to use the minimum image here to take into account unit
      // cell boundaries
      const Common::Vector3 relPos = s.unitCell().minimumImage(pos2 - pos1);

      const double r = s.distance(entry.ind, entry.rInd);
      // Get the angles for the spherical coordinates
      const double angle1 = acos(relPos[2] / r) * RAD2DEG;
      const double angle2 = atan2(relPos[1], relPos[0]) * RAD2DEG;

      out << " " << std::setw(15) << std::fixed << std::setprecision(8)
          << angle1;
      out << " " << std::setw(15) << std::fixed << std::setprecision(8)
          << angle2;
      // We might fix the distance, but we will not fix the angles
      out << "   " << !fixR << " 1 1\n";
      continue;
    }

    out << " " << std::setw(15) << std::fixed << std::setprecision(8)
        << s.angle(entry.ind, entry.rInd, entry.angleInd);

    // If we don't have a dihedralInd, make a dihedral with a notional atom
    // 1 unit in the z-direction above the second atom
    if (entry.dihedralInd == -1) {
      const Common::Vector3& pos1 = s.atom(entry.ind).pos();
      const Common::Vector3& pos2 = s.atom(entry.rInd).pos();
      const Common::Vector3& pos3 = s.atom(entry.angleInd).pos();
      const Common::Vector3 pos4 = pos3 + Common::Vector3(0.00000, 0.00000, 1.00000);
      out << " " << std::setw(15) << std::fixed << std::setprecision(8)
          << s.dihedral(pos1, pos2, pos3, pos4);
      // We might fix the distance and the angle, but not the dihedral
      out << "   " << !fixR << " " << !fixA << " 1\n";
      continue;
    }

    out << " " << std::setw(15) << std::fixed << std::setprecision(8)
        << s.dihedral(entry.ind, entry.rInd, entry.angleInd, entry.dihedralInd);

    out << "   " << !fixR << " " << !fixA << " " << !fixT << "\n";
  }

  out << "%endblock Zmatrix\n\n";

  if (reorderAtomsToMatch)
    reorderAtomsToMatchZMatrix(s);

  return true;
}

QString SiestaFormat::writeSiestaZMatrixToString(Atoms::Geometry& s, bool fixR, bool fixA,
                                                 bool fixT, bool reorderAtomsToMatch)
{
  std::ostringstream out;
  if (!writeSiestaZMatrix(s, out, fixR, fixA, fixT, reorderAtomsToMatch))
    return QString();
  return QString::fromStdString(out.str());
}

} // namespace Atoms

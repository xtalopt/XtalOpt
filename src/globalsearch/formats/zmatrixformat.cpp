/**********************************************************************
  ZMatrixFormat -- A simple reader for ZMATRIX output.

  Copyright (C) 2017 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <globalsearch/formats/zmatrixformat.h>

#include <globalsearch/eleminfo.h>
#include <globalsearch/structure.h>
#include <globalsearch/utilities/utilityfunctions.h>

#include <fstream>
#include <iomanip>
#include <iostream>

#include <QDebug>
#include <QString>

using std::string;
using std::vector;

namespace GlobalSearch {

static bool parseInd(const vector<string>& words, size_t numAtoms, int index,
                     long& result)
{
  result = atol(words[index].c_str()) - 1;

  if (result < 0 || result + 1 > numAtoms) {
    qDebug() << "Error in Z-matrix reader: invalid index read:"
             << words[index].c_str();
    return false;
  }
  return true;
}

static inline bool parseRInd(const vector<string>& words, size_t numAtoms,
                             long& result)
{
  return parseInd(words, numAtoms, 1, result);
}

static inline bool parseThetaInd(const vector<string>& words, size_t numAtoms,
                                 long& result)
{
  return parseInd(words, numAtoms, 3, result);
}

static inline bool parsePhiInd(const vector<string>& words, size_t numAtoms,
                               long& result)
{
  return parseInd(words, numAtoms, 5, result);
}

static bool parseR(const vector<string>& words, double& result)
{
  result = atof(words[2].c_str());
  if (result < 0.0) {
    qDebug() << "Error in Z-matrix reader: invalid bond length read: "
             << words[2].c_str();
    return false;
  }
  return true;
}

static bool parseAngle(const vector<string>& words, int index, double& result)
{
  result = atof(words[index].c_str());
  if (result < -180.0 || result > 180.0) {
    qDebug() << "Error in Z-matrix reader: invalid angle read: "
             << words[index].c_str();
    return false;
  }
  // Convert to radians
  result = deg2rad(result);
  return true;
}

static inline bool parseTheta(const vector<string>& words, double& result)
{
  return parseAngle(words, 4, result);
}

static inline bool parsePhi(const vector<string>& words, double& result)
{
  return parseAngle(words, 6, result);
}

static string concatenateLine(const vector<string>& list)
{
  string s;
  for (const auto& word : list)
    s += word + " ";
  return s;
}

static bool parseLine(const vector<string>& words, std::vector<Atom>& atoms)
{

  if (atoms.empty()) {
    // First atom
    if (words.size() != 1) {
      qDebug() << "Error in Z-matrix reader: failed to read first line in"
               << "the z-matrix:" << concatenateLine(words).c_str();
      qDebug() << "The first line should only contain an atomic symbol";
      return false;
    }

    Atom newAtom(ElemInfo::getAtomicNum(words[0]), Vector3(0.0, 0.0, 0.0));
    atoms.push_back(newAtom);
  } else if (atoms.size() == 1) {
    // Second atom. Should only have an R value.
    if (words.size() < 3) {
      qDebug() << "Error in Z-matrix reader: failed to read second line in"
               << "the z-matrix:" << concatenateLine(words).c_str();
      qDebug() << "The second atom must have an index and a distance to"
               << "the first atom.";
      return false;
    }

    long ind;
    if (!parseRInd(words, atoms.size(), ind)) {
      qDebug() << "Error occurred in this line: "
               << concatenateLine(words).c_str();
      return false;
    }

    double r;
    if (!parseR(words, r)) {
      qDebug() << "Error occurred in this line: "
               << concatenateLine(words).c_str();
      return false;
    }

    Atom newAtom(ElemInfo::getAtomicNum(words[0]), Vector3(0.0, 0.0, r));
    atoms.push_back(newAtom);
  } else if (atoms.size() == 2) {
    // Third atom. Should have R and theta.
    if (words.size() < 5) {
      qDebug() << "Error in Z-matrix reader: failed to read third line in"
               << "the z-matrix:" << concatenateLine(words).c_str();
      qDebug() << "The third atom must have a defined distance and angle.";
      return false;
    }

    long rInd;
    if (!parseRInd(words, atoms.size(), rInd)) {
      qDebug() << "Error occurred in this line: "
               << concatenateLine(words).c_str();
      return false;
    }

    double r;
    if (!parseR(words, r)) {
      qDebug() << "Error occurred in this line: "
               << concatenateLine(words).c_str();
      return false;
    }

    long thetaInd;
    if (!parseThetaInd(words, atoms.size(), thetaInd)) {
      qDebug() << "Error occurred in this line: "
               << concatenateLine(words).c_str();
      return false;
    }

    double theta;
    if (!parseTheta(words, theta)) {
      qDebug() << "Error occurred in this line: "
               << concatenateLine(words).c_str();
      return false;
    }

    if (rInd == thetaInd) {
      qDebug() << "Error in Z-matrix reader: cannot use the same reference"
               << "atom for both the distance and the angle!";
      qDebug() << "Error occurred in this line: "
               << concatenateLine(words).c_str();
      return false;
    }

    Vector3 coords;
    coords[0] = r * sin(theta);
    coords[1] = 0.0;
    coords[2] =
      (thetaInd == 0) ? atoms[1].pos()[2] - r * cos(theta) : r * cos(theta);

    Atom newAtom(ElemInfo::getAtomicNum(words[0]), coords);
    atoms.push_back(newAtom);
  } else {
    // General atom. Should have R, theta, and phi.
    if (words.size() < 7) {
      qDebug() << "Error in Z-matrix reader: failed to read a line in"
               << "the z-matrix:" << concatenateLine(words).c_str();
      qDebug() << "The line should contain at least 7 separate words.";
      return false;
    }

    long rInd;
    if (!parseRInd(words, atoms.size(), rInd)) {
      qDebug() << "Error occurred in this line: "
               << concatenateLine(words).c_str();
      return false;
    }

    double r;
    if (!parseR(words, r)) {
      qDebug() << "Error occurred in this line: "
               << concatenateLine(words).c_str();
      return false;
    }

    long thetaInd;
    if (!parseThetaInd(words, atoms.size(), thetaInd)) {
      qDebug() << "Error occurred in this line: "
               << concatenateLine(words).c_str();
      return false;
    }

    double theta;
    if (!parseTheta(words, theta)) {
      qDebug() << "Error occurred in this line: "
               << concatenateLine(words).c_str();
      return false;
    }

    long phiInd;
    if (!parsePhiInd(words, atoms.size(), phiInd)) {
      qDebug() << "Error occurred in this line: "
               << concatenateLine(words).c_str();
      return false;
    }

    double phi;
    if (!parsePhi(words, phi)) {
      qDebug() << "Error occurred in this line: "
               << concatenateLine(words).c_str();
      return false;
    }

    if (rInd == thetaInd || rInd == phiInd || thetaInd == phiInd) {
      qDebug() << "Error in Z-matrix reader: cannot use the same reference"
               << "atom twice in one line!";
      qDebug() << "Error occurred in this line: "
               << concatenateLine(words).c_str();
      return false;
    }

    // Reference coords
    Vector3 aPos(atoms[phiInd].pos());
    Vector3 bPos(atoms[thetaInd].pos());
    Vector3 cPos(atoms[rInd].pos());

    Vector3 x1((bPos - aPos).normalized());
    Vector3 x2((cPos - bPos).normalized());

    Vector3 u(x1 - x1.dot(x2) * x2);
    double uNorm = u.norm();
    if (uNorm < 1e-5) {
      qDebug() << "Error in Z-matrix reader: dihedral atoms on the same"
               << "line!";
      qDebug() << "Error occurred in this line: "
               << concatenateLine(words).c_str();
      return false;
    }
    u /= uNorm;
    Vector3 v(u.cross(x2));

    // Calculate the position
    Vector3 pos(cPos);
    pos -= r * (cos(theta) * x2 + sin(theta) * (cos(phi) * u - sin(phi) * v));

    // Add the atom in
    Atom newAtom(ElemInfo::getAtomicNum(words[0]), pos);
    atoms.push_back(newAtom);
  }

  return true;
}

bool ZMatrixFormat::read(Structure* s, std::istream& in)
{
  string line;

  // We need to store the lines so we can process the variables later
  // Ignore the first line. It is just a title.
  getline(in, line);

  // First, store the actual z-matrix without any variables replaced.
  vector<vector<string>> zMat;
  while (getline(in, line)) {
    line = trim(line);
    if (!line.empty())
      zMat.push_back(split(line, ' '));
    else
      break;
  }

  // Now store the variables
  vector<string> vars;
  while (getline(in, line)) {
    line = trim(line);
    if (!line.empty())
      vars.push_back(line);
    else
      break;
  }

  // Replace the variables in the z-matrix
  for (const auto& var : vars) {
    vector<string> lineSplit = split(var, '=');
    for (auto& line : lineSplit)
      line = trim(line);

    if (lineSplit.size() != 2) {
      qDebug() << "Error in Z-Matrix reader: Error parsing variables"
               << "on this line: " << var.c_str();
      return false;
    }

    string name = trim(lineSplit[0]);
    string val = trim(lineSplit[1]);
    // Find this name in the z-matrix and replace it
    for (auto& line : zMat) {
      if (line.size() >= 3 && caseInsensitiveCompare(line[2], name))
        line[2] = val;
      if (line.size() >= 5 && caseInsensitiveCompare(line[4], name))
        line[4] = val;
      if (line.size() >= 7 && caseInsensitiveCompare(line[6], name))
        line[6] = val;
    }
  }

  vector<Atom> atoms;

  // Now that the variables have been replaced, parse the z-matrix
  for (const auto& line : zMat) {
    if (!parseLine(line, atoms))
      return false;
  }

  // We are going to use the z-matrix to generate bonding information
  // unless there are dummy atoms. If there are dummy atoms, we will
  // use bond perception
  bool containsDummyAtoms = false;

  // Get rid of the dummy atoms
  for (size_t i = 0; i < atoms.size(); ++i) {
    if (atoms[i].atomicNumber() == 0) {
      containsDummyAtoms = true;
      atoms.erase(atoms.begin() + i);
      --i;
    }
  }

  // Now set the atoms!
  s->setAtoms(atoms);

  // Let's figure out how we are going to set the bonding information
  if (containsDummyAtoms) {
    // Bonding information will not be correct. Just perceive the bonds
    s->perceiveBonds();
  } else {
    // We can use the bonding information from the z-matrix
    for (size_t i = 1; i < zMat.size(); ++i) {
      long ind;
      if (!parseRInd(zMat[i], i, ind)) {
        // This really should never happen, but just in case...
        qDebug() << "Error setting bonding information from the z-matrix.";
        qDebug() << "Using bond perception instead";
        s->clearBonds();
        s->perceiveBonds();
        break;
      }
      s->addBond(ind, i);
    }
  }

  /* // DEBUG SECTION
  qDebug() << "atom positions are as follows: ";
  for (const auto& atom: atoms)
    qDebug() << ElemInfo::getAtomicSymbol(atom.atomicNumber()).c_str()
             << atom.pos()[0] << atom.pos()[1] << atom.pos()[2];

  qDebug() << "Bonds are: "; // TMP
  for (const auto& bond: s->bonds()) // TMP
    qDebug() << bond.first() << bond.second(); // TMP
  */

  return true;
}

template <typename T>
bool alreadyInList(const std::vector<T>& v, const T& item)
{
  for (const auto& elem : v) {
    if (elem == item)
      return true;
  }
  return false;
}

inline long long indInEntries(long long ind,
                              const std::vector<ZMatrixEntry>& entries,
                              size_t numAtomsInPreviousMolecules)
{
  for (size_t i = 0; i < entries.size(); ++i) {
    if (entries[i].ind == ind)
      return i - numAtomsInPreviousMolecules;
  }
  return -1;
}

std::map<unsigned short, size_t> getSiestaSpeciesNumbers(const Structure& s)
{
  std::map<unsigned short, size_t> ret;

  // We are going to store the species numbers in alphabetical order
  QStringList atomicSyms = s.getSymbols();

  for (size_t i = 0; i < atomicSyms.size(); ++i)
    ret[ElemInfo::getAtomicNum(atomicSyms[i].toStdString())] = i + 1;

  return ret;
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
bool ZMatrixFormat::writeSiestaZMatrix(Structure& s, std::ostream& out,
                                       bool fixR, bool fixA, bool fixT)
{
  // We must perform a molecule wrap first
  s.wrapMoleculesToSmallestBonds();

  std::vector<ZMatrixEntry> entries = generateZMatrixEntries(&s);

  if (entries.empty())
    return false;

  std::map<unsigned short, size_t> speciesNumbers = getSiestaSpeciesNumbers(s);

  // We will for sure use angstroms here
  out << "ZM.UnitsLength = Angstrom\n";
  out << "ZM.UnitsAngle = degrees\n";
  out << "%block Zmatrix\n";

  size_t numAtomsInPreviousMolecules = 0;
  for (size_t i = 0; i < entries.size(); ++i) {
    const auto& entry = entries[i];
    if (i == 0 && entry.rInd != -1) {
      std::cerr << "Error in " << __FUNCTION__
                << ": the first entry should have no rInd, angleInd, and "
                << "dihedralInd.\n";
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
    out << std::setw(4) << speciesNumbers[s.atomicNumber(entry.ind)] << "   ";

    // Next, all the indices of the atoms we are connected to
    out << std::setw(3)
        << (entry.rInd != -1
              ? indInEntries(entry.rInd, entries, numAtomsInPreviousMolecules) +
                  1
              : 0)
        << " ";
    out << std::setw(3) << (entry.angleInd != -1
                              ? indInEntries(entry.angleInd, entries,
                                             numAtomsInPreviousMolecules) +
                                  1
                              : 0)
        << " ";
    out << std::setw(3) << (entry.dihedralInd != -1
                              ? indInEntries(entry.dihedralInd, entries,
                                             numAtomsInPreviousMolecules) +
                                  1
                              : 0)
        << "   ";

    // If we don't have an rInd, fill in the Cartesian coordinates
    if (entry.rInd == -1) {
      out << " " << std::setw(15) << std::fixed << std::setprecision(8)
          << s.atom(entry.ind).pos()[0] << " " << std::setw(15) << std::fixed
          << std::setprecision(8) << s.atom(entry.ind).pos()[1] << " "
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
      const Vector3& pos1 = s.atom(entry.rInd).pos();
      const Vector3& pos2 = s.atom(entry.ind).pos();

      // We need to use the minimum image here to take into account unit
      // cell boundaries
      Vector3 relPos = s.unitCell().minimumImage(pos2 - pos1);

      double r = s.distance(entry.ind, entry.rInd);
      // Get the angles for the spherical coordinates
      double angle1 = acos(relPos[2] / r) * RAD2DEG;

      double angle2 = atan2(relPos[1], relPos[0]) * RAD2DEG;

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
      const Vector3& pos1 = s.atom(entry.ind).pos();
      const Vector3& pos2 = s.atom(entry.rInd).pos();
      const Vector3& pos3 = s.atom(entry.angleInd).pos();
      const Vector3& pos4 = pos3 + Vector3(0.00000, 0.00000, 1.00000);
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

  return true;
}

bool ZMatrixFormat::write(const Structure& s, std::ostream& out)
{
  std::vector<ZMatrixEntry> entries = generateZMatrixEntries(&s);

  if (entries.empty())
    return false;

  size_t moleculeCounter = 1;
  size_t numAtomsInPreviousMolecules = 0;
  for (size_t i = 0; i < entries.size(); ++i) {
    const auto& entry = entries[i];
    if (i == 0 && entry.rInd != -1) {
      std::cerr << "Error in " << __FUNCTION__
                << ": the first entry should have no rInd, angleInd, and "
                << "dihedralInd.\n";
      return false;
    }

    // Put in a title if we do not have an rInd
    if (entry.rInd == -1) {
      out << "molecule " << moleculeCounter << "\n";
      ++moleculeCounter;
      numAtomsInPreviousMolecules = i;
    }

    // First, the atomic symbol.
    out << std::setw(2) << ElemInfo::getAtomicSymbol(s.atomicNumber(entry.ind));

    // If we don't have an rInd, end the line and continue
    if (entry.rInd == -1) {
      out << "\n";
      continue;
    }

    out << " " << std::setw(3)
        << indInEntries(entry.rInd, entries, numAtomsInPreviousMolecules) + 1
        << " " << std::setw(15) << std::fixed << std::setprecision(8)
        << s.distance(entry.ind, entry.rInd);

    // If we don't have an angleInd, end the line and continue
    if (entry.angleInd == -1) {
      out << "\n";
      continue;
    }

    out << " " << std::setw(3)
        << indInEntries(entry.angleInd, entries, numAtomsInPreviousMolecules) +
             1
        << " " << std::setw(15) << std::fixed << std::setprecision(8)
        << s.angle(entry.ind, entry.rInd, entry.angleInd);

    // If we don't have a dihedralInd, end the line and continue
    if (entry.dihedralInd == -1) {
      out << "\n";
      continue;
    }

    out << " " << std::setw(3)
        << indInEntries(entry.dihedralInd, entries,
                        numAtomsInPreviousMolecules) +
             1
        << " " << std::setw(15) << std::fixed << std::setprecision(8)
        << s.dihedral(entry.ind, entry.rInd, entry.angleInd, entry.dihedralInd)
        << "\n";
  }
  return true;
}

std::vector<ZMatrixEntry> ZMatrixFormat::generateZMatrixEntries(
  const Structure* s)
{
  std::vector<ZMatrixEntry> ret;

  const std::vector<unsigned short>& atomicNums = s->atomicNumbers();
  std::vector<bool> atomAlreadyUsed(atomicNums.size(), false);

  // Let's make a priority list for atom selection
  std::vector<int> priorityList(atomicNums.size(), 1);

  // Carbon gets 2. Hydrogen gets 0. Everything else gets 1.
  for (size_t i = 0; i < atomicNums.size(); ++i) {
    if (atomicNums[i] == 6)
      priorityList[i] = 2;
    else if (atomicNums[i] == 1)
      priorityList[i] = 0;
  }

  // We'll keep doing this until we run out of molecules
  while (true) {
    // Find one that hasn't been used yet. If there aren't any, break
    auto it = std::find(atomAlreadyUsed.begin(), atomAlreadyUsed.end(), false);

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
    if (bondedAtoms.empty())
      continue;

    // Make sure none of these have been used. If they have been, report
    // an error, because that shouldn't be possible.
    for (const auto& bondedAtom : bondedAtoms) {
      if (atomAlreadyUsed[bondedAtom]) {
        std::cerr << "Error in " << __FUNCTION__ << ": an atom bonded to "
                  << "the first atom in this molecule has already been "
                  << "used!\nThis error should not be possible. Please "
                  << "contact a developer.\n";
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
    if (bondedAtoms.empty())
      continue;

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
    if (bondedAtoms.empty())
      continue;

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
        std::cerr << "Error in " << __FUNCTION__ << ": no atom was found "
                  << "that is bonded to atom index " << entry.ind << "\n"
                  << "This error should not be possible. Please contact "
                  << "a developer of this program.\n";
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
        std::cerr << "Error(2) in " << __FUNCTION__ << ": no atom was found "
                  << "that is bonded to atom index " << entry.rInd << "\n"
                  << "This error should not be possible. Please contact "
                  << "a developer of this program.\n";
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

void ZMatrixFormat::reorderAtomsToMatchZMatrix(Structure& s)
{
  std::vector<ZMatrixEntry> entries = generateZMatrixEntries(&s);

  std::vector<size_t> newOrder;
  for (const auto& entry : entries)
    newOrder.push_back(entry.ind);

  s.reorderAtoms(newOrder);
}
}

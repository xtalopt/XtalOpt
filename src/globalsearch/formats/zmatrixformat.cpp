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
#include <globalsearch/utilities/utilityfunctions.h>
#include <globalsearch/structure.h>

#include <fstream>

#include <QDebug>
#include <QString>

using std::string;
using std::vector;

namespace GlobalSearch {

  static bool parseInd(const vector<string>& words,
                       size_t numAtoms,
                       int index,
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

  static inline bool parseRInd(const vector<string>& words,
                               size_t numAtoms,
                               long& result)
  {
    return parseInd(words, numAtoms, 1, result);
  }

  static inline bool parseThetaInd(const vector<string>& words,
                                   size_t numAtoms,
                                   long& result)
  {
    return parseInd(words, numAtoms, 3, result);
  }

  static inline bool parsePhiInd(const vector<string>& words,
                                 size_t numAtoms,
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

  static bool parseAngle(const vector<string>& words, int index,
                         double& result)
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
    for (const auto& word: list)
      s += word + " ";
    return s;
  }

  static bool parseLine(const vector<string>& words,
                        std::vector<Atom>& atoms)
  {

    if (atoms.empty()) {
      // First atom
      if (words.size() != 1) {
        qDebug() << "Error in Z-matrix reader: failed to read first line in"
                 << "the z-matrix:" << concatenateLine(words).c_str();
        qDebug() << "The first line should only contain an atomic symbol";
        return false;
      }

      Atom newAtom(ElemInfo::getAtomicNum(words[0]),
                   Vector3(0.0, 0.0, 0.0));
      atoms.push_back(newAtom);
    }
    else if (atoms.size() == 1) {
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

      Atom newAtom(ElemInfo::getAtomicNum(words[0]),
                   Vector3(0.0, 0.0, r));
      atoms.push_back(newAtom);
    }
    else if (atoms.size() == 2) {
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
      coords[2] = (thetaInd == 0) ? atoms[1].pos()[2] - r * cos(theta) :
                                    r * cos(theta);

      Atom newAtom(ElemInfo::getAtomicNum(words[0]),
                   coords);
      atoms.push_back(newAtom);
    }
    else {
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
      pos -= r * (cos(theta) * x2 +
                  sin(theta) * (cos(phi) * u -
                  sin(phi) * v));

      // Add the atom in
      Atom newAtom(ElemInfo::getAtomicNum(words[0]),
                   pos);
      atoms.push_back(newAtom);
    }

    return true;
  }

  bool ZMatrixFormat::read(Structure* s, const QString& filename)
  {
    std::ifstream ifs(filename.toStdString());
    if (!ifs) {
      qDebug() << "Error: ZMATRIX output, " << filename << ", could not "
               << "be opened!";
      return false;
    }

    string line;

    // We need to store the lines so we can process the variables later
    // Ignore the first line. It is just a title.
    getline(ifs, line);

    // First, store the actual z-matrix without any variables replaced.
    vector<vector<string>> zMat;
    while (getline(ifs, line)) {
      line = trim(line);
      if (!line.empty())
        zMat.push_back(split(line, ' '));
      else
        break;
    }

    // Now store the variables
    vector<string> vars;
    while (getline(ifs, line)) {
      line = trim(line);
      if (!line.empty())
        vars.push_back(line);
      else
        break;
    }

    // Replace the variables in the z-matrix
    for (const auto& var: vars) {
      vector<string> lineSplit = split(var, '=');
      for (auto& line: lineSplit)
        line = trim(line);

      if (lineSplit.size() != 2) {
        qDebug() << "Error in Z-Matrix reader: Error parsing variables"
                 << "on this line: " << var.c_str();
        return false;
      }

      string name = trim(lineSplit[0]);
      string val = trim(lineSplit[1]);
      // Find this name in the z-matrix and replace it
      for (auto& line: zMat) {
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
    for (const auto& line: zMat) {
      if (!parseLine(line, atoms))
        return false;
    }

    // Get rid of the dummy atoms
    for (size_t i = 0; i < atoms.size(); ++i) {
      if (atoms[i].atomicNumber() == 0) {
        atoms.erase(atoms.begin() + i);
        --i;
      }
    }

    // Now set the atoms!
    s->setAtoms(atoms);
    s->perceiveBonds();

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
}

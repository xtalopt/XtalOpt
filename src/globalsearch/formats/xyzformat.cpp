/**********************************************************************
  XyzFormat -- A simple reader for XYZ files.

  Copyright (C) 2017 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <globalsearch/formats/xyzformat.h>

#include <globalsearch/eleminfo.h>
#include <globalsearch/structure.h>
#include <globalsearch/utilities/utilityfunctions.h>

#include <fstream>

#include <QDebug>
#include <QString>

namespace GlobalSearch {

bool XyzFormat::read(Structure* s, const QString& filename)
{
  std::ifstream ifs(filename.toStdString());
  if (!ifs) {
    qDebug() << "Error: XYZ file, " << filename << ", could not "
             << "be opened!";
    return false;
  }

  std::string line;
  std::vector<std::string> lineSplit;
  std::vector<Atom> atoms;

  getline(ifs, line);

  // We need a while loop here because we may have multiple molecules
  while (true) {
    // First line should be the number of atoms
    lineSplit = split(line, ' ');
    if (lineSplit.size() != 1) {
      qDebug() << "Error in XYZ reader: the line containing the number of"
               << "atoms must only contain one word: the number of atoms!";
      qDebug() << "Error occurred on this line: " << line.c_str();
      return false;
    }

    size_t numAtoms = atol(lineSplit[0].c_str());

    // Next line is just a comment line
    getline(ifs, line);

    // Now, let's read a number of times equal to numAtoms!
    for (size_t i = 0; i < numAtoms; ++i) {
      if (!getline(ifs, line)) {
        qDebug() << "Error in XYZ reader: the end of file was reached"
                 << "before we finished reading the number of atoms: "
                 << numAtoms << "!";
        return false;
      }

      lineSplit = split(line, ' ');
      if (lineSplit.size() != 4) {
        qDebug() << "Error in XYZ reader: each atom coordinate line should"
                 << "contain exactly 4 words: <element> <x> <y> <z>";
        qDebug() << "Error occurred on this line: " << line.c_str();
        return false;
      }

      // If it is a number, it is the atomic number. Otherwise, a symbol
      int atomicNum = 0;
      if (isNumber(lineSplit[0])) {
        atomicNum = atoi(lineSplit[0].c_str());
        if (atomicNum < 1 || atomicNum > 255) {
          qDebug() << "Error in XYZ reader: Invalid atomic number entered.";
          qDebug() << "Error occurred on this line: " << line.c_str();
          return false;
        }
      } else {
        atomicNum = ElemInfo::getAtomicNum(lineSplit[0]);
        if (atomicNum < 1 || atomicNum > 255) {
          qDebug() << "Error in XYZ reader: Invalid atomic symbol entered.";
          qDebug() << "Error occurred on this line: " << line.c_str();
          return false;
        }
      }
      Atom newAtom(atomicNum, Vector3(atof(lineSplit[1].c_str()),
                                      atof(lineSplit[2].c_str()),
                                      atof(lineSplit[3].c_str())));
      atoms.push_back(newAtom);
    }

    // If the next line has exactly one word, assume it is the start of
    // a new molecule. Otherwise, break.
    if (!getline(ifs, line))
      break;
    line = trim(line);
    if (line.empty())
      break;

    lineSplit = split(line, ' ');
    if (lineSplit.size() != 1)
      break;

    // Otherwise, just keep on going.
  }
  s->setAtoms(atoms);
  s->perceiveBonds();

  /* // DEBUG SECTION
  qDebug() << "Atoms are: "; // TMP
  for (const auto& atom: atoms) // TMP
    qDebug() << ElemInfo::getAtomicSymbol(atom.atomicNumber()).c_str() // TMP
             << atom.pos()[0] << atom.pos()[1] << atom.pos()[2]; // TMP

  qDebug() << "Bonds are: "; // TMP
  for (const auto& bond: s->bonds()) // TMP
    qDebug() << bond.first() << bond.second(); // TMP
  */

  return true;
}
}

/**********************************************************************
  elemInfo - Contains functions for getting atomic radii and atomic
             symbols and for setting atomic radii.

  Copyright (C) 2015 - 2016 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <atoms/eleminfo.h>

#include <common/constants.h>
#include <common/output.h>
#include <common/stringutils.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <QString>

using namespace std;

namespace Atoms {

// Set the static member variables
vector<string> ElementInfo::atomicSymbols = ElementInfoDatabase::_atomicSymbols;
vector<double> ElementInfo::masses = ElementInfoDatabase::_masses;
vector<double> ElementInfo::covalentRadii = ElementInfoDatabase::_covalentRadii;
vector<double> ElementInfo::vdwRadii = ElementInfoDatabase::_vdwRadii;

unsigned int ElementInfo::getNumberOfElements()
{
  // Return number of entries in symbol, mass, and radii list
  //   that are defined for real elements.
  // This assumes the lists are always maintain the same length!
  return atomicSymbols.size() - 1;
}

std::string ElementInfo::getAtomicSymbol(unsigned int atomicNum)
{
  if (atomicNum == 0 || atomicNum >= atomicSymbols.size()) {
    Common::error(QString("%1: invalid atomicNum, %2.")
                 .arg(__func__)
                 .arg(atomicNum));
    return "";
  }
  return atomicSymbols[atomicNum];
}

double ElementInfo::getAtomicMass(unsigned int atomicNum)
{
  if (atomicNum == 0 || atomicNum >= masses.size()) {
    Common::error(QString("%1: invalid atomicNum, %2.")
                 .arg(__func__)
                 .arg(atomicNum));
    return 0;
  }
  return masses[atomicNum];
}

double ElementInfo::getAverageAtomicMass()
{
  return ElementInfoDatabase::_avgAtomicMass;
}

unsigned int ElementInfo::getAtomicNum(std::string symbol)
{
  for (unsigned int i = 0; i < atomicSymbols.size(); i++) {
    if (Common::caseInsensitiveCompare(atomicSymbols[i], symbol))
      return i;
  }

  return 0;
}

bool ElementInfo::readComposition(string compStr, map<unsigned int, unsigned int>& comp)
{
  // As a "map", this always returns entries sorted by atomic number (first entry).
  compStr = Common::removeSpaces(compStr);
  comp.clear();

  vector<string> symbols = Common::reSplit(compStr, "[0-9]");
  vector<string> countsStr = Common::reSplit(compStr, "[A-Za-z]");

  if (symbols.size() != countsStr.size()) {
    Common::error(QString("Invalid composition '%1'. Symbols should be "
                         "followed by a number!")
                   .arg(compStr.c_str()));
    return false;
  }

  vector<unsigned int> counts;
  counts.reserve(countsStr.size());
  for (size_t i = 0; i < countsStr.size(); ++i) {
    const string& countStr = countsStr[i];
    if (countStr.empty() ||
        !all_of(countStr.begin(), countStr.end(), [](unsigned char c) {
          return std::isdigit(c);
        })) {
      Common::error(QString("Could not read numbers in composition, %1")
                   .arg(compStr.c_str()));
      Common::error("Check your input and try again");
      return false;
    }

    try {
      const unsigned long count = stoul(countStr);
      if (count == 0 || count > numeric_limits<unsigned int>::max()) {
        Common::error(QString("Could not read numbers in composition, %1")
                     .arg(compStr.c_str()));
        Common::error("Check your input and try again");
        return false;
      }
      counts.push_back(static_cast<unsigned int>(count));
    } catch (const std::exception&) {
      Common::error(QString("Could not read numbers in composition, %1")
                   .arg(compStr.c_str()));
      Common::error("Check your input and try again");
      return false;
    }
  }

  for (size_t i = 0; i < symbols.size(); ++i) {
    int atomicNum = getAtomicNum(symbols[i]);
    if (atomicNum == 0) {
      Common::error(QString("Invalid elemental symbol, %1, was entered in the "
                           "composition, %2\n"
                           "Note: every symbol must be followed by a number "
                           "(i.e., Ti1O2)")
                   .arg(symbols[i].c_str())
                   .arg(compStr.c_str()));
      return false;
    }
    const unsigned int oldCount = comp[atomicNum];
    if (counts[i] > numeric_limits<unsigned int>::max() - oldCount) {
      Common::error(QString("Composition count is too large in %1")
                    .arg(compStr.c_str()));
      return false;
    }
    comp[atomicNum] = oldCount + counts[i];
  }

  return true;
}

bool ElementInfo::readComposition(const string& comp, vector<unsigned int>& atoms)
{
  atoms.clear();
  map<unsigned int, unsigned int> compMap;
  if (!readComposition(comp, compMap))
    return false;

  for (const auto& elem : compMap) {
    for (unsigned int i = 0; i < elem.second; ++i)
      atoms.push_back(elem.first);
  }

  return true;
}

double ElementInfo::getVdwRadius(unsigned int atomicNum)
{
  if (atomicNum == 0 || atomicNum >= vdwRadii.size()) {
    Common::error(QString("%1: invalid atomicNum, %2.")
                 .arg(__func__)
                 .arg(atomicNum));
    return 0;
  }
  return vdwRadii[atomicNum];
}

double ElementInfo::getAverageVdwRadius()
{
  return ElementInfoDatabase::_avgAtomicVdwRadius;
}

double ElementInfo::getVdwVolume(unsigned int atomicNum)
{
  if (atomicNum == 0 || atomicNum >= vdwRadii.size()) {
    Common::error(QString("%1: invalid atomicNum, %2.")
                 .arg(__func__)
                 .arg(atomicNum));
    return 0;
  }
  return 4.0 * PI * pow(vdwRadii[atomicNum], 3.0) / 3.0;
}

double ElementInfo::getCovalentRadius(unsigned int atomicNum)
{
  if (atomicNum == 0 || atomicNum >= covalentRadii.size()) {
    Common::error(QString("%1: invalid atomicNum, %2.")
                 .arg(__func__)
                 .arg(atomicNum));
    return 0;
  }
  return covalentRadii[atomicNum];
}

double ElementInfo::getAverageCovalentRadius()
{
  return ElementInfoDatabase::_avgAtomicCovRadius;
}

double ElementInfo::getCovalentVolume(unsigned int atomicNum)
{
  if (atomicNum == 0 || atomicNum >= covalentRadii.size()) {
    Common::error(QString("%1: invalid atomicNum, %2.")
                 .arg(__func__)
                 .arg(atomicNum));
    return 0;
  }
  return 4.0 * PI * pow(covalentRadii[atomicNum], 3.0) / 3.0;
}

void ElementInfo::applyScalingFactor(double sf)
{
  for (size_t i = 1; i < covalentRadii.size(); i++) {
    covalentRadii[i] = ElementInfoDatabase::_covalentRadii[i] * sf;
    vdwRadii[i] = ElementInfoDatabase::_vdwRadii[i] * sf;
  }
}

// We allow the user to set a radius here
// We will set both radii since we know the user will only be using one of
// them, but we don't know which...
void ElementInfo::setRadius(unsigned int atomicNum, double newRadius)
{
  if (atomicNum == 0 || atomicNum >= covalentRadii.size()) {
    Common::error(QString("%1: invalid atomicNum, %2.")
                 .arg(__func__)
                 .arg(atomicNum));
    return;
  }

  if (newRadius < 0) {
    Common::error(QString("%1: negative radius, '%2'.")
                 .arg(__func__)
                 .arg(newRadius));
    return;
  }

  covalentRadii[atomicNum] = newRadius;
  vdwRadii[atomicNum] = newRadius;
}

void ElementInfo::setMinRadius(double minRadius)
{
  for (size_t i = 1; i < covalentRadii.size(); i++) {
    if (covalentRadii[i] < minRadius)
      covalentRadii[i] = minRadius;
    if (vdwRadii[i] < minRadius)
      vdwRadii[i] = minRadius;
  }
}

double ElementInfo::getRadius(unsigned int atomicNum, bool usingVdwRadius)
{
  if (usingVdwRadius)
    return getVdwRadius(atomicNum);
  else
    return getCovalentRadius(atomicNum);
}

} // namespace Atoms

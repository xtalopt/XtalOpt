/**********************************************************************
  elemInfo.cpp - Contains functions for getting atomic radii and atomic symbols
                 and for setting atomic radii.

  Copyright (C) 2015 - 2016 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#include <globalsearch/eleminfo.h>
#include <globalsearch/utilities/utilityfunctions.h>

#include <algorithm>
#include <iostream>

using namespace std;

// Set the static member variables
vector<string> ElemInfo::atomicSymbols = ElemInfoDatabase::_atomicSymbols;
vector<double> ElemInfo::masses = ElemInfoDatabase::_masses;
vector<double> ElemInfo::covalentRadii = ElemInfoDatabase::_covalentRadii;
vector<double> ElemInfo::vdwRadii = ElemInfoDatabase::_vdwRadii;

std::string ElemInfo::getAtomicSymbol(uint atomicNum)
{
  if (atomicNum == 0 || atomicNum > 117) {
    std::cout << "Error: Invalid atomicNum, " << atomicNum
              << ", was entered in " << __FUNCTION__ << "!\n";
    return 0;
  }
  return atomicSymbols[atomicNum];
}

double ElemInfo::getAtomicMass(uint atomicNum)
{
  if (atomicNum == 0 || atomicNum > 117) {
    std::cout << "Error: Invalid atomicNum, " << atomicNum
              << ", was entered in " << __FUNCTION__ << "!\n";
    return 0;
  }
  return masses[atomicNum];
}

uint ElemInfo::getAtomicNum(std::string symbol)
{
  for (uint i = 0; i < atomicSymbols.size(); i++) {
    if (caseInsensitiveCompare(atomicSymbols[i], symbol))
      return i;
  }

  return 0;
}

bool ElemInfo::readComposition(string compStr, map<uint, uint>& comp)
{
  compStr = removeSpaces(compStr);
  comp.clear();

  vector<string> symbols = reSplit(compStr, "[0-9]");
  vector<string> countsStr = reSplit(compStr, "[A-Za-z]");

  if (symbols.size() != countsStr.size()) {
    cerr << "Error: invalid composition, " << compStr << ", was entered.\n";
    cerr << "Every symbol must be followed by a number (i.e., Ti1O2).\n";
    return false;
  }

  // Transform the counts vector into an int vector
  vector<uint> counts(countsStr.size());
  transform(countsStr.begin(), countsStr.end(), counts.begin(),
            [](const string& s) { return stoi(s); });

  // Make sure none of the counts are zero
  if (!none_of(counts.begin(), counts.end(), [](uint a) { return a == 0; })) {
    cerr << "Error reading numbers in composition, " << compStr << "\n";
    cerr << "Check your input and try again\n";
    return false;
  }

  for (size_t i = 0; i < symbols.size(); ++i) {
    int atomicNum = getAtomicNum(symbols[i]);
    if (atomicNum == 0) {
      cerr << "Error: invalid elemental symbol, " << symbols[i]
           << ", was entered in the composition, " << compStr << "\n"
           << "Note: every symbol must be followed by a number (i.e., Ti1O2)\n";
      return false;
    }
    comp[atomicNum] = counts[i];
  }

  return true;
}

bool ElemInfo::readComposition(const string& comp, vector<uint>& atoms)
{
  atoms.clear();
  map<uint, uint> compMap;
  if (!readComposition(comp, compMap))
    return false;

  for (const auto& elem : compMap) {
    for (uint i = 0; i < elem.second; ++i)
      atoms.push_back(elem.first);
  }

  return true;
}

double ElemInfo::getVdwRadius(uint atomicNum)
{
  if (atomicNum == 0 || atomicNum > 117) {
    std::cout << "Error: Invalid atomicNum, " << atomicNum
              << ", was entered in " << __FUNCTION__ << "!\n";
    return 0;
  }
  return vdwRadii[atomicNum];
}

double ElemInfo::getCovalentRadius(uint atomicNum)
{
  if (atomicNum == 0 || atomicNum > 117) {
    std::cout << "Error: Invalid atomicNum, " << atomicNum
              << ", was entered in " << __FUNCTION__ << "!\n";
    return 0;
  }
  return covalentRadii[atomicNum];
}

void ElemInfo::applyScalingFactor(double sf)
{
  for (int i = 1; i < 118; i++) {
    covalentRadii[i] = ElemInfoDatabase::_covalentRadii[i] * sf;
    vdwRadii[i] = ElemInfoDatabase::_vdwRadii[i] * sf;
  }
}

// We allow the user to set a radius here
// We will set both radii since we know the user will only be using one of
// them, but we don't know which...
void ElemInfo::setRadius(uint atomicNum, double newRadius)
{
  if (atomicNum == 0 || atomicNum > 117) {
    std::cout << "Error: Invalid atomicNum, " << atomicNum
              << ", was entered in " << __FUNCTION__ << "!\n";
    return;
  }

  if (newRadius < 0) {
    std::cout << "Error in " << __FUNCTION__ << ": a negative radius, '"
              << newRadius << "', was entered.\n";
    return;
  }

  covalentRadii[atomicNum] = newRadius;
  vdwRadii[atomicNum] = newRadius;
}

void ElemInfo::setMinRadius(double minRadius)
{
  for (size_t i = 1; i < covalentRadii.size(); i++) {
    if (covalentRadii[i] < minRadius)
      covalentRadii[i] = minRadius;
    if (vdwRadii[i] < minRadius)
      vdwRadii[i] = minRadius;
  }
}

double ElemInfo::getRadius(uint atomicNum, bool usingVdwRadius)
{
  if (usingVdwRadius)
    return getVdwRadius(atomicNum);
  else
    return getCovalentRadius(atomicNum);
}

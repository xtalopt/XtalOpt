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

#include "elemInfo.h"
#include "utilityFunctions.h"

using namespace std;

// Set the static member variables
vector<string> ElemInfo::atomicSymbols = ElemInfoDatabase::_atomicSymbols;
vector<double> ElemInfo::covalentRadii = ElemInfoDatabase::_covalentRadii;
vector<double> ElemInfo::vdwRadii      = ElemInfoDatabase::_vdwRadii;

std::string ElemInfo::getAtomicSymbol(uint atomicNum)
{
  if (atomicNum == 0 || atomicNum > 117) {
    std::cout << "Error: Invalid atomicNum, " << atomicNum << ", was entered in "
         << __FUNCTION__ << "!\n";
    return 0;
  }
  return atomicSymbols[atomicNum];
}

uint ElemInfo::getAtomicNum(std::string symbol)
{
  for (uint i = 0; i < atomicSymbols.size(); i++) {
    if (atomicSymbols[i] == symbol) return i;
  }

  std::cout << "Error: Invalid symbol, " << symbol << ", was entered into "
       << __FUNCTION__ << "!\n";
  return 0;
}

// Reads until a number is reached and returns what it read
string readLetters(string input)
{
  string ret;
  size_t i = 0;
  while (true) {
    if (i == input.size()) return ret;
    if (!isDigit(input[i])) ret.push_back(input[i]);
    else return ret;
    i++;
  }
}

// Reads until a non-number is reached. Then it returns that number.
string readNumbers(string input)
{
  string ret;
  size_t i = 0;
  while (true) {
    if (i == input.size()) return ret;
    if (isDigit(input[i])) ret.push_back(input[i]);
    else return ret;
    i++;
  }
}

bool ElemInfo::readComposition(string comp, vector<uint>& atoms)
{
  comp = removeSpacesAndReturns(comp);
  atoms.clear();
  while (comp.size() != 0 && !containsOnlySpaces(comp)) {

    // Find the symbol
    string symbol = readLetters(comp);

    // remove the symbol from the string
    comp = comp.substr(symbol.size());

    // Find the number
    string number = readNumbers(comp);
    // remove the number from the string
    comp = comp.substr(number.size());

    uint atomicNum = getAtomicNum(symbol);

    if (atomicNum == 0) {
      cout << "Error in " << __FUNCTION__ << ": invalid atomic symbol\n";
      atoms.clear();
      return false;
    }

    size_t num = stoi(number);
    if (num == 0) {
      cout << "Error in " << __FUNCTION__ << ": invalid number read\n";
      atoms.clear();
      return false;
    }

    for (size_t i = 0; i < num; i++) {
      atoms.push_back(atomicNum);
    }
  }
  return true;
}

double ElemInfo::getVdwRadius(uint atomicNum)
{
  if (atomicNum == 0 || atomicNum > 117) {
    std::cout << "Error: Invalid atomicNum, " << atomicNum << ", was entered in "
         << __FUNCTION__ << "!\n";
    return 0;
  }
  return vdwRadii[atomicNum];
}

double ElemInfo::getCovalentRadius(uint atomicNum)
{
  if (atomicNum == 0 || atomicNum > 117) {
    std::cout << "Error: Invalid atomicNum, " << atomicNum << ", was entered in "
         << __FUNCTION__ << "!\n";
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
    std::cout << "Error: Invalid atomicNum, " << atomicNum << ", was entered in "
         << __FUNCTION__ << "!\n";
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
    if (covalentRadii[i] < minRadius) covalentRadii[i] = minRadius;
    if (vdwRadii[i] < minRadius) vdwRadii[i] = minRadius;
  }
}

double ElemInfo::getRadius(uint atomicNum, bool usingVdwRadius)
{
  if (usingVdwRadius) return getVdwRadius(atomicNum);
  else return getCovalentRadius(atomicNum);
}

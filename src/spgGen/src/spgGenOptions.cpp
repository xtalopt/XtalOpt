/**********************************************************************
  spgGenOptions.cpp - Options class for spacegroup generation.

  Copyright (C) 2015 - 2016 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#include <fstream>
#include <iostream>

#include "elemInfo.h"
#include "spgGenOptions.h"
#include "utilityFunctions.h"

using namespace std;

latticeStruct defaultLatticeMins(3.0, 3.0, 3.0, 60.0, 60.0, 60.0);
latticeStruct defaultLatticeMaxes(10.0, 10.0, 10.0, 120.0, 120.0, 120.0);

SpgGenOptions::SpgGenOptions() :
m_filename(""),
m_composition(""),
m_spacegroups(vector<uint>()),
m_latticeMinsSet(false),
m_latticeMaxesSet(false),
m_latticeMins(defaultLatticeMins),
m_latticeMaxes(defaultLatticeMaxes),
m_numOfEachSpgToGenerate(1),
m_forceMostGeneralWyckPos(true),
m_forcedWyckAssignments(vector<pair<uint, char>>()),
m_radiusVector(vector<pair<uint, double>>()),
m_setAllMinRadii(false),
m_minRadii(0),
m_scalingFactor(1.0),
m_minVolume(-1),
m_maxVolume(-1),
m_maxAttempts(100),
m_outputDir("."),
m_verbosity('r'),
m_optionsAreValid(true)
{

}

// This function is static
SpgGenOptions SpgGenOptions::readOptions(string filename)
{
  SpgGenOptions options;

  ifstream f;
  f.open(filename);

  if (!f.is_open()) {
    cerr << "Error: " << filename << " was not opened successfully! Please "
         << "check the permissions and that it exists.\n";
    options.m_optionsAreValid = false;
    return options;
  }

  // So that we avoid code duplication, read the contents of this file into
  // a string and call the other function. We are assuming that
  // the input file is going to be small so this should not cause any
  // memory issues...

  string temp;
  string inputStr;
  while (getline(f, temp)) inputStr += (temp + "\n");
  f.close();

  return readOptionsFromCharArray(inputStr.c_str(), filename);
}

// This version of the function reads a character array that contains the full
// input. It is used for the html version of the code
SpgGenOptions SpgGenOptions::readOptionsFromCharArray(const char* input,
                                                      string filename)
{
  SpgGenOptions options;
  options.m_filename = filename;

  string stdstr(input);
  istringstream lines(stdstr);
  string line;

  // First line is a comment, so ignore it
  std::getline(lines, line);
  // Read each line and set options
  while (std::getline(lines, line)) options.interpretLineAndSetOption(line);

  // Check to make sure the composition and some spacegroups were set.
  // If not, exit as a failure
  if (options.m_composition == "") {
    cerr << "Error: option 'composition' was not set in " << filename << "!\n"
         << "Please set the composition.\n";
    options.m_optionsAreValid = false;
    return options;
  }

  else if (options.m_spacegroups.size() == 0) {
    cerr << "Error: option 'spacegroups' was not set in " << filename << "!\n"
         << "Please set the spacegroups.\n";
    options.m_optionsAreValid = false;
    return options;
  }

  return options;
}

vector<uint> createSpgVector(string s)
{
  vector<uint> ret;

  s = trim(s);
  s = removeSpacesAndReturns(s); // important for reading input from html
  // Split it according to commas
  vector<string> ssplit = split(s, ',');

  for (size_t i = 0; i < ssplit.size(); i++) {
    // Add all individual numbers
    if (isNumber(trim(ssplit.at(i)))) ret.push_back(stoi(trim(ssplit.at(i))));
    // Add hyphenated numbers
    else if (contains(ssplit.at(i), '-')) {
      // Split it
      vector<string> hyphenSplit = split(ssplit.at(i), '-');

      if (hyphenSplit.size() != 2) {
        cerr << "Error understanding the spacegroups option. Please verify that"
             << " it is properly formatted with commas and hyphens.\n";
        return ret;
      }
      // Find the first number, and keep adding numbers till we get to the final
      // one
      uint firstNum = stoi(trim(hyphenSplit.at(0)));
      uint lastNum  = stoi(trim(hyphenSplit.at(1)));
      for (uint i = firstNum; i <= lastNum; i++) ret.push_back(i);
    }
  }

  // Sort the numbers
  sort(ret.begin(), ret.end());

  // Remove duplicates
  ret = removeDuplicates<uint>(ret);

  return ret;
}

latticeStruct interpretLatticeString(const string& s)
{
  latticeStruct ret;
  vector<string> theSplit = split(s, ',');
  if (theSplit.size() != 6) {
    cerr << "Error reading lattice string: " << s << "!\n";
    cerr << "Please make sure it is formatted correctly.\n";
    return ret;
  }

  ret.a     = stof(trim(theSplit.at(0)));
  ret.b     = stof(trim(theSplit.at(1)));
  ret.c     = stof(trim(theSplit.at(2)));
  ret.alpha = stof(trim(theSplit.at(3)));
  ret.beta  = stof(trim(theSplit.at(4)));
  ret.gamma = stof(trim(theSplit.at(5)));

  return ret;
}

void SpgGenOptions::interpretLineAndSetOption(string line)
{
  // First, trim it
  line = trim(line);

  // If the line is empty, return
  // If the line starts with '#', return
  if (line.size() == 0) return;
  if (line.at(0) == '#') return;

  // Remove anything to the right of any # in the line - this is a comment
  line = split(line, '#').at(0);

  // Separate the option and the value
  vector<string> theSplit = split(line, '=');

  // If this is not two, then there is some kind of error in the line
  if (theSplit.size() != 2) {
    cerr << "In options files, '" << this->m_filename << "', error reading "
         << "the following line: '" << line << "'\n";
    return;
  }

  string option = trim(theSplit.at(0));
  string value  = trim(theSplit.at(1));

  // Now let's figure out what the option is
  if (option == "composition") {
    m_composition = value;
  }
  else if (option == "spacegroups") {
    m_spacegroups = createSpgVector(value);
  }
  else if (option == "latticeMins") {
    m_latticeMinsSet = true;
    m_latticeMins  = interpretLatticeString(value);
  }
  else if (option == "latticeMaxes") {
    m_latticeMaxesSet = true;
    m_latticeMaxes = interpretLatticeString(value);
  }
  else if (option == "numOfEachSpgToGenerate") {
    m_numOfEachSpgToGenerate = stoi(value);
  }
  else if (option == "forceMostGeneralWyckPos") {
    if (value.at(0) == 'F' || value.at(0) == 'f')
      m_forceMostGeneralWyckPos = false;
    else if (value.at(0) == 'T' || value.at(0) == 't')
      m_forceMostGeneralWyckPos = true;
    else {
      cerr << "Error reading 'forceMostGeneralWyckPos' setting: " << value
           << "\nValid settings are 'True' or 'False' or 'T' or 'F'\n";
      cerr << "The value will remain the default: true\n";
    }
  }
  else if (contains(option, "forceWyckPos")) {
    vector<string> tempSplit = split(option, ' ');
    if (tempSplit.size() != 2 || value.size() != 1) {
      cerr << "Error reading 'forceWyckPos' option: " << line
           << "\nProper format is: forceWyckPos <atomicSymbol> = <char>\n";
      m_optionsAreValid = false;
      return;
    }
    uint atomicNum = ElemInfo::getAtomicNum(tempSplit.at(1));
    m_forcedWyckAssignments.push_back(make_pair(atomicNum, value.at(0)));
  }
  else if (contains(option, "setRadius")) {
    // There should be a space after 'setRadius' with the atomic symbol there
    vector<string> tempSplit = split(option, ' ');
    if (tempSplit.size() != 2) {
      cerr << "Error reading 'setRadius' option: " << line
           << "\nProper format is: setRadius <atomicSymbol> = <value>\n";
      m_optionsAreValid = false;
      return;
    }
    uint atomicNum = ElemInfo::getAtomicNum(tempSplit.at(1));
    m_radiusVector.push_back(make_pair(atomicNum, stof(value)));
  }
  else if (option == "setMinRadii") {
    m_setAllMinRadii = true;
    m_minRadii       = stof(value);
  }
  else if (option == "scalingFactor") {
    m_scalingFactor = stof(value);
  }
  else if (option == "minVolume") {
    m_minVolume = stof(value);
  }
  else if (option == "maxVolume") {
    m_maxVolume = stof(value);
  }
  else if (option == "maxAttempts") {
    m_maxAttempts = stoi(value);
  }
  else if (option == "outputDir") {
    m_outputDir = value;
  }
  else if (option == "verbosity") {
    if (value.at(0) != 'n' && value.at(0) != 'r' && value.at(0) != 'v') {
      cerr << "Error: the value given for verbosity, '" << value << "', is "
           << "not a valid option!\nValid options are: 'n' for no output, "
           << "'r' for regular output, or 'v' for verbose output\n";
      m_optionsAreValid = false;
      return;
    }
    m_verbosity = value.at(0);
  }
  else {
    cerr << "Warning: the following line contained an unrecognizable option: "
         << line << "\n";
  }

  return;
}

string getSpacegroupsString(vector<uint> v)
{
  string ret;
  for (size_t i = 0; i < v.size(); i++) {
    // Last value
    if (i == v.size() - 1) {
      ret += to_string(v.at(i));
    }
    // Second to last value
    else if (i == v.size() - 2) {
      ret += to_string(v.at(i));
      ret += ", ";
    }
    // Anything else
    else {
      // Check for hyphenation
      if (v.at(i) + 1 == v.at(i + 1) && v.at(i) + 2 == v.at(i + 2)) {
        // Hyphenate this segment
        ret += to_string(v.at(i));
        ret += " - ";
        for (size_t j = i; j < v.size(); j++) {
          if (v.at(j) - v.at(i) != j - i) {
            // End the hyphenation
            ret += to_string(v.at(j - 1));
            ret += ", ";
            i = j - 1;
            break;
          }
          // We reached the final value
          else if (j == v.size() - 1) {
            ret += to_string(v.at(j));
            i = j;
            break;
          }
        }
      }
      // Just add the number with a comma
      else {
        ret += to_string(v.at(i));
        ret += ", ";
      }
    }
  }
  return ret;
}

string getLatticeString(const latticeStruct& l)
{
  stringstream s;
  s << "  a:     " << l.a     << "\n";
  s << "  b:     " << l.b     << "\n";
  s << "  c:     " << l.c     << "\n";
  s << "  alpha: " << l.alpha << "\n";
  s << "  beta:  " << l.beta  << "\n";
  s << "  gamma: " << l.gamma << "\n";
  return s.str();
}

string SpgGenOptions::getOptionsString() const
{
  stringstream s;
  s << "\n*** Options from '" << m_filename
    << "' have been set as follows***\n";
  s << "composition: " << m_composition << "\n";
  s << "spacegroups: " << getSpacegroupsString(m_spacegroups) << "\n";

  if (!m_latticeMinsSet) {
    s << "latticeMins were not explicitly set. Using the defaults:\n";
    s << getLatticeString(m_latticeMins);
  }
  else s << "latticeMins:\n" << getLatticeString(m_latticeMins);

  if (!m_latticeMaxesSet) {
    s << "latticeMaxes were not explicitly set. Using the defaults:\n";
    s << getLatticeString(m_latticeMaxes);
  }
  else s << "latticeMaxes:\n" << getLatticeString(m_latticeMaxes);

  if (m_minVolume == -1) s << "minVolume: none\n";
  else s << "minVolume: " << m_minVolume << "\n";

  if (m_maxVolume == -1) s << "maxVolume: none\n";
  else s << "maxVolume: " << m_maxVolume << "\n";

  s << "numOfEachSpgToGenerate: " << m_numOfEachSpgToGenerate << "\n";
  if (m_setAllMinRadii) {
    s << "default minRadii: " << m_minRadii << "\n";
  }
  s << "explicity set radii: \n";
  for (size_t i = 0; i < m_radiusVector.size(); i++) {
    s << "  " << ElemInfo::getAtomicSymbol(m_radiusVector.at(i).first)
      << ": " << m_radiusVector.at(i).second << "\n";
  }
  s << "scalingFactor: " << m_scalingFactor << "\n";
  s << "maxAttempts: " << m_maxAttempts << "\n";
  s << "outputDir: " << m_outputDir << "\n";
  s << "output verbosity: " << m_verbosity << "\n";
  s << "\n";
  return s.str();
}

void SpgGenOptions::printOptions() const
{
  cout << getOptionsString();
}

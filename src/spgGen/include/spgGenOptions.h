/**********************************************************************
  spgGenOptions.h - Options class for spacegroup initialization.

  Copyright (C) 2015 - 2016 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#ifndef SPG_GEN_OPTIONS_H
#define SPG_GEN_OPTIONS_H

#include <string>
#include <vector>

// This is for 'latticeStruct'
#include "crystal.h"

class SpgGenOptions {
 public:
  explicit SpgGenOptions();

  /* Reads options from a specified file and returns a SpgGenOptions object
   * that has the options set.
   *
   * @param filename The name of the file to be read.
   *
   * @return The SpgGenOptions object that has the options set.
   */
  static SpgGenOptions readOptions(std::string filename);

  /* Reads the options from a character array (in the complete format of
   * the input file) and returns a SpgGenOptions object that has the options
   * set.
   *
   * @param input A character array containing the whole input file that is
   *              to be read.
   *
   * @param filename The optional name of the file. It is only used when
   *                 printing error messages.
   * @return The SpgGenOptions object that has the options set.
   */
  static SpgGenOptions readOptionsFromCharArray(const char* input,
                                                std::string filename = "");

  /* Reads a line and sets an option based upon the contents
   *
   * @param line The line to be interpreted
   */
  void interpretLineAndSetOption(std::string line);

  /* Reads all the options and returns a string that gives all the information.
   * This string is meant to be printed to the terminal or a log file.
   *
   * @return The options info.
   */
  std::string getOptionsString() const;

  /* Call Print options info to the terminal. Calls getOptionsString() and
   * prints the string to the terminal output.
   */
  void printOptions() const;

  // Getters
  std::string getFileName() const {return m_filename;};
  std::string getComposition() const {return m_composition;};
  std::vector<uint> getSpacegroups() const {return m_spacegroups;};
  bool latticeMinsSet() const {return m_latticeMinsSet;};
  bool latticeMaxesSet() const {return m_latticeMaxesSet;};
  latticeStruct getLatticeMins()  const {return m_latticeMins;};
  latticeStruct getLatticeMaxes() const {return m_latticeMaxes;};
  uint getNumOfEachSpgToGenerate() const {return m_numOfEachSpgToGenerate;};
  bool forceMostGeneralWyckPos() const {return m_forceMostGeneralWyckPos;};
  std::vector<std::pair<uint, char>> getForcedWyckAssignments() const {return m_forcedWyckAssignments;};
  std::vector<std::pair<uint, double>> getRadiusVector() const {return m_radiusVector;};
  bool setAllMinRadii() const {return m_setAllMinRadii;};
  double getMinRadii() const {return m_minRadii;};
  double getScalingFactor() const {return m_scalingFactor;};
  double getMinVolume() const {return m_minVolume;};
  double getMaxVolume() const {return m_maxVolume;};
  int getMaxAttempts() const {return m_maxAttempts;};
  std::string getOutputDir() const {return m_outputDir;};
  char getVerbosity() const {return m_verbosity;};
  // This will return false if the options are invalid
  bool optionsAreValid() const {return m_optionsAreValid;};

  // Setters
  void setFileName(const std::string& s) {m_filename = s;};
  void setComposition(const std::string& s) {m_composition = s;};
  void setSpacegroups(const std::vector<uint>& v) {m_spacegroups = v;};
  void setLatticeMins(const latticeStruct& ls) {m_latticeMins = ls; m_latticeMinsSet = true;};
  void setLatticeMaxes(const latticeStruct& ls) {m_latticeMaxes = ls; m_latticeMaxesSet = true;};
  void setNumOfEachSpgToGenerate(uint u) {m_numOfEachSpgToGenerate = u;};
  void setForceMostGeneralWyckPos(bool b) {m_forceMostGeneralWyckPos = b;};
  void setForcedWyckoffAssignments(std::vector<std::pair<uint, char>> v) {m_forcedWyckAssignments = v;};
  void setRadiusVector(const std::vector<std::pair<uint, double>>& v) {m_radiusVector = v;};
  void setMinRadii(double d) {m_minRadii = d; m_setAllMinRadii = true;};
  void setScalingFactor(double d) {m_scalingFactor = d;};
  void setMinVolume(double d) {m_minVolume = d;};
  void setMaxVolume(double d) {m_maxVolume = d;};
  void setMaxAttempts(int i) {m_maxAttempts = i;};
  void setOutputDir(const std::string& s) {m_outputDir = s;};
  void setVerbosity(char c) {m_verbosity = c;};

 private:
  // m_filename: string for the filename that the options were read from
  // m_composition: string for the composition (e. g. Mg3Al2)
  std::string m_filename, m_composition;

  // m_spacegroups: vector of unsigned integers representing the spacegroups
  std::vector<uint> m_spacegroups;

  // m_latticeMinsSet and m_latticeMaxesSet: automatically set true if the user
  // sets the respective option.
  bool m_latticeMinsSet, m_latticeMaxesSet;

  // m_latticeMins and m_latticeMaxes: contains a list of the respective mins
  // or maxes for the lattice to be generated.
  latticeStruct m_latticeMins, m_latticeMaxes;

  // m_numOfEachSpgToGenerate: the number of each spg to generate
  uint m_numOfEachSpgToGenerate;

  bool m_forceMostGeneralWyckPos;

  // m_forcedWyckAssignments: a vector of pairs containing an atomic number
  // and a Wyckoff assignment that the user wants to force
  std::vector<std::pair<uint, char>> m_forcedWyckAssignments;

  // m_radiusVector: a vector of pairs containing each atomic number
  // whose radius the user wants to manually set and the value for the radius
  // Each pair is as such: <atomicNum, newRadius>
  std::vector<std::pair<uint, double>> m_radiusVector;

  // m_setAllMinRadii: automatically set true if m_minRadii is set
  bool m_setAllMinRadii;

  // m_minRadii: the minimum radius for all atoms
  double m_minRadii;

  // m_scalingFactor: the scaling factor for the atomic radii
  // new radii will be default radii * scaling factor
  double m_scalingFactor;

  // m_minVolume, m_maxVolume: the min and max volume of the crystals
  double m_minVolume, m_maxVolume;

  // m_maxAttempts: the maximum number of attempts to generate a crystal
  // that has that composition, spacegroup, lattice constraints, and IADs
  int m_maxAttempts;

  // m_outputDir: the name of the output directory
  std::string m_outputDir;

  // m_verbosity: the verbosity of the log file: 'n' for none, 'r' for regular,
  // and 'v' for verbose.
  char m_verbosity;

  // This will be false if the options are not valid
  bool m_optionsAreValid;
};

#endif

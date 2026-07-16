/**********************************************************************
  elemInfo - Contains functions for getting atomic radii and atomic
             symbols and for setting atomic radii

  Copyright (C) 2015 - 2016 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef ATOMS_ELEM_INFO_H
#define ATOMS_ELEM_INFO_H

#include <atoms/data/eleminfodatabase.h>

#include <map>
#include <string>
#include <vector>

namespace Atoms {

/**
 * @class ElementInfo eleminfo.h
 * @brief Lookup tables for the chemical elements: symbols, names, masses, and
 *        covalent radii, with helpers to convert between them.
 */
class ElementInfo
{
public:
  static std::string getAtomicSymbol(unsigned int atomicNum);

  static double getAtomicMass(unsigned int atomicNum);

  static double getAverageAtomicMass();

  static unsigned int getAtomicNum(std::string symbol);

  // This function will read 'compStr' and write the result to 'comp'
  // where the keys to comp are the atomic numbers, and the values are
  // the numbers of each.
  static bool readComposition(std::string comp, std::map<unsigned int, unsigned int>& map);

  // This function will read 'comp' and write the result to 'atoms'
  // 'atoms' is a vector of atomic numbers. One for each atom.
  // Returns true if the read was successful and false if it was not
  static bool readComposition(const std::string& comp, std::vector<unsigned int>& atoms);

  static double getVdwRadius(unsigned int atomicNum);

  static double getAverageVdwRadius();

  static double getVdwVolume(unsigned int atomicNum);

  static double getCovalentRadius(unsigned int atomicNum);

  static double getAverageCovalentRadius();

  static double getCovalentVolume(unsigned int atomicNum);

  // Applies a specified scaling factor to every radius for both
  // covalent and vdw radii to the database radii and sets the
  // static members of this class to be them.
  // So, if we call applyScalingFactor(0.5) twice, it will still be the
  // same thing. This will also erase any radius settings induced by
  // 'setRadius()'
  static void applyScalingFactor(double sf);

  // We allow the user to set a radius here
  // We will set both radii since we know the user will only be using one of
  // them, but we don't know which...
  static void setRadius(unsigned int atomicNum, double newRadius);

  // Changes all radii to be at least this radius
  static void setMinRadius(double minRadius);

  static double getRadius(unsigned int atomicNum, bool usingVdwRadius);

private:
  // Retain a copy of the database pieces here so they may be edited
  static std::vector<std::string> atomicSymbols;
  static std::vector<double> masses;
  static std::vector<double> covalentRadii;
  static std::vector<double> vdwRadii;

}; // class ElemInfo

} // namespace Atoms

#endif

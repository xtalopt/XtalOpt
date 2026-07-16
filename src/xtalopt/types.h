/**********************************************************************
  types - Basic types for elements/xtals handling in the search

  Copyright (C) 2009-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef XTALOPT_TYPES_H
#define XTALOPT_TYPES_H

#include <common/constants.h>

#include <QList>
#include <QMap>
#include <QPair>
#include <QString>

#include <algorithm>
#include <map>

namespace XtalOpt {


// As of XtalOpt v14, we use the user-provided "chemical formula" strings
//   to obtain the list of chemical composition, elemental volumes, and
//   reference energies.
// The "Cell Composition" object will be used to store the information about
//   the chemical composition of a cell (symbol, atomic number, number of atoms
//   of a symbol for all elements in the cell); and is obtained by parsing a
//   (full) chemical formula string in "formulaToComposition" function.
//
// We maintain a list of these objects as the list of user-provided formula, and
//   use them to generate new cells.
// Further, we use them as a convenience tool to parse elemental volume
//   and reference energy entries.
//
// The set of "get..." function are interfaces to access various information.
//   Since composition object stores elemental data as a qMap; these functions
//   return "sorted" lists: symbols are alphabetically sorted, and the rest are
//   sorted accordingly. Although, we generally don't rely on this order in the code.
//
class CellComp
{
public:
  void clearCompositionEntries() {m_data.clear();}
  // Set an element's entry in cell composition using symbol, atomic number, atom count
  void setCompositionEntry(QString symb, uint atomicn, uint acount)
    {m_data[symb] = qMakePair(atomicn, acount);}
  // Access atom count of "symbol" or "atomic number"
  uint getCount(const QString& s) const
    {return (m_data.contains(s) ? m_data.value(s).second : 0);}
  uint getCount(const uint& i) const
    {for (const auto& ed : m_data) {if (ed.first == i) return ed.second;} return 0;}
  // Access total number of atoms and types
  int getNumAtoms() const
    {int n = 0; for (const auto& ed : m_data) n += ed.second; return n;}
  int getNumTypes() const
    {return m_data.size();}
  QString getFormula() const
    {QString f = "";
    for (auto it = m_data.constBegin(); it != m_data.constEnd(); ++it)
      f += QString("%1%2").arg(it.key()).arg(it.value().second);
    return f;}
  // Access lists: the symbols are always sorted alphabetically and the
  // rest of the lists are sorted accordingly.
  QList<QString> getCompositionSymbols() const
    {return m_data.keys();}
  QList<uint> getCompositionAtomicNumbers() const
    {QList<uint> a; for (const auto &ed : m_data) a.append(ed.first); return a;}
  QList<uint> getCompositionCounts() const
    {QList<uint> c; for (const auto &ed : m_data) c.append(ed.second); return c;}
private:
  // This is a map of  <"element symbol" , "atomic number , atom count">
  QMap<QString, QPair<uint, uint> > m_data;
};

// Reference energy object: for each formula in the reference energies input
//   we construct a cell composition object, and assign to it the corresponding
//   energy. A list of this "RefEnergy" objects will be used in the code to
//   produce the "reference energies vector" for convex hull calculation.
struct RefEnergy
{
  CellComp cell;
  double   energy;
};

// Minimum radii of elements: an instance of this class is created once the
//   user's input formula are processed, by assigning the values for all
//   elements in the search space.
// Note: this information is separate to simplify it's runtime update by
//   eliminating the need to update all composition objects every time
//   radii-related stuff are being updated.
class EleRadii
{
public:
  void clearElementRadii() {m_data.clear();}
  // Set an element's entry with atomic number and minimum radius
  void setElementRadius(const uint& atomcn, const double& minradius)
    {m_data[atomcn] = minradius;}
  // Access the list of elements stored in the object
  QList<uint> getRadiusAtomicNumbers() const
    {return m_data.keys();}
  // Access the min radius for an atomic number (if element is not there, return 1e300)
  double getMinRadius(uint a) const
    {if (m_data.contains(a)) return m_data.value(a); return PINF;}
private:
  QMap<uint, double> m_data;
};

// Elemental volume object (as of XtalOpt v14): user can provide a list of min/max
//   values for elemental volumes; besides the absolute and scaled volume limit
//   options. An instance of this class is initialized once the input is processed,
//   and being updated at runtime if user provides new values.
class EleVolume
{
public:
  void clearElementVolumes() {m_data.clear();}
  // Set an element's entry with atomic number and minimum/maximum volumes
  void setElementVolumeRange(const uint& atomcn, const double& min, const double& max)
    {m_data[atomcn] = qMakePair(min, max);}
  // Access the list of elements stored in the object
  QList<uint> getVolumeAtomicNumbers() const
    {return m_data.keys();}
  // Access the min/max volumes for an atomic number
  double getMinVolume(uint a) const
    {if (m_data.contains(a)) return m_data.value(a).first; return 0.0;}
  double getMaxVolume(uint a) const
    {if (m_data.contains(a)) return m_data.value(a).second; return 0.0;}
private:
  QMap<uint, QPair<double, double> > m_data;
};

struct IAD
{
  double minIAD;
};

// A simple minIADs class that uses unordered atomic numbers for
// the key and a double for the value. In order to set a value,
// you must use set() and not ().
class minIADs
{
public:
  // Set a specific atomic number pair to have a specific IAD
  void setMinIAD(short i, short j, double d) { m_data[std::minmax(i, j)] = d; }

  void clearMinIADs() { m_data.clear(); }

  // Get the IAD value for a specific atomic number pair, or
  // 1e300 if the value does not exist.
  double operator()(short i, short j) const
  {
    if (m_data.count(std::minmax(i, j)) != 1)
      return PINF;
    return m_data.at(std::minmax(i, j));
  }

private:
  std::map<std::pair<short, short>, double> m_data;
};

} // end namespace XtalOpt

#endif

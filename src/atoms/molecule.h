/**********************************************************************
  molecule - A basic molecule (0D geometries) module.

  Copyright (C) 2016-2017 by Patrick S. Avery
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef ATOMS_MOLECULE_H
#define ATOMS_MOLECULE_H

#include <atoms/geometry.h>

#include <QString>

#include <string>
#include <vector>

namespace Atoms {

// A molecule unit is a 0D Geometry: atoms in Cartesian coordinates with no unit
//   cell. This module builds those geometries (from a template + formula, or from
//   a Cartesian coordinate string) and describes the built-in template catalog.
//   All atom queries (formula, symbols, point group, ...) are Geometry methods,
//   so there is no separate molecule type.

// One entry of the built-in molecule-template catalog, for display/selection.
struct MoleculeTemplateInfo
{
  QString name;
  QString pointGroup;
  QString speciesPattern;
  QString formulaPattern;
  QString description;
  std::vector<unsigned int> orbitSizes;
};

// Build a molecule (0D Geometry) for formula (eg "C1H4") using a named built-in
//   template. scaleFactor rescales so no interatomic distance is shorter than the
//   covalent distance times scaleFactor. Returns false and sets error on failure.
bool buildMoleculeFromFormula(const std::string& formula, const std::string& templateName,
                              Geometry& molecule, QString& error, double scaleFactor = 1.0);

// Build a molecule (0D Geometry) from a comma-delimited "symbol x y z, ..." list.
//   Coordinates are used as-is, no rescaling. Returns false and sets error on failure.
bool buildMoleculeFromCartesianString(const std::string& text, Geometry& molecule, QString& error);

// The built-in templates whose species pattern fits formula.
std::vector<MoleculeTemplateInfo> moleculeTemplatesForFormula(const std::string& formula);

// The whole built-in template catalog.
std::vector<MoleculeTemplateInfo> moleculeTemplatesCatalog();

// The whole built-in template catalog, formatted as a table for CLI/GUI display.
QString moleculeTemplateCatalogText();

} // namespace Atoms

#endif // ATOMS_MOLECULE_H

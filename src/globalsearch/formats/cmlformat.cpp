/**********************************************************************
  CmlFormat -- A simple reader and writer for the CML format.

  Copyright (C) 2017 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include "cmlformat.h"

#include <pugixml/pugixml.hpp>

#include <globalsearch/eleminfo.h>
#include <globalsearch/structure.h>
#include <globalsearch/utilities/utilityfunctions.h>

#include <bitset>
#include <cmath>
#include <iostream>
#include <map>
#include <sstream>
#include <streambuf>

using std::string;

using pugi::xml_document;
using pugi::xml_node;
using pugi::xml_attribute;

using namespace GlobalSearch;

namespace {

class CmlFormatPrivate
{
public:
  CmlFormatPrivate(Structure* s, xml_document& document)
    : m_success(false), m_structure(s), m_structureNode(nullptr)
  {
    m_structure->clear();
    m_structure->resetEnergy();
    m_structure->resetEnthalpy();

    // Parse the CML document, and create molecules/elements as necessary.
    m_structureNode = document.child("molecule");
    xml_node cmlNode = document.child("cml");
    if (cmlNode)
      m_structureNode = cmlNode.child("molecule");

    if (m_structureNode) {
      // Parse the various components we know about.
      m_success = unitCell();
      if (m_success)
        m_success = atoms();
      if (m_success)
        m_success = bonds();
      if (m_success)
        m_success = properties();
    } else {
      m_error += "Error, no molecule node found.";
      m_success = false;
    }
  }

  bool unitCell()
  {
    xml_attribute attribute;
    xml_node node;

    // Unit cell:
    node = m_structureNode.child("crystal");
    if (node) {
      float a(0);
      float b(0);
      float c(0);
      float alpha(0);
      float beta(0);
      float gamma(0);
      enum
      {
        CellA = 0,
        CellB,
        CellC,
        CellAlpha,
        CellBeta,
        CellGamma
      };
      std::bitset<6> parsedValues;
      for (pugi::xml_node scalar = node.child("scalar"); scalar;
           scalar = scalar.next_sibling("scalar")) {
        pugi::xml_attribute title = scalar.attribute("title");
        if (title) {
          std::string titleStr(title.value());
          if (titleStr == "a") {
            a = scalar.text().as_float();
            parsedValues.set(CellA);
          } else if (titleStr == "b") {
            b = scalar.text().as_float();
            parsedValues.set(CellB);
          } else if (titleStr == "c") {
            c = scalar.text().as_float();
            parsedValues.set(CellC);
          } else if (titleStr == "alpha") {
            alpha = scalar.text().as_float();
            parsedValues.set(CellAlpha);
          } else if (titleStr == "beta") {
            beta = scalar.text().as_float();
            parsedValues.set(CellBeta);
          } else if (titleStr == "gamma") {
            gamma = scalar.text().as_float();
            parsedValues.set(CellGamma);
          }
        }
      }
      if (parsedValues.count() != 6) {
        m_error += "Incomplete unit cell description.";
        return false;
      }

      m_structure->unitCell().setCellParameters(a, b, c, alpha, beta, gamma);
    }
    return true;
  }

  bool atoms()
  {
    xml_node atomArray = m_structureNode.child("atomArray");
    if (!atomArray)
      return false;

    xml_node node = atomArray.child("atom");

    while (node) {
      Atom atom;

      // Step through all of the atom attributes and store them.
      xml_attribute attribute = node.attribute("elementType");
      if (attribute) {
        atom.setAtomicNumber(ElemInfo::getAtomicNum(attribute.value()));
      } else {
        // There is no element data, this atom node is corrupt.
        m_error += "Warning, corrupt element node found.";
        return false;
      }

      attribute = node.attribute("id");
      if (attribute)
        m_atomIds[std::string(attribute.value())] = m_structure->numAtoms();
      else // Atom nodes must have IDs - bail.
        return false;

      // Check for 3D geometry.
      attribute = node.attribute("x3");
      if (attribute) {
        xml_attribute& x3 = attribute;
        xml_attribute y3 = node.attribute("y3");
        xml_attribute z3 = node.attribute("z3");
        if (y3 && z3) {
          // It looks like we have a valid 3D position.
          atom.setPos(Vector3(x3.as_float(), y3.as_float(), z3.as_float()));
        } else {
          // Corrupt 3D position supplied for atom.
          return false;
        }
      } else if ((attribute = node.attribute("xFract"))) {
        if (!m_structure->hasUnitCell()) {
          m_error += "No unit cell defined. "
                     "Cannot interpret fractional coordinates.";
          return false;
        }
        xml_attribute& xF = attribute;
        xml_attribute yF = node.attribute("yFract");
        xml_attribute zF = node.attribute("zFract");
        if (yF && zF) {
          Vector3 coord((xF.as_float()), (yF.as_float()), (zF.as_float()));
          coord = m_structure->unitCell().toCartesian(coord);
          atom.setPos(coord);
        } else {
          m_error += "Missing y or z fractional coordinate on atom.";
          return false;
        }
      } else {
        m_error += "Atom positions not found!";
        return false;
      }

      m_structure->addAtom(atom);

      // Move on to the next atom node (if there is one).
      node = node.next_sibling("atom");
    }
    return true;
  }

  bool bonds()
  {
    xml_node bondArray = m_structureNode.child("bondArray");
    if (!bondArray)
      return true;

    xml_node node = bondArray.child("bond");

    while (node) {
      xml_attribute attribute = node.attribute("atomRefs2");
      size_t ind1, ind2;
      unsigned short bondOrder = 1;
      if (attribute) {
        // Should contain two elements separated by a space.
        std::string refs(attribute.value());
        std::vector<std::string> tokens = split(refs, ' ');
        if (tokens.size() != 2) // Corrupted file/input we don't understand
          return false;
        std::map<std::string, size_t>::const_iterator begin, end;
        begin = m_atomIds.find(tokens[0]);
        end = m_atomIds.find(tokens[1]);
        if (begin != m_atomIds.end() && end != m_atomIds.end() &&
            begin->second < m_structure->numAtoms() &&
            end->second < m_structure->numAtoms()) {
          ind1 = begin->second;
          ind2 = end->second;
        } else { // Couldn't parse the bond begin and end.
          m_error += "Failed to parse bond indices.";
          return false;
        }
      }

      attribute = node.attribute("order");
      if (attribute && strlen(attribute.value()) == 1) {
        char o = attribute.value()[0];
        switch (o) {
          case '1':
          case 'S':
          case 's':
            bondOrder = 1;
            break;
          case '2':
          case 'D':
          case 'd':
            bondOrder = 2;
            break;
          case '3':
          case 'T':
          case 't':
            bondOrder = 3;
            break;
          case '4':
            bondOrder = 4;
            break;
          case '5':
            bondOrder = 5;
            break;
          case '6':
            bondOrder = 6;
            break;
          default:
            bondOrder = 1;
        }
      } else {
        bondOrder = 1;
      }
      m_structure->addBond(ind1, ind2, bondOrder);

      // Move on to the next bond node (if there is one).
      node = node.next_sibling("bond");
    }

    return true;
  }

  bool properties()
  {
    bool enthalpyFound = false, energyFound = false;
    double enthalpy = 0.0, energy = 0.0;

    xml_node propertyList = m_structureNode.child("propertyList");

    // Properties aren't essential. If they don't exist, just return
    if (!propertyList)
      return true;

    xml_node property = propertyList.child("property");

    while (property) {
      // What is the title of our property?
      xml_attribute title = property.attribute("title");
      if (title) {
        // Is this enthalpy?
        if (std::string(title.value()) == "Enthalpy (eV)") {
          xml_node scalar = property.child("scalar");
          if (!scalar) {
            m_error += "Warning: Enthalpy (eV) exists, but no value!";
            return false;
          }
          enthalpyFound = true;
          enthalpy = scalar.text().as_float();
          // std::cout << "Enthalpy is " << enthalpy << "\n";
        }
        // Is this energy?
        else if (std::string(title.value()) == "Energy") {
          xml_node scalar = property.child("scalar");
          if (!scalar) {
            m_error += "Warning: Energy exists, but no value!";
            return false;
          }
          energy = scalar.text().as_float();

          xml_attribute units = scalar.attribute("units");

          if (units) {
            if (std::string(units.value()) == "kJ/mol") {
              energy *= KJ_PER_MOL_TO_EV;
            } else if (std::string(units.value()) == "eV") {
              // Do nothing...
            } else {
              m_error += ("Warning: we do not have a unit conversion for " +
                          std::string(units.value()) + "yet. Please email " +
                          "a developer of this program about this.");
              return false;
            }
          }
          // If there aren't any units, we'll just assume eV...

          energyFound = true;
          // std::cout << "Energy is " << energy << "\n"; // TMP
        }
      } else {
        // If there is no title, this property node is corrupt.
        m_error += "Warning: no title found for a property.";
        return false;
      }

      // Move on to the next property node (if there is one).
      property = property.next_sibling("property");
    }
    if (energyFound)
      m_structure->setEnergy(energy);
    if (enthalpyFound)
      m_structure->setEnthalpy(enthalpy);
    if (enthalpyFound && !energyFound)
      m_structure->setEnergy(enthalpy);

    return true;
  }

  bool m_success;
  Structure* m_structure;
  xml_node m_structureNode;
  std::map<std::string, size_t> m_atomIds;
  string m_error;
};
}

bool CmlFormat::read(Structure& s, std::istream& file)
{
  xml_document document;
  pugi::xml_parse_result result = document.load(file);
  if (!result) {
    std::cerr << "Error parsing XML in CML file: " +
                   std::string(result.description()) + "\n";
    return false;
  }

  CmlFormatPrivate parser(&s, document);
  if (!parser.m_success)
    std::cerr << "Error reading CML file: " << parser.m_error;

  return parser.m_success;
}

bool CmlFormat::write(const Structure& s, std::ostream& out)
{
  xml_document document;

  // Add a custom declaration node.
  xml_node declaration = document.prepend_child(pugi::node_declaration);
  declaration.append_attribute("version") = "1.0";
  declaration.append_attribute("encoding") = "UTF-8";

  xml_node structureNode = document.append_child("molecule");
  // Standard XML namespaces for CML.
  structureNode.append_attribute("xmlns") = "http://www.xml-cml.org/schema";
  structureNode.append_attribute("xmlns:cml") =
    "http://www.xml-cml.org/dict/cml";
  structureNode.append_attribute("xmlns:units") =
    "http://www.xml-cml.org/units/units";
  structureNode.append_attribute("xmlns:xsd") =
    "http://www.w3c.org/2001/XMLSchema";
  structureNode.append_attribute("xmlns:iupac") = "http://www.iupac.org";

  // Cell specification
  const UnitCell& cell = s.unitCell();
  if (cell.isValid()) {
    xml_node crystalNode = structureNode.append_child("crystal");

    xml_node crystalANode = crystalNode.append_child("scalar");
    xml_node crystalBNode = crystalNode.append_child("scalar");
    xml_node crystalCNode = crystalNode.append_child("scalar");
    xml_node crystalAlphaNode = crystalNode.append_child("scalar");
    xml_node crystalBetaNode = crystalNode.append_child("scalar");
    xml_node crystalGammaNode = crystalNode.append_child("scalar");

    crystalANode.append_attribute("title") = "a";
    crystalBNode.append_attribute("title") = "b";
    crystalCNode.append_attribute("title") = "c";
    crystalAlphaNode.append_attribute("title") = "alpha";
    crystalBetaNode.append_attribute("title") = "beta";
    crystalGammaNode.append_attribute("title") = "gamma";

    crystalANode.append_attribute("units") = "units:angstrom";
    crystalBNode.append_attribute("units") = "units:angstrom";
    crystalCNode.append_attribute("units") = "units:angstrom";
    crystalAlphaNode.append_attribute("units") = "units:degree";
    crystalBetaNode.append_attribute("units") = "units:degree";
    crystalGammaNode.append_attribute("units") = "units:degree";

    crystalANode.text() = cell.a();
    crystalBNode.text() = cell.b();
    crystalCNode.text() = cell.c();
    crystalAlphaNode.text() = cell.alpha();
    crystalBetaNode.text() = cell.beta();
    crystalGammaNode.text() = cell.gamma();
  }

  xml_node atomArrayNode = structureNode.append_child("atomArray");
  for (size_t i = 0; i < s.numAtoms(); ++i) {
    xml_node atomNode = atomArrayNode.append_child("atom");
    std::ostringstream index;
    index << 'a' << i + 1;
    atomNode.append_attribute("id") = index.str().c_str();
    const Atom& atom = s.atom(i);
    atomNode.append_attribute("elementType") =
      ElemInfo::getAtomicSymbol(atom.atomicNumber()).c_str();
    if (cell.isValid()) {
      Vector3 fracPos = cell.toFractional(atom.pos());
      atomNode.append_attribute("xFract") = fracPos.x();
      atomNode.append_attribute("yFract") = fracPos.y();
      atomNode.append_attribute("zFract") = fracPos.z();
    } else {
      atomNode.append_attribute("x3") = atom.pos().x();
      atomNode.append_attribute("y3") = atom.pos().y();
      atomNode.append_attribute("z3") = atom.pos().z();
    }
  }

  xml_node bondArrayNode = structureNode.append_child("bondArray");
  for (size_t i = 0; i < s.numBonds(); ++i) {
    xml_node bondNode = bondArrayNode.append_child("bond");
    const Bond& b = s.bond(i);
    std::ostringstream index;
    index << "a" << b.first() + 1 << " a" << b.second() + 1;
    bondNode.append_attribute("atomRefs2") = index.str().c_str();
    bondNode.append_attribute("order") = b.bondOrder();
  }

  document.save(out, "  ");

  return true;
}

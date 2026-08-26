/**********************************************************************
  CmlFormat - Handlers for CML format structure files.

  Copyright (C) 2017 by Patrick Avery
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <atoms/formats/cmlformat.h>

#include <atoms/eleminfo.h>
#include <common/fileutils.h>
#include <common/output.h>
#include <atoms/geometry.h>

#include <QByteArray>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <common/compatibility/qt_compat.h>

#include <bitset>
#include <iostream>
#include <sstream>
#include <streambuf>

namespace Atoms {

namespace {

// Some helpers for reading file content

// Find the next molecule element, inside an optional <cml> wrapper.
bool skipToMolecule(QXmlStreamReader& xml)
{
  while (!xml.atEnd()) {
    xml.readNext();
    if (xml.isStartElement()) {
      if (xml.name() == QLatin1String("molecule"))
        return true;
      // Look for a molecule inside cml.
      if (xml.name() == QLatin1String("cml"))
        continue;
    }
  }
  return false;
}

bool readUnitCell(QXmlStreamReader& xml, Atoms::Geometry* s, std::string& error)
{
  // Read the crystal values in <crystal>.
  double a = 0, b = 0, c = 0, alpha = 0, beta = 0, gamma = 0;
  enum { CellA = 0, CellB, CellC, CellAlpha, CellBeta, CellGamma };
  std::bitset<6> parsed;

  while (!xml.atEnd()) {
    xml.readNext();
    if (xml.isEndElement() && xml.name() == QLatin1String("crystal"))
      break;
    if (!xml.isStartElement() || xml.name() != QLatin1String("scalar"))
      continue;

    QString title = xml.attributes().value("title").toString();
    bool ok = false;
    const double val = xml.readElementText().toDouble(&ok);
    if (!ok) {
      error += "Invalid number in unit cell description.";
      return false;
    }

    if (title == "a")     { a     = val; parsed.set(CellA); }
    else if (title == "b")     { b     = val; parsed.set(CellB); }
    else if (title == "c")     { c     = val; parsed.set(CellC); }
    else if (title == "alpha") { alpha = val; parsed.set(CellAlpha); }
    else if (title == "beta")  { beta  = val; parsed.set(CellBeta); }
    else if (title == "gamma") { gamma = val; parsed.set(CellGamma); }
  }

  if (parsed.count() != 6) {
    error += "Incomplete unit cell description.";
    return false;
  }
  s->setCellInfo(a, b, c, alpha, beta, gamma);
  return true;
}

bool readAtomArray(QXmlStreamReader& xml, Atoms::Geometry* s,
                          std::map<std::string, size_t>& atomIds, std::string& error)
{
  // Read the atoms in <atomArray>.
  while (!xml.atEnd()) {
    xml.readNext();
    if (xml.isEndElement() && xml.name() == QLatin1String("atomArray"))
      break;
    if (!xml.isStartElement() || xml.name() != QLatin1String("atom"))
      continue;

    Atoms::Atom atom;
    const QXmlStreamAttributes attrs = xml.attributes();

    // elementType
    auto elemType = attrs.value("elementType");
    if (elemType.isEmpty()) {
      error += "Warning, corrupt element node found.";
      return false;
    }
    const unsigned int atomicNum =
      Atoms::ElementInfo::getAtomicNum(elemType.toString().toStdString());
    if (atomicNum == 0) {
      error += "Unrecognized element symbol '" + elemType.toString().toStdString() + "'.";
      return false;
    }
    atom.setAtomicNumber(atomicNum);

    // id
    auto idRef = attrs.value("id");
    if (idRef.isEmpty())
      return false;
    atomIds[idRef.toString().toStdString()] = s->numAtoms();

    // Read Cartesian (if none, then fractional) coordinates.
    auto x3 = attrs.value("x3");
    if (!x3.isEmpty()) {
      auto y3 = attrs.value("y3");
      auto z3 = attrs.value("z3");
      if (y3.isEmpty() || z3.isEmpty()) {
        error += "Missing y or z Cartesian coordinate on atom.";
        return false;
      }
      bool xOk = false, yOk = false, zOk = false;
      const double x = x3.toString().toDouble(&xOk);
      const double y = y3.toString().toDouble(&yOk);
      const double z = z3.toString().toDouble(&zOk);
      if (!xOk || !yOk || !zOk) {
        error += "Invalid Cartesian coordinate on atom.";
        return false;
      }
      atom.setPos(Common::Vector3(x, y, z));
    } else {
      auto xFract = attrs.value("xFract");
      auto yFract = attrs.value("yFract");
      auto zFract = attrs.value("zFract");
      if (xFract.isEmpty() || yFract.isEmpty() || zFract.isEmpty()) {
        error += "Atom positions not found in CML input!";
        return false;
      }
      if (!s->is3D()) {
        error += "No unit cell defined. Cannot interpret fractional coordinates.";
        return false;
      }
      bool xOk = false, yOk = false, zOk = false;
      const double x = xFract.toString().toDouble(&xOk);
      const double y = yFract.toString().toDouble(&yOk);
      const double z = zFract.toString().toDouble(&zOk);
      if (!xOk || !yOk || !zOk) {
        error += "Invalid fractional coordinate on atom.";
        return false;
      }
      Common::Vector3 coord(x, y, z);
      coord = s->unitCell().toCartesian(coord);
      atom.setPos(coord);
    }

    s->addAtom(atom);
    xml.skipCurrentElement(); // Skip this element
  }
  return true;
}

bool readBondArray(QXmlStreamReader& xml, Atoms::Geometry* s,
                          const std::map<std::string, size_t>& atomIds, std::string& error)
{
  while (!xml.atEnd()) {
    xml.readNext();
    if (xml.isEndElement() && xml.name() == QLatin1String("bondArray"))
      break;
    if (!xml.isStartElement() || xml.name() != QLatin1String("bond"))
      continue;

    const QXmlStreamAttributes attrs = xml.attributes();
    QString refs = attrs.value("atomRefs2").toString();
    QStringList tokens = refs.split(' ', QtCompat::SkipEmptyParts);
    if (tokens.size() != 2) {
      error += "Failed to parse bond atomRefs2.";
      return false;
    }

    auto it1 = atomIds.find(tokens[0].toStdString());
    auto it2 = atomIds.find(tokens[1].toStdString());
    if (it1 == atomIds.end() || it2 == atomIds.end() ||
        it1->second >= s->numAtoms() || it2->second >= s->numAtoms()) {
      error += "Failed to parse bond indices.";
      return false;
    }

    unsigned short bondOrder = 1;
    QString order = attrs.value("order").toString();
    if (!order.isEmpty()) {
      char o = order.at(0).toLatin1();
      switch (o) {
        case '2': case 'D': case 'd': bondOrder = 2; break;
        case '3': case 'T': case 't': bondOrder = 3; break;
        case '4': bondOrder = 4; break;
        case '5': bondOrder = 5; break;
        case '6': bondOrder = 6; break;
        default:  bondOrder = 1; break;
      }
    }
    s->addBond(it1->second, it2->second, bondOrder);
    xml.skipCurrentElement();
  }
  return true;
}

} // anonymous namespace

bool CmlFormat::read(Atoms::Geometry& s, const QString& filename)
{
  std::string text;
  if (!Common::readFileToString(filename, &text)) {
    Common::error(QString("Failed to open CML file: %1").arg(filename));
    return false;
  }

  std::istringstream in(text);
  Atoms::Geometry parsed;
  if (!read(parsed, in))
    return false;

  s = parsed;
  return true;
}

bool CmlFormat::read(Atoms::Geometry& s, std::istream& file)
{
  // Read the input stream into a QByteArray.
  const std::string text{ std::istreambuf_iterator<char>(file),
                          std::istreambuf_iterator<char>() };
  QByteArray data(text.c_str(), static_cast<int>(text.size()));

  QXmlStreamReader xml(data);

  bool moleculeFound = false;

  // A CML file may hold several molecule entries; the last one is read.
  while (skipToMolecule(xml)) {
    s.clear();

    std::string error;
    std::map<std::string, size_t> atomIds;
    bool success = true;

    while (!xml.atEnd() && success) {
      xml.readNext();
      if (xml.isEndElement() && xml.name() == QLatin1String("molecule"))
        break;
      if (!xml.isStartElement())
        continue;

      if (xml.name() == QLatin1String("crystal")) {
        success = readUnitCell(xml, &s, error);
      } else if (xml.name() == QLatin1String("atomArray")) {
        success = readAtomArray(xml, &s, atomIds, error);
      } else if (xml.name() == QLatin1String("bondArray")) {
        success = readBondArray(xml, &s, atomIds, error);
      } else if (xml.name() == QLatin1String("propertyList")) {
        xml.skipCurrentElement();
      }
    }

    if (xml.hasError()) {
      Common::error(QString("Could not parse XML in CML file: %1")
                   .arg(xml.errorString()));
      return false;
    }

    if (!success) {
      Common::error(QString("Could not read CML file: %1")
                   .arg(QString::fromStdString(error)));
      return false;
    }

    moleculeFound = true;
  }

  if (!moleculeFound) {
    Common::error("No molecule element found in CML file.");
    return false;
  }

  return true;
}

bool CmlFormat::write(const Atoms::Geometry& s, std::ostream& out)
{
  QByteArray buffer;
  QXmlStreamWriter xml(&buffer);
  xml.setAutoFormatting(true);
  xml.setAutoFormattingIndent(2);

  xml.writeStartDocument("1.0");

  xml.writeStartElement("molecule");
  xml.writeAttribute("xmlns",       "http://www.xml-cml.org/schema");
  xml.writeAttribute("xmlns:cml",   "http://www.xml-cml.org/dict/cml");
  xml.writeAttribute("xmlns:units", "http://www.xml-cml.org/units/units");
  xml.writeAttribute("xmlns:xsd",   "http://www.w3c.org/2001/XMLSchema");
  xml.writeAttribute("xmlns:iupac", "http://www.iupac.org");

  // Unit cell
  const Atoms::UnitCell& cell = s.unitCell();
  if (cell.is3D()) {
    xml.writeStartElement("crystal");

    auto writeScalar = [&](const char* title, const char* units, double val) {
      xml.writeStartElement("scalar");
      xml.writeAttribute("title", title);
      xml.writeAttribute("units", units);
      xml.writeCharacters(QString::number(val, 'f', 8));
      xml.writeEndElement();
    };
    writeScalar("a",     "units:angstrom", cell.a());
    writeScalar("b",     "units:angstrom", cell.b());
    writeScalar("c",     "units:angstrom", cell.c());
    writeScalar("alpha", "units:degree",   cell.alpha());
    writeScalar("beta",  "units:degree",   cell.beta());
    writeScalar("gamma", "units:degree",   cell.gamma());

    xml.writeEndElement(); // crystal
  }

  // Atoms
  xml.writeStartElement("atomArray");
  for (size_t i = 0; i < s.numAtoms(); ++i) {
    const Atoms::Atom& atom = s.atom(i);
    xml.writeStartElement("atom");
    xml.writeAttribute("id", QString("a%1").arg(i + 1));
    xml.writeAttribute("elementType", QString::fromStdString(
        Atoms::ElementInfo::getAtomicSymbol(atom.atomicNumber())));
    if (cell.is3D()) {
      Common::Vector3 frac = cell.toFractional(atom.pos());
      xml.writeAttribute("xFract", QString::number(frac.x(), 'f', 8));
      xml.writeAttribute("yFract", QString::number(frac.y(), 'f', 8));
      xml.writeAttribute("zFract", QString::number(frac.z(), 'f', 8));
    } else {
      xml.writeAttribute("x3", QString::number(atom.pos().x(), 'f', 8));
      xml.writeAttribute("y3", QString::number(atom.pos().y(), 'f', 8));
      xml.writeAttribute("z3", QString::number(atom.pos().z(), 'f', 8));
    }
    xml.writeEndElement(); // atom
  }
  xml.writeEndElement(); // atomArray

  // Bonds
  xml.writeStartElement("bondArray");
  for (size_t i = 0; i < s.numBonds(); ++i) {
    const Atoms::Bond& b = s.bond(i);
    xml.writeStartElement("bond");
    xml.writeAttribute("atomRefs2", QString("a%1 a%2").arg(b.first() + 1).arg(b.second() + 1));
    xml.writeAttribute("order", QString::number(b.bondOrder()));
    xml.writeEndElement(); // bond
  }
  xml.writeEndElement(); // bondArray

  xml.writeEndElement(); // molecule
  xml.writeEndDocument();

  out << buffer.constData();
  return true;
}

} // namespace Atoms

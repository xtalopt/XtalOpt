/**********************************************************************
  CifFormat - Handlers for CIF format structure files.

  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <atoms/formats/cifformat.h>

#include <common/compatibility/qt_compat.h>
#include <atoms/eleminfo.h>
#include <common/constants.h>
#include <common/output.h>
#include <common/numericutils.h>
#include <atoms/geometry.h>

#include <spglib/spglib.h>

#include <QFile>
#include <QStringList>
#include <QTextStream>

#include <cmath>
#include <iomanip>
#include <limits>
#include <map>
#include <ostream>
#include <vector>

// read() gets the cell and atom sites, applies the symmetry operations,
//   and checks the formula and space group. write() stores every atom in P1.

namespace Atoms {

namespace {

struct CifAtomSite
{
  QString symbol;
  Common::Vector3 frac;
  int multiplicity = 0;
};

struct SymOp
{
  int rot[3][3];
  double trans[3];
};

struct AtomKey
{
  unsigned short atomicNumber;
  Common::Vector3 frac;
};

QString stripQuotes(const QString& value)
{
  QString ret = value.trimmed();
  if (ret.size() >= 2 && ((ret.startsWith("'") && ret.endsWith("'")) ||
       (ret.startsWith("\"") && ret.endsWith("\""))))
    ret = ret.mid(1, ret.size() - 2);
  return ret.trimmed();
}

QStringList parseCif(const QString& data)
{
  QStringList all_str;
  QString str;
  bool inQuote = false;
  QChar quote;
  bool atLineStart = true;

  for (int i = 0; i < data.size(); ++i) {
    const QChar c = data.at(i);

    if (!inQuote && atLineStart && c == ';') {
      int end = data.indexOf("\n;", i + 1);
      if (end < 0)
        break;
      all_str.append(data.mid(i + 1, end - i - 1).trimmed());
      i = end + 1;
      atLineStart = false;
      continue;
    }

    if (!inQuote && c == '#') {
      while (i < data.size() && data.at(i) != '\n')
        ++i;
      atLineStart = true;
      continue;
    }

    if (inQuote) {
      if (c == quote) {
        all_str.append(str);
        str.clear();
        inQuote = false;
      } else {
        str.append(c);
      }
      atLineStart = (c == '\n');
      continue;
    }

    if (c == '\'' || c == '"') {
      if (!str.isEmpty()) {
        all_str.append(str);
        str.clear();
      }
      inQuote = true;
      quote = c;
      atLineStart = false;
      continue;
    }

    if (c.isSpace()) {
      if (!str.isEmpty()) {
        all_str.append(str);
        str.clear();
      }
      atLineStart = (c == '\n' || c == '\r');
      continue;
    }

    str.append(c);
    atLineStart = false;
  }

  if (!str.isEmpty())
    all_str.append(str);

  return all_str;
}

QString normalizeSymbol(QString symbol)
{
  symbol = stripQuotes(symbol);
  QString letters;
  for (int i = 0; i < symbol.size(); ++i) {
    if (!symbol.at(i).isLetter())
      break;
    letters.append(symbol.at(i));
  }

  const unsigned int atomicNum = Atoms::ElementInfo::getAtomicNum(letters.toStdString());
  if (atomicNum == 0)
    return QString();

  return QString::fromStdString(Atoms::ElementInfo::getAtomicSymbol(atomicNum));
}

double parseCifDouble(QString value, bool* ok = nullptr)
{
  value = stripQuotes(value);
  const int paren = value.indexOf('(');
  if (paren >= 0)
    value = value.left(paren);
  if (value == "." || value == "?") {
    if (ok)
      *ok = false;
    return 0.0;
  }
  return value.toDouble(ok);
}

std::map<unsigned short, double> parseFormulaCounts(QString formula)
{
  std::map<unsigned short, double> counts;
  formula = stripQuotes(formula);

  int i = 0;
  while (i < formula.size()) {
    while (i < formula.size() && !formula.at(i).isUpper())
      ++i;
    if (i >= formula.size())
      break;

    QString symbol;
    symbol.append(formula.at(i++));
    if (i < formula.size() && formula.at(i).isLower())
      symbol.append(formula.at(i++));

    QString number;
    while (i < formula.size()) {
      const QChar c = formula.at(i);
      if (c.isDigit() || c == '.' || c == '+' || c == '-')
        number.append(c);
      else
        break;
      ++i;
    }

    bool ok = true;
    double count = 1.0;
    if (!number.isEmpty())
      count = number.toDouble(&ok);

    const unsigned short atomicNumber =
      static_cast<unsigned short>(Atoms::ElementInfo::getAtomicNum(symbol.toStdString()));
    if (ok && atomicNumber != 0)
      counts[atomicNumber] += count;
  }

  return counts;
}

double parseFraction(QString expr, bool* ok)
{
  expr = stripQuotes(expr);
  const int slash = expr.indexOf('/');
  if (slash >= 0) {
    bool ok1 = false, ok2 = false;
    const double numerator = expr.left(slash).toDouble(&ok1);
    const double denominator = expr.mid(slash + 1).toDouble(&ok2);
    if (ok1 && ok2 && std::fabs(denominator) > ZERO12) {
      if (ok)
        *ok = true;
      return numerator / denominator;
    }
    if (ok)
      *ok = false;
    return 0.0;
  }
  return parseCifDouble(expr, ok);
}

bool parseCoordinateExpr(QString expr, int rot[3], double& trans)
{
  expr = stripQuotes(expr).toLower();
  expr.remove(' ');
  expr.replace("-", "+-");
  const QStringList terms = expr.split("+", QtCompat::SkipEmptyParts);

  rot[0] = rot[1] = rot[2] = 0;
  trans = 0.0;

  for (QString term : terms) {
    if (term == "x" || term == "+x")
      rot[0] += 1;
    else if (term == "-x")
      rot[0] -= 1;
    else if (term == "y" || term == "+y")
      rot[1] += 1;
    else if (term == "-y")
      rot[1] -= 1;
    else if (term == "z" || term == "+z")
      rot[2] += 1;
    else if (term == "-z")
      rot[2] -= 1;
    else {
      bool ok = false;
      const double value = parseFraction(term, &ok);
      if (!ok)
        return false;
      trans += value;
    }
  }

  return true;
}

bool parseSymOp(const QString& value, SymOp& op)
{
  const QStringList parts = stripQuotes(value).split(",", QtCompat::SkipEmptyParts);
  if (parts.size() != 3)
    return false;

  for (int i = 0; i < 3; ++i) {
    if (!parseCoordinateExpr(parts.at(i), op.rot[i], op.trans[i]))
      return false;
  }
  return true;
}

QString normalizeSpaceGroupText(QString text)
{
  text = stripQuotes(text).toLower();
  QString normalized;
  for (int i = 0; i < text.size(); ++i) {
    const QChar c = text.at(i);
    if (c != ' ' && c != '_' && c != '\'')
      normalized.append(c);
  }
  text = normalized;
  return text;
}

// First Hall number whose spacegroup type satisfies pred, or 0 if none.
template <typename Pred>
int firstHallMatching(Pred pred)
{
  for (int hall = 1; hall <= 530; ++hall) {
    const SpglibSpacegroupType type = spg_get_spacegroup_type(hall);
    if (pred(type))
      return hall;
  }
  return 0;
}

int findHallNumber(const QString& hallSymbol, const QString& hmSymbol, int spaceGroupNumber)
{
  const QString targetHall = normalizeSpaceGroupText(hallSymbol);
  const QString targetHm = normalizeSpaceGroupText(hmSymbol);

  if (!targetHall.isEmpty()) {
    const int hall = firstHallMatching([&](const SpglibSpacegroupType& type) {
      return type.number != 0 && normalizeSpaceGroupText(type.hall_symbol) == targetHall;
    });
    if (hall)
      return hall;
  }

  // Start by checking if H-M symbol is present.
  if (!targetHm.isEmpty()) {
    const int hall = firstHallMatching([&](const SpglibSpacegroupType& type) {
      return type.number != 0 && (normalizeSpaceGroupText(type.international_short) == targetHm ||
                                  normalizeSpaceGroupText(type.international_full) == targetHm ||
                                  normalizeSpaceGroupText(type.international) == targetHm);
    });
    if (hall)
      return hall;
  }

  if (spaceGroupNumber > 0) {
    const int hall = firstHallMatching([&](const SpglibSpacegroupType& type) {
      return type.number == spaceGroupNumber;
    });
    if (hall)
      return hall;
  }

  return 0;
}

std::vector<SymOp> symmetryFromDatabase(int hallNumber)
{
  std::vector<SymOp> operations;
  if (hallNumber <= 0)
    return operations;

  int rotations[192][3][3];
  double translations[192][3];
  const int count = spg_get_symmetry_from_database(rotations, translations, hallNumber);
  for (int n = 0; n < count; ++n) {
    SymOp op;
    for (int i = 0; i < 3; ++i) {
      op.trans[i] = translations[n][i];
      for (int j = 0; j < 3; ++j)
        op.rot[i][j] = rotations[n][i][j];
    }
    operations.push_back(op);
  }
  return operations;
}

Common::Vector3 applySymOp(const SymOp& op, const Common::Vector3& frac)
{
  Common::Vector3 ret;
  for (int i = 0; i < 3; ++i) {
    ret[i] = op.trans[i];
    for (int j = 0; j < 3; ++j)
      ret[i] += op.rot[i][j] * frac[j];
  }
  return ret;
}

bool sameFractionalPosition(const Common::Vector3& a, const Common::Vector3& b, double tol)
{
  for (int i = 0; i < 3; ++i) {
    double diff = a[i] - b[i];
    diff -= std::floor(diff + 0.5);
    if (std::fabs(diff) > tol)
      return false;
  }
  return true;
}

void appendUniqueAtom(std::vector<AtomKey>& atoms, unsigned short atomicNumber,
                      const Common::Vector3& frac, const Atoms::UnitCell& cell)
{
  const Common::Vector3 wrapped = cell.wrapFractional(frac);
  for (const auto& atom : atoms) {
    if (atom.atomicNumber == atomicNumber && sameFractionalPosition(atom.frac, wrapped, ZERO05))
      return;
  }
  AtomKey atom;
  atom.atomicNumber = atomicNumber;
  atom.frac = wrapped;
  atoms.push_back(atom);
}

std::map<unsigned short, int> atomCounts(const std::vector<AtomKey>& atoms)
{
  std::map<unsigned short, int> counts;
  for (const AtomKey& atom : atoms)
    counts[atom.atomicNumber] += 1;
  return counts;
}

bool formulaMatchesAtoms(const std::map<unsigned short, double>& formula,
                         const std::map<unsigned short, int>& atoms, double scale)
{
  if (formula.empty())
    return true;

  for (const auto& atomCount : atoms) {
    const auto it = formula.find(atomCount.first);
    const double expected = (it == formula.end()) ? 0.0 : it->second * scale;
    if (Common::neq(expected, atomCount.second, ZERO03))
      return false;
  }

  for (const auto& formulaCount : formula) {
    if (atoms.find(formulaCount.first) == atoms.end() &&
        std::fabs(formulaCount.second * scale) > ZERO03)
      return false;
  }

  return true;
}

QString countsToString(const std::map<unsigned short, int>& counts)
{
  QStringList parts;
  for (const auto& count : counts) {
    parts.append(QString("%1%2")
                 .arg(QString::fromStdString(
                   Atoms::ElementInfo::getAtomicSymbol(count.first)))
                 .arg(count.second));
  }
  return parts.join(" ");
}

QString formulaToString(const std::map<unsigned short, double>& counts, double scale)
{
  QStringList parts;
  for (const auto& count : counts) {
    parts.append(QString("%1%2")
                 .arg(QString::fromStdString(
                   Atoms::ElementInfo::getAtomicSymbol(count.first)))
                 .arg(count.second * scale, 0, 'g', 8));
  }
  return parts.join(" ");
}

} // namespace



bool CifFormat::read(Geometry& s, const QString& filename)
{
  // Read the complete data block first; some entries can refer to subsequent fields.
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    Common::error(QString("CIF file, %1, could not be opened!").arg(filename));
    return false;
  }

  QTextStream stream(&file);
  const QStringList all_str = parseCif(stream.readAll());
  if (all_str.isEmpty()) {
    Common::error(QString("CIF file, %1, is empty.").arg(filename));
    return false;
  }
  int dataBlocks = 0;
  for (int i = 0; i < all_str.size(); ++i) {
    if (all_str.at(i).startsWith("data_", Qt::CaseInsensitive))
      ++dataBlocks;
  }
  if (dataBlocks > 1) {
    Common::error(QString("CIF file, %1, contains more than one data block.").arg(filename));
    return false;
  }

  // Read the scalar values, atom sites, and symmetry operations (if any).
  std::map<QString, QString> values;
  std::vector<CifAtomSite> sites;
  std::vector<QString> explicitSymOps;
  std::map<unsigned short, double> formulaCounts;

  for (int i = 0; i < all_str.size();) {
    const QString str = all_str.at(i);
    const QString lower = str.toLower();

    if (lower == "loop_") {
      ++i;
      QStringList headers;
      while (i < all_str.size() && all_str.at(i).startsWith("_"))
        headers.append(all_str.at(i++).toLower());
      if (headers.isEmpty())
        continue;

      QList<QStringList> rows;
      while (i < all_str.size()) {
        const QString nextLower = all_str.at(i).toLower();
        if (nextLower == "loop_" || nextLower.startsWith("data_") || all_str.at(i).startsWith("_"))
          break;
        QStringList row;
        for (int j = 0; j < headers.size() && i < all_str.size(); ++j)
          row.append(all_str.at(i++));
        if (row.size() == headers.size())
          rows.append(row);
      }

      const int symIndex1 = headers.indexOf("_symmetry_equiv_pos_as_xyz");
      const int symIndex2 = headers.indexOf("_space_group_symop_operation_xyz");
      const int symIndex = (symIndex1 >= 0) ? symIndex1 : symIndex2;
      if (symIndex >= 0) {
        for (const QStringList& row : rows)
          explicitSymOps.push_back(row.at(symIndex));
      }

      const int fx = headers.indexOf("_atom_site_fract_x");
      const int fy = headers.indexOf("_atom_site_fract_y");
      const int fz = headers.indexOf("_atom_site_fract_z");
      if (fx >= 0 && fy >= 0 && fz >= 0) {
        const int typeIndex = headers.indexOf("_atom_site_type_symbol");
        const int labelIndex = headers.indexOf("_atom_site_label");
        const int mult = headers.indexOf("_atom_site_symmetry_multiplicity");
        const int occupancy = headers.indexOf("_atom_site_occupancy");
        if (typeIndex >= 0 || labelIndex >= 0) {
          for (const QStringList& row : rows) {
            bool okx = false, oky = false, okz = false;
            CifAtomSite site;
            if (typeIndex >= 0) {
              const QString typeValue = stripQuotes(row.at(typeIndex));
              if (typeValue.isEmpty() || typeValue == "." || typeValue == "?") {
                if (labelIndex >= 0)
                  site.symbol = normalizeSymbol(row.at(labelIndex));
              } else {
                site.symbol = normalizeSymbol(typeValue);
                if (site.symbol.isEmpty()) {
                  Common::error(QString("CIF file, %1, contains an invalid "
                                        "atom type symbol: %2")
                                        .arg(filename).arg(typeValue));
                  return false;
                }
              }
            } else {
              site.symbol = normalizeSymbol(row.at(labelIndex));
            }
            if (site.symbol.isEmpty()) {
              const QString label =
                labelIndex >= 0 ? stripQuotes(row.at(labelIndex)) : QString();
              Common::error(QString("CIF file, %1, contains an atom site "
                                    "without a valid element symbol: %2")
                                    .arg(filename).arg(label));
              return false;
            }
            site.frac = Common::Vector3(parseCifDouble(row.at(fx), &okx),
                                parseCifDouble(row.at(fy), &oky), parseCifDouble(row.at(fz), &okz));
            if (mult >= 0) {
              bool ok = false;
              site.multiplicity = stripQuotes(row.at(mult)).toInt(&ok);
              if (!ok)
                site.multiplicity = 0;
            }
            if (occupancy >= 0) {
              bool occupancyOk = false;
              const double value =
                parseCifDouble(row.at(occupancy), &occupancyOk);
              if (!occupancyOk || std::fabs(value - 1.0) > ZERO05) {
                Common::error(QString("CIF file, %1, contains a partial or "
                                      "invalid atom-site occupancy.")
                                      .arg(filename));
                return false;
              }
            }
            if (okx && oky && okz)
              sites.push_back(site);
          }
        }
      }
      continue;
    }

    if (str.startsWith("_") && i + 1 < all_str.size()) {
      values[str.toLower()] = all_str.at(i + 1);
      i += 2;
      continue;
    }

    ++i;
  }

  // A periodic cell and at least one atom site are required for expanding the cell.
  bool okA = false, okB = false, okC = false;
  bool okAlpha = false, okBeta = false, okGamma = false;
  const double a = parseCifDouble(values["_cell_length_a"], &okA);
  const double b = parseCifDouble(values["_cell_length_b"], &okB);
  const double c = parseCifDouble(values["_cell_length_c"], &okC);
  const double alpha = parseCifDouble(values["_cell_angle_alpha"], &okAlpha);
  const double beta = parseCifDouble(values["_cell_angle_beta"], &okBeta);
  const double gamma = parseCifDouble(values["_cell_angle_gamma"], &okGamma);

  if (!okA || !okB || !okC || !okAlpha || !okBeta || !okGamma) {
    Common::error(QString("CIF file, %1, does not contain a complete unit cell.")
                   .arg(filename));
    return false;
  }

  if (sites.empty()) {
    Common::error(QString("CIF file, %1, does not contain atom-site fractional coordinates.")
                   .arg(filename));
    return false;
  }

  UnitCell cell(a, b, c, alpha, beta, gamma);

  if (values.count("_chemical_formula_sum"))
    formulaCounts = parseFormulaCounts(values["_chemical_formula_sum"]);
  else if (values.count("_chemical_formula_structural"))
    formulaCounts = parseFormulaCounts(values["_chemical_formula_structural"]);

  // Use symmetry ops if given; if not try to recover them from space group information.
  std::vector<SymOp> operations;
  for (const QString& opText : explicitSymOps) {
    SymOp op;
    if (!parseSymOp(opText, op)) {
      Common::error(QString("CIF file, %1, contains an invalid symmetry "
                            "operation: %2").arg(filename).arg(opText));
      return false;
    }
    operations.push_back(op);
  }

  if (operations.empty()) {
    int spgNumber = 0;
    bool okNumber = false;
    if (values.count("_space_group_it_number"))
      spgNumber = stripQuotes(values["_space_group_it_number"]).toInt(&okNumber);
    if (!okNumber && values.count("_symmetry_int_tables_number"))
      spgNumber = stripQuotes(values["_symmetry_int_tables_number"]).toInt(&okNumber);

    QString hall;
    if (values.count("_space_group_name_hall"))
      hall = values["_space_group_name_hall"];

    QString hm;
    if (values.count("_space_group_name_h-m_alt"))
      hm = values["_space_group_name_h-m_alt"];
    else if (values.count("_symmetry_space_group_name_h-m"))
      hm = values["_symmetry_space_group_name_h-m"];

    const int hallNumber = findHallNumber(hall, hm, okNumber ? spgNumber : 0);
    operations = symmetryFromDatabase(hallNumber);
    if (operations.empty()) {
      Common::error(QString("CIF file, %1, does not contain usable symmetry information.")
                     .arg(filename));
      return false;
    }
  }

  // Expand the given atom sites into full conventional cell.
  // A duplicate position produced by an atom is kept only once!
  std::vector<AtomKey> expandedAtoms;
  for (const CifAtomSite& site : sites) {
    const unsigned short atomicNumber =
      static_cast<unsigned short>(Atoms::ElementInfo::getAtomicNum(site.symbol.toStdString()));
    if (atomicNumber == 0) {
      Common::error(QString("CIF file, %1, has unknown atom symbol '%2'.")
                     .arg(filename)
                     .arg(site.symbol));
      return false;
    }

    const size_t before = expandedAtoms.size();
    for (const SymOp& op : operations)
      appendUniqueAtom(expandedAtoms, atomicNumber, applySymOp(op, site.frac), cell);

    if (site.multiplicity > 0 &&
        static_cast<int>(expandedAtoms.size() - before) != site.multiplicity) {
      Common::warning(QString("CIF atom %1 generated %2 unique positions, "
                             "but multiplicity is %3.")
                       .arg(site.symbol)
                       .arg(expandedAtoms.size() - before)
                       .arg(site.multiplicity));
    }
  }

  if (expandedAtoms.empty()) {
    Common::error(QString("CIF file, %1, did not produce any atoms.").arg(filename));
    return false;
  }

  // Sanity check: If formula is given, use it to verify the constructed cell.
  if (!formulaCounts.empty()) {
    const std::map<unsigned short, int> finalCounts = atomCounts(expandedAtoms);
    bool zOk = false;
    const double z = values.count("_cell_formula_units_z") ?
      parseCifDouble(values["_cell_formula_units_z"], &zOk) : 0.0;
    const bool directMatch = formulaMatchesAtoms(formulaCounts, finalCounts, 1.0);
    const bool zMatch = zOk && formulaMatchesAtoms(formulaCounts, finalCounts, z);
    if (!directMatch && !zMatch) {
      Common::warning(QString("CIF formula check for %1 did not match expanded "
                             "atoms. Formula: %2%3; expanded atoms: %4.")
                     .arg(filename)
                     .arg(formulaToString(formulaCounts, 1.0))
                     .arg(zOk ? QString(" (Z=%1 gives %2)")
                                  .arg(z, 0, 'g', 8)
                                  .arg(formulaToString(formulaCounts, z))
                              : QString())
                     .arg(countsToString(finalCounts)));
    }
  }

  // Sanity check: if symmetry was given, verify the produced cell against it.
  {
    int declaredNumber = 0;
    bool okNumber = false;
    if (values.count("_space_group_it_number"))
      declaredNumber = stripQuotes(values["_space_group_it_number"]).toInt(&okNumber);
    if (!okNumber && values.count("_symmetry_int_tables_number"))
      declaredNumber = stripQuotes(values["_symmetry_int_tables_number"]).toInt(&okNumber);

    QString declaredHm;
    if (values.count("_space_group_name_h-m_alt"))
      declaredHm = values["_space_group_name_h-m_alt"];
    else if (values.count("_symmetry_space_group_name_h-m"))
      declaredHm = values["_symmetry_space_group_name_h-m"];

    if (okNumber || !stripQuotes(declaredHm).isEmpty()) {
      double lattice[3][3];
      cellToColumnLatticeArray(cell.cellMatrix(), lattice);

      std::vector<double> positionsBuffer(expandedAtoms.size() * 3);
      std::vector<int> types(expandedAtoms.size());
      for (size_t i = 0; i < expandedAtoms.size(); ++i) {
        positionsBuffer[3 * i + 0] = expandedAtoms[i].frac[0];
        positionsBuffer[3 * i + 1] = expandedAtoms[i].frac[1];
        positionsBuffer[3 * i + 2] = expandedAtoms[i].frac[2];
        types[i] = expandedAtoms[i].atomicNumber;
      }
      const double (*positions)[3] = reinterpret_cast<const double (*)[3]>(positionsBuffer.data());

      SpglibDataset* dataset = spg_get_dataset(lattice, positions, types.data(),
                        static_cast<int>(expandedAtoms.size()), SPGLIB_TOL);
      if (!dataset) {
        Common::warning(QString("CIF space-group check for %1 failed: spglib "
                               "could not detect symmetry.")
                       .arg(filename));
      } else {
        const QString normalizedDeclaredHm = normalizeSpaceGroupText(declaredHm);
        const bool declaredP1 = okNumber ? declaredNumber == 1
                                         : normalizedDeclaredHm == "p1";
        const bool numberMismatch = okNumber && dataset->spacegroup_number != declaredNumber;
        const bool symbolMismatch = !normalizedDeclaredHm.isEmpty() &&
          normalizeSpaceGroupText(dataset->international_symbol) != normalizedDeclaredHm;
        if (!declaredP1 && (numberMismatch || (!okNumber && symbolMismatch))) {
          Common::warning(QString("CIF space-group check for %1 did not match. "
                                 "Declared: %2 %3; detected: %4 %5.")
                         .arg(filename)
                         .arg(okNumber ? QString::number(declaredNumber)
                                       : QString("-"))
                         .arg(stripQuotes(declaredHm))
                         .arg(dataset->spacegroup_number)
                         .arg(dataset->international_symbol));
        }
        spg_free_dataset(dataset);
      }
    }
  }

  // Set up the output Geometry.
  std::vector<Atom> atoms;
  atoms.reserve(expandedAtoms.size());
  for (const AtomKey& key : expandedAtoms)
    atoms.push_back(Atom(key.atomicNumber, cell.toCartesian(key.frac)));

  s.clear();
  s.setUnitCell(cell);
  s.setAtoms(atoms);
  return true;
}

bool CifFormat::write(const Geometry& s, std::ostream& out)
{
  if (!s.is3D() || s.numAtoms() < 1) {
    Common::error("CIF writer requires a periodic structure with atoms.");
    return false;
  }

  out << "data_generated\n";
  out << "_chemical_formula_sum           '"
      << s.getChemicalFormula().toStdString() << "'\n";
  out << std::defaultfloat
      << std::setprecision(std::numeric_limits<double>::max_digits10);
  out << "_cell_length_a                  " << s.getA() << "\n";
  out << "_cell_length_b                  " << s.getB() << "\n";
  out << "_cell_length_c                  " << s.getC() << "\n";
  out << "_cell_angle_alpha               " << s.getAlpha() << "\n";
  out << "_cell_angle_beta                " << s.getBeta() << "\n";
  out << "_cell_angle_gamma               " << s.getGamma() << "\n";
  out << "_cell_volume                    " << s.getVolume() << "\n";
  out << "_space_group_name_H-M_alt       'P 1'\n";
  out << "_space_group_IT_number          1\n";
  out << "_space_group_name_Hall          'P 1'\n\n";

  out << "loop_\n";
  out << "_space_group_symop_id\n";
  out << "_space_group_symop_operation_xyz\n";
  out << "1 'x,y,z'\n\n";

  out << "loop_\n";
  out << "_atom_site_label\n";
  out << "_atom_site_type_symbol\n";
  out << "_atom_site_fract_x\n";
  out << "_atom_site_fract_y\n";
  out << "_atom_site_fract_z\n";
  out << "_atom_site_occupancy\n";

  // Write every atom. P1 needs no symmetry expansion.
  std::map<QString, int> labelCounts;
  for (size_t atomIndex = 0; atomIndex < s.numAtoms(); ++atomIndex) {
    const QString symbol = QString::fromStdString(
      Atoms::ElementInfo::getAtomicSymbol(s.atom(atomIndex).atomicNumber()));
    const int labelIndex = ++labelCounts[symbol];
    const Common::Vector3 frac = s.unitCell().wrapFractional(
      s.cartToFrac(s.atom(atomIndex).pos()));

    out << symbol.toStdString() << labelIndex << " "
        << symbol.toStdString() << " "
        << frac.x() << " "
        << frac.y() << " "
        << frac.z() << " "
        << "1\n";
  }

  return true;
}

} // namespace Atoms

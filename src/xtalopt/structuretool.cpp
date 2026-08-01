/**********************************************************************
  structuretool - Collection of structure and molecule command-line tools

  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <common/constants.h>
#include <common/compatibility/qt_compat.h>
#include <common/output.h>
#include <atoms/formats/castepformat.h>
#include <atoms/formats/cifformat.h>
#include <atoms/formats/cmlformat.h>
#include <atoms/formats/formats.h>
#include <atoms/formats/gulpformat.h>
#include <atoms/formats/mtpformat.h>
#include <atoms/formats/poscarformat.h>
#include <atoms/formats/pwscfformat.h>
#include <atoms/formats/siestaformat.h>
#include <atoms/formats/xyzformat.h>
#include <atoms/eleminfo.h>
#include <atoms/generators.h>
#include <atoms/molecule.h>
#include <atoms/geometry.h>
#include <xtalopt/structuretool.h>

#include <common/fileutils.h>

#include <QString>
#include <QStringList>
#include <QMap>
#include <QTextStream>

extern "C" {
#include <args.h>
}

#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {

enum OutputCell
{
  Cell_AsRead = 0,
  Cell_Primitive,
  Cell_Conventional
};

// Selected run mode
enum ToolMode
{
  Mode_Structure = 0,
  Mode_Show,
  Mode_MolUnit,
  Mode_MolCrystal
};

enum MoleculeSource
{
  MoleculeSource_None = 0,
  MoleculeSource_Cartesian,
  MoleculeSource_FormulaTemplate,
  MoleculeSource_XyzFile
};

struct StructureToolOptions
{
  ToolMode mode = Mode_Structure;
  MoleculeSource molCrystalSource = MoleculeSource_None;

  // The "--input" should be an XYZ molecule file in molcrystal mode; otherwise a structure file.
  std::string inputFile;
  std::string inputFormat;

  std::string outputFile;
  std::string outputFormat;
  double symprec = SPGLIB_TOL;
  double distanceScale = 1.0;

  std::string compareFile;
  std::string compareXtalCompFile;
  std::string compareFormat;
  OutputCell outputCell = Cell_AsRead;
  double rdfCutoff = 6.0;
  double rdfSigma = 0.008;
  double rdfTolerance = 0.98;
  double xtalcompLengthTol = 0.1;
  double xtalcompAngleTol = 2.0;
  double rescaleVolume = 0.0;
  int rdfBins = 3000;
  bool printSymmetry = false;
  bool doNiggli = false;
  bool doStandardOrientation = false;
  bool doPrintVolume = false;
  bool doShortestDistance = false;
  bool doNearestNeighbors = false;
  bool doPrintRDF = false;
  bool doFormula = false;
  bool doRescaleVolume = false;
  bool showMoleculeCatalog = false;

  std::string molUnitRequest;
  std::string molCrystalFormulaRequest;
  std::string molCrystalCartesian;
  int molCrystalSpaceGroup = 0;

  QString errorMessage;
};

void registerParserOptions(ArgParser* parser, const StructureToolOptions& defaults)
{
  ap_add_flag(parser, "primitive");
  ap_add_flag(parser, "conventional");
  ap_add_flag(parser, "symmetry");
  ap_add_flag(parser, "niggli");
  ap_add_flag(parser, "standard-orient");
  ap_add_flag(parser, "volume");
  ap_add_flag(parser, "shortest-distance");
  ap_add_flag(parser, "nearest-neighbors");
  ap_add_flag(parser, "rdf");
  ap_add_flag(parser, "formula");
  ap_add_flag(parser, "molecules");
  ap_add_flag(parser, "molcrystal");

  ap_add_str_opt(parser, "input i", "");
  ap_add_str_opt(parser, "format f", "");
  ap_add_str_opt(parser, "output o", "");
  ap_add_str_opt(parser, "output-format", "");
  ap_add_str_opt(parser, "compare-rdf", "");
  ap_add_str_opt(parser, "compare-xtalcomp", "");
  ap_add_str_opt(parser, "compare-format", "");
  ap_add_str_opt(parser, "molunit", "");
  ap_add_int_opt(parser, "rdf-bins", defaults.rdfBins);
  ap_add_dbl_opt(parser, "rdf-cutoff", defaults.rdfCutoff);
  ap_add_dbl_opt(parser, "rdf-sigma", defaults.rdfSigma);
  ap_add_dbl_opt(parser, "rdf-tol", defaults.rdfTolerance);
  ap_add_dbl_opt(parser, "xtalcomp-tol-len", defaults.xtalcompLengthTol);
  ap_add_dbl_opt(parser, "xtalcomp-tol-ang", defaults.xtalcompAngleTol);
  ap_add_dbl_opt(parser, "rescale-volume", defaults.rescaleVolume);
  ap_add_dbl_opt(parser, "symprec", defaults.symprec);
  ap_add_dbl_opt(parser, "distance-scale", defaults.distanceScale);
}

OutputCell outputCellFromCommandLine(int argc, char* argv[])
{
  OutputCell outputCell = Cell_AsRead;
  for (int i = 1; i < argc; ++i) {
    const QString arg = QString::fromLocal8Bit(argv[i]);
    if (arg == "--primitive")
      outputCell = Cell_Primitive;
    else if (arg == "--conventional")
      outputCell = Cell_Conventional;
  }
  return outputCell;
}

void readRawOptions(ArgParser* parser, int argc, char* argv[], StructureToolOptions& options)
{
  options.outputCell = outputCellFromCommandLine(argc, argv);
  options.printSymmetry = ap_found(parser, "symmetry");
  options.doNiggli = ap_found(parser, "niggli");
  options.doStandardOrientation = ap_found(parser, "standard-orient");
  options.doPrintVolume = ap_found(parser, "volume");
  options.doShortestDistance = ap_found(parser, "shortest-distance");
  options.doNearestNeighbors = ap_found(parser, "nearest-neighbors");
  options.doPrintRDF = ap_found(parser, "rdf");
  options.doFormula = ap_found(parser, "formula");
  options.showMoleculeCatalog = ap_found(parser, "molecules");

  options.inputFile = ap_get_str_value(parser, "input");
  options.inputFormat = ap_get_str_value(parser, "format");
  options.outputFile = ap_get_str_value(parser, "output");
  options.outputFormat = ap_get_str_value(parser, "output-format");
  options.compareFile = ap_get_str_value(parser, "compare-rdf");
  options.compareXtalCompFile = ap_get_str_value(parser, "compare-xtalcomp");
  options.compareFormat = ap_get_str_value(parser, "compare-format");
  options.molUnitRequest = ap_get_str_value(parser, "molunit");

  options.rdfBins = ap_get_int_value(parser, "rdf-bins");
  options.rdfCutoff = ap_get_dbl_value(parser, "rdf-cutoff");
  options.rdfSigma = ap_get_dbl_value(parser, "rdf-sigma");
  options.rdfTolerance = ap_get_dbl_value(parser, "rdf-tol");
  options.xtalcompLengthTol = ap_get_dbl_value(parser, "xtalcomp-tol-len");
  options.xtalcompAngleTol = ap_get_dbl_value(parser, "xtalcomp-tol-ang");
  options.rescaleVolume = ap_get_dbl_value(parser, "rescale-volume");
  options.symprec = ap_get_dbl_value(parser, "symprec");
  options.distanceScale = ap_get_dbl_value(parser, "distance-scale");
  options.doRescaleVolume = ap_found(parser, "rescale-volume");

  if (ap_found(parser, "molcrystal"))
    options.mode = Mode_MolCrystal;
  else if (!options.molUnitRequest.empty())
    options.mode = Mode_MolUnit;
  else if (options.showMoleculeCatalog)
    options.mode = Mode_Show;
  else
    options.mode = Mode_Structure;
}

bool parseSpaceGroupText(const QString& text, int& spaceGroup)
{
  bool ok = false;
  const int parsed = text.toInt(&ok);
  if (!ok || parsed < 1 || parsed > 230)
    return false;
  spaceGroup = parsed;
  return true;
}

QString normalizedCompositionFormula(const QString& formula, QString& errorMessage)
{
  std::map<unsigned int, unsigned int> composition;
  if (!Atoms::ElementInfo::readComposition(formula.toStdString(), composition)) {
    errorMessage = QString("Invalid molecule composition: %1").arg(formula);
    return QString();
  }

  QMap<QString, unsigned int> alphabeticComposition;
  for (std::map<unsigned int, unsigned int>::const_iterator it = composition.begin(); it != composition.end(); ++it) {
    alphabeticComposition.insert(
      QString::fromStdString(Atoms::ElementInfo::getAtomicSymbol(it->first)), it->second);
  }

  QString normalized;
  for (QMap<QString, unsigned int>::const_iterator it = alphabeticComposition.constBegin(); it != alphabeticComposition.constEnd(); ++it) {
    normalized += QString("%1%2").arg(it.key()).arg(it.value());
  }

  if (normalized.isEmpty())
    errorMessage = QString("Invalid molecule composition: %1").arg(formula);
  return normalized;
}

bool setMoleculeUnitRequest(const QStringList& fields, std::string& request, QString& errorMessage)
{
  if (fields.size() < 2) {
    errorMessage = "--molunit requires <formula> <template>.";
    return false;
  }

  const QString selectedTemplate = fields.last().simplified();
  if (selectedTemplate.isEmpty() ||
      selectedTemplate.split(' ', QtCompat::SkipEmptyParts).size() != 1) {
    errorMessage = "--molunit template expects a single template name.";
    return false;
  }

  QStringList formulaFields = fields;
  formulaFields.removeLast();
  const QString formula = formulaFields.join(" ").simplified();
  const QString normalizedFormula = normalizedCompositionFormula(formula, errorMessage);

  if (normalizedFormula.isEmpty())
    return false;

  request = QString("%1 %2").arg(normalizedFormula).arg(selectedTemplate).toStdString();

  return true;
}

bool processMolCrystalPositionals(ArgParser* parser, StructureToolOptions& options)
{
  if (!options.molUnitRequest.empty()) {
    options.errorMessage = "--molunit and --molcrystal cannot be used together.";
    return false;
  }

  const int positionalCount = ap_count_args(parser);
  if (positionalCount == 0) {
    options.errorMessage = "--molcrystal expects <space-group> plus one molecule source: "
      "--input <xyz-file>, \"SYMBOL X Y Z, ...\", or <formula> <template>.";
    return false;
  }

  const QString spgText = QString::fromLocal8Bit(ap_get_arg_at_index(parser, 0));
  if (!parseSpaceGroupText(spgText, options.molCrystalSpaceGroup)) {
    options.errorMessage = QString("--molcrystal space group must be an integer 1-230: %1").arg(spgText);
    return false;
  }

  const bool hasInputFile = !options.inputFile.empty();
  const int moleculeArgCount = positionalCount - 1;
  if (hasInputFile && moleculeArgCount != 0) {
    options.errorMessage = "--molcrystal accepts only one molecule source; use either "
      "--input <xyz-file>, \"SYMBOL X Y Z, ...\", or <formula> <template>.";
    return false;
  }
  if (!hasInputFile && moleculeArgCount == 0) {
    options.errorMessage = "--molcrystal expects <space-group> plus one molecule source: "
      "--input <xyz-file>, \"SYMBOL X Y Z, ...\", or <formula> <template>.";
    return false;
  }

  if (hasInputFile) {
    options.molCrystalSource = MoleculeSource_XyzFile;
  } else if (positionalCount == 2) {
    options.molCrystalSource = MoleculeSource_Cartesian;
    options.molCrystalCartesian = ap_get_arg_at_index(parser, 1);
  } else {
    options.molCrystalSource = MoleculeSource_FormulaTemplate;
    QStringList moleculeFields;
    for (int i = 1; i < positionalCount; ++i) {
      moleculeFields << QString::fromLocal8Bit(ap_get_arg_at_index(parser, i)).split(' ', QtCompat::SkipEmptyParts);
    }
    if (!setMoleculeUnitRequest(moleculeFields, options.molCrystalFormulaRequest, options.errorMessage))
      return false;
  }

  return true;
}

bool processMolUnitPositionals(ArgParser* parser, StructureToolOptions& options)
{
  const int positionalCount = ap_count_args(parser);
  if (positionalCount < 1) {
    options.errorMessage = "--molunit expects <formula> <template>.";
    return false;
  }

  // Shell may split the formula; last field is the template, the rest the composition.
  QStringList moleculeFields = QString::fromLocal8Bit(options.molUnitRequest.c_str()).split(' ', QtCompat::SkipEmptyParts);
  for (int i = 0; i < positionalCount; ++i) {
    moleculeFields << QString::fromLocal8Bit(ap_get_arg_at_index(parser, i)).split(' ', QtCompat::SkipEmptyParts);
  }
  return setMoleculeUnitRequest(moleculeFields, options.molUnitRequest, options.errorMessage);
}

bool processModeAndPositionals(ArgParser* parser, StructureToolOptions& options)
{
  if (options.mode == Mode_MolCrystal)
    return processMolCrystalPositionals(parser, options);
  if (options.mode == Mode_MolUnit)
    return processMolUnitPositionals(parser, options);

  const int positionalCount = ap_count_args(parser);
  if (positionalCount > 0) {
    options.errorMessage = QString("Unknown option: %1").arg(QString::fromLocal8Bit(ap_get_arg_at_index(parser, 0)));
    return false;
  }
  return true;
}

bool validateOptions(StructureToolOptions& options)
{
  if (options.rdfBins <= 0) {
    options.errorMessage = "--rdf-bins requires a positive integer";
    return false;
  }
  if (options.rdfCutoff <= 0.0) {
    options.errorMessage = "--rdf-cutoff requires a positive numeric value";
    return false;
  }
  if (options.rdfSigma <= 0.0) {
    options.errorMessage = "--rdf-sigma requires a positive numeric value";
    return false;
  }
  if (options.xtalcompLengthTol < 0.0) {
    options.errorMessage = "--xtalcomp-tol-len requires a non-negative numeric value";
    return false;
  }
  if (options.xtalcompAngleTol < 0.0) {
    options.errorMessage = "--xtalcomp-tol-ang requires a non-negative numeric value";
    return false;
  }
  if (options.distanceScale <= 0.0) {
    options.errorMessage = "--distance-scale requires a positive numeric value";
    return false;
  }
  if (options.doRescaleVolume) {
    if (options.rescaleVolume <= 0.0) {
      options.errorMessage = "--rescale-volume requires a positive numeric value";
      return false;
    }
  }
  if (options.mode == Mode_Structure && options.inputFile.empty()) {
    options.errorMessage = "StructureTool options require --input, except --molunit and --molcrystal.";
    return false;
  }

  return true;
}

typedef std::unique_ptr<ArgParser, void (*)(ArgParser*)> ArgParserPtr;

bool parseCommandLine(int argc, char* argv[], StructureToolOptions& options)
{
  ArgParserPtr parser(ap_new_parser(), &ap_free);
  if (!parser) {
    options.errorMessage = "Failed to initialize command-line parser.";
    return false;
  }

  ap_set_exit_on_error(parser.get(), false);
  registerParserOptions(parser.get(), options);

  if (!ap_parse(parser.get(), argc, argv)) {
    const char* error = ap_get_parse_error(parser.get());
    options.errorMessage = error && error[0] != '\0' ? QString::fromLocal8Bit(error) : "Failed to parse command-line options.";
    return false;
  }

  // Start by copying the "raw" options; then process the positional args and
  //   run mode, and finally validate them.
  readRawOptions(parser.get(), argc, argv, options);
  if (!processModeAndPositionals(parser.get(), options))
    return false;
  return validateOptions(options);
}

int runShowInfo(const StructureToolOptions& options)
{
  if (options.showMoleculeCatalog) {
    Common::message(Atoms::moleculeTemplateCatalogText());
  }
    return 0;
}

bool generateMoleculeFromFormulaRequest(const std::string& requestText, Atoms::Geometry& molecule,
                                        QString& error, double distanceScale,
                                        const QString& usageError)
{
  const QString request = QString::fromLocal8Bit(requestText.c_str()).simplified();
  if (request.isEmpty()) {
    error = usageError;
    return false;
  }

  const QStringList fields = request.split(' ', QtCompat::SkipEmptyParts);
  if (fields.size() != 2) {
    error = usageError;
    return false;
  }

  return Atoms::buildMoleculeFromFormula(fields.at(0).toStdString(), fields.at(1).toStdString(),
                                         molecule, error, distanceScale);
}

bool writeStructure(Atoms::Geometry& structure, const std::string& outputFile,
                    const QString& format, double symprec)
{
  std::ostream* out = &std::cout;
  std::ofstream file;
  if (!outputFile.empty()) {
    file.open(outputFile.c_str(), std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
      Common::error(QString("Failed to open output file: %1").arg(QString::fromLocal8Bit(outputFile.c_str())));
      return false;
    }
    out = &file;
  }

  if (format == "POSCAR")
    return Atoms::PoscarFormat::write(structure, *out);
  if (format == "CML")
    return Atoms::CmlFormat::write(structure, *out);
  if (format == "CIF")
    return Atoms::CifFormat::write(structure, *out, symprec);
  if (format == "XYZ")
    return Atoms::XyzFormat::write(structure, *out);
  if (format == "MTP")
    return Atoms::MtpFormat::write(structure, *out);

  Common::error(QString("Unsupported output format: %1. Currently supported formats are:\n"
                       "  POSCAR/VASP, CML, CIF, XYZ, and MTP.")
                       .arg(format));
  return false;
}

int runMolUnit(const StructureToolOptions& options)
{
  // Builds a molecular unit (a 0D Geometry): always written as XYZ format.
  Atoms::Geometry molecule;
  QString error;
  if (!generateMoleculeFromFormulaRequest(options.molUnitRequest, molecule, error, options.distanceScale,
                                          "--molunit requires <formula> <template>.")) {
    Common::error(error);
    return 1;
  }

  return writeStructure(molecule, options.outputFile, "XYZ", SPGLIB_TOL) ? 0 : 1;
}

bool readMoleculeFromXyzFile(const std::string& inputFile, Atoms::Geometry& molecule, QString& error)
{
  // A molecule is a 0D Geometry: read the XYZ, then keep only its atoms so any
  //   cell the file may carry is dropped.
  Atoms::Geometry structure;
  const QString filename = QString::fromLocal8Bit(inputFile.c_str());
  if (!Atoms::XyzFormat::read(&structure, filename)) {
    error = QString("--molcrystal failed to read XYZ molecule file: %1").arg(filename);
    return false;
  }
  molecule = Atoms::Geometry(structure.atoms());
  return true;
}

bool readMolCrystalMolecule(const StructureToolOptions& options,
                            Atoms::Geometry& molecule, QString& error)
{
  switch (options.molCrystalSource) {
    case MoleculeSource_XyzFile:
      return readMoleculeFromXyzFile(options.inputFile, molecule, error);
    case MoleculeSource_FormulaTemplate:
      return generateMoleculeFromFormulaRequest(options.molCrystalFormulaRequest, molecule, error,
                         options.distanceScale, "--molcrystal expects <space-group> <formula> <template>.");
    case MoleculeSource_Cartesian:
      return Atoms::buildMoleculeFromCartesianString(options.molCrystalCartesian, molecule, error);
    case MoleculeSource_None:
      break;
  }

  error = "--molcrystal requires a molecule source.";
  return false;
}

bool applyStructureTransforms(Atoms::Geometry& structure, const StructureToolOptions& options)
{
  if (options.outputCell == Cell_Primitive) {
    if (!structure.reduceToPrimitive(options.symprec)) {
      Common::error("Failed to generate primitive cell");
      return false;
    }
  } else if (options.outputCell == Cell_Conventional) {
    if (!structure.standardizeToConventionalCell(options.symprec)) {
      Common::error("Failed to generate conventional cell");
      return false;
    }
  }

  if (options.doNiggli && !structure.niggliReduce()) {
    Common::error("Failed to perform Niggli reduction");
    return false;
  }

  if (options.doStandardOrientation && !structure.rotateCellAndCoordsToStandardOrientation()) {
    Common::error("Failed to rotate cell to standard orientation");
    return false;
  }

  if (options.doRescaleVolume) {
    if (!structure.is3D()) {
      Common::error("Volume rescaling is applicable only to a crystal");
      return false;
    }
    structure.setVolume(options.rescaleVolume);
  }

  return true;
}

void printSymmetryInfo(Atoms::Geometry& structure, double symprec)
{
  if (!structure.is3D() && !structure.is0D()) {
    Common::error("Input structure is not a molecule or crystal");
    return;
  }

  if (structure.is0D()) {
    QString pnt = structure.getPointGroupSymbol();
    Common::message(QString("Point group: %1").arg(pnt, 10));
    return;
  }

  structure.findSpaceGroup(symprec);
  uint spg = structure.getSpaceGroupNumber();
  Common::message(QString("Space group: %1     %2     '%3'")
                 .arg(spg, 10)
                 .arg(structure.getSpaceGroupSymbol(), 10)
                 .arg(structure.getHMName(spg)));
}

void printVolumeInfo(const Atoms::Geometry& structure)
{
  // For a molecule this is the volume of the convex hull of its atoms.
  if (!structure.is3D() && !structure.is0D()) {
    Common::error("Input structure is not a molecule or crystal");
    return;
  }
  Common::message(QString("Volume per atom: %1    Number of atoms: %2    Total volume: %3")
                  .arg(structure.getVolumePerAtom(), 14, 'f', 6)
                  .arg(structure.numAtoms(), 6)
                  .arg(structure.getVolume(), 14, 'f', 6));
}

bool printShortestDistance(const Atoms::Geometry& structure)
{
  QList<QString> symbol1, symbol2;
  QList<double> distance;
  if (!structure.getShortestInteratomicDistancesBySpecies(symbol1, symbol2, distance)) {
    Common::error("Failed to calculate shortest interatomic distance");
    return false;
  }

  // Find the overall shortest (ignoring pairs with no distance)
  double overall = PINF;
  for (int i = 0; i < distance.size(); i++) {
    if (distance.at(i) > 0.0 && distance.at(i) < overall)
      overall = distance.at(i);
  }
  Common::message(QString("Overall shortest interatomic distance: %1").arg(overall, 12, 'f', 5, QLatin1Char(' ')));

  QString out = "";
  for (int i = 0; i < distance.size(); i++) {
    out += QString("   %1 %2 %3\n")
             .arg(symbol1.at(i), 4)
             .arg(symbol2.at(i), 4)
             .arg(distance.at(i), 12, 'f', 5, QLatin1Char(' '));
  }
  Common::message(out);
  return true;
}

bool printNearestNeighbors(Atoms::Geometry& structure, double cutoff)
{
  if (!structure.calculateNearestNeighborLists(cutoff)) {
    Common::error("Failed to calculate nearest neighbors list");
    return false;
  }

  auto nnlist = structure.getNearestNeighborLists();

  int colmns = 6; // entry per row in n.n. output
  QString out = "";

  for (int i = 0; i < static_cast<int>(structure.numAtoms()); i++) {
    out += QString("\n[%1]:").arg(i+1, 4, 10, QLatin1Char('0'));
    int index = 0;
    for (int j = 0; j < static_cast<int>(nnlist.at(i).size()); j++) {
      int    a1 = nnlist.at(i).at(j).first;
      double d1 = nnlist.at(i).at(j).second;
      if (index != 0 && index % colmns == 0)
        out += "\n       ";
      index++;
      out += QString(" [%1] %2").arg(a1+1, 4, 10, QLatin1Char('0')).arg(d1, 9, 'f', 5, QLatin1Char(' '));
    }
    out += "\n";
  }
  Common::message(out);

  return true;
}

void printFormulaInfo(const Atoms::Geometry& structure)
{
  const QStringList symbols = structure.getSymbols();
  const std::vector<unsigned int> counts = structure.getNumberOfAtomsAlpha();
  QStringList parts;
  for (int i = 0; i < symbols.size(); ++i)
    parts << QString("%1:%2").arg(symbols.at(i)).arg(counts.at(i));

  QString stype = "Crystal";
  if (structure.is0D())
    stype = "Molecule";

  Common::message(QString("Structure type:        %1").arg(stype));
  Common::message(QString("Formula:               %1").arg(structure.getChemicalFormula()));
  Common::message(QString("Formula units:         %1").arg(structure.getFormulaUnits()));
  Common::message(QString("Total number of atoms: %1").arg(structure.numAtoms()));
  Common::message(QString("Species atom counts:   %1").arg(parts.join(", ")));
  Common::message(QString("Composition:           %1").arg(structure.getCompositionString(false)));
  Common::message(QString("Empirical composition: %1").arg(structure.getCompositionString(true)));
}

bool printNormalizedRDF(Atoms::Geometry& structure, int bins, double cutoff, double sigma)
{
  if (!structure.calculateNormalizedRDF(bins, cutoff, sigma)) {
    Common::error("Failed to calculate normalized RDF");
    return false;
  }

  std::vector<double> total;
  if (!structure.calculateTotalNormalizedRDF(bins, cutoff, sigma, total)) {
    Common::error("Failed to calculate total normalized RDF");
    return false;
  }

  const QStringList symbols = structure.getSymbols();
  const std::vector<float>& rdf = structure.getNormalizedRDF();
  const double delta = cutoff / bins;

  QString header = QString("%1").arg("#distance", 14);
  header += QString("%1").arg("total", 15);
  for (int i = 0; i < symbols.size(); ++i) {
    for (int j = i; j < symbols.size(); ++j) {
      QString kinds = QString("%1-%2").arg(symbols.at(i)).arg(symbols.at(j));
      header += QString("%1").arg(kinds, 15);
    }
  }
  Common::message(header);

  // RDF vector is bin-major; for each bin entries are ordered as natural
  //   order of unique pairs.
  size_t next = 0;
  for (int bin = 0; bin < bins; ++bin) {
    QString line = QString("%1").arg(bin * delta, 14, 'f', 6);
    line += QString(" %1").arg(total[bin], 14, 'f', 6);
    for (int i = 0; i < symbols.size(); ++i) {
      for (int j = i; j < symbols.size(); ++j)
        line += QString(" %1").arg(rdf[next++], 14, 'f', 6);
    }
    Common::message(line);
  }

  return true;
}

QString normalizedFormat(const QString& format)
{
  const QString upper = format.toUpper();
  if (upper == "VASP")
    return "POSCAR";
  if (upper == "CFG")
    return "MTP";
  return upper;
}

bool readStructure(Atoms::Geometry& structure, const std::string& inputFile, const std::string& format)
{
  const QString filename = QString::fromLocal8Bit(inputFile.c_str());
  if (format.empty())
    return Atoms::Formats::read(&structure, filename);

  const QString upper = normalizedFormat(QString::fromLocal8Bit(format.c_str()));

  if (upper == "CASTEP")
    return Atoms::CastepFormat::read(&structure, filename);
  if (upper == "CIF")
    return Atoms::CifFormat::read(&structure, filename);
  if (upper == "CML") {
    std::string text;
    if (!Common::readFileToString(filename, &text)) {
      Common::error(QString("Failed to open CML file: %1").arg(filename));
      return false;
    }
    std::istringstream in(text);
    return Atoms::CmlFormat::read(structure, in);
  }
  if (upper == "GULP")
    return Atoms::GulpFormat::readOutput(&structure, filename);
  if (upper == "MTP")
    return Atoms::MtpFormat::read(&structure, filename);
  if (upper == "POSCAR") {
    std::string text;
    if (!Common::readFileToString(filename, &text)) {
      Common::error(QString("Failed to open POSCAR file: %1").arg(filename));
      return false;
    }
    std::istringstream in(text);
    return Atoms::PoscarFormat::read(structure, in);
  }
  if (upper == "PWSCF")
    return Atoms::PwscfFormat::read(&structure, filename);
  if (upper == "SIESTA")
    return Atoms::SiestaFormat::read(&structure, filename);
  if (upper == "XYZ")
    return Atoms::XyzFormat::read(&structure, filename);

  Common::error(QString("Unsupported input format: %1").arg(QString::fromLocal8Bit(format.c_str())));
  return false;
}

bool compareRDFtoFile(Atoms::Geometry& structure, const std::string& compareFile,
                const std::string& compareFormat,
                int bins, double cutoff, double sigma, double tolerance)
{
  Atoms::Geometry other;
  const QString filename = QString::fromLocal8Bit(compareFile.c_str());
  const bool readOk = readStructure(other, compareFile, compareFormat);

  if (!readOk) {
    Common::error(QString("Failed to read comparison structure: %1").arg(filename));
    return false;
  }

  double dotprod = 0.0;
  bool similar = false;
  if (structure.getSymbols() == other.getSymbols()) {
    similar = structure.compareRDF(other, bins, cutoff, sigma, tolerance, dotprod);
  }

  Common::message(QString("comparison_RDF:        %1   cxc: %2   tol: %3")
                     .arg(similar ? "similar" : "different")
                     .arg(dotprod, 0, 'f', 6).arg(tolerance, 0, 'f', 6));
  return true;
}

bool compareXtalComptoFile(const Atoms::Geometry& structure, const std::string& compareFile,
                     const std::string& compareFormat, double lengthTol, double angleTol)
{
  Atoms::Geometry other;
  if (!readStructure(other, compareFile, compareFormat)) {
    Common::error(QString("Failed to read comparison structure: %1").arg(QString::fromLocal8Bit(compareFile.c_str())));
    return false;
  }

  if (!structure.is3D() || !other.is3D()) {
    Common::error("XtalComp comparison is applicable only to crystals");
    return false;
  }

  const bool similar = structure.compareXtalComp(other, lengthTol, angleTol);
  Common::message(QString("comparison_XtalComp:   %1").arg(similar ? "similar" : "different"));
  return true;
}

bool printStructureReports(Atoms::Geometry& structure, const StructureToolOptions& options)
{
  if (options.printSymmetry)
    printSymmetryInfo(structure, options.symprec);
  if (options.doPrintVolume)
    printVolumeInfo(structure);
  if (options.doShortestDistance && !printShortestDistance(structure))
    return false;
  if (options.doNearestNeighbors && !printNearestNeighbors(structure, options.rdfCutoff))
    return false;
  if (options.doFormula)
    printFormulaInfo(structure);
  if (options.doPrintRDF &&
      !printNormalizedRDF(structure, options.rdfBins, options.rdfCutoff, options.rdfSigma))
    return false;
  if (!options.compareFile.empty() &&
      !compareRDFtoFile(structure, options.compareFile, options.compareFormat,
                  options.rdfBins, options.rdfCutoff, options.rdfSigma, options.rdfTolerance))
    return false;
  if (!options.compareXtalCompFile.empty() &&
      !compareXtalComptoFile(structure, options.compareXtalCompFile,
                       options.compareFormat, options.xtalcompLengthTol, options.xtalcompAngleTol))
    return false;

  return true;
}

bool hasStructureReport(const StructureToolOptions& options)
{
  return options.printSymmetry || options.doPrintVolume ||
         options.doShortestDistance || options.doNearestNeighbors || options.doPrintRDF ||
         options.doFormula || !options.compareFile.empty() || !options.compareXtalCompFile.empty();
}

QString outputFormatForFile(const std::string& outputFile, const std::string& explicitFormat)
{
  if (!explicitFormat.empty())
    return normalizedFormat(QString::fromLocal8Bit(explicitFormat.c_str()));

  if (outputFile.empty())
    return "POSCAR";

  const QString path = QString::fromLocal8Bit(outputFile.c_str()).toLower();
  if (path.endsWith("poscar") || path.endsWith("contcar") || path.endsWith(".vasp"))
    return "POSCAR";
  if (path.endsWith(".cml"))
    return "CML";
  if (path.endsWith(".cif"))
    return "CIF";
  if (path.endsWith(".xyz"))
    return "XYZ";
  if (path.endsWith(".cfg") || path.endsWith(".mtp"))
    return "MTP";

  return "POSCAR";
}

bool writeStructureOutputIfNeeded(Atoms::Geometry& structure, const StructureToolOptions& options)
{
  // The run modes that produce a structure will write to output directly;
  //   unless an output file is given.
  const bool shouldWriteStructure = !options.outputFile.empty() || !hasStructureReport(options);
  if (!shouldWriteStructure)
    return true;

  const QString outFormat = outputFormatForFile(options.outputFile, options.outputFormat);
  if (outFormat.isEmpty())
    return false;
  if (!writeStructure(structure, options.outputFile, outFormat, options.symprec)) {
    Common::error("Failed to write structure output");
    return false;
  }

  return true;
}

int finishStructureWorkflow(Atoms::Geometry& structure, const StructureToolOptions& options)
{
  if (!applyStructureTransforms(structure, options))
    return 1;
  if (!printStructureReports(structure, options))
    return 1;
  if (!writeStructureOutputIfNeeded(structure, options))
    return 1;
  return 0;
}

int runMolCrystal(const StructureToolOptions& options)
{
  // Build a molecular crystal by processing input molecule
  //   and then using the normal structure path.
  QString error;
  Atoms::Geometry molecule;
  if (!readMolCrystalMolecule(options, molecule, error)) {
    Common::error(error);
    return 1;
  }

  std::unique_ptr<Atoms::Geometry> generated =
    Atoms::Generators::generateMolecularCrystal(options.molCrystalSpaceGroup, molecule, error,options.symprec, options.distanceScale);
  if (!generated) {
    Common::error(error);
    return 1;
  }

  return finishStructureWorkflow(*generated, options);
}

int runStructureMode(const StructureToolOptions& options)
{
  // Run modes that handle a periodic structure
  Atoms::Geometry structure;
  if (!readStructure(structure, options.inputFile, options.inputFormat)) {
    Common::error(QString("Failed to read input structure: %1")
                          .arg(QString::fromLocal8Bit(options.inputFile.c_str())));
    return 1;
  }

  return finishStructureWorkflow(structure, options);
}

bool isStructureToolOptionName(const QString& name)
{
  static const char* const structureToolOptions[] = {
    "--primitive", "--conventional", "--symmetry", "--niggli",
    "--standard-orient", "--volume", "--shortest-distance", "--rdf",
    "--formula", "--format", "-f", "--output", "-o", "--output-format",
    "--compare-rdf", "--compare-xtalcomp", "--compare-format", "--rdf-bins",
    "--rdf-cutoff", "--rdf-sigma", "--rdf-tol",
    "--xtalcomp-tol-len", "--xtalcomp-tol-ang", "--rescale-volume",
    "--symprec", "--distance-scale", "--molecules", "--molunit", "--molcrystal",
    "--nearest-neighbors", nullptr
  };

  for (int optIndex = 0; structureToolOptions[optIndex] != nullptr; ++optIndex) {
    if (name == structureToolOptions[optIndex])
      return true;
  }
  return false;
}

} // namespace

namespace StructureTool {

int run(int argc, char* argv[])
{
  StructureToolOptions options;
  if (!parseCommandLine(argc, argv, options)) {
    Common::error(options.errorMessage);
    return 2;
  }

  switch (options.mode) {
    case Mode_Show:
      return runShowInfo(options);
    case Mode_MolUnit:
      return runMolUnit(options);
    case Mode_MolCrystal:
      return runMolCrystal(options);
    case Mode_Structure:
      return runStructureMode(options);
  }

  Common::error("Unknown structuretool mode.");
  return 2;
}

bool isInvocation(int argc, char* argv[])
{
  bool hasStructureToolOption = false;
  for (int i = 1; i < argc; ++i) {
    QString name = QString::fromLocal8Bit(argv[i]);
    const int equalsIndex = name.indexOf('=');
    if (equalsIndex != -1)
      name = name.left(equalsIndex);

    if (name.startsWith("-") && !name.startsWith("--")) {
      for (int charIndex = 1; charIndex < name.size(); ++charIndex) {
        const QChar option = name.at(charIndex);
        hasStructureToolOption = hasStructureToolOption || option == 'f' || option == 'o';
      }
      continue;
    }

    if (isStructureToolOptionName(name))
      hasStructureToolOption = true;
  }
  return hasStructureToolOption;
}

void printHelp(QTextStream& out)
{
  out << "Structure Toolset Options (used with \"--input\" structure file):\n";
  out << "  -f, --format <fmt>                  Input <file> FORMAT (POSCAR,CIF,XYZ,CML,MTP,CASTEP,PWSCF,SIESTA; Default: auto)\n";
  out << "  -o, --output <file>                 Write output structure to <file> instead of printing to output\n";
  out << "      --output-format <fmt>           Output <file> FORMAT (POSCAR,CIF,XYZ,MTP,CML; Default: POSCAR)\n";
  out << "      --symprec <val>                 Spglib symmetry tolerance VALUE (Default: 0.01)\n";
  out << "      --symmetry                      Print space group (crystals) and point group (molecules)\n";
  out << "      --primitive                     Generate primitive cell\n";
  out << "      --conventional                  Generate standardized conventional cell\n";
  out << "      --rescale-volume <val>          Rescale cell to target volume VALUE\n";
  out << "      --niggli                        Apply Niggli reduction\n";
  out << "      --standard-orient               Rotate cell and coordinates to standard orientation\n";
  out << "      --shortest-distance             Print shortest interatomic distance\n";
  out << "      --nearest-neighbors             Print nearest neighbors distances list\n";
  out << "      --formula                       Print chemical formula and composition information\n";
  out << "      --volume                        Print volume and volume per atom of structure\n";
  out << "      --rdf                           Print normalized RDF vector of the cell\n";
  out << "      --rdf-cutoff <val>              RDF vector cutoff VALUE (Default: 6.0 Ang.)\n";
  out << "      --rdf-bins <num>                RDF vector bins NUMBER (Default: 3000)\n";
  out << "      --rdf-sigma <val>               RDF vector Gaussian sigma VALUE (Default: 0.008 Ang.)\n";
  out << "      --compare-rdf <file>            Compare with given structure <file> using normalized RDF vectors\n";
  out << "      --rdf-tol <val>                 RDF comparison similarity tolerance VALUE (Default: 0.98)\n";
  out << "      --compare-xtalcomp <file>       Compare with given structure <file> using XtalComp algorithm\n";
  out << "      --xtalcomp-tol-len <val>        XtalComp comparison distance tolerance VALUE (Default: 0.1 Ang.)\n";
  out << "      --xtalcomp-tol-ang <val>        XtalComp comparison angle tolerance VALUE (Default: 2.0 degrees)\n";
  out << "      --compare-format <fmt>          Comparison structure <file> FORMAT (Default: auto)\n";
  out << "\n";
  out << "Molecule/MolecularCrystal Builder Options (used with standalone \"--molunit\" or \"--molcrystal\"):\n";
  out << "      --molecules                     Print all available molecular unit TEMPLATEs\n";
  out << "      --distance-scale <val>          Distance scaling factor VALUE (Default: 1.0)\n";
  out << "      --molunit <formula> <temp>      Generate molecular unit from FORMULA and TEMPLATE\n";
  out << "      --molcrystal <spg> <\"xyz\">      Generate molecular crystal from SPACEGROUP and \"SYMBOL X Y Z, ...\" molecule\n";
  out << "      --molcrystal <spg> <frm> <temp> Generate molecular crystal from SPACEGROUP and generated molecular unit\n";
  out << "      --molcrystal <spg> -i <file>    Generate molecular crystal from SPACEGROUP and input XYZ molecule <file>\n";
}

} // namespace StructureTool

/**********************************************************************
  AtomsTest - Unit tests for the Atoms API

  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <atoms/generators.h>
#include <atoms/geometry.h>
#include <atoms/molecule.h>
#include <atoms/eleminfo.h>

#include <common/matrix.h>

#include <QtTest>

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <type_traits>

#define APPROX_EQ(a, b) (std::fabs((a) - (b)) < 1e-6)

class AtomsTest : public QObject
{
  Q_OBJECT

private slots:
  void geometryBasics();
  void cellOperationsAndDistances();
  void crystallographyOperations();
  void iadHistogram();
  void coordinateComparison();
  void iadDistributionComparison();
  void randomAtomPlacement();
  void normalizedRDF();
  void rdfDotProduct();
  void randomCrystalGeneration();
  void elementInfoRejectsMalformedCompositions();
  void moleculeTemplateCatalog();
};

namespace {

// Make a molecule from a template.
Atoms::Geometry buildMolecule(const std::string& formula, const std::string& templateName,
                              double scaleFactor = 1.0)
{
  Atoms::Geometry molecule;
  QString error;
  if (!Atoms::buildMoleculeFromFormula(formula, templateName, molecule, error, scaleFactor))
    return Atoms::Geometry();
  return molecule;
}

double zSpan(const Atoms::Geometry& molecule)
{
  const std::vector<Atoms::Atom>& atoms = molecule.atoms();
  if (atoms.empty())
    return 0.0;

  double minZ = atoms.front().pos().z();
  double maxZ = atoms.front().pos().z();
  for (size_t i = 1; i < atoms.size(); ++i) {
    minZ = std::min(minZ, atoms[i].pos().z());
    maxZ = std::max(maxZ, atoms[i].pos().z());
  }
  return maxZ - minZ;
}

} // namespace

void AtomsTest::geometryBasics()
{
  Atoms::Geometry structure;
  structure.setUnitCell(Atoms::UnitCell(4.0, 5.0, 6.0, 90.0, 90.0, 90.0));
  structure.addAtom(8, Common::Vector3(0.0, 0.0, 0.0));
  structure.addAtom(1, Common::Vector3(1.0, 0.0, 0.0));
  structure.addBond(0, 1);

  QCOMPARE(structure.numAtoms(), static_cast<size_t>(2));
  QCOMPARE(structure.numBonds(), static_cast<size_t>(1));
  QVERIFY(APPROX_EQ(structure.getA(), 4.0));
  QVERIFY(APPROX_EQ(structure.getB(), 5.0));
  QVERIFY(APPROX_EQ(structure.getC(), 6.0));
  QVERIFY(APPROX_EQ(structure.getVolume(), 120.0));
  QVERIFY(APPROX_EQ(structure.getVolumePerAtom(), 60.0));
  QVERIFY(APPROX_EQ(structure.distance(0, 1), 1.0));

  const Common::Vector3 frac(0.25, 0.25, 0.25);
  const Common::Vector3 cart = structure.fracToCart(frac);
  const Common::Vector3 fullConversion = structure.cartToFrac(cart);
  QVERIFY(APPROX_EQ(fullConversion.x(), frac.x()));
  QVERIFY(APPROX_EQ(fullConversion.y(), frac.y()));
  QVERIFY(APPROX_EQ(fullConversion.z(), frac.z()));
}

void AtomsTest::cellOperationsAndDistances()
{
  Atoms::Geometry structure;
  structure.setCellInfo(10.0, 10.0, 10.0, 90.0, 90.0, 90.0);
  structure.addAtom(6, Common::Vector3(0.0, 0.0, 0.0));
  structure.addAtom(6, Common::Vector3(1.0, 0.0, 0.0));

  double shortest = 0.0;
  QVERIFY(structure.getShortestInteratomicDistance(shortest));
  QVERIFY(APPROX_EQ(shortest, 1.0));

  structure.setVolume(8000.0);
  QVERIFY(APPROX_EQ(structure.getA(), 20.0));
  QVERIFY(APPROX_EQ(structure.getVolume(), 8000.0));
  QVERIFY(APPROX_EQ(structure.atom(1).pos().x(), 2.0));

  QList<double> squaredDistances;
  QVERIFY(structure.getSquaredAtomicDistancesToPoint(
    Common::Vector3(0.0, 0.0, 0.0), &squaredDistances));
  QCOMPARE(squaredDistances.size(), 2);
  QVERIFY(APPROX_EQ(squaredDistances.at(0), 0.0));
  QVERIFY(APPROX_EQ(squaredDistances.at(1), 4.0));

  structure.addAtom(1, Common::Vector3(22.0, -1.0, 0.0));
  structure.wrapAtomsToCell();
  QVERIFY(APPROX_EQ(structure.atom(2).pos().x(), 2.0));
  QVERIFY(APPROX_EQ(structure.atom(2).pos().y(), 19.0));
}

void AtomsTest::crystallographyOperations()
{
  Atoms::Geometry bcc;
  bcc.setCellInfo(3.0, 3.0, 3.0, 90.0, 90.0, 90.0);
  bcc.addAtom(1, Common::Vector3(0.0, 0.0, 0.0));
  bcc.addAtom(1, Common::Vector3(1.5, 1.5, 1.5));

  bcc.findSpaceGroup();
  QCOMPARE(bcc.getSpaceGroupNumber(), 229U);
  QCOMPARE(bcc.getSpaceGroupSymbol(), QString("Im-3m"));
  QCOMPARE(Atoms::Geometry::getHMName(229), QString("I m 3 m"));
  QVERIFY(!bcc.isPrimitive());

  Atoms::Geometry primitive(bcc);
  QVERIFY(primitive.reduceToPrimitive());
  QCOMPARE(primitive.numAtoms(), static_cast<size_t>(1));
  QVERIFY(primitive.isPrimitive());

  QVERIFY(primitive.standardizeToConventionalCell());
  primitive.findSpaceGroup();
  QCOMPARE(primitive.getSpaceGroupNumber(), 229U);
  QCOMPARE(primitive.getSpaceGroupSymbol(), QString("Im-3m"));

  QVERIFY(!Atoms::Geometry::isNiggliReduced(2.0, 3.0, 4.0, 80.0, 100.0, 100.0));
}

void AtomsTest::iadHistogram()
{
  Atoms::Geometry structure;
  structure.setCellInfo(10.0, 10.0, 10.0, 90.0, 90.0, 90.0);
  structure.addAtom(6, Common::Vector3(0.0, 0.0, 0.0));
  structure.addAtom(6, Common::Vector3(1.0, 0.0, 0.0));

  std::vector<double> distances;
  std::vector<double> frequencies;
  QVERIFY(structure.generateIADHistogram(&distances, &frequencies, 0.0, 2.0, 0.5));
  QCOMPARE(distances.size(), static_cast<size_t>(4));
  QCOMPARE(frequencies.size(), static_cast<size_t>(4));
  QVERIFY(APPROX_EQ(distances.at(2), 1.0));
  QVERIFY(frequencies.at(2) > 0.0);
}

void AtomsTest::coordinateComparison()
{
  Atoms::Geometry structure;
  structure.setCellInfo(2.0, 2.0, 2.0, 90.0, 90.0, 90.0);
  structure.addAtom(6, Common::Vector3(0.0, 0.0, 0.0));
  structure.addAtom(1, Common::Vector3(1.0, 1.0, 1.0));

  Atoms::Geometry same = structure;
  QVERIFY(structure.compareXtalComp(same));

  Atoms::Geometry different = structure;
  different.removeAtom(0);
  QVERIFY(!structure.compareXtalComp(different));
}

void AtomsTest::iadDistributionComparison()
{
  std::vector<double> distances;
  distances.push_back(0.0);
  distances.push_back(1.0);
  distances.push_back(2.0);

  std::vector<double> frequencies1;
  frequencies1.push_back(0.0);
  frequencies1.push_back(1.0);
  frequencies1.push_back(0.0);

  std::vector<double> frequencies2;
  frequencies2.push_back(0.0);
  frequencies2.push_back(0.5);
  frequencies2.push_back(0.0);

  double error = 0.0;
  QVERIFY(Atoms::Geometry::compareIADDistributions(
    distances, frequencies1, frequencies2, 0.0, 0.0, &error));
  QVERIFY(APPROX_EQ(error, 0.5));
}

void AtomsTest::randomAtomPlacement()
{
  Atoms::Geometry structure;
  structure.setCellInfo(10.0, 10.0, 10.0, 90.0, 90.0, 90.0);

  QVERIFY(structure.addAtomRandomly(6));
  QCOMPARE(structure.numAtoms(), static_cast<size_t>(1));
  QCOMPARE(structure.atom(0).atomicNumber(), static_cast<unsigned short>(6));
  QVERIFY(APPROX_EQ(structure.atom(0).pos().x(), 0.0));
  QVERIFY(APPROX_EQ(structure.atom(0).pos().y(), 0.0));
  QVERIFY(APPROX_EQ(structure.atom(0).pos().z(), 0.0));

  QVERIFY(structure.addAtomRandomly(1, 0.1, 1000));
  QCOMPARE(structure.numAtoms(), static_cast<size_t>(2));

  QList<QString> symbols = structure.getAtomicSymbolsInOrder();
  QCOMPARE(symbols.size(), 2);
  QCOMPARE(symbols.at(0), QString("C"));
  QCOMPARE(symbols.at(1), QString("H"));
}

void AtomsTest::normalizedRDF()
{
  Atoms::Geometry structure;
  structure.setCellInfo(10.0, 10.0, 10.0, 90.0, 90.0, 90.0);
  structure.addAtom(6, Common::Vector3(0.0, 0.0, 0.0));
  structure.addAtom(6, Common::Vector3(1.0, 0.0, 0.0));

  QVERIFY(!structure.hasNormalizedRDF());
  QVERIFY(structure.calculateNormalizedRDF(50, 5.0, 0.1));
  QVERIFY(structure.hasNormalizedRDF());

  const std::vector<float>& rdf = structure.getNormalizedRDF();
  QCOMPARE(rdf.size(), static_cast<size_t>(50));

  structure.clearNormalizedRDF();
  QVERIFY(!structure.hasNormalizedRDF());
}

void AtomsTest::rdfDotProduct()
{
  Atoms::Geometry structure;
  structure.setCellInfo(10.0, 10.0, 10.0, 90.0, 90.0, 90.0);
  structure.addAtom(6, Common::Vector3(0.0, 0.0, 0.0));
  structure.addAtom(6, Common::Vector3(1.0, 0.0, 0.0));

  Atoms::Geometry same = structure;
  double dotProduct = 0.0;
  QVERIFY(structure.compareRDF(same, 50, 5.0, 0.1, 1.0, dotProduct));
  QVERIFY(APPROX_EQ(dotProduct, 1.0));

  Atoms::Geometry different = structure;
  different.atom(1).setAtomicNumber(1);
  QVERIFY(!structure.compareRDF(different, 50, 5.0, 0.1, 1.0, dotProduct));
}

void AtomsTest::randomCrystalGeneration()
{
  Atoms::Generators::CrystalGenerationOptions options;
  options.atomicNumbers.push_back(8);
  options.atomicNumbers.push_back(1);
  options.aMin = options.aMax = 8.0;
  options.bMin = options.bMax = 8.0;
  options.cMin = options.cMax = 8.0;
  options.alphaMin = options.alphaMax = 90.0;
  options.betaMin = options.betaMax = 90.0;
  options.gammaMin = options.gammaMax = 90.0;
  options.atomicRadii[1] = 0.3;
  options.atomicRadii[8] = 0.6;
  options.reduceCell = false;

  std::unique_ptr<Atoms::Geometry> generated = Atoms::Generators::generateRandom(options);
  QVERIFY(generated != nullptr);
  QCOMPARE(generated->numAtoms(), static_cast<size_t>(2));
  QCOMPARE(generated->atom(0).atomicNumber(), static_cast<unsigned short>(8));
  QCOMPARE(generated->atom(1).atomicNumber(), static_cast<unsigned short>(1));
  QVERIFY(APPROX_EQ(generated->getA(), 8.0));
  QVERIFY(APPROX_EQ(generated->getB(), 8.0));
  QVERIFY(APPROX_EQ(generated->getC(), 8.0));

  std::vector<unsigned int> atoms;
  atoms.push_back(1);
  QVERIFY(Atoms::Generators::canGenerateRandSpg(1, atoms));
}

void AtomsTest::elementInfoRejectsMalformedCompositions()
{
  std::map<unsigned int, unsigned int> composition;

  QVERIFY(!Atoms::ElementInfo::readComposition("C", composition));
  QVERIFY(composition.empty());

  QVERIFY(!Atoms::ElementInfo::readComposition("C1H", composition));
  QVERIFY(composition.empty());

  QVERIFY(!Atoms::ElementInfo::readComposition("C1.5", composition));
  QVERIFY(composition.empty());

  QVERIFY(!Atoms::ElementInfo::readComposition("C999999999999999999999999999999", composition));
  QVERIFY(composition.empty());

  QVERIFY(Atoms::ElementInfo::readComposition("C1H4", composition));
  QCOMPARE(composition[6], static_cast<unsigned int>(1));
  QCOMPARE(composition[1], static_cast<unsigned int>(4));
}

void AtomsTest::moleculeTemplateCatalog()
{
  const std::vector<Atoms::MoleculeTemplateInfo> catalog = Atoms::moleculeTemplatesCatalog();
  QStringList names;
  for (size_t i = 0; i < catalog.size(); ++i)
    names << catalog[i].name;

  QVERIFY(!names.contains("trigonal_c3v_shell"));
  QVERIFY(!names.contains("square_planar_c4v_shell"));
  QVERIFY(!names.contains("pentagonal_c5v_shell"));
  QVERIFY(!names.contains("hexagonal_c6v_shell"));
  QVERIFY(names.contains("trigonal_planar_d3h_shell"));
  QVERIFY(names.contains("pentagonal_planar_d5h_shell"));
  QVERIFY(names.contains("hexagonal_planar_d6h_shell"));
  QVERIFY(names.contains("trigonal_pyramidal_c3v_center_shell"));
  QVERIFY(names.contains("square_pyramidal_c4v_center_shell"));

  // Check the template shape.
  QVERIFY(APPROX_EQ(zSpan(buildMolecule("Cu3", "trigonal_planar_d3h_shell")), 0.0));

  QVERIFY(zSpan(buildMolecule("Cu1Ag3", "trigonal_pyramidal_c3v_center_shell")) > 0.1);
  QVERIFY(APPROX_EQ(zSpan(buildMolecule("Cu1Ag3", "trigonal_planar_d3h_center_shell")), 0.0));

  QVERIFY(zSpan(buildMolecule("Cu1Ag4", "square_pyramidal_c4v_center_shell")) > 0.1);
  QVERIFY(APPROX_EQ(zSpan(buildMolecule("Cu1Ag4", "square_planar_d4h_center_shell")), 0.0));

  // Check the hydrogen bond length.
  const std::vector<Atoms::MoleculeTemplateInfo> h2Templates =
    Atoms::moleculeTemplatesForFormula("H2");
  QCOMPARE(h2Templates.size(), static_cast<size_t>(1));
  const std::string h2Template = h2Templates.front().name.toStdString();

  const Atoms::Geometry h2 = buildMolecule("H2", h2Template);
  const double h2Distance = (h2.atoms()[0].pos() - h2.atoms()[1].pos()).norm();
  const double h2Target = 2.0 * Atoms::ElementInfo::getCovalentRadius(1);
  QVERIFY(APPROX_EQ(h2Distance, h2Target));

  const Atoms::Geometry h2Scaled = buildMolecule("H2", h2Template, 0.5);
  const double scaledH2Distance = (h2Scaled.atoms()[0].pos() - h2Scaled.atoms()[1].pos()).norm();
  QVERIFY(APPROX_EQ(scaledH2Distance, 0.5 * h2Target));
}

QTEST_MAIN(AtomsTest)

#include "atomstest.moc"

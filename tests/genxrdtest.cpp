/**********************************************************************
  GenXrdTest - Testing the generation and comparison of XRD patterns

  Copyright (C) 2018 Patrick Avery
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <common/compatibility/qt_compat.h>

#include <common/constants.h>
#include <common/fileutils.h>
#include <search/structure.h>
#include <atoms/formats/poscarformat.h>
#include <atoms/geometry.h>
#include <generatexrd/generatexrd.h>

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QtTest>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <vector>

namespace {

using Peak = std::pair<double, double>;

QString testDataPath(const QString& filename)
{
  return Common::localPath(Common::localPath(QString(TESTDATADIR), "formats"), filename);
}

std::vector<Peak> extractPeaks(const GenerateXrd::XrdData& data, double minRelativeIntensity)
{
  std::vector<Peak> peaks;
  if (data.size() < 3)
    return peaks;

  double maxIntensity = 0.0;
  for (const auto& point : data)
    maxIntensity = std::max(maxIntensity, point.second);

  const double threshold = maxIntensity * minRelativeIntensity;
  for (size_t i = 1; i + 1 < data.size(); ++i) {
    if (data[i].second >= threshold && data[i].second >= data[i - 1].second &&
        data[i].second >= data[i + 1].second) {
      peaks.push_back(data[i]);
    }
  }
  return peaks;
}

bool loadReferencePattern(const QString& filename, GenerateXrd::XrdData& data)
{
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return false;

  QTextStream stream(&file);
  data.clear();

  while (!stream.atEnd()) {
    const QString line = stream.readLine();
    const QStringList fields = line.split(QRegularExpression("\\s+"), QtCompat::SkipEmptyParts);
    if (fields.size() < 2)
      continue;

    bool okX = false;
    bool okY = false;
    const double x = fields.at(0).toDouble(&okX);
    const double y = fields.at(1).toDouble(&okY);
    if (okX && okY)
      data.push_back(std::make_pair(x, y));
  }

  return !data.empty();
}

double nearestPeakDistance(double position, const std::vector<Peak>& peaks)
{
  double best = std::numeric_limits<double>::max();
  for (const auto& peak : peaks)
    best = std::min(best, std::fabs(position - peak.first));
  return best;
}

} // namespace

class GenXrdTest : public QObject
{
  Q_OBJECT

public:
  GenXrdTest();

private slots:
  /**
   * Called before the first test function is executed.
   */
  void initTestCase();

  /**
   * Called after the last test function is executed.
   */
  void cleanupTestCase();

  /**
   * Called before each test function is executed.
   */
  void init();

  /**
   * Called after every test function.
   */
  void cleanup();

  // Tests
  void generatePatternTest();
  void generatePatternFromAtoms();
  void compareAgainstReferencePattern();
};

GenXrdTest::GenXrdTest()
{
}

void GenXrdTest::initTestCase()
{
}

void GenXrdTest::cleanupTestCase()
{
}

void GenXrdTest::init()
{
}

void GenXrdTest::cleanup()
{
}

void GenXrdTest::generatePatternTest()
{
  /* Read rutile from a poscar file */
  QString rutileFileName = testDataPath("rutile.POSCAR");
  Search::Structure rutile;
  std::ifstream in(rutileFileName.toStdString().c_str());
  QVERIFY(in.is_open());

  QVERIFY(Atoms::PoscarFormat::read(rutile, in));

  GenerateXrd::XrdData results;

  double wavelength = GenerateXrd::DEFAULT_WAVELENGTH;
  double peakwidth = GenerateXrd::DEFAULT_PEAKWIDTH;
  size_t numpoints = GenerateXrd::DEFAULT_NUMPOINTS;
  double max2theta = GenerateXrd::DEFAULT_MAX_2THETA;

  QVERIFY(GenerateXrd::generatePattern(
    rutile, results, wavelength, peakwidth, numpoints, max2theta));

  // Our results should be equal in size to numpoints
  QVERIFY(results.size() == numpoints);

  // Check the X-ray angle values (to be within max2theta).
  QVERIFY(!results.empty());
  QVERIFY(results.front().first >= 0.0);
  QVERIFY(results.back().first <= max2theta + ZERO08);
  for (size_t i = 1; i < results.size(); ++i)
    QVERIFY(results[i - 1].first <= results[i].first);

  // Check the X-ray intensities (should contain at least one non-zero).
  bool hasSignal = false;
  for (const auto& point : results) {
    if (point.second > 0.0) {
      hasSignal = true;
      break;
    }
  }
  QVERIFY(hasSignal);
}

void GenXrdTest::generatePatternFromAtoms()
{
  const QString rutileFileName = testDataPath("rutile.POSCAR");
  std::ifstream in(rutileFileName.toStdString().c_str());
  QVERIFY(in.is_open());

  Atoms::Geometry rutile;
  QVERIFY(Atoms::PoscarFormat::read(rutile, in));

  GenerateXrd::XrdData results;
  QVERIFY(GenerateXrd::generatePattern(rutile, results));
  QCOMPARE(results.size(), static_cast<size_t>(GenerateXrd::DEFAULT_NUMPOINTS));
}

void GenXrdTest::compareAgainstReferencePattern()
{
  const QString poscarFile = testDataPath("rutile.POSCAR");
  const QString referenceFile = testDataPath("rutile.xrd");

  Atoms::Geometry structure;
  std::ifstream in(poscarFile.toStdString().c_str());
  QVERIFY(in.is_open());
  QVERIFY(Atoms::PoscarFormat::read(structure, in));

  GenerateXrd::XrdData generated;
  QVERIFY(GenerateXrd::generatePattern(structure, generated, GenerateXrd::DEFAULT_WAVELENGTH,
    GenerateXrd::DEFAULT_PEAKWIDTH, GenerateXrd::DEFAULT_NUMPOINTS,
    GenerateXrd::DEFAULT_MAX_2THETA));

  GenerateXrd::XrdData reference;
  QVERIFY(loadReferencePattern(referenceFile, reference));

  QCOMPARE(generated.size(), reference.size());
  for (size_t i = 0; i < generated.size(); ++i)
    QVERIFY(std::fabs(generated[i].first - reference[i].first) <= ZERO06);

  const std::vector<Peak> generatedPeaks = extractPeaks(generated, 0.01);
  const std::vector<Peak> referencePeaks = extractPeaks(reference, 0.01);

  QVERIFY(generatedPeaks.size() >= 6);
  QVERIFY(referencePeaks.size() >= generatedPeaks.size());

  const double twoThetaTolerance = 0.2;
  for (const auto& peak : generatedPeaks) {
    QVERIFY2(nearestPeakDistance(peak.first, referencePeaks) <= twoThetaTolerance,
             qPrintable(QString("Generated peak at %1 has no nearby reference peak")
                          .arg(peak.first, 0, 'f', 4)));
  }
}

QTEST_MAIN(GenXrdTest)

#include "genxrdtest.moc"

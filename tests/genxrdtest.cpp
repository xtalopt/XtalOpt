/**********************************************************************
  GenXrd test - make sure we can correctly generate XRD patterns

  Copyright (C) 2018 Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 **********************************************************************/

#include <globalsearch/formats/formats.h>
#include <globalsearch/structure.h>
#include <globalsearch/xrd/generatexrd.h>

#include <QDebug>
#include <QtTest>

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
  void generateXrdPatternTest();
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

void GenXrdTest::generateXrdPatternTest()
{
  /* Read rutile from a poscar file */
  QString rutileFileName = QString(TESTDATADIR) + "/data/rutile.POSCAR";
  GlobalSearch::Structure rutile;

  QVERIFY(GlobalSearch::Formats::read(&rutile, rutileFileName, "POSCAR"));

  GlobalSearch::XrdData results;

  double wavelength = 1.5056;
  double peakwidth = 0.52958;
  size_t numpoints = 1000;
  double max2theta = 162.0;

  QVERIFY(GlobalSearch::GenerateXrd::generateXrdPattern(
    rutile, results, wavelength, peakwidth, numpoints, max2theta));

  // Our results should be equal in size to numpoints
  QVERIFY(results.size() == numpoints);
}

QTEST_MAIN(GenXrdTest)

#include "genxrdtest.moc"

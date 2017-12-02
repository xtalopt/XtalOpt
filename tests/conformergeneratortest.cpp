/**********************************************************************
  ConformerGeneratorTest - make sure we can generate conformers

  Copyright (C) 2017 Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 **********************************************************************/

#include <globalsearch/molecular/conformergenerator.h>

#include <QDebug>
#include <QString>
#include <QtTest>

#include <fstream>

class ConformerGeneratorTest : public QObject
{
  Q_OBJECT

public:
  ConformerGeneratorTest();

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
  void generateButaneConformers();
};

ConformerGeneratorTest::ConformerGeneratorTest()
{
}

void ConformerGeneratorTest::initTestCase()
{
}

void ConformerGeneratorTest::cleanupTestCase()
{
}

void ConformerGeneratorTest::init()
{
}

void ConformerGeneratorTest::cleanup()
{
}

void ConformerGeneratorTest::generateButaneConformers()
{
  std::string butaneFileName = std::string(TESTDATADIR) + "/data/butane.mol";

  // Open it up in an ifstream
  std::ifstream butaneSDF(butaneFileName);
  QVERIFY(butaneSDF.is_open());

  // Make a directory in our current folder called "conformers"
  QDir().mkdir("conformers");
  std::string outDir = QDir().absolutePath().toStdString() + "/conformers/";

  // It won't actually generate 1000 because of pruning
  size_t numConformers = 1000;
  size_t maxOptimizationIters = 1000;
  double rmsdThreshold = 0.1;
  bool pruneConfsAfterOpt = true;

  GlobalSearch::ConformerGenerator::generateConformers(
    butaneSDF, outDir, numConformers, maxOptimizationIters, rmsdThreshold,
    pruneConfsAfterOpt);

  butaneSDF.close();

  // First check: there should be 4 files in the conformers directory plus
  // '.' and '..' == 6
  QVERIFY(QDir("./conformers/").count() == 6);

  QFile energiesFile("./conformers/energies.txt");
  QVERIFY(energiesFile.open(QFile::ReadOnly | QFile::Text));

  QString data = energiesFile.readAll();

  // Make sure that the file contains certain strings
  QVERIFY(data.contains("conformer-1.sdf"));
  QVERIFY(data.contains("conformer-2.sdf"));
  QVERIFY(data.contains("conformer-3.sdf"));
  QVERIFY(data.contains("-5.07597"));

  // We can try testing the reading of the SDF files later
}

QTEST_MAIN(ConformerGeneratorTest)

#include "conformergeneratortest.moc"

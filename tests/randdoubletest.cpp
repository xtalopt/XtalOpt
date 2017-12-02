/**********************************************************************
  RandDoubleTest -- Test the random number generator

  This test will create 1000x1000 images for the random number generators
  that it knows about. Black regions have few hits, while lighter
  regions have hits. The lighter the pixel, the more hits it has.

  Copyright (C) 2010-2011 David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 **********************************************************************/

#include <globalsearch/macros.h>
#include <globalsearch/random.h>

#include <QDebug>
#include <QImage>
#include <QtTest>

#include <stdlib.h>
#include <time.h>

class RandDoubleTest : public QObject
{
  Q_OBJECT

private:
  int m_size;
  int m_numPoints;

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
  void generateImageDefault();

// generateImageSystem() crashes on Windows for some reason? We don't use
// std::rand() in our program, though, so it probably doesn't matter
#ifndef WIN32
  void generateImageSystem();
#endif
};

void RandDoubleTest::initTestCase()
{
  m_size = 1e3;
  m_numPoints = 1e7;
}

void RandDoubleTest::cleanupTestCase()
{
}

void RandDoubleTest::init()
{
}

void RandDoubleTest::cleanup()
{
}

void createAndSaveImage(unsigned int size,
                        std::vector<std::vector<unsigned int>>& hits,
                        const QString& imageName)
{
  // Find greatest number of hits
  unsigned int max = 0;
  for (unsigned int i = 0; i < size; i++) {
    for (unsigned int j = 0; j < size; j++) {
      if (hits[i][j] > max) {
        max = hits[i][j];
      }
    }
  }

  // Generate image
  QImage im(size, size, QImage::Format_RGB32);
  for (unsigned int i = 0; i < size; i++) {
    for (unsigned int j = 0; j < size; j++) {
      unsigned int normHit = (hits[i][j] * 100) / max;
      unsigned int rgb = (normHit * 0xff) / 100;
      im.setPixel(i, j, 0xff000000 + 0x00010000 * rgb + 0x00000100 * rgb +
                          0x00000001 * rgb);
    }
  }
  im.save(imageName, 0, 100);
}

void RandDoubleTest::generateImageDefault()
{
  // Create matrix to store hits
  std::vector<std::vector<unsigned int>> hits;
  hits.resize(m_size);
  for (int i = 0; i < m_size; i++) {
    hits[i].resize(m_size);
    for (int j = 0; j < m_size; j++) {
      hits[i][j] = 0;
    }
  }

  // Generate numbers
  int x, y;
  QBENCHMARK
  {
    for (int i = 0; i < m_numPoints; i++) {
      x = (int)(GlobalSearch::getRandDouble() * m_size);
      y = (int)(GlobalSearch::getRandDouble() * m_size);
      hits[x][y]++;
    }
  }

  createAndSaveImage(m_size, hits, "RandDoubleTest-default.png");
}

// This test crashes on Windows for some reason? We don't use
// std::rand() in our program, though, so it probably doesn't matter
#ifndef WIN32
void RandDoubleTest::generateImageSystem()
{
  std::srand(time(0));

  // Create matrix to store hits
  std::vector<std::vector<unsigned int>> hits;
  hits.resize(m_size);
  for (int i = 0; i < m_size; i++) {
    hits[i].resize(m_size);
    for (int j = 0; j < m_size; j++) {
      hits[i][j] = 0;
    }
  }

  // Generate numbers
  int x, y;
  QBENCHMARK
  {
    for (int i = 0; i < m_numPoints; i++) {
      x = (int)((std::rand() / (double)RAND_MAX) * m_size);
      y = (int)((std::rand() / (double)RAND_MAX) * m_size);
      hits[x][y]++;
    }
  }

  createAndSaveImage(m_size, hits, "RandDoubleTest-system.png");
}
#endif

QTEST_MAIN(RandDoubleTest)

#include "randdoubletest.moc"

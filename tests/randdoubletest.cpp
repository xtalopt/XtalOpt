/**********************************************************************
  RANDDOUBLETest -- Test the random number generator

  This test will create 1000x1000 images for the random number generators
  that it knows about. Black regions have few hits, while lighter
  regions have hits. The lighter the pixel, the more hits it has.

  Copyright (C) 2010-2011 David C. Lonie

  XtalOpt is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
 **********************************************************************/

#include <globalsearch/macros.h>

#include <openbabel/rand.h>

#include <QtCore/QDebug>
#include <QtGui/QImage>
#include <QtTest/QtTest>

#include <stdlib.h>
#include <time.h>

class RANDDOUBLETest : public QObject
{
  Q_OBJECT

  private:
  int size;
  int numPoints;

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
  void generateImageOBRandom();
  void generateImageSystem();
};

void RANDDOUBLETest::initTestCase()
{
  size = 1e3;
  numPoints = 1e7;
}

void RANDDOUBLETest::cleanupTestCase()
{
}

void RANDDOUBLETest::init()
{
}

void RANDDOUBLETest::cleanup()
{
}

void RANDDOUBLETest::generateImageDefault()
{
  INIT_RANDOM_GENERATOR();

  // Create matrix to store hits
  std::vector<std::vector<unsigned int> > hits;
  hits.resize(size);
  for (int i = 0; i < size; i++) {
    hits[i].resize(size);
    for (int j = 0; j < size; j++) {
      hits[i][j] = 0;
    }
  }

  // Generate numbers
  int x,y;
  QBENCHMARK {
    for (int i = 0; i < numPoints; i++) {
      x = (int)(RANDDOUBLE() * size);
      y = (int)(RANDDOUBLE() * size);
      hits[x][y]++;
    }
  }

  // Find greatest number of hits
  unsigned int max = 0;
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < size; j++) {
      if (hits[i][j] > max) {
        max = hits[i][j];
      }
    }
  }

  // Generate image
  unsigned int normHit;
  unsigned int rgb;
  QImage im (size, size, QImage::Format_RGB32);
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < size; j++) {
      normHit = (hits[i][j] * 100) / max;
      rgb = (normHit * 0xff) / 100;
      im.setPixel(i, j,
                  0xff000000 +
                  0x00010000 * rgb +
                  0x00000100 * rgb +
                  0x00000001 * rgb );
    }
  }
  im.save("RANDDOUBLETest-default.png", 0, 100);
}

void RANDDOUBLETest::generateImageOBRandom()
{
  OpenBabel::OBRandom rand;
  rand.TimeSeed();

  // Create matrix to store hits
  std::vector<std::vector<unsigned int> > hits;
  hits.resize(size);
  for (int i = 0; i < size; i++) {
    hits[i].resize(size);
    for (int j = 0; j < size; j++) {
      hits[i][j] = 0;
    }
  }

  // Generate numbers
  int x,y;
  QBENCHMARK {
    for (int i = 0; i < numPoints; i++) {
      x = (int)(rand.NextFloat() * size);
      y = (int)(rand.NextFloat() * size);
      hits[x][y]++;
    }
  }

  // Find greatest number of hits
  unsigned int max = 0;
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < size; j++) {
      if (hits[i][j] > max) {
        max = hits[i][j];
      }
    }
  }

  // Generate image
  unsigned int normHit;
  unsigned int rgb;
  QImage im (size, size, QImage::Format_RGB32);
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < size; j++) {
      normHit = (hits[i][j] * 100) / max;
      rgb = (normHit * 0xff) / 100;
      im.setPixel(i, j,
                  0xff000000 +
                  0x00010000 * rgb +
                  0x00000100 * rgb +
                  0x00000001 * rgb );
    }
  }
  im.save("RANDDOUBLETest-OBRandom.png", 0, 100);
}

void RANDDOUBLETest::generateImageSystem()
{
  std::srand(time(0));

  // Create matrix to store hits
  std::vector<std::vector<unsigned int> > hits;
  hits.resize(size);
  for (int i = 0; i < size; i++) {
    hits[i].resize(size);
    for (int j = 0; j < size; j++) {
      hits[i][j] = 0;
    }
  }

  // Generate numbers
  int x,y;
  QBENCHMARK {
    for (int i = 0; i < numPoints; i++) {
      x = (int)((std::rand()/(double)RAND_MAX) * size);
      y = (int)((std::rand()/(double)RAND_MAX) * size);
      hits[x][y]++;
    }
  }

  // Find greatest number of hits
  unsigned int max = 0;
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < size; j++) {
      if (hits[i][j] > max) {
        max = hits[i][j];
      }
    }
  }

  // Generate image
  unsigned int normHit;
  unsigned int rgb;
  QImage im (size, size, QImage::Format_RGB32);
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < size; j++) {
      normHit = (hits[i][j] * 100) / max;
      rgb = (normHit * 0xff) / 100;
      im.setPixel(i, j,
                  0xff000000 +
                  0x00010000 * rgb +
                  0x00000100 * rgb +
                  0x00000001 * rgb );
    }
  }
  im.save("RANDDOUBLETest-system.png", 0, 100);
}

QTEST_MAIN(RANDDOUBLETest)

#include "moc_randdoubletest.cxx"

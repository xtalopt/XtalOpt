/**********************************************************************
  RANDDOUBLETest -- Test the random number generator

  Copyright (C) 2010 David C. Lonie

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
  void generateImage();
  void generateImageOBRandom();
};

void RANDDOUBLETest::initTestCase()
{
  size = 1e4;
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

void RANDDOUBLETest::generateImage()
{
  INIT_RANDOM_GENERATOR();

  QImage im (size, size, QImage::Format_Mono);
  im.fill(0);
  int x,y;
  QBENCHMARK {
    for (int i = 0; i < numPoints; i++) {
      x = (int)(RANDDOUBLE() * size);
      y = (int)(RANDDOUBLE() * size);
      im.setPixel(x,y, 1);
    }
  }
  im.save("RANDDOUBLETest-default.png");
}

void RANDDOUBLETest::generateImageOBRandom()
{
  OpenBabel::OBRandom rand;
  rand.TimeSeed();

  QImage im (size, size, QImage::Format_Mono);
  im.fill(0);
  int x,y;
  QBENCHMARK {
    for (int i = 0; i < numPoints; i++) {
      x = (int)(rand.NextFloat() * size);
      y = (int)(rand.NextFloat() * size);
      im.setPixel(x,y, 1);
    }
  }
  im.save("RANDDOUBLETest-OBRandom.png");
}

QTEST_MAIN(RANDDOUBLETest)

#include "moc_randdoubletest.cxx"

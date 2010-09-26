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

#include <QtCore/QDebug>
#include <QtGui/QImage>
#include <QtTest/QtTest>

class RANDDOUBLETest : public QObject
{
  Q_OBJECT

  private:

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
};

void RANDDOUBLETest::initTestCase()
{
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
  const int size = 1e4;
  const int numPoints = 1e7;

  INIT_RANDOM_GENERATOR();

  QImage im (size, size, QImage::Format_Mono);
  im.fill(0);
  int x,y;
  for (int i = 0; i < numPoints; i++) {
    x = (int)(RANDDOUBLE() * size);
    y = (int)(RANDDOUBLE() * size);
    im.setPixel(x,y, 1);
    qDebug() << i << " of " << numPoints;
  }
  im.save("RANDDOUBLETest.png");
}

QTEST_MAIN(RANDDOUBLETest)

#include "moc_randdoubletest.cxx"

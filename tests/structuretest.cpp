/**********************************************************************
  StructureTest - StructureTest class provides unit testing for Structure

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

#include <globalsearch/structure.h>

#include <QtTest/QtTest>

#define APPROX_EQ(a, b) (fabs((a) - (b)) < 1e-6)

using namespace OpenBabel;
using namespace Avogadro;
using namespace GlobalSearch;

class StructureTest : public QObject
{
  Q_OBJECT

  private:
  Structure *m_structure;

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
  void enthalpyFallBack();
};

void StructureTest::initTestCase()
{
}

void StructureTest::cleanupTestCase()
{
}

void StructureTest::init()
{
  m_structure = new Structure;
}

void StructureTest::cleanup()
{
  if (m_structure) {
    delete m_structure;
  }
  m_structure = 0;
}

void StructureTest::enthalpyFallBack()
{
  Structure s;
  s.addAtom();

  s.setEnergy(1.0 * EV_TO_KJ_PER_MOL);
  qDebug() << s.getEnthalpy();
  QVERIFY(APPROX_EQ(s.getEnthalpy(), 1.0));

  s.setEnergy(-1.0 * EV_TO_KJ_PER_MOL);
  QVERIFY(APPROX_EQ(s.getEnthalpy(), -1.0));

  s.setEnthalpy(3.0);
  QVERIFY(APPROX_EQ(s.getEnthalpy(), 3.0));

  s.setEnthalpy(-3.0);
  QVERIFY(APPROX_EQ(s.getEnthalpy(), -3.0));

  QList<unsigned int> anums;
  anums << 1;
  QList<Eigen::Vector3d> coords;
  coords << Eigen::Vector3d(0,0,0);

  s.updateAndSkipHistory(anums,
                         coords,
                         1.0, // energy
                         0.0); // enthalpy
  QVERIFY(APPROX_EQ(s.getEnthalpy(), 1.0));

  s.updateAndSkipHistory(anums,
                         coords,
                         -1.0, // energy
                         0.0); // enthalpy
  QVERIFY(APPROX_EQ(s.getEnthalpy(), -1.0));

  s.updateAndAddToHistory(anums,
                          coords,
                          1.0, // energy
                          0.0); // enthalpy
  QVERIFY(APPROX_EQ(s.getEnthalpy(), 1.0));

  s.updateAndAddToHistory(anums,
                          coords,
                          -1.0, // energy
                          0.0); // enthalpy
  QVERIFY(APPROX_EQ(s.getEnthalpy(), -1.0));

  s.updateAndSkipHistory(anums,
                         coords,
                         0.0, // energy
                         1.0); // enthalpy
  QVERIFY(APPROX_EQ(s.getEnthalpy(), 1.0));

  s.updateAndSkipHistory(anums,
                         coords,
                         0.0, // energy
                         -1.0); // enthalpy
  QVERIFY(APPROX_EQ(s.getEnthalpy(), -1.0));

  s.updateAndAddToHistory(anums,
                          coords,
                          0.0, // energy
                          1.0); // enthalpy
  QVERIFY(APPROX_EQ(s.getEnthalpy(), 1.0));

  s.updateAndAddToHistory(anums,
                          coords,
                          0.0, // energy
                          -1.0); // enthalpy
  QVERIFY(APPROX_EQ(s.getEnthalpy(), -1.0));

}

QTEST_MAIN(StructureTest)

#include "moc_structuretest.cxx"

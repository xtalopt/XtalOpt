/**********************************************************************
  SPGLib - SPGLibTest class provides unit testing for SPGLib

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

#include <xtalopt/structures/xtal.h>

#include <QtCore/QDebug>
#include <QtCore/QString>
#include <QtTest/QtTest>

using namespace XtalOpt;
using namespace Avogadro;

class SPGLibTest : public QObject
{
  Q_OBJECT

  private:
  Xtal *m_xtal;

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
  void idealBCC();
};

void SPGLibTest::initTestCase()
{
}

void SPGLibTest::cleanupTestCase()
{
}

void SPGLibTest::init()
{
  m_xtal = new Xtal;
}

void SPGLibTest::cleanup()
{
  if (m_xtal) {
    delete m_xtal;
    m_xtal = 0;
  }
}

void SPGLibTest::idealBCC() {
  m_xtal->setCellInfo(3.0, 3.0, 3.0, 90.0, 90.0, 90.0);

  // Build bcc structure
  Atom *atm;

  atm = m_xtal->addAtom();
  atm->setPos(Eigen::Vector3d(0.0, 0.0, 0.0));
  atm->setAtomicNumber(1);

  atm = m_xtal->addAtom();
  atm->setPos(Eigen::Vector3d(1.5, 1.5, 1.5));
  atm->setAtomicNumber(1);

  QCOMPARE(m_xtal->getSpaceGroupNumber(), 229U);
  QCOMPARE(m_xtal->getSpaceGroupSymbol(), QString("Im-3m"));
  QCOMPARE(m_xtal->getHTMLSpaceGroupSymbol(),
           QString("<HTML>Im<span style=\"text-decoration: overline\">3</span>m</HTML>"));
}

QTEST_MAIN(SPGLibTest)

#include "moc_spglibtest.cxx"

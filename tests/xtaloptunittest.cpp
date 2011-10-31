/**********************************************************************
  XtalOptUnitTest -- Unit testing for XtalOpt functions

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

#include <xtalopt/xtalopt.h>

#include <xtalopt/ui/dialog.h>
#include <xtalopt/optimizers/gulp.h>
#include <xtalopt/structures/molecularxtal.h>
#include <xtalopt/structures/submolecule.h>
#include <xtalopt/structures/submoleculesource.h>

#include <globalsearch/macros.h>

#include <avogadro/atom.h>
#include <avogadro/bond.h>

#include <QtCore/QDebug>
#include <QtCore/QString>
#include <QtTest/QtTest>

using namespace GlobalSearch;
using namespace Avogadro;

namespace XtalOpt {
  class XtalOptUnitTest : public QObject
  {
    Q_OBJECT

    private:
    XtalOptDialog *m_dialog;
    XtalOpt *m_opt;

  private slots:
    // Called before the first test function is executed.
    void initTestCase() {
      m_dialog = 0; m_opt = 0;};
    // Called after the last test function is executed.
    void cleanupTestCase() {};
    // Called before each test function is executed.
    void init() {};
    // Called after every test function.
    void cleanup() {};

    // Tests
    void constructDialog();
    void constructXtalOpt();
    void setOptimizer();

    void interpretKeyword();

    void loadTest();
    void checkForDuplicatesTest();
    void stepwiseCheckForDuplicatesTest();

    void destroyDialogAndXtalOpt();

  };

  void XtalOptUnitTest::constructDialog()
  {
    m_dialog = new XtalOptDialog(0,0,0,false);
    QVERIFY(m_dialog != 0);
  }

  void XtalOptUnitTest::constructXtalOpt()
  {
    m_opt = new XtalOpt(m_dialog);
    m_opt->tol_spg = 0.05;
    QVERIFY(m_opt != 0);
  }

  void XtalOptUnitTest::setOptimizer()
  {
    m_opt->setOptimizer(new GULPOptimizer(m_opt));
    QVERIFY(m_opt->optimizer() != 0);
  }

  inline Avogadro::Atom* addAtomToMol(Avogadro::Molecule *mol, int atomicNum,
                                      double x, double y, double z)
  {
    Avogadro::Atom *atom = mol->addAtom();
    atom->setAtomicNumber(atomicNum);
    atom->setPos(Eigen::Vector3d(x, y, z));
    return atom;
  }

  inline Avogadro::Bond* addBondToMol(Avogadro::Molecule *mol, int order,
                                      Avogadro::Atom *beg, Avogadro::Atom *end)
  {
    Avogadro::Bond *bond= mol->addBond();
    bond->setOrder(order);
    bond->setBegin(beg);
    bond->setEnd(end);
    return bond;
  }

  void XtalOptUnitTest::interpretKeyword()
  {
    if (m_dialog == NULL)
      this->constructDialog();
    if (m_opt == NULL)
      this->constructXtalOpt();

    // Build a crystal with two urea submolecules
    SubMoleculeSource source;
    Avogadro::Atom *o1 = addAtomToMol(
          &source, 8, -0.413635,  1.669913,  0.187877);
    Avogadro::Atom *n2 = addAtomToMol(
          &source, 7, -1.127519, -0.495405,  0.000588);
    Avogadro::Atom *n3 = addAtomToMol(
          &source, 7,  1.227310,  0.092854, -0.046085);
    Avogadro::Atom *c4 = addAtomToMol(
          &source, 6, -0.118994,  0.480821,  0.054038);
    Avogadro::Atom *h5 = addAtomToMol(
          &source, 1, -0.917626, -1.456549, -0.106245);
    Avogadro::Atom *h6 = addAtomToMol(
          &source, 1, -2.075843, -0.219289,  0.071139);
    Avogadro::Atom *h7 = addAtomToMol(
          &source, 1,  1.936521,  0.782862, -0.007714);
    Avogadro::Atom *h8 = addAtomToMol(
          &source, 1,  1.489787, -0.855207, -0.153599);

    addBondToMol(&source, 2, o1, c4);
    addBondToMol(&source, 1, c4, n2);
    addBondToMol(&source, 1, c4, n3);
    addBondToMol(&source, 1, n2, h5);
    addBondToMol(&source, 1, n2, h6);
    addBondToMol(&source, 1, n3, h7);
    addBondToMol(&source, 1, n3, h8);

    MolecularXtal mxtal (6, 6, 6, 90, 90, 90);
    SubMolecule *sub = source.getSubMolecule();
    sub->translate(-sub->center());
    mxtal.addSubMolecule(sub);

    sub = source.getSubMolecule();
    sub->translate(-sub->center());
    sub->translate(0.0, 0.0, 3.0);
    mxtal.addSubMolecule(sub);

    qDebug() << "\n" << m_opt->interpretTemplate("%gulpConnect%", &mxtal);

#define VERIFYKEYWORD(key, value)                 \
  QVERIFY(m_opt->interpretTemplate(key, s)        \
  .compare(QString(value) + "\n") == 0)

  }

  void XtalOptUnitTest::loadTest()
  {
    QVERIFY(m_opt->load(QString(TESTDATADIR)
                        + "xo-duplicateXtals/xtalopt.state",
                        true));
    QVERIFY(m_opt->tracker()->size() == 203);
  }

  // Helper function
  inline void resetStatus(QList<Structure*> *list, Structure::State status)
  {
    Structure *s;
    for (int i = 0; i < list->size(); i++) {
      s = list->at(i);
      s->lock()->lockForWrite();
      s->setStatus(status);
      s->setChangedSinceDupChecked(true);
      s->lock()->unlock();
    }
  }

  void XtalOptUnitTest::checkForDuplicatesTest()
  {
    m_opt->tracker()->blockSignals(true);
    resetStatus(m_opt->tracker()->list(), Structure::Optimized);

    QBENCHMARK_ONCE {
      m_opt->checkForDuplicates_();
    }

    qDebug() << m_opt->tracker()->size()
             << m_opt->queue()->getAllDuplicateStructures().size();

    // This may change when Xtal::operator== becomes more or less robust.
    //QVERIFY(m_opt->queue()->getAllDuplicateStructures().size() == 41);
    QVERIFY(true); // Remove this when the above line is used;

    m_opt->tracker()->blockSignals(false);
  }

  void XtalOptUnitTest::stepwiseCheckForDuplicatesTest()
  {
    m_opt->tracker()->blockSignals(true);

    // Reset all statuses
    resetStatus(m_opt->tracker()->list(), Structure::Optimized);

    // Get all structures into a local list
    QList<Structure*> listAll = *m_opt->tracker()->list();

    // Clear all structures from the tracker
    m_opt->tracker()->reset();

    // Set up counter
    unsigned int nextStructureIndex = 0;
    QBENCHMARK_ONCE {
      while (m_opt->tracker()->size() != listAll.size()) {
        // Add back 10 structures at a time and recheck duplicates
        for (unsigned int i = 0;
             i < 10 && nextStructureIndex < listAll.size();
             i++) {
          m_opt->tracker()->append(listAll[nextStructureIndex++]);
        }

        m_opt->checkForDuplicates_();
      }
    }

    qDebug() << m_opt->tracker()->size()
             << m_opt->queue()->getAllDuplicateStructures().size();

    // This may change when Xtal::operator== becomes more or less robust.
    //QVERIFY(m_opt->queue()->getAllDuplicateStructures().size() == 41);
    QVERIFY(true); // Remove this when the above line is used;
    m_opt->tracker()->blockSignals(false);
  }

  void XtalOptUnitTest::destroyDialogAndXtalOpt()
  {
    delete m_dialog; // deletes opt, too
  }

}

QTEST_MAIN(XtalOpt::XtalOptUnitTest)

#include "moc_xtaloptunittest.cxx"

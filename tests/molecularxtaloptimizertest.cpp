/**********************************************************************
  MolecularXtalOptimizerTest

  Copyright (C) 2011 David C. Lonie

  XtalOpt is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
 **********************************************************************/

#include <xtalopt/molecularxtaloptimizer.h>

#include <xtalopt/structures/molecularxtal.h>
#include <xtalopt/structures/submolecule.h>
#include <xtalopt/structures/submoleculesource.h>

#include <avogadro/moleculefile.h>

#include <QtTest/QtTest>

#define _USE_MATH_DEFINES
#include <cmath>

using namespace Avogadro;
using namespace XtalOpt;

class MolecularXtalOptimizerTest : public QObject
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
  void benchmark();
  void ureaOptimization();
};

void MolecularXtalOptimizerTest::initTestCase()
{
}

void MolecularXtalOptimizerTest::cleanupTestCase()
{
}

void MolecularXtalOptimizerTest::init()
{
}

void MolecularXtalOptimizerTest::cleanup()
{
}

void MolecularXtalOptimizerTest::benchmark()
{
  // Build a molecular xtal with 2 urea, optimize and time.
  Molecule *urea = MoleculeFile::readMolecule(TESTDATADIR "/cmls/urea.cml");
  SubMoleculeSource source (urea);
  delete urea;
  urea = NULL;

  // The initial urea has the carbon at the origin, C=O along z axis, all
  // atoms in xz plane.

  MolecularXtal mxtal (5, 5, 5, 90, 90, 90);

  SubMolecule *sub = source.getSubMolecule();
  mxtal.addSubMolecule(sub);

  sub = source.getSubMolecule();
  mxtal.addSubMolecule(sub);
  sub->translateFrac(0, 0.5, 0);

  sub = source.getSubMolecule();
  mxtal.addSubMolecule(sub);
  sub->translateFrac(0.5, 0.25, 0.5);
  sub->rotate(M_PI, 0, 1, 0);

  sub = source.getSubMolecule();
  mxtal.addSubMolecule(sub);
  sub->translateFrac(0.5, 0.75, 0.5);
  sub->rotate(M_PI, 0, 1, 0);

  MolecularXtalOptimizer opt;
  opt.setMXtal(&mxtal);
  opt.setEnergyConvergence(1e-10); // We don't want to converge here
  opt.setNumberOfGeometrySteps(50);
  opt.setSuperCellUpdateInterval(20);

  mxtal.wrapAtomsToCell();

//  MoleculeFile::writeMolecule(&mxtal, "MXtalOptBenchmark-before.cml");

  QBENCHMARK_ONCE {
    opt.setup();
    opt.run();
  }

  QVERIFY(opt.reachedStepLimit());

  opt.updateMXtalCoords();
//  MoleculeFile::writeMolecule(&mxtal, "MXtalOptBenchmark-after.cml");
}

// Orient a urea submolecule so that the carbon is a 0,0,0, the normal of the
// molecule is aligned with y, and C=O points down z.
void orientUrea(SubMolecule *sub)
{
  // Find the carbon and oxygen atoms
  Atom *C = NULL;
  Atom *O = NULL;
  foreach (Atom *a, sub->atoms()) {
    if (a->atomicNumber() == 6) {
      C = a;
    }
    else if (a->atomicNumber() == 8) {
      O = a;
    }
    if (C != NULL && O != NULL) {
      break;
    }
  }

  // Move c to center
  sub->translate(-(*C->pos()));

  // Align molecule to x/z plane
  sub->align(Eigen::Vector3d(0.0, 1.0, 0.0),
             Eigen::Vector3d(0.0, 0.0, 1.0));

  // Rotate so that C=O bond points down z
  Eigen::Vector3d CObond = (*O->pos() - *C->pos()).normalized();
  Eigen::Vector3d zAxis (0.0, 0.0, 1.0);
  double angleRad = acos(CObond.dot(zAxis));

  sub->rotate(angleRad, CObond.cross(zAxis));
}

void MolecularXtalOptimizerTest::ureaOptimization()
{
  Molecule *urea = MoleculeFile::readMolecule(TESTDATADIR "/cmls/urea.cml");
  SubMoleculeSource source (urea);
  delete urea;
  urea = NULL;

  MolecularXtal mxtal (5.565, 5.565, 4.684, 90.0, 90.0, 90.0);

  // Construct the 12K structure described in Acta Cryst. (1984). B40, 300-306
  // doi:10.1107/S0108768184002135
  SubMolecule *sub = source.getSubMolecule();
  orientUrea(sub);
  mxtal.addSubMolecule(sub);
  sub->translateFrac(0.5, 0.0, 0.3260);
  sub->rotate(-M_PI_4, 0.0, 0.0, 1.0);

  sub = source.getSubMolecule();
  orientUrea(sub);
  mxtal.addSubMolecule(sub);
  sub->translateFrac(0.0, 0.5, 0.6740);
  sub->rotate(M_PI, 0.0, 1.0, 0.0);
  sub->rotate(M_PI_4, 0.0, 0.0, 1.0);

//  MoleculeFile::writeMolecule(&mxtal, "/tmp/urea-01-unoptimized.cml", "cml", "", NULL);

  MolecularXtalOptimizer mxtalOpt;
  mxtalOpt.setMXtal(&mxtal);
  mxtalOpt.setEnergyConvergence(1e-8);
  mxtalOpt.setNumberOfGeometrySteps(10000);
  mxtalOpt.setSuperCellUpdateInterval(20);
  mxtalOpt.setVDWCutoff(15.0);
  mxtalOpt.setElectrostaticCutoff(15.0);
  mxtalOpt.setCutoffUpdateInterval(-1); // Only on cell updates

//  mxtalOpt.setDebug(true);
  mxtalOpt.setup();
  mxtalOpt.run();
  mxtalOpt.waitForFinished();

  mxtalOpt.updateMXtalCoords();
  mxtalOpt.updateMXtalEnergy();
  mxtalOpt.releaseMXtal();

  QVERIFY(mxtalOpt.isConverged());
  // Expected energy in kJ/mol, remember that OB calculates kcal/mol if
  // comparing optimizer output!
  const double actualEnergy = mxtal.energy();
  const double expectedEnergy = -994.373442;
  QVERIFY(fabs(actualEnergy - expectedEnergy) < 1e-3);

//  MoleculeFile::writeMolecule(&mxtal, "/tmp/urea-02-optimized.cml", "cml", "p", NULL);

}

QTEST_MAIN(MolecularXtalOptimizerTest)

#include "moc_molecularxtaloptimizertest.cxx"

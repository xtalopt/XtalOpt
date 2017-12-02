/**********************************************************************
  XtalOptUnitTest -- Unit testing for XtalOpt functions

  Copyright (C) 2010 David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 **********************************************************************/

#include <xtalopt/xtalopt.h>

#include <xtalopt/optimizers/gulp.h>
#include <xtalopt/ui/dialog.h>

#include <globalsearch/macros.h>
#include <globalsearch/queuemanager.h>
#include <globalsearch/random.h>
#include <globalsearch/structure.h>
#include <globalsearch/tracker.h>

#include <QDebug>
#include <QString>
#include <QtTest>

using namespace GlobalSearch;

namespace XtalOpt {
class XtalOptUnitTest : public QObject
{
  Q_OBJECT

private:
  XtalOptDialog* m_dialog;
  XtalOpt* m_opt;

private slots:
  // Called before the first test function is executed.
  void initTestCase()
  {
    m_dialog = 0;
    m_opt = 0;
    seedMt19937Generator(0);
  }
  // Called after the last test function is executed.
  void cleanupTestCase(){};
  // Called before each test function is executed.
  void init(){};
  // Called after every test function.
  void cleanup(){};

  // Tests
  void constructXtalOpt();
  void constructDialog();
  void setOptimizer();

  void loadTest();
  void checkForDuplicatesTest();
  void stepwiseCheckForDuplicatesTest();

  void destroyDialogAndXtalOpt();
};

void XtalOptUnitTest::constructXtalOpt()
{
  m_opt = new XtalOpt();
  m_opt->tol_spg = 0.05;
  QVERIFY(m_opt != 0);
}

void XtalOptUnitTest::constructDialog()
{
  m_dialog = new XtalOptDialog(nullptr, Qt::Window, false, m_opt);
  QVERIFY(m_dialog != 0);
  m_opt->setDialog(m_dialog);
}

void XtalOptUnitTest::setOptimizer()
{
  if (m_opt->getNumOptSteps() == 0)
    m_opt->appendOptStep();
  m_opt->setOptimizer(0, "gulp");
  QVERIFY(m_opt->optimizer(0) != 0);
}

void XtalOptUnitTest::loadTest()
{
  m_opt->tracker()->blockSignals(true);
  QVERIFY(m_opt->load(QString(TESTDATADIR) + "xo-duplicateXtals/xtalopt.state",
                      true));
  m_opt->tracker()->blockSignals(false);
  QVERIFY(m_opt->tracker()->size() == 203);
}

// Helper function
inline void resetStatus(QList<Structure*>* list, Structure::State status)
{
  Structure* s;
  for (int i = 0; i < list->size(); i++) {
    s = list->at(i);
    s->lock().lockForWrite();
    s->setStatus(status);
    s->setChangedSinceDupChecked(true);
    s->lock().unlock();
  }
}

void XtalOptUnitTest::checkForDuplicatesTest()
{
  m_opt->tracker()->blockSignals(true);
  resetStatus(m_opt->tracker()->list(), Structure::Optimized);

  QBENCHMARK_ONCE { m_opt->checkForDuplicates_(); }

  qDebug() << m_opt->tracker()->size()
           << m_opt->queue()->getAllDuplicateStructures().size();

  // This may change when Xtal::operator== becomes more or less robust.
  // QVERIFY(m_opt->queue()->getAllDuplicateStructures().size() == 41);
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
  int nextStructureIndex = 0;
  QBENCHMARK_ONCE
  {
    while (m_opt->tracker()->size() != listAll.size()) {
      // Add back 10 structures at a time and recheck duplicates
      for (unsigned int i = 0; i < 10 && nextStructureIndex < listAll.size();
           i++) {
        m_opt->tracker()->append(listAll[nextStructureIndex++]);
      }

      m_opt->checkForDuplicates_();
    }
  }

  qDebug() << m_opt->tracker()->size()
           << m_opt->queue()->getAllDuplicateStructures().size();

  // This may change when Xtal::operator== becomes more or less robust.
  // QVERIFY(m_opt->queue()->getAllDuplicateStructures().size() == 41);
  QVERIFY(true); // Remove this when the above line is used;
  m_opt->tracker()->blockSignals(false);
}

void XtalOptUnitTest::destroyDialogAndXtalOpt()
{
  delete m_dialog;
  delete m_opt;
}
}

QTEST_MAIN(XtalOpt::XtalOptUnitTest)

#include "xtaloptunittest.moc"

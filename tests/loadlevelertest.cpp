/**********************************************************************
  LoadLevelerTest

  Copyright (C) 2012 David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 **********************************************************************/

#include <globalsearch/queueinterfaces/loadleveler.h>

#include <xtalopt/xtalopt.h>

#include <QDebug>
#include <QString>
#include <QtTest>

namespace GlobalSearch {

class LoadLevelerTest : public QObject
{
  Q_OBJECT

private:
  XtalOpt::XtalOpt* m_opt;
  LoadLevelerQueueInterface* m_qi;

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
  void parseJobId();
  void parseStatus();
};

void LoadLevelerTest::initTestCase()
{
  m_opt = new XtalOpt::XtalOpt(NULL);
  m_qi = new LoadLevelerQueueInterface(m_opt, "");
}

void LoadLevelerTest::cleanupTestCase()
{
  delete m_opt;
}

void LoadLevelerTest::init()
{
}

void LoadLevelerTest::cleanup()
{
}

void LoadLevelerTest::parseJobId()
{
  QString testJobString = "llsubmit: The job \"host.102\" has been submitted.";
  bool ok;
  unsigned int jobId = m_qi->parseJobId(testJobString, &ok);
  QVERIFY(ok);
  QCOMPARE(jobId, 102u);
}

void LoadLevelerTest::parseStatus()
{
  QStringList statusList;
  statusList << "Id                       Owner      Submitted   ST PRI Class  "
                "      Running On";
  statusList << "------------------------ ---------- ----------- -- --- "
                "------------ -----------";
  statusList << "mars.498.0               brownap    5/20 11:31  R  100 silver "
                "      mars";
  statusList << "mars.499.0               brownap    5/20 11:31  Q  50  "
                "No_Class     mars";
  statusList << "mars.501.0               brownap    5/20 11:31  I  50  silver";
  statusList << "";
  statusList << "3 job step(s) in query, 1 waiting, 0 pending, 2 running, 0 "
                "held, 0 preempted";

  QString Status498 = m_qi->parseStatus(statusList, 498u);
  QString Status499 = m_qi->parseStatus(statusList, 499u);
  QString Status501 = m_qi->parseStatus(statusList, 501u);

  QCOMPARE(Status498, QString("R"));
  QCOMPARE(Status499, QString("Q"));
  QCOMPARE(Status501, QString("I"));
}
}

QTEST_MAIN(GlobalSearch::LoadLevelerTest)

#include "loadlevelertest.moc"

/**********************************************************************
  SSHManager - Unit test for the SSHManager class: libssh

  Copyright (C) 2010-2011 David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/ssh/sshconnection.h>
#include <search/ssh/sshmanager_libssh.h>
#include <common/fileutils.h>
#include <common/output.h>

#include "sshtestconfig.h"

#include <QString>
#include <QTemporaryFile>
#include <QtTest>

#define NUM_CONN 5

using namespace Search;

class SSHManagerLibSSHTest : public QObject
{
  Q_OBJECT

private:
  SSHManager* manager = nullptr;

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
  void lockAllAndExecute();
};

void SSHManagerLibSSHTest::initTestCase()
{
  const SSHTestConfig config = readSSHTestConfig();
  if (!config.enabled)
    QSKIP(qPrintable(config.skipMessage));

  // Open ssh connection
  manager = new SSHManagerLibSSH(NUM_CONN);
  try {
    manager->makeConnections(config.host, config.user, config.password, config.port);
  } catch (SSHConnection::SSHConnectionException) {
    QFAIL("Cannot connect to ssh server. Make sure that the connection opened "
          "in initTestCase() points to a valid account on a real host before "
          "debugging this failure.");
  }
}

void SSHManagerLibSSHTest::cleanupTestCase()
{
  if (manager)
    delete manager;
  manager = nullptr;
}

void SSHManagerLibSSHTest::init()
{
}

void SSHManagerLibSSHTest::cleanup()
{
}

void SSHManagerLibSSHTest::lockAllAndExecute()
{
  QList<SSHConnection*> list;

  for (int i = 0; i < NUM_CONN; i++) {
    list.append(manager->getFreeConnection());
  }

  QString command = "expr 2 + 4";
  QString stdout_str, stderr_str;
  int ec;

  for (auto* conn : list) {
    QVERIFY(conn->execute(command, stdout_str, stderr_str, ec));
    QCOMPARE(ec, 0);
    QCOMPARE(stdout_str, QString("6\n"));
    QVERIFY(stderr_str.isEmpty());
  }

  for (auto* conn : list) {
    manager->unlockConnection(conn);
  }
}

QTEST_MAIN(SSHManagerLibSSHTest)

#include "sshmanager_libsshtest.moc"

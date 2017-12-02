/**********************************************************************
  SSHManager - SSHManagerTest class provides unit testing for the
  SSHManager class

  Copyright (C) 2010-2011 David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 **********************************************************************/

#include <globalsearch/sshmanager_libssh.h>

#include <QString>
#include <QTemporaryFile>
#include <QtTest>

#define NUM_CONN 5

using namespace GlobalSearch;

class SSHManagerLibSSHTest : public QObject
{
  Q_OBJECT

private:
  SSHManager* manager;

  // Create a local directory structure:
  // [tmp path]/sshtesttmp/
  //                       testfile1
  //                       newdir/
  //                              testfile2
  QStringList m_dirLayout;
  QDir m_localTempDir;
  QString m_remoteDir;
  QString m_localNewDir;
  QString m_testfile1Contents;
  QString m_testfile2Contents;

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
  void copyThreads();
};

void SSHManagerLibSSHTest::initTestCase()
{
  // Create a local directory structure:
  // [tmp path]/sshtesttmp/
  //                       testfile1
  //                       newdir/
  //                              testfile2
  m_remoteDir = ".sshtmpdir";
  m_dirLayout << m_remoteDir + "/testfile1" << m_remoteDir + "/newdir/"
              << m_remoteDir + "/newdir/testfile2";
  m_localTempDir.mkpath(QDir::tempPath() + "/sshtesttmp");
  m_localTempDir.mkpath(QDir::tempPath() + "/sshtesttmp/newdir");
  m_localTempDir.setPath(QDir::tempPath() + "/sshtesttmp");
  m_localNewDir = m_localTempDir.path() + ".new";

  // Each testfile is ~1MB
  QString buffer(104857, '0');
  QFile testfile1(m_localTempDir.path() + "/testfile1");
  testfile1.open(QIODevice::WriteOnly);
  QTextStream teststream1(&testfile1);
  m_testfile1Contents = "This is the first file's contents.\n" + buffer;
  teststream1 << m_testfile1Contents;
  testfile1.close();

  QFile testfile2(m_localTempDir.path() + "/newdir/testfile2");
  testfile2.open(QIODevice::WriteOnly);
  QTextStream teststream2(&testfile2);
  m_testfile2Contents = "and these are the second's.\n" + buffer;
  teststream2 << m_testfile2Contents;
  testfile2.close();

  // Open ssh connection
  manager = new SSHManagerLibSSH(NUM_CONN);
  try {
    // Ensure that the following points to a real server, acct, and pw
    // combo. Do not commit any changes here! (considering using
    // /etc/hosts to map "testserver" to a real server with a
    // chroot-jailed acct/pw = "test")
    manager->makeConnections("testserver", "test", "test", 22);
  } catch (SSHConnection::SSHConnectionException) {
    QFAIL("Cannot connect to ssh server. Make sure that the connection opened "
          "in initTestCase() points to a valid account on a real host before "
          "debugging this failure.");
  }
}

void SSHManagerLibSSHTest::cleanupTestCase()
{
  QFile::remove(m_localTempDir.path() + "/testfile1");
  QFile::remove(m_localTempDir.path() + "/newdir/testfile2");
  m_localTempDir.rmdir(m_localTempDir.path() + "/newdir");
  m_localTempDir.rmdir(m_localTempDir.path());
  QFile::remove(m_localNewDir + "/testfile1");
  QFile::remove(m_localNewDir + "/newdir/testfile2");
  m_localTempDir.rmdir(m_localNewDir + "/newdir");
  m_localTempDir.rmdir(m_localNewDir);

  if (manager)
    delete manager;
  manager = 0;
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
  SSHConnection* conn;
  int ec;

  for (int i = 0; i < NUM_CONN; i++) {
    conn = list.at(i);
    QVERIFY(conn->execute(command, stdout_str, stderr_str, ec));
    QCOMPARE(ec, 0);
    QCOMPARE(stdout_str, QString("6\n"));
    QVERIFY(stderr_str.isEmpty());
  }

  for (int i = 0; i < NUM_CONN; i++) {
    manager->unlockConnection(list.at(i));
  }
}

class CopyThread : public QThread
{
public:
  CopyThread(SSHManager* m, const QString& f, const QString& t)
    : QThread(0), manager(m), from(f), to(t)
  {
  }
  void run()
  {
    SSHConnection* conn = manager->getFreeConnection();
    conn->copyDirectoryToServer(from, to);
    conn->removeRemoteDirectory(to);
    manager->unlockConnection(conn);
  }

private:
  SSHManager* manager;
  QString from, to;
};

void SSHManagerLibSSHTest::copyThreads()
{
  QList<CopyThread*> list;
  for (int i = 0; i < NUM_CONN * 20; ++i) {
    CopyThread* ct = new CopyThread(manager, m_localTempDir.path(),
                                    m_remoteDir + QString::number(i + 1));
    list.append(ct);
  }

  QBENCHMARK
  {
    for (int i = 0; i < list.size(); ++i) {
      list.at(i)->start();
    }

    for (int i = 0; i < list.size(); ++i) {
      list.at(i)->wait();
      qDebug() << "Thread " << i + 1 << " of " << list.size() << " finished.";
    }
  }
}

QTEST_MAIN(SSHManagerLibSSHTest)

#include "sshmanager_libsshtest.moc"

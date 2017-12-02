/**********************************************************************
  SSHConnection - SSHConnectionTest class provides unit testing for the
  SSHConnection class

  Copyright (C) 2010-2011 David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 **********************************************************************/

#include <globalsearch/sshconnection_cli.h>

#include <QString>
#include <QTemporaryFile>
#include <QtTest>

using namespace GlobalSearch;

class SSHConnectionCLITest : public QObject
{
  Q_OBJECT

private:
  SSHConnectionCLI* conn;
  QTemporaryFile m_localTempFile;
  QString m_remoteFileName;
  QString m_localNewFileName;
  QString m_fileContents;

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

  // Large file test
  QTemporaryFile m_largeTempFile;

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
  //  void isValid();
  //  void isConnected1();
  //  void disconnectSession();
  //  void isConnected2();
  //  void reconnectSession();

  void execute();
  //  void executeLargeOutput();

  void copyFileToServer();
  void readRemoteFile();
  void copyFileFromServer();
  void removeRemoteFile();

  void copyDirectoryToServer();
  void readRemoteDirectoryContents();
  void copyDirectoryFromServer();
  void removeRemoteDirectory();

  void largeFileCopyBenchmark();
};

void SSHConnectionCLITest::initTestCase()
{
  // Write local file for later manipulation
  m_fileContents = "This is a test file.\n\nIt has text in it.";
  m_localTempFile.open();
  QTextStream ts1(&m_localTempFile);
  ts1 << m_fileContents;
  m_localTempFile.close();

  // Set up other filenames
  m_remoteFileName = ".sshconnectiontest.tmp";
  m_localNewFileName = m_localTempFile.fileName() + ".new";

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

  QFile testfile1(m_localTempDir.path() + "/testfile1");
  testfile1.open(QIODevice::WriteOnly);
  QTextStream teststream1(&testfile1);
  m_testfile1Contents = "This is the first file's contents.\n";
  teststream1 << m_testfile1Contents;
  testfile1.close();

  QFile testfile2(m_localTempDir.path() + "/newdir/testfile2");
  testfile2.open(QIODevice::WriteOnly);
  QTextStream teststream2(&testfile2);
  m_testfile2Contents = "and these are the second's.\n";
  teststream2 << m_testfile2Contents;
  testfile2.close();

  // Create a large file, 10 MB
  // / Open local file
  m_largeTempFile.open();
  QTextStream lts(&m_largeTempFile);
  // / Create buffer
  QString buffer(1048576, '0');
  // / Write
  for (int i = 0; i < 10; i++)
    lts << buffer;
  m_largeTempFile.close();

  // Open ssh connection
  try {
    // Ensure that the following points to a real server, acct, and pw
    // combo. Do not commit any changes here! (considering using
    // /etc/hosts to map "testserver" to a real server with a
    // chroot-jailed acct/pw = "test")
    conn = new SSHConnectionCLI();
    conn->setLoginDetails("testserver", "test", "test");
    //    conn->connectSession();
  } catch (SSHConnection::SSHConnectionException) {
    conn = 0;
    QFAIL("Cannot connect to ssh server. Make sure that the connection opened "
          "in initTestCase() points to a valid account on a real host before "
          "debugging this failure.");
  }
}

void SSHConnectionCLITest::cleanupTestCase()
{
  QFile::remove(m_localTempFile.fileName());
  QFile::remove(m_localNewFileName);

  QFile::remove(m_localTempDir.path() + "/testfile1");
  QFile::remove(m_localTempDir.path() + "/newdir/testfile2");
  m_localTempDir.rmdir(m_localTempDir.path() + "/newdir");
  m_localTempDir.rmdir(m_localTempDir.path());
  QFile::remove(m_localNewDir + "/testfile1");
  QFile::remove(m_localNewDir + "/newdir/testfile2");
  m_localTempDir.rmdir(m_localNewDir + "/newdir");
  m_localTempDir.rmdir(m_localNewDir);

  QFile::remove(m_largeTempFile.fileName());

  if (conn)
    delete conn;
  conn = 0;
}

void SSHConnectionCLITest::init()
{
}

void SSHConnectionCLITest::cleanup()
{
}

void SSHConnectionCLITest::execute()
{
  QString command = "expr 2 + 4";
  QString stdout_str, stderr_str;
  int ec;

  // Execute the command 10 times.
  QBENCHMARK_ONCE
  {
    for (int i = 1; i <= 10; i++) {
      QVERIFY2(conn->execute(command, stdout_str, stderr_str, ec),
               QString("Execution of \'" + command + "\' (#" +
                       QString::number(i) + ") failed.")
                 .toStdString()
                 .c_str());
      QCOMPARE(ec, 0);
      QCOMPARE(stdout_str, QString("6\n"));
      QVERIFY2(stderr_str.isEmpty(),
               QString("Execution of \'" + command + "\' produced an error: " +
                       stderr_str)
                 .toStdString()
                 .c_str());
    }
  }
}

void SSHConnectionCLITest::copyFileToServer()
{
  QVERIFY2(conn->copyFileToServer(m_localTempFile.fileName(), m_remoteFileName),
           "Error copying file to server.");
}

void SSHConnectionCLITest::readRemoteFile()
{
  QString fileContents;
  QVERIFY2(conn->readRemoteFile(m_remoteFileName, fileContents),
           "Error reading remote file.");
  QCOMPARE(fileContents, m_fileContents);
}

void SSHConnectionCLITest::copyFileFromServer()
{
  QVERIFY2(conn->copyFileFromServer(m_remoteFileName, m_localNewFileName),
           "Error copying file from server.");
  // Ensure that new local file matches original local file.
  QFile newFile(m_localNewFileName);
  newFile.open(QIODevice::ReadOnly);
  m_localTempFile.open();
  QCOMPARE(newFile.readAll(), m_localTempFile.readAll());
  newFile.close();
  m_localTempFile.close();
}

void SSHConnectionCLITest::removeRemoteFile()
{
  QVERIFY2(conn->removeRemoteFile(m_remoteFileName),
           "Error removing remote file.");
}

void SSHConnectionCLITest::copyDirectoryToServer()
{
  QVERIFY2(conn->copyDirectoryToServer(m_localTempDir.path(), m_remoteDir),
           "Error copying directory to server.");
  QString cont1, cont2;
  conn->readRemoteFile(m_remoteDir + "/testfile1", cont1);
  conn->readRemoteFile(m_remoteDir + "/newdir/testfile2", cont2);
  QCOMPARE(cont1, m_testfile1Contents);
  QCOMPARE(cont2, m_testfile2Contents);
}

void SSHConnectionCLITest::readRemoteDirectoryContents()
{
  QStringList contents;
  QVERIFY(conn->readRemoteDirectoryContents(m_remoteDir, contents));
  qSort(contents);
  qSort(m_dirLayout);
  QCOMPARE(contents, m_dirLayout);
}

void SSHConnectionCLITest::copyDirectoryFromServer()
{
  QVERIFY2(conn->copyDirectoryFromServer(m_remoteDir, m_localNewDir),
           "Error copying directory from server.");

  QFile f1(m_localNewDir + "/testfile1");
  f1.open(QIODevice::ReadOnly);
  QString cont1(f1.readAll());
  QCOMPARE(cont1, m_testfile1Contents);

  QFile f2(m_localNewDir + "/newdir/testfile2");
  f2.open(QIODevice::ReadOnly);
  QString cont2(f2.readAll());
  QCOMPARE(cont2, m_testfile2Contents);
}

void SSHConnectionCLITest::removeRemoteDirectory()
{
  QVERIFY2(conn->removeRemoteDirectory(m_remoteDir),
           "Error removing remote directory.");
}

void SSHConnectionCLITest::largeFileCopyBenchmark()
{
  QBENCHMARK
  {
    conn->copyFileToServer(m_largeTempFile.fileName(), ".sshlargetest");
  }
  QBENCHMARK
  {
    conn->copyFileFromServer(".sshlargetest",
                             m_largeTempFile.fileName() + ".new");
  }
  conn->removeRemoteFile(".sshlargetest");
  QFile largeCopy(m_largeTempFile.fileName() + ".new");
  largeCopy.open(QIODevice::ReadOnly);
  m_largeTempFile.open();
  QCOMPARE(largeCopy.readAll(), m_largeTempFile.readAll());
  largeCopy.close();
  m_largeTempFile.close();
}

QTEST_MAIN(SSHConnectionCLITest)

#include "sshconnection_clitest.moc"

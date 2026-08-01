/**********************************************************************
  SSHConnection - Unit test for the SSHConnection class: cli-ssh

  Copyright (C) 2010-2011 David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/ssh/sshconnection_cli.h>

#include "sshtestconfig.h"

#include <common/fileutils.h>

#include <QString>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QtTest>

#include <algorithm>

using namespace Search;

class SSHConnectionCLITest : public QObject
{
  Q_OBJECT

private:
  SSHConnectionCLI* conn = nullptr;
  bool m_testFilesCreated = false;
  QTemporaryDir m_localTempRoot;
  QTemporaryFile m_localTempFile;
  QString m_remoteFileName;
  QString m_localNewFileName;
  QString m_fileContents;

  // Create a local directory structure:
  // [temporary path]/source/
  //                         testfile1
  //                         newdir/
  //                                testfile2
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
  void execute();

  void copyFileToServer();
  void readRemoteFile();
  void copyFileFromServer();
  void removeRemoteFile();

  void copyDirectoryToServer();
  void readRemoteDirectoryContents();
  void copyDirectoryFromServer();
  void removeRemoteDirectory();
};

void SSHConnectionCLITest::initTestCase()
{
  const SSHTestConfig config = readSSHTestConfig(false);
  if (!config.enabled)
    QSKIP(qPrintable(config.skipMessage));

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
  // [temporary path]/source/
  //                         testfile1
  //                         newdir/
  //                                testfile2
  m_remoteDir = ".sshtmpdir";
  const QString remoteNewDir = Common::remotePath(m_remoteDir, "newdir");
  m_dirLayout << Common::remotePath(m_remoteDir, "testfile1")
              << remoteNewDir
              << Common::remotePath(remoteNewDir, "testfile2");
  QVERIFY(m_localTempRoot.isValid());
  const QString localTestDir = Common::localPath(m_localTempRoot.path(), "source");
  const QString localNewSubdir = Common::localPath(localTestDir, "newdir");
  m_localTempDir.mkpath(localTestDir);
  m_localTempDir.mkpath(localNewSubdir);
  m_localTempDir.setPath(localTestDir);
  m_localNewDir = Common::localPath(m_localTempRoot.path(), "copy");

  QFile testfile1(Common::localPath(m_localTempDir.path(), "testfile1"));
  testfile1.open(QIODevice::WriteOnly);
  QTextStream teststream1(&testfile1);
  m_testfile1Contents = "This is the first file's contents.\n";
  teststream1 << m_testfile1Contents;
  testfile1.close();

  QFile testfile2(Common::localPath(localNewSubdir, "testfile2"));
  testfile2.open(QIODevice::WriteOnly);
  QTextStream teststream2(&testfile2);
  m_testfile2Contents = "and these are the second's.\n";
  teststream2 << m_testfile2Contents;
  testfile2.close();

  m_testFilesCreated = true;

  // Open ssh connection
  try {
    conn = new SSHConnectionCLI();
    conn->setLoginDetails(config.host, config.user, config.password, config.port);
    //    conn->connectSession();
  } catch (SSHConnection::SSHConnectionException) {
    conn = nullptr;
    QFAIL("Cannot connect to ssh server. Make sure that the connection opened "
          "in initTestCase() points to a valid account on a real host before "
          "debugging this failure.");
  }
}

void SSHConnectionCLITest::cleanupTestCase()
{
  if (m_testFilesCreated) {
    QFile::remove(m_localTempFile.fileName());
    QFile::remove(m_localNewFileName);

    QFile::remove(Common::localPath(m_localTempDir.path(), "testfile1"));
    QFile::remove(Common::localPath(
      Common::localPath(m_localTempDir.path(), "newdir"), "testfile2"));
    m_localTempDir.rmdir(Common::localPath(m_localTempDir.path(), "newdir"));
    m_localTempDir.rmdir(m_localTempDir.path());
    QFile::remove(Common::localPath(m_localNewDir, "testfile1"));
    QFile::remove(Common::localPath(Common::localPath(m_localNewDir, "newdir"), "testfile2"));
    m_localTempDir.rmdir(Common::localPath(m_localNewDir, "newdir"));
    m_localTempDir.rmdir(m_localNewDir);
  }

  if (conn)
    delete conn;
  conn = nullptr;
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

  // Only one execution to verify the command channel works.
  QVERIFY2(conn->execute(command, stdout_str, stderr_str, ec),
           QString("Execution of \'" + command + "\' failed.")
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
  conn->readRemoteFile(Common::remotePath(m_remoteDir, "testfile1"), cont1);
  conn->readRemoteFile(Common::remotePath(Common::remotePath(m_remoteDir, "newdir"), "testfile2"),
                       cont2);
  QCOMPARE(cont1, m_testfile1Contents);
  QCOMPARE(cont2, m_testfile2Contents);
}

void SSHConnectionCLITest::readRemoteDirectoryContents()
{
  QStringList contents;
  QVERIFY(conn->readRemoteDirectoryContents(m_remoteDir, contents));
  std::sort(contents.begin(), contents.end());
  std::sort(m_dirLayout.begin(), m_dirLayout.end());
  QCOMPARE(contents, m_dirLayout);
}

void SSHConnectionCLITest::copyDirectoryFromServer()
{
  QVERIFY2(conn->copyDirectoryFromServer(m_remoteDir, m_localNewDir),
           "Error copying directory from server.");

  QFile f1(Common::localPath(m_localNewDir, "testfile1"));
  f1.open(QIODevice::ReadOnly);
  QString cont1(f1.readAll());
  QCOMPARE(cont1, m_testfile1Contents);

  QFile f2(Common::localPath(Common::localPath(m_localNewDir, "newdir"), "testfile2"));
  f2.open(QIODevice::ReadOnly);
  QString cont2(f2.readAll());
  QCOMPARE(cont2, m_testfile2Contents);
}

void SSHConnectionCLITest::removeRemoteDirectory()
{
  QVERIFY2(conn->removeRemoteDirectory(m_remoteDir),
           "Error removing remote directory.");
}

QTEST_MAIN(SSHConnectionCLITest)

#include "sshconnection_clitest.moc"

/**********************************************************************
  FileUtilsTest - Unit tests for file unility in common

  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <common/fileutils.h>

#include <QDir>
#include <QtTest>

class FileUtilsTest : public QObject
{
  Q_OBJECT

private slots:
  void localPathUsesQtPathRules();
  void remotePathUsesPosixPathRules();
};

void FileUtilsTest::localPathUsesQtPathRules()
{
  QCOMPARE(Common::localPath("", "child"), QString("child"));
  QCOMPARE(Common::localPath("base", ""), QString("base"));
  QCOMPARE(Common::localPath("base", "child"), QDir("base").filePath("child"));

  const QString absoluteChild = QDir::current().absoluteFilePath("absolute-child");
  QCOMPARE(Common::localPath("base", absoluteChild), absoluteChild);
}

void FileUtilsTest::remotePathUsesPosixPathRules()
{
  QCOMPARE(Common::remotePath("", "child"), QString("child"));
  QCOMPARE(Common::remotePath("/", "child"), QString("/child"));
  QCOMPARE(Common::remotePath("/remote/base/", "child"), QString("/remote/base/child"));
  QCOMPARE(Common::remotePath("~/work", "child"), QString("~/work/child"));
  QCOMPARE(Common::remotePath("/remote/base", "/absolute"), QString("/absolute"));
}

QTEST_MAIN(FileUtilsTest)

#include "fileutilstest.moc"

/**********************************************************************
  PasswordPromptTest - Non-interactive tests for CLI password prompt

  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/cliinterface.h>
#include <search/passwordprompt.h>

#include <xtalopt/xtalopt.h>

#include <iostream>
#include <sstream>

#include <QString>
#include <QtTest>

using namespace Search;

class ScopedCinRedirect
{
public:
  explicit ScopedCinRedirect(const std::string& input)
    : m_input(input), m_oldBuffer(std::cin.rdbuf(m_input.rdbuf()))
  {
  }

  ~ScopedCinRedirect()
  {
    std::cin.rdbuf(m_oldBuffer);
  }

private:
  std::istringstream m_input;
  std::streambuf* m_oldBuffer;
};

class PasswordPromptTest : public QObject
{
  Q_OBJECT

private slots:
  void readsPasswordFromStandardInput();
  void terminalInterfaceInstallsPasswordHandler();
};

void PasswordPromptTest::readsPasswordFromStandardInput()
{
  ScopedCinRedirect cin("direct-password\n");

  const std::string password = PasswordPrompt::getPassword("Password: ");

  QCOMPARE(QString::fromStdString(password), QString("direct-password"));
}

void PasswordPromptTest::terminalInterfaceInstallsPasswordHandler()
{
  ScopedCinRedirect cin("handler-password\n");
  ::XtalOpt::XtalOpt search;
  QString password;

  installTerminalInterface(search);
  const bool accepted = search.requestPassword("Password: ", password);

  QVERIFY(accepted);
  QCOMPARE(password, QString("handler-password"));
}

QTEST_MAIN(PasswordPromptTest)

#include "passwordprompttest.moc"

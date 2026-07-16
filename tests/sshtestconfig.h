/**********************************************************************
  SSHTestConfig - Some helpful tools for SSH testing

  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef SSH_TEST_CONFIG_H
#define SSH_TEST_CONFIG_H

#include <QByteArray>
#include <QString>
#include <QtGlobal>

struct SSHTestConfig
{
  bool enabled;
  QString host;
  QString user;
  QString password;
  unsigned int port;
  QString skipMessage;
};

inline SSHTestConfig readSSHTestConfig(bool requirePassword = true)
{
  SSHTestConfig config;
  config.enabled = false;
  config.port = 22;

  config.host = QString::fromLocal8Bit(qgetenv("XTALOPT_SSH_TEST_HOST"));
  config.user = QString::fromLocal8Bit(qgetenv("XTALOPT_SSH_TEST_USER"));
  config.password = QString::fromLocal8Bit(qgetenv("XTALOPT_SSH_TEST_PASSWORD"));

  const QByteArray portEnv = qgetenv("XTALOPT_SSH_TEST_PORT");
  if (!portEnv.isEmpty()) {
    bool ok = false;
    const uint port = portEnv.toUInt(&ok);
    if (ok && port > 0)
      config.port = port;
  }

  if (config.host.isEmpty() || config.user.isEmpty() ||
      (requirePassword && config.password.isEmpty())) {
    if (requirePassword) {
      config.skipMessage = "Skipping SSH integration test. Set XTALOPT_SSH_TEST_HOST, "
        "XTALOPT_SSH_TEST_USER, and XTALOPT_SSH_TEST_PASSWORD to enable it.";
    } else {
      config.skipMessage = "Skipping SSH integration test. Set XTALOPT_SSH_TEST_HOST and "
        "XTALOPT_SSH_TEST_USER to enable it.";
    }
    return config;
  }

  config.enabled = true;
  return config;
}

#endif // SSH_TEST_CONFIG_H

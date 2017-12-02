/**********************************************************************
  OBConvert -- Use an Open Babel "obabel" executable to convert file types

  Copyright (C) 2017 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QProcess>
#include <QString>
#include <QStringList>

#include "obconvert.h"

namespace GlobalSearch {

bool OBConvert::convertFormat(const QString& inFormat, const QString& outFormat,
                              const QByteArray& input, QByteArray& output,
                              const QStringList& options)
{
  QStringList args;
  args << "-i" + inFormat;
  args << "-o" + outFormat;
  args << options;

  return executeOBabel(args, input, output);
}

bool OBConvert::executeOBabel(const QStringList& args, const QByteArray& input,
                              QByteArray& output)
{
  QString program;
  // If the OBABEL_EXECUTABLE environment variable is set, then
  // use that to find obabel
  QByteArray obabelExec = qgetenv("OBABEL_EXECUTABLE");
  if (!obabelExec.isEmpty()) {
    program = obabelExec;
  } else {
// Otherwise, search in the current directory, and then ../bin
#ifdef _WIN32
    QString executable = "obabel.exe";
#else
    QString executable = "obabel";
#endif
    QString path = QCoreApplication::applicationDirPath();
    if (QFile::exists(path + "/" + executable))
      program = path + "/" + executable;
    else if (QFile::exists(path + "/../bin/" + executable))
      program = path + "/../bin/" + executable;
    else {
      qDebug() << "Error: could not find obabel executable!";
      return false;
    }
  }

  QProcess p;
  p.start(program, args);

  if (!p.waitForStarted()) {
    qDebug() << "Error: The obabel executable at" << program
             << "failed to start.";
    return false;
  }

  // Give it the input!
  p.write(input.data());

  // Close the write channel
  p.closeWriteChannel();

  if (!p.waitForFinished()) {
    qDebug() << "Error: " << program << "failed to finish.";
    output = p.readAll();
    qDebug() << "Output is as follows:\n" << output;
    return false;
  }

  int exitStatus = p.exitStatus();
  output = p.readAll();

  if (exitStatus == QProcess::CrashExit) {
    qDebug() << "Error: obabel crashed!\n";
    qDebug() << "Output is as follows:\n" << output;
    return false;
  }

  if (exitStatus != QProcess::NormalExit) {
    qDebug() << "Error: obabel finished abnormally with exit code "
             << exitStatus;
    qDebug() << "Output is as follows:\n" << output;
    return false;
  }

  // We did it!
  return true;
}
}

/**********************************************************************
  GenerateXrd - Use ObjCryst++ to generate a simulated x-ray diffraction
                pattern

  Copyright (C) 2018 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <sstream>

#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QProcess>
#include <QString>

#include <globalsearch/formats/cmlformat.h>
#include <globalsearch/formats/obconvert.h>
#include <globalsearch/structure.h>

#include "generatexrd.h"

namespace GlobalSearch {

bool GenerateXrd::executeGenXrdPattern(const QStringList& args,
                                       const QByteArray& input,
                                       QByteArray& output)
{
  QString program;
  // If the GENXRDPATTERN_EXECUTABLE environment variable is set, then
  // use that
  QByteArray xrdExec = qgetenv("GENXRDPATTERN_EXECUTABLE");
  if (!xrdExec.isEmpty()) {
    program = xrdExec;
  } else {
// Otherwise, search in the current directory, and then ../bin
#ifdef _WIN32
    QString executable = "genXrdPattern.exe";
#else
    QString executable = "genXrdPattern";
#endif
    QString path = QCoreApplication::applicationDirPath();
    if (QFile::exists(path + "/" + executable))
      program = path + "/" + executable;
    else if (QFile::exists(path + "/../bin/" + executable))
      program = path + "/../bin/" + executable;
    else {
      qDebug() << "Error: could not find genXrdPattern executable!";
      return false;
    }
  }

  QProcess p;
  p.start(program, args);

  if (!p.waitForStarted()) {
    qDebug() << "Error: The genXrdPattern executable at" << program
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
    qDebug() << "Error:" << program << "crashed!\n";
    qDebug() << "Output is as follows:\n" << output;
    return false;
  }

  if (exitStatus != QProcess::NormalExit) {
    qDebug() << "Error:" << program << "finished abnormally with exit code "
             << exitStatus;
    qDebug() << "Output is as follows:\n" << output;
    return false;
  }

  // We did it!
  return true;
}

bool GenerateXrd::generateXrdPattern(const Structure& s, XrdData& results,
                                     double wavelength, double peakwidth,
                                     size_t numpoints, double max2theta)
{
  // First, write the structure in CML format
  std::stringstream cml;

  if (!CmlFormat::write(s, cml)) {
    qDebug() << "Error in" << __FUNCTION__ << ": failed to convert structure'"
             << s.getGeneration() << "x" << s.getIDNumber() << "' to CML"
             << "format!";
    return false;
  }

  // Then, convert to CIF format
  QByteArray cif;
  if (!OBConvert::convertFormat("cml", "cif", cml.str().c_str(), cif)) {
    qDebug() << "Error in" << __FUNCTION__ << ": failed to convert CML"
             << "format to CIF format with obabel";
    return false;
  }

  // Now, execute genXrdPattern with the given inputs
  QStringList args;
  args << "--read-from-stdin"
       << "--wavelength=" + QString::number(wavelength)
       << "--peakwidth=" + QString::number(peakwidth)
       << "--numpoints=" + QString::number(numpoints)
       << "--max2theta=" + QString::number(max2theta);

  QByteArray output;
  if (!executeGenXrdPattern(args, cif, output)) {
    qDebug() << "Error in" << __FUNCTION__ << ": failed to run external"
             << "genXrdPattern program";
    return false;
  }

  // Store the results
  results.clear();
  bool dataStarted = false;

  QStringList lines = QString(output).split(QRegExp("[\r\n]"),
                                            QString::SkipEmptyParts);
  for (const auto& line : lines) {
    if (!dataStarted && line.contains("#    2Theta/TOF    ICalc")) {
      dataStarted = true;
      continue;
    }

    if (dataStarted) {
      QStringList rowData = line.split(" ", QString::SkipEmptyParts);
      if (rowData.size() != 2) {
        qDebug() << "Error in" << __FUNCTION__ << ": data read from"
                 << "genXrdPattern appears to be corrupt! Data is:";
        for (const auto& lineTmp: lines)
          qDebug() << lineTmp;
        return false;
      }
      results.push_back(
        std::make_pair(rowData[0].toDouble(), rowData[1].toDouble()));
    }
  }

  return true;
}

} // end namespace GlobalSearch

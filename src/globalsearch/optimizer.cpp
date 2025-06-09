/**********************************************************************
  Optimizer - Generic optimizer interface

  Copyright (C) 2010-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <globalsearch/optimizer.h>

#include <globalsearch/formats/formats.h>
#include <globalsearch/macros.h>
#include <globalsearch/searchbase.h>
#include <globalsearch/optimizerdialog.h>
#include <globalsearch/queueinterface.h>
#include <globalsearch/structure.h>
#include <globalsearch/utilities/utilityfunctions.h>

#include <Eigen/Core>

#include <QDebug>
#include <QDir>
#include <QReadLocker>
#include <QSettings>
#include <QString>

namespace GlobalSearch {

Optimizer::Optimizer(SearchBase* parent, const QString& filename)
  : QObject(parent), m_search(parent), m_hasDialog(true), m_dialog(0)
{
  // Set allowed data structure keys, if any, e.g.
  // m_data.insert("Identifier name",QVariant())

  // Set allowed filenames, e.g.
  // m_templates.insert("filename.extension",QStringList)

  // Setup for completion values
  // m_completionFilename = name of file to check when opt stops
  // m_completionStrings.clear();
  // m_completionStrings.append("string in m_completionFilename to search for");
  // m_completionStrings.append("Another string");

  // Set output filenames to try to read data from, e.g.
  // m_outputFilenames.append("output filename");
  // m_outputFilenames.append("input  filename");

  // Set up commands
  // Three common scenarios:
  //
  // VASP-esque: $ vasp
  //  Runs in working directory reading from predefined input
  //  filenames (ie. POSCAR). Set m_localRunCommand="vasp", and
  //  m_stdinFilename=m_stdoutFilename=m_stderrFilename="";
  //
  // GULP-esque: $ gulp < job.gin 1>job.got 2>job.err
  //
  //  Runs in working directory using redirection to specify
  //  input/output. Set m_localRunCommand="gulp",
  //  m_stdinFilename="job.gin", m_stdoutFilename="job.got",
  //  m_stderrFilename="job.err".
  //
  // MOPAC-esque: $ mopac job
  //
  //  Runs in working directory, specifying either an input filename
  //  or a base name. In both cases, put the entire command line
  //  into m_localRunCommand, ="mopac job",
  //  m_stdinFilename=m_stdoutFilename=m_stderrFilename=""
  //
  // m_localRunCommand = how to run this optimizer from the commandline
  // m_stdinFilename   = name of standard input file
  // m_stdoutFilename  = name of standard output file
  // m_stderrFilename  = name of standard error file
  //
  // Stdin/out/err is not used by default:
  m_stdinFilename = "";
  m_stdoutFilename = "";
  m_stderrFilename = "";

  // Set the name of the optimizer to be returned by getIDString()
  m_idString = "Generic";

  readSettings(filename);
}

Optimizer::~Optimizer()
{
}

void Optimizer::readSettings(const QString& filename)
{
  // Don't consider default setting,, only schemes and states.
  if (filename.isEmpty())
    return;

  readDataFromSettings(filename);
}

void Optimizer::readDataFromSettings(const QString& filename)
{
  SETTINGS(filename);

  // Figure out what opt index this is.
  int optInd = m_search->optimizerIndex(this);
  if (optInd < 0)
    return;

  QStringList ids = getDataIdentifiers();
  for (int i = 0; i < ids.size(); i++) {
    m_data.insert(ids.at(i), settings->value(m_search->getIDString().toLower() +
                                             "/optimizer/" + getIDString() +
                                             "/" + QString::number(optInd) +
                                             "/data/" + ids.at(i), ""));
  }
}

void Optimizer::writeSettings(const QString& filename)
{
  // Don't consider default settings, only schemes and states.
  if (filename.isEmpty())
    return;

  writeDataToSettings(filename);
}

void Optimizer::writeDataToSettings(const QString& filename)
{
  SETTINGS(filename);

  // Figure out what opt index this is.
  int optInd = m_search->optimizerIndex(this);
  if (optInd < 0)
    return;

  QStringList ids = getDataIdentifiers();
  for (int i = 0; i < ids.size(); i++) {
    settings->setValue(m_search->getIDString().toLower() + "/optimizer/" +
                       getIDString() + "/" + QString::number(optInd) +
                       "/data/" + ids.at(i), m_data.value(ids.at(i)));
  }
}

QHash<QString, QString> Optimizer::getInterpretedTemplates(Structure* s)
{
  // Stop any running jobs associated with this structure
  m_search->queueInterface(s->getCurrentOptStep())->stopJob(s);

  // Lock
  QReadLocker locker(&s->lock());

  // Check optstep info
  int optStep = s->getCurrentOptStep();

  // Unlock for optimizer calls
  locker.unlock();

  // Build hash
  QHash<QString, QString> hash;
  QStringList filenames =
    m_search->queueInterface(optStep)->getTemplateFileNames();
  filenames.append(m_search->optimizer(optStep)->getTemplateFileNames());

  for (const auto& filename : filenames) {
    // For debugging template issues
    // qDebug() << "optStep is" << optStep;
    std::string temp = m_search->getTemplate(optStep, filename.toStdString());
    //
    //
    // FIXME: this is a "mark"! Adjustment for sub-system POTCAR files.
    // For such a run with other optimizers, this fix does not seem to be needed.
    // Also, this is not done for a single "system" POTCAR file.
    std::vector<std::string> all_temp = splitStringByNewline(temp);
    if (all_temp.size() > 1 && filename == "POTCAR") {
      temp = "";
      QList<QString> allsymbols = m_search->getChemicalSystem();
      for (int i = 0; i < allsymbols.size(); i++) {
        if (s->getNumberOfAtomsOfSymbol(allsymbols[i]) != 0)
          temp += all_temp[i] + '\n';
      }
    }
    //
    //
    hash.insert(filename, m_search->interpretTemplate(temp.c_str(), s));
  }

  return hash;
}

QDialog* Optimizer::dialog()
{
  if (!m_dialog) {
    if (!m_search->dialog())
      return nullptr;
    m_dialog = new OptimizerConfigDialog(m_search->dialog(), m_search, this);
  }
  OptimizerConfigDialog* d = qobject_cast<OptimizerConfigDialog*>(m_dialog);
  d->updateGUI();

  return d;
}

bool Optimizer::checkIfOutputFileExists(Structure* s, bool* exists)
{
  return m_search->queueInterface(s->getCurrentOptStep())
    ->checkIfFileExists(s, m_completionFilename, exists);
}

bool Optimizer::checkForSuccessfulOutput(Structure* s, bool* success)
{
  int ec;
  *success = false;
  for (QStringList::const_iterator it = m_completionStrings.constBegin(),
                                   it_end = m_completionStrings.constEnd();
       it != it_end; ++it) {
    if (!m_search->queueInterface(s->getCurrentOptStep())
           ->grepFile(s, (*it), m_completionFilename, 0, &ec)) {
      qDebug().noquote() << QString("Completion string '%1' not found in the output (%2)")
                                .arg((*it)).arg(s->getTag());
      return false;
    }
    if (ec == 0) {
      *success = true;
      return true;
    }
  }
  return true;
}

bool Optimizer::update(Structure* structure)
{
  // lock structure
  QWriteLocker locker(&structure->lock());

  structure->stopOptTimer();

  // Copy remote files over, other prep work:
  locker.unlock();
  bool ok = m_search->queueInterface(structure->getCurrentOptStep())
              ->prepareForStructureUpdate(structure);
  locker.relock();
  if (!ok) {
    m_search->warning(
      tr("Optimizer::update: Error while preparing to update structure %1")
        .arg(structure->getTag()));
    return false;
  }

  // Try to read all files in outputFileNames
  ok = false;
  for (int i = 0; i < m_outputFilenames.size(); i++) {
    if (read(structure,
             structure->getLocpath() + "/" + m_outputFilenames.at(i))) {
      ok = true;
      break;
    }
  }
  if (!ok) {
    m_search->warning(tr("Optimizer::Update: Error loading structure at %1")
                     .arg(structure->getLocpath()));
    return false;
  }

  structure->setJobID(0);
  locker.unlock();
  return true;
}

bool Optimizer::load(Structure* structure)
{
  QWriteLocker locker(&structure->lock());

  // Try to read all files in outputFileNames
  bool ok = false;
  for (int i = 0; i < m_outputFilenames.size(); i++) {
    if (read(structure,
             structure->getLocpath() + "/" + m_outputFilenames.at(i))) {
      ok = true;
      break;
    }
  }
  if (!ok) {
    m_search->warning(tr("Optimizer::load: Error loading structure at %1")
                     .arg(structure->getLocpath()));
    return false;
  }
  return true;
}

bool Optimizer::read(Structure* structure, const QString& filename)
{
  // Test filename
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return false;
  }
  file.close();

  if (!Formats::read(structure, filename, m_idString)) {
    qDebug().noquote() << "Failed to read the output file " +
      filename + " for " + structure->getTag();
    return false;
  }

  return true;
}

bool Optimizer::setData(const QString& identifier, const QVariant& data)
{
  Q_ASSERT(m_data.contains(identifier));

  m_data.insert(identifier, data);
  return true;
}

QVariant Optimizer::getData(const QString& identifier)
{
  Q_ASSERT(m_data.contains(identifier));

  return m_data.value(identifier);
}

} // end namespace GlobalSearch

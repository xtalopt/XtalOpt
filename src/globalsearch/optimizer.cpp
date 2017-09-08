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
#include <globalsearch/optbase.h>
#include <globalsearch/optimizerdialog.h>
#include <globalsearch/queueinterface.h>
#include <globalsearch/structure.h>

#include <Eigen/Core>

#include <QDebug>
#include <QDir>
#include <QReadLocker>
#include <QSettings>
#include <QString>

#define KCAL_PER_MOL_TO_EV 0.043364122

namespace GlobalSearch {

  Optimizer::Optimizer(OptBase *parent, const QString &filename) :
    QObject(parent),
    m_opt(parent),
    m_hasDialog(true),
    m_dialog(0)
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

    if (m_opt->queueInterface()) {
      updateQueueInterface();
    }
    connect(m_opt, SIGNAL(queueInterfaceChanged(const std::string&)),
            this, SLOT(updateQueueInterface()));

    readSettings(filename);
  }

  Optimizer::~Optimizer()
  {
  }

  void Optimizer::readSettings(const QString &filename)
  {
    // Don't consider default setting,, only schemes and states.
    if (filename.isEmpty())
      return;

    readTemplatesFromSettings(filename);
    readUserValuesFromSettings(filename);
    readDataFromSettings(filename);
  }

  void Optimizer::readTemplatesFromSettings(const QString &filename)
  {
    SETTINGS(filename);

    QStringList filenames = getTemplateNames();
    for (int i = 0; i < filenames.size(); i++) {
      QStringList temp = settings->value(m_opt->getIDString().toLower() +
                                         "/optimizer/" +
                                         getIDString() + "/" +
                                         filenames.at(i) + "_list",
                                         "").toStringList();
      temp.removeAll("");
      if (!temp.empty()) {
        m_templates.insert(filenames.at(i), temp);
        continue;
      }

      // If "temp" is empty, perhaps we have some template filenames to open
      QStringList templateFileNames =
        settings->value(m_opt->getIDString().toLower() + "/optimizer/" +
                        getIDString() + "/" + filenames.at(i) + "_templates",
                        "").toStringList();
      templateFileNames.removeAll("");
      // Loop through the files and see if they exist. If they do, store
      // the contents in m_templates
      for (const auto& templateFile : templateFileNames) {
        QFile file(templateFile);
        if (!file.exists()) {
          qWarning() << "Warning in " << __FUNCTION__ << ": " << templateFile
                     << "does not exist!";
          continue;
        }
        if (!file.open(QIODevice::ReadOnly)) {
          qWarning() << "Warning in " << __FUNCTION__ << ": " << templateFile
                     << "could not be opened!";
          continue;
        }
        temp.append(file.readAll());
        file.close();
      }
      m_templates.insert(filenames[i], temp);
    }

    // QueueInterface templates
    settings->beginGroup(m_opt->getIDString().toLower() +
                         "/optimizer/" +
                         getIDString() + "/QI/" +
                         m_opt->queueInterface()->getIDString());
    filenames = m_QITemplates.keys();
    for (QStringList::const_iterator
           it = filenames.constBegin(),
           it_end = filenames.constEnd();
         it != it_end;
         ++it) {
      m_QITemplates.insert(*it,
                           settings->value((*it) + "_list",
                                           "").toStringList());
    }
    settings->endGroup();

    fixTemplateLengths();
  }

  void Optimizer::readUserValuesFromSettings(const QString &filename)
  {
    SETTINGS(filename);

    settings->beginGroup(m_opt->getIDString().toLower() +
                         "/optimizer/" +
                         getIDString());
    m_user1 = settings->value("/user1", "").toString();
    m_user2 = settings->value("/user2", "").toString();
    m_user3 = settings->value("/user3", "").toString();
    m_user4 = settings->value("/user4", "").toString();
    settings->endGroup();
  }

  void Optimizer::readDataFromSettings(const QString &filename)
  {
    SETTINGS(filename);

    QStringList ids = getDataIdentifiers();
    for (int i = 0; i < ids.size(); i++) {
      m_data.insert(ids.at(i),
                         settings->value(m_opt->getIDString().toLower() +
                                         "/optimizer/" +
                                         getIDString() + "/data/" +
                                         ids.at(i),
                                         ""));
    }
  }

  void Optimizer::writeSettings(const QString &filename)
  {
    // Don't consider default settings, only schemes and states.
    if (filename.isEmpty())
      return;

    writeTemplatesToSettings(filename);
    writeUserValuesToSettings(filename);
    writeDataToSettings(filename);
    if (m_opt->queueInterface()) {
      m_opt->queueInterface()->writeSettings(filename);
    }
  }

  void Optimizer::writeTemplatesToSettings(const QString &filename)
  {
    SETTINGS(filename);
    QStringList filenames = getTemplateNames();
    for (int i = 0; i < filenames.size(); i++) {
      settings->setValue(m_opt->getIDString().toLower() +
                         "/optimizer/" +
                         getIDString() + "/" +
                         filenames.at(i) + "_list",
                         m_templates.value(filenames.at(i)));
    }

    // QueueInterface templates
    settings->beginGroup(m_opt->getIDString().toLower() +
                         "/optimizer/" +
                         getIDString() + "/QI/" +
                         m_opt->queueInterface()->getIDString());
    filenames = m_QITemplates.keys();
    for (QStringList::const_iterator
           it = filenames.constBegin(),
           it_end = filenames.constEnd();
         it != it_end;
         ++it) {
      settings->setValue((*it) + "_list",
                         m_QITemplates.value(*it));
    }
    settings->endGroup();
  }

  void Optimizer::writeUserValuesToSettings(const QString &filename)
  {
    SETTINGS(filename);

    settings->setValue(m_opt->getIDString().toLower() +
                       "/optimizer/" +
                       getIDString() +
                       "/user1",
                       m_user1);
    settings->setValue(m_opt->getIDString().toLower() +
                       "/optimizer/" +
                       getIDString() +
                       "/user2",
                       m_user2);
    settings->setValue(m_opt->getIDString().toLower() +
                       "/optimizer/" +
                       getIDString() +
                       "/user3",
                       m_user3);
    settings->setValue(m_opt->getIDString().toLower() +
                       "/optimizer/" +
                       getIDString() +
                       "/user4",
                       m_user4);
  }

  void Optimizer::writeDataToSettings(const QString &filename)
  {
    SETTINGS(filename);
    QStringList ids = getDataIdentifiers();
    for (int i = 0; i < ids.size(); i++) {
      settings->setValue(m_opt->getIDString().toLower() +
                         "/optimizer/" +
                         getIDString() +
                         "/data/" +
                         ids.at(i),
                         m_data.value(ids.at(i)));
    }
  }

  QHash<QString, QString>
  Optimizer::getInterpretedTemplates(Structure *structure)
  {
    // Stop any running jobs associated with this structure
    m_opt->queueInterface()->stopJob(structure);

    // Lock
    QReadLocker locker (&structure->lock());

    // Check optstep info
    int optStep = structure->getCurrentOptStep();

    Q_ASSERT_X(optStep <= m_opt->optimizer()->getNumberOfOptSteps(),
               Q_FUNC_INFO, QString("OptStep of Structure %1 exceeds "
                                    "number of known OptSteps (%2, limit %3).")
               .arg(structure->getIDString()).arg(optStep)
               .arg(m_opt->optimizer()->getNumberOfOptSteps()).toStdString().c_str());

    // Unlock for optimizer calls
    locker.unlock();

    // Build hash
    QHash<QString, QString> hash;
    QStringList filenames = m_templates.keys();
    QString contents;
    for (QStringList::const_iterator
           it = filenames.constBegin(),
           it_end = filenames.constEnd();
         it != it_end;
         it++) {
      // For debugging template issues
      //qDebug() << "Templates: *it is" << *it;
      //qDebug() << "size is" << m_templates.value(*it).size();
      //qDebug() << "optStep is" << optStep - 1;
      hash.insert((*it), m_opt->interpretTemplate(m_templates.value(*it)
                                                  .at(optStep - 1),
                                                  structure));
      //qDebug() << "Interpreted template is " << hash[*it];
    }
    // QueueInterface templates
    filenames = m_QITemplates.keys();
    for (QStringList::const_iterator
           it = filenames.constBegin(),
           it_end = filenames.constEnd();
         it != it_end;
         it++) {
      // For debugging template issues
      //qDebug() << "QITemplates: *it is" << *it;
      //qDebug() << "size is" << m_QITemplates.value(*it).size();
      //qDebug() << "optStep is" << optStep - 1;
      hash.insert((*it), m_opt->interpretTemplate(m_QITemplates.value(*it)
                                                  .at(optStep - 1),
                                                  structure));
      //qDebug() << "Interpreted QITemplate is " << hash[*it];
    }

    return hash;
  }

  QDialog* Optimizer::dialog()
  {
    if (!m_dialog) {
      if (!m_opt->dialog())
        return nullptr;
      m_dialog = new OptimizerConfigDialog (m_opt->dialog(),
                                            m_opt,
                                            this);
    }
    OptimizerConfigDialog *d =
      qobject_cast<OptimizerConfigDialog*>(m_dialog);
    d->updateGUI();

    return d;
  }

  void Optimizer::updateQueueInterface()
  {
    m_QITemplates.clear();

    QStringList init;
    for (unsigned int i = 0; i < getNumberOfOptSteps(); ++i) {
      init << "";
    }

    QStringList filenames = m_opt->queueInterface()->getTemplateFileNames();
    for (QStringList::const_iterator
           it = filenames.constBegin(),
           it_end = filenames.constEnd();
         it != it_end;
         ++it) {
      m_QITemplates.insert(*it, init);
    }
  }

  bool Optimizer::checkIfOutputFileExists(Structure *s, bool *exists)
  {
    return m_opt->queueInterface()->checkIfFileExists(s,
                                                      m_completionFilename,
                                                      exists);
  }

  bool Optimizer::checkForSuccessfulOutput(Structure *s, bool *success)
  {
    int ec;
    *success = false;
    for (QStringList::const_iterator
           it = m_completionStrings.constBegin(),
           it_end = m_completionStrings.constEnd();
         it != it_end;
         ++it) {
      if (!m_opt->queueInterface()->grepFile(s,
                                             (*it),
                                             m_completionFilename,
                                             0,
                                             &ec)) {
        qDebug() << "For structure "
                 << QString::number(s->getGeneration()) + "x" +
                    QString::number(s->getIDNumber()) << ":";
        qDebug() << "The completion string, " << (*it) << ", was not found"
                 << "in the output file. Job failed.";
        return false;
      }
      if (ec == 0) {
        *success = true;
        return true;
      }
    }
    return true;
  }

  bool Optimizer::update(Structure *structure)
  {
    // lock structure
    QWriteLocker locker (&structure->lock());

    structure->stopOptTimer();

    // Copy remote files over, other prep work:
    locker.unlock();
    bool ok = m_opt->queueInterface()->prepareForStructureUpdate(structure);
    locker.relock();
    if (!ok) {
      m_opt->warning(tr("Optimizer::update: Error while preparing to update structure %1")
                     .arg(structure->getIDString()));
      return false;
    }

    // Try to read all files in outputFileNames
    ok = false;
    for (int i = 0; i < m_outputFilenames.size(); i++) {
      if (read(structure,
               structure->fileName() + "/" + m_outputFilenames.at(i))) {
        ok = true;
        break;
      }
    }
    if (!ok) {
      m_opt->warning(tr("Optimizer::Update: Error loading structure at %1")
                 .arg(structure->fileName()));
      return false;
    }

    structure->setJobID(0);
    locker.unlock();
    return true;
  }

  bool Optimizer::load(Structure *structure)
  {
    QWriteLocker locker (&structure->lock());

    // Try to read all files in outputFileNames
    bool ok = false;
    for (int i = 0; i < m_outputFilenames.size(); i++) {
      if (read(structure,
               structure->fileName() + "/" + m_outputFilenames.at(i))) {
        ok = true;
        break;
      }
    }
    if (!ok) {
      m_opt->warning(tr("Optimizer::load: Error loading structure at %1")
                 .arg(structure->fileName()));
      return false;
    }
    return true;
  }

  bool Optimizer::read(Structure *structure,
                       const QString & filename) {
    // Test filename
    QFile file (filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      return false;
    }
    file.close();

    if (!Formats::read(structure, filename, m_idString)) {
      qDebug() << "Failed to read the output file!";
      return false;
    }

    return true;
  }

  int Optimizer::getNumberOfOptSteps() const
  {
    if (m_templates.isEmpty())
      return 0;
    else
      return std::max_element(m_templates.cbegin(), m_templates.cend(),
                              [](const QStringList& lhs,
                                 const QStringList& rhs)
                              {
                                return lhs.size() < rhs.size();
                              })->size();
  }

  bool Optimizer::setTemplate(const QString &filename,
                              const QString &templateData,
                              int optStepIndex)
  {
    Q_ASSERT(m_templates.contains(filename) ||
             m_QITemplates.contains(filename));
    Q_ASSERT(optStepIndex >= 0 && optStepIndex < getNumberOfOptSteps());

    resolveTemplateHash(filename)[filename][optStepIndex] = templateData;
    return true;
  }

  bool Optimizer::setTemplate(const QString &filename,
                              const QStringList &templateData)
  {
    Q_ASSERT(m_templates.contains(filename) ||
             m_QITemplates.contains(filename));

    resolveTemplateHash(filename).insert(filename, templateData);
    return true;
  }


  QString Optimizer::getTemplate(const QString &filename,
                                 int optStepIndex)
  {
    Q_ASSERT(m_templates.contains(filename) ||
             m_QITemplates.contains(filename));
    Q_ASSERT(optStepIndex >= 0 && optStepIndex < getNumberOfOptSteps());

    return resolveTemplateHash(filename)[filename][optStepIndex];
  }

  QStringList Optimizer::getTemplate(const QString &filename)
  {
    Q_ASSERT(m_templates.contains(filename) ||
             m_QITemplates.contains(filename));

    return resolveTemplateHash(filename)[filename];
  }

  bool Optimizer::appendTemplate(const QString &filename,
                                 const QString &templateData)
  {
    Q_ASSERT(m_templates.contains(filename) ||
             m_QITemplates.contains(filename));

    resolveTemplateHash(filename)[filename].append(templateData);
    return true;
  }

  bool Optimizer::removeAllTemplatesForOptStep(int optStepIndex)
  {
    Q_ASSERT(optStepIndex >= 0 &&
             optStepIndex < getNumberOfOptSteps());

    // Remove the indicated optStep from each template
    QList<QString> templateKeys = m_templates.keys();
    QList<QString> QITemplateKeys = m_QITemplates.keys();
    for (QStringList::const_iterator
           it = templateKeys.constBegin(),
           it_end = templateKeys.constEnd();
         it != it_end; ++it) {
      if (m_templates[*it].size() > optStepIndex)
        m_templates[*it].removeAt(optStepIndex);
    }
    for (QStringList::const_iterator
           it = QITemplateKeys.constBegin(),
           it_end = QITemplateKeys.constEnd();
         it != it_end; ++it) {
      if (m_QITemplates[*it].size() > optStepIndex)
        m_QITemplates[*it].removeAt(optStepIndex);
    }

    return true;
  }

  bool Optimizer::setData(const QString &identifier, const QVariant &data)
  {
    Q_ASSERT(m_data.contains(identifier));

    m_data.insert(identifier, data);
    return true;
  }

  QVariant Optimizer::getData(const QString &identifier)
  {
    Q_ASSERT(m_data.contains(identifier));

    return m_data.value(identifier);
  }

  QHash<QString, QStringList> &
  Optimizer::resolveTemplateHash(const QString &filename)
  {
    if (m_templates.contains(filename)) {
      return m_templates;
    }
    else if (m_QITemplates.contains(filename)) {
      return m_QITemplates;
    }
    else {
      qFatal("In %s:\n\t%s '%s'\n",
             Q_FUNC_INFO,
             "No current template contains file",
             filename.toStdString().c_str());
    }
    // Shouldn't be reached, but otherwise MSVC complains:
    Q_ASSERT(false);
    return *(new QHash<QString, QStringList>());
  }

  const QHash<QString, QStringList> &
  Optimizer::resolveTemplateHash(const QString &filename) const
  {
    if (m_templates.contains(filename)) {
      return m_templates;
    }
    else if (m_QITemplates.contains(filename)) {
      return m_QITemplates;
    }
    else {
      qFatal("In %s:\n\t%s '%s'\n",
             Q_FUNC_INFO,
             "No current template contains file",
             filename.toStdString().c_str());
    }
    // Shouldn't be reached, but otherwise MSVC complains:
    Q_ASSERT(false);
    return *(new QHash<QString, QStringList>());
  }

  void Optimizer::fixTemplateLengths()
  {
    int steps = getNumberOfOptSteps();
    if (steps < 1) {
      steps = 1;
    }

    // Optimizer:
    QStringList filenames = m_templates.keys();
    for (QStringList::const_iterator
           it = filenames.constBegin(),
           it_end = filenames.constEnd();
         it != it_end;
         ++it) {
      QStringList &current = m_templates[*it];
      while (current.size() > steps) {
        current.removeLast();
      }
      while (current.size() < steps) {
        current << "";
      }
    }

    // Optimizer:
    filenames = m_QITemplates.keys();
    for (QStringList::const_iterator
           it = filenames.constBegin(),
           it_end = filenames.constEnd();
         it != it_end;
         ++it) {
      QStringList &current = m_QITemplates[*it];
      while (current.size() > steps) {
        current.removeLast();
      }
      while (current.size() < steps) {
        current << "";
      }
    }
  }

} // end namespace GlobalSearch

/**********************************************************************
  QueueInterface - Job submission interface

  Copyright (C) 2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/queueinterface.h>

#include <common/fileutils.h>
#include <search/registeredcreators.h>
#include <common/output.h>
#include <common/random.h>
#include <search/search.h>
#include <search/optimizer.h>
#include <search/structure.h>
#include <search/queueinterfaces/queueinterfaces.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QTextStream>

namespace Search {

namespace {
// Registered queues' helpers
typedef RegisteredCreators<QueueInterface, SearchBase> RegisteredQueueInterfaces;

typedef std::unique_ptr<QueueInterface> (*QueueInterfaceCreator)(SearchBase*);

template <typename Interface>
std::unique_ptr<QueueInterface> createQueueInterface(SearchBase* s)
{
  return std::unique_ptr<QueueInterface>(new Interface(s));
}

// List of queue interfaces.
struct BuiltInQueueInterfaceDefinition
{
  const char* queueInterfaceName;
  QueueInterfaceCreator queueInterfaceCreator;
};

const BuiltInQueueInterfaceDefinition builtInQueueInterfaceDefinitions[] = {
  { "none",        &createQueueInterface<DirectRunInterface>        },
  { "slurm",       &createQueueInterface<SlurmQueueInterface>       },
  { "pbs",         &createQueueInterface<PbsQueueInterface>         },
  { "sge",         &createQueueInterface<SgeQueueInterface>         },
  { "lsf",         &createQueueInterface<LsfQueueInterface>         },
  { "loadleveler", &createQueueInterface<LoadLevelerQueueInterface> }
};

QString normalizedBuiltInQueueInterfaceName(const QString& name)
{
  return name.toLower();
}

const BuiltInQueueInterfaceDefinition* builtInQueueInterfaceDefinition(const QString& name)
{
  const QString key = normalizedBuiltInQueueInterfaceName(name);
  for (const auto& definition : builtInQueueInterfaceDefinitions) {
    if (key.compare(definition.queueInterfaceName, Qt::CaseInsensitive) == 0)
      return &definition;
  }
  return nullptr;
}
} // anonymous namespace

QueueInterface::QueueInterface(SearchBase* parent, const QString& settingFile)
  : m_search(parent)
{
  Q_UNUSED(settingFile);
}

QueueInterface::~QueueInterface() = default;

bool QueueInterface::registerQueueInterface(const QString& name,
  std::function<std::unique_ptr<QueueInterface>(SearchBase*)> creator)
{
  return RegisteredQueueInterfaces::shared().registerCreator(name, creator);
}

QStringList QueueInterface::registeredQueueInterfaces()
{
  return RegisteredQueueInterfaces::shared().names();
}

QStringList QueueInterface::availableBuiltInQueueInterfaces()
{
  QStringList names;
  for (const auto& definition : builtInQueueInterfaceDefinitions)
    names.append(definition.queueInterfaceName);
  return names;
}

std::unique_ptr<QueueInterface> QueueInterface::createRegisteredQueueInterface(
  const QString& name, SearchBase* parent)
{
  const QString key = normalizedBuiltInQueueInterfaceName(name);
  std::unique_ptr<QueueInterface> queue = RegisteredQueueInterfaces::shared().create(key, parent);
  if (queue)
    return queue;
  if (parent) {
    Common::error(QString("%1: unknown interface: %2")
                   .arg(__func__)
                   .arg(name));
  }
  return std::unique_ptr<QueueInterface>();
}

std::unique_ptr<QueueInterface> SearchBase::createQueueInterface(
  const std::string& queueName)
{
  return QueueInterface::createRegisteredQueueInterface(QString::fromStdString(queueName), this);
}

bool QueueInterface::registerBuiltInQueueInterface(const QString& name)
{
  const BuiltInQueueInterfaceDefinition* definition = builtInQueueInterfaceDefinition(name);
  if (definition)
    return registerQueueInterface(definition->queueInterfaceName, definition->queueInterfaceCreator);

  Common::error(QString("%1: unknown built-in queue interface: %2")
                  .arg(__func__)
                  .arg(name));
  return false;
}

bool QueueInterface::localWorkingDirectoryReady(QString* err) const
{
  if (err)
    err->clear();

  const QString workDir = m_search->getLocWorkDir();
  if (workDir.isEmpty()) {
    if (err) {
      *err = tr("Local working directory is not set. Check your Queue " "configuration.");
    }
    return false;
  }

  QDir workingdir(workDir);
  bool writable = true;
  if (!workingdir.exists()) {
    writable = workingdir.mkpath(workDir);
  } else {
    const QString filename = Common::localPath(workDir, QString("queuetest-") +
                          QString::number(Common::getRandUInt()));
    QFile file(filename);
    writable = file.open(QFile::ReadWrite);
    file.remove();
  }

  if (!writable) {
    if (err) {
      *err = tr("Cannot write to working directory '%1'.\n\nPlease "
                "change the permissions on this directory or specify "
                "a different one in the Queue configuration.")
               .arg(workDir);
    }
    return false;
  }

  return true;
}

Optimizer* QueueInterface::getCurrentOptimizer(Structure* s) const
{
  return m_search->optimizer(s->getCurrentOptStep());
}

bool QueueInterface::safeRelativeFilename(const QString& filename)
{
  QString normalized = filename;
  normalized.replace('\\', '/');
  const QString clean = QDir::cleanPath(normalized);
  return !clean.isEmpty() && clean != "." && !QFileInfo(clean).isAbsolute() &&
         clean != ".." && !clean.startsWith("../") && !clean.contains("/../");
}

bool QueueInterface::writeHashToLocalDir(Structure* s,
                                          const QHash<QString, QString>& fileHash) const
{
  for (auto it = fileHash.constBegin(), itEnd = fileHash.constEnd(); it != itEnd; ++it) {
    if (!safeRelativeFilename(it.key())) {
      Common::error(tr("Refusing unsafe input filename %1").arg(it.key()));
      return false;
    }
    // Write with plain newlines on every platform: these files go to the
    // optimizer and, for remote runs, to the cluster, where schedulers
    // reject scripts with Windows line endings.
    QFile file(Common::localPath(s->getLocpath(), it.key()));
    if (!file.open(QIODevice::WriteOnly)) {
      Common::error(tr("Cannot write input file %1 (file writing failure)",
                        "1 is a file path")
                     .arg(file.fileName()));
      return false;
    }

    QTextStream stream(&file);
    stream << it.value();
    stream.flush();
    if (stream.status() != QTextStream::Ok || !file.flush() || file.error() != QFileDevice::NoError) {
      Common::error(tr("Cannot write input file %1 (file writing failure)",
                       "1 is a file path").arg(file.fileName()));
      return false;
    }
  }

  return true;
}

bool QueueInterface::copyFileFromExecutionHost(const QString& rem_file, const QString& loc_file)
{
  Q_UNUSED(rem_file);
  Q_UNUSED(loc_file);
  return true;
}

bool QueueInterface::copyFileToExecutionHost(const QString& loc_file, const QString& rem_file)
{
  Q_UNUSED(loc_file);
  Q_UNUSED(rem_file);
  return true;
}

bool QueueInterface::removeAFile(Structure* s, const QString& filename)
{
  if (!safeRelativeFilename(filename))
    return false;
  return QFile::remove(Common::localPath(s->getLocpath(), filename));
}

bool QueueInterface::checkIfFileExists(Structure* s, const QString& filename,
                                        bool* exists)
{
  if (!exists || !safeRelativeFilename(filename))
    return false;
  *exists = QFile::exists(Common::localPath(s->getLocpath(), filename));
  return true;
}

bool QueueInterface::checkIfFilesExist(Structure* s, const QStringList& filenames,
                                        QHash<QString, bool>* exists)
{
  if (!exists)
    return false;

  exists->clear();
  for (const auto& filename : filenames) {
    bool fileExists = false;
    if (!checkIfFileExists(s, filename, &fileExists))
      return false;
    exists->insert(filename, fileExists);
  }
  return true;
}

bool QueueInterface::fetchFile(Structure* s, const QString& filename, QString* contents) const
{
  if (!contents || !safeRelativeFilename(filename))
    return false;

  return Common::readFileToQString(Common::localPath(s->getLocpath(), filename), contents);
}

bool QueueInterface::grepFile(Structure* s, const QString& matchText,
                               const QString& filename, QStringList* matches,
                               int* exitcode,
                               const bool caseSensitive) const
{
  if (exitcode)
    *exitcode = 1;
  if (!safeRelativeFilename(filename))
    return false;

  QFile file(Common::localPath(s->getLocpath(), filename));
  if (!file.open(QFile::ReadOnly | QFile::Text))
    return false;

  const Qt::CaseSensitivity sensitivity = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
  QTextStream in(&file);
  QString line;
  do {
    line = in.readLine();
    if (line.contains(matchText, sensitivity)) {
      if (exitcode)
        *exitcode = 0;
      if (matches)
        *matches << line;
    }
  } while (!line.isNull());

  return true;
}

bool QueueInterface::writeCopyFilesToLocalDir(Structure* s, QStringList& extraFilenames) const
{
  if (s->copyFiles().empty())
    return true;

  for (const auto& copyFile : s->copyFiles()) {
    QFile infile(copyFile.c_str());
    QString filename = QFileInfo(infile).fileName();
    QFile outfile(Common::localPath(s->getLocpath(), filename));
    const QFileInfo inputInfo(infile);
    const QFileInfo outputInfo(outfile);

    if (!inputInfo.exists()) {
      Common::error(tr("Cannot copy file %1: it was not found").arg(infile.fileName()));
      return false;
    }

    if (extraFilenames.contains(filename, Qt::CaseInsensitive)) {
      Common::error(tr("Cannot copy file %1: the name %2 is already used by another input file.")
                      .arg(infile.fileName()).arg(filename));
      return false;
    }

    // A %copyFile% entry can already point into the structure's own directory;
    //   there is nothing to copy then, and copying would remove the file.
    if (inputInfo.canonicalFilePath() == outputInfo.canonicalFilePath()) {
      extraFilenames.append(filename);
      continue;
    }

    // The same file is copied again whenever the structure is written a second
    //   time (a restart, a later optimization step, or a resumed run). Copying
    //   does not overwrite, so remove an earlier copy first.
    if (outfile.exists())
      outfile.remove();

    if (!infile.copy(outfile.fileName())) {
      Common::error(tr("Failed to copy file %1 to %2")
                     .arg(infile.fileName())
                     .arg(outfile.fileName()));
      return false;
    }
    extraFilenames.append(filename);
  }
  s->clearCopyFiles();
  return true;
}

bool QueueInterface::writeInputFiles(Structure* s) const
{
  Optimizer* optimizer = m_search->optimizer(s->getCurrentOptStep());
  if (!optimizer) {
    Common::error(tr("No optimizer is available for structure %1 at opt step %2.")
                    .arg(s->getTag()).arg(s->getCurrentOptStep() + 1));
    return false;
  }

  // Check if the input files were found.
  const QHash<QString, QString> files = optimizer->getInputFiles(s);
  if (files.isEmpty()) {
    Common::error(tr("No input files to write for structure %1 in opt step %2;"
                    " check the optimizer templates and input assets.")
                    .arg(s->getTag())
                    .arg(s->getCurrentOptStep() + 1));
    return false;
  }
  return writeFiles(s, files);
}

} // end namespace Search

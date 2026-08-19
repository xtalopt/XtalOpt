/**********************************************************************
  Optimizer - Local optimizers' interface

  Copyright (C) 2010-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/optimizer.h>

#include <common/fileutils.h>
#include <search/registeredcreators.h>
#include <common/output.h>
#include <search/search.h>
#include <search/queueinterface.h>
#include <search/structure.h>
#include <search/optimizers/optimizers.h>

#include <QFile>
#include <QIODevice>
#include <QReadLocker>
#include <QString>

namespace Search {


namespace {
// Registered optimizers' helpers
typedef RegisteredCreators<Optimizer, SearchBase> RegisteredOptimizers;

typedef std::unique_ptr<Optimizer> (*OptimizerCreator)(SearchBase*);

template <typename OptimizerType>
std::unique_ptr<Optimizer> createOptimizer(SearchBase* s)
{
  return std::unique_ptr<Optimizer>(new OptimizerType(s));
}

// This table just maps each optimzier name to its creator function. The
//   per-optimzier strings (templates, assets, completion, outputs, commands)
//   live in each optimizer class's defaults() (see e.g.
//   optimizers/vaspoptimizer.cpp); each optimizer sets m_optimizerDefaults from its own
//   defaults() in its ctor.
struct BuiltInOptimizerDefinition
{
  const char* optimizerName;
  OptimizerCreator optimizerCreator;
};

const BuiltInOptimizerDefinition builtInOptimizerDefinitions[] = {
  { "vasp",   &createOptimizer<VASPOptimizer>   },
  { "pwscf",  &createOptimizer<PWSCFOptimizer>  },
  { "castep", &createOptimizer<CASTEPOptimizer> },
  { "siesta", &createOptimizer<SIESTAOptimizer> },
  { "gulp",   &createOptimizer<GULPOptimizer>   },
  { "mtp",    &createOptimizer<MTPOptimizer>    }
};

const BuiltInOptimizerDefinition* builtInOptimizerDefinition(const QString& name)
{
  for (const auto& definition : builtInOptimizerDefinitions) {
    if (name.compare(definition.optimizerName, Qt::CaseInsensitive) == 0)
      return &definition;
  }
  return nullptr;
}

// Build a QStringList from a null-terminated array of C strings.
QStringList defaultStringList(const char* const* values)
{
  QStringList list;
  if (values)
    for (const char* const* value = values; *value; ++value)
      list.append(QString::fromLatin1(*value));
  return list;
}

} // anonymous namespace

bool Optimizer::readInputAssetFile(const QString& filename, QString& contents)
{
  if (!Common::readFileToQString(filename, &contents)) {
    Common::error(QString("%1: could not open %2")
                    .arg(__func__)
                    .arg(filename));
    return false;
  }
  if (contents.endsWith('\n'))
    contents.chop(1);
  return true;
}

bool Optimizer::registerOptimizer(const QString& name,
  std::function<std::unique_ptr<Optimizer>(SearchBase*)> creator)
{
  return RegisteredOptimizers::shared().registerCreator(name, creator);
}

QStringList Optimizer::registeredOptimizers()
{
  return RegisteredOptimizers::shared().names();
}

QStringList Optimizer::availableBuiltInOptimizers()
{
  QStringList names;
  for (const auto& definition : builtInOptimizerDefinitions)
    names.append(definition.optimizerName);
  return names;
}

std::unique_ptr<Optimizer> Optimizer::createRegisteredOptimizer(
  const QString& name, SearchBase* parent)
{
  std::unique_ptr<Optimizer> optimizer = RegisteredOptimizers::shared().create(name, parent);
  if (optimizer)
    return optimizer;
  if (parent) {
    Common::error(QString("%1: unknown optimizer: %2")
                   .arg(__func__)
                   .arg(name));
  }
  return std::unique_ptr<Optimizer>();
}

std::unique_ptr<Optimizer> SearchBase::createOptimizer(const std::string& optName)
{
  return Optimizer::createRegisteredOptimizer(QString::fromStdString(optName), this);
}

bool Optimizer::registerBuiltInOptimizer(const QString& name)
{
  const BuiltInOptimizerDefinition* definition = builtInOptimizerDefinition(name);
  if (definition)
    return registerOptimizer(definition->optimizerName, definition->optimizerCreator);

  Common::error(QString("%1: unknown built-in optimizer: %2")
                  .arg(__func__)
                  .arg(name));
  return false;
}

Optimizer::Optimizer(SearchBase* parent)
  : m_search(parent)
{
}

Optimizer::~Optimizer()
{
}

QHash<QString, QString> Optimizer::getInputFiles(Structure* s)
{
  QWriteLocker structureLocker(&s->lock());
  const int optStep = s->getCurrentOptStep();

  QueueInterface* queue = m_search->queueInterface(optStep);
  if (!queue)
    return QHash<QString, QString>();

  // Build hash
  QHash<QString, QString> hash;
  const QStringList queueFilenames = queue->getQueueInterfaceTemplateFileNames();
  for (const auto& filename : queueFilenames) {
    std::string temp = m_search->getQueueInterfaceTemplate(optStep, filename.toStdString());
    hash.insert(filename, m_search->interpretTemplate(temp.c_str(), s));
  }

  const QStringList optimizerFilenames = getOptimizerTemplateFileNames();
  for (const auto& filename : optimizerFilenames) {
    std::string temp = m_search->getOptimizerTemplate(optStep, filename.toStdString());
    hash.insert(filename, m_search->interpretTemplate(temp.c_str(), s));
  }

  // Optimizer-specific inputs (e.g. VASP POSCAR/POTCAR, SIESTA per-species PSF).
  if (!addOptimizerInputFiles(s, optStep, hash))
    return QHash<QString, QString>();

  return hash;
}

bool Optimizer::addOptimizerInputFiles(Structure*, int, QHash<QString, QString>&) const
{
  // The base optimizer has no optimizer-specific input files.
  return true;
}

QStringList Optimizer::getOptimizerTemplateFileNames() const
{
  return defaultStringList(m_optimizerDefaults ? m_optimizerDefaults->templateFiles : nullptr);
}

QStringList Optimizer::getOptimizerInputAssetNames() const
{
  return defaultStringList(m_optimizerDefaults ? m_optimizerDefaults->inputAssets : nullptr);
}

bool Optimizer::checkIfOutputFileExists(Structure* s, bool* exists)
{
  *exists = false;
  int optStep;
  {
    QReadLocker locker(&s->lock());
    optStep = s->getCurrentOptStep();
  }
  QueueInterface* queue = m_search->queueInterface(optStep);
  if (!queue)
    return false;

  const QString completionFilename =
    m_optimizerDefaults ? QString::fromLatin1(m_optimizerDefaults->completionFilename) : QString();
  return queue->checkIfFileExists(s, completionFilename, exists);
}

bool Optimizer::checkForSuccessfulOutput(Structure* s, bool* success)
{
  *success = false;
  if (!m_optimizerDefaults)
    return true;

  int optStep;
  {
    QReadLocker locker(&s->lock());
    optStep = s->getCurrentOptStep();
  }
  QueueInterface* queue = m_search->queueInterface(optStep);
  if (!queue)
    return false;

  const QString completionFilename = QString::fromLatin1(m_optimizerDefaults->completionFilename);
  const QString completionString = QString::fromLatin1(m_optimizerDefaults->completionString);
  int ec;
  if (!queue->grepFile(s, completionString, completionFilename, 0, &ec)) {
    Common::debug(
      QString("Could not check the completion string in the output (%1)"
              " of opt step %2")
        .arg(s->getTag())
        .arg(optStep + 1));
    return false;
  }
  if (ec == 0)
    *success = true;
  return true;
}

bool Optimizer::update(Structure* structure)
{
  // lock structure
  QWriteLocker locker(&structure->lock());

  structure->stopOptTimer();
  const int optStep = structure->getCurrentOptStep();
  QueueInterface* queue = m_search->queueInterface(optStep);
  if (!queue)
    return false;

  // Copy remote files over, other prep work:
  locker.unlock();
  bool ok = queue->prepareForStructureUpdate(structure);
  locker.relock();
  if (structure->getStatus() != Structure::Updating)
    return false;
  if (!ok) {
    Common::warning(tr("%1: could not prepare to update structure %2"
                       " of opt step %3")
                     .arg(__func__)
                     .arg(structure->getTag())
                     .arg(optStep + 1));
    return false;
  }

  // Try to read each output file in order until one succeeds.
  ok = false;
  const QStringList outputFilenames = defaultStringList(m_optimizerDefaults ? m_optimizerDefaults->outputFiles : nullptr);
  for (const auto& outputFilename : outputFilenames) {
    if (read(structure, Common::localPath(structure->getLocpath(), outputFilename))) {
      ok = true;
      break;
    }
  }
  if (!ok) {
    Common::warning(tr("%1: could not load structure at %2")
                     .arg(__func__)
                     .arg(structure->getLocpath()));
    return false;
  }

  structure->setJobID(0);
  locker.unlock();
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

  if (!readOutput(structure, filename)) {
    Common::debug("Failed to read the output file " + filename + " for " + structure->getTag());
    return false;
  }

  return true;
}

bool Optimizer::readOutput(Structure*, const QString&) const
{
  // The base optimizer has no output reader; subclasses parse their own format.
  return false;
}

} // end namespace Search

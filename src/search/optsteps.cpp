/**********************************************************************
  OptSteps - The per-optimization-step support chain.

  Copyright (C) 2010-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/optsteps.h>

#include <search/optimizer.h>
#include <search/queueinterface.h>

#include <atoms/eleminfo.h>
#include <common/output.h>

#include <QString>

namespace Search {

OptSteps::OptSteps(QueueInterfaceCreatorByName createQueueInterfaceByName,
                   OptimizerCreatorByName createOptimizerByName)
  : m_queueInterfaceCreatorByName(createQueueInterfaceByName),
    m_optimizerCreatorByName(createOptimizerByName),
    m_numSteps(0)
{
}

OptSteps::~OptSteps() = default;

QueueInterface* OptSteps::queueInterface(int optStep) const
{
  if (optStep < 0 || optStep >= static_cast<int>(numSteps())) {
    Common::error(QString("%1: optStep %2 is out of bounds. The number of "
                  "optimization steps is %3.")
            .arg(__func__)
            .arg(optStep + 1)
            .arg(numSteps()));
    return nullptr;
  }
  return m_queueInterfaceAtOptStep[optStep].get();
}

int OptSteps::indexOf(const QueueInterface* qi) const
{
  for (size_t i = 0; i < m_queueInterfaceAtOptStep.size(); ++i) {
    if (qi == m_queueInterfaceAtOptStep[i].get())
      return i;
  }
  return -1;
}

Optimizer* OptSteps::optimizer(int optStep) const
{
  if (optStep < 0 || optStep >= static_cast<int>(numSteps())) {
    Common::error(QString("%1: optStep %2 is out of bounds. The number of "
                  "optimization steps is %3.")
            .arg(__func__)
            .arg(optStep + 1)
            .arg(numSteps()));
    return nullptr;
  }
  return m_optimizerAtOptStep[optStep].get();
}

void OptSteps::clear()
{
  m_queueInterfaceAtOptStep.clear();
  m_optimizerAtOptStep.clear();
  m_queueInterfaceTemplates.clear();
  m_optimizerTemplates.clear();
  m_optimizerInputAssets.clear();
  m_numSteps = 0;
}

void OptSteps::append()
{
  // If there are no opt steps, we can't copy previous ones
  if (m_numSteps == 0) {
    using templateMap = std::map<std::string, std::string>;
    m_queueInterfaceAtOptStep.push_back(nullptr);
    m_optimizerAtOptStep.push_back(nullptr);
    m_queueInterfaceTemplates.push_back(templateMap());
    m_optimizerTemplates.push_back(templateMap());
    m_optimizerInputAssets.push_back(PerStepInputAssetMap::value_type());
  }
  // We will duplicate the most recent opt step otherwise
  else {
    QueueInterface* previousQueue = m_queueInterfaceAtOptStep.back().get();

    std::unique_ptr<QueueInterface> queueInterface = previousQueue
      ? m_queueInterfaceCreatorByName(previousQueue->getIDString().toStdString()) : nullptr;

    if (queueInterface) {
      queueInterface->setSubmitCommand(previousQueue->submitCommand());
      queueInterface->setStatusCommand(previousQueue->statusCommand());
      queueInterface->setCancelCommand(previousQueue->cancelCommand());
    }

    m_queueInterfaceAtOptStep.push_back(std::move(queueInterface));

    std::unique_ptr<Optimizer> optimizer = m_optimizerAtOptStep.back() ? m_optimizerCreatorByName(
                                           m_optimizerAtOptStep.back()->getIDString().toStdString()) : nullptr;

    if (optimizer)
      optimizer->setDirectRunCommand(m_optimizerAtOptStep.back()->getDirectRunCommand());
    m_optimizerAtOptStep.push_back(std::move(optimizer));
    m_queueInterfaceTemplates.push_back(m_queueInterfaceTemplates.back());
    m_optimizerTemplates.push_back(m_optimizerTemplates.back());
    m_optimizerInputAssets.push_back(m_optimizerInputAssets.back());
  }

  ++m_numSteps;
}

void OptSteps::remove(size_t optStep)
{
  if (optStep >= m_numSteps) {
    Common::error(QString("%1: attempting to remove opt step %2, which is "
                  "out of bounds. The number of opt steps is %3.")
            .arg(__func__)
            .arg(optStep + 1)
            .arg(m_numSteps));
    return;
  }

  m_queueInterfaceAtOptStep.erase(m_queueInterfaceAtOptStep.begin() + optStep);
  m_optimizerAtOptStep.erase(m_optimizerAtOptStep.begin() + optStep);

  m_queueInterfaceTemplates.erase(m_queueInterfaceTemplates.begin() + optStep);
  m_optimizerTemplates.erase(m_optimizerTemplates.begin() + optStep);
  m_optimizerInputAssets.erase(m_optimizerInputAssets.begin() + optStep);

  --m_numSteps;
}

bool OptSteps::setQueueInterface(size_t optStep, const std::string& qiName)
{
  if (optStep >= m_numSteps) {
    Common::error(QString("%1: optStep %2 is out of bounds. Number of opt "
                  "steps is %3.")
            .arg(__func__)
            .arg(optStep + 1)
            .arg(m_numSteps));
    return false;
  }
  std::unique_ptr<QueueInterface> queueInterface = m_queueInterfaceCreatorByName(qiName);
  if (!queueInterface)
    return false;

  m_queueInterfaceAtOptStep[optStep] = std::move(queueInterface);
  m_queueInterfaceTemplates[optStep].clear();

  // We need to populate the templates list with empty templates
  for (const auto& templateName :
       m_queueInterfaceAtOptStep[optStep]->getQueueInterfaceTemplateFileNames()) {
    setQueueInterfaceTemplate(optStep, templateName.toStdString(), "");
  }
  return true;
}

std::string OptSteps::queueInterfaceTemplate(size_t optStep, const std::string& name) const
{
  if (optStep >= m_numSteps) {
    Common::error(QString("%1: optStep %2 is out of bounds. Number of opt "
                  "steps is %3.")
            .arg(__func__)
            .arg(optStep + 1)
            .arg(m_numSteps));
    return "";
  }
  if (m_queueInterfaceTemplates[optStep].count(name) == 0) {
    Common::error(QString("%1: invalid key entry name %2 for opt step %3.")
            .arg(__func__)
            .arg(name.c_str())
            .arg(optStep + 1));
    return "";
  }
  return m_queueInterfaceTemplates[optStep].at(name);
}

void OptSteps::setQueueInterfaceTemplate(size_t optStep, const std::string& name, const std::string& temp)
{
  if (optStep >= m_numSteps) {
    Common::error(QString("%1: optStep %2 is out of bounds. Number of opt "
                  "steps is %3.")
            .arg(__func__)
            .arg(optStep + 1)
            .arg(m_numSteps));
    return;
  }
  m_queueInterfaceTemplates[optStep][name] = temp;
}

bool OptSteps::setOptimizer(size_t optStep, const std::string& optName)
{
  if (optStep >= m_numSteps) {
    Common::error(QString("%1: optStep %2 is out of bounds. Number of opt "
                  "steps is %3.")
            .arg(__func__)
            .arg(optStep + 1)
            .arg(m_numSteps));
    return false;
  }
  std::unique_ptr<Optimizer> optimizer = m_optimizerCreatorByName(optName);
  if (!optimizer)
    return false;

  m_optimizerAtOptStep[optStep] = std::move(optimizer);
  m_optimizerTemplates[optStep].clear();
  m_optimizerInputAssets[optStep].clear();

  // We need to populate the templates list with empty templates
  for (const auto& templateName : m_optimizerAtOptStep[optStep]->getOptimizerTemplateFileNames()) {
    setOptimizerTemplate(optStep, templateName.toStdString(), "");
  }
  for (const auto& assetName : m_optimizerAtOptStep[optStep]->getOptimizerInputAssetNames()) {
    setOptimizerInputAssets(optStep, assetName.toStdString(), OptimizerInputAssetMap());
  }
  return true;
}

std::string OptSteps::optimizerTemplate(size_t optStep, const std::string& name) const
{
  if (optStep >= m_numSteps) {
    Common::error(QString("%1: optStep %2 is out of bounds. Number of opt "
                  "steps is %3.")
            .arg(__func__)
            .arg(optStep + 1)
            .arg(m_numSteps));
    return "";
  }
  if (m_optimizerTemplates[optStep].count(name) == 0) {
    Common::error(QString("%1: invalid key entry name %2 for opt step %3.")
            .arg(__func__)
            .arg(name.c_str())
            .arg(optStep + 1));
    return "";
  }
  return m_optimizerTemplates[optStep].at(name);
}

void OptSteps::setOptimizerTemplate(size_t optStep,
                                        const std::string& name,
                                        const std::string& temp)
{
  if (optStep >= m_numSteps) {
    Common::error(QString("%1: optStep %2 is out of bounds. Number of opt "
                  "steps is %3.")
            .arg(__func__)
            .arg(optStep + 1)
            .arg(m_numSteps));
    return;
  }
  m_optimizerTemplates[optStep][name] = temp;
}

OptimizerInputAssetMap OptSteps::optimizerInputAssets(size_t optStep,
                                                      const std::string& name) const
{
  if (optStep >= m_numSteps) {
    Common::error(QString("%1: optStep %2 is out of bounds. Number of opt "
                  "steps is %3.")
            .arg(__func__)
            .arg(optStep + 1)
            .arg(m_numSteps));
    return OptimizerInputAssetMap();
  }
  if (m_optimizerInputAssets[optStep].count(name) == 0) {
    Common::error(QString("%1: invalid input asset name %2 for opt step %3.")
            .arg(__func__)
            .arg(name.c_str())
            .arg(optStep + 1));
    return OptimizerInputAssetMap();
  }
  return m_optimizerInputAssets[optStep].at(name);
}

void OptSteps::setOptimizerInputAssets(size_t optStep, const std::string& name,
                                       const OptimizerInputAssetMap& assets)
{
  if (optStep >= m_numSteps) {
    Common::error(QString("%1: optStep %2 is out of bounds. Number of opt "
                  "steps is %3.")
            .arg(__func__)
            .arg(optStep + 1)
            .arg(m_numSteps));
    return;
  }
  OptimizerInputAssetMap canonicalAssets;
  for (const auto& asset : assets) {
    std::string id = asset.first;
    if (QString::compare(QString::fromStdString(id), "system", Qt::CaseInsensitive) == 0) {
      id = "system";
    } else {
      const unsigned int atomicNum = Atoms::ElementInfo::getAtomicNum(id);
      if (atomicNum != 0)
        id = Atoms::ElementInfo::getAtomicSymbol(atomicNum);
    }
    canonicalAssets[id] = asset.second;
  }
  m_optimizerInputAssets[optStep][name] = canonicalAssets;
}

} // namespace Search

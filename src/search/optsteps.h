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

#ifndef SEARCH_OPTSTEPS_H
#define SEARCH_OPTSTEPS_H

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Search {
class Optimizer;
class QueueInterface;

using OptimizerInputAssetMap = std::map<std::string, std::string>;

// Queue and optimizer settings for one optimization step.
class OptSteps
{
public:
  using QueueInterfaceCreatorByName = std::function<std::unique_ptr<QueueInterface>(const std::string&)>;
  using OptimizerCreatorByName = std::function<std::unique_ptr<Optimizer>(const std::string&)>;

  OptSteps(QueueInterfaceCreatorByName createQueueInterfaceByName,
           OptimizerCreatorByName createOptimizerByName);
  ~OptSteps();

  size_t numSteps() const { return m_numSteps; }

  /** Remove all steps. */
  void clear();

  /** Add one step. */
  void append();

  /** Remove one step. */
  void remove(size_t optStep);

  /** @return The queue interface at @p optStep, or nullptr. */
  QueueInterface* queueInterface(int optStep) const;

  /** @return The step index of @p qi, or -1 when not in the chain. */
  int indexOf(const QueueInterface* qi) const;

  /** @return The optimizer at @p optStep, or nullptr. */
  Optimizer* optimizer(int optStep) const;

  /**
   * Update the QueueInterface to the one indicated
   *
   * @param optStep The optimization step for which to set the QI.
   * @param qiName The name of the queue interface to use.
   *
   * @return True if successful, false if @p optStep or @p qiName is invalid.
   * @sa queueInterface
   */
  bool setQueueInterface(size_t optStep, const std::string& qiName);

  /**
   * Update the Optimizer to the one indicated
   *
   * @param optStep The opt step for which to set the optimizer
   * @param optName New Optimizer to use.
   *
   * @return True if successful, false if @p optStep or @p optName is invalid.
   * @sa optimizer
   */
  bool setOptimizer(size_t optStep, const std::string& optName);

  /**
   * Get a queue interface template for a particular opt step and a
   * particular file name.
   *
   * @param optStep The optimization step for which to get the template.
   * @param name The name of the file for which to get the template.
   *
   * @return The queue interface template. Returns an empty string if
   *         @p optStep or @p name are invalid.
   */
  std::string queueInterfaceTemplate(size_t optStep, const std::string& name) const;
  /**
   * Set the queue interface template for a particular opt step and
   * file name.
   *
   * @param optStep The optimization step for which to set the template.
   * @param name The file name for which to set the template.
   * @param temp The template to be set.
   */
  void setQueueInterfaceTemplate(size_t optStep, const std::string& name,
                                 const std::string& temp);

  /**
   * Get an optimizer template for a particular opt step and a
   * particular file name.
   *
   * @param optStep The optimization step for which to get the template.
   * @param name The name of the file for which to get the template.
   *
   * @return The optimization template. Returns an empty string if
   *         @p optStep or @p name are invalid.
   */
  std::string optimizerTemplate(size_t optStep, const std::string& name) const;
  /**
   * Set the optimizer template for a particular opt step and
   * file name.
   *
   * @param optStep The optimization step for which to set the template.
   * @param name The file name for which to set the template.
   * @param temp The template to be set.
   */
  void setOptimizerTemplate(size_t optStep, const std::string& name,
                            const std::string& temp);

  OptimizerInputAssetMap optimizerInputAssets(size_t optStep, const std::string& name) const;
  void setOptimizerInputAssets(size_t optStep, const std::string& name,
                               const OptimizerInputAssetMap& assets);

private:
  using PerStepFileTextMap = std::vector<std::map<std::string, std::string>>;
  using PerStepInputAssetMap =
    std::vector<std::map<std::string, OptimizerInputAssetMap>>;

  QueueInterfaceCreatorByName m_queueInterfaceCreatorByName;
  OptimizerCreatorByName m_optimizerCreatorByName;

  size_t m_numSteps;

  /// The queue interface for each optimization step.
  std::vector<std::unique_ptr<QueueInterface>> m_queueInterfaceAtOptStep;

  /// The optimizer for each optimization step.
  std::vector<std::unique_ptr<Optimizer>> m_optimizerAtOptStep;

  /// Queue templates.
  PerStepFileTextMap m_queueInterfaceTemplates;

  /// Optimizer templates.
  PerStepFileTextMap m_optimizerTemplates;

  /// Optimizer input files.
  PerStepInputAssetMap m_optimizerInputAssets;
};

} // namespace Search

#endif // SEARCH_OPTSTEPS_H

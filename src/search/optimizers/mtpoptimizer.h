/**********************************************************************
  MTPOptimizer - Optimizer interface for MTP (machine-learned potentials).

  Copyright (C) 2010-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef MTPOPTIMIZER_H
#define MTPOPTIMIZER_H

#include <search/optimizer.h>

namespace Search {

/**
 * @class MTPOptimizer mtpoptimizer.h <search/optimizers/mtpoptimizer.h>
 *
 * @brief Optimizer interface for MTP (machine-learned potentials).
 */
class MTPOptimizer : public Optimizer
{
  Q_OBJECT

public:
  // Default values: id, templates, assets, completion, outputs, command.
  static const OptimizerDefaults& defaults();

  explicit MTPOptimizer(SearchBase* parent);
  virtual ~MTPOptimizer() override;

protected:
  bool readOutput(Structure* s, const QString& filename) const override;
};
} // namespace Search

#endif // MTPOPTIMIZER_H

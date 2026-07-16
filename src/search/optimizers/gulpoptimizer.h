/**********************************************************************
  GULPOptimizer - Optimizer interface for GULP.

  Copyright (C) 2010-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef GULPOPTIMIZER_H
#define GULPOPTIMIZER_H

#include <search/optimizer.h>

namespace Search {

/**
 * @class GULPOptimizer gulpoptimizer.h <search/optimizers/gulpoptimizer.h>
 *
 * @brief Optimizer interface for GULP.
 */
class GULPOptimizer : public Optimizer
{
  Q_OBJECT

public:
  // Default values: id, templates, assets, completion, outputs, command.
  static const OptimizerDefaults& defaults();

  explicit GULPOptimizer(SearchBase* parent);
  virtual ~GULPOptimizer() override;

protected:
  bool readOutput(Structure* s, const QString& filename) const override;
};
} // namespace Search

#endif // GULPOPTIMIZER_H

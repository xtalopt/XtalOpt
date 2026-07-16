/**********************************************************************
  SIESTAOptimizer - Optimizer interface for SIESTA.

  Copyright (C) 2010-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef SIESTAOPTIMIZER_H
#define SIESTAOPTIMIZER_H

#include <search/optimizer.h>

namespace Search {

/**
 * @class SIESTAOptimizer siestaoptimizer.h <search/optimizers/siestaoptimizer.h>
 *
 * @brief Optimizer interface for SIESTA.
 */
class SIESTAOptimizer : public Optimizer
{
  Q_OBJECT

public:
  // Default values: id, templates, assets, completion, outputs, command.
  static const OptimizerDefaults& defaults();

  explicit SIESTAOptimizer(SearchBase* parent);
  virtual ~SIESTAOptimizer() override;

protected:
  // SIESTA writes one per-species PSF file from the input assets.
  bool addOptimizerInputFiles(Structure* s, int optStep,
                              QHash<QString, QString>& files) const override;

  bool readOutput(Structure* s, const QString& filename) const override;
};
} // namespace Search

#endif // SIESTAOPTIMIZER_H

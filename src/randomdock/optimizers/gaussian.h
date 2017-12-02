/**********************************************************************
  GuassianOptimizer - Tools to interface with Gaussian

  Copyright (C) 2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef RD_GAUSSIANOPTIMIZER_H
#define RD_GAUSSIANOPTIMIZER_H

#include <globalsearch/optimizer.h>

namespace RandomDock {

class GaussianOptimizer : public GlobalSearch::Optimizer
{
  Q_OBJECT

public:
  explicit GaussianOptimizer(GlobalSearch::OptBase* parent,
                             const QString& filename = "");
};

} // end namespace RandomDock

#endif

/**********************************************************************
  GULPOptimizer - Tools to interface with GULP

  Copyright (C) 2009-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef GULPOPTIMIZER_H
#define GULPOPTIMIZER_H

#include <xtalopt/optimizers/xtaloptoptimizer.h>

#include <QObject>

namespace GlobalSearch {
class Structure;
class OptBase;
class Optimizer;
}

namespace XtalOpt {
class GULPOptimizer : public XtalOptOptimizer
{
  Q_OBJECT

public:
  GULPOptimizer(GlobalSearch::OptBase* parent, const QString& filename = "");
};

} // end namespace XtalOpt

#endif

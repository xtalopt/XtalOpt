/**********************************************************************
  XtalOptOptimizer - Generic optimizer interface

  Copyright (C) 2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef XTALOPTOPTIMIZER_H
#define XTALOPTOPTIMIZER_H

#include <globalsearch/optimizer.h>

#include <QObject>

namespace GlobalSearch {
class Structure;
class OptBase;
class Optimizer;
}

namespace XtalOpt {

class XtalOptOptimizer : public GlobalSearch::Optimizer
{
  Q_OBJECT

public:
  explicit XtalOptOptimizer(GlobalSearch::OptBase* parent,
                            const QString& filename = "");
  virtual ~XtalOptOptimizer() override;

  virtual bool read(GlobalSearch::Structure* structure,
                    const QString& filename) override;
};

} // end namespace XtalOpt

#endif

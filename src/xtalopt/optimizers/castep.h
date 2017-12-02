/**********************************************************************
  CASTEPOptimizer - Tools to interface with CASTEP

  Copyright (C) 2009-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef CASTEPOPTIMIZER_H
#define CASTEPOPTIMIZER_H

#include <xtalopt/optimizers/xtaloptoptimizer.h>

#include <QObject>

namespace GlobalSearch {
class OptBase;
}

namespace XtalOpt {
class CASTEPOptimizer : public XtalOptOptimizer
{
  Q_OBJECT

public:
  CASTEPOptimizer(GlobalSearch::OptBase* parent, const QString& filename = "");
};

} // end namespace XtalOpt

#endif

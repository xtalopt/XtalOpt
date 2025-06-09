/**********************************************************************
  MTPOptimizer - Tools to interface with MTP

  Copyright (C) 2025 by Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef MTPOPTIMIZER_H
#define MTPOPTIMIZER_H

#include <xtalopt/optimizers/xtaloptoptimizer.h>

#include <QObject>

namespace GlobalSearch {
class Structure;
class SearchBase;
class Optimizer;
}

namespace XtalOpt {
class MTPOptimizer : public XtalOptOptimizer
{
  Q_OBJECT

public:
  MTPOptimizer(GlobalSearch::SearchBase* parent, const QString& filename = "");
};

} // end namespace XtalOpt

#endif // MTPOPTIMIZER_H

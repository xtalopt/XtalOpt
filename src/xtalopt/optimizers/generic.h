/**********************************************************************
  GenericOptimizer - Tools for a generic optimizer

  Copyright (C) 2018 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef GENERICOPTIMIZER_H
#define GENERICOPTIMIZER_H

#include <xtalopt/optimizers/xtaloptoptimizer.h>

#include <QObject>

namespace GlobalSearch {
class Structure;
class OptBase;
class Optimizer;
}

namespace XtalOpt {
class GenericOptimizer : public XtalOptOptimizer
{
  Q_OBJECT

public:
  GenericOptimizer(GlobalSearch::OptBase* parent, const QString& filename = "");

  // This will always return true for the generic optimizer
  bool checkForSuccessfulOutput(GlobalSearch::Structure* s,
                                bool* success) override
  {
    *success = true;
    return true;
  };

  // Override the read() function so we can produce an error in the case that
  // some parts were not set.
  bool read(GlobalSearch::Structure* structure,
            const QString& filename) override;
};

} // end namespace XtalOpt

#endif

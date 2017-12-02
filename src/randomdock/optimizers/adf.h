/**********************************************************************
  ADFOptimizer - Tools to interface with ADF remotely

  Copyright (C) 2010-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef RDADF_H
#define RDADF_H

#include <globalsearch/optimizer.h>

namespace GlobalSearch {
class Structure;
}

namespace RandomDock {

class ADFOptimizer : public GlobalSearch::Optimizer
{
  Q_OBJECT

public:
  explicit ADFOptimizer(GlobalSearch::OptBase* parent,
                        const QString& filename = "");

  bool checkForSuccessfulOutput(GlobalSearch::Structure* s, bool* success);
};

} // end namespace RandomDock

#endif

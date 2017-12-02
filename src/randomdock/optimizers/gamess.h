/**********************************************************************
  RandomDockGAMESS - Tools to interface with GAMESS remotely

  Copyright (C) 2009-2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef RDGAMESS_H
#define RDGAMESS_H

#include <globalsearch/optimizer.h>

namespace RandomDock {

class GAMESSOptimizer : public GlobalSearch::Optimizer
{
  Q_OBJECT

public:
  explicit GAMESSOptimizer(GlobalSearch::OptBase* parent,
                           const QString& filename = "");
};

} // end namespace RandomDock

#endif

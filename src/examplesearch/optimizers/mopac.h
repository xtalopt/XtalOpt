/**********************************************************************
  MopacOptimizer - Tools to interface with Mopac

  Copyright (C) 2012 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef EXAMPLESEARCH_MOPACOPTIMIZER_H
#define EXAMPLESEARCH_MOPACOPTIMIZER_H

#include <globalsearch/optimizer.h>

namespace ExampleSearch {

class MopacOptimizer : public GlobalSearch::Optimizer
{
  Q_OBJECT

public:
  explicit MopacOptimizer(GlobalSearch::OptBase* parent,
                          const QString& filename = "");
};

} // end namespace ExampleSearch

#endif

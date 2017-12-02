/**********************************************************************
  ExampleSearch

  Copyright (C) 2012 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef EXAMPLESEARCH_H
#define EXAMPLESEARCH_H

#include <globalsearch/optbase.h>

#include <Eigen/Geometry>

#include <QDebug>
#include <QReadWriteLock>
#include <QStringList>

#include <QInputDialog>

namespace GlobalSearch {
class SlottedWaitCondition;
class Structure;
}

namespace ExampleSearch {
class ExampleSearchDialog;

class ExampleSearch : public GlobalSearch::OptBase
{
  Q_OBJECT

public:
  explicit ExampleSearch(ExampleSearchDialog* parent);
  virtual ~ExampleSearch();

  enum OptTypes
  {
    OT_GAMESS = 0,
    OT_MOPAC
  };

  enum QueueInterfaces
  {
    QI_LOCAL = 0
#ifdef ENABLE_SSH
    ,
    QI_PBS,
    QI_SGE,
    QI_LSF,
    QI_SLURM,
    QI_LOADLEVELER
#endif // ENABLE_SSH
  };

  GlobalSearch::Structure* replaceWithRandom(GlobalSearch::Structure* s,
                                             const QString& reason = "");

  GlobalSearch::Structure* generateRandomStructure();

  bool checkLimits();

public slots:
  void startSearch();
  void generateNewStructure();
  void initializeAndAddStructure(GlobalSearch::Structure* structure);

private:
  GlobalSearch::SlottedWaitCondition* m_initWC;
  QMutex* m_structureInitMutex;
};

} // end namespace ExampleSearch

#endif

/**********************************************************************
  RandomDock -- A tool for analysing a matrix-substrate docking problem

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef TAB_PARAMS_H
#define TAB_PARAMS_H

#include <globalsearch/ui/abstracttab.h>

#include "ui_tab_params.h"

namespace RandomDock {
class RandomDockDialog;
class RandomDock;

class TabParams : public GlobalSearch::AbstractTab
{
  Q_OBJECT

public:
  explicit TabParams(RandomDockDialog* dialog, RandomDock* opt);
  virtual ~TabParams();

public slots:
  void lockGUI();
  void readSettings(const QString& filename = "");
  void writeSettings(const QString& filename = "");
  void updateOptimizationInfo();

signals:

private:
  Ui::Tab_Params ui;
};
}

#endif

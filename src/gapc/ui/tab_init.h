/**********************************************************************
  TabInit - Parameter for initializing the search

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef TAB_INIT_H
#define TAB_INIT_H

#include <globalsearch/ui/abstracttab.h>

#include "ui_tab_init.h"

namespace GAPC {
class GAPCDialog;
class OptGAPC;

class TabInit : public GlobalSearch::AbstractTab
{
  Q_OBJECT

public:
  explicit TabInit(GAPCDialog* parent, OptGAPC* p);
  virtual ~TabInit();

public slots:
  void lockGUI();
  void readSettings(const QString& filename = "");
  void writeSettings(const QString& filename = "");
  void updateGUI();
  void getComposition(const QString& str);
  void updateComposition();
  void updateParams();

signals:

private:
  Ui::Tab_Init ui;
};
}

#endif

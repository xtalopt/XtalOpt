/**********************************************************************
  XtalOpt - Tools for advanced crystal optimization

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef TAB_MO_H
#define TAB_MO_H

#include <globalsearch/ui/abstracttab.h>

#include "ui_tab_mo.h"

namespace GlobalSearch {
class AbstractDialog;
}

namespace XtalOpt {
class XtalOpt;

class TabMo : public GlobalSearch::AbstractTab
{
  Q_OBJECT

public:
  explicit TabMo(GlobalSearch::AbstractDialog* parent, XtalOpt* p);
  virtual ~TabMo() override;

  enum ObjectivesColumns
    {
      Oc_TYPE = 0,
      Oc_PATH,
      Oc_OUTPUT,
      Oc_WEIGHT
    };

public slots:
  void lockGUI() override;
  void readSettings(const QString& filename = "") override;
  void writeSettings(const QString& filename = "") override;
  void updateGUI() override;
  bool updateOptTypeInfo();
  bool updateObjectives();
  void addObjectives();
  void removeObjectives();
  void updateObjectivesTable();
  void updateFieldsWithOptSelection(QString value_type);
  void errorPromptWindow(const QString& instr);

signals:

private:
  Ui::Tab_Mo ui;
};
}

#endif

/**********************************************************************
  TabMo - The multi-objective tab: define objectives and constraints.

  Copyright (C) 2009-2011 by David Lonie
  Copyright (C) 2024 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef TAB_MO_H
#define TAB_MO_H

#include <search/gui/abstracttab.h>

#include "ui_tab_mo.h"

namespace Search {
class AbstractDialog;
}

namespace XtalOpt {
class XtalOpt;

/**
 * The multi-objective tab: define objectives and constraints.
 */
class TabMo : public Search::AbstractTab
{
  Q_OBJECT

public:
  explicit TabMo(Search::AbstractDialog* parent, XtalOpt* p);
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
  void updateGUI() override;
  bool updateObjectives();
  void updateScriptCancel();
  void addObjectives();
  void removeObjectives();
  void updateObjectivesTable();
  void addConstraint();
  void removeConstraint();
  void updateConstraintsTable();
  void updateFieldsWithOptSelection(QString value_type);

signals:

private:
  Ui::Tab_Mo ui;
};
}

#endif

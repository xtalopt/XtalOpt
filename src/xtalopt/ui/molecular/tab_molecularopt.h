/**********************************************************************
  TabMolecularOpt - the tab for molecular optimization options in XtalOpt.

  Copyright (C) 2017 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef TAB_MOLECULAROPT_H
#define TAB_MOLECULAROPT_H

#include <globalsearch/ui/abstracttab.h>

#include "ui_tab_molecularopt.h"

namespace GlobalSearch {
class AbstractDialog;
}

namespace XtalOpt {
class XtalOpt;

class TabMolecularOpt : public GlobalSearch::AbstractTab
{
  Q_OBJECT

public:
  explicit TabMolecularOpt(GlobalSearch::AbstractDialog* parent, XtalOpt* p);
  virtual ~TabMolecularOpt() override;

public slots:
  void lockGUI() override;
  void readSettings(const QString& filename = "") override;
  void writeSettings(const QString& filename = "") override;
  void updateGUI() override;
  void updateOptimizationInfo();
  void addSeed(QListWidgetItem* item = nullptr);
  void removeSeed();
  void updateSeeds();

signals:

private:
  Ui::TabMolecularOpt ui;
};
}

#endif

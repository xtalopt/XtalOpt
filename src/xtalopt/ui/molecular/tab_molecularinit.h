/**********************************************************************
  TabMolecularInit - the tab for molecular initialization in XtalOpt

  Copyright (C) 2017 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef TAB_MOLECULARINIT_H
#define TAB_MOLECULARINIT_H

#include <globalsearch/ui/abstracttab.h>

#include "ui_tab_molecularinit.h"

namespace GlobalSearch {
class AbstractDialog;
class ConformerGeneratorDialog;
}

namespace XtalOpt {
class XtalOpt;

class TabMolecularInit : public GlobalSearch::AbstractTab
{
  Q_OBJECT

public:
  explicit TabMolecularInit(GlobalSearch::AbstractDialog* parent, XtalOpt* p);
  virtual ~TabMolecularInit() override;

public slots:
  void lockGUI() override;
  void readSettings(const QString& filename = "") override;
  void writeSettings(const QString& filename = "") override;
  void updateGUI() override;
  void updateDimensions();
  void updateMinRadii();
  void updateFormulaUnits();
  void updateFormulaUnitsListUI();
  void updateInitOptions();
  void adjustVolumesToBePerFU(uint FU);
  void updateMinIAD();

  // Conformer stuff
  void showConformerGeneratorDialog();
  void updateConformerDir();
  void browseConfDir();

signals:

private:
  Ui::TabMolecularInit ui;

  GlobalSearch::ConformerGeneratorDialog* m_confGenDialog;
};
}

#endif

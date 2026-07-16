/**********************************************************************
  TabStruc - The structure-settings tab: composition, cell limits, and volumes.

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef TAB_STRUC_H
#define TAB_STRUC_H

#include <search/gui/abstracttab.h>

#include <xtalopt/types.h>

#include "ui_tab_struc.h"

#include <QPointer>

class QWidget;

namespace Search {
class AbstractDialog;
}

namespace XtalOpt {
class RandSpgDialog;
class XtalOpt;

/**
 * The structure-settings tab: composition, cell limits, and volumes.
 */
class TabStruc : public Search::AbstractTab
{
  Q_OBJECT

public:
  explicit TabStruc(Search::AbstractDialog* parent, XtalOpt* p);
  virtual ~TabStruc() override;

  enum CompositionColumns
  {
    CC_SYMBOL = 0,
    CC_MINRADIUS,
    CC_REFENE,
    CC_MINVOL,
    CC_MAXVOL
  };

  enum IADColumns
  {
    IC_SYMBOL1 = 0,
    IC_SYMBOL2 = 1,
    IC_MINIAD = 2,
  };

  enum MoleculeUnitColumns
  {
    MC_FORMULA = 0,
    MC_TEMPLATE = 1
  };

public slots:
  void lockGUI() override;
  void updateGUI() override;
  void updateAtomCountLimits();
  void updateVolumes();
  void updateReferenceEnergies();
  void updateSearchType();
  void getComposition();
  void updateCompositionTable();
  void updateDimensions();
  void updateInitOptions();
  void updateMoleculeUnits();
  void updateCustomIAD();
  void addRow();
  void removeRow();
  void removeAll();
  void openSpgOptions();
  void showMoleculeUnitHelp();

signals:

private:
  void refreshMoleculeUnitTable();
  void updateMoleculeUnitTableEnabled();
  void updateCustomIADTableEnabled();

  Ui::Tab_Init ui;
  RandSpgDialog* m_spgOptions;
  QPointer<QWidget> m_moleculeUnitHelpDialog;
  bool m_openingSpgOptions;
  bool m_updateMoleculeUnitsInProgress;
  bool m_moleculeUnitTableLocked;
  bool m_customIADTableLocked;
};
}

#endif

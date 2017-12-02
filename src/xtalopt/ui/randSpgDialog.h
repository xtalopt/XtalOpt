/**********************************************************************
  randSpgDialog.h - Contains table for viewing and selecting spacegroups for
                    spacegroup generation.

  Copyright (C) 2015 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#ifndef RAND_SPG_DIALOG_H
#define RAND_SPG_DIALOG_H

// Include the qt-generated ui header
#include "ui_randSpgDialog.h"

#include <QBrush>
#include <QDialog>

class QCheckBox;
class QSpinBox;

namespace XtalOpt {
class XtalOpt;
struct XtalCompositionStruct;

class RandSpgDialog : public QDialog, public Ui::RandSpgDialog
{
  Q_OBJECT

  enum TableColumns
  {
    HM_Spg = 0,
    FormulaUnitsPossible,
    CheckBox,
    SpinBox
  };

  struct Spg_Table_Entry
  {
    QString HM_spg;
    QString formulaUnitsPossible;
    QBrush brush;
  };

public:
  explicit RandSpgDialog(XtalOpt* p, QWidget* parent = 0);
  virtual ~RandSpgDialog() override;
  void setTableEntry(uint row, const Spg_Table_Entry& e);
  bool isCompositionSame(XtalOpt* p);

public slots:
  void selectAll();
  void deselectAll();
  void incrementAll();
  void decrementAll();
  void updateAll();

private:
  QSpinBox* getNewSpinBox();
  void setLabel();

  XtalOpt* m_xtalopt;
  QHash<uint, XtalCompositionStruct> m_comp;
  QList<uint> m_FUList;
  QList<QCheckBox*> m_checkBoxList;
  QList<QSpinBox*> m_spinBoxList;
};
}

#endif

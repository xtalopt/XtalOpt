/**********************************************************************
  spgInitDialog.h - Contains table for viewing and selecting spacegroups for
                    spacegroup initialization.

  Copyright (C) 2015 by Patrick Avery

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public icense for more details.
 ***********************************************************************/

#ifndef SPG_INIT_DIALOG_H
#define SPG_INIT_DIALOG_H

#include "spgInit.h"

// Include the qt-generated ui header
#include "ui_spgInitDialog.h"

#include <QtGui/QDialog>
#include <QtGui/QBrush>

class QCheckBox;

namespace XtalOpt {
  class XtalOptDialog;
  class XtalOpt;
  class Xtal;

  class SpgInitDialog : public QDialog, public Ui::SpgInitDialog
  {
    Q_OBJECT

    enum TableColumns {
      HM_Spg = 0,
      FormulaUnitsPossible,
      FormulaUnitsAllowed,
      MinNumOfEach
    };

    struct Spg_Table_Entry {
      QString HM_spg;
      QString formulaUnitsPossible;
      QString formulaUnitsAllowed;
      uint minNumOfEach;
      QBrush brush;
    };

   public:
    explicit SpgInitDialog(XtalOpt* p, QWidget* parent = 0);
    virtual ~SpgInitDialog();
    void setTableEntry(uint row, const Spg_Table_Entry& e);
   public slots:
    void selectAll();
    void deselectAll();

   private:
    XtalOpt* m_xtalopt;
    QList<QCheckBox*> m_checkBoxList;
  };
}

#endif

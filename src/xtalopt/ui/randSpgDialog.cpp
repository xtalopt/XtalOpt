/**********************************************************************
  RandSpgDialog.cpp - The dialog for spacegroup generation.

  Copyright (C) 2015 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#include <vector>

#include <QCheckBox>
#include <QDebug>
#include <QSpinBox>

#include "randSpgDialog.h"
#include <randSpg/include/randSpg.h>

#include <xtalopt/structures/xtal.h>
#include <xtalopt/xtalopt.h>

#include <globalsearch/eleminfo.h>
#include <globalsearch/utilities/fileutils.h>

namespace XtalOpt {

RandSpgDialog::RandSpgDialog(XtalOpt* p, QWidget* parent)
  : QDialog(parent), m_xtalopt(p), m_comp(p->comp),
    m_FUList(p->formulaUnitsList), m_checkBoxList(QList<QCheckBox*>()),
    m_spinBoxList(QList<QSpinBox*>())
{
  // Since RandSpgDialog inherits from the qt-created class, Ui::RandSpgDialog
  // We can just tell it to set up itself
  setupUi(this);

  // Make connections
  connect(this->push_selectAll, SIGNAL(clicked()), this, SLOT(selectAll()));
  connect(this->push_deselectAll, SIGNAL(clicked()), this, SLOT(deselectAll()));
  connect(this->push_incrementAll, SIGNAL(clicked()), this,
          SLOT(incrementAll()));
  connect(this->push_decrementAll, SIGNAL(clicked()), this,
          SLOT(decrementAll()));

  // Set the label
  setLabel();

  // Get the list of atoms
  QList<uint> atomicNums = m_comp.keys();

  QList<uint> atoms;
  for (size_t i = 0; i < atomicNums.size(); i++) {
    for (size_t j = 0; j < m_comp.value(atomicNums[i]).quantity; j++) {
      atoms.push_back(atomicNums[i]);
    }
  }

  // Let's investigate every spacegroup!
  for (size_t spg = 1; spg <= 230; spg++) {
    uint index = spg - 1;
    QString FUPossible = "";
    // Let's also investigate every formula unit possible!
    for (size_t i = 0; i < m_FUList.size(); i++) {
      // Make an atoms list that is adjusted by the formula units
      std::vector<uint> tempAtoms;
      tempAtoms.reserve(atoms.size() * m_FUList.at(i));
      for (size_t j = 0; j < atoms.size(); j++) {
        for (size_t k = 0; k < m_FUList.at(i); k++) {
          tempAtoms.push_back(atoms.at(j));
        }
      }
      // Append each formula unit to the list followed by a comma
      if (RandSpg::isSpgPossible(spg, tempAtoms)) {
        FUPossible.append(QString::number(m_FUList.at(i)) + ",");
      }
    }

    QString result = "";
    // This returns a list of unsigned integers. It returns empty
    // if it cannot be made successfully
    if (FileUtils::parseUIntString(FUPossible, result).size() != 0)
      FUPossible = result;

    else
      FUPossible = "";

    // Add the new row
    this->table_list->insertRow(index);
    for (int i = 0; i < 4; i++)
      this->table_list->setItem(index, i, new QTableWidgetItem());
    Spg_Table_Entry e;

    e.HM_spg = Xtal::getHMName(spg);

    // Add a new checkbox
    m_checkBoxList.append(new QCheckBox);
    // Set the checkbox checked if the formula unit is possible
    if (FUPossible.size() != 0)
      m_checkBoxList.at(index)->setChecked(true);
    // If the spacegroup cannot be made, disable the checkbox
    else
      m_checkBoxList.at(index)->setEnabled(false);

    // This starts off disabled...
    m_spinBoxList.append(getNewSpinBox());
    if (FUPossible.size() != 0)
      m_spinBoxList.at(index)->setEnabled(true);

    e.formulaUnitsPossible = FUPossible;
    e.brush = QBrush(Qt::green);
    setTableEntry(index, e);

    // Make connections to the checkboxes and spinboxes in the table
    connect(this->m_checkBoxList.at(index), SIGNAL(toggled(bool)), this,
            SLOT(updateAll()));
    connect(this->m_spinBoxList.at(index), SIGNAL(editingFinished()), this,
            SLOT(updateAll()));
  }
  // Let's go ahead and save all the values to m_xtalopt
  updateAll();
}

RandSpgDialog::~RandSpgDialog()
{
  // Delete the dynamically allocated checkbox list
  for (size_t i = 0; i < m_checkBoxList.size(); i++) {
    if (m_checkBoxList.at(i)) {
      delete m_checkBoxList.at(i);
      m_checkBoxList[i] = 0;
    }
  }

  for (size_t i = 0; i < m_spinBoxList.size(); i++) {
    if (m_spinBoxList.at(i)) {
      delete m_spinBoxList.at(i);
      m_spinBoxList[i] = 0;
    }
  }
}

void RandSpgDialog::setTableEntry(uint row, const Spg_Table_Entry& e)
{
  this->table_list->item(row, HM_Spg)->setText(e.HM_spg);
  this->table_list->item(row, FormulaUnitsPossible)
    ->setText(e.formulaUnitsPossible);
  // Maybe we'll use a brush in the future...
  // this->table_list->item(row, FormulaUnitsPossible)->setBackground(e.brush);
  this->table_list->setCellWidget(row, CheckBox, m_checkBoxList.at(row));
  this->table_list->setCellWidget(row, SpinBox, m_spinBoxList.at(row));
}

void RandSpgDialog::selectAll()
{
  for (size_t i = 0; i < m_checkBoxList.size(); i++) {
    if (m_checkBoxList.at(i)->isEnabled())
      m_checkBoxList.at(i)->setChecked(true);
  }
}

void RandSpgDialog::deselectAll()
{
  for (size_t i = 0; i < m_checkBoxList.size(); i++) {
    if (m_checkBoxList.at(i)->isEnabled())
      m_checkBoxList.at(i)->setChecked(false);
  }
}

void RandSpgDialog::incrementAll()
{
  for (size_t i = 0; i < m_spinBoxList.size(); i++) {
    if (m_spinBoxList.at(i)->isEnabled())
      m_spinBoxList.at(i)->setValue(m_spinBoxList.at(i)->value() + 1);
  }
  updateAll();
}

void RandSpgDialog::decrementAll()
{
  for (size_t i = 0; i < m_spinBoxList.size(); i++) {
    if (m_spinBoxList.at(i)->isEnabled())
      m_spinBoxList.at(i)->setValue(m_spinBoxList.at(i)->value() - 1);
  }
  updateAll();
}

QSpinBox* RandSpgDialog::getNewSpinBox()
{
  QSpinBox* spinBox = new QSpinBox;
  spinBox->setMinimum(0);
  spinBox->setMaximum(10000);
  spinBox->setSingleStep(1);
  spinBox->setValue(0);
  spinBox->setEnabled(false);
  return spinBox;
}

void RandSpgDialog::updateAll()
{
  m_xtalopt->minXtalsOfSpgPerFU.clear();
  for (size_t i = 0; i < m_checkBoxList.size(); i++) {
    if (!m_checkBoxList.at(i)->isEnabled() ||
        !m_checkBoxList.at(i)->isChecked()) {
      m_spinBoxList.at(i)->setEnabled(false);
      // -1 means we cannot use this one...
      m_xtalopt->minXtalsOfSpgPerFU.append(-1);
    } else {
      m_spinBoxList.at(i)->setEnabled(true);
      // Append to the minXtalsOfSpgPerFU
      m_xtalopt->minXtalsOfSpgPerFU.append(m_spinBoxList.at(i)->value());
    }
  }
}

bool RandSpgDialog::isCompositionSame(XtalOpt* p)
{
  if (p->comp == m_comp && p->formulaUnitsList == m_FUList)
    return true;
  else
    return false;
}

void RandSpgDialog::setLabel()
{
  QList<uint> atomicNums = m_comp.keys();

  // Just keep the default label if no composition is set
  if (atomicNums.isEmpty())
    return;

  QString label = " ";
  for (size_t i = 0; i < atomicNums.size(); i++) {
    QString tmp = QString(ElemInfo::getAtomicSymbol(atomicNums.at(i)).c_str()) +
                  QString::number(m_comp.value(atomicNums.at(i)).quantity) +
                  " ";
    label.append(tmp);
  }

  this->ui_label->setText(label);
}
}

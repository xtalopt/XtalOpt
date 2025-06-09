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

#include <globalsearch/eleminfo.h>
#include <globalsearch/utilities/fileutils.h>

namespace XtalOpt {

RandSpgDialog::RandSpgDialog(XtalOpt* p, QWidget* parent)
    : QDialog(parent), m_xtalopt(p), m_compList(p->compList),
    m_checkBoxList(QList<QCheckBox*>()),
    m_spinBoxList(QList<QSpinBox*>())
{
  // First thing: if no composition is set; just return!
  if (m_compList.isEmpty())
    return;

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
  this->ui_label->setText("Possible space groups for the compositions");
  //setLabel(p);

  // Let's investigate every spacegroup!
  for (size_t spg = 1; spg <= 230; spg++) {

    bool spgPossible = false;

    uint index = spg - 1;

    // List of compositions' formula for which this spg is possible
    QString possibleComps = "";

    // Check every composition, and add to list if we can use
    //   this space group for the composition
    for (int c = 0; c < m_compList.size(); c++) {
      QList<uint> atomicNums = m_compList[c].getAtomicNumbers();
      QList<uint> atoms;
      for (size_t i = 0; i < atomicNums.size(); i++) {
        for (size_t j = 0; j < m_compList[c].getCount(atomicNums[i]); j++) {
          atoms.push_back(atomicNums[i]);
        }
      }

      // Make an atoms list, and check if spg can be created!
      std::vector<uint> tempAtoms;
      tempAtoms.reserve(atoms.size());
      for (size_t j = 0; j < atoms.size(); j++)
        tempAtoms.push_back(atoms.at(j));
      if (RandSpg::isSpgPossible(spg, tempAtoms)) {
        possibleComps += m_compList[c].getFormula() + ",";
        spgPossible = true;
      }
    }

    // Add the new row
    this->table_list->insertRow(index);
    for (int i = 0; i < 4; i++)
      this->table_list->setItem(index, i, new QTableWidgetItem());
    Spg_Table_Entry e;

    if (possibleComps.endsWith(","))
      possibleComps.chop(1);
    e.possibleFormulas = possibleComps;
    e.HM_spg = Xtal::getHMName(spg);

    // Add a new checkbox
    m_checkBoxList.append(new QCheckBox);
    // Set the checkbox checked if the spg is possible
    if (spgPossible)
      m_checkBoxList.at(index)->setChecked(true);
    // If the spacegroup cannot be made, disable the checkbox
    else
      m_checkBoxList.at(index)->setEnabled(false);

    // This starts off disabled...
    m_spinBoxList.append(getNewSpinBox());
    if (spgPossible)
      m_spinBoxList.at(index)->setEnabled(true);

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
  this->table_list->item(row, PossibleFormulas)->setText(e.possibleFormulas);
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
  m_xtalopt->minXtalsOfSpg.clear();
  for (size_t i = 0; i < m_checkBoxList.size(); i++) {
    if (!m_checkBoxList.at(i)->isEnabled() ||
        !m_checkBoxList.at(i)->isChecked()) {
      m_spinBoxList.at(i)->setEnabled(false);
      // -1 means we cannot use this one...
      m_xtalopt->minXtalsOfSpg.append(-1);
    } else {
      m_spinBoxList.at(i)->setEnabled(true);
      // Append to the minXtalsOfSpg
      m_xtalopt->minXtalsOfSpg.append(m_spinBoxList.at(i)->value());
    }
  }
}

bool RandSpgDialog::isCompositionSame(XtalOpt* p)
{
  if (p->compList.size() != m_compList.size())
    return false;

  for (int i = 0; i < p->compList.size(); i++) {
    QString frm1 = p->compList[i].getFormula();
    QString frm2 = m_compList[i].getFormula();
    if (frm1 != frm2)
      return false;
  }

  return true;
}
}

/**********************************************************************
  RandSpgDialog - The dialog for spacegroup generation.

  Copyright (C) 2015 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <QAbstractSpinBox>
#include <QPushButton>
#include <QSpinBox>

#include "randspg_dialog.h"

#include <xtalopt/structures/xtal.h>

#include <atoms/eleminfo.h>
#include <common/fileutils.h>
#include <atoms/geometry.h>

namespace XtalOpt {

RandSpgDialog::RandSpgDialog(XtalOpt* p, QWidget* parent)
    : QDialog(parent), m_xtalopt(p), m_compList(p->compList()),
    m_spinBoxList(QList<QSpinBox*>())
{
  // First thing: if no composition is set; just return!
  if (m_compList.isEmpty())
    return;

  // Since RandSpgDialog inherits from the qt-created class, Ui::RandSpgDialog
  // We can just tell it to set up itself
  setupUi(this);

  // Make connections
  connect(this->push_incrementAll, &QPushButton::clicked, this, &RandSpgDialog::incrementAll);
  connect(this->push_decrementAll, &QPushButton::clicked, this, &RandSpgDialog::decrementAll);
  connect(this->push_resetAll, &QPushButton::clicked, this, &RandSpgDialog::resetAll);

  // Set the label
  this->ui_label->setText("Possible space groups for the compositions");

  // Let's investigate every spacegroup!
  for (size_t spg = 1; spg <= 230; spg++) {

    uint index = spg - 1;
    const QStringList possibleFormulaList = p->randSpgCompatibleFormulaStrings(spg);
    const bool spgPossible = !possibleFormulaList.isEmpty();

    // List of compositions' formula for which this spg is possible
    QString possibleComps = possibleFormulaList.join(",");

    // Add the new row
    this->table_list->insertRow(index);
    for (int i = 0; i < 3; i++)
      this->table_list->setItem(index, i, new QTableWidgetItem());
    Spg_Table_Entry e;

    e.possibleFormulas = possibleComps;
    e.HM_spg = Atoms::Geometry::getHMName(spg);

    m_spinBoxList.append(getNewSpinBox());
    m_spinBoxList.at(index)->setEnabled(spgPossible);

    e.brush = QBrush(Qt::green);
    setTableEntry(index, e);

    // Re-apply the values whenever a spin box edit completes.
    connect(this->m_spinBoxList.at(index), &QAbstractSpinBox::editingFinished,
            this, &RandSpgDialog::updateAll);
  }
  // Show the forced counts as currently set, and save them back.
  updateSpinBoxes();
  updateAll();
}

void RandSpgDialog::updateSpinBoxes()
{
  // Set the spin boxes to the forced counts as currently set in the
  // search settings (index = spg-1, value = number of forced xtals).
  const QList<int>& forcedCounts = m_xtalopt->minXtalsOfSpg();
  for (int i = 0; i < static_cast<int>(m_spinBoxList.size()); i++) {
    const int count = i < forcedCounts.size() ? forcedCounts.at(i) : 0;
    m_spinBoxList.at(i)->setValue(count > 0 ? count : 0);
  }
}

RandSpgDialog::~RandSpgDialog()
{
  for (int i = 0; i < static_cast<int>(m_spinBoxList.size()); i++) {
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
  this->table_list->setCellWidget(row, SpinBox, m_spinBoxList.at(row));
}

void RandSpgDialog::incrementAll()
{
  for (int i = 0; i < static_cast<int>(m_spinBoxList.size()); i++) {
    if (m_spinBoxList.at(i)->isEnabled())
      m_spinBoxList.at(i)->setValue(m_spinBoxList.at(i)->value() + 1);
  }
  updateAll();
}

void RandSpgDialog::decrementAll()
{
  for (int i = 0; i < static_cast<int>(m_spinBoxList.size()); i++) {
    if (m_spinBoxList.at(i)->isEnabled())
      m_spinBoxList.at(i)->setValue(m_spinBoxList.at(i)->value() - 1);
  }
  updateAll();
}

void RandSpgDialog::resetAll()
{
  for (int i = 0; i < static_cast<int>(m_spinBoxList.size()); i++) {
    if (m_spinBoxList.at(i)->isEnabled())
      m_spinBoxList.at(i)->setValue(0);
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
  // Do not change forced space groups during a run.
  if (m_xtalopt->isSessionInProgress())
    return;

  // Change the settings.
  QWriteLocker runtimeLocker(m_xtalopt->runtimeSettingsLock());
  QList<int> minXtalsOfSpg;
  for (int i = 0; i < static_cast<int>(m_spinBoxList.size()); i++) {
    // Forced count for this space group (0 means "not forced"). Space groups
    // that are impossible for the input compositions have a disabled spin box
    // and contribute 0.
    if (m_spinBoxList.at(i)->isEnabled())
      minXtalsOfSpg.append(m_spinBoxList.at(i)->value());
    else
      minXtalsOfSpg.append(0);
  }
  m_xtalopt->minXtalsOfSpg() = minXtalsOfSpg;

  // Update the forced space group input.
  QStringList forced;
  for (int i = 0; i < minXtalsOfSpg.size(); ++i) {
    for (int n = 0; n < minXtalsOfSpg.at(i); ++n)
      forced.append(QString::number(i + 1));
  }
  m_xtalopt->setInputForcedSpgsString(forced.join(","));
}

bool RandSpgDialog::isCompositionSame(XtalOpt* p)
{
  if (p->compList().size() != m_compList.size())
    return false;

  for (int i = 0; i < p->compList().size(); i++) {
    QString frm1 = p->compList()[i].getFormula();
    QString frm2 = m_compList[i].getFormula();
    if (frm1 != frm2)
      return false;
  }

  return true;
}
}

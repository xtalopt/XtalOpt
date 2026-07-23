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

#include <xtalopt/gui/tab_mo.h>

#include <xtalopt/gui/dialog.h>
#include <xtalopt/xtalopt.h>

#include <QAbstractItemView>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QReadWriteLock>
#include <QTableWidget>

#include <QInputDialog>
#include <QHeaderView>

using namespace std;

namespace XtalOpt {

TabMo::TabMo(Search::AbstractDialog* parent, XtalOpt* p)
  : AbstractTab(parent, p)
{
  ui.setupUi(m_tab_widget);

  // Setup objective table header sizes
  ui.gridLayout_2->setColumnStretch(0, 3);
  ui.gridLayout_2->setColumnStretch(1, 2);
  ui.table_objectives->setColumnWidth(0, 120);
  ui.table_objectives->setColumnWidth(2, 190);
  ui.table_objectives->setColumnWidth(3, 80);
  ui.table_objectives->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
  ui.table_constraints->setColumnWidth(1, 190);
  ui.table_constraints->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  const int filtrationIndex = ui.combo_type->findText("Filtration");
  if (filtrationIndex >= 0)
    ui.combo_type->removeItem(filtrationIndex);

  updateGUI();

  // Update fields with opt type selection
  connect(ui.combo_type, &QComboBox::currentTextChanged,
          this, &TabMo::updateFieldsWithOptSelection);

  // Objectives
  connect(ui.push_addObjectives,    &QPushButton::clicked, this, &TabMo::addObjectives);
  connect(ui.push_removeObjectives, &QPushButton::clicked, this, &TabMo::removeObjectives);
  connect(ui.cb_redo_constraints, &QCheckBox::toggled, this, &TabMo::updateObjectives);
  connect(ui.push_addConstraint, &QPushButton::clicked, this, &TabMo::addConstraint);
  connect(ui.push_removeConstraint, &QPushButton::clicked, this, &TabMo::removeConstraint);

  // The script-cancel setting
  connect(ui.cb_cancelScriptAfterTime, &QCheckBox::toggled, this, &TabMo::updateScriptCancel);
  connect(ui.spin_hoursForCancelScript,
          static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
          this, &TabMo::updateScriptCancel);

  initialize();
}

TabMo::~TabMo()
{
}

void TabMo::updateFieldsWithOptSelection(QString /*value_type*/)
{
  ui.line_path->setDisabled(false);
  ui.line_output->setDisabled(false);
  ui.line_path->setText("");
  ui.line_output->setText("");
}

void TabMo::updateGUI()
{
  m_updateGuiInProgress = true;
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  bool wasBlocked = ui.cb_redo_constraints->blockSignals(true);
  ui.cb_redo_constraints->setChecked(xtalopt->isConstraintsReDo());
  ui.cb_redo_constraints->blockSignals(wasBlocked);

  wasBlocked = ui.cb_cancelScriptAfterTime->blockSignals(true);
  ui.cb_cancelScriptAfterTime->setChecked(xtalopt->cancelScriptAfterTime());
  ui.cb_cancelScriptAfterTime->blockSignals(wasBlocked);

  wasBlocked = ui.spin_hoursForCancelScript->blockSignals(true);
  ui.spin_hoursForCancelScript->setValue(xtalopt->hoursForCancelScriptAfterTime());
  ui.spin_hoursForCancelScript->setEnabled(xtalopt->cancelScriptAfterTime());
  ui.spin_hoursForCancelScript->blockSignals(wasBlocked);

  // Initiate the objectives table
  updateObjectivesTable();
  updateConstraintsTable();

  m_updateGuiInProgress = false;
}

void TabMo::lockGUI()
{
  ui.combo_type->setDisabled(true);
  ui.sb_weight->setDisabled(true);
  ui.line_path->setDisabled(true);
  ui.line_output->setDisabled(true);
  ui.push_addObjectives->setDisabled(true);
  ui.push_removeObjectives->setDisabled(true);
  ui.table_objectives->setDisabled(true);
  ui.line_constraintPath->setDisabled(true);
  ui.line_constraintOutput->setDisabled(true);
  ui.push_addConstraint->setDisabled(true);
  ui.push_removeConstraint->setDisabled(true);
  ui.table_constraints->setDisabled(true);

  // The script-cancel setting is runtime-adjustable
  if (m_search->isReadOnly()) {
    ui.cb_redo_constraints->setDisabled(true);
    ui.cb_cancelScriptAfterTime->setDisabled(true);
    ui.spin_hoursForCancelScript->setDisabled(true);
  }
}

bool TabMo::updateObjectives()
{
  if (m_updateGuiInProgress)
    return true;

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);
  const bool value = ui.cb_redo_constraints->isChecked();
  bool changed = false;
  {
    QWriteLocker runtimeLocker(m_search->runtimeSettingsLock());
    changed = xtalopt->isConstraintsReDo() != value;
    if (changed)
      xtalopt->setConstraintsReDo(value);
  }
  if (changed && m_search->isSessionInProgress())
    xtalopt->requestSettingsStateSave();

  return true;
}

void TabMo::updateScriptCancel()
{
  ui.spin_hoursForCancelScript->setEnabled(ui.cb_cancelScriptAfterTime->isChecked());
  if (m_updateGuiInProgress)
    return;

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);
  const bool enabled = ui.cb_cancelScriptAfterTime->isChecked();
  const double hours = ui.spin_hoursForCancelScript->value();
  bool changed = false;
  {
    QWriteLocker runtimeLocker(m_search->runtimeSettingsLock());
    if (xtalopt->cancelScriptAfterTime() != enabled) {
      xtalopt->setCancelScriptAfterTime(enabled);
      changed = true;
    }
    if (xtalopt->hoursForCancelScriptAfterTime() != hours) {
      xtalopt->setHoursForCancelScriptAfterTime(hours);
      changed = true;
    }
  }
  if (changed && m_search->isSessionInProgress())
    xtalopt->requestSettingsStateSave();
}

void TabMo::addObjectives()
{
  // Do not change objectives during a run.
  if (m_search->isSessionInProgress())
    return;

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  QString value_type = ui.combo_type->currentText();
  QString value_path = ui.line_path->text();
  QString value_outf = ui.line_output->text();
  QString value_wegt = QString::number(ui.sb_weight->value());

  QString ins = value_type + " " + value_path + " "
                + value_outf + " " + value_wegt;

  if (!xtalopt->processInputObjectives(ins)) {
    errorPromptWindow("Error adding objective!");
    return;
  }
  xtalopt->processInputData();

  updateObjectivesTable();

  // Clean up the entry fields in "Add Objective" after adding a objective
  ui.line_output->clear();
  ui.line_path->clear();
  ui.sb_weight->setValue(0.0);
  ui.combo_type->setCurrentIndex(0);
  ui.push_addObjectives->setDefault(false);
}

void TabMo::removeObjectives()
{
  // Do not change objectives during a run.
  if (m_search->isSessionInProgress())
    return;

  // First, check if a row is selected in the objective list
  if (!ui.table_objectives->selectionModel()->hasSelection())
    return;

  if (ui.table_objectives->selectionModel()->selectedRows().size() <= 0)
    return;

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  int row = ui.table_objectives->currentRow();
  int tot = ui.table_objectives->rowCount();

  if (tot == 0 || row < 0 || row >= tot)
    return;
  if (row < XtalOpt::getFirstUserObjectiveIndex())
    return;

  if (!xtalopt->removeUserObjective(row))
    return;

  updateObjectivesTable();

  ui.push_removeObjectives->setDefault(false);
}

void TabMo::addConstraint()
{
  // Do not change constraints during a run.
  if (m_search->isSessionInProgress())
    return;

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  QString ins = ui.line_constraintPath->text() + " " + ui.line_constraintOutput->text();
  if (!xtalopt->processInputConstraint(ins)) {
    errorPromptWindow("Error adding constraint!");
    return;
  }

  updateConstraintsTable();
  ui.line_constraintOutput->clear();
  ui.line_constraintPath->clear();
  ui.push_addConstraint->setDefault(false);
}

void TabMo::removeConstraint()
{
  // Do not change constraints during a run.
  if (m_search->isSessionInProgress())
    return;

  if (!ui.table_constraints->selectionModel()->hasSelection())
    return;

  if (ui.table_constraints->selectionModel()->selectedRows().size() <= 0)
    return;

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);
  int row = ui.table_constraints->currentRow();
  int tot = ui.table_constraints->rowCount();

  if (tot == 0 || row < 0 || row >= tot)
    return;

  if (!xtalopt->removeConstraint(row))
    return;

  updateConstraintsTable();
  ui.push_removeConstraint->setDefault(false);
}

void TabMo::updateObjectivesTable()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  // Adjust the table size.
  const int numRows = xtalopt->getObjectivesNum();

  ui.table_objectives->setRowCount(numRows);

  for (int i = 0; i < numRows; i++) {
    QString tmp;
    if (xtalopt->getObjectivesTyp(i) == xtalopt->Ot_Min)
      tmp = "minimization";
    else if (xtalopt->getObjectivesTyp(i) == xtalopt->Ot_Max)
      tmp = "maximization";
    else
      tmp = "unknown";
    QTableWidgetItem* value_type = new QTableWidgetItem(tmp);
    QTableWidgetItem* value_path = nullptr;
    QTableWidgetItem* value_outf = nullptr;
    QTableWidgetItem* value_wegt =
      new QTableWidgetItem(QString::number(xtalopt->getObjectivesWgt(i)));

    if (i == XtalOpt::getBuiltinObjectiveIndex()) {
      value_path = new QTableWidgetItem("(built-in)");
      value_outf = new QTableWidgetItem("Above hull");
    } else {
      value_path = new QTableWidgetItem(xtalopt->getObjectivesExe(i));
      value_outf = new QTableWidgetItem(xtalopt->getObjectivesOut(i));
    }

    ui.table_objectives->setItem(i, Oc_TYPE, value_type);
    ui.table_objectives->setItem(i, Oc_PATH, value_path);
    ui.table_objectives->setItem(i, Oc_OUTPUT, value_outf);
    ui.table_objectives->setItem(i, Oc_WEIGHT, value_wegt);
  }
}

void TabMo::updateConstraintsTable()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);
  const int numRows = xtalopt->getConstraintsNum();

  ui.table_constraints->setRowCount(numRows);
  for (int i = 0; i < numRows; i++) {
    ui.table_constraints->setItem(i, 0, new QTableWidgetItem(xtalopt->getConstraintExe(i)));
    ui.table_constraints->setItem(i, 1, new QTableWidgetItem(xtalopt->getConstraintOut(i)));
  }
}
}

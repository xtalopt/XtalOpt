/**********************************************************************
  XtalOpt - Tools for advanced crystal optimization

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/ui/tab_mo.h>

#include <xtalopt/ui/dialog.h>
#include <xtalopt/xtalopt.h>

#include <QDebug>
#include <QSettings>

#include <QFileDialog>
#include <QMessageBox>

#include <QInputDialog>
#include <QHeaderView>

using namespace std;

namespace XtalOpt {

TabMo::TabMo(GlobalSearch::AbstractDialog* parent, XtalOpt* p)
  : AbstractTab(parent, p)
{
  ui.setupUi(m_tab_widget);

  // Setup objective table header sizes
  ui.table_objectives->setColumnWidth(0, 150);
  ui.table_objectives->setColumnWidth(1, 350);
  ui.table_objectives->setColumnWidth(2, 200);
  ui.table_objectives->horizontalHeader()->setSectionResizeMode(3,QHeaderView::Stretch);

  // Before we make any connections, let's read the settings
  readSettings();

  // Update fields with opt type selection
  connect(ui.combo_type, SIGNAL(currentIndexChanged(const QString&)), this,
          SLOT(updateFieldsWithOptSelection(const QString&)));

  // Fitness
  connect(ui.cb_tournament, SIGNAL(toggled(bool)), this,
          SLOT(updateOptTypeInfo()));
  connect(ui.cb_restrictPool, SIGNAL(toggled(bool)), this,
          SLOT(updateOptTypeInfo()));
  connect(ui.cb_crowding, SIGNAL(toggled(bool)), this,
          SLOT(updateOptTypeInfo()));
  connect(ui.combo_optType, SIGNAL(currentIndexChanged(int)), this,
          SLOT(updateOptTypeInfo()));
  connect(ui.sb_prec, SIGNAL(editingFinished()), this,
          SLOT(updateOptTypeInfo()));

  // Objectives
  connect(ui.push_addObjectives, SIGNAL(clicked()), this, SLOT(addObjectives()));
  connect(ui.push_removeObjectives, SIGNAL(clicked()), this, SLOT(removeObjectives()));
  connect(ui.cb_redo_objectives, SIGNAL(toggled(bool)), this,
          SLOT(updateObjectives()));

  initialize();
}

TabMo::~TabMo()
{
}

void TabMo::writeSettings(const QString& filename)
{
}

void TabMo::readSettings(const QString& filename)
{
  updateGUI();
}

void TabMo::updateFieldsWithOptSelection(QString value_type)
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

  // Optimization and selection type and info
  ui.cb_tournament->setChecked(xtalopt->m_tournamentSelection);
  ui.cb_crowding->setChecked(xtalopt->m_crowdingDistance);
  ui.sb_prec->setValue(xtalopt->m_objectivePrecision);

  if (xtalopt->m_optimizationType == "pareto") {
    ui.combo_optType->setCurrentIndex(1);
    ui.cb_crowding->setEnabled(true);
    ui.cb_tournament->setEnabled(true);
  } else if (xtalopt->m_optimizationType == "basic") {
    ui.combo_optType->setCurrentIndex(0);
    ui.cb_tournament->setDisabled(true);
    ui.cb_crowding->setDisabled(true);
  }

  if (xtalopt->m_optimizationType == "pareto" &&
      ui.cb_tournament->isChecked()) {
    ui.cb_restrictPool->setEnabled(true);
  } else {
    ui.cb_restrictPool->setDisabled(true);
  }

  // Objectives
  ui.line_path->setPlaceholderText("Full path to the executable script");
  ui.line_output->setPlaceholderText("Script's output file name");

  bool wasBlocked = ui.cb_redo_objectives->blockSignals(true);
  ui.cb_redo_objectives->setChecked(xtalopt->m_objectivesReDo);
  ui.cb_redo_objectives->blockSignals(wasBlocked);

  // Initiate the objectives table
  updateObjectivesTable();

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
  //ui.cb_redo_objectives->setDisabled(true);
}

bool TabMo::updateOptTypeInfo()
{
  if (m_updateGuiInProgress)
    return false;

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  xtalopt->m_objectivePrecision = ui.sb_prec->value();
  xtalopt->m_tournamentSelection = ui.cb_tournament->isChecked();
  xtalopt->m_restrictedPool = ui.cb_restrictPool->isChecked();
  xtalopt->m_crowdingDistance = ui.cb_crowding->isChecked();
  xtalopt->m_optimizationType = ui.combo_optType->currentText().toLower();
  if (xtalopt->m_optimizationType == "basic") {
    ui.cb_tournament->setDisabled(true);
    ui.cb_crowding->setDisabled(true);
  } else if (xtalopt->m_optimizationType == "pareto"){
    ui.cb_tournament->setEnabled(true);
    ui.cb_crowding->setEnabled(true);
  }

  if (xtalopt->m_optimizationType == "pareto" &&
      ui.cb_tournament->isChecked()) {
    ui.cb_restrictPool->setEnabled(true);
  } else {
    ui.cb_restrictPool->setDisabled(true);
  }
  return true;
}

bool TabMo::updateObjectives()
{
  if (m_updateGuiInProgress)
    return true;

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  xtalopt->m_objectivesReDo = ui.cb_redo_objectives->isChecked();

  return true;
}

void TabMo::addObjectives()
{
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

  updateObjectivesTable();

  // Clean up the entry fields in "Add Objective" after adding a objective
  ui.line_output->clear();
  ui.line_output->setPlaceholderText("Script's output file name");
  ui.line_path->clear();
  ui.line_path->setPlaceholderText("Full path to the executable script");
  ui.sb_weight->setValue(0.0);
  ui.combo_type->setCurrentIndex(0);
  ui.push_addObjectives->setDefault(false);
}

void TabMo::removeObjectives()
{
  // First, check if a row is selected in the objective list
  if (!ui.table_objectives->selectionModel()->hasSelection())
    return;

  if (ui.table_objectives->selectionModel()->selectedRows().size() <= 0)
    return;

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  int row = ui.table_objectives->currentRow();
  int tot = ui.table_objectives->rowCount();

  if (tot == 0 || row < 0 || row > tot)
    return;

  xtalopt->removeObjective(row);

  updateObjectivesTable();

  ui.push_removeObjectives->setDefault(false);
}

void TabMo::updateObjectivesTable()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  // Adjust the table size.
  int numRows = xtalopt->getObjectivesNum();

  ui.table_objectives->setRowCount(numRows);

  for (int i = 0; i < numRows; i++) {
    QString tmp;
    if (xtalopt->getObjectivesTyp(i) == xtalopt->Ot_Min)
      tmp = "minimization";
    else if (xtalopt->getObjectivesTyp(i) == xtalopt->Ot_Max)
      tmp = "maximization";
    else
      tmp = "filtration";
    QTableWidgetItem* value_type = new QTableWidgetItem(tmp);
    QTableWidgetItem* value_path = new QTableWidgetItem(xtalopt->getObjectivesExe(i));
    QTableWidgetItem* value_outf = new QTableWidgetItem(xtalopt->getObjectivesOut(i));
    QTableWidgetItem* value_wegt = new QTableWidgetItem(QString::number(xtalopt->getObjectivesWgt(i)));

    ui.table_objectives->setItem(i, Oc_TYPE, value_type);
    ui.table_objectives->setItem(i, Oc_PATH, value_path);
    ui.table_objectives->setItem(i, Oc_OUTPUT, value_outf);
    ui.table_objectives->setItem(i, Oc_WEIGHT, value_wegt);
  }
}

void TabMo::errorPromptWindow(const QString& instr)
{
  QMessageBox msgBox;
  msgBox.setText(instr);
  msgBox.exec();
}
}

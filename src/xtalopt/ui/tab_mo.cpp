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

  // Update active button if objectives are being entered; this is to avoid doing anything but adding!
  connect(ui.combo_type, SIGNAL(activated(int)), this, SLOT(setActiveButtonAdd()));
  connect(ui.line_output, SIGNAL(returnPressed()), this, SLOT(setActiveButtonAdd()));
  connect(ui.line_path, SIGNAL(returnPressed()), this, SLOT(setActiveButtonAdd()));
  connect(ui.sb_weight, SIGNAL(editingFinished()), this, SLOT(setActiveButtonAdd()));
  connect(ui.table_objectives, SIGNAL(cellClicked(int, int)), this, SLOT(setActiveButtonRemove()));

  // Update fields with opt type selection
  connect(ui.combo_type, SIGNAL(currentIndexChanged(const QString&)), this,
          SLOT(updateFieldsWithOptSelection(const QString&)));

  // Objectives
  connect(ui.push_addObjectives, SIGNAL(clicked()), this, SLOT(addObjectives()));
  connect(ui.push_removeObjectives, SIGNAL(clicked()), this, SLOT(removeObjectives()));
  connect(ui.cb_redo_objectives, SIGNAL(toggled(bool)), this,
          SLOT(updateOptimizationInfo()));

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

void TabMo::setActiveButtonAdd()
{
  ui.push_addObjectives->setDefault(true);
}

void TabMo::setActiveButtonRemove()
{
  ui.push_removeObjectives->setDefault(true);
}

void TabMo::updateFieldsWithOptSelection(QString value_type)
{
  if (value_type == "Hardness:AFLOW-ML")
  {
    ui.line_path->setText("N/A");
    ui.line_output->setText("N/A");
    ui.line_path->setDisabled(true);
    ui.line_output->setDisabled(true);
  }
  else
  {
    ui.line_path->setDisabled(false);
    ui.line_output->setDisabled(false);
    ui.line_path->setText("");
    ui.line_output->setText("");
  }
}

void TabMo::updateGUI()
{
  m_updateGuiInProgress = true;
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_opt);

  // Objectives
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

bool TabMo::updateOptimizationInfo()
{
  if (m_updateGuiInProgress)
    return true;

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_opt);

  // Objectives/hardness; the process_objectives_info is called
  // here to properly initialize the number of objectives and
  // the hardness weight.
  xtalopt->m_objectivesReDo = ui.cb_redo_objectives->isChecked();
  bool ret = xtalopt->processObjectivesInfo();
  updateObjectivesTable();

  return ret;
}

void TabMo::addObjectives()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_opt);

  // A copy: for restoring table if anything goes wrong!
  QStringList tmpobjectiveslst;
  for(int i = 0; i < xtalopt->objectiveListSize(); i++)
   tmpobjectiveslst.push_back(xtalopt->objectiveListGet(i));

  QString value_type = ui.combo_type->currentText();
  QString value_path = ui.line_path->text();
  QString value_outf = ui.line_output->text();
  QString value_wegt = QString::number(ui.sb_weight->value());

  if (value_path.split(" ", QString::SkipEmptyParts).size() != 1
      || value_outf.split(" ", QString::SkipEmptyParts).size() != 1)
  {
    errorPromptWindow("Invalid user-defined script/output file name!");
    return;
  }

  QString ftxt = value_type + " " + value_path + " " + value_outf + " " + value_wegt;

  xtalopt->objectiveListAdd(ftxt);

  // If anything goes wrong with objective/weights; restore last ok info!
  if (!updateOptimizationInfo())
  {
    errorPromptWindow("Total weight can't exceed 1.0!");
    xtalopt->objectiveListClear();
    for (int i = 0; i < tmpobjectiveslst.size(); i++)
      xtalopt->objectiveListAdd(tmpobjectiveslst.at(i));
    updateOptimizationInfo();
  }

  // Clean up the entry fields in "Add Objective" after adding a objective
  //ui.line_path->setText("/path_to/script");
  //ui.line_output->setText("/path_to/output_file");
  ui.line_output->clear();
  ui.line_path->clear();
  ui.sb_weight->setValue(0.0);
  ui.combo_type->setCurrentIndex(0);
  ui.push_addObjectives->setDefault(false);
}

void TabMo::removeObjectives()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_opt);

  // A copy: for restoring table if anything goes wrong!
  QStringList tmpobjectiveslst;
  for(int i = 0; i < xtalopt->objectiveListSize(); i++)
    tmpobjectiveslst.push_back(xtalopt->objectiveListGet(i));

  int row = ui.table_objectives->currentRow();
  int tot = ui.table_objectives->rowCount();

  if (tot == 0 || row < 0 || row > tot)
    return;

  xtalopt->objectiveListRemove(row);

  // If anything goes wrong with objective/weights; restore last ok info!
  if (!updateOptimizationInfo())
  {
    errorPromptWindow("Total weight can't exceed 1.0!");
    xtalopt->objectiveListClear();
    for (int i = 0; i < tmpobjectiveslst.size(); i++)
      xtalopt->objectiveListAdd(tmpobjectiveslst.at(i));
    updateOptimizationInfo();
  }
  ui.push_removeObjectives->setDefault(false);
}

void TabMo::updateObjectivesTable()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_opt);

  // Adjust the table size.
  int numRows = xtalopt->objectiveListSize();

  ui.table_objectives->setRowCount(numRows);

  for (int i = 0; i < numRows; i++) {
    QStringList fline = xtalopt->objectiveListGet(i).split(" ", QString::SkipEmptyParts);
    QTableWidgetItem* value_type = new QTableWidgetItem(fline[0]);
    QTableWidgetItem* value_path = new QTableWidgetItem(fline[1]);
    QTableWidgetItem* value_outf = new QTableWidgetItem(fline[2]);
    QTableWidgetItem* value_wegt = new QTableWidgetItem(fline[3]);

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

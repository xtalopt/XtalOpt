/**********************************************************************
  TabOpt - Parameters to control the run of the search

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <gapc/ui/tab_opt.h>

#include <gapc/gapc.h>
#include <gapc/ui/dialog.h>

#include <globalsearch/macros.h>

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

using namespace std;

namespace GAPC {

TabOpt::TabOpt(GAPCDialog* parent, OptGAPC* p) : AbstractTab(parent, p)
{
  ui.setupUi(m_tab_widget);

  // Optimization connections
  // Initial generation
  connect(ui.spin_numInitial, SIGNAL(valueChanged(int)), this,
          SLOT(updateOptimizationInfo()));
  // Seeds
  connect(ui.push_addSeed, SIGNAL(clicked()), this, SLOT(addSeed()));
  connect(ui.push_removeSeed, SIGNAL(clicked()), this, SLOT(removeSeed()));
  connect(ui.list_seeds, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this,
          SLOT(addSeed(QListWidgetItem*)));

  // Search params
  connect(ui.spin_popSize, SIGNAL(valueChanged(int)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_contStructs, SIGNAL(valueChanged(int)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.cb_limitRunningJobs, SIGNAL(stateChanged(int)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_runningJobLimit, SIGNAL(valueChanged(int)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_failLimit, SIGNAL(valueChanged(int)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.combo_failAction, SIGNAL(currentIndexChanged(int)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_explode, SIGNAL(valueChanged(double)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.combo_explode, SIGNAL(currentIndexChanged(int)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_p_cross, SIGNAL(valueChanged(int)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_p_twist, SIGNAL(valueChanged(int)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_twist_minRot, SIGNAL(valueChanged(int)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_p_exch, SIGNAL(valueChanged(int)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_p_randw, SIGNAL(valueChanged(int)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_twist_minRot, SIGNAL(valueChanged(int)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_exch_numExch, SIGNAL(valueChanged(int)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_randw_numWalkers, SIGNAL(valueChanged(int)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_randw_minWalk, SIGNAL(valueChanged(double)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_randw_maxWalk, SIGNAL(valueChanged(double)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_p_aniso, SIGNAL(valueChanged(int)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_aniso_amp, SIGNAL(valueChanged(double)), this,
          SLOT(updateOptimizationInfo()));

  // Duplicate tolerances
  connect(ui.spin_tol_enthalpy, SIGNAL(editingFinished()), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_tol_geo, SIGNAL(editingFinished()), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.push_dup_reset, SIGNAL(clicked()), m_opt, SLOT(resetDuplicates()));

  initialize();
}

TabOpt::~TabOpt()
{
}

void TabOpt::writeSettings(const QString& filename)
{
  SETTINGS(filename);

  OptGAPC* gapc = qobject_cast<OptGAPC*>(m_opt);

  settings->beginGroup("gapc/opt/");

  // config version
  const int version = 1;
  settings->setValue("version", version);

  // Initial generation
  settings->setValue("numInitial", gapc->numInitial);

  // Search parameters
  settings->setValue("popSize", gapc->popSize);
  settings->setValue("contStructs", gapc->contStructs);
  settings->setValue("limitRunningJobs", gapc->limitRunningJobs);
  settings->setValue("runningJobLimit", gapc->runningJobLimit);
  settings->setValue("failLimit", gapc->failLimit);
  settings->setValue("failAction", gapc->failAction);
  settings->setValue("explodeLimit", gapc->explodeLimit);
  settings->setValue("explodeAction", gapc->explodeAction);

  // Duplicates
  settings->setValue("tol/enthalpy", gapc->tol_enthalpy);
  settings->setValue("tol/geo", gapc->tol_geo);

  // Crossover
  settings->setValue("p_cross", gapc->p_cross);

  // Twist
  settings->setValue("p_twist", gapc->p_twist);
  settings->setValue("twist_minRot", gapc->twist_minRot);

  // Exchange
  settings->setValue("p_exch", gapc->p_exch);
  settings->setValue("exch_numExch", gapc->exch_numExch);

  // Random Walk
  settings->setValue("p_randw", gapc->p_randw);
  settings->setValue("randw_numWalkers", gapc->randw_numWalkers);
  settings->setValue("randw_minWalk", gapc->randw_minWalk);
  settings->setValue("randw_maxWalk", gapc->randw_maxWalk);

  // Anisotropic Expansion
  settings->setValue("p_aniso", gapc->p_aniso);
  settings->setValue("aniso_amp", gapc->aniso_amp);

  settings->endGroup();

  DESTROY_SETTINGS(filename);
}

void TabOpt::readSettings(const QString& filename)
{
  SETTINGS(filename);

  settings->beginGroup("gapc/opt/");

  // Config version
  int loadedVersion = settings->value("version", 0).toInt();

  // Initial generation
  ui.spin_numInitial->setValue(settings->value("numInitial", 20).toInt());

  // Search parameters
  ui.spin_popSize->setValue(settings->value("popSize", 20).toUInt());
  ui.spin_contStructs->setValue(settings->value("contStructs", 10).toUInt());
  ui.cb_limitRunningJobs->setChecked(
    settings->value("limitRunningJobs", false).toBool());
  ui.spin_runningJobLimit->setValue(
    settings->value("runningJobLimit", 1).toUInt());
  ui.spin_failLimit->setValue(settings->value("failLimit", 2).toUInt());
  ui.combo_failAction->setCurrentIndex(
    settings->value("failAction", OptGAPC::FA_Randomize).toUInt());
  ui.spin_explode->setValue(settings->value("explodeLimit", 5.0).toDouble());
  ui.combo_explode->setCurrentIndex(
    settings->value("explodeAction", OptGAPC::EA_Randomize).toUInt());

  // Duplicates
  ui.spin_tol_enthalpy->setValue(
    settings->value("tol/enthalpy", 1e-2).toDouble());
  ui.spin_tol_geo->setValue(settings->value("tol/geo", 5e2).toDouble());

  // Crossover
  ui.spin_p_cross->setValue(settings->value("p_cross", 10).toInt());

  // Twist
  ui.spin_p_twist->setValue(settings->value("p_twist", 20).toInt());
  ui.spin_twist_minRot->setValue(settings->value("twist_minRot", 25).toInt());

  // Exchange
  ui.spin_p_exch->setValue(settings->value("p_exch", 20).toInt());
  ui.spin_exch_numExch->setValue(settings->value("exch_numExch", 4).toInt());

  // Random Walk
  ui.spin_p_randw->setValue(settings->value("p_randw", 20).toInt());
  ui.spin_randw_numWalkers->setValue(
    settings->value("randw_numWalkers", 4).toInt());
  ui.spin_randw_minWalk->setValue(
    settings->value("randw_minWalk", 1.00).toDouble());
  ui.spin_randw_maxWalk->setValue(
    settings->value("randw_maxWalk", 5.00).toDouble());

  // Anisotropic Expansion
  ui.spin_p_aniso->setValue(settings->value("p_aniso", 30).toInt());
  ui.spin_aniso_amp->setValue(settings->value("aniso_amp", 1.0).toDouble());

  settings->endGroup();

  // Update config data
  switch (loadedVersion) {
    case 0:
    case 1:
    default:
      break;
  }

  updateOptimizationInfo();
}

void TabOpt::updateGUI()
{
  OptGAPC* gapc = qobject_cast<OptGAPC*>(m_opt);

  // Initial generation
  ui.spin_numInitial->setValue(gapc->numInitial);

  // Search parameters
  ui.spin_popSize->setValue(gapc->popSize);
  ui.spin_contStructs->setValue(gapc->contStructs);
  ui.cb_limitRunningJobs->setChecked(gapc->limitRunningJobs);
  ui.spin_runningJobLimit->setValue(gapc->runningJobLimit);
  ui.spin_failLimit->setValue(gapc->failLimit);
  ui.combo_failAction->setCurrentIndex(gapc->failAction);
  ui.spin_explode->setValue(gapc->explodeLimit);
  ui.combo_explode->setCurrentIndex(gapc->explodeAction);

  // Duplicates
  ui.spin_tol_enthalpy->setValue(gapc->tol_enthalpy);
  ui.spin_tol_geo->setValue(gapc->tol_geo);

  // Crossover
  ui.spin_p_cross->setValue(gapc->p_cross);

  // Twist
  ui.spin_p_twist->setValue(gapc->p_twist);
  ui.spin_twist_minRot->setValue(gapc->twist_minRot);

  // Exchange
  ui.spin_p_exch->setValue(gapc->p_exch);
  ui.spin_exch_numExch->setValue(gapc->exch_numExch);

  // Random Walk
  ui.spin_p_randw->setValue(gapc->p_randw);
  ui.spin_randw_numWalkers->setValue(gapc->randw_numWalkers);
  ui.spin_randw_minWalk->setValue(gapc->randw_minWalk);
  ui.spin_randw_maxWalk->setValue(gapc->randw_maxWalk);

  // Anisotropic Expansion
  ui.spin_p_aniso->setValue(gapc->p_aniso);
  ui.spin_aniso_amp->setValue(gapc->aniso_amp);
}

void TabOpt::lockGUI()
{
  ui.spin_numInitial->setDisabled(true);
  ui.list_seeds->setDisabled(true);
  ui.push_addSeed->setDisabled(true);
  ui.push_addSeed->setDisabled(true);
  ui.push_removeSeed->setDisabled(true);
}

void TabOpt::updateOptimizationInfo()
{
  OptGAPC* gapc = qobject_cast<OptGAPC*>(m_opt);

  // See if any of the spin boxes caused this change
  if (sender() == ui.spin_p_cross || sender() == ui.spin_p_twist ||
      sender() == ui.spin_p_exch || sender() == ui.spin_p_randw) {
    gapc->p_cross = ui.spin_p_cross->value();
    gapc->p_twist = ui.spin_p_twist->value();
    gapc->p_exch = ui.spin_p_exch->value();
    gapc->p_randw = ui.spin_p_randw->value();
    gapc->p_aniso =
      100 - (gapc->p_cross + gapc->p_twist + gapc->p_exch + gapc->p_randw);
    ui.spin_p_aniso->blockSignals(true);
    ui.spin_p_aniso->setValue(gapc->p_aniso);
    ui.spin_p_aniso->blockSignals(false);
  } else if (sender() == ui.spin_p_aniso) {
    gapc->p_aniso = ui.spin_p_aniso->value();
    gapc->p_twist = ui.spin_p_twist->value();
    gapc->p_exch = ui.spin_p_exch->value();
    gapc->p_randw = ui.spin_p_randw->value();
    gapc->p_cross =
      100 - (gapc->p_aniso + gapc->p_twist + gapc->p_exch + gapc->p_randw);
    ui.spin_p_cross->blockSignals(true);
    ui.spin_p_cross->setValue(gapc->p_cross);
    ui.spin_p_cross->blockSignals(false);
  } else {
    gapc->p_cross = ui.spin_p_cross->value();
    gapc->p_twist = ui.spin_p_twist->value();
    gapc->p_exch = ui.spin_p_exch->value();
    gapc->p_randw = ui.spin_p_randw->value();
    gapc->p_aniso = ui.spin_p_aniso->value();
  }

  // Initial generation
  gapc->numInitial = ui.spin_numInitial->value();
  if (int(gapc->numInitial) < ui.list_seeds->count())
    ui.spin_numInitial->setValue(ui.list_seeds->count());

  // Search parameters
  gapc->popSize = ui.spin_popSize->value();
  gapc->contStructs = ui.spin_contStructs->value();
  gapc->runningJobLimit = ui.spin_runningJobLimit->value();
  gapc->limitRunningJobs = ui.cb_limitRunningJobs->isChecked();
  gapc->failLimit = ui.spin_failLimit->value();
  gapc->failAction = OptGAPC::FailActions(ui.combo_failAction->currentIndex());
  gapc->explodeLimit = ui.spin_explode->value();
  gapc->explodeAction =
    OptGAPC::ExplodeActions(ui.combo_explode->currentIndex());

  // Duplicates
  gapc->tol_enthalpy = ui.spin_tol_enthalpy->value();
  gapc->tol_geo = ui.spin_tol_geo->value();

  // Crossover
  // p_cross set above

  // Twist
  // p_twist set above
  gapc->twist_minRot = ui.spin_twist_minRot->value();

  // Exchange
  // p_exch set above
  gapc->exch_numExch = ui.spin_exch_numExch->value();

  // Random Walk
  // p_randw set above
  gapc->randw_numWalkers = ui.spin_randw_numWalkers->value();
  gapc->randw_minWalk = ui.spin_randw_minWalk->value();
  gapc->randw_maxWalk = ui.spin_randw_maxWalk->value();

  // Anisotropic Expansion
  // p_aniso set above
  gapc->aniso_amp = ui.spin_aniso_amp->value();
}

void TabOpt::addSeed(QListWidgetItem* item)
{
  QSettings settings; // Already set up in avogadro/src/main.cpp
  QString filename("");
  bool replace = false;
  if (item)
    replace = true;

  // Set filename
  if (replace) {
    filename = item->text();
  } else {
    filename = settings.value("gapc/opt/seedPath", m_opt->filePath).toString();
  }

  // Launch file dialog
  QFileDialog dialog(m_dialog, QString("Select structure file to use as seed"),
                     filename, "All Files (*)");
  dialog.selectFile(filename);
  dialog.setFileMode(QFileDialog::ExistingFile);
  if (dialog.exec())
    filename = dialog.selectedFiles().first();
  else {
    return;
  } // User cancel file selection.

  settings.setValue("gapc/opt/seedPath", filename);

  // Update text
  if (replace)
    item->setText(filename);
  else
    ui.list_seeds->addItem(filename);
  updateOptimizationInfo();
  updateSeeds();
}

void TabOpt::removeSeed()
{
  if (ui.list_seeds->count() == 0)
    return;
  delete ui.list_seeds->takeItem(ui.list_seeds->currentRow());
  updateSeeds();
}

void TabOpt::updateSeeds()
{
  OptGAPC* gapc = qobject_cast<OptGAPC*>(m_opt);

  gapc->seedList.clear();
  for (int i = 0; i < ui.list_seeds->count(); i++)
    gapc->seedList.append(ui.list_seeds->item(i)->text());
}
}

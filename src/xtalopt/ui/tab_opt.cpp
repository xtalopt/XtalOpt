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

#include <xtalopt/ui/tab_opt.h>

#include <xtalopt/ui/dialog.h>
#include <xtalopt/xtalopt.h>

#include <QDebug>
#include <QSettings>

#include <QFileDialog>
#include <QMessageBox>

using namespace std;

namespace XtalOpt {

TabOpt::TabOpt(GlobalSearch::AbstractDialog* parent, XtalOpt* p)
  : AbstractTab(parent, p)
{
  ui.setupUi(m_tab_widget);

  // Before we make any connections, let's read the settings
  readSettings();

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
  connect(ui.spin_cutoff, SIGNAL(valueChanged(int)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.cb_using_FU_crossovers, SIGNAL(toggled(bool)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_FU_crossovers_generation, SIGNAL(valueChanged(int)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.cb_using_mitotic_growth, SIGNAL(toggled(bool)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.cb_using_one_pool, SIGNAL(toggled(bool)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_chance_of_mitosis, SIGNAL(valueChanged(int)), this,
          SLOT(updateOptimizationInfo()));

  // Duplicate tolerances
  connect(ui.spin_tol_xcLength, SIGNAL(editingFinished()), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_tol_xcAngle, SIGNAL(editingFinished()), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_tol_spg, SIGNAL(editingFinished()), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.push_dup_reset, SIGNAL(clicked()), m_opt, SLOT(resetDuplicates()));
  connect(ui.push_spg_reset, SIGNAL(clicked()), m_opt,
          SLOT(resetSpacegroups()));

  // Crossover
  connect(ui.spin_p_cross, SIGNAL(valueChanged(int)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_cross_minimumContribution, SIGNAL(valueChanged(int)), this,
          SLOT(updateOptimizationInfo()));

  // Stripple
  connect(ui.spin_p_strip, SIGNAL(valueChanged(int)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_strip_strainStdev_min, SIGNAL(valueChanged(double)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_strip_strainStdev_max, SIGNAL(valueChanged(double)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_strip_amp_min, SIGNAL(valueChanged(double)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_strip_amp_max, SIGNAL(valueChanged(double)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_strip_per1, SIGNAL(valueChanged(int)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_strip_per2, SIGNAL(valueChanged(int)), this,
          SLOT(updateOptimizationInfo()));

  // Permustrain
  connect(ui.spin_p_perm, SIGNAL(valueChanged(int)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_perm_strainStdev_max, SIGNAL(valueChanged(double)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_perm_ex, SIGNAL(valueChanged(int)), this,
          SLOT(updateOptimizationInfo()));

  // Hardness stuff
  connect(ui.cb_calculateHardness, SIGNAL(toggled(bool)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.cb_calculateHardness, &QCheckBox::toggled, m_opt,
          &GlobalSearch::OptBase::resubmitUnfinishedHardnessCalcs);
  connect(ui.cb_calculateHardness, &QCheckBox::toggled, m_opt,
          &GlobalSearch::OptBase::startHardnessResubmissionThread);
  connect(ui.spin_hardnessFitnessWeight, SIGNAL(valueChanged(double)), this,
          SLOT(updateOptimizationInfo()));

  initialize();
}

TabOpt::~TabOpt()
{
}

void TabOpt::writeSettings(const QString& filename)
{
}

void TabOpt::readSettings(const QString& filename)
{
  updateGUI();
}

void TabOpt::updateGUI()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_opt);

  // Initial generation
  ui.spin_numInitial->setValue(xtalopt->numInitial);

  // Search parameters
  ui.spin_popSize->setValue(xtalopt->popSize);
  ui.spin_contStructs->setValue(xtalopt->contStructs);
  ui.cb_limitRunningJobs->setChecked(xtalopt->limitRunningJobs);
  ui.spin_runningJobLimit->setValue(xtalopt->runningJobLimit);
  ui.spin_failLimit->setValue(xtalopt->failLimit);
  ui.combo_failAction->setCurrentIndex(xtalopt->failAction);
  ui.spin_cutoff->setValue(xtalopt->cutoff);
  ui.cb_using_mitotic_growth->setChecked(xtalopt->using_mitotic_growth);
  ui.cb_using_FU_crossovers->setChecked(xtalopt->using_FU_crossovers);
  ui.spin_FU_crossovers_generation->setValue(xtalopt->FU_crossovers_generation);
  ui.cb_using_one_pool->setChecked(xtalopt->using_one_pool);
  ui.spin_chance_of_mitosis->setValue(xtalopt->chance_of_mitosis);

  // Duplicates
  ui.spin_tol_xcLength->setValue(xtalopt->tol_xcLength);
  ui.spin_tol_xcAngle->setValue(xtalopt->tol_xcAngle);
  ui.spin_tol_spg->setValue(xtalopt->tol_spg);

  // Crossover
  ui.spin_p_cross->setValue(xtalopt->p_cross);
  ui.spin_cross_minimumContribution->setValue(
    xtalopt->cross_minimumContribution);

  // Stripple
  ui.spin_p_strip->setValue(xtalopt->p_strip);
  ui.spin_strip_strainStdev_min->setValue(xtalopt->strip_strainStdev_min);
  ui.spin_strip_strainStdev_max->setValue(xtalopt->strip_strainStdev_max);
  ui.spin_strip_amp_min->setValue(xtalopt->strip_amp_min);
  ui.spin_strip_amp_max->setValue(xtalopt->strip_amp_max);
  ui.spin_strip_per1->setValue(xtalopt->strip_per1);
  ui.spin_strip_per2->setValue(xtalopt->strip_per2);

  // Permustrain
  ui.spin_p_perm->setValue(xtalopt->p_perm);
  ui.spin_perm_strainStdev_max->setValue(xtalopt->perm_strainStdev_max);
  ui.spin_perm_ex->setValue(xtalopt->perm_ex);

  // Block this signal so we don't start a resubmission thread
  bool wasBlocked = ui.cb_calculateHardness->blockSignals(true);
  ui.cb_calculateHardness->setChecked(xtalopt->m_calculateHardness.load());
  ui.cb_calculateHardness->blockSignals(wasBlocked);

  ui.spin_hardnessFitnessWeight->setEnabled(ui.cb_calculateHardness->isChecked());
  ui.spin_hardnessFitnessWeight->setValue(
    xtalopt->m_hardnessFitnessWeight * 100.0);
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
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_opt);

  // See if the spin boxes caused this change.
  if (sender() == ui.spin_p_cross || sender() == ui.spin_p_strip) {
    xtalopt->p_cross = ui.spin_p_cross->value();
    xtalopt->p_strip = ui.spin_p_strip->value();
    xtalopt->p_perm = 100 - (xtalopt->p_cross + xtalopt->p_strip);
    ui.spin_p_perm->blockSignals(true);
    ui.spin_p_perm->setValue(xtalopt->p_perm);
    ui.spin_p_perm->blockSignals(false);
  } else if (sender() == ui.spin_p_perm) {
    xtalopt->p_perm = ui.spin_p_perm->value();
    xtalopt->p_strip = ui.spin_p_strip->value();
    xtalopt->p_cross = 100 - (xtalopt->p_perm + xtalopt->p_strip);
    ui.spin_p_cross->blockSignals(true);
    ui.spin_p_cross->setValue(xtalopt->p_cross);
    ui.spin_p_cross->blockSignals(false);
  } else {
    xtalopt->p_perm = ui.spin_p_perm->value();
    xtalopt->p_strip = ui.spin_p_strip->value();
    xtalopt->p_cross = ui.spin_p_cross->value();
  }

  // Initial generation
  xtalopt->numInitial = ui.spin_numInitial->value();
  if (int(xtalopt->numInitial) < ui.list_seeds->count())
    ui.spin_numInitial->setValue(ui.list_seeds->count());

  // Search parameters
  xtalopt->popSize = ui.spin_popSize->value();
  xtalopt->contStructs = ui.spin_contStructs->value();
  xtalopt->runningJobLimit = ui.spin_runningJobLimit->value();
  xtalopt->limitRunningJobs = ui.cb_limitRunningJobs->isChecked();
  xtalopt->failLimit = ui.spin_failLimit->value();
  xtalopt->failAction =
    XtalOpt::FailActions(ui.combo_failAction->currentIndex());
  xtalopt->cutoff = ui.spin_cutoff->value();
  xtalopt->using_mitotic_growth = ui.cb_using_mitotic_growth->isChecked();
  xtalopt->using_FU_crossovers = ui.cb_using_FU_crossovers->isChecked();
  xtalopt->FU_crossovers_generation = ui.spin_FU_crossovers_generation->value();
  xtalopt->using_one_pool = ui.cb_using_one_pool->isChecked();
  xtalopt->chance_of_mitosis = ui.spin_chance_of_mitosis->value();

  // Duplicates
  xtalopt->tol_xcLength = ui.spin_tol_xcLength->value();
  xtalopt->tol_xcAngle = ui.spin_tol_xcAngle->value();
  xtalopt->tol_spg = ui.spin_tol_spg->value();

  // Crossover
  xtalopt->cross_minimumContribution =
    ui.spin_cross_minimumContribution->value();

  // Stripple
  xtalopt->strip_strainStdev_min = ui.spin_strip_strainStdev_min->value();
  xtalopt->strip_strainStdev_max = ui.spin_strip_strainStdev_max->value();
  xtalopt->strip_amp_min = ui.spin_strip_amp_min->value();
  xtalopt->strip_amp_max = ui.spin_strip_amp_max->value();
  xtalopt->strip_per1 = ui.spin_strip_per1->value();
  xtalopt->strip_per2 = ui.spin_strip_per2->value();

  // Permustrain
  xtalopt->perm_strainStdev_max = ui.spin_perm_strainStdev_max->value();
  xtalopt->perm_ex = ui.spin_perm_ex->value();

  // Hardness stuff
  xtalopt->m_calculateHardness = ui.cb_calculateHardness->isChecked();
  xtalopt->m_hardnessFitnessWeight =
    ui.spin_hardnessFitnessWeight->value() / 100.0;
}

void TabOpt::addSeed(QListWidgetItem* item)
{
  // qDebug() << "TabOpt::addSeed( " << item << " ) called";
  QSettings settings;
  QString filename("");
  bool replace = false;
  if (item)
    replace = true;

  // Set filename
  if (replace) {
    filename = item->text();
  } else {
    filename =
      settings.value("xtalopt/opt/seedPath", m_opt->filePath + "/POSCAR")
        .toString();
  }

  // Launch file dialog
  QString newFilename = QFileDialog::getOpenFileName(
    m_dialog, QString("Select structure file to use as seed"), filename,
    "Common formats (*POSCAR *CONTCAR *.got *.cml *cif"
    " *.out);;All Files (*)",
    0, QFileDialog::DontUseNativeDialog);

  // User canceled
  if (newFilename.isEmpty())
    return;

  settings.setValue("xtalopt/opt/seedPath", newFilename);

  // Update text
  if (replace)
    item->setText(newFilename);
  else
    ui.list_seeds->addItem(newFilename);
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
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_opt);

  xtalopt->seedList.clear();
  for (int i = 0; i < ui.list_seeds->count(); i++)
    xtalopt->seedList.append(ui.list_seeds->item(i)->text());
}
}

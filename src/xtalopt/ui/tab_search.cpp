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

#include <xtalopt/ui/tab_search.h>

#include <xtalopt/ui/dialog.h>
#include <xtalopt/xtalopt.h>

#include <globalsearch/random.h>

#include <QDebug>
#include <QSettings>

#include <QFileDialog>
#include <QMessageBox>

using namespace std;

namespace XtalOpt {

TabSearch::TabSearch(GlobalSearch::AbstractDialog* parent, XtalOpt* p)
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
  connect(ui.spin_parentsPoolSize, SIGNAL(valueChanged(int)), this,
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
  connect(ui.cb_saveHulls, SIGNAL(stateChanged(int)), this,
          SLOT(updateOptimizationInfo()));


  // Spglib
  connect(ui.spin_tol_spg, SIGNAL(editingFinished()), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.push_spg_reset, SIGNAL(clicked()), m_search, SLOT(resetSpacegroups()));

  // XtalComp similarity tolerances
  connect(ui.spin_tol_xcLength, SIGNAL(editingFinished()), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_tol_xcAngle, SIGNAL(editingFinished()), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.push_sim_reset, SIGNAL(clicked()), m_search, SLOT(resetSimilarities()));

  // RDF similarity parameters
  connect(ui.spin_rdf_tol, SIGNAL(editingFinished()), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_rdf_cut, SIGNAL(editingFinished()), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_rdf_sig, SIGNAL(editingFinished()), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_rdf_bin, SIGNAL(editingFinished()), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.push_sim_reset_2, SIGNAL(clicked(bool)), m_search, SLOT(resetSimilarities()));

  // Crossover
  connect(ui.spin_p_cross, SIGNAL(editingFinished()), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_cross_minimumContribution, SIGNAL(valueChanged(int)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.sb_ncuts, SIGNAL(valueChanged(int)), this,
          SLOT(updateOptimizationInfo()));

  // Stripple
  connect(ui.spin_p_strip, SIGNAL(editingFinished()), this,
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
  connect(ui.spin_p_perm, SIGNAL(editingFinished()), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_perm_strainStdev_max, SIGNAL(valueChanged(double)), this,
          SLOT(updateOptimizationInfo()));
  connect(ui.spin_perm_ex, SIGNAL(valueChanged(int)), this,
          SLOT(updateOptimizationInfo()));

  // Permutomic
  connect(ui.spin_p_atom, SIGNAL(editingFinished()), this,
          SLOT(updateOptimizationInfo()));

  // Permucomp
  connect(ui.spin_p_comp, SIGNAL(editingFinished()), this,
          SLOT(updateOptimizationInfo()));

  // Random supercell generation
  connect(ui.sb_rand_supercell, SIGNAL(editingFinished()), this,
          SLOT(updateOptimizationInfo()));

  initialize();
}

TabSearch::~TabSearch()
{
}

void TabSearch::writeSettings(const QString& filename)
{
}

void TabSearch::readSettings(const QString& filename)
{
  updateGUI();
}

void TabSearch::updateGUI()
{
  m_updateGuiInProgress = true;
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  // Initial generation
  ui.spin_numInitial->setValue(xtalopt->numInitial);

  // Search parameters
  ui.spin_parentsPoolSize->setValue(xtalopt->parentsPoolSize);
  ui.spin_contStructs->setValue(xtalopt->contStructs);
  ui.cb_limitRunningJobs->setChecked(xtalopt->limitRunningJobs);
  ui.spin_runningJobLimit->setEnabled(xtalopt->limitRunningJobs);
  ui.spin_runningJobLimit->setValue(xtalopt->runningJobLimit);
  ui.spin_failLimit->setValue(xtalopt->failLimit);
  ui.combo_failAction->setCurrentIndex(xtalopt->failAction);
  ui.spin_cutoff->setValue(xtalopt->maxNumStructures);
  ui.cb_saveHulls->setChecked(xtalopt->m_saveHullSnapshots);

  // Spglib tolerance
  ui.spin_tol_spg->setValue(xtalopt->tol_spg);

  // XtalComp similarities
  ui.spin_tol_xcLength->setValue(xtalopt->tol_xcLength);
  ui.spin_tol_xcAngle->setValue(xtalopt->tol_xcAngle);

  // RDF similarities
  ui.spin_rdf_tol->setValue(xtalopt->tol_rdf);
  ui.spin_rdf_cut->setValue(xtalopt->tol_rdf_cutoff);
  ui.spin_rdf_sig->setValue(xtalopt->tol_rdf_sigma);
  ui.spin_rdf_bin->setValue(xtalopt->tol_rdf_nbins);

  // Crossover
  ui.spin_p_cross->setValue(xtalopt->p_cross);
  ui.sb_ncuts->setValue(xtalopt->cross_ncuts);
  ui.spin_cross_minimumContribution->setValue(
    xtalopt->cross_minimumContribution);
  if (xtalopt->cross_ncuts > 1) {
    ui.spin_cross_minimumContribution->setEnabled(false);
  } else {
    ui.spin_cross_minimumContribution->setEnabled(true);
  }

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

  // Permutomic
  ui.spin_p_atom->setValue(xtalopt->p_atomic);

  // Permucomp
  ui.spin_p_comp->setValue(xtalopt->p_comp);

  // Random supercell generation
  ui.sb_rand_supercell->setValue(xtalopt->p_supercell);

  m_updateGuiInProgress = false;
}

void TabSearch::lockGUI()
{
  ui.spin_numInitial->setDisabled(true);
  ui.list_seeds->setDisabled(true);
  ui.push_addSeed->setDisabled(true);
  ui.push_addSeed->setDisabled(true);
  ui.push_removeSeed->setDisabled(true);
  // RDF details are not runtime adjustable
  ui.spin_rdf_bin->setDisabled(true);
  ui.spin_rdf_cut->setDisabled(true);
  ui.spin_rdf_sig->setDisabled(true);
}

void TabSearch::updateOptimizationInfo()
{
  if (m_updateGuiInProgress)
    return;

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  // Initial generation
  xtalopt->numInitial = ui.spin_numInitial->value();
  if (int(xtalopt->numInitial) < ui.list_seeds->count())
    ui.spin_numInitial->setValue(ui.list_seeds->count());

  // Search parameters
  xtalopt->parentsPoolSize = ui.spin_parentsPoolSize->value();
  xtalopt->contStructs = ui.spin_contStructs->value();
  xtalopt->runningJobLimit = ui.spin_runningJobLimit->value();
  xtalopt->limitRunningJobs = ui.cb_limitRunningJobs->isChecked();
  xtalopt->failLimit = ui.spin_failLimit->value();
  xtalopt->failAction =
    XtalOpt::FailActions(ui.combo_failAction->currentIndex());
  xtalopt->maxNumStructures = ui.spin_cutoff->value();
  xtalopt->m_saveHullSnapshots = ui.cb_saveHulls->isChecked();

  // Make sure running job limit is properly accessable!
  ui.spin_runningJobLimit->setEnabled(xtalopt->limitRunningJobs);

  // Spglib tolerance
  xtalopt->tol_xcLength = ui.spin_tol_xcLength->value();

  // XtalComp similarities
  xtalopt->tol_xcAngle = ui.spin_tol_xcAngle->value();
  xtalopt->tol_spg = ui.spin_tol_spg->value();

  // RDF similarities
  xtalopt->tol_rdf = ui.spin_rdf_tol->value();
  xtalopt->tol_rdf_cutoff = ui.spin_rdf_cut->value();
  xtalopt->tol_rdf_sigma = ui.spin_rdf_sig->value();
  xtalopt->tol_rdf_nbins = ui.spin_rdf_bin->value();

  // Operation weights
  xtalopt->p_cross = ui.spin_p_cross->value();
  xtalopt->p_strip = ui.spin_p_strip->value();
  xtalopt->p_perm = ui.spin_p_perm->value();
  xtalopt->p_atomic = ui.spin_p_atom->value();
  xtalopt->p_comp = ui.spin_p_comp->value();

  // Crossover
  xtalopt->cross_ncuts = ui.sb_ncuts->value();
  xtalopt->cross_minimumContribution =
    ui.spin_cross_minimumContribution->value();
  if (xtalopt->cross_ncuts > 1) {
    ui.spin_cross_minimumContribution->setEnabled(false);
  } else {
    ui.spin_cross_minimumContribution->setEnabled(true);
  }

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

  // Random supercell generation
  xtalopt->p_supercell = ui.sb_rand_supercell->value();
}

void TabSearch::addSeed(QListWidgetItem* item)
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
      settings.value("xtalopt/opt/seedPath", m_search->locWorkDir + "/POSCAR")
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

void TabSearch::removeSeed()
{
  if (ui.list_seeds->count() == 0)
    return;
  delete ui.list_seeds->takeItem(ui.list_seeds->currentRow());
  updateSeeds();
}

void TabSearch::showSeeds()
{
  ui.list_seeds->clear();
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);
  for (const auto& s : xtalopt->seedList) {
    ui.list_seeds->addItem(s);
  }
}

void TabSearch::updateSeeds()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  xtalopt->seedList.clear();
  for (int i = 0; i < ui.list_seeds->count(); i++)
    xtalopt->seedList.append(ui.list_seeds->item(i)->text());
}
}

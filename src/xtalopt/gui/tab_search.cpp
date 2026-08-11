/**********************************************************************
  TabSearch - The search-settings tab: edit search parameters and seed structures.

  Copyright (C) 2009-2011 by David Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/gui/tab_search.h>

#include <xtalopt/gui/dialog.h>
#include <xtalopt/xtalopt.h>

#include <common/fileutils.h>
#include <common/random.h>

#include <QAbstractSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDir>
#include <QFileDialog>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>

using namespace std;

namespace XtalOpt {

TabSearch::TabSearch(Search::AbstractDialog* parent, XtalOpt* p)
  : AbstractTab(parent, p)
{
  ui.setupUi(m_tab_widget);

  updateGUI();

  XtalOpt* xo = qobject_cast<XtalOpt*>(m_search);

  // Optimization connections
  // Initial generation
  connect(ui.spin_numInitial, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this, &TabSearch::updateOptimizationInfo);

  // Seeds
  connect(ui.push_addSeed,    &QPushButton::clicked, this, [this]() { addSeed(); });
  connect(ui.push_removeSeed, &QPushButton::clicked, this, &TabSearch::removeSeed);
  connect(ui.list_seeds, &QListWidget::itemDoubleClicked, this, &TabSearch::addSeed);

  // Search params
  connect(ui.spin_parentsPoolSize, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this, &TabSearch::updateOptimizationInfo);
  connect(ui.spin_contStructs, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this, &TabSearch::updateOptimizationInfo);
  connect(ui.cb_limitRunningJobs, &QCheckBox::toggled, this, &TabSearch::updateOptimizationInfo);
  connect(ui.spin_runningJobLimit, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this, &TabSearch::updateOptimizationInfo);
  connect(ui.spin_failLimit, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this, &TabSearch::updateOptimizationInfo);
  connect(ui.combo_failAction,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this, &TabSearch::updateOptimizationInfo);
  connect(ui.spin_cutoff, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this, &TabSearch::updateOptimizationInfo);
  connect(ui.cb_saveHulls, &QCheckBox::toggled, this, &TabSearch::updateOptimizationInfo);
  connect(ui.cb_tournament,   &QCheckBox::toggled, this, &TabSearch::updateOptTypeInfo);
  connect(ui.cb_restrictPool, &QCheckBox::toggled, this, &TabSearch::updateOptTypeInfo);
  connect(ui.cb_crowding,     &QCheckBox::toggled, this, &TabSearch::updateOptTypeInfo);
  connect(ui.cb_paretoFilterZeroWeights, &QCheckBox::toggled, this, &TabSearch::updateOptTypeInfo);
  connect(ui.combo_optType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this, &TabSearch::updateOptTypeInfo);
  connect(ui.sb_prec, &QAbstractSpinBox::editingFinished, this, &TabSearch::updateOptTypeInfo);


  // Spglib
  connect(ui.spin_tol_spg, &QAbstractSpinBox::editingFinished,
          this, &TabSearch::updateOptimizationInfo);
  connect(ui.push_spg_reset, &QPushButton::clicked,
          this, [this, xo]() {
            updateOptimizationInfo();
            xo->resetSpacegroups();
          });

  // XtalComp similarity tolerances
  connect(ui.spin_tol_xcLength, &QAbstractSpinBox::editingFinished,
          this, &TabSearch::updateOptimizationInfo);
  connect(ui.spin_tol_xcAngle, &QAbstractSpinBox::editingFinished,
          this, &TabSearch::updateOptimizationInfo);
  connect(ui.push_sim_reset, &QPushButton::clicked,
          this, [this, xo]() {
            updateOptimizationInfo();
            xo->resetSimilarities();
          });

  // RDF similarity parameters
  connect(ui.spin_rdf_tol, &QAbstractSpinBox::editingFinished,
          this, &TabSearch::updateOptimizationInfo);
  connect(ui.spin_rdf_cut, &QAbstractSpinBox::editingFinished,
          this, &TabSearch::updateOptimizationInfo);
  connect(ui.spin_rdf_sig, &QAbstractSpinBox::editingFinished,
          this, &TabSearch::updateOptimizationInfo);
  connect(ui.spin_rdf_bin, &QAbstractSpinBox::editingFinished,
          this, &TabSearch::updateOptimizationInfo);
  connect(ui.push_sim_reset_2, &QPushButton::clicked,
          this, [this, xo]() {
            updateOptimizationInfo();
            xo->resetSimilarities();
          });

  // Crossover
  connect(ui.spin_p_cross, &QAbstractSpinBox::editingFinished,
          this, &TabSearch::updateOptimizationInfo);
  connect(ui.spin_cross_minimumContribution,
          static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this, &TabSearch::updateOptimizationInfo);
  connect(ui.sb_ncuts, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this, &TabSearch::updateOptimizationInfo);

  // Stripple
  connect(ui.spin_p_strip, &QAbstractSpinBox::editingFinished,
          this, &TabSearch::updateOptimizationInfo);
  connect(ui.spin_strip_strainStdev_min,
          static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
          this, &TabSearch::updateOptimizationInfo);
  connect(ui.spin_strip_strainStdev_max,
          static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
          this, &TabSearch::updateOptimizationInfo);
  connect(ui.spin_strip_amp_min,
          static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
          this, &TabSearch::updateOptimizationInfo);
  connect(ui.spin_strip_amp_max,
          static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
          this, &TabSearch::updateOptimizationInfo);
  connect(ui.spin_strip_per1, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this, &TabSearch::updateOptimizationInfo);
  connect(ui.spin_strip_per2, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this, &TabSearch::updateOptimizationInfo);

  // Permustrain
  connect(ui.spin_p_perm, &QAbstractSpinBox::editingFinished,
          this, &TabSearch::updateOptimizationInfo);
  connect(ui.spin_perm_strainStdev_max,
          static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
          this, &TabSearch::updateOptimizationInfo);
  connect(ui.spin_perm_ex, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this, &TabSearch::updateOptimizationInfo);

  // Permutomic
  connect(ui.spin_p_atom, &QAbstractSpinBox::editingFinished,
          this, &TabSearch::updateOptimizationInfo);

  // Permucomp
  connect(ui.spin_p_comp, &QAbstractSpinBox::editingFinished,
          this, &TabSearch::updateOptimizationInfo);

  // Random supercell generation
  connect(ui.sb_rand_supercell, &QAbstractSpinBox::editingFinished,
          this, &TabSearch::updateOptimizationInfo);

  initialize();
}

TabSearch::~TabSearch()
{
}

void TabSearch::updateGUI()
{
  m_updateGuiInProgress = true;
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  showSeeds();

  // Initial generation
  ui.spin_numInitial->setValue(xtalopt->getNumInitial());

  // Search parameters
  ui.spin_parentsPoolSize->setValue(xtalopt->getParentsPoolSize());
  ui.spin_contStructs->setValue(xtalopt->getContStructs());
  ui.cb_limitRunningJobs->setChecked(xtalopt->isLimitRunningJobs());
  ui.spin_runningJobLimit->setEnabled(xtalopt->isLimitRunningJobs());
  ui.spin_runningJobLimit->setValue(xtalopt->getRunningJobLimit());
  ui.spin_failLimit->setValue(xtalopt->getFailLimit());
  ui.combo_failAction->setCurrentIndex(xtalopt->getFailAction());
  ui.spin_cutoff->setValue(xtalopt->getMaxNumStructures());
  ui.cb_saveHulls->setChecked(xtalopt->getSaveHullSnapshots());
  ui.cb_tournament->setChecked(xtalopt->isTournamentSelection());
  ui.cb_restrictPool->setChecked(xtalopt->isRestrictedPool());
  ui.cb_crowding->setChecked(xtalopt->isCrowdingDistance());
  ui.cb_paretoFilterZeroWeights->setChecked(xtalopt->isParetoFilterZeroWeights());
  ui.sb_prec->setValue(xtalopt->getObjectivePrecision());

  if (xtalopt->getOptimizationType() == Search::SearchBase::OT_Pareto) {
    ui.combo_optType->setCurrentIndex(1);
    ui.cb_crowding->setEnabled(true);
    ui.cb_paretoFilterZeroWeights->setEnabled(true);
    ui.cb_tournament->setEnabled(true);
  } else if (xtalopt->getOptimizationType() == Search::SearchBase::OT_Basic) {
    ui.combo_optType->setCurrentIndex(0);
    ui.cb_tournament->setDisabled(true);
    ui.cb_crowding->setDisabled(true);
    ui.cb_paretoFilterZeroWeights->setDisabled(true);
  }

  if (xtalopt->getOptimizationType() == Search::SearchBase::OT_Pareto && ui.cb_tournament->isChecked()) {
    ui.cb_restrictPool->setEnabled(true);
  } else {
    ui.cb_restrictPool->setDisabled(true);
  }

  // Spglib tolerance
  ui.spin_tol_spg->setValue(xtalopt->getTolSpg());

  // XtalComp similarities
  ui.spin_tol_xcLength->setValue(xtalopt->getTolXcLength());
  ui.spin_tol_xcAngle->setValue(xtalopt->getTolXcAngle());

  // RDF similarities
  ui.spin_rdf_tol->setValue(xtalopt->getTolRdf());
  ui.spin_rdf_cut->setValue(xtalopt->getTolRdfCutoff());
  ui.spin_rdf_sig->setValue(xtalopt->getTolRdfSigma());
  ui.spin_rdf_bin->setValue(xtalopt->getTolRdfNbins());

  // Crossover
  ui.spin_p_cross->setValue(xtalopt->getPCross());
  ui.sb_ncuts->setValue(xtalopt->getCrossNcuts());
  ui.spin_cross_minimumContribution->setValue(xtalopt->getCrossMinimumContribution());
  if (xtalopt->getCrossNcuts() > 1) {
    ui.spin_cross_minimumContribution->setEnabled(false);
  } else {
    ui.spin_cross_minimumContribution->setEnabled(true);
  }

  // Stripple
  ui.spin_p_strip->setValue(xtalopt->getPStrip());
  ui.spin_strip_strainStdev_min->setValue(xtalopt->getStripStrainStdevMin());
  ui.spin_strip_strainStdev_max->setValue(xtalopt->getStripStrainStdevMax());
  ui.spin_strip_amp_min->setValue(xtalopt->getStripAmpMin());
  ui.spin_strip_amp_max->setValue(xtalopt->getStripAmpMax());
  ui.spin_strip_per1->setValue(xtalopt->getStripPer1());
  ui.spin_strip_per2->setValue(xtalopt->getStripPer2());

  // Permustrain
  ui.spin_p_perm->setValue(xtalopt->getPPerm());
  ui.spin_perm_strainStdev_max->setValue(xtalopt->getPermStrainStdevMax());
  ui.spin_perm_ex->setValue(xtalopt->getPermEx());

  // Permutomic
  ui.spin_p_atom->setValue(xtalopt->getPAtomic());

  // Permucomp
  ui.spin_p_comp->setValue(xtalopt->getPComp());

  // Random supercell generation
  ui.sb_rand_supercell->setValue(xtalopt->getPSupercell());

  m_updateGuiInProgress = false;
}

bool TabSearch::updateOptTypeInfo()
{
  if (m_updateGuiInProgress)
    return false;

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);
  bool settingsChanged = false;
  bool selectionSettingsChanged = false;
  {
    QWriteLocker runtimeLocker(m_search->runtimeSettingsLock());
    const Settings::ScalarSnapshot before = Settings::captureScalarSettings(*xtalopt);

    xtalopt->setObjectivePrecision(ui.sb_prec->value());
    xtalopt->setTournamentSelection(ui.cb_tournament->isChecked());
    xtalopt->setRestrictedPool(ui.cb_restrictPool->isChecked());
    xtalopt->setCrowdingDistance(ui.cb_crowding->isChecked());
    xtalopt->setParetoFilterZeroWeights(ui.cb_paretoFilterZeroWeights->isChecked());
    xtalopt->setOptimizationTypeText(ui.combo_optType->currentText());
    if (xtalopt->getOptimizationType() == Search::SearchBase::OT_Basic) {
      ui.cb_tournament->setDisabled(true);
      ui.cb_crowding->setDisabled(true);
      ui.cb_paretoFilterZeroWeights->setDisabled(true);
    } else if (xtalopt->getOptimizationType() == Search::SearchBase::OT_Pareto) {
      ui.cb_tournament->setEnabled(true);
      ui.cb_crowding->setEnabled(true);
      ui.cb_paretoFilterZeroWeights->setEnabled(true);
    }

    if (xtalopt->getOptimizationType() == Search::SearchBase::OT_Pareto && ui.cb_tournament->isChecked()) {
      ui.cb_restrictPool->setEnabled(true);
    } else {
      ui.cb_restrictPool->setDisabled(true);
    }
    for (const auto& keyword : Settings::runtimeKeywords()) {
      if (Settings::scalarSettingValue(*xtalopt, keyword) != before.value(keyword)) {
        settingsChanged = true;
        if (keyword == "objectivePrecision" || keyword == "optimizationType" ||
            keyword == "crowdingDistance" || keyword == "paretoFilterZeroWeights")
          selectionSettingsChanged = true;
      }
    }
  }

  if (selectionSettingsChanged) {
    if (m_search->isSessionInProgress() && xtalopt->applyParentSelectionFronts()) {
      emit xtalopt->structureViewDataChanged();
      xtalopt->requestResultsFileSave();
    }
  }

  if (settingsChanged && m_search->isSessionInProgress())
    xtalopt->requestStateFileSave();

  return true;
}

void TabSearch::lockGUI()
{
  // Non runtime-adjustable settings are locked during a run.
  ui.spin_numInitial->setDisabled(true);
  ui.list_seeds->setDisabled(true);
  ui.push_addSeed->setDisabled(true);
  ui.push_removeSeed->setDisabled(true);
  if (m_search->isReadOnly())
    ui.combo_optType->setDisabled(true);

  if (m_search->isReadOnly()) {
    ui.spin_parentsPoolSize->setDisabled(true);
    ui.spin_contStructs->setDisabled(true);
    ui.cb_limitRunningJobs->setDisabled(true);
    ui.spin_runningJobLimit->setDisabled(true);
    ui.spin_failLimit->setDisabled(true);
    ui.combo_failAction->setDisabled(true);
    ui.spin_cutoff->setDisabled(true);
    ui.cb_saveHulls->setDisabled(true);
    ui.cb_tournament->setDisabled(true);
    ui.cb_restrictPool->setDisabled(true);
    ui.cb_crowding->setDisabled(true);
    ui.cb_paretoFilterZeroWeights->setDisabled(true);
    ui.sb_prec->setDisabled(true);
    ui.spin_tol_spg->setDisabled(true);
    ui.push_spg_reset->setDisabled(true);
    ui.spin_tol_xcLength->setDisabled(true);
    ui.spin_tol_xcAngle->setDisabled(true);
    ui.push_sim_reset->setDisabled(true);
    ui.spin_rdf_tol->setDisabled(true);
    ui.spin_rdf_cut->setDisabled(true);
    ui.spin_rdf_bin->setDisabled(true);
    ui.spin_rdf_sig->setDisabled(true);
    ui.push_sim_reset_2->setDisabled(true);
    ui.spin_p_cross->setDisabled(true);
    ui.spin_p_strip->setDisabled(true);
    ui.spin_p_perm->setDisabled(true);
    ui.spin_p_atom->setDisabled(true);
    ui.spin_p_comp->setDisabled(true);
    ui.spin_cross_minimumContribution->setDisabled(true);
    ui.sb_ncuts->setDisabled(true);
    ui.spin_strip_strainStdev_min->setDisabled(true);
    ui.spin_strip_strainStdev_max->setDisabled(true);
    ui.spin_strip_amp_min->setDisabled(true);
    ui.spin_strip_amp_max->setDisabled(true);
    ui.spin_strip_per1->setDisabled(true);
    ui.spin_strip_per2->setDisabled(true);
    ui.spin_perm_strainStdev_max->setDisabled(true);
    ui.spin_perm_ex->setDisabled(true);
    ui.sb_rand_supercell->setDisabled(true);
  }
}

void TabSearch::updateOptimizationInfo()
{
  if (m_updateGuiInProgress)
    return;

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);
  bool settingsChanged = false;
  bool spacegroupSettingsChanged = false;
  bool similaritySettingsChanged = false;
  {
    // Change the settings.
    QWriteLocker runtimeLocker(m_search->runtimeSettingsLock());
    const Settings::ScalarSnapshot before = Settings::captureScalarSettings(*xtalopt);

    // Initial generation
    xtalopt->setNumInitial(ui.spin_numInitial->value());

    // Search parameters
    xtalopt->setParentsPoolSize(ui.spin_parentsPoolSize->value());
    xtalopt->setContStructs(ui.spin_contStructs->value());
    xtalopt->setRunningJobLimit(ui.spin_runningJobLimit->value());
    xtalopt->setLimitRunningJobs(ui.cb_limitRunningJobs->isChecked());
    xtalopt->setFailLimit(ui.spin_failLimit->value());
    xtalopt->setFailAction(XtalOpt::FailActions(ui.combo_failAction->currentIndex()));
    xtalopt->setMaxNumStructures(ui.spin_cutoff->value());
    xtalopt->setSaveHullSnapshots(ui.cb_saveHulls->isChecked());

    // Make sure running job limit is properly accessible.
    ui.spin_runningJobLimit->setEnabled(xtalopt->isLimitRunningJobs());

    // Spglib tolerance
    xtalopt->setTolSpg(ui.spin_tol_spg->value());

    // XtalComp similarities
    xtalopt->setTolXcLength(ui.spin_tol_xcLength->value());
    xtalopt->setTolXcAngle(ui.spin_tol_xcAngle->value());

    // RDF similarities
    xtalopt->setTolRdf(ui.spin_rdf_tol->value());
    xtalopt->setTolRdfCutoff(ui.spin_rdf_cut->value());
    xtalopt->setTolRdfSigma(ui.spin_rdf_sig->value());
    xtalopt->setTolRdfNbins(ui.spin_rdf_bin->value());

    // Operation weights
    xtalopt->setPCross(ui.spin_p_cross->value());
    xtalopt->setPStrip(ui.spin_p_strip->value());
    xtalopt->setPPerm(ui.spin_p_perm->value());
    xtalopt->setPAtomic(ui.spin_p_atom->value());
    xtalopt->setPComp(ui.spin_p_comp->value());

    // Crossover
    xtalopt->setCrossNcuts(ui.sb_ncuts->value());
    xtalopt->setCrossMinimumContribution(ui.spin_cross_minimumContribution->value());
    if (xtalopt->getCrossNcuts() > 1) {
      ui.spin_cross_minimumContribution->setEnabled(false);
    } else {
      ui.spin_cross_minimumContribution->setEnabled(true);
    }

    // Stripple
    xtalopt->setStripStrainStdevMin(ui.spin_strip_strainStdev_min->value());
    xtalopt->setStripStrainStdevMax(ui.spin_strip_strainStdev_max->value());
    xtalopt->setStripAmpMin(ui.spin_strip_amp_min->value());
    xtalopt->setStripAmpMax(ui.spin_strip_amp_max->value());
    xtalopt->setStripPer1(ui.spin_strip_per1->value());
    xtalopt->setStripPer2(ui.spin_strip_per2->value());

    // Permustrain
    xtalopt->setPermStrainStdevMax(ui.spin_perm_strainStdev_max->value());
    xtalopt->setPermEx(ui.spin_perm_ex->value());

    // Random supercell generation
    xtalopt->setPSupercell(ui.sb_rand_supercell->value());

    // Validate the new settings (KeepPrevious): a GUI edit that makes a
    //   setting invalid is restored to the previous value.
    Settings::validateSettings(*xtalopt, Settings::InvalidSettingAction::KeepPrevious, &before);
    for (const auto& keyword : Settings::runtimeKeywords()) {
      if (Settings::scalarSettingValue(*xtalopt, keyword) != before.value(keyword)) {
        settingsChanged = true;
        if (keyword == "spglibTolerance")
          spacegroupSettingsChanged = true;
        if (xtalopt->similarityKeywordInUse(keyword))
          similaritySettingsChanged = true;
      }
    }
  }

  if (spacegroupSettingsChanged)
    xtalopt->resetSpacegroups();
  if (similaritySettingsChanged)
    xtalopt->resetSimilarities();
  if (settingsChanged && m_search->isSessionInProgress())
    xtalopt->requestStateFileSave();
}

void TabSearch::addSeed(QListWidgetItem* item)
{
  QString filename("");
  bool replace = false;
  if (item)
    replace = true;

  // Set filename
  if (replace) {
    filename = item->text();
  } else {
    filename = QDir::homePath();
  }

  // Launch file dialog
  QString newFilename = QFileDialog::getOpenFileName(
    m_dialog, QString("Select structure file to use as seed"), filename,
    "VASP input (*POSCAR *CONTCAR *.vasp);;" "PWSCF input/output (*.pwscf);;"
    "CASTEP input/output (*.cell *.castep);;" "SIESTA input/output (*.fdf *.siesta);;"
    "GULP output (*.got *.gout);;" "MTP input/output (*.cfg *.mot);;" "CIF (*.cif);;"
    "XYZ (*.xyz);;" "CML (*.cml);;" "All Files (*)", 0, QFileDialog::DontUseNativeDialog);

  // User canceled
  if (newFilename.isEmpty())
    return;

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
  for (const auto& s : xtalopt->seedList()) {
    ui.list_seeds->addItem(s);
  }
}

void TabSearch::updateSeeds()
{
  // Hold the runtime-settings write lock while applying GUI edits,
  // like the CLI runtime path does.
  QWriteLocker runtimeLocker(m_search->runtimeSettingsLock());
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  QStringList seeds;
  for (int i = 0; i < ui.list_seeds->count(); i++)
    seeds.append(ui.list_seeds->item(i)->text());
  xtalopt->seedList() = seeds;
}
}

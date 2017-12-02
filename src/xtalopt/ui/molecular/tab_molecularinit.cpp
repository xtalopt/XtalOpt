/**********************************************************************
  TabMolecularInit - the tab for molecular initialization in XtalOpt

  Copyright (C) 2017 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include "tab_molecularinit.h"

#include <xtalopt/xtalopt.h>

#include <globalsearch/eleminfo.h>
#include <globalsearch/queuemanager.h>
#include <globalsearch/structure.h>
#include <globalsearch/utilities/fileutils.h>

#include <QFileDialog>
#include <QHeaderView>
#include <QSettings>
#include <QTableWidget>
#include <QTableWidgetItem>

#include "moleculardialog.h"

#include "conformergeneratordialog.h"

namespace XtalOpt {

TabMolecularInit::TabMolecularInit(GlobalSearch::AbstractDialog* parent,
                                   XtalOpt* p)
  : AbstractTab(parent, p), m_confGenDialog(nullptr)
{
  ui.setupUi(m_tab_widget);

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_opt);

  xtalopt->loaded = false;

  // Initialize the formula units list...
  if (xtalopt->formulaUnitsList.isEmpty()) {
    xtalopt->formulaUnitsList.append(1);
    // We need to append this one twice... lowestEnthalpyFUList.at(0) does not
    // correspond to anything. lowestEnthalpyFUList.at(1) corresponds to 1 FU
    xtalopt->lowestEnthalpyFUList.append(0);
    xtalopt->lowestEnthalpyFUList.append(0);
  }

  // Unit cell
  connect(ui.spin_a_min, SIGNAL(editingFinished()), this,
          SLOT(updateDimensions()));
  connect(ui.spin_b_min, SIGNAL(editingFinished()), this,
          SLOT(updateDimensions()));
  connect(ui.spin_c_min, SIGNAL(editingFinished()), this,
          SLOT(updateDimensions()));
  connect(ui.spin_alpha_min, SIGNAL(editingFinished()), this,
          SLOT(updateDimensions()));
  connect(ui.spin_beta_min, SIGNAL(editingFinished()), this,
          SLOT(updateDimensions()));
  connect(ui.spin_gamma_min, SIGNAL(editingFinished()), this,
          SLOT(updateDimensions()));
  connect(ui.spin_vol_min, SIGNAL(editingFinished()), this,
          SLOT(updateDimensions()));
  connect(ui.spin_a_max, SIGNAL(editingFinished()), this,
          SLOT(updateDimensions()));
  connect(ui.spin_b_max, SIGNAL(editingFinished()), this,
          SLOT(updateDimensions()));
  connect(ui.spin_c_max, SIGNAL(editingFinished()), this,
          SLOT(updateDimensions()));
  connect(ui.spin_alpha_max, SIGNAL(editingFinished()), this,
          SLOT(updateDimensions()));
  connect(ui.spin_beta_max, SIGNAL(editingFinished()), this,
          SLOT(updateDimensions()));
  connect(ui.spin_gamma_max, SIGNAL(editingFinished()), this,
          SLOT(updateDimensions()));
  connect(ui.spin_vol_max, SIGNAL(editingFinished()), this,
          SLOT(updateDimensions()));
  connect(ui.spin_fixedVolume, SIGNAL(editingFinished()), this,
          SLOT(updateDimensions()));
  connect(ui.cb_fixedVolume, SIGNAL(toggled(bool)), this,
          SLOT(updateDimensions()));

  // Interatomic Distances
  connect(ui.spin_scaleFactor, SIGNAL(valueChanged(double)), this,
          SLOT(updateDimensions()));
  connect(ui.spin_minRadius, SIGNAL(valueChanged(double)), this,
          SLOT(updateDimensions()));
  connect(ui.cb_interatomicDistanceLimit, SIGNAL(toggled(bool)), this,
          SLOT(updateDimensions()));

  // Formula unit
  connect(ui.edit_formula_units, SIGNAL(editingFinished()), this,
          SLOT(updateFormulaUnits()));
  connect(xtalopt, SIGNAL(updateFormulaUnitsListUIText()), this,
          SLOT(updateFormulaUnitsListUI()));
  connect(xtalopt, SIGNAL(updateVolumesToBePerFU(uint)), this,
          SLOT(adjustVolumesToBePerFU(uint)));

  // Conformer stuff
  connect(ui.push_generateConformers, SIGNAL(clicked()), this,
          SLOT(showConformerGeneratorDialog()));
  connect(ui.edit_conformerDir, SIGNAL(editingFinished()), this,
          SLOT(updateConformerDir()));
  connect(ui.push_browseConfDir, SIGNAL(clicked()), this,
          SLOT(browseConfDir()));

  initialize();
}

TabMolecularInit::~TabMolecularInit()
{
  delete m_confGenDialog;
}

void TabMolecularInit::writeSettings(const QString& filename)
{
}

void TabMolecularInit::readSettings(const QString& filename)
{
  updateGUI();

  updateFormulaUnitsListUI();

  updateDimensions();
}

void TabMolecularInit::updateGUI()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_opt);

  ui.spin_a_min->setValue(xtalopt->a_min);
  ui.spin_b_min->setValue(xtalopt->b_min);
  ui.spin_c_min->setValue(xtalopt->c_min);
  ui.spin_a_max->setValue(xtalopt->a_max);
  ui.spin_b_max->setValue(xtalopt->b_max);
  ui.spin_c_max->setValue(xtalopt->c_max);
  ui.spin_alpha_min->setValue(xtalopt->alpha_min);
  ui.spin_beta_min->setValue(xtalopt->beta_min);
  ui.spin_gamma_min->setValue(xtalopt->gamma_min);
  ui.spin_alpha_max->setValue(xtalopt->alpha_max);
  ui.spin_beta_max->setValue(xtalopt->beta_max);
  ui.spin_gamma_max->setValue(xtalopt->gamma_max);
  ui.spin_vol_min->setValue(xtalopt->vol_min);
  ui.spin_vol_max->setValue(xtalopt->vol_max);
  ui.spin_fixedVolume->setValue(xtalopt->vol_fixed);
  ui.spin_scaleFactor->setValue(xtalopt->scaleFactor);
  ui.spin_minRadius->setValue(xtalopt->minRadius);
  ui.cb_fixedVolume->setChecked(xtalopt->using_fixed_volume);
  ui.cb_interatomicDistanceLimit->setChecked(
    xtalopt->using_interatomicDistanceLimit);
  ui.edit_conformerDir->setText(xtalopt->m_conformerOutDir.c_str());
}

void TabMolecularInit::lockGUI()
{
}

void TabMolecularInit::updateDimensions()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_opt);

  // Check for conflicts -- favor lower value
  if (ui.spin_a_min->value() > ui.spin_a_max->value())
    ui.spin_a_max->setValue(ui.spin_a_min->value());
  if (ui.spin_b_min->value() > ui.spin_b_max->value())
    ui.spin_b_max->setValue(ui.spin_b_min->value());
  if (ui.spin_c_min->value() > ui.spin_c_max->value())
    ui.spin_c_max->setValue(ui.spin_c_min->value());
  if (ui.spin_alpha_min->value() > ui.spin_alpha_max->value())
    ui.spin_alpha_max->setValue(ui.spin_alpha_min->value());
  if (ui.spin_beta_min->value() > ui.spin_beta_max->value())
    ui.spin_beta_max->setValue(ui.spin_beta_min->value());
  if (ui.spin_gamma_min->value() > ui.spin_gamma_max->value())
    ui.spin_gamma_max->setValue(ui.spin_gamma_min->value());
  if (ui.spin_vol_min->value() > ui.spin_vol_max->value())
    ui.spin_vol_max->setValue(ui.spin_vol_min->value());

  // Assign variables
  xtalopt->a_min = ui.spin_a_min->value();
  xtalopt->b_min = ui.spin_b_min->value();
  xtalopt->c_min = ui.spin_c_min->value();
  xtalopt->alpha_min = ui.spin_alpha_min->value();
  xtalopt->beta_min = ui.spin_beta_min->value();
  xtalopt->gamma_min = ui.spin_gamma_min->value();
  xtalopt->vol_min = ui.spin_vol_min->value();

  xtalopt->a_max = ui.spin_a_max->value();
  xtalopt->b_max = ui.spin_b_max->value();
  xtalopt->c_max = ui.spin_c_max->value();
  xtalopt->alpha_max = ui.spin_alpha_max->value();
  xtalopt->beta_max = ui.spin_beta_max->value();
  xtalopt->gamma_max = ui.spin_gamma_max->value();
  xtalopt->vol_max = ui.spin_vol_max->value();

  xtalopt->using_fixed_volume = ui.cb_fixedVolume->isChecked();
  xtalopt->vol_fixed = ui.spin_fixedVolume->value();
}

void TabMolecularInit::updateMinRadii()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_opt);
}

void TabMolecularInit::updateMinIAD()
{
}

void TabMolecularInit::updateFormulaUnits()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_opt);

  QString tmp;

  QList<uint> formulaUnitsList =
    FileUtils::parseUIntString(ui.edit_formula_units->text(), tmp);

  // If nothing valid was obtained, return 1
  if (formulaUnitsList.size() == 0) {
    xtalopt->formulaUnitsList.append(1);
    tmp = "1";
    ui.edit_formula_units->setText(tmp.trimmed());
    return;
  }

  // If we changed the formula units, reset the spacegroup generation
  // min xtals per FU to be zero
  if (xtalopt->formulaUnitsList != formulaUnitsList &&
      xtalopt->minXtalsOfSpgPerFU.size() != 0) {
    // We're assuming that if the composition is enabled, the run has not
    // yet begun. This is probably a valid assumption (unless we allow the
    // composition to be changed during the run in the future)
    xtalopt->minXtalsOfSpgPerFU = QList<int>();
  }

  xtalopt->formulaUnitsList = formulaUnitsList;

  // Update the size of the lowestEnthalpyFUList
  while (xtalopt->lowestEnthalpyFUList.size() <= xtalopt->maxFU())
    xtalopt->lowestEnthalpyFUList.append(0);

  // Update UI
  ui.edit_formula_units->setText(tmp.trimmed());
}

// Updates the UI with the contents of xtalopt->formulaUnitsList
void TabMolecularInit::updateFormulaUnitsListUI()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_opt);
  QString tmp;
  QList<uint> formulaUnitsList = xtalopt->formulaUnitsList;
  for (size_t i = 0; i < formulaUnitsList.size(); i++) {
    tmp += QString::number(formulaUnitsList.at(i)) + ", ";
  }
  ui.edit_formula_units->setText(tmp);
  updateFormulaUnits();
}

// Updates the UI by disabling/enabling options for initialization
void TabMolecularInit::updateInitOptions()
{
  updateDimensions();
}

// This is only used when resuming older version of XtalOpt
// It adjusts the volumes so that they are per FU instead of just
// pure volumes
void TabMolecularInit::adjustVolumesToBePerFU(uint FU)
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_opt);
  ui.spin_vol_min->setValue(ui.spin_vol_min->value() / static_cast<double>(FU));
  ui.spin_vol_max->setValue(ui.spin_vol_max->value() / static_cast<double>(FU));
  ui.spin_fixedVolume->setValue(ui.spin_fixedVolume->value() /
                                static_cast<double>(FU));
  xtalopt->vol_min = ui.spin_vol_min->value();
  xtalopt->vol_max = ui.spin_vol_max->value();
  xtalopt->vol_fixed = ui.spin_fixedVolume->value();
}

void TabMolecularInit::showConformerGeneratorDialog()
{
  if (!m_confGenDialog) {
    m_confGenDialog =
      new GlobalSearch::ConformerGeneratorDialog(this->m_dialog, m_opt);
    connect(m_confGenDialog, SIGNAL(finished(int)), this, SLOT(updateGUI()));
  }

  m_confGenDialog->exec();
}

void TabMolecularInit::updateConformerDir()
{
  m_opt->m_conformerOutDir = ui.edit_conformerDir->text().toStdString();
}

void TabMolecularInit::browseConfDir()
{
  QString oldDir = m_opt->m_conformerOutDir.c_str();
  QString newDir = QFileDialog::getExistingDirectory(
    m_dialog, tr("Select the conformer directory..."), oldDir);

  if (!newDir.isEmpty() && !newDir.endsWith(QDir::separator()))
    newDir.append(QDir::separator());

  if (!newDir.isEmpty()) {
    m_opt->m_conformerOutDir = newDir.toStdString();
    updateGUI();
  }
}
}

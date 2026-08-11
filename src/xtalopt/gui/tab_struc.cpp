/**********************************************************************
  TabStruc - The structure-settings tab: composition, cell limits, and volumes.

  Copyright (C) 2009-2011 by David Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/gui/randspg_dialog.h>
#include <xtalopt/gui/tab_struc.h>

#include <xtalopt/xtalopt.h>

#include <atoms/eleminfo.h>
#include <atoms/molecule.h>
#include <search/queuemanager.h>
#include <search/structure.h>
#include <common/compatibility/qt_compat.h>
#include <common/fileutils.h>

#include <QAbstractSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QAbstractItemView>
#include <QDoubleSpinBox>
#include <QFontDatabase>
#include <QHeaderView>
#include <QLineEdit>
#include <QList>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

#include "dialog.h"

namespace XtalOpt {

namespace {

const char* MOLECULE_FORMULA_PLACEHOLDER = "Formula, e.g. C1H4";

QString templateLabel(const Atoms::MoleculeTemplateInfo& info)
{
  return QString("%1  (%2)").arg(info.name).arg(info.pointGroup);
}

QLineEdit* createMoleculeFormulaEdit(const QString& formula = QString())
{
  QLineEdit* edit = new QLineEdit();
  edit->setPlaceholderText(MOLECULE_FORMULA_PLACEHOLDER);
  edit->setText(formula);
  edit->setFrame(false);
  return edit;
}

QComboBox* ensureMoleculeTemplateCombo(QTableWidget* table, int row)
{
  QComboBox* combo = qobject_cast<QComboBox*>(table->cellWidget(row, TabStruc::MC_TEMPLATE));
  if (!combo) {
    combo = new QComboBox();
    table->setCellWidget(row, TabStruc::MC_TEMPLATE, combo);
  }
  return combo;
}

} // namespace

void clearMoleculeTemplateCombo(QTableWidget* table, int row)
{
  QComboBox* combo = ensureMoleculeTemplateCombo(table, row);
  combo->blockSignals(true);
  combo->clear();
  combo->setEnabled(false);
  combo->blockSignals(false);
}

TabStruc::TabStruc(Search::AbstractDialog* parent, XtalOpt* p)
  : AbstractTab(parent, p), m_spgOptions(nullptr),
    m_moleculeUnitHelpDialog(nullptr),
    m_openingSpgOptions(false),
    m_updateMoleculeUnitsInProgress(false),
    m_moleculeUnitTableLocked(false),
    m_customIADTableLocked(false)
{
  ui.setupUi(m_tab_widget);

  updateGUI();
  updateCompositionTable();

  // Composition
  connect(ui.edit_composition, &QLineEdit::returnPressed, this, &TabStruc::getComposition);

  // Search type
  connect(ui.cb_vcsearch, &QCheckBox::clicked, this, [this](bool) { updateSearchType(); });

  // Min and Max number of atoms
  connect(ui.sb_max_atoms, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this, &TabStruc::updateAtomCountLimits);
  connect(ui.sb_min_atoms, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this, &TabStruc::updateAtomCountLimits);

  // Updating reference energy values
  connect(ui.edit_ref_enes, &QLineEdit::returnPressed, this, &TabStruc::updateReferenceEnergies);

  // Unit cell
  connect(ui.spin_a_min,     &QAbstractSpinBox::editingFinished, this, &TabStruc::updateDimensions);
  connect(ui.spin_b_min,     &QAbstractSpinBox::editingFinished, this, &TabStruc::updateDimensions);
  connect(ui.spin_c_min,     &QAbstractSpinBox::editingFinished, this, &TabStruc::updateDimensions);
  connect(ui.spin_alpha_min, &QAbstractSpinBox::editingFinished, this, &TabStruc::updateDimensions);
  connect(ui.spin_beta_min,  &QAbstractSpinBox::editingFinished, this, &TabStruc::updateDimensions);
  connect(ui.spin_gamma_min, &QAbstractSpinBox::editingFinished, this, &TabStruc::updateDimensions);
  connect(ui.spin_a_max,     &QAbstractSpinBox::editingFinished, this, &TabStruc::updateDimensions);
  connect(ui.spin_b_max,     &QAbstractSpinBox::editingFinished, this, &TabStruc::updateDimensions);
  connect(ui.spin_c_max,     &QAbstractSpinBox::editingFinished, this, &TabStruc::updateDimensions);
  connect(ui.spin_alpha_max, &QAbstractSpinBox::editingFinished, this, &TabStruc::updateDimensions);
  connect(ui.spin_beta_max,  &QAbstractSpinBox::editingFinished, this, &TabStruc::updateDimensions);
  connect(ui.spin_gamma_max, &QAbstractSpinBox::editingFinished, this, &TabStruc::updateDimensions);

  // Volumes
  connect(ui.spin_vol_min,       &QAbstractSpinBox::editingFinished, this, &TabStruc::updateVolumes);
  connect(ui.spin_vol_max,       &QAbstractSpinBox::editingFinished, this, &TabStruc::updateVolumes);
  connect(ui.spin_maxVolumeScale, &QAbstractSpinBox::editingFinished, this, &TabStruc::updateVolumes);
  connect(ui.spin_minVolumeScale, &QAbstractSpinBox::editingFinished, this, &TabStruc::updateVolumes);
  connect(ui.edit_ele_vols, &QLineEdit::returnPressed, this, &TabStruc::updateVolumes);

  // Interatomic Distances
  connect(ui.spin_scaleFactor,
          static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
          this, &TabStruc::updateDimensions);
  connect(ui.spin_minRadius,
          static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
          this, &TabStruc::updateDimensions);
  connect(ui.cb_interatomicDistanceLimit, &QCheckBox::toggled, this, &TabStruc::updateDimensions);
  connect(ui.cb_customIAD,   &QCheckBox::toggled, this, &TabStruc::updateDimensions);
  connect(ui.cb_customIAD, &QCheckBox::toggled, this, &TabStruc::updateCustomIADTableEnabled);
  connect(ui.cb_checkStepOpt, &QCheckBox::toggled, this, &TabStruc::updateDimensions);
  connect(ui.table_IAD, &QTableWidget::itemChanged, this, &TabStruc::updateCustomIAD);

  // Molecule-unit builder
  connect(ui.table_moleculeUnit, &QTableWidget::itemSelectionChanged,
          this, &TabStruc::updateMoleculeUnitTableEnabled);
  connect(ui.table_moleculeUnit, &QTableWidget::itemChanged, this, &TabStruc::updateMoleculeUnits);
  connect(ui.pushButton_addMoleculeUnit,      &QPushButton::clicked,
          this, [this](bool) { addRow(); });
  connect(ui.pushButton_removeMoleculeUnit,   &QPushButton::clicked,
          this, [this](bool) { removeRow(); });
  connect(ui.pushButton_removeAllMoleculeUnit, &QPushButton::clicked,
          this, [this](bool) { removeAll(); });
  connect(ui.pushButton_moleculeUnitHelp, &QPushButton::clicked,
          this, [this](bool) { showMoleculeUnitHelp(); });

  // randSpg
  connect(ui.cb_allowRandSpg, &QCheckBox::toggled, this, &TabStruc::updateDimensions);
  connect(ui.push_spgOptions, &QPushButton::clicked, this, &TabStruc::openSpgOptions);

  // RandSpg initialization controls
  connect(ui.cb_allowRandSpg,  &QCheckBox::toggled, this, &TabStruc::updateInitOptions);

  initialize();

  ui.table_moleculeUnit->setColumnCount(2);
  ui.table_moleculeUnit->setHorizontalHeaderLabels(QStringList() << "Formula" << "Template");
  ui.table_moleculeUnit->setToolTip("Each row defines one molecule unit. Enter a formula, then "
    "choose one of the matching catalog templates.");

  refreshMoleculeUnitTable();

  // Header resize modes are not represented in the .ui file.
  ui.table_comp->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  ui.table_moleculeUnit->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  ui.table_IAD->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

  updateCustomIADTableEnabled();
  updateInitOptions();
}

TabStruc::~TabStruc()
{
  if (m_spgOptions)
    delete m_spgOptions;
}

void TabStruc::updateGUI()
{
  m_updateGuiInProgress = true;

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  ui.spin_a_min->setValue(xtalopt->getAMin());
  ui.spin_b_min->setValue(xtalopt->getBMin());
  ui.spin_c_min->setValue(xtalopt->getCMin());
  ui.spin_a_max->setValue(xtalopt->getAMax());
  ui.spin_b_max->setValue(xtalopt->getBMax());
  ui.spin_c_max->setValue(xtalopt->getCMax());
  ui.spin_alpha_min->setValue(xtalopt->getAlphaMin());
  ui.spin_beta_min->setValue(xtalopt->getBetaMin());
  ui.spin_gamma_min->setValue(xtalopt->getGammaMin());
  ui.spin_alpha_max->setValue(xtalopt->getAlphaMax());
  ui.spin_beta_max->setValue(xtalopt->getBetaMax());
  ui.spin_gamma_max->setValue(xtalopt->getGammaMax());
  ui.spin_scaleFactor->setValue(xtalopt->getScaleFactor());
  ui.spin_minRadius->setValue(xtalopt->getMinRadius());
  ui.cb_interatomicDistanceLimit->setChecked(xtalopt->getUsingScaledIAD());
  ui.cb_customIAD->setChecked(xtalopt->getUsingCustomIAD());
  ui.cb_checkStepOpt->setChecked(xtalopt->getUsingCheckStepOpt());
  ui.cb_allowRandSpg->setChecked(xtalopt->getUsingRandSpg());

  ui.spin_maxVolumeScale->setValue(xtalopt->getVolScaleMax());
  ui.spin_minVolumeScale->setValue(xtalopt->getVolScaleMin());
  ui.spin_vol_min->setValue(xtalopt->getVolMin());
  ui.spin_vol_max->setValue(xtalopt->getVolMax());

  ui.edit_composition->setText(xtalopt->getInputFormulasString());
  ui.edit_ele_vols->setText(xtalopt->getInputEleVolmString());
  ui.edit_ref_enes->setText(xtalopt->getInputEneRefsString());
  refreshMoleculeUnitTable();
  updateCompositionTable();
  updateCustomIADTableEnabled();

  ui.cb_vcsearch->setChecked(xtalopt->getVcSearch());
  ui.sb_max_atoms->setValue(xtalopt->getMaxAtoms());
  ui.sb_min_atoms->setValue(xtalopt->getMinAtoms());

  m_updateGuiInProgress = false;
}

void TabStruc::refreshMoleculeUnitTable()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);
  const QStringList& moleculeUnitInputs = xtalopt->moleculeUnitInputs();

  disconnect(ui.table_moleculeUnit, 0, 0, 0);
  ui.table_moleculeUnit->setRowCount(0);

  for (int i = 0; i < moleculeUnitInputs.size(); ++i) {
    const QStringList fields = moleculeUnitInputs.at(i).split(' ', QtCompat::SkipEmptyParts);
    if (fields.size() != 2)
      continue;

    const int row = ui.table_moleculeUnit->rowCount();
    ui.table_moleculeUnit->insertRow(row);
    ui.table_moleculeUnit->setCellWidget(
      row, MC_FORMULA, createMoleculeFormulaEdit(fields.at(0).simplified()));

    QComboBox* templateCombo = new QComboBox();
    const QString templateName = fields.at(1).simplified();
    templateCombo->addItem(templateName, templateName);
    ui.table_moleculeUnit->setCellWidget(row, MC_TEMPLATE, templateCombo);
  }

  updateMoleculeUnits();
  updateMoleculeUnitTableEnabled();
}

void TabStruc::lockGUI()
{
  ui.edit_composition->setDisabled(true);
  m_moleculeUnitTableLocked = true;
  ui.table_moleculeUnit->setEditTriggers(QAbstractItemView::NoEditTriggers);
  updateMoleculeUnitTableEnabled();
  ui.cb_allowRandSpg->setDisabled(true);
  ui.push_spgOptions->setDisabled(true);
  ui.edit_ref_enes->setDisabled(true);
  m_customIADTableLocked = true;
  updateCustomIADTableEnabled();
  ui.cb_vcsearch->setDisabled(true);
  ui.sb_max_atoms->setDisabled(true);
  ui.sb_min_atoms->setDisabled(true);

  if (m_search->isReadOnly()) {
    ui.spin_a_min->setDisabled(true);
    ui.spin_a_max->setDisabled(true);
    ui.spin_b_min->setDisabled(true);
    ui.spin_b_max->setDisabled(true);
    ui.spin_c_min->setDisabled(true);
    ui.spin_c_max->setDisabled(true);
    ui.spin_alpha_min->setDisabled(true);
    ui.spin_alpha_max->setDisabled(true);
    ui.spin_beta_min->setDisabled(true);
    ui.spin_beta_max->setDisabled(true);
    ui.spin_gamma_min->setDisabled(true);
    ui.spin_gamma_max->setDisabled(true);
    ui.spin_scaleFactor->setDisabled(true);
    ui.spin_minRadius->setDisabled(true);
    ui.cb_interatomicDistanceLimit->setDisabled(true);
    ui.cb_checkStepOpt->setDisabled(true);
    ui.spin_vol_min->setDisabled(true);
    ui.spin_vol_max->setDisabled(true);
    ui.spin_minVolumeScale->setDisabled(true);
    ui.spin_maxVolumeScale->setDisabled(true);
    ui.edit_ele_vols->setDisabled(true);
  }
}

void TabStruc::getComposition()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  // Do not change the composition during a run.
  if (m_search->isSessionInProgress()) {
    ui.edit_composition->setText(xtalopt->getInputFormulasString());
    return;
  }

  // Show errors after changing the settings.
  QString err;
  {
    // Change the settings.
    QWriteLocker runtimeLocker(m_search->runtimeSettingsLock());

    // The existing value for chemical formulas string
    QString init_formula_input = xtalopt->getInputFormulasString();
    // The existing minimal composition, used to reset molecule units when needed.
    CellComp init_comp = xtalopt->getMinimalComposition();

    // The user input for chemical formulas string
    QString input_text = ui.edit_composition->text();

    // This is the case at a fresh start
    if (input_text.isEmpty() && init_formula_input.isEmpty())
      return;

    if (!xtalopt->processInputChemicalFormulas(input_text)) {
      // We will be here if: (1) input text is re-entered after a list
      //   was already set, and (2) the new list is incorrect or empty.
      err = tr("Invalid or no chemical formula entry!");
      //
      // To avoid any issues, we reset everything.
      //
      // Clear the input values.
      ui.edit_composition->clear();
      ui.table_comp->setRowCount(0);
      xtalopt->setInputFormulasString("");
      ui.edit_ref_enes->clear();
      xtalopt->setInputEneRefsString("");
      ui.edit_ele_vols->clear();
      xtalopt->setInputEleVolmString("");
      ui.table_moleculeUnit->setRowCount(0);
      xtalopt->clearMoleculeUnits();
      xtalopt->setInputForcedSpgsString("");
      xtalopt->rebuildDerivedSettings();
      // Clear custom IAD values.
      ui.table_IAD->setRowCount(0);
      ui.cb_customIAD->setChecked(false);
      xtalopt->clearCustomIADs();
      // Reset the search type
      this->updateSearchType();
      this->updateInitOptions();
    } else if (input_text == init_formula_input) {
      // A valid list that is just the same as the old one: nothing to reset.
      this->updateInitOptions();
    } else {
      // A valid, changed list: reset the composition related entries
      //   where it's needed.

      // Update the composition input.
      xtalopt->setInputFormulasString(ui.edit_composition->text());

      // Clear values that depend on the composition.
      ui.edit_ref_enes->clear();
      xtalopt->setInputEneRefsString("");
      ui.edit_ele_vols->clear();
      xtalopt->setInputEleVolmString("");

      const bool resetForcedSpgs = (init_formula_input != input_text &&
         !xtalopt->getInputForcedSpgsString().isEmpty());
      if (resetForcedSpgs) {
        xtalopt->setInputForcedSpgsString("");
        err = tr("Composition changed: reset the spacegroups list!");
      }

      // Rebuild every parsed cache from the raw inputs in one pass.
      xtalopt->rebuildDerivedSettings();

      // Update various relevant tables/variables
      this->updateAtomCountLimits();
      xtalopt->clearCustomIADs();
      this->updateCompositionTable();
      this->updateSearchType();

      // If compositions have changed while molecule units are already set,
      //   reset them so each entry can be validated against the new input list.
      QString frm1 = init_comp.getFormula();
      QString frm2 = xtalopt->getMinimalComposition().getFormula();
      if (frm1 != frm2) {
        if (ui.table_moleculeUnit->rowCount() > 0)
          ui.table_moleculeUnit->setRowCount(0);
        xtalopt->clearMoleculeUnits();
      }

      this->updateInitOptions();
    }
  }

  if (!err.isEmpty())
    errorPromptWindow(err);
}

void TabStruc::updateAtomCountLimits()
{
  if (m_updateGuiInProgress)
    return;

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  // Do not change the atom limits during a run.
  if (m_search->isSessionInProgress()) {
    ui.sb_max_atoms->blockSignals(true);
    ui.sb_max_atoms->setValue(xtalopt->getMaxAtoms());
    ui.sb_max_atoms->blockSignals(false);
    ui.sb_min_atoms->blockSignals(true);
    ui.sb_min_atoms->setValue(xtalopt->getMinAtoms());
    ui.sb_min_atoms->blockSignals(false);
    return;
  }

  // Change the settings.
  QWriteLocker runtimeLocker(m_search->runtimeSettingsLock());
  disconnect(ui.sb_max_atoms, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
             this, &TabStruc::updateAtomCountLimits);
  disconnect(ui.sb_min_atoms, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
             this, &TabStruc::updateAtomCountLimits);

  // Set the minimum/maximum number of atoms
  xtalopt->setMaxAtoms(ui.sb_max_atoms->value());
  xtalopt->setMinAtoms(ui.sb_min_atoms->value());

  // Widen the atom limits to cover every input composition, then sync the UI.
  for (int i = 0; i < xtalopt->compList().size(); i++)
    xtalopt->adjustAtomCountLimits(xtalopt->compList()[i].getNumAtoms());
  ui.sb_max_atoms->setValue(xtalopt->getMaxAtoms());
  ui.sb_min_atoms->setValue(xtalopt->getMinAtoms());

  connect(ui.sb_max_atoms, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this, &TabStruc::updateAtomCountLimits);
  connect(ui.sb_min_atoms, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
          this, &TabStruc::updateAtomCountLimits);
}

void TabStruc::updateReferenceEnergies()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  // Do not change reference energies during a run.
  if (m_search->isSessionInProgress()) {
    ui.edit_ref_enes->setText(xtalopt->getInputEneRefsString());
    return;
  }

  // Show errors after changing the settings.
  QString err;
  {
    // Change the settings.
    QWriteLocker runtimeLocker(m_search->runtimeSettingsLock());

    // This can work only if composition is already set
    if (xtalopt->compList().isEmpty()) {
      err = tr("Set the composition first!");
      ui.edit_ref_enes->clear();
    } else {
      // The existing value for reference energies string
      QString init_ref_input = xtalopt->getInputEneRefsString();
      // The user input for reference energies
      QString input_text = ui.edit_ref_enes->text();

      if (!xtalopt->processInputReferenceEnergies(input_text)) {
        // No valid input! Just restore the previous values.
        ui.edit_ref_enes->setText(init_ref_input);
        xtalopt->setInputEneRefsString(init_ref_input);
        err = tr("Invalid entries for reference energies!");
      } else {
        // If here, we have the global ref energy variable set successfully
        // Update the input string (needed to write state file etc)
        xtalopt->setInputEneRefsString(input_text);

        // Update the composition table to reflect on elemental references
        updateCompositionTable();
      }
    }
  }

  if (!err.isEmpty())
    errorPromptWindow(err);
}

void TabStruc::updateVolumes()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  // Update the volume limits.
  QString err;
  bool settingsChanged = false;
  {
    // Change the settings.
    QWriteLocker runtimeLocker(m_search->runtimeSettingsLock());
    const Settings::ScalarSnapshot before = Settings::captureScalarSettings(*xtalopt);

    // Absolute volume limits
    if (ui.spin_vol_min->value() > ui.spin_vol_max->value())
      ui.spin_vol_max->setValue(ui.spin_vol_min->value());
    xtalopt->setVolMin(ui.spin_vol_min->value());
    xtalopt->setVolMax(ui.spin_vol_max->value());

    // Scaled volume limits
    if (ui.spin_minVolumeScale->value() > ui.spin_maxVolumeScale->value())
      ui.spin_maxVolumeScale->setValue(ui.spin_minVolumeScale->value());
    xtalopt->setVolScaleMax(ui.spin_maxVolumeScale->value());
    xtalopt->setVolScaleMin(ui.spin_minVolumeScale->value());

    // Now, let's get to the elemental volumes, if they have changed
    if (sender() != ui.edit_ele_vols) {
      updateCompositionTable();
    } else {
      // Elemental volumes can be handled only if composition is set
      if (xtalopt->compList().isEmpty()) {
        err = tr("Set the composition first!");
        ui.edit_ele_vols->clear();
      } else {
        // The existing value for elemental volumes string
        QString init_vol_input = xtalopt->getInputEleVolmString();
        // The user input for elemental volumes
        QString input_text = ui.edit_ele_vols->text();

        if (!xtalopt->processInputElementalVolumes(input_text)) {
          // No valid input! Just restore the previous state
          ui.edit_ele_vols->setText(init_vol_input);
          xtalopt->setInputEleVolmString(init_vol_input);
          err = tr("Invalid entries for elemental volumes!");
        } else {
          // If here, we have the global elemental volume variable set successfully
          // Update the input string (needed to write state file etc)
          xtalopt->setInputEleVolmString(input_text);

          // Update the composition table to reflect on the new elemental volumes
          updateCompositionTable();
        }
      }
    }
    for (const auto& keyword : Settings::runtimeKeywords()) {
      if (Settings::scalarSettingValue(*xtalopt, keyword) != before.value(keyword)) {
        settingsChanged = true;
        break;
      }
    }
  }

  if (settingsChanged && m_search->isSessionInProgress())
    xtalopt->requestStateFileSave();

  if (!err.isEmpty())
    errorPromptWindow(err);
}

void TabStruc::updateSearchType()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  // Do not change the search type during a run.
  if (m_search->isSessionInProgress()) {
    ui.cb_vcsearch->setChecked(xtalopt->getVcSearch());
    return;
  }

  // Change the settings.
  QWriteLocker runtimeLocker(m_search->runtimeSettingsLock());

  xtalopt->setVcSearch(ui.cb_vcsearch->isChecked());
}

void TabStruc::updateCompositionTable()
{
  // This function actually updates "composition-related" tables,
  //   i.e., the composition table (with various info) and the
  //   custom IAD table entries, using the current information.
  // This is important especially when a change is made to the
  //   input compositions list.

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  // If composition is not set, make sure the GUI does not keep old rows.
  if (xtalopt->compList().isEmpty()) {
    ui.table_comp->setRowCount(0);
    return;
  }

  QList<unsigned int> keys = xtalopt->compList()[0].getCompositionAtomicNumbers();
  keys.removeAll(0);
  std::sort(keys.begin(), keys.end());

  // Adjust table size:
  int numRows = keys.size();
  ui.table_comp->setRowCount(numRows);
  int numRows2 = keys.size();

  for (int j = numRows2 - 1; j > 0; j--) {
    numRows2 = numRows2 + j;
  }
  int z = 0;
  const bool tableSignalsWereBlocked = ui.table_IAD->blockSignals(true);
  ui.table_IAD->setRowCount(numRows2);

  for (int i = 0; i < numRows; i++) {
    if (keys.at(i) == 0)
      continue;

    uint atomicNum = keys.at(i);

    // Symbol column
    QString symbol = Atoms::ElementInfo::getAtomicSymbol(atomicNum).c_str();
    // Min radius column
    QString minRadius = (xtalopt->getUsingScaledIAD()) ?
                        QString::number(xtalopt->eleMinRadii().getMinRadius(atomicNum)) : "n/a";
    // Reference energy column
    QString referenceEnergy = "n/a";
    for (int m = 0; m < xtalopt->refEnergies().size(); m++) {
      if (xtalopt->refEnergies()[m].cell.getNumTypes() == 1 &&
          xtalopt->refEnergies()[m].cell.getCompositionAtomicNumbers()[0] == atomicNum) {
        double refEnePerAtom = xtalopt->refEnergies()[m].energy;
        refEnePerAtom /= xtalopt->refEnergies()[m].cell.getNumAtoms();
        referenceEnergy = QString::number(refEnePerAtom);
      }
    }
    // Volume columns
    QString eleVolMin = "n/a";
    QString eleVolMax = "n/a";
    //
    if (xtalopt->eleVolumes().getVolumeAtomicNumbers().size() != 0) {
      eleVolMin = QString::number(xtalopt->eleVolumes().getMinVolume(atomicNum));
      eleVolMax = QString::number(xtalopt->eleVolumes().getMaxVolume(atomicNum));
    } else if (xtalopt->getVolScaleMin() > 0 && xtalopt->getVolScaleMax() >= xtalopt->getVolScaleMin()) {
      double evol = Atoms::ElementInfo::getCovalentVolume(atomicNum);
      eleVolMin = QString::number(xtalopt->getVolScaleMin() * evol);
      eleVolMax = QString::number(xtalopt->getVolScaleMax() * evol);
    } else {
      eleVolMin = QString::number(xtalopt->getVolMin());
      eleVolMax = QString::number(xtalopt->getVolMax());
    }

    QTableWidgetItem* symbolItem = new QTableWidgetItem(symbol);
    QTableWidgetItem* minRadiusItem = new QTableWidgetItem(minRadius);
    QTableWidgetItem* refEnergyItem = new QTableWidgetItem(referenceEnergy);
    QTableWidgetItem* volMinItem = new QTableWidgetItem(eleVolMin);
    QTableWidgetItem* volMaxItem = new QTableWidgetItem(eleVolMax);

    ui.table_comp->setItem(i, CC_SYMBOL, symbolItem);
    ui.table_comp->setItem(i, CC_MINRADIUS, minRadiusItem);
    ui.table_comp->setItem(i, CC_REFENE, refEnergyItem);
    ui.table_comp->setItem(i, CC_MINVOL, volMinItem);
    ui.table_comp->setItem(i, CC_MAXVOL, volMaxItem);

    // Custom IAD table
    for (int k = i; k < numRows; k++) {
      unsigned int atomicNum2 = keys.at(k);

      QString symbol1 = Atoms::ElementInfo::getAtomicSymbol(atomicNum).c_str();
      QString symbol2 = Atoms::ElementInfo::getAtomicSymbol(atomicNum2).c_str();

      QTableWidgetItem* symbol1Item = new QTableWidgetItem(symbol1);
      QTableWidgetItem* symbol2Item = new QTableWidgetItem(symbol2);

      // Element symbols don't need to be editable!
      symbol1Item->setFlags(symbol1Item->flags() & ~Qt::ItemIsEditable);
      symbol2Item->setFlags(symbol2Item->flags() & ~Qt::ItemIsEditable);

      ui.table_IAD->setItem(z, IC_SYMBOL1, symbol1Item);
      ui.table_IAD->setItem(z, IC_SYMBOL2, symbol2Item);

      const QPair<int, int> pair(atomicNum, atomicNum2);
      QString minIAD;
      const QHash<QPair<int, int>, IAD>& customIADs =
        static_cast<const XtalOpt*>(xtalopt)->interComp();
      auto customIAD = customIADs.constFind(pair);
      if (customIAD != customIADs.constEnd())
        minIAD = QString::number(customIAD.value().minIAD, 'f', 3);
      QTableWidgetItem* minIADItem = new QTableWidgetItem(minIAD);
      ui.table_IAD->setItem(z, IC_MINIAD, minIADItem);

      z++;
    }
  }
  ui.table_IAD->blockSignals(tableSignalsWereBlocked);
}

void TabStruc::updateDimensions()
{
  // Change the settings.
  QWriteLocker runtimeLocker(m_search->runtimeSettingsLock());
  if (m_updateGuiInProgress)
    return;

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);
  const Settings::ScalarSnapshot before = Settings::captureScalarSettings(*xtalopt);

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

  // Assign variables
  xtalopt->setAMin(ui.spin_a_min->value());
  xtalopt->setBMin(ui.spin_b_min->value());
  xtalopt->setCMin(ui.spin_c_min->value());
  xtalopt->setAlphaMin(ui.spin_alpha_min->value());
  xtalopt->setBetaMin(ui.spin_beta_min->value());
  xtalopt->setGammaMin(ui.spin_gamma_min->value());

  xtalopt->setAMax(ui.spin_a_max->value());
  xtalopt->setBMax(ui.spin_b_max->value());
  xtalopt->setCMax(ui.spin_c_max->value());
  xtalopt->setAlphaMax(ui.spin_alpha_max->value());
  xtalopt->setBetaMax(ui.spin_beta_max->value());
  xtalopt->setGammaMax(ui.spin_gamma_max->value());

  // Allow RandSpg
  xtalopt->setUsingRandSpg(ui.cb_allowRandSpg->isChecked());

  if (xtalopt->getScaleFactor() != ui.spin_scaleFactor->value() ||
      xtalopt->getMinRadius() != ui.spin_minRadius->value() || xtalopt->getUsingScaledIAD() !=
        ui.cb_interatomicDistanceLimit->isChecked()) {
    xtalopt->setScaleFactor(ui.spin_scaleFactor->value());
    xtalopt->setMinRadius(ui.spin_minRadius->value());
    xtalopt->setUsingScaledIAD(ui.cb_interatomicDistanceLimit->isChecked());
  }

  if (xtalopt->getUsingCustomIAD() != ui.cb_customIAD->isChecked()) {
    xtalopt->setUsingCustomIAD(ui.cb_customIAD->isChecked());
  }
  if (xtalopt->getUsingCheckStepOpt() != ui.cb_checkStepOpt->isChecked()) {
    xtalopt->setUsingCheckStepOpt(ui.cb_checkStepOpt->isChecked());
  }

  // Check the new settings.
  Settings::validateSettings(*xtalopt, Settings::InvalidSettingAction::KeepPrevious, &before);
  xtalopt->rebuildDerivedSettings();
  this->updateCompositionTable();
  this->updateCustomIADTableEnabled();
  bool settingsChanged = false;
  for (const auto& keyword : Settings::runtimeKeywords()) {
    if (Settings::scalarSettingValue(*xtalopt, keyword) != before.value(keyword)) {
      settingsChanged = true;
      break;
    }
  }
  runtimeLocker.unlock();

  if (settingsChanged && m_search->isSessionInProgress())
    xtalopt->requestStateFileSave();
}

void TabStruc::updateCustomIAD()
{
  // Do not change custom IAD values during a run.
  if (m_updateGuiInProgress || m_search->isSessionInProgress() || !ui.cb_customIAD->isChecked())
    return;

  // Change the settings.
  QWriteLocker runtimeLocker(m_search->runtimeSettingsLock());
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  for (int i = 0; i < ui.table_IAD->rowCount(); ++i) {
    QTableWidgetItem* minIADItem = ui.table_IAD->item(i, IC_MINIAD);
    if (!minIADItem || minIADItem->text().trimmed().isEmpty()) {
      xtalopt->clearCustomIADs();
      return;
    }
  }

  xtalopt->clearCustomIADs();

  for (int i = 0; i < ui.table_IAD->rowCount(); ++i) {
    QTableWidgetItem* symbol1Item = ui.table_IAD->item(i, IC_SYMBOL1);
    QTableWidgetItem* symbol2Item = ui.table_IAD->item(i, IC_SYMBOL2);
    QTableWidgetItem* minIADItem = ui.table_IAD->item(i, IC_MINIAD);
    const QString entry = QString("%1, %2, %3")
                            .arg(symbol1Item ? symbol1Item->text() : QString())
                            .arg(symbol2Item ? symbol2Item->text() : QString())
                            .arg(minIADItem ? minIADItem->text() : QString());
    if (!xtalopt->processInputCustomIAD(entry)) {
      xtalopt->clearCustomIADs();
      runtimeLocker.unlock();
      errorPromptWindow("Invalid custom IAD value!");
      return;
    }
  }

  if (!xtalopt->checkCustomIADs()) {
    xtalopt->clearCustomIADs();
    runtimeLocker.unlock();
    errorPromptWindow("The custom IAD table is incomplete!");
  }
}

void TabStruc::updateMoleculeUnitTableEnabled()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);
  const bool hasComposition = xtalopt && !xtalopt->compList().isEmpty();
  const bool hasRows = ui.table_moleculeUnit->rowCount() > 0;
  const bool editable = !m_moleculeUnitTableLocked && hasComposition;

  ui.table_moleculeUnit->setEnabled(hasComposition || hasRows);
  ui.pushButton_addMoleculeUnit->setEnabled(editable);
  ui.pushButton_removeMoleculeUnit->setEnabled(
    editable && hasRows && ui.table_moleculeUnit->currentRow() >= 0);
  ui.pushButton_removeAllMoleculeUnit->setEnabled(editable && hasRows);

  for (int row = 0; row < ui.table_moleculeUnit->rowCount(); ++row) {
    QLineEdit* formulaEdit =
      qobject_cast<QLineEdit*>(ui.table_moleculeUnit->cellWidget(row, MC_FORMULA));
    if (formulaEdit)
      formulaEdit->setReadOnly(!editable);

    QComboBox* templateCombo =
      qobject_cast<QComboBox*>(ui.table_moleculeUnit->cellWidget(row, MC_TEMPLATE));
    if (templateCombo)
      templateCombo->setEnabled(editable && templateCombo->count() > 0);
  }
}

void TabStruc::updateCustomIADTableEnabled()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);
  const bool customIADsReady = xtalopt && xtalopt->checkCustomIADs(false);
  const bool canSelectCustom = !m_search->isReadOnly() &&
    !ui.cb_interatomicDistanceLimit->isChecked() &&
    (!m_search->isSessionInProgress() || customIADsReady);
  ui.cb_customIAD->setEnabled(canSelectCustom);
  ui.table_IAD->setEnabled(!m_customIADTableLocked && ui.cb_customIAD->isChecked());
}

// Updates the GUI by disabling/enabling options for initialization
void TabStruc::updateInitOptions()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);
  const bool hasComposition = !xtalopt->compList().isEmpty();

  if (!hasComposition) {
    ui.cb_allowRandSpg->setChecked(false);

    updateMoleculeUnitTableEnabled();

    ui.cb_allowRandSpg->setEnabled(false);
    ui.push_spgOptions->setEnabled(false);

    updateDimensions();
    return;
  }

  updateMoleculeUnitTableEnabled();

  ui.cb_allowRandSpg->setEnabled(!m_moleculeUnitTableLocked);
  ui.push_spgOptions->setEnabled(!m_moleculeUnitTableLocked);

  updateDimensions();
}

void TabStruc::updateMoleculeUnits()
{
  // Do not change molecule units during a run.
  if (m_moleculeUnitTableLocked || m_search->isSessionInProgress()) {
    updateMoleculeUnitTableEnabled();
    return;
  }

  if (m_updateMoleculeUnitsInProgress)
    return;

  m_updateMoleculeUnitsInProgress = true;

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  disconnect(ui.table_moleculeUnit, 0, 0, 0);

  xtalopt->clearMoleculeUnits();

  QList<int> invalidRows;
  QString validationError;

  for (int row = 0; row < ui.table_moleculeUnit->rowCount(); ++row) {
    QLineEdit* formulaEdit =
      qobject_cast<QLineEdit*>(ui.table_moleculeUnit->cellWidget(row, MC_FORMULA));
    if (!formulaEdit) {
      QString formulaText;
      QTableWidgetItem* formulaItem = ui.table_moleculeUnit->item(row, MC_FORMULA);
      if (formulaItem)
        formulaText = formulaItem->text();
      formulaEdit = createMoleculeFormulaEdit(formulaText.simplified());
      ui.table_moleculeUnit->setCellWidget(row, MC_FORMULA, formulaEdit);
    }
    disconnect(formulaEdit, 0, this, 0);
    connect(formulaEdit, &QLineEdit::editingFinished, this, &TabStruc::updateMoleculeUnits);

    const QString formula = formulaEdit->text().simplified();
    formulaEdit->setToolTip(QString());
    QComboBox* templateCombo =
      qobject_cast<QComboBox*>(ui.table_moleculeUnit->cellWidget(row, MC_TEMPLATE));
    QString selectedTemplate = templateCombo ? templateCombo->currentData().toString()
                                             : QString();

    std::vector<Atoms::MoleculeTemplateInfo> templates;
    if (!formula.isEmpty())
      templates = Atoms::moleculeTemplatesForFormula(formula.toStdString());

    if (!formula.isEmpty() && templates.empty()) {
      if (validationError.isEmpty())
        validationError = "No molecule template matches the formula. The row will be removed.";
      invalidRows.append(row);
      continue;
    }

    templateCombo = ensureMoleculeTemplateCombo(ui.table_moleculeUnit, row);
    templateCombo->setToolTip(QString());

    templateCombo->blockSignals(true);
    templateCombo->clear();
    for (size_t i = 0; i < templates.size(); ++i) {
      templateCombo->addItem(templateLabel(templates[i]), templates[i].name);
      templateCombo->setItemData(static_cast<int>(i),
                                 QString("%1  %2  %3")
                                   .arg(templates[i].speciesPattern)
                                   .arg(templates[i].formulaPattern)
                                   .arg(templates[i].description),
                                 Qt::ToolTipRole);
    }
    const int selectedIndex = templateCombo->findData(selectedTemplate);
    templateCombo->setCurrentIndex(selectedIndex < 0 ? 0 : selectedIndex);
    templateCombo->setEnabled(!templates.empty());
    templateCombo->blockSignals(false);
    disconnect(templateCombo, 0, this, 0);
    connect(templateCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &TabStruc::updateMoleculeUnits);

    if (formula.isEmpty())
      continue;

    const QString templateName = templateCombo->currentData().toString();

    const QString entry = QString("%1 %2").arg(formula).arg(templateName);
    if (!xtalopt->processInputMoleculeUnit(entry)) {
      if (validationError.isEmpty())
        validationError = "Molecule unit is invalid for the current composition. "
          "The row will be removed.";
      invalidRows.append(row);
      continue;
    }
  }

  if (!validationError.isEmpty())
    errorPromptWindow(validationError);

  for (int i = invalidRows.size() - 1; i >= 0; --i)
    ui.table_moleculeUnit->removeRow(invalidRows.at(i));

  connect(ui.table_moleculeUnit, &QTableWidget::itemChanged, this, &TabStruc::updateMoleculeUnits);
  connect(ui.table_moleculeUnit, &QTableWidget::itemSelectionChanged,
          this, &TabStruc::updateMoleculeUnitTableEnabled);

  m_updateMoleculeUnitsInProgress = false;
  updateMoleculeUnitTableEnabled();
}

// Actions for buttons to add/remove rows from the molecule unit table
void TabStruc::addRow()
{
  if (m_moleculeUnitTableLocked)
    return;

  disconnect(ui.table_moleculeUnit, 0, 0, 0);
  int row = ui.table_moleculeUnit->rowCount();
  ui.table_moleculeUnit->insertRow(row);

  QLineEdit* formulaEdit = createMoleculeFormulaEdit();
  ui.table_moleculeUnit->setCellWidget(row, MC_FORMULA, formulaEdit);
  QComboBox* templateCombo = new QComboBox();
  templateCombo->setEnabled(false);
  ui.table_moleculeUnit->setCellWidget(row, MC_TEMPLATE, templateCombo);

  this->updateMoleculeUnits();
  formulaEdit->setFocus();
}

void TabStruc::removeRow()
{
  if (m_moleculeUnitTableLocked)
    return;

  if (ui.table_moleculeUnit->currentRow() < 0)
    return;

  disconnect(ui.table_moleculeUnit, 0, 0, 0);
  ui.table_moleculeUnit->removeRow(ui.table_moleculeUnit->currentRow());

  connect(ui.table_moleculeUnit, &QTableWidget::itemChanged, this, &TabStruc::updateMoleculeUnits);
  connect(ui.table_moleculeUnit, &QTableWidget::itemSelectionChanged,
          this, &TabStruc::updateMoleculeUnitTableEnabled);

  this->updateMoleculeUnits();
}

void TabStruc::removeAll()
{
  if (m_moleculeUnitTableLocked)
    return;

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);
  xtalopt->clearMoleculeUnits();

  int row = ui.table_moleculeUnit->rowCount();

  disconnect(ui.table_moleculeUnit, 0, 0, 0);
  for (int i = row - 1; i >= 0; i--) {
    ui.table_moleculeUnit->removeRow(i);
  }
  connect(ui.table_moleculeUnit, &QTableWidget::itemChanged, this, &TabStruc::updateMoleculeUnits);
  connect(ui.table_moleculeUnit, &QTableWidget::itemSelectionChanged,
          this, &TabStruc::updateMoleculeUnitTableEnabled);

  this->updateMoleculeUnits();
}

void TabStruc::showMoleculeUnitHelp()
{
  if (!m_moleculeUnitHelpDialog) {
    m_moleculeUnitHelpDialog = new QWidget(m_dialog, Qt::Tool);
    m_moleculeUnitHelpDialog->setAttribute(Qt::WA_DeleteOnClose, true);
    m_moleculeUnitHelpDialog->setAttribute(Qt::WA_QuitOnClose, false);
    m_moleculeUnitHelpDialog->setWindowTitle("Molecule Unit Templates");

    QPlainTextEdit* text = new QPlainTextEdit(m_moleculeUnitHelpDialog);
    text->setReadOnly(true);
    text->setLineWrapMode(QPlainTextEdit::NoWrap);
    text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close,
                           m_moleculeUnitHelpDialog);
    connect(buttons, &QDialogButtonBox::rejected, m_moleculeUnitHelpDialog.data(), &QWidget::close);

    QVBoxLayout* layout = new QVBoxLayout(m_moleculeUnitHelpDialog);
    layout->addWidget(text);
    layout->addWidget(buttons);

    connect(m_moleculeUnitHelpDialog.data(), &QObject::destroyed, this, [this]() {
      m_moleculeUnitHelpDialog = nullptr;
    });
    m_moleculeUnitHelpDialog->resize(900, 600);
  }

  QPlainTextEdit* text = m_moleculeUnitHelpDialog->findChild<QPlainTextEdit*>();
  if (text)
    text->setPlainText(Atoms::moleculeTemplateCatalogText());

  m_moleculeUnitHelpDialog->show();
  m_moleculeUnitHelpDialog->raise();
  m_moleculeUnitHelpDialog->activateWindow();
}

void TabStruc::openSpgOptions()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  // If compositions are not set yet, just return
  if (xtalopt->compList().isEmpty()) {
    errorPromptWindow("Set the composition first!");
    return;
  }

  // If m_spgOptions already exists, delete it if the current
  //   compositions do not equal the old compositions
  if (m_spgOptions) {
    if (!m_spgOptions->isCompositionSame(xtalopt)) {
      delete m_spgOptions;
      m_spgOptions = nullptr;
    }
  }

  // If m_spgOptions does not exist or was just deleted, create a new one
  if (!m_spgOptions) {
    // Do not open a second dialog.
    if (m_openingSpgOptions)
      return;
    m_openingSpgOptions = true;

    // Display a message to ask the user to wait while the image is loading...
    QMessageBox msgBox;
    msgBox.setText("Calculating possible spacegroups for the given system."
                   " Please wait...");
    msgBox.setStandardButtons(QMessageBox::NoButton);
    msgBox.setWindowModality(Qt::NonModal);
    msgBox.open();
    QCoreApplication::processEvents();

    // Open up the RandSpg dialog
    m_spgOptions = new RandSpgDialog(xtalopt, m_dialog);
    m_openingSpgOptions = false;

    // Close the mesage box
    msgBox.close();
  }

  // Display m_spgOptions. Re-read the forced counts first: they may have
  // changed since the dialog was created (e.g. by importing settings).
  m_spgOptions->updateSpinBoxes();
  m_spgOptions->exec();
}
}

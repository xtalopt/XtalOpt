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

#include <xtalopt/ui/randSpgDialog.h>
#include <xtalopt/ui/tab_struc.h>

#include <xtalopt/xtalopt.h>

#include <globalsearch/eleminfo.h>
#include <globalsearch/queuemanager.h>
#include <globalsearch/structure.h>
#include <globalsearch/utilities/fileutils.h>

#include <QSettings>

#include <QHeaderView>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QComboBox>

#include "dialog.h"

namespace XtalOpt {

TabStruc::TabStruc(GlobalSearch::AbstractDialog* parent, XtalOpt* p)
  : AbstractTab(parent, p), m_spgOptions(nullptr)
{
  ui.setupUi(m_tab_widget);

  readSettings();

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  xtalopt->loaded = false;

  // Composition
  connect(ui.edit_composition, SIGNAL(returnPressed()), this,
          SLOT(getComposition()));

  // Search type
  connect(ui.cb_vcsearch, SIGNAL(clicked(bool)), this,
          SLOT(updateSearchType()));

  // Min and Max number of atoms
  connect(ui.sb_max_atoms, SIGNAL(valueChanged(int)), this,
          SLOT(updateAtomCountLimits()));
  connect(ui.sb_min_atoms, SIGNAL(valueChanged(int)), this,
          SLOT(updateAtomCountLimits()));

  // Updating reference energy values
  connect(ui.edit_ref_enes, SIGNAL(returnPressed()), this,
          SLOT(updateReferenceEnergies()));

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

  // Volumes
  connect(ui.spin_vol_min, SIGNAL(editingFinished()), this,
          SLOT(updateVolumes()));
  connect(ui.spin_vol_max, SIGNAL(editingFinished()), this,
          SLOT(updateVolumes()));
  connect(ui.spin_maxVolumeScale, SIGNAL(editingFinished()), this,
          SLOT(updateVolumes()));
  connect(ui.spin_minVolumeScale, SIGNAL(editingFinished()), this,
          SLOT(updateVolumes()));
  connect(ui.edit_ele_vols, SIGNAL(returnPressed()), this,
          SLOT(updateVolumes()));

  // Interatomic Distances
  connect(ui.spin_scaleFactor, SIGNAL(valueChanged(double)), this,
          SLOT(updateDimensions()));
  connect(ui.spin_minRadius, SIGNAL(valueChanged(double)), this,
          SLOT(updateDimensions()));
  connect(ui.cb_interatomicDistanceLimit, SIGNAL(toggled(bool)), this,
          SLOT(updateDimensions()));
  connect(ui.cb_customIAD, SIGNAL(toggled(bool)), this,
          SLOT(updateDimensions()));
  connect(ui.cb_checkStepOpt, SIGNAL(toggled(bool)), this,
          SLOT(updateDimensions()));
  connect(ui.table_IAD, SIGNAL(itemSelectionChanged()), this,
          SLOT(updateCustomIAD()));

  // MolUnit builder
  connect(ui.cb_useMolUnit, SIGNAL(toggled(bool)), this,
          SLOT(updateDimensions()));
  connect(ui.table_molUnit, SIGNAL(itemSelectionChanged()), this,
          SLOT(updateMolUnits()));
  connect(ui.pushButton_addMolUnit, SIGNAL(clicked(bool)), this,
          SLOT(addRow()));
  connect(ui.pushButton_removeMolUnit, SIGNAL(clicked(bool)), this,
          SLOT(removeRow()));
  connect(ui.pushButton_removeAllMolUnit, SIGNAL(clicked(bool)), this,
          SLOT(removeAll()));

  // randSpg
  connect(ui.cb_allowRandSpg, SIGNAL(toggled(bool)), this,
          SLOT(updateDimensions()));
  connect(ui.push_spgOptions, SIGNAL(clicked()), this, SLOT(openSpgOptions()));

  // MolUnit and RandSpg enabling/disabling of each other
  connect(ui.cb_useMolUnit, SIGNAL(toggled(bool)), this,
          SLOT(updateInitOptions()));
  connect(ui.cb_allowRandSpg, SIGNAL(toggled(bool)), this,
          SLOT(updateInitOptions()));

  initialize();

  // Some adjustments for tables and default texts
  ui.table_comp->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  ui.table_molUnit->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  ui.table_IAD->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  ui.edit_composition->setPlaceholderText("Press 'return' after entering or adjusting input...");
  ui.edit_ele_vols->setPlaceholderText("Press 'return' after entering or adjusting input...");
  ui.edit_ref_enes->setPlaceholderText("Press 'return' after entering or adjusting input...");
  // Weirdly, and when compiled without debug flags; these push buttons receive 'return'
  //   all the time! This is to avoid this.
  ui.push_spgOptions->setAutoDefault(false);
  ui.pushButton_addMolUnit->setAutoDefault(false);
  ui.pushButton_removeMolUnit->setAutoDefault(false);
  ui.pushButton_removeAllMolUnit->setAutoDefault(false);
}

TabStruc::~TabStruc()
{
  if (m_spgOptions)
    delete m_spgOptions;
}

void TabStruc::writeSettings(const QString& filename)
{
}

void TabStruc::readSettings(const QString& filename)
{
  updateGUI();

  SETTINGS(filename);

  settings->beginGroup("xtalopt/init/");

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  if (!filename.isEmpty() && ui.cb_useMolUnit->isChecked() == true) {
    int size = settings->beginReadArray("compMolUnit");
    xtalopt->compMolUnit = QHash<QPair<int, int>, MolUnit>();
    for (int i = 0; i < size; i++) {
      settings->setArrayIndex(i);
      int centerNum, numCenters, neighborNum, numNeighbors;
      unsigned int geom;
      double dist;
      MolUnit entry;

      QString center = settings->value("center").toString();
      centerNum = ElementInfo::getAtomicNum(center.trimmed().toStdString());
      QString strNumCenters = settings->value("number_of_centers").toString();
      numCenters = strNumCenters.toInt();
      QString neighbor = settings->value("neighbor").toString();
      neighborNum = ElementInfo::getAtomicNum(neighbor.trimmed().toStdString());
      QString strNumNeighbors =
        settings->value("number_of_neighbors").toString();
      numNeighbors = strNumNeighbors.toInt();
      QString strGeom = settings->value("geometry").toString();
      this->setGeom(geom, strGeom);
      dist = settings->value("distance").toDouble();
      QString strDist = QString::number(dist, 'f', 3);
      entry.numCenters = numCenters;
      entry.numNeighbors = numNeighbors;
      entry.geom = geom;
      entry.dist = dist;

      xtalopt->compMolUnit.insert(qMakePair<int, int>(centerNum, neighborNum),
                                  entry);

      ui.table_molUnit->insertRow(i);

      QComboBox* combo_center = new QComboBox();
      combo_center->insertItem(0, center);
      ui.table_molUnit->setCellWidget(i, MC_CENTER, combo_center);
      connect(combo_center, SIGNAL(currentIndexChanged(int)), this,
              SLOT(updateMolUnits()));

      QComboBox* combo_numCenters = new QComboBox();
      combo_numCenters->insertItem(0, strNumCenters);
      ui.table_molUnit->setCellWidget(i, MC_NUMCENTERS, combo_numCenters);
      connect(combo_numCenters, SIGNAL(currentIndexChanged(int)), this,
              SLOT(updateMolUnits()));

      QComboBox* combo_neighbor = new QComboBox();
      combo_neighbor->insertItem(0, neighbor);
      ui.table_molUnit->setCellWidget(i, MC_NEIGHBOR, combo_neighbor);
      connect(combo_neighbor, SIGNAL(currentIndexChanged(int)), this,
              SLOT(updateMolUnits()));

      QComboBox* combo_numNeighbors = new QComboBox();
      combo_numNeighbors->insertItem(0, strNumNeighbors);
      ui.table_molUnit->setCellWidget(i, MC_NUMNEIGHBORS, combo_numNeighbors);
      connect(combo_numNeighbors, SIGNAL(currentIndexChanged(int)), this,
              SLOT(updateMolUnits()));

      QComboBox* combo_geom = new QComboBox();
      combo_geom->insertItem(0, strGeom);
      ui.table_molUnit->setCellWidget(i, MC_GEOM, combo_geom);
      connect(combo_geom, SIGNAL(currentIndexChanged(int)), this,
              SLOT(updateMolUnits()));

      QTableWidgetItem* distItem = new QTableWidgetItem(strDist);
      ui.table_molUnit->setItem(i, MC_DIST, distItem);
      connect(ui.table_molUnit, SIGNAL(itemChanged(QTableWidgetItem*)), this,
              SLOT(updateMolUnits()));
    }
    this->updateMolUnits();
    settings->endArray();
  }
  settings->endGroup();

  updateCompositionTable();
  updateCustomIAD();
  updateDimensions();
}

void TabStruc::updateGUI()
{
  m_updateGuiInProgress = true;

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

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
  ui.spin_scaleFactor->setValue(xtalopt->scaleFactor);
  ui.spin_minRadius->setValue(xtalopt->minRadius);
  ui.cb_interatomicDistanceLimit->setChecked(
    xtalopt->using_interatomicDistanceLimit);
  ui.cb_customIAD->setChecked(xtalopt->using_customIAD);
  ui.cb_checkStepOpt->setChecked(xtalopt->using_checkStepOpt);
  ui.cb_useMolUnit->setChecked(xtalopt->using_molUnit);
  ui.cb_allowRandSpg->setChecked(xtalopt->using_randSpg);

  ui.spin_maxVolumeScale->setValue(xtalopt->vol_scale_max);
  ui.spin_minVolumeScale->setValue(xtalopt->vol_scale_min);
  ui.spin_vol_min->setValue(xtalopt->vol_min);
  ui.spin_vol_max->setValue(xtalopt->vol_max);

  ui.edit_composition->setText(xtalopt->input_formulas_string);
  ui.edit_ele_vols->setText(xtalopt->input_ele_volm_string);
  ui.edit_ref_enes->setText(xtalopt->input_ene_refs_string);

  ui.cb_vcsearch->setChecked(xtalopt->vcSearch);
  ui.sb_max_atoms->setValue(xtalopt->maxAtoms);
  ui.sb_min_atoms->setValue(xtalopt->minAtoms);

  m_updateGuiInProgress = false;
}

void TabStruc::lockGUI()
{
  ui.edit_composition->setDisabled(true);
  ui.cb_useMolUnit->setDisabled(true);
  ui.table_molUnit->setDisabled(true);
  ui.pushButton_addMolUnit->setDisabled(true);
  ui.pushButton_removeMolUnit->setDisabled(true);
  ui.pushButton_removeAllMolUnit->setDisabled(true);
  ui.cb_allowRandSpg->setDisabled(true);
  ui.push_spgOptions->setDisabled(true);
  ui.edit_ref_enes->setDisabled(true);
  ui.cb_vcsearch->setDisabled(true);
  ui.sb_max_atoms->setDisabled(true);
  ui.sb_min_atoms->setDisabled(true);
}

void TabStruc::getComposition()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  // The existing value for chemical formulas string
  QString init_formula_input = xtalopt->input_formulas_string;
  // The existing minimal composition (to see if molunit update is needed)
  CellComp init_comp = xtalopt->getMinimalComposition();

  // The user input for chemical formulas string
  QString input_text = ui.edit_composition->text();

  // This is the case at a fresh start
  if (input_text.isEmpty() && init_formula_input.isEmpty())
    return;

  if (!xtalopt->processInputChemicalFormulas(input_text)) {
    // We will be here if: (1) input text is re-entered after a list
    //   was already set, and (2) the new list is incorrect or empty.
    errorPromptWindow("Invalid or no chemical formula entry!");
    //
    // To avoid any issues, we reset everything.
    //
    // Reset composition
    ui.edit_composition->clear();
    ui.table_comp->setRowCount(0);
    xtalopt->input_formulas_string.clear();
    xtalopt->compList.clear();
    // Reset reference energies
    ui.edit_ref_enes->clear();
    xtalopt->input_ene_refs_string.clear();
    xtalopt->refEnergies.clear();
    // Reset elemental volumes
    ui.edit_ele_vols->clear();
    xtalopt->input_ele_volm_string.clear();
    xtalopt->eleVolumes.clear();
    // Reset molUnit
    ui.table_molUnit->setRowCount(0);
    ui.cb_useMolUnit->setChecked(false);
    // Reset spg min xtals
    xtalopt->minXtalsOfSpg = QList<int>();
    // Reset interatomic distances
    ui.table_IAD->setRowCount(0);
    ui.cb_customIAD->setChecked(false);
    xtalopt->interComp.clear();
    // Reset the search type
    this->updateSearchType();
    //
    return;
  }

  // If we are here, it means that: (1) there has been either no
  //   composition list or we've had a valid list initiated, and
  //   (2) now user has pressed 'return' with a valid list.
  // Global composition variables are now set by processinputchemcial...

  // If the new list is just the same as old one, we can return from here.
  if (input_text == init_formula_input)
    return;

  // If the new list is not the same as the old one; we will reset various
  //   composition related entries where it's needed.

  // Update the input string (needed to write state file etc)
  xtalopt->input_formulas_string = ui.edit_composition->text();

  // Reset reference energies
  ui.edit_ref_enes->clear();
  xtalopt->input_ene_refs_string.clear();
  xtalopt->processInputReferenceEnergies(xtalopt->input_ene_refs_string);
  // Reset elemental volumes
  ui.edit_ele_vols->clear();
  xtalopt->input_ele_volm_string.clear();
  xtalopt->processInputElementalVolumes(xtalopt->input_ele_volm_string);
  // Update various relevant tables/variables
  this->updateAtomCountLimits();
  this->updateScaledIAD();
  this->updateCustomIAD();
  this->updateCompositionTable();
  this->updateSearchType();

  // If compositions have changed while mol units already set,
  //   we need to make sure the minimal composition is the same.
  //   Otherwise, we will reset the mol units.
  QString frm1 = init_comp.getFormula();
  QString frm2 = xtalopt->getMinimalComposition().getFormula();
  if (frm1 != frm2 && ui.table_molUnit->rowCount() > 0) {
    ui.table_molUnit->setRowCount(0);
    ui.cb_useMolUnit->setChecked(false);
  }

  // If compositions have changed while spg min xtals already set,
  //   we need to make sure the compositions are the same.
  //   Otherwise, we will reset the spg min xtals.
  if (init_formula_input != input_text && xtalopt->minXtalsOfSpg.size() != 0) {
    errorPromptWindow("Composition changed: reset the spacegroups list!");
    xtalopt->minXtalsOfSpg = QList<int>();
  }
}

void TabStruc::updateAtomCountLimits()
{
  disconnect(ui.sb_max_atoms, SIGNAL(valueChanged(int)), this,
             SLOT(updateAtomCountLimits()));
  disconnect(ui.sb_min_atoms, SIGNAL(valueChanged(int)), this,
             SLOT(updateAtomCountLimits()));

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);
  // Set the minimum/maximum number of atoms
  xtalopt->maxAtoms = ui.sb_max_atoms->value();
  xtalopt->minAtoms = ui.sb_min_atoms->value();

  // What is the largest/smallest number of atoms in the input cells?
  int maximum_atoms_in_compositions = 0;
  int minimum_atoms_in_compositions = std::numeric_limits<int>::max();
  for (int i = 0; i < xtalopt->compList.size(); i++) {
    if (xtalopt->compList[i].getNumAtoms() > maximum_atoms_in_compositions)
      maximum_atoms_in_compositions = xtalopt->compList[i].getNumAtoms();
    if (xtalopt->compList[i].getNumAtoms() < minimum_atoms_in_compositions)
      minimum_atoms_in_compositions = xtalopt->compList[i].getNumAtoms();
  }

  // Increase "maxAtoms" if it's smaller than the largest initial cell.
  if (maximum_atoms_in_compositions > xtalopt->maxAtoms) {
    xtalopt->maxAtoms = maximum_atoms_in_compositions;
    ui.sb_max_atoms->setValue(maximum_atoms_in_compositions);
  }
  // Decrease "minAtoms" if it's larger than the smallest initial cell.
  if (minimum_atoms_in_compositions < xtalopt->minAtoms) {
    xtalopt->minAtoms = minimum_atoms_in_compositions;
    ui.sb_min_atoms->setValue(minimum_atoms_in_compositions);
  }

  connect(ui.sb_max_atoms, SIGNAL(valueChanged(int)), this,
          SLOT(updateAtomCountLimits()));
  connect(ui.sb_min_atoms, SIGNAL(valueChanged(int)), this,
          SLOT(updateAtomCountLimits()));
}

void TabStruc::updateReferenceEnergies()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  // This can work only if composition is already set
  if (xtalopt->compList.isEmpty()) {
    errorPromptWindow("Set the composition first!");
    ui.edit_ref_enes->clear();
    return;
  }

  // The existing value for reference energies string
  QString init_ref_input = xtalopt->input_ene_refs_string;
  // The user input for reference energies
  QString input_text = ui.edit_ref_enes->text();

  if (!xtalopt->processInputReferenceEnergies(input_text)) {
    // No valid input! Just restore the previous values.
    ui.edit_ref_enes->setText(init_ref_input);
    xtalopt->input_ene_refs_string = init_ref_input;
    // Show a warning to the user
    errorPromptWindow("Invalid entries for reference energies!");
    return;
  }

  // If here, we have the global ref energy variable set successfully
  // Update the input string (needed to write state file etc)
  xtalopt->input_ene_refs_string = input_text;

  // Update the composition table to reflect on elemental references
  updateCompositionTable();
}

void TabStruc::updateVolumes()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  // Absolute volume limits
  if (ui.spin_vol_min->value() > ui.spin_vol_max->value())
    ui.spin_vol_max->setValue(ui.spin_vol_min->value());
  xtalopt->vol_min = ui.spin_vol_min->value();
  xtalopt->vol_max = ui.spin_vol_max->value();

  // Scaled volume limits
  if (ui.spin_minVolumeScale->value() > ui.spin_maxVolumeScale->value())
    ui.spin_maxVolumeScale->setValue(ui.spin_minVolumeScale->value());
  xtalopt->vol_scale_max = ui.spin_maxVolumeScale->value();
  xtalopt->vol_scale_min = ui.spin_minVolumeScale->value();

  // Now, let's get to the elemental volumes, if they have changed
  if (sender() != ui.edit_ele_vols) {
    updateCompositionTable();
    return;
  }

  // Elemental volumes can be handled only if composition is set
  if (xtalopt->compList.isEmpty()) {
    errorPromptWindow("Set the composition first!");
    ui.edit_ele_vols->clear();
    return;
  }

  // The existing value for elemental volumes string
  QString init_vol_input = xtalopt->input_ele_volm_string;
  // The user input for elemental volumes
  QString input_text = ui.edit_ele_vols->text();

  if (!xtalopt->processInputElementalVolumes(input_text)) {
    // No valid input! Just restore the previous state
    ui.edit_ele_vols->setText(init_vol_input);
    xtalopt->input_ele_volm_string = init_vol_input;
    // Show a warning to the user
    errorPromptWindow("Invalid entries for elemental volumes!");
    return;
  }

  // If here, we have the global elemental volume variable set successfully
  // Update the input string (needed to write state file etc)
  xtalopt->input_ele_volm_string = input_text;

  // Update the composition table to reflect on the new elemental volumes
  updateCompositionTable();
}

void TabStruc::updateSearchType()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  xtalopt->vcSearch = ui.cb_vcsearch->isChecked();
}

void TabStruc::updateCompositionTable()
{
  // This function actually updates "composition-related" tables,
  //   i.e., the composition table (with various info) and the
  //   custom IAD table entries, using the current information.
  // This is important especially when a change is made to the
  //   input compositions list.

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  // If composition is not set, just return
  if (xtalopt->compList.isEmpty())
    return;

  QList<unsigned int> keys = xtalopt->compList[0].getAtomicNumbers();
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

  for (int i = 0; i < numRows; i++) {
    if (keys.at(i) == 0)
      continue;

    uint atomicNum = keys.at(i);

    // Symbol column
    QString symbol = ElementInfo::getAtomicSymbol(atomicNum).c_str();
    // Min radius column
    QString minRadius = (xtalopt->using_interatomicDistanceLimit) ?
                        QString::number(xtalopt->eleMinRadii.getMinRadius(atomicNum)) : "n/a";
    // Reference energy column
    QString referenceEnergy = "n/a";
    for (int m = 0; m < xtalopt->refEnergies.size(); m++) {
      if (xtalopt->refEnergies[m].cell.getNumTypes() == 1 &&
          xtalopt->refEnergies[m].cell.getAtomicNumbers()[0] == atomicNum) {
        double refEnePerAtom = xtalopt->refEnergies[m].energy;
        refEnePerAtom /= xtalopt->refEnergies[m].cell.getNumAtoms();
        referenceEnergy = QString::number(refEnePerAtom);
      }
    }
    // Volume columns
    QString eleVolMin = "n/a";
    QString eleVolMax = "n/a";
    //
    if (xtalopt->eleVolumes.getAtomicNumbers().size() != 0) {
      eleVolMin = QString::number(xtalopt->eleVolumes.getMinVolume(atomicNum));
      eleVolMax = QString::number(xtalopt->eleVolumes.getMaxVolume(atomicNum));
    } else if (xtalopt->vol_scale_min > 0 && xtalopt->vol_scale_max >= xtalopt->vol_scale_min) {
      double evol = ElementInfo::getCovalentVolume(atomicNum);
      eleVolMin = QString::number(xtalopt->vol_scale_min * evol);
      eleVolMax = QString::number(xtalopt->vol_scale_max * evol);
    } else {
      eleVolMin = QString::number(xtalopt->vol_min);
      eleVolMax = QString::number(xtalopt->vol_max);
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
    if (ui.cb_customIAD->isChecked()) {
      ui.table_IAD->setRowCount(numRows2);

      for (int k = i; k < numRows; k++) {
        unsigned int atomicNum2 = keys.at(k);

        QString symbol1 = ElementInfo::getAtomicSymbol(atomicNum).c_str();
        QString symbol2 = ElementInfo::getAtomicSymbol(atomicNum2).c_str();

        QTableWidgetItem* symbol1Item = new QTableWidgetItem(symbol1);
        QTableWidgetItem* symbol2Item = new QTableWidgetItem(symbol2);

        // Element symbols don't need to be editable!
        symbol1Item->setFlags(symbol1Item->flags() & ~Qt::ItemIsEditable);
        symbol2Item->setFlags(symbol2Item->flags() & ~Qt::ItemIsEditable);

        ui.table_IAD->setItem(z, IC_SYMBOL1, symbol1Item);
        ui.table_IAD->setItem(z, IC_SYMBOL2, symbol2Item);

        QString minIAD = QString::number(
          xtalopt->interComp[qMakePair<int, int>(atomicNum, atomicNum2)].minIAD,
          'f', 3);
        QTableWidgetItem* minIADItem = new QTableWidgetItem(minIAD);
        ui.table_IAD->setItem(z, IC_MINIAD, minIADItem);

        z++;
      }
    }
  }
}

void TabStruc::updateDimensions()
{
  if (m_updateGuiInProgress)
    return;

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

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
  xtalopt->a_min = ui.spin_a_min->value();
  xtalopt->b_min = ui.spin_b_min->value();
  xtalopt->c_min = ui.spin_c_min->value();
  xtalopt->alpha_min = ui.spin_alpha_min->value();
  xtalopt->beta_min = ui.spin_beta_min->value();
  xtalopt->gamma_min = ui.spin_gamma_min->value();

  xtalopt->a_max = ui.spin_a_max->value();
  xtalopt->b_max = ui.spin_b_max->value();
  xtalopt->c_max = ui.spin_c_max->value();
  xtalopt->alpha_max = ui.spin_alpha_max->value();
  xtalopt->beta_max = ui.spin_beta_max->value();
  xtalopt->gamma_max = ui.spin_gamma_max->value();

  // Allow Molecular units
  xtalopt->using_molUnit = ui.cb_useMolUnit->isChecked();

  // Allow RandSpg
  xtalopt->using_randSpg = ui.cb_allowRandSpg->isChecked();
  if (xtalopt->using_molUnit == false)
    this->removeAll();

  if (xtalopt->scaleFactor != ui.spin_scaleFactor->value() ||
      xtalopt->minRadius != ui.spin_minRadius->value() ||
      xtalopt->using_interatomicDistanceLimit !=
        ui.cb_interatomicDistanceLimit->isChecked()) {
    xtalopt->scaleFactor = ui.spin_scaleFactor->value();
    xtalopt->minRadius = ui.spin_minRadius->value();
    xtalopt->using_interatomicDistanceLimit =
      ui.cb_interatomicDistanceLimit->isChecked();
    this->updateScaledIAD();
    this->updateCompositionTable();
  }

  if (xtalopt->using_customIAD != ui.cb_customIAD->isChecked()) {
    xtalopt->using_customIAD = ui.cb_customIAD->isChecked();
    this->updateCustomIAD();
    this->updateCompositionTable();
  }
  if (xtalopt->using_checkStepOpt != ui.cb_checkStepOpt->isChecked()) {
    xtalopt->using_checkStepOpt = ui.cb_checkStepOpt->isChecked();
  }
}

void TabStruc::updateScaledIAD()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  for (const auto& atomcn : xtalopt->eleMinRadii.getAtomicNumbers()) {
    double  minr = ElementInfo::getCovalentRadius(atomcn) * xtalopt->scaleFactor;
    minr = (minr > xtalopt->minRadius) ? minr : xtalopt->minRadius;
    xtalopt->eleMinRadii.set(atomcn, minr);
  }
}

void TabStruc::updateCustomIAD()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);
  QHash<QPair<int, int>, IAD> interComp;
  unsigned int length = ui.table_IAD->rowCount();

  for (uint i = 0; i < length; i++) {
    QString symbol1 = ui.table_IAD->item(i, IC_SYMBOL1)->text();
    int atomicNum1 =
      ElementInfo::getAtomicNum(symbol1.trimmed().toStdString().c_str());
    QString symbol2 = ui.table_IAD->item(i, IC_SYMBOL2)->text();
    int atomicNum2 =
      ElementInfo::getAtomicNum(symbol2.trimmed().toStdString().c_str());
    double minIAD = ui.table_IAD->item(i, IC_MINIAD)->text().toDouble();
    QTableWidgetItem* minIADItem =
      new QTableWidgetItem(QString::number(minIAD, 'f', 3));
    ui.table_IAD->setItem(i, IC_MINIAD, minIADItem);

    interComp[qMakePair<int, int>(atomicNum1, atomicNum2)].minIAD = minIAD;
    xtalopt->interComp[qMakePair<int, int>(atomicNum1, atomicNum2)].minIAD =
      minIAD;

    if (atomicNum1 != atomicNum2) {
      xtalopt->interComp[qMakePair<int, int>(atomicNum2, atomicNum1)].minIAD =
        minIAD;
      interComp[qMakePair<int, int>(atomicNum2, atomicNum1)].minIAD = minIAD;
    }
  }
}

// Updates the UI by disabling/enabling options for initialization
void TabStruc::updateInitOptions()
{
  // We can't use molUnit in conjunction with randSpg
  if (ui.cb_useMolUnit->isChecked()) {
    ui.cb_allowRandSpg->setChecked(false);
    ui.cb_allowRandSpg->setEnabled(false);
  } else {
    ui.cb_allowRandSpg->setEnabled(true);
  }
  // We can't use molUnit in conjunction with randSpg
  if (ui.cb_allowRandSpg->isChecked()) {
    // Disable all molUnit stuff
    ui.cb_useMolUnit->setChecked(false);
    ui.cb_useMolUnit->setEnabled(false);
  } else {
    ui.cb_useMolUnit->setEnabled(true);
  }
  updateDimensions();
}

void TabStruc::updateMolUnits()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  disconnect(ui.table_molUnit, 0, 0, 0);

  QHash<QPair<int, int>, MolUnit> compMolUnit;
  compMolUnit.clear();

  xtalopt->compMolUnit.clear();

  ui.table_comp->rowCount();
  unsigned int numRowsMolUnit = ui.table_molUnit->rowCount();
  if (numRowsMolUnit == 0) {
    xtalopt->compMolUnit.clear();
    return;
  }

  // Build table - forward
  for (int i = 0; i < numRowsMolUnit; i++) {
    QString center =
      qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, MC_CENTER))
        ->currentText();
    int centerNum;
    if (center == "None")
      centerNum = 0;
    else
      centerNum = ElementInfo::getAtomicNum(center.trimmed().toStdString());
    QString neighbor =
      qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, MC_NEIGHBOR))
        ->currentText();
    int neighborNum = ElementInfo::getAtomicNum(neighbor.trimmed().toStdString());

    // Update center and neighbor lists
    QList<QString> centerList;
    QList<QString> neighborList;

    this->getCentersAndNeighbors(centerList, centerNum, neighborList,
                                 neighborNum);

    if (centerList.isEmpty() || neighborList.isEmpty())
      return;

    for (int k = 0; k < neighborList.size(); k++) {
      int n =
        ElementInfo::getAtomicNum(neighborList.at(k).trimmed().toStdString());
      if (xtalopt->compMolUnit.contains(qMakePair<int, int>(centerNum, n)) &&
          n != neighborNum) {
        neighborList.removeAt(k);
        k--;
      }
    }

    QComboBox* combo_center = new QComboBox();
    combo_center->insertItems(0, centerList);
    if (centerList.contains(center)) {
      combo_center->setCurrentIndex(combo_center->findText(center));
    }
    ui.table_molUnit->setCellWidget(i, MC_CENTER, combo_center);
    connect(combo_center, SIGNAL(currentIndexChanged(int)), this,
            SLOT(updateMolUnits()));

    QComboBox* combo_neighbor = new QComboBox();
    combo_neighbor->insertItems(0, neighborList);
    if (neighborList.contains(neighbor)) {
      combo_neighbor->setCurrentIndex(combo_neighbor->findText(neighbor));
    }
    ui.table_molUnit->setCellWidget(i, MC_NEIGHBOR, combo_neighbor);
    connect(combo_neighbor, SIGNAL(currentIndexChanged(int)), this,
            SLOT(updateMolUnits()));

    center =
      qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, MC_CENTER))
        ->currentText();
    if (center == "None")
      centerNum = 0;
    else
      centerNum = ElementInfo::getAtomicNum(center.trimmed().toStdString());
    neighbor =
      qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, MC_NEIGHBOR))
        ->currentText();
    neighborNum = ElementInfo::getAtomicNum(neighbor.trimmed().toStdString());

    // Number of Centers
    unsigned int numCenters =
      qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, MC_NUMCENTERS))
        ->currentText()
        .toUInt();
    QList<QString> numCentersList;

    this->getNumCenters(centerNum, neighborNum, numCentersList);

    QComboBox* combo_numCenters = new QComboBox();
    combo_numCenters->insertItems(0, numCentersList);
    if (numCentersList.contains(QString::number(numCenters))) {
      combo_numCenters->setCurrentIndex(
        combo_numCenters->findText(QString::number(numCenters)));
    }
    ui.table_molUnit->setCellWidget(i, MC_NUMCENTERS, combo_numCenters);
    connect(combo_numCenters, SIGNAL(currentIndexChanged(int)), this,
            SLOT(updateMolUnits()));

    if (numCentersList.isEmpty())
      return;

    numCenters =
      qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, MC_NUMCENTERS))
        ->currentText()
        .toUInt();

    compMolUnit[qMakePair<int, int>(centerNum, neighborNum)].numCenters =
      numCenters;

    // Number of Neighbors
    QList<QString> numNeighborsList;
    unsigned int numNeighbors =
      qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, MC_NUMNEIGHBORS))
        ->currentText()
        .toUInt();
    this->getNumNeighbors(centerNum, neighborNum, numNeighborsList);

    QComboBox* combo_numNeighbors = new QComboBox();
    combo_numNeighbors->insertItems(0, numNeighborsList);
    if (numNeighborsList.contains(QString::number(numNeighbors))) {
      combo_numNeighbors->setCurrentIndex(
        combo_numNeighbors->findText(QString::number(numNeighbors)));
    }
    ui.table_molUnit->setCellWidget(i, MC_NUMNEIGHBORS, combo_numNeighbors);
    connect(combo_numNeighbors, SIGNAL(currentIndexChanged(int)), this,
            SLOT(updateMolUnits()));

    if (numNeighborsList.isEmpty())
      return;

    numNeighbors =
      qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, MC_NUMNEIGHBORS))
        ->currentText()
        .toUInt();

    compMolUnit[qMakePair<int, int>(centerNum, neighborNum)].numNeighbors =
      numNeighbors;

    // Geometry
    unsigned int geom =
      qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, MC_GEOM))
        ->currentText()
        .toUInt();
    QString strGeom =
      qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, MC_GEOM))
        ->currentText();
    QList<QString> geomList;

    this->getGeom(geomList, numNeighbors);

    QComboBox* combo_geom = new QComboBox();
    combo_geom->insertItems(0, geomList);
    if (geomList.contains(strGeom)) {
      combo_geom->setCurrentIndex(combo_geom->findText(strGeom));
    }
    ui.table_molUnit->setCellWidget(i, MC_GEOM, combo_geom);
    connect(combo_geom, SIGNAL(currentIndexChanged(int)), this,
            SLOT(updateMolUnits()));

    geom = qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, MC_GEOM))
             ->currentText()
             .toUInt();
    strGeom = qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, MC_GEOM))
                ->currentText();
    this->setGeom(geom, strGeom);

    compMolUnit[qMakePair<int, int>(centerNum, neighborNum)].geom = geom;

    // MolUnit distance
    double dist = ui.table_molUnit->item(i, MC_DIST)->text().toDouble();
    QString strDist = QString::number(dist, 'f', 3);
    QTableWidgetItem* distItem = new QTableWidgetItem(strDist);
    ui.table_molUnit->setItem(i, MC_DIST, distItem);
    dist = ui.table_molUnit->item(i, MC_DIST)->text().toDouble();

    compMolUnit[qMakePair<int, int>(centerNum, neighborNum)].dist = dist;

    // Update the global hash
    xtalopt->compMolUnit = compMolUnit;
  }

  // Go through table again - backward
  for (int i = numRowsMolUnit - 1; i >= 0; i--) {
    QString center =
      qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, MC_CENTER))
        ->currentText();
    int centerNum;
    if (center == "None")
      centerNum = 0;
    else
      centerNum = ElementInfo::getAtomicNum(center.trimmed().toStdString());
    QString neighbor =
      qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, MC_NEIGHBOR))
        ->currentText();
    int neighborNum = ElementInfo::getAtomicNum(neighbor.trimmed().toStdString());

    // Update center and neighbor lists
    QList<QString> centerList;
    QList<QString> neighborList;

    this->getCentersAndNeighbors(centerList, centerNum, neighborList,
                                 neighborNum);

    if (centerList.isEmpty() || neighborList.isEmpty())
      return;

    for (int k = 0; k < neighborList.size(); k++) {
      int n =
        ElementInfo::getAtomicNum(neighborList.at(k).trimmed().toStdString());
      if (xtalopt->compMolUnit.contains(qMakePair<int, int>(centerNum, n)) &&
          n != neighborNum) {
        neighborList.removeAt(k);
        k--;
      }
    }

    QComboBox* combo_center = new QComboBox();
    combo_center->insertItems(0, centerList);
    if (centerList.contains(center)) {
      combo_center->setCurrentIndex(combo_center->findText(center));
    }
    ui.table_molUnit->setCellWidget(i, MC_CENTER, combo_center);
    connect(combo_center, SIGNAL(currentIndexChanged(int)), this,
            SLOT(updateMolUnits()));

    QComboBox* combo_neighbor = new QComboBox();
    combo_neighbor->insertItems(0, neighborList);
    if (neighborList.contains(neighbor)) {
      combo_neighbor->setCurrentIndex(combo_neighbor->findText(neighbor));
    }
    ui.table_molUnit->setCellWidget(i, MC_NEIGHBOR, combo_neighbor);
    connect(combo_neighbor, SIGNAL(currentIndexChanged(int)), this,
            SLOT(updateMolUnits()));

    center =
      qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, MC_CENTER))
        ->currentText();
    if (center == "None")
      centerNum = 0;
    else
      centerNum = ElementInfo::getAtomicNum(center.trimmed().toStdString());
    neighbor =
      qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, MC_NEIGHBOR))
        ->currentText();
    neighborNum = ElementInfo::getAtomicNum(neighbor.trimmed().toStdString());

    // Number of Centers
    unsigned int numCenters =
      qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, MC_NUMCENTERS))
        ->currentText()
        .toUInt();
    QList<QString> numCentersList;

    this->getNumCenters(centerNum, neighborNum, numCentersList);

    QComboBox* combo_numCenters = new QComboBox();
    combo_numCenters->insertItems(0, numCentersList);
    if (numCentersList.contains(QString::number(numCenters))) {
      combo_numCenters->setCurrentIndex(
        combo_numCenters->findText(QString::number(numCenters)));
    }
    ui.table_molUnit->setCellWidget(i, MC_NUMCENTERS, combo_numCenters);
    connect(combo_numCenters, SIGNAL(currentIndexChanged(int)), this,
            SLOT(updateMolUnits()));

    numCenters =
      qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, MC_NUMCENTERS))
        ->currentText()
        .toUInt();

    compMolUnit[qMakePair<int, int>(centerNum, neighborNum)].numCenters =
      numCenters;

    // Number of Neighbors
    QList<QString> numNeighborsList;
    unsigned int numNeighbors =
      qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, MC_NUMNEIGHBORS))
        ->currentText()
        .toUInt();
    this->getNumNeighbors(centerNum, neighborNum, numNeighborsList);

    QComboBox* combo_numNeighbors = new QComboBox();
    combo_numNeighbors->insertItems(0, numNeighborsList);
    if (numNeighborsList.contains(QString::number(numNeighbors))) {
      combo_numNeighbors->setCurrentIndex(
        combo_numNeighbors->findText(QString::number(numNeighbors)));
    }
    ui.table_molUnit->setCellWidget(i, MC_NUMNEIGHBORS, combo_numNeighbors);
    connect(combo_numNeighbors, SIGNAL(currentIndexChanged(int)), this,
            SLOT(updateMolUnits()));

    numNeighbors =
      qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, MC_NUMNEIGHBORS))
        ->currentText()
        .toUInt();

    compMolUnit[qMakePair<int, int>(centerNum, neighborNum)].numNeighbors =
      numNeighbors;

    // Geometry
    unsigned int geom =
      qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, MC_GEOM))
        ->currentText()
        .toUInt();
    QString strGeom =
      qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, MC_GEOM))
        ->currentText();
    QList<QString> geomList;

    this->getGeom(geomList, numNeighbors);

    QComboBox* combo_geom = new QComboBox();
    combo_geom->insertItems(0, geomList);
    if (geomList.contains(strGeom)) {
      combo_geom->setCurrentIndex(combo_geom->findText(strGeom));
    }
    ui.table_molUnit->setCellWidget(i, MC_GEOM, combo_geom);
    connect(combo_geom, SIGNAL(currentIndexChanged(int)), this,
            SLOT(updateMolUnits()));

    geom = qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, MC_GEOM))
             ->currentText()
             .toUInt();
    strGeom = qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, MC_GEOM))
                ->currentText();
    this->setGeom(geom, strGeom);

    compMolUnit[qMakePair<int, int>(centerNum, neighborNum)].geom = geom;

    // MolUnit distance
    double dist = ui.table_molUnit->item(i, MC_DIST)->text().toDouble();
    QString strDist = QString::number(dist, 'f', 3);
    QTableWidgetItem* distItem = new QTableWidgetItem(strDist);
    ui.table_molUnit->setItem(i, MC_DIST, distItem);
    dist = ui.table_molUnit->item(i, MC_DIST)->text().toDouble();

    compMolUnit[qMakePair<int, int>(centerNum, neighborNum)].dist = dist;

    connect(ui.table_molUnit, SIGNAL(itemSelectionChanged()), this,
            SLOT(updateMolUnits()));

    // Update the global hash
    xtalopt->compMolUnit = compMolUnit;
  }
}

// Actions for buttons to add/remove rows from the mol unit table
void TabStruc::addRow()
{
  disconnect(ui.table_molUnit, 0, 0, 0);

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  QHash<QPair<int, int>, MolUnit> compMolUnit;
  compMolUnit.clear();

  QList<unsigned int> keys = xtalopt->getMinimalComposition().getAtomicNumbers();
  std::sort(keys.begin(), keys.end());
  int numKeys = keys.size();

  QList<QPair<int, int>> keysMolUnit = xtalopt->compMolUnit.keys();
  std::sort(keysMolUnit.begin(), keysMolUnit.end());
  int numKeysMolUnit = keysMolUnit.size();

  QList<unsigned int> firstKeys;
  firstKeys.clear();
  QList<unsigned int> secondKeys;
  secondKeys.clear();
  for (int i = 0; i < numKeysMolUnit; i++) {
    firstKeys.append((keysMolUnit.at(i).first));
    secondKeys.append((keysMolUnit.at(i).second));
  }

  if (numKeys == 0)
    return;

  // Center and Neighbor Lists
  int centerNum = 0;
  QList<QString> centerList;
  centerList.clear();
  int neighborNum = 0;
  QList<QString> neighborList;
  neighborList.clear();

  this->getCentersAndNeighbors(centerList, centerNum, neighborList,
                               neighborNum);

  if (centerList.isEmpty() || neighborList.isEmpty())
    return;

  QString center = centerList.at(0);
  if (center == "None")
    centerNum = 0;
  else
    centerNum = ElementInfo::getAtomicNum(center.trimmed().toStdString());
  QString neighbor = neighborList.at(0);
  neighborNum = ElementInfo::getAtomicNum(neighbor.trimmed().toStdString());

  if (xtalopt->compMolUnit.contains(
        qMakePair<int, int>(centerNum, neighborNum))) {
    neighborList.removeAt(0);
    if (centerList.isEmpty() || neighborList.isEmpty())
      return;
    neighbor = neighborList.at(0);
    neighborNum = ElementInfo::getAtomicNum(neighbor.trimmed().toStdString());
  }

  // Number of Centers
  int numCenters = 0;
  QList<QString> numCentersList;
  numCentersList.clear();

  this->getNumCenters(centerNum, neighborNum, numCentersList);

  if (numCentersList.isEmpty())
    return;

  numCenters = numCentersList.at(0).toInt();

  compMolUnit[qMakePair<int, int>(centerNum, neighborNum)].numCenters =
    numCenters;

  // Number of Neighbors
  int numNeighbors = 0;
  QList<QString> numNeighborsList;
  numNeighborsList.clear();

  this->getNumNeighbors(centerNum, neighborNum, numNeighborsList);

  if (numNeighborsList.isEmpty())
    return;

  numNeighbors = numNeighborsList.at(0).toInt();

  compMolUnit[qMakePair<int, int>(centerNum, neighborNum)].numNeighbors =
    numNeighbors;

  // Determine possible geometries for the number of neigbors
  QList<QString> geomList;
  geomList.clear();

  this->getGeom(geomList, numNeighbors);

  unsigned int geom = geomList.at(0).toUInt();
  compMolUnit[qMakePair<int, int>(centerNum, neighborNum)].geom = geom;

  // Distance
  double distNum = 1.0;
  if (centerNum != 0)
    distNum = ElementInfo::getCovalentRadius(centerNum) +
              ElementInfo::getCovalentRadius(neighborNum);
  QString dist = QString::number(distNum, 'f', 3);

  compMolUnit[qMakePair<int, int>(centerNum, neighborNum)].dist = distNum;

  // Build table
  int row = ui.table_molUnit->rowCount();
  ui.table_molUnit->insertRow(row);

  QComboBox* combo_center = new QComboBox();
  combo_center->insertItems(0, centerList);
  ui.table_molUnit->setCellWidget(row, MC_CENTER, combo_center);
  connect(combo_center, SIGNAL(currentIndexChanged(int)), this,
          SLOT(updateMolUnits()));

  QComboBox* combo_numCenters = new QComboBox();
  combo_numCenters->insertItems(0, numCentersList);
  ui.table_molUnit->setCellWidget(row, MC_NUMCENTERS, combo_numCenters);
  connect(combo_numCenters, SIGNAL(currentIndexChanged(int)), this,
          SLOT(updateMolUnits()));

  QComboBox* combo_neighbor = new QComboBox();
  combo_neighbor->insertItems(0, neighborList);
  ui.table_molUnit->setCellWidget(row, MC_NEIGHBOR, combo_neighbor);
  connect(combo_neighbor, SIGNAL(currentIndexChanged(int)), this,
          SLOT(updateMolUnits()));

  QComboBox* combo_numNeighbors = new QComboBox();
  combo_numNeighbors->insertItems(0, numNeighborsList);
  ui.table_molUnit->setCellWidget(row, MC_NUMNEIGHBORS, combo_numNeighbors);
  connect(combo_numNeighbors, SIGNAL(currentIndexChanged(int)), this,
          SLOT(updateMolUnits()));

  QComboBox* combo_geom = new QComboBox();
  combo_geom->insertItems(0, geomList);
  ui.table_molUnit->setCellWidget(row, MC_GEOM, combo_geom);
  connect(combo_geom, SIGNAL(currentIndexChanged(int)), this,
          SLOT(updateMolUnits()));

  QTableWidgetItem* distItem = new QTableWidgetItem(dist);
  ui.table_molUnit->setItem(row, MC_DIST, distItem);
  connect(ui.table_molUnit, SIGNAL(itemChanged(QTableWidgetItem*)), this,
          SLOT(updateMolUnits()));

  connect(ui.table_molUnit, SIGNAL(itemSelectionChanged()), this,
          SLOT(updateMolUnits()));

  // Update the global hash
  xtalopt->compMolUnit = compMolUnit;

  this->updateMolUnits();
}

void TabStruc::removeRow()
{
  disconnect(ui.table_molUnit, 0, 0, 0);
  ui.table_molUnit->removeRow(ui.table_molUnit->currentRow());

  connect(ui.table_molUnit, SIGNAL(itemSelectionChanged()), this,
          SLOT(updateMolUnits()));

  this->updateMolUnits();
}

void TabStruc::removeAll()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);
  xtalopt->compMolUnit.clear();

  int row = ui.table_molUnit->rowCount();

  disconnect(ui.table_molUnit, 0, 0, 0);
  for (int i = row - 1; i >= 0; i--) {
    ui.table_molUnit->removeRow(i);
  }
  connect(ui.table_molUnit, SIGNAL(itemSelectionChanged()), this,
          SLOT(updateMolUnits()));

  this->updateMolUnits();
}

void TabStruc::getCentersAndNeighbors(QList<QString>& centerList, int centerNum,
                                     QList<QString>& neighborList,
                                     int neighborNum)
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  QList<unsigned int> keys = xtalopt->getMinimalComposition().getAtomicNumbers();
  std::sort(keys.begin(), keys.end());
  int numKeys = keys.size();

  QList<QPair<int, int>> keysMolUnit = xtalopt->compMolUnit.keys();
  std::sort(keysMolUnit.begin(), keysMolUnit.end());
  int numKeysMolUnit = keysMolUnit.size();

  QList<unsigned int> firstKeys;
  firstKeys.clear();
  QList<unsigned int> secondKeys;
  secondKeys.clear();

  for (int j = 0; j < numKeysMolUnit; j++) {
    firstKeys.append((keysMolUnit.at(j).first));
    secondKeys.append((keysMolUnit.at(j).second));
  }

  for (int j = 0; j < numKeys; j++) {
    unsigned int atomicNum = keys.at(j);
    QString symbol;
    if (atomicNum != 0) {
      symbol = QString(ElementInfo::getAtomicSymbol((atomicNum)).c_str());
    }
    unsigned int qComp = xtalopt->getMinimalComposition().getCount(symbol);

    // Add center atom to list
    unsigned int qMolUnit = 0;
    for (QHash<QPair<int, int>, MolUnit>::const_iterator
           it = xtalopt->compMolUnit.constBegin(),
           it_end = xtalopt->compMolUnit.constEnd();
         it != it_end; it++) {
      unsigned int first = const_cast<QPair<int, int>&>(it.key()).first;
      unsigned int second = const_cast<QPair<int, int>&>(it.key()).second;
      if (first == atomicNum) {
        if (centerNum != first || neighborNum != second) {
          qMolUnit += it->numCenters;
        }
      }
      if (second == atomicNum) {
        if (centerNum != first || neighborNum != second) {
          qMolUnit += it->numCenters * it->numNeighbors;
        }
      }
    }

    if (qComp > qMolUnit) {
      centerList.append(symbol);
      if (qComp - qMolUnit >= 1)
        neighborList.append(symbol);
    }
  }

  if (numKeys == 1)
    centerList.prepend("None");
  else
    centerList.append("None");
}

void TabStruc::getNumCenters(int centerNum, int neighborNum,
                            QList<QString>& numCentersList)
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  int q = 0;

  for (QHash<QPair<int, int>, MolUnit>::const_iterator
         it = xtalopt->compMolUnit.constBegin(),
         it_end = xtalopt->compMolUnit.constEnd();
       it != it_end; it++) {
    if (it.key() != QPair<int, int>(centerNum, neighborNum) &&
        it.key().first == centerNum)
      q += it->numCenters;
    if (it.key() != QPair<int, int>(centerNum, neighborNum) &&
        it.key().second == centerNum)
      q += it->numNeighbors;
  }

  int numCenters = xtalopt->getMinimalComposition().getCount(centerNum) - q;

  if (centerNum == neighborNum)
    numCenters /= 2;

  if (centerNum == 0)
    numCenters = 6;

  if (numCenters == 0)
    return;

  for (int i = numCenters; i > 0; i--) {
    numCentersList.append(QString::number(i));
  }
}

void TabStruc::getNumNeighbors(int centerNum, int neighborNum,
                              QList<QString>& numNeighborsList)
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  int numCenters = 0;
  int q = 0;
  bool divide = false;

  for (QHash<QPair<int, int>, MolUnit>::const_iterator
         it = xtalopt->compMolUnit.constBegin(),
         it_end = xtalopt->compMolUnit.constEnd();
       it != it_end; it++) {
    if (it.key() != QPair<int, int>(centerNum, neighborNum) &&
        it.key().first == neighborNum)
      q += it->numCenters;
    if (it.key() != QPair<int, int>(centerNum, neighborNum) &&
        it.key().second == neighborNum)
      q += it->numNeighbors * it->numCenters;
    if (it.key() == QPair<int, int>(centerNum, neighborNum)) {
      numCenters = it->numCenters;
      divide = true;
      if (centerNum == neighborNum)
        q += it->numCenters;
    }
  }

  int numNeighbors = xtalopt->getMinimalComposition().getCount(neighborNum) - q;

  if (divide == true)
    numNeighbors = numNeighbors / numCenters;

  if (numNeighbors == 0) {
    return;
  }

  if (numNeighbors > 6)
    numNeighbors = 6;

  for (int j = numNeighbors; j > 0; j--) {
    numNeighborsList.append(QString::number(j));
  }
}

void TabStruc::getGeom(QList<QString>& geomList, unsigned int numNeighbors)
{
  geomList.clear();
  switch (numNeighbors) {
    case 1:
      geomList.append("Linear");
      break;
    case 2:
      geomList.append("Linear");
      geomList.append("Bent");
      break;
    case 3:
      geomList.append("Trigonal Planar");
      geomList.append("Trigonal Pyramidal");
      geomList.append("T-Shaped");
      break;
    case 4:
      geomList.append("Tetrahedral");
      geomList.append("See-Saw");
      geomList.append("Square Planar");
      break;
    case 5:
      geomList.append("Trigonal Bipyramidal");
      geomList.append("Square Pyramidal");
      break;
    case 6:
      geomList.append("Octahedral");
      break;
    default:
      break;
  }
}

void TabStruc::setGeom(unsigned int& geom, QString strGeom)
{
  // Two neighbors
  if (strGeom.contains("Linear")) {
    geom = 1;
  } else if (strGeom.contains("Bent")) {
    geom = 2;
    // Three neighbors
  } else if (strGeom.contains("Trigonal Planar")) {
    geom = 2;
  } else if (strGeom.contains("Trigonal Pyramidal")) {
    geom = 3;
  } else if (strGeom.contains("T-Shaped")) {
    geom = 4;
    // Four neighbors
  } else if (strGeom.contains("Tetrahedral")) {
    geom = 3;
  } else if (strGeom.contains("See-Saw")) {
    geom = 5;
  } else if (strGeom.contains("Square Planar")) {
    geom = 4;
    // Five neighbors
  } else if (strGeom.contains("Trigonal Bipyramidal")) {
    geom = 5;
  } else if (strGeom.contains("Square Pyramidal")) {
    geom = 6;
    // Six neighbors
  } else if (strGeom.contains("Octahedral")) {
    geom = 6;
    // Default
  } else {
    geom = 0;
  }
}

void TabStruc::openSpgOptions()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  // If compositions are not set yet, just return
  if (xtalopt->compList.isEmpty()) {
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
    // Display a message to ask the user to wait while the image is loading...
    QMessageBox msgBox;
    msgBox.setText("Calculating possible spacegroups for the given system."
                   " Please wait...");
    msgBox.setStandardButtons(QMessageBox::NoButton);
    msgBox.setWindowModality(Qt::NonModal);
    msgBox.open();
    QCoreApplication::processEvents();

    // Open up the RandSpg dialog
    m_spgOptions = new RandSpgDialog(xtalopt, xtalopt->dialog());

    // Close the mesage box
    msgBox.close();
  }

  // Display m_spgOptions
  m_spgOptions->exec();
}

void TabStruc::errorPromptWindow(const QString& instr)
{
  QMessageBox msgBox;
  msgBox.setText(instr);
  msgBox.exec();
}
}

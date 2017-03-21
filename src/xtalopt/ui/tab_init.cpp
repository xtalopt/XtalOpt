/**********************************************************************
  XtalOpt - Tools for advanced crystal optimization

  Copyright (C) 2009-2011 by David Lonie

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <xtalopt/ui/tab_init.h>
#include <xtalopt/ui/randSpgDialog.h>

#include <xtalopt/xtalopt.h>

#include <globalsearch/eleminfo.h>
#include <globalsearch/queuemanager.h>
#include <globalsearch/structure.h>
#include <globalsearch/utilities/fileutils.h>

#include <QSettings>

#include <QHeaderView>
#include <QTableWidget>
#include <QTableWidgetItem>

#include "dialog.h"

namespace XtalOpt {

  TabInit::TabInit( XtalOptDialog *parent, XtalOpt *p ) :
    AbstractTab(parent, p),
    m_spgOptions(nullptr)
  {
    ui.setupUi(m_tab_widget);

    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    xtalopt->loaded =   false;

    // Initialize the formula units list...
    if (xtalopt->formulaUnitsList.isEmpty()) {
      xtalopt->formulaUnitsList.append(1);
      // We need to append this one twice... lowestEnthalpyFUList.at(0) does not
      // correspond to anything. lowestEnthalpyFUList.at(1) corresponds to 1 FU
      xtalopt->lowestEnthalpyFUList.append(0);
      xtalopt->lowestEnthalpyFUList.append(0);
    }

    // Composition
    connect(ui.edit_composition, SIGNAL(textChanged(QString)),
            this, SLOT(getComposition(QString)));
    connect(ui.edit_composition, SIGNAL(editingFinished()),
            this, SLOT(updateComposition()));

    // Unit cell
    connect(ui.spin_a_min, SIGNAL(editingFinished()),
            this, SLOT(updateDimensions()));
    connect(ui.spin_b_min, SIGNAL(editingFinished()),
            this, SLOT(updateDimensions()));
    connect(ui.spin_c_min, SIGNAL(editingFinished()),
            this, SLOT(updateDimensions()));
    connect(ui.spin_alpha_min, SIGNAL(editingFinished()),
            this, SLOT(updateDimensions()));
    connect(ui.spin_beta_min, SIGNAL(editingFinished()),
            this, SLOT(updateDimensions()));
    connect(ui.spin_gamma_min, SIGNAL(editingFinished()),
            this, SLOT(updateDimensions()));
    connect(ui.spin_vol_min, SIGNAL(editingFinished()),
            this, SLOT(updateDimensions()));
    connect(ui.spin_a_max, SIGNAL(editingFinished()),
            this, SLOT(updateDimensions()));
    connect(ui.spin_b_max, SIGNAL(editingFinished()),
            this, SLOT(updateDimensions()));
    connect(ui.spin_c_max, SIGNAL(editingFinished()),
            this, SLOT(updateDimensions()));
    connect(ui.spin_alpha_max, SIGNAL(editingFinished()),
            this, SLOT(updateDimensions()));
    connect(ui.spin_beta_max, SIGNAL(editingFinished()),
            this, SLOT(updateDimensions()));
    connect(ui.spin_gamma_max, SIGNAL(editingFinished()),
            this, SLOT(updateDimensions()));
    connect(ui.spin_vol_max, SIGNAL(editingFinished()),
            this, SLOT(updateDimensions()));
    connect(ui.spin_fixedVolume, SIGNAL(editingFinished()),
            this, SLOT(updateDimensions()));
    connect(ui.cb_fixedVolume, SIGNAL(toggled(bool)),
            this, SLOT(updateDimensions()));

    // Mitosis
    connect(ui.cb_mitosis, SIGNAL(toggled(bool)),
            this, SLOT(updateNumDivisions()));
    connect(ui.combo_divisions, SIGNAL(activated(int)),
            this, SLOT(updateA()));
    connect(ui.combo_a, SIGNAL(activated(int)),
            this, SLOT(writeA()));
    connect(ui.combo_b, SIGNAL(activated(int)),
            this, SLOT(writeB()));
    connect(ui.combo_c, SIGNAL(activated(int)),
            this, SLOT(writeC()));
    connect(ui.cb_subcellPrint, SIGNAL(toggled(bool)),
            this, SLOT(updateDimensions()));

    // Interatomic Distances
    connect(ui.spin_scaleFactor, SIGNAL(valueChanged(double)),
            this, SLOT(updateDimensions()));
    connect(ui.spin_minRadius, SIGNAL(valueChanged(double)),
            this, SLOT(updateDimensions()));
    connect(ui.cb_interatomicDistanceLimit, SIGNAL(toggled(bool)),
            this, SLOT(updateDimensions()));

    // MolUnit builder
    connect(ui.cb_useMolUnit, SIGNAL(toggled(bool)),
            this, SLOT(updateDimensions()));
    connect(ui.table_molUnit, SIGNAL(itemSelectionChanged()),
            this, SLOT(updateIAD()));
    connect(ui.pushButton_addMolUnit, SIGNAL(clicked(bool)),
            this, SLOT(addRow()));
    connect(ui.pushButton_removeMolUnit, SIGNAL(clicked(bool)),
            this, SLOT(removeRow()));
    connect(ui.pushButton_removeAllMolUnit, SIGNAL(clicked(bool)),
            this, SLOT(removeAll()));

    // Formula unit
    connect(ui.edit_formula_units, SIGNAL(editingFinished()),
            this, SLOT(updateFormulaUnits()));
    connect(xtalopt, SIGNAL(updateFormulaUnitsListUIText()),
            this, SLOT(updateFormulaUnitsListUI()));
    connect(xtalopt, SIGNAL(updateVolumesToBePerFU(uint)),
            this, SLOT(adjustVolumesToBePerFU(uint)));

    // randSpg
    connect(ui.cb_allowRandSpg, SIGNAL(toggled(bool)),
            this, SLOT(updateDimensions()));
    connect(ui.push_spgOptions, SIGNAL(clicked()),
            this, SLOT(openSpgOptions()));

    // MolUnit, RandSpg, and Mitosis enabling/disabling of each other
    connect(ui.cb_useMolUnit, SIGNAL(toggled(bool)),
            this, SLOT(updateInitOptions()));
    connect(ui.cb_allowRandSpg, SIGNAL(toggled(bool)),
            this, SLOT(updateInitOptions()));
    connect(ui.cb_mitosis, SIGNAL(toggled(bool)),
            this, SLOT(updateInitOptions()));


    QHeaderView *horizontal = ui.table_comp->horizontalHeader();
    horizontal->setSectionResizeMode(QHeaderView::ResizeToContents);

    initialize();
  }

  TabInit::~TabInit()
  {
    if (m_spgOptions) delete m_spgOptions;
  }

  void TabInit::writeSettings(const QString &filename)
  {
    SETTINGS(filename);

    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    settings->beginGroup("xtalopt/init/");

    const int version = 2;
    settings->setValue("version", version);

    settings->setValue("limits/a/min",        xtalopt->a_min);
    settings->setValue("limits/b/min",        xtalopt->b_min);
    settings->setValue("limits/c/min",        xtalopt->c_min);
    settings->setValue("limits/a/max",        xtalopt->a_max);
    settings->setValue("limits/b/max",        xtalopt->b_max);
    settings->setValue("limits/c/max",        xtalopt->c_max);
    settings->setValue("limits/alpha/min",    xtalopt->alpha_min);
    settings->setValue("limits/beta/min",     xtalopt->beta_min);
    settings->setValue("limits/gamma/min",    xtalopt->gamma_min);
    settings->setValue("limits/alpha/max",    xtalopt->alpha_max);
    settings->setValue("limits/beta/max",     xtalopt->beta_max);
    settings->setValue("limits/gamma/max",    xtalopt->gamma_max);
    settings->setValue("limits/volume/min",   xtalopt->vol_min);
    settings->setValue("limits/volume/max",   xtalopt->vol_max);
    settings->setValue("limits/volume/fixed", xtalopt->vol_fixed);
    settings->setValue("limits/scaleFactor",  xtalopt->scaleFactor);
    settings->setValue("limits/minRadius",    xtalopt->minRadius);
    settings->setValue("using/fixedVolume",   xtalopt->using_fixed_volume);
    settings->setValue("using/mitosis",      xtalopt->using_mitosis);
    settings->setValue("using/subcellPrint",      xtalopt->using_subcellPrint);
    settings->setValue("limits/divisions",      xtalopt->divisions);
    settings->setValue("limits/ax",      xtalopt->ax);
    settings->setValue("limits/bx",      xtalopt->bx);
    settings->setValue("limits/cx",      xtalopt->cx);
    settings->setValue("using/interatomicDistanceLimit",
                       xtalopt->using_interatomicDistanceLimit);
    settings->setValue("using/molUnit",      xtalopt->using_molUnit);

    // Composition
    // We only want to save POTCAR info and Composition to the resume
    // file, not the main config file, so only dump the data here if
    // we are given a filename and it contains the string
    // "xtalopt.state"
    if (!filename.isEmpty() && filename.contains("xtalopt.state")) {
      settings->beginWriteArray("composition");
      QList<uint> keys = xtalopt->comp.keys();
      for (int i = 0; i < keys.size(); i++) {
        if (keys.at(i) == 0)
          continue;
        settings->setArrayIndex(i);
        settings->setValue("atomicNumber", keys.at(i));
        settings->setValue("quantity",
                           xtalopt->comp.value(keys.at(i)).quantity);
        settings->setValue("minRadius",
                           xtalopt->comp.value(keys.at(i)).minRadius);
      }
      settings->endArray();
    }

    // MoUnit composition
    if (!filename.isEmpty() && filename.contains("xtalopt.state") && xtalopt->using_molUnit == true) {
      settings->beginWriteArray("compMolUnit");
      unsigned int numRowsMolUnit = ui.table_molUnit->rowCount();
      for (int i = 0; i < numRowsMolUnit; i++) {
        settings->setArrayIndex(i);
        settings->setValue("center",
                           qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, IC_CENTER))->currentText());
        settings->setValue("number_of_centers",
                           qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, IC_NUMCENTERS))->currentText());
        settings->setValue("neighbor",
                           qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, IC_NEIGHBOR))->currentText());
        settings->setValue("number_of_neighbors",
                           qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, IC_NUMNEIGHBORS))->currentText());
        settings->setValue("geometry",
                           qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, IC_GEOM))->currentText());
        settings->setValue("distance",
                           ui.table_molUnit->item(i, IC_DIST)->text());
      }
      settings->endArray();
    }

    // Formula Units List
    if (!filename.isEmpty() && filename.contains("xtalopt.state")) {
      settings->beginWriteArray("Formula_Units");
      QList<uint> tempFormulaUnitsList = xtalopt->formulaUnitsList;
      for (int i = 0; i < tempFormulaUnitsList.size(); i++) {
        settings->setArrayIndex(i);
        settings->setValue("FU", tempFormulaUnitsList.at(i));
      }
      settings->endArray();
    }

    settings->endGroup();

    DESTROY_SETTINGS(filename);
  }

  void TabInit::readSettings(const QString &filename)
  {
    SETTINGS(filename);

    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    settings->beginGroup("xtalopt/init/");
    int loadedVersion = settings->value("version", 0).toInt();

    ui.spin_a_min->setValue(		settings->value("limits/a/min",		3).toDouble()   );
    ui.spin_b_min->setValue(		settings->value("limits/b/min",		3).toDouble()   );
    ui.spin_c_min->setValue(		settings->value("limits/c/min",		3).toDouble()   );
    ui.spin_a_max->setValue(		settings->value("limits/a/max",		10).toDouble()  );
    ui.spin_b_max->setValue(		settings->value("limits/b/max",		10).toDouble()  );
    ui.spin_c_max->setValue(		settings->value("limits/c/max",		10).toDouble()  );
    ui.spin_alpha_min->setValue(	settings->value("limits/alpha/min",	60).toDouble()  );
    ui.spin_beta_min->setValue(		settings->value("limits/beta/min",	60).toDouble()  );
    ui.spin_gamma_min->setValue(	settings->value("limits/gamma/min",	60).toDouble()  );
    ui.spin_alpha_max->setValue(	settings->value("limits/alpha/max",	120).toDouble() );
    ui.spin_beta_max->setValue(		settings->value("limits/beta/max",	120).toDouble() );
    ui.spin_gamma_max->setValue(	settings->value("limits/gamma/max",	120).toDouble() );
    ui.spin_vol_min->setValue(		settings->value("limits/volume/min",	1).toDouble()   );
    ui.spin_vol_max->setValue(		settings->value("limits/volume/max",	100000).toDouble());
    ui.spin_fixedVolume->setValue(	settings->value("limits/volume/fixed",	500).toDouble()	);
    ui.spin_scaleFactor->setValue(	settings->value("limits/scaleFactor",0.5).toDouble());
    ui.spin_minRadius->setValue(    settings->value("limits/minRadius",0.25).toDouble());
    ui.cb_fixedVolume->setChecked(	settings->value("using/fixedVolume",	false).toBool()	);
    ui.cb_interatomicDistanceLimit->setChecked( settings->value("using/interatomicDistanceLimit",false).toBool());
    ui.cb_mitosis->setChecked(      settings->value("using/mitosis",       false).toBool()     );
    ui.cb_subcellPrint->setChecked(      settings->value("using/subcellPrint",       false).toBool()     );
    ui.cb_useMolUnit->setChecked(      settings->value("using/molUnit",       false).toBool()     );
    xtalopt->divisions = settings->value("limits/divisions").toInt();
    xtalopt->ax = settings->value("limits/ax").toInt();
    xtalopt->bx = settings->value("limits/bx").toInt();
    xtalopt->cx = settings->value("limits/cx").toInt();
    updateNumDivisions();

    // Composition
    if (!filename.isEmpty()) {
      int size = settings->beginReadArray("composition");
      xtalopt->comp = QHash<uint,XtalCompositionStruct> ();
      for (int i = 0; i < size; i++) {
        settings->setArrayIndex(i);
        uint atomicNum, quantity;
        XtalCompositionStruct entry;
        atomicNum = settings->value("atomicNumber").toUInt();
        if (atomicNum == 0)
          continue;
        quantity = settings->value("quantity").toUInt();
        entry.quantity = quantity;
        xtalopt->comp.insert(atomicNum, entry);
      }
      this->updateMinRadii();
      settings->endArray();
    }

    if (!filename.isEmpty() && ui.cb_useMolUnit->isChecked() == true) {
        int size = settings->beginReadArray("compMolUnit");
        xtalopt->compMolUnit = QHash<QPair<int, int>, MolUnit> ();
        for (int i = 0; i < size; i++) {
          settings->setArrayIndex(i);
          int centerNum, numCenters, neighborNum, numNeighbors;
          unsigned int geom;
          double dist;
          MolUnit entry;

          QString center = settings->value("center").toString();
          centerNum = ElemInfo::getAtomicNum(center.trimmed().toStdString());
          QString strNumCenters = settings->value("number_of_centers").toString();
          numCenters = strNumCenters.toInt();
          QString neighbor = settings->value("neighbor").toString();
          neighborNum = ElemInfo::getAtomicNum(
                            neighbor.trimmed().toStdString());
          QString strNumNeighbors = settings->value("number_of_neighbors").toString();
          numNeighbors = strNumNeighbors.toInt();
          QString strGeom = settings->value("geometry").toString();
          this->setGeom(geom, strGeom);
          dist = settings->value("distance").toDouble();
          QString strDist = QString::number(dist, 'f', 3);
          entry.numCenters = numCenters;
          entry.numNeighbors = numNeighbors;
          entry.geom = geom;
          entry.dist = dist;

          xtalopt->compMolUnit.insert(qMakePair<int, int>(centerNum, neighborNum), entry);

          ui.table_molUnit->insertRow(i);

          QComboBox* combo_center = new QComboBox();
          combo_center->insertItem(0, center);
          ui.table_molUnit->setCellWidget(i, IC_CENTER, combo_center);
          connect(combo_center, SIGNAL(currentIndexChanged(int)),
                  this, SLOT(updateIAD()));

          QComboBox* combo_numCenters = new QComboBox();
          combo_numCenters->insertItem(0, strNumCenters);
          ui.table_molUnit->setCellWidget(i, IC_NUMCENTERS, combo_numCenters);
          connect(combo_numCenters, SIGNAL(currentIndexChanged(int)),
                  this, SLOT(updateIAD()));

          QComboBox* combo_neighbor = new QComboBox();
          combo_neighbor->insertItem(0, neighbor);
          ui.table_molUnit->setCellWidget(i, IC_NEIGHBOR, combo_neighbor);
          connect(combo_neighbor, SIGNAL(currentIndexChanged(int)),
                  this, SLOT(updateIAD()));

          QComboBox* combo_numNeighbors = new QComboBox();
          combo_numNeighbors->insertItem(0, strNumNeighbors);
          ui.table_molUnit->setCellWidget(i, IC_NUMNEIGHBORS, combo_numNeighbors);
          connect(combo_numNeighbors, SIGNAL(currentIndexChanged(int)),
                  this, SLOT(updateIAD()));

          QComboBox* combo_geom = new QComboBox();
          combo_geom->insertItem(0, strGeom);
          ui.table_molUnit->setCellWidget(i, IC_GEOM, combo_geom);
          connect(combo_geom, SIGNAL(currentIndexChanged(int)),
                  this, SLOT(updateIAD()));

          QTableWidgetItem *distItem = new QTableWidgetItem(strDist);
          ui.table_molUnit->setItem(i, IC_DIST, distItem);
          connect(ui.table_molUnit, SIGNAL(itemChanged(QTableWidgetItem *)),
                this, SLOT(updateIAD()));

        }
        this->updateIAD();
        settings->endArray();
    }

    // Formula Units List
    if (!filename.isEmpty()) {
      int size = settings->beginReadArray("Formula_Units");
      QString formulaUnits;
      formulaUnits.clear();
      for (int i = 0; i < size; i++) {
        settings->setArrayIndex(i);
        uint FU = settings->value("FU").toUInt();
        formulaUnits.append(QString::number(FU));
        formulaUnits.append(",");
      }
      ui.edit_formula_units->setText(formulaUnits);
      updateFormulaUnits();
      settings->endArray();
    }

    settings->endGroup();

    // Update config data
    switch (loadedVersion) {
    case 0:
    case 1:
      ui.cb_interatomicDistanceLimit->setChecked(
            settings->value("using/shortestInteratomicDistance",false).toBool());
    case 2:
    default:
      break;
    }

    // Enact changesSetup templates
    updateDimensions();
  }

  void TabInit::updateGUI()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    ui.spin_a_min->setValue(       xtalopt->a_min);
    ui.spin_b_min->setValue(       xtalopt->b_min);
    ui.spin_c_min->setValue(       xtalopt->c_min);
    ui.spin_a_max->setValue(       xtalopt->a_max);
    ui.spin_b_max->setValue(       xtalopt->b_max);
    ui.spin_c_max->setValue(       xtalopt->c_max);
    ui.spin_alpha_min->setValue(   xtalopt->alpha_min);
    ui.spin_beta_min->setValue(    xtalopt->beta_min);
    ui.spin_gamma_min->setValue(   xtalopt->gamma_min);
    ui.spin_alpha_max->setValue(   xtalopt->alpha_max);
    ui.spin_beta_max->setValue(    xtalopt->beta_max);
    ui.spin_gamma_max->setValue(   xtalopt->gamma_max);
    ui.spin_vol_min->setValue(     xtalopt->vol_min);
    ui.spin_vol_max->setValue(     xtalopt->vol_max);
    ui.spin_fixedVolume->setValue( xtalopt->vol_fixed);
    ui.spin_scaleFactor->setValue( xtalopt->scaleFactor);
    ui.spin_minRadius->setValue(   xtalopt->minRadius);
    ui.cb_fixedVolume->setChecked( xtalopt->using_fixed_volume);
    ui.cb_mitosis->setChecked( xtalopt->using_mitosis);
    ui.cb_subcellPrint->setChecked( xtalopt->using_subcellPrint);
    ui.combo_divisions->setItemText(ui.combo_divisions->currentIndex(), QString::number(xtalopt->divisions));
    ui.combo_a->setItemText(ui.combo_a->currentIndex(), QString::number(xtalopt->ax));
    ui.combo_b->setItemText(ui.combo_b->currentIndex(), QString::number(xtalopt->bx));
    ui.combo_c->setItemText(ui.combo_c->currentIndex(), QString::number(xtalopt->cx));
    ui.cb_interatomicDistanceLimit->setChecked(
          xtalopt->using_interatomicDistanceLimit);
    ui.cb_useMolUnit->setChecked( xtalopt->using_molUnit);
    ui.cb_allowRandSpg->setChecked( xtalopt->using_randSpg);

    updateComposition();
  }

  void TabInit::lockGUI()
  {
    ui.edit_composition->setDisabled(true);
    ui.cb_useMolUnit->setDisabled(true);
    ui.table_molUnit->setDisabled(true);
    ui.pushButton_addMolUnit->setDisabled(true);
    ui.pushButton_removeMolUnit->setDisabled(true);
    ui.pushButton_removeAllMolUnit->setDisabled(true);
    ui.cb_allowRandSpg->setDisabled(true);
    ui.push_spgOptions->setDisabled(true);
    ui.cb_mitosis->setDisabled(true);
    ui.combo_divisions->setDisabled(true);
    ui.combo_a->setDisabled(true);
    ui.combo_b->setDisabled(true);
    ui.combo_c->setDisabled(true);
    ui.cb_subcellPrint->setDisabled(true);
  }

  void TabInit::getComposition(const QString &str)
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    QHash<uint, XtalCompositionStruct> comp;
    QString symbol;
    unsigned int atomicNum;
    unsigned int quantity;
    QStringList symbolList;
    QStringList quantityList;

    // Parse numbers between letters
    symbolList = str.split(QRegExp("[0-9]"), QString::SkipEmptyParts);
    // Parse letters between numbers
    quantityList = str.split(QRegExp("[A-Z,a-z]"), QString::SkipEmptyParts);

    xtalopt->testingMode = (str.contains("testingMode")) ? true : false;

    // Use the shorter of the lists for the length
    unsigned int length = (symbolList.size() < quantityList.size())
        ? symbolList.size() : quantityList.size();

    if ( length == 0 ) {
      xtalopt->comp.clear();
      this->updateCompositionTable();
      return;
    }
    // Reduce to empirical formula
    if (quantityList.size() == symbolList.size()){
      unsigned int minimumQuantityOfAtomType = quantityList.at(0).toUInt();
      for (int i = 1; i < symbolList.size(); ++i) {
        if (minimumQuantityOfAtomType > quantityList.at(i).toUInt()){
          minimumQuantityOfAtomType = quantityList.at(i).toUInt();
        }
      }
      unsigned int numberOfFormulaUnits = 1;
      bool formulaUnitsFound;
      for (int i = minimumQuantityOfAtomType; i > 1; i--){
        formulaUnitsFound = true;
        for (int j = 0; j < symbolList.size(); ++j) {
          if(quantityList.at(j).toUInt() % i != 0){
            formulaUnitsFound = false;
          }
        }
        if(formulaUnitsFound == true) {
          numberOfFormulaUnits = i;
          i = 1;
          for (int k = 0; k < symbolList.size(); ++k) {
            quantityList[k] = QString::number(quantityList.at(k).toUInt() / numberOfFormulaUnits);
          }
        }
      }
    }

    // Build hash
    for (uint i = 0; i < length; i++){
      symbol    = symbolList.at(i);
      atomicNum = ElemInfo::getAtomicNum(symbol.trimmed().toStdString());
      quantity	= quantityList.at(i).toUInt();

      if (symbol.contains("nRunsStart")) {
        xtalopt->test_nRunsStart = quantity;
        continue;
      }
      if (symbol.contains("nRunsEnd")) {
        xtalopt->test_nRunsEnd = quantity;
        continue;
      }
      if (symbol.contains("nStructs")) {
        xtalopt->test_nStructs = quantity;
        continue;
      }

      // Validate symbol
      if (!atomicNum) continue; // Invalid symbol entered
      if (atomicNum == 0) continue;

      // Add to hash
      if (!comp.keys().contains(atomicNum)) {
        XtalCompositionStruct entry;
        entry.quantity = 0;
        entry.minRadius = 0.0;
        comp[atomicNum] = entry; // initialize if needed
      }

      comp[atomicNum].quantity += quantity;
    }

    // If we changed the composition, reset the spacegroup generation
    // min xtals per FU to be zero
    if (xtalopt->comp != comp && xtalopt->minXtalsOfSpgPerFU.size() != 0) {
      xtalopt->error(tr(
               "Warning: because the composition have been changed, "
               "the spacegroups to be generated using spacegroup "
               "initialization have been reset. Please open the spacegroup "
               "options to set them again."));
      xtalopt->minXtalsOfSpgPerFU = QList<int>();
    }

    xtalopt->comp = comp;

    this->updateMinRadii();
    this->updateCompositionTable();
    this->updateNumDivisions();
  }

  void TabInit::updateCompositionTable()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    QList<unsigned int> keys = xtalopt->comp.keys();
    keys.removeAll(0);
    qSort(keys);

    // Adjust table size:
    int numRows = keys.size();
    ui.table_comp->setRowCount(numRows);

    for (int i = 0; i < numRows; i++) {
      unsigned int atomicNum = keys.at(i);

      if (atomicNum == 0) continue;

      QString symbol	= ElemInfo::getAtomicSymbol(atomicNum).c_str();
      unsigned int quantity  = xtalopt->comp[atomicNum].quantity;
      double mass	= ElemInfo::getAtomicMass(atomicNum);
      double minRadius = xtalopt->comp[atomicNum].minRadius;

      QTableWidgetItem *symbolItem =
          new QTableWidgetItem(symbol);
      QTableWidgetItem *atomicNumItem =
          new QTableWidgetItem(QString::number(atomicNum));
      QTableWidgetItem *quantityItem =
          new QTableWidgetItem(QString::number(quantity));
      QTableWidgetItem *massItem =
          new QTableWidgetItem(QString::number(mass));
      QTableWidgetItem *minRadiusItem;
      if (xtalopt->using_interatomicDistanceLimit)
        minRadiusItem = new QTableWidgetItem(QString::number(minRadius));
      else
        minRadiusItem = new QTableWidgetItem(tr("n/a"));

      ui.table_comp->setItem(i, CC_SYMBOL, symbolItem);
      ui.table_comp->setItem(i, CC_ATOMICNUM, atomicNumItem);
      ui.table_comp->setItem(i, CC_QUANTITY, quantityItem);
      ui.table_comp->setItem(i, CC_MASS, massItem);
      ui.table_comp->setItem(i, CC_MINRADIUS, minRadiusItem);
    }
  }

  void TabInit::updateComposition()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    QList<uint> keys = xtalopt->comp.keys();
    qSort(keys);
    QString tmp;
    QTextStream str (&tmp);
    for (int i = 0; i < keys.size(); i++) {
      if (keys.at(i) == 0) continue;
      uint q = xtalopt->comp.value(keys.at(i)).quantity;
      str << ElemInfo::getAtomicSymbol(keys.at(i)).c_str() << q << " ";
    }
    if (xtalopt->testingMode) {
      str << "nRunsStart" << xtalopt->test_nRunsStart << " "
          << "nRunsEnd" << xtalopt->test_nRunsEnd << " "
          << "nStructs" << xtalopt->test_nStructs << " "
          << "testingMode ";
    }

    ui.edit_composition->setText(tmp.trimmed());

    this->updateIAD();
  }

  void TabInit::updateDimensions()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    // Check for conflicts -- favor lower value
    if (ui.spin_a_min->value()		> ui.spin_a_max->value())	ui.spin_a_max->setValue(	ui.spin_a_min->value());
    if (ui.spin_b_min->value()          > ui.spin_b_max->value())	ui.spin_b_max->setValue(	ui.spin_b_min->value());
    if (ui.spin_c_min->value()          > ui.spin_c_max->value())	ui.spin_c_max->setValue(	ui.spin_c_min->value());
    if (ui.spin_alpha_min->value()      > ui.spin_alpha_max->value())	ui.spin_alpha_max->setValue(	ui.spin_alpha_min->value());
    if (ui.spin_beta_min->value()       > ui.spin_beta_max->value())	ui.spin_beta_max->setValue(     ui.spin_beta_min->value());
    if (ui.spin_gamma_min->value()      > ui.spin_gamma_max->value())	ui.spin_gamma_max->setValue(	ui.spin_gamma_min->value());
    if (ui.spin_vol_min->value()        > ui.spin_vol_max->value())	ui.spin_vol_max->setValue(	ui.spin_vol_min->value());

    // Assign variables
    xtalopt->a_min		= ui.spin_a_min->value();
    xtalopt->b_min		= ui.spin_b_min->value();
    xtalopt->c_min		= ui.spin_c_min->value();
    xtalopt->alpha_min	= ui.spin_alpha_min->value();
    xtalopt->beta_min	= ui.spin_beta_min->value();
    xtalopt->gamma_min	= ui.spin_gamma_min->value();
    xtalopt->vol_min	= ui.spin_vol_min->value();

    xtalopt->a_max		= ui.spin_a_max->value();
    xtalopt->b_max		= ui.spin_b_max->value();
    xtalopt->c_max		= ui.spin_c_max->value();
    xtalopt->alpha_max	= ui.spin_alpha_max->value();
    xtalopt->beta_max	= ui.spin_beta_max->value();
    xtalopt->gamma_max	= ui.spin_gamma_max->value();
    xtalopt->vol_max    = ui.spin_vol_max->value();

    xtalopt->using_fixed_volume = ui.cb_fixedVolume->isChecked();
    xtalopt->vol_fixed	= ui.spin_fixedVolume->value();

    // Allow mitosis
    xtalopt->using_mitosis = ui.cb_mitosis->isChecked();
    xtalopt->divisions = ui.combo_divisions->currentText().toInt();
    xtalopt->using_subcellPrint = ui.cb_subcellPrint->isChecked();

    // Allow Molecular units
    xtalopt->using_molUnit = ui.cb_useMolUnit->isChecked();

    // Allow RandSpg
    xtalopt->using_randSpg = ui.cb_allowRandSpg->isChecked();
    if (xtalopt->using_molUnit == false)
       this->removeAll();

    if (xtalopt->scaleFactor != ui.spin_scaleFactor->value() ||
        xtalopt->minRadius   != ui.spin_minRadius->value() ||
        xtalopt->using_interatomicDistanceLimit !=
        ui.cb_interatomicDistanceLimit->isChecked()) {
      xtalopt->scaleFactor = ui.spin_scaleFactor->value();
      xtalopt->minRadius = ui.spin_minRadius->value();
      xtalopt->using_interatomicDistanceLimit =
          ui.cb_interatomicDistanceLimit->isChecked();
      this->updateMinRadii();
      this->updateCompositionTable();
    }
  }

  void TabInit::updateMinRadii()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    for (QHash<unsigned int, XtalCompositionStruct>::iterator
         it = xtalopt->comp.begin(), it_end = xtalopt->comp.end();
         it != it_end; ++it) {
      if (it.key() == 0) continue;
      it.value().minRadius = xtalopt->scaleFactor * ElemInfo::getCovalentRadius(it.key());
      // Ensure that all minimum radii are > 0.25 (esp. H!)
      if (it.value().minRadius < xtalopt->minRadius) {
        it.value().minRadius = xtalopt->minRadius;
      }
    }
  }

  void TabInit::updateFormulaUnits()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    QString tmp;

    QList<uint> formulaUnitsList = FileUtils::parseUIntString(ui.edit_formula_units->text(), tmp);

    // If nothing valid was obtained, return 1
    if (formulaUnitsList.size() == 0) {
      xtalopt->formulaUnitsList.append(1);
      tmp = "1";
      ui.edit_formula_units->setText(tmp.trimmed());
      return;
    }

    // Reset the supercell checks
    QList<GlobalSearch::Structure*> structures (*m_opt->tracker()->list());
    for (size_t i = 0; i < structures.size(); i++) {
      structures.at(i)->setSupercellGenerationChecked(false);
    }

    // If we changed the formula units, reset the spacegroup generation
    // min xtals per FU to be zero
    if (xtalopt->formulaUnitsList != formulaUnitsList &&
        xtalopt->minXtalsOfSpgPerFU.size() != 0) {
      // We're assuming that if the composition is enabled, the run has not
      // yet begun. This is probably a valid assumption (unless we allow the
      // composition to be changed during the run in the future)
      if (ui.edit_composition->isEnabled()) {
        xtalopt->error(tr(
                 "Warning: because the formula units have been changed, "
                 "the spacegroups to be generated using spacegroup "
                 "initialization have been reset. Please open the spacegroup "
                 "options to set them again."));
      }
      xtalopt->minXtalsOfSpgPerFU = QList<int>();
    }

    xtalopt->formulaUnitsList = formulaUnitsList;

    // Update the size of the lowestEnthalpyFUList
    while (xtalopt->lowestEnthalpyFUList.size() <= xtalopt->maxFU())
      xtalopt->lowestEnthalpyFUList.append(0);

    // Update UI
    ui.edit_formula_units->setText(tmp.trimmed());

    // Update the nubmer of divisions
    this->updateNumDivisions();
  }

  // Updates the UI with the contents of xtalopt->formulaUnitsList
  void TabInit::updateFormulaUnitsListUI()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);
    QString tmp;
    QList<uint> formulaUnitsList = xtalopt->formulaUnitsList;
    for (size_t i = 0; i < formulaUnitsList.size(); i++) {
      tmp += QString::number(formulaUnitsList.at(i)) + ", ";
    }
    ui.edit_formula_units->setText(tmp);
    updateFormulaUnits();
  }

  // Updates the UI by disabling/enabling options for initialization
  void TabInit::updateInitOptions()
  {
    // We can't use molUnit or mitosis in conjunction with randSpg
    if (ui.cb_useMolUnit->isChecked() || ui.cb_mitosis->isChecked()) {
      ui.cb_allowRandSpg->setChecked(false);
      ui.cb_allowRandSpg->setEnabled(false);
    }
    else {
      ui.cb_allowRandSpg->setEnabled(true);
    }
    // We can't use molUnit or mitosis in conjunction with randSpg
    if (ui.cb_allowRandSpg->isChecked()) {
      // Disable all molUnit stuff
      ui.cb_useMolUnit->setChecked(false);
      ui.cb_useMolUnit->setEnabled(false);

      // Disable all mitosis stuff
      ui.cb_mitosis->setChecked(false);
      ui.cb_mitosis->setEnabled(false);
    }
    else {
      ui.cb_useMolUnit->setEnabled(true);
      ui.cb_mitosis->setEnabled(true);
    }
    updateDimensions();
  }

  // This is only used when resuming older version of XtalOpt
  // It adjusts the volumes so that they are per FU instead of just
  // pure volumes
  void TabInit::adjustVolumesToBePerFU(uint FU)
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);
    ui.spin_vol_min->setValue(ui.spin_vol_min->value() /
      static_cast<double>(FU));
    ui.spin_vol_max->setValue(ui.spin_vol_max->value() /
      static_cast<double>(FU));
    ui.spin_fixedVolume->setValue(ui.spin_fixedVolume->value() /
      static_cast<double>(FU));
    xtalopt->vol_min = ui.spin_vol_min->value();
    xtalopt->vol_max = ui.spin_vol_max->value();
    xtalopt->vol_fixed = ui.spin_fixedVolume->value();
  }

  // Determine the possible number of divisions for mitosis and update combobox with options
  void TabInit::updateNumDivisions()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    int counter = 0;
    QList<QString> divisions;
    QList<uint> atomicNums = xtalopt->comp.keys();
    size_t smallestFormulaUnit = xtalopt->formulaUnitsList[0];
    if (xtalopt->formulaUnitsList.isEmpty()) smallestFormulaUnit = 1;

    if (ui.cb_mitosis->isChecked()){
        divisions.clear();
        ui.combo_divisions->clear();
        if (xtalopt->loaded==true) {
            ui.combo_divisions->insertItem(0, QString::number(xtalopt->divisions));
        } else {
        for (int j = 1000; j >= 1; --j) {
            for (int i = 0; i <= atomicNums.size()-1; ++i) {
                if ((xtalopt->comp.value(atomicNums[i]).quantity *
                     smallestFormulaUnit) % j > 0) {
                    if ((xtalopt->comp.value(atomicNums[i]).quantity *
                         smallestFormulaUnit) == 1) {
                        counter = 0;
                        break;
                    } else if ((xtalopt->comp.value(atomicNums[i]).quantity *
                                smallestFormulaUnit) / j > 0 &&
                               (xtalopt->comp.value(atomicNums[i]).quantity *
                                smallestFormulaUnit) % j <= 5) {
                        counter++;
                    } else {
                        counter = 0;
                        break;
                    }
                } else {
                    counter++;
                }
                if (counter == atomicNums.size()){
                    divisions.append(QString::number(j));
                    counter = 0;
                    break;
                }
            }
        }
        }
        ui.combo_divisions->insertItems(0, divisions);
    } else {
        ui.combo_divisions->clear();
        ui.combo_a->clear();
        ui.combo_b->clear();
        ui.combo_c->clear();
    }
    this->updateDimensions();
    this->updateA();
  }

  // Determine and update the number of divisions occurring in cell vector 'a' direction
  void TabInit::updateA()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    QList<QString> a;
    ui.combo_a->clear();

    this->updateDimensions();

    if (xtalopt->using_mitosis && xtalopt->divisions!=0){
      if (xtalopt->loaded==true) {
        ui.combo_a->insertItem(0, QString::number(xtalopt->ax));
        this->writeA();
      } else {
        int divide = xtalopt->divisions;
        for (int i = divide; i >=1; --i){
            if (divide % i == 0) {
                a.append(QString::number(i));
            }
        }
        ui.combo_a->insertItems(0, a);
        this->writeA();
      }
    }
  }

  void TabInit::writeA()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    xtalopt->ax = ui.combo_a->currentText().toInt();
    this->updateB();
  }

  // Determine and update the number of divisions occurring in cell vector 'b' direction
  void TabInit::updateB()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    QList<QString> b;
    ui.combo_b->clear();

    if (xtalopt->using_mitosis && xtalopt->divisions!=0){
       if (xtalopt->loaded==true) {
        ui.combo_b->insertItem(0, QString::number(xtalopt->bx));
        this->writeB();
      } else {
        int divide = xtalopt->divisions;
        int a = ui.combo_a->currentText().toInt();
        int diff = divide / a;

        for (int i = diff; i >=1; --i){
            if (diff % i == 0) {
                b.append(QString::number(i));
            }
        }
        ui.combo_b->insertItems(0, b);
        this->writeB();
    }
    }
  }

  void TabInit::writeB()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    xtalopt->bx = ui.combo_b->currentText().toInt();
    this->updateC();
  }

 // Determine and update the number of divisions occurring in cell vector 'c' direction
  void TabInit::updateC()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    QList<QString> c;
    ui.combo_c->clear();

    if (xtalopt->using_mitosis && xtalopt->divisions != 0){
      if (xtalopt->loaded==true) {
        ui.combo_c->insertItem(0, QString::number(xtalopt->cx));
        this->writeC();
      } else {
        int divide = xtalopt->divisions;
        int a = ui.combo_a->currentText().toInt();
        int b = ui.combo_b->currentText().toInt();
        int diff = (divide / a) / b;

        c.append(QString::number(diff));
        ui.combo_c->insertItems(0, c);

        this->writeC();
      }
    }
  }

  void TabInit::writeC()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    xtalopt->cx = ui.combo_c->currentText().toInt();
  }

  void TabInit::updateIAD()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    disconnect(ui.table_molUnit, 0, 0, 0);

    QHash<QPair<int, int>, MolUnit> compMolUnit;
    compMolUnit.clear();

    xtalopt->compMolUnit.clear();

    unsigned int numRows = ui.table_comp->rowCount();
    unsigned int numRowsMolUnit = ui.table_molUnit->rowCount();
    if (numRowsMolUnit == 0) {
      xtalopt->compMolUnit.clear();
      return;
    }

    //Build table - forward
    for (int i = 0; i < numRowsMolUnit; i++) {
      QString center = qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, IC_CENTER))->currentText();
      int centerNum;
      if (center == "None")
        centerNum = 0;
      else
        centerNum = ElemInfo::getAtomicNum(center.trimmed().toStdString());
      QString neighbor = qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, IC_NEIGHBOR))->currentText();
      int neighborNum =
          ElemInfo::getAtomicNum(neighbor.trimmed().toStdString());

      //Update center and neighbor lists
      QList<QString> centerList;
      QList<QString> neighborList;

      this->getCentersAndNeighbors(centerList, centerNum, neighborList, neighborNum);

      if (centerList.isEmpty() || neighborList.isEmpty())
        return;

      for(int k = 0; k < neighborList.size(); k++) {
        int n =
            ElemInfo::getAtomicNum(neighborList.at(k).trimmed().toStdString());
        if(xtalopt->compMolUnit.contains(qMakePair<int,int>(centerNum, n)) && n != neighborNum) {
          neighborList.removeAt(k);
          k--;
        }
      }

      QComboBox* combo_center = new QComboBox();
      combo_center->insertItems(0, centerList);
      if (centerList.contains(center)) {
        combo_center->setCurrentIndex(combo_center->findText(center));
      }
      ui.table_molUnit->setCellWidget(i, IC_CENTER, combo_center);
      connect(combo_center, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateIAD()));

      QComboBox* combo_neighbor = new QComboBox();
      combo_neighbor->insertItems(0, neighborList);
      if (neighborList.contains(neighbor)) {
        combo_neighbor->setCurrentIndex(combo_neighbor->findText(neighbor));
      }
      ui.table_molUnit->setCellWidget(i, IC_NEIGHBOR, combo_neighbor);
      connect(combo_neighbor, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateIAD()));

      center = qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, IC_CENTER))->currentText();
      if (center == "None")
        centerNum = 0;
      else
        centerNum = ElemInfo::getAtomicNum(center.trimmed().toStdString());
      neighbor = qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, IC_NEIGHBOR))->currentText();
      neighborNum = ElemInfo::getAtomicNum(neighbor.trimmed().toStdString());

      //Number of Centers
      unsigned int numCenters = qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, IC_NUMCENTERS))->currentText().toUInt();
      QList<QString> numCentersList;

      this->getNumCenters(centerNum, neighborNum, numCentersList);

      QComboBox* combo_numCenters = new QComboBox();
      combo_numCenters->insertItems(0, numCentersList);
      if (numCentersList.contains(QString::number(numCenters))) {
        combo_numCenters->setCurrentIndex(combo_numCenters->findText(QString::number(numCenters)));
      }
      ui.table_molUnit->setCellWidget(i, IC_NUMCENTERS, combo_numCenters);
      connect(combo_numCenters, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateIAD()));

      if (numCentersList.isEmpty())
        return;

      numCenters = qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, IC_NUMCENTERS))->currentText().toUInt();

      compMolUnit[qMakePair<int, int>(centerNum, neighborNum)].numCenters = numCenters;

      //Number of Neighbors
      QList<QString> numNeighborsList;
      unsigned int numNeighbors = qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, IC_NUMNEIGHBORS))->currentText().toUInt();
      this->getNumNeighbors(centerNum, neighborNum, numNeighborsList);

      QComboBox* combo_numNeighbors = new QComboBox();
      combo_numNeighbors->insertItems(0, numNeighborsList);
      if (numNeighborsList.contains(QString::number(numNeighbors))) {
        combo_numNeighbors->setCurrentIndex(combo_numNeighbors->findText(QString::number(numNeighbors)));
      }
      ui.table_molUnit->setCellWidget(i, IC_NUMNEIGHBORS, combo_numNeighbors);
      connect(combo_numNeighbors, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateIAD()));

      if (numNeighborsList.isEmpty())
        return;

      numNeighbors = qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, IC_NUMNEIGHBORS))->currentText().toUInt();

      compMolUnit[qMakePair<int, int>(centerNum, neighborNum)].numNeighbors = numNeighbors;

      //Geometry
      unsigned int geom = qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, IC_GEOM))->currentText().toUInt();
      QString strGeom = qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, IC_GEOM))->currentText();
      QList<QString> geomList;

      this->getGeom(geomList, numNeighbors);

      QComboBox* combo_geom = new QComboBox();
      combo_geom->insertItems(0, geomList);
      if (geomList.contains(strGeom)) {
        combo_geom->setCurrentIndex(combo_geom->findText(strGeom));
      }
      ui.table_molUnit->setCellWidget(i, IC_GEOM, combo_geom);
      connect(combo_geom, SIGNAL(currentIndexChanged(int)),
          this, SLOT(updateIAD()));

      geom = qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, IC_GEOM))->currentText().toUInt();
      strGeom = qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, IC_GEOM))->currentText();
      this->setGeom(geom, strGeom);

      compMolUnit[qMakePair<int, int>(centerNum, neighborNum)].geom = geom;

      //MolUnit distance
      double dist = ui.table_molUnit->item(i, IC_DIST)->text().toDouble();
      QString strDist = QString::number(dist, 'f', 3);
      QTableWidgetItem *distItem = new QTableWidgetItem(strDist);
      ui.table_molUnit->setItem(i, IC_DIST, distItem);
      dist = ui.table_molUnit->item(i, IC_DIST)->text().toDouble();

      compMolUnit[qMakePair<int, int>(centerNum, neighborNum)].dist = dist;

      //Update the global hash
      xtalopt->compMolUnit = compMolUnit;
    }


    //Go through table again - backward
    for (int i = numRowsMolUnit-1; i >= 0; i--) {
      QString center = qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, IC_CENTER))->currentText();
      int centerNum;
      if (center == "None")
        centerNum = 0;
      else
        centerNum = ElemInfo::getAtomicNum(center.trimmed().toStdString());
      QString neighbor = qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, IC_NEIGHBOR))->currentText();
      int neighborNum =
          ElemInfo::getAtomicNum(neighbor.trimmed().toStdString());

      //Update center and neighbor lists
      QList<QString> centerList;
      QList<QString> neighborList;

      this->getCentersAndNeighbors(centerList, centerNum, neighborList, neighborNum);

      if (centerList.isEmpty() || neighborList.isEmpty())
        return;

      for(int k = 0; k < neighborList.size(); k++) {
        int n =
          ElemInfo::getAtomicNum(neighborList.at(k).trimmed().toStdString());
        if(xtalopt->compMolUnit.contains(qMakePair<int,int>(centerNum, n)) && n != neighborNum) {
          neighborList.removeAt(k);
          k--;
        }
      }

      QComboBox* combo_center = new QComboBox();
      combo_center->insertItems(0, centerList);
      if (centerList.contains(center)) {
        combo_center->setCurrentIndex(combo_center->findText(center));
      }
      ui.table_molUnit->setCellWidget(i, IC_CENTER, combo_center);
      connect(combo_center, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateIAD()));

      QComboBox* combo_neighbor = new QComboBox();
      combo_neighbor->insertItems(0, neighborList);
      if (neighborList.contains(neighbor)) {
        combo_neighbor->setCurrentIndex(combo_neighbor->findText(neighbor));
      }
      ui.table_molUnit->setCellWidget(i, IC_NEIGHBOR, combo_neighbor);
      connect(combo_neighbor, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateIAD()));

      center = qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, IC_CENTER))->currentText();
      if (center == "None")
        centerNum = 0;
      else
        centerNum = ElemInfo::getAtomicNum(center.trimmed().toStdString());
      neighbor = qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, IC_NEIGHBOR))->currentText();
      neighborNum = ElemInfo::getAtomicNum(neighbor.trimmed().toStdString());

      //Number of Centers
      unsigned int numCenters = qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, IC_NUMCENTERS))->currentText().toUInt();
      QList<QString> numCentersList;

      this->getNumCenters(centerNum, neighborNum, numCentersList);

      QComboBox* combo_numCenters = new QComboBox();
      combo_numCenters->insertItems(0, numCentersList);
      if (numCentersList.contains(QString::number(numCenters))) {
        combo_numCenters->setCurrentIndex(combo_numCenters->findText(QString::number(numCenters)));
      }
      ui.table_molUnit->setCellWidget(i, IC_NUMCENTERS, combo_numCenters);
      connect(combo_numCenters, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateIAD()));

      numCenters = qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, IC_NUMCENTERS))->currentText().toUInt();

      compMolUnit[qMakePair<int, int>(centerNum, neighborNum)].numCenters = numCenters;

      //Number of Neighbors
      QList<QString> numNeighborsList;
      unsigned int numNeighbors = qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, IC_NUMNEIGHBORS))->currentText().toUInt();
      this->getNumNeighbors(centerNum, neighborNum, numNeighborsList);

      QComboBox* combo_numNeighbors = new QComboBox();
      combo_numNeighbors->insertItems(0, numNeighborsList);
      if (numNeighborsList.contains(QString::number(numNeighbors))) {
        combo_numNeighbors->setCurrentIndex(combo_numNeighbors->findText(QString::number(numNeighbors)));
      }
      ui.table_molUnit->setCellWidget(i, IC_NUMNEIGHBORS, combo_numNeighbors);
      connect(combo_numNeighbors, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateIAD()));

      numNeighbors = qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, IC_NUMNEIGHBORS))->currentText().toUInt();

      compMolUnit[qMakePair<int, int>(centerNum, neighborNum)].numNeighbors = numNeighbors;

      //Geometry
      unsigned int geom = qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, IC_GEOM))->currentText().toUInt();
      QString strGeom = qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, IC_GEOM))->currentText();
      QList<QString> geomList;

      this->getGeom(geomList, numNeighbors);

      QComboBox* combo_geom = new QComboBox();
      combo_geom->insertItems(0, geomList);
      if (geomList.contains(strGeom)) {
        combo_geom->setCurrentIndex(combo_geom->findText(strGeom));
      }
      ui.table_molUnit->setCellWidget(i, IC_GEOM, combo_geom);
      connect(combo_geom, SIGNAL(currentIndexChanged(int)),
          this, SLOT(updateIAD()));

      geom = qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, IC_GEOM))->currentText().toUInt();
      strGeom = qobject_cast<QComboBox*>(ui.table_molUnit->cellWidget(i, IC_GEOM))->currentText();
      this->setGeom(geom, strGeom);

      compMolUnit[qMakePair<int, int>(centerNum, neighborNum)].geom = geom;

      //MolUnit distance
      double dist = ui.table_molUnit->item(i, IC_DIST)->text().toDouble();
      QString strDist = QString::number(dist, 'f', 3);
      QTableWidgetItem *distItem = new QTableWidgetItem(strDist);
      ui.table_molUnit->setItem(i, IC_DIST, distItem);
      dist = ui.table_molUnit->item(i, IC_DIST)->text().toDouble();

      compMolUnit[qMakePair<int, int>(centerNum, neighborNum)].dist = dist;

      connect(ui.table_molUnit, SIGNAL(itemSelectionChanged()),
          this, SLOT(updateIAD()));

      //Update the global hash
      xtalopt->compMolUnit = compMolUnit;
    }

  }

  //Actions for buttons to add/remove rows from the IAD table
  void TabInit::addRow()
  {
    disconnect(ui.table_molUnit, 0, 0, 0);

    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    QHash<QPair<int, int>, MolUnit> compMolUnit;
    compMolUnit.clear();

    QList<unsigned int> keys = xtalopt->comp.keys();
    qSort(keys);
    int numKeys = keys.size();

    QList<QPair<int, int> > keysMolUnit = xtalopt->compMolUnit.keys();
    qSort(keysMolUnit);
    int numKeysMolUnit = keysMolUnit.size();

    QList<unsigned int> firstKeys;
    firstKeys.clear();
    QList<unsigned int> secondKeys;
    secondKeys.clear();
    for (int i = 0; i < numKeysMolUnit; i++) {
      firstKeys.append((keysMolUnit.at(i).first));
      secondKeys.append((keysMolUnit.at(i).second));
    }

    if (numKeys==0)
      return;

    //Center and Neighbor Lists
    int centerNum = 0;
    QList<QString> centerList;
    centerList.clear();
    int neighborNum = 0;
    QList<QString> neighborList;
    neighborList.clear();

    this->getCentersAndNeighbors(centerList, centerNum, neighborList, neighborNum);

    if (centerList.isEmpty() || neighborList.isEmpty())
      return;

    QString center = centerList.at(0);
    if (center == "None")
      centerNum = 0;
    else
      centerNum = ElemInfo::getAtomicNum(center.trimmed().toStdString());
    QString neighbor = neighborList.at(0);
    neighborNum = ElemInfo::getAtomicNum(neighbor.trimmed().toStdString());

    if(xtalopt->compMolUnit.contains(qMakePair<int, int>(centerNum, neighborNum))) {
      neighborList.removeAt(0);
      if (centerList.isEmpty() || neighborList.isEmpty())
        return;
      neighbor = neighborList.at(0);
      neighborNum = ElemInfo::getAtomicNum(neighbor.trimmed().toStdString());
    }

    //Number of Centers
    int numCenters = 0;
    QList<QString> numCentersList;
    numCentersList.clear();

    this->getNumCenters(centerNum, neighborNum, numCentersList);

    if (numCentersList.isEmpty())
      return;

    numCenters = numCentersList.at(0).toInt();

    compMolUnit[qMakePair<int, int>(centerNum, neighborNum)].numCenters = numCenters;

    //Number of Neighbors
    int numNeighbors = 0;
    QList<QString> numNeighborsList;
    numNeighborsList.clear();

    this->getNumNeighbors(centerNum, neighborNum, numNeighborsList);

    if (numNeighborsList.isEmpty())
      return;

    numNeighbors = numNeighborsList.at(0).toInt();

    compMolUnit[qMakePair<int, int>(centerNum, neighborNum)].numNeighbors = numNeighbors;

    //Determine possible geometries for the number of neigbors
    QList<QString> geomList;
    geomList.clear();

    this->getGeom(geomList, numNeighbors);

    unsigned int geom = geomList.at(0).toUInt();
    compMolUnit[qMakePair<int, int>(centerNum, neighborNum)].geom = geom;

    //Distance
    double distNum = 1.0;
    if (centerNum != 0)
        distNum = ElemInfo::getCovalentRadius(centerNum) + ElemInfo::getCovalentRadius(neighborNum);
    QString dist = QString::number(distNum, 'f', 3);

    compMolUnit[qMakePair<int, int>(centerNum, neighborNum)].dist = distNum;

    //Build table
    int row = ui.table_molUnit->rowCount();
    ui.table_molUnit->insertRow(row);

    QComboBox* combo_center = new QComboBox();
    combo_center->insertItems(0, centerList);
    ui.table_molUnit->setCellWidget(row, IC_CENTER, combo_center);
    connect(combo_center, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateIAD()));

    QComboBox* combo_numCenters = new QComboBox();
    combo_numCenters->insertItems(0, numCentersList);
    ui.table_molUnit->setCellWidget(row, IC_NUMCENTERS, combo_numCenters);
    connect(combo_numCenters, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateIAD()));

    QComboBox* combo_neighbor = new QComboBox();
    combo_neighbor->insertItems(0, neighborList);
    ui.table_molUnit->setCellWidget(row, IC_NEIGHBOR, combo_neighbor);
    connect(combo_neighbor, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateIAD()));

    QComboBox* combo_numNeighbors = new QComboBox();
    combo_numNeighbors->insertItems(0, numNeighborsList);
    ui.table_molUnit->setCellWidget(row, IC_NUMNEIGHBORS, combo_numNeighbors);
    connect(combo_numNeighbors, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateIAD()));

    QComboBox* combo_geom = new QComboBox();
    combo_geom->insertItems(0, geomList);
    ui.table_molUnit->setCellWidget(row, IC_GEOM, combo_geom);
    connect(combo_geom, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateIAD()));

    QTableWidgetItem *distItem = new QTableWidgetItem(dist);
    ui.table_molUnit->setItem(row, IC_DIST, distItem);
    connect(ui.table_molUnit, SIGNAL(itemChanged(QTableWidgetItem *)),
          this, SLOT(updateIAD()));

    connect(ui.table_molUnit, SIGNAL(itemSelectionChanged()),
          this, SLOT(updateIAD()));

    //Update the global hash
    xtalopt->compMolUnit = compMolUnit;

    this->updateIAD();
  }

  void TabInit::removeRow()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    disconnect(ui.table_molUnit, 0, 0, 0);
    ui.table_molUnit->removeRow(ui.table_molUnit->currentRow());

    connect(ui.table_molUnit, SIGNAL(itemSelectionChanged()),
        this, SLOT(updateIAD()));

    this->updateIAD();
  }

  void TabInit::removeAll()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);
    xtalopt->compMolUnit.clear();

    int row = ui.table_molUnit->rowCount();

    disconnect(ui.table_molUnit, 0, 0, 0);
    for (int i = row-1; i >= 0; i--) {
      ui.table_molUnit->removeRow(i);
    }
    connect(ui.table_molUnit, SIGNAL(itemSelectionChanged()),
        this, SLOT(updateIAD()));

    this->updateIAD();
  }

  void TabInit::getCentersAndNeighbors(QList<QString> & centerList, int centerNum,
          QList<QString> & neighborList, int neighborNum)
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    QList<unsigned int> keys = xtalopt->comp.keys();
    qSort(keys);
    int numKeys = keys.size();

    QList<QPair<int, int> > keysMolUnit = xtalopt->compMolUnit.keys();
    qSort(keysMolUnit);
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
        symbol = QString(ElemInfo::getAtomicSymbol((atomicNum)).c_str());
      }
      unsigned int qComp = xtalopt->comp[atomicNum].quantity * xtalopt->minFU();

      //Add center atom to list
      unsigned int qMolUnit = 0;
      for (QHash<QPair<int, int>, MolUnit>::const_iterator it = xtalopt->compMolUnit.constBegin(), it_end = xtalopt->compMolUnit.constEnd(); it != it_end; it++) {
        unsigned int first = const_cast<QPair<int, int> &>(it.key()).first;
        unsigned int second = const_cast<QPair<int, int> &>(it.key()).second;
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
    else centerList.append("None");

  }


  void TabInit::getNumCenters(int centerNum, int neighborNum, QList<QString> & numCentersList)
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    int q = 0;

    for (QHash<QPair<int, int>, MolUnit>::const_iterator it = xtalopt->compMolUnit.constBegin(), it_end = xtalopt->compMolUnit.constEnd(); it != it_end; it++) {
      if (it.key() != QPair<int, int>(centerNum, neighborNum) && it.key().first == centerNum)
        q += it->numCenters;
      if (it.key() != QPair<int, int>(centerNum, neighborNum) && it.key().second == centerNum)
        q += it->numNeighbors;
    }

    int numCenters = xtalopt->comp[centerNum].quantity * xtalopt->minFU() - q;

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

  void TabInit::getNumNeighbors(int centerNum, int neighborNum, QList<QString> & numNeighborsList)
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    int numCenters = 0;
    int q = 0;
    bool divide = false;

    for (QHash<QPair<int, int>, MolUnit>::const_iterator it = xtalopt->compMolUnit.constBegin(), it_end = xtalopt->compMolUnit.constEnd(); it != it_end; it++) {
      if (it.key() != QPair<int, int>(centerNum, neighborNum) && it.key().first == neighborNum)
        q += it->numCenters;
      if (it.key() != QPair<int, int>(centerNum, neighborNum) && it.key().second == neighborNum)
        q += it->numNeighbors * it->numCenters;
      if (it.key() == QPair<int, int>(centerNum, neighborNum)) {
        numCenters = it->numCenters;
        divide = true;
      if (centerNum == neighborNum)
          q += it->numCenters;
      }
    }

    int numNeighbors = xtalopt->comp[neighborNum].quantity *
                       xtalopt->minFU() - q;

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

  void TabInit::getGeom(QList<QString> & geomList, unsigned int numNeighbors)
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

  void TabInit::setGeom(unsigned int & geom, QString strGeom)
  {
    //Two neighbors
    if (strGeom.contains("Linear")) {
      geom = 1;
    } else if (strGeom.contains("Bent")) {
      geom = 2;
    //Three neighbors
    } else if (strGeom.contains("Trigonal Planar")) {
      geom = 2;
    } else if (strGeom.contains("Trigonal Pyramidal")) {
      geom = 3;
    } else if (strGeom.contains("T-Shaped")) {
      geom = 4;
    //Four neighbors
    } else if (strGeom.contains("Tetrahedral")) {
      geom = 3;
    } else if (strGeom.contains("See-Saw")) {
      geom = 5;
    } else if (strGeom.contains("Square Planar")) {
      geom = 4;
    //Five neighbors
    } else if (strGeom.contains("Trigonal Bipyramidal")) {
      geom = 5;
    } else if (strGeom.contains("Square Pyramidal")) {
      geom = 6;
    //Six neighbors
    } else if (strGeom.contains("Octahedral")) {
      geom = 6;
    //Default
    } else {
      geom = 0;
    }
  }

  void TabInit::openSpgOptions()
  {
    XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_opt);
    // If m_spgOptions already exists, delete it if the current comp does
    // not equal the old comp
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
      msgBox.setText("Calculating possible spacegroups for the given formula units. Please wait...");
      msgBox.setStandardButtons(QMessageBox::NoButton);
      msgBox.setWindowModality(Qt::NonModal);
      msgBox.open();
      QCoreApplication::processEvents();

      // Open up the RandSpg dialog
      m_spgOptions = new RandSpgDialog(xtalopt);

      // Close the mesage box
      msgBox.close();
    }

    // Display m_spgOptions
    m_spgOptions->exec();
  }

}

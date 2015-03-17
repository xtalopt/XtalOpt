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

#include <xtalopt/xtalopt.h>

#include <QtCore/QSettings>

#include <QtGui/QHeaderView>
#include <QtGui/QTableWidget>
#include <QtGui/QTableWidgetItem>

#include "dialog.h"

namespace XtalOpt {

  TabInit::TabInit( XtalOptDialog *parent, XtalOpt *p ) :
    AbstractTab(parent, p)
  {
    ui.setupUi(m_tab_widget);
    
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    xtalopt->loaded =   false;

    // composition connections
    connect(ui.edit_composition, SIGNAL(textChanged(QString)),
            this, SLOT(getComposition(QString)));
    connect(ui.edit_composition, SIGNAL(editingFinished()),
            this, SLOT(updateComposition()));

    // unit cell dimension connections
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
    connect(ui.spin_scaleFactor, SIGNAL(valueChanged(double)),
            this, SLOT(updateDimensions()));
    connect(ui.spin_minRadius, SIGNAL(valueChanged(double)),
            this, SLOT(updateDimensions()));
    connect(ui.cb_interatomicDistanceLimit, SIGNAL(toggled(bool)),
            this, SLOT(updateDimensions()));

    // formula unit connections

    connect(ui.edit_formula_units, SIGNAL(editingFinished()),
            this, SLOT(updateFormulaUnits()));

    QHeaderView *horizontal = ui.table_comp->horizontalHeader();
    horizontal->setResizeMode(QHeaderView::ResizeToContents);

    initialize();
  }

  TabInit::~TabInit()
  {
  }

  void TabInit::writeSettings(const QString &filename)
  {
    SETTINGS(filename);

    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    settings->beginGroup("xtalopt/init/");

    const int VERSION = 2;
    settings->setValue("version",VERSION);

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

    // Composition
    // We only want to save POTCAR info and Composition to the resume
    // file, not the main config file, so only dump the data here if
    // we are given a filename and it contains the string
    // "xtalopt.state"
    if (!filename.isEmpty() && filename.contains("xtalopt.state")) {
      settings->beginWriteArray("composition");
      QList<uint> keys = xtalopt->comp.keys();
      for (int i = 0; i < keys.size(); i++) {
        settings->setArrayIndex(i);
        settings->setValue("atomicNumber", keys.at(i));
        settings->setValue("quantity",
                           xtalopt->comp.value(keys.at(i)).quantity);
        settings->setValue("minRadius",
                           xtalopt->comp.value(keys.at(i)).minRadius);
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
    ui.cb_mitosis->setChecked(      settings->value("using/subcellPrint",       false).toBool()     ); 
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
        quantity = settings->value("quantity").toUInt();
        entry.quantity = quantity;
        xtalopt->comp.insert(atomicNum, entry);
      }
      this->updateMinRadii();
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
    updateComposition();
  }

  void TabInit::lockGUI()
  {
    ui.edit_composition->setDisabled(true);
    ui.cb_mitosis->setDisabled(true);
    ui.combo_divisions->setDisabled(true);
    ui.combo_a->setDisabled(true);
    ui.combo_b->setDisabled(true);
    ui.combo_c->setDisabled(true);
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
      atomicNum = OpenBabel::etab.GetAtomicNum(
            symbol.trimmed().toStdString().c_str());
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

      // Add to hash
      if (!comp.keys().contains(atomicNum)) {
        XtalCompositionStruct entry;
        entry.quantity = 0;
        entry.minRadius = 0.0;
        comp[atomicNum] = entry; // initialize if needed
      }

      comp[atomicNum].quantity += quantity;
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
    qSort(keys);

    // Adjust table size:
    int numRows = keys.size();
    ui.table_comp->setRowCount(numRows);

    for (int i = 0; i < numRows; i++) {
      unsigned int atomicNum = keys.at(i);

      QString symbol	= QString(OpenBabel::etab.GetSymbol(atomicNum));
      unsigned int quantity  = xtalopt->comp[atomicNum].quantity;
      double mass	= OpenBabel::etab.GetMass(atomicNum);
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
      uint q = xtalopt->comp.value(keys.at(i)).quantity;
      str << OpenBabel::etab.GetSymbol(keys.at(i)) << q << " ";
    }
    if (xtalopt->testingMode) {
      str << "nRunsStart" << xtalopt->test_nRunsStart << " "
          << "nRunsEnd" << xtalopt->test_nRunsEnd << " "
          << "nStructs" << xtalopt->test_nStructs << " "
          << "testingMode ";
    }

    ui.edit_composition->setText(tmp.trimmed());
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
    xtalopt->using_mitosis = ui.cb_mitosis->isChecked();
    xtalopt->divisions = ui.combo_divisions->currentText().toInt();
    xtalopt->using_subcellPrint = ui.cb_subcellPrint->isChecked();
    

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
      it.value().minRadius = xtalopt->scaleFactor *
          OpenBabel::etab.GetCovalentRad(it.key());
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
    QStringList tmp2;
    QTextStream string (&tmp);
    QList<bool> series;
    QStringList tempFormulaUnitsList;

    // Split up values separated by commas
    tempFormulaUnitsList = ui.edit_formula_units->text().split(",", QString::SkipEmptyParts);

    // Fix to correct crashing when there is a hyphen at the beginning
    if (!tempFormulaUnitsList.isEmpty()) {
      tempFormulaUnitsList[0].prepend(" ");
    }

    // Check for values that begin, are between, or end hyphens
    int i = 0, j = 0;
    bool isNumeric;
    for (int i = 0; i < tempFormulaUnitsList.size(); i++) {
      tmp2 = tempFormulaUnitsList.at(i).split("-", QString::SkipEmptyParts);
      if (tmp2.at(0) != tempFormulaUnitsList.at(i)) {
        tmp2.at(0).toUInt(&isNumeric);
        if (isNumeric == true) {
          tmp2.at(1).toUInt(&isNumeric);
          if (isNumeric == true) {
            uint smaller = tmp2.at(0).toUInt();
            uint larger = tmp2.at(1).toUInt();
            if (larger < smaller) {
              smaller = tmp2.at(1).toUInt();
              larger = tmp2.at(0).toUInt();
            }
            for (j = smaller; j <= larger; j++) {
              tempFormulaUnitsList.append(QString::number(j));
            }
          }
        }
      }
    }

    // Remove leading zeros
    for (int i = 0; i < tempFormulaUnitsList.size(); i++) {
      while (tempFormulaUnitsList.at(i).trimmed().startsWith("0")) {
        tempFormulaUnitsList[i].remove(0,1);
      }
    }

    // Check that each QString may be converted to an unsigned int. Discard it if it cannot.
    for (int i = 0; i < tempFormulaUnitsList.size(); i++) {
      tempFormulaUnitsList.at(i).toUInt(&isNumeric);
      if (isNumeric == false) {
        tempFormulaUnitsList.removeAt(i);
        i--;
      }
    }

    // Remove all numbers greater than 100
    for (int i = 0; i < tempFormulaUnitsList.size(); i++) {
      if (tempFormulaUnitsList.at(i).toUInt() > 100) {
        tempFormulaUnitsList.removeAt(i);
      }
    }

    // If nothing valid was entered, return 1
    if ( tempFormulaUnitsList.size() == 0 ) {
      xtalopt->formulaUnitsList.append(1);
      string << "1";
      ui.edit_formula_units->setText(tmp.trimmed());
      return;
    }

    // Remove duplicates from the tempFormulaUnitsList
    for (int i = 0; i < tempFormulaUnitsList.size() - 1; i++) {
      for (int j = i + 1; j < tempFormulaUnitsList.size(); j++) {
        if (tempFormulaUnitsList.at(i) == tempFormulaUnitsList.at(j)) {
          tempFormulaUnitsList.removeAt(j);
          j--;
        }
      }
    }

    // Sort from smallest value to greatest value
    for (int i = 0; i < tempFormulaUnitsList.size() - 1; i++) {
      for (int j = i + 1; j < tempFormulaUnitsList.size(); j++) {
        if (tempFormulaUnitsList.at(i).toUInt() > tempFormulaUnitsList.at(j).toUInt()) {
          tempFormulaUnitsList.swap(i,j);
        }
      }
    }

    // Populate series with false
    series.clear();
    for (int i = 0; i < tempFormulaUnitsList.size(); i++) {
      series.append(false);
    }

    // Check for series to hyphenate
    for (int i = 0; i < tempFormulaUnitsList.size() - 2; i++) {
      if ( (tempFormulaUnitsList.at(i).toUInt() + 1 == tempFormulaUnitsList.at(i + 1).toUInt()) && (tempFormulaUnitsList.at(i + 1).toUInt() + 1 == tempFormulaUnitsList.at(i + 2).toUInt()) ) {
        series.replace(i, true);
        series.replace(i + 1, true);
        series.replace(i + 2, true);
      }
    }

    // Create the text stream to put back into the UI
    for (int i = 0; i < tempFormulaUnitsList.size(); i++) {
      if (series.at(i) == false) {
        if (i + 1 == tempFormulaUnitsList.size()) {
          string << tempFormulaUnitsList.at(i).trimmed();
        }
        else if (i + 1 != tempFormulaUnitsList.size()) {
          string << tempFormulaUnitsList.at(i).trimmed() << ", ";
        }
      }
      else if (series.at(i) == true) {
        uint seriesLength = 1;
        int j = i + 1;
        while ( (j != tempFormulaUnitsList.size()) && (series.at(j) == true) && ( tempFormulaUnitsList.at(j - 1).toUInt() + 1 == tempFormulaUnitsList.at(j).toUInt() ) ) {
          seriesLength += 1;
          j++;
        }
        if (i + seriesLength == tempFormulaUnitsList.size()) {
          string << tempFormulaUnitsList.at(i).trimmed() << " - " << tempFormulaUnitsList.at(j - 1).trimmed();
        }
        else if (i + seriesLength != tempFormulaUnitsList.size()) {
          string << tempFormulaUnitsList.at(i).trimmed() << " - " << tempFormulaUnitsList.at(j - 1).trimmed() << ", ";
        }
        i = i + seriesLength - 1;
      }
    }

    //Create the UInt formulaUnitsList
    QList<uint> formulaUnitsList;
    formulaUnitsList.clear();
    for (int i = 0; i < tempFormulaUnitsList.size(); i++) {
      formulaUnitsList.append(tempFormulaUnitsList.at(i).toUInt());
    }

    xtalopt->minFU = formulaUnitsList.at(0);
    xtalopt->maxFU = formulaUnitsList.at(formulaUnitsList.size() - 1);
    xtalopt->formulaUnitsList = formulaUnitsList;

    // Update UI
    ui.edit_formula_units->setText(tmp.trimmed());

  }

  // Determine the possible number of divisions for mitosis and update combobox with options
  void TabInit::updateNumDivisions()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    int counter = 0;
    QList<QString> divisions;
    QList<uint> atomicNums = xtalopt->comp.keys();
    
    if (ui.cb_mitosis->isChecked()){
        divisions.clear();
        ui.combo_divisions->clear();
        if (xtalopt->loaded==true) {
            ui.combo_divisions->insertItem(0, QString::number(xtalopt->divisions));
        } else {
        for (int j = 1000; j >= 1; --j) {
            for (int i = 0; i <= atomicNums.size()-1; ++i) {
                if (xtalopt->comp.value(atomicNums[i]).quantity % j > 0) {
                    if (xtalopt->comp.value(atomicNums[i]).quantity == 1) {
                        counter = 0;
                        break;
                    } else if (xtalopt->comp.value(atomicNums[i]).quantity / j > 0 && xtalopt->comp.value(atomicNums[i]).quantity % j <= 5) {
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

    if (xtalopt->using_mitosis && xtalopt->divisions!=0){
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

}

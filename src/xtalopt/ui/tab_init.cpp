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
#include <QtCore/QDebug>

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
    connect(ui.spin_alpha_min, SIGNAL(valueChanged(double)),
            this, SLOT(updateDimensions()));
    connect(ui.spin_beta_min, SIGNAL(valueChanged(double)),
            this, SLOT(updateDimensions()));
    connect(ui.spin_gamma_min, SIGNAL(valueChanged(double)),
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
    connect(ui.spin_gamma_max, SIGNAL(valueChanged(double)),
            this, SLOT(updateDimensions()));
    connect(ui.spin_vol_max, SIGNAL(editingFinished()),
            this, SLOT(updateDimensions()));
    connect(ui.spin_fixedVolume, SIGNAL(valueChanged(double)),
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
    connect(ui.cb_useIAD, SIGNAL(toggled(bool)),
            this, SLOT(updateDimensions()));
    connect(ui.table_iad, SIGNAL(itemSelectionChanged()),
            this, SLOT(updateIAD()));
    connect(ui.pushButton_addIAD, SIGNAL(clicked(bool)),
            this, SLOT(addRow()));
    connect(ui.pushButton_removeIAD, SIGNAL(clicked(bool)),
            this, SLOT(removeRow()));
    connect(ui.pushButton_removeAllIAD, SIGNAL(clicked(bool)),
            this, SLOT(removeAll()));

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

    // Update min and max volume based upon min and max lengths (a,b,c)
    if ((ui.spin_a_min->value() * ui.spin_b_min->value() * ui.spin_c_min->value()) > ui.spin_vol_min->value()) ui.spin_vol_min->setValue(ui.spin_a_min->value() * ui.spin_b_min->value() * ui.spin_c_min->value());
    if ((ui.spin_a_max->value() * ui.spin_b_max->value() * ui.spin_c_max->value()) < ui.spin_vol_min->value()) ui.spin_vol_min->setValue(ui.spin_a_max->value() * ui.spin_b_max->value() * ui.spin_c_max->value());
    if ((ui.spin_a_max->value() * ui.spin_b_max->value() * ui.spin_c_max->value()) < ui.spin_vol_max->value()) ui.spin_vol_max->setValue(ui.spin_a_max->value() * ui.spin_b_max->value() * ui.spin_c_max->value());

    // Update fixed volume based upon min and max lengths (a,b,c)
    if (ui.cb_fixedVolume->isChecked()) {
        if ((ui.spin_a_min->value() * ui.spin_b_min->value() * ui.spin_c_min->value()) > ui.spin_fixedVolume->value()) (ui.spin_fixedVolume->setValue(ui.spin_a_min->value() * ui.spin_b_min->value() * ui.spin_c_min->value()));
        if ((ui.spin_a_max->value() * ui.spin_b_max->value() * ui.spin_c_max->value()) < ui.spin_fixedVolume->value()) (ui.spin_fixedVolume->setValue(ui.spin_a_max->value() * ui.spin_b_max->value() * ui.spin_c_max->value()));
    }

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
    
    xtalopt->using_customIAD = ui.cb_useIAD->isChecked();

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
        it != it_end; ++it) 
    {
      it.value().minRadius = xtalopt->scaleFactor *
      OpenBabel::etab.GetCovalentRad(it.key());
      // Ensure that all minimum radii are > 0.25 (esp. H!)
      if (it.value().minRadius < xtalopt->minRadius) {
        it.value().minRadius = xtalopt->minRadius;
      }
    }
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

  void TabInit::updateIAD()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    QHash<QPair<int, int>, IAD> compIAD;

    unsigned int length = ui.table_iad->rowCount();
    if (length == 0) {
        xtalopt->compIAD.clear();
      //  this->updateIADTable();
        return;
    }

    for (uint i = 0; i < length; i++) {
      QString center = qobject_cast<QComboBox*>(ui.table_iad->cellWidget(i, IC_CENTER))->currentText();
      int centerNum = OpenBabel::etab.GetAtomicNum(center.trimmed().toStdString().c_str());
      QString neighbor = qobject_cast<QComboBox*>(ui.table_iad->cellWidget(i, IC_NEIGHBOR))->currentText();
      int neighborNum = OpenBabel::etab.GetAtomicNum(neighbor.trimmed().toStdString().c_str());
      
      unsigned int number = qobject_cast<QComboBox*>(ui.table_iad->cellWidget(i, IC_NUMBER))->currentText().toUInt();
      double dist = ui.table_iad->item(i, IC_DIST)->text().toDouble();
      unsigned int geom = qobject_cast<QComboBox*>(ui.table_iad->cellWidget(i, IC_GEOM))->currentText().toUInt();
      QString strGeom = qobject_cast<QComboBox*>(ui.table_iad->cellWidget(i, IC_GEOM))->currentText();
      this->setGeom(geom, strGeom);

      compIAD[qMakePair<int, int>(centerNum, neighborNum)].number = number;
      compIAD[qMakePair<int, int>(centerNum, neighborNum)].dist = dist;
      //compIAD[qMakePair<int, int>(centerNum, neighborNum)].geom = geom;
      
      xtalopt->compIAD[qMakePair<int, int>(centerNum, neighborNum)].number = number;
      xtalopt->compIAD[qMakePair<int, int>(centerNum, neighborNum)].dist = dist;
      xtalopt->compIAD[qMakePair<int, int>(centerNum, neighborNum)].geom = geom;
    }
  }

  /*void TabInit::updateIADTable()
  {

  }*/

  //Actions for buttons to add/remove rows from the IAD table
  void TabInit::addRow()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    QList<unsigned int> keys = xtalopt->comp.keys();
    qSort(keys);
    int numRows = keys.size();
    
    if (numRows==0)
      return;

    QList<QString> centerList;
    QList<QString> neighborList;
    QList<QString> numberList;
    QList<QString> geomList;

    for (int i = 0; i < numRows; i++) {
      unsigned int atomicNum = keys.at(i);

      QString symbol = QString(OpenBabel::etab.GetSymbol(atomicNum));
      if (i!=0)
        centerList.append(symbol);
      if (i!=1)
        neighborList.append(symbol);
    }
    
    QString center = centerList.at(0);
    QString neighbor = neighborList.at(0);
    unsigned int centerNum = OpenBabel::etab.GetAtomicNum(center.trimmed().toStdString().c_str());
    unsigned int neighborNum = OpenBabel::etab.GetAtomicNum(neighbor.trimmed().toStdString().c_str());
      
    unsigned int numCenters = xtalopt->comp[centerNum].quantity;
    unsigned int numNeighbors = xtalopt->comp[neighborNum].quantity;
    numNeighbors /= numCenters;
    for (int i = numNeighbors; i > 0; i--) {
      numberList.append(QString::number(i));
    }
    
    //Determine possible geometries for the number of neigbors 
    this->getGeom(geomList, numberList.at(0).toUInt());

    double distNum = OpenBabel::etab.GetCovalentRad(centerNum) + OpenBabel::etab.GetCovalentRad(neighborNum);
    QString dist = QString::number(distNum, 'f', 3);

    int row = ui.table_iad->rowCount();
    ui.table_iad->insertRow(row);
    
    QComboBox* combo_center = new QComboBox();
    combo_center->insertItems(0, centerList);
    ui.table_iad->setCellWidget(row, IC_CENTER, combo_center);
    connect(combo_center, SIGNAL(currentIndexChanged(int)), 
            this, SLOT(updateIAD()));

    QComboBox* combo_neighbor = new QComboBox();
    combo_neighbor->insertItems(0, neighborList);
    ui.table_iad->setCellWidget(row, IC_NEIGHBOR, combo_neighbor);
    connect(combo_neighbor, SIGNAL(currentIndexChanged(int)), 
            this, SLOT(updateIAD()));
   
    QComboBox* combo_number = new QComboBox();
    combo_number->insertItems(0, numberList);
    ui.table_iad->setCellWidget(row, IC_NUMBER, combo_number);
    connect(combo_number, SIGNAL(currentIndexChanged(int)), 
            this, SLOT(updateIAD()));
  
    QTableWidgetItem *distItem = new QTableWidgetItem(dist);
    ui.table_iad->setItem(row, IC_DIST, distItem);
    
    QComboBox* combo_geom = new QComboBox();
    combo_geom->insertItems(0, geomList);
    ui.table_iad->setCellWidget(row, IC_GEOM, combo_geom);
    connect(combo_geom, SIGNAL(currentIndexChanged(int)), 
            this, SLOT(updateIAD()));

 
    this->updateIAD();
  }

  void TabInit::removeRow()
  {
    disconnect(ui.table_iad, 0, 0, 0);
    ui.table_iad->removeRow(ui.table_iad->currentRow());
    connect(ui.table_iad, SIGNAL(itemSelectionChanged()),
        this, SLOT(updateIAD()));
    this->updateIAD();
  }

  void TabInit::removeAll()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);
    xtalopt->compIAD.clear();
    
    int row = ui.table_iad->rowCount();
    
    disconnect(ui.table_iad, 0, 0, 0);
    for (int i = row-1; i >= 0; i--) {
      ui.table_iad->removeRow(i);
    }
    connect(ui.table_iad, SIGNAL(itemSelectionChanged()),
        this, SLOT(updateIAD()));
    
    this->updateIAD();
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
      geom = 4;
    } else if (strGeom.contains("Square Planar")) {
      geom = 5;
    //Five neighbors
    } else if (strGeom.contains("Trigonal Bipyramidal")) {
      geom = 4;
    } else if (strGeom.contains("Square Pyramidal")) {
      geom = 5;
    //Six neighbors
    } else if (strGeom.contains("Octahedral")) {
      geom = 5;
    //Default
    } else {
      geom = 0;
    }
  }

}


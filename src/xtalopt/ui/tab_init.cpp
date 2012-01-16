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

#include <xtalopt/structures/submoleculesource.h>
#include <xtalopt/xtalopt.h>

#include <avogadro/molecule.h>
#include <avogadro/moleculefile.h>

#include <QtCore/QSettings>

#include <QtGui/QFileDialog>
#include <QtGui/QHeaderView>
#include <QtGui/QInputDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QTableWidget>
#include <QtGui/QTableWidgetItem>

#include "dialog.h"

using namespace Avogadro;

namespace XtalOpt {

  TabInit::TabInit( XtalOptDialog *parent, XtalOpt *p ) :
    AbstractTab(parent, p)
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);
    ui.setupUi(m_tab_widget);

    // xtalopt connections
    connect(xtalopt, SIGNAL(isMolecularXtalSearchChanged(bool)),
            this, SLOT(updateGUI()));

    // crystal type
    connect(ui.combo_type, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateCrystalType()));

    // molecular composition settings
    this->connect(ui.push_add, SIGNAL(clicked()), SLOT(addSubMolecule()));
    this->connect(ui.push_remove, SIGNAL(clicked()), SLOT(removeSubMolecule()));

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
    connect(ui.spin_scaleFactor, SIGNAL(valueChanged(double)),
            this, SLOT(updateDimensions()));
    connect(ui.spin_minRadius, SIGNAL(valueChanged(double)),
            this, SLOT(updateDimensions()));
    connect(ui.cb_interatomicDistanceLimit, SIGNAL(toggled(bool)),
            this, SLOT(updateDimensions()));
    connect(ui.spin_maxConf, SIGNAL(valueChanged(int)),
            this, SLOT(updateDimensions()));

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

    settings->setValue("isMolecularXtalSearch",
                       xtalopt->isMolecularXtalSearch());
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
    settings->setValue("using/interatomicDistanceLimit",
                       xtalopt->using_interatomicDistanceLimit);
    settings->setValue("limits/maxConf",    xtalopt->maxConf);

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

    if (settings->value("isMolecularXtalSearch", false).toBool()) {
      ui.combo_type->setCurrentIndex(CT_Molecular);
    }
    else {
      ui.combo_type->setCurrentIndex(CT_Ionic);
    }
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
    ui.spin_minRadius->setValue(	  settings->value("limits/minRadius",0.25).toDouble());
    ui.cb_fixedVolume->setChecked(	settings->value("using/fixedVolume",	false).toBool()	);
    ui.cb_interatomicDistanceLimit->setChecked(	settings->value("using/interatomicDistanceLimit",false).toBool());
    ui.spin_maxConf->setValue(      settings->value("limits/maxConf",20).toInt());

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

    if (xtalopt->isMolecularXtalSearch()) {
      ui.combo_type->setCurrentIndex(CT_Molecular);
      ui.gb_comp->setDisabled(true);
      ui.gb_mcomp->show();
    }
    else {
      ui.combo_type->setCurrentIndex(CT_Ionic);
      ui.gb_comp->setEnabled(true);
      ui.gb_mcomp->hide();
    }

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
    ui.cb_interatomicDistanceLimit->setChecked(
          xtalopt->using_interatomicDistanceLimit);
    ui.spin_maxConf->setValue(     xtalopt->maxConf);
    updateComposition();
    updateTables();
  }

  void TabInit::lockGUI()
  {
    ui.combo_type->setDisabled(true);
    ui.edit_composition->setDisabled(true);
    ui.push_add->setDisabled(true);
    ui.push_remove->setDisabled(true);
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

    this->updateTables();
  }

  void TabInit::updateCrystalType()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    switch(static_cast<CrystalType>(ui.combo_type->currentIndex()))
    {
    default:
    case CT_Ionic:
      xtalopt->setMolecularXtalSearch(false);
      break;
    case CT_Molecular:
      xtalopt->setMolecularXtalSearch(true);
      break;
    }

    return;
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
    xtalopt->vol_min		= ui.spin_vol_min->value();

    xtalopt->a_max		= ui.spin_a_max->value();
    xtalopt->b_max		= ui.spin_b_max->value();
    xtalopt->c_max		= ui.spin_c_max->value();
    xtalopt->alpha_max	= ui.spin_alpha_max->value();
    xtalopt->beta_max	= ui.spin_beta_max->value();
    xtalopt->gamma_max	= ui.spin_gamma_max->value();
    xtalopt->vol_max		= ui.spin_vol_max->value();

    xtalopt->using_fixed_volume = ui.cb_fixedVolume->isChecked();
    xtalopt->vol_fixed	= ui.spin_fixedVolume->value();

    xtalopt->maxConf		= ui.spin_maxConf->value();

    if (xtalopt->scaleFactor != ui.spin_scaleFactor->value() ||
        xtalopt->minRadius   != ui.spin_minRadius->value() ||
        xtalopt->using_interatomicDistanceLimit !=
        ui.cb_interatomicDistanceLimit->isChecked()) {
      xtalopt->scaleFactor = ui.spin_scaleFactor->value();
      xtalopt->minRadius = ui.spin_minRadius->value();
      xtalopt->using_interatomicDistanceLimit =
          ui.cb_interatomicDistanceLimit->isChecked();


      this->updateTables();
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

  void TabInit::updateTables()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);
    if (xtalopt->isMolecularXtalSearch()) {
      this->updateMXtalCompositionTable();
      this->updateCompFromMComp();
    }
    this->updateMinRadii();
    this->updateCompositionTable();
  }

  void TabInit::addSubMolecule()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);
    Q_ASSERT(xtalopt->isMolecularXtalSearch());
    SETTINGS("");
    bool ok;
    QString err;

    // Ask for molecule filename
    QString submolDir = settings->value("submolDir").toString();
    QString filename = QFileDialog::getOpenFileName(
          m_dialog, tr("Add Molecule..."), submolDir);

    // If user cancels:
    if (filename.isEmpty()) {
      return;
    }

    // store directory for next time
    settings->setValue("submolDir", filename);

    // Attempt to read molecule:
    Molecule *mol = MoleculeFile::readMolecule(filename, QString(), QString(),
                                               &err);

    // Bad molecule; show error and return
    if (!mol) {
      if (!err.isEmpty()) {
        err.prepend(tr("\n\nThe returned error is:\n\t"));
      }
      else {
        err.clear();
      }
      QMessageBox::warning(m_dialog, tr("Cannot load molecule!"),
                           tr("The file '%1' cannot be opened.%2")
                           .arg(filename).arg(err));
      return;
    }

    // Ask for quantity
    int quantity = QInputDialog::getInt(
          m_dialog, tr("Specify Quantity"), tr("How many of this molecule "
                                               "(%1) per unit cell?")
          .arg(mol->fileName()), 1, 1, 9999, 1, &ok);

    // User cancels:
    if (!ok) {
      return;
    }

    // Add entry to mcomp
    MolecularCompStruct entry;
    entry.source = new SubMoleculeSource (mol, m_opt);
    entry.source->setSourceId(xtalopt->mcomp.size()); // next unique id
    entry.quantity = quantity;
    xtalopt->mcomp.append(entry);

    // Update GUI
    this->updateTables();
  }

  void TabInit::removeSubMolecule()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);
    Q_ASSERT(xtalopt->isMolecularXtalSearch());
    int index = ui.table_mcomp->currentRow();

    if (index < 0 || index >= xtalopt->mcomp.size()) {
      qDebug() << "Submolecule index out of range: " << index << "out of"
               << xtalopt->mcomp.size();
      return;
    }

    xtalopt->mcomp.at(index).source->deleteLater();
    xtalopt->mcomp.removeAt(index);
    this->updateTables();
  }

  void TabInit::updateMXtalCompositionTable()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    // Adjust table size:
    int numRows = xtalopt->mcomp.size();
    ui.table_mcomp->setRowCount(numRows);

    for (int i = 0; i < numRows; i++) {
      SubMoleculeSource *source = xtalopt->mcomp.at(i).source;
      unsigned int quantity = xtalopt->mcomp.at(i).quantity;

      QTableWidgetItem *quantityItem =
          new QTableWidgetItem(QString::number(quantity));
      QTableWidgetItem *filenameItem =
          new QTableWidgetItem(source->name());

      ui.table_mcomp->setItem(i, MC_QUANTITY, quantityItem);
      ui.table_mcomp->setItem(i, MC_FILENAME, filenameItem);
    }
  }

  void TabInit::updateCompFromMComp()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);
    Q_ASSERT(xtalopt->isMolecularXtalSearch());

    int numSubMolecules = xtalopt->mcomp.size();
    xtalopt->comp.clear();

    for (int i = 0; i < numSubMolecules; i++) {
      SubMoleculeSource *source = xtalopt->mcomp.at(i).source;
      unsigned int quantity = xtalopt->mcomp.at(i).quantity;

      QList<Atom*> atoms = source->atoms();
      for (QList<Atom*>::const_iterator it = atoms.constBegin(),
           it_end = atoms.constEnd(); it != it_end; ++it) {
        if (!xtalopt->comp.contains((*it)->atomicNumber())) {
          XtalCompositionStruct newStruct;
          newStruct.quantity = 0;
          xtalopt->comp.insert((*it)->atomicNumber(), newStruct);
        }
        xtalopt->comp[(*it)->atomicNumber()].quantity += quantity;
      }
    }
  }
}

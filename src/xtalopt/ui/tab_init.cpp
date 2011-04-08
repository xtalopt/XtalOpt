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

#include "dialog.h"

namespace XtalOpt {

  TabInit::TabInit( XtalOptDialog *parent, XtalOpt *p ) :
    AbstractTab(parent, p)
  {
    ui.setupUi(m_tab_widget);

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
    connect(ui.spin_shortestInteratomicDistance, SIGNAL(editingFinished()),
            this, SLOT(updateDimensions()));
    connect(ui.cb_shortestInteratomicDistance, SIGNAL(toggled(bool)),
            this, SLOT(updateDimensions()));

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

    const int VERSION = 1;
    settings->setValue("version",          VERSION);

    settings->setValue("limits/a/min",				xtalopt->a_min);
    settings->setValue("limits/b/min",            		xtalopt->b_min);
    settings->setValue("limits/c/min",            		xtalopt->c_min);
    settings->setValue("limits/a/max",            		xtalopt->a_max);
    settings->setValue("limits/b/max",            		xtalopt->b_max);
    settings->setValue("limits/c/max",            		xtalopt->c_max);
    settings->setValue("limits/alpha/min",			xtalopt->alpha_min);
    settings->setValue("limits/beta/min",			xtalopt->beta_min);
    settings->setValue("limits/gamma/min",			xtalopt->gamma_min);
    settings->setValue("limits/alpha/max",			xtalopt->alpha_max);
    settings->setValue("limits/beta/max",			xtalopt->beta_max);
    settings->setValue("limits/gamma/max",			xtalopt->gamma_max);
    settings->setValue("limits/volume/min",			xtalopt->vol_min);
    settings->setValue("limits/volume/max",			xtalopt->vol_max);
    settings->setValue("limits/volume/fixed",			xtalopt->vol_fixed);
    settings->setValue("limits/shortestInteratomicDistance",	xtalopt->shortestInteratomicDistance);
    settings->setValue("using/fixedVolume",			xtalopt->using_fixed_volume);
    settings->setValue("using/shortestInteratomicDistance",	xtalopt->using_shortestInteratomicDistance);

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
        settings->setValue("quantity", xtalopt->comp.value(keys.at(i)));
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
    ui.spin_shortestInteratomicDistance->setValue(	settings->value("limits/shortestInteratomicDistance",1).toDouble());
    ui.cb_fixedVolume->setChecked(	settings->value("using/fixedVolume",	false).toBool()	);
    ui.cb_shortestInteratomicDistance->setChecked(	settings->value("using/shortestInteratomicDistance",false).toBool());

    // Composition
    if (!filename.isEmpty()) {
      int size = settings->beginReadArray("composition");
      xtalopt->comp = QHash<uint,uint> ();
      for (int i = 0; i < size; i++) {
        settings->setArrayIndex(i);
        uint atomicNum, quant;
        atomicNum = settings->value("atomicNumber").toUInt();
        quant = settings->value("quantity").toUInt();
        xtalopt->comp.insert(atomicNum, quant);
      }
      settings->endArray();
    }

    settings->endGroup();

    // Update config data
    switch (loadedVersion) {
    case 0:
    case 1:
    default:
      break;
    }

    // Enact changesSetup templates
    updateDimensions();
  }

  void TabInit::updateGUI()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    ui.spin_a_min->setValue(				xtalopt->a_min);
    ui.spin_b_min->setValue(				xtalopt->b_min);
    ui.spin_c_min->setValue(				xtalopt->c_min);
    ui.spin_a_max->setValue(				xtalopt->a_max);
    ui.spin_b_max->setValue(				xtalopt->b_max);
    ui.spin_c_max->setValue(				xtalopt->c_max);
    ui.spin_alpha_min->setValue(			xtalopt->alpha_min);
    ui.spin_beta_min->setValue(				xtalopt->beta_min);
    ui.spin_gamma_min->setValue(			xtalopt->gamma_min);
    ui.spin_alpha_max->setValue(			xtalopt->alpha_max);
    ui.spin_beta_max->setValue(				xtalopt->beta_max);
    ui.spin_gamma_max->setValue(			xtalopt->gamma_max);
    ui.spin_vol_min->setValue(				xtalopt->vol_min);
    ui.spin_vol_max->setValue(				xtalopt->vol_max);
    ui.spin_fixedVolume->setValue(			xtalopt->vol_fixed);
    ui.spin_shortestInteratomicDistance->setValue(	xtalopt->shortestInteratomicDistance);
    ui.cb_fixedVolume->setChecked(			xtalopt->using_fixed_volume);
    ui.cb_shortestInteratomicDistance->setChecked(	xtalopt->using_shortestInteratomicDistance);
    updateComposition();
  }

  void TabInit::lockGUI()
  {
    ui.edit_composition->setDisabled(true);
  }

  void TabInit::getComposition(const QString &str)
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    QHash<uint, uint> comp;
    QString symbol;
    uint atomicNum;
    uint quantity;
    QStringList symbolList;
    QStringList quantityList;

    // Parse numbers between letters
    symbolList		= str.split(QRegExp("[0-9]"), QString::SkipEmptyParts);
    // Parse letters between numbers
    quantityList        = str.split(QRegExp("[A-Z,a-z]"), QString::SkipEmptyParts);

    xtalopt->testingMode = (str.contains("testingMode")) ? true : false;

    // Use the shorter of the lists for the length
    uint length = (symbolList.size() < quantityList.size()) ? symbolList.size() : quantityList.size();

    if ( length == 0 ) {
      ui.list_composition->clear();
      xtalopt->comp.clear();
      return;
    }

    // Build hash
    for (uint i = 0; i < length; i++){
      symbol    = symbolList.at(i);
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
      atomicNum = OpenBabel::etab.GetAtomicNum(symbol.trimmed().toStdString().c_str());

      // Validate symbol
      if (!atomicNum) continue; // Invalid symbol entered

      // Add to hash
      if (!comp.keys().contains(atomicNum)) comp[atomicNum] = 0; // initialize if needed
      comp[atomicNum] += quantity;
    }

    // Dump hash into list
    ui.list_composition->clear();
    QList<uint> keys = comp.keys();
    qSort(keys);
    QString line ("%1=%2 x %3");
    for (int i = 0; i < keys.size(); i++) {
      atomicNum = keys.at(i);
      quantity  = comp[atomicNum];
      symbol	= OpenBabel::etab.GetSymbol(atomicNum);
      new QListWidgetItem(line.arg(atomicNum,3).arg(symbol,2).arg(quantity), ui.list_composition);
    }

    // Save hash
    xtalopt->comp = comp;
  }

  void TabInit::updateComposition()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    QList<uint> keys = xtalopt->comp.keys();
    qSort(keys);
    QString tmp;
    QTextStream str (&tmp);
    for (int i = 0; i < keys.size(); i++) {
      uint q = xtalopt->comp.value(keys.at(i));
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

    xtalopt->using_shortestInteratomicDistance       = ui.cb_shortestInteratomicDistance->isChecked();
    xtalopt->shortestInteratomicDistance		= ui.spin_shortestInteratomicDistance->value();
  }
}

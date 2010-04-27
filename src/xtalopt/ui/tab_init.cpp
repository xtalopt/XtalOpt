/**********************************************************************
  XtalOpt - Tools for advanced crystal optimization

  Copyright (C) 2009-2010 by David Lonie

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include "tab_init.h"

#include "../xtalopt.h"

#include <QSettings>

#include "dialog.h"

using namespace std;

namespace Avogadro {

  TabInit::TabInit( XtalOptDialog *parent, XtalOpt *p ) :
    QObject( parent ), m_dialog(parent), m_opt(p)
  {
    //qDebug() << "TabInit::TabInit( " << parent <<  " ) called.";

    m_tab_widget = new QWidget;
    ui.setupUi(m_tab_widget);

    m_dialog = parent;

    // dialog connections
    connect(m_dialog, SIGNAL(tabsReadSettings(const QString &)),
            this, SLOT(readSettings(const QString &)));
    connect(m_dialog, SIGNAL(tabsWriteSettings(const QString &)),
            this, SLOT(writeSettings(const QString &)));
    connect(m_dialog, SIGNAL(tabsUpdateGUI()),
            this, SLOT(updateGUI()));
    connect(m_dialog, SIGNAL(tabsDisconnectGUI()),
            this, SLOT(disconnectGUI()));
    connect(m_dialog, SIGNAL(tabsLockGUI()),
            this, SLOT(lockGUI()));

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
  }

  TabInit::~TabInit()
  {
    //qDebug() << "TabInit::~TabInit() called";
  }

  void TabInit::writeSettings(const QString &filename) {
    SETTINGS(filename);

    settings->beginGroup("xtalopt/init/");
    settings->setValue("limits/a/min",				m_opt->a_min);
    settings->setValue("limits/b/min",            		m_opt->b_min);
    settings->setValue("limits/c/min",            		m_opt->c_min);
    settings->setValue("limits/a/max",            		m_opt->a_max);
    settings->setValue("limits/b/max",            		m_opt->b_max);
    settings->setValue("limits/c/max",            		m_opt->c_max);
    settings->setValue("limits/alpha/min",			m_opt->alpha_min);
    settings->setValue("limits/beta/min",			m_opt->beta_min);
    settings->setValue("limits/gamma/min",			m_opt->gamma_min);
    settings->setValue("limits/alpha/max",			m_opt->alpha_max);
    settings->setValue("limits/beta/max",			m_opt->beta_max);
    settings->setValue("limits/gamma/max",			m_opt->gamma_max);
    settings->setValue("limits/volume/min",			m_opt->vol_min);
    settings->setValue("limits/volume/max",			m_opt->vol_max);
    settings->setValue("limits/volume/fixed",			m_opt->vol_fixed);
    settings->setValue("limits/shortestInteratomicDistance",	m_opt->shortestInteratomicDistance);
    settings->setValue("using/fixedVolume",			m_opt->using_fixed_volume);
    settings->setValue("using/shortestInteratomicDistance",	m_opt->using_shortestInteratomicDistance);

    // Composition
    settings->beginWriteArray("composition");
    QList<uint> keys = m_opt->comp.keys();
    for (int i = 0; i < keys.size(); i++) {
      settings->setArrayIndex(i);
      settings->setValue("atomicNumber", keys.at(i));
      settings->setValue("quantity", m_opt->comp.value(keys.at(i)));
    }
    settings->endArray();

    settings->endGroup();
  }

  void TabInit::readSettings(const QString &filename) {
    SETTINGS(filename);

    settings->beginGroup("xtalopt/init/");
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
      m_opt->comp = QHash<uint,uint> ();
      for (int i = 0; i < size; i++) {
        settings->setArrayIndex(i);
        uint atomicNum, quant;
        atomicNum = settings->value("atomicNumber").toUInt();
        quant = settings->value("quantity").toUInt();
        m_opt->comp.insert(atomicNum, quant);
      }
      settings->endArray();
    }

    settings->endGroup();

    // Enact changesSetup templates
    updateDimensions();
  }

  void TabInit::updateGUI() {
    //qDebug() << "TabInit::updateGUI() called";
    ui.spin_a_min->setValue(				m_opt->a_min);
    ui.spin_b_min->setValue(				m_opt->b_min);
    ui.spin_c_min->setValue(				m_opt->c_min);
    ui.spin_a_max->setValue(				m_opt->a_max);
    ui.spin_b_max->setValue(				m_opt->b_max);
    ui.spin_c_max->setValue(				m_opt->c_max);
    ui.spin_alpha_min->setValue(			m_opt->alpha_min);
    ui.spin_beta_min->setValue(				m_opt->beta_min);
    ui.spin_gamma_min->setValue(			m_opt->gamma_min);
    ui.spin_alpha_max->setValue(			m_opt->alpha_max);
    ui.spin_beta_max->setValue(				m_opt->beta_max);
    ui.spin_gamma_max->setValue(			m_opt->gamma_max);
    ui.spin_vol_min->setValue(				m_opt->vol_min);
    ui.spin_vol_max->setValue(				m_opt->vol_max);
    ui.spin_fixedVolume->setValue(			m_opt->vol_fixed);
    ui.spin_shortestInteratomicDistance->setValue(	m_opt->shortestInteratomicDistance);
    ui.cb_fixedVolume->setChecked(			m_opt->using_fixed_volume);
    ui.cb_shortestInteratomicDistance->setChecked(	m_opt->using_shortestInteratomicDistance);
    updateComposition();
  }

  void TabInit::disconnectGUI() {
    //qDebug() << "TabInit::disconnectGUI() called";
    // Nothing I want to disconnect here.
  }

  void TabInit::lockGUI() {
    //qDebug() << "TabInit::lockGUI() called";
    ui.edit_composition->setDisabled(true);
  }

  void TabInit::getComposition(const QString &str) {
    //qDebug() << "TabInit::getComposition( " << str << ") called";
    QHash<uint, uint> comp;
    QString symbol;
    uint atomicNum;
    uint quantity;
    QStringList symbolList;
    QStringList quantityList;

    symbolList		= str.split(QRegExp("[0-9]"), QString::SkipEmptyParts);         // Parse numbers between letters
    quantityList        = str.split(QRegExp("[A-Z,a-z]"), QString::SkipEmptyParts);	// Parse letters between numbers

    m_opt->testingMode = (str.contains("testingMode")) ? true : false;

    // Use the shorter of the lists for the length
    uint length = (symbolList.size() < quantityList.size()) ? symbolList.size() : quantityList.size();

    if ( length == 0 ) {
      ui.list_composition->clear();
      m_opt->comp.clear();
      return;
    }

    // Build hash
    for (uint i = 0; i < length; i++){
      symbol    = symbolList.at(i);
      quantity	= quantityList.at(i).toUInt();

      if (symbol.contains("nRunsStart")) {
        m_opt->test_nRunsStart = quantity;
        continue;
      }
      if (symbol.contains("nRunsEnd")) {
        m_opt->test_nRunsEnd = quantity;
        continue;
      }
      if (symbol.contains("nStructs")) {
        m_opt->test_nStructs = quantity;
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
    m_opt->comp = comp;
  }

  void TabInit::updateComposition() {
    //qDebug() << "TabInit::updateComposition() called";
    QList<uint> keys = m_opt->comp.keys();
    qSort(keys);
    QString tmp;
    QTextStream str (&tmp);
    for (int i = 0; i < keys.size(); i++) {
      uint q = m_opt->comp.value(keys.at(i));
      str << OpenBabel::etab.GetSymbol(keys.at(i)) << q << " ";
    }
    if (m_opt->testingMode) {
      str << "nRunsStart" << m_opt->test_nRunsStart << " "
          << "nRunsEnd" << m_opt->test_nRunsEnd << " "
          << "nStructs" << m_opt->test_nStructs << " "
          << "testingMode ";
    }
    ui.edit_composition->setText(tmp.trimmed());
  }

  void TabInit::updateDimensions() {
    //qDebug() << "TabInit::updateDimensions() called";

    // Check for conflicts -- favor lower value
    if (ui.spin_a_min->value()		> ui.spin_a_max->value())	ui.spin_a_max->setValue(	ui.spin_a_min->value());
    if (ui.spin_b_min->value()          > ui.spin_b_max->value())	ui.spin_b_max->setValue(	ui.spin_b_min->value());
    if (ui.spin_c_min->value()          > ui.spin_c_max->value())	ui.spin_c_max->setValue(	ui.spin_c_min->value());
    if (ui.spin_alpha_min->value()      > ui.spin_alpha_max->value())	ui.spin_alpha_max->setValue(	ui.spin_alpha_min->value());
    if (ui.spin_beta_min->value()       > ui.spin_beta_max->value())	ui.spin_beta_max->setValue(     ui.spin_beta_min->value());
    if (ui.spin_gamma_min->value()      > ui.spin_gamma_max->value())	ui.spin_gamma_max->setValue(	ui.spin_gamma_min->value());
    if (ui.spin_vol_min->value()        > ui.spin_vol_max->value())	ui.spin_vol_max->setValue(	ui.spin_vol_min->value());

    // Assign variables
    m_opt->a_min		= ui.spin_a_min->value();
    m_opt->b_min		= ui.spin_b_min->value();
    m_opt->c_min		= ui.spin_c_min->value();
    m_opt->alpha_min	= ui.spin_alpha_min->value();
    m_opt->beta_min	= ui.spin_beta_min->value();
    m_opt->gamma_min	= ui.spin_gamma_min->value();
    m_opt->vol_min		= ui.spin_vol_min->value();

    m_opt->a_max		= ui.spin_a_max->value();
    m_opt->b_max		= ui.spin_b_max->value();
    m_opt->c_max		= ui.spin_c_max->value();
    m_opt->alpha_max	= ui.spin_alpha_max->value();
    m_opt->beta_max	= ui.spin_beta_max->value();
    m_opt->gamma_max	= ui.spin_gamma_max->value();
    m_opt->vol_max		= ui.spin_vol_max->value();

    m_opt->using_fixed_volume = ui.cb_fixedVolume->isChecked();
    m_opt->vol_fixed	= ui.spin_fixedVolume->value();

    m_opt->using_shortestInteratomicDistance       = ui.cb_shortestInteratomicDistance->isChecked();
    m_opt->shortestInteratomicDistance		= ui.spin_shortestInteratomicDistance->value();
  }
}

#include "tab_init.moc"

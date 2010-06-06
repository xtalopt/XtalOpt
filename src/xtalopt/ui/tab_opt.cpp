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

#include <xtalopt/ui/tab_opt.h>

#include <xtalopt/xtalopt.h>
#include <xtalopt/ui/dialog.h>

#include <QDebug>
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>

using namespace std;

namespace Avogadro {

  TabOpt::TabOpt( XtalOptDialog *parent, XtalOpt *p ) :
    QObject(parent), m_dialog(parent), m_opt(p)
  {
    //qDebug() << "TabOpt::TabOpt( " << parent <<  " ) called.";

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

    // Optimization connections
    // Initial generation
    connect(ui.spin_numInitial, SIGNAL(valueChanged(int)),
            this, SLOT(updateOptimizationInfo()));
    // Seeds
    connect(ui.push_addSeed, SIGNAL(clicked()),
            this, SLOT(addSeed()));
    connect(ui.push_removeSeed, SIGNAL(clicked()),
            this, SLOT(removeSeed()));
    connect(ui.list_seeds, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
            this, SLOT(addSeed(QListWidgetItem*)));

    // Search params
    connect(ui.spin_popSize, SIGNAL(valueChanged(int)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_contStructs, SIGNAL(valueChanged(int)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.cb_limitRunningJobs, SIGNAL(stateChanged(int)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_runningJobLimit, SIGNAL(valueChanged(int)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_failLimit, SIGNAL(valueChanged(int)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.combo_failAction, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateOptimizationInfo()));

    // Duplicate tolerances
    connect(ui.spin_tol_enthalpy, SIGNAL(editingFinished()),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_tol_volume, SIGNAL(editingFinished()),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.push_dup_reset, SIGNAL(clicked()),
            m_opt, SLOT(resetDuplicates()));

    // Crossover
    connect(ui.spin_p_cross, SIGNAL(valueChanged(int)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_cross_minimumContribution, SIGNAL(valueChanged(int)),
            this, SLOT(updateOptimizationInfo()));

    // Stripple
    connect(ui.spin_p_strip, SIGNAL(valueChanged(int)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_strip_strainStdev_min, SIGNAL(valueChanged(double)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_strip_strainStdev_max, SIGNAL(valueChanged(double)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_strip_amp_min, SIGNAL(valueChanged(double)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_strip_amp_max, SIGNAL(valueChanged(double)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_strip_per1, SIGNAL(valueChanged(int)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_strip_per2, SIGNAL(valueChanged(int)),
            this, SLOT(updateOptimizationInfo()));

    // Permustrain
    connect(ui.spin_p_perm, SIGNAL(valueChanged(int)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_perm_strainStdev_max, SIGNAL(valueChanged(double)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_perm_ex, SIGNAL(valueChanged(int)),
            this, SLOT(updateOptimizationInfo()));
  }

  TabOpt::~TabOpt()
  {
    //qDebug() << "TabOpt::~TabOpt() called";
  }

  void TabOpt::writeSettings(const QString &filename) {
    SETTINGS(filename);

    settings->beginGroup("xtalopt/opt/");

    // Initial generation
    settings->setValue("opt/numInitial",	m_opt->numInitial);

    // Search parameters
    settings->setValue("opt/popSize",		m_opt->popSize);
    settings->setValue("opt/contStructs",	m_opt->contStructs);
    settings->setValue("opt/limitRunningJobs",  m_opt->limitRunningJobs);
    settings->setValue("opt/runningJobLimit",   m_opt->runningJobLimit);
    settings->setValue("opt/failLimit",		m_opt->failLimit);
    settings->setValue("opt/failAction",	m_opt->failAction);

    // Duplicates
    settings->setValue("tol/enthalpy",		m_opt->tol_enthalpy);
    settings->setValue("tol/volume",		m_opt->tol_volume);

    // Crossover
    settings->setValue("opt/p_cross",		m_opt->p_cross);
    settings->setValue("opt/cross_minimumContribution",m_opt->cross_minimumContribution);

    // Stripple
    settings->setValue("opt/p_strip",		m_opt->p_strip);
    settings->setValue("opt/strip_strainStdev_min",	m_opt->strip_strainStdev_min);
    settings->setValue("opt/strip_strainStdev_max",	m_opt->strip_strainStdev_max);
    settings->setValue("opt/strip_amp_min",	m_opt->strip_amp_min);
    settings->setValue("opt/strip_amp_max",	m_opt->strip_amp_max);
    settings->setValue("opt/strip_per1",	m_opt->strip_per1);
    settings->setValue("opt/strip_per2",	m_opt->strip_per2);

    // Permustrain
    settings->setValue("opt/p_perm",		m_opt->p_perm);
    settings->setValue("opt/perm_strainStdev_max",m_opt->perm_strainStdev_max);
    settings->setValue("opt/perm_ex",		m_opt->perm_ex);

    settings->endGroup();

    DESTROY_SETTINGS(filename);
  }

  void TabOpt::readSettings(const QString &filename) {
    SETTINGS(filename);

    settings->beginGroup("xtalopt/opt/");


    // Initial generation
    ui.spin_numInitial->setValue(	settings->value("opt/numInitial",	20).toInt()     );

    // Search parameters
    ui.spin_popSize->setValue(		settings->value("opt/popSize",		20).toUInt()    );
    ui.spin_contStructs->setValue(	settings->value("opt/contStructs",	10).toUInt()    );
    ui.cb_limitRunningJobs->setChecked(	settings->value("opt/limitRunningJobs"	,false).toBool());
    ui.spin_runningJobLimit->setValue(	settings->value("opt/runningJobLimit",	1).toUInt()    );
    ui.spin_failLimit->setValue(	settings->value("opt/failLimit",	2).toUInt()    );
    ui.combo_failAction->setCurrentIndex(settings->value("opt/failAction",	XtalOpt::FA_Randomize).toUInt()    );

    // Duplicates
    ui.spin_tol_enthalpy->setValue(	settings->value("tol/enthalpy",		1e-2).toDouble());
    ui.spin_tol_volume->setValue(       settings->value("tol/volume",		1e-2).toDouble());

    // Crossover
    ui.spin_p_cross->setValue(		settings->value("opt/p_cross",		15).toUInt()    );
    ui.spin_cross_minimumContribution->setValue(settings->value("opt/cross_minimumContribution",25).toUInt());

    // Stripple
    ui.spin_p_strip->setValue(		settings->value("opt/p_strip",		50).toUInt()    );
    ui.spin_strip_strainStdev_min->setValue( settings->value("opt/strip_strainStdev_min", 0.5).toDouble());
    ui.spin_strip_strainStdev_max->setValue( settings->value("opt/strip_strainStdev_max", 0.5).toDouble());
    ui.spin_strip_amp_min->setValue(	settings->value("opt/strip_amp_min",	0.5).toDouble() );
    ui.spin_strip_amp_max->setValue(	settings->value("opt/strip_amp_max",	1.0).toDouble() );
    ui.spin_strip_per1->setValue(		settings->value("opt/strip_per1",		1).toUInt()     );
    ui.spin_strip_per2->setValue(		settings->value("opt/strip_per2",		1).toUInt()     );

    // Permustrain
    ui.spin_p_perm->setValue(		settings->value("opt/p_perm",		35).toUInt()     );
    ui.spin_perm_strainStdev_max->setValue(settings->value("opt/perm_strainStdev_max",0.5).toDouble());
    ui.spin_perm_ex->setValue(		settings->value("opt/perm_ex",		4).toUInt()     );

    settings->endGroup();

    updateOptimizationInfo();
  }

  void TabOpt::updateGUI() {
    //qDebug() << "TabOpt::updateGUI() called";
    // Initial generation
    ui.spin_numInitial->setValue(	m_opt->numInitial);

    // Search parameters
    ui.spin_popSize->setValue(		m_opt->popSize);
    ui.spin_contStructs->setValue(	m_opt->contStructs);
    ui.cb_limitRunningJobs->setChecked(	m_opt->limitRunningJobs);
    ui.spin_runningJobLimit->setValue(	m_opt->runningJobLimit);
    ui.spin_failLimit->setValue(	m_opt->failLimit);
    ui.combo_failAction->setCurrentIndex(m_opt->failAction);

    // Duplicates
    ui.spin_tol_enthalpy->setValue(	m_opt->tol_enthalpy);
    ui.spin_tol_volume->setValue(       m_opt->tol_volume);

    // Crossover
    ui.spin_p_cross->setValue(		m_opt->p_cross);
    ui.spin_cross_minimumContribution->setValue(m_opt->cross_minimumContribution);

    // Stripple
    ui.spin_p_strip->setValue(		m_opt->p_strip);
    ui.spin_strip_strainStdev_min->setValue( m_opt->strip_strainStdev_min);
    ui.spin_strip_strainStdev_max->setValue( m_opt->strip_strainStdev_max);
    ui.spin_strip_amp_min->setValue(	m_opt->strip_amp_min);
    ui.spin_strip_amp_max->setValue(	m_opt->strip_amp_max);
    ui.spin_strip_per1->setValue(		m_opt->strip_per1);
    ui.spin_strip_per2->setValue(		m_opt->strip_per2);

    // Permustrain
    ui.spin_p_perm->setValue(	m_opt->p_perm);
    ui.spin_perm_strainStdev_max->setValue( m_opt->perm_strainStdev_max);
    ui.spin_perm_ex->setValue(	m_opt->perm_ex);
  }

  void TabOpt::disconnectGUI() {
    //qDebug() << "TabOpt::disconnectGUI() called";
    // nothing I want to disconnect here!
  }

  void TabOpt::lockGUI() {
    //qDebug() << "TabPlot::lockGUI() called";
    ui.spin_numInitial->setDisabled(true);
    ui.list_seeds->setDisabled(true);
    ui.push_addSeed->setDisabled(true);
    ui.push_addSeed->setDisabled(true);
    ui.push_removeSeed->setDisabled(true);
  }

  void TabOpt::updateOptimizationInfo() {
    //qDebug() << "TabOpt::updateOptimizationInfo( ) called";
    m_opt->p_cross                = ui.spin_p_cross->value();
    m_opt->p_strip		= ui.spin_p_strip->value();
    m_opt->p_perm		= 100 - (m_opt->p_cross + m_opt->p_strip);
    ui.spin_p_perm->blockSignals(true);
    ui.spin_p_perm->setValue(m_opt->p_perm);
    ui.spin_p_perm->blockSignals(false);

    // Initial generation
    m_opt->numInitial           = ui.spin_numInitial->value();
    if (int(m_opt->numInitial) < ui.list_seeds->count())
      ui.spin_numInitial->setValue(ui.list_seeds->count());

    // Search parameters
    m_opt->popSize              = ui.spin_popSize->value();
    m_opt->contStructs          = ui.spin_contStructs->value();
    m_opt->runningJobLimit	= ui.spin_runningJobLimit->value();
    m_opt->limitRunningJobs	= ui.cb_limitRunningJobs->isChecked();
    m_opt->failLimit		= ui.spin_failLimit->value();
    m_opt->failAction		= XtalOpt::FailActions(ui.combo_failAction->currentIndex());

    // Duplicates
    m_opt->tol_enthalpy         = ui.spin_tol_enthalpy->value();
    m_opt->tol_volume           = ui.spin_tol_volume->value();

    // Crossover
    m_opt->cross_minimumContribution=ui.spin_cross_minimumContribution->value();

    // Stripple
    m_opt->strip_strainStdev_min  = ui.spin_strip_strainStdev_min->value();
    m_opt->strip_strainStdev_max  = ui.spin_strip_strainStdev_max->value();
    m_opt->strip_amp_min          = ui.spin_strip_amp_min->value();
    m_opt->strip_amp_max          = ui.spin_strip_amp_max->value();
    m_opt->strip_per1             = ui.spin_strip_per1->value();
    m_opt->strip_per2             = ui.spin_strip_per2->value();

    // Permustrain
    m_opt->perm_strainStdev_max	= ui.spin_perm_strainStdev_max->value();
    m_opt->perm_ex              = ui.spin_perm_ex->value();
  }

  void TabOpt::addSeed(QListWidgetItem *item) {
    //qDebug() << "TabOpt::addSeed( " << item << " ) called";
    QSettings settings; // Already set up in avogadro/src/main.cpp
    QString filename ("");
    bool replace = false;
    if (item) replace = true;

    // Set filename
    if (replace) {
      filename = item->text();
    }
    else {
      filename = settings.value("xtalopt/opt/seedPath", m_opt->filePath + "/POSCAR").toString();
    }

    // Launch file dialog
    QFileDialog dialog (m_dialog,
                        QString("Select structure file to use as seed"),
                        filename,
                        "VASP files (CONTCAR, POSCAR);;GULP files (*.got);;All Files (*)");
    dialog.selectFile(filename);
    dialog.setFileMode(QFileDialog::ExistingFile);
    if (dialog.exec())
      filename = dialog.selectedFiles().first();
    else { return;} // User cancel file selection.

    settings.setValue("xtalopt/opt/seedPath", filename);

    // Update text
    if (replace)	item->setText(filename);
    else		ui.list_seeds->addItem(filename);
    updateOptimizationInfo();
    updateSeeds();
  }

  void TabOpt::removeSeed() {
    //qDebug() << "TabOpt::removeSeeds() called";
    if (ui.list_seeds->count() == 0) return;
    delete ui.list_seeds->takeItem(ui.list_seeds->currentRow());
    updateSeeds();
  }

  void TabOpt::updateSeeds() {
    //qDebug() << "TabOpt::updateSeeds() called";
    m_opt->seedList.clear();
    for (int i = 0; i < ui.list_seeds->count(); i++)
      m_opt->seedList.append(ui.list_seeds->item(i)->text());
  }

}
//#include "tab_opt.moc"

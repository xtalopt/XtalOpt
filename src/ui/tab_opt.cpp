/**********************************************************************
  XtalOpt - Tools for advanced crystal optimization

  Copyright (C) 2009 by David Lonie

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include "tab_opt.h"

#include "dialog.h"

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
    connect(m_dialog, SIGNAL(tabsReadSettings()),
            this, SLOT(readSettings()));
    connect(m_dialog, SIGNAL(tabsWriteSettings()),
            this, SLOT(writeSettings()));
    connect(m_dialog, SIGNAL(tabsUpdateGUI()),
            this, SLOT(updateGUI()));
    connect(m_dialog, SIGNAL(tabsDisconnectGUI()),
            this, SLOT(disconnectGUI()));
    connect(m_dialog, SIGNAL(tabsLockGUI()),
            this, SLOT(lockGUI()));

    // Optimization connections
    // Initial generation
    connect(ui.spin_numStructures, SIGNAL(valueChanged(int)),
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
    connect(ui.spin_genTotal, SIGNAL(valueChanged(int)),
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

    // Heredity
    connect(ui.spin_p_her, SIGNAL(valueChanged(int)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_her_minimumContribution, SIGNAL(valueChanged(int)),
            this, SLOT(updateOptimizationInfo()));

    // Mutation
    connect(ui.spin_p_mut, SIGNAL(valueChanged(int)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_mut_strainStdev_min, SIGNAL(valueChanged(double)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_mut_strainStdev_max, SIGNAL(valueChanged(double)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_mut_amp_min, SIGNAL(valueChanged(double)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_mut_amp_max, SIGNAL(valueChanged(double)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_mut_per1, SIGNAL(valueChanged(int)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_mut_per2, SIGNAL(valueChanged(int)),
            this, SLOT(updateOptimizationInfo()));

    // Permutation
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

  void TabOpt::writeSettings() {
    //qDebug() << "TabOpt::writeSettings() called";
    QSettings settings; // Already set up in avogadro/src/main.cpp

    // Initial generation
    settings.setValue("xtalopt/dialog/opt/numStructs",		m_opt->numInitial);

    // Search parameters
    settings.setValue("xtalopt/dialog/opt/popSize",		m_opt->popSize);
    settings.setValue("xtalopt/dialog/opt/genTotal",		m_opt->genTotal);
    settings.setValue("xtalopt/dialog/opt/limitRunningJobs",    m_opt->limitRunningJobs);
    settings.setValue("xtalopt/dialog/opt/runningJobLimit",     m_opt->runningJobLimit);
    settings.setValue("xtalopt/dialog/opt/failLimit",		m_opt->failLimit);
    settings.setValue("xtalopt/dialog/opt/failAction",		m_opt->failAction);

    // Duplicates
    settings.setValue("xtalopt/dialog/tol/enthalpy",		m_opt->tol_enthalpy);
    settings.setValue("xtalopt/dialog/tol/volume",		m_opt->tol_volume);

    // Heredity
    settings.setValue("xtalopt/dialog/opt/p_her",		m_opt->p_her);
    settings.setValue("xtalopt/dialog/opt/her_minimumContribution",m_opt->her_minimumContribution);

    // Mutation
    settings.setValue("xtalopt/dialog/opt/p_mut",		m_opt->p_mut);
    settings.setValue("xtalopt/dialog/opt/mut_strainStdev_min",	m_opt->mut_strainStdev_min);
    settings.setValue("xtalopt/dialog/opt/mut_strainStdev_max",	m_opt->mut_strainStdev_max);
    settings.setValue("xtalopt/dialog/opt/mut_amp_min",		m_opt->mut_amp_min);
    settings.setValue("xtalopt/dialog/opt/mut_amp_max",		m_opt->mut_amp_max);
    settings.setValue("xtalopt/dialog/opt/mut_per1",		m_opt->mut_per1);
    settings.setValue("xtalopt/dialog/opt/mut_per2",		m_opt->mut_per2);

    // Permutation
    settings.setValue("xtalopt/dialog/opt/p_perm",		m_opt->p_perm);
    settings.setValue("xtalopt/dialog/opt/perm_strainStdev_max",m_opt->perm_strainStdev_max);
    settings.setValue("xtalopt/dialog/opt/perm_ex",		m_opt->perm_ex);
  }

  void TabOpt::readSettings() {
    //qDebug() << "TabOpt::readSettings() called";
    QSettings settings; // Already set up in avogadro/src/main.cpp

    // Initial generation
    ui.spin_numStructures->setValue(	settings.value("xtalopt/dialog/opt/numStructs",		20).toInt()     );

    // Search parameters
    ui.spin_popSize->setValue(		settings.value("xtalopt/dialog/opt/popSize",		60).toUInt()    );
    ui.spin_genTotal->setValue(		settings.value("xtalopt/dialog/opt/genTotal",		10).toUInt()    );
    ui.cb_limitRunningJobs->setChecked(	settings.value("xtalopt/dialog/opt/limitRunningJobs"	,false).toBool());
    ui.spin_runningJobLimit->setValue(	settings.value("xtalopt/dialog/opt/runningJobLimit",	1).toUInt()    );
    ui.spin_failLimit->setValue(	settings.value("xtalopt/dialog/opt/failLimit",		5).toUInt()    );
    ui.combo_failAction->setCurrentIndex(settings.value("xtalopt/dialog/opt/failAction",	0).toUInt()    );

    // Duplicates
    ui.spin_tol_enthalpy->setValue(	settings.value("xtalopt/dialog/tol/enthalpy",		1e-3).toDouble());
    ui.spin_tol_volume->setValue(       settings.value("xtalopt/dialog/tol/volume",		1e-3).toDouble());

    // Heredity
    ui.spin_p_her->setValue(		settings.value("xtalopt/dialog/opt/p_her",		50).toUInt()    );
    ui.spin_her_minimumContribution->setValue(settings.value("xtalopt/dialog/opt/her_minimumContribution",20).toUInt());

    // Mutation
    ui.spin_p_mut->setValue(		settings.value("xtalopt/dialog/opt/p_mut",		15).toUInt()    );
    ui.spin_mut_strainStdev_min->setValue( settings.value("xtalopt/dialog/opt/mut_strainStdev_min", 0.2).toDouble());
    ui.spin_mut_strainStdev_max->setValue( settings.value("xtalopt/dialog/opt/mut_strainStdev_max", 1.0).toDouble());
    ui.spin_mut_amp_min->setValue(	settings.value("xtalopt/dialog/opt/mut_amp_min",	0.2).toDouble() );
    ui.spin_mut_amp_max->setValue(	settings.value("xtalopt/dialog/opt/mut_amp_max",	1.0).toDouble() );
    ui.spin_mut_per1->setValue(		settings.value("xtalopt/dialog/opt/mut_per1",		1).toUInt()     );
    ui.spin_mut_per2->setValue(		settings.value("xtalopt/dialog/opt/mut_per2",		1).toUInt()     );

    // Permutation
    ui.spin_p_perm->setValue(		settings.value("xtalopt/dialog/opt/p_perm",		15).toUInt()     );
    ui.spin_perm_strainStdev_max->setValue(settings.value("xtalopt/dialog/opt/perm_strainStdev_max",0.5).toDouble());
    ui.spin_perm_ex->setValue(		settings.value("xtalopt/dialog/opt/perm_ex",			2).toUInt()     );
    updateOptimizationInfo();
  }

  void TabOpt::updateGUI() {
    //qDebug() << "TabOpt::updateGUI() called";
    // Initial generation
    ui.spin_numStructures->setValue(	m_opt->numInitial);

    // Search parameters
    ui.spin_popSize->setValue(		m_opt->popSize);
    ui.spin_genTotal->setValue(		m_opt->genTotal);
    ui.cb_limitRunningJobs->setChecked(	m_opt->limitRunningJobs);
    ui.spin_runningJobLimit->setValue(	m_opt->runningJobLimit);
    ui.spin_failLimit->setValue(	m_opt->failLimit);
    ui.combo_failAction->setCurrentIndex(m_opt->failAction);

    // Duplicates
    ui.spin_tol_enthalpy->setValue(	m_opt->tol_enthalpy);
    ui.spin_tol_volume->setValue(       m_opt->tol_volume);

    // Heredity
    ui.spin_p_her->setValue(		m_opt->p_her);
    ui.spin_her_minimumContribution->setValue(m_opt->her_minimumContribution);

    // Mutation
    ui.spin_p_mut->setValue(		m_opt->p_mut);
    ui.spin_mut_strainStdev_min->setValue( m_opt->mut_strainStdev_min);
    ui.spin_mut_strainStdev_max->setValue( m_opt->mut_strainStdev_max);
    ui.spin_mut_amp_min->setValue(	m_opt->mut_amp_min);
    ui.spin_mut_amp_max->setValue(	m_opt->mut_amp_max);
    ui.spin_mut_per1->setValue(		m_opt->mut_per1);
    ui.spin_mut_per2->setValue(		m_opt->mut_per2);

    // Permutation
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
    ui.spin_numStructures->setDisabled(true);
    ui.list_seeds->setDisabled(true);
    ui.push_addSeed->setDisabled(true);
    ui.push_addSeed->setDisabled(true);
    ui.push_removeSeed->setDisabled(true);
  }

  void TabOpt::updateOptimizationInfo() {
    //qDebug() << "TabOpt::updateOptimizationInfo( ) called";
    m_opt->p_her                = ui.spin_p_her->value();
    m_opt->p_mut		= ui.spin_p_mut->value();
    m_opt->p_perm		= 100 - (m_opt->p_her + m_opt->p_mut);
    ui.spin_p_perm->blockSignals(true);
    ui.spin_p_perm->setValue(m_opt->p_perm);
    ui.spin_p_perm->blockSignals(false);

    // Initial generation
    m_opt->numInitial           = ui.spin_numStructures->value();
    if (int(m_opt->numInitial) < ui.list_seeds->count())
      ui.spin_numStructures->setValue(ui.list_seeds->count());

    // Search parameters
    m_opt->popSize              = ui.spin_popSize->value();
    m_opt->genTotal             = ui.spin_genTotal->value();
    m_opt->runningJobLimit	= ui.spin_runningJobLimit->value();
    m_opt->limitRunningJobs	= ui.cb_limitRunningJobs->isChecked();
    m_opt->failLimit		= ui.spin_failLimit->value();
    m_opt->failAction		= ui.combo_failAction->currentIndex();

    // Duplicates
    m_opt->tol_enthalpy         = ui.spin_tol_enthalpy->value();
    m_opt->tol_volume           = ui.spin_tol_volume->value();

    // Heredity
    m_opt->her_minimumContribution=ui.spin_her_minimumContribution->value();

    // Mutation
    m_opt->mut_strainStdev_min  = ui.spin_mut_strainStdev_min->value();
    m_opt->mut_strainStdev_max  = ui.spin_mut_strainStdev_max->value();
    m_opt->mut_amp_min          = ui.spin_mut_amp_min->value();
    m_opt->mut_amp_max          = ui.spin_mut_amp_max->value();
    m_opt->mut_per1             = ui.spin_mut_per1->value();
    m_opt->mut_per2             = ui.spin_mut_per2->value();

    // Permutation
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
#include "tab_opt.moc"

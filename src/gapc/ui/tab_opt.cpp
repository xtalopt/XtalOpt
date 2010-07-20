/**********************************************************************
  TabOpt - Parameters to control the run of the search

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

#include <gapc/ui/tab_opt.h>

#include <gapc/gapc.h>
#include <gapc/ui/dialog.h>

#include <globalsearch/macros.h>

#include <QDebug>
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>

using namespace std;

namespace GAPC {

  TabOpt::TabOpt( GAPCDialog *parent, OptGAPC *p ) :
    AbstractTab(parent, p)
  {
    ui.setupUi(m_tab_widget);

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
    connect(ui.push_dup_reset, SIGNAL(clicked()),
            m_opt, SLOT(resetDuplicates()));

    initialize();
  }

  TabOpt::~TabOpt()
  {
  }

  void TabOpt::writeSettings(const QString &filename)
  {
    SETTINGS(filename);

    OptGAPC *gapc = qobject_cast<OptGAPC*>(m_opt);

    settings->beginGroup("gapc/opt/");

    // config version
    const int VERSION = 1;
    settings->setValue("version",               VERSION);

    // Initial generation
    settings->setValue("opt/numInitial",        gapc->numInitial);

    // Search parameters
    settings->setValue("opt/popSize",           gapc->popSize);
    settings->setValue("opt/contStructs",       gapc->contStructs);
    settings->setValue("opt/limitRunningJobs",  gapc->limitRunningJobs);
    settings->setValue("opt/runningJobLimit",   gapc->runningJobLimit);
    settings->setValue("opt/failLimit",         gapc->failLimit);
    settings->setValue("opt/failAction",        gapc->failAction);

    // Duplicates
    settings->setValue("tol/enthalpy",          gapc->tol_enthalpy);

    settings->endGroup();

    DESTROY_SETTINGS(filename);
  }

  void TabOpt::readSettings(const QString &filename)
  {
    SETTINGS(filename);

    settings->beginGroup("gapc/opt/");

    // Config version
    int loadedVersion = settings->value("version", 0).toInt();

    // Initial generation
    ui.spin_numInitial->setValue(       settings->value("opt/numInitial",       20).toInt()     );

    // Search parameters
    ui.spin_popSize->setValue(          settings->value("opt/popSize",          20).toUInt()    );
    ui.spin_contStructs->setValue(      settings->value("opt/contStructs",      10).toUInt()    );
    ui.cb_limitRunningJobs->setChecked( settings->value("opt/limitRunningJobs"  ,false).toBool());
    ui.spin_runningJobLimit->setValue(  settings->value("opt/runningJobLimit",  1).toUInt()    );
    ui.spin_failLimit->setValue(        settings->value("opt/failLimit",        2).toUInt()    );
    ui.combo_failAction->setCurrentIndex(settings->value("opt/failAction",      OptGAPC::FA_Randomize).toUInt()    );

    // Duplicates
    ui.spin_tol_enthalpy->setValue(     settings->value("tol/enthalpy",         1e-2).toDouble());

    settings->endGroup();

    // Update config data
    switch (loadedVersion) {
    case 0:
    case 1:
    default:
      break;
    }

    updateOptimizationInfo();
  }

  void TabOpt::updateGUI()
  {
    OptGAPC *gapc = qobject_cast<OptGAPC*>(m_opt);

    // Initial generation
    ui.spin_numInitial->setValue(       gapc->numInitial);

    // Search parameters
    ui.spin_popSize->setValue(          gapc->popSize);
    ui.spin_contStructs->setValue(      gapc->contStructs);
    ui.cb_limitRunningJobs->setChecked( gapc->limitRunningJobs);
    ui.spin_runningJobLimit->setValue(  gapc->runningJobLimit);
    ui.spin_failLimit->setValue(        gapc->failLimit);
    ui.combo_failAction->setCurrentIndex(gapc->failAction);

    // Duplicates
    ui.spin_tol_enthalpy->setValue(     gapc->tol_enthalpy);
  }

  void TabOpt::lockGUI()
  {
    ui.spin_numInitial->setDisabled(true);
    ui.list_seeds->setDisabled(true);
    ui.push_addSeed->setDisabled(true);
    ui.push_addSeed->setDisabled(true);
    ui.push_removeSeed->setDisabled(true);
  }

  void TabOpt::updateOptimizationInfo()
  {
    OptGAPC *gapc = qobject_cast<OptGAPC*>(m_opt);

    // Initial generation
    gapc->numInitial           = ui.spin_numInitial->value();
    if (int(gapc->numInitial) < ui.list_seeds->count())
      ui.spin_numInitial->setValue(ui.list_seeds->count());

    // Search parameters
    gapc->popSize              = ui.spin_popSize->value();
    gapc->contStructs          = ui.spin_contStructs->value();
    gapc->runningJobLimit	= ui.spin_runningJobLimit->value();
    gapc->limitRunningJobs	= ui.cb_limitRunningJobs->isChecked();
    gapc->failLimit		= ui.spin_failLimit->value();
    gapc->failAction		= OptGAPC::FailActions(ui.combo_failAction->currentIndex());

    // Duplicates
    gapc->tol_enthalpy         = ui.spin_tol_enthalpy->value();
  }

  void TabOpt::addSeed(QListWidgetItem *item)
  {
    QSettings settings; // Already set up in avogadro/src/main.cpp
    QString filename ("");
    bool replace = false;
    if (item) replace = true;

    // Set filename
    if (replace) {
      filename = item->text();
    }
    else {
      filename = settings.value("gapc/opt/seedPath", m_opt->filePath).toString();
    }

    // Launch file dialog
    QFileDialog dialog (m_dialog,
                        QString("Select structure file to use as seed"),
                        filename,
                        "All Files (*)");
    dialog.selectFile(filename);
    dialog.setFileMode(QFileDialog::ExistingFile);
    if (dialog.exec())
      filename = dialog.selectedFiles().first();
    else { return;} // User cancel file selection.

    settings.setValue("gapc/opt/seedPath", filename);

    // Update text
    if (replace)	item->setText(filename);
    else		ui.list_seeds->addItem(filename);
    updateOptimizationInfo();
    updateSeeds();
  }

  void TabOpt::removeSeed()
  {
    if (ui.list_seeds->count() == 0) return;
    delete ui.list_seeds->takeItem(ui.list_seeds->currentRow());
    updateSeeds();
  }

  void TabOpt::updateSeeds()
  {
    OptGAPC *gapc = qobject_cast<OptGAPC*>(m_opt);

    gapc->seedList.clear();
    for (int i = 0; i < ui.list_seeds->count(); i++)
      gapc->seedList.append(ui.list_seeds->item(i)->text());
  }

}

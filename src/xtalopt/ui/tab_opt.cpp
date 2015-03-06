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

#include <xtalopt/ui/tab_opt.h>

#include <xtalopt/xtalopt.h>
#include <xtalopt/ui/dialog.h>

#include <QtCore/QDebug>
#include <QtCore/QSettings>

#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

using namespace std;

namespace XtalOpt {

  TabOpt::TabOpt( XtalOptDialog *parent, XtalOpt *p ) :
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
    connect(ui.cb_using_FU_crossovers, SIGNAL(toggled(bool)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_FU_crossovers_generation, SIGNAL(valueChanged(int)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.cb_using_mitosis, SIGNAL(toggled(bool)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.cb_using_one_pool, SIGNAL(toggled(bool)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_chance_of_mitosis, SIGNAL(valueChanged(int)),
            this, SLOT(updateOptimizationInfo()));

    // Duplicate tolerances
    connect(ui.spin_tol_xcLength, SIGNAL(editingFinished()),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_tol_xcAngle, SIGNAL(editingFinished()),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_tol_spg, SIGNAL(editingFinished()),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.push_dup_reset, SIGNAL(clicked()),
            m_opt, SLOT(resetDuplicates()));
    connect(ui.push_spg_reset, SIGNAL(clicked()),
            m_opt, SLOT(resetSpacegroups()));

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

    initialize();
  }

  TabOpt::~TabOpt()
  {
  }

  void TabOpt::writeSettings(const QString &filename)
  {
    SETTINGS(filename);

    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    settings->beginGroup("xtalopt/opt/");

    // config version
    const int VERSION = 1;
    settings->setValue("version",               VERSION);

    // Initial generation
    settings->setValue("opt/numInitial",        xtalopt->numInitial);

    // Search parameters
    settings->setValue("opt/popSize",           xtalopt->popSize);
    settings->setValue("opt/contStructs",       xtalopt->contStructs);
    settings->setValue("opt/limitRunningJobs",  xtalopt->limitRunningJobs);
    settings->setValue("opt/runningJobLimit",   xtalopt->runningJobLimit);
    settings->setValue("opt/failLimit",         xtalopt->failLimit);
    settings->setValue("opt/failAction",        xtalopt->failAction);
    settings->setValue("opt/using_mitosis", xtalopt->using_mitosis);
    settings->setValue("opt/using_FU_crossovers", xtalopt->using_FU_crossovers);
    settings->setValue("opt/FU_crossovers_generation", xtalopt->FU_crossovers_generation);
    settings->setValue("opt/using_one_pool", xtalopt->using_one_pool);
    settings->setValue("opt/chance_of_mitosis", xtalopt->chance_of_mitosis);

    // Duplicates
    settings->setValue("tol/xtalcomp/length",   xtalopt->tol_xcLength);
    settings->setValue("tol/xtalcomp/angle",    xtalopt->tol_xcAngle);
    settings->setValue("tol/spg",               xtalopt->tol_spg);

    // Crossover
    settings->setValue("opt/p_cross",           xtalopt->p_cross);
    settings->setValue("opt/cross_minimumContribution",xtalopt->cross_minimumContribution);

    // Stripple
    settings->setValue("opt/p_strip",           xtalopt->p_strip);
    settings->setValue("opt/strip_strainStdev_min",     xtalopt->strip_strainStdev_min);
    settings->setValue("opt/strip_strainStdev_max",     xtalopt->strip_strainStdev_max);
    settings->setValue("opt/strip_amp_min",     xtalopt->strip_amp_min);
    settings->setValue("opt/strip_amp_max",     xtalopt->strip_amp_max);
    settings->setValue("opt/strip_per1",        xtalopt->strip_per1);
    settings->setValue("opt/strip_per2",        xtalopt->strip_per2);

    // Permustrain
    settings->setValue("opt/p_perm",            xtalopt->p_perm);
    settings->setValue("opt/perm_strainStdev_max",xtalopt->perm_strainStdev_max);
    settings->setValue("opt/perm_ex",           xtalopt->perm_ex);

    settings->endGroup();

    DESTROY_SETTINGS(filename);
  }

  void TabOpt::readSettings(const QString &filename)
  {
    SETTINGS(filename);

    settings->beginGroup("xtalopt/opt/");

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
    ui.combo_failAction->setCurrentIndex(settings->value("opt/failAction",      XtalOpt::FA_Randomize).toUInt()    );
    ui.cb_using_mitosis->setChecked(settings->value("opt/using_mitosis",false).toBool());
    ui.cb_using_FU_crossovers->setChecked(settings->value("opt/using_FU_crossovers",false).toBool());
    ui.spin_FU_crossovers_generation->setValue( settings->value("opt/FU_crossovers_generation",4).toUInt());
    ui.cb_using_one_pool->setChecked(settings->value("opt/using_one_pool",false).toBool());
    ui.spin_chance_of_mitosis->setValue( settings->value("opt/chance_of_mitosis",50).toUInt());

    // Duplicates
    ui.spin_tol_xcLength->setValue(     settings->value("tol/xtalcomp/length",  0.1).toDouble());
    ui.spin_tol_xcAngle->setValue(      settings->value("tol/xtalcomp/angle", 2.0).toDouble());
    ui.spin_tol_spg->setValue(          settings->value("tol/spg",              0.05).toDouble());

    // Crossover
    ui.spin_p_cross->setValue(          settings->value("opt/p_cross",          15).toUInt()    );
    ui.spin_cross_minimumContribution->setValue(settings->value("opt/cross_minimumContribution",25).toUInt());

    // Stripple
    ui.spin_p_strip->setValue(          settings->value("opt/p_strip",          50).toUInt()    );
    ui.spin_strip_strainStdev_min->setValue( settings->value("opt/strip_strainStdev_min", 0.5).toDouble());
    ui.spin_strip_strainStdev_max->setValue( settings->value("opt/strip_strainStdev_max", 0.5).toDouble());
    ui.spin_strip_amp_min->setValue(    settings->value("opt/strip_amp_min",    0.5).toDouble() );
    ui.spin_strip_amp_max->setValue(    settings->value("opt/strip_amp_max",    1.0).toDouble() );
    ui.spin_strip_per1->setValue(               settings->value("opt/strip_per1",               1).toUInt()     );
    ui.spin_strip_per2->setValue(               settings->value("opt/strip_per2",               1).toUInt()     );

    // Permustrain
    ui.spin_p_perm->setValue(           settings->value("opt/p_perm",           35).toUInt()     );
    ui.spin_perm_strainStdev_max->setValue(settings->value("opt/perm_strainStdev_max",0.5).toDouble());
    ui.spin_perm_ex->setValue(          settings->value("opt/perm_ex",          4).toUInt()     );

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
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    // Initial generation
    ui.spin_numInitial->setValue(       xtalopt->numInitial);

    // Search parameters
    ui.spin_popSize->setValue(          xtalopt->popSize);
    ui.spin_contStructs->setValue(      xtalopt->contStructs);
    ui.cb_limitRunningJobs->setChecked( xtalopt->limitRunningJobs);
    ui.spin_runningJobLimit->setValue(  xtalopt->runningJobLimit);
    ui.spin_failLimit->setValue(        xtalopt->failLimit);
    ui.combo_failAction->setCurrentIndex(xtalopt->failAction);
    ui.cb_using_mitosis->setChecked(xtalopt->using_mitosis);
    ui.cb_using_FU_crossovers->setChecked(xtalopt->using_FU_crossovers);
    ui.spin_FU_crossovers_generation->setValue(  xtalopt->FU_crossovers_generation);
    ui.cb_using_one_pool->setChecked(   xtalopt->using_one_pool);
    ui.spin_chance_of_mitosis->setValue(xtalopt->chance_of_mitosis);

    // Duplicates
    ui.spin_tol_xcLength->setValue(     xtalopt->tol_xcLength);
    ui.spin_tol_xcAngle->setValue(      xtalopt->tol_xcAngle);
    ui.spin_tol_spg->setValue(          xtalopt->tol_spg);

    // Crossover
    ui.spin_p_cross->setValue(          xtalopt->p_cross);
    ui.spin_cross_minimumContribution->setValue(xtalopt->cross_minimumContribution);

    // Stripple
    ui.spin_p_strip->setValue(          xtalopt->p_strip);
    ui.spin_strip_strainStdev_min->setValue( xtalopt->strip_strainStdev_min);
    ui.spin_strip_strainStdev_max->setValue( xtalopt->strip_strainStdev_max);
    ui.spin_strip_amp_min->setValue(    xtalopt->strip_amp_min);
    ui.spin_strip_amp_max->setValue(    xtalopt->strip_amp_max);
    ui.spin_strip_per1->setValue(       xtalopt->strip_per1);
    ui.spin_strip_per2->setValue(       xtalopt->strip_per2);

    // Permustrain
    ui.spin_p_perm->setValue(   xtalopt->p_perm);
    ui.spin_perm_strainStdev_max->setValue( xtalopt->perm_strainStdev_max);
    ui.spin_perm_ex->setValue(  xtalopt->perm_ex);
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
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    // See if the spin boxes caused this change.
    if (sender() == ui.spin_p_cross ||
        sender() == ui.spin_p_strip) {
      xtalopt->p_cross            = ui.spin_p_cross->value();
      xtalopt->p_strip            = ui.spin_p_strip->value();
      xtalopt->p_perm             = 100 - (xtalopt->p_cross + xtalopt->p_strip);
      ui.spin_p_perm->blockSignals(true);
      ui.spin_p_perm->setValue(xtalopt->p_perm);
      ui.spin_p_perm->blockSignals(false);
    }
    else if (sender() == ui.spin_p_perm) {
      xtalopt->p_perm             = ui.spin_p_perm->value();
      xtalopt->p_strip            = ui.spin_p_strip->value();
      xtalopt->p_cross            = 100 - (xtalopt->p_perm + xtalopt->p_strip);
      ui.spin_p_cross->blockSignals(true);
      ui.spin_p_cross->setValue(xtalopt->p_cross);
      ui.spin_p_cross->blockSignals(false);
    }
    else {
      xtalopt->p_perm             = ui.spin_p_perm->value();
      xtalopt->p_strip            = ui.spin_p_strip->value();
      xtalopt->p_cross            = ui.spin_p_cross->value();
    }

    // Initial generation
    xtalopt->numInitial           = ui.spin_numInitial->value();
    if (int(xtalopt->numInitial) < ui.list_seeds->count())
      ui.spin_numInitial->setValue(ui.list_seeds->count());

    // Search parameters
    xtalopt->popSize              = ui.spin_popSize->value();
    xtalopt->contStructs          = ui.spin_contStructs->value();
    xtalopt->runningJobLimit	= ui.spin_runningJobLimit->value();
    xtalopt->limitRunningJobs	= ui.cb_limitRunningJobs->isChecked();
    xtalopt->failLimit		= ui.spin_failLimit->value();
    xtalopt->failAction		= XtalOpt::FailActions(ui.combo_failAction->currentIndex());
    xtalopt->using_mitosis = ui.cb_using_mitosis->isChecked();
    xtalopt->using_FU_crossovers = ui.cb_using_FU_crossovers->isChecked();
    xtalopt->FU_crossovers_generation = ui.spin_FU_crossovers_generation->value();
    xtalopt->using_one_pool = ui.cb_using_one_pool->isChecked();
    xtalopt->chance_of_mitosis = ui.spin_chance_of_mitosis->value();

    // Duplicates
    xtalopt->tol_xcLength         = ui.spin_tol_xcLength->value();
    xtalopt->tol_xcAngle          = ui.spin_tol_xcAngle->value();
    xtalopt->tol_spg              = ui.spin_tol_spg->value();

    // Crossover
    xtalopt->cross_minimumContribution=ui.spin_cross_minimumContribution->value();

    // Stripple
    xtalopt->strip_strainStdev_min  = ui.spin_strip_strainStdev_min->value();
    xtalopt->strip_strainStdev_max  = ui.spin_strip_strainStdev_max->value();
    xtalopt->strip_amp_min          = ui.spin_strip_amp_min->value();
    xtalopt->strip_amp_max          = ui.spin_strip_amp_max->value();
    xtalopt->strip_per1             = ui.spin_strip_per1->value();
    xtalopt->strip_per2             = ui.spin_strip_per2->value();

    // Permustrain
    xtalopt->perm_strainStdev_max	= ui.spin_perm_strainStdev_max->value();
    xtalopt->perm_ex              = ui.spin_perm_ex->value();
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
                        "Common formats (*POSCAR *CONTCAR *.got *.cml *cif"
                        " *.out);;All Files (*)");
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

  void TabOpt::removeSeed()
  {
    if (ui.list_seeds->count() == 0) return;
    delete ui.list_seeds->takeItem(ui.list_seeds->currentRow());
    updateSeeds();
  }

  void TabOpt::updateSeeds()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    xtalopt->seedList.clear();
    for (int i = 0; i < ui.list_seeds->count(); i++)
      xtalopt->seedList.append(ui.list_seeds->item(i)->text());
  }

}

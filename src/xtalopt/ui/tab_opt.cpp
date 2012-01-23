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

    // Toggle GUI components when we switch search types
    connect(p, SIGNAL(isMolecularXtalSearchChanged(bool)),
            this, SLOT(updateGUI()));

    // Initial generation
    connect(ui.spin_numInitial, SIGNAL(valueChanged(int)),
            this, SLOT(updateSearchParams()));
    // Seeds
    connect(ui.push_addSeed, SIGNAL(clicked()),
            this, SLOT(addSeed()));
    connect(ui.push_removeSeed, SIGNAL(clicked()),
            this, SLOT(removeSeed()));
    connect(ui.list_seeds, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
            this, SLOT(addSeed(QListWidgetItem*)));

    // Search params
    connect(ui.spin_popSize, SIGNAL(valueChanged(int)),
            this, SLOT(updateSearchParams()));
    connect(ui.spin_contStructs, SIGNAL(valueChanged(int)),
            this, SLOT(updateSearchParams()));
    connect(ui.cb_limitRunningJobs, SIGNAL(stateChanged(int)),
            this, SLOT(updateSearchParams()));
    connect(ui.spin_runningJobLimit, SIGNAL(valueChanged(int)),
            this, SLOT(updateSearchParams()));
    connect(ui.spin_failLimit, SIGNAL(valueChanged(int)),
            this, SLOT(updateSearchParams()));
    connect(ui.combo_failAction, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateSearchParams()));

    // Duplicate tolerances
    connect(ui.spin_tol_xcLength, SIGNAL(editingFinished()),
            this, SLOT(updateSearchParams()));
    connect(ui.spin_tol_xcAngle, SIGNAL(editingFinished()),
            this, SLOT(updateSearchParams()));
    connect(ui.spin_tol_spg, SIGNAL(editingFinished()),
            this, SLOT(updateSearchParams()));
    connect(ui.push_dup_reset, SIGNAL(clicked()),
            m_opt, SLOT(resetDuplicates()));

    // Crossover - Ionic
    connect(ui.spin_p_cross, SIGNAL(valueChanged(int)),
            this, SLOT(updateIonicSearchParams()));
    connect(ui.spin_cross_minimumContribution, SIGNAL(valueChanged(int)),
            this, SLOT(updateIonicSearchParams()));

    // Stripple - Ionic
    connect(ui.spin_p_strip, SIGNAL(valueChanged(int)),
            this, SLOT(updateIonicSearchParams()));
    connect(ui.spin_strip_strainStdev_min, SIGNAL(valueChanged(double)),
            this, SLOT(updateIonicSearchParams()));
    connect(ui.spin_strip_strainStdev_max, SIGNAL(valueChanged(double)),
            this, SLOT(updateIonicSearchParams()));
    connect(ui.spin_strip_amp_min, SIGNAL(valueChanged(double)),
            this, SLOT(updateIonicSearchParams()));
    connect(ui.spin_strip_amp_max, SIGNAL(valueChanged(double)),
            this, SLOT(updateIonicSearchParams()));
    connect(ui.spin_strip_per1, SIGNAL(valueChanged(int)),
            this, SLOT(updateIonicSearchParams()));
    connect(ui.spin_strip_per2, SIGNAL(valueChanged(int)),
            this, SLOT(updateIonicSearchParams()));

    // Permustrain - Ionic
    connect(ui.spin_p_perm, SIGNAL(valueChanged(int)),
            this, SLOT(updateIonicSearchParams()));
    connect(ui.spin_perm_strainStdev_max, SIGNAL(valueChanged(double)),
            this, SLOT(updateIonicSearchParams()));
    connect(ui.spin_perm_ex, SIGNAL(valueChanged(int)),
            this, SLOT(updateIonicSearchParams()));

    // Mutation - Molecular
    connect(ui.spin_mga_latticeSamples, SIGNAL(valueChanged(int)),
            this, SLOT(updateMolecularSearchParams()));
    connect(ui.spin_mga_strainMin, SIGNAL(valueChanged(double)),
            this, SLOT(updateMolecularSearchParams()));
    connect(ui.spin_mga_strainMax, SIGNAL(valueChanged(double)),
            this, SLOT(updateMolecularSearchParams()));
    connect(ui.spin_mga_numMovers, SIGNAL(valueChanged(int)),
            this, SLOT(updateMolecularSearchParams()));
    connect(ui.spin_mga_numDisplacements, SIGNAL(valueChanged(int)),
            this, SLOT(updateMolecularSearchParams()));
    connect(ui.spin_mga_rotResDeg, SIGNAL(valueChanged(int)),
            this, SLOT(updateMolecularSearchParams()));
    connect(ui.spin_mga_numVolSamples, SIGNAL(valueChanged(int)),
            this, SLOT(updateMolecularSearchParams()));
    connect(ui.spin_mga_volMinFrac, SIGNAL(valueChanged(double)),
            this, SLOT(updateMolecularSearchParams()));
    connect(ui.spin_mga_volMaxFrac, SIGNAL(valueChanged(double)),
            this, SLOT(updateMolecularSearchParams()));

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

    // swirl - molecular
    settings->setValue("mxtalopt/mga_latticeSamples",
                       xtalopt->mga_numLatticeSamples);
    settings->setValue("mxtalopt/mga_stainMin",
                       xtalopt->mga_strainMin);
    settings->setValue("mxtalopt/mga_strainMax",
                       xtalopt->mga_strainMax);
    settings->setValue("mxtalopt/mga_numMovers",
                       xtalopt->mga_numMovers);
    settings->setValue("mxtalopt/mga_numDisplacements",
                       xtalopt->mga_numDisplacements);
    settings->setValue("mxtalopt/mga_rotResDeg",
                       xtalopt->mga_rotResDeg);
    settings->setValue("mxtalopt/mga_numVolSamples",
                       xtalopt->mga_numVolSamples);
    settings->setValue("mxtalopt/mga_volMinFrac",
                       xtalopt->mga_volMinFrac);
    settings->setValue("mxtalopt/mga_volMaxFrac",
                       xtalopt->mga_volMaxFrac);

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
    ui.spin_strip_per1->setValue(       settings->value("opt/strip_per1",       1).toUInt()     );
    ui.spin_strip_per2->setValue(       settings->value("opt/strip_per2",       1).toUInt()     );

    // Permustrain
    ui.spin_p_perm->setValue(           settings->value("opt/p_perm",           35).toUInt()     );
    ui.spin_perm_strainStdev_max->setValue(settings->value("opt/perm_strainStdev_max",0.5).toDouble());
    ui.spin_perm_ex->setValue(          settings->value("opt/perm_ex",          4).toUInt()     );

    // Mutation - Molecular
    ui.spin_mga_latticeSamples->setValue(
          settings->value("mxtalopt/mga_latticeSamples", 10).toInt());
    ui.spin_mga_strainMin->setValue(
          settings->value("mxtalopt/mga_stainMin", 0.0).toDouble());
    ui.spin_mga_strainMax->setValue(
          settings->value("mxtalopt/mga_strainMax", 0.5).toDouble());
    ui.spin_mga_numMovers->setValue(
          settings->value("mxtalopt/mga_numMovers", 2).toInt());
    ui.spin_mga_numDisplacements->setValue(
          settings->value("mxtalopt/mga_numDisplacements", 20).toInt());
    ui.spin_mga_rotResDeg->setValue(
          settings->value("mxtalopt/mga_rotResDeg", 30).toInt());
    ui.spin_mga_numVolSamples->setValue(
          settings->value("mxtalopt/mga_numVolSamples", 5).toInt());
    ui.spin_mga_volMinFrac->setValue(
          settings->value("mxtalopt/mga_volMinFrac", 0.9).toDouble());
    ui.spin_mga_volMaxFrac->setValue(
          settings->value("mxtalopt/mga_volMaxFrac", 1.1).toDouble());

    settings->endGroup();

    // Update config data
    switch (loadedVersion) {
    case 0:
    case 1:
    default:
      break;
    }

    updateSearchParams();
    updateIonicSearchParams();
    updateMolecularSearchParams();
  }

  void TabOpt::updateGUI()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    // hide/show GUI
    if (xtalopt->isMolecularXtalSearch()) {
      ui.gb_ionic->setVisible(false);
      ui.gb_molecular->setVisible(true);
      ui.list_seeds->clear();
      ui.gb_seeds->setVisible(false);
    }
    else {
      ui.gb_ionic->setVisible(true);
      ui.gb_molecular->setVisible(false);
      ui.gb_seeds->setVisible(true);
    }

    // Initial generation
    ui.spin_numInitial->setValue(       xtalopt->numInitial);

    // Search parameters
    ui.spin_popSize->setValue(          xtalopt->popSize);
    ui.spin_contStructs->setValue(      xtalopt->contStructs);
    ui.cb_limitRunningJobs->setChecked( xtalopt->limitRunningJobs);
    ui.spin_runningJobLimit->setValue(  xtalopt->runningJobLimit);
    ui.spin_failLimit->setValue(        xtalopt->failLimit);
    ui.combo_failAction->setCurrentIndex(xtalopt->failAction);

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

    // Crossover - Molecular
    ui.spin_mga_latticeSamples->setValue(xtalopt->mga_numLatticeSamples);
    ui.spin_mga_strainMin->setValue(xtalopt->mga_strainMin);
    ui.spin_mga_strainMax->setValue(xtalopt->mga_strainMax);
    ui.spin_mga_numMovers->setValue(xtalopt->mga_numMovers);
    ui.spin_mga_numDisplacements->setValue(xtalopt->mga_numDisplacements);
    ui.spin_mga_rotResDeg->setValue(xtalopt->mga_rotResDeg);
    ui.spin_mga_numVolSamples->setValue(xtalopt->mga_numVolSamples);
    ui.spin_mga_volMinFrac->setValue(xtalopt->mga_volMinFrac);
    ui.spin_mga_volMaxFrac->setValue(xtalopt->mga_volMaxFrac);
  }

  void TabOpt::lockGUI()
  {
    ui.spin_numInitial->setDisabled(true);
    ui.list_seeds->setDisabled(true);
    ui.push_addSeed->setDisabled(true);
    ui.push_addSeed->setDisabled(true);
    ui.push_removeSeed->setDisabled(true);
  }

  void TabOpt::updateSearchParams()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

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

    // Duplicates
    xtalopt->tol_xcLength         = ui.spin_tol_xcLength->value();
    xtalopt->tol_xcAngle          = ui.spin_tol_xcAngle->value();
    xtalopt->tol_spg              = ui.spin_tol_spg->value();
  }

  void TabOpt::updateIonicSearchParams()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    // See if the "percent new..." spin boxes caused this change.
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

    // Crossover
    xtalopt->cross_minimumContribution =
        ui.spin_cross_minimumContribution->value();

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

  void TabOpt::updateMolecularSearchParams()
  {
    XtalOpt *xtalopt = qobject_cast<XtalOpt*>(m_opt);

    // Mutation
    xtalopt->mga_numLatticeSamples = ui.spin_mga_latticeSamples->value();
    xtalopt->mga_strainMin = ui.spin_mga_strainMin->value();
    xtalopt->mga_strainMax = ui.spin_mga_strainMax->value();
    xtalopt->mga_numMovers = ui.spin_mga_numMovers->value();
    xtalopt->mga_numDisplacements = ui.spin_mga_numDisplacements->value();
    xtalopt->mga_rotResDeg = ui.spin_mga_rotResDeg->value();
    xtalopt->mga_numVolSamples = ui.spin_mga_numVolSamples->value();
    xtalopt->mga_volMinFrac = ui.spin_mga_volMinFrac->value();
    xtalopt->mga_volMaxFrac = ui.spin_mga_volMaxFrac->value();
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
    updateSearchParams();
    updateIonicSearchParams();
    updateMolecularSearchParams();
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

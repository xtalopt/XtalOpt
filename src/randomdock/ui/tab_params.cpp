
/**********************************************************************
  RandomDock -- A tool for analysing a matrix-substrate docking problem

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

#include "tab_params.h"

#include "randomdock.h"
#include "randomdockdialog.h"
#include "substratemol.h"
#include "matrixmol.h"

#include <QDebug>
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>

using namespace std;

namespace Avogadro {

  TabParams::TabParams( RandomDockParams *p ) :
    QObject(p->dialog), m_params(p)
  {
    qDebug() << "TabParams::TabParams( " << p <<  " ) called.";

    m_tab_widget = new QWidget;
    ui.setupUi(m_tab_widget);

    // dialog connections
    connect(p->dialog, SIGNAL(tabsReadSettings()),
            this, SLOT(readSettings()));
    connect(p->dialog, SIGNAL(tabsWriteSettings()),
            this, SLOT(writeSettings()));

    // Optimization connections
    connect(ui.spin_numSearches, SIGNAL(valueChanged(int)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_numMatrixMols, SIGNAL(valueChanged(int)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_cutoff, SIGNAL(valueChanged(int)),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_IAD_min, SIGNAL(editingFinished()),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_IAD_max, SIGNAL(editingFinished()),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_radius_min, SIGNAL(editingFinished()),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.spin_radius_max, SIGNAL(editingFinished()),
            this, SLOT(updateOptimizationInfo()));
    connect(ui.cb_radius_auto, SIGNAL(toggled(bool)),
            this, SLOT(updateOptimizationInfo()));
  }

  TabParams::~TabParams()
  {
    qDebug() << "TabParams::~TabParams() called";
    writeSettings();
  }

  void TabParams::writeSettings() {
    qDebug() << "TabParams::writeSettings() called";
    QSettings settings; // Already set up in avogadro/src/main.cpp

    settings.setValue("randomdock/params/numSearches",      	m_params->numSearches);
    settings.setValue("randomdock/params/numMatrixMol",      	m_params->numMatrixMol);
    settings.setValue("randomdock/params/cutoff",	      	m_params->cutoff);
    settings.setValue("randomdock/params/IAD_min",      	m_params->IAD_min);
    settings.setValue("randomdock/params/IAD_max",      	m_params->IAD_max);
    settings.setValue("randomdock/params/radius_min",      	m_params->radius_min);
    settings.setValue("randomdock/params/radius_max",      	m_params->radius_max);
    settings.setValue("randomdock/params/radius_auto",      	m_params->radius_auto);
  }

  void TabParams::readSettings() {
    qDebug() << "TabParams::readSettings() called";
    QSettings settings; // Already set up in avogadro/src/main.cpp

    ui.spin_numSearches->setValue(	settings.value("randomdock/params/numSearches",		10).toInt());
    ui.spin_numMatrixMols->setValue(	settings.value("randomdock/params/numMatrixMol",	1).toInt());
    ui.spin_cutoff->setValue(		settings.value("randomdock/params/cutoff",		0).toInt());
    ui.spin_IAD_min->setValue(		settings.value("randomdock/params/IAD_min",		0.8).toDouble());
    ui.spin_IAD_max->setValue(		settings.value("randomdock/params/IAD_max",		3.0).toDouble());
    ui.spin_radius_min->setValue(	settings.value("randomdock/params/radius_min",		20).toDouble());
    ui.spin_radius_max->setValue(	settings.value("randomdock/params/radius_max",		100).toDouble());
    ui.cb_radius_auto->setChecked(	settings.value("randomdock/params/radius_auto",		true).toBool());
    updateOptimizationInfo();
  }

  void TabParams::updateOptimizationInfo() {
    qDebug() << "TabParams::updateOptimizationInfo( ) called";

    // Logic first!
    if (ui.spin_IAD_min->value() > ui.spin_IAD_max->value())
      ui.spin_IAD_max->setValue(ui.spin_IAD_min->value());
    if (ui.spin_radius_min->value() > ui.spin_radius_max->value())
      ui.spin_radius_max->setValue(ui.spin_radius_min->value());

    m_params->numSearches	= ui.spin_numSearches->value();
    m_params->numMatrixMol	= ui.spin_numMatrixMols->value();
    m_params->cutoff		= ui.spin_cutoff->value();
    m_params->IAD_min		= ui.spin_IAD_min->value();
    m_params->IAD_max		= ui.spin_IAD_max->value();
    //  Auto radius --
    if (ui.cb_radius_auto->isChecked()) {
      // Check that we have substrate and at least one matrix element
      if (!m_params->substrate || m_params->matrixList->size() == 0) {
        ui.cb_radius_auto->setChecked(false);
        return;
      }
      // Iterate over all substrate conformers, find the shortest and the largest radii.
      double sub_short, sub_long, tmp;
      Substrate *sub = m_params->substrate;
      sub_short = sub_long = sub->radius();
      for (uint i = 0; i < sub->numConformers(); i++) {
        sub->setConformer(i);
        sub->updateMolecule();
        tmp = sub->radius();
        if (tmp < sub_short) sub_short = tmp;
        if (tmp > sub_long) sub_long = tmp;
      }
      //   Iterate over all atoms in matrix elements conformers, find longest radius of all
      double mat_short, mat_long;
      mat_short = mat_long = m_params->matrixList->first()->radius();
      for (int m = 0; m < m_params->matrixList->size(); m++) {
        Matrix *mat = m_params->matrixList->at(m);
        for (uint i = 0; i < mat->numConformers(); i++) {
          mat->setConformer(i);
          mat->updateMolecule();
          tmp = mat->radius();
          if (tmp < mat_short) mat_short = tmp;
          if (tmp > mat_long) mat_long = tmp;
        }
      }
      ui.spin_radius_min->blockSignals(true);
      ui.spin_radius_max->blockSignals(true);
      ui.spin_radius_min->setValue(mat_short);
      ui.spin_radius_max->setValue(mat_long + sub_long + m_params->IAD_max);
      ui.spin_radius_min->blockSignals(false);
      ui.spin_radius_max->blockSignals(false);
      ui.spin_radius_min->update();
      ui.spin_radius_max->update();
      qDebug() << "Done!";
    }
    m_params->radius_min	= ui.spin_radius_min->value();
    m_params->radius_max	= ui.spin_radius_max->value();
    m_params->radius_auto	= ui.cb_radius_auto->isChecked();
  }

  void TabParams::stopSubmission() {
    qDebug() << "TabParams::stopSubmission() called";
    ui.spin_numSearches->setValue(0);
  }

}
#include "tab_params.moc"

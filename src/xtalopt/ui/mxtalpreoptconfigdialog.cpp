/**********************************************************************
  MXtalPreoptConfigDialog

  Copyright (C) 2012 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include <xtalopt/xtalopt.h>

#include "mxtalpreoptconfigdialog.h"
#include "ui_mxtalpreoptconfigdialog.h"

namespace XtalOpt
{

MXtalPreoptConfigDialog::MXtalPreoptConfigDialog(XtalOpt *xtalopt, QWidget *parent) :
  QDialog(parent),
  ui(new Ui::MXtalPreoptConfigDialog),
  m_opt(xtalopt)
{
  ui->setupUi(this);

  // econv is displayed as 1eX, where X is configurable:
  double X = log(m_opt->mpo_econv)/log(10.0);
  // Round to nearest integer
  ui->spin_econv->setValue(floor(X+0.5));

  ui->spin_maxSteps->setValue(xtalopt->mpo_maxSteps);
  ui->spin_sCUpdateInterval->setValue(xtalopt->mpo_sCUpdateInterval);
  ui->spin_vdwCut->setValue(xtalopt->mpo_vdwCut);
  ui->spin_eleCut->setValue(xtalopt->mpo_eleCut);
  ui->spin_cutoffUpdateInterval->setValue(xtalopt->mpo_cutoffUpdateInterval);
  ui->cb_updateWithSc->setChecked((xtalopt->mpo_cutoffUpdateInterval < 0));
  ui->cb_debug->setChecked(xtalopt->mpo_debug);

  ui->label_warning->hide();
}

MXtalPreoptConfigDialog::~MXtalPreoptConfigDialog()
{
  delete ui;
}

void MXtalPreoptConfigDialog::accept()
{
  double X = ui->spin_econv->value();
  m_opt->mpo_econv = pow(10.0, X);

  m_opt->mpo_maxSteps = ui->spin_maxSteps->value();
  m_opt->mpo_sCUpdateInterval = ui->spin_sCUpdateInterval->value();
  m_opt->mpo_vdwCut = ui->spin_vdwCut->value();
  m_opt->mpo_eleCut = ui->spin_eleCut->value();

  if (ui->cb_updateWithSc->isChecked())
    m_opt->mpo_cutoffUpdateInterval = -1;
  else
    m_opt->mpo_cutoffUpdateInterval = ui->spin_cutoffUpdateInterval->value();

  m_opt->mpo_debug = ui->cb_debug->isChecked();

  QDialog::accept();
}

}

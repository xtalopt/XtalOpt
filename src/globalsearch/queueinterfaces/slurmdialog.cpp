/**********************************************************************
  SlurmConfigDialog -- Setup for remote SLURM queues

  Copyright (C) 2011 by David Lonie

  This library is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifdef ENABLE_SSH

// Doxygen skip:
/// @cond

#include <globalsearch/queueinterfaces/slurmdialog.h>

#include <globalsearch/queueinterfaces/slurm.h>

#include <globalsearch/ui/abstractdialog.h>
#include <globalsearch/optbase.h>

#include "ui_slurmdialog.h"

namespace GlobalSearch {

  SlurmConfigDialog::SlurmConfigDialog(AbstractDialog *parent,
                                       OptBase *o,
                                       SlurmQueueInterface *p)
    : QDialog(parent),
      ui(new Ui::SlurmConfigDialog),
      m_opt(o),
      m_slurm(p)
  {
    ui->setupUi(this);
  }

  SlurmConfigDialog::~SlurmConfigDialog()
  {
    delete ui;
  }

  void SlurmConfigDialog::updateGUI()
  {
    ui->edit_description->blockSignals(true);
    ui->edit_host->blockSignals(true);
    ui->edit_scancel->blockSignals(true);
    ui->edit_squeue->blockSignals(true);
    ui->edit_sbatch->blockSignals(true);
    ui->edit_rempath->blockSignals(true);
    ui->edit_locpath->blockSignals(true);
    ui->edit_username->blockSignals(true);
    ui->spin_port->blockSignals(true);
    ui->spin_interval->blockSignals(true);
    ui->cb_cleanRemoteOnStop->blockSignals(true);

    ui->edit_description->setText(m_opt->description);
    ui->edit_host->setText(m_opt->host);
    ui->edit_scancel->setText(m_slurm->m_scancel);
    ui->edit_squeue->setText(m_slurm->m_squeue);
    ui->edit_sbatch->setText(m_slurm->m_sbatch);
    ui->edit_rempath->setText(m_opt->rempath);
    ui->edit_locpath->setText(m_opt->filePath);
    ui->edit_username->setText(m_opt->username);
    ui->spin_port->setValue(m_opt->port);
    ui->spin_interval->setValue(m_slurm->m_interval);
    ui->cb_cleanRemoteOnStop->setChecked(m_slurm->m_cleanRemoteOnStop);

    ui->edit_description->blockSignals(false);
    ui->edit_host->blockSignals(false);
    ui->edit_scancel->blockSignals(false);
    ui->edit_squeue->blockSignals(false);
    ui->edit_sbatch->blockSignals(false);
    ui->edit_rempath->blockSignals(false);
    ui->edit_locpath->blockSignals(false);
    ui->edit_username->blockSignals(false);
    ui->spin_port->blockSignals(false);
    ui->spin_interval->blockSignals(false);
    ui->cb_cleanRemoteOnStop->blockSignals(false);
  }

  void SlurmConfigDialog::accept()
  {
    m_opt->description = ui->edit_description->text().trimmed();
    m_opt->host = ui->edit_host->text().trimmed();
    m_slurm->m_scancel = ui->edit_scancel->text().trimmed();
    m_slurm->m_squeue = ui->edit_squeue->text().trimmed();
    m_slurm->m_sbatch = ui->edit_sbatch->text().trimmed();
    m_opt->rempath = ui->edit_rempath->text().trimmed();
    m_opt->filePath = ui->edit_locpath->text().trimmed();
    m_opt->username = ui->edit_username->text().trimmed();
    m_opt->port = ui->spin_port->value();
    // Use setter for interval -- mutex must be locked.
    m_slurm->setInterval(ui->spin_interval->value());
    m_slurm->m_cleanRemoteOnStop = ui->cb_cleanRemoteOnStop->isChecked();
    QDialog::accepted();
    close();
  }

  void SlurmConfigDialog::reject()
  {
    updateGUI();
    QDialog::reject();
    close();
  }

}

/// @endcond
#endif // ENABLE_SSH

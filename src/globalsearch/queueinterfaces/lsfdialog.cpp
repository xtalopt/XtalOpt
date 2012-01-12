/**********************************************************************
  LsfConfigDialog -- Setup for remote LSF queues

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

#include <globalsearch/queueinterfaces/lsfdialog.h>

#include <globalsearch/queueinterfaces/lsf.h>

#include <globalsearch/ui/abstractdialog.h>
#include <globalsearch/optbase.h>

#include "ui_lsfdialog.h"

namespace GlobalSearch {

  LsfConfigDialog::LsfConfigDialog(AbstractDialog *parent,
                                   OptBase *o,
                                   LsfQueueInterface *p)
    : QDialog(parent),
      ui(new Ui::LsfConfigDialog),
      m_opt(o),
      m_lsf(p)
  {
    ui->setupUi(this);
  }

  LsfConfigDialog::~LsfConfigDialog()
  {
    delete ui;
  }

  void LsfConfigDialog::updateGUI()
  {
    ui->edit_description->blockSignals(true);
    ui->edit_host->blockSignals(true);
    ui->edit_bkill->blockSignals(true);
    ui->edit_bjobs->blockSignals(true);
    ui->edit_bsub->blockSignals(true);
    ui->edit_rempath->blockSignals(true);
    ui->edit_locpath->blockSignals(true);
    ui->edit_username->blockSignals(true);
    ui->spin_port->blockSignals(true);
    ui->cb_cleanRemoteOnStop->blockSignals(true);

    ui->edit_description->setText(m_opt->description);
    ui->edit_host->setText(m_opt->host);
    ui->edit_bkill->setText(m_lsf->m_bkill);
    ui->edit_bjobs->setText(m_lsf->m_bjobs);
    ui->edit_bsub->setText(m_lsf->m_bsub);
    ui->edit_rempath->setText(m_opt->rempath);
    ui->edit_locpath->setText(m_opt->filePath);
    ui->edit_username->setText(m_opt->username);
    ui->spin_port->setValue(m_opt->port);
    ui->cb_cleanRemoteOnStop->setChecked(m_lsf->m_cleanRemoteOnStop);

    ui->edit_description->blockSignals(false);
    ui->edit_host->blockSignals(false);
    ui->edit_bkill->blockSignals(false);
    ui->edit_bjobs->blockSignals(false);
    ui->edit_bsub->blockSignals(false);
    ui->edit_rempath->blockSignals(false);
    ui->edit_locpath->blockSignals(false);
    ui->edit_username->blockSignals(false);
    ui->spin_port->blockSignals(false);
    ui->cb_cleanRemoteOnStop->blockSignals(false);
  }

  void LsfConfigDialog::accept()
  {
    m_opt->description = ui->edit_description->text().trimmed();
    m_opt->host = ui->edit_host->text().trimmed();
    m_lsf->m_bkill = ui->edit_bkill->text().trimmed();
    m_lsf->m_bjobs = ui->edit_bjobs->text().trimmed();
    m_lsf->m_bsub = ui->edit_bsub->text().trimmed();
    m_opt->rempath = ui->edit_rempath->text().trimmed();
    m_opt->filePath = ui->edit_locpath->text().trimmed();
    m_opt->username = ui->edit_username->text().trimmed();
    m_opt->port = ui->spin_port->value();
    m_lsf->m_cleanRemoteOnStop = ui->cb_cleanRemoteOnStop->isChecked();
    QDialog::accepted();
    close();
  }

  void LsfConfigDialog::reject()
  {
    updateGUI();
    QDialog::reject();
    close();
  }

}

/// @endcond
#endif // ENABLE_SSH

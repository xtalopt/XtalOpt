/**********************************************************************
  PbsConfigDialog -- Setup for remote PBS queues

  Copyright (C) 2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifdef ENABLE_SSH

// Doxygen skip:
/// @cond

#include <globalsearch/queueinterfaces/pbsdialog.h>

#include <globalsearch/queueinterfaces/pbs.h>

#include <globalsearch/ui/abstractdialog.h>
#include <globalsearch/optbase.h>

#include "ui_pbsdialog.h"

namespace GlobalSearch {

  PbsConfigDialog::PbsConfigDialog(AbstractDialog *parent,
                                   OptBase *o,
                                   PbsQueueInterface *p)
    : QDialog(parent),
      ui(new Ui::PbsConfigDialog),
      m_opt(o),
      m_pbs(p)
  {
    ui->setupUi(this);
  }

  PbsConfigDialog::~PbsConfigDialog()
  {
    delete ui;
  }

  void PbsConfigDialog::updateGUI()
  {
    ui->edit_description->blockSignals(true);
    ui->edit_host->blockSignals(true);
    ui->edit_qdel->blockSignals(true);
    ui->edit_qstat->blockSignals(true);
    ui->edit_qsub->blockSignals(true);
    ui->edit_rempath->blockSignals(true);
    ui->edit_locpath->blockSignals(true);
    ui->edit_username->blockSignals(true);
    ui->spin_port->blockSignals(true);
    ui->spin_interval->blockSignals(true);
    ui->cb_cleanRemoteOnStop->blockSignals(true);
    ui->cb_logErrorDirs->blockSignals(true);

    ui->edit_description->setText(m_opt->description);
    ui->edit_host->setText(m_opt->host);
    ui->edit_qdel->setText(m_pbs->m_cancelCommand);
    ui->edit_qstat->setText(m_pbs->m_statusCommand);
    ui->edit_qsub->setText(m_pbs->m_submitCommand);
    ui->edit_rempath->setText(m_opt->rempath);
    ui->edit_locpath->setText(m_opt->filePath);
    ui->edit_username->setText(m_opt->username);
    ui->spin_port->setValue(m_opt->port);
    ui->spin_interval->setValue(m_pbs->m_interval);
    ui->cb_cleanRemoteOnStop->setChecked(m_pbs->m_cleanRemoteOnStop);
    ui->cb_logErrorDirs->setChecked(m_opt->m_logErrorDirs);

    ui->edit_description->blockSignals(false);
    ui->edit_host->blockSignals(false);
    ui->edit_qdel->blockSignals(false);
    ui->edit_qstat->blockSignals(false);
    ui->edit_qsub->blockSignals(false);
    ui->edit_rempath->blockSignals(false);
    ui->edit_locpath->blockSignals(false);
    ui->edit_username->blockSignals(false);
    ui->spin_port->blockSignals(false);
    ui->spin_interval->blockSignals(false);
    ui->cb_cleanRemoteOnStop->blockSignals(false);
    ui->cb_logErrorDirs->blockSignals(false);
  }

  void PbsConfigDialog::accept()
  {
    m_opt->description = ui->edit_description->text().trimmed();
    m_opt->host = ui->edit_host->text().trimmed();
    m_pbs->m_cancelCommand = ui->edit_qdel->text().trimmed();
    m_pbs->m_statusCommand = ui->edit_qstat->text().trimmed();
    m_pbs->m_submitCommand = ui->edit_qsub->text().trimmed();
    m_opt->rempath = ui->edit_rempath->text().trimmed();
    m_opt->filePath = ui->edit_locpath->text().trimmed();
    m_opt->username = ui->edit_username->text().trimmed();
    m_opt->port = ui->spin_port->value();
    // Use setter for interval -- mutex must be locked.
    m_pbs->setInterval(ui->spin_interval->value());
    m_pbs->m_cleanRemoteOnStop = ui->cb_cleanRemoteOnStop->isChecked();
    m_opt->m_logErrorDirs = ui->cb_logErrorDirs->isChecked();
    QDialog::accepted();
    close();
  }

  void PbsConfigDialog::reject()
  {
    updateGUI();
    QDialog::reject();
    close();
  }

}

/// @endcond
#endif // ENABLE_SSH

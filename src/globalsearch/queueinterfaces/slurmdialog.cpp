/**********************************************************************
  SlurmConfigDialog -- Setup for remote SLURM queues

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

#include <globalsearch/queueinterfaces/slurmdialog.h>

#include <globalsearch/queueinterfaces/slurm.h>

#include <globalsearch/optbase.h>
#include <globalsearch/ui/abstractdialog.h>

#include "ui_slurmdialog.h"

namespace GlobalSearch {

SlurmConfigDialog::SlurmConfigDialog(AbstractDialog* parent, OptBase* o,
                                     SlurmQueueInterface* p)
  : QDialog(parent), m_opt(o), m_slurm(p), ui(new Ui::SlurmConfigDialog)
{
  ui->setupUi(this);
  ui->cb_logErrorDirs->setChecked(m_opt->m_logErrorDirs);
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
  ui->cb_logErrorDirs->blockSignals(true);

  ui->edit_description->setText(m_opt->description);
  ui->edit_host->setText(m_opt->host);
  ui->edit_scancel->setText(m_slurm->m_cancelCommand);
  ui->edit_squeue->setText(m_slurm->m_statusCommand);
  ui->edit_sbatch->setText(m_slurm->m_submitCommand);
  ui->edit_rempath->setText(m_opt->rempath);
  ui->edit_locpath->setText(m_opt->filePath);
  ui->edit_username->setText(m_opt->username);
  ui->spin_port->setValue(m_opt->port);
  ui->spin_interval->setValue(m_opt->queueRefreshInterval());
  ui->cb_cleanRemoteOnStop->setChecked(m_opt->cleanRemoteOnStop());
  ui->cb_logErrorDirs->setChecked(m_opt->m_logErrorDirs);

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
  ui->cb_logErrorDirs->blockSignals(false);
}

void SlurmConfigDialog::accept()
{
  m_opt->description = ui->edit_description->text().trimmed();
  m_opt->host = ui->edit_host->text().trimmed();
  m_slurm->m_cancelCommand = ui->edit_scancel->text().trimmed();
  m_slurm->m_statusCommand = ui->edit_squeue->text().trimmed();
  m_slurm->m_submitCommand = ui->edit_sbatch->text().trimmed();
  m_opt->rempath = ui->edit_rempath->text().trimmed();
  m_opt->filePath = ui->edit_locpath->text().trimmed();
  m_opt->username = ui->edit_username->text().trimmed();
  m_opt->port = ui->spin_port->value();
  // Use setter for interval -- mutex must be locked.
  m_opt->setQueueRefreshInterval(ui->spin_interval->value());
  m_opt->setCleanRemoteOnStop(ui->cb_cleanRemoteOnStop->isChecked());
  m_opt->m_logErrorDirs = ui->cb_logErrorDirs->isChecked();
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

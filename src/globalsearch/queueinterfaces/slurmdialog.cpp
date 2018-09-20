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
}

SlurmConfigDialog::~SlurmConfigDialog()
{
  delete ui;
}

void SlurmConfigDialog::updateGUI()
{
  ui->edit_scancel->blockSignals(true);
  ui->edit_squeue->blockSignals(true);
  ui->edit_sbatch->blockSignals(true);

  ui->edit_scancel->setText(m_slurm->m_cancelCommand);
  ui->edit_squeue->setText(m_slurm->m_statusCommand);
  ui->edit_sbatch->setText(m_slurm->m_submitCommand);

  ui->edit_scancel->blockSignals(false);
  ui->edit_squeue->blockSignals(false);
  ui->edit_sbatch->blockSignals(false);

  ui->widget_globalQueueInterfaceSettings->updateGUI(m_opt);
}

void SlurmConfigDialog::accept()
{
  m_slurm->m_cancelCommand = ui->edit_scancel->text().trimmed();
  m_slurm->m_statusCommand = ui->edit_squeue->text().trimmed();
  m_slurm->m_submitCommand = ui->edit_sbatch->text().trimmed();

  ui->widget_globalQueueInterfaceSettings->accept(m_opt);

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

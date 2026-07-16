/**********************************************************************
  SlurmConfigDialog - Setup for SLURM queues

  Copyright (C) 2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

// Doxygen skip:
/// @cond

#include <search/gui/queueinterfaces/slurmdialog.h>

#include <search/queueinterfaces/slurm.h>

#include <search/search.h>

#include "ui_slurmdialog.h"

namespace Search {

SlurmConfigDialog::SlurmConfigDialog(QWidget* parent, SearchBase* o, SlurmQueueInterface* p)
  : QDialog(parent), m_search(o), m_slurm(p), ui(new Ui::SlurmConfigDialog)
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

  ui->edit_scancel->setText(m_slurm->cancelCommand());
  ui->edit_squeue->setText(m_slurm->statusCommand());
  ui->edit_sbatch->setText(m_slurm->submitCommand());

  ui->edit_scancel->blockSignals(false);
  ui->edit_squeue->blockSignals(false);
  ui->edit_sbatch->blockSignals(false);

  ui->widget_globalQueueInterfaceSettings->updateGUI(m_search);
}

void SlurmConfigDialog::accept()
{
  if (!m_search->isSessionInProgress()) {
    m_slurm->setCancelCommand(ui->edit_scancel->text().trimmed());
    m_slurm->setStatusCommand(ui->edit_squeue->text().trimmed());
    m_slurm->setSubmitCommand(ui->edit_sbatch->text().trimmed());
  }

  ui->widget_globalQueueInterfaceSettings->accept(m_search);

  QDialog::accept();
}

void SlurmConfigDialog::reject()
{
  updateGUI();
  QDialog::reject();
  close();
}
}

/// @endcond

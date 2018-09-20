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

#include <globalsearch/optbase.h>
#include <globalsearch/ui/abstractdialog.h>

#include "ui_pbsdialog.h"

namespace GlobalSearch {

PbsConfigDialog::PbsConfigDialog(AbstractDialog* parent, OptBase* o,
                                 PbsQueueInterface* p)
  : QDialog(parent), m_opt(o), m_pbs(p), ui(new Ui::PbsConfigDialog)
{
  ui->setupUi(this);
}

PbsConfigDialog::~PbsConfigDialog()
{
  delete ui;
}

void PbsConfigDialog::updateGUI()
{
  ui->edit_qdel->blockSignals(true);
  ui->edit_qstat->blockSignals(true);
  ui->edit_qsub->blockSignals(true);

  ui->edit_qdel->setText(m_pbs->m_cancelCommand);
  ui->edit_qstat->setText(m_pbs->m_statusCommand);
  ui->edit_qsub->setText(m_pbs->m_submitCommand);

  ui->edit_qdel->blockSignals(false);
  ui->edit_qstat->blockSignals(false);
  ui->edit_qsub->blockSignals(false);

  ui->widget_globalQueueInterfaceSettings->updateGUI(m_opt);
}

void PbsConfigDialog::accept()
{
  m_pbs->m_cancelCommand = ui->edit_qdel->text().trimmed();
  m_pbs->m_statusCommand = ui->edit_qstat->text().trimmed();
  m_pbs->m_submitCommand = ui->edit_qsub->text().trimmed();

  ui->widget_globalQueueInterfaceSettings->accept(m_opt);

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

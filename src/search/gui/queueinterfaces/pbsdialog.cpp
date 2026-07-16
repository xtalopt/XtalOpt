/**********************************************************************
  PbsConfigDialog - Setup for PBS queues

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

#include <search/gui/queueinterfaces/pbsdialog.h>

#include <search/queueinterfaces/pbs.h>

#include <search/search.h>

#include "ui_pbsdialog.h"

namespace Search {

PbsConfigDialog::PbsConfigDialog(QWidget* parent, SearchBase* o, PbsQueueInterface* p)
  : QDialog(parent), m_search(o), m_pbs(p), ui(new Ui::PbsConfigDialog)
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

  ui->edit_qdel->setText(m_pbs->cancelCommand());
  ui->edit_qstat->setText(m_pbs->statusCommand());
  ui->edit_qsub->setText(m_pbs->submitCommand());

  ui->edit_qdel->blockSignals(false);
  ui->edit_qstat->blockSignals(false);
  ui->edit_qsub->blockSignals(false);

  ui->widget_globalQueueInterfaceSettings->updateGUI(m_search);
}

void PbsConfigDialog::accept()
{
  if (!m_search->isSessionInProgress()) {
    m_pbs->setCancelCommand(ui->edit_qdel->text().trimmed());
    m_pbs->setStatusCommand(ui->edit_qstat->text().trimmed());
    m_pbs->setSubmitCommand(ui->edit_qsub->text().trimmed());
  }

  ui->widget_globalQueueInterfaceSettings->accept(m_search);

  QDialog::accept();
}

void PbsConfigDialog::reject()
{
  updateGUI();
  QDialog::reject();
  close();
}
}

/// @endcond

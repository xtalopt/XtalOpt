/**********************************************************************
  SgeConfigDialog - Setup for SGE queues

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

#include <search/gui/queueinterfaces/sgedialog.h>

#include <search/queueinterfaces/sge.h>

#include <search/search.h>

#include "ui_sgedialog.h"

namespace Search {

SgeConfigDialog::SgeConfigDialog(QWidget* parent, SearchBase* o, SgeQueueInterface* p)
  : QDialog(parent), m_search(o), m_sge(p), ui(new Ui::SgeConfigDialog)
{
  ui->setupUi(this);
}

SgeConfigDialog::~SgeConfigDialog()
{
  delete ui;
}

void SgeConfigDialog::updateGUI()
{
  ui->edit_qdel->blockSignals(true);
  ui->edit_qstat->blockSignals(true);
  ui->edit_qsub->blockSignals(true);

  ui->edit_qdel->setText(m_sge->cancelCommand());
  ui->edit_qstat->setText(m_sge->statusCommand());
  ui->edit_qsub->setText(m_sge->submitCommand());

  ui->edit_qdel->blockSignals(false);
  ui->edit_qstat->blockSignals(false);
  ui->edit_qsub->blockSignals(false);

  ui->widget_globalQueueInterfaceSettings->updateGUI(m_search);
}

void SgeConfigDialog::accept()
{
  if (!m_search->isSessionInProgress()) {
    m_sge->setCancelCommand(ui->edit_qdel->text().trimmed());
    m_sge->setStatusCommand(ui->edit_qstat->text().trimmed());
    m_sge->setSubmitCommand(ui->edit_qsub->text().trimmed());
  }

  ui->widget_globalQueueInterfaceSettings->accept(m_search);

  QDialog::accept();
}

void SgeConfigDialog::reject()
{
  updateGUI();
  QDialog::reject();
  close();
}
}

/// @endcond

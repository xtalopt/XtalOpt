/**********************************************************************
  LsfConfigDialog - Setup for LSF queues

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

#include <search/gui/queueinterfaces/lsfdialog.h>

#include <search/queueinterfaces/lsf.h>

#include <search/search.h>

#include "ui_lsfdialog.h"

namespace Search {

LsfConfigDialog::LsfConfigDialog(QWidget* parent, SearchBase* o, LsfQueueInterface* p)
  : QDialog(parent), m_search(o), m_lsf(p), ui(new Ui::LsfConfigDialog)
{
  ui->setupUi(this);
}

LsfConfigDialog::~LsfConfigDialog()
{
  delete ui;
}

void LsfConfigDialog::updateGUI()
{
  ui->edit_bkill->blockSignals(true);
  ui->edit_bjobs->blockSignals(true);
  ui->edit_bsub->blockSignals(true);

  ui->edit_bkill->setText(m_lsf->cancelCommand());
  ui->edit_bjobs->setText(m_lsf->statusCommand());
  ui->edit_bsub->setText(m_lsf->submitCommand());

  ui->edit_bkill->blockSignals(false);
  ui->edit_bjobs->blockSignals(false);
  ui->edit_bsub->blockSignals(false);

  ui->widget_globalQueueInterfaceSettings->updateGUI(m_search);
}

void LsfConfigDialog::accept()
{
  if (!m_search->isSessionInProgress()) {
    m_lsf->setCancelCommand(ui->edit_bkill->text().trimmed());
    m_lsf->setStatusCommand(ui->edit_bjobs->text().trimmed());
    m_lsf->setSubmitCommand(ui->edit_bsub->text().trimmed());
  }

  ui->widget_globalQueueInterfaceSettings->accept(m_search);

  QDialog::accept();
}

void LsfConfigDialog::reject()
{
  updateGUI();
  QDialog::reject();
  close();
}
}

/// @endcond

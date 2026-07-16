/**********************************************************************
  LoadLevelerConfigDialog - Setup for LoadLeveler queues

  Copyright (C) 2012 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

// Doxygen skip:
/// @cond

#include <search/gui/queueinterfaces/loadlevelerdialog.h>

#include <search/queueinterfaces/loadleveler.h>

#include <search/search.h>

#include "ui_loadlevelerdialog.h"

namespace Search {

LoadLevelerConfigDialog::LoadLevelerConfigDialog(QWidget* parent, SearchBase* o,
                                                 LoadLevelerQueueInterface* p)
  : QDialog(parent), m_search(o), m_ll(p), ui(new Ui::LoadLevelerConfigDialog)
{
  ui->setupUi(this);
}

LoadLevelerConfigDialog::~LoadLevelerConfigDialog()
{
  delete ui;
}

void LoadLevelerConfigDialog::updateGUI()
{
  ui->edit_llcancel->blockSignals(true);
  ui->edit_llq->blockSignals(true);
  ui->edit_llsubmit->blockSignals(true);

  ui->edit_llcancel->setText(m_ll->cancelCommand());
  ui->edit_llq->setText(m_ll->statusCommand());
  ui->edit_llsubmit->setText(m_ll->submitCommand());

  ui->edit_llcancel->blockSignals(false);
  ui->edit_llq->blockSignals(false);
  ui->edit_llsubmit->blockSignals(false);

  ui->widget_globalQueueInterfaceSettings->updateGUI(m_search);
}

void LoadLevelerConfigDialog::accept()
{
  if (!m_search->isSessionInProgress()) {
    m_ll->setCancelCommand(ui->edit_llcancel->text().trimmed());
    m_ll->setStatusCommand(ui->edit_llq->text().trimmed());
    m_ll->setSubmitCommand(ui->edit_llsubmit->text().trimmed());
  }

  ui->widget_globalQueueInterfaceSettings->accept(m_search);

  QDialog::accept();
}

void LoadLevelerConfigDialog::reject()
{
  updateGUI();
  QDialog::reject();
  close();
}
}

/// @endcond

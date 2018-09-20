/**********************************************************************
  LoadLevelerConfigDialog

  Copyright (C) 2012 by David Lonie

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

#include <globalsearch/queueinterfaces/loadlevelerdialog.h>

#include <globalsearch/queueinterfaces/loadleveler.h>

#include <globalsearch/optbase.h>
#include <globalsearch/ui/abstractdialog.h>

#include "ui_loadlevelerdialog.h"

namespace GlobalSearch {

LoadLevelerConfigDialog::LoadLevelerConfigDialog(AbstractDialog* parent,
                                                 OptBase* o,
                                                 LoadLevelerQueueInterface* p)
  : QDialog(parent), m_opt(o), m_ll(p), ui(new Ui::LoadLevelerConfigDialog)
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

  ui->edit_llcancel->setText(m_ll->m_cancelCommand);
  ui->edit_llq->setText(m_ll->m_statusCommand);
  ui->edit_llsubmit->setText(m_ll->m_submitCommand);

  ui->edit_llcancel->blockSignals(false);
  ui->edit_llq->blockSignals(false);
  ui->edit_llsubmit->blockSignals(false);

  ui->widget_globalQueueInterfaceSettings->updateGUI(m_opt);
}

void LoadLevelerConfigDialog::accept()
{
  m_ll->m_cancelCommand = ui->edit_llcancel->text().trimmed();
  m_ll->m_statusCommand = ui->edit_llq->text().trimmed();
  m_ll->m_submitCommand = ui->edit_llsubmit->text().trimmed();

  ui->widget_globalQueueInterfaceSettings->accept(m_opt);

  QDialog::accepted();
  close();
}

void LoadLevelerConfigDialog::reject()
{
  updateGUI();
  QDialog::reject();
  close();
}
}

/// @endcond
#endif // ENABLE_SSH

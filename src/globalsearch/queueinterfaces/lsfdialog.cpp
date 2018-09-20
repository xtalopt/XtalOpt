/**********************************************************************
  LsfConfigDialog -- Setup for remote LSF queues

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

#include <globalsearch/queueinterfaces/lsfdialog.h>

#include <globalsearch/queueinterfaces/lsf.h>

#include <globalsearch/optbase.h>
#include <globalsearch/ui/abstractdialog.h>

#include "ui_lsfdialog.h"

namespace GlobalSearch {

LsfConfigDialog::LsfConfigDialog(AbstractDialog* parent, OptBase* o,
                                 LsfQueueInterface* p)
  : QDialog(parent), m_opt(o), m_lsf(p), ui(new Ui::LsfConfigDialog)
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

  ui->edit_bkill->setText(m_lsf->m_cancelCommand);
  ui->edit_bjobs->setText(m_lsf->m_statusCommand);
  ui->edit_bsub->setText(m_lsf->m_submitCommand);

  ui->edit_bkill->blockSignals(false);
  ui->edit_bjobs->blockSignals(false);
  ui->edit_bsub->blockSignals(false);

  ui->widget_globalQueueInterfaceSettings->updateGUI(m_opt);
}

void LsfConfigDialog::accept()
{
  m_lsf->m_cancelCommand = ui->edit_bkill->text().trimmed();
  m_lsf->m_statusCommand = ui->edit_bjobs->text().trimmed();
  m_lsf->m_submitCommand = ui->edit_bsub->text().trimmed();

  ui->widget_globalQueueInterfaceSettings->accept(m_opt);

  QDialog::accepted();
  close();
}

void LsfConfigDialog::reject()
{
  updateGUI();
  QDialog::reject();
  close();
}
}

/// @endcond
#endif // ENABLE_SSH

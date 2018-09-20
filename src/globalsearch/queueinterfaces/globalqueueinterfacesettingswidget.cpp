/**********************************************************************
  GlobalQueueInterfaceSettingsWidget - set the global QI settings

  Copyright (C) 2018 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <globalsearch/optbase.h>

#include "globalqueueinterfacesettingswidget.h"
#include "ui_globalqueueinterfacesettingswidget.h"

namespace GlobalSearch {

GlobalQueueInterfaceSettingsWidget::GlobalQueueInterfaceSettingsWidget(
  QWidget* parent)
  : QWidget(parent)
  , m_ui(new Ui::GlobalQueueInterfaceSettingsWidget)
{
  m_ui->setupUi(this);
}

GlobalQueueInterfaceSettingsWidget::~GlobalQueueInterfaceSettingsWidget() =
  default;

void GlobalQueueInterfaceSettingsWidget::updateGUI(GlobalSearch::OptBase* opt)
{
  // Block signals for all widgets when we update the GUI
  QList<bool> wasBlocked;
  QList<QWidget*> children = this->findChildren<QWidget*>();
  for (auto& widget : children)
    wasBlocked.append(widget->blockSignals(true));

  m_ui->edit_description->setText(opt->description);
  m_ui->edit_host->setText(opt->host);
  m_ui->edit_rempath->setText(opt->rempath);
  m_ui->edit_locpath->setText(opt->filePath);
  m_ui->edit_username->setText(opt->username);
  m_ui->spin_port->setValue(opt->port);
  m_ui->cb_cleanRemoteOnStop->setChecked(opt->cleanRemoteOnStop());
  m_ui->cb_logErrorDirs->setChecked(opt->m_logErrorDirs);
  m_ui->cb_cancelJobAfterTime->setChecked(opt->cancelJobAfterTime());
  m_ui->spin_hoursForCancelJobAfterTime->setValue(
    opt->hoursForCancelJobAfterTime());

  m_ui->spin_hoursForCancelJobAfterTime->setEnabled(
    m_ui->cb_cancelJobAfterTime->isChecked());

  // Restore the previous states of the widgets
  for (int i = 0; i < children.size(); ++i)
    children[i]->blockSignals(wasBlocked[i]);
}

void GlobalQueueInterfaceSettingsWidget::accept(GlobalSearch::OptBase* opt)
{
  opt->description = m_ui->edit_description->text().trimmed();
  opt->host = m_ui->edit_host->text().trimmed();
  opt->rempath = m_ui->edit_rempath->text().trimmed();
  opt->filePath = m_ui->edit_locpath->text().trimmed();
  opt->username = m_ui->edit_username->text().trimmed();
  opt->port = m_ui->spin_port->value();
  opt->setCleanRemoteOnStop(m_ui->cb_cleanRemoteOnStop->isChecked());
  opt->m_logErrorDirs = m_ui->cb_logErrorDirs->isChecked();
  opt->m_cancelJobAfterTime = m_ui->cb_cancelJobAfterTime->isChecked();
  opt->m_hoursForCancelJobAfterTime =
    m_ui->spin_hoursForCancelJobAfterTime->value();
}
}

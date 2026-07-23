/**********************************************************************
  GlobalQueueInterfaceSettingsWidget - set the global QI settings

  Copyright (C) 2018 by Patrick Avery
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/search.h>

#include "globalqueueinterfacesettingswidget.h"
#include "ui_globalqueueinterfacesettingswidget.h"

#include <QCheckBox>
#include <QReadWriteLock>

namespace Search {

GlobalQueueInterfaceSettingsWidget::GlobalQueueInterfaceSettingsWidget(
  QWidget* parent)
  : QWidget(parent)
  , m_ui(new Ui::GlobalQueueInterfaceSettingsWidget)
  , m_runtimeOptionsEditable(true)
{
  m_ui->setupUi(this);

#ifndef ENABLE_LIBSSH
  m_ui->combo_sshMethod->addItem(tr("System SSH (ssh/scp)"), "system");
  m_ui->combo_sshMethod->setToolTip(
    tr("This build uses the system ssh/scp method. The equivalent ssh/scp "
       "commands must already work non-interactively."));
#else
  m_ui->combo_sshMethod->addItem(tr("System SSH (ssh/scp)"), "system");
  m_ui->combo_sshMethod->addItem(tr("libssh"), "libssh");
  m_ui->combo_sshMethod->addItem(tr("Auto"), "auto");
  m_ui->combo_sshMethod->setCurrentIndex(1);
  m_ui->combo_sshMethod->setToolTip(
    tr("System SSH uses ssh/scp commands. libssh uses the built-in SSH "
       "library. Auto tries System SSH first, then libssh."));
#endif

  connect(m_ui->cb_remoteQueue, &QCheckBox::toggled,
          this, [this]() { updateTransportWidgetsEnabled(); });
  updateTransportWidgetsEnabled();
  updateRuntimeOptionWidgetsEnabled();
}

GlobalQueueInterfaceSettingsWidget::~GlobalQueueInterfaceSettingsWidget() =
  default;

void GlobalQueueInterfaceSettingsWidget::updateGUI(Search::SearchBase* opt)
{
  // Block signals for all widgets when we update the GUI
  QList<bool> wasBlocked;
  QList<QWidget*> children = this->findChildren<QWidget*>();
  for (auto& widget : children)
    wasBlocked.append(widget->blockSignals(true));

  m_ui->cb_remoteQueue->setChecked(opt->isRemoteQueue());
  m_ui->edit_host->setText(opt->getHost());
  m_ui->edit_rempath->setText(opt->getRemWorkDir());
  m_ui->edit_username->setText(opt->getUsername());
  m_ui->spin_port->setValue(opt->getPort());
  const int sshIndex = m_ui->combo_sshMethod->findData(opt->sshMethod());
  if (sshIndex >= 0)
    m_ui->combo_sshMethod->setCurrentIndex(sshIndex);
  m_ui->spin_interval->setValue(opt->queueRefreshInterval());
  m_ui->cb_cleanRemoteOnStop->setChecked(opt->cleanRemoteOnStop());

  updateTransportWidgetsEnabled();
  updateRuntimeOptionWidgetsEnabled();

  // Restore the previous states of the widgets
  for (int i = 0; i < children.size(); ++i)
    children[i]->blockSignals(wasBlocked[i]);
}

void GlobalQueueInterfaceSettingsWidget::setRuntimeOptionsEditable(bool editable)
{
  m_runtimeOptionsEditable = editable;
  updateRuntimeOptionWidgetsEnabled();
}

void GlobalQueueInterfaceSettingsWidget::accept(Search::SearchBase* opt)
{
  QWriteLocker runtimeLocker(opt->runtimeSettingsLock());
  if (!opt->isSessionInProgress()) {
    opt->setRemoteQueue(m_ui->cb_remoteQueue->isChecked());
    opt->setHost(m_ui->edit_host->text().trimmed());
    opt->setRemWorkDir(m_ui->edit_rempath->text().trimmed());
    opt->setUsername(m_ui->edit_username->text().trimmed());
    opt->setPort(m_ui->spin_port->value());
    const QString sshMethod = m_ui->combo_sshMethod->currentData().toString();
    if (!sshMethod.isEmpty())
      opt->setSshMethod(sshMethod);
    opt->setCleanRemoteOnStop(m_ui->cb_cleanRemoteOnStop->isChecked());
  }
  opt->setQueueRefreshInterval(m_ui->spin_interval->value());
}

void GlobalQueueInterfaceSettingsWidget::updateRuntimeOptionWidgetsEnabled()
{
  m_ui->spin_interval->setEnabled(m_runtimeOptionsEditable);
}

void GlobalQueueInterfaceSettingsWidget::updateTransportWidgetsEnabled()
{
  const bool remoteQueue = m_ui->cb_remoteQueue->isChecked();

  m_ui->label_sshMethod->setEnabled(false);
  m_ui->combo_sshMethod->setEnabled(false);
#ifdef ENABLE_LIBSSH
  m_ui->label_sshMethod->setEnabled(remoteQueue);
  m_ui->combo_sshMethod->setEnabled(remoteQueue);
#endif

  m_ui->label_18->setEnabled(remoteQueue);
  m_ui->edit_host->setEnabled(remoteQueue);
  m_ui->spin_port->setEnabled(remoteQueue);
  m_ui->label_19->setEnabled(remoteQueue);
  m_ui->edit_username->setEnabled(remoteQueue);
  m_ui->label_20->setEnabled(remoteQueue);
  m_ui->edit_rempath->setEnabled(remoteQueue);
  m_ui->cb_cleanRemoteOnStop->setEnabled(remoteQueue);
}
}

/**********************************************************************
  TabOpt - The optimization-settings tab: pick the queue/optimizer and save or load schemes.

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/gui/tab_opt.h>

#include <xtalopt/gui/dialog.h>
#include <xtalopt/settings.h>
#include <xtalopt/xtalopt.h>

#include <search/optimizer.h>
#include <search/queueinterface.h>
#include <search/queueinterfaces/queueinterfaces.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QReadWriteLock>

using namespace Search;

namespace XtalOpt {

TabOpt::TabOpt(AbstractDialog* parent, XtalOpt* p) : DefaultOptTab(parent, p)
{
  // Fill m_optimizers from the optimizers added in the XtalOpt constructor
  m_optimizers = Optimizer::registeredOptimizers();

  // Set the correct index
  if (m_search->optimizer(0)) {
    int optIndex = m_optimizers.indexOf(m_search->optimizer(0)->getIDString());
    ui_combo_optimizers->setCurrentIndex(optIndex);
  }

  // Fill m_queueInterfaces from the queues added in the XtalOpt constructor
  m_queueInterfaces = QueueInterface::registeredQueueInterfaces();

  // Set the queue interface index
  if (m_search->queueInterface(0)) {
    int qiIndex =
      m_queueInterfaces.indexOf(m_search->queueInterface(0)->getIDString());
    ui_combo_queueInterfaces->setCurrentIndex(qiIndex);
  }

  DefaultOptTab::initialize();

  // The job-cancel setting
  connect(ui_cb_cancelJobAfterTime, &QCheckBox::toggled, this, &TabOpt::updateJobCancel);
  connect(ui_spin_hoursForCancelJob,
          static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
          this, &TabOpt::updateJobCancel);

  populateTemplates();
}

TabOpt::~TabOpt()
{
}

void TabOpt::updateJobCancel()
{
  ui_spin_hoursForCancelJob->setEnabled(ui_cb_cancelJobAfterTime->isChecked());

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);
  const bool enabled = ui_cb_cancelJobAfterTime->isChecked();
  const double hours = ui_spin_hoursForCancelJob->value();
  bool changed = false;
  {
    QWriteLocker runtimeLocker(m_search->runtimeSettingsLock());
    if (xtalopt->cancelJobAfterTime() != enabled) {
      xtalopt->setCancelJobAfterTime(enabled);
      changed = true;
    }
    if (xtalopt->hoursForCancelJobAfterTime() != hours) {
      xtalopt->setHoursForCancelJobAfterTime(hours);
      changed = true;
    }
  }
  if (changed && m_search->isSessionInProgress())
    xtalopt->requestSettingsStateSave();
}

void TabOpt::writeSettings(const QString& filename)
{
  if (filename.isEmpty())
    return;

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  // A scheme file stores the settings from the Optimization tab.
  xtalopt->writeOptScheme(filename);
}

void TabOpt::loadScheme()
{
  if (m_configDialogsReadOnly)
    return;

  QString filename = QFileDialog::getOpenFileName(
    nullptr, tr("Select Optimization Scheme to load..."), QDir::homePath(), "*.scheme;;*.*", 0,
    QFileDialog::DontUseNativeDialog);

  // User canceled
  if (filename.isEmpty())
    return;

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);

  // A scheme file stores the settings from the Optimization tab.
  xtalopt->readOptScheme(filename, false);

  updateGUI();
}

void TabOpt::configureQueueInterface()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);
  Settings::ScalarSnapshot before;
  {
    QReadLocker runtimeLocker(m_search->runtimeSettingsLock());
    before = Settings::captureScalars(*xtalopt);
  }

  DefaultOptTab::configureQueueInterface();

  bool settingsChanged = false;
  {
    QWriteLocker runtimeLocker(m_search->runtimeSettingsLock());
    Settings::validateSettings(*xtalopt, Settings::InvalidSettingAction::KeepPrevious, &before);
    const QStringList keywords = { "queueRefreshInterval" };
    for (const auto& keyword : keywords) {
      if (Settings::scalarValue(*xtalopt, keyword) != before.value(keyword)) {
        settingsChanged = true;
        break;
      }
    }
  }

  if (settingsChanged && m_search->isSessionInProgress())
    xtalopt->requestSettingsStateSave();
}

}

/**********************************************************************
  TabLog - The Log tab: shows program messages as they arrive.

  Copyright (C) 2009-2011 by David Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <xtalopt/gui/tab_log.h>

#include <xtalopt/gui/dialog.h>
#include <xtalopt/xtalopt.h>

#include <QFile>
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include <QReadWriteLock>
#include <QTextStream>

using namespace std;

namespace XtalOpt {

TabLog::TabLog(Search::AbstractDialog* parent, XtalOpt* p)
  : AbstractTab(parent, p)
{
  ui.setupUi(m_tab_widget);

  /*
   * The Log tab shows the Common output messages in the GUI.
   *
   * installGuiInterface() adds an output handler to Common and emits
   * AbstractDialog::newLog with already formatted text. This tab just
   * stores and displays those entries and lets the user save the visible
   * log. It does not touch Qt's global message handler and does not
   * change the CLI/core output.
   */
  connect(m_dialog, &Search::AbstractDialog::newLog, this, &TabLog::newLog);
  connect(ui.push_saveLog, &QPushButton::clicked, this, &TabLog::saveLog);
  ui.cb_verbose->setChecked(m_search->isVerbose());
  connect(ui.cb_verbose, &QCheckBox::toggled, this, &TabLog::updateVerboseOutput);
  ui.cb_debug->setChecked(m_search->isDebugOutput());
  connect(ui.cb_debug, &QCheckBox::toggled, this, &TabLog::updateDebugOutput);

  initialize();
}

TabLog::~TabLog()
{
}

void TabLog::disconnectGUI()
{
  disconnect(m_dialog, 0, this, 0);
}

void TabLog::lockGUI()
{
  ui.cb_verbose->setEnabled(!m_search->isReadOnly());
  ui.cb_debug->setEnabled(!m_search->isReadOnly());
}

void TabLog::updateGUI()
{
  const bool wasBlocked = ui.cb_verbose->blockSignals(true);
  ui.cb_verbose->setChecked(m_search->isVerbose());
  ui.cb_verbose->blockSignals(wasBlocked);

  const bool wasBlockedDebug = ui.cb_debug->blockSignals(true);
  ui.cb_debug->setChecked(m_search->isDebugOutput());
  ui.cb_debug->blockSignals(wasBlockedDebug);
}

void TabLog::newLog(const QString& info)
{
  ui.text_log->appendPlainText(info);
}

void TabLog::saveLog()
{
  const QString filename = QFileDialog::getSaveFileName(m_dialog, tr("Save Log"),
                                 QDir::home().filePath("outlog_xtalopt.txt"),
                                 tr("Log files (*.log *.txt);;All files (*.*)"),
                                 nullptr, QFileDialog::DontUseNativeDialog);
  if (filename.isEmpty())
    return;

  QFile file(filename);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QMessageBox::warning(m_dialog, tr("Save Log"), tr("Cannot write log file:\n%1").arg(filename));
    return;
  }

  QTextStream stream(&file);
  stream << ui.text_log->toPlainText();
}

void TabLog::updateVerboseOutput()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);
  const bool value = ui.cb_verbose->isChecked();
  bool changed = false;
  {
    QWriteLocker runtimeLocker(m_search->runtimeSettingsLock());
    changed = xtalopt->isVerbose() != value;
    if (changed)
      xtalopt->setVerbose(value);
  }
  if (changed && m_search->isSessionInProgress())
    xtalopt->requestSettingsStateSave();
}

void TabLog::updateDebugOutput()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_search);
  const bool value = ui.cb_debug->isChecked();
  bool changed = false;
  {
    QWriteLocker runtimeLocker(m_search->runtimeSettingsLock());
    changed = xtalopt->isDebugOutput() != value;
    if (changed)
      xtalopt->setDebugOutput(value);
  }
  if (changed && m_search->isSessionInProgress())
    xtalopt->requestSettingsStateSave();
}
}

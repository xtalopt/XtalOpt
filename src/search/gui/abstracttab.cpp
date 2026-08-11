/**********************************************************************
  AbstractTab - Basic Search tab functionality

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/gui/abstracttab.h>

#include <search/search.h>
#include <search/structure.h>
#include <search/gui/abstractdialog.h>

#include <QApplication>
#include <QMessageBox>

#include <QThread>

namespace Search {

AbstractTab::AbstractTab(AbstractDialog* parent, SearchBase* p)
  : QObject(parent), m_dialog(parent), m_search(p), m_isInitialized(false)
{
  m_tab_widget = new QWidget;
}

void AbstractTab::initialize()
{
  // dialog connections
  connect(m_dialog, &AbstractDialog::tabsUpdateGUI,    this, &AbstractTab::updateGUI);
  connect(m_dialog, &AbstractDialog::tabsDisconnectGUI, this, &AbstractTab::disconnectGUI);
  connect(m_dialog, &AbstractDialog::tabsLockGUI,       this, &AbstractTab::lockGUI);
  connect(this, &AbstractTab::startingBackgroundProcessing,
          this, &AbstractTab::setBusyCursor, Qt::QueuedConnection);
  connect(this, &AbstractTab::finishedBackgroundProcessing,
          this, &AbstractTab::clearBusyCursor, Qt::QueuedConnection);

  m_isInitialized = true;
  emit initialized();
}

AbstractTab::~AbstractTab()
{
}

void AbstractTab::setBusyCursor()
{
  Q_ASSERT_X(QThread::currentThread() == qApp->thread(), Q_FUNC_INFO,
             "This function cannot be called from an background thread. "
             "Emit AbstractTab::startingBackgroundProcessing instead.");
  qApp->setOverrideCursor(Qt::WaitCursor);
}

void AbstractTab::clearBusyCursor()
{
  Q_ASSERT_X(QThread::currentThread() == qApp->thread(), Q_FUNC_INFO,
             "This function cannot be called from an background thread. "
             "Emit AbstractTab::finishedBackgroundProcessing instead.");
  qApp->restoreOverrideCursor();
}

void AbstractTab::errorPromptWindow(const QString& instr)
{
  QMessageBox::warning(m_dialog, tr("XtalOpt"), instr);
}
}

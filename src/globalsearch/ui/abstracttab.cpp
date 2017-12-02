/**********************************************************************
  AbstractTab -- Basic GlobalSearch tab functionality

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <globalsearch/ui/abstracttab.h>

#include <globalsearch/optbase.h>
#include <globalsearch/ui/abstractdialog.h>

#include <QApplication>

#include <QThread>

namespace GlobalSearch {

AbstractTab::AbstractTab(AbstractDialog* parent, OptBase* p)
  : QObject(parent), m_dialog(parent), m_opt(p), m_isInitialized(false)
{
  m_tab_widget = new QWidget;
}

void AbstractTab::initialize()
{
  // dialog connections
  connect(m_dialog, SIGNAL(tabsReadSettingsDirect(const QString&)), this,
          SLOT(readSettings(const QString&)), Qt::DirectConnection);
  connect(m_dialog, SIGNAL(tabsReadSettingsBlockingQueued(const QString&)),
          this, SLOT(readSettings(const QString&)),
          Qt::BlockingQueuedConnection);
  connect(m_dialog, SIGNAL(tabsWriteSettingsDirect(const QString&)), this,
          SLOT(writeSettings(const QString&)), Qt::DirectConnection);
  connect(m_dialog, SIGNAL(tabsWriteSettingsBlockingQueued(const QString&)),
          this, SLOT(writeSettings(const QString&)),
          Qt::BlockingQueuedConnection);
  connect(m_dialog, SIGNAL(tabsUpdateGUI()), this, SLOT(updateGUI()));
  connect(m_dialog, SIGNAL(tabsDisconnectGUI()), this, SLOT(disconnectGUI()));
  connect(m_dialog, SIGNAL(tabsLockGUI()), this, SLOT(lockGUI()));
  connect(this, SIGNAL(moleculeChanged(GlobalSearch::Structure*)), m_dialog,
          SIGNAL(moleculeChanged(GlobalSearch::Structure*)));
  connect(this, SIGNAL(startingBackgroundProcessing()), this,
          SLOT(setBusyCursor()), Qt::QueuedConnection);
  connect(this, SIGNAL(finishedBackgroundProcessing()), this,
          SLOT(clearBusyCursor()), Qt::QueuedConnection);

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
}

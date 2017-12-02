/**********************************************************************
  TabLog - A simple logging widget

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <gapc/ui/tab_log.h>

#include <gapc/gapc.h>
#include <gapc/ui/dialog.h>

#include <QDateTime>
#include <QSettings>

using namespace std;

namespace GAPC {

TabLog::TabLog(GAPCDialog* parent, OptGAPC* p) : AbstractTab(parent, p)
{
  ui.setupUi(m_tab_widget);

  connect(m_dialog, SIGNAL(newLog(QString)), this, SLOT(newLog(QString)));

  initialize();
}

TabLog::~TabLog()
{
}

void TabLog::disconnectGUI()
{
  connect(m_dialog, 0, this, 0);
}

void TabLog::newLog(const QString& info)
{
  QString entry;
  QString timestamp =
    QDateTime::currentDateTime().toString("MM/dd/yyyy hh:mm:ss (zzz) -- ");

  entry = timestamp + info;
  ui.list_list->addItem(entry);
}
}

/**********************************************************************
  RandomDock -- A tool for analysing a matrix-substrate docking problem

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <randomdock/ui/tab_log.h>

#include <globalsearch/macros.h>
#include <randomdock/randomdock.h>
#include <randomdock/ui/dialog.h>

#include <QDateTime>
#include <QSettings>

using namespace std;
using namespace Avogadro;

namespace RandomDock {

TabLog::TabLog(RandomDockDialog* dialog, RandomDock* opt)
  : AbstractTab(dialog, opt)
{
  ui.setupUi(m_tab_widget);

  // Log
  connect(dialog, SIGNAL(newLog(QString)), this, SLOT(newLog(QString)));

  initialize();
}

TabLog::~TabLog()
{
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

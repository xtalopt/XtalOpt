/**********************************************************************
  XtalOpt - Tools for advanced crystal optimization

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef TAB_LOG_H
#define TAB_LOG_H

#include <globalsearch/ui/abstracttab.h>

#include "ui_tab_log.h"

namespace GlobalSearch {
class AbstractDialog;
}

namespace XtalOpt {
class XtalOptDialog;
class XtalOpt;

class TabLog : public GlobalSearch::AbstractTab
{
  Q_OBJECT

public:
  explicit TabLog(GlobalSearch::AbstractDialog* parent, XtalOpt* p);
  virtual ~TabLog() override;

public slots:
  void disconnectGUI() override;
  void newLog(const QString& info);

signals:

private:
  Ui::Tab_Log ui;
};
}

#endif

/**********************************************************************
  TabLog - The Log tab: shows program messages as they arrive.

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

#include <search/gui/abstracttab.h>

#include "ui_tab_log.h"

namespace Search {
class AbstractDialog;
}

namespace XtalOpt {
class XtalOptDialog;
class XtalOpt;

/**
 * The Log tab: shows program messages as they arrive.
 */
class TabLog : public Search::AbstractTab
{
  Q_OBJECT

public:
  explicit TabLog(Search::AbstractDialog* parent, XtalOpt* p);
  virtual ~TabLog() override;

public slots:
  void disconnectGUI() override;
  void lockGUI() override;
  void updateGUI() override;
  void newLog(const QString& info);
  void saveLog();
  void updateVerboseOutput();
  void updateDebugOutput();

signals:

private:
  Ui::Tab_Log ui;
};
}

#endif

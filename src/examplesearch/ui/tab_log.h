/**********************************************************************
  ExampleSearch -- A tool for analysing a matrix-substrate docking problem

  Copyright (C) 2012 by David C. Lonie

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

namespace ExampleSearch {
class ExampleSearchDialog;
class ExampleSearch;

class TabLog : public GlobalSearch::AbstractTab
{
  Q_OBJECT

public:
  explicit TabLog(ExampleSearchDialog* dialog, ExampleSearch* opt);
  virtual ~TabLog();

public slots:
  void newLog(const QString& info);

signals:

private:
  Ui::Tab_Log ui;
};
}

#endif

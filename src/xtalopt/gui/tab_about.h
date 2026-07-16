/**********************************************************************
  TabAbout - The About tab: program version and credits.

  Copyright (C) 2025 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef TAB_ABOUT_H
#define TAB_ABOUT_H

#include <search/gui/abstracttab.h>

#include "ui_tab_about.h"

namespace Search {
class AbstractDialog;
}

namespace XtalOpt {
class XtalOptDialog;
class XtalOpt;

/**
 * The About tab: program version and credits.
 */
class TabAbout : public Search::AbstractTab
{
  Q_OBJECT

public:
  explicit TabAbout(Search::AbstractDialog* parent, XtalOpt* p);
  virtual ~TabAbout() override;

public slots:
  void disconnectGUI() override;

signals:

private:
  Ui::Tab_About ui;
};
}

#endif

/**********************************************************************
  XtalOpt - Tools for advanced crystal optimization

  Copyright (C) 2025 by Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef TAB_ABOUT_H
#define TAB_ABOUT_H

#include <globalsearch/ui/abstracttab.h>

#include "ui_tab_about.h"

namespace GlobalSearch {
class AbstractDialog;
}

namespace XtalOpt {
class XtalOptDialog;
class XtalOpt;

class TabAbout : public GlobalSearch::AbstractTab
{
  Q_OBJECT

public:
  explicit TabAbout(GlobalSearch::AbstractDialog* parent, XtalOpt* p);
  virtual ~TabAbout() override;

public slots:
  void disconnectGUI() override;

signals:

private:
  Ui::Tab_About ui;
};
}

#endif

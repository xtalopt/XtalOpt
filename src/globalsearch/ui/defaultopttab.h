/**********************************************************************
  DefaultOptTab - Simple implementation of AbstractOptTab

  Copyright (C) 2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef DEFAULTOPTTAB_H
#define DEFAULTOPTTAB_H

#include <globalsearch/ui/abstractopttab.h>

namespace Ui {
class DefaultOptTab;
}

namespace GlobalSearch {
class AbstractDialog;
class SearchBase;

/**
 * @class DefaultOptTab defaultopttab.h <globalsearch/defaultopttab.h>
 *
 * @brief Default implementation of a template editor tab.
 *
 * @author David C. Lonie
 */
class DefaultOptTab : public GlobalSearch::AbstractOptTab
{
  Q_OBJECT

public:
  /**
   * Constructor
   *
   * @param dialog Parent AbstractDialog
   * @param opt Associated SearchBase
   */
  explicit DefaultOptTab(AbstractDialog* dialog, SearchBase* opt);

  /**
   * Destructor
   */
  virtual ~DefaultOptTab() override;

protected slots:
  /**
   * Set up the GUI pointers and call AbstractOptTab::initialize()
   */
  virtual void initialize() override;

private:
  Ui::DefaultOptTab* ui;
};
}

#endif

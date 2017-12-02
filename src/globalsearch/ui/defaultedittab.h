/**********************************************************************
  DefaultEditTab - Simple implementation of AbstractEditTab

  Copyright (C) 2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef DEFAULTEDITTAB_H
#define DEFAULTEDITTAB_H

#include <globalsearch/ui/abstractedittab.h>

namespace Ui {
class DefaultEditTab;
}

namespace GlobalSearch {
class AbstractDialog;
class OptBase;

/**
 * @class DefaultEditTab defaultedittab.h <globalsearch/defaultedittab.h>
 *
 * @brief Default implementation of a template editor tab.
 *
 * @author David C. Lonie
 */
class DefaultEditTab : public GlobalSearch::AbstractEditTab
{
  Q_OBJECT

public:
  /**
   * Constructor
   *
   * @param dialog Parent AbstractDialog
   * @param opt Associated OptBase
   */
  explicit DefaultEditTab(AbstractDialog* dialog, OptBase* opt);

  /**
   * Destructor
   */
  virtual ~DefaultEditTab() override;

protected slots:
  /**
   * Set up the GUI pointers and call AbstractEditTab::initialize()
   */
  virtual void initialize() override;

private:
  Ui::DefaultEditTab* ui;
};
}

#endif

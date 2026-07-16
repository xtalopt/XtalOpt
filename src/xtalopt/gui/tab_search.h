/**********************************************************************
  TabSearch - The search-settings tab: edit search parameters and seed structures.

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef TAB_SEARCH_H
#define TAB_SEARCH_H

#include <search/gui/abstracttab.h>

#include "ui_tab_search.h"

namespace Search {
class AbstractDialog;
}

namespace XtalOpt {
class XtalOpt;

/**
 * The search-settings tab: edit search parameters and seed
 * structures.
 */
class TabSearch : public Search::AbstractTab
{
  Q_OBJECT

public:
  explicit TabSearch(Search::AbstractDialog* parent, XtalOpt* p);
  virtual ~TabSearch() override;

public slots:
  void lockGUI() override;
  void updateGUI() override;
  bool updateOptTypeInfo();
  void updateOptimizationInfo();
  void addSeed(QListWidgetItem* item = nullptr);
  void removeSeed();
  void showSeeds();
  void updateSeeds();

signals:

private:
  Ui::Tab_Opt ui;
};
}

#endif

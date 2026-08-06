/**********************************************************************
  TabOpt - The optimization-settings tab: pick the queue/optimizer and save or load schemes.

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef TAB_OPT_H
#define TAB_OPT_H

#include <search/gui/defaultopttab.h>

namespace Search {
class AbstractDialog;
}

namespace XtalOpt {
class XtalOpt;
class XtalOptDialog;

/**
 * The optimization-settings tab: pick the queues/optimizers
 * and save or load schemes.
 */
class TabOpt : public Search::DefaultOptTab
{
  Q_OBJECT

public:
  explicit TabOpt(Search::AbstractDialog* parent, XtalOpt* p);
  virtual ~TabOpt() override;

public slots:
  void writeSchemeFile(const QString& filename) override;
  void loadScheme() override;
  void configureQueueInterface() override;
  void updateJobCancel();
};
}

#endif

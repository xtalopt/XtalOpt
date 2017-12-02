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

#ifndef TAB_EDIT_H
#define TAB_EDIT_H

#include <globalsearch/ui/defaultedittab.h>

class QListWidgetItem;

namespace GlobalSearch {
class AbstractDialog;
}

namespace XtalOpt {
class XtalOpt;
class XtalOptDialog;

class TabEdit : public GlobalSearch::DefaultEditTab
{
  Q_OBJECT

public:
  explicit TabEdit(GlobalSearch::AbstractDialog* parent, XtalOpt* p);
  virtual ~TabEdit() override;

public slots:
  void readSettings(const QString& filename = "") override;
  void writeSettings(const QString& filename = "") override;
  void loadScheme() override;
  void updateEditWidget() override;
  void appendOptStep() override;
  void removeCurrentOptStep() override;

protected slots:
  // Returns false if user cancels
  bool generateVASP_POTCAR_info();
  bool generateSIESTA_PSF_info();
  void changePOTCAR(QListWidgetItem* item);
  void changePSF(QListWidgetItem* item);
};
}

#endif

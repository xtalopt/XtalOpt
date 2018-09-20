/**********************************************************************
  GlobalQueueInterfaceSettingsWidget - set the global QI settings

  Copyright (C) 2018 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef GLOBALSEARCH_GLOBALQUEUEINTERFACESETTINGSWIDGET_H
#define GLOBALSEARCH_GLOBALQUEUEINTERFACESETTINGSWIDGET_H

#include <memory>

#include <QWidget>

namespace Ui {
class GlobalQueueInterfaceSettingsWidget;
}

namespace GlobalSearch {

class OptBase;

class GlobalQueueInterfaceSettingsWidget : public QWidget
{
  Q_OBJECT

public:
  explicit GlobalQueueInterfaceSettingsWidget(QWidget* parent = nullptr);
  ~GlobalQueueInterfaceSettingsWidget();

  void updateGUI(GlobalSearch::OptBase* opt);
  void accept(GlobalSearch::OptBase* opt);

private:
  std::unique_ptr<Ui::GlobalQueueInterfaceSettingsWidget> m_ui;
};
}

#endif // GLOBALSEARCH_GLOBALQUEUEINTERFACESETTINGSWIDGET_H

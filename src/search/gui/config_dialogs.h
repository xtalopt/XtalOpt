/**********************************************************************
  config_dialogs - Handler for creating optimizers and queue interfaces

  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef SEARCH_GUI_CONFIG_DIALOGS_H
#define SEARCH_GUI_CONFIG_DIALOGS_H

class QDialog;
class QWidget;

namespace Search {

class Optimizer;
class QueueInterface;
class SearchBase;

bool hasOptimizerEditor(const Optimizer* optimizer, const QueueInterface* queueInterface);
QDialog* createOptimizerEditor(QWidget* parent, SearchBase* search, Optimizer* optimizer,
                               QueueInterface* queueInterface);

bool hasQueueInterfaceEditor(const QueueInterface* queueInterface);
QDialog* createQueueInterfaceEditor(QWidget* parent, SearchBase* search,
                                    QueueInterface* queueInterface);

} // namespace Search

#endif

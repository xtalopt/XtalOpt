/**********************************************************************
  LoadLevelerConfigDialog - Setup for LoadLeveler queues

  Copyright (C) 2012 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef LOADLEVELERCONFIGDIALOG_H
#define LOADLEVELERCONFIGDIALOG_H

// Doxygen should ignore this file:
/// @cond

#include <QDialog>

namespace Ui {
/**
 * Settings dialog for the LoadLeveler queue.
 */
class LoadLevelerConfigDialog;
}

namespace Search {
class SearchBase;
class LoadLevelerQueueInterface;

class LoadLevelerConfigDialog : public QDialog
{
  Q_OBJECT

public:
  explicit LoadLevelerConfigDialog(QWidget* parent, SearchBase* o, LoadLevelerQueueInterface* p);
  virtual ~LoadLevelerConfigDialog() override;

public slots:
  void updateGUI();

protected slots:
  void accept() override;
  void reject() override;

protected:
  SearchBase* m_search;
  LoadLevelerQueueInterface* m_ll;

private:
  Ui::LoadLevelerConfigDialog* ui;
};
}

/// @endcond
#endif // LOADLEVELERCONFIGDIALOG_H

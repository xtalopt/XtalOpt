/**********************************************************************
  LoadLevelerConfigDialog

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

#ifdef ENABLE_SSH

// Doxygen should ignore this file:
/// @cond

#include <QDialog>

namespace Ui {
class LoadLevelerConfigDialog;
}

namespace GlobalSearch {
class AbstractDialog;
class OptBase;
class LoadLevelerQueueInterface;

class LoadLevelerConfigDialog : public QDialog
{
  Q_OBJECT

public:
  explicit LoadLevelerConfigDialog(AbstractDialog* parent, OptBase* o,
                                   LoadLevelerQueueInterface* p);
  virtual ~LoadLevelerConfigDialog() override;

public slots:
  void updateGUI();

protected slots:
  void accept() override;
  void reject() override;

protected:
  OptBase* m_opt;
  LoadLevelerQueueInterface* m_ll;

private:
  Ui::LoadLevelerConfigDialog* ui;
};
}

/// @endcond
#endif // ENABLE_SSH
#endif // LOADLEVELERCONFIGDIALOG_H

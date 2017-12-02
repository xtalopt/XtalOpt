/**********************************************************************
  LocalQueueInterfaceConfigDialog

  Copyright (C) 2010 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

// Don't document this:
/// @cond
#ifndef LOCALQUEUEINTERFACECONFIGDIALOG_H
#define LOCALQUEUEINTERFACECONFIGDIALOG_H

#include <QDialog>

class QCheckBox;
class QDialogButtonBox;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QSpacerItem;
class QVBoxLayout;

namespace GlobalSearch {
class AbstractDialog;
class OptBase;
class LocalQueueInterface;

// Basic input dialog for local QueueInterfaces
class LocalQueueInterfaceConfigDialog : public QDialog
{
  Q_OBJECT
public:
  LocalQueueInterfaceConfigDialog(AbstractDialog* parent, OptBase* opt,
                                  LocalQueueInterface* qi);
  ~LocalQueueInterfaceConfigDialog();

public slots:
  void accept() override;
  void reject() override;
  void updateGUI();

protected:
  OptBase* m_opt;
  LocalQueueInterface* m_queueInterface;

  QCheckBox* m_cb_logErrorDirs;
  QDialogButtonBox* m_bbox;
  QHBoxLayout *m_top_label_layout, *m_desc_layout, *m_workdir_layout;
  QLabel *m_label0, *m_label1, *m_label2;
  QLineEdit *m_edit_workdir, *m_edit_description;
  QSpacerItem* m_spacer;
  QVBoxLayout* m_vlayout;
};

} // end namespace GlobalSearch

#endif
/// @endcond

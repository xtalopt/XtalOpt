/**********************************************************************
  LocalQueueInterfaceConfigDialog

  Copyright (C) 2010 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

// Don't document this:
/// @cond
#ifndef LOCALQUEUEINTERFACECONFIGDIALOG_H
#define LOCALQUEUEINTERFACECONFIGDIALOG_H

#include <QtWidgets/QDialog>

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
    Q_OBJECT;
  public:
    LocalQueueInterfaceConfigDialog(AbstractDialog *parent,
                                    OptBase *opt,
                                    LocalQueueInterface *qi);
    ~LocalQueueInterfaceConfigDialog();

  public slots:
    void accept() override;
    void reject() override;
    void updateGUI();

  protected:
    OptBase *m_opt;
    LocalQueueInterface *m_queueInterface;

    QCheckBox *m_cb_logErrorDirs;
    QDialogButtonBox *m_bbox;
    QHBoxLayout *m_desc_layout, *m_workdir_layout;
    QLabel *m_label1, *m_label2;
    QLineEdit *m_edit_workdir, *m_edit_description;
    QSpacerItem *m_spacer;
    QVBoxLayout *m_vlayout;
  };

} // end namespace GlobalSearch

#endif
/// @endcond

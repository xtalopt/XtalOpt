/**********************************************************************
  InternalQueueInterfaceConfigDialog

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
#ifndef INTERNALQUEUEINTERFACECONFIGDIALOG_H
#define INTERNALQUEUEINTERFACECONFIGDIALOG_H

#include <QtGui/QDialog>

class QLineEdit;

namespace GlobalSearch {
  class AbstractDialog;
  class OptBase;
  class InternalQueueInterface;

  // Basic input dialog for Internal QueueInterfaces
  class InternalQueueInterfaceConfigDialog : public QDialog
  {
    Q_OBJECT;
  public:
    InternalQueueInterfaceConfigDialog(AbstractDialog *parent,
                                    OptBase *opt,
                                    InternalQueueInterface *qi);

  public slots:
    void accept();
    void reject();
    void updateGUI();

  protected:
    OptBase *m_opt;
    InternalQueueInterface *m_queueInterface;
    QLineEdit *m_edit_workdir;
    QLineEdit *m_edit_description;

  };

} // end namespace GlobalSearch

#endif
/// @endcond
